#include <stdio.h>
#include <stdlib.h>
#include "types.h"
#include "function.h"
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
void TimeAdjust(int time_used, PLAYER player)
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
  
  if (player == crafty) {
    tc_moves_remaining--;
    tc_time_remaining-=(tc_time_remaining > time_used) ? 
                       time_used : tc_time_remaining;
    if (!tc_moves_remaining) {
      if (tc_sudden_death == 2) tc_sudden_death=1;
      tc_moves+=tc_secondary_moves;
      tc_time+=tc_secondary_time;
      tc_moves_remaining+=tc_secondary_moves;
      tc_time_remaining+=tc_secondary_time;
      tc_time_remaining_opponent+=tc_secondary_time;
      Print(0,"time control reached\n");
    }
    if (tc_increment) tc_time_remaining+=tc_increment;
  }
  else {
    tc_time_remaining_opponent-=(tc_time_remaining_opponent > time_used) ? 
                                time_used : tc_time_remaining_opponent;
    if (tc_increment) tc_time_remaining_opponent+=tc_increment;
  }
}

/* last modified 09/02/96 */
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
int TimeCheck(int abort)
{
  int time_used;
  int value, last_value, searched;
  int *i, ndone;
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
  ndone=0;
  for (i=last[0];i<last[1];i++)
    if (searched_this_root_move[i-last[0]]) ndone++;
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
  time_used=(GetTime(time_type)-start_time);
  if ((nodes_searched+q_nodes_searched) > noise_level &&
      verbosity_level >= 9 && time_used>burp) {
    printf("               %2i   %s\r",iteration_depth,DisplayTime(time_used));
    burp=(time_used/1500)*1500+1500;
    fflush(stdout);
  }
  if (search_depth || pondering || analyze_mode) return(0);
  if (easy_move && !search_time_limit) {
    if ((time_limit > 100) && (time_used < time_limit/3)) return (0);
  }
  else if (time_used < time_limit) return(0);
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
  if (((value >= last_value-333) && !search_failed_low ) || ((value>3500 && (value >= last_value-667)))) {/*if we're up by piece we need more than 667 point fal to increase time*/
    if (time_used > time_limit*2) return(1);
    else return(abort);
  }
/*
 ----------------------------------------------------------
|                                                          |
|   first, make sure that we have actually found a score   |
|   at the root of the tree.  if not, we can safely stop   |
|   searching without wasting any more time.               |
|                                                          |
 ----------------------------------------------------------
*/
  if ((root_value == root_alpha) && !search_failed_low) {
    searched=0;
    for (i=last[0];i<last[1];i++)
      if (searched_this_root_move[i-last[0]]) searched++;
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
  if (time_used < time_limit*2.5 && time_used+500<tc_time_remaining) return(0);
  if ((value >= last_value-667 && !search_failed_low) ||
      value>5500) return(abort);
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

/* last modified 09/29/96 */
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
void TimeSet(int search_type)
{
  int last_value, test_time_limit;
  float u_time,u_otime;
  int adjust=0;
  float percent_adjust=0.0;
  
  last_value=last_search_value;
  u_time=tc_time_remaining;
  u_otime=tc_time_remaining_opponent;

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
    if (xboard) {
/*
    for tuning down time_usage -  but we still want to keep the usage up-
     if we're ahead in time.
*/
      if (tc_time_remaining>tc_time_remaining_opponent &&  
          last_value <6000 && move_number>5 && u_time>3000) {
        usage_time=((float)Min((((Max(u_time,1.0)/Max(u_otime,1.0))-1.0)*50.0),50.0)); 
        adjust=1;
        percent_adjust=usage_time;
      }
/*
     We do have an increment.  If behind in time, we decraese usage for moves
     20>60.  As long as we are within 2/3 of pawn  After move 60. with
     increment; we're not interested in any further time usage reductions.
*/
      else if (tc_time_remaining<tc_time_remaining_opponent &&
               move_number>20 &&  move_number<60 && last_value >-667 && (tc_increment)) {
       usage_time=((float)Max((((Max(u_time,1.0)/Max(u_otime,1.0))-1.0)*50.0),-20.0));
        adjust=2;
        percent_adjust=usage_time;
      }
/*
    w/ zero increment - we need to be more aggresive about tuning down
    time usage.
*/
      else if (tc_time_remaining<tc_time_remaining_opponent && move_number>20 &&
               move_number<50 && last_value >-667 && (!tc_increment)) {
        usage_time=((float)Max((((Max(u_time,1.0)/Max(u_otime,1.0))-1.0)*50.0),-20.0));
        adjust=3;
        percent_adjust=usage_time;
      }
/*
    at this point we may be lost anyway - but we want to ensure that we don't
    get beat on time but we don't cut time_limit by more than 50%.
*/
      else if (tc_time_remaining<tc_time_remaining_opponent  &&
               move_number>49  && move_number< 80 &&  last_value >-5000 &&
               !tc_increment) {
        usage_time=((float)Max((((Max(u_time,1.0)/Max(u_otime,1.0))-1.0)*50.0),-40.0));
        adjust=4;
        percent_adjust=usage_time;
      }
/*
    final tune-down.
*/
      else if (tc_time_remaining<tc_time_remaining_opponent && move_number>79 &&
               last_value >-5000 && (!tc_increment) && u_time<3000) {
        usage_time=((float)Max((((Max(u_time,1.0)/Max(u_otime,1.0))-1.0)*100.0),-50.0));
        Print(1,"case 5 auto usage %7.1f percent/(-)decrease\n",usage_time);
        adjust=5;
        percent_adjust=usage_time;
      }
      else
        usage_time=0;
    }
    time_divisor=Max(move_limit-move_number,2);
    if (tc_increment) {
/*
    in case 0 * game.
*/
      time_limit=Min(((((tc_time_remaining*inc_time_multiplier/(time_divisor))+
                       tc_increment-tc_operator_time))*(1+usage_level/100)),
                       ((tc_time_remaining-500)));
/*
    Min function is here just to make sure our calculation does not extend
    beyond time remaining; next move, increment added will force test to
    meet < 3*increment test below.  increase this ^5 if you don't have
    timestamp.
*/
      if (tc_time_remaining < Max(1000,tc_increment*3))
        time_limit=tc_increment/2;
      absolute_time_limit=time_limit*6;
    }
    else {
      time_limit=(tc_time_remaining*zero_inc_factor/(time_divisor));
      absolute_time_limit=time_limit*6;
    }
  }
/*
 ----------------------------------------------------------
|                                                          |
|   we are not in a sudden_death situation.                |
|                                                          |
 ----------------------------------------------------------
*/
  else {
    time_limit=(tc_time_remaining - tc_operator_time)/
         (tc_moves_remaining+3)*1.8;
  }
  absolute_time_limit=time_limit+(tc_time_remaining/4);
  if (absolute_time_limit > 6*time_limit) absolute_time_limit=6*time_limit;

/*
   new option 'usage' increases and decrease the time factors)
*/
  
  if (usage_level != 0.0)  time_limit*=((float) 1.0+(usage_level/100.0));
  if (usage_time != 0.0)  time_limit*=((float) 1.0+(usage_time/100.0));
/*
 ----------------------------------------------------------
|                                                          |
|   if just out of book, increase the search time for a    |
|   couple of moves to avoid trouble.                      |
|                                                          |
 ----------------------------------------------------------*/

 if  (last_move_in_book+1 > move_number && !tc_sudden_death)
   time_limit*=((float) 1.4); 

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
  if ((search_type == puzzle) || (search_type == booking)) {
    time_limit/=30;
    absolute_time_limit=time_limit*3;
  }

  if (search_type != puzzle) {
    Print(1,"              time limit %s", DisplayTime(time_limit));
    if (adjust) Print(1," [%d/%5.1f",adjust,percent_adjust);
    if (usage_level != 0.0) {
      Print(1,"/");
      Print(1,"%5.1f",usage_level);
    }
    if (adjust | (usage_level != 0)) Print(1,"] ");
    if (easy_move) Print(1," [easy move]");
    Print(1,"\n");
  }
  test_time_limit=time_limit;
  if (time_used+time_used_opponent < test_time_limit && last_value>0 &&
      time_used+time_used_opponent>=1 && search_type != puzzle &&
      tc_time_remaining<tc_time_remaining_opponent && !auto_kibitzing &&
	  mode!=tournament_mode) {
    time_limit=Max(time_used+time_used_opponent,1+tc_increment/2);
    Print(1,"time_used   = %s\n",DisplayTime(time_used));
    Print(1,"time_used_opponent = %s\n",DisplayTime(time_used_opponent));
    Print(1,"reduced to time limit %s,  opponent is playing fast\n", 
          DisplayTime(time_limit));
  }
/* auto_kibitzers*/
  if (time_used+time_used_opponent < test_time_limit && last_value>0 &&
      time_used+time_used_opponent>=1 && search_type != puzzle &&
      tc_time_remaining<tc_time_remaining_opponent &&
      auto_kibitzing && mode!=tournament_mode) {
    time_limit=Max(time_used+time_used_opponent,1+tc_increment/2);
    Print(1,"time_used   = %s\n",DisplayTime(time_used));
    Print(1,"time_used_opponent = %s\n",DisplayTime(time_used_opponent));
    Print(1,"reduced to time limit %s,  computer opponent is playing fast\n", 
          DisplayTime(time_limit));
  } 
  if (time_limit <= 1) {
    time_limit= 1;
    usage_level=0;
  }
}
 