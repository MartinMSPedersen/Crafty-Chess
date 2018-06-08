#include <stdio.h>
#include <stdlib.h>
#include "chess.h"
#include "data.h"

/* last modified 08/23/96 */
/*
********************************************************************************
*                                                                              *
*   AttacksFrom() is used to produce a BITBOARD which is a map of all squares  *
*   attacked from this <square>.  this procedure uses the rotated bitboard     *
*   technique to compute the attack maps for sliding pieces.                   *
*                                                                              *
********************************************************************************
*/
BITBOARD AttacksFrom(int square, int wtm) {
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

/* last modified 08/23/96 */
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
BITBOARD AttacksTo(int square) {
  register BITBOARD attacks;
/*
 ----------------------------------------------------------
|                                                          |
|  start with the pawn attacks by checking in both         |
|  directions to see if a pawn on <square> would attack    |
|  a pawn.                                                 |
|                                                          |
 ----------------------------------------------------------
*/
  attacks=And(w_pawn_attacks[square],BlackPawns);
  attacks=Or(attacks,And(b_pawn_attacks[square],WhitePawns));
/*
 ----------------------------------------------------------
|                                                          |
|  now the knights.  same drill as above.                  |
|                                                          |
 ----------------------------------------------------------
*/
  attacks=Or(attacks,And(knight_attacks[square],Or(BlackKnights,
                                                   WhiteKnights)));
/*
 ----------------------------------------------------------
|                                                          |
|  now the bishops and queens.  we generate the diagonal   |
|  attacks from <square> then see if the blocking piece    |
|  is a bishop or queen for either side.  if so, we add    |
|  in that attack.                                         |
|                                                          |
 ----------------------------------------------------------
*/
  attacks=Or(attacks,And(AttacksBishop(square),BishopsQueens));
/*
 ----------------------------------------------------------
|                                                          |
|  now the rooks and queens.  just like bishops and        |
|  queens, but along ranks and files.                      |
|                                                          |
 ----------------------------------------------------------
*/
  attacks=Or(attacks,And(AttacksRook(square),RooksQueens));
/*
 ----------------------------------------------------------
|                                                          |
|  now the kings.  just like pawns and knights.            |
|                                                          |
 ----------------------------------------------------------
*/
  attacks=Or(attacks,And(king_attacks[square],Or(BlackKing,
                                                 WhiteKing)));

  return(attacks);
}

/* last modified 08/23/96 */
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

int Attacked(int square, int wtm) {
  register BITBOARD attacks;

  if (wtm) {
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
    attacks=And(b_pawn_attacks[square],WhitePawns);
    if (attacks) return(1);
    attacks=And(knight_attacks[square],WhiteKnights);
    if(attacks) return(1);
    attacks=And(And(AttacksBishop(square),BishopsQueens),WhitePieces);
    if(attacks) return(1);
    attacks=And(And(AttacksRook(square),RooksQueens),WhitePieces);
    if(attacks) return(1);
    attacks=And(king_attacks[square],WhiteKing);
    if(attacks) return(1);
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
    attacks= And(w_pawn_attacks[square],BlackPawns);
    if (attacks) return(1);
    attacks=And(knight_attacks[square],BlackKnights);
    if(attacks) return(1);
    attacks=And(And(AttacksBishop(square),BishopsQueens),BlackPieces);
    if (attacks) return(1);
    attacks=And(And(AttacksRook(square),RooksQueens),BlackPieces);
    if(attacks) return(1);
    attacks=And(king_attacks[square],BlackKing);
    if(attacks) return(1);
    return(0);
  }
}
