#include <stdio.h>
#include <stdlib.h>
#include "chess.h"
#include "data.h"

/* last modified 09/27/96 */
/*
********************************************************************************
*                                                                              *
*   TimeAdjust() is called to adjust timing variables after each program move  *
*   is made.  it simply increments the number of moves made, decrements the    *
*   amount of time used, and then makes any necessary adjustments based on the *
*   time controls.                                                             *
*   next search.                                                               *
*                                                                              *
********************************************************************************
*/
void TimeAdjust(int time_used, PLAYER player) {
/*
 ----------------------------------------------------------
|                                                          |
|   decrement the number of moves remaining to the next    |
|   time control.  then subtract the time the program took |
|   to choose its move from the time remaining.            |
|                                                          |
 ----------------------------------------------------------
*/
  if (player == crafty) {
    tc_moves_remaining--;
    tc_time_remaining-=(tc_time_remaining > time_used) ? 
                       time_used : tc_time_remaining;
    if (!tc_moves_remaining) {
      if (tc_sudden_death == 2) tc_sudden_death=1;
      tc_moves_remaining+=tc_secondary_moves;
      tc_time_remaining+=tc_secondary_time;
      tc_time_remaining_opponent+=tc_secondary_time;
      Print(4095,"time control reached\n");
    }
    if (tc_increment) tc_time_remaining+=tc_increment;
  }
  else {
    tc_time_remaining_opponent-=(tc_time_remaining_opponent > time_used) ? 
                                time_used : tc_time_remaining_opponent;
    if (tc_increment) tc_time_remaining_opponent+=tc_increment;
  }
}

/* last modified 01/26/00 */
/*
********************************************************************************
*                                                                              *
*   TimeCheck() is used to determine when the search should stop.  it uses     *
*   several conditions to make this determination:  (1) the search time has    *
*   exceeded the time per move limit;  (2) the value at the root of the tree   *
*   has not dropped to low.  (3) if the root move was flagged as "easy" and    *
*   no move has replaced it as best, the search can actually be stopped early  *
*   to save some time on the clock.  if (2) is true, then the time is extended *
*   based on how far the root value has dropped in an effort to avoid whatever *
*   is being lost.                                                             *
*                                                                              *
********************************************************************************
*/
int TimeCheck(int abort) {
  int time_used;
  int value, last_value;
  int i, ndone;
  TREE * const tree=local[0];
/*
 ----------------------------------------------------------
|                                                          |
|   first, check to see if we are searching the first move |
|   at this depth.  if so, and we run out of time, we can  |
|   abort the search rather than waiting to complete this  |
|   ply=1 move to see if it's better.                      |
|                                                          |
 ----------------------------------------------------------
*/
  if (search_nodes && tree->nodes_searched > search_nodes) return(1);
  if (n_root_moves==1 && !booking && !annotate_mode &&
      !pondering && iteration_depth>4) return(1);
  ndone=0;
  for (i=0;i<n_root_moves;i++)
    if (root_moves[i].status&128) ndone++;
  if (ndone == 1) abort=1;
  if (iteration_depth <= 2) return(0);
/*
 ----------------------------------------------------------
|                                                          |
|   now, check to see if we need to "burp" the time to     |
|   let the operator know the search is progressing and    |
|   how much time has been used so far.                    |
|                                                          |
 ----------------------------------------------------------
*/
  time_used=(ReadClock(time_type)-start_time);
  if (tree->nodes_searched>noise_level &&
      display_options&32 && time_used>burp) {
    Lock(lock_io);
#if defined(MACOS)
    if (pondering)
      printf("               %2i   %s%7s?  ",iteration_depth,
             DisplayTime(time_used,tree->remaining_moves_text));
    else
      printf("               %2i   %s%7s*  ",iteration_depth,
             DisplayTime(time_used,tree->remaining_moves_text));
    printf("%d. ",move_number);
    printf("%s      \n",tree->root_move_text);
#else
    if (pondering)
      printf("               %2i   %s%7s?  ",iteration_depth,
             DisplayTime(time_used),tree->remaining_moves_text);
    else
      printf("               %2i   %s%7s*  ",iteration_depth,
             DisplayTime(time_used),tree->remaining_moves_text);
    if (display_options&32 && display_options&64)
    printf("%d. ",move_number);
    if ((display_options&32) && (display_options&64) && !root_wtm)
      printf("... ");
    printf("%s      \r",tree->root_move_text);
#endif
    burp=(time_used/1500)*1500+1500;
    fflush(stdout);
    UnLock(lock_io);
  }
  if (pondering || analyze_mode) return(0);
  if (time_used > absolute_time_limit) return(1);
  if (easy_move && !search_time_limit) {
    if (time_limit>100 && time_used<time_limit/3) return (0);
  }
  else if (time_used < time_limit) return(0);
  if (search_time_limit) return(1);
/*
 ----------------------------------------------------------
|                                                          |
|   ok.  we have used "time_limit" at this point.  now the |
|   question is, can we stop the search?                   |
|                                                          |
|   first, make sure that we have actually found a score   |
|   at the root of the tree.  if not, we can safely stop   |
|   searching without wasting any more time.               |
|                                                          |
 ----------------------------------------------------------
*/
  if (root_value==root_alpha && !(root_moves[0].status&1) &&
      ndone==1) return(1);
/*
 ----------------------------------------------------------
|                                                          |
|   we have a score at the root of the tree, if the        |
|   evaluation is not significantly worse than the last    |
|   evaluation (from the previous iteration...) then we    |
|   safely stop the search.  note also that if the current |
|   evaluation is quite a bit worse, but we are still way  |
|   ahead, we can still avoid using extra time.            |
|                                                          |
 ----------------------------------------------------------
*/
  value=root_value;
  last_value=last_root_value;
  if ((value>=last_value-24 && !(root_moves[0].status&1)) ||
      (value>350 && value >= last_value-50)) {
    if (time_used > time_limit*2) return(1);
    else return(abort);
  }
/*
 ----------------------------------------------------------
|                                                          |
|   we are in trouble at the root.  depending on how much  |
|   the score has dropped, increase the search time limit  |
|   to try and correct the problem.  for a positional drop |
|   we can double the search time (this is for a serious   |
|   drop, of course).                                      |
|                                                          |
 ----------------------------------------------------------
*/
  if (time_used < time_limit*2.5 && time_used+500<tc_time_remaining) return(0);
  if ((value >= last_value-67 && !(root_moves[0].status&1)) ||
      value>550) return(abort);
/*
 ----------------------------------------------------------
|                                                          |
|   we are in really serious trouble at the root, losing   |
|   material.  increase the time limit to 6X the original  |
|   target, as losing material is tantamount to losing the |
|   game anyway.                                           |
|                                                          |
 ----------------------------------------------------------
*/
  if (time_used < time_limit*6 && time_used+500<tc_time_remaining) return(0);
  return(1);
}

