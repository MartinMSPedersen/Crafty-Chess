#include <stdio.h>
#include <stdlib.h>
#include "types.h"
#include "function.h"
#include "data.h"

/* last modified 02/12/96 */
/*
********************************************************************************
*                                                                              *
*   MakeMove() is responsible for updating the position database whenever a    *
*   piece is moved.  it performs the following operations:  (1) update the     *
*   board structure itself by moving the piece and removing any captured       *
*   piece.  (2) update the hash keys.  (3) update material counts.  (4) update *
*   castling status.  (5) update number of moves since last reversible move.   *
*                                                                              *
********************************************************************************
*/
static BITBOARD bit_move;
void MakeMove(int ply, int move, int wtm)
{
/*
 ----------------------------------------------------------
|                                                          |
|   first, clear the EnPassant_Target bit-mask.  moving a  |
|   pawn two ranks will set it later in MakeMove().        |
|                                                          |
 ----------------------------------------------------------
*/
#if defined(DEBUG)
  ValidatePosition(ply,move,"MakeMove(1)");
#endif
  position[ply+1]=position[ply];
  save_hash_key[ply]=HashKey;
  save_pawn_hash_key[ply]=PawnHashKey;
  if (EnPassant(ply+1)) {
    HashEP(EnPassant(ply+1),HashKey);
    EnPassant(ply+1)=0;
  }
  Rule50Moves(ply+1)++;
/*
 ----------------------------------------------------------
|                                                          |
|   now do the piece-specific things by calling the        |
|   appropriate routine.                                   |
|                                                          |
 ----------------------------------------------------------
*/
  bit_move=Or(set_mask[From(move)],set_mask[To(move)]);
  switch (Piece(move)) {
  case pawn:
    MakeMovePawn(ply,From(move),To(move),Captured(move),Promote(move),wtm);
    if (Captured(move) == 1) {
      if (wtm) {
        if (!And(BlackPawns,set_mask[To(move)])) move&=~(7<<15);
      }
      else {
        if (!And(WhitePawns,set_mask[To(move)])) move&=~(7<<15);
      }
    }
    Rule50Moves(ply+1)=0;
    break;
  case knight:
    MakeMoveKnight(From(move),To(move),wtm);
    break;
  case bishop:
    MakeMoveBishop(From(move),To(move),wtm);
    break;
  case rook:
    MakeMoveRook(ply,From(move),To(move),wtm);
    break;
  case queen:
    MakeMoveQueen(From(move),To(move),wtm);
    break;
  case king:
    MakeMoveKing(ply,From(move),To(move),wtm);
    break;
  }
/*
********************************************************************************
*                                                                              *
*   now it is time to "gracefully" remove a piece from the game board since it *
*   is being captured.  this includes updating the board structure.            *
*                                                                              *
********************************************************************************
*/
  if(Captured(move)) {
    Rule50Moves(ply+1)=0;
    if (Promote(move)) move=(move&(~(7<<12)))|(Promote(move)<<12);
    switch (Captured(move)) {
/*
 ----------------------------------------------------------
|                                                          |
|   remove a captured pawn.                                |
|                                                          |
 ----------------------------------------------------------
*/
    case pawn: 
      if (wtm) {
        Clear(To(move),BlackPawns);
        Clear(To(move),BlackPieces);
        HashPB(To(move),HashKey);
        HashPB(To(move),PawnHashKey);
        Material+=PAWN_VALUE;
        TotalBlackPawns--;
      }
      else {
        Clear(To(move),WhitePawns);
        Clear(To(move),WhitePieces);
        HashPW(To(move),HashKey);
        HashPW(To(move),PawnHashKey);
        Material-=PAWN_VALUE;
        TotalWhitePawns--;
      }
    break;
/*
 ----------------------------------------------------------
|                                                          |
|   remove a captured knight.                              |
|                                                          |
 ----------------------------------------------------------
*/
    case knight: 
      if (wtm) {
        Clear(To(move),BlackKnights);
        Clear(To(move),BlackPieces);
        HashNB(To(move),HashKey);
        TotalBlackPieces-=knight_v;
        Material+=KNIGHT_VALUE;
      }
      else {
        Clear(To(move),WhiteKnights);
        Clear(To(move),WhitePieces);
        HashNW(To(move),HashKey);
        TotalWhitePieces-=knight_v;
        Material-=KNIGHT_VALUE;
      }
    break;
/*
 ----------------------------------------------------------
|                                                          |
|   remove a captured bishop.                              |
|                                                          |
 ----------------------------------------------------------
*/
    case bishop: 
      if (SlidingDiag(Piece(move))) Set(To(move),BishopsQueens);
      else Clear(To(move),BishopsQueens);
      if (wtm) {
        Clear(To(move),BlackBishops);
        Clear(To(move),BlackPieces);
        HashBB(To(move),HashKey);
        TotalBlackPieces-=bishop_v;
        Material+=BISHOP_VALUE;
      }
      else {
        Clear(To(move),WhiteBishops);
        Clear(To(move),WhitePieces);
        HashBW(To(move),HashKey);
        TotalWhitePieces-=bishop_v;
        Material-=BISHOP_VALUE;
      }
    break;
/*
 ----------------------------------------------------------
|                                                          |
|   remove a captured rook.                                |
|                                                          |
 ----------------------------------------------------------
*/
    case rook: 
      if (SlidingRow(Piece(move))) Set(To(move),RooksQueens);
      else Clear(To(move),RooksQueens);
      if (wtm) {
        Clear(To(move),BlackRooks);
        Clear(To(move),BlackPieces);
        HashRB(To(move),HashKey);
        if (BlackCastle(ply) > 0) {
          if ((To(move) == 56) && (BlackCastle(ply+1)&2)) {
            BlackCastle(ply+1)&=1;
            HashCastleB(1,HashKey);
          }
          else if ((To(move) == 63) && (BlackCastle(ply+1)&1)) {
            BlackCastle(ply+1)&=2;
            HashCastleB(0,HashKey);
          }
        }
        TotalBlackPieces-=rook_v;
        Material+=ROOK_VALUE;
      }
      else {
        Clear(To(move),WhiteRooks);
        Clear(To(move),WhitePieces);
        HashRW(To(move),HashKey);
        if (WhiteCastle(ply) > 0) {
          if ((To(move) == 0) && (WhiteCastle(ply+1)&2)) {
            WhiteCastle(ply+1)&=1;
            HashCastleW(1,HashKey);
          }
          else if ((To(move) == 7) && (WhiteCastle(ply+1)&1)) {
            WhiteCastle(ply+1)&=2;
            HashCastleW(0,HashKey);
          }
        }
        TotalWhitePieces-=rook_v;
        Material-=ROOK_VALUE;
      }
    break;
/*
 ----------------------------------------------------------
|                                                          |
|   remove a captured queen.                               |
|                                                          |
 ----------------------------------------------------------
*/
    case queen: 
      if (SlidingDiag(Piece(move))) Set(To(move),BishopsQueens);
      else Clear(To(move),BishopsQueens);
      if (SlidingRow(Piece(move))) Set(To(move),RooksQueens);
      else Clear(To(move),RooksQueens);
      if (wtm) {
        Clear(To(move),BlackQueens);
        Clear(To(move),BlackPieces);
        HashQB(To(move),HashKey);
        TotalBlackPieces-=queen_v;
        Material+=QUEEN_VALUE;
      }
      else {
        Clear(To(move),WhiteQueens);
        Clear(To(move),WhitePieces);
        HashQW(To(move),HashKey);
        TotalWhitePieces-=queen_v;
        Material-=QUEEN_VALUE;
      }
      break;
/*
 ----------------------------------------------------------
|                                                          |
|   remove a captured king. [this is an error condition]   |
|                                                          |
 ----------------------------------------------------------
*/
    case king: 
      Print(1,"captured a king\n");
      Print(1,"piece=%d,from=%d,to=%d,captured=%d\n",
            Piece(move),From(move),
            To(move),Captured(move));
      Print(1,"ply=%d\n",ply);
      if (log_file) DisplayChessBoard(log_file,search);
    }
  }
#if defined(DEBUG)
  ValidatePosition(ply+1,move,"MakeMove(2)");
#endif
  return;
}

