#include "chess.h"
#include "data.h"
#include "epdglue.h"
/* modified 04/30/14 */
/*
 *******************************************************************************
 *                                                                             *
 *   Thread() is the driver for the threaded parallel search in Crafty.  The   *
 *   basic idea is that whenever we notice that one (or more) threads are in   *
 *   their idle loop, we drop into thread, from Search(), and begin a new      *
 *   parallel search from this node.  This is simply a problem of copying the  *
 *   search state space for each thread working at this node, then sending     *
 *   everyone off to SearchParallel() to search this node in parallel.         *
 *                                                                             *
 *   There are a number of settable options via the command-line or .craftyrc  *
 *   initialization file.  Here's a concise explanation for each option and an *
 *   occasional suggestion for testing/tuning.                                 *
 *                                                                             *
 *   smp_max_threads (command = smpmt=n) sets the total number of allowable    *
 *      threads for the search.  The default is one (1) as Crafty does not     *
 *      assume it should use all available resources.  For optimal performance *
 *      this should be set to the number of physical cores your machine has,   *
 *      which does NOT include hyperthreaded cores.                            *
 *                                                                             *
 *   smp_split_group (command = smpgroup=n) sets the maximum number of threads *
 *      at any single split point, with the exception of split points where    *
 *      ply <= 4 where ALL threads are allowed to split together ignoring this *
 *      limit.                                                                 *
 *                                                                             *
 *   smp_min_split_depth (command = smpmin=n) avoids splitting when remaining  *
 *      depth < n.  This is used to balance splitting overhead cost against    *
 *      the speed gain the parallel search produes.  The default is currently  *
 *      5 (which could change with future generations of Intel hardware) but   *
 *      values between 4 and 8 will work.  Larger values allow somewhat fewer  *
 *      splits, which reduces overhead, but it also increases the percentage   *
 *      of the time where a thread is waiting on work.                         *
 *                                                                             *
 *   smp_split_nodes (command = smpnodes=n) will not allow a thread to split   *
 *      until it has searched at least N nodes, to prevent excessive splitting *
 *      out near the tips in endgames.  The default is 2000.  Larger values    *
 *      will reduce splitting overhead but will increase the amoutn of time    *
 *      a thread is waiting on work.                                           *
 *                                                                             *
 *   smp_split_at_root (command=smproot=0 or 1) enables (1) or disables (0)    *
 *      splitting the tree at the root.  This defaults to 1 which produces the *
 *      best performance by a signficiant margin.  But it can be disabled if   *
 *      you are playing with code changes.                                     *
 *                                                                             *
 *   The best way to tune any of these parameters is to run SEVERAL test cases *
 *   (positions) with max threads set.  Run each set of positions several      *
 *   times with each parameter change you want to try (do them ONE at a time   *
 *   for obvious reasons).  Pick the value with the smallest overall search    *
 *   time.  The more cores you use, the more times you should run each test,   *
 *   since parallel search is highly non-deterministic and you need several    *
 *   runs to get a reasonable average.                                         *
 *                                                                             *
 *   A few basic "rules of the road" for anyone interested in changing or      *
 *   adding to any of this code.                                               *
 *                                                                             *
 *   1.  If, at any time, you want to modify your private split block, no lock *
 *       is required.                                                          *
 *                                                                             *
 *   2.  If, at any time, you want to modify another split block, such as the  *
 *       parent split block shared move list, you must acquire the lock in the *
 *       split block first.  IE (tree->parent->lock to lock the parent split   *
 *       block during NextMove() and such).  Note that this ONLY applies to    *
 *       search-related data, NOT the SMP data within the split block (parent  *
 *       pointer, sibling pointers, number of processors working here, etc).   *
 *       Modifying those falls under the next lock below.                      *
 *                                                                             *
 *   3.  If you want to modify any SMP-related data, such as allocating a      *
 *       split block, or telling sibling processes to stop, etc, then you must *
 *       acquire the global "smp_lock" lock first.  This prevents any sort of  *
 *       race condition that could corrupt the split block tree organization.  *
 *       This applies to ANY smp-related variable from the simple smp_idle     *
 *       variable through the various sibling chains and such.                 *
 *                                                                             *
 *   4.  If you want to do any sort of I/O operation, you must acquire the     *
 *       "lock_io" lock first.  Since threads share descriptors, there are     *
 *       lots of potential race conditions, from the simple tty-output getting *
 *       interlaced from different threads, to outright data corruption in the *
 *       book or log files.                                                    *
 *                                                                             *
 *   5.  There is a specific "lock_split" that is used to prevent a lot of     *
 *       queued waiting to do splits, but it is unlikely that lock would ever  *
 *       be used anywhere other than where it is currently accessed in         *
 *       Search() at the parallel split initiation code.                       *
 *                                                                             *
 *   6.  If you want to alter the root move list, you must first acquire       *
 *       lock_root, since the root move list is shared and multiple threads    *
 *       can attempt to modify it at the same time.  Overlooking this can      *
 *       result in a corrupted root move list with one or more moves missing   *
 *       or duplicated.                                                        *
 *                                                                             *
 *   Some of the bugs caused by failing to acquire the correct lock will only  *
 *   occur infrequently, and they are extremely difficult to find.  Some only  *
 *   show up in a public game where everyone is watching, to cause maximum     *
 *   embarassment and causes the program to do something extremely stupid.     *
 *                                                                             *
 *******************************************************************************
 */
