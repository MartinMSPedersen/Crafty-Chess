#include "chess.h"
#include "data.h"
/* last modified 06/05/98 */
/*
 *******************************************************************************
 *                                                                             *
 *   Drawn() is used to answer the question "is this position a hopeless draw?"*
 *   Several considerations are included in making this decision, but the most *
 *   basic one is simply the remaining material for each side.  If either side *
 *   has pawns, it's not a draw.  With no pawns, equal material is a draw.     *
 *   Otherwise, the superior side must have enough material to be able to force*
 *   a mate.                                                                   *
 *                                                                             *
 *******************************************************************************
 */
int Drawn(TREE * RESTRICT tree, int value) {
/*
 ************************************************************
 *                                                          *
 *   If either side has pawns, the game is not a draw for   *
 *   lack of material.                                      *
 *                                                          *
 ************************************************************
 */
  if (TotalPieces(white, pawn) || TotalPieces(black, pawn))
    return (0);
/*
 ************************************************************
 *                                                          *
 *   The search result must indicate a draw also, otherwise *
 *   it could be a tactical win or loss.                    *
 *                                                          *
 ************************************************************
 */
  if (value != DrawScore(wtm))
    return (0);
/*
 ************************************************************
 *                                                          *
 *   If neither side has pawns, and one side has some sort  *
 *   of material superiority, then determine if the winning *
 *   side has enough material to win.                       *
 *                                                          *
 ************************************************************
 */
  if (TotalPieces(white, occupied) < 5 && TotalPieces(black, occupied) < 5)
    return (2);
  if (TotalPieces(white, occupied) == 5 || TotalPieces(white, occupied) > 6)
    return (0);
  if (TotalPieces(black, occupied) == 5 || TotalPieces(black, occupied) > 6)
    return (0);
  if ((TotalPieces(white, occupied) == 6 && !Bishops(white) && Material > 0)
      || (TotalPieces(black, occupied) == 6 && !Bishops(black) &&
          Material < 0))
    return (2);
/*
 ************************************************************
 *                                                          *
 *   If neither side has pawns, then one side must have     *
 *   some sort of material superiority, otherwise it is a   *
 *   draw.                                                  *
 *                                                          *
 ************************************************************
 */
  if (TotalPieces(white, occupied) == TotalPieces(black, occupied))
    return (1);
  return (0);
}
