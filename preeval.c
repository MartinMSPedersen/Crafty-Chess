#include <stdio.h>
#include <stdlib.h>
#include "chess.h"
#include "data.h"
#include "evaluate.h"

/* last modified 05/18/96 */
/*
********************************************************************************
*                                                                              *
*   PreEvaluate() is used to set the piece/square tables.  these tables        *
*   contain evaluation parameters that are not dynamic in nature and don't     *
*   change during the course of a single iteration.                            *
*                                                                              *
*   there is one piece/square table for each type of piece on the board, one   *
*   for each side so that opposite sides evaluate things from their per-       *
*   spective.                                                                  *
*                                                                              *
********************************************************************************
*/
void PreEvaluate(TREE *tree, int wtm)
{
  int i, j;
  static int hashing_pawns = 0;
  static int hashing_opening = 0;
  static int hashing_middle_game = 0;
  static int hashing_end_game = 0;
  static int last_wtm = 0;
  int hash_pawns=0;
/*
 ----------------------------------------------------------
|                                                          |
|   the first step is to determine if we are in the        |
|   opening (not castled), middle-game, or end-game (with  |
|   material <= 15 for both sides).                        |
|                                                          |
 ----------------------------------------------------------
*/
  Phase();
/*
 ----------------------------------------------------------
|                                                          |
|   pawn advances.  before castling, moving the king-side  |
|   pawns is a no-no.  also, the b-pawn is restrained just |
|   in case we castle queen-side.                          |
|                                                          |
 ----------------------------------------------------------
*/
  if (opening) {
    hash_pawns=1;
    pawn_advance[0]=PAWN_ADVANCE_BC_A;
    pawn_advance[1]=PAWN_ADVANCE_BC_B;
    pawn_advance[2]=PAWN_ADVANCE_BC_C;
    pawn_advance[3]=PAWN_ADVANCE_BC_D;
    pawn_advance[4]=PAWN_ADVANCE_BC_E;
    pawn_advance[5]=PAWN_ADVANCE_BC_F;
    pawn_advance[6]=PAWN_ADVANCE_BC_G;
    pawn_advance[7]=PAWN_ADVANCE_BC_H;
  }
  else if (middle_game) {
    hash_pawns=2;
    pawn_advance[0]=PAWN_ADVANCE_A;
    pawn_advance[1]=PAWN_ADVANCE_B;
    pawn_advance[2]=PAWN_ADVANCE_C;
    pawn_advance[3]=PAWN_ADVANCE_D;
    pawn_advance[4]=PAWN_ADVANCE_E;
    pawn_advance[5]=PAWN_ADVANCE_F;
    pawn_advance[6]=PAWN_ADVANCE_G;
    pawn_advance[7]=PAWN_ADVANCE_H;
  }
  else {
    hash_pawns=3;
    pawn_advance[0]=PAWN_ADVANCE_EG_A;
    pawn_advance[1]=PAWN_ADVANCE_EG_B;
    pawn_advance[2]=PAWN_ADVANCE_EG_C;
    pawn_advance[3]=PAWN_ADVANCE_EG_D;
    pawn_advance[4]=PAWN_ADVANCE_EG_E;
    pawn_advance[5]=PAWN_ADVANCE_EG_F;
    pawn_advance[6]=PAWN_ADVANCE_EG_G;
    pawn_advance[7]=PAWN_ADVANCE_EG_H;
  }
/*
 ----------------------------------------------------------
|                                                          |
|   white pawns.                                           |
|                                                          |
 ----------------------------------------------------------
*/
  for (i=1;i<7;i++)
    for (j=0;j<8;j++)
      pawn_value_w[i*8+j]=pawn_advance[j]*(i-3);
  for (j=A6;j<A8;j++)
    pawn_value_w[j]=PAWN_JAM;
  if (!end_game) {
    pawn_value_w[E4]+=CENTER_PAWNS;
    pawn_value_w[E5]+=CENTER_PAWNS;
    pawn_value_w[E6]+=CENTER_PAWNS;
    pawn_value_w[D4]+=CENTER_PAWNS;
    pawn_value_w[D5]+=CENTER_PAWNS;
    pawn_value_w[D6]+=CENTER_PAWNS;
  }
/*
 ----------------------------------------------------------
|                                                          |
|   black pawns.                                           |
|                                                          |
 ----------------------------------------------------------
*/
  for (i=6;i>0;i--)
    for (j=0;j<8;j++)
      pawn_value_b[i*8+j]=pawn_advance[j]*(4-i);
  for (j=A2;j<A4;j++)
    pawn_value_b[j]=PAWN_JAM;
  if (!end_game) {
    pawn_value_b[E3]+=CENTER_PAWNS;
    pawn_value_b[E4]+=CENTER_PAWNS;
    pawn_value_b[E5]+=CENTER_PAWNS;
    pawn_value_b[D3]+=CENTER_PAWNS;
    pawn_value_b[D4]+=CENTER_PAWNS;
    pawn_value_b[D5]+=CENTER_PAWNS;
  }
/*
 ----------------------------------------------------------
|                                                          |
|   now, if any of the values above were changed, we must  |
|   clear the appropriate hash tables so that the new      |
|   values will be used to compute scores.                 |
|                                                          |
 ----------------------------------------------------------
*/
  if (((last_wtm            != wtm) ||
       (hashing_pawns       != hash_pawns) ||
       (hashing_opening     != opening) ||
       (hashing_middle_game != middle_game) ||
       (hashing_end_game    != end_game)) && !test_mode) {
/*
 ------------------------------------------------
|                                                |
|   if anything changed, the transposition table |
|   must be cleared of positional evaluations.   |
|                                                |
 ------------------------------------------------
*/
    Print(128,"              clearing hash tables\n");
    ClearHashTables();
  }
  hashing_pawns=hash_pawns;
  hashing_opening=opening;
  hashing_middle_game=middle_game;
  hashing_end_game=end_game;
  last_wtm=wtm;
}
