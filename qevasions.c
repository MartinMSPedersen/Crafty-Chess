#include "chess.h"
#include "data.h"
/* last modified 12/04/08 */
/*
 *******************************************************************************
 *                                                                             *
 *   QuiesceEvasions() is the recursive routine used to implement the alpha/   *
 *   beta negamax search (similar to minimax but simpler to code.)             *
 *   QuiesceChecks() is called at leaf nodes and tries the usual captures,     *
 *   plus any other moves that give check.  If a move searched is a check, the *
 *   next ply will be a full-width search to escape the check.  Once           *
 *   QuiesceChecks() has been called in any path, it will not be called again. *
 *                                                                             *
 *******************************************************************************
 */
int QuiesceEvasions(TREE * RESTRICT tree, int alpha, int beta, int wtm, int ply)
{
  register int o_alpha, value;
  register int moves_searched = 0;

/*
 ************************************************************
 *                                                          *
 *   Initialize.                                            *
 *                                                          *
 ************************************************************
 */
  if (ply >= MAXPLY - 1)
    return (beta);
  tree->nodes_searched++;
#if defined(NODES)
  temp_search_nodes--;
  if (temp_search_nodes <= 0) {
    time_abort++;
    abort_search = 1;
    return (0);
  }
  if (tree->thread_id == 0)
    next_time_check--;
#endif
/*
 ************************************************************
 *                                                          *
 *   Check for draw by repetition.                          *
 *                                                          *
 ************************************************************
 */
  if (RepetitionCheck(tree, ply, wtm)) {
    value = DrawScore(wtm);
    if (value < beta)
      SavePV(tree, ply, 0);
#if defined(TRACE)
    if (ply <= trace_level)
      printf("draw by repetition detected, ply=%d.\n", ply);
#endif
    return (value);
  }
  tree->last[ply] = tree->last[ply - 1];
  o_alpha = alpha;
  tree->next_status[ply].phase = HASH_MOVE;
  tree->hash_move[ply] = 0;
/*
 ************************************************************
 *                                                          *
 *   Iterate through the move list and search the resulting *
 *   positions.                                             *
 *                                                          *
 ************************************************************
 */
  while ((tree->phase[ply] = NextEvasion(tree, ply, wtm))) {
#if defined(TRACE)
    if (ply <= trace_level)
      Trace(tree, ply, 0, wtm, alpha, beta, "qevasions", tree->phase[ply]);
#endif
    moves_searched++;
    MakeMove(tree, ply, tree->curmv[ply], wtm);
    value = -Quiesce(tree, -beta, -alpha, Flip(wtm), ply + 1);
    UnmakeMove(tree, ply, tree->curmv[ply], wtm);
    if (value > alpha) {
      if (value >= beta)
        return (value);
      alpha = value;
    }
    if (tree->stop)
      return (0);
  }
/*
 ************************************************************
 *                                                          *
 *   All moves have been searched.  If none were legal,     *
 *   return either MATE or DRAW depending on whether the    *
 *   side to move is in check or not.                       *
 *                                                          *
 ************************************************************
 */
  if (moves_searched == 0) {
    value = (Check(wtm)) ? -(MATE - ply) : DrawScore(wtm);
    if (value >= alpha && value < beta) {
      SavePV(tree, ply, 0);
#if defined(TRACE)
      if (ply <= trace_level)
        printf("Search() no moves!  ply=%d\n", ply);
#endif
    }
    return (value);
  } else if (alpha != o_alpha) {
    memcpy(&tree->pv[ply - 1].path[ply], &tree->pv[ply].path[ply],
        (tree->pv[ply].pathl - ply + 1) * sizeof(int));
    memcpy(&tree->pv[ply - 1].pathh, &tree->pv[ply].pathh, 3);
    tree->pv[ply - 1].path[ply - 1] = tree->curmv[ply - 1];
  }
  return (alpha);
}
