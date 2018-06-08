#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "chess.h"
#include "data.h"

/* last modified 06/05/98 */
/*
********************************************************************************
*                                                                              *
*   ResignOrDraw() is used to determine if the program should either resign    *
*   or offer a draw.  this decision is based on two criteria:  (1) current     *
*   search evaluation and (2) time remaining on opponent's clock.              *
*                                                                              *
*   the evaluation returned by the last search must be less than the resign    *
*   threshold to trigger the resign code, or else must be exactly equal to the *
*   draw score to trigger the draw code.                                       *
*                                                                              *
*   the opponent must have enough time to be able to win or draw the game if   *
*   were played out as well.                                                   *
*                                                                              *
********************************************************************************
*/
void ResignOrDraw(TREE *tree, int value) {
  int returnv=0;
/*
 ----------------------------------------------------------
|                                                          |
|   if the game is a technical draw, where there are no    |
|   pawns and material is balanced, the offer a draw.      |
|                                                          |
 ----------------------------------------------------------
*/
  if(Drawn(tree,value) == 1) returnv=2;
/*
 ----------------------------------------------------------
|                                                          |
|   first check to see if the increment is 2 seconds or    |
|   more.  if so, then the game is being played slowly     |
|   enough that a draw offer or resignation is worth       |
|   consideration.  otherwise, if the opponent has at      |
|   least 30 seconds left, he can probably play the draw   |
|   or win out.                                            |
|                                                          |
|   if the value is below the resignation threshold, and   |
|   then crafty should resign and get on to the next game. |
|   note that it is necessary to have a bad score for      |
|   <resign_count> moves in a row before resigning.        |
|                                                          |
 ----------------------------------------------------------
*/
  if ((tc_increment > 200) || (tc_time_remaining_opponent >= 3000)) {
    if (resign) {
      if (value < -(MATE-15)) {
        if (++resign_counter >= resign_count) returnv=1;
      }
      else if (value<-resign*100 && value>-(MATE-300)) {
        if (++resign_counter >= resign_count) returnv=1;
      }
      else resign_counter=0;
    }
  }
/*
 ----------------------------------------------------------
|                                                          |
|   if the value is exactly equal to the draw score, then  |
|   crafty should offer the opponent a draw.  note that  . |
|   it is necessary that the draw score occur on exactly   |
|   <draw_count> moves in a row before making the offer.   |
|   note also that the draw offer will be repeated every   |
|   <draw_count> moves so setting this value too low can   |
|   make the program behave "obnoxiously."                 |
|                                                          |
 ----------------------------------------------------------
*/
  if ((tc_increment > 200) || (tc_time_remaining_opponent >= 3000)) {
    if (value==DrawScore(wtm) && moves_out_of_book>3) {
      if (++draw_counter >= draw_count) {
        draw_counter=0;
        returnv=3;
      }
    }
    else draw_counter=0;
  }
/*
 ----------------------------------------------------------
|                                                          |
|   now print the draw offer or resignation if appropriate |
|   but be sure and do it in a form that ICC/FICS will     |
|   understand if the "ics" or "xboard" flag is set.       |
|                                                          |
 ----------------------------------------------------------
*/
  if (returnv == 1) {
    LearnBook(tree,crafty_is_white,-300,0,1,2);
    if (xboard) Print(4095,"tellics resign\n");
    if (crafty_is_white) {
      Print(4095,"0-1 {White resigns}\n");
      strcpy(pgn_result,"0-1");
    }
    else {
      Print(4095,"1-0 {Black resigns}\n");
      strcpy(pgn_result,"1-0");
    }
  }
  if (returnv == 2) {
    if (!ics && !xboard) Print(4095,"\nI offer a draw.\n\n");
    else if (xboard) Print(4095,"offer draw\n");
    else Print(4095,"\n*draw\n");
  }
}
