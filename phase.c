#include <stdio.h>
#include <stdlib.h>
#include "types.h"
#include "function.h"
#include "data.h"
/*
********************************************************************************
*                                                                              *
*   Phase() is use to determine the phase of the game: opening, middlegame or  *
*   endgame.  the three phases are identified by the following criteria:       *
*                                                                              *
*   (a) opening:  if any minor pieces are still on their original squares, or  *
*       the king has not yet castled nor given up the right to castle, then    *
*       opening development rules still apply.                                 *
*   (b) middle-game:  if the opening criteria are not met, and the end-game    *
*       criteria are not met, then all that's left is a middle-game.           *
*   (c) end-game:  if both sides have less than 17 points of material, not     *
*       counting pawns, (Q=9, R=5, BN=3) then we are in an end-game.           *
*                                                                              *
********************************************************************************
*/
void Phase(void)
{

/*
 ----------------------------------------------------------
|                                                          |
|   if the side-to-move has not yet lost the right to      |
|   castle, then this is still an opening position.        |
|                                                          |
 ----------------------------------------------------------
*/
  if (root_wtm)
    opening=White_Castle(1) ||
            Popcnt(And(Or(White_Knights(1),
                          White_Bishops(1)),
                       white_minor_pieces));
  else
    opening=Black_Castle(1) ||
            Popcnt(And(Or(Black_Knights(1),
                          Black_Bishops(1)),
                       black_minor_pieces));
/*
 ----------------------------------------------------------
|                                                          |
|   if the piece point count drops below 17 (two rooks and |
|   two minor pieces) then this is an end-game.            |
|                                                          |
 ----------------------------------------------------------
*/
  end_game= (Total_White_Pieces(1) < 17) &&
            (Total_Black_Pieces(1) < 17);
/*
 ----------------------------------------------------------
|                                                          |
|   if we are are not in an end-game, and not in the       |
|   opening, then we are in a middle-game position.        |
|                                                          |
 ----------------------------------------------------------
*/
  if (end_game) {
    opening=0;
    middle_game=0;
  }
  else if (opening) {
    middle_game=0;
    end_game=0;
  }
  else {
    opening=0;
    middle_game=1;
    end_game=0;
  }
  if (opening)
    Print(6,"opening phase\n");
  else if (middle_game)
    Print(6,"middle-game phase\n");
  else
    Print(6,"end-game phase\n");
}
