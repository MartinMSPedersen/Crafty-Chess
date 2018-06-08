#include <math.h>
#include "chess.h"
#include "data.h"
/* last modified 01/17/09 */
/*
 *******************************************************************************
 *                                                                             *
 *   TimeAdjust() is called to adjust timing variables after each program move *
 *   is made.  It simply increments the number of moves made, decrements the   *
 *   amount of time used, and then makes any necessary adjustments based on    *
 *   the time controls.                                                        *
 *                                                                             *
 *******************************************************************************
 */
void TimeAdjust(int time_used, int side) {
/*
 ************************************************************
 *                                                          *
 *   Decrement the number of moves remaining to the next    *
 *   time control.  Then subtract the time the program took *
 *   to choose its move from the time remaining.            *
 *                                                          *
 ************************************************************
 */
  tc_moves_remaining[side]--;
  tc_time_remaining[side] -=
      (tc_time_remaining[side] >
      time_used) ? time_used : tc_time_remaining[side];
  if (!tc_moves_remaining[side]) {
    if (tc_sudden_death == 2)
      tc_sudden_death = 1;
    tc_moves_remaining[side] += tc_secondary_moves;
    tc_time_remaining[side] += tc_secondary_time;
    Print(4095, "time control reached (%s)\n", (side) ? "white" : "black");
  }
  if (tc_increment)
    tc_time_remaining[side] += tc_increment;
}

/* last modified 06/09/13 */
/*
 *******************************************************************************
 *                                                                             *
 *   TimeCheck() is used to determine when the search should stop.  It uses    *
 *   several conditions to make this determination:  (1) The search time has   *
 *   exceeded the time per move limit;  (2) The value at the root of the tree  *
 *   has not dropped too low.  (3) If the root move was flagged as "easy" and  *
 *   no move has replaced it as best, the search can actually be stopped early *
 *   to save some time on the clock.  If (2) is false, then the time is        *
 *   extended up to 6x in an effort to avoid playing a poor move.              *
 *                                                                             *
 *   We use one additional trick here to avoid stopping the search just before *
 *   we change to a better move.  Once we reach the time limit, we set do not  *
 *   immediately stop the search, rather, we let it continue until all root    *
 *   moves that are "in progress" complete.  This is important, particularly   *
 *   on a parallel search that splits at the root like Crafty does.  A new     *
 *   best move will take quite a bit of time to search, and with just one CPU  *
 *   working on it, we might not have time to complete it before the search    *
 *   time limit is reached.  Once we reach this point, we set a flag that      *
 *   says "do not start searching a new root move, but do not abort any active *
 *   search.  As processors finish their root moves, they will begin to help   *
 *   those with incomplete root move searches to make sure that none of them   *
 *   will become a new best move.  This way, once we start searching any root  *
 *   move, we will not give up on it until the search has completed and the    *
 *   move was proved worse than the best root move so far, or until we         *
 *   discover that this root move is actually better.                          *
 *                                                                             *
 *   This is implemented by having Search() call TimeCheck() passing it a      *
 *   value of zero (0) for the parameter abort.  TimeCheck() will only end the *
 *   search if we have exceeded the max time limit or we have not gotten a     *
 *   best score at the root on the current iteration and we have reached the   *
 *   normal time limit.  Otherwise TimeCheck() will return the "abort" value   *
 *   which is always zero when called from Search().  We also call TimeCheck() *
 *   from NextRootMove() and it passes TimeCheck() a value of 1 for the abort  *
 *   flag.  Once anyone posts the "abort" flag, NextRootMove() will return a   *
 *   indication saying "no more root moves to search", which will eventually   *
 *   end the search when all current root moves are completed and              *
 *   NextRootMove() is called to obtain another root move to search.           *
 *                                                                             *
 *   The global variable "abort_after_ply1" is initially set to zero, and so   *
 *   long as it is zero, NextRootMove() will continue to return moves to       *
 *   search until the iteration ends.  Whenever the time_abort flag becomes    *
 *   non-zero, NextRootMove() refuses to search any new moves, but the current *
 *   searches are allowed to continue until they all complete or the hard time *
 *   limit (absolute_time_limit) is reached where the search is terminated     *
 *   immediately to avoid overstepping the time control limits.                *
 *                                                                             *
 *   The "difficulty" value is used to implement the concept of an "easy move" *
 *   or a "hard move".  With an easy move, we want to spend less time since    *
 *   the easy move is obvious.  The opposite idea is a hard move, where we     *
 *   actually want to spend more time to be sure we don't make a mistake by`   *
 *   moving too quickly.                                                       *
 *                                                                             *
 *   The basic methodology revolves around how many times we change our mind   *
 *   on the best move at the root of the tree.                                 *
 *                                                                             *
 *   The "difficulty" variable is initially set to 100, which represents a     *
 *   percentage of the actual target time we should spend on this move.  If    *
 *   we end an iteration without having changed our mind at all, difficulty    *
 *   is reduced by multiplying by .9, with a lower bound of 60%.               *
 *                                                                             *
 *   If we change our mind during an iteration, there are two cases.  (1) If   *
 *   difficulty is < 100%, we set it back to 100% +20% for each time we        *
 *   changed the best move.  (2) if difficulty is already at 100% or higher,   *
 *   we multiply difficulty by .80, then add 20% for each root move change.    *
 *   For example, suppose we are at difficulty=60%, and we suddenly change our *
 *   mind twice this iteration (3 different best moves).                       *
 *                                                                             *
 *   difficulty = 100 + 3*20 = 160% of the actual target time will be used.    *
 *                                                                             *
 *   Suppose we change back and forth between two best moves multiple times,   *
 *   with difficulty currently at 100%.  The first time:                       *
 *                                                                             *
 *   difficulty = .80 * 100 + 2*20 = 120%                                      *
 *                                                                             *
 *   The next iteration:                                                       *
 *                                                                             *
 *   difficulty = .80 * 120 + 2 * 20 = 96% _ 40% = 136%                        *
 *                                                                             *
 *   The next iteration:                                                       *
 *                                                                             *
 *   difficulty = .80 * 136% + 40% = 149%                                      *
 *                                                                             *
 *   If we stop changing our mind, then difficulty starts on a downward trend. *
 *   The basic idea is that if we are locked in on a move, we can make it a    *
 *   bit quicker, but if we are changing back and forth, we are going to spend *
 *   more time to try to choose the best move.                                 *
 *                                                                             *
 *   Notice that this doesn't alter the usual "use way more time if we fail    *
 *   low at the root (score drop).  That works just as it always has.          *
 *                                                                             *
 *******************************************************************************
 */
