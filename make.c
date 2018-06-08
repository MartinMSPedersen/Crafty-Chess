#include <stdio.h>
#include <stdlib.h>
#include "types.h"
#include "function.h"
#include "data.h"
#include "evaluate.h"
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
  MakeMoveCopy(&position[ply+1], &position[ply]);
/*
  position[ply+1]=position[ply];
*/
  if (EnPassantTarget(ply+1)) {
    HashEP(FirstOne(EnPassantTarget(ply+1)),HashKey(ply+1));
    EnPassantTarget(ply+1)=0;
  }
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
        if (!And(BlackPawns(ply),set_mask[To(move)])) move&=~(7<<15);
      }
      else {
        if (!And(WhitePawns(ply),set_mask[To(move)])) move&=~(7<<15);
      }
    }
    position[ply+1].rule_50_moves=0;
    break;
  case knight:
    MakeMoveKnight(ply,From(move),To(move),wtm);
    break;
  case bishop:
    MakeMoveBishop(ply,From(move),To(move),wtm);
    break;
  case rook:
    MakeMoveRook(ply,From(move),To(move),wtm);
    break;
  case queen:
    MakeMoveQueen(ply,From(move),To(move),wtm);
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
    position[ply+1].rule_50_moves=0;
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
        Clear(To(move),BlackPawns(ply+1));
        Clear(To(move),BlackPieces(ply+1));
        HashPB(To(move),HashKey(ply+1));
        HashPB(To(move),PawnHashKey(ply+1));
        Material(ply+1)+=PAWN_VALUE;
        TotalBlackPawns(ply+1)--;
      }
      else {
        Clear(To(move),WhitePawns(ply+1));
        Clear(To(move),WhitePieces(ply+1));
        HashPW(To(move),HashKey(ply+1));
        HashPW(To(move),PawnHashKey(ply+1));
        Material(ply+1)-=PAWN_VALUE;
        TotalWhitePawns(ply+1)--;
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
        Clear(To(move),BlackKnights(ply+1));
        Clear(To(move),BlackPieces(ply+1));
        HashNB(To(move),HashKey(ply+1));
        TotalBlackPieces(ply+1)-=knight_v;
        Material(ply+1)+=KNIGHT_VALUE;
      }
      else {
        Clear(To(move),WhiteKnights(ply+1));
        Clear(To(move),WhitePieces(ply+1));
        HashNW(To(move),HashKey(ply+1));
        TotalWhitePieces(ply+1)-=knight_v;
        Material(ply+1)-=KNIGHT_VALUE;
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
      if (SlidingDiag(Piece(move))) Set(To(move),BishopsQueens(ply+1));
      else Clear(To(move),BishopsQueens(ply+1));
      if (wtm) {
        Clear(To(move),BlackBishops(ply+1));
        Clear(To(move),BlackPieces(ply+1));
        HashBB(To(move),HashKey(ply+1));
        TotalBlackPieces(ply+1)-=bishop_v;
        Material(ply+1)+=BISHOP_VALUE;
      }
      else {
        Clear(To(move),WhiteBishops(ply+1));
        Clear(To(move),WhitePieces(ply+1));
        HashBW(To(move),HashKey(ply+1));
        TotalWhitePieces(ply+1)-=bishop_v;
        Material(ply+1)-=BISHOP_VALUE;
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
      if (SlidingRow(Piece(move))) Set(To(move),RooksQueens(ply+1));
      else Clear(To(move),RooksQueens(ply+1));
      if (wtm) {
        Clear(To(move),BlackRooks(ply+1));
        Clear(To(move),BlackPieces(ply+1));
        HashRB(To(move),HashKey(ply+1));
        if (BlackCastle(ply)) {
          if ((To(move) == 56) && (BlackCastle(ply+1)&2)) {
            BlackCastle(ply+1)&=1;
            HashCastleB(1,HashKey(ply+1));
          }
          else if ((To(move) == 63) && (BlackCastle(ply+1)&1)) {
            BlackCastle(ply+1)&=2;
            HashCastleB(0,HashKey(ply+1));
          }
        }
        TotalBlackPieces(ply+1)-=rook_v;
        Material(ply+1)+=ROOK_VALUE;
      }
      else {
        Clear(To(move),WhiteRooks(ply+1));
        Clear(To(move),WhitePieces(ply+1));
        HashRW(To(move),HashKey(ply+1));
        if (WhiteCastle(ply)) {
          if ((To(move) == 0) && (WhiteCastle(ply+1)&2)) {
            WhiteCastle(ply+1)&=1;
            HashCastleW(1,HashKey(ply+1));
          }
          else if ((To(move) == 7) && (WhiteCastle(ply+1)&1)) {
            WhiteCastle(ply+1)&=2;
            HashCastleW(0,HashKey(ply+1));
          }
        }
        TotalWhitePieces(ply+1)-=rook_v;
        Material(ply+1)-=ROOK_VALUE;
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
      if (Piece(move) == queen) {
        Set(To(move),BishopsQueens(ply+1));
        Set(To(move),RooksQueens(ply+1));
      }
      else if (Piece(move) == rook) {
        Set(To(move),RooksQueens(ply+1));
        Clear(To(move),BishopsQueens(ply+1));
      }
      else if (Piece(move) == bishop) {
        Set(To(move),BishopsQueens(ply+1));
        Clear(To(move),RooksQueens(ply+1));
      }
      else {
        Clear(To(move),BishopsQueens(ply+1));
        Clear(To(move),RooksQueens(ply+1));
      }
      if (wtm) {
        Clear(To(move),BlackQueens(ply+1));
        Clear(To(move),BlackPieces(ply+1));
        HashQB(To(move),HashKey(ply+1));
        TotalBlackPieces(ply+1)-=queen_v;
        Material(ply+1)+=QUEEN_VALUE;
      }
      else {
        Clear(To(move),WhiteQueens(ply+1));
        Clear(To(move),WhitePieces(ply+1));
        HashQW(To(move),HashKey(ply+1));
        TotalWhitePieces(ply+1)-=queen_v;
        Material(ply+1)-=QUEEN_VALUE;
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
      if (log_file) DisplayChessBoard(log_file,position[ply]);
    }
  }
  position[ply+1].rule_50_moves++;
  return;
}

static void MakeMoveCopy(CHESS_POSITION *to, CHESS_POSITION *from)
{
  BITBOARD *tp = (BITBOARD *) to;
  BITBOARD *fp = (BITBOARD *) from;

  tp[ 0] = fp[ 0];
  tp[ 1] = fp[ 1];
  tp[ 2] = fp[ 2];
  tp[ 3] = fp[ 3];
  tp[ 4] = fp[ 4];
  tp[ 5] = fp[ 5];
  tp[ 6] = fp[ 6];
  tp[ 7] = fp[ 7];
  tp[ 8] = fp[ 8];
  tp[ 9] = fp[ 9];
  tp[10] = fp[10];
  tp[11] = fp[11];
  tp[12] = fp[12];
  tp[13] = fp[13];
  tp[14] = fp[14];
  tp[15] = fp[15];
  tp[16] = fp[16];
  tp[17] = fp[17];
  tp[18] = fp[18];
  tp[19] = fp[19];
  tp[20] = fp[20];
  tp[21] = fp[21];
  tp[22] = fp[22];
  tp[23] = fp[23];
  tp[24] = fp[24];
  tp[25] = fp[25];
  tp[26] = fp[26];
  tp[27] = fp[27];
  tp[28] = fp[28];
  tp[29] = fp[29];
  tp[30] = fp[30];
  tp[31] = fp[31];
}

/*
********************************************************************************
*                                                                              *
*   make bishop moves.                                                         *
*                                                                              *
********************************************************************************
*/
void MakeMoveBishop(int ply, int from, int to, int wtm)
{
/*
 --------------------------------------------------------------------
|                                                                    |
|  first, update the occupied-square bitboards, of which there are   |
|  several.                                                          |
|                                                                    |
 --------------------------------------------------------------------
*/
  ClearSet(bit_move,BishopsQueens(ply+1));
  ClearRL90(from,OccupiedRL90(ply+1));
  ClearRL45(from,OccupiedRL45(ply+1));
  ClearRR45(from,OccupiedRR45(ply+1));
  SetRL90(to,OccupiedRL90(ply+1));
  SetRL45(to,OccupiedRL45(ply+1));
  SetRR45(to,OccupiedRR45(ply+1));
  if (wtm) {
    ClearSet(bit_move,WhiteBishops(ply+1));
    ClearSet(bit_move,WhitePieces(ply+1));
    HashBW(from,HashKey(ply+1));
    HashBW(to,HashKey(ply+1));
    PieceOnSquare(ply+1,from)=0;
    PieceOnSquare(ply+1,to)=bishop;
  }
  else {
    ClearSet(bit_move,BlackBishops(ply+1));
    ClearSet(bit_move,BlackPieces(ply+1));
    HashBB(from,HashKey(ply+1));
    HashBB(to,HashKey(ply+1));
    PieceOnSquare(ply+1,from)=0;
    PieceOnSquare(ply+1,to)=-bishop;
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
  ClearRL90(from,OccupiedRL90(ply+1));
  ClearRL45(from,OccupiedRL45(ply+1));
  ClearRR45(from,OccupiedRR45(ply+1));
  SetRL90(to,OccupiedRL90(ply+1));
  SetRL45(to,OccupiedRL45(ply+1));
  SetRR45(to,OccupiedRR45(ply+1));
  if (wtm) {
    ClearSet(bit_move,WhiteKing(ply+1));
    ClearSet(bit_move,WhitePieces(ply+1));
    HashKW(from,HashKey(ply+1));
    HashKW(to,HashKey(ply+1));
    PieceOnSquare(ply+1,from)=0;
    PieceOnSquare(ply+1,to)=king;
    if (WhiteCastle(ply)) {
      if (WhiteCastle(ply+1)&2) HashCastleW(1,HashKey(ply+1));
      if (WhiteCastle(ply+1)&1) HashCastleW(0,HashKey(ply+1));
      WhiteCastle(ply+1)=0;
    }
    WhiteKingSQ(ply+1)=to;
    if (abs(to-from) == 2)
      if (to == 6) {
        bit_move=Or(set_mask[5],set_mask[7]);
        MakeMoveRook(ply,7,5,wtm);
      }
      else {
        bit_move=Or(set_mask[0],set_mask[3]);
        MakeMoveRook(ply,0,3,wtm);
      }
  }
  else {
    ClearSet(bit_move,BlackKing(ply+1));
    ClearSet(bit_move,BlackPieces(ply+1));
    HashKB(from,HashKey(ply+1));
    HashKB(to,HashKey(ply+1));
    PieceOnSquare(ply+1,from)=0;
    PieceOnSquare(ply+1,to)=-king;
    BlackKingSQ(ply+1)=to;
    if (BlackCastle(ply+1)) {
      if (BlackCastle(ply+1)&2) HashCastleB(1,HashKey(ply+1));
      if (BlackCastle(ply+1)&1) HashCastleB(0,HashKey(ply+1));
      BlackCastle(ply+1)=0;
    }
    if (abs(to-from) == 2)
      if (to == 62) {
        bit_move=Or(set_mask[61],set_mask[63]);
        MakeMoveRook(ply,63,61,wtm);
      }
      else {
        bit_move=Or(set_mask[56],set_mask[59]);
        MakeMoveRook(ply,56,59,wtm);
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
void MakeMoveKnight(int ply, int from, int to, int wtm)
{
/*
 --------------------------------------------------------------------
|                                                                    |
|  first, update the occupied-square bitboards, of which there are   |
|  several.                                                          |
|                                                                    |
 --------------------------------------------------------------------
*/
  ClearRL90(from,OccupiedRL90(ply+1));
  ClearRL45(from,OccupiedRL45(ply+1));
  ClearRR45(from,OccupiedRR45(ply+1));
  SetRL90(to,OccupiedRL90(ply+1));
  SetRL45(to,OccupiedRL45(ply+1));
  SetRR45(to,OccupiedRR45(ply+1));
  if (wtm) {
    ClearSet(bit_move,WhiteKnights(ply+1));
    ClearSet(bit_move,WhitePieces(ply+1));
    HashNW(from,HashKey(ply+1));
    HashNW(to,HashKey(ply+1));
    PieceOnSquare(ply+1,from)=0;
    PieceOnSquare(ply+1,to)=knight;
  }
  else {
    ClearSet(bit_move,BlackKnights(ply+1));
    ClearSet(bit_move,BlackPieces(ply+1));
    HashNB(from,HashKey(ply+1));
    HashNB(to,HashKey(ply+1));
    PieceOnSquare(ply+1,from)=0;
    PieceOnSquare(ply+1,to)=-knight;
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
  ClearRL90(from,OccupiedRL90(ply+1));
  ClearRL45(from,OccupiedRL45(ply+1));
  ClearRR45(from,OccupiedRR45(ply+1));
  SetRL90(to,OccupiedRL90(ply+1));
  SetRL45(to,OccupiedRL45(ply+1));
  SetRR45(to,OccupiedRR45(ply+1));
  if (wtm) {
    ClearSet(bit_move,WhitePawns(ply+1));
    ClearSet(bit_move,WhitePieces(ply+1));
    HashPW(from,HashKey(ply+1));
    HashPW(from,PawnHashKey(ply+1));
    HashPW(to,HashKey(ply+1));
    HashPW(to,PawnHashKey(ply+1));
    PieceOnSquare(ply+1,from)=0;
    PieceOnSquare(ply+1,to)=pawn;
    if (Captured == 1) {
      if(!And(BlackPawns(ply+1),set_mask[to])) {
        ClearRL90(to-8,OccupiedRL90(ply+1));
        ClearRL45(to-8,OccupiedRL45(ply+1));
        ClearRR45(to-8,OccupiedRR45(ply+1));
        Clear(to-8,BlackPawns(ply+1));
        Clear(to-8,BlackPieces(ply+1));
        HashPB(to-8,HashKey(ply+1));
        HashPB(to-8,PawnHashKey(ply+1));
        PieceOnSquare(ply+1,to-8)=0;
        Material(ply+1)+=PAWN_VALUE;
        TotalBlackPawns(ply+1)--;
      }
    }
/*
 --------------------------------------------------------------------
|                                                                    |
|  if this is a pawn promotion, remove the pawn from the pawn  |
|  then update the correct piece board to reflect the piece just     |
|  created.                                                          |
|                                                                    |
 --------------------------------------------------------------------
*/
    if (Promote) {
      TotalWhitePawns(ply+1)--;
      Material(ply+1)-=PAWN_VALUE;
      Clear(to,WhitePawns(ply+1));
      HashPW(to,HashKey(ply+1));
      HashPW(to,PawnHashKey(ply+1));
      switch (Promote) {
      case knight:
        Set(to,WhiteKnights(ply+1));
        HashNW(to,HashKey(ply+1));
        PieceOnSquare(ply+1,to)=knight;
        TotalWhitePieces(ply+1)+=knight_v;
        Material(ply+1)+=KNIGHT_VALUE;
        break;
      case bishop:
        Set(to,WhiteBishops(ply+1));
        Set(to,BishopsQueens(ply+1));
        HashBW(to,HashKey(ply+1));
        PieceOnSquare(ply+1,to)=bishop;
        TotalWhitePieces(ply+1)+=bishop_v;
        Material(ply+1)+=BISHOP_VALUE;
        break;
      case rook:
        Set(to,WhiteRooks(ply+1));
        Set(to,RooksQueens(ply+1));
        HashRW(to,HashKey(ply+1));
        PieceOnSquare(ply+1,to)=rook;
        TotalWhitePieces(ply+1)+=rook_v;
        Material(ply+1)+=ROOK_VALUE;
        break;
      case queen:
        Set(to,WhiteQueens(ply+1));
        Set(to,BishopsQueens(ply+1));
        Set(to,RooksQueens(ply+1));
        HashQW(to,HashKey(ply+1));
        PieceOnSquare(ply+1,to)=queen;
        TotalWhitePieces(ply+1)+=queen_v;
        Material(ply+1)+=QUEEN_VALUE;
        break;
      }
    }
    else 
      if ((to-from == 16) && And(mask_enpassant_test[to],BlackPawns(ply+1))) {
        EnPassantTarget(ply+1)=set_mask[to-8];
        HashEP(to-8,HashKey(ply+1));
      }
  }
  else {
    ClearSet(bit_move,BlackPawns(ply+1));
    ClearSet(bit_move,BlackPieces(ply+1));
    HashPB(from,HashKey(ply+1));
    HashPB(from,PawnHashKey(ply+1));
    HashPB(to,HashKey(ply+1));
    HashPB(to,PawnHashKey(ply+1));
    PieceOnSquare(ply+1,from)=0;
    PieceOnSquare(ply+1,to)=-pawn;
    if (Captured == 1) {
      if(!And(WhitePawns(ply+1),set_mask[to])) {
        ClearRL90(to+8,OccupiedRL90(ply+1));
        ClearRL45(to+8,OccupiedRL45(ply+1));
        ClearRR45(to+8,OccupiedRR45(ply+1));
        Clear(to+8,WhitePawns(ply+1));
        Clear(to+8,WhitePieces(ply+1));
        HashPW(to+8,HashKey(ply+1));
        HashPW(to+8,PawnHashKey(ply+1));
        PieceOnSquare(ply+1,to+8)=0;
        Material(ply+1)-=PAWN_VALUE;
        TotalWhitePawns(ply+1)--;
      }
    }
/*
 --------------------------------------------------------------------
|                                                                    |
|  if this is a pawn promotion, remove the pawn from the pawn  |
|  then update the correct piece board to reflect the piece just     |
|  created.                                                          |
|                                                                    |
 --------------------------------------------------------------------
*/
    if (Promote) {
      TotalBlackPawns(ply+1)--;
      Material(ply+1)+=PAWN_VALUE;
      Clear(to,BlackPawns(ply+1));
      HashPB(to,HashKey(ply+1));
      HashPB(to,PawnHashKey(ply+1));
      switch (Promote) {
      case knight:
        Set(to,BlackKnights(ply+1));
        HashNB(to,HashKey(ply+1));
        PieceOnSquare(ply+1,to)=-knight;
        TotalBlackPieces(ply+1)+=knight_v;
        Material(ply+1)-=KNIGHT_VALUE;
        break;
      case bishop:
        Set(to,BlackBishops(ply+1));
        Set(to,BishopsQueens(ply+1));
        HashBB(to,HashKey(ply+1));
        PieceOnSquare(ply+1,to)=-bishop;
        TotalBlackPieces(ply+1)+=bishop_v;
        Material(ply+1)-=BISHOP_VALUE;
        break;
      case rook:
        Set(to,BlackRooks(ply+1));
        Set(to,RooksQueens(ply+1));
        HashRB(to,HashKey(ply+1));
        PieceOnSquare(ply+1,to)=-rook;
        TotalBlackPieces(ply+1)+=rook_v;
        Material(ply+1)-=ROOK_VALUE;
        break;
      case queen:
        Set(to,BlackQueens(ply+1));
        Set(to,BishopsQueens(ply+1));
        Set(to,RooksQueens(ply+1));
        HashQB(to,HashKey(ply+1));
        PieceOnSquare(ply+1,to)=-queen;
        TotalBlackPieces(ply+1)+=queen_v;
        Material(ply+1)-=QUEEN_VALUE;
        break;
      }
    }
    else 
      if ((from-to == 16) && And(mask_enpassant_test[to],WhitePawns(ply+1))) {
        EnPassantTarget(ply+1)=set_mask[to+8];
        HashEP(to+8,HashKey(ply+1));
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
void MakeMoveQueen(int ply, int from, int to, int wtm)
{
/*
 --------------------------------------------------------------------
|                                                                    |
|  first, update the occupied-square bitboards, of which there are   |
|  several.                                                          |
|                                                                    |
 --------------------------------------------------------------------
*/
  ClearSet(bit_move,BishopsQueens(ply+1));
  ClearSet(bit_move,RooksQueens(ply+1));
  ClearRL90(from,OccupiedRL90(ply+1));
  ClearRL45(from,OccupiedRL45(ply+1));
  ClearRR45(from,OccupiedRR45(ply+1));
  SetRL90(to,OccupiedRL90(ply+1));
  SetRL45(to,OccupiedRL45(ply+1));
  SetRR45(to,OccupiedRR45(ply+1));
  if (wtm) {
    ClearSet(bit_move,WhiteQueens(ply+1));
    ClearSet(bit_move,WhitePieces(ply+1));
    HashQW(from,HashKey(ply+1));
    HashQW(to,HashKey(ply+1));
    PieceOnSquare(ply+1,from)=0;
    PieceOnSquare(ply+1,to)=queen;
  }
  else {
    ClearSet(bit_move,BlackQueens(ply+1));
    ClearSet(bit_move,BlackPieces(ply+1));
    HashQB(from,HashKey(ply+1));
    HashQB(to,HashKey(ply+1));
    PieceOnSquare(ply+1,from)=0;
    PieceOnSquare(ply+1,to)=-queen;
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
  ClearSet(bit_move,RooksQueens(ply+1));
  ClearRL90(from,OccupiedRL90(ply+1));
  ClearRL45(from,OccupiedRL45(ply+1));
  ClearRR45(from,OccupiedRR45(ply+1));
  SetRL90(to,OccupiedRL90(ply+1));
  SetRL45(to,OccupiedRL45(ply+1));
  SetRR45(to,OccupiedRR45(ply+1));
  if (wtm) {
    ClearSet(bit_move,WhiteRooks(ply+1));
    ClearSet(bit_move,WhitePieces(ply+1));
    HashRW(from,HashKey(ply+1));
    HashRW(to,HashKey(ply+1));
    PieceOnSquare(ply+1,from)=0;
    PieceOnSquare(ply+1,to)=rook;
    if (WhiteCastle(ply+1)) {
      if ((from == 0) && (WhiteCastle(ply+1)&2)) {
        WhiteCastle(ply+1)&=1;
        HashCastleW(1,HashKey(ply+1));
      }
      else if ((from == 7) && (WhiteCastle(ply+1)&1)) {
        WhiteCastle(ply+1)&=2;
        HashCastleW(0,HashKey(ply+1));
      }
    }
  }
  else {
    ClearSet(bit_move,BlackRooks(ply+1));
    ClearSet(bit_move,BlackPieces(ply+1));
    HashRB(from,HashKey(ply+1));
    HashRB(to,HashKey(ply+1));
    PieceOnSquare(ply+1,from)=0;
    PieceOnSquare(ply+1,to)=-rook;
    if (BlackCastle(ply+1)) {
      if ((from == 56) && (BlackCastle(ply+1)&2)) {
        BlackCastle(ply+1)&=1;
        HashCastleB(1,HashKey(ply+1));
      }
      else if ((from == 63) && (BlackCastle(ply+1)&1)) {
        BlackCastle(ply+1)&=2;
        HashCastleB(0,HashKey(ply+1));
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
  if ((Piece(move) == pawn) || (Captured(move)) ||
      (WhiteCastle(0) != WhiteCastle(1)) ||
      (BlackCastle(0) != BlackCastle(1))) {
    if (wtm)
      repetition_head=0;
    else {
      repetition_head=1;
      repetition_list[1]=0;
    }
    position[1].rule_50_moves=0;
  }
  position[0]=position[1];
  repetition_list[++repetition_head]=HashKey(0);
}