int Thread(TREE * RESTRICT tree) {
  int proc, tsplit = 0, nblocks = 0;

/*
 ************************************************************
 *                                                          *
 *  First, we make sure that there are idle threads. It is  *
 *  possible that we get here after they have all been      *
 *  claimed by another thread.  In that case we return.     *
 *                                                          *
 ************************************************************
 */
  Lock(lock_smp);
  for (proc = 0; proc < smp_max_threads && !thread[proc].idle; proc++);
  if (proc == smp_max_threads) {
    smp_split = 0;
    Unlock(lock_smp);
    return 0;
  }
/*
 ************************************************************
 *                                                          *
 *  Now we start copying the current "state" from this      *
 *  thread into new thread blocks so that the threads can   *
 *  search this position without interfering with each      *
 *  other.  As we copy, we link those blocks as siblings of *
 *  the parent node, and point each sibling back to the     *
 *  parent so we can unwind this confusion as the threads   *
 *  complete their work.                                    *
 *                                                          *
 *  CopyToChild() allocates a split block optimally (see    *
 *  that code for an explanation) and then copies anything  *
 *  necessary for the parallel search.                      *
 *                                                          *
 *  Special case 1:  In the loop to allocate a split block  *
 *  and copy current data to the new child thread, we skip  *
 *  over the current splitting thread's data.  After the    *
 *  loop we then explicitly allocate a block for this       *
 *  thread so that it will be included in the thread group  *
 *  (remember that this thread is the one that has to back  *
 *  up through the split block, so it will always call      *
 *  ThreadWait() telling it to do so.  There was a peculiar *
 *  condition that caused an inefficiency.  If we are at    *
 *  a depth where we honor the smp_split_group limit, it is *
 *  possible that on occasion, there could be so many idle  *
 *  processors that we would not include ourself.  For      *
 *  example, suppose we are thread 6, and when we enter     *
 *  Thread() to do the split, threads 0-5 are idle, and     *
 *  smp_split_group = 6.  We would pick up the first 6 idle *
 *  threads and sic them on this split point, but we would  *
 *  fail to include ourself.  We would hit ThreadWait()     *
 *  with no work to do, and then split somewhere else.      *
 *  Unfortunately, THIS thread is the one that needs to     *
 *  wait for all the other threads to complete so that it   *
 *  can back up to the previous ply.  But if it is busy on  *
 *  another split point, it won't be able to return to this *
 *  point until it is idle.  Which means progress at this   *
 *  split point is held up.  We now forcefully include the  *
 *  current thread, and only include smp_split_group - 1    *
 *  other threads to work here with this thread.            *
 *                                                          *
 *  Special case 2:  it is possible that there are no split *
 *  blocks available.  In that case, CopyToChild() will     *
 *  terminate Crafty with a log-file error message telling  *
 *  the user to re-compile with a larger number of split    *
 *  blocks (the probability of this actually happening is   *
 *  incredibly small, but it is non-zero.  We use a VERY    *
 *  liberal estimate for the number of split blocks to      *
 *  allocate to avoid this.)                                *
 *                                                          *
 ************************************************************
 */
  thread[tree->thread_id].tree = 0;
  tree->nprocs = 0;
  for (proc = 0; proc < smp_max_threads; proc++) {
    tree->siblings[proc] = 0;
    if (thread[proc].idle) {
      CopyToChild(tree, proc);
      nblocks++;
      if (nblocks >= smp_split_group && tree->ply > tree->depth / 2)
        break;
    }
  }
  CopyToChild(tree, tree->thread_id);
  nblocks++;
/*
 ************************************************************
 *                                                          *
 *  Everything is set.  Now we can stuff the address of the *
 *  thread blocks into thread[i].tree so that those idle    *
 *  threads can begin the parallel search.  We also flag    *
 *  them as "not idle" so that a split immediately after we *
 *  exit won't try to pick them up again, breaking things.  *
 *                                                          *
 ************************************************************
 */
  parallel_splits++;
  for (proc = 0; proc < smp_max_threads; proc++)
    if (tree->siblings[proc]) {
      thread[proc].tree = tree->siblings[proc];
      thread[proc].idle = 0;
    }
/*
 ************************************************************
 *                                                          *
 *  One final test before leaving.  We test all threads to  *
 *  determine if any are still idle (most likely due to the *
 *  smp_split_group limit, but a thread could go idle after *
 *  our loop to pick up idle threads).  If so we set split  *
 *  back to one (actually a temporary split variable since  *
 *  we don't want to remove the split=-1 state until just   *
 *  before we exit to avoid any races.)                     *
 *                                                          *
 ************************************************************
 */
  for (proc = 0; proc < smp_max_threads; proc++)
    if (thread[proc].idle)
      tsplit = 1;
/*
 ************************************************************
 *                                                          *
 *  After the threads are kicked off to start the parallel  *
 *  search using the idle threads, this thread has to be    *
 *  inserted as well.  However, since it is possible that   *
 *  this thread may finish before any or all of the other   *
 *  parallel threads, this thread is sent to ThreadWait()   *
 *  which will immediately send it to SearchParallel() like *
 *  the other threads.  Going to ThreadWait() allows this   *
 *  thread to join others if it runs out of work to do.  We *
 *  do pass ThreadWait() the address of the parent thread   *
 *  block, so that if this thread becomes idle, and this    *
 *  thread block shows no threads are still busy, then this *
 *  thread can return to here and then back up into the     *
 *  previous ply as it should.  Note that no other thread   *
 *  can back up to the previous ply since their recursive   *
 *  call stacks are not set for that.                       *
 *                                                          *
 ************************************************************
 */
  smp_split = tsplit;
  Unlock(lock_smp);
  ThreadWait(tree->thread_id, tree);
  return 1;
}