/*
********************************************************************************
*                                                                              *
*   make bishop moves.                                                         *
*                                                                              *
********************************************************************************
*/
void MakeMoveBishop(int from, int to, int wtm)
{
/*
 --------------------------------------------------------------------
|                                                                    |
|  first, update the occupied-square bitboards, of which there are   |
|  several.                                                          |
|                                                                    |
 --------------------------------------------------------------------
*/
  ClearSet(bit_move,BishopsQueens);
  ClearRL90(from,OccupiedRL90);
  ClearRL45(from,OccupiedRL45);
  ClearRR45(from,OccupiedRR45);
  SetRL90(to,OccupiedRL90);
  SetRL45(to,OccupiedRL45);
  SetRR45(to,OccupiedRR45);
  if (wtm) {
    ClearSet(bit_move,WhiteBishops);
    ClearSet(bit_move,WhitePieces);
    HashBW(from,HashKey);
    HashBW(to,HashKey);
    PieceOnSquare(from)=0;
    PieceOnSquare(to)=bishop;
  }
  else {
    ClearSet(bit_move,BlackBishops);
    ClearSet(bit_move,BlackPieces);
    HashBB(from,HashKey);
    HashBB(to,HashKey);
    PieceOnSquare(from)=0;
    PieceOnSquare(to)=-bishop;
  }
}

