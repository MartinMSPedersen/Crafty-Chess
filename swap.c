#include "chess.h"
#include "data.h"
/* last modified 08/11/10 */
/*
 *******************************************************************************
 *                                                                             *
 *   Swap() is used to analyze capture moves to see whether or not they appear *
 *   to be profitable.  The basic algorithm is extremely fast since it uses the*
 *   bitmaps to determine which squares are attacking the [target] square.     *
 *                                                                             *
 *   The algorithm is quite simple.  Using the attack bitmaps, we enumerate all*
 *   the pieces that are attacking [target] for either side.  Then we simply   *
 *   use the lowest piece (value) for the correct side to capture on [target]. *
 *   we continually "flip" sides taking the lowest piece each time.            *
 *                                                                             *
 *   As a piece is used, if it is a sliding piece (pawn, bishop, rook or queen)*
 *   we remove the piece, then generate moves of bishop/queen or rook/queen    *
 *   and then add those in to the attackers, removing any attacks that have    *
 *   already been used.                                                        *
 *                                                                             *
 *******************************************************************************
 */
int Swap(TREE * RESTRICT tree, int move, int wtm) {
  BITBOARD attacks, temp = 0, toccupied = OccupiedSquares;
  BITBOARD bsliders =
      Bishops(white) | Bishops(black) | Queens(white) | Queens(black);
  BITBOARD rsliders =
      Rooks(white) | Rooks(black) | Queens(white) | Queens(black);
  int attacked_piece, piece, color, nc = 1, swap_list[32];
  int source = From(move);
  int target = To(move);

/*
 ************************************************************
 *                                                          *
 *   Determine which squares attack <target> for each side. *
 *   initialize by placing the piece on <target> first in   *
 *   the list as it is being captured to start things off.  *
 *                                                          *
 ************************************************************
 */
  attacks = AttacksTo(tree, target);
  attacked_piece = pc_values[Captured(move)];
/*
 ************************************************************
 *                                                          *
 *   The first piece to capture on <target> is the piece    *
 *   standing on <source>.                                  *
 *                                                          *
 ************************************************************
 */
  color = Flip(wtm);
  swap_list[0] = attacked_piece;
  piece = Piece(move);
  attacked_piece = pc_values[piece];
  Clear(source, toccupied);
  if (piece != knight && piece != king) {
    if (piece & 1)
      attacks |= AttacksBishop(target, toccupied) & bsliders;
    if (piece == pawn || piece & 4)
      attacks |= AttacksRook(target, toccupied) & rsliders;
  }
/*
 ************************************************************
 *                                                          *
 *   Now pick out the least valuable piece for the correct  *
 *   side that is bearing on <target>.  As we find one, we  *
 *   update the attacks (if this is a sliding piece) to get *
 *   the attacks for any sliding piece that is lined up     *
 *   behind the attacker we are removing.                   *
 *                                                          *
 *   Once we know there is a piece attacking the last       *
 *   capturing piece, add it to the swap list and repeat    *
 *   until one side has no more captures.                   *
 *                                                          *
 ************************************************************
 */
  for (attacks &= toccupied; attacks; attacks &= toccupied) {
    for (piece = pawn; piece <= king; piece++)
      if ((temp = Pieces(color, piece) & attacks))
        break;
    if (piece > king)
      break;
    toccupied ^= (temp & -temp);
    if (piece != knight && piece != king) {
      if (piece & 1)
        attacks |= AttacksBishop(target, toccupied) & bsliders;
      if (piece & 4)
        attacks |= AttacksRook(target, toccupied) & rsliders;
    }
    swap_list[nc] = -swap_list[nc - 1] + attacked_piece;
    attacked_piece = pc_values[piece];
    if (swap_list[nc++] - attacked_piece > 0)
      break;
    color = Flip(color);
  }
/*
 ************************************************************
 *                                                          *
 *   Starting at the end of the sequence of values, use a   *
 *   "minimax" like procedure to decide where the captures  *
 *   will stop.                                             *
 *                                                          *
 ************************************************************
 */
  while (--nc)
    swap_list[nc - 1] = -Max(-swap_list[nc - 1], swap_list[nc]);
  return (swap_list[0]);
}

