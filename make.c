#include <stdio.h>
#include <stdlib.h>
#include "chess.h"
#include "data.h"

/* last modified 03/11/98 */
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
void MakeMove(TREE *tree, int ply, int move, int wtm)
{
  register int piece, from, to, captured, promote;
  BITBOARD bit_move;
/*
 ----------------------------------------------------------
|                                                          |
|   first, clear the EnPassant_Target bit-mask.  moving a  |
|   pawn two ranks will set it later in MakeMove().        |
|                                                          |
 ----------------------------------------------------------
*/
#if defined(DEBUG)
  ValidatePosition(tree,ply,move,"MakeMove(1)");
#endif
  tree->position[ply+1]=tree->position[ply];
  tree->save_hash_key[ply]=HashKey;
  tree->save_pawn_hash_key[ply]=PawnHashKey;
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
  piece=Piece(move);
  from=From(move);
  to=To(move);
  captured=Captured(move);
  promote=Promote(move);
MakePieceMove:
  ClearRL90(from,OccupiedRL90);
  ClearRL45(from,OccupiedRL45);
  ClearRR45(from,OccupiedRR45);
  SetRL90(to,OccupiedRL90);
  SetRL45(to,OccupiedRL45);
  SetRR45(to,OccupiedRR45);
  bit_move=Or(SetMask(from),SetMask(to));
  PieceOnSquare(from)=0;
  switch (piece) {
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
  case pawn:
    if (wtm) {
      ClearSet(bit_move,WhitePawns);
      ClearSet(bit_move,WhitePieces);
      HashPW(from,HashKey);
      HashPW32(from,PawnHashKey);
      HashPW(to,HashKey);
      HashPW32(to,PawnHashKey);
      if (captured == 1) {
        if(!PieceOnSquare(to)) {
          ClearRL90(to-8,OccupiedRL90);
          ClearRL45(to-8,OccupiedRL45);
          ClearRR45(to-8,OccupiedRR45);
          Clear(to-8,BlackPawns);
          Clear(to-8,BlackPieces);
          HashPB(to-8,HashKey);
          HashPB32(to-8,PawnHashKey);
          PieceOnSquare(to-8)=0;
          Material+=PAWN_VALUE;
          TotalBlackPawns--;
          TotalPieces--;
          captured=0;
        }
      }
      PieceOnSquare(to)=pawn;
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
        TotalWhitePawns--;
        Material-=PAWN_VALUE;
        Clear(to,WhitePawns);
        HashPW(to,HashKey);
        HashPW32(to,PawnHashKey);
        switch (promote) {
        case knight:
          Set(to,WhiteKnights);
          HashNW(to,HashKey);
          PieceOnSquare(to)=knight;
          TotalWhitePieces+=knight_v;
          WhiteMinors++;
          Material+=KNIGHT_VALUE;
          break;
        case bishop:
          Set(to,WhiteBishops);
          Set(to,BishopsQueens);
          HashBW(to,HashKey);
          PieceOnSquare(to)=bishop;
          TotalWhitePieces+=bishop_v;
          WhiteMinors++;
          Material+=BISHOP_VALUE;
          break;
        case rook:
          Set(to,WhiteRooks);
          Set(to,RooksQueens);
          HashRW(to,HashKey);
          PieceOnSquare(to)=rook;
          TotalWhitePieces+=rook_v;
          WhiteMajors++;
          Material+=ROOK_VALUE;
          break;
        case queen:
          Set(to,WhiteQueens);
          Set(to,BishopsQueens);
          Set(to,RooksQueens);
          HashQW(to,HashKey);
          PieceOnSquare(to)=queen;
          TotalWhitePieces+=queen_v;
          WhiteMajors+=2;
          Material+=QUEEN_VALUE;
          break;
        }
      }
      else 
        if (((to-from) == 16) && And(mask_eptest[to],BlackPawns)) {
          EnPassant(ply+1)=to-8;
          HashEP(to-8,HashKey);
        }
    }
    else {
      ClearSet(bit_move,BlackPawns);
      ClearSet(bit_move,BlackPieces);
      HashPB(from,HashKey);
      HashPB32(from,PawnHashKey);
      HashPB(to,HashKey);
      HashPB32(to,PawnHashKey);
      if (captured == 1) {
        if(!PieceOnSquare(to)) {
          ClearRL90(to+8,OccupiedRL90);
          ClearRL45(to+8,OccupiedRL45);
          ClearRR45(to+8,OccupiedRR45);
          Clear(to+8,WhitePawns);
          Clear(to+8,WhitePieces);
          HashPW(to+8,HashKey);
          HashPW32(to+8,PawnHashKey);
          PieceOnSquare(to+8)=0;
          Material-=PAWN_VALUE;
          TotalWhitePawns--;
          TotalPieces--;
          captured=0;
        }
      }
      PieceOnSquare(to)=-pawn;
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
        TotalBlackPawns--;
        Material+=PAWN_VALUE;
        Clear(to,BlackPawns);
        HashPB(to,HashKey);
        HashPB32(to,PawnHashKey);
        switch (promote) {
        case knight:
          Set(to,BlackKnights);
          HashNB(to,HashKey);
          PieceOnSquare(to)=-knight;
          TotalBlackPieces+=knight_v;
          BlackMinors++;
          Material-=KNIGHT_VALUE;
          break;
        case bishop:
          Set(to,BlackBishops);
          Set(to,BishopsQueens);
          HashBB(to,HashKey);
          PieceOnSquare(to)=-bishop;
          TotalBlackPieces+=bishop_v;
          BlackMinors++;
          Material-=BISHOP_VALUE;
          break;
        case rook:
          Set(to,BlackRooks);
          Set(to,RooksQueens);
          HashRB(to,HashKey);
          PieceOnSquare(to)=-rook;
          TotalBlackPieces+=rook_v;
          BlackMajors++;
          Material-=ROOK_VALUE;
          break;
        case queen:
          Set(to,BlackQueens);
          Set(to,BishopsQueens);
          Set(to,RooksQueens);
          HashQB(to,HashKey);
          PieceOnSquare(to)=-queen;
          TotalBlackPieces+=queen_v;
          BlackMajors+=2;
          Material-=QUEEN_VALUE;
          break;
        }
      }
      else 
        if (((from-to) == 16) && And(mask_eptest[to],WhitePawns)) {
          EnPassant(ply+1)=to+8;
          HashEP(to+8,HashKey);
        }
    }
    Rule50Moves(ply+1)=0;
    break;