/*
********************************************************************************
*                                                                              *
*   make king moves.  the only special case is castling, which is indicated    *
*   by from=4, to=6 for o-o as an example.  the king is moving from e1-g1      *
*   which is normally illegal.  in this case, the correct rook is also moved.  *
*                                                                              *
*   note that moving the king in any direction resets the x_castle [x=w or b]  *
*   flag indicating that castling is not possible in *this* position.          *
*                                                                              *
********************************************************************************
*/
void MakeMoveKing(int ply, int from, int to, int wtm)
{
/*
 --------------------------------------------------------------------
|                                                                    |
|  first, update the occupied-square bitboards, of which there are   |
|  several.                                                          |
|                                                                    |
 --------------------------------------------------------------------
*/
  ClearRL90(from,OccupiedRL90);
  ClearRL45(from,OccupiedRL45);
  ClearRR45(from,OccupiedRR45);
  SetRL90(to,OccupiedRL90);
  SetRL45(to,OccupiedRL45);
  SetRR45(to,OccupiedRR45);
  if (wtm) {
    ClearSet(bit_move,WhitePieces);
    HashKW(from,HashKey);
    HashKW(to,HashKey);
    PieceOnSquare(from)=0;
    PieceOnSquare(to)=king;
    if (WhiteCastle(ply) > 0) {
      if (WhiteCastle(ply+1)&2) HashCastleW(1,HashKey);
      if (WhiteCastle(ply+1)&1) HashCastleW(0,HashKey);
      if (abs(to-from) == 2) WhiteCastle(ply+1)=-4;
      else WhiteCastle(ply+1)=0;
    }
    WhiteKingSQ=to;
    if (abs(to-from) == 2)
      if (to == 6) {
        bit_move=Or(set_mask[F1],set_mask[H1]);
        MakeMoveRook(ply,H1,F1,wtm);
      }
      else {
        bit_move=Or(set_mask[A1],set_mask[D1]);
        MakeMoveRook(ply,A1,D1,wtm);
      }
  }
  else {
    ClearSet(bit_move,BlackPieces);
    HashKB(from,HashKey);
    HashKB(to,HashKey);
    PieceOnSquare(from)=0;
    PieceOnSquare(to)=-king;
    BlackKingSQ=to;
    if (BlackCastle(ply+1) > 0) {
      if (BlackCastle(ply+1)&2) HashCastleB(1,HashKey);
      if (BlackCastle(ply+1)&1) HashCastleB(0,HashKey);
      if (abs(to-from) == 2) BlackCastle(ply+1)=-4;
      else BlackCastle(ply+1)=0;
    }
    if (abs(to-from) == 2)
      if (to == 62) {
        bit_move=Or(set_mask[F8],set_mask[H8]);
        MakeMoveRook(ply,H8,F8,wtm);
      }
      else {
        bit_move=Or(set_mask[A8],set_mask[D8]);
        MakeMoveRook(ply,A8,D8,wtm);
      }
  }
}

