#include "chess.h"
#include "data.h"
#include "epdglue.h"
/* last modified 11/02/08 */
/*
 *******************************************************************************
 *                                                                             *
 *   Search() is the recursive routine used to implement the alpha/beta        *
 *   negamax search (similar to minimax but simpler to code.)  Search() is     *
 *   called whenever there is "depth" remaining so that all moves are subject  *
 *   to searching.  Search() recursively calls itself so long as there is at   *
 *   least one ply of depth left, otherwise it calls QuiesceChecks() instead.  *
 *                                                                             *
 *******************************************************************************
 */
int Search(TREE * RESTRICT tree, int alpha, int beta, int wtm, int depth,
    int ply, int do_null)
{
  register int moves_searched = 0;
  register int o_alpha, value = 0, t_beta = beta;
  register int extensions, extended, pieces;
  int fprune;

/*
 ************************************************************
 *                                                          *
 *   check to see if we have searched enough nodes that it  *
 *   is time to peek at how much time has been used, or if  *
 *   is time to check for operator keyboard input.  this is *
 *   usually enough nodes to force a time/input check about *
 *   once per second, except when the target time per move  *
 *   is very small, in which case we try to check the time  *
 *   at least 10 times during the search.                   *
 *                                                          *
 ************************************************************
 */
  tree->nodes_searched++;
#if defined(NODES)
  temp_search_nodes--;
  if (temp_search_nodes <= 0) {
    time_abort++;
    abort_search = 1;
    return (0);
  }
#endif
  if (tree->thread_id == 0) {
    if (--next_time_check <= 0) {
      next_time_check = nodes_between_time_checks;
      if (CheckInput())
        Interrupt(ply);
      if (TimeCheck(tree, 0)) {
        time_abort++;
        abort_search = 1;
        return (0);
      }
    }
  }
  if (ply >= MAXPLY - 1)
    return (beta);
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
/*
 ************************************************************
 *                                                          *
 *   now call HashProbe() to see if this position has been  *
 *   searched before.  if so, we may get a real score,      *
 *   produce a cutoff, or get nothing more than a good move *
 *   to try first.  there are four cases to handle:         *
 *                                                          *
 *   1. HashProbe() returns "EXACT" if this score is        *
 *   greater than beta, return beta.  otherwise, return the *
 *   score.  In either case, no further searching is needed *
 *   from this position.  note that lookup verified that    *
 *   the table position has sufficient "draft" to meet the  *
 *   requirements of the current search depth remaining.    *
 *                                                          *
 *   2. HashProbe() returns "UPPER" which means that when   *
 *   this position was searched previously, every move was  *
 *   "refuted" by one of its descendents.  as a result,     *
 *   when the search was completed, we returned alpha at    *
 *   that point.  we simply return alpha here as well.      *
 *                                                          *
 *   3. HashProbe() returns "LOWER" which means that when   *
 *   we encountered this position before, we searched one   *
 *   branch (probably) which promptly refuted the move at   *
 *   the previous ply.                                      *
 *                                                          *
 *   4. HashProbe() returns "AVOID_NULL_MOVE" which means   *
 *   the hashed score/bound was no good, but it indicated   *
 *   that trying a null-move in this position would be a    *
 *   waste of time since it will likely fail low, not high. *
 *                                                          *
 ************************************************************
 */
  switch (HashProbe(tree, ply, depth, wtm, &alpha, beta)) {
  case EXACT:
    if (alpha < beta)
      SavePV(tree, ply, 1);
    return (alpha);
  case EXACTEGTB:
    if (alpha < beta)
      SavePV(tree, ply, 2);
    return (alpha);
  case LOWER:
    return (beta);
  case UPPER:
    return (alpha);
  case AVOID_NULL_MOVE:
    do_null = 0;
  }
/*
 ************************************************************
 *                                                          *
 *   now it's time to try a probe into the endgame table-   *
 *   base files.  this is done if we notice that there are  *
 *   6 or fewer pieces left on the board.  EGTB_use tells   *
 *   us how many pieces to probe on.  note that this can be *
 *   zero when trying to swindle the opponent, so that no   *
 *   probes are done since we know it is a draw.            *
 *                                                          *
 ************************************************************
 */
#if !defined(NOEGTB)
  if (ply <= iteration_depth && TotalAllPieces <= EGTB_use &&
      Castle(ply, white) + Castle(ply, black) == 0 &&
      (CaptureOrPromote(tree->curmv[ply - 1]) || ply < 3)) {
    int egtb_value;

    tree->egtb_probes++;
    if (EGTBProbe(tree, ply, wtm, &egtb_value)) {
      tree->egtb_probes_successful++;
      alpha = egtb_value;
      if (abs(alpha) > MATE - 300)
        alpha += (alpha > 0) ? -ply + 1 : ply;
      else if (alpha == 0) {
        alpha = DrawScore(wtm);
        if (Material > 0)
          alpha += (wtm) ? 1 : -1;
        else if (Material < 0)
          alpha -= (wtm) ? 1 : -1;
      }
      if (alpha < beta)
        SavePV(tree, ply, 2);
      tree->pv[ply].pathl = 0;
      HashStore(tree, ply, MAX_DRAFT, wtm, EXACT, alpha);
      return (alpha);
    }
  }
#endif
/*
 ************************************************************
 *                                                          *
 *  first, we try a null move to see if we can get a quick  *
 *  cutoff with only a little work.  this operates as       *
 *  follows.  instead of making a legal move, the side on   *
 *  move 'passes' and does nothing.  the resulting position *
 *  is searched to a shallower depth than normal (usually   *
 *  3 plies less but settable by the operator) this will    *
 *  result in a cutoff if our position is very good, but it *
 *  produces the cutoff much quicker since the search is    *
 *  far shallower than a normal search that would also be   *
 *  likely to fail high.                                    *
 *                                                          *
 *  this is skipped for any of the following reasons:       *
 *                                                          *
 *  1.  the side on move is in check.  the null move        *
 *      results in an illegal position.                     *
 *  2.  no more than one null move can appear in succession *
 *      or else the search will degenerate into nothing.    *
 *  3.  the side on move has little material left making    *
 *      zugzwang positions more likely.                     *
 *  4.  the transposition table probe found an entry that   *
 *      indicates that a null-move search will not fail     *
 *      high, so we avoid the wasted effort.                *
 *  5.  if the alpha/beta window is non-null, this is a PV  *
 *      node where the null move should be avoided.         *
 *                                                          *
 *  the null-move search is also used to detect certain     *
 *  types of threats.  the original idea of using the value *
 *  returned by the null-move search was reported by C.     *
 *  Donninger, but was modified by Bruce Moreland (Ferret)  *
 *  in the following way:  if the null-move search returns  *
 *  a score that says "mated in 1" then this position is a  *
 *  dangerous one, because not moving gets the side to move *
 *  mated.  we extend the search in this case, although as  *
 *  always, no more than one ply of extensions is allowed   *
 *  at any one level in the tree.  note also that this      *
 *  "threat" condition is hashed so that later, if the hash *
 *  table says "don't try the null move because it likely   *
 *  will fail low, we still know that this is a threat      *
 *  position and that it should be extended.                *
 *                                                          *
 ************************************************************
 */
  tree->inchk[ply + 1] = 0;
  o_alpha = alpha;
  tree->last[ply] = tree->last[ply - 1];
  pieces = (wtm) ? TotalPieces(white, occupied) : TotalPieces(black, occupied);
  if (do_null && alpha == beta - 1) {
    if (!tree->inchk[ply] && pieces && (pieces > 9 || depth < 7)) {
      register BITBOARD save_hash_key;

      tree->curmv[ply] = 0;
      tree->phase[ply] = NULL_MOVE;
#if defined(TRACE)
      if (ply <= trace_level)
        Trace(tree, ply, depth, wtm, beta - 1, beta, "Search1", 0);
#endif
      tree->position[ply + 1] = tree->position[ply];
      Rule50Moves(ply + 1) = 0;
      save_hash_key = HashKey;
      if (EnPassant(ply)) {
        HashEP(EnPassant(ply + 1), HashKey);
        EnPassant(ply + 1) = 0;
      }
      if (depth - null_depth - 1 > 0)
        value =
            -Search(tree, -beta, 1 - beta, Flip(wtm), depth - null_depth - 1,
            ply + 1, NO_NULL);
      else
        value = -QuiesceChecks(tree, -beta, 1 - beta, Flip(wtm), ply + 1);
      HashKey = save_hash_key;
      if (abort_search || tree->stop)
        return (0);
      if (value >= beta) {
        HashStore(tree, ply, depth, wtm, LOWER, value);
        return (value);
      }
    }
  }
/*
 ************************************************************
 *                                                          *
 *   if there is no best move from the hash table, and this *
 *   is a PV node, then we need a good move to search       *
 *   first.  while killers and history moves are good, they *
 *   are not "good enough".  the simplest action is to try  *
 *   a shallow search (depth-2) to get a move.  note that   *
 *   when we call Search() with depth-2, it, too, will      *
 *   not have a hash move, and will therefore recursively   *
 *   continue this process, hence the name "internal        *
 *   iterative deepening."                                  *
 *                                                          *
 ************************************************************
 */
  tree->next_status[ply].phase = HASH_MOVE;
  if (tree->hash_move[ply] == 0 && do_null && depth >= 3)
    do {
      int abound = (ply & 1) ? root_alpha : -root_beta;
      int bbound = (ply & 1) ? root_beta : -root_alpha;

      if (alpha != abound || beta != bbound)
        break;
      tree->curmv[ply] = 0;
      if (depth - 2 > 0)
        value = Search(tree, alpha, beta, wtm, depth - 2, ply, DO_NULL);
      else
        value = QuiesceChecks(tree, alpha, beta, wtm, ply);
      if (abort_search || tree->stop)
        return (0);
      if (value <= alpha) {
        if (depth - 2 > 0)
          value = Search(tree, -MATE, beta, wtm, depth - 2, ply, DO_NULL);
        else
          value = QuiesceChecks(tree, -MATE, beta, wtm, ply);
        if (abort_search || tree->stop)
          return (0);
      }
      if (value < beta) {
        if ((int) tree->pv[ply - 1].pathl >= ply)
          tree->hash_move[ply] = tree->pv[ply - 1].path[ply];
      } else
        tree->hash_move[ply] = tree->curmv[ply];
      tree->last[ply] = tree->last[ply - 1];
      tree->next_status[ply].phase = HASH_MOVE;
    } while (0);
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
  while ((tree->phase[ply] =
          (tree->inchk[ply]) ? NextEvasion(tree, ply, wtm) : NextMove(tree, ply,
              wtm))) {
#if defined(TRACE)
    if (ply <= trace_level)
      Trace(tree, ply, depth, wtm, alpha, beta, "Search2", tree->phase[ply]);
#endif
    MakeMove(tree, ply, tree->curmv[ply], wtm);
    if (tree->inchk[ply] || !Check(wtm))
      do {
        extended = SearchExtensions(tree, wtm, ply, depth);
        extensions = extended - 1;
        fprune = 0;
        if (moves_searched && extended <= 0 && !tree->inchk[ply] &&
            !tree->inchk[ply + 1] && abs(alpha) < (MATE - 500)) {
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
              -Search(tree, -t_beta, -alpha, Flip(wtm), depth + extensions,
              ply + 1, DO_NULL);
          if (value > alpha && extended < 0)
            value =
                -Search(tree, -t_beta, -alpha, Flip(wtm), depth - 1, ply + 1,
                DO_NULL);
        } else
          value = -QuiesceChecks(tree, -t_beta, -alpha, Flip(wtm), ply + 1);
        if (abort_search || tree->stop)
          break;
        if (value > alpha && value < beta && moves_searched) {
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
        if (value > alpha) {
          if (value >= beta) {
            Killer(tree, ply, tree->curmv[ply]);
            UnmakeMove(tree, ply, tree->curmv[ply], wtm);
            HashStore(tree, ply, depth, wtm, LOWER, value);
            tree->fail_high++;
            if (!moves_searched)
              tree->fail_high_first++;
            return (value);
          }
          alpha = value;
        }
        t_beta = alpha + 1;
        moves_searched++;
      } while (0);
    else
      tree->nodes_searched++;
    UnmakeMove(tree, ply, tree->curmv[ply], wtm);
    if (abort_search || tree->stop)
      return (0);
/*
 ************************************************************
 *                                                          *
 *   if this is an SMP search, and we have idle processors, *
 *   now is the time to get them involved.  we have now     *
 *   satisfied the "young brothers wait" condition since we *
 *   have searched one move.  All that is left is to check  *
 *   the remaining depth so that we do not split too near   *
 *   the tips.  min_thread_depth gives us a percentage of   *
 *   the tree depth near the frontier where we can not      *
 *   afford to split.  For example, if min_thread_depth =   *
 *   40%, and we are doing a 20 ply search, we will not     *
 *   split within 8 plies of the frontier.                  *
 *                                                          *
 ************************************************************
 */
#if (CPUS > 1)
    if (smp_idle && moves_searched &&
        depth >= min_thread_depth * (ply + depth) / 100) {
      tree->alpha = alpha;
      tree->beta = beta;
      tree->value = alpha;
      tree->wtm = wtm;
      tree->ply = ply;
      tree->depth = depth;
      if (Thread(tree)) {
        if (abort_search || tree->stop)
          return (0);
        if (tree->thread_id == 0 && CheckInput())
          Interrupt(ply);
        value = tree->search_value;
        if (value > alpha) {
          if (value >= beta) {
            Killer(tree, ply, tree->curmv[ply]);
            HashStore(tree, ply, depth, wtm, LOWER, value);
            tree->fail_high++;
            return (value);
          }
          alpha = value;
          break;
        }
      }
    }
#endif
  }
/*
 ************************************************************
 *                                                          *
 *   all moves have been searched.  if none were legal,     *
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
  } else {
    if (alpha != o_alpha) {
      memcpy(&tree->pv[ply - 1].path[ply], &tree->pv[ply].path[ply],
          (tree->pv[ply].pathl - ply + 1) * sizeof(int));
      memcpy(&tree->pv[ply - 1].pathh, &tree->pv[ply].pathh, 3);
      tree->pv[ply - 1].path[ply - 1] = tree->curmv[ply - 1];
      Killer(tree, ply, tree->pv[ply].path[ply]);
    }
    HashStore(tree, ply, depth, wtm, (alpha == o_alpha) ? UPPER : EXACT, alpha);
    return (alpha);
  }
}

/* last modified 11/10/08 */
/*
 *******************************************************************************
 *                                                                             *
 *   SearchExtensions() is used to adjust the search depth for the tree below  *
 *   this node.  checking moves are problematic as they sometimes lead to a    *
 *   deep win of material or a checkmate, or sometimes they are used to delay  *
 *   something (push it beyond the search horizon) so that it is not even      *
 *   found by the search.                                                      *
 *                                                                             *
 *   additionally, the opposite is also true.  some moves do not deserve as    *
 *   much effort as others, and we try to recognize that here and rather than  *
 *   extending the search depth (and effort) we actually reduce the search     *
 *   depth to save time that can be better spent on other moves.  any move     *
 *   is tactical in nature (captures, checks, passed pawn pushes, and so       *
 *   forth) are not reduced.  normal moves that appear to be "good" are also   *
 *   not reduced, including the hash table "best move", killer moves, and      *
 *   other such moves.  that leaves moves that have not been identified as     *
 *   "good" to be searched with reduced depth.  the final criterion here is    *
 *   that the "history value" must lie below some threshold indicating that    *
 *   this move has never been very good in the current search.                 *
 *                                                                             *
 *   In reality, what we are doing is extending the obvious moves (checks) to  *
 *   search them deeper, reducing the moves that appear to be bad, leaving the *
 *   more normal-looking moves alone.                                          *
 *                                                                             *
 *******************************************************************************
 */
int SearchExtensions(TREE * RESTRICT tree, int wtm, int ply, int depth)
{
  register int adjustment = 0, move, square;

/*
 ************************************************************
 *                                                          *
 *   if the move to be made checks the opponent, then we    *
 *   need to remember that he's in check and also extend    *
 *   the depth by one ply for him to get out.               *
 *                                                          *
 ************************************************************
 */
  if (Check(Flip(wtm))) {
    tree->inchk[ply + 1] = 1;
    tree->check_extensions_done++;
    adjustment += check_depth;
    return (adjustment);
  } else
    tree->inchk[ply + 1] = 0;
/*
 ************************************************************
 *                                                          *
 *   no extensions were triggered, or we would have exited. *
 *                                                          *
 *   now it's time to try to reduce the search depth if the *
 *   move appears to be "weak".  to reduce the search, the  *
 *   following requirements must be met:                    *
 *                                                          *
 *   (1) we must be in the "REMAINING_MOVES part of the     *
 *       move ordering, so that we have nearly given up on  *
 *       failing high on any move.                          *
 *                                                          *
 *   (2) the remaining search depth left must satisfy the   *
 *       following inequality:                              *
 *                                                          *
 *         depth - 1 - LMR_depth < LMR_min_depth            *
 *                                                          *
 *       simply stated, for default settings, there must    *
 *       be at least 3 plies of depth left or we won't      *
 *       reduce since we are too close to the frontier and  *
 *       that leads to tactical mistakes.                   *
 *                                                          *
 *   (3) the current move must not be a checking move.      *
 *                                                          *
 *   (4) the side to move can not be in check;              *
 *                                                          *
 *   (5) the moving piece is not a passed pawn;             *
 *                                                          *
 *   (6) the current move can not affect the material       *
 *       balance, that is it can not be a capture or pawn   *
 *       promotion;                                         *
 *                                                          *
 ************************************************************
 */
  if (tree->phase[ply] != REMAINING_MOVES)
    return (0);
  tree->reductions_attempted++;
  move = tree->curmv[ply];
  square = To(move);
  if (depth - 1 - LMR_depth < LMR_min_depth || tree->inchk[ply] ||
      CaptureOrPromote(move))
    return (0);
/*
 ************************************************************
 *                                                          *
 *   check the move to see if a passed pawn is being        *
 *   advanced.                                              *
 *                                                          *
 ************************************************************
 */
  if (Piece(move) == pawn) {
    if (!(mask_pawn_passed[wtm][square] & Pawns(Flip(wtm))))
      return (0);
  }
/*
 ************************************************************
 *                                                          *
 *   move is safe to reduce.                                *
 *                                                          *
 ************************************************************
 */
  tree->reductions_done++;
  return (-LMR_depth);
}
