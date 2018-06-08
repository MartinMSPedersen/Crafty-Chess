#include "chess.h"
#include "data.h"
/* last modified 08/25/08 */
/*
 *******************************************************************************
 *                                                                             *
 *   QuiesceChecks() is the recursive routine used to implement the alpha/beta *
 *   negamax search (similar to minimax but simpler to code.)  QuiesceChecks() *
 *   is called at leaf nodes and tries the usual captures, plus any other moves*
 *   that give check.  If a move searched is a check, the next ply will be a   *
 *   full-width search to escape the check.  Once QuiesceChecks() has been     *
 *   called in any path, it will not be called again.                          *
 *                                                                             *
 *******************************************************************************
 */
int QuiesceChecks(TREE * RESTRICT tree, int alpha, int beta, int wtm, int ply)
{
  register int o_alpha, value;
  register int *next_move;
  register int *goodmv, *movep, moves = 0, *sortv, temp;

/*
 ************************************************************
 *                                                          *
 *   initialize.                                            *
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
#endif
/*
 ************************************************************
 *                                                          *
 *   check for draw by repetition.                          *
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
  if (tree->thread_id == 0)
    next_time_check--;
  o_alpha = alpha;
/*
 ************************************************************
 *                                                          *
 *   now call Evaluate() to produce the "stand-pat" score   *
 *   that will be returned if no capture is acceptable.     *
 *   if this score is > alpha, then we also have to save    *
 *   the "path" to this node as it is the PV that leads     *
 *   to this score.                                         *
 *                                                          *
 ************************************************************
 */
  value = Evaluate(tree, ply, wtm, alpha, beta);
#if defined(TRACE)
  if (trace_level >= 99)
    Trace(tree, ply, value, wtm, alpha, beta, "qchecks", EVALUATION);
#endif
  if (value > alpha) {
    if (value >= beta)
      return (value);
    alpha = value;
    tree->pv[ply].pathl = ply - 1;
    tree->pv[ply].pathh = 0;
    tree->pv[ply].pathd = iteration_depth;
  }
/*
 ************************************************************
 *                                                          *
 *   generate moves and sort them based on (a) the value    *
 *   of the captured piece - the value of the capturing     *
 *   piece if this is > 0; or, (b) the value returned by    *
 *   Swap().  if the capture leaves the opponent with no    *
 *   minor pieces, then we search that capture always since *
 *   the endgame might be won or lost with no pieces left.  *
 *   after we order the captures, we tack the moves that    *
 *   give check onto the end of the move list so that they  *
 *   are searched last.                                     *
 *                                                          *
 ************************************************************
 */
  tree->last[ply] = GenerateCaptures(tree, ply, wtm, tree->last[ply - 1]);
  goodmv = tree->last[ply - 1];
  sortv = tree->sort_value;
  for (movep = tree->last[ply - 1]; movep < tree->last[ply]; movep++) {
    if (Captured(*movep) == king)
      return (beta);
    if (pc_values[Piece(*movep)] < pc_values[Captured(*movep)] ||
        ((wtm) ? TotalPieces(black, occupied) : TotalPieces(white,
                occupied)) - p_vals[Captured(*movep)] == 0) {
      *goodmv++ = *movep;
      *sortv++ = pc_values[Captured(*movep)];
      moves++;
    } else {
      temp = Swap(tree, From(*movep), To(*movep), wtm);
      if (temp >= 0) {
        *goodmv++ = *movep;
        *sortv++ = temp;
        moves++;
      }
    }
  }
/*
 ************************************************************
 *                                                          *
 *   don't disdain the lowly bubble sort here.  the list of *
 *   captures is always short, and experiments with other   *
 *   algorithms are always slightly slower.  this is very   *
 *   cache-friendly and runs quickly.                       *
 *                                                          *
 ************************************************************
 */
  if (moves > 1) {
    register int done;
    register int *end = tree->last[ply - 1] + moves - 1;

    do {
      done = 1;
      sortv = tree->sort_value;
      for (movep = tree->last[ply - 1]; movep < end; movep++, sortv++)
        if (*sortv < *(sortv + 1)) {
          temp = *sortv;
          *sortv = *(sortv + 1);
          *(sortv + 1) = temp;
          temp = *movep;
          *movep = *(movep + 1);
          *(movep + 1) = temp;
          done = 0;
        }
    } while (!done);
  }
  next_move = tree->last[ply - 1];
/*
 ************************************************************
 *                                                          *
 *   now iterate through the move list and search the       *
 *   resulting positions.                                   *
 *                                                          *
 ************************************************************
 */
  while (moves--) {
    tree->curmv[ply] = *(next_move++);
#if defined(TRACE)
    if (ply <= trace_level)
      Trace(tree, ply, 0, wtm, alpha, beta, "qchecks", CAPTURE_MOVES);
#endif
    MakeMove(tree, ply, tree->curmv[ply], wtm);
    if (!Check(wtm)) {
      if (Check(Flip(wtm))) {
        tree->qsearch_check_extensions_done++;
        value = -QuiesceEvasions(tree, -beta, -alpha, Flip(wtm), ply + 1);
      } else
        value = -Quiesce(tree, -beta, -alpha, Flip(wtm), ply + 1);
    }
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
 *   now generate just the moves (non-captures) that give   *
 *   check and search the ones that Swap() says are safe.   *
 *                                                          *
 ************************************************************
 */
  tree->last[ply] = GenerateChecks(tree, ply, wtm, tree->last[ply - 1]);
  next_move = tree->last[ply - 1];
  moves = tree->last[ply] - tree->last[ply - 1];
/*
 ************************************************************
 *                                                          *
 *   now iterate through the move list and search the       *
 *   resulting positions.                                   *
 *                                                          *
 ************************************************************
 */
  while (moves--) {
    tree->curmv[ply] = *(next_move++);
    if (Swap(tree, From(tree->curmv[ply]), To(tree->curmv[ply]), wtm) >= 0) {
#if defined(TRACE)
      if (ply <= trace_level)
        Trace(tree, ply, 0, wtm, alpha, beta, "qchecks", CAPTURE_MOVES);
#endif
      MakeMove(tree, ply, tree->curmv[ply], wtm);
      if (!Check(wtm)) {
        if (Check(Flip(wtm))) {
          tree->qsearch_check_extensions_done++;
          value = -QuiesceEvasions(tree, -beta, -alpha, Flip(wtm), ply + 1);
        } else
          value = -Quiesce(tree, -beta, -alpha, Flip(wtm), ply + 1);
      }
      UnmakeMove(tree, ply, tree->curmv[ply], wtm);
      if (value > alpha) {
        if (value >= beta)
          return (value);
        alpha = value;
      }
      if (tree->stop)
        return (0);
    }
  }
/*
 ************************************************************
 *                                                          *
 *   all moves have been searched.  return the search       *
 *   result that was found.  if the result is not the       *
 *   original alpha score, then we need to return the PV    *
 *   that is associated with this score.                    *
 *                                                          *
 ************************************************************
 */
  if (alpha != o_alpha) {
    memcpy(&tree->pv[ply - 1].path[ply], &tree->pv[ply].path[ply],
        (tree->pv[ply].pathl - ply + 1) * sizeof(int));
    memcpy(&tree->pv[ply - 1].pathh, &tree->pv[ply].pathh, 3);
    tree->pv[ply - 1].path[ply - 1] = tree->curmv[ply - 1];
  }
  return (alpha);
}
