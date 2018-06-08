#include "chess.h"
#include "data.h"

/* last modified 06/26/99 */
/*
 *******************************************************************************
 *                                                                             *
 *   AttacksTo() is used to produce a BITBOARD which is a map of all squares   *
 *   that directly attack this <square>.  the non-sliding pieces are trivial   *
 *   to detect, but for sliding pieces, we use a bitboard trick.  the idea is  *
 *   to compute the squares a queen would attack, if it was standing on        *
 *   <square> and then look at the last square attacked in each direction to   *
 *   determine if it is a sliding piece that moves in the right direction.  to *
 *   finish up, we simply need to Or() all these attackers together.           *
 *                                                                             *
 *******************************************************************************
 */
BITBOARD AttacksTo(TREE * RESTRICT tree, int square)
{

  return ((pawn_attacks[white][square] & Pawns(black)) |
      (pawn_attacks[black][square]
          & Pawns(white)) | (knight_attacks[square] & (Knights(black) |
              Knights(white)))
      | (AttacksBishop(square,
              OccupiedSquares) & BishopsQueens) | (AttacksRook(square,
              OccupiedSquares) & RooksQueens) | (king_attacks[square] &
          (Kings(black) | Kings(white))));
}

/* last modified 06/26/99 */
/*
 *******************************************************************************
 *                                                                             *
 *   Attacked() is used to determine if <square> is attacked by "wtm".  the    *
 *   algorithm is simple, and is based on the AttacksTo() algorithm, but,      *
 *   rather than returning a bitmap of squares attacking <square> it returns a *
 *   "1" as soon as it finds anything that attacks <square>.                   *
 *                                                                             *
 *******************************************************************************
 */

int Attacked(TREE * RESTRICT tree, int square, int wtm)
{
  if (pawn_attacks[Flip(wtm)][square] & Pawns(wtm))
    return (1);
  if (knight_attacks[square] & Knights(wtm))
    return (1);
  if (AttacksBishop(square, OccupiedSquares) & BishopsQueens & Occupied(wtm))
    return (1);
  if (AttacksRook(square, OccupiedSquares) & RooksQueens & Occupied(wtm))
    return (1);
  if (king_attacks[square] & Kings(wtm))
    return (1);
  return (0);
}
