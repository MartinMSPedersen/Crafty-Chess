#include "chess.h"
#include "data.h"
/* last modified 05/08/14 */
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
int Search(TREE * RESTRICT tree, int alpha, int beta, int side, int depth,
    int ply, int do_null) {
  ROOT_MOVE temp_rm;
  uint64_t start_nodes = tree->nodes_searched;
  int first_tried = 0, moves_searched = 0, repeat = 0;
  int original_alpha = alpha, value = 0, t_beta = beta;
  int extend, reduce, max_reduce, i;
  int pv_node = alpha != beta - 1;

/*
 ************************************************************
 *                                                          *
 *  Step 1.  Check to see if we have searched enough nodes  *
 *  that it is time to peek at how much time has been used, *
 *  or if is time to check for operator keyboard input.     *
 *  This is usually enough nodes to force a time/input      *
 *  check about once per second, except when the target     *
 *  time per move is very small, in which case we try to    *
 *  check the time more frequently.                         *
 *                                                          *
 *  Note that we check input or time-out in thread 0.  This *
 *  makes the code simpler and eliminates some problematic  *
 *  race conditions.                                        *
 *                                                          *
 ************************************************************
 */
#if defined(NODES)
  if (--temp_search_nodes <= 0) {
    abort_search = 1;
    return 0;
  }
#endif
  if (tree->thread_id == 0) {
    if (--next_time_check <= 0) {
      next_time_check = nodes_between_time_checks;
      if (TimeCheck(tree, 1)) {
        abort_search = 1;
        return 0;
      }
      if (CheckInput()) {
        Interrupt(ply);
        if (abort_search)
          return 0;
      }
    }
  }
  if (ply >= MAXPLY - 1)
    return beta;
/*
 ************************************************************
 *                                                          *
 *  Step 2.  Check for draw by repetition, which includes   *
 *  50 move draws also.  This is the quickest way to get    *
 *  out of further searching, with minimal effort.  This    *
 *  and the next two steps are skipped for moves at the     *
 *  root (ply = 1).                                         *
 *                                                          *
 ************************************************************
 */
  if (ply > 1) {
    if ((repeat = Repeat(tree, ply))) {
      value = DrawScore(side);
      if (value < beta)
        SavePV(tree, ply, 0);
#if defined(TRACE)
      if (ply <= trace_level)
        printf("draw by repetition detected, ply=%d.\n", ply);
#endif
      return value;
    }
/*
 ************************************************************
 *                                                          *
 *  Step 3.  Check the transposition/refutation (hash)      *
 *  table to see if we have searched this position          *
 *  previously and still have the results available.  We    *
 *  might get a real score, or a bound, or perhaps only a   *
 *  good move to try first.  The possible results are:      *
 *                                                          *
 *  1. HashProbe() returns "HASH_HIT".  This terminates     *
 *  the search instantly and we simply return the value     *
 *  found in the hash table.  This value is simply the      *
 *  value we found when we did a real search in this        *
 *  position previously, and HashProbe() verifies that the  *
 *  value is useful based on draft and current bounds.      *
 *                                                          *
 *  2. HashProbe() returns "AVOID_NULL_MOVE" which means    *
 *  the hashed score/bound was no good, but it indicated    *
 *  that trying a null-move in this position would be a     *
 *  waste of time since it will likely fail low, not high.  *
 *                                                          *
 *  3. HashProbe() returns "HASH_MISS" when forces us to    *
 *  do a normal search to resolve this node.                *
 *                                                          *
 ************************************************************
 */
    switch (HashProbe(tree, ply, depth, side, alpha, beta, &value)) {
      case HASH_HIT:
        return value;
      case AVOID_NULL_MOVE:
        do_null = 0;
      case HASH_MISS:
        break;
    }
/*
 ************************************************************
 *                                                          *
 *  Step 4.  Now it's time to try a probe into the endgame  *
 *  tablebase files.  This is done if we notice that there  *
 *  are 6 or fewer pieces left on the board.  EGTB_use      *
 *  tells us how many pieces to probe on.  Note that this   *
 *  can be zero when trying to swindle the opponent, so     *
 *  that no probes are done since we know it is a draw.     *
 *  This is another way to get out of the search quickly,   *
 *  but not as quickly as the previous steps since this can *
 *  result in an I/O operation.                             *
 *                                                          *
 *  Note that in "swindle mode" this can be turned off by   *
 *  Iterate() setting "EGTB_use = 0" so that we won't probe *
 *  the EGTBs since we are searching only the root moves    *
 *  that lead to a draw and we want to play the move that   *
 *  makes the draw more difficult to reach by the opponent  *
 *  to give him a chance to make a mistake.                 *
 *                                                          *
 *  Another special case is that we slightly fudge the      *
 *  score for draws.  In a normal circumstance, draw=0.00   *
 *  since it is "equal".  However, here we add 0.01 if      *
 *  white has more material, or subtract 0.01 if black has  *
 *  more material, since in a drawn KRP vs KR we would      *
 *  prefer to have the KRP side since the opponent can make *
 *  a mistake and convert the draw to a loss.               *
 *                                                          *
 ************************************************************
 */
#if !defined(NOEGTB)
    if (depth > 6 && TotalAllPieces <= EGTB_use &&
        Castle(ply, white) + Castle(ply, black) == 0 &&
        (CaptureOrPromote(tree->curmv[ply - 1]) || ply < 3)) {
      int egtb_value;

      tree->egtb_probes++;
      if (EGTBProbe(tree, ply, side, &egtb_value)) {
        tree->egtb_probes_successful++;
        alpha = egtb_value;
        if (MateScore(alpha))
          alpha += (alpha > 0) ? -ply + 1 : ply;
        else if (alpha == 0) {
          alpha = DrawScore(side);
          if (Material > 0)
            alpha += (side) ? 1 : -1;
          else if (Material < 0)
            alpha -= (side) ? 1 : -1;
        }
        if (alpha < beta)
          SavePV(tree, ply, 2);
        return alpha;
      }
    }
#endif
/*
 ************************************************************
 *                                                          *
 *  Step 5.  We now know there is no quick way to get out   *
 *  of here, which leaves one more possibility, although it *
 *  does require s search, but to a reduced depth.  We      *
 *  try a null move to see if we can get a quick cutoff     *
 *  with only a little work.  This operates as follows.     *
 *  Instead of making a legal move, the side on move passes *
 *  and does nothing.  The resulting position is searched   *
 *  to a shallower depth than normal (usually 3 plies less  *
 *  but settable by the user.) This will result in a cutoff *
 *  if our position is very good, but it produces the       *
 *  cutoff much quicker since the search is far shallower   *
 *  than a normal search that would also be likely to fail  *
 *  high.                                                   *
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
    if (do_null && !pv_node && depth > 1 && !tree->inchk[ply]
        && TotalPieces(side, occupied)) {
      uint64_t save_hash_key;

      tree->curmv[ply] = 0;
      tree->phase[ply] = NULL_MOVE;
#if defined(TRACE)
      if (ply <= trace_level)
        Trace(tree, ply, depth, side, beta - 1, beta, "Search1", NULL_MOVE);
#endif
      tree->status[ply + 1] = tree->status[ply];
      Reversible(ply + 1) = 0;
      save_hash_key = HashKey;
      if (EnPassant(ply + 1)) {
        HashEP(EnPassant(ply + 1));
        EnPassant(ply + 1) = 0;
      }
      if (depth - null_depth - 1 > 0)
        value =
            -Search(tree, -beta, -beta + 1, Flip(side),
            depth - null_depth - 1, ply + 1, NO_NULL);
      else
        value = -Quiesce(tree, -beta, -beta + 1, Flip(side), ply + 1, 1);
      HashKey = save_hash_key;
      if (abort_search || tree->stop)
        return 0;
      if (value >= beta) {
        HashStore(tree, ply, depth, side, LOWER, value, tree->hash_move[ply]);
        return value;
      }
    }
/*
 ************************************************************
 *                                                          *
 *  Step 6.  This step is rarely executed.  It is used when *
 *  there is no best move from the hash table, and this is  *
 *  a PV node, since we need a good move to search first.   *
 *  While killers moves are good, they are not quite good   *
 *  enough.  the simplest solution is to try a shallow      *
 *  search (depth-2) to get a move.  note that when we call *
 *  Search() with depth-2, it, too, will not have a hash    *
 *  move, and will therefore recursively continue this      *
 *  process, hence the name "internal iterative deepening." *
 *                                                          *
 ************************************************************
 */
    tree->next_status[ply].phase = HASH_MOVE;
    if (!tree->hash_move[ply] && depth >= 6 && do_null && ply > 1) {
      if (pv_node) {
        do {
          tree->curmv[ply] = 0;
          if (depth - 2 > 0)
            value = Search(tree, alpha, beta, side, depth - 2, ply, DO_NULL);
          else
            value = Quiesce(tree, alpha, beta, side, ply, 1);
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
      }
    }
  }
/*  
 ************************************************************
 *                                                          *
 *  Step 7.  Now iterate through the move list and search   *
 *  the resulting positions.  Note that Search() culls any  *
 *  move that is not legal by using Check().  The special   *
 *  case is that we must find one legal move to search to   *
 *  confirm that it's not a mate or draw.                   *
 *                                                          *
 *  We have three possible procedures we call here, one is  *
 *  specific to ply=1 (NextRootMove()), the second is a     *
 *  specific routine to escape check (NextEvasion()) and    *
 *  the last is the normal NextMove() procedure that does   *
 *  the usual move ordering stuff.                          *
 *                                                          *
 *  Special case:  if we have searched one move at root,    *
 *  and the returned score == alpha, we want to get out of  *
 *  here and return to Iterate() where the search bounds    *
 *  will be adjusted.  Otherwise we would search all root   *
 *  moves and possibly fail low after expending a sig-      *
 *  nificant amount of time.                                *
 *                                                          *
 ************************************************************
 */
  tree->next_status[ply].phase = HASH_MOVE;
  while (1) {
    if (ply > 1)
      tree->phase[ply] =
          (tree->inchk[ply]) ? NextEvasion(tree, ply, side) : NextMove(tree,
          ply, side);
    else if (moves_searched == 1 && alpha == original_alpha)
      break;
    else
      tree->phase[ply] = NextRootMove(tree, tree, side);
    if (!tree->phase[ply])
      break;
#if defined(TRACE)
    if (ply <= trace_level)
      Trace(tree, ply, depth, side, alpha, beta, "Search2", tree->phase[ply]);
#endif
    MakeMove(tree, ply, tree->curmv[ply], side);
    tree->nodes_searched++;
    if (tree->inchk[ply] || !Check(side))
      do {
        if (++moves_searched == 1)
          first_tried = tree->curmv[ply];
/*
 ************************************************************
 *                                                          *
 *  Step 7a.  If the move to be made checks the opponent,   *
 *  then we need to remember that he's in check and also    *
 *  extend the depth by one ply for him to get out.  Note   *
 *  that if the move gives check, it is not a candidate for *
 *  either depth reduction or forward-pruning.              *
 *                                                          *
 *  We do not extend unsafe checking moves (as indicated by *
 *  Swap(), a SEE algorithm, since these are usually a      *
 *  waste of time and simply blow up the tree search space. *
 *                                                          *
 ************************************************************
 */
        extend = 0;
        reduce = 0;
        if (Check(Flip(side))) {
          tree->inchk[ply + 1] = 1;
          if (SwapO(tree, tree->curmv[ply], side) <= 0) {
            extend = check_depth;
            tree->extensions_done++;
          }
        } else
          tree->inchk[ply + 1] = 0;
/*
 ************************************************************
 *                                                          *
 *  Step 7b.  Now for the forward-pruning stuff.  The idea  *
 *  here is based on the old FUTILITY idea, where if the    *
 *  current material + a fudge factor is lower than alpha,  *
 *  then there is little promise in searching this move to  *
 *  make up for the material deficit.  This is not done if  *
 *  we chose to extend this move in the previous step.      *
 *                                                          *
 *  This is a useful idea in today's 20+ ply searches, as   *
 *  when we near the tips, if we are too far behind in      *
 *  material, there is little time left to recover it and   *
 *  moves that don't bring us closer to a reasonable        *
 *  material balance can safely be skipped.  It is much     *
 *  more dangerous in shallow searches.                     *
 *                                                          *
 *  We have an array of pruning margin values that are      *
 *  indexed by depth (remaining plies left until we drop    *
 *  into the quiescence search) and which increase with     *
 *  depth since more depth means a greater chance of        *
 *  bringing the score back up to alpha or beyond.  If the  *
 *  current material + the bonus is less than alpha, we     *
 *  simply avoid searching this move at all, and skip to    *
 *  the next move without expending any more effort.  Note  *
 *  that this is classic forward-pruning and can certainly  *
 *  introduce errors into the search.  However, cluster     *
 *  testing has shown that this improves play in real       *
 *  games.  The current implementation only prunes in the   *
 *  last 5 plies before quiescence, although this can be    *
 *  tuned with the "eval" command changing the "pruning     *
 *  depth" value to something other than 6 (test is for     *
 *  depth < pruning depth, current value is 6 which prunes  *
 *  in last 5 plies only).  Testing shows no benefit in     *
 *  larger values than 6, although this might change in     *
 *  future versions as other things are modified.           *
 *                                                          *
 *  Exception:                                              *
 *                                                          *
 *    We do not prune if we are safely pushing a passed     *
 *    pawn to the 6th rank, where it becomes very dangerous *
 *    since it can promote in two more moves.               *
 *                                                          *
 ************************************************************
 */
        if (tree->phase[ply] == REMAINING_MOVES && !tree->inchk[ply]
            && !extend && moves_searched > 1) {
          if (depth < pruning_depth &&
              MaterialSTM(side) + pruning_margin[depth] <= alpha) {
            if (Piece(tree->curmv[ply]) != pawn ||
                !Passed(To(tree->curmv[ply]), side)
                || rankflip[side][Rank(To(tree->curmv[ply]))] < RANK6) {
              tree->moves_fpruned++;
              continue;
            }
          }
/*
 ************************************************************
 *                                                          *
 *  Step 7c.  Now it's time to try to reduce the search     *
 *  depth if the move appears to be "poor".  To reduce the  *
 *  search, the following requirements must be met:         *
 *                                                          *
 *  (1) We must be in the REMAINING_MOVES part of the move  *
 *      ordering, so that we have nearly given up on        *
 *      failing high on any move.                           *
 *                                                          *
 *  (2) We must not be too close to the horizon (this is    *
 *      the LMR_remaining_depth value).                     *
 *                                                          *
 *  (3) The current move must not be a checking move and    *
 *      the side to move can not be in check.               *
 *                                                          *
 ************************************************************
 */
          if (Piece(tree->curmv[ply]) != pawn ||
              !Passed(To(tree->curmv[ply]), side)
              || rankflip[side][Rank(To(tree->curmv[ply]))] < RANK6) {
            reduce = LMR_min_reduction;
            if (moves_searched > 3)
              reduce = LMR_max_reduction;
            max_reduce = Max(depth - 1 - LMR_remaining_depth, 0);
            if (reduce > max_reduce)
              reduce = max_reduce;
            if (reduce)
              tree->reductions_done++;
          }
        }
/*  
 ************************************************************
 *                                                          *
 *  Step 7d.  We have determined whether the depth is to    *
 *  be changed by an extension or a reduction.  If we get   *
 *  to this point, then the move is not being pruned.  So   *
 *  off we go to a recursive search/quiescence call to work *
 *  our way toward a terminal node.                         *
 *                                                          *
 *  There is one special-case to handle.  If the depth was  *
 *  reduced, and Search() returns a value >= beta then      *
 *  accepting that is risky (we reduced the move as we      *
 *  thought it was bad and expected it to fail low) so we   *
 *  repeat the search using the original (non-reduced)      *
 *  depth to see if the fail-high happens again.            *
 *                                                          *
 ************************************************************
 */
        if (depth + extend - reduce - 1 > 0) {
          value =
              -Search(tree, -t_beta, -alpha, Flip(side),
              depth + extend - reduce - 1, ply + 1, DO_NULL);
          if (value > alpha && reduce)
            value =
                -Search(tree, -t_beta, -alpha, Flip(side), depth - 1, ply + 1,
                DO_NULL);
        } else
          value = -Quiesce(tree, -t_beta, -alpha, Flip(side), ply + 1, 1);
        if (abort_search || tree->stop)
          break;
/*
 ************************************************************
 *                                                          *
 *  Step 7e.  This is the PVS re-search code.  If we reach  *
 *  this point and value > alpha and value < beta, then     *
 *  this can not be a null-window search.  We have to re-   *
 *  search the position with the original beta value (not   *
 *  alpha+1 as is the usual case in PVS) to see if it still *
 *  fails high before we treat this as a real fail-high and *
 *  back up the value to the previous ply.                  *
 *                                                          *
 *  Special case:  ply == 1.                                *
 *                                                          *
 *  In this case, we need to clean up and then move the     *
 *  best move to the top of the root move list, and return  *
 *  back to Iterate() to let it produce the usual informa-  *
 *  tive output and re-start the search with a new beta     *
 *  value.  We also reset the failhi_delta back to 16,      *
 *  since an earlier fail-high or fail low in this          *
 *  iteration could have left it at a large value.          *
 *                                                          *
 *  Last step is to build a usable PV in case this move     *
 *  fails low on the re-search, because we do want to play  *
 *  this move no matter what happens.                       *
 *                                                          *
 ************************************************************
 */
        if (value > alpha && value < beta && moves_searched > 1) {
          if (ply == 1) {
            alpha = value;
            UnmakeMove(tree, ply, tree->curmv[ply], side);
            root_beta = alpha;
            failhi_delta = 16;
            for (i = 0; i < n_root_moves; i++)
              if (tree->curmv[1] == root_moves[i].move)
                break;
            if (i < n_root_moves) {
              temp_rm = root_moves[i];
              for (; i > 0; i--)
                root_moves[i] = root_moves[i - 1];
              root_moves[0] = temp_rm;
            }
            root_moves[0].bm_age = 4;
            tree->pv[1].path[1] = tree->curmv[1];
            tree->pv[1].pathl = 2;
            tree->pv[1].pathh = 0;
            tree->pv[1].pathd = iteration_depth;
            tree->pv[0] = tree->pv[1];
            return alpha;
          }
          if (depth + extend - 1 > 0)
            value =
                -Search(tree, -beta, -alpha, Flip(side), depth + extend - 1,
                ply + 1, DO_NULL);
          else
            value = -Quiesce(tree, -beta, -alpha, Flip(side), ply + 1, 1);
          if (abort_search || tree->stop)
            break;
        }
/*  
 ************************************************************
 *                                                          *
 *  Step 7f.  We have completed the search/re-search and we *
 *  we have the final score.  Now we need to check for a    *
 *  fail-high which terminates this search instantly since  *
 *  no further searching is required.  On a fail high, we   *
 *  need to update the killer moves, and hash table before  *
 *  we return.                                              *
 *                                                          *
 *  If ply == 1, we call Output() which will dump the new   *
 *  PV.  But but we need to back up the PV to ply=0 so that *
 *  it will be available to tell main() which move to make. *
 *                                                          *
 ************************************************************
 */
        if (value > alpha) {
          alpha = value;
          if (ply == 1) {
            tree->pv[1].pathv = value;
            Output(tree, beta);
            tree->pv[0] = tree->pv[1];
          }
          if (value >= beta) {
            Killer(tree, ply, tree->curmv[ply]);
            UnmakeMove(tree, ply, tree->curmv[ply], side);
            HashStore(tree, ply, depth, side, LOWER, value, tree->curmv[ply]);
            tree->fail_highs++;
            if (moves_searched == 1)
              tree->fail_high_first_move++;
            return value;
          }
        }
        t_beta = alpha + 1;
      } while (0);
    UnmakeMove(tree, ply, tree->curmv[ply], side);
    if (abort_search || tree->stop)
      return 0;
/*
 ************************************************************
 *                                                          *
 *  Step 7g.  If are doing an SMP search, and we have idle  *
 *  processors, now is the time to get them involved.  We   *
 *  have now satisfied the "young brothers wait" condition  *
 *  since we have searched one move.  All that is left is   *
 *  to check the split constraints to see if we are an      *
 *  acceptable split point.                                 *
 *                                                          *
 *    (1) We can't split within N plies of the frontier     *
 *        nodes to avoid excessive split overhead.          *
 *                                                          *
 *    (2) We can't split until at least M nodes have been   *
 *        searched since this thread was last split, to     *
 *        avoid splitting too often, mainly in endgames.    *
 *                                                          *
 *    (3) We have to have searched one legal move to avoid  *
 *        splitting at a node where we have no legal moves  *
 *        (the first move tried might have been illegal as  *
 *        in when we encounter a stalemate).                *
 *                                                          *
 *    (4) If we are at ply=1, we can't split unless the     *
 *        smp_split_at_root flag is set to 1, AND the next  *
 *        move in the ply=1 move list is not flagged as     *
 *        "do not search in parallel" which happens when    *
 *        this move was a best move in the last couple of   *
 *        searches and we want all processors on it at once *
 *        to get a score back quicker.                      *
 *                                                          *
 *    (5) if the variable smp_split is > 0, we have idle    *
 *        threads that can help, however if smp_split < 0,  *
 *        we are already doing a split on another thread    *
 *        so there is no point in waiting to try one here.  *
 *                                                          *
 *  SearchParallel() primarily contains steps 7 through 7f  *
 *  which is the main search loop.  We do the final clean-  *
 *  up below when either we finish the search normally or   *
 *  a parallel search completes and returns to this point.  *
 *                                                          *
 *  Special case:  we do not split if we are at ply=1 and   *
 *  alpha == original_alpha.  That means the first move     *
 *  failed low, and we are going to exit search and return  *
 *  to Iterate() to report this.                            *
 *                                                          *
 *  One potential problem is that multiple threads can get  *
 *  to this point at the same time, and they all stack up   *
 *  waiting to grab the lock_smp lock that protects the     *
 *  split operation.  I now have a new lock, lock_split,    *
 *  to try to limit this wasted time.  A thread now has to  *
 *  acquire that lock, and then it tests the smp_split      *
 *  to see if a split STILL needs to be done.  If not, we   *
 *  release the lock and move on, rather than waiting on    *
 *  main lock_smp lock.                                     *
 *                                                          *
 *  If we acquire the lock_split lock AND we notice that    *
 *  smp_split is still set to 1, we quickly set smp_split   *
 *  to zero (-1) so that other threads will bug out rather  *
 *  than trying to split and end up in a queue behind us,   *
 *  waiting while we split and they try to split and fail.  *
 *  We release lock_split to eliminate the log-jam of       *
 *  threads waiting to split and get them back into their   *
 *  normal searches, and jump right into Thread().          *
 *                                                          *
 *  The smp_split = -1 has a complex meaning, but simply    *
 *  stated it means "split currently in progress".  Here,   *
 *  that means "do not attempt a split since we are already *
 *  in the middle of one."  It also tells individual        *
 *  threads to not change smp_split to 1 when they become   *
 *  idle, because with a split in progress, it is likely we *
 *  will pick them up automatically.                        *
 *                                                          *
 ************************************************************
 */
#if (CPUS > 1)
    if (smp_split > 0 && depth >= smp_min_split_depth && moves_searched &&
        tree->nodes_searched - start_nodes > smp_split_nodes && (ply > 1 ||
            (smp_split_at_root && NextRootMoveParallel() &&
                alpha != original_alpha)))
      do {
        Lock(lock_split);
        if (smp_split <= 0) {
          Unlock(lock_split);
          break;
        }
        smp_split = -1;
        Unlock(lock_split);
        tree->alpha = alpha;
        tree->beta = beta;
        tree->value = alpha;
        tree->side = side;
        tree->ply = ply;
        tree->depth = depth;
        tree->moves_searched = moves_searched;
        if (Thread(tree)) {
          if (abort_search || tree->stop)
            return 0;
          if (tree->thread_id == 0 && CheckInput())
            Interrupt(ply);
          value = tree->value;
          if (value > alpha) {
            if (value >= beta) {
              HashStore(tree, ply, depth, side, LOWER, value, tree->cutmove);
              return value;
            }
            alpha = value;
            break;
          }
        }
      } while (0);
#endif
  }
/*
 ************************************************************
 *                                                          *
 *  Step 8.  All moves have been searched.  If none were    *
 *  legal, return either MATE or DRAW depending on whether  *
 *  the side to move is in check or not.                    *
 *                                                          *
 ************************************************************
 */
  if (abort_search || tree->stop)
    return 0;
  if (moves_searched == 0) {
    value = (Check(side)) ? -(MATE - ply) : DrawScore(side);
    if (value >= alpha && value < beta) {
      SavePV(tree, ply, 0);
#if defined(TRACE)
      if (ply <= trace_level)
        printf("Search() no moves!  ply=%d\n", ply);
#endif
    }
    return value;
  } else {
    int bestmove, type;
    bestmove =
        (alpha == original_alpha) ? first_tried : tree->pv[ply].path[ply];
    type = (alpha == original_alpha) ? UPPER : EXACT;
    if (repeat == 2 && alpha != -(MATE - ply - 1)) {
      value = DrawScore(side);
      if (value < beta)
        SavePV(tree, ply, 0);
#if defined(TRACE)
      if (ply <= trace_level)
        printf("draw by 50 move rule detected, ply=%d.\n", ply);
#endif
      return value;
    } else if (alpha != original_alpha) {
      tree->pv[ply - 1] = tree->pv[ply];
      tree->pv[ply - 1].path[ply - 1] = tree->curmv[ply - 1];
      Killer(tree, ply, tree->pv[ply].path[ply]);
    }
    HashStore(tree, ply, depth, side, type, alpha, bestmove);
    return alpha;
  }
}

/* last modified 05/08/14 */
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
    int side, int depth, int ply) {
  ROOT_MOVE temp_rm;
  int extend, reduce, max_reduce, i;

/*
 ************************************************************
 *                                                          *
 *  Step 7.  Now we continue to iterate through the move    *
 *  list and search the resulting positions.  Note that     *
 *  SearchParallel() culls any move that is not legal by    *
 *  using Check().  The special case mentioned in Search()  *
 *  is not an issue here as we don't do a parallel split    *
 *  until we have searched one legal move.                  *
 *                                                          *
 *  We have three possible procedures we call here, one is  *
 *  specific to ply=1 (NextRootMove()), the second is a     *
 *  specific routine to escape check (NextEvasion()) and    *
 *  the last is the normal NextMove() procedure that does   *
 *  the usual move ordering stuff.                          *
 *                                                          *
 ************************************************************
 */

  while (1) {
    Lock(tree->parent->lock);
    if (ply > 1)
      tree->phase[ply] =
          (tree->inchk[ply]) ? NextEvasion((TREE *) tree->parent, ply,
          side) : NextMove((TREE *)
          tree->parent, ply, side);
    else
      tree->phase[ply] = NextRootMove(tree->parent, tree, side);
    tree->curmv[ply] = tree->parent->curmv[ply];
    Unlock(tree->parent->lock);
    if (!tree->phase[ply])
      break;
#if defined(TRACE)
    if (ply <= trace_level)
      Trace(tree, ply, depth, side, alpha, beta, "SearchParallel",
          tree->phase[ply]);
#endif
    MakeMove(tree, ply, tree->curmv[ply], side);
    tree->nodes_searched++;
    if (tree->inchk[ply] || !Check(side))
      do {
        tree->parent->moves_searched++;
/*
 ************************************************************
 *                                                          *
 *  Step 7a.  If the move to be made checks the opponent,   *
 *  then we need to remember that he's in check and also    *
 *  extend the depth by one ply for him to get out.  Note   *
 *  that if the move gives check, it is not a candidate for *
 *  either depth reduction or forward-pruning.              *
 *                                                          *
 *  We do not extend unsafe checking moves (as indicated by *
 *  Swap(), a SEE algorithm, since these are usually a      *
 *  waste of time and simply blow up the tree search space. *
 *                                                          *
 ************************************************************
 */
        extend = 0;
        reduce = 0;
        if (Check(Flip(side))) {
          tree->inchk[ply + 1] = 1;
          if (SwapO(tree, tree->curmv[ply], side) <= 0) {
            extend = check_depth;
            tree->extensions_done++;
          }
        } else
          tree->inchk[ply + 1] = 0;
/*
 ************************************************************
 *                                                          *
 *  Step 7b.  Now for the forward-pruning stuff.  The idea  *
 *  here is based on the old FUTILITY idea, where if the    *
 *  current material + a fudge factor is lower than alpha,  *
 *  then there is little promise in searching this move to  *
 *  make up for the material deficit.  This is not done if  *
 *  we chose to extend this move in the previous step.      *
 *                                                          *
 *  This is a useful idea in today's 20+ ply searches, as   *
 *  when we near the tips, if we are too far behind in      *
 *  material, there is little time left to recover it and   *
 *  moves that don't bring us closer to a reasonable        *
 *  material balance can safely be skipped.  It is much     *
 *  more dangerous in shallow searches.                     *
 *                                                          *
 *  We have an array of pruning margin values that are      *
 *  indexed by depth (remaining plies left until we drop    *
 *  into the quiescence search) and which increase with     *
 *  depth since more depth means a greater chance of        *
 *  bringing the score back up to alpha or beyond.  If the  *
 *  current material + the bonus is less than alpha, we     *
 *  simply avoid searching this move at all, and skip to    *
 *  the next move without expending any more effort.  Note  *
 *  that this is classic forward-pruning and can certainly  *
 *  introduce errors into the search.  However, cluster     *
 *  testing has shown that this improves play in real       *
 *  games.  The current implementation only prunes in the   *
 *  last 5 plies before quiescence, although this can be    *
 *  tuned with the "eval" command changing the "pruning     *
 *  depth" value to something other than 6 (test is for     *
 *  depth < pruning depth, current value is 6 which prunes  *
 *  in last 5 plies only).  Testing shows no benefit in     *
 *  larger values than 6, although this might change in     *
 *  future versions as other things are modified.           *
 *                                                          *
 *  Exception:                                              *
 *                                                          *
 *    We do not prune if we are safely pushing a passed     *
 *    pawn to the 6th rank, where it becomes very dangerous *
 *    since it can promote in two more moves.               *
 *                                                          *
 ************************************************************
 */
        if (tree->phase[ply] == REMAINING_MOVES && !tree->inchk[ply]
            && !extend && tree->parent->moves_searched > 1) {
          if (depth < pruning_depth &&
              MaterialSTM(side) + pruning_margin[depth] <= alpha) {
            if (Piece(tree->curmv[ply]) != pawn ||
                !Passed(To(tree->curmv[ply]), side)
                || rankflip[side][Rank(To(tree->curmv[ply]))] < RANK6) {
              tree->moves_fpruned++;
              continue;
            }
          }
/*
 ************************************************************
 *                                                          *
 *  Step 7c.  Now it's time to try to reduce the search     *
 *  depth if the move appears to be "poor".  To reduce the  *
 *  search, the following requirements must be met:         *
 *                                                          *
 *  (1) We must be in the REMAINING_MOVES part of the move  *
 *      ordering, so that we have nearly given up on        *
 *      failing high on any move.                           *
 *                                                          *
 *  (2) We must not be too close to the horizon (this is    *
 *      the LMR_remaining_depth value).                     *
 *                                                          *
 *  (3) The current move must not be a checking move and    *
 *      the side to move can not be in check.               *
 *                                                          *
 ************************************************************
 */
          if (Piece(tree->curmv[ply]) != pawn ||
              !Passed(To(tree->curmv[ply]), side)
              || rankflip[side][Rank(To(tree->curmv[ply]))] < RANK6) {
            reduce = LMR_min_reduction;
            if (tree->parent->moves_searched > 3)
              reduce = LMR_max_reduction;
            max_reduce = Max(depth - 1 - LMR_remaining_depth, 0);
            if (reduce > max_reduce)
              reduce = max_reduce;
            if (reduce)
              tree->reductions_done++;
          }
        }
/*
 ************************************************************
 *                                                          *
 *  Step 7d.  We have determined whether the depth is to    *
 *  be changed by an extension or a reduction.  If we get   *
 *  to this point, then the move is not being pruned.  So   *
 *  off we go to a recursive search/quiescence call to work *
 *  our way toward a terminal node.                         *
 *                                                          *
 *  There is one special-cases to handle.  If the depth was *
 *  reduced, and Search() returns a value >= beta then      *
 *  accepting that is risky (we reduced the move as we      *
 *  thought it was bad and expected it to fail low) so we   *
 *  repeat the search using the original (non-reduced)      *
 *  depth to see if the fail-high happens again.            *
 *                                                          *
 ************************************************************
 */
        if (depth + extend - reduce - 1 > 0) {
          value =
              -Search(tree, -alpha - 1, -alpha, Flip(side),
              depth + extend - reduce - 1, ply + 1, DO_NULL);
          if (value > alpha && reduce)
            value =
                -Search(tree, -alpha - 1, -alpha, Flip(side), depth - 1,
                ply + 1, DO_NULL);
        } else
          value = -Quiesce(tree, -alpha - 1, -alpha, Flip(side), ply + 1, 1);
        if (abort_search || tree->stop)
          break;
/*
 ************************************************************
 *                                                          *
 *  Step 7e.  This is the PVS re-search code.  If we reach  *
 *  this point and value > alpha and value < beta, then     *
 *  this can not be a null-window search.  We have to re-   *
 *  search the position with the original beta value (not   *
 *  alpha+1 as is the usual case in PVS) to see if it still *
 *  fails high before we treat this as a real fail-high and *
 *  back up the value to the previous ply.                  *
 *                                                          *
 *  Special case:  ply == 1.                                *
 *                                                          *
 *  In this case, we need to clean up and then move the     *
 *  best move to the top of the root move list, and then    *
 *  return back to the normal Search(), which will then     *
 *  return back to Iterate() to let it produce the usual    *
 *  informative output and re-start the search with a new   *
 *  beta value.  We also reset the failhi_delta back to 16, *
 *  since an earlier fail-high or fail low in this          *
 *  iteration could have left it at a large value.          *
 *                                                          *
 *  Last step is to build a usable PV in case this move     *
 *  fails low on the re-search, because we do want to play  *
 *  this move no matter what happens.                       *
 *                                                          *
 ************************************************************
 */
        if (value > alpha && value < beta) {
          if (ply == 1) {
            int proc;

            alpha = value;
            parallel_aborts++;
            UnmakeMove(tree, ply, tree->curmv[ply], side);
            Lock(lock_smp);
            Lock(tree->parent->lock);
            if (!tree->stop) {
              for (proc = 0; proc < smp_max_threads; proc++)
                if (tree->parent->siblings[proc] && proc != tree->thread_id)
                  ThreadStop(tree->parent->siblings[proc]);
              root_beta = alpha;
              failhi_delta = 16;
              Lock(lock_root);
              for (i = 0; i < n_root_moves; i++)
                if (tree->curmv[1] == root_moves[i].move)
                  break;
              if (i < n_root_moves) {
                temp_rm = root_moves[i];
                for (; i > 0; i--)
                  root_moves[i] = root_moves[i - 1];
                root_moves[0] = temp_rm;
              }
              root_moves[0].bm_age = 4;
              Unlock(lock_root);
              tree->pv[1].path[1] = tree->curmv[1];
              tree->pv[1].pathl = 2;
              tree->pv[1].pathh = 0;
              tree->pv[1].pathd = iteration_depth;
              tree->pv[0] = tree->pv[1];
            }
            Unlock(tree->parent->lock);
            Unlock(lock_smp);
            return alpha;
          }
          if (depth + extend - 1 > 0)
            value =
                -Search(tree, -beta, -alpha, Flip(side), depth + extend - 1,
                ply + 1, DO_NULL);
          else
            value = -Quiesce(tree, -beta, -alpha, Flip(side), ply + 1, 1);
          if (abort_search || tree->stop)
            break;
        }
/*
 ************************************************************
 *                                                          *
 *  Step 7f.  We have completed the search/re-search and we *
 *  we have the final score.  Now we need to check for a    *
 *  fail-high which terminates this search instantly since  *
 *  no further searching is required.  On a fail high, we   *
 *  need to update the killer moves, and hash table before  *
 *  we return.                                              *
 *                                                          *
 *  Note that we can not produce a new PV here.  At best,   *
 *  we can produce a fail-high which will abort other       *
 *  threads at this node (wasting time).                    *
 *                                                          *
 *  Special case:  If ply == 1, and we fail high on the     *
 *  null-window search, we simply abort the search and then *
 *  return to the normal search, which will back us out to  *
 *  Iterate() and inform the user and re-start the search.  *
 *                                                          *
 *  We then stop all threads (except the current thread     *
 *  that is dealing with the fail high) since we are going  *
 *  to back out quickly and then start a new search from    *
 *  the root position.  The split-block sibling ids lets us *
 *  know which threads should be stopped, and since we are  *
 *  at the root (ply == 1) that essentially means "all      *
 *  threads except for this one."                           *
 *                                                          *
 ************************************************************
 */
        if (value > alpha) {
          alpha = value;
          if (value >= beta) {
            int proc;

            parallel_aborts++;
            UnmakeMove(tree, ply, tree->curmv[ply], side);
            Lock(lock_smp);
            Lock(tree->parent->lock);
            if (!tree->stop)
              for (proc = 0; proc < smp_max_threads; proc++)
                if (tree->parent->siblings[proc] && proc != tree->thread_id)
                  ThreadStop(tree->parent->siblings[proc]);
            Unlock(tree->parent->lock);
            Unlock(lock_smp);
            return alpha;
          }
        }
      } while (0);
    UnmakeMove(tree, ply, tree->curmv[ply], side);
    if (abort_search || tree->stop)
      break;
  }
/*
 ************************************************************
 *                                                          *
 *  Step 8.  We are doing an SMP search, so there are no    *
 *  "end-of-search" things to do.  We have searched all the *
 *  remaining moves at this ply in parallel, and now return *
 *  and let the original search that started this sub-tree) *
 *  clean up, and do the tests for mate/stalemate, update   *
 *  the hash table, etc.                                    *
 *                                                          *
 *  As we return, we end back up in Thread() where we       *
 *  started, which then copies the best score/etc back to   *
 *  the parent thread.                                      *
 *                                                          *
 *  We do need to flag the root move we tried to search, if *
 *  we were stopped early due to another root move failing  *
 *  high.  Otherwise this move appears to have been         *
 *  searched already and will not be searched again until   *
 *  the next iteration.                                     *
 *                                                          *
 ************************************************************
 */
  if (tree->stop && ply == 1) {
    int which;

    Lock(lock_root);
    for (which = 0; which < n_root_moves; which++)
      if (root_moves[which].move == tree->curmv[ply]) {
        root_moves[which].status &= 0xf7;
        break;
      }
    Unlock(lock_root);
  }
  return alpha;
}