/*
********************************************************************************
*                                                                              *
*   make knight moves.                                                         *
*                                                                              *
********************************************************************************
*/
void MakeMoveKnight(int from, int to, int wtm)
{
/*
 --------------------------------------------------------------------
|                                                                    |
|  first, update the occupied-square bitboards, of which there are   |
|  several.                                                          |
|                                                                    |
 --------------------------------------------------------------------
*/
  ClearRL90(from,OccupiedRL90);
  ClearRL45(from,OccupiedRL45);
  ClearRR45(from,OccupiedRR45);
  SetRL90(to,OccupiedRL90);
  SetRL45(to,OccupiedRL45);
  SetRR45(to,OccupiedRR45);
  if (wtm) {
    ClearSet(bit_move,WhiteKnights);
    ClearSet(bit_move,WhitePieces);
    HashNW(from,HashKey);
    HashNW(to,HashKey);
    PieceOnSquare(from)=0;
    PieceOnSquare(to)=knight;
  }
  else {
    ClearSet(bit_move,BlackKnights);
    ClearSet(bit_move,BlackPieces);
    HashNB(from,HashKey);
    HashNB(to,HashKey);
    PieceOnSquare(from)=0;
    PieceOnSquare(to)=-knight;
  }
}

/*
********************************************************************************
*                                                                              *
*   make pawn moves.  there are two special cases:  (a) enpassant captures     *
*   where the captured pawn is not on the "to" square and must be removed in   *
*   a different way, and (2) pawn promotions (where the "Promote" variable     *
*   is non-zero) requires updating the appropriate bit boards since we are     *
*   creating a new piece.                                                      *
*                                                                              *
********************************************************************************
*/
void MakeMovePawn(int ply, int from, int to, int Captured, int Promote, int wtm)
{
/*
 --------------------------------------------------------------------
|                                                                    |
|  now, update the occupied-square bitboards, of which there are     |
|  several.                                                          |
|                                                                    |
 --------------------------------------------------------------------
*/
  ClearRL90(from,OccupiedRL90);
  ClearRL45(from,OccupiedRL45);
  ClearRR45(from,OccupiedRR45);
  SetRL90(to,OccupiedRL90);
  SetRL45(to,OccupiedRL45);
  SetRR45(to,OccupiedRR45);
  if (wtm) {
    ClearSet(bit_move,WhitePawns);
    ClearSet(bit_move,WhitePieces);
    HashPW(from,HashKey);
    HashPW(from,PawnHashKey);
    HashPW(to,HashKey);
    HashPW(to,PawnHashKey);
    PieceOnSquare(from)=0;
    PieceOnSquare(to)=pawn;
    if (Captured == 1) {
      if(!And(BlackPawns,set_mask[to])) {
        ClearRL90(to-8,OccupiedRL90);
        ClearRL45(to-8,OccupiedRL45);
        ClearRR45(to-8,OccupiedRR45);
        Clear(to-8,BlackPawns);
        Clear(to-8,BlackPieces);
        HashPB(to-8,HashKey);
        HashPB(to-8,PawnHashKey);
        PieceOnSquare(to-8)=0;
        Material+=PAWN_VALUE;
        TotalBlackPawns--;
      }
    }
/*
 --------------------------------------------------------------------
|                                                                    |
|  if this is a pawn promotion, remove the pawn from the counts      |
|  then update the correct piece board to reflect the piece just     |
|  created.                                                          |
|                                                                    |
 --------------------------------------------------------------------
*/
    if (Promote) {
      TotalWhitePawns--;
      Material-=PAWN_VALUE;
      Clear(to,WhitePawns);
      HashPW(to,HashKey);
      HashPW(to,PawnHashKey);
      switch (Promote) {
      case knight:
        Set(to,WhiteKnights);
        HashNW(to,HashKey);
        PieceOnSquare(to)=knight;
        TotalWhitePieces+=knight_v;
        Material+=KNIGHT_VALUE;
        break;
      case bishop:
        Set(to,WhiteBishops);
        Set(to,BishopsQueens);
        HashBW(to,HashKey);
        PieceOnSquare(to)=bishop;
        TotalWhitePieces+=bishop_v;
        Material+=BISHOP_VALUE;
        break;
      case rook:
        Set(to,WhiteRooks);
        Set(to,RooksQueens);
        HashRW(to,HashKey);
        PieceOnSquare(to)=rook;
        TotalWhitePieces+=rook_v;
        Material+=ROOK_VALUE;
        break;
      case queen:
        Set(to,WhiteQueens);
        Set(to,BishopsQueens);
        Set(to,RooksQueens);
        HashQW(to,HashKey);
        PieceOnSquare(to)=queen;
        TotalWhitePieces+=queen_v;
        Material+=QUEEN_VALUE;
        break;
      }
    }
    else 
      if (((to-from) == 16) && And(mask_enpassant_test[to],BlackPawns)) {
        EnPassant(ply+1)=to-8;
        HashEP(to-8,HashKey);
      }
  }
  else {
    ClearSet(bit_move,BlackPawns);
    ClearSet(bit_move,BlackPieces);
    HashPB(from,HashKey);
    HashPB(from,PawnHashKey);
    HashPB(to,HashKey);
    HashPB(to,PawnHashKey);
    PieceOnSquare(from)=0;
    PieceOnSquare(to)=-pawn;
    if (Captured == 1) {
      if(!And(WhitePawns,set_mask[to])) {
        ClearRL90(to+8,OccupiedRL90);
        ClearRL45(to+8,OccupiedRL45);
        ClearRR45(to+8,OccupiedRR45);
        Clear(to+8,WhitePawns);
        Clear(to+8,WhitePieces);
        HashPW(to+8,HashKey);
        HashPW(to+8,PawnHashKey);
        PieceOnSquare(to+8)=0;
        Material-=PAWN_VALUE;
        TotalWhitePawns--;
      }
    }
/*
 --------------------------------------------------------------------
|                                                                    |
|  if this is a pawn promotion, remove the pawn from the counts      |
|  then update the correct piece board to reflect the piece just     |
|  created.                                                          |
|                                                                    |
 --------------------------------------------------------------------
*/
    if (Promote) {
      TotalBlackPawns--;
      Material+=PAWN_VALUE;
      Clear(to,BlackPawns);
      HashPB(to,HashKey);
      HashPB(to,PawnHashKey);
      switch (Promote) {
      case knight:
        Set(to,BlackKnights);
        HashNB(to,HashKey);
        PieceOnSquare(to)=-knight;
        TotalBlackPieces+=knight_v;
        Material-=KNIGHT_VALUE;
        break;
      case bishop:
        Set(to,BlackBishops);
        Set(to,BishopsQueens);
        HashBB(to,HashKey);
        PieceOnSquare(to)=-bishop;
        TotalBlackPieces+=bishop_v;
        Material-=BISHOP_VALUE;
        break;
      case rook:
        Set(to,BlackRooks);
        Set(to,RooksQueens);
        HashRB(to,HashKey);
        PieceOnSquare(to)=-rook;
        TotalBlackPieces+=rook_v;
        Material-=ROOK_VALUE;
        break;
      case queen:
        Set(to,BlackQueens);
        Set(to,BishopsQueens);
        Set(to,RooksQueens);
        HashQB(to,HashKey);
        PieceOnSquare(to)=-queen;
        TotalBlackPieces+=queen_v;
        Material-=QUEEN_VALUE;
        break;
      }
    }
    else 
      if (((from-to) == 16) && And(mask_enpassant_test[to],WhitePawns)) {
        EnPassant(ply+1)=to+8;
        HashEP(to+8,HashKey);
      }
  }
}

