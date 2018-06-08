#include "chess.h"
#include "data.h"
#include "epdglue.h"
#define RAZOR_MARGIN ((queen_value+1)/2)
#define F_MARGIN ((bishop_value+1)/2)
/* modified 11/02/08 */
/*
 *******************************************************************************
 *                                                                             *
 *   SearchParallel() is the recursive routine used to implement alpha/beta    *
 *   negamax search using parallel threads.  when this code is called, the     *
 *   first move has already been searched, so all that is left is to search    *
 *   the remainder of the moves and then return.  note that the hash table and *
 *   such can't be modified here since this only represents a part of the      *
 *   search at this ply.                                                       *
 *                                                                             *
 *******************************************************************************
 */
int SearchParallel(TREE * RESTRICT tree, int alpha, int beta, int value,
    int wtm, int depth, int ply)
{
  register int fprune, extended, extensions;
  BITBOARD begin_root_nodes;

/*  
 ************************************************************
 *                                                          *
 *   now iterate through the move list and search the       *
 *   resulting positions.  note that Search() culls any     *
 *   move that is not legal by using Check().  the special  *
 *   case is that we must find one legal move to search to  *
 *   confirm that it's not a mate or draw.                  *
 *                                                          *
 *   first step is to see if we need to extend this move    *
 *   for some tactical reason.  If not, we check to see if  *
 *   we can reduce the depth (LMR) to save time.  A final   *
 *   case is to determine if we can use AEL pruning         *
 *   (Heinz 2000) near the search frontier.  Note for those *
 *   that have read Heinz's paper.  Frontier nodes in       *
 *   crafty have a depth = 2.  pre-frontier nodes  have a   *
 *   depth = 3.  And finally, pre-pre-frontier nodes have a *
 *   depth = 4.                                             *
 *                                                          *
 ************************************************************
 */
  while (1) {
    Lock(tree->parent->lock);
    if (ply == 1) {
      tree->phase[ply] = NextRootMove(tree->parent, tree, wtm);
      tree->root_move = tree->parent->root_move;
    } else
      tree->phase[ply] =
          (tree->inchk[ply]) ? NextEvasion((TREE *) tree->parent, ply,
          wtm) : NextMove((TREE *) tree->parent, ply, wtm);
    tree->curmv[ply] = tree->parent->curmv[ply];
    Unlock(tree->parent->lock);
    if (!tree->phase[ply])
      break;
#if defined(TRACE)
    if (ply <= trace_level)
      Trace(tree, ply, depth, wtm, alpha, beta, "SearchParallel",
          tree->phase[ply]);
#endif
    MakeMove(tree, ply, tree->curmv[ply], wtm);
    if (tree->inchk[ply] || !Check(wtm))
      do {
        extended = SearchExtensions(tree, wtm, ply, depth);
        extensions = extended - 1;
        begin_root_nodes = tree->nodes_searched;
        fprune = 0;
        if (extended <= 0 && !tree->inchk[ply] && !tree->inchk[ply + 1] &&
            abs(alpha) < (MATE - 500)) {
          if (depth < 5) {
            if (depth == 2) {
              if (MaterialSTM + futility_margin <= alpha)
                fprune = 1;
            } else if (depth == 3 && MaterialSTM + extended_futility_margin <= alpha)
              fprune = 1;
            else if (depth == 4 && MaterialSTM + razor_margin <= alpha)
              extensions -= 1;
          }
        }
        if (depth + extensions > 0 && !fprune) {
          value =
              -Search(tree, -alpha - 1, -alpha, Flip(wtm), depth + extensions,
              ply + 1, DO_NULL);
          if (value > alpha && extensions < -1)
            value =
                -Search(tree, -alpha - 1, -alpha, Flip(wtm), depth - 1,
                ply + 1, DO_NULL);
        } else
          value = -QuiesceChecks(tree, -alpha - 1, -alpha, Flip(wtm), ply + 1);
        if (abort_search || tree->stop)
          break;
        if (value > alpha && value < beta) {
          extensions = Max(extensions, -1);
          if (depth + extensions > 0)
            value =
                -Search(tree, -beta, -alpha, Flip(wtm), depth + extensions,
                ply + 1, DO_NULL);
          else
            value = -QuiesceChecks(tree, -beta, -alpha, Flip(wtm), ply + 1);
          if (abort_search || tree->stop)
            break;
        }
/*
 ************************************************************
 *                                                          *
 *   now we check for an undesirable case, that of failing  *
 *   high while doing a parallel (threaded) search.  this   *
 *   means our 'helpers' are doing stuff that is not needed *
 *   so we 'stop' them now.                                 *
 *                                                          *
 ************************************************************
 */
        if (ply == 1)
          root_moves[tree->root_move].nodes =
              tree->nodes_searched - begin_root_nodes;
        if (value > alpha) {
          alpha = value;
          if (ply == 1) {
            Lock(lock_root);
            if (value > root_value) {
              Output(tree, value, beta);
              root_value = value;
            }
            Unlock(lock_root);
          }
          if (value >= beta) {
            register int proc;

            parallel_aborts++;
            UnmakeMove(tree, ply, tree->curmv[ply], wtm);
            Lock(lock_smp);
            Lock(tree->parent->lock);
            if (!tree->stop) {
              for (proc = 0; proc < max_threads; proc++)
                if (tree->parent->siblings[proc] && proc != tree->thread_id)
                  ThreadStop(tree->parent->siblings[proc]);
            }
            Unlock(tree->parent->lock);
            Unlock(lock_smp);
            return (alpha);
          }
        }
      } while (0);
    UnmakeMove(tree, ply, tree->curmv[ply], wtm);
    if (abort_search || tree->stop)
      break;
  }
/*
 ************************************************************
 *                                                          *
 *   there are no "end-of-search" things to do.  We have    *
 *   searched all the remaining moves at this ply in        *
 *   parallel, and now return and let the original search   *
 *   (that started this sub-tree) clean up, and do the      *
 *   tests for mate/stalemate, update the hash table, etc.  *
 *                                                          *
 *   we do need to flag the root move we tried to search,   *
 *   if we were stopped early due to another root move      *
 *   failing high.  otherwise this move appears to have     *
 *   been searched already and will not be searched again   *
 *   until the next iteration.                              *
 *                                                          *
 ************************************************************
 */
  if (tree->stop && ply == 1)
    root_moves[tree->root_move].status &= 4095 - 256;
  return (alpha);
}
