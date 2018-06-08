#include <stdio.h>
#include <stdlib.h>
#include "chess.h"
#include "data.h"
#include "evaluate.h"

/* last modified 11/15/00 */
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
void PreEvaluate(TREE *tree, int crafty_is_white) {
  int i, j;
  static int hashing_opening = 0;
  static int hashing_middle_game = 0;
  static int hashing_end_game = 0;
  static int last_crafty_is_white = 0;
  static int last_trojan_check = 0;
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
|   pawn advances.                                         |
|                                                          |
 ----------------------------------------------------------
*/
  pawn_advance[0]=PAWN_ADVANCE_A;
  pawn_advance[1]=PAWN_ADVANCE_B;
  pawn_advance[2]=PAWN_ADVANCE_C;
  pawn_advance[3]=PAWN_ADVANCE_D;
  pawn_advance[4]=PAWN_ADVANCE_E;
  pawn_advance[5]=PAWN_ADVANCE_F;
  pawn_advance[6]=PAWN_ADVANCE_G;
  pawn_advance[7]=PAWN_ADVANCE_H;
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
      pval_w[i*8+j]=pawn_base[j]+pawn_advance[j]*(i-1);
  for (j=A6;j<A8;j++)
    pval_w[j]+=PAWN_JAM;
  pval_w[D2]-=CENTER_PAWN_UNMOVED;
  pval_w[E2]-=CENTER_PAWN_UNMOVED;
  for (i=6;i>0;i--)
    for (j=0;j<8;j++)
      pval_b[i*8+j]=pawn_base[j]+pawn_advance[j]*(6-i);
  for (j=A2;j<A4;j++)
    pval_b[j]+=PAWN_JAM;
  pval_b[D7]-=CENTER_PAWN_UNMOVED;
  pval_b[E7]-=CENTER_PAWN_UNMOVED;
/*
 ----------------------------------------------------------
|                                                          |
|   now we set the king safety values based on the values  |
|   set by the user, or the default values.                |
|                                                          |
 ----------------------------------------------------------
*/
  if (crafty_is_white) {
    for (i=0;i<64;i++) {
      temper_w[i]=temper[i];
      temper_b[i]=temper[i]+temper[i]*king_safety_asymmetry/100;
    }
  }
  else {
    for (i=0;i<64;i++) {
      temper_w[i]=temper[i]+temper[i]*king_safety_asymmetry/100;
      temper_b[i]=temper[i];
    }
  }
  for (i=0;i<64;i++) {
    temper_w[i]=temper_w[i]*king_safety_scale/100;
    temper_b[i]=temper_b[i]*king_safety_scale/100;
  }
  for (i=0;i<128;i++)
    tropism[i]=king_tropism[i]*king_safety_tropism/100;
  for (i=0;i<9;i++)
    pawn_rams[i]=blocked_scale*pawn_rams_v[i]/100;
/*
 ----------------------------------------------------------
|                                                          |
|   now check to see if the "trojan check" code should be  |
|   turned on.  basically if the king is in the corner,    |
|   the opponent has placed a piece on g4/g5, and both     |
|   sides have pawns attacking that piece, and queens are  |
|   still on the board, then it is a threat that must be   |
|   handled.                                               |
|                                                          |
|   this is handled as 4 separate cases for each corner of |
|   the board, for simplicity.                             |
|                                                          |
 ----------------------------------------------------------
*/
  trojan_check=0;
  if (BlackQueens && BlackRooks) {
    if (WhiteKingSQ==G1 || WhiteKingSQ==H1) {
      if (SetMask(G4)&BlackKnights || SetMask(G4)&BlackBishops) {
        if (SetMask(H3)&WhitePawns && SetMask(H5)&BlackPawns) trojan_check=1;
      }
    }
    if (WhiteKingSQ==B1 || WhiteKingSQ==A1) {
      if (SetMask(B4)&BlackKnights || SetMask(B4)&BlackBishops) {
        if (SetMask(A3)&WhitePawns && SetMask(A5)&BlackPawns) trojan_check=1;
      }
    }
  }
  if (WhiteQueens && WhiteRooks) {
    if (BlackKingSQ==G8 || BlackKingSQ==H8) {
      if (SetMask(G5)&WhiteKnights || SetMask(G5)&WhiteBishops) {
        if (SetMask(H6)&BlackPawns && SetMask(H4)&WhitePawns) trojan_check=1;
      }
    }
    if (BlackKingSQ==B8 || BlackKingSQ==A8) {
      if (SetMask(B5)&WhiteKnights || SetMask(B5)&WhiteBishops) {
        if (SetMask(A6)&BlackPawns && SetMask(A4)&BlackPawns) trojan_check=1;
      }
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
  if (((last_crafty_is_white != crafty_is_white) ||
       (last_trojan_check    != trojan_check) ||
       (hashing_opening      != opening) ||
       (hashing_middle_game  != middle_game) ||
       (hashing_end_game     != end_game)) && !test_mode) {
/*
 ------------------------------------------------
|                                                |
|   if anything changed, the transposition table |
|   must be cleared of positional evaluations.   |
|                                                |
 ------------------------------------------------
*/
    if (trojan_check)
      Print(128,"              trojan check enabled\n");
    Print(128,"              clearing hash tables\n");
    ClearHashTableScores();
  }
  hashing_opening=opening;
  hashing_middle_game=middle_game;
  hashing_end_game=end_game;
  last_crafty_is_white=crafty_is_white;
  last_trojan_check=trojan_check;
}