/* modified 04/30/14 */
/*
 *******************************************************************************
 *                                                                             *
 *   CopyToChild() is used to copy data from a parent thread to a particular   *
 *   child thread.  This only copies the appropriate parts of the TREE         *
 *   structure to avoid burning memory bandwidth by copying everything.        *
 *                                                                             *
 *******************************************************************************
 */
void CopyToChild(TREE * RESTRICT parent, int thread) {
  int i, j, max, ply = parent->ply;
  TREE *child;
  static int warnings = 0;
  int first = thread * MAX_BLOCKS_PER_CPU + 1;
  int last = first + MAX_BLOCKS_PER_CPU;
  int maxb = smp_max_threads * MAX_BLOCKS_PER_CPU + 1;

/*
 ************************************************************
 *                                                          *
 *  One NUMA-related trick is that we first try to allocate *
 *  a split block in the thread's local memory.  Each       *
 *  thread has a group of split blocks that were first      *
 *  touched by the correct CPU so that the split blocks     *
 *  page faulted into local memory for that specific        *
 *  processor.  If we can't find an optimal-placed block,   *
 *  the second pass will find the first available block.    *
 *  If none can be found, we return as we can not split     *
 *  until other split blocks are freed up.                  *
 *                                                          *
 ************************************************************
 */
  for (i = first; i < last && block[i]->used; i++);
  if (i >= last) {
    if (++warnings < 6)
      Print(128,
          "WARNING.  optimal SMP block cannot be allocated, thread %d\n",
          thread);
    for (i = 1; i < maxb && block[i]->used; i++);
    if (i >= maxb) {
      Print(128, "ERROR.  no SMP block can be allocated\n");
      exit(1);
    }
  }
  max = 0;
  for (j = 1; j < maxb; j++)
    if (block[j]->used)
      max++;
  max_split_blocks = Max(max_split_blocks, max);
/*
 ************************************************************
 *                                                          *
 *  We have allocated a split block.  Now we copy the tree  *
 *  search state from the parent block to the child in      *
 *  preparation for starting the parallel search.           *
 *                                                          *
 ************************************************************
 */
  child = block[i];
  child->used = 1;
  child->stop = 0;
  for (i = 0; i < smp_max_threads; i++)
    child->siblings[i] = 0;
  child->nprocs = 0;
  child->ply = ply;
  child->position = parent->position;
  child->pv[ply - 1] = parent->pv[ply - 1];
  child->pv[ply] = parent->pv[ply];
  child->next_status[ply] = parent->next_status[ply];
  child->save_hash_key[ply] = parent->save_hash_key[ply];
  child->save_pawn_hash_key[ply] = parent->save_pawn_hash_key[ply];
  child->rep_index = parent->rep_index;
  for (i = 0; i <= parent->rep_index + parent->ply; i++)
    child->rep_list[i] = parent->rep_list[i];
  child->last[ply] = child->move_list;
  child->hash_move[ply] = parent->hash_move[ply];
  for (i = 1; i <= ply + 1; i++) {
    child->status[i] = parent->status[i];
    child->curmv[i] = parent->curmv[i];
    child->inchk[i] = parent->inchk[i];
    child->phase[i] = parent->phase[i];
  }
  for (i = 1; i < MAXPLY; i++)
    child->killers[i] = parent->killers[i];
  child->nodes_searched = 0;
  child->fail_highs = 0;
  child->fail_high_first_move = 0;
  child->evaluations = 0;
  child->egtb_probes = 0;
  child->egtb_probes_successful = 0;
  child->extensions_done = 0;
  child->qchecks_done = 0;
  child->reductions_done = 0;
  child->moves_fpruned = 0;
  child->alpha = parent->alpha;
  child->beta = parent->beta;
  child->value = parent->value;
  child->side = parent->side;
  child->depth = parent->depth;
  parent->siblings[thread] = child;
  child->thread_id = thread;
  child->parent = parent;
  parent->nprocs++;
  strcpy(child->root_move_text, parent->root_move_text);
  strcpy(child->remaining_moves_text, parent->remaining_moves_text);
}

