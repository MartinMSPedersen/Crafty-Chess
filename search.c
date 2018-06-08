#include "chess.h"
#include "data.h"
/* last modified 07/04/09 */
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
    int ply, int do_null) {
  register BITBOARD start_nodes = tree->nodes_searched;
  register int first_tried, moves_searched = 0;
  register int o_alpha = alpha, value = 0, t_beta = beta;
  register int extensions, pieces;

/*
 ************************************************************
 *                                                          *
 *   Check to see if we have searched enough nodes that it  *
 *   is time to peek at how much time has been used, or if  *
 *   is time to check for operator keyboard input.  This is *
 *   usually enough nodes to force a time/input check about *
 *   once per second, except when the target time per move  *
 *   is very small, in which case we try to check the time  *
 *   at least 10 times during the search.                   *
 *                                                          *
 *   Note that we check for timeout in all active threads,  *
 *   but we only do I/O in thread 0 to avoid read race      *
 *   conditions that are problematic.                       *
 *                                                          *
 ************************************************************
 */
#if defined(NODES)
  temp_search_nodes--;
  if (temp_search_nodes <= 0) {
    time_abort++;
    abort_search = 1;
    return (0);
  }
#endif
  if (--next_time_check <= 0) {
    next_time_check = nodes_between_time_checks;
    if (TimeCheck(tree, 0)) {
      time_abort++;
      abort_search = 1;
      return (0);
    }
    if (tree->thread_id == 0) {
      if (CheckInput())
        Interrupt(ply);
    }
  }
  if (ply >= MAXPLY - 1)
    return (beta);
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
/*
 ************************************************************
 *                                                          *
 *   Now call HashProbe() to see if this position has been  *
 *   searched before.  If so, we may get a real score,      *
 *   produce a cutoff, or get nothing more than a good move *
 *   to try first.  There are four cases to handle:         *
 *                                                          *
 *   1. HashProbe() returns "EXACT" if this score is        *
 *   greater than beta, return beta.  Otherwise, return the *
 *   score.  In either case, no further searching is needed *
 *   from this position.  Note that lookup verified that    *
 *   the table position has sufficient "draft" to meet the  *
 *   requirements of the current search depth remaining.    *
 *                                                          *
 *   2. HashProbe() returns "UPPER" which means that when   *
 *   this position was searched previously, every move was  *
 *   "refuted" by one of its descendents.  As a result,     *
 *   when the search was completed, we returned alpha at    *
 *   that point.  We simply return alpha here as well.      *
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
 *   Now it's time to try a probe into the endgame table-   *
 *   base files.  This is done if we notice that there are  *
 *   6 or fewer pieces left on the board.  EGTB_use tells   *
 *   us how many pieces to probe on.  Note that this can be *
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
      HashStore(tree, ply, MAX_DRAFT, wtm, EXACT, alpha,
          tree->pv[ply].path[ply]);
      return (alpha);
    }
  }
#endif
/*
 ************************************************************
 *                                                          *
 *  We now know there is no easy way out via a hash hit, a  *
 *  repetition hit, or an EGTB hit.  Which leaves us one    *
 *  more way of getting out with minimal effort, where we   *
 *  try a null move to see if we can get a quick cutoff     *
 *  with only a little work.  this operates as follows.     *
 *  Instead of making a legal move, the side on move        *
 *  "passes" and does nothing.  The resulting position is   *
 *  searched to a shallower depth than normal (usually 3    *
 *  plies less but settable by the operator.) This will     *
 *  result in a cutoff if our position is very good, but it *
 *  produces the cutoff much quicker since the search is    *
 *  far shallower than a normal search that would also be   *
 *  likely to fail high.                                    *
 *                                                          *
 *  This is skipped for any of the following reasons:       *
 *                                                          *
 *  1.  The side on move is in check.  The null move        *
 *      results in an illegal position.                     *
 *  2.  No more than one null move can appear in succession *
 *      or else the search will degenerate into nothing.    *
 *  3.  The side on move has little material left making    *
 *      zugzwang positions more likely.                     *
 *  4.  The transposition table probe found an entry that   *
 *      indicates that a null-move search will not fail     *
 *      high, so we avoid the wasted effort.                *
 *                                                          *
 ************************************************************
 */
  tree->inchk[ply + 1] = 0;
  tree->last[ply] = tree->last[ply - 1];
  pieces =
      (wtm) ? TotalPieces(white, occupied) : TotalPieces(black, occupied);
  if (do_null && alpha == beta - 1 && depth > 1) {
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
        HashEP(EnPassant(ply + 1));
        EnPassant(ply + 1) = 0;
      }
      if (depth - null_depth - 1 > 0)
        value =
            -Search(tree, -beta, -beta + 1, Flip(wtm), depth - null_depth - 1,
            ply + 1, NO_NULL);
      else
        value = -QuiesceChecks(tree, -beta, -beta + 1, Flip(wtm), ply + 1);
      HashKey = save_hash_key;
      if (abort_search || tree->stop)
        return (0);
      if (value >= beta) {
        HashStore(tree, ply, depth, wtm, LOWER, value, tree->curmv[ply]);
        return (value);
      }
    }
  }
