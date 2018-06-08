#include <stdio.h>
#include <stdlib.h>
#include "chess.h"
#include "data.h"
#include "evaluate.h"

/* last modified 06/06/99 */
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
void PreEvaluate(TREE *tree, int wtm) {
  int i, j;
  static int hashing_pawns = 0;
  static int hashing_opening = 0;
  static int hashing_middle_game = 0;
  static int hashing_end_game = 0;
  static int hashing_kings = 0;
  static int last_wtm = 0;
  int hash_pawns=0, hash_kings=0;
  int pawn_advance[8], pawn_base[8];
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
  pawn_base[0]=PAWN_BASE_A;
  pawn_base[1]=PAWN_BASE_B;
  pawn_base[2]=PAWN_BASE_C;
  pawn_base[3]=PAWN_BASE_D;
  pawn_base[4]=PAWN_BASE_E;
  pawn_base[5]=PAWN_BASE_F;
  pawn_base[6]=PAWN_BASE_G;
  pawn_base[7]=PAWN_BASE_H;
/*
 ----------------------------------------------------------
|                                                          |
|   pawns.                                                 |
|                                                          |
 ----------------------------------------------------------
*/
  for (i=1;i<7;i++)
    for (j=0;j<8;j++)
      pval_w[i*8+j]=pawn_base[j]+pawn_advance[j]*(i-3);
  for (j=A6;j<A8;j++)
    pval_w[j]+=PAWN_JAM;
  pval_w[D2]-=CENTER_PAWN_UNMOVED;
  pval_w[E2]-=CENTER_PAWN_UNMOVED;
  for (i=6;i>0;i--)
    for (j=0;j<8;j++)
      pval_b[i*8+j]=pawn_base[j]+pawn_advance[j]*(4-i);
  for (j=A2;j<A4;j++)
    pval_b[j]+=PAWN_JAM;
  pval_b[D7]-=CENTER_PAWN_UNMOVED;
  pval_b[E7]-=CENTER_PAWN_UNMOVED;
/*
 9---------------------------------------------------------
|                                                          |
|   kings.                                                 |
|                                                          |
 ----------------------------------------------------------
*/
  if (((WhitePawns | BlackPawns) & mask_efgh) &&
      ((WhitePawns | BlackPawns) & mask_abcd)) {
    hash_kings=1;
    for (i=0;i<64;i++) {
      kval_w[i]=kval_wn[i];
      kval_b[i]=kval_bn[i];
    }
  }
  else if ((WhitePawns | BlackPawns) & mask_efgh) {
    hash_kings=2;
    for (i=0;i<64;i++) {
      kval_w[i]=kval_wk[i];
      kval_b[i]=kval_bk[i];
    }
  }
  else if ((WhitePawns | BlackPawns) & mask_abcd) {
    hash_kings=3;
    for (i=0;i<64;i++) {
      kval_w[i]=kval_wq[i];
      kval_b[i]=kval_bq[i];
    }
  }
/*
 ----------------------------------------------------------
|                                                          |
|   now we set the king safety values based on the values  |
|   set by the user, or the default values.                |
|                                                          |
 ----------------------------------------------------------
*/
  if (wtm) {
    for (i=0;i<32;i++) {
      temper_w[i]=temper[i];
      temper_b[i]=temper[i]+temper[i]*king_safety_asymmetry/100;
    }
  }
  else {
    for (i=0;i<32;i++) {
      temper_w[i]=temper[i]+temper[i]*king_safety_asymmetry/100;
      temper_b[i]=temper[i];
    }
  }
  for (i=0;i<32;i++) {
    temper_w[i]=temper_w[i]*king_safety_scale/100;
    temper_b[i]=temper_b[i]*king_safety_scale/100;
  }
  for (i=0;i<128;i++)
    tropism[i]=king_tropism[i]*king_safety_tropism/100;
  for (i=0;i<32;i++)
    temper_b[i]=temper_b[i]*king_safety_scale/100;
  for (i=0;i<9;i++)
    pawn_rams[i]=blocked_scale*pawn_rams_v[i]/100;
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
       (hashing_kings       != hash_kings) ||
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
    ClearHashTableScores();
  }
  hashing_pawns=hash_pawns;
  hashing_kings=hash_kings;
  hashing_opening=opening;
  hashing_middle_game=middle_game;
  hashing_end_game=end_game;
  last_wtm=wtm;
}