/* modified 04/18/14 */
/*
 *******************************************************************************
 *                                                                             *
 *   CopyToParent() is used to copy data from a child thread to a parent       *
 *   thread.  This only copies the appropriate parts of the TREE structure to  *
 *   avoid burning memory bandwidth by copying everything.                     *
 *                                                                             *
 *******************************************************************************
 */
void CopyToParent(TREE * RESTRICT parent, TREE * RESTRICT child, int value) {
  int ply = parent->ply;

/*
 ************************************************************
 *                                                          *
 *  Only concern here is to make sure that the info is only *
 *  copied to the parent if our score is > than the parent  *
 *  value, and that we were not stopped for any reason      *
 *  which could produce a partial score that is worthless.  *
 *                                                          *
 *  In any case, we add our statistical counters to the     *
 *  parent's totals no matter whether we finished or not    *
 *  since the total nodes searched and such should consider *
 *  everything searched, not just the "useful stuff."       *
 *                                                          *
 ************************************************************
 */
  if (child->nodes_searched && !child->stop && value > parent->value &&
      !abort_search) {
    parent->pv[ply] = child->pv[ply];
    parent->value = value;
    parent->cutmove = child->curmv[ply];
  }
  parent->nodes_searched += child->nodes_searched;
  parent->fail_highs += child->fail_highs;
  parent->fail_high_first_move += child->fail_high_first_move;
  parent->evaluations += child->evaluations;
  parent->egtb_probes += child->egtb_probes;
  parent->egtb_probes_successful += child->egtb_probes_successful;
  parent->extensions_done += child->extensions_done;
  parent->qchecks_done += child->qchecks_done;
  parent->reductions_done += child->reductions_done;
  parent->moves_fpruned += child->moves_fpruned;
  child->used = 0;
}

