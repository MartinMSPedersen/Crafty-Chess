#include "chess.h"
#include "data.h"

/* last modified 06/05/98 */
/*
 *******************************************************************************
 *                                                                             *
 *   Drawn() is used to answer the question "is this position a hopeless draw?"*
 *   several considerations are included in making this decision, but the most *
 *   basic one is simple the remaining material for each side.  if either side *
 *   has pawns, it's not a draw.  with no pawns, equal material is a draw.     *
 *   otherwise, the superior side must have enough material to be able to force*
 *   a mate.                                                                   *
 *                                                                             *
 *******************************************************************************
 */
int Drawn(TREE * RESTRICT tree, int value)
{
/*
 ************************************************************
 *                                                          *
 *   if either side has pawns, the game is not a draw for   *
 *   lack of material.                                      *
 *                                                          *
 ************************************************************
 */
  if (TotalPawns(white) || TotalPawns(black))
    return (0);
/*
 ************************************************************
 *                                                          *
 *   the search result must indicate a draw also, otherwise *
 *   it could be a tactical win or loss.                    *
 *                                                          *
 ************************************************************
 */
  if (value != DrawScore(wtm))
    return (0);
/*
 ************************************************************
 *                                                          *
 *   if neither side has pawns, and one side has some sort  *
 *   of material superiority, then determine if the winning *
 *   side has enough material to win.                       *
 *                                                          *
 ************************************************************
 */
  if (TotalPieces(white) < 5 && TotalPieces(black) < 5)
    return (2);
  if (TotalPieces(white) == 5 || TotalPieces(white) > 6)
    return (0);
  if (TotalPieces(black) == 5 || TotalPieces(black) > 6)
    return (0);
  if ((TotalPieces(white) == 6 && !Bishops(white) && Material > 0) ||
      (TotalPieces(black) == 6 && !Bishops(black) && Material < 0))
    return (2);
/*
 ************************************************************
 *                                                          *
 *   if neither side has pawns, then one side must have     *
 *   some sort of material superiority, otherwise it is a   *
 *   draw.                                                  *
 *                                                          *
 ************************************************************
 */
  if (TotalPieces(white) == TotalPieces(black))
    return (1);
  return (0);
}
