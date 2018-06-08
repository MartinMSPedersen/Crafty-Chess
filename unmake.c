#include <stdio.h>
#include <stdlib.h>
#include "chess.h"
#include "data.h"

/* last modified 02/12/96 */
/*
********************************************************************************
*                                                                              *
*   UnMakeMove() is responsible for updating the position database whenever a  *
*   move is retracted.  it is the exact inverse of MakeMove().                 *
*                                                                              *
********************************************************************************
*/
static BITBOARD bit_move;
void UnMakeMove(int ply, int move, int wtm)
{
/*
 ----------------------------------------------------------
|                                                          |
|   first, take care of the hash key if there's a possible |
|   enpassant pawn capture.                                |
|                                                          |
 ----------------------------------------------------------
*/
  HashKey=save_hash_key[ply];
  PawnHashKey=save_pawn_hash_key[ply];
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
    UnMakeMovePawn(ply,From(move),To(move),Captured(move),Promote(move),wtm);
    if (Captured(move) == 1 && To(move) == EnPassant(ply)) move&=~(7<<15);
    break;
  case knight:
    UnMakeMoveKnight(From(move),To(move),wtm);
    break;
  case bishop:
    UnMakeMoveBishop(From(move),To(move),wtm);
    break;
  case rook:
    UnMakeMoveRook(From(move),To(move),wtm);
    break;
  case queen:
    UnMakeMoveQueen(From(move),To(move),wtm);
    break;
  case king:
    UnMakeMoveKing(From(move),To(move),wtm);
    break;
  }
