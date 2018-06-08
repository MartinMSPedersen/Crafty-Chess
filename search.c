#include "chess.h"
#include "data.h"
/* last modified 11/05/10 */
/*
 *******************************************************************************
 *                                                                             *
 *   Search() is the recursive routine used to implement the alpha/beta        *
 *   negamax search (similar to minimax but simpler to code.)  Search() is     *
 *   called whenever there is "depth" remaining so that all moves are subject  *
 *   to searching.  Search() recursively calls itself so long as there is at   *
 *   least one ply of depth left, otherwise it calls Quiesce() instead.        *
 *                                                                             *
 *******************************************************************************
 */
int Search(TREE * RESTRICT tree, int alpha, int beta, int wtm, int depth,
    int ply, int do_null) {
  BITBOARD start_nodes = tree->nodes_searched, begin_root_nodes = 0;
  int first_tried = 0, moves_searched = 0, repeat = 0;
  int o_alpha = alpha, value = 0, t_beta = beta;
  int extensions;

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
  if (--temp_search_nodes <= 0) {
    abort_after_ply1 = 1;
    abort_search = 1;
    return (0);
  }
#endif
  if (--next_time_check <= 0) {
    next_time_check = nodes_between_time_checks;
    if (TimeCheck(tree, 0)) {
      abort_after_ply1 = 1;
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
 *   Check for draw by repetition, which includes 50 move   *
 *   draws also.  This and the next two steps are skipped   *
 *   for root moves (ply = 1).                              *
 *                                                          *
 ************************************************************
 */
  tree->rep_list[wtm][Repetition(wtm) + (ply - 1) / 2] = HashKey;
  if (ply > 1) {
    if ((repeat = RepetitionCheck(tree, ply, wtm))) {
      if (repeat == 1 || !tree->inchk[ply]) {
        value = DrawScore(wtm);
        if (value < beta)
          SavePV(tree, ply, 0);
#if defined(TRACE)
        if (ply <= trace_level)
          printf("draw by repetition detected, ply=%d.\n", ply);
#endif
        return (value);
      }
    }
/*
 ************************************************************
 *                                                          *
 *   Now call HashProbe() to see if this position has been  *
 *   searched before.  If so, we may get a real score,      *
 *   produce a cutoff, or get nothing more than a good move *
 *   to try first.  There are four cases to handle:         *
 *                                                          *
 *   1. HashProbe() returns "HASH_HIT".  This terminates    *
 *   the search instantly and we simply return the value    *
 *   found in the hash table.  This value is simply the     *
 *   value we found when we did a real search in this       *
 *   position previously, and HashProbe() verifies that the *
 *   value is useful based on draft and current bounds.     *
 *                                                          *
 *   2. HashProbe() returns "AVOID_NULL_MOVE" which means   *
 *   the hashed score/bound was no good, but it indicated   *
 *   that trying a null-move in this position would be a    *
 *   waste of time since it will likely fail low, not high. *
 *                                                          *
 *   3. HashProbe() returns "HASH_MISS" when forces us to   *
 *   do a normal search to resolve this node.               *
 *                                                          *
 ************************************************************
 */
    switch (HashProbe(tree, ply, depth, wtm, alpha, beta, &value)) {
      case HASH_HIT:
        return (value);
      case AVOID_NULL_MOVE:
        do_null = 0;
      case HASH_MISS:
        break;
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
 *   Note that in "swindle mode" this can be turned off by  *
 *   Iterate() setting "EGTB_use = 0" so that we won't      *
 *   probe the EGTBs since we are searching only the root   *
 *   moves that lead to a draw and we want to play the move *
 *   that makes the draw more difficult to reach by the     *
 *   opponent to give him a chance to make a mistake.       *
 *                                                          *
 *   Another special case is that we slightly fudge the     *
 *   score for draws.  In a normal circumstance, draw=0.00  *
 *   since it is "equal".  However, here we add 0.01 if     *
 *   white has more material, or subtract 0.01 if black has *
 *   more material, since in a drawn KRP vs KR we would     *
 *   prefer to have the KRP side since the opponent can     *
 *   make a mistake and convert the draw to a loss.         *
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
        if (Abs(alpha) > MATE - 300)
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
        HashStore(tree, ply, MAX_DRAFT, wtm, EXACT, alpha, 0);
        return (alpha);
      }
    }
#endif
/*
 ************************************************************
 *                                                          *
 *  We now know there is no easy way out via a hash hit, a  *
 *  repetition hit, or an EGTB hit, which leaves us one     *
 *  more way of getting out with minimal effort, where we   *
 *  try a null move to see if we can get a quick cutoff     *
 *  with only a little work.  This operates as follows.     *
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
 *      as all this does is burn 6 plies of depth.          *
 *  3.  The side on move has only pawns left, which makes   *
 *      zugzwang positions more likely.                     *
 *  4.  The transposition table probe found an entry that   *
 *      indicates that a null-move search will not fail     *
 *      high, so we avoid the wasted effort.                *
 *                                                          *
 ************************************************************
 */
    tree->inchk[ply + 1] = 0;
    tree->last[ply] = tree->last[ply - 1];
    if (do_null && alpha == beta - 1 && depth > 1 && !tree->inchk[ply] &&
        TotalPieces(wtm, occupied)) {
      BITBOARD save_hash_key;

      tree->curmv[ply] = 0;
      tree->phase[ply] = NULL_MOVE;
#if defined(TRACE)
      if (ply <= trace_level)
        Trace(tree, ply, depth, wtm, beta - 1, beta, "Search1", NULL_MOVE);
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
        value = -Quiesce(tree, -beta, -beta + 1, Flip(wtm), ply + 1, 1);
      HashKey = save_hash_key;
      if (abort_search || tree->stop)
        return (0);
      if (value >= beta) {
        HashStore(tree, ply, depth, wtm, LOWER, value, tree->hash_move[ply]);
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
  if (depth >= 6 && !tree->hash_move[ply] && do_null && ply > 1) {
    int abound = (ply & 1) ? root_alpha : -root_beta;
    int bbound = (ply & 1) ? root_beta : -root_alpha;
    if (alpha == abound && beta == bbound) {
      PATH temp_path = tree->pv[ply];
      do {
        tree->curmv[ply] = 0;
        if (depth - 2 > 0)
          value = Search(tree, alpha, beta, wtm, depth - 2, ply, DO_NULL);
        else
          value = Quiesce(tree, alpha, beta, wtm, ply, 1);
        if (abort_search || tree->stop)
          break;
        if (value > alpha) {
          if (value < beta) {
            if ((int) tree->pv[ply - 1].pathl > ply)
              tree->hash_move[ply] = tree->pv[ply - 1].path[ply];
          } else
            tree->hash_move[ply] = tree->curmv[ply];
          tree->last[ply] = tree->last[ply - 1];
          tree->next_status[ply].phase = HASH_MOVE;
        }
      } while (0);
      tree->pv[ply] = temp_path;
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
          (ply > 1) ? ((tree->inchk[ply]) ? NextEvasion(tree, ply,
                  wtm) : NextMove(tree, ply, wtm)) : NextRootMove(tree, tree,
              wtm))) {
#if defined(TRACE)
    if (ply <= trace_level)
      Trace(tree, ply, depth, wtm, alpha, beta, "Search2", tree->phase[ply]);
#endif
    tree->nodes_searched++;
    if (ply == 1)
      begin_root_nodes = tree->nodes_searched;
    if (moves_searched == 0)
      first_tried = tree->curmv[ply];
    MakeMove(tree, ply, tree->curmv[ply], wtm);
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
          if (SwapO(tree, tree->curmv[ply], wtm) <= 0) {
            tree->extensions_done++;
            extensions = check_depth;
          }
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
 *       the LMR_remaining_depth value).                    *
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
            !extensions && moves_searched) {
          if ((Piece(tree->curmv[ply]) != pawn ||
                  mask_passed[wtm][To(tree->
                          curmv[ply])] & Pawns(Flip(wtm)))) {
            extensions =
                Min(-Min(depth - 1 - LMR_remaining_depth,
                    (moves_searched >
                        2) ? LMR_max_reduction : LMR_min_reduction), 0);
            if (extensions)
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
 *   This is a useful idea in today's 20+ ply searches, as  *
 *   when we near the tips, if we are too far behind in     *
 *   material, there is little time left to recover it and  *
 *   moves that don't bring us closer to a reasonable       *
 *   material balance can safely be skipped.  It is much    *
 *   more dangerous in shallow searches.                    *
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
          if (ply > 1 && depth < pruning_depth && moves_searched &&
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
 *   There are a couple of special-cases to handle.  If the *
 *   depth was reduced, and Search() returns a value >=     *
 *   beta, accepting that is risky (we reduced the move as  *
 *   we thought it was bad and expected it to fail low) so  *
 *   we repeat the search using the original (non-reduced)  *
 *   depth to see if the fail-high happens again.           *
 *                                                          *
 *   The other special-case is a result of the PVS idea and *
 *   is again a result of a fail-high.  Since we often      *
 *   narrow the window in PVS, if we narrow it at this ply  *
 *   and get a fail-high, we have to open it back up and    *
 *   repeat the search to see if it really fails high.  In  *
 *   most searches this is not done since we almost always  *
 *   reach this point with alpha = beta-1 so that there is  *
 *   no widening required.                                  *
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
          value = -Quiesce(tree, -t_beta, -alpha, Flip(wtm), ply + 1, 1);
        if (abort_search || tree->stop)
          break;
/*
 ************************************************************
 *                                                          *
 *   This is the PVS re-search code.  If we reach this      *
 *   point and value > alpha and value < beta, then this    *
 *   can not be a null-window search.  We have to re-search *
 *   the position with the original beta value (not alpha+1 *
 *   as is the usual case in PVS) to see if it still fails  *
 *   high before we treat this as a real fail-high and back *
 *   up the value to the previous ply.                      *
 *                                                          *
 ************************************************************
 */
        if (value > alpha && value < beta && moves_searched) {
          extensions = Max(extensions, 0);
          if (depth + extensions - 1 > 0)
            value =
                -Search(tree, -beta, -alpha, Flip(wtm),
                depth + extensions - 1, ply + 1, DO_NULL);
          else
            value = -Quiesce(tree, -beta, -alpha, Flip(wtm), ply + 1, 1);
          if (abort_search || tree->stop)
            break;
        }
        if (ply == 1)
          root_moves[tree->root_move].nodes =
              tree->nodes_searched - begin_root_nodes;
/*  
 ************************************************************
 *                                                          *
 *   Search (and/or re-search) has been completed.  Now we  *
 *   check for a fail high which terminates the search      *
 *   immediately as no further searching is required.       *
 *                                                          *
 *   Subtle code:  If ply == 1, we call Output() which will *
 *   dump the new PV.  But it also backs up the PV to ply=0 *
 *   which tells us which move to make.  We often time-out  *
 *   in the last iteration of a search and don't complete   *
 *   it.  We can't back up partial results, since they are  *
 *   not accurate, but when we dump a PV, it is from a full *
 *   search and it is safe to back it up to the root PV.    *
 *                                                          *
 ************************************************************
 */
        if (value > alpha) {
          if (ply == 1) {
            Output(tree, value, beta);
            root_value = alpha;
          }
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
        if (ply == 1)
          root_value = alpha;
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
        tree->nodes_searched - start_nodes > smp_split_nodes && (ply > 1 ||
            (smp_split_at_root && NextRootMoveParallel()))) {
      tree->alpha = alpha;
      tree->beta = beta;
      tree->value = alpha;
      tree->wtm = wtm;
      tree->ply = ply;
      tree->depth = depth;
      tree->moves_searched = moves_searched;
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
 *   Subtle code warning.  "abort_after_ply1" is used to    *
 *   avoid aborting the search in the middle of searching   *
 *   any ply-1 move that has already been started.  Once we *
 *   reach the target time, the abort_after_ply1 flag is    *
 *   set so that any call to NextRootMove() will return a   *
 *   value of NONE, but we don't abort the other searches.  *
 *   Once all pending root moves are completed, we get to   *
 *   this point.  Any time we find abort_search or stop set *
 *   we have to get out without backing up anything.  And   *
 *   at the root, we have the additional escape where we    *
 *   find abort_after_ply1 set and we still do not want to  *
 *   overwrite anything that has not already been backed up *
 *   when we called Output() to display a new PV (if we did *
 *   this.)                                                 *
 *                                                          *
 *   When we reach time_limit abort_after_ply1 gets set to  *
 *   1 and after all currently pending ply-1 moves are      *
 *   finished, we get here.  If any produced a new best     *
 *   move, Output() has already backed the PV up to the     *
 *   ply=0 PV so we are done.  If we go over the absolute   *
 *   max time limit, abort_search is set and that will      *
 *   immediately terminate the search and get us to this    *
 *   point.  Finally, if we are in a parallel search and    *
 *   some node above this one in the tree finds a fail high *
 *   condition, that thread will set the stop flag for any  *
 *   threads working below that node, and again we get to   *
 *   this point and do not want to back up anything.        *
 *                                                          *
 ************************************************************
 */
  if (abort_search || tree->stop || (ply == 1 && abort_after_ply1))
    return (0);
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
    int bestmove, type;
    bestmove = (alpha == o_alpha) ? first_tried : tree->pv[ply].path[ply];
    type = (alpha == o_alpha) ? UPPER : EXACT;
    if (repeat == 2 && alpha != -(MATE - ply - 1)) {
      value = DrawScore(wtm);
      if (value < beta)
        SavePV(tree, ply, 0);
#if defined(TRACE)
      if (ply <= trace_level)
        printf("draw by repetition detected, ply=%d.\n", ply);
#endif
      return (value);
    } else if (alpha != o_alpha) {
      memcpy(&tree->pv[ply - 1].path[ply], &tree->pv[ply].path[ply],
          (tree->pv[ply].pathl - ply) * sizeof(int));
      memcpy(&tree->pv[ply - 1].pathh, &tree->pv[ply].pathh, 3);
      tree->pv[ply - 1].path[ply - 1] = tree->curmv[ply - 1];
      Killer(tree, ply, tree->pv[ply].path[ply]);
    }
    HashStore(tree, ply, depth, wtm, type, alpha, bestmove);
    return (alpha);
  }
}

/* last modified 08/24/10 */
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
 *   all the threads that are helping here.                                    *
 *                                                                             *
 *******************************************************************************
 */
int SearchParallel(TREE * RESTRICT tree, int alpha, int beta, int value,
    int wtm, int depth, int ply) {
  BITBOARD begin_root_nodes;
  int extensions;

/*  
 ************************************************************
 *                                                          *
 *   Continue iterating through the move list and search    *
 *   the resulting positions.  Note that Search() culls any *
 *   move that is not legal by using Check().  Since this   *
 *   proceeding in parallel, we use the lock in the parent  *
 *   split-block so that all threads searching for that     *
 *   parent thread use the same move list and synchronize   *
 *   selecting the next move using the parent's lock.       *
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
          if (SwapO(tree, tree->curmv[ply], wtm) <= 0) {
            tree->extensions_done++;
            extensions = check_depth;
          }
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
 *       the LMR_remaining_depth value).                    *
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
            !extensions) {
          if ((Piece(tree->curmv[ply]) != pawn ||
                  mask_passed[wtm][To(tree->curmv[ply])] & Pawns(Flip(wtm))))
          {
            extensions =
                Min(-Min(depth - 1 - LMR_remaining_depth,
                    (tree->parent->moves_searched >
                        2) ? LMR_max_reduction : LMR_min_reduction), 0);
            if (extensions)
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
 *   If we reduce the search, and it fails high, we first   *
 *   do a verification search by using the original depth   *
 *   (no reduction) to see if it also fails high.  If not,  *
 *   we ignore the fail high and continue the search.       *
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
          value = -Quiesce(tree, -alpha - 1, -alpha, Flip(wtm), ply + 1, 1);
        if (abort_search || tree->stop)
          break;
/*
 ************************************************************
 *                                                          *
 *   This is the PVS re-search code.  If we reach this      *
 *   point and value > alpha and value < beta, then this    *
 *   can not be a null-window search.  We have to re-search *
 *   the position with the original beta value (not alpha+1 *
 *   as is the usual case in PVS) to see if it still fails  *
 *   high before we treat this as a real fail-high and back *
 *   up the value to the previous ply.                      *
 *                                                          *
 ************************************************************
 */
        if (value > alpha && value < beta) {
          extensions = Max(extensions, 0);
          if (depth + extensions - 1 > 0)
            value =
                -Search(tree, -beta, -alpha, Flip(wtm),
                depth + extensions - 1, ply + 1, DO_NULL);
          else
            value = -Quiesce(tree, -beta, -alpha, Flip(wtm), ply + 1, 1);
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
 *   Split-blocks are linked together so that we can walk   *
 *   this tree and terminate anyone helping us at this      *
 *   point in the tree, and also anyone helping at some     *
 *   deeper point on one of the sub-trees below this split  *
 *   point.  This stops anyone working on any subtree that  *
 *   has the current split point as an ancestor node.       *
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
            int proc;

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
        tree->parent->moves_searched++;
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
