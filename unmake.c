#include "chess.h"
#include "data.h"
/* last modified 09/23/09 */
/*
 *******************************************************************************
 *                                                                             *
 *   UnmakeMove() is responsible for updating the position database whenever a *
 *   move is retracted.  It is the exact inverse of MakeMove(). The hash       *
 *   signature(s) are not updated, they are just restored to their status that *
 *   was saved before the move was made, to save time.                         *
 *                                                                             *
 *******************************************************************************
 */
void UnmakeMove(TREE * RESTRICT tree, int ply, int move, int wtm) {
  BITBOARD bit_move;
  int piece, from, to, captured, promote, btm = Flip(wtm);

/*
 ************************************************************
 *                                                          *
 *   First, restore the hash signatures to their state      *
 *   prior to this move being made, and remove the current  *
 *   position from the repetition list.                     *
 *                                                          *
 ************************************************************
 */
  HashKey = tree->save_hash_key[ply];
  PawnHashKey = tree->save_pawn_hash_key[ply];
/*
 ************************************************************
 *                                                          *
 *   Now do the things that are common to all pieces, such  *
 *   as updating the bitboards and hash signature.          *
 *                                                          *
 ************************************************************
 */
  piece = Piece(move);
  from = From(move);
  to = To(move);
  captured = Captured(move);
  promote = Promote(move);
  bit_move = SetMask(from) | SetMask(to);
  ClearSet(bit_move, Pieces(wtm, piece));
  ClearSet(bit_move, Occupied(wtm));
  PcOnSq(to) = 0;
  PcOnSq(from) = pieces[wtm][piece];
/*
 ************************************************************
 *                                                          *
 *   Now do the piece-specific things by jumping to the     *
 *   appropriate routine.                                   *
 *                                                          *
 ************************************************************
 */
  switch (piece) {
    case pawn:
      if (captured == 1) {
        if (EnPassant(ply) == to) {
          TotalAllPieces++;
          Set(to + epsq[wtm], Pawns(btm));
          Set(to + epsq[wtm], Occupied(btm));
          PcOnSq(to + epsq[wtm]) = pieces[btm][pawn];
          Material -= PieceValues(wtm, pawn);
          TotalPieces(btm, pawn)++;
          captured = 0;
        }
      }
      if (promote) {
        TotalPieces(wtm, pawn)++;
        Clear(to, Pawns(wtm));
        Clear(to, Occupied(wtm));
        Clear(to, Pieces(wtm, promote));
        Material -= PieceValues(wtm, promote);
        Material += PieceValues(wtm, pawn);
        TotalPieces(wtm, occupied) -= p_vals[promote];
        TotalPieces(wtm, promote)--;
        switch (promote) {
          case knight:
            tree->pos.minors[wtm]--;
            break;
          case bishop:
            tree->pos.minors[wtm]--;
            break;
          case rook:
            tree->pos.majors[wtm]--;
            break;
          case queen:
            tree->pos.majors[wtm] -= 2;
            break;
        }
      }
      break;
    case knight:
      break;
    case bishop:
      break;
    case rook:
      break;
    case queen:
      break;
    case king:
      KingSQ(wtm) = from;
      if (Abs(to - from) == 2) {
        if (to == rook_G[wtm]) {
          from = rook_H[wtm];
          to = rook_F[wtm];
        } else {
          from = rook_A[wtm];
          to = rook_D[wtm];
        }
        bit_move = SetMask(from) | SetMask(to);
        ClearSet(bit_move, Rooks(wtm));
        ClearSet(bit_move, Occupied(wtm));
        PcOnSq(to) = 0;
        PcOnSq(from) = pieces[wtm][rook];
      }
      break;
  }
/*
 ************************************************************
 *                                                          *
 *   Next we restore information related to a piece that    *
 *   was captured and is now being returned to the board.   *
 *                                                          *
 ************************************************************
 */
  if (captured) {
    TotalAllPieces++;
    Set(to, Pieces(btm, captured));
    Set(to, Occupied(btm));
    Material += PieceValues(btm, captured);
    PcOnSq(to) = pieces[btm][captured];
    TotalPieces(btm, captured)++;
    if (captured != pawn)
      TotalPieces(btm, occupied) += p_vals[captured];
    switch (captured) {
      case pawn:
        break;
      case knight:
        tree->pos.minors[btm]++;
        break;
      case bishop:
        tree->pos.minors[btm]++;
        break;
      case rook:
        tree->pos.majors[btm]++;
        break;
      case queen:
        tree->pos.majors[btm] += 2;
        break;
      case king:
        break;
    }
  }
#if defined(DEBUG)
  ValidatePosition(tree, ply, move, "UnmakeMove(1)");
#endif
  return;
}
