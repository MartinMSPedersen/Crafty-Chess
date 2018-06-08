#include <stdio.h>
#include <stdlib.h>
#include "chess.h"
#include "data.h"

/* last modified 08/07/05 */
/*
 *******************************************************************************
 *                                                                             *
 *   PreEvaluate() is used to set some scoring information that can be user-   *
 *   modified by changing the asymmetry and scaling parameters.  all we need   *
 *   to do is perform the scaling of the values, based on user parameters and  *
 *   then return.  this code also recognizes a potential "trojan horse" attack *
 *   and turns on the flag that forces Evaluate() to handle this.              *
 *                                                                             *
 *******************************************************************************
 */
void PreEvaluate(TREE * RESTRICT tree, int crafty_is_white)
{
  int i;
  static int last_crafty_is_white = 0;
  static int last_trojan_check = 0;
  static int last_clear = 0;

/*
 ************************************************************
 *                                                          *
 *   now we set the king safety values based on the values  *
 *   set by the user, or the default values.                *
 *                                                          *
 ************************************************************
 */
  if (crafty_is_white) {
    for (i = 0; i < 64; i++) {
      temper_w[i] = temper[i];
      temper_b[i] = temper[i] + temper[i] * king_safety_asymmetry / 100;
    }
  } else {
    for (i = 0; i < 64; i++) {
      temper_w[i] = temper[i] + temper[i] * king_safety_asymmetry / 100;
      temper_b[i] = temper[i];
    }
  }
  for (i = 0; i < 64; i++) {
    temper_w[i] = temper_w[i] * king_safety_scale / 100;
    temper_b[i] = temper_b[i] * king_safety_scale / 100;
  }
  for (i = 0; i < 128; i++)
    tropism[i] = king_tropism[i] * king_safety_tropism / 100;
  for (i = 0; i < 9; i++)
    pawn_rams[i] = blocked_scale * pawn_rams_v[i] / 100;
/*
 ************************************************************
 *                                                          *
 *   now check to see if the "trojan check" code should be  *
 *   turned on.  basically if the king is in the corner,    *
 *   the opponent has placed a piece on g4/g5, and both     *
 *   sides have pawns attacking that piece, and queens are  *
 *   still on the board, then it is a threat that must be   *
 *   handled.                                               *
 *                                                          *
 *   this is handled as 4 separate cases for each corner of *
 *   the board, for simplicity.                             *
 *                                                          *
 ************************************************************
 */
  shared->trojan_check = 0;
  if (BlackQueens && BlackRooks) {
    if (WhiteKingSQ == G1 || WhiteKingSQ == H1) {
      if (SetMask(G4) & BlackKnights || SetMask(G4) & BlackBishops) {
        if (SetMask(H3) & WhitePawns && SetMask(H5) & BlackPawns)
          shared->trojan_check = 1;
      }
    }
    if (WhiteKingSQ == B1 || WhiteKingSQ == A1) {
      if (SetMask(B4) & BlackKnights || SetMask(B4) & BlackBishops) {
        if (SetMask(A3) & WhitePawns && SetMask(A5) & BlackPawns)
          shared->trojan_check = 1;
      }
    }
  }
  if (WhiteQueens && WhiteRooks) {
    if (BlackKingSQ == G8 || BlackKingSQ == H8) {
      if (SetMask(G5) & WhiteKnights || SetMask(G5) & WhiteBishops) {
        if (SetMask(H6) & BlackPawns && SetMask(H4) & WhitePawns)
          shared->trojan_check = 1;
      }
    }
    if (BlackKingSQ == B8 || BlackKingSQ == A8) {
      if (SetMask(B5) & WhiteKnights || SetMask(B5) & WhiteBishops) {
        if (SetMask(A6) & BlackPawns && SetMask(A4) & BlackPawns)
          shared->trojan_check = 1;
      }
    }
  }
/*
 ************************************************************
 *                                                          *
 *   now, if any of the values above were changed, we must  *
 *   clear the appropriate hash tables so that the new      *
 *   values will be used to compute scores.                 *
 *                                                          *
 ************************************************************
 */
  if (((last_crafty_is_white != crafty_is_white) ||
          (last_trojan_check != shared->trojan_check)) && !test_mode) {
/*
 **************************************************
 *                                                *
 *   if anything changed, the transposition table *
 *   must be cleared of positional evaluations.   *
 *                                                *
 **************************************************
 */
    if (shared->trojan_check)
      Print(128, "              trojan check enabled\n");
    Print(128, "              clearing hash tables\n");
    ClearHashTableScores(1);
/*
 ***************************************************
 *                                                 *
 *   now install the learned position information  *
 *   in the hash table.                            *
 *                                                 *
 ***************************************************
 */
    LearnPositionLoad();
    last_clear = shared->move_number;
  }
/*
 ************************************************************
 *                                                          *
 *   now for a kludge.  If we are beyond the halfway point  *
 *   with respect to a 50 move draw, then clear the hash    *
 *   scores every 10 moves to avoid hiding the 50 move      *
 *   result from the search due to hash hits.               *
 *                                                          *
 ************************************************************
 */
  if (Rule50Moves(0) > 50) {
    if (last_clear < shared->move_number - 10 || Rule50Moves(0) > 80) {
      ClearHashTableScores(0);
      LearnPositionLoad();
      Print(128, "              clearing hash tables (50 moves fix)\n");
    }
  }
  last_crafty_is_white = crafty_is_white;
  last_trojan_check = shared->trojan_check;
}