/*
********************************************************************************
*                                                                              *
*   make queen moves                                                           *
*                                                                              *
********************************************************************************
*/
void MakeMoveQueen(int from, int to, int wtm)
{
/*
 --------------------------------------------------------------------
|                                                                    |
|  first, update the occupied-square bitboards, of which there are   |
|  several.                                                          |
|                                                                    |
 --------------------------------------------------------------------
*/
  ClearSet(bit_move,BishopsQueens);
  ClearSet(bit_move,RooksQueens);
  ClearRL90(from,OccupiedRL90);
  ClearRL45(from,OccupiedRL45);
  ClearRR45(from,OccupiedRR45);
  SetRL90(to,OccupiedRL90);
  SetRL45(to,OccupiedRL45);
  SetRR45(to,OccupiedRR45);
  if (wtm) {
    ClearSet(bit_move,WhiteQueens);
    ClearSet(bit_move,WhitePieces);
    HashQW(from,HashKey);
    HashQW(to,HashKey);
    PieceOnSquare(from)=0;
    PieceOnSquare(to)=queen;
  }
  else {
    ClearSet(bit_move,BlackQueens);
    ClearSet(bit_move,BlackPieces);
    HashQB(from,HashKey);
    HashQB(to,HashKey);
    PieceOnSquare(from)=0;
    PieceOnSquare(to)=-queen;
  }
}