/*
 *******************************************************************************
 *                                                                             *
 *   WaitForAllThreadsInitialized() waits until all smp_max_threads are        *
 *   initialized.  We have to initialize each thread and malloc() its split    *
 *   blocks before we start the actual parallel search.  Otherwise we will see *
 *   invalid memory accesses and crash instantly.                              *
 *                                                                             *
 *******************************************************************************
 */
void WaitForAllThreadsInitialized(void) {
  while (initialized_threads < smp_max_threads);        /* Do nothing */
}

/* modified 06/07/09 */
/*
 *******************************************************************************
 *                                                                             *
 *   ThreadInit() is called after a process is created.  Its main task is to   *
 *   initialize the process local memory so that it will fault in and be       *
 *   allocated on the local node rather than the node where the original       *
 *   (first) process was running.  All threads will hang here via a custom     *
 *   WaitForALlThreadsInitialized() procedure so that all the local thread     *
 *   blocks are usable before the search actually begins.                      *
 *                                                                             *
 *******************************************************************************
 */
void *STDCALL ThreadInit(void *tid) {
  int i, j;
#if defined(AFFINITY)
  int64_t k;
  cpu_set_t cpuset;
  pthread_t current_thread = pthread_self();

  k = (int64_t) tid;
  CPU_ZERO(&cpuset);
  CPU_SET(k, &cpuset);
  pthread_setaffinity_np(current_thread, sizeof(cpu_set_t), &cpuset);
#endif
#if !defined(UNIX)
  ThreadMalloc((uint64_t) tid);
#endif
  for (i = 0; i < MAX_BLOCKS_PER_CPU; i++) {
    memset((void *) block[(uint64_t) tid * MAX_BLOCKS_PER_CPU + i + 1], 0,
        sizeof(TREE));
    block[(uint64_t) tid * MAX_BLOCKS_PER_CPU + i + 1]->used = 0;
    block[(uint64_t) tid * MAX_BLOCKS_PER_CPU + i + 1]->parent = NULL;
    LockInit(block[(uint64_t) tid * MAX_BLOCKS_PER_CPU + i + 1]->lock);
    for (j = 0; j < 64; j++)
      block[(uint64_t) tid * MAX_BLOCKS_PER_CPU + i + 1]->cache_n[j] = ~0ull;
  }
  Lock(lock_smp);
  initialized_threads++;
  Unlock(lock_smp);
  WaitForAllThreadsInitialized();
  ThreadWait((uint64_t) tid, (TREE *) 0);
  Lock(lock_smp);
  smp_threads--;
  Unlock(lock_smp);
  return 0;
}

#if !defined (UNIX)
/* modified 01/17/09 */
/*
 *******************************************************************************
 *                                                                             *
 *   ThreadMalloc() is called from the ThreadInit() function.  It malloc's the *
 *   split blocks in the local memory for the processor associated with the    *
 *   specific thread that is calling this code.                                *
 *                                                                             *
 *******************************************************************************
 */
extern void *WinMalloc(size_t, int);
void ThreadMalloc(int64_t tid) {
  int i, n = MAX_BLOCKS_PER_CPU;

  for (i = MAX_BLOCKS_PER_CPU * (tid) + 1; n; i++, n--) {
    if (block[i] == NULL)
      block[i] =
          (TREE *) ((~(size_t) 127) & (127 + (size_t) WinMalloc(sizeof(TREE) +
                  127, tid)));
    block[i]->used = 0;
    block[i]->parent = NULL;
    LockInit(block[i]->lock);
  }
}
#endif

