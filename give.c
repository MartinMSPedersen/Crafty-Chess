#include <stdio.h>
#include <stdlib.h>
#include "types.h"
#include "function.h"
#include "data.h"
/*
********************************************************************************
*                                                                              *
*   Give_Check() is used to determine if a move checks the opponent.  this is  *
*   not as simple as it sounds because we don't want to use make_move() to     *
*   update the attack bitboards since it is relatively slow.  this code is     *
*   much faster, and operates in two phases:                                   *
*                                                                              *
*   phase(1) simple determines if the piece attacks the enemy king after the   *
*   move is "made" (but the attack boards are *not* updated.  this is actually *
*   somewhat akin to what make_move() does, but it takes the attack vector of  *
*   a piece and checks to see if it attacks the kings square (only.)  for      *
*   knights and pawns, the piece must directly attack the square the king is   *
*   on.  for sliding pieces, we only check the ray connecting the destination  *
*   square with the king's square (if any.)          .                         *
*                                                                              *
*   phase(2) takes the origin square of the move and determines if it inter-   *
*   sects with the king's square.  if so, we simply ask if the origin square   *
*   is attacked by a sliding piece in that same direction.  if the answer is   *
*   yes, this is a "discovered check."                                         *
*                                                                              *
********************************************************************************
*/
int Give_Check(int ply, int wtm, int *move)
{
  int from, to, king_square, piece;
  BITBOARD target, king_mask, occupied, temp, from_attacks;
/*
 ----------------------------------------------------------
|                                                          |
|   first, determine the square of the enemy king.         |
|                                                          |
 ----------------------------------------------------------
*/
  to=To(*move);
  from=From(*move);
  if (wtm) {
    king_square=Black_King_SQ(ply);
    king_mask=Black_King(ply);
  }
  else {
    king_square=White_King_SQ(ply);
    king_mask=White_King(ply);
  }
  piece=Promote(*move) ? Promote(*move) : Piece(*move);
  occupied=Or(White_Pieces(ply),
              Black_Pieces(ply));
/*
 ----------------------------------------------------------
|                                                          |
|   now the question is, does the piece directly attack    |
|   king after the piece is placed on the destination      |
|   square?                                                |
|                                                          |
 ----------------------------------------------------------
*/
  switch (piece) {
/*
 ------------------------------------------------
|                                                |
|   pawn move.                                   |
|                                                |
 ------------------------------------------------
*/
  case pawn:
    if (wtm) {
      if (And(w_pawn_attacks[to],king_mask)) return(1);
    }
    else {
      if (And(b_pawn_attacks[to],king_mask)) return(1);
    }
    break;
/*
 ------------------------------------------------
|                                                |
|   knight move.                                 |
|                                                |
 ------------------------------------------------
*/
  case knight:
    if (And(knight_attacks[to],king_mask)) return(1);
    break;
/*
 ------------------------------------------------
|                                                |
|   bishop move.  this is a little harder, but   |
|   the idea is to find the first blocker in     |
|   each direction and see if it's the king.  if |
|   so, this is a direct checking move.          |
|                                                |
 ------------------------------------------------
*/
  case bishop:
    temp=And(bishop_attacks[to],occupied);
    if (king_square == 
        First_One(And(temp,mask_plus7dir[to]))) return(1);
    if (king_square == 
        First_One(And(temp,mask_plus9dir[to]))) return(1);
    if (king_square == 
        Last_One(And(temp,mask_minus7dir[to]))) return(1);
    if (king_square == 
        Last_One(And(temp,mask_minus9dir[to]))) return(1);
    break;
/*
 ------------------------------------------------
|                                                |
|   rook move.  this is a little harder, but     |
|   the idea is to find the first blocker in     |
|   each direction and see if it's the king.  if |
|   so, this is a direct checking move.          |
|                                                |
 ------------------------------------------------
*/
  case rook:
    temp=And(rook_attacks[to],occupied);
    if (king_square == 
        First_One(And(temp,mask_plus1dir[to]))) return(1);
    if (king_square == 
        First_One(And(temp,mask_plus8dir[to]))) return(1);
    if (king_square == 
        Last_One(And(temp,mask_minus1dir[to]))) return(1);
    if (king_square == 
        Last_One(And(temp,mask_minus8dir[to]))) return(1);
    break;
/*
 ------------------------------------------------
|                                                |
|   queen move.  this is a little harder, but    |
|   the idea is to find the first blocker in     |
|   each direction and see if it's the king.  if |
|   so, this is a direct checking move.          |
|                                                |
 ------------------------------------------------
*/
  case queen:
    temp=And(queen_attacks[to],occupied);
    if (king_square == 
        First_One(And(temp,mask_plus1dir[to]))) return(1);
    if (king_square == 
        First_One(And(temp,mask_plus7dir[to]))) return(1);
    if (king_square == 
        First_One(And(temp,mask_plus8dir[to]))) return(1);
    if (king_square == 
        First_One(And(temp,mask_plus9dir[to]))) return(1);
    if (king_square == 
        Last_One(And(temp,mask_minus1dir[to]))) return(1);
    if (king_square == 
        Last_One(And(temp,mask_minus7dir[to]))) return(1);
    if (king_square == 
        Last_One(And(temp,mask_minus8dir[to]))) return(1);
    if (king_square == 
        Last_One(And(temp,mask_minus9dir[to]))) return(1);
    break;
/*
 ------------------------------------------------
|                                                |
|   king move.                                   |
|                                                |
 ------------------------------------------------
*/
  case king:
    if (abs(from-to) == 2) {
      if (wtm) {
        if (to == 2)
          to=3;
        else
          to=5;
        temp=And(rook_attacks[to],occupied);
        if (king_square == 
            First_One(And(temp,mask_plus8dir[to]))) return(1);
      }
      else {
        if (to == 58)
          to=59;
        else
          to=61;
        temp=And(rook_attacks[to],occupied);
        if (king_square == 
            Last_One(And(temp,mask_minus8dir[to]))) return(1);
      }
      return(0);
    }
  }
/*
 ----------------------------------------------------------
|                                                          |
|   now determine if the origin square of this piece is on |
|   the same ray as the king.  if so, then we have a       |
|   potential discovered check.  the test, then is to      |
|   continue looking in the same direction (king->piece)   |
|   to see if the origin square is attacked by a sliding   |
|   piece in the same direction.  if so, we have a classic |
|   discovered check.                                      |
|                                                          |
 ----------------------------------------------------------
*/
  if (!directions[king_square][from] ||
      (Piece(*move) == queen)) return(0);
  if (wtm)
    from_attacks=And(Attacks_To(from,ply),White_Pieces(ply));
  else
    from_attacks=And(Attacks_To(from,ply),Black_Pieces(ply));

  switch (directions[king_square][from]) {
    case +1:
      if (Piece(*move) == rook) return(0);
      if (Last_One(And(mask_minus1dir[from],occupied))
          != king_square) return(0);
      target=And(mask_plus1dir[from],from_attacks);
      break;
    case +7:
      if (Piece(*move) == bishop) return(0);
      if (Last_One(And(mask_minus7dir[from],occupied))
          != king_square) return(0);
      target=And(mask_plus7dir[from],from_attacks);
      break;
    case +8:
      if (Piece(*move) == rook) return(0);
      if (Last_One(And(mask_minus8dir[from],occupied))
          != king_square) return(0);
      target=And(mask_plus8dir[from],from_attacks);
      break;
    case +9:
      if (Piece(*move) == bishop) return(0);
      if (Last_One(And(mask_minus9dir[from],occupied))
          != king_square) return(0);
      target=And(mask_plus9dir[from],from_attacks);
      break;
    case -1:
      if (Piece(*move) == rook) return(0);
      if (First_One(And(mask_plus1dir[from],occupied))
          != king_square) return(0);
      target=And(mask_minus1dir[from],from_attacks);
      break;
    case -7:
      if (Piece(*move) == bishop) return(0);
      if (First_One(And(mask_plus7dir[from],occupied))
          != king_square) return(0);
      target=And(mask_minus7dir[from],from_attacks);
      break;
    case -8:
      if (Piece(*move) == rook) return(0);
      if (First_One(And(mask_plus8dir[from],occupied))
          != king_square) return(0);
      target=And(mask_minus8dir[from],from_attacks);
      break;
    case -9:
      if (Piece(*move) == bishop) return(0);
      if (First_One(And(mask_plus9dir[from],occupied))
          != king_square) return(0);
      target=And(mask_minus9dir[from],from_attacks);
      break;
    default:
      return(0);
  }
  if (target) 
    return(1);
  else
    return(0);
}
