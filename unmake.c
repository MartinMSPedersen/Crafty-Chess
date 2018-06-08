#include <stdio.h>
#include <stdlib.h>
#include "chess.h"
#include "data.h"

/* last modified 03/11/98 */
/*
********************************************************************************
*                                                                              *
*   UnMakeMove() is responsible for updating the position database whenever a  *
*   move is retracted.  it is the exact inverse of MakeMove().                 *
*                                                                              *
********************************************************************************
*/
void UnMakeMove(TREE *tree, int ply, int move, int wtm) {
  register int piece, from, to, captured, promote;
  BITBOARD bit_move;
/*
 ----------------------------------------------------------
|                                                          |
|   first, take care of the hash key if there's a possible |
|   enpassant pawn capture.                                |
|                                                          |
 ----------------------------------------------------------
*/
  HashKey=tree->save_hash_key[ply];
  PawnHashKey=tree->save_pawn_hash_key[ply];
/*
 ----------------------------------------------------------
|                                                          |
|   now do the piece-specific things by calling the        |
|   appropriate routine.                                   |
|                                                          |
 ----------------------------------------------------------
*/
  piece=Piece(move);
  from=From(move);
  to=To(move);
  captured=Captured(move);
  promote=Promote(move);
UnMakePieceMove:
  SetRL90(from,OccupiedRL90);
  SetRL45(from,OccupiedRL45);
  SetRR45(from,OccupiedRR45);
  ClearRL90(to,OccupiedRL90);
  ClearRL45(to,OccupiedRL45);
  ClearRR45(to,OccupiedRR45);
  bit_move=Or(SetMask(from),SetMask(to));
  PieceOnSquare(to)=0;
  switch (piece) {

/*
********************************************************************************
*                                                                              *
*   unmake pawn moves.                                                         *
*                                                                              *
********************************************************************************
*/
  case pawn:
    if (wtm) {
      ClearSet(bit_move,WhitePawns);
      ClearSet(bit_move,WhitePieces);
      PieceOnSquare(from)=pawn;
      if (captured == 1) {
        if(EnPassant(ply) == to) {
          TotalPieces++;
          SetRL90(to-8,OccupiedRL90);
          SetRL45(to-8,OccupiedRL45);
          SetRR45(to-8,OccupiedRR45);
          Set(to-8,BlackPawns);
          Set(to-8,BlackPieces);
          PieceOnSquare(to-8)=-pawn;
          Material-=PAWN_VALUE;
          TotalBlackPawns++;
          captured=0;
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
      if (promote) {
        TotalWhitePawns++;
        Material+=PAWN_VALUE;
        Clear(to,WhitePawns);
        Clear(to,WhitePieces);
        switch (promote) {
        case knight:
          Clear(to,WhiteKnights);
          TotalWhitePieces-=knight_v;
          WhiteMinors--;
          Material-=KNIGHT_VALUE;
          break;
        case bishop:
          Clear(to,WhiteBishops);
          Clear(to,BishopsQueens);
          TotalWhitePieces-=bishop_v;
          WhiteMinors--;
          Material-=BISHOP_VALUE;
          break;
        case rook:
          Clear(to,WhiteRooks);
          Clear(to,RooksQueens);
          TotalWhitePieces-=rook_v;
          WhiteMajors--;
          Material-=ROOK_VALUE;
          break;
        case queen:
          Clear(to,WhiteQueens);
          Clear(to,BishopsQueens);
          Clear(to,RooksQueens);
          TotalWhitePieces-=queen_v;
          WhiteMajors-=2;
          Material-=QUEEN_VALUE;
          break;
        }
      }
    }
    else {
      ClearSet(bit_move,BlackPawns);
      ClearSet(bit_move,BlackPieces);
      PieceOnSquare(from)=-pawn;
      if (captured == 1) {
        if(EnPassant(ply) == to) {
          TotalPieces++;
          SetRL90(to+8,OccupiedRL90);
          SetRL45(to+8,OccupiedRL45);
          SetRR45(to+8,OccupiedRR45);
          Set(to+8,WhitePawns);
          Set(to+8,WhitePieces);
          PieceOnSquare(to+8)=pawn;
          Material+=PAWN_VALUE;
          TotalWhitePawns++;
          captured=0;
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
      if (promote) {
        TotalBlackPawns++;
        Material-=PAWN_VALUE;
        Clear(to,BlackPawns);
        Clear(to,BlackPieces);
        switch (promote) {
        case knight:
          Clear(to,BlackKnights);
          TotalBlackPieces-=knight_v;
          BlackMinors--;
          Material+=KNIGHT_VALUE;
          break;
        case bishop:
          Clear(to,BlackBishops);
          Clear(to,BishopsQueens);
          TotalBlackPieces-=bishop_v;
          BlackMinors--;
          Material+=BISHOP_VALUE;
          break;
        case rook:
          Clear(to,BlackRooks);
          Clear(to,RooksQueens);
          TotalBlackPieces-=rook_v;
          BlackMajors--;
          Material+=ROOK_VALUE;
          break;
        case queen:
          Clear(to,BlackQueens);
          Clear(to,BishopsQueens);
          Clear(to,RooksQueens);
          TotalBlackPieces-=queen_v;
          BlackMajors-=2;
          Material+=QUEEN_VALUE;
          break;
        }
      }
    }
    break;

/*
********************************************************************************
*                                                                              *
*   unmake knight moves.                                                       *
*                                                                              *
********************************************************************************
*/
  case knight:
    if (wtm) {
      ClearSet(bit_move,WhiteKnights);
      ClearSet(bit_move,WhitePieces);
      PieceOnSquare(from)=knight;
    }
    else {
      ClearSet(bit_move,BlackKnights);
      ClearSet(bit_move,BlackPieces);
      PieceOnSquare(from)=-knight;
    }
    break;

/*
********************************************************************************
*                                                                              *
*   unmake bishop moves.                                                       *
*                                                                              *
********************************************************************************
*/
  case bishop:
    ClearSet(bit_move,BishopsQueens);
    if (wtm) {
      ClearSet(bit_move,WhiteBishops);
      ClearSet(bit_move,WhitePieces);
      PieceOnSquare(from)=bishop;
    }
    else {
      ClearSet(bit_move,BlackBishops);
      ClearSet(bit_move,BlackPieces);
      PieceOnSquare(from)=-bishop;
    }
    break;
/*
********************************************************************************
*                                                                              *
*   unmake rook moves.                                                         *
*                                                                              *
********************************************************************************
*/
  case rook:
    ClearSet(bit_move,RooksQueens);
    if (wtm) {
      ClearSet(bit_move,WhiteRooks);
      ClearSet(bit_move,WhitePieces);
      PieceOnSquare(from)=rook;
    }
    else {
      ClearSet(bit_move,BlackRooks);
      ClearSet(bit_move,BlackPieces);
      PieceOnSquare(from)=-rook;
    }
    break;
/*
********************************************************************************
*                                                                              *
*   unmake queen moves.                                                        *
*                                                                              *
********************************************************************************
*/
  case queen:
    ClearSet(bit_move,BishopsQueens);
    ClearSet(bit_move,RooksQueens);
    if (wtm) {
      ClearSet(bit_move,WhiteQueens);
      ClearSet(bit_move,WhitePieces);
      PieceOnSquare(from)=queen;
    }
    else {
      ClearSet(bit_move,BlackQueens);
      ClearSet(bit_move,BlackPieces);
      PieceOnSquare(from)=-queen;
    }
    break;
/*
********************************************************************************
*                                                                              *
*   unmake king moves.                                                         *
*                                                                              *
********************************************************************************
*/
  case king:
    if (wtm) {
      ClearSet(bit_move,WhitePieces);
      PieceOnSquare(from)=king;
      WhiteKingSQ=from;
      if (abs(to-from) == 2) {
        if (to == G1) {
          from=H1;
          to=F1;
          piece=rook;
          goto UnMakePieceMove;
        }
        else {
          from=A1;
          to=D1;
          piece=rook;
          goto UnMakePieceMove;
        }
      }
    }
    else {
      ClearSet(bit_move,BlackPieces);
      PieceOnSquare(from)=-king;
      BlackKingSQ=from;
      if (abs(to-from) == 2) {
        if (to == G8) {
          from=H8;
          to=F8;
          piece=rook;
          goto UnMakePieceMove;
        }
        else {
          from=A8;
          to=D8;
          piece=rook;
          goto UnMakePieceMove;
        }
      }
    }
    break;
  }
/*
********************************************************************************
*                                                                              *
*   now it is time to restore a piece that was captured.                       *
*                                                                              *
********************************************************************************
*/
  if(captured) {
    TotalPieces++;
    SetRL90(to,OccupiedRL90);
    SetRL45(to,OccupiedRL45);
    SetRR45(to,OccupiedRR45);
    switch (captured) {
/*
 ----------------------------------------------------------
|                                                          |
|   restore a captured pawn.                               |
|                                                          |
 ----------------------------------------------------------
*/
    case pawn: 
      if (wtm) {
        Set(to,BlackPawns);
        Set(to,BlackPieces);
        PieceOnSquare(to)=-pawn;
        Material-=PAWN_VALUE;
        TotalBlackPawns++;
      }
      else {
        Set(to,WhitePawns);
        Set(to,WhitePieces);
        PieceOnSquare(to)=pawn;
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
        Set(to,BlackKnights);
        Set(to,BlackPieces);
        PieceOnSquare(to)=-knight;
        TotalBlackPieces+=knight_v;
        BlackMinors++;
        Material-=KNIGHT_VALUE;
      }
      else {
        Set(to,WhiteKnights);
        Set(to,WhitePieces);
        PieceOnSquare(to)=knight;
        TotalWhitePieces+=knight_v;
        WhiteMinors++;
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
      Set(to,BishopsQueens);
      if (wtm) {
        Set(to,BlackBishops);
        Set(to,BlackPieces);
        PieceOnSquare(to)=-bishop;
        TotalBlackPieces+=bishop_v;
        BlackMinors++;
        Material-=BISHOP_VALUE;
      }
      else {
        Set(to,WhiteBishops);
        Set(to,WhitePieces);
        PieceOnSquare(to)=bishop;
        TotalWhitePieces+=bishop_v;
        WhiteMinors++;
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
      Set(to,RooksQueens);
      if (wtm) {
        Set(to,BlackRooks);
        Set(to,BlackPieces);
        PieceOnSquare(to)=-rook;
        TotalBlackPieces+=rook_v;
        BlackMajors++;
        Material-=ROOK_VALUE;
      }
      else {
        Set(to,WhiteRooks);
        Set(to,WhitePieces);
        PieceOnSquare(to)=rook;
        TotalWhitePieces+=rook_v;
        WhiteMajors++;
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
      Set(to,BishopsQueens);
      Set(to,RooksQueens);
      if (wtm) {
        Set(to,BlackQueens);
        Set(to,BlackPieces);
        PieceOnSquare(to)=-queen;
        TotalBlackPieces+=queen_v;
        BlackMajors+=2;
        Material-=QUEEN_VALUE;
      }
      else {
        Set(to,WhiteQueens);
        Set(to,WhitePieces);
        PieceOnSquare(to)=queen;
        TotalWhitePieces+=queen_v;
        WhiteMajors+=2;
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
      Print(128,"captured a king\n");
      Print(128,"piece=%d,from=%d,to=%d,captured=%d\n",
            piece,from,to,captured);
      Print(128,"ply=%d\n",ply);
      if (log_file) DisplayChessBoard(log_file,tree->pos);
    }
  }
#if defined(DEBUG)
  ValidatePosition(tree,ply,move,"UnMakeMove(2)");
#endif
  return;
}
