#include "chess.h"
#include "data.h"
/* last modified 02/18/08 */
/*
 *******************************************************************************
 *                                                                             *
 *   Swap() is used to analyze capture moves to see whether or not they appear *
 *   to be profitable.  the basic algorithm is extremely fast since it uses the*
 *   bitmaps to determine which squares are attacking the [target] square.     *
 *                                                                             *
 *   the algorithm is quite simple.  using the attack bitmaps, we enumerate all*
 *   the pieces that are attacking [target] for either side.  then we simply   *
 *   use the lowest piece (value) for the correct side to capture on [target]. *
 *   we continually "flip" sides taking the lowest piece each time.            *
 *                                                                             *
 *   as a piece is used, if it is a sliding piece (pawn, bishop, rook or queen)*
 *   we remove the piece, then generate moves of bishop/queen or rook/queen    *
 *   and then add those in to the attackers, removing any attacks that have    *
 *   already been used.                                                        *
 *                                                                             *
 *******************************************************************************
 */
int Swap(TREE * RESTRICT tree, int source, int target, int wtm)
{
  register BITBOARD attacks, temp = 0, toccupied;
  register int attacked_piece;
  register int piece;
  register int color, nc = 1;
  int swap_list[32];

/*
 ************************************************************
 *                                                          *
 *   determine which squares attack <target> for each side. *
 *   initialize by placing the piece on <target> first in   *
 *   the list as it is being captured to start things off.  *
 *                                                          *
 ************************************************************
 */
  toccupied = OccupiedSquares;
  attacks = AttacksTo(tree, target);
  attacked_piece = p_values[PcOnSq(target) + 6];
/*
 ************************************************************
 *                                                          *
 *   the first piece to capture on <target> is the piece    *
 *   standing on <source>.                                  *
 *                                                          *
 ************************************************************
 */
  color = Flip(wtm);
  swap_list[0] = attacked_piece;
  piece = Abs(PcOnSq(source));
  attacked_piece = pc_values[piece];
  Clear(source, toccupied);
  if (piece != knight && piece != king) {
    if (piece & 1)
      attacks |= AttacksBishop(target, toccupied) & BishopsQueens;
    if (piece == pawn || piece & 4)
      attacks |= AttacksRook(target, toccupied) & RooksQueens;
  }
  attacks &= toccupied;
/*
 ************************************************************
 *                                                          *
 *   now pick out the least valuable piece for the correct  *
 *   side that is bearing on <target>.  as we find one, we  *
 *   update the attacks (if this is a sliding piece) to get *
 *   the attacks for any sliding piece that is lined up     *
 *   behind the attacker we are removing.                   *
 *                                                          *
 *   once we know there is a piece attacking the last       *
 *   capturing piece.  add it to the swap list and repeat   *
 *   until one side has no more captures.                   *
 *                                                          *
 ************************************************************
 */
  while (attacks) {
    for (piece = pawn; piece <= king; piece++)
      if ((temp = Pieces(color, piece) & attacks))
        break;
    if (piece > king)
      break;
    toccupied ^= (temp & -temp);
    if (piece != knight && piece != king) {
      if (piece & 1)
        attacks |= AttacksBishop(target, toccupied) & BishopsQueens;
      if (piece & 4)
        attacks |= AttacksRook(target, toccupied) & RooksQueens;
    }
    attacks &= toccupied;
    swap_list[nc] = -swap_list[nc - 1] + attacked_piece;
    attacked_piece = pc_values[piece];
    nc++;
    color = Flip(color);
  }
/*
 ************************************************************
 *                                                          *
 *   starting at the end of the sequence of values, use a   *
 *   "minimax" like procedure to decide where the captures  *
 *   will stop.                                             *
 *                                                          *
 ************************************************************
 */
  while (--nc)
    swap_list[nc - 1] = -Max(-swap_list[nc - 1], swap_list[nc]);
  return (swap_list[0]);
}
