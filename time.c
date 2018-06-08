#include <stdio.h>
#include <stdlib.h>
#include "types.h"
#include "function.h"
#include "data.h"
/*
********************************************************************************
*                                                                              *
*   Time_Adjust() is called to adjust timing variables after each program move *
*   is made.  it simply increments the number of moves made, decrements the    *
*   amount of time used, and then makes any necessary adjustments based on the *
*   time controls.                                                             *
*   next search.                                                               *
*                                                                              *
********************************************************************************
*/
void Time_Adjust(int time_used)
{
/*
 ----------------------------------------------------------
|                                                          |
|   decrement the number of moves remaining to the next    |
|   time control.  then subtract the time the program took |
|   to choose its move from the time remaining.            |
|                                                          |
 ----------------------------------------------------------
*/
  tc_moves_remaining--;
  tc_time_remaining-=time_used/10;
  if (!tc_moves_remaining) {
    if (tc_sudden_death == 2) tc_sudden_death=1;
    tc_moves+=tc_secondary_moves;
    tc_time+=tc_secondary_time;
    tc_moves_remaining+=tc_secondary_moves;
    tc_time_remaining+=tc_secondary_time;
    Print(0,"time control reached\n");
  }
  if (tc_increment) {
    tc_time_remaining+=tc_increment;
  }
}

/*
********************************************************************************
*                                                                              *
*   Time_Check() is used to determine when the search should stop.  it uses    *
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
int Time_Check()
{
  int time_used;
  int value, last_value, searched;
  int *i;

/*
 ----------------------------------------------------------
|                                                          |
|   first, check to see if we need to "burp" the time to   |
|   let the operator know the search is progressing and    |
|   how much time has been used so far.                    |
|                                                          |
 ----------------------------------------------------------
*/
  time_used=(Get_Time(time_type)-start_time);
  if ((nodes_searched > noise_level) && (verbosity_level >= 9) &&
      !((time_used/10)%burp)) {
    printf("               %2i   %s\r",iteration_depth,
           Display_Time(time_used));
    fflush(stdout);
  }
  if (search_depth || pondering || analyze_mode) return(0);
  if (easy_move && !search_time_limit) {
    if (time_used < time_limit/3) return (0);
  }
  else {
    if (time_used < time_limit) return(0);
  }
  if (search_time_limit) return(1);
  if (time_used > absolute_time_limit) return(1);
/*
 ----------------------------------------------------------
|                                                          |
|   ok.  we have used "time_limit" at this point.  now the |
|   question is, can we stop the search?  if the current   |
|   evaluation is not significantly worse than the last    |
|   evaluation (from the previous iteration...) then we    |
|   safely stop the search.                                |
|                                                          |
 ----------------------------------------------------------
*/
  value=root_value;
  last_value=last_search_value;
  if ((value >= last_value-250) && !failed_low) return(1);
/*
 ----------------------------------------------------------
|                                                          |
|   first, make sure that we have actually found a score   |
|   at the root of the tree.  if not, we can safely stop   |
|   searching without wasting any more time.               |
|                                                          |
 ----------------------------------------------------------
*/
  if ((root_value == root_alpha) && !failed_low) {
    searched=0;
    for (i=first[1];i<last[1];i++)
      if (searched_this_root_move[i-first[1]]) searched++;
    if (searched == 1) return(1);
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
  if (time_used < time_limit*2) return(0);
  if ((value >= last_value-600) && !failed_low) return(1);
/*
 ----------------------------------------------------------
|                                                          |
|   we are in really serious trouble at the root, losing   |
|   material.  increase the time limit to 5X the original  |
|   target, as losing material is tantamount to losing the |
|   game anyway.                                           |
|                                                          |
 ----------------------------------------------------------
*/
  if (time_used < time_limit*5) return(0);
  return(1);
}

/*
********************************************************************************
*                                                                              *
*   Time_Set() is called to set the two variables "time_limit" and             *
*   "absolute_time_limit" which controls the amount of time taken by the       *
*   iterated search.  it simply takes the timing controls as set by the user   *
*   and uses these values to calculate how much time should be spent on the    *
*   next search.                                                               *
*                                                                              *
********************************************************************************
*/
void Time_Set()
{
  int surplus, average;
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
      time_limit=(tc_time_remaining/25+tc_increment-tc_operator_time)*10;
      if (tc_time_remaining < 60) time_limit=tc_increment*10;
      if (tc_time_remaining < 10) time_limit=tc_increment*5;
      absolute_time_limit=(tc_time_remaining/2)*10;
    }
    else {
      time_limit=tc_time_remaining*10/25;
      absolute_time_limit=time_limit*5;
    }
    surplus=0;
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
    surplus=tc_time_remaining-tc_simple_average_time*tc_moves_remaining;
    average=(tc_time_remaining+tc_moves_remaining*(tc_increment-
                                                   tc_operator_time))/
             tc_moves_remaining;
    if (surplus < tc_safety_margin) {
      time_limit = ((average < tc_simple_average_time) ? 
          average : tc_simple_average_time)*10;
    }
    else {
      time_limit=((average < 1.5*tc_simple_average_time) ?
         average : 1.5*tc_simple_average_time)*10;
    }
    absolute_time_limit=time_limit+(surplus+tc_time_remaining/4)*10;
    if (absolute_time_limit > 5*time_limit) absolute_time_limit=5*time_limit;
  }
  if (time_limit <= 0) time_limit=1;
  if (puzzling) {
    time_limit/=10;
    absolute_time_limit=time_limit*3;
  }
/*
 ----------------------------------------------------------
|                                                          |
|   if just out of book, increase the search time for a    |
|   couple of moves to avoid trouble.                      |
|                                                          |
 ----------------------------------------------------------
*/
  if ((last_move_in_book+1) > move_number) time_limit*=1.5;
/*
 ----------------------------------------------------------
|                                                          |
|   if the operator has set an absolute search time limit  |
|   already, then we simply copy this value and return.    |
|                                                          |
 ----------------------------------------------------------
*/
  if (search_time_limit) {
    time_limit=search_time_limit*10;
    absolute_time_limit=time_limit;
  }

  if (!puzzling) {
    if (tc_sudden_death != 1)
      Print(1,"              time surplus %s  ",Display_Time(10*surplus));
    else
      Print(1,"              ");
    Print(1,"time limit %s ", Display_Time(time_limit));
    if (easy_move)
      Print(1," [easy move]\n");
    else
      Print(1,"\n");
  }
/*
 ----------------------------------------------------------
|                                                          |
|   kludge-time.  if the target is less than a second, we  |
|   have to use cpu time.  unix keeps elapsed time only to |
|   the nearest second, which is not accurate enough here. |
|   also, if we are using elapsed time, we need to round   |
|   the target "down" to the nearest second.               |
|                                                          |
 ----------------------------------------------------------
*/
  if ((!puzzling) && (time_limit < 10) && (time_type == elapsed)) {
    Print(0,"switching to cpu time\n");
    time_type=cpu;
  }
  if (time_type == elapsed) time_limit=(time_limit/10)*10;
}
