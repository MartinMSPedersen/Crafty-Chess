#include "chess.h"
#include "data.h"
/* last modified 01/17/09 */
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
  tree->nodes_searched++;
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
 *  First, we try a null move to see if we can get a quick  *
 *  cutoff with only a little work.  this operates as       *
 *  follows.  Instead of making a legal move, the side on   *
 *  move 'passes' and does nothing.  The resulting position *
 *  is searched to a shallower depth than normal (usually   *
 *  3 plies less but settable by the operator.) This will   *
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
 *  5.  If the alpha/beta window is non-null, this is a PV  *
 *      node where the null move should be avoided.         *
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
 *   First step is to see if we need to extend this move    *
 *   for some tactical reason.  If not, we check to see if  *
 *   we can reduce the depth (LMR) to save time.  A final   *
 *   case is to determine if we can use AEL pruning         *
 *   (Heinz 2000) near the search frontier.  Note for those *
 *   that have read Heinz's paper.  Frontier nodes in       *
 *   crafty have a depth = 2.  Pre-frontier nodes  have a   *
 *   depth = 3.  And finally, pre-pre-frontier nodes have a *
 *   depth = 4.                                             *
 *                                                          *
 ************************************************************
 */
  tree->next_status[ply].phase = HASH_MOVE;
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
            } else if (depth == 3 &&
                MaterialSTM + extended_futility_margin <= alpha)
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
    else
      tree->nodes_searched++;
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
 *   the remaining depth so that we do not split too near   *
 *   the tips.  Min_thread_depth gives us a percentage of   *
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
    int bestmove = (alpha == o_alpha) ? 0 : tree->pv[ply].path[ply];
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

/* last modified 01/17/09 */
/*
 *******************************************************************************
 *                                                                             *
 *   SearchExtensions() is used to adjust the search depth for the tree below  *
 *   this node.  Checking moves are problematic as they sometimes lead to a    *
 *   deep win of material or a checkmate, or sometimes they are used to delay  *
 *   something (push it beyond the search horizon) so that it is not even      *
 *   found by the search.                                                      *
 *                                                                             *
 *   Additionally, the opposite is also true.  Some moves do not deserve as    *
 *   much effort as others, and we try to recognize that here and rather than  *
 *   extending the search depth (and effort) we actually reduce the search     *
 *   depth to save time that can be better spent on other moves.  Any move     *
 *   tactical in nature (captures, checks, passed pawn pushes, and so forth)   *
 *   are not reduced.  normal moves that appear to be "good" are also not      *
 *   reduced, including the hash table "best move", killer moves, and other    *
 *   such moves.  that leaves moves that have not been identified as good to   *
 *   be searched with reduced depth.
 *                                                                             *
 *   In reality, what we are doing is extending the obvious moves (checks) to  *
 *   search them deeper, reducing the moves that appear to be bad, to search   *
 *   those shallowly, and searching the normal-looking moves to the usual      *
 *   depth.                                                                    *
 *                                                                             *
 *******************************************************************************
 */
int SearchExtensions(TREE * RESTRICT tree, int wtm, int ply, int depth)
{
  register int adjustment = 0, move, square;

/*
 ************************************************************
 *                                                          *
 *   If the move to be made checks the opponent, then we    *
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
 *   The current move is not a check or we would have       *
 *   already extended it and exited.                        *
 *                                                          *
 *   Now it's time to try to reduce the search depth if the *
 *   move appears to be "poor".  To reduce the search, the  *
 *   following requirements must be met:                    *
 *                                                          *
 *   (1) We must be in the "REMAINING_MOVES part of the     *
 *       move ordering, so that we have nearly given up on  *
 *       failing high on any move.                          *
 *                                                          *
 *   (2) The remaining search depth left must satisfy the   *
 *       following inequality:                              *
 *                                                          *
 *         depth - 1 - LMR_depth < LMR_min_depth            *
 *                                                          *
 *       Simply stated, for default settings, there must    *
 *       be at least 3 plies of depth left or we won't      *
 *       reduce since we are too close to the frontier and  *
 *       that leads to tactical mistakes.                   *
 *                                                          *
 *   (3) The current move must not be a checking move.      *
 *                                                          *
 *   (4) The side to move can not be in check;              *
 *                                                          *
 *   (5) The moving piece is not a passed pawn;             *
 *                                                          *
 *   (6) The current move can not affect the material       *
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
 *   Check the move to see if a passed pawn is being        *
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
 *   Move is safe to reduce.                                *
 *                                                          *
 ************************************************************
 */
  tree->reductions_done++;
  return (-LMR_depth);
}

/* modified 01/17/09 */
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
    int wtm, int depth, int ply)
{
  register int fprune, extended, extensions;
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
 *   First step is to see if we need to extend this move    *
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
            } else if (depth == 3 &&
                MaterialSTM + extended_futility_margin <= alpha)
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
                -Search(tree, -alpha - 1, -alpha, Flip(wtm), depth - 1, ply + 1,
                DO_NULL);
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

/* modified 01/17/09 */
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
int SearchRoot(TREE * RESTRICT tree, int alpha, int beta, int wtm, int depth)
{
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
    do {
      extensions = SearchExtensions(tree, wtm, 1, depth);
      begin_root_nodes = tree->nodes_searched;
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
        if (depth - 1 + extensions > 0)
          value =
              -Search(tree, -beta, -alpha, Flip(wtm), depth - 1 + extensions, 2,
              DO_NULL);
        else
          value = -QuiesceChecks(tree, -beta, -alpha, Flip(wtm), 2);
        first_move = 0;
      } else {
        if (depth - 1 + extensions > 0) {
          value =
              -Search(tree, -alpha - 1, -alpha, Flip(wtm),
              depth - 1 + extensions, 2, DO_NULL);
          if (value > alpha && extensions < 0)
            value =
                -Search(tree, -alpha - 1, -alpha, Flip(wtm), depth - 1, 2,
                DO_NULL);
        } else
          value = -QuiesceChecks(tree, -alpha - 1, -alpha, Flip(wtm), 2);
        if (abort_search)
          break;
        if ((value > alpha) && (value < beta)) {
          if (depth - 1 + extensions > 0)
            value =
                -Search(tree, -beta, -alpha, Flip(wtm), depth - 1 + extensions,
                2, DO_NULL);
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
    if (split_at_root && smp_idle && NextRootMoveParallel()) {
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
