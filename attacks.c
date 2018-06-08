#include <stdio.h>
#include <stdlib.h>
#include "types.h"
#include "function.h"
#include "data.h"

/*
********************************************************************************
*                                                                              *
*   Attack() is used to determine if a newly promoted pawn (queen)             *
*   attacks <square>.  normally <square> will be the location of the opposing  *
*   king, but it can also be the location of the opposing side's queening      *
*   square in case this pawn prevents the other pawn from safely queening on   *
*   the next move.                                                             *
*                                                                              *
********************************************************************************
*/
int Attack(int square, int queen, int ply)
{
  register BITBOARD occupied_squares;

  occupied_squares=Or(WhitePieces(ply),
                      BlackPieces(ply));
/*
 ----------------------------------------------------------
|                                                          |
|  is the queen on the same rank/file/diagonal as <square> |
|  which it must be to attack it.   if so, and the         |
|  intervening squares are empty, then the attack con-     |
|  dition is true.                                         |
|                                                          |
 ----------------------------------------------------------
*/
  if (And(obstructed[square][queen],occupied_squares)) return(0);
  return(1);
}

/*
********************************************************************************
*                                                                              *
*   AttacksFrom() is used to produce a BITBOARD which is a map of all squares  *
*   attacked from this <square>.  this procedure uses the rotated bitboard     *
*   technique to compute the attack maps for sliding pieces.                   *
*                                                                              *
********************************************************************************
*/
BITBOARD AttacksFrom(int square, int ply, int wtm)
{

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
  switch (abs(PieceOnSquare(ply,square))) {
  case pawn:
    if (wtm)
      return(w_pawn_attacks[square]);
    else
      return(b_pawn_attacks[square]);
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
BITBOARD AttacksTo(int square, int ply)
{
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
  attacks=And(w_pawn_attacks[square],BlackPawns(ply));
  attacks=Or(attacks,And(b_pawn_attacks[square],WhitePawns(ply)));
/*
 ----------------------------------------------------------
|                                                          |
|  now the knights.  same drill as above.                  |
|                                                          |
 ----------------------------------------------------------
*/
  attacks=Or(attacks,
             And(knight_attacks[square],Or(BlackKnights(ply),
                                           WhiteKnights(ply))));
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
  attacks=Or(attacks,And(AttacksBishop(square),
                         BishopsQueens(ply)));
/*
 ----------------------------------------------------------
|                                                          |
|  now the rooks and queens.  just like bishops and        |
|  queens, but along ranks and files.                      |
|                                                          |
 ----------------------------------------------------------
*/
  attacks=Or(attacks,And(AttacksRook(square),
                         RooksQueens(ply)));
/*
 ----------------------------------------------------------
|                                                          |
|  now the kings.  just like pawns and knights.            |
|                                                          |
 ----------------------------------------------------------
*/
  attacks=Or(attacks,And(king_attacks[square],Or(BlackKing(ply),
                                                 WhiteKing(ply))));

  return(attacks);
}

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
int Attacked(int square, int ply, int wtm)
{
/*
 ----------------------------------------------------------
|                                                          |
|  start with the pawn attacks by checking in both         |
|  directions to see if a pawn on <square> would attack    |
|  a pawn.                                                 |
|                                                          |
 ----------------------------------------------------------
*/
  if (wtm) {
    if(And(b_pawn_attacks[square],WhitePawns(ply))) return(1);
  }
  else {
    if(And(w_pawn_attacks[square],BlackPawns(ply))) return(1);
  }
/*
 ----------------------------------------------------------
|                                                          |
|  now the knights.  same drill as above.                  |
|                                                          |
 ----------------------------------------------------------
*/
  if (wtm) {
    if(And(knight_attacks[square],WhiteKnights(ply))) return(1);
  }
  else {
    if(And(knight_attacks[square],BlackKnights(ply))) return(1);
  }
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
  if (wtm) {
    if(And(And(AttacksBishop(square),BishopsQueens(ply)),
           WhitePieces(ply))) return(1);
  }
  else {
    if(And(And(AttacksBishop(square),BishopsQueens(ply)),
           BlackPieces(ply))) return(1);
  }
/*
 ----------------------------------------------------------
|                                                          |
|  now the rooks and queens.  just like bishops and        |
|  queens, but along ranks and files.                      |
|                                                          |
 ----------------------------------------------------------
*/
  if (wtm) {
    if(And(And(AttacksRook(square),RooksQueens(ply)),
           WhitePieces(ply))) return(1);
  }
  else {
    if(And(And(AttacksRook(square),RooksQueens(ply)),
           BlackPieces(ply))) return(1);
  }
/*
 ----------------------------------------------------------
|                                                          |
|  now the kings.  just like pawns and knights.            |
|                                                          |
 ----------------------------------------------------------
*/
  if (wtm) {
    if(And(king_attacks[square],WhiteKing(ply))) return(1);
  }
  else {
    if(And(king_attacks[square],BlackKing(ply))) return(1);
  }

  return(0);
}
