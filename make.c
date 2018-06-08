#include "chess.h"
#include "data.h"
/* last modified 11/05/10 */
/*
 *******************************************************************************
 *                                                                             *
 *   MakeMove() is responsible for updating the position database whenever a   *
 *   piece is moved.  It performs the following operations:  (1) update the    *
 *   board structure itself by moving the piece and removing any captured      *
 *   piece.  (2) update the hash keys.  (3) update material counts.  (4) update*
 *   castling status.  (5) update number of moves since last reversible move.  *
 *                                                                             *
 *   There are some special-cases handled here, such as en passant captures    *
 *   where the enemy pawn is not on the <target> square, castling which moves  *
 *   both the king and rook, and then rook moves/captures which give up the    *
 *   castling right to that side when the rook is moved.                       *
 *                                                                             *
 *   note:  wtm = 1 if white is to move, 0 otherwise.  btm is the opposite and *
 *   is 1 if it is not white to move, 0 otherwise.                             *
 *                                                                             *
 *******************************************************************************
 */
void MakeMove(TREE * RESTRICT tree, int ply, int move, int wtm) {
  BITBOARD bit_move;
  int piece, from, to, captured, promote, btm = Flip(wtm);
  int cpiece;
#if defined(DEBUG)
  int i;
#endif

/*
 ************************************************************
 *                                                          *
 *   First, some basic information is updated for all moves *
 *   before we do the piece-specific stuff.  We need to     *
 *   save the current position and both hash signatures,    *
 *   and add the current position to the repetition-list    *
 *   for the side on move, before the move is actually made *
 *   on the board.  We also update the 50 move rule         *
 *   counter, which will be reset if a capture or pawn move *
 *   is made here.                                          *
 *                                                          *
 *   If the en passant flag was set the previous ply, we    *
 *   have already used it to generate moves at this ply,    *
 *   and we need to clear it before continuing.  If it is   *
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
  tree->position[ply + 1] = tree->position[ply];
  tree->save_hash_key[ply] = HashKey;
  tree->save_pawn_hash_key[ply] = PawnHashKey;
  if (EnPassant(ply + 1)) {
    HashEP(EnPassant(ply + 1));
    EnPassant(ply + 1) = 0;
  }
  Rule50Moves(ply + 1)++;
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
 *   Now do the piece-specific things by jumping to the     *
 *   appropriate routine.                                   *
 *                                                          *
 ************************************************************
 */
  switch (piece) {
    case pawn:
      HashP(wtm, from);
      HashP(wtm, to);
      Rule50Moves(ply + 1) = 0;
      if (captured == 1 && !cpiece) {
        Clear(to + epsq[wtm], Pawns(btm));
        Clear(to + epsq[wtm], Occupied(btm));
        Hash(btm, pawn, to + epsq[wtm]);
        HashP(btm, to + epsq[wtm]);
        PcOnSq(to + epsq[wtm]) = 0;
        Material -= PieceValues(btm, pawn);
        TotalPieces(btm, pawn)--;
        TotalAllPieces--;
        captured = 0;
      }
      if (promote) {
        TotalPieces(wtm, pawn)--;
        Material -= PieceValues(wtm, pawn);
        Clear(to, Pawns(wtm));
        Hash(wtm, pawn, to);
        HashP(wtm, to);
        Hash(wtm, promote, to);
        PcOnSq(to) = pieces[wtm][promote];
        TotalPieces(wtm, occupied) += p_vals[promote];
        TotalPieces(wtm, promote)++;
        Material += PieceValues(wtm, promote);
        Set(to, Pieces(wtm, promote));
        switch (promote) {
          case knight:
            tree->pos.minors[wtm]++;
            break;
          case bishop:
            tree->pos.minors[wtm]++;
            break;
          case rook:
            tree->pos.majors[wtm]++;
            break;
          case queen:
            tree->pos.majors[wtm] += 2;
            break;
        }
      } else if ((Abs(to - from) == 16) && (mask_eptest[to] & Pawns(btm))) {
        EnPassant(ply + 1) = to + epsq[wtm];
        HashEP(to + epsq[wtm]);
      }
      break;
    case knight:
      break;
    case bishop:
      break;
    case rook:
      if (Castle(ply + 1, wtm) > 0) {
        if ((from == rook_A[wtm]) && (Castle(ply + 1, wtm) & 2)) {
          Castle(ply + 1, wtm) &= 1;
          HashCastle(1, wtm);
        } else if ((from == rook_H[wtm]) && (Castle(ply + 1, wtm) & 1)) {
          Castle(ply + 1, wtm) &= 2;
          HashCastle(0, wtm);
        }
      }
      break;
    case queen:
      break;
    case king:
      KingSQ(wtm) = to;
      if (Castle(ply + 1, wtm) > 0) {
        if (Castle(ply + 1, wtm) & 2)
          HashCastle(1, wtm);
        if (Castle(ply + 1, wtm) & 1)
          HashCastle(0, wtm);
        if (Abs(to - from) == 2) {
          Castle(ply + 1, wtm) = -ply;
          piece = rook;
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
 ************************************************************
 *                                                          *
 *   If this is a capture move, we also have to update the  *
 *   information that must change when a piece is removed   *
 *   from the board.                                        *
 *                                                          *
 ************************************************************
 */
  if (captured) {
    Rule50Moves(ply + 1) = 0;
    TotalAllPieces--;
    if (promote)
      piece = promote;
    Hash(btm, captured, to);
    Clear(to, Pieces(btm, captured));
    Clear(to, Occupied(btm));
    Material -= PieceValues(btm, captured);
    TotalPieces(btm, captured)--;
    if (captured != pawn)
      TotalPieces(btm, occupied) -= p_vals[captured];
    switch (captured) {
      case pawn:
        HashP(btm, to);
        break;
      case knight:
        tree->pos.minors[btm]--;
        break;
      case bishop:
        tree->pos.minors[btm]--;
        break;
      case rook:
        if (Castle(ply + 1, btm) > 0) {
          if ((to == rook_A[btm]) && (Castle(ply + 1, btm) & 2)) {
            Castle(ply + 1, btm) &= 1;
            HashCastle(1, btm);
          } else if ((to == rook_H[btm]) && (Castle(ply + 1, btm) & 1)) {
            Castle(ply + 1, btm) &= 2;
            HashCastle(0, btm);
          }
        }
        tree->pos.majors[btm]--;
        break;
      case queen:
        tree->pos.majors[btm] -= 2;
        break;
      case king:
#if defined(DEBUG)
        Print(128, "captured a king (Make)\n");
        for (i = 1; i <= ply; i++)
          Print(128, "ply=%2d, piece=%2d,from=%2d,to=%2d,captured=%2d\n", i,
              Piece(tree->curmv[i]), From(tree->curmv[i]), To(tree->curmv[i]),
              Captured(tree->curmv[i]));
        Print(128, "ply=%2d, piece=%2d,from=%2d,to=%2d,captured=%2d\n", i,
            piece, from, to, captured);
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

/* last modified 11/05/10 */
/*
 *******************************************************************************
 *                                                                             *
 *   MakeMoveRoot() is used to make a move at the root of the game tree,       *
 *   before any searching is done.  It uses MakeMove() to execute the move,    *
 *   but then copies the resulting position back to position[0], the actual    *
 *   board position.  It handles the special-case of the draw-by-repetition    *
 *   rule by clearing the repetition list when a non-reversible move is made,  *
 *   since no repetitions are possible once such a move is played.             *
 *                                                                             *
 *******************************************************************************
 */
void MakeMoveRoot(TREE * RESTRICT tree, int move, int wtm) {
  int side;

/*
 ************************************************************
 *                                                          *
 *   First, make the move and replace position[0] with the  *
 *   new position.                                          *
 *                                                          *
 ************************************************************
 */
  tree->rep_list[wtm][Repetition(wtm)++] = HashKey;
  MakeMove(tree, 0, move, wtm);
/*
 ************************************************************
 *                                                          *
 *   Now, if this is a non-reversible move, reset the       *
 *   repetition list pointer to start the count over.       *
 *                                                          *
 *   One odd action is to note if the castle status is      *
 *   currently negative, which indicates that that side     *
 *   castled during the previous search.  We simply set the *
 *   castle status for that side to zero and we are done.   *
 *                                                          *
 ************************************************************
 */
  for (side = black; side <= white; side++) {
    Castle(1, side) = Max(0, Castle(1, side));
    if (Rule50Moves(1) == 0)
      Repetition(side) = 0;
  }
  tree->position[0] = tree->position[1];
}
