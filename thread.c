#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "chess.h"
#include "data.h"
#include "epdglue.h"

/* modified 04/28/98 */
/*
 *******************************************************************************
 *                                                                             *
 *   Thread() is the driver for the threaded parallel search in Crafty.  the   *
 *   basic idea is that whenever we notice that one (or more) threads are      *
 *   in their idle loop, we drop into thread, from Search(), and begin a new   *
 *   parallel search from this node.  this is simply a problem of copying the  *
 *   search state space for each thread working at this node, then sending     *
 *   everyone off to SearchSMP() to search this node in parallel.              *
 *                                                                             *
 *******************************************************************************
 */
#if defined(SMP)
int Thread(TREE * RESTRICT tree)
{
  TREE *block;
  int proc;
  int nblocks = 0;

/*
 ************************************************************
 *                                                          *
 *   first, we make sure that there are idle threads. it    *
 *   is possible that we get here after they have all been  *
 *   claimed by another thread.  in that case we return.    *
 *                                                          *
 ************************************************************
 */
  Lock(lock_smp);
  for (proc = 0; proc < max_threads && thread[proc]; proc++);
  if (proc == max_threads || tree->stop) {
    Unlock(lock_smp);
    return (0);
  }
#  if defined(DEBUGSMP)
  Lock(lock_io);
  Print(128, "thread %d  block %d  ply %d  parallel split\n", tree->thread_id,
      FindBlockID(tree), tree->ply);
  Print(128, "thread %d  threads(s) idle:", tree->thread_id);
  for (proc = 0; proc < max_threads; proc++)
    if (!thread[proc])
      Print(128, " %d", proc);
  Print(128, "\n");
  Unlock(lock_io);
#  endif
/*
 ************************************************************
 *                                                          *
 *   now we start copying the current "state" from this     *
 *   thread into new thread blocks so that the threads can  *
 *   search this position without interfering with each     *
 *   other.  as we copy, we link those blocks as siblings   *
 *   of the parent node, and point each sibling back to the *
 *   parent so we can unwind this confusion as the threads  *
 *   complete their work.                                   *
 *                                                          *
 ************************************************************
 */
  splitting = 1;
  parallel_splits++;
  thread[tree->thread_id] = 0;
  tree->nprocs = 0;
  for (proc = 0; proc < max_threads && nblocks < max_thread_group; proc++) {
    tree->siblings[proc] = 0;
    if (thread[proc] == 0) {
      block = CopyToSMP(tree, proc);
      if (!block)
        continue;
      nblocks++;
      tree->siblings[proc] = block;
      block->thread_id = proc;
      block->parent = tree;
      tree->nprocs++;
#  if defined(DEBUGSMP)
      Print(128, "thread %d  block %d  allocated at ply=%d\n", proc,
          FindBlockID(block), tree->ply);
#  endif
    } else
      tree->siblings[proc] = 0;
  }
  tree->search_value = tree->value;
  if (!nblocks) {
    Unlock(lock_smp);
    thread[tree->thread_id] = tree;
    splitting = 0;
    return (0);
  }
/*
 ************************************************************
 *                                                          *
 *   everything is set.  now we can stuff the address of    *
 *   the thread blocks into thread[i] so that those idle    *
 *   threads can begin the parallel search.                 *
 *                                                          *
 ************************************************************
 */
  for (proc = 0; proc < max_threads; proc++)
    if (tree->siblings[proc])
      thread[proc] = tree->siblings[proc];
  splitting = 0;
/*
 ************************************************************
 *                                                          *
 *   after the threads are kicked off to start the parallel *
 *   search using the idle threads, this thread has to be   *
 *   inserted as well.  however, since it is possible that  *
 *   this thread may finish before any or all of the other  *
 *   parallel threads, this thread is sent to ThreadWait()  *
 *   which will immediately send it to SearchSMP() just     *
 *   like the other threads.  going to ThreadWait() allows  *
 *   this thread to join others if it runs out of work to   *
 *   do.  we do pass ThreadWait() the address of the parent *
 *   thread block, so that if this thread becomes idle, and *
 *   this thread block shows no threads are still busy,     *
 *   then this thread can return to here and then back up   *
 *   into the previous ply as it should.  note that no      *
 *   other thread can back up to the previous ply since     *
 *   their recursive call stacks are not set for that.      *
 *                                                          *
 ************************************************************
 */
  Unlock(lock_smp);
  ThreadWait(tree->thread_id, tree);
#  if defined(DEBUGSMP)
  Print(128, "thread %d  block %d  ply %d  parallel join\n", tree->thread_id,
      FindBlockID(tree), tree->ply);
#  endif
  return (1);
}