/* last modified 01/04/98 */
/*
********************************************************************************
*                                                                              *
*   TimeSet() is called to set the two variables "time_limit" and              *
*   "absolute_time_limit" which controls the amount of time taken by the       *
*   iterated search.  it simply takes the timing controls as set by the user   *
*   and uses these values to calculate how much time should be spent on the    *
*   next search.                                                               *
*                                                                              *
********************************************************************************
*/
void TimeSet(int search_type) {
  static const float behind[6] = {32.0, 16.0, 8.0, 4.0, 2.0, 1.5};
  static const int reduce[6] = {96, 48, 24, 12, 6, 3};
  int i;
  int surplus, average;
  int simple_average;

  surplus=0;
  average=0;
/*
 ----------------------------------------------------------
|                                                          |
|   check to see if we are in a sudden-death type of time  |
|   control.  if so, we have a fixed amount of time        |
|   remaining.  set the search time accordingly and exit.  |
|                                                          |
 ----------------------------------------------------------
*/
  if (tc_sudden_death == 1) {
    if (tc_increment) {
      time_limit=(tc_time_remaining-tc_operator_time*tc_moves_remaining)/35+
                 tc_increment;
      if (tc_time_remaining < 3000) time_limit=tc_increment;
      if (tc_time_remaining < 1500) time_limit=tc_increment/2;
      absolute_time_limit=tc_time_remaining/2;
    }
    else {
      time_limit=tc_time_remaining/40;
      absolute_time_limit=Min(time_limit*6,tc_time_remaining/2);
    }
  }
/*
 ----------------------------------------------------------
|                                                          |
|   we are not in a sudden_death situation.  we now have   |
|   two choices:  if the program has saved enough time to  |
|   meet the surplus requirement, then we simply divide    |
|   the time left evenly among the moves left.  if we      |
|   haven't yet saved up a cushion so that "fail-lows"     |
|   have extra time to find a solution, we simply take the |
|   number of moves divided into the total time less the   |
|   necessary operator time as the target.                 |
|                                                          |
 ----------------------------------------------------------
*/
  else {
    if (move_number <= tc_moves)
      simple_average=(tc_time-(tc_operator_time*tc_moves_remaining))/tc_moves;
    else
      simple_average=(tc_secondary_time-(tc_operator_time*tc_moves_remaining))/
                     tc_secondary_moves;
    surplus=Max(tc_time_remaining-(tc_operator_time*tc_moves_remaining)-
                 simple_average*tc_moves_remaining,0);
    average=(tc_time_remaining-(tc_operator_time*tc_moves_remaining)+
            tc_moves_remaining*tc_increment)
            /tc_moves_remaining;
    if (surplus < tc_safety_margin) 
      time_limit = (average<simple_average) ? average : simple_average;
    else
      time_limit=(average<2.0*simple_average) ? average : 2.0*simple_average;
  }
  if (surplus < 0) surplus=0;
  if (tc_increment>200 && moves_out_of_book<2) time_limit*=1.2;
  if (time_limit <= 0) time_limit=5;
  absolute_time_limit=time_limit+surplus/2+((tc_time_remaining-
                      tc_operator_time*tc_moves_remaining)/4);
  if (absolute_time_limit > 6*time_limit) absolute_time_limit=6*time_limit;
  if (absolute_time_limit > tc_time_remaining/2)
    absolute_time_limit=tc_time_remaining/2;
/*
   new option 'usage' increases and decrease the time factors)
*/
  if (usage_level)  time_limit*=1.0+usage_level/100.0;
/*
 ----------------------------------------------------------
|                                                          |
|  this code is used to handle the case where someone is   |
|  trying to "blitz" crafty by reaching a position where   |
|  things are locked up, and then just shuffling pieces    |
|  back and forth.  when crafty reaches the point where it |
|  has less than 3/4 of the time the opponent has, it      |
|  starts decreasing the target time.  at 1/2, it          |
|  decreases it further.                                   |
|                                                          |
 ----------------------------------------------------------
*/
  if (mode!=tournament_mode && !computer_opponent) {
    for (i=0;i<6;i++) {
      if ((float)tc_time_remaining*behind[i] <
          (float)tc_time_remaining_opponent) {
        time_limit=time_limit/reduce[i];
        Print(128,"crafty is behind %4.1f on time, reducing by 1/%d.\n",
               behind[i],reduce[i]);
        break;
      }
    }
    if (tc_increment==0 && tc_time_remaining_opponent>tc_time_remaining) {
      if (tc_time_remaining < 3000) time_limit/=2;
      if (tc_time_remaining < 2000) time_limit/=2;
      if (tc_time_remaining < 1000) time_limit=1;
    }
  }
/*
 ----------------------------------------------------------
|                                                          |
|   if the operator has set an absolute search time limit  |
|   already, then we simply copy this value and return.    |
|                                                          |
 ----------------------------------------------------------
*/
  if (search_time_limit) {
    time_limit=search_time_limit;
    absolute_time_limit=time_limit;
  }
  if (search_type==puzzle || booking) {
    time_limit/=10;
    absolute_time_limit=time_limit*3;
  }

  if (!tc_sudden_death && !search_time_limit && time_limit>3*tc_time/tc_moves)
    time_limit=3*tc_time/tc_moves;
  if (search_type != puzzle) {
    if (!tc_sudden_death)
      Print(128,"              time surplus %s  ",DisplayTime(surplus));
    else
      Print(128,"              ");
    Print(128,"time limit %s", DisplayTimeWhisper(time_limit));
    Print(128," (%s)", DisplayTimeWhisper(absolute_time_limit));
    if (usage_level != 0.0) {
      Print(128,"/");
      Print(128,"(%d)",usage_level);
    }
    if (easy_move) Print(128," [easy move]");
    Print(128,"\n");
  }
  if (time_limit <= 1) {
    time_limit= 1;
    usage_level=0;
  }
}
