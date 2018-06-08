#include <stdio.h>
#include <stdlib.h>
#include "types.h"
#include "function.h"
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
void PreEvaluate(int wtm)
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
  for (i=0;i<8;i++)
    for (j=0;j<8;j++)
      pawn_value_w[i*8+j]=pawn_advance[j]*(((i-1)>0) ? i-1 : 0);
  if (!end_game &&!WhiteCastle(1)>0 && !BlackCastle(1)>0) {
    if (And(WhiteKing,left_half_mask) && 
        And(BlackKing,right_half_mask)) {
      for (i=0;i<8;i++)
        for (j=5;j<8;j++)
          pawn_value_w[i*8+j]=PAWN_ADVANCE_KING*(((i-1)>0) ? i-1 : 0);
    }
    if (And(WhiteKing,right_half_mask) && 
        And(BlackKing,left_half_mask)) {
      for (i=0;i<8;i++)
        for (j=0;j<3;j++)
          pawn_value_w[i*8+j]=PAWN_ADVANCE_KING*(((i-1)>0) ? i-1 : 0);
    }
  }
/*
 ----------------------------------------------------------
|                                                          |
|   black pawns.                                           |
|                                                          |
 ----------------------------------------------------------
*/
  for (i=7;i>=0;i--)
    for (j=0;j<8;j++)
      pawn_value_b[i*8+j]=pawn_advance[j]*(((6-i)>0) ? 6-i : 0);
  if (!end_game && WhiteCastle(1)<=0 && BlackCastle(1)<=0) {
    if (And(BlackKing,left_half_mask) && 
        And(WhiteKing,right_half_mask)) {
      for (i=0;i<8;i++)
        for (j=5;j<8;j++)
          pawn_value_b[i*8+j]=PAWN_ADVANCE_KING*(((6-i)>0) ? 6-i : 0);
    }
    if (And(BlackKing,right_half_mask) && 
        And(WhiteKing,left_half_mask)) {
      for (i=0;i<8;i++)
        for (j=0;j<3;j++)
          pawn_value_b[i*8+j]=PAWN_ADVANCE_KING*(((6-i)>0) ? 6-i : 0);
    }
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
  if ((last_wtm              != wtm) ||
      (hashing_pawns         != hash_pawns) ||
      (hashing_opening       != opening) ||
      (hashing_middle_game   != middle_game) ||
      (hashing_end_game      != end_game)) {
/*
 ------------------------------------------------
|                                                |
|   if anything changed, the transposition table |
|   must be cleared of positional evaluations.   |
|                                                |
 ------------------------------------------------
*/
    if (trans_ref_ba && trans_ref_wa) {
      Print(4,"              clearing transposition table\n");
      for (i=0;i<hash_table_size;i++) {
        (trans_ref_ba+i)->word1=Or(And((trans_ref_ba+i)->word1,
                        mask_clear_entry),Shiftl((BITBOARD) 131072,21));
        (trans_ref_wa+i)->word1=Or(And((trans_ref_wa+i)->word1,
                        mask_clear_entry),Shiftl((BITBOARD) 131072,21));
      }
      for (i=0;i<2*hash_table_size;i++) {
        (trans_ref_bb+i)->word1=Or(And((trans_ref_bb+i)->word1,
                        mask_clear_entry),Shiftl((BITBOARD) 131072,21));
        (trans_ref_wb+i)->word1=Or(And((trans_ref_wb+i)->word1,
                        mask_clear_entry),Shiftl((BITBOARD) 131072,21));
      }
    }
/*
 ------------------------------------------------
|                                                |
|   if pawn status has changed or we switched    |
|   sides, pawn hash tables must be cleared.     |
|                                                |
 ------------------------------------------------
*/
    if ((hashing_pawns         != hash_pawns) ||
        (hashing_opening       != opening) ||
        (hashing_middle_game   != middle_game) ||
        (hashing_end_game      != end_game) ||
        (last_wtm              != wtm)) {
      if (pawn_hash_table) {
        Print(4,"              clearing pawn hash tables\n");
        for (i=0;i<pawn_hash_table_size;i++) {
          (pawn_hash_table+i)->word1=0;
          (pawn_hash_table+i)->word2=0;
        }
      }
    }
  }
  hashing_pawns=hash_pawns;
  hashing_opening=opening;
  hashing_middle_game=middle_game;
  hashing_end_game=end_game;
  last_wtm=wtm;
}