/* modified 01/17/09 */
/*
 *******************************************************************************
 *                                                                             *
 *   ThreadStop() is called from SearchParallel() when it detects a beta       *
 *   cutoff (fail high) at a node that is being searched in parallel.  We need *
 *   to stop all threads here, and since this threading algorithm is recursive *
 *   it may be necessary to stop other threads that are helping search this    *
 *   branch further down into the tree.  This function simply sets tree->stop  *
 *   to 1, which will stop that particular thread instantly and return it to   *
 *   the idle loop in ThreadWait().                                            *
 *                                                                             *
 *******************************************************************************
 */
void ThreadStop(TREE * RESTRICT tree) {
  int proc;

  Lock(tree->lock);
  tree->stop = 1;
  for (proc = 0; proc < smp_max_threads; proc++)
    if (tree->siblings[proc])
      ThreadStop(tree->siblings[proc]);
  Unlock(tree->lock);
}

/* modified 04/23/14 */
/*
 *******************************************************************************
 *                                                                             *
 *   ThreadWait() is the idle loop for the N threads that are created at the   *
 *   beginning when Crafty searches.  Threads are "parked" here waiting on a   *
 *   pointer to something they should search (a parameter block built in the   *
 *   function Thread() in this case.  When this pointer becomes non-zero, each *
 *   thread "parked" here will immediately call SearchParallel() and begin the *
 *   parallel search as directed.                                              *
 *                                                                             *
 *******************************************************************************
 */
int ThreadWait(int64_t tid, TREE * RESTRICT waiting) {
  int value, tstart, tend;

/*
 ************************************************************
 *                                                          *
 *  The N initial threads enter here and are kept penned    *
 *  here forever.  However, once a thread leaves here it    *
 *  may well re-enter ThreadWait() from the top while it    *
 *  waits for a parallel search to complete.  While it      *
 *  waits here it can also join in to help other busy       *
 *  threads search their subtrees as well.                  *
 *                                                          *
 ************************************************************
 */
  while (1) {
    tstart = ReadClock();
    Lock(lock_split);
    Lock(lock_smp);
    smp_idle++;
    if (!waiting && smp_split == 0)
      smp_split = 1;
    if (!thread[tid].tree)
      thread[tid].idle = 1;
    Unlock(lock_smp);
    Unlock(lock_split);
/*
 ************************************************************
 *                                                          *
 *  We can exit if our thread[i] is non-zero, or if we are  *
 *  waiting on others to finish a block that *we* have to   *
 *  return through.  When the busy count on such a block    *
 *  hits zero, we return immediately which unwinds the      *
 *  search as it should be.                                 *
 *                                                          *
 ************************************************************
 */
    while (!thread[tid].tree && (!waiting || waiting->nprocs))
      Pause();
    thread[tid].idle = 0;
    tend = ReadClock();
    Lock(lock_smp);
    idle_time += tend - tstart;
    if (!thread[tid].tree)
      thread[tid].tree = waiting;
/*
 ************************************************************
 *                                                          *
 *  We either have work to do, or threads we were waiting   *
 *  on have finished their work.                            *
 *                                                          *
 ************************************************************
 */
    smp_idle--;
    Unlock(lock_smp);
/*
 ************************************************************
 *                                                          *
 *  If we are waiting on a block and the busy count is now  *
 *  zero, we simply return to finish up the bookkeeping at  *
 *  that point.                                             *
 *                                                          *
 ************************************************************
 */
    if (thread[tid].tree == waiting || thread[tid].tree == (TREE *) - 1)
      return 0;
/*
 ************************************************************
 *                                                          *
 *  Else our thread[i] pointer is non-zero, meaning we have *
 *  been assigned something to search.  Hi-ho, hi-ho, it's  *
 *  off to work we go...                                    *
 *                                                          *
 ************************************************************
 */
    value =
        SearchParallel(thread[tid].tree, thread[tid].tree->alpha,
        thread[tid].tree->beta, thread[tid].tree->value,
        thread[tid].tree->side, thread[tid].tree->depth,
        thread[tid].tree->ply);
    Lock(thread[tid].tree->parent->lock);
    CopyToParent((TREE *) thread[tid].tree->parent, thread[tid].tree, value);
    thread[tid].tree->parent->nprocs--;
    thread[tid].tree->parent->siblings[tid] = 0;
    Unlock(thread[tid].tree->parent->lock);
    thread[tid].tree = 0;
  }
}