/*
********************************************************************************
*                                                                              *
*   now it is time to restore a piece that was captured.                       *
*                                                                              *
********************************************************************************
*/
  if(Captured(move)) {
    SetRL90(To(move),OccupiedRL90);
    SetRL45(To(move),OccupiedRL45);
    SetRR45(To(move),OccupiedRR45);
    switch (Captured(move)) {
/*
 ----------------------------------------------------------
|                                                          |
|   restore a captured pawn.                               |
|                                                          |
 ----------------------------------------------------------
*/
    case pawn: 
      if (wtm) {
        Set(To(move),BlackPawns);
        Set(To(move),BlackPieces);
        PieceOnSquare(To(move))=-pawn;
        Material-=PAWN_VALUE;
        TotalBlackPawns++;
      }
      else {
        Set(To(move),WhitePawns);
        Set(To(move),WhitePieces);
        PieceOnSquare(To(move))=pawn;
        Material+=PAWN_VALUE;
        TotalWhitePawns++;
      }
    break;
/*
 ----------------------------------------------------------
|                                                          |
|   restore a captured knight.                             |
|                                                          |
 ----------------------------------------------------------
*/
    case knight: 
      if (wtm) {
        Set(To(move),BlackKnights);
        Set(To(move),BlackPieces);
        PieceOnSquare(To(move))=-knight;
        TotalBlackPieces+=knight_v;
        Material-=KNIGHT_VALUE;
      }
      else {
        Set(To(move),WhiteKnights);
        Set(To(move),WhitePieces);
        PieceOnSquare(To(move))=knight;
        TotalWhitePieces+=knight_v;
        Material+=KNIGHT_VALUE;
      }
    break;
/*
 ----------------------------------------------------------
|                                                          |
|   restore a captured bishop.                             |
|                                                          |
 ----------------------------------------------------------
*/
    case bishop: 
      Set(To(move),BishopsQueens);
      if (wtm) {
        Set(To(move),BlackBishops);
        Set(To(move),BlackPieces);
        PieceOnSquare(To(move))=-bishop;
        TotalBlackPieces+=bishop_v;
        Material-=BISHOP_VALUE;
      }
      else {
        Set(To(move),WhiteBishops);
        Set(To(move),WhitePieces);
        PieceOnSquare(To(move))=bishop;
        TotalWhitePieces+=bishop_v;
        Material+=BISHOP_VALUE;
      }
    break;
/*
 ----------------------------------------------------------
|                                                          |
|   restore a captured rook.                               |
|                                                          |
 ----------------------------------------------------------
*/
    case rook: 
      Set(To(move),RooksQueens);
      if (wtm) {
        Set(To(move),BlackRooks);
        Set(To(move),BlackPieces);
        PieceOnSquare(To(move))=-rook;
        TotalBlackPieces+=rook_v;
        Material-=ROOK_VALUE;
      }
      else {
        Set(To(move),WhiteRooks);
        Set(To(move),WhitePieces);
        PieceOnSquare(To(move))=rook;
        TotalWhitePieces+=rook_v;
        Material+=ROOK_VALUE;
      }
    break;
/*
 ----------------------------------------------------------
|                                                          |
|   restore a captured queen.                              |
|                                                          |
 ----------------------------------------------------------
*/
    case queen: 
      Set(To(move),BishopsQueens);
      Set(To(move),RooksQueens);
      if (wtm) {
        Set(To(move),BlackQueens);
        Set(To(move),BlackPieces);
        PieceOnSquare(To(move))=-queen;
        TotalBlackPieces+=queen_v;
        Material-=QUEEN_VALUE;
      }
      else {
        Set(To(move),WhiteQueens);
        Set(To(move),WhitePieces);
        PieceOnSquare(To(move))=queen;
        TotalWhitePieces+=queen_v;
        Material+=QUEEN_VALUE;
      }
      break;
/*
 ----------------------------------------------------------
|                                                          |
|   restore a captured king. [this is an error condition]  |
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
  ValidatePosition(ply,move,"UnMakeMove(2)");
#endif
  return;
}

/*
********************************************************************************
*                                                                              *
*   unmake bishop moves.                                                       *
*                                                                              *
********************************************************************************
*/
void UnMakeMoveBishop(int from, int to, int wtm)
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
  SetRL90(from,OccupiedRL90);
  SetRL45(from,OccupiedRL45);
  SetRR45(from,OccupiedRR45);
  ClearRL90(to,OccupiedRL90);
  ClearRL45(to,OccupiedRL45);
  ClearRR45(to,OccupiedRR45);
  if (wtm) {
    ClearSet(bit_move,WhiteBishops);
    ClearSet(bit_move,WhitePieces);
    PieceOnSquare(to)=0;
    PieceOnSquare(from)=bishop;
  }
  else {
    ClearSet(bit_move,BlackBishops);
    ClearSet(bit_move,BlackPieces);
    PieceOnSquare(to)=0;
    PieceOnSquare(from)=-bishop;
  }
}

/*
********************************************************************************
*                                                                              *
*   unmake king moves.                                                         *
*                                                                              *
********************************************************************************
*/
void UnMakeMoveKing(int from, int to, int wtm)
{
/*
 --------------------------------------------------------------------
|                                                                    |
|  first, update the occupied-square bitboards, of which there are   |
|  several.                                                          |
|                                                                    |
 --------------------------------------------------------------------
*/
  SetRL90(from,OccupiedRL90);
  SetRL45(from,OccupiedRL45);
  SetRR45(from,OccupiedRR45);
  ClearRL90(to,OccupiedRL90);
  ClearRL45(to,OccupiedRL45);
  ClearRR45(to,OccupiedRR45);
  if (wtm) {
    ClearSet(bit_move,WhitePieces);
    PieceOnSquare(to)=0;
    PieceOnSquare(from)=king;
    WhiteKingSQ=from;
    if (abs(to-from) == 2) {
      if (to == 6) {
        bit_move=Or(set_mask[F1],set_mask[H1]);
        UnMakeMoveRook(H1,F1,wtm);
      }
      else {
        bit_move=Or(set_mask[A1],set_mask[D1]);
        UnMakeMoveRook(A1,D1,wtm);
      }
    }
  }
  else {
    ClearSet(bit_move,BlackPieces);
    PieceOnSquare(to)=0;
    PieceOnSquare(from)=-king;
    BlackKingSQ=from;
    if (abs(to-from) == 2) {
      if (to == 62) {
        bit_move=Or(set_mask[F8],set_mask[H8]);
        UnMakeMoveRook(H8,F8,wtm);
      }
      else {
        bit_move=Or(set_mask[A8],set_mask[D8]);
        UnMakeMoveRook(A8,D8,wtm);
      }
    }
  }
}

/*
********************************************************************************
*                                                                              *
*   unmake knight moves.                                                       *
*                                                                              *
********************************************************************************
*/
void UnMakeMoveKnight(int from, int to, int wtm)
{
/*
 --------------------------------------------------------------------
|                                                                    |
|  first, update the occupied-square bitboards, of which there are   |
|  several.                                                          |
|                                                                    |
 --------------------------------------------------------------------
*/
  SetRL90(from,OccupiedRL90);
  SetRL45(from,OccupiedRL45);
  SetRR45(from,OccupiedRR45);
  ClearRL90(to,OccupiedRL90);
  ClearRL45(to,OccupiedRL45);
  ClearRR45(to,OccupiedRR45);
  if (wtm) {
    ClearSet(bit_move,WhiteKnights);
    ClearSet(bit_move,WhitePieces);
    PieceOnSquare(to)=0;
    PieceOnSquare(from)=knight;
  }
  else {
    ClearSet(bit_move,BlackKnights);
    ClearSet(bit_move,BlackPieces);
    PieceOnSquare(to)=0;
    PieceOnSquare(from)=-knight;
  }
}

/*
********************************************************************************
*                                                                              *
*   unmake pawn moves.                                                         *
*                                                                              *
********************************************************************************
*/
void UnMakeMovePawn(int ply, int from, int to, int Captured, int Promote, int wtm)
{
/*
 --------------------------------------------------------------------
|                                                                    |
|  now, update the occupied-square bitboards, of which there are     |
|  several.                                                          |
|                                                                    |
 --------------------------------------------------------------------
*/
  SetRL90(from,OccupiedRL90);
  SetRL45(from,OccupiedRL45);
  SetRR45(from,OccupiedRR45);
  ClearRL90(to,OccupiedRL90);
  ClearRL45(to,OccupiedRL45);
  ClearRR45(to,OccupiedRR45);
  if (wtm) {
    ClearSet(bit_move,WhitePawns);
    ClearSet(bit_move,WhitePieces);
    PieceOnSquare(to)=0;
    PieceOnSquare(from)=pawn;
    if (Captured == 1) {
      if(EnPassant(ply) == to) {
        SetRL90(to-8,OccupiedRL90);
        SetRL45(to-8,OccupiedRL45);
        SetRR45(to-8,OccupiedRR45);
        Set(to-8,BlackPawns);
        Set(to-8,BlackPieces);
        PieceOnSquare(to-8)=-pawn;
        Material-=PAWN_VALUE;
        TotalBlackPawns++;
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
      TotalWhitePawns++;
      Material+=PAWN_VALUE;
      Clear(to,WhitePawns);
      Clear(to,WhitePieces);
      switch (Promote) {
      case knight:
        Clear(to,WhiteKnights);
        TotalWhitePieces-=knight_v;
        Material-=KNIGHT_VALUE;
        break;
      case bishop:
        Clear(to,WhiteBishops);
        Clear(to,BishopsQueens);
        TotalWhitePieces-=bishop_v;
        Material-=BISHOP_VALUE;
        break;
      case rook:
        Clear(to,WhiteRooks);
        Clear(to,RooksQueens);
        TotalWhitePieces-=rook_v;
        Material-=ROOK_VALUE;
        break;
      case queen:
        Clear(to,WhiteQueens);
        Clear(to,BishopsQueens);
        Clear(to,RooksQueens);
        TotalWhitePieces-=queen_v;
        Material-=QUEEN_VALUE;
        break;
      }
    }
  }
  else {
    ClearSet(bit_move,BlackPawns);
    ClearSet(bit_move,BlackPieces);
    PieceOnSquare(to)=0;
    PieceOnSquare(from)=-pawn;
    if (Captured == 1) {
      if(EnPassant(ply) == to) {
        SetRL90(to+8,OccupiedRL90);
        SetRL45(to+8,OccupiedRL45);
        SetRR45(to+8,OccupiedRR45);
        Set(to+8,WhitePawns);
        Set(to+8,WhitePieces);
        PieceOnSquare(to+8)=pawn;
        Material+=PAWN_VALUE;
        TotalWhitePawns++;
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
      TotalBlackPawns++;
      Material-=PAWN_VALUE;
      Clear(to,BlackPawns);
      Clear(to,BlackPieces);
      switch (Promote) {
      case knight:
        Clear(to,BlackKnights);
        TotalBlackPieces-=knight_v;
        Material+=KNIGHT_VALUE;
        break;
      case bishop:
        Clear(to,BlackBishops);
        Clear(to,BishopsQueens);
        TotalBlackPieces-=bishop_v;
        Material+=BISHOP_VALUE;
        break;
      case rook:
        Clear(to,BlackRooks);
        Clear(to,RooksQueens);
        TotalBlackPieces-=rook_v;
        Material+=ROOK_VALUE;
        break;
      case queen:
        Clear(to,BlackQueens);
        Clear(to,BishopsQueens);
        Clear(to,RooksQueens);
        TotalBlackPieces-=queen_v;
        Material+=QUEEN_VALUE;
        break;
      }
    }
  }
}

/*
********************************************************************************
*                                                                              *
*   unmake queen moves.                                                        *
*                                                                              *
********************************************************************************
*/
void UnMakeMoveQueen(int from, int to, int wtm)
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
  SetRL90(from,OccupiedRL90);
  SetRL45(from,OccupiedRL45);
  SetRR45(from,OccupiedRR45);
  ClearRL90(to,OccupiedRL90);
  ClearRL45(to,OccupiedRL45);
  ClearRR45(to,OccupiedRR45);
  if (wtm) {
    ClearSet(bit_move,WhiteQueens);
    ClearSet(bit_move,WhitePieces);
    PieceOnSquare(to)=0;
    PieceOnSquare(from)=queen;
  }
  else {
    ClearSet(bit_move,BlackQueens);
    ClearSet(bit_move,BlackPieces);
    PieceOnSquare(to)=0;
    PieceOnSquare(from)=-queen;
  }
}

/*
********************************************************************************
*                                                                              *
*   unmake rook moves.                                                         *
*                                                                              *
********************************************************************************
*/
void UnMakeMoveRook(int from, int to, int wtm)
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
  SetRL90(from,OccupiedRL90);
  SetRL45(from,OccupiedRL45);
  SetRR45(from,OccupiedRR45);
  ClearRL90(to,OccupiedRL90);
  ClearRL45(to,OccupiedRL45);
  ClearRR45(to,OccupiedRR45);
  if (wtm) {
    ClearSet(bit_move,WhiteRooks);
    ClearSet(bit_move,WhitePieces);
    PieceOnSquare(to)=0;
    PieceOnSquare(from)=rook;
  }
  else {
    ClearSet(bit_move,BlackRooks);
    ClearSet(bit_move,BlackPieces);
    PieceOnSquare(to)=0;
    PieceOnSquare(from)=-rook;
  }
}
