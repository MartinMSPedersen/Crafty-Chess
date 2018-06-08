#include "chess.h"
#include "data.h"
#include "epdglue.h"
/* modified 06/07/09 */
/*
 *******************************************************************************
 *                                                                             *
 *   Thread() is the driver for the threaded parallel search in Crafty.  The   *
 *   basic idea is that whenever we notice that one (or more) threads are      *
 *   in their idle loop, we drop into thread, from Search(), and begin a new   *
 *   parallel search from this node.  This is simply a problem of copying the  *
 *   search state space for each thread working at this node, then sending     *
 *   everyone off to SearchParallel() to search this node in parallel.         *
 *                                                                             *
 *******************************************************************************
 */
int Thread(TREE * RESTRICT tree) {
  TREE *block;
  int proc;
  int nblocks = 0;

/*
 ************************************************************
 *                                                          *
 *   First, we make sure that there are idle threads. It    *
 *   is possible that we get here after they have all been  *
 *   claimed by another thread.  In that case we return.    *
 *                                                          *
 ************************************************************
 */
  Lock(lock_smp);
  for (proc = 0; proc < smp_max_threads && thread[proc]; proc++);
  if (proc == smp_max_threads || tree->stop) {
    Unlock(lock_smp);
    return (0);
  }
#if defined(DEBUGSMP)
  Lock(lock_io);
  Print(128, "thread %d  block %d  ply %d  parallel split\n", tree->thread_id,
      FindBlockID(tree), tree->ply);
  Print(128, "thread %d  threads(s) idle:", tree->thread_id);
  for (proc = 0; proc < smp_max_threads; proc++)
    if (!thread[proc])
      Print(128, " %d", proc);
  Print(128, "\n");
  Unlock(lock_io);
#endif
/*
 ************************************************************
 *                                                          *
 *   Now we start copying the current "state" from this     *
 *   thread into new thread blocks so that the threads can  *
 *   search this position without interfering with each     *
 *   other.  As we copy, we link those blocks as siblings   *
 *   of the parent node, and point each sibling back to the *
 *   parent so we can unwind this confusion as the threads  *
 *   complete their work.                                   *
 *                                                          *
 ************************************************************
 */
  thread[tree->thread_id] = 0;
  tree->nprocs = 0;
  for (proc = 0;
      proc < smp_max_threads && (nblocks < smp_max_thread_group ||
          tree->ply == 1); proc++) {
    tree->siblings[proc] = 0;
    if (thread[proc] == 0) {
      block = CopyToChild(tree, proc);
      if (!block)
        continue;
      nblocks++;
      tree->siblings[proc] = block;
      block->thread_id = proc;
      block->parent = tree;
      tree->nprocs++;
#if defined(DEBUGSMP)
      Print(128, "thread %d  block %d  allocated at ply=%d\n", proc,
          FindBlockID(block), tree->ply);
#endif
    }
  }
  if (!nblocks) {
    Unlock(lock_smp);
    thread[tree->thread_id] = tree;
    return (0);
  }
/*
 ************************************************************
 *                                                          *
 *   Everything is set.  Now we can stuff the address of    *
 *   the thread blocks into thread[i] so that those idle    *
 *   threads can begin the parallel search.                 *
 *                                                          *
 ************************************************************
 */
  parallel_splits++;
  for (proc = 0; proc < smp_max_threads; proc++)
    if (tree->siblings[proc])
      thread[proc] = tree->siblings[proc];
/*
 ************************************************************
 *                                                          *
 *   After the threads are kicked off to start the parallel *
 *   search using the idle threads, This thread has to be   *
 *   inserted as well.  However, since it is possible that  *
 *   this thread may finish before any or all of the other  *
 *   parallel threads, this thread is sent to ThreadWait()  *
 *   which will immediately send it to SearchParallel()     *
 *   like the other threads.  Going to ThreadWait() allows  *
 *   this thread to join others if it runs out of work to   *
 *   do.  We do pass ThreadWait() the address of the parent *
 *   thread block, so that if this thread becomes idle, and *
 *   this thread block shows no threads are still busy,     *
 *   then this thread can return to here and then back up   *
 *   into the previous ply as it should.  Note that no      *
 *   other thread can back up to the previous ply since     *
 *   their recursive call stacks are not set for that.      *
 *                                                          *
 ************************************************************
 */
  Unlock(lock_smp);
  ThreadWait(tree->thread_id, tree);
#if defined(DEBUGSMP)
  Print(128, "thread %d  block %d  ply %d  parallel join\n", tree->thread_id,
      FindBlockID(tree), tree->ply);
#endif
  return (1);
}

