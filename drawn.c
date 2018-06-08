#include <stdio.h>
#include <stdlib.h>
#include "types.h"
#include "function.h"
#include "evaluate.h"
#include "data.h"
/*
********************************************************************************
*                                                                              *
*   Drawn() is used to answer the question "is this position a hopeless draw?" *
*   several considerations are included in making this decision, but the most  *
*   basic one is simple the remaining material for each side.  if either side  *
*   has pawns, it's not a draw.  with no pawns, equal material is a draw.      *
*   otherwise, the superior side must have enough material to be able to force *
*   a mate.                                                                    *
*                                                                              *
********************************************************************************
*/
int Drawn(int ply)
{
/*
 ----------------------------------------------------------
|                                                          |
|   if either side has pawns, the game is not a draw for   |
|   lack of material.                                      |
|                                                          |
 ----------------------------------------------------------
*/
  if (White_Pawns(ply) ||
      Black_Pawns(ply)) return(0);
/*
 ----------------------------------------------------------
|                                                          |
|   if neither side has pawns, then one side must have     |
|   some sort of material superiority, otherwise it is a   |
|   draw.                                                  |
|                                                          |
 ----------------------------------------------------------
*/
  if (Total_White_Pieces(ply) ==
      Total_Black_Pieces(ply)) return(1);
/*
 ----------------------------------------------------------
|                                                          |
|   if neither side has pawns, and one side has some sort  |
|   of material superiority, then determine if the winning |
|   side has enough material to win.                       |
|                                                          |
 ----------------------------------------------------------
*/
  if ((Total_White_Pieces(ply) != 5) ||
      (Total_Black_Pieces(ply) != 5)) {
    if ((Total_White_Pieces(ply) == 5) ||
        (Total_White_Pieces(ply) > 6) ||
        ((Total_White_Pieces(ply) == 6) &&
         (White_Bishops(ply)))) return(0);
    if ((Total_Black_Pieces(ply) == 5) ||
        (Total_Black_Pieces(ply) > 6) ||
        ((Total_Black_Pieces(ply) == 6) &&
         (Black_Bishops(ply)))) return(0);
  }
  return(1);
}