/*  
 ************************************************************
 *                                                          *
 *   Now iterate through the move list and search the       *
 *   resulting positions.  Note that Search() culls any     *
 *   move that is not legal by using Check().  The special  *
 *   case is that we must find one legal move to search to  *
 *   confirm that it's not a mate or draw.                  *
 *                                                          *
 ************************************************************
 */
  tree->next_status[ply].phase = HASH_MOVE;
  while ((tree->phase[ply] =
          (tree->inchk[ply]) ? NextEvasion(tree, ply, wtm) : NextMove(tree,
              ply, wtm))) {
#if defined(TRACE)
    if (ply <= trace_level)
      Trace(tree, ply, depth, wtm, alpha, beta, "Search2", tree->phase[ply]);
#endif
    MakeMove(tree, ply, tree->curmv[ply], wtm);
    if (moves_searched == 0)
      first_tried = tree->curmv[ply];
    tree->nodes_searched++;
    if (tree->inchk[ply] || !Check(wtm))
      do {
/*
 ************************************************************
 *                                                          *
 *   If the move to be made checks the opponent, then we    *
 *   need to remember that he's in check and also extend    *
 *   the depth by one ply for him to get out.  Note that if *
 *   the move gives check, it is not a candidate for either *
 *   depth reduction or forward-pruning.                    *
 *                                                          *
 ************************************************************
 */
        extensions = 0;
        if (Check(Flip(wtm))) {
          tree->inchk[ply + 1] = 1;
          tree->extensions_done++;
          extensions = check_depth;
        } else
          tree->inchk[ply + 1] = 0;
/*
 ************************************************************
 *                                                          *
 *   Now it's time to try to reduce the search depth if the *
 *   move appears to be "poor".  To reduce the search, the  *
 *   following requirements must be met:                    *
 *                                                          *
 *   (1) We must be in the REMAINING_MOVES part of the move *
 *       ordering, so that we have nearly given up on       *
 *       failing high on any move.                          *
 *   (2) We must not be too close to the horizon (this is   *
 *       the LMR_min_depth value).                          *
 *   (3) The current move must not be a checking move and   *
 *       the side to move can not be in check.              *
 *   (4) The moving piece is not a passed pawn.             *
 *   (5) The current move can not affect the material       *
 *       balance, that is it can not be a capture or pawn   *
 *       promotion.                                         *
 *                                                          *
 ************************************************************
 */
        if (tree->phase[ply] == REMAINING_MOVES && !tree->inchk[ply] &&
            !tree->inchk[ply + 1] && !CaptureOrPromote(tree->curmv[ply])) {
          if (depth - 1 - LMR_depth >= LMR_min_depth &&
              (Piece(tree->curmv[ply]) != pawn ||
                  mask_passed[wtm][To(tree->
                          curmv[ply])] & Pawns(Flip(wtm)))) {
            extensions -= LMR_depth;
            tree->reductions_done++;
          }
/*
 ************************************************************
 *                                                          *
 *   Now for the forward-pruning stuff.  The idea here is   *
 *   based on the old FUTILITY idea, where if the current   *
 *   material + a fudge factor is lower than alpha, then    *
 *   there is little promise in searching this move to make *
 *   up for the material deficit.                           *
 *                                                          *
 *   We have an array of pruning margin values that are     *
 *   indexed by depth (remaining plies left until we drop   *
 *   into the quiescence search) and which increase with    *
 *   depth since more depth means a greater chance of       *
 *   bringing the score back up to alpha or beyond.  If the *
 *   current material + the bonus is less than alpha, we    *
 *   simply avoid searching this move at all, and skip to   *
 *   the next move without expending any more effort.  Note *
 *   that this is classic forward-pruning and can certainly *
 *   introduce errors into the search.  However, cluster    *
 *   testing has shown that this improves play in real      *
 *   games.  The current implementation only prunes in the  *
 *   last 4 plies before quiescence, although this can be   *
 *   tuned with the "eval" command changing the "pruning    *
 *   depth" value to something other than 5 (test is for    *
 *   depth < pruning depth, current value is 5 which prunes *
 *   in last 4 plies only).  Testing shows no benefit in    *
 *   larger values than 5, although this might change in    *
 *   future versions as other things are modified.          *
 *                                                          *
 ************************************************************
 */
          if (depth < pruning_depth && moves_searched &&
              MaterialSTM(wtm) + pruning_margin[depth] <= alpha) {
            tree->moves_pruned++;
            continue;
          }
        }
/*  
 ************************************************************
 *                                                          *
 *   We have determined whether the depth is to be changed  *
 *   by an extension or a reduction.  If we get to this     *
 *   point, then the move is not being pruned.  So off we   *
 *   go to a recursive search/quiescence call to work our   *
 *   way toward a terminal node.                            *
 *                                                          *
 ************************************************************
 */
        if (depth + extensions - 1 > 0) {
          value =
              -Search(tree, -t_beta, -alpha, Flip(wtm),
              depth + extensions - 1, ply + 1, DO_NULL);
          if (value > alpha && extensions < 0)
            value =
                -Search(tree, -t_beta, -alpha, Flip(wtm), depth - 1, ply + 1,
                DO_NULL);
        } else
          value = -QuiesceChecks(tree, -t_beta, -alpha, Flip(wtm), ply + 1);
        if (abort_search || tree->stop)
          break;
        if (value > alpha && value < beta && moves_searched) {
          extensions = Max(extensions, 0);
          if (depth + extensions - 1 > 0)
            value =
                -Search(tree, -beta, -alpha, Flip(wtm),
                depth + extensions - 1, ply + 1, DO_NULL);
          else
            value = -QuiesceChecks(tree, -beta, -alpha, Flip(wtm), ply + 1);
          if (abort_search || tree->stop)
            break;
        }
/*  
 ************************************************************
 *                                                          *
 *   Search (and/or re-search) has been completed.  Now we  *
 *   check for a fail high which terminates the search      *
 *   immediately as no further searching is required.       *
 *                                                          *
 ************************************************************
 */
        if (value > alpha) {
          if (value >= beta) {
            Killer(tree, ply, tree->curmv[ply]);
            UnmakeMove(tree, ply, tree->curmv[ply], wtm);
            HashStore(tree, ply, depth, wtm, LOWER, value, tree->curmv[ply]);
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
    UnmakeMove(tree, ply, tree->curmv[ply], wtm);
    if (abort_search || tree->stop)
      return (0);
/*
 ************************************************************
 *                                                          *
 *   If this is an SMP search, and we have idle processors, *
 *   now is the time to get them involved.  We have now     *
 *   satisfied the "young brothers wait" condition since we *
 *   have searched one move.  All that is left is to check  *
 *   the size of the tree we have searched so far, so that  *
 *   we do not split too near the tips and drive up the     *
 *   overhead unacceptably.  This has the additional effect *
 *   that we might split after 2-3 moves have been searched *
 *   which might sound like an issue, but the overhead is   *
 *   not so critical if we are more certain that we need to *
 *   actually search every move.  The more moves we have    *
 *   searched, the greater the probability that we are      *
 *   going to search them all.                              *
 *                                                          *
 ************************************************************
 */
#if (CPUS > 1)
    if (smp_idle && moves_searched &&
        tree->nodes_searched - start_nodes > smp_split_nodes) {
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
            Killer(tree, ply, tree->cutmove);
            HashStore(tree, ply, depth, wtm, LOWER, value, tree->cutmove);
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
  while (0);
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
  } else {
    int bestmove = (alpha == o_alpha) ? first_tried : tree->pv[ply].path[ply];
    int type = (alpha == o_alpha) ? UPPER : EXACT;
    if (alpha != o_alpha) {
      memcpy(&tree->pv[ply - 1].path[ply], &tree->pv[ply].path[ply],
          (tree->pv[ply].pathl - ply + 1) * sizeof(int));
      memcpy(&tree->pv[ply - 1].pathh, &tree->pv[ply].pathh, 3);
      tree->pv[ply - 1].path[ply - 1] = tree->curmv[ply - 1];
      Killer(tree, ply, tree->pv[ply].path[ply]);
    }
    HashStore(tree, ply, depth, wtm, type, alpha, bestmove);
    return (alpha);
  }
}

/* last modified 07/04/09 */
/*
 *******************************************************************************
 *                                                                             *
 *   SearchParallel() is the recursive routine used to implement alpha/beta    *
 *   negamax search using parallel threads.  When this code is called, the     *
 *   first move has already been searched, so all that is left is to search    *
 *   the remainder of the moves and then return.  Note that the hash table and *
 *   such can't be modified here since this only represents a part of the      *
 *   search at this ply.  All of that is deferred until we return and reach    *
 *   the original instance of Search() where we have the complete results from *
 *   all the threads that were helping here.                                   *
 *                                                                             *
 *******************************************************************************
 */
int SearchParallel(TREE * RESTRICT tree, int alpha, int beta, int value,
    int wtm, int depth, int ply) {
  register int extensions;
  BITBOARD begin_root_nodes;
/*  
 ************************************************************
 *                                                          *
 *   Now iterate through the move list and search the       *
 *   resulting positions.  Note that Search() culls any     *
 *   move that is not legal by using Check().  The special  *
 *   case is that we must find one legal move to search to  *
 *   confirm that it's not a mate or draw.                  *
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
          wtm) : NextMove((TREE *)
          tree->parent, ply, wtm);
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
    tree->nodes_searched++;
    if (tree->inchk[ply] || !Check(wtm))
      do {
/*
 ************************************************************
 *                                                          *
 *   If the move to be made checks the opponent, then we    *
 *   need to remember that he's in check and also extend    *
 *   the depth by one ply for him to get out.  Note that if *
 *   the move gives check, it is not a candidate for either *
 *   depth reduction or forward-pruning.                    *
 *                                                          *
 ************************************************************
 */

        begin_root_nodes = tree->nodes_searched;
        extensions = 0;
        if (Check(Flip(wtm))) {
          tree->inchk[ply + 1] = 1;
          tree->extensions_done++;
          extensions = check_depth;
        } else
          tree->inchk[ply + 1] = 0;
/*
 ************************************************************
 *                                                          *
 *   Now it's time to try to reduce the search depth if the *
 *   move appears to be "poor".  To reduce the search, the  *
 *   following requirements must be met:                    *
 *                                                          *
 *   (1) We must be in the REMAINING_MOVES part of the move *
 *       ordering, so that we have nearly given up on       *
 *       failing high on any move.                          *
 *   (2) We must not be too close to the horizon (this is   *
 *       the LMR_min_depth value).                          *
 *   (3) The current move must not be a checking move and   *
 *       the side to move can not be in check;              *
 *   (4) The moving piece is not a passed pawn;             *
 *   (5) The current move can not affect the material       *
 *       balance, that is it can not be a capture or pawn   *
 *       promotion;                                         *
 *                                                          *
 ************************************************************
 */
        if (tree->phase[ply] == REMAINING_MOVES && !tree->inchk[ply] &&
            !tree->inchk[ply + 1] && !CaptureOrPromote(tree->curmv[ply])) {
          if (depth - 1 - LMR_depth >= LMR_min_depth &&
              (Piece(tree->curmv[ply]) != pawn ||
                  mask_passed[wtm][To(tree->
                          curmv[ply])] & Pawns(Flip(wtm)))) {
            extensions -= LMR_depth;
            tree->reductions_done++;
          }
/*
 ************************************************************
 *                                                          *
 *   Now for the forward-pruning stuff.  The idea here is   *
 *   based on the old FUTILITY idea, where if the current   *
 *   material + a fudge factor is lower than alpha, then    *
 *   there is little promise in searching this move to make *
 *   up for the material deficit.                           *
 *                                                          *
 *   We have an array of pruning margin values that are     *
 *   indexed by depth (remaining plies left until we drop   *
 *   into the quiescence search) and which increase with    *
 *   depth since more depth means a greater chance of       *
 *   bringing the score back up to alpha or beyond.  If the *
 *   current material + the bonus is less than alpha, we    *
 *   simply avoid searching this move at all, and skip to   *
 *   the next move without expending any more effort.  Note *
 *   that this is classic forward-pruning and can certainly *
 *   introduce errors into the search.  However, cluster    *
 *   testing has shown that this improves play in real      *
 *   games.  The current implementation only prunes in the  *
 *   last 4 plies before quiescence, although this can be   *
 *   tuned with the "eval" command changing the "pruning    *
 *   depth" value to something other than 5 (test is for    *
 *   depth < pruning depth, current value is 5 which prunes *
 *   in last 4 plies only).  Testing shows no benefit in    *
 *   larger values than 5, although this might change in    *
 *   future versions as other things are modified.          *
 *                                                          *
 ************************************************************
 */
          if (depth < pruning_depth &&
              MaterialSTM(wtm) + pruning_margin[depth] <= alpha) {
            tree->moves_pruned++;
            continue;
          }
        }
/*  
 ************************************************************
 *                                                          *
 *   We have determined whether the depth is to be changed  *
 *   by an extension or a reduction.  If we get to this     *
 *   point, then the move is not being pruned.  So off we   *
 *   go to a recursive search/quiescence call to work our   *
 *   way toward a terminal node.                            *
 *                                                          *
 ************************************************************
 */
        if (depth + extensions - 1 > 0) {
          value =
              -Search(tree, -alpha - 1, -alpha, Flip(wtm),
              depth + extensions - 1, ply + 1, DO_NULL);
          if (value > alpha && extensions < 0)
            value =
                -Search(tree, -alpha - 1, -alpha, Flip(wtm), depth - 1,
                ply + 1, DO_NULL);
        } else
          value =
              -QuiesceChecks(tree, -alpha - 1, -alpha, Flip(wtm), ply + 1);
        if (abort_search || tree->stop)
          break;
        if (value > alpha && value < beta) {
          extensions = Max(extensions, 0);
          if (depth + extensions - 1 > 0)
            value =
                -Search(tree, -beta, -alpha, Flip(wtm),
                depth + extensions - 1, ply + 1, DO_NULL);
          else
            value = -QuiesceChecks(tree, -beta, -alpha, Flip(wtm), ply + 1);
          if (abort_search || tree->stop)
            break;
        }
/*
 ************************************************************
 *                                                          *
 *   Now we check for an undesirable case, that of failing  *
 *   high while doing a parallel (threaded) search.  This   *
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
              for (proc = 0; proc < smp_max_threads; proc++)
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
 *   There are no "end-of-search" things to do.  We have    *
 *   searched all the remaining moves at this ply in        *
 *   parallel, and now return and let the original search   *
 *   (that started this sub-tree) clean up, and do the      *
 *   tests for mate/stalemate, update the hash table, etc.  *
 *                                                          *
 *   We do need to flag the root move we tried to search,   *
 *   if we were stopped early due to another root move      *
 *   failing high.  Otherwise this move appears to have     *
 *   been searched already and will not be searched again   *
 *   until the next iteration.                              *
 *                                                          *
 ************************************************************
 */
  if (tree->stop && ply == 1)
    root_moves[tree->root_move].status &= 4095 - 256;
  return (alpha);
}

/* last modified 07/04/09 */
/*
 *******************************************************************************
 *                                                                             *
 *   SearchRoot() is the recursive routine used to implement the alpha/beta    *
 *   negamax search (similar to minimax but simpler to code.)  SearchRoot() is *
 *   only called when ply=1.  It is somewhat different from Search() in that   *
 *   some things (null move search, hash lookup, etc.) are not useful at the   *
 *   root of the tree.  SearchRoot() calls Search() to search any positions    *
 *   that are below ply=1.                                                     *
 *                                                                             *
 *******************************************************************************
 */
int SearchRoot(TREE * RESTRICT tree, int alpha, int beta, int wtm, int depth) {
  register int first_move = 1;
  register int value;
  register int extensions;
  BITBOARD begin_root_nodes;
/*
 ************************************************************
 *                                                          *
 *   Now iterate through the move list and search the       *
 *   resulting positions.  Note that SearchRoot() does not  *
 *   search illegal moves since RootMoves() screened each   *
 *   move before adding it to the permanent ply-1 move list *
 *   earlier.                                               *
 *                                                          *
 ************************************************************
 */
  tree->inchk[1] = Check(wtm);
  while ((tree->phase[1] = NextRootMove(tree, tree, wtm))) {
#if defined(TRACE)
    if (trace_level > 0)
      Trace(tree, 1, depth, wtm, alpha, beta, "SearchRoot", tree->phase[1]);
#endif
    MakeMove(tree, 1, tree->curmv[1], wtm);
    tree->nodes_searched++;
    do {
      begin_root_nodes = tree->nodes_searched;
      extensions = 0;
      if (Check(Flip(wtm))) {
        tree->inchk[2] = 1;
        tree->extensions_done++;
        extensions = check_depth;
      } else
        tree->inchk[2] = 0;
/*
 ************************************************************
 *                                                          *
 *   Now it's time to try to reduce the search depth if the *
 *   move appears to be "poor".  To reduce the search, the  *
 *   following requirements must be met:                    *
 *                                                          *
 *   (1) We must be in the REMAINING_MOVES part of the move *
 *       ordering, so that we have nearly given up on       *
 *       failing high on any move.                          *
 *   (2) We must not be too close to the horizon (this is   *
 *       the LMR_min_depth value).                          *
 *   (3) The current move must not be a checking move and   *
 *       the side to move can not be in check;              *
 *   (4) The moving piece is not a passed pawn;             *
 *   (5) The current move can not affect the material       *
 *       balance, that is it can not be a capture or pawn   *
 *       promotion;                                         *
 *                                                          *
 ************************************************************
 */
      if (tree->phase[1] == REMAINING_MOVES && !tree->inchk[1] &&
          !tree->inchk[2] && !CaptureOrPromote(tree->curmv[1])) {
        if (depth - 1 - LMR_depth >= LMR_min_depth &&
            (Piece(tree->curmv[1]) != pawn ||
                mask_passed[wtm][To(tree->curmv[1])] & Pawns(Flip(wtm)))) {
          extensions -= LMR_depth;
          tree->reductions_done++;
        }
      }
/*
 ************************************************************
 *                                                          *
 *   If this is the first move searched at ply=1, we search *
 *   using the normal alpha/beta bounds.  If not, we first  *
 *   search with alpha, alpha+1 (PVS search).  If this      *
 *   search fails high, we re-search using the normal       *
 *   bounds.  If allowed by inter-iteration analysis, some  *
 *   root moves might be searched with a reduced depth (as  *
 *   in the LMR idea).  If a reduced move fails high, we    *
 *   always re-search it with the normal depth before we    *
 *   accept the fail-high as a real one and re-search with  *
 *   the original alpha/beta bounds.  So a single move      *
 *   might fail high on the reduced search, get re-searched *
 *   with the original depth, and if it fails high here, we *
 *   then re-search it a third time with the original       *
 *   search bound to see if it is really a new best move.   *
 *                                                          *
 ************************************************************
 */
      if (first_move) {
        if (depth + extensions - 1 > 0)
          value =
              -Search(tree, -beta, -alpha, Flip(wtm), depth + extensions - 1,
              2, DO_NULL);
        else
          value = -QuiesceChecks(tree, -beta, -alpha, Flip(wtm), 2);
        first_move = 0;
      } else {
        if (depth + extensions - 1 > 0) {
          value =
              -Search(tree, -alpha - 1, -alpha, Flip(wtm),
              depth + extensions - 1, 2, DO_NULL);
          if (value > alpha && extensions < 0)
            value =
                -Search(tree, -alpha - 1, -alpha, Flip(wtm), depth - 1, 2,
                DO_NULL);
        } else
          value = -QuiesceChecks(tree, -alpha - 1, -alpha, Flip(wtm), 2);
        if (abort_search)
          break;
        if ((value > alpha) && (value < beta)) {
          extensions = Max(extensions, 0);
          if (depth + extensions - 1 > 0)
            value =
                -Search(tree, -beta, -alpha, Flip(wtm),
                depth + extensions - 1, 2, DO_NULL);
          else
            value = -QuiesceChecks(tree, -beta, -alpha, Flip(wtm), 2);
        }
      }
/*
 ************************************************************
 *                                                          *
 *   Now the move has been searched to a satisfactory       *
 *   conclusion.  We check to see if the search was aborted *
 *   due to time constraints, and if so, we just return and *
 *   do not modify the best move or anything.  If not, we   *
 *   then test for a beta cutoff and return to the control  *
 *   for the iterated search (Iterate()) to alter the       *
 *   aspiration window if needed.                           *
 *                                                          *
 ************************************************************
 */
      if (abort_search)
        break;
      root_moves[tree->root_move].nodes =
          tree->nodes_searched - begin_root_nodes;
/*  
 ************************************************************
 *                                                          *
 *   Search (and/or re-search) has been completed.  Now we  *
 *   check for a fail high which terminates the search      *
 *   immediately as no further searching is required.       *
 *                                                          *
 ************************************************************
 */
      if (value > alpha) {
        Output(tree, value, beta);
        root_value = alpha;
        if (value >= beta) {
          Killer(tree, 1, tree->curmv[1]);
          UnmakeMove(tree, 1, tree->curmv[1], wtm);
          return (value);
        }
        alpha = value;
      }
      root_value = alpha;
    } while (0);
    UnmakeMove(tree, 1, tree->curmv[1], wtm);
    if (abort_search)
      return (0);
/*
 ************************************************************
 *                                                          *
 *   After searching the first move, we can now begin a     *
 *   parallel search at the root if root splitting is       *
 *   allowed, and the next move is not flagged as a "serial *
 *   search only" type move because we think it might be a  *
 *   new best move when searched further.  Otherwise, back  *
 *   to the top of the NextRootMove() loop to search the    *
 *   remaining root moves one at a time.                    *
 *                                                          *
 ************************************************************
 */
#if (CPUS > 1)
    if (smp_split_at_root && smp_idle && NextRootMoveParallel()) {
      tree->alpha = alpha;
      tree->beta = beta;
      tree->value = alpha;
      tree->wtm = wtm;
      tree->ply = 1;
      tree->depth = depth;
      if (Thread(tree)) {
        if (abort_search)
          return (0);
        value = tree->search_value;
        if (value > alpha) {
          if (value >= beta) {
            Killer(tree, 1, tree->cutmove);
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
 *   All moves have been searched.  If none were legal,     *
 *   return either MATE or DRAW depending on whether the    *
 *   side to move is in check or not.                       *
 *                                                          *
 ************************************************************
 */
  if (abort_search || time_abort)
    return (0);
  if (first_move == 1) {
    value = (Check(wtm)) ? -(MATE - 1) : DrawScore(wtm);
    if (value >= alpha && value < beta) {
      tree->pv[0].pathl = 0;
      tree->pv[0].pathh = 0;
      tree->pv[0].pathd = iteration_depth;
      Output(tree, value, beta);
#if defined(TRACE)
      if (trace_level > 0)
        printf("Search() no moves!  ply=1\n");
#endif
    }
    return (value);
  } else {
    Killer(tree, 1, tree->pv[1].path[1]);
    return (alpha);
  }
}