/* modified 09/25/13 */
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

  if (child->nodes_searched && !child->stop && value > parent->value &&
      !abort_search) {
    parent->pv[parent->ply] = child->pv[parent->ply];
    parent->value = value;
    parent->cutmove = child->curmv[parent->ply];
  }
  parent->nodes_searched += child->nodes_searched;
  parent->fail_highs += child->fail_highs;
  parent->fail_high_number += child->fail_high_number;
  parent->evaluations += child->evaluations;
  parent->egtb_probes += child->egtb_probes;
  parent->egtb_probes_successful += child->egtb_probes_successful;
  parent->extensions_done += child->extensions_done;
  parent->qchecks_done += child->qchecks_done;
  parent->reductions_done += child->reductions_done;
  parent->moves_pruned += child->moves_pruned;
  child->used = 0;
}

/* modified 09/25/13 */
/*
 *******************************************************************************
 *                                                                             *
 *   CopyToChild() is used to copy data from a parent thread to a particular   *
 *   child thread.  This only copies the appropriate parts of the TREE         *
 *   structure to avoid burning memory bandwidth by copying everything.        *
 *                                                                             *
 *******************************************************************************
 */
TREE *CopyToChild(TREE * RESTRICT parent, int thread) {
  int i, j, max;
  TREE *child;
  static int warnings = 0;
  int first = thread * MAX_BLOCKS_PER_CPU + 1;
  int last = first + MAX_BLOCKS_PER_CPU;
  int maxb = smp_max_threads * MAX_BLOCKS_PER_CPU + 1;

  for (i = first; i < last && block[i]->used; i++);
  if (i >= last) {
    if (++warnings < 6)
      Print(128,
          "WARNING.  optimal SMP block cannot be allocated, thread %d\n",
          thread);
    for (i = 1; i < maxb && block[i]->used; i++);
    if (i >= maxb) {
      if (warnings < 6)
        Print(128, "ERROR.  no SMP block can be allocated\n");
      return (0);
    }
  }
  max = 0;
  for (j = 1; j < maxb; j++)
    if (block[j]->used)
      max++;
  max_split_blocks = Max(max_split_blocks, max);
  child = block[i];
  child->used = 1;
  child->stop = 0;
  for (i = 0; i < smp_max_threads; i++)
    child->siblings[i] = 0;
  child->ply = parent->ply;
  child->pos = parent->pos;
  child->pv[child->ply - 1] = parent->pv[child->ply - 1];
  child->pv[child->ply] = parent->pv[child->ply];
  child->next_status[child->ply] = parent->next_status[child->ply];
  child->save_hash_key[child->ply] = parent->save_hash_key[child->ply];
  child->save_pawn_hash_key[child->ply] =
      parent->save_pawn_hash_key[child->ply];
  for (i = 0; i < 2; i++)
    child->rep_index[i] = parent->rep_index[i];
  for (i = 0; i < 2; i++)
    for (j = 0; j < child->rep_index[i] + (child->ply - 1) / 2; j++)
      child->rep_list[i][j] = parent->rep_list[i][j];
  child->last[child->ply] = child->move_list;
  child->hash_move[child->ply] = parent->hash_move[child->ply];
  for (i = 1; i <= child->ply + 1; i++) {
    child->position[i] = parent->position[i];
    child->curmv[i] = parent->curmv[i];
    child->inchk[i] = parent->inchk[i];
    child->phase[i] = parent->phase[i];
  }
  for (i = 1; i < MAXPLY; i++)
    child->killers[i] = parent->killers[i];
  child->nodes_searched = 0;
  child->fail_highs = 0;
  child->fail_high_number = 0;
  child->evaluations = 0;
  child->egtb_probes = 0;
  child->egtb_probes_successful = 0;
  child->extensions_done = 0;
  child->qchecks_done = 0;
  child->reductions_done = 0;
  child->moves_pruned = 0;
  child->alpha = parent->alpha;
  child->beta = parent->beta;
  child->value = parent->value;
  child->wtm = parent->wtm;
  child->depth = parent->depth;
  strcpy(child->root_move_text, parent->root_move_text);
  strcpy(child->remaining_moves_text, parent->remaining_moves_text);
  return (child);
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

#if defined(_WIN32) || defined(_WIN64)
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
  return (0);
}

