#include <stdio.h>
#include "types.h"
#include "function.h"
#include "data.h"
#include "evaluate.h"
/*
********************************************************************************
*                                                                              *
*   Draw_Score() is used to determine how good or bad a draw is in the current *
*   game situation.  It depends on several factors, including the stage of the *
*   game (opening [draw=bad], middlegame [draw=somewhat bad] and endgames      *
*   [draw=default_draw_score.])  Additionally, it looks at the opponent's time *
*   remaining in the game, and if it's low, it will eschew the draw and press  *
*   the opponent to win before his flag falls.                                 *
*                                                                              *
********************************************************************************
*/
int Draw_Score(void)
{
  int draw_score;
/*
 ----------------------------------------------------------
|                                                          |
|   first, set the draw score base on the phase of the     |
|   game, as would be done normally, anyway.               |
|                                                          |
 ----------------------------------------------------------
*/
  if (opening) 
    draw_score=default_draw_score-PAWN_VALUE/2;
  else if (middle_game)
    draw_score=default_draw_score-PAWN_VALUE/5;
  else if (end_game)
    draw_score=default_draw_score;
/*
 ----------------------------------------------------------
|                                                          |
|   now that the base draw score has been set, look at the |
|   opponent's remaining time and set the draw score to a  |
|   lower value if his time is low.                        |
|                                                          |
|   the first test:  if both have less than one minute on  |
|   the clock, then if crafty has 2x the opponent's time   |
|   the draw should be avoided if possible.                |
|                                                          |
 ----------------------------------------------------------
*/
  if (tc_increment < 5) {
    if ((tc_time_remaining_opponent < 60) && (tc_time_remaining < 60)) {
      if (tc_time_remaining/Max(tc_time_remaining_opponent,1) > 1)
        draw_score=default_draw_score-PAWN_VALUE/2;
    }
    else {
      if (tc_time_remaining_opponent < 60)
        draw_score=default_draw_score-PAWN_VALUE/2;
      if (tc_time_remaining_opponent < 30)
        draw_score=default_draw_score-PAWN_VALUE;
    }
  }
  return(draw_score);
}
