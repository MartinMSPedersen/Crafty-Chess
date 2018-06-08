#include "chess.h"
#include "data.h"
/* last modified 07/26/09 */
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
 *   sort them into descending using the value                                 *
 *                                                                             *
 *        val = 128 * captured_piece_value + capturing_piece_value             *
 *                                                                             *
 *   This is the classic MVV/LVA ordering approach that removes heavy pieces   *
 *   first in an attempt to reduce the size of the sub-tree this capture       *
 *   produces.                                                                 *
 *                                                                             *
 *   (3) When we get ready to actually search each capture, we use Swap() to   *
 *   compute the SEE score.  If this is less than zero, we do not search this  *
 *   move at all to avoid wasting time, since a losing capture rarely helps    *
 *   improve the score in the q-search.  The goal here is to find a capture    *
 *   that improves on the stand-pat score and get us closer to a position that *
 *   we would describe as "quiet" or "static".                                 *
 *                                                                             *
 *******************************************************************************
 */
int Quiesce(TREE * RESTRICT tree, int alpha, int beta, int wtm, int ply) {
  register int o_alpha, value;
  register int *next_move;
  register int *movep, *sortv, temp;

/*
 ************************************************************
 *                                                          *
 *   Initialize.                                            *
 *                                                          *
 ************************************************************
 */
  if (ply >= MAXPLY - 1)
    return (beta);
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
 *   Generate captures and sort them based on simple        *
 *   MVV/LVA order.  We simply try to capture the most      *
 *   valuable piece possible, using the least valuable      *
 *   attacker possible, to get rid of heavy pieces quickly  *
 *   and reduce the overall size of the tree.               *
 *                                                          *
 *   Note that later we use the value of the capturing      *
 *   piece, the value of the captured piece, and possibly   *
 *   Swap() to exclude captures that appear to lose         *
 *   material, but we delay expending this effort as long   *
 *   as possible, hoping a beta cutoff will avoid most of   *
 *   the work completely.                                   *
 *                                                          *
 ************************************************************
 */
  tree->last[ply] = GenerateCaptures(tree, ply, wtm, tree->last[ply - 1]);
  sortv = tree->sort_value;
  for (movep = tree->last[ply - 1]; movep < tree->last[ply]; movep++) {
    if (Captured(*movep) == king)
      return (beta);
    *sortv++ = 128 * pc_values[Captured(*movep)] - pc_values[Piece(*movep)];
  }
  if (tree->last[ply] == tree->last[ply - 1]) {
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
  if (tree->last[ply] > tree->last[ply - 1] + 1) {
    register int done;

    do {
      done = 1;
      sortv = tree->sort_value;
      for (movep = tree->last[ply - 1]; movep < tree->last[ply] - 1;
          movep++, sortv++)
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
 *   positions.  Now that we are ready to actually search   *
 *   the set of capturing moves, we try three quick tests   *
 *   to see if the move should be excluded because it       *
 *   appears to lose material.                              * 
 *                                                          *
 *   (1) If the capture removes the last opponent piece, we *
 *   always search this kind of capture since this can be   *
 *   the move the allows a passed pawn to promote when the  *
 *   opponent has no piece to catch it.                     *
 *                                                          *
 *   (2) If the capturing piece is not more valuable than   *
 *   the captured piece, then the move can't lose material  *
 *   and should be searched.                                *
 *                                                          *
 *   (3) Otherwise, If the capturing piece is more valuable *
 *   than the captured piece, we use Swap() to determine if *
 *   the capture is losing or not so we don't search        *
 *   hopeless moves.                                        *
 *                                                          *
 ************************************************************
 */
  while (next_move < tree->last[ply]) {
    tree->curmv[ply] = *(next_move++);
    if (pc_values[Piece(tree->curmv[ply])] >
        pc_values[Captured(tree->curmv[ply])] && TotalPieces(wtm, occupied)
        - p_vals[Captured(tree->curmv[ply])] > 0 &&
        Swap(tree, tree->curmv[ply], wtm) < 0)
      continue;
#if defined(TRACE)
    if (ply <= trace_level)
      Trace(tree, ply, 0, wtm, alpha, beta, "Quiesce", CAPTURE_MOVES);
#endif
    MakeMove(tree, ply, tree->curmv[ply], wtm);
    tree->nodes_searched++;
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

/* last modified 07/26/09 */
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
 *   sort them into descending using the value                                 *
 *                                                                             *
 *        val = 128 * captured_piece_value + capturing_piece_value             *
 *                                                                             *
 *   This is the classic MVV/LVA ordering approach that removes heavy pieces   *
 *   first in an attempt to reduce the size of the sub-tree this capture       *
 *   produces.                                                                 *
 *                                                                             *
 *   (3) When we get ready to actually search each capture, we use Swap() to   *
 *   compute the SEE score.  If this is less than zero, we do not search this  *
 *   move at all to avoid wasting time, since a losing capture rarely helps    *
 *   improve the score in the q-search.  The goal here is to find a capture    *
 *   that improves on the stand-pat score and get us closer to a position that *
 *   we would describe as "quiet" or "static".                                 *
 *                                                                             *
 *   (4) We "stage" the moves in this order, hoping that a normal capture will *
 *   be good enough to produce a beta cutoff and get us out of here before we  *
 *   go to the trouble of generating checking moves, which add to the size of  *
 *   the tree since the effort of legally escaping checks often require more   *
 *   than just a simple capture move.                                          *
 *                                                                             *
 *   (5) After captures have been tried then we will try adding on the non-    *
 *   capture checking moves to see if one of those will improve on the stand-  *
 *   pat score since checking moves often expose dangerous weaknesses that a   *
 *   static evaluation might miss.                                             *
 *                                                                             *
 *   If any move searched here checks the opponent, rather than calling        *
 *   Quiesce() to find a reply, we call QuiesceEvasions() instead, which will  *
 *   try all moves (without having a stand-pat option) to make sure that we do *
 *   mate the opponent with our checking move.  This happens whether the move  *
 *   we try is a capture or not, it just has to give check to trigger this     *
 *   action.                                                                   *
 *                                                                             *
 *******************************************************************************
 */
int QuiesceChecks(TREE * RESTRICT tree, int alpha, int beta, int wtm, int ply) {
  register int o_alpha, value;
  register int *next_move;
  register int *movep, *sortv, temp;

/*
 ************************************************************
 *                                                          *
 *   Initialize.                                            *
 *                                                          *
 ************************************************************
 */
  if (ply >= MAXPLY - 1)
    return (beta);
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
 *   Generate captures and sort them based on simple        *
 *   MVV/LVA order.  We simply try to capture the most      *
 *   valuable piece possible, using the least valuable      *
 *   attacker possible, to get rid of heavy pieces quickly  *
 *   and reduce the overall size of the tree.               *
 *                                                          *
 *   Note that later we use the value of the capturing      *
 *   piece, the value of the captured piece, and possibly   *
 *   Swap() to exclude captures that appear to lose         *
 *   material, but we delay expending this effort as long   *
 *   as possible, hoping a beta cutoff will avoid most of   *
 *   the work completely.                                   *
 *                                                          *
 ************************************************************
 */
  tree->last[ply] = GenerateCaptures(tree, ply, wtm, tree->last[ply - 1]);
  sortv = tree->sort_value;
  for (movep = tree->last[ply - 1]; movep < tree->last[ply]; movep++) {
    if (Captured(*movep) == king)
      return (beta);
    *sortv++ = 128 * pc_values[Captured(*movep)] - pc_values[Piece(*movep)];
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
  if (tree->last[ply] > tree->last[ply - 1] + 1) {
    register int done;

    do {
      done = 1;
      sortv = tree->sort_value;
      for (movep = tree->last[ply - 1]; movep < tree->last[ply] - 1;
          movep++, sortv++)
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
 *   positions.  Now that we are ready to actually search   *
 *   the set of capturing moves, we try three quick tests   *
 *   to see if the move should be excluded because it       *
 *   appears to lose material.                              * 
 *                                                          *
 *   (1) If the capture removes the last opponent piece, we *
 *   always search this kind of capture since this can be   *
 *   the move the allows a passed pawn to promote when the  *
 *   opponent has no piece to catch it.                     *
 *                                                          *
 *   (2) If the capturing piece is not more valuable than   *
 *   the captured piece, then the move can't lose material  *
 *   and should be searched.                                *
 *                                                          *
 *   (3) Otherwise, If the capturing piece is more valuable *
 *   than the captured piece, we use Swap() to determine if *
 *   the capture is losing or not so we don't search        *
 *   hopeless moves.                                        *
 *                                                          *
 ************************************************************
 */
  while (next_move < tree->last[ply]) {
    tree->curmv[ply] = *(next_move++);
    if (pc_values[Piece(tree->curmv[ply])] >
        pc_values[Captured(tree->curmv[ply])] &&
        TotalPieces(wtm, occupied) - p_vals[Captured(tree->curmv[ply])] > 0 &&
        Swap(tree, tree->curmv[ply], wtm) < 0)
      continue;
#if defined(TRACE)
    if (ply <= trace_level)
      Trace(tree, ply, 0, wtm, alpha, beta, "qchecks", CAPTURE_MOVES);
#endif
    MakeMove(tree, ply, tree->curmv[ply], wtm);
    tree->nodes_searched++;
    if (!Check(wtm)) {
      if (Check(Flip(wtm))) {
        tree->qchecks_done++;
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
/*
 ************************************************************
 *                                                          *
 *   iterate through the move list and search the resulting *
 *   positions.                                             *
 *                                                          *
 ************************************************************
 */
  while (next_move < tree->last[ply]) {
    tree->curmv[ply] = *(next_move++);
    if (Swap(tree, tree->curmv[ply], wtm) >= 0) {
#if defined(TRACE)
      if (ply <= trace_level)
        Trace(tree, ply, 0, wtm, alpha, beta, "qchecks", CAPTURE_MOVES);
#endif
      MakeMove(tree, ply, tree->curmv[ply], wtm);
      tree->nodes_searched++;
      if (!Check(wtm)) {
        tree->qchecks_done++;
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
 *   beta negamax quiescence search.  The primary function here is to escape a *
 *   check that was delivered by QuiesceChecks() at the previous ply.  We do   *
 *   not have the usual "stand pat" option because we have to find a legal     *
 *   move to prove we have not been checkmated.                                *
 *                                                                             *
 *   QuiesceEvasions() uses the legal move generator (GenerateCheckEvasions()) *
 *   to produce only the set of legal moves that escape check.  We try those   *
 *   in the the usual MVV/LVA order, except here we do not skip a move just    *
 *   because it appears to lose material, as we are trying to make sure we do  *
 *   lose the king instead.                                                    *
 *                                                                             *
 *******************************************************************************
 */
int QuiesceEvasions(TREE * RESTRICT tree, int alpha, int beta, int wtm,
    int ply) {
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
    tree->nodes_searched++;
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
