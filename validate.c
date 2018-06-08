#include <stdio.h>
#include <stdlib.h>
#include "chess.h"
#include "data.h"

void ValidatePosition(int ply, int move, char *caller)
{
  BITBOARD temp, temp1, temp_occ, temp_occ_rl90, temp_occ_rl45;
  BITBOARD temp_occ_rr45, temp_occx, cattacks, rattacks;
  int i,square,error;
  int temp_score;
/*
  first, test w_occupied and b_occupied
*/
  error=0;
  temp_occ=Or(Or(Or(Or(Or(WhitePawns,WhiteKnights),WhiteBishops),
                    WhiteRooks),WhiteQueens),WhiteKing);
  if(Xor(WhitePieces,temp_occ)) {
    Print(1,"ERROR white occupied squares is bad!\n");
    Display2BitBoards(temp_occ,WhitePieces);
    error=1;
  }
  temp_occ=Or(Or(Or(Or(Or(BlackPawns,BlackKnights),BlackBishops),
                    BlackRooks),BlackQueens),BlackKing);
  if(Xor(BlackPieces,temp_occ)) {
    Print(1,"ERROR black occupied squares is bad!\n");
    Display2BitBoards(temp_occ,BlackPieces);
    error=1;
  }
/*
  now test rotated occupied bitboards.
*/
  temp_occ_rl90=0;
  temp_occ_rl45=0;
  temp_occ_rr45=0;
  for (i=0;i<64;i++) {
    if (PieceOnSquare(i)) {
      temp_occ_rl90=Or(temp_occ_rl90,set_mask_rl90[i]);
      temp_occ_rl45=Or(temp_occ_rl45,set_mask_rl45[i]);
      temp_occ_rr45=Or(temp_occ_rr45,set_mask_rr45[i]);
    }
  }
  if(Xor(OccupiedRL90,temp_occ_rl90)) {
    Print(1,"ERROR occupied squares (rotated left 90) is bad!\n");
    Display2BitBoards(temp_occ_rl90,OccupiedRL90);
    error=1;
  }
  if(Xor(OccupiedRL45,temp_occ_rl45)) {
    Print(1,"ERROR occupied squares (rotated left 45) is bad!\n");
    Display2BitBoards(temp_occ_rl45,OccupiedRL45);
    error=1;
  }
  if(Xor(OccupiedRR45,temp_occ_rr45)) {
    Print(1,"ERROR occupied squares (rotated right 45) is bad!\n");
    Display2BitBoards(temp_occ_rr45,OccupiedRR45);
    error=1;
  }
/*
  now test bishops_queens and rooks_queens
*/
  temp_occ=Or(Or(Or(WhiteBishops,WhiteQueens),BlackBishops),
              BlackQueens);
  if(Xor(BishopsQueens,temp_occ)) {
    Print(1,"ERROR bishops_queens is bad!\n");
    Display2BitBoards(temp_occ,BishopsQueens);
    error=1;
  }
    temp_occ=Or(Or(Or(WhiteRooks,WhiteQueens),BlackRooks),
                BlackQueens);
  if(Xor(RooksQueens,temp_occ)) {
    Print(1,"ERROR rooks_queens is bad!\n");
    Display2BitBoards(temp_occ,RooksQueens);
    error=1;
  }
/*
  check individual piece bit-boards to make sure two pieces
  don't occupy the same square (bit)
*/
    temp_occ=Xor(Xor(Xor(Xor(Xor(Xor(Xor(Xor(Xor(Xor(Xor(
       WhitePawns,WhiteKnights),WhiteBishops),WhiteRooks),
       WhiteQueens),BlackPawns),BlackKnights),BlackBishops),
       BlackRooks),BlackQueens),WhiteKing),BlackKing);
    temp_occx=Or(Or(Or(Or(Or(Or(Or(Or(Or(Or(Or(
       WhitePawns,WhiteKnights),WhiteBishops),WhiteRooks),
       WhiteQueens),BlackPawns),BlackKnights),BlackBishops),
       BlackRooks),BlackQueens),WhiteKing),BlackKing);
    if(Xor(temp_occ,temp_occx)) {
      Print(1,"ERROR two pieces on same square\n");
      error=1;
    }
/*
  test material_evaluation
*/
  temp_score=PopCnt(WhitePawns)*PAWN_VALUE;
  temp_score-=PopCnt(BlackPawns)*PAWN_VALUE;
  temp_score+=PopCnt(WhiteKnights)*KNIGHT_VALUE;
  temp_score-=PopCnt(BlackKnights)*KNIGHT_VALUE;
  temp_score+=PopCnt(WhiteBishops)*BISHOP_VALUE;
  temp_score-=PopCnt(BlackBishops)*BISHOP_VALUE;
  temp_score+=PopCnt(WhiteRooks)*ROOK_VALUE;
  temp_score-=PopCnt(BlackRooks)*ROOK_VALUE;
  temp_score+=PopCnt(WhiteQueens)*QUEEN_VALUE;
  temp_score-=PopCnt(BlackQueens)*QUEEN_VALUE;
  if(temp_score != Material) {
    Print(1,"ERROR  material_evaluation is wrong, good=%d, bad=%d\n",
           temp_score,Material);
    error=1;
  }
  temp_score=PopCnt(WhiteKnights)*knight_v;
  temp_score+=PopCnt(WhiteBishops)*bishop_v;
  temp_score+=PopCnt(WhiteRooks)*rook_v;
  temp_score+=PopCnt(WhiteQueens)*queen_v;
  if(temp_score != TotalWhitePieces) {
    Print(1,"ERROR  white_pieces is wrong, good=%d, bad=%d\n",
           temp_score,TotalWhitePieces);
    error=1;
  }
  temp_score=PopCnt(WhitePawns);
  if(temp_score != TotalWhitePawns) {
    Print(1,"ERROR  white_pawns is wrong, good=%d, bad=%d\n",
           temp_score,TotalWhitePawns);
    error=1;
  }
  temp_score=PopCnt(BlackKnights)*knight_v;
  temp_score+=PopCnt(BlackBishops)*bishop_v;
  temp_score+=PopCnt(BlackRooks)*rook_v;
  temp_score+=PopCnt(BlackQueens)*queen_v;
  if(temp_score != TotalBlackPieces) {
    Print(1,"ERROR  black_pieces is wrong, good=%d, bad=%d\n",
           temp_score,TotalBlackPieces);
    error=1;
  }
  temp_score=PopCnt(BlackPawns);
  if(temp_score != TotalBlackPawns) {
    Print(1,"ERROR  black_pawns is wrong, good=%d, bad=%d\n",
           temp_score,TotalBlackPawns);
    error=1;
  }
/*
  now test the board[...] to make sure piece values are correct.
*/
/*
   test pawn locations
*/
    temp=WhitePawns;
    while(temp) {
      square=FirstOne(temp);
      if (PieceOnSquare(square) != pawn) {
        Print(1,"ERROR!  board[%d]=%d, should be 1\n",square,
              PieceOnSquare(square));
        error=1;
      }
      Clear(square,temp);
    }
    temp=BlackPawns;
    while(temp) {
      square=FirstOne(temp);
      if (PieceOnSquare(square) != -pawn) {
        Print(1,"ERROR!  board[%d]=%d, should be -1\n",square,
              PieceOnSquare(square));
        error=1;
      }
      Clear(square,temp);
    }
/*
   test knight locations
*/
    temp=WhiteKnights;
    while(temp) {
      square=FirstOne(temp);
      if (PieceOnSquare(square) != knight) {
        Print(1,"ERROR!  board[%d]=%d, should be 2\n",square,
              PieceOnSquare(square));
        error=1;
      }
      Clear(square,temp);
    }
    temp=BlackKnights;
    while(temp) {
      square=FirstOne(temp);
      if (PieceOnSquare(square) != -knight) {
        Print(1,"ERROR!  board[%d]=%d, should be -2\n",square,
              PieceOnSquare(square));
        error=1;
      }
      Clear(square,temp);
    }
/*
   test bishop locations
*/
    temp=WhiteBishops;
    while(temp) {
      square=FirstOne(temp);
      if (PieceOnSquare(square) != bishop) {
        Print(1,"ERROR!  board[%d]=%d, should be 3\n",square,
              PieceOnSquare(square));
        error=1;
      }
      rattacks=AttacksBishop(square);
      cattacks=ValidateComputeBishopAttacks(square);
      if (rattacks != cattacks) {
        Print(1,"ERROR!  bishop attacks wrong, square=%d\n",square);
        Display2BitBoards(rattacks,cattacks);
        error=1;
      }
      Clear(square,temp);
    }
    temp=BlackBishops;
    while(temp) {
      square=FirstOne(temp);
      if (PieceOnSquare(square) != -bishop) {
        Print(1,"ERROR!  board[%d]=%d, should be -3\n",square,
              PieceOnSquare(square));
        error=1;
      }
      rattacks=AttacksBishop(square);
      cattacks=ValidateComputeBishopAttacks(square);
      if (rattacks != cattacks) {
        Print(1,"ERROR!  bishop attacks wrong, square=%d\n",square);
        Display2BitBoards(rattacks,cattacks);
        error=1;
      }
      Clear(square,temp);
    }
/*
   test rook locations
*/
    temp=WhiteRooks;
    while(temp) {
      square=FirstOne(temp);
      if (PieceOnSquare(square) != rook) {
        Print(1,"ERROR!  board[%d]=%d, should be 4\n",square,
              PieceOnSquare(square));
        error=1;
      }
      rattacks=AttacksRook(square);
      cattacks=ValidateComputeRookAttacks(square);
      if (rattacks != cattacks) {
        Print(1,"ERROR!  Rook attacks wrong, square=%d\n",square);
        Display2BitBoards(rattacks,cattacks);
        error=1;
      }
      Clear(square,temp);
    }
    temp=BlackRooks;
    while(temp) {
      square=FirstOne(temp);
      if (PieceOnSquare(square) != -rook) {
        Print(1,"ERROR!  board[%d]=%d, should be -4\n",square,
              PieceOnSquare(square));
        error=1;
      }
      rattacks=AttacksRook(square);
      cattacks=ValidateComputeRookAttacks(square);
      if (rattacks != cattacks) {
        Print(1,"ERROR!  Rook attacks wrong, square=%d\n",square);
        Display2BitBoards(rattacks,cattacks);
        error=1;
      }
      Clear(square,temp);
    }
/*
   test queen locations
*/
    temp=WhiteQueens;
    while(temp) {
      square=FirstOne(temp);
      if (PieceOnSquare(square) != queen) {
        Print(1,"ERROR!  board[%d]=%d, should be 5\n",square,
              PieceOnSquare(square));
        error=1;
      }
      rattacks=AttacksQueen(square);
      cattacks=Or(ValidateComputeRookAttacks(square),ValidateComputeBishopAttacks(square));
      if (rattacks != cattacks) {
        Print(1,"ERROR!  queen attacks wrong, square=%d\n",square);
        Display2BitBoards(rattacks,cattacks);
        error=1;
      }
      Clear(square,temp);
    }
    temp=BlackQueens;
    while(temp) {
      square=FirstOne(temp);
      if (PieceOnSquare(square) != -queen) {
        Print(1,"ERROR!  board[%d]=%d, should be -5\n",square,
              PieceOnSquare(square));
        error=1;
      }
      rattacks=AttacksQueen(square);
      cattacks=Or(ValidateComputeRookAttacks(square),ValidateComputeBishopAttacks(square));
      if (rattacks != cattacks) {
        Print(1,"ERROR!  queen attacks wrong, square=%d\n",square);
        Display2BitBoards(rattacks,cattacks);
        error=1;
      }
      Clear(square,temp);
    }
/*
   test king locations
*/
    temp=WhiteKing;
    while(temp) {
      square=FirstOne(temp);
      if (PieceOnSquare(square) != king) {
        Print(1,"ERROR!  board[%d]=%d, should be 6\n",square,
              PieceOnSquare(square));
        error=1;
      }
      if (WhiteKingSQ != square) {
        Print(1,"ERROR!  white_king is %d, should be %d\n",
              WhiteKingSQ,square);
        error=1;
      }
      Clear(square,temp);
    }
    temp=BlackKing;
    while(temp) {
      square=FirstOne(temp);
      if (PieceOnSquare(square) != -king) {
        Print(1,"ERROR!  board[%d]=%d, should be -6\n",square,
              PieceOnSquare(square));
        error=1;
      }
      if (BlackKingSQ != square) {
        Print(1,"ERROR!  black_king is %d, should be %d\n",
              BlackKingSQ,square);
        error=1;
      }
      Clear(square,temp);
    }
/*
   test board[i] fully now.
*/
    for (i=0;i<64;i++)
    switch (PieceOnSquare(i)) {
      case -king:
        if (!And(BlackKing,set_mask[i])) {
          Print(1,"ERROR!  b_king/board[%d] don't agree!\n",i);
          error=1;
        }
        break;
      case -queen:
        if (!And(BlackQueens,set_mask[i])) {
          Print(1,"ERROR!  b_queen/board[%d] don't agree!\n",i);
          error=1;
        }
        break;
      case -rook:
        if (!And(BlackRooks,set_mask[i])) {
          Print(1,"ERROR!  b_rook/board[%d] don't agree!\n",i);
          error=1;
        }
        break;
      case -bishop:
        if (!And(BlackBishops,set_mask[i])) {
          Print(1,"ERROR!  b_bishop/board[%d] don't agree!\n",i);
          error=1;
        }
        break;
      case -knight:
        if (!And(BlackKnights,set_mask[i])) {
          Print(1,"ERROR!  b_knight/board[%d] don't agree!\n",i);
          error=1;
        }
        break;
      case -pawn:
        if (!And(BlackPawns,set_mask[i])) {
          Print(1,"ERROR!  b_pawn/board[%d] don't agree!\n",i);
          error=1;
        }
        break;
      case king:
        if (!And(WhiteKing,set_mask[i])) {
          Print(1,"ERROR!  w_king/board[%d] don't agree!\n",i);
          error=1;
        }
        break;
      case queen:
        if (!And(WhiteQueens,set_mask[i])) {
          Print(1,"ERROR!  w_queen/board[%d] don't agree!\n",i);
          error=1;
        }
        break;
      case rook:
        if (!And(WhiteRooks,set_mask[i])) {
          Print(1,"ERROR!  w_rook/board[%d] don't agree!\n",i);
          error=1;
        }
        break;
      case bishop:
        if (!And(WhiteBishops,set_mask[i])) {
          Print(1,"ERROR!  w_bishop/board[%d] don't agree!\n",i);
          error=1;
        }
        break;
      case knight:
        if (!And(WhiteKnights,set_mask[i])) {
          Print(1,"ERROR!  w_knight/board[%d] don't agree!\n",i);
          error=1;
        }
        break;
      case pawn:
        if (!And(WhitePawns,set_mask[i])) {
          Print(1,"ERROR!  w_pawn/board[%d] don't agree!\n",i);
          error=1;
        }
        break;
    }
/*
   test empty squares now
*/
    temp=Compl(Or(temp_occ,temp_occx));
    while(temp) {
      square=FirstOne(temp);
      if (PieceOnSquare(square)) {
        Print(1,"ERROR!  board[%d]=%d, should be 0\n",square,
              PieceOnSquare(square));
        error=1;
      }
      Clear(square,temp);
    }
/*
   test hash key
*/
  temp=0;
  temp1=0;
  for (i=0;i<64;i++) {
    switch (PieceOnSquare(i)) {
      case king:
        temp=Xor(temp,w_king_random[i]);
        break;
      case queen:
        temp=Xor(temp,w_queen_random[i]);
        break;
      case rook:
        temp=Xor(temp,w_rook_random[i]);
        break;
      case bishop:
        temp=Xor(temp,w_bishop_random[i]);
        break;
      case knight:
        temp=Xor(temp,w_knight_random[i]);
        break;
      case pawn:
        temp=Xor(temp,w_pawn_random[i]);
        temp1=Xor(temp1,w_pawn_random[i]);
        break;
      case -pawn:
        temp=Xor(temp,b_pawn_random[i]);
        temp1=Xor(temp1,b_pawn_random[i]);
        break;
      case -knight:
        temp=Xor(temp,b_knight_random[i]);
        break;
      case -bishop:
        temp=Xor(temp,b_bishop_random[i]);
        break;
      case -rook:
        temp=Xor(temp,b_rook_random[i]);
        break;
      case -queen:
        temp=Xor(temp,b_queen_random[i]);
        break;
      case -king:
        temp=Xor(temp,b_king_random[i]);
        break;
      default:
        break;
    }
  }
  if (EnPassant(ply)) HashEP(EnPassant(ply),temp);
  if (!(WhiteCastle(ply)&1)) HashCastleW(0,temp);
  if (!(WhiteCastle(ply)&2)) HashCastleW(1,temp);
  if (!(BlackCastle(ply)&1)) HashCastleB(0,temp);
  if (!(BlackCastle(ply)&2)) HashCastleB(1,temp);
  if(Xor(temp,HashKey)) {
    Print(1,"ERROR!  hash_key is bad.\n");
    error=1;
  }
  if(Xor(temp1,PawnHashKey)) {
    Print(1,"ERROR!  pawn_hash_key is bad.\n");
    error=1;
  }
  if (error) {
/*
    Print(0,"active path:\n");
    for (i=1;i<=ply;i++)
      DisplayChessMove("move=",move);
*/
    Print(0,"current move:\n");
    DisplayChessMove("move=",move);
    DisplayChessBoard(stdout,search);
    Print(0,"called from %s, ply=%d\n",caller,ply);
    Print(0,"node=%d\n",nodes_searched+q_nodes_searched);
    exit(1);
  }
}

