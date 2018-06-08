#include "chess.h"
#include "data.h"
/* last modified 01/17/09 */
/*
 *******************************************************************************
 *                                                                             *
 *   Quiece() is the recursive routine used to implement the quiescence        *
 *   search part of the alpha/beta negamax search.  It has two essential       *
 *   functions:                                                                *
 *                                                                             *
 *   (1) It computes a stand-pat score, which gives the side-on-move the       *
 *   choice of standing pat and not playing any move at all and just accepting *
 *   the current static evaluation, or else it may try captures and/or         *
 *   checking moves to see if it can improve the stand-pat score by making a   *
 *   move that leads to some sort of positional or material gain.              *
 *                                                                             *
 *   (2) The first phase is to generate all possible capture moves and then    *
 *   use SEE (Static Exchange Evaluator) to screen out moves that appear to    *
 *   lose material, such as QxN where the N is defended and the resulting      *
 *   trade will lose material.  Any of these moves can improve the stand-pat   *
 *   score.                                                                    *
 *                                                                             *
 *******************************************************************************
 */
int Quiesce(TREE * RESTRICT tree, int alpha, int beta, int wtm, int ply)
{
  register int o_alpha, value;
  register int *next_move;
  register int *goodmv, *movep, moves = 0, *sortv, temp;

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
#endif
  if (tree->thread_id == 0)
    next_time_check--;
  tree->last[ply] = tree->last[ply - 1];
  o_alpha = alpha;
/*
 ************************************************************
 *                                                          *
 *   Now call Evaluate() to produce the "stand-pat" score   *
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
    Trace(tree, ply, value, wtm, alpha, beta, "Quiesce", EVALUATION);
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
 *   Generate captures and sort them based on (a) the value *
 *   of the captured piece - the value of the capturing     *
 *   piece if this is > 0; or, (b) the value returned by    *
 *   Swap().  If the capture leaves the opponent with no    *
 *   minor pieces, then we search that capture always since *
 *   the endgame might be won or lost with no pieces left.  *
 *                                                          *
 *   Once we confirm that the capture is not losing any     *
 *   material, we sort these non-losing captures into       *
 *   MVV/LVA order which appears to be a slightly faster    *
 *   move ordering idea.                                    *
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
      *sortv++ = 128 * pc_values[Captured(*movep)] - pc_values[Piece(*movep)];
      moves++;
    } else {
      temp = Swap(tree, From(*movep), To(*movep), wtm);
      if (temp >= 0) {
        *goodmv++ = *movep;
        *sortv++ = 128 * pc_values[Captured(*movep)] - pc_values[Piece(*movep)];
        moves++;
      }
    }
  }
  if (!moves) {
    if (alpha != o_alpha) {
      memcpy(&tree->pv[ply - 1].path[ply], &tree->pv[ply].path[ply],
          (tree->pv[ply].pathl - ply + 1) * sizeof(int));
      memcpy(&tree->pv[ply - 1].pathh, &tree->pv[ply].pathh, 3);
      tree->pv[ply - 1].path[ply - 1] = tree->curmv[ply - 1];
    }
    return (value);
  }
/*
 ************************************************************
 *                                                          *
 *   Don't disdain the lowly bubble sort here.  The list of *
 *   captures is always short, and experiments with other   *
 *   algorithms are always slightly slower.  This is very   *
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
 *   Iterate through the move list and search the resulting *
 *   positions.                                             *
 *                                                          *
 ************************************************************
 */
  while (moves--) {
    tree->curmv[ply] = *(next_move++);
#if defined(TRACE)
    if (ply <= trace_level)
      Trace(tree, ply, 0, wtm, alpha, beta, "Quiesce", CAPTURE_MOVES);
#endif
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
 *   All moves have been searched.  Return the search       *
 *   result that was found.  If the result is not the       *
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

/* last modified 01/17/09 */
/*
 *******************************************************************************
 *                                                                             *
 *   QuieceChecks() is the recursive routine used to implement the quiescence  *
 *   search part of the alpha/beta negamax search.  It has three essential     *
 *   functions:                                                                *
 *                                                                             *
 *   (1) It computes a stand-pat score, which gives the side-on-move the       *
 *   choice of standing pat and not playing any move at all and just accepting *
 *   the current static evaluation, or else it may try captures and/or         *
 *   checking moves to see if it can improve the stand-pat score by making a   *
 *   move that leads to some sort of positional or material gain.              *
 *                                                                             *
 *   (2) The first phase is to generate all possible capture moves and then    *
 *   use SEE (Static Exchange Evaluator) to screen out moves that appear to    *
 *   lose material, such as QxN where the N is defended and the resulting      *
 *   trade will lose material.  Any of these moves can improve the stand-pat   *
 *   score.                                                                    *
 *                                                                             *
 *   (3) After captures have been tried then we will try adding on the non-    *
 *   capture checking moves to see if one of those will improve on the stand-  *
 *   pat score since checking moves often expose dangerous weaknesses that a   *
 *   static evaluation might miss.                                             *
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
  if (tree->thread_id == 0)
    next_time_check--;
  o_alpha = alpha;
/*
 ************************************************************
 *                                                          *
 *   Now call Evaluate() to produce the "stand-pat" score   *
 *   that will be returned if no capture is acceptable.     *
 *   If this score is > alpha, then we also have to save    *
 *   the "path" to this node as it is the PV that leads     *
 *   to this score.                                         *
 *                                                          *
 ************************************************************
 */
  value = Evaluate(tree, ply, wtm, alpha, beta);
#if defined(TRACE)
  if (trace_level >= 99)
    Trace(tree, ply, value, wtm, alpha, beta, "QuiesceChecks", EVALUATION);
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
 *   Generate captures and sort them based on (a) the value *
 *   of the captured piece - the value of the capturing     *
 *   piece if this is > 0; or, (b) the value returned by    *
 *   Swap().  If the capture leaves the opponent with no    *
 *   minor pieces, then we search that capture always since *
 *   the endgame might be won or lost with no pieces left.  *
 *                                                          *
 *   Once we confirm that the capture is not losing any     *
 *   material, we sort these non-losing captures into       *
 *   MVV/LVA order which appears to be a slightly faster    *
 *   move ordering idea.                                    *
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
      *sortv++ = 128 * pc_values[Captured(*movep)] - pc_values[Piece(*movep)];
      moves++;
    } else {
      temp = Swap(tree, From(*movep), To(*movep), wtm);
      if (temp >= 0) {
        *goodmv++ = *movep;
        *sortv++ = 128 * pc_values[Captured(*movep)] - pc_values[Piece(*movep)];
        moves++;
      }
    }
  }
/*
 ************************************************************
 *                                                          *
 *   Don't disdain the lowly bubble sort here.  The list of *
 *   captures is always short, and experiments with other   *
 *   algorithms are always slightly slower.  This is very   *
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
 *   iterate through the move list and search the resulting *
 *   positions.                                             *
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
 *   Generate just the moves (non-captures) that give check *
 *   and search the ones that Swap() says are safe.         *
 *                                                          *
 ************************************************************
 */
  tree->last[ply] = GenerateChecks(tree, ply, wtm, tree->last[ply - 1]);
  next_move = tree->last[ply - 1];
  moves = tree->last[ply] - tree->last[ply - 1];
/*
 ************************************************************
 *                                                          *
 *   iterate through the move list and search the resulting *
 *   positions.                                             *
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
        tree->qsearch_check_extensions_done++;
        value = -QuiesceEvasions(tree, -beta, -alpha, Flip(wtm), ply + 1);
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
 *   All moves have been searched.  Return the search       *
 *   result that was found.  If the result is not the       *
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

/* last modified 01/17/09 */
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