/*
********************************************************************************
*                                                                              *
*   make rook moves.  the only special case handling required is to determine  *
*   if x_castle is non-zero [x=w or b based on side to move].  if it is non-   *
*   zero, the value must be corrected if either rook is moving from its        *
*   original square, so that castling with that rook becomes impossible.       *
*                                                                              *
********************************************************************************
*/
void MakeMoveRook(int ply, int from, int to, int wtm)
{
/*
 --------------------------------------------------------------------
|                                                                    |
|  first, update the occupied-square bitboards, of which there are   |
|  several.                                                          |
|                                                                    |
 --------------------------------------------------------------------
*/
  ClearSet(bit_move,RooksQueens);
  ClearRL90(from,OccupiedRL90);
  ClearRL45(from,OccupiedRL45);
  ClearRR45(from,OccupiedRR45);
  SetRL90(to,OccupiedRL90);
  SetRL45(to,OccupiedRL45);
  SetRR45(to,OccupiedRR45);
  if (wtm) {
    ClearSet(bit_move,WhiteRooks);
    ClearSet(bit_move,WhitePieces);
    HashRW(from,HashKey);
    HashRW(to,HashKey);
    PieceOnSquare(from)=0;
    PieceOnSquare(to)=rook;
    if (WhiteCastle(ply+1) > 0) {
      if ((from == 0) && (WhiteCastle(ply+1)&2)) {
        WhiteCastle(ply+1)&=1;
        HashCastleW(1,HashKey);
      }
      else if ((from == 7) && (WhiteCastle(ply+1)&1)) {
        WhiteCastle(ply+1)&=2;
        HashCastleW(0,HashKey);
      }
    }
  }
  else {
    ClearSet(bit_move,BlackRooks);
    ClearSet(bit_move,BlackPieces);
    HashRB(from,HashKey);
    HashRB(to,HashKey);
    PieceOnSquare(from)=0;
    PieceOnSquare(to)=-rook;
    if (BlackCastle(ply+1) > 0) {
      if ((from == 56) && (BlackCastle(ply+1)&2)) {
        BlackCastle(ply+1)&=1;
        HashCastleB(1,HashKey);
      }
      else if ((from == 63) && (BlackCastle(ply+1)&1)) {
        BlackCastle(ply+1)&=2;
        HashCastleB(0,HashKey);
      }
    }
  }
}

/*
********************************************************************************
*                                                                              *
*   MakeMoveRoot() is used to make a move at the root of the game tree,        *
*   before any searching is done.  it uses MakeMove() to execute the move,      *
*   but then copies the resulting position back to position[0], the actual     *
*   board position.  it handles the special-case of the draw-by-repetition     *
*   rule by maintaining a list of previous positions, which is reset each time *
*   a non-reversible (pawn move or capture move) is made.                      *
*                                                                              *
********************************************************************************
*/
void MakeMoveRoot(int move, int wtm)
{
/*
 ----------------------------------------------------------
|                                                          |
|   first, make the move and replace position[0] with the  |
|   new position.                                          |
|                                                          |
 ----------------------------------------------------------
*/
  MakeMove(0,move,wtm);
/*
 ----------------------------------------------------------
|                                                          |
|   now, if this is a non-reversible move, reset the       |
|   repetition list pointer to start the count over.       |
|                                                          |
 ----------------------------------------------------------
*/
  if (Rule50Moves(1) == 0) {
    repetition_head_b=repetition_list_b;
    repetition_head_w=repetition_list_w;
  }
  WhiteCastle(1)=Max(0,WhiteCastle(1));
  BlackCastle(1)=Max(0,BlackCastle(1));
  position[0]=position[1];
  if (ChangeSide(wtm))
    *repetition_head_w++=HashKey;
  else
    *repetition_head_b++=HashKey;
}
