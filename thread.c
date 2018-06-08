#include "chess.h"
#include "data.h"
#include "epdglue.h"
/* modified 09/18/14 */
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
 *   This is generation II of Thread().  The main difference is the effort     *
 *   required to split the tree and which thread(s) expend this effort.  In    *
 *   generation I, the parent thread was responsible for allocating a split    *
 *   block for each child thread, and then copying the necessary data from the *
 *   parent split block to these child split blocks.  When all of this was     *
 *   completed, the child processes were released to start the parallel search *
 *   after being held while the split / copy operations were done.  In the     *
 *   generation II Thread() we now simply allocate the split blocks, and fill  *
 *   in the bare minimum needed to avoid any potential race conditions, such   *
 *   as making sure the thread stop flag is zero, the child split blocks have  *
 *   a pointer back to the parent split block, the child split block has the   *
 *   correct thread id, etc..  As soon as each child has a split block that is *
 *   usable, it is released and now uses CopyFromParent() to copy the tree-    *
 *   related stuff to its local split block.  This is done in in parallel by   *
 *   the threads that are going to be added here, rather than serially by the  *
 *   parent thread,  which eliminates some unnecessary wait time.              *
 *                                                                             *
 *   Generation II is also much more lightweight, in that it copies only the   *
 *   bare minimum from parent to child.  Generation I safely copied far too    *
 *   much since this code was being changed regularly, but that is no longer   *
 *   necessary overhead.                                                       *
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
 *      at any single split point, with the exception of split points fairly   *
 *      close to the root  where ALL threads are allowed to split together     *
 *      ignoring this limit.  A subtle point.  This should be #threads - 1,    *
 *      which will then add the parent thread as well, making the number of    *
 *      threads at this split point one more than the value of this option.    *
 *      Five (5) is the usual default giving a total of six threads per split  *
 *      point.  Note that this is ignored in the first 1/2 of the tree (the    *
 *      nodes closer to the root).  There is is actually good to split and get *
 *      all active threads involved.                                           *
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
 *      The above two values are complementary.  We won't split unless depth   *
 *      >= smp_min_split_depth plies, AND this thread has searched at least    *
 *      smp_split_nodes since it was created, which is a test to avoid trouble *
 *      by splitting too frequently (aka thrashing).  If you want to test and  *
 *      disable either (or both), just set them to zero (0) and you will see   *
 *      massive numbers of splits, and a significantly slower search to go     *
 *      along with that.                                                       *
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
 *       split block first.  IE tree->parent->lock to lock the parent split    *
 *       block during NextMove() and such.  Note that this ONLY applies to     *
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
  int tid, nblocks = 0, nidle = 0;
  TREE *child;

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
  for (tid = 0; tid < smp_max_threads; tid++)
    if (thread[tid].idle)
      break;
  if (tid == smp_max_threads) {
    smp_split = 0;
    Unlock(lock_smp);
    return 0;
  }
/*
 ************************************************************
 *                                                          *
 *  Now we prepare to split the tree.  The idea is quite    *
 *  simple - we cycle through the list of threads, and for  *
 *  any thread that is currently listed as "idle" we grab   *
 *  a split block and assign it to that thread.             *
 *                                                          *
 *  Special case:  In the loop to allocate a split block    *
 *  we skip over the current thread (the one that is doing  *
 *  the split operation).  After we complete the loop, we   *
 *  explicitly allocate a block for this thread to force it *
 *  to be included in the thread group (it is possible that *
 *  enough threads are idle so that we would allocate too   *
 *  many threads to this split, not leaving room for this   *
 *  thread.  While this would work, it is inefficient since *
 *  the current thread needs to join here as it is the only *
 *  thread that can back up through this split point.       *
 *                                                          *
 *  For example, suppose we are thread 6, and when we enter *
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
 *  For this reason, smp_split_group should always be set   *
 *  to max threads at a split point - 1, since we ALWAYS    *
 *  add in the current thread after selecting the rest of   *
 *  the group.                                              *
 *                                                          *
 ************************************************************
 */
  thread[tree->thread_id].tree = 0;
  tree->nprocs = 0;
  for (tid = 0; tid < smp_max_threads; tid++)
    tree->siblings[tid] = 0;
  for (tid = 0; tid < smp_max_threads; tid++) {
    if (thread[tid].idle) {
      child = GetBlock(tree, tid);
      if (child) {
        nblocks++;
        if (nblocks >= smp_split_group && tree->ply > tree->depth / 2)
          break;
      }
    }
  }
  thread[tree->thread_id].tree = GetBlock(tree, tree->thread_id);
  parallel_splits++;