#if defined (_WIN32) || defined (_WIN64)
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
#if defined(DEBUGSMP)
  Lock(lock_io);
  Print(128, "thread %d (block %d) being stopped by beta cutoff.\n",
      tree->thread_id, FindBlockID(tree));
  Unlock(lock_io);
#endif
}

/* modified 01/17/09 */
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
  int value;

/*
 ************************************************************
 *                                                          *
 *   The N initial threads enter here and are kept penned   *
 *   here forever.  However, once a thread leaves here it   *
 *   may well re-enter ThreadWait() from the top while it   *
 *   waits for a parallel search to complete.  While it     *
 *   waits here it can also join in to help other busy      *
 *   threads search their subtrees as well.                 *
 *                                                          *
 ************************************************************
 */
  while (1) {
    Lock(lock_smp);
    smp_idle++;
    Unlock(lock_smp);
#if defined(DEBUGSMP)
    Lock(lock_io);
    Print(128, "thread %d now idle (%d procs, %d idle).\n", tid,
        smp_max_threads, smp_idle);
    if (FindBlockID(waiting) >= 0)
      Print(128,
          "thread %d  waiting on block %d, still %d threads busy there\n",
          tid, FindBlockID(waiting), waiting->nprocs);
    Unlock(lock_io);
#endif
/*
 ************************************************************
 *                                                          *
 *   We can exit if our thread[i] is non-zero, or if we are *
 *   waiting on others to finish a block that *we* have to  *
 *   return through.  When the busy count on such a block   *
 *   hits zero, we return immediately which unwinds the     *
 *   search as it should be.                                *
 *                                                          *
 ************************************************************
 */
    while (!thread[tid] && (!waiting || waiting->nprocs));
    Lock(lock_smp);
    if (!thread[tid])
      thread[tid] = waiting;
/*
 ************************************************************
 *                                                          *
 *   We either have work to do, or threads we were waiting  *
 *   on have finished their work.                           *
 *                                                          *
 ************************************************************
 */
#if defined(DEBUGSMP)
    Lock(lock_io);
    Print(128, "thread %d now has work at block %d.\n", tid,
        FindBlockID(thread[tid]));
    Unlock(lock_io);
#endif
    smp_idle--;
/*
 ************************************************************
 *                                                          *
 *   If we are waiting on a block and the busy count is now *
 *   zero, we simply return to finish up the bookkeeping at *
 *   that point.                                            *
 *                                                          *
 ************************************************************
 */
    Unlock(lock_smp);
    if (thread[tid] == waiting || thread[tid] == (TREE *) - 1)
      return (0);
/*
 ************************************************************
 *                                                          *
 *   Else our thread[i] pointer is non-zero, meaning we     *
 *   have been assigned something to search.  hi-ho, hi-ho, *
 *   off to work we go...                                   *
 *                                                          *
 ************************************************************
 */
    value =
        SearchParallel(thread[tid], thread[tid]->alpha, thread[tid]->beta,
        thread[tid]->value, thread[tid]->wtm, thread[tid]->depth,
        thread[tid]->ply);
    Lock(lock_smp);
    Lock(thread[tid]->parent->lock);
#if defined(DEBUGSMP)
    Lock(lock_io);
    Print(128, "thread %d  block %d marked free at ply %d\n", tid,
        FindBlockID(thread[tid]), thread[tid]->ply);
    Unlock(lock_io);
#endif
    CopyToParent((TREE *) thread[tid]->parent, thread[tid], value);
    thread[tid]->parent->nprocs--;
#if defined(DEBUGSMP)
    Lock(lock_io);
    Print(128, "thread %d decremented block %d  nprocs=%d\n", tid,
        FindBlockID(thread[tid]->parent), thread[tid]->parent->nprocs);
    Unlock(lock_io);
#endif
    thread[tid]->parent->siblings[tid] = 0;
    Unlock(thread[tid]->parent->lock);
#if defined(DEBUGSMP)
    Lock(lock_io);
    Print(128, "thread %d  block %d  parent %d  nprocs %d exit at ply %d\n",
        tid, FindBlockID(thread[tid]), FindBlockID(thread[tid]->parent),
        thread[tid]->parent->nprocs, thread[tid]->ply);
    Unlock(lock_io);
#endif
    thread[tid] = 0;
    Unlock(lock_smp);
  }
}