int TimeCheck(TREE * RESTRICT tree, int abort) {
  int time_used;
  int i, ndone;

/*
 ************************************************************
 *                                                          *
 *   Check to see if we need to "burp" the time to let the  *
 *   operator know the search is progressing and how much   *
 *   time has been used so far.                             *
 *                                                          *
 ************************************************************
 */
  time_used = (ReadClock() - start_time);
  if (tree->nodes_searched > noise_level && display_options & 32 &&
      time_used > burp) {
    Lock(lock_io);
    if (pondering)
      printf("         %2i   %s%7s?  ", iteration_depth,
          Display2Times(time_used), tree->remaining_moves_text);
    else
      printf("         %2i   %s%7s*  ", iteration_depth,
          Display2Times(time_used), tree->remaining_moves_text);
    if (display_options & 32 && display_options & 64)
      printf("%d. ", move_number);
    if ((display_options & 32) && (display_options & 64) && Flip(root_wtm))
      printf("... ");
    printf("%s(%snps)             \r", tree->root_move_text,
        DisplayKM(nodes_per_second));
    burp = (time_used / 1500) * 1500 + 1500;
    fflush(stdout);
    Unlock(lock_io);
  }
/*
 ************************************************************
 *                                                          *
 *   First, check to see if there is only one root move.    *
 *   If so, and we are not pondering, searching a book move *
 *   or annotating a game, we can return and make this move *
 *   instantly.  We do need to finish iteration 1 so that   *
 *   we actually back up a move to play.                    *
 *                                                          *
 ************************************************************
 */
  if (n_root_moves == 1 && !booking && !annotate_mode && !pondering &&
      iteration_depth > 1)
    return (1);
  if (iteration_depth <= 2)
    return (0);
/*
 ************************************************************
 *                                                          *
 *   If we are pondering or in analyze mode, we do not      *
 *   terminate on time since there is no time limit placed  *
 *   on these searches.  If we have reached the absolute    *
 *   time limit, we stop the search instantly.              *
 *                                                          *
 *   If we are under the time limit already set, we do not  *
 *   terminate the search.  If the operator set a specific  *
 *   search time limit, we stop when we hit that regardless *
 *   of the score.                                          *
 *                                                          *
 *   The only other case is an "easy move" which is a move  *
 *   that looks significantly better than the rest of the   *
 *   root moves when they are initially ordered, and this   *
 *   move does not fail low during subsequent searches.     *
 *                                                          *
 ************************************************************
 */
  if (pondering || analyze_mode)
    return (0);
  if (!search_time_limit) {
    if (time_used < (difficulty * time_limit) / 100)
      return (0);
  } else {
    if (time_used < time_limit)
      return (0);
    else
      return (1);
  }
  if (time_used > absolute_time_limit)
    return (1);
/*
 ************************************************************
 *                                                          *
 *   Now, check to see if we are searching the first move   *
 *   at this depth.  If so, and we run out of time, we can  *
 *   abort the search rather than waiting to complete this  *
 *   ply=1 move to see if it's better.                      *
 *                                                          *
 ************************************************************
 */
  ndone = 0;
  for (i = 0; i < n_root_moves; i++)
    if (root_moves[i].status & 16)
      ndone++;
  if (ndone == 1)
    abort = 1;
/*
 ************************************************************
 *                                                          *
 *   Ok.  We have used "time_limit" at this point, and we   *
 *   have not gone over "absolute_time_limit".  Now the     *
 *   question is, can we stop the search?                   *
 *                                                          *
 *   If we have a score at the root of the tree and if the  *
 *   evaluation is not worse than the last evaluation       *
 *   (from the previous iteration...) then we safely stop   *
 *   the search.                                            *
 *                                                          *
 ************************************************************
 */
  if (root_value == root_alpha && !(root_moves[0].status & 1) && ndone == 1)
    return (1);
  if ((root_value >= last_root_value && !(root_moves[0].status & 1)))
    return (abort);
/*
 ************************************************************
 *                                                          *
 *   We are in trouble at the root.  We have now used the   *
 *   allocated time limit, yet the score for the current    *
 *   iteration is still worse than the score for the        *
 *   previous iteration.  We will continue to search until  *
 *   we use 6x the normal target time in an effort to avoid *
 *   playing a move that might end up losing the game.      *
 *                                                          *
 *   One note for clarification.  After the first root move *
 *   has been searched, the rest "fly by" thanks to the     *
 *   reductions and forward-pruning stuff.  It is not very  *
 *   likely that we are going to go 6x before every root    *
 *   move has been searched, unless one of them actually    *
 *   has the potential to become a new best move.  Most of  *
 *   the time, we are going to end the current iteration    *
 *   quickly and we won't start another since we are past   *
 *   the time limit.  It sounds risky to extend 6x for just *
 *   a 0.01 score drop, but using that much time is not so  *
 *   common as to cause an unnecessary loss of time.  We do *
 *   prefer to spend some additional time to stop any score *
 *   drop, if we can, as any drop is a bad thing overall.   *
 *                                                          *
 ************************************************************
 */
  if (time_used > time_limit * 6 ||
      time_used + 300 > tc_time_remaining[root_wtm])
    return (1);
  return (0);
}