/*
 *******************************************************************************
 *                                                                             *
 *   WaitForAllThreadsInitialized() waits till all max_threads are initialized.*
 *   Otherwise we can try to use not yest initialized local[] data.            *
 *                                                                             *
 *******************************************************************************
 */

static volatile int InitializedThreads = 0;

void WaitForAllThreadsInitialized(void)
{
  while (InitializedThreads < max_threads);     /* Do nothing */
}

/* modified 10/21/03 */
/*
 *******************************************************************************
 *                                                                             *
 *   ThreadInit() is called from the pthread_create() function.  it then calls *
 *   ThreadWait() with the appropriate arguments to park the new threads until *
 *   work is available.                                                        *
 *                                                                             *
 *******************************************************************************
 */

void *STDCALL ThreadInit(void *tid)
{
  ThreadMalloc((int) tid);
  WaitForAllThreadsInitialized();
  ThreadWait((int) tid, (TREE *) 0);
  return (0);
}

/* modified 10/21/03 */
/*
 *******************************************************************************
 *                                                                             *
 *   ThreadMalloc() is called from the ThreadInit() function.  it malloc's the *
 *   split blocks in the local memory for the processor associated with the    *
 *   specific thread that is calling this code.                                *
 *                                                                             *
 *******************************************************************************
 */
#  if defined (_WIN32) || defined (_WIN64)
extern void *WinMalloc(size_t, int);

#    define MALLOC(cb, iThread) WinMalloc(cb, iThread)
#  else
#    define MALLOC(cb, iThread) malloc(cb)
#  endif

lock_t lock_thread_init;

void ThreadMalloc(int tid)
{
  int i, n = MAX_BLOCKS / CPUS;

  if (0 == tid)
    LockInit(lock_thread_init);
  for (i = MAX_BLOCKS / CPUS * ((int) tid) + 1; n; i++, n--) {
    local[i] =
        (TREE *) ((~(size_t) 127) & (127 + (size_t) MALLOC(sizeof(TREE) + 127,
                tid)));
    local[i]->used = 0;
    local[i]->parent = (TREE *) - 1;
    LockInit(local[i]->lock);
  }
  Lock(lock_thread_init);
  InitializedThreads++;
  Unlock(lock_thread_init);
}

/* modified 04/26/98 */
/*
 *******************************************************************************
 *                                                                             *
 *   ThreadStop() is called from SearchSMP() when it detects a beta cutoff (or *
 *   fail high) at a node that is being searched in parallel.  we need to stop *
 *   all threads here, and since this threading algorithm is "recursive" it may*
 *   be necessary to stop other threads that are helping search this branch    *
 *   further down into the tree.  this function simply sets tree->stop to 1,   *
 *   which will stop that particular thread instantly and return it to the idle*
 *   loop in ThreadWait().                                                     *
 *                                                                             *
 *******************************************************************************
 */
void ThreadStop(TREE * RESTRICT tree)
{
  int proc;

  Lock(tree->lock);
  tree->stop = 1;
  for (proc = 0; proc < max_threads; proc++)
    if (tree->siblings[proc])
      ThreadStop(tree->siblings[proc]);
  Unlock(tree->lock);
#  if defined(DEBUGSMP)
  Lock(lock_io);
  Print(128, "thread %d (block %d) being stopped by beta cutoff.\n",
      tree->thread_id, FindBlockID(tree));
  Unlock(lock_io);
#  endif
}

/* modified 04/09/98 */
/*
 *******************************************************************************
 *                                                                             *
 *   ThreadWait() is the idle loop for the N threads that are created at the   *
 *   beginning when Crafty searches.  threads are "parked" here waiting on a   *
 *   pointer to something they should search (a parameter block built in the   *
 *   function Thread() in this case.  when this pointer becomes non-zero, each *
 *   thread "parked" here will immediately call SearchSMP() and begin the      *
 *   parallel search as directed.                                              *
 *                                                                             *
 *******************************************************************************
 */