/*
 ************************************************************
 *                                                          *
 *  One final test before leaving.  We test all threads to  *
 *  determine if any are still idle (most likely due to the *
 *  smp_split_group limit, but a thread could go idle after *
 *  our loop to pick up idle threads).  If so we set split  *
 *  back to one to force the next thread that satisfies the *
 *  YBW criterion to do a split and pick up the rest of the *
 *  idle threads.                                           *
 *                                                          *
 ************************************************************
 */
  for (tid = 0; tid < smp_max_threads; tid++)
    if (thread[tid].idle)
      nidle++;
  smp_split = (nidle) ? 1 : 0;
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
 *  call stacks are not set for that, while this call stack *
 *  will bring us back to this point where we return to the *
 *  normal search, which we just completed.                 *
 *                                                          *
 ************************************************************
 */
  Unlock(lock_smp);
  ThreadWait(tree->thread_id, tree);
  return 1;
}

/* modified 09/18/14 */
/*
 *******************************************************************************
 *                                                                             *
 *   CopyFromParent() is used to copy data from a parent thread to a child     *
 *   thread.  This only copies the appropriate parts of the TREE structure to  *
 *   avoid burning memory bandwidth by copying everything.                     *
 *                                                                             *
 *******************************************************************************
 */
void CopyFromParent(TREE * RESTRICT child) {
  TREE *parent = child->parent;
  int i, ply;

/*
 ************************************************************
 *                                                          *
 *  We have allocated a split block.  Now we copy the tree  *
 *  search state from the parent block to the child in      *
 *  preparation for starting the parallel search.           *
 *                                                          *
 ************************************************************
 */
  ply = parent->ply;
  child->ply = ply;
  child->position = parent->position;
  child->rep_index = parent->rep_index;
  for (i = 0; i <= parent->rep_index + parent->ply; i++)
    child->rep_list[i] = parent->rep_list[i];
  for (i = ply - 1; i < MAXPLY; i++)
    child->killers[i] = parent->killers[i];
  for (i = ply - 1; i <= ply; i++) {
    child->curmv[i] = parent->curmv[i];
    child->pv[i] = parent->pv[i];
  }
  child->in_check = parent->in_check;
  child->last[ply] = child->move_list;
  child->status[ply] = parent->status[ply];
  child->status[1] = parent->status[1];
  child->save_hash_key[ply] = parent->save_hash_key[ply];
  child->save_pawn_hash_key[ply] = parent->save_pawn_hash_key[ply];
  child->nodes_searched = 0;
  child->fail_highs = 0;
  child->fail_high_first_move = 0;
  child->evaluations = 0;
  child->egtb_probes = 0;
  child->egtb_probes_successful = 0;
  child->extensions_done = 0;
  child->qchecks_done = 0;
  child->moves_fpruned = 0;
  for (i = 0; i < 16; i++)
    child->LMR_done[i] = 0;
  for (i = 0; i < 32; i++)
    child->null_done[i] = 0;
  child->alpha = parent->alpha;
  child->beta = parent->beta;
  child->value = parent->value;
  child->wtm = parent->wtm;
  child->depth = parent->depth;
  strcpy(child->root_move_text, parent->root_move_text);
  strcpy(child->remaining_moves_text, parent->remaining_moves_text);
}

/* modified 09/18/14 */
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
  int i, ply = parent->ply;

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
  parent->moves_fpruned += child->moves_fpruned;
  for (i = 1; i < 16; i++)
    parent->LMR_done[i] += child->LMR_done[i];
  for (i = 1; i < 32; i++)
    parent->null_done[i] += child->null_done[i];
  child->used = 0;
}

/* modified 09/18/14 */
/*
 *******************************************************************************
 *                                                                             *
 *   GetBlock() is used to allocate a split block and fill in only SMP-        *
 *   critical information.  The child process will copy the rest of the split  *
 *   block information as needed.                                              *
 *                                                                             *
 *******************************************************************************
 */
