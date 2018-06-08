#include "chess.h"
#include "data.h"

/* last modified 02/20/08 */
/*
 *******************************************************************************
 *                                                                             *
 *   MakeMove() is responsible for updating the position database whenever a   *
 *   piece is moved.  it performs the following operations:  (1) update the    *
 *   board structure itself by moving the piece and removing any captured      *
 *   piece.  (2) update the hash keys.  (3) update material counts.  (4) update*
 *   castling status.  (5) update number of moves since last reversible move.  *
 *                                                                             *
 *   note:  wtm = 1 if white is to move, 0 otherwise.  btm is the opposite and *
 *   is 1 if it is not white to move, 0 otherwise.                             *
 *                                                                             *
 *******************************************************************************
 */
void MakeMove(TREE * RESTRICT tree, int ply, int move, int wtm)
{
  register int piece, from, to, captured, promote, btm = Flip(wtm);
  register int cpiece;

#if defined(DEBUG)
  register int i;
#endif
  BITBOARD bit_move;

/*
 ************************************************************
 *                                                          *
 *   first, some basic information is updated for all moves *
 *   before we do the piece-specific stuff.  we need to     *
 *   save the current position and both hash signatures,    *
 *   and add the current position to the repetition-list    *
 *   for the side on move, before the move is actually made *
 *   on the board.  we also update the 50 move rule         *
 *   counter, which will be reset if a capture or pawn move *
 *   is made here.                                          *
 *                                                          *
 *   if the en passant flag was set the previous ply, we    *
 *   have already used it to generate moves at this ply,    *
 *   and we need to clear it before continuing.  if it is   *
 *   set, we also need to update the hash signature since   *
 *   the EP opportunity no longer exists after making any   *
 *   move at this ply (one ply deeper than when a pawn was  *
 *   advanced two squares).                                 *
 *                                                          *
 ************************************************************
 */
#if defined(DEBUG)
  ValidatePosition(tree, ply, move, "MakeMove(1)");
#endif
  tree->rep_list[wtm][tree->rep_index[wtm]++] = HashKey;
  tree->position[ply + 1] = tree->position[ply];
  tree->save_hash_key[ply] = HashKey;
  tree->save_pawn_hash_key[ply] = PawnHashKey;
  if (EnPassant(ply + 1)) {
    HashEP(EnPassant(ply + 1), HashKey);
    EnPassant(ply + 1) = 0;
  }
  Rule50Moves(ply + 1)++;
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
  cpiece = PcOnSq(to);
  ClearSet(bit_move, Pieces(wtm, piece));
  ClearSet(bit_move, Occupied(wtm));
  Hash(wtm, piece, from);
  Hash(wtm, piece, to);
  PcOnSq(from) = 0;
  PcOnSq(to) = pieces[wtm][piece];
/*
 ************************************************************
 *                                                          *
 *   now do the piece-specific things by calling the        *
 *   appropriate routine.                                   *
 *                                                          *
 ************************************************************
 */
  switch (piece) {
/*
 *******************************************************************************
 *                                                                             *
 *   make pawn moves.  there are two special cases:  (a) en passant captures   *
 *   where the captured pawn is not on the "to" square and must be removed in  *
 *   a different way, and (2) pawn promotions (where the "Promote" variable    *
 *   is non-zero) requires updating the appropriate bit boards since we are    *
 *   creating a new piece.                                                     *
 *                                                                             *
 *******************************************************************************
 */
  case pawn:
    HashP(wtm, from);
    HashP(wtm, to);
    if (captured == 1 && !cpiece) {
      Clear(to + epsq[wtm], Pawns(btm));
      Clear(to + epsq[wtm], Occupied(btm));
      Hash(btm, pawn, to + epsq[wtm]);
      HashP(btm, to + epsq[wtm]);
      PcOnSq(to + epsq[wtm]) = 0;
      Material -= PieceValues(btm, pawn);
      TotalPawns(btm)--;
      TotalAllPieces--;
      captured = 0;
    }
/*
 **********************************************************************
 *                                                                    *
 *  if this is a pawn promotion, remove the pawn from the counts      *
 *  then update the correct piece board to reflect the piece just     *
 *  created.                                                          *
 *                                                                    *
 **********************************************************************
 */
    if (promote) {
      TotalPawns(wtm)--;
      Material -= PieceValues(wtm, pawn);
      Clear(to, Pawns(wtm));
      Hash(wtm, pawn, to);
      HashP(wtm, to);
      Hash(wtm, promote, to);
      PcOnSq(to) = pieces[wtm][promote];
      switch (promote) {
      case knight:
        Set(to, Knights(wtm));
        TotalPieces(wtm) += knight_v;
        TotalKnights(wtm)++;
        Material += PieceValues(wtm, knight);
        break;
      case bishop:
        Set(to, Bishops(wtm));
        Set(to, BishopsQueens);
        TotalPieces(wtm) += bishop_v;
        TotalBishops(wtm)++;
        Material += PieceValues(wtm, bishop);
        break;
      case rook:
        Set(to, Rooks(wtm));
        Set(to, RooksQueens);
        TotalPieces(wtm) += rook_v;
        TotalRooks(wtm)++;
        Material += PieceValues(wtm, rook);
        break;
      case queen:
        Set(to, Queens(wtm));
        Set(to, BishopsQueens);
        Set(to, RooksQueens);
        TotalPieces(wtm) += queen_v;
        TotalQueens(wtm)++;
        Material += PieceValues(wtm, queen);
        break;
      }
    } else if ((Abs(to - from) == 16) && (mask_eptest[to] & Pawns(btm))) {
      EnPassant(ply + 1) = to + epsq[wtm];
      HashEP(to + epsq[wtm], HashKey);
    }
    Rule50Moves(ply + 1) = 0;
    break;
/*
 *******************************************************************************
 *                                                                             *
 *   make knight moves.                                                        *
 *                                                                             *
 *******************************************************************************
 */
  case knight:
    break;
/*
 *******************************************************************************
 *                                                                             *
 *   make bishop moves.                                                        *
 *                                                                             *
 *******************************************************************************
 */
  case bishop:
    Clear(from, BishopsQueens);
    Set(to, BishopsQueens);
    break;
/*
 *******************************************************************************
 *                                                                             *
 *   make rook moves.  the only special case handling required is to determine *
 *   if x_castle is non-zero [x=w or b based on side to move].  if it is non-  *
 *   zero, the value must be corrected if either rook is moving from its       *
 *   original square, so that castling with that rook becomes impossible.      *
 *                                                                             *
 *******************************************************************************
 */
  case rook:
    Clear(from, RooksQueens);
    Set(to, RooksQueens);
    if (Castle(ply + 1, wtm) > 0) {
      if ((from == rook_A[wtm]) && (Castle(ply + 1, wtm) & 2)) {
        Castle(ply + 1, wtm) &= 1;
        HashCastle(1, HashKey, wtm);
      } else if ((from == rook_H[wtm]) && (Castle(ply + 1, wtm) & 1)) {
        Castle(ply + 1, wtm) &= 2;
        HashCastle(0, HashKey, wtm);
      }
    }
    break;
/*
 *******************************************************************************
 *                                                                             *
 *   make queen moves                                                          *
 *                                                                             *
 *******************************************************************************
 */
  case queen:
    Clear(from, BishopsQueens);
    Set(to, BishopsQueens);
    Clear(from, RooksQueens);
    Set(to, RooksQueens);
    break;
/*
 *******************************************************************************
 *                                                                             *
 *   make king moves.  the only special case is castling, which is indicated   *
 *   by from=E1, to=G1 for o-o as an example.  the king is moving from e1-g1   *
 *   which is normally illegal.  in this case, the correct rook is also moved. *
 *                                                                             *
 *   note that moving the king in any direction resets the castle status flag  *
 *   indicating that castling is not possible in any position below this point *
 *   in the tree.                                                              *
 *                                                                             *
 *******************************************************************************
 */
  case king:
    KingSQ(wtm) = to;
    if (Castle(ply + 1, wtm) > 0) {
      if (Castle(ply + 1, wtm) & 2)
        HashCastle(1, HashKey, wtm);
      if (Castle(ply + 1, wtm) & 1)
        HashCastle(0, HashKey, wtm);
      if (abs(to - from) == 2) {
        Castle(ply + 1, wtm) = -ply;
        piece = rook;
        if (to == rook_G[wtm]) {
          from = rook_H[wtm];
          to = rook_F[wtm];
        } else {
          from = rook_A[wtm];
          to = rook_D[wtm];
        }
        Clear(from, RooksQueens);
        Set(to, RooksQueens);
        bit_move = SetMask(from) | SetMask(to);
        ClearSet(bit_move, Rooks(wtm));
        ClearSet(bit_move, Occupied(wtm));
        Hash(wtm, rook, from);
        Hash(wtm, rook, to);
        PcOnSq(from) = 0;
        PcOnSq(to) = pieces[wtm][rook];
      } else
        Castle(ply + 1, wtm) = 0;
    }
    break;
  }
/*
 *******************************************************************************
 *                                                                             *
 *   now it is time to "gracefully" remove a piece from the game board since it*
 *   is being captured.  this includes updating the board structure.           *
 *                                                                             *
 *******************************************************************************
 */
  if (captured) {
    Rule50Moves(ply + 1) = 0;
    TotalAllPieces--;
    if (promote)
      piece = promote;
    Hash(btm, captured, to);
    Clear(to, Pieces(btm, captured));
    Clear(to, Occupied(btm));
    switch (captured) {
/*
 ************************************************************
 *                                                          *
 *   remove a captured pawn.                                *
 *                                                          *
 ************************************************************
 */
    case pawn:
      HashP(btm, to);
      Material -= PieceValues(btm, pawn);
      TotalPawns(btm)--;
      break;
/*
 ************************************************************
 *                                                          *
 *   remove a captured knight.                              *
 *                                                          *
 ************************************************************
 */
    case knight:
      TotalPieces(btm) -= knight_v;
      TotalKnights(btm)--;
      Material -= PieceValues(btm, knight);
      break;
/*
 ************************************************************
 *                                                          *
 *   remove a captured bishop.                              *
 *                                                          *
 ************************************************************
 */
    case bishop:
      if (piece != bishop && piece != queen)
        Clear(to, BishopsQueens);
      TotalPieces(btm) -= bishop_v;
      TotalBishops(btm)--;
      Material -= PieceValues(btm, bishop);
      break;
/*
 ************************************************************
 *                                                          *
 *   remove a captured rook.                                *
 *                                                          *
 ************************************************************
 */
    case rook:
      if (piece != rook && piece != queen)
        Clear(to, RooksQueens);
      if (Castle(ply + 1, btm) > 0) {
        if ((to == rook_A[btm]) && (Castle(ply + 1, btm) & 2)) {
          Castle(ply + 1, btm) &= 1;
          HashCastle(1, HashKey, btm);
        } else if ((to == rook_H[btm]) && (Castle(ply + 1, btm) & 1)) {
          Castle(ply + 1, btm) &= 2;
          HashCastle(0, HashKey, btm);
        }
      }
      TotalPieces(btm) -= rook_v;
      TotalRooks(btm)--;
      Material -= PieceValues(btm, rook);
      break;
/*
 ************************************************************
 *                                                          *
 *   remove a captured queen.                               *
 *                                                          *
 ************************************************************
 */
    case queen:
      if (piece != bishop && piece != queen)
        Clear(to, BishopsQueens);
      if (piece != rook && piece != queen)
        Clear(to, RooksQueens);
      TotalPieces(btm) -= queen_v;
      TotalQueens(btm)--;
      Material -= PieceValues(btm, queen);
      break;
/*
 ************************************************************
 *                                                          *
 *   remove a captured king. [this is an error condition]   *
 *                                                          *
 ************************************************************
 */
    case king:
#if defined(DEBUG)
      Print(128, "captured a king (Make)\n");
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
  ValidatePosition(tree, ply + 1, move, "MakeMove(2)");
#endif
  return;
}

/*
 *******************************************************************************
 *                                                                             *
 *   MakeMoveRoot() is used to make a move at the root of the game tree,       *
 *   before any searching is done.  it uses MakeMove() to execute the move,    *
 *   but then copies the resulting position back to position[0], the actual    *
 *   board position.  it handles the special-case of the draw-by-repetition    *
 *   rule by maintaining a list of previous positions, which is reset each time*
 *   a non-reversible (pawn move or capture move) is made.                     *
 *                                                                             *
 *******************************************************************************
 */
void MakeMoveRoot(TREE * RESTRICT tree, int move, int wtm)
{
  int i;

/*
 ************************************************************
 *                                                          *
 *   first, make the move and replace position[0] with the  *
 *   new position.                                          *
 *                                                          *
 ************************************************************
 */
  MakeMove(tree, 0, move, wtm);
/*
 ************************************************************
 *                                                          *
 *   now, if this is a non-reversible move, reset the       *
 *   repetition list pointer to start the count over.       *
 *                                                          *
 ************************************************************
 */
  if (Rule50Moves(1) == 0)
    for (i = 0; i < 2; i++)
      tree->rep_index[i] = 0;
  Castle(1, black) = Max(0, Castle(1, black));
  Castle(1, white) = Max(0, Castle(1, white));
  tree->position[0] = tree->position[1];
}