/*
********************************************************************************
*                                                                              *
*   make knight moves.                                                         *
*                                                                              *
********************************************************************************
*/
  case knight:
    if (wtm) {
      ClearSet(bit_move,WhiteKnights);
      ClearSet(bit_move,WhitePieces);
      HashNW(from,HashKey);
      HashNW(to,HashKey);
      PieceOnSquare(to)=knight;
    }
    else {
      ClearSet(bit_move,BlackKnights);
      ClearSet(bit_move,BlackPieces);
      HashNB(from,HashKey);
      HashNB(to,HashKey);
      PieceOnSquare(to)=-knight;
    }
    break;
/*
********************************************************************************
*                                                                              *
*   make bishop moves.                                                         *
*                                                                              *
********************************************************************************
*/
  case bishop:
    ClearSet(bit_move,BishopsQueens);
    if (wtm) {
      ClearSet(bit_move,WhiteBishops);
      ClearSet(bit_move,WhitePieces);
      HashBW(from,HashKey);
      HashBW(to,HashKey);
      PieceOnSquare(to)=bishop;
    }
    else {
      ClearSet(bit_move,BlackBishops);
      ClearSet(bit_move,BlackPieces);
      HashBB(from,HashKey);
      HashBB(to,HashKey);
      PieceOnSquare(to)=-bishop;
    }
    break;
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
  case rook:
    ClearSet(bit_move,RooksQueens);
    if (wtm) {
      ClearSet(bit_move,WhiteRooks);
      ClearSet(bit_move,WhitePieces);
      HashRW(from,HashKey);
      HashRW(to,HashKey);
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
    break;
/*
********************************************************************************
*                                                                              *
*   make queen moves                                                           *
*                                                                              *
********************************************************************************
*/
  case queen:
    ClearSet(bit_move,BishopsQueens);
    ClearSet(bit_move,RooksQueens);
    if (wtm) {
      ClearSet(bit_move,WhiteQueens);
      ClearSet(bit_move,WhitePieces);
      HashQW(from,HashKey);
      HashQW(to,HashKey);
      PieceOnSquare(to)=queen;
    }
    else {
      ClearSet(bit_move,BlackQueens);
      ClearSet(bit_move,BlackPieces);
      HashQB(from,HashKey);
      HashQB(to,HashKey);
      PieceOnSquare(to)=-queen;
    }
    break;
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
  case king:
    if (wtm) {
      ClearSet(bit_move,WhitePieces);
      HashKW(from,HashKey);
      HashKW(to,HashKey);
      PieceOnSquare(to)=king;
      WhiteKingSQ=to;
      if (WhiteCastle(ply) > 0) {
        if (WhiteCastle(ply+1)&2) HashCastleW(1,HashKey);
        if (WhiteCastle(ply+1)&1) HashCastleW(0,HashKey);
        if (abs(to-from) == 2) WhiteCastle(ply+1)=-4;
        else WhiteCastle(ply+1)=0;
        if (abs(to-from) == 2) {
          if (to == G1) {
            from=H1;
            to=F1;
            piece=rook;
            goto MakePieceMove;
          }
          else {
            from=A1;
            to=D1;
            piece=rook;
            goto MakePieceMove;
          }
        }
      }
    }
    else {
      ClearSet(bit_move,BlackPieces);
      HashKB(from,HashKey);
      HashKB(to,HashKey);
      PieceOnSquare(to)=-king;
      BlackKingSQ=to;
      if (BlackCastle(ply+1) > 0) {
        if (BlackCastle(ply+1)&2) HashCastleB(1,HashKey);
        if (BlackCastle(ply+1)&1) HashCastleB(0,HashKey);
        if (abs(to-from) == 2) BlackCastle(ply+1)=-4;
        else BlackCastle(ply+1)=0;
        if (abs(to-from) == 2) {
          if (to == G8) {
            from=H8;
            to=F8;
            piece=rook;
            goto MakePieceMove;
          }
          else {
            from=A8;
            to=D8;
            piece=rook;
            goto MakePieceMove;
          }
        }
      }
    }
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
  if(captured) {
    Rule50Moves(ply+1)=0;
    TotalPieces--;
    if (promote) piece=promote;
    switch (captured) {
/*
 ----------------------------------------------------------
|                                                          |
|   remove a captured pawn.                                |
|                                                          |
 ----------------------------------------------------------
*/
    case pawn: 
      if (wtm) {
        Clear(to,BlackPawns);
        Clear(to,BlackPieces);
        HashPB(to,HashKey);
        HashPB32(to,PawnHashKey);
        Material+=PAWN_VALUE;
        TotalBlackPawns--;
      }
      else {
        Clear(to,WhitePawns);
        Clear(to,WhitePieces);
        HashPW(to,HashKey);
        HashPW32(to,PawnHashKey);
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
        Clear(to,BlackKnights);
        Clear(to,BlackPieces);
        HashNB(to,HashKey);
        TotalBlackPieces-=knight_v;
        BlackMinors--;
        Material+=KNIGHT_VALUE;
      }
      else {
        Clear(to,WhiteKnights);
        Clear(to,WhitePieces);
        HashNW(to,HashKey);
        TotalWhitePieces-=knight_v;
        WhiteMinors--;
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
      if (SlidingDiag(piece)) Set(to,BishopsQueens);
      else Clear(to,BishopsQueens);
      if (wtm) {
        Clear(to,BlackBishops);
        Clear(to,BlackPieces);
        HashBB(to,HashKey);
        TotalBlackPieces-=bishop_v;
        BlackMinors--;
        Material+=BISHOP_VALUE;
      }
      else {
        Clear(to,WhiteBishops);
        Clear(to,WhitePieces);
        HashBW(to,HashKey);
        TotalWhitePieces-=bishop_v;
        WhiteMinors--;
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
      if (SlidingRow(piece)) Set(to,RooksQueens);
      else Clear(to,RooksQueens);
      if (wtm) {
        Clear(to,BlackRooks);
        Clear(to,BlackPieces);
        HashRB(to,HashKey);
        if (BlackCastle(ply) > 0) {
          if ((to == 56) && (BlackCastle(ply+1)&2)) {
            BlackCastle(ply+1)&=1;
            HashCastleB(1,HashKey);
          }
          else if ((to == 63) && (BlackCastle(ply+1)&1)) {
            BlackCastle(ply+1)&=2;
            HashCastleB(0,HashKey);
          }
        }
        TotalBlackPieces-=rook_v;
        BlackMajors--;
        Material+=ROOK_VALUE;
      }
      else {
        Clear(to,WhiteRooks);
        Clear(to,WhitePieces);
        HashRW(to,HashKey);
        if (WhiteCastle(ply) > 0) {
          if ((to == 0) && (WhiteCastle(ply+1)&2)) {
            WhiteCastle(ply+1)&=1;
            HashCastleW(1,HashKey);
          }
          else if ((to == 7) && (WhiteCastle(ply+1)&1)) {
            WhiteCastle(ply+1)&=2;
            HashCastleW(0,HashKey);
          }
        }
        TotalWhitePieces-=rook_v;
        WhiteMajors--;
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
      if (SlidingDiag(piece)) Set(to,BishopsQueens);
      else Clear(to,BishopsQueens);
      if (SlidingRow(piece)) Set(to,RooksQueens);
      else Clear(to,RooksQueens);
      if (wtm) {
        Clear(to,BlackQueens);
        Clear(to,BlackPieces);
        HashQB(to,HashKey);
        TotalBlackPieces-=queen_v;
        BlackMajors-=2;
        Material+=QUEEN_VALUE;
      }
      else {
        Clear(to,WhiteQueens);
        Clear(to,WhitePieces);
        HashQW(to,HashKey);
        TotalWhitePieces-=queen_v;
        WhiteMajors-=2;
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
      Print(128,"captured a king\n");
      Print(128,"piece=%d,from=%d,to=%d,captured=%d\n",
            piece,from,to,captured);
      Print(128,"ply=%d\n",ply);
      if (log_file) DisplayChessBoard(log_file,tree->pos);
    }
  }
#if defined(DEBUG)
  ValidatePosition(tree,ply+1,move,"MakeMove(2)");
#endif
  return;
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
void MakeMoveRoot(TREE *tree, int move, int wtm)
{
/*
 ----------------------------------------------------------
|                                                          |
|   first, make the move and replace position[0] with the  |
|   new position.                                          |
|                                                          |
 ----------------------------------------------------------
*/
  MakeMove(tree, 0,move,wtm);
/*
 ----------------------------------------------------------
|                                                          |
|   now, if this is a non-reversible move, reset the       |
|   repetition list pointer to start the count over.       |
|                                                          |
 ----------------------------------------------------------
*/
  if (Rule50Moves(1) == 0) {
    tree->rephead_b=tree->replist_b;
    tree->rephead_w=tree->replist_w;
  }
  WhiteCastle(1)=Max(0,WhiteCastle(1));
  BlackCastle(1)=Max(0,BlackCastle(1));
  tree->position[0]=tree->position[1];
  if (ChangeSide(wtm))
    *tree->rephead_w++=HashKey;
  else
    *tree->rephead_b++=HashKey;
}
