#include <stdio.h>
#include <stdlib.h>
#include "chess.h"
#include "data.h"

/* last modified 03/14/96 */
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
  int t_opening, t_middle_game, t_end_game;
  TREE *tree=local[0];

/*
 ----------------------------------------------------------
|                                                          |
|   if the side-to-move has not yet lost the right to      |
|   castle, then this is still an opening position.        |
|                                                          |
 ----------------------------------------------------------
*/
  t_opening=opening;
  t_middle_game=middle_game;
  t_end_game=end_game;
  if (opening) {
    do {
      if (move_number < 26) {
        if (root_wtm) {
          if (WhiteCastle(1)>0) break;
          if (And(WhiteBishops,white_minor_pieces)) break;
          if (And(WhiteKnights,white_minor_pieces)) break;
        }
        else {
          if (BlackCastle(1)>0) break;
          if (And(BlackBishops,black_minor_pieces)) break;
          if (And(BlackKnights,black_minor_pieces)) break;
        }
      }
      opening=0;
      middle_game=1;
      end_game=0;
    } while (0);
  }
/*
 ----------------------------------------------------------
|                                                          |
|   if the piece point count drops below 17 (two rooks and |
|   two minor pieces) then this is an end-game.            |
|                                                          |
 ----------------------------------------------------------
*/
  if (TotalWhitePieces < 14 && TotalBlackPieces < 14) {
    opening=0;
    middle_game=0;
    end_game=1;
  }
  if (opening && opening != t_opening)
    Print(128,"opening phase\n");
  else if (middle_game && middle_game != t_middle_game)
    Print(128,"middle-game phase\n");
  else if (end_game != t_end_game)
    Print(128,"end-game phase\n");
}
