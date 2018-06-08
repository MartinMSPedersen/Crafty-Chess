#include <stdio.h>
#include <stdlib.h>
#include "chess.h"
#include "data.h"

/* last modified 03/12/98 */
/*
********************************************************************************
*                                                                              *
*   AttacksFrom() is used to produce a BITBOARD which is a map of all squares  *
*   attacked from this <square>.  this procedure uses the rotated bitboard     *
*   technique to compute the attack maps for sliding pieces.                   *
*                                                                              *
********************************************************************************
*/
BITBOARD AttacksFrom(TREE *tree, int square, int wtm) {
/*
 ----------------------------------------------------------
|                                                          |
|  determine the type of piece on <square>.  if it's not a |
|  sliding piece, simply return the normal attack bitmap.  |
|  otherwise, use the rotated bitboards to compute the     |
|  attack bitmap..                                         |
|                                                          |
 ----------------------------------------------------------
*/
  switch (abs(PieceOnSquare(square))) {
  case pawn:
    if (wtm) return(w_pawn_attacks[square]);
    else return(b_pawn_attacks[square]);
  case knight:
    return(knight_attacks[square]);
  case bishop:
    return(AttacksBishop(square));
  case rook:
    return(AttacksRook(square));
  case queen:
    return(AttacksQueen(square));
  case king:
    return(king_attacks[square]);
  default:
    return(0);
  }
}

/* last modified 01/12/99 */
/*
********************************************************************************
*                                                                              *
*   AttacksTo() is used to produce a BITBOARD which is a map of all squares    *
*   that directly attack this <square>.  the non-sliding pieces are trivial    *
*   to detect, but for sliding pieces, we use a rotated bitboard trick.  the   *
*   idea is to compute the squares a queen would attack, if it was standing on *
*   <square> and then look at the last square attacked in each direction to    *
*   determine if it is a sliding piece that moves in the right direction.  to  *
*   finish up, we simply need to Or() all these attackers together.            *
*                                                                              *
********************************************************************************
*/
BITBOARD AttacksTo(TREE *tree, int square) {
/*
 ----------------------------------------------------------
|                                                          |
|  start with the pawn attacks by checking in both         |
|  directions to see if a pawn on <square> would attack    |
|  a pawn.  then fold in knights, bishops, rooks and then  |
|  finally kings.                                          |
|                                                          |
 ----------------------------------------------------------
*/
  return(Or(Or(Or(Or(Or(And(w_pawn_attacks[square],BlackPawns),
                        And(b_pawn_attacks[square],WhitePawns)),
    And(knight_attacks[square],Or(BlackKnights,WhiteKnights))),
    And(AttacksBishop(square),BishopsQueens)),
    And(AttacksRook(square),RooksQueens)),
    And(king_attacks[square],Or(BlackKing,WhiteKing))));
}

/* last modified 01/12/99 */
/*
********************************************************************************
*                                                                              *
*   Attacked() is used to determine if <square> is attacked by "wtm".  the     *
*   algorithm is simple, and is based on the AttacksTo() algorithm, but,       *
*   rather than returning a bitmap of squares attacking <square> it returns a  *
*   "1" as soon as it finds anything that attacks <square>.                    *
*                                                                              *
********************************************************************************
*/

int Attacked(TREE *tree, int square, int wtm) {

/*
 ----------------------------------------------------------
|                                                          |
|  start with the white attacks by checking in both        |
|  directions to see if a pawn on <square> would attack    |
|  a pawn.  ditto for knights, bishops, rooks, queens and  |
|  kings                                                   |
|                                                          |
 ----------------------------------------------------------
*/
  if (wtm) {
    if (And(b_pawn_attacks[square],WhitePawns)) return(1);
    if (And(knight_attacks[square],WhiteKnights)) return(1);
    if (And(And(AttacksBishop(square),BishopsQueens),WhitePieces)) return(1);
    if (And(And(AttacksRook(square),RooksQueens),WhitePieces)) return(1);
    if (And(king_attacks[square],WhiteKing)) return(1);
    return(0);
  }

/*
 ----------------------------------------------------------
|                                                          |
|  start with the white attacks by checking in both        |
|  directions to see if a pawn on <square> would attack    |
|  a pawn.  ditto for knights, bishops, rooks, queens and  |
|  kings                                                   |
|                                                          |
 ----------------------------------------------------------
*/
  else {
    if (And(w_pawn_attacks[square],BlackPawns)) return(1);
    if (And(knight_attacks[square],BlackKnights)) return(1);
    if (And(And(AttacksBishop(square),BishopsQueens),BlackPieces)) return(1);
    if (And(And(AttacksRook(square),RooksQueens),BlackPieces)) return(1);
    if (And(king_attacks[square],BlackKing)) return(1);
    return(0);
  }
}
