#include "chess.h"
#include "data.h"
/* last modified 09/23/09 */
/*
 *******************************************************************************
 *                                                                             *
 *   AttacksTo() is used to produce a bitboard which is a map of all squares   *
 *   that directly attack this <square>.  The non-sliding pieces are trivial   *
 *   to detect, but for sliding pieces, we use a bitboard trick.  The idea is  *
 *   to compute the squares a queen would attack, if it was standing on        *
 *   <square> and then look at the last square attacked in each direction to   *
 *   determine if it is a sliding piece that moves in the right direction.  To *
 *   finish up, we simply need to Or() all these attackers together.           *
 *                                                                             *
 *******************************************************************************
 */
uint64_t AttacksTo(TREE * RESTRICT tree, int square) {
  uint64_t attacks =
      (pawn_attacks[white][square] & Pawns(black)) |
      (pawn_attacks[black][square] & Pawns(white));
  uint64_t bsliders =
      Bishops(white) | Bishops(black) | Queens(white) | Queens(black);
  uint64_t rsliders =
      Rooks(white) | Rooks(black) | Queens(white) | Queens(black);
  attacks |= knight_attacks[square] & (Knights(black) | Knights(white));
  if (bishop_attacks[square] & bsliders)
    attacks |= AttacksBishop(square, OccupiedSquares) & bsliders;
  if (rook_attacks[square] & rsliders)
    attacks |= AttacksRook(square, OccupiedSquares) & rsliders;
  attacks |= king_attacks[square] & (Kings(black) | Kings(white));
  return (attacks);
}

/* last modified 12/02/10 */
/*
 *******************************************************************************
 *                                                                             *
 *   Attacks() is used to determine if <side> attacks <square>.  The algorithm *
 *   is simple, and is based on the AttacksTo() algorithm, but, rather than    *
 *   returning a bitmap of squares attacking <square> it returns a "1" as soon *
 *   as it finds anything that attacks <square>.                               *
 *                                                                             *
 *******************************************************************************
 */
int Attacks(TREE * RESTRICT tree, int square, int side) {
  if (pawn_attacks[Flip(side)][square] & Pawns(side))
    return (1);
  if (knight_attacks[square] & Knights(side))
    return (1);
  if (king_attacks[square] & Kings(side))
    return (1);
  if ((rook_attacks[square] & (Rooks(side) | Queens(side)))
      && (AttacksRook(square,
              OccupiedSquares) & (Rooks(side) | Queens(side))))
    return (1);
  if ((bishop_attacks[square] & (Bishops(side) | Queens(side))) &&
      (AttacksBishop(square,
              OccupiedSquares) & (Bishops(side) | Queens(side))))
    return (1);
  return (0);
}