TREE *GetBlock(TREE * RESTRICT parent, int tid) {
  int i, max;
  TREE *child;
  static int warnings = 0;
  int first = tid * MAX_BLOCKS_PER_CPU + 1;
  int last = first + MAX_BLOCKS_PER_CPU;
  int maxb = smp_max_threads * MAX_BLOCKS_PER_CPU + 1;
/*
 ************************************************************
 *                                                          *
 *  One NUMA-related trick is that we only try to allocate  *
 *  a split block in the thread's local memory.  Each       *
 *  thread has a group of split blocks that were first      *
 *  touched by the correct CPU so that the split blocks     *
 *  page faulted into local memory for that specific        *
 *  processor.  If we can't find an optimal-placed block,   *
 *  we return a zero which will prevent this thread from    *
 *  joining the split point.                                *
 *                                                          *
 ************************************************************
 */
  for (i = first; i < last && block[i]->used; i++);
  if (i >= last) {
    if (++warnings < 6)
      Print(128,
          "WARNING.  optimal SMP block cannot be allocated, thread %d\n",
          tid);
    return 0;
  }
  child = block[i];
/*
 ************************************************************
 *                                                          *
 *  Found a split block.  Now we need to fill in only the   *
 *  critical information that can't be delayed due to race  *
 *  conditions.                                             *
 *                                                          *
 ************************************************************
 */
  child->used = 1;
  for (i = 0; i < smp_max_threads; i++)
    child->siblings[i] = 0;
  child->nprocs = 0;
  child->stop = 0;
  child->parent = parent;
  thread[tid].idle = 0;
  child->thread_id = tid;
  parent->nprocs++;
  parent->siblings[tid] = child;
  thread[tid].tree = child;
/*
 ************************************************************
 *                                                          *
 *  Remember the max blocks used so that we can detect the  *
 *  case where block usage becomes excessive.               *
 *                                                          *
 ************************************************************
 */
  max = 0;
  for (i = 1; i < maxb; i++)
    if (block[i]->used)
      max++;
  max_split_blocks = Max(max_split_blocks, max);
  return child;
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
void *STDCALL ThreadInit(void *t) {
  int i, tid = (int64_t) t;
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
  }
  Lock(lock_smp);
  initialized_threads++;
  Unlock(lock_smp);
  WaitForAllThreadsInitialized();
  ThreadWait(tid, (TREE *) 0);
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
 *   branch further down into the tree.  This function simply sets appropriate *
 *   tree->stop variables to 1, which will stop those particular threads       *
 *   instantly and return them to the idle loop in ThreadWait().               *
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

/* modified 09/18/14 */
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
int ThreadWait(int tid, TREE * RESTRICT waiting) {
  int value, tstart, tend;

/*
 ************************************************************
 *                                                          *
 *  The N-1 initial threads enter here and are kept penned  *
 *  here forever.  However, once a thread leaves here it    *
 *  may well re-enter ThreadWait() from the top while it    *
 *  waits for a parallel search to complete.  While it      *
 *  waits here it can also join in to help other busy       *
 *  threads search their subtrees as well.                  *
 *                                                          *
 *  The short while(1) loop below is tricky.  We check,     *
 *  without locking, to see if we are splitting already     *
 *  (smp_split = -1).  If so we just hang until smp_split   *
 *  is not -1.  Once that is true, we then mark this thread *
 *  as idle and set the split flag.  The issue here is that *
 *  the split operation is very lightweight.  If we are in  *
 *  the middle of a split, there is a 50-50 chance that     *
 *  this thread won't be picked up, and since we set split  *
 *  to zero after the split is done, we might leave the     *
 *  current thread in the idle loop with no split signal to *
 *  pick it up.  Simple solution is to wait until the split *
 *  opertion has been completed, then set the idle flag for *
 *  this thread and then post a split request.              *
 *                                                          *
 ************************************************************
 */
  while (1) {
    tstart = ReadClock();
    while (smp_split < 0);
    Lock(lock_smp);
    smp_idle++;
    if (!thread[tid].tree)
      thread[tid].idle = 1;
    smp_split = 1;
    Unlock(lock_smp);
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
 *  Now we copy the parent split block data to our local    *
 *  split block and then jump into the parallel search.  So *
 *  hi-ho, hi-ho, it's off to work we go...                 *
 *                                                          *
 ************************************************************
 */
    CopyFromParent(thread[tid].tree);
    value =
        SearchParallel(thread[tid].tree, thread[tid].tree->alpha,
        thread[tid].tree->beta, thread[tid].tree->value,
        thread[tid].tree->wtm, thread[tid].tree->depth, thread[tid].tree->ply,
        thread[tid].tree->in_check);
    Lock(thread[tid].tree->parent->lock);
    CopyToParent((TREE *) thread[tid].tree->parent, thread[tid].tree, value);
    thread[tid].tree->parent->nprocs--;
    thread[tid].tree->parent->siblings[tid] = 0;
    Unlock(thread[tid].tree->parent->lock);
    thread[tid].tree = 0;
  }
}