BITBOARD ValidateComputeBishopAttacks(int square)
{
  BITBOARD attacks, temp_attacks;
  BITBOARD temp7, temp9;
  attacks=bishop_attacks[square];
  temp_attacks=And(attacks,Compl(Occupied));
  temp_attacks=Compl(Or(temp_attacks,Compl(bishop_attacks[square])));
  temp7=And(temp_attacks,mask_plus7dir[square]);
  temp9=And(temp_attacks,mask_plus9dir[square]);
  attacks=Xor(attacks,mask_plus7dir[FirstOne(temp7)]);
  attacks=Xor(attacks,mask_plus9dir[FirstOne(temp9)]);
  temp7=And(temp_attacks,mask_minus7dir[square]);
  temp9=And(temp_attacks,mask_minus9dir[square]);
  attacks=Xor(attacks,mask_minus7dir[LastOne(temp7)]);
  attacks=Xor(attacks,mask_minus9dir[LastOne(temp9)]);
  return(attacks);
}

BITBOARD ValidateComputeRookAttacks(int square)
{
  BITBOARD attacks, temp_attacks;
  BITBOARD temp1, temp8;
  attacks=rook_attacks[square];
  temp_attacks=And(attacks,Compl(Occupied));
  temp_attacks=Compl(Or(temp_attacks,Compl(rook_attacks[square])));
  temp1=And(temp_attacks,mask_plus1dir[square]);
  temp8=And(temp_attacks,mask_plus8dir[square]);
  attacks=Xor(attacks,mask_plus1dir[FirstOne(temp1)]);
  attacks=Xor(attacks,mask_plus8dir[FirstOne(temp8)]);
  temp1=And(temp_attacks,mask_minus1dir[square]);
  temp8=And(temp_attacks,mask_minus8dir[square]);
  attacks=Xor(attacks,mask_minus1dir[LastOne(temp1)]);
  attacks=Xor(attacks,mask_minus8dir[LastOne(temp8)]);
  return(attacks);
}
