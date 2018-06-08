#include "chess.h"
#include "data.h"

/* last modified 03/06/08 */
/*
 *******************************************************************************
 *                                                                             *
 *   UnmakeMove() is responsible for updating the position database whenever a *
 *   move is retracted.  it is the exact inverse of MakeMove(). the hash       *
 *   signature(s) are not updated, they are just restored to their status that *
 *   was saved before the move was made, to save time.                         *
 *                                                                             *
 *******************************************************************************
 */
void UnmakeMove(TREE * RESTRICT tree, int ply, int move, int wtm)
{
  register int piece, from, to, captured, promote, btm = Flip(wtm);

#if defined(DEBUG)
  register int i;
#endif
  BITBOARD bit_move;

/*
 ************************************************************
 *                                                          *
 *   first, restore the hash signatures to their state      *
 *   prior to this move being made, and remove the current  *
 *   position from the repetition list.                     *
 *                                                          *
 ************************************************************
 */
  HashKey = tree->save_hash_key[ply];
  PawnHashKey = tree->save_pawn_hash_key[ply];
  Repetition(wtm)--;
/*
 ************************************************************
 *                                                          *
 *   now do the things that are common to all pieces, such  *
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
 *   now do the piece-specific things by calling the        *
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
        break;
      case bishop:
        Clear(to, BishopsQueens);
        break;
      case rook:
        Clear(to, RooksQueens);
        break;
      case queen:
        Clear(to, BishopsQueens);
        Clear(to, RooksQueens);
        break;
      }
    }
    break;
  case knight:
    break;
  case bishop:
    ClearSet(bit_move, BishopsQueens);
    break;
  case rook:
    ClearSet(bit_move, RooksQueens);
    break;
  case queen:
    ClearSet(bit_move, BishopsQueens);
    ClearSet(bit_move, RooksQueens);
    break;
  case king:
    KingSQ(wtm) = from;
    if (abs(to - from) == 2) {
      if (to == rook_G[wtm]) {
        from = rook_H[wtm];
        to = rook_F[wtm];
      } else {
        from = rook_A[wtm];
        to = rook_D[wtm];
      }
      bit_move = SetMask(from) | SetMask(to);
      ClearSet(bit_move, RooksQueens);
      ClearSet(bit_move, Rooks(wtm));
      ClearSet(bit_move, Occupied(wtm));
      PcOnSq(to) = 0;
      PcOnSq(from) = pieces[wtm][rook];
    }
    break;
  }
/*
 *******************************************************************************
 *                                                                             *
 *   now it is time to restore a piece that was captured.                      *
 *                                                                             *
 *******************************************************************************
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
      break;
    case bishop:
      Set(to, BishopsQueens);
      break;
    case rook:
      Set(to, RooksQueens);
      break;
    case queen:
      Set(to, BishopsQueens);
      Set(to, RooksQueens);
      break;
    case king:
#if defined(DEBUG)
      Print(128, "captured a king (Unmake)\n");
      for (i = 1; i <= ply; i++)
        Print(128, "ply=%2d, piece=%2d,from=%2d,to=%2d,captured=%2d\n", i,
            Piece(tree->curmv[i]), From(tree->curmv[i]), To(tree->curmv[i]),
            Captured(tree->curmv[i]));
      Print(128, "ply=%2d, piece=%2d,from=%2d,to=%2d,captured=%2d\n", i, piece,
          from, to, captured);
      if (log_file)
        DisplayChessBoard(log_file, tree->pos);
#endif
      break;
    }
  }
#if defined(DEBUG)
  ValidatePosition(tree, ply, move, "UnmakeMove(1)");
#endif
  return;
}