/* last modified 08/11/10 */
/*
 *******************************************************************************
 *                                                                             *
 *   SwapO() is used to analyze a move already made to see if it appears to be *
 *   safe.  It is similar to Swap() except that the move has already been made *
 *   and we are checking to see whether the opponent can gain material by      *
 *   capturing the piece just moved.                                           *
 *                                                                             *
 *******************************************************************************
 */
int SwapO(TREE * RESTRICT tree, int move, int wtm) {
  BITBOARD attacks, temp = 0, toccupied = OccupiedSquares;
  BITBOARD bsliders =
      Bishops(white) | Bishops(black) | Queens(white) | Queens(black);
  BITBOARD rsliders =
      Rooks(white) | Rooks(black) | Queens(white) | Queens(black);
  int attacked_piece, piece, color, nc = 1, swap_list[32];
  int target = To(move);

/*
 ************************************************************
 *                                                          *
 *   Determine which squares attack <target> for each side. *
 *   initialize by placing the piece on <target> first in   *
 *   the list as it is being captured to start things off.  *
 *                                                          *
 ************************************************************
 */
  attacks = AttacksTo(tree, target);
  attacked_piece = pc_values[Piece(move)];
/*
 ************************************************************
 *                                                          *
 *   The first piece to capture on <target> is the piece    *
 *   standing on that square.  We have to find out the      *
 *   least valuable attacker for that square first.         *
 *                                                          *
 ************************************************************
 */
  color = Flip(wtm);
  swap_list[0] = attacked_piece;
  for (piece = pawn; piece <= king; piece++)
    if ((temp = Pieces(color, piece) & attacks))
      break;
  if (piece > king)
    return (0);
  toccupied ^= (temp & -temp);
  if (piece != knight && piece != king) {
    if (piece & 1)
      attacks |= AttacksBishop(target, toccupied) & bsliders;
    if (piece & 4)
      attacks |= AttacksRook(target, toccupied) & rsliders;
  }
  attacked_piece = pc_values[piece];
  color = Flip(color);
/*
 ************************************************************
 *                                                          *
 *   Now pick out the least valuable piece for the correct  *
 *   side that is bearing on <target>.  As we find one, we  *
 *   update the attacks (if this is a sliding piece) to get *
 *   the attacks for any sliding piece that is lined up     *
 *   behind the attacker we are removing.                   *
 *                                                          *
 *   Once we know there is a piece attacking the last       *
 *   capturing piece, add it to the swap list and repeat    *
 *   until one side has no more captures.                   *
 *                                                          *
 ************************************************************
 */
  for (attacks &= toccupied; attacks; attacks &= toccupied) {
    for (piece = pawn; piece <= king; piece++)
      if ((temp = Pieces(color, piece) & attacks))
        break;
    if (piece > king)
      break;
    toccupied ^= (temp & -temp);
    if (piece != knight && piece != king) {
      if (piece & 1)
        attacks |= AttacksBishop(target, toccupied) & bsliders;
      if (piece & 4)
        attacks |= AttacksRook(target, toccupied) & rsliders;
    }
    swap_list[nc] = -swap_list[nc - 1] + attacked_piece;
    attacked_piece = pc_values[piece];
    if (swap_list[nc++] - attacked_piece > 0)
      break;
    color = Flip(color);
  }
/*
 ************************************************************
 *                                                          *
 *   Starting at the end of the sequence of values, use a   *
 *   "minimax" like procedure to decide where the captures  *
 *   will stop.                                             *
 *                                                          *
 ************************************************************
 */
  while (--nc)
    swap_list[nc - 1] = -Max(-swap_list[nc - 1], swap_list[nc]);
  return (swap_list[0]);
}