/* last modified 05/05/13 */
/*
 *******************************************************************************
 *                                                                             *
 *   TimeSet() is called to set the two variables "time_limit" and             *
 *   "absolute_time_limit" which controls the amount of time taken by the      *
 *   iterated search.  It simply takes the timing controls as set by the user  *
 *   and uses these values to calculate how much time should be spent on the   *
 *   next search.                                                              *
 *                                                                             *
 *******************************************************************************
 */
void TimeSet(TREE * RESTRICT tree, int search_type) {
  int mult = 0, extra = 0;
  int surplus, average;
  int simple_average;

  surplus = 0;
  average = 0;
/*
 ************************************************************
 *                                                          *
 *   Check to see if we are in a sudden-death type of time  *
 *   control.  If so, we have a fixed amount of time        *
 *   remaining.  Set the search time accordingly and exit.  *
 *                                                          *
 *   If we have less than 5 seconds on the clock prior to   *
 *   the increment, then limit our search to the increment. *
 *                                                          *
 *   If we have less than 2.5 seconds on the clock prior to *
 *   the increment, then limit our search to half the       *
 *   increment in an attempt to add some time to our buffer.*
 *                                                          *
 *   Set our MAX search time to half the remaining time.    *
 *                                                          *
 *   If our search time will drop the clock below 1 second, *
 *   then limit our MAX search time to the normal search    *
 *   time.  This is done to stop any extensions from        *
 *   dropping us too low.                                   *
 *                                                          *
 ************************************************************
 */
  if (tc_sudden_death == 1) {
    if (tc_increment) {
      time_limit =
          (tc_time_remaining[root_wtm] -
          tc_operator_time * tc_moves_remaining[root_wtm]) /
          (ponder ? 20 : 26) + tc_increment;
      if (tc_time_remaining[root_wtm] < 500 + tc_increment) {
        time_limit = tc_increment;
        if (tc_time_remaining[root_wtm] < 250 + tc_increment)
          time_limit /= 2;
      }
      absolute_time_limit = tc_time_remaining[root_wtm] / 2 + tc_increment;
      if (absolute_time_limit < time_limit ||
          tc_time_remaining[root_wtm] - time_limit < 100)
        absolute_time_limit = time_limit;
      if (tc_time_remaining[root_wtm] - time_limit < 50) {
        time_limit = tc_time_remaining[root_wtm] - 50;
        if (time_limit < 5)
          time_limit = 5;
      }
      if (tc_time_remaining[root_wtm] - absolute_time_limit < 25) {
        absolute_time_limit = tc_time_remaining[root_wtm] - 25;
        if (absolute_time_limit < 5)
          absolute_time_limit = 5;
      }

    } else {
      time_limit = tc_time_remaining[root_wtm] / (ponder ? 20 : 26);
      absolute_time_limit =
          Min(time_limit * 5, tc_time_remaining[root_wtm] / 2);
    }
  }
/*
 ************************************************************
 *                                                          *
 *   We are not in a sudden_death situation.  We now have   *
 *   two choices:  If the program has saved enough time to  *
 *   meet the surplus requirement, then we simply divide    *
 *   the time left evenly among the moves left.  If we      *
 *   haven't yet saved up a cushion so that "fail-lows"     *
 *   have extra time to find a solution, we simply take the *
 *   number of moves divided into the total time less the   *
 *   necessary operator time as the target.                 *
 *                                                          *
 ************************************************************
 */
  else {
    if (move_number <= tc_moves)
      simple_average =
          (tc_time -
          (tc_operator_time * tc_moves_remaining[root_wtm])) / tc_moves;
    else
      simple_average =
          (tc_secondary_time -
          (tc_operator_time * tc_moves_remaining[root_wtm])) /
          tc_secondary_moves;
    surplus =
        Max(tc_time_remaining[root_wtm] -
        (tc_operator_time * tc_moves_remaining[root_wtm]) -
        simple_average * tc_moves_remaining[root_wtm], 0);
    average =
        (tc_time_remaining[root_wtm] -
        (tc_operator_time * tc_moves_remaining[root_wtm]) +
        tc_moves_remaining[root_wtm] * tc_increment)
        / tc_moves_remaining[root_wtm];
    if (surplus < tc_safety_margin)
      time_limit = (average < simple_average) ? average : simple_average;
    else
      time_limit =
          (average < 2.0 * simple_average) ? average : 2.0 * simple_average;
  }
  if (surplus < 0)
    surplus = 0;
  if (tc_increment > 200 && moves_out_of_book < 2)
    time_limit *= 1.2;
  if (time_limit <= 0)
    time_limit = 5;
  absolute_time_limit =
      time_limit + surplus / 2 + ((tc_time_remaining[root_wtm] -
          tc_operator_time * tc_moves_remaining[root_wtm]) / 4);
  if (absolute_time_limit > 6 * time_limit)
    absolute_time_limit = 6 * time_limit;
  if (absolute_time_limit > tc_time_remaining[root_wtm] / 2)
    absolute_time_limit = tc_time_remaining[root_wtm] / 2;
/*
 ************************************************************
 *                                                          *
 *  The "usage" option can be used to force the time limit  *
 *  higher or lower than normal.  The new "timebook"        *
 *  command can also modify the target time making the      *
 *  program use more time early in the game as it exits the *
 *  book, knowing it will save time later on by ponder hits *
 *  and instant moves.                                      *
 *                                                          *
 ************************************************************
 */
  if (usage_level)
    time_limit *= 1.0 + usage_level / 100.0;
  if (first_nonbook_factor && moves_out_of_book < first_nonbook_span) {
    mult =
        (first_nonbook_span - moves_out_of_book + 1) * first_nonbook_factor;
    extra = time_limit * mult / first_nonbook_span / 100;
    time_limit += extra;
  }
/*
 ************************************************************
 *                                                          *
 *   If the operator has set an absolute search time limit  *
 *   already, then we simply copy this value and return.    *
 *                                                          *
 ************************************************************
 */
  if (search_time_limit) {
    time_limit = search_time_limit;
    absolute_time_limit = time_limit;
  }
  if (search_type == puzzle || booking) {
    time_limit /= 10;
    absolute_time_limit = time_limit * 3;
  }
  if (!tc_sudden_death && !search_time_limit &&
      time_limit > 3 * tc_time / tc_moves)
    time_limit = 3 * tc_time / tc_moves;
  time_limit = Min(time_limit, absolute_time_limit);
  if (search_type != puzzle) {
    if (!tc_sudden_death)
      Print(128, "        time surplus %s  ", DisplayTime(surplus));
    else
      Print(128, "         ");
    Print(128, "time limit %s", DisplayTimeKibitz(time_limit));
    Print(128, " (+%s)", DisplayTimeKibitz(extra));
    Print(128, " (%s)", DisplayTimeKibitz(absolute_time_limit));
    if (fabs(usage_level) > 0.0001) {
      Print(128, "/");
      Print(128, "(%d)", usage_level);
    }
    Print(128, "\n");
  }
  if (time_limit <= 1) {
    time_limit = 1;
    usage_level = 0;
  }
}