int ThreadWait(int tid, TREE * RESTRICT waiting)
{
/*
 ************************************************************
 *                                                          *
 *   the N initial threads enter here and are kept penned   *
 *   here forever.  however, once a thread leaves here it   *
 *   may well re-enter ThreadWait() from the top while it   *
 *   waits for a parallel search to complete.  while it     *
 *   waits here it can also join in to help other busy      *
 *   threads search their subtrees as well.                 *
 *                                                          *
 ************************************************************
 */
  while (1) {
    Lock(lock_smp);
    cpu_time_used += ReadClock(cpu) - thread_start_time[tid];
    smp_idle++;
    Unlock(lock_smp);
#  if defined(DEBUGSMP)
    Lock(lock_io);
    Print(128, "thread %d now idle (%d procs, %d idle).\n", tid, max_threads,
        smp_idle);
    if (FindBlockID(waiting) >= 0)
      Print(128,
          "thread %d  waiting on block %d, still %d threads busy there\n", tid,
          FindBlockID(waiting), waiting->nprocs);
    Unlock(lock_io);
#  endif
/*
 ************************************************************
 *                                                          *
 *   we can exit if our thread[i] is non-zero, or if we are *
 *   waiting on others to finish a block that *we* have to  *
 *   return through.  when the busy count on such a block   *
 *   hits zero, we return immediately which unwinds the     *
 *   search as it should be.                                *
 *                                                          *
 ************************************************************
 */
    while (!thread[tid] && !quit && (!waiting || waiting->nprocs))
      Pause();
    if (quit)
      return (0);
    Lock(lock_smp);
    if (!thread[tid])
      thread[tid] = waiting;
/*
 ************************************************************
 *                                                          *
 *   we either have work to do, or threads we were waiting  *
 *   on have finished their work.                           *
 *                                                          *
 ************************************************************
 */
#  if defined(DEBUGSMP)
    Lock(lock_io);
    Print(128, "thread %d now has work at block %d.\n", tid,
        FindBlockID(thread[tid]));
    Unlock(lock_io);
#  endif
    smp_idle--;
/*
 ************************************************************
 *                                                          *
 *   if we are waiting on a block and the busy count is now *
 *   zero, we simply return to finish up the bookkeeping at *
 *   that point.                                            *
 *                                                          *
 ************************************************************
 */
    thread_start_time[tid] = ReadClock(cpu);
    Unlock(lock_smp);
    if (thread[tid] == waiting)
      return (0);
    if (quit || thread[tid] == (TREE *) - 1) {
      Lock(lock_io);
      Print(128, "thread %d exiting\n", tid);
      Unlock(lock_io);
      return (0);
    }
/*
 ************************************************************
 *                                                          *
 *   else our thread[i] pointer is non-zero, meaning we     *
 *   have been assigned something to search.  off we go,    *
 *   into the wild, blue yonder, up and away...  :)         *
 *                                                          *
 ************************************************************
 */
    SearchSMP(thread[tid], thread[tid]->alpha, thread[tid]->beta,
        thread[tid]->value, thread[tid]->wtm, thread[tid]->depth,
        thread[tid]->ply, thread[tid]->mate_threat, thread[tid]->lp_recapture);
    Lock(lock_smp);
    Lock(thread[tid]->parent->lock);
#  if defined(DEBUGSMP)
    Lock(lock_io);
    Print(128, "thread %d  block %d marked free at ply %d\n", tid,
        FindBlockID(thread[tid]), thread[tid]->ply);
    Unlock(lock_io);
#  endif
    CopyFromSMP((TREE *) thread[tid]->parent, thread[tid]);
    thread[tid]->parent->nprocs--;
#  if defined(DEBUGSMP)
    Lock(lock_io);
    Print(128, "thread %d decremented block %d  nprocs=%d\n", tid,
        FindBlockID(thread[tid]->parent), thread[tid]->parent->nprocs);
    Unlock(lock_io);
#  endif
    thread[tid]->parent->siblings[tid] = 0;
    Unlock(thread[tid]->parent->lock);
#  if defined(DEBUGSMP)
    Lock(lock_io);
    Print(128, "thread %d  block %d  parent %d  nprocs %d exit at ply %d\n",
        tid, FindBlockID(thread[tid]), FindBlockID(thread[tid]->parent),
        thread[tid]->parent->nprocs, thread[tid]->ply);
    Unlock(lock_io);
#  endif
    thread[tid] = 0;
    Unlock(lock_smp);
  }
}
#endif
