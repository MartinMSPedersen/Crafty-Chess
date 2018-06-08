#include "chess.h"
#include "data.h"
/* last modified 01/18/09 */
/*
 *******************************************************************************
 *                                                                             *
 *   PreEvaluate() is used to recognize positions where the "Trojan horse  -   *
 *   attack" is threatened, and it enables the specific code to handle that    *
 *   case.                                                                     *
 *                                                                             *
 *******************************************************************************
 */
void PreEvaluate(TREE * RESTRICT tree) {
  static int last_trojan_check = 0;
  static int last_clear = 0;

/*
 ************************************************************
 *                                                          *
 *   Check to see if the "trojan check" code should be      *
 *   turned on.  basically if the king is in the corner,    *
 *   the opponent has placed a piece on g4/g5, and both     *
 *   sides have pawns attacking that piece, and queens are  *
 *   still on the board, then it is a threat that must be   *
 *   handled.                                               *
 *                                                          *
 *   This is handled as 4 separate cases for each corner of *
 *   the board, for simplicity.                             *
 *                                                          *
 ************************************************************
 */
  trojan_check = 0;
  if (Queens(black) && Rooks(black)) {
    if (KingSQ(white) == G1 || KingSQ(white) == H1) {
      if (SetMask(G4) & Knights(black) || SetMask(G4) & Bishops(black)) {
        if (SetMask(H3) & Pawns(white) && SetMask(H5) & Pawns(black))
          trojan_check = 1;
      }
    }
    if (KingSQ(white) == B1 || KingSQ(white) == A1) {
      if (SetMask(B4) & Knights(black) || SetMask(B4) & Bishops(black)) {
        if (SetMask(A3) & Pawns(white) && SetMask(A5) & Pawns(black))
          trojan_check = 1;
      }
    }
  }
  if (Queens(white) && Rooks(white)) {
    if (KingSQ(black) == G8 || KingSQ(black) == H8) {
      if (SetMask(G5) & Knights(white) || SetMask(G5) & Bishops(white)) {
        if (SetMask(H6) & Pawns(black) && SetMask(H4) & Pawns(white))
          trojan_check = 1;
      }
    }
    if (KingSQ(black) == B8 || KingSQ(black) == A8) {
      if (SetMask(B5) & Knights(white) || SetMask(B5) & Bishops(white)) {
        if (SetMask(A6) & Pawns(black) && SetMask(A4) & Pawns(black))
          trojan_check = 1;
      }
    }
  }
/*
 ************************************************************
 *                                                          *
 *   Now, if any of the values above were changed, we must  *
 *   clear the appropriate hash tables so that the new      *
 *   values will be used to compute scores.                 *
 *                                                          *
 ************************************************************
 */
  if (last_trojan_check != trojan_check) {
    if (trojan_check)
      Print(128, "              trojan check enabled\n");
    Print(128, "              clearing hash tables\n");
    ClearHashTableScores();
    last_clear = move_number;
  }
/*
 ************************************************************
 *                                                          *
 *   Now for a kludge.  If we are in the last 10 moves of a *
 *   potential 50 move rule draw, we clear the hash table   *
 *   (scores only) every move to avoid hiding the 50 move   *
 *   draw scores from the search when we get hash table     *
 *   hits.                                                  *
 *                                                          *
 ************************************************************
 */
  if (Rule50Moves(0) > 80) {
    ClearHashTableScores();
    Print(128, "              clearing hash tables (50 moves fix)\n");
  }
  last_trojan_check = trojan_check;
}
