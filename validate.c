#include <stdio.h>
#include <stdlib.h>
#include "types.h"
#include "function.h"
#include "data.h"
#include "evaluate.h"
void ValidatePosition(int ply)
{
  BITBOARD temp, temp1, temp_occ, temp_occ_rl90, temp_occ_rl45;
  BITBOARD temp_occ_rr45, temp_occx, cattacks, rattacks;
  int i,square,error;
  int temp_score;
/*
  first, test w_occupied and b_occupied
*/
  error=0;
  temp_occ=Or(Or(Or(Or(Or(WhitePawns(ply),WhiteKnights(ply)),WhiteBishops(ply)),
                    WhiteRooks(ply)),WhiteQueens(ply)),WhiteKing(ply));
  if(Xor(WhitePieces(ply),temp_occ)) {
    Print(1,"ERROR white occupied squares is bad!\n");
    Display2BitBoards(temp_occ,WhitePieces(ply));
    error=1;
  }
  temp_occ=Or(Or(Or(Or(Or(BlackPawns(ply),BlackKnights(ply)),BlackBishops(ply)),
                    BlackRooks(ply)),BlackQueens(ply)),BlackKing(ply));
  if(Xor(BlackPieces(ply),temp_occ)) {
    Print(1,"ERROR black occupied squares is bad!\n");
    Display2BitBoards(temp_occ,BlackPieces(ply));
    error=1;
  }
/*
  now test rotated occupied bitboards.
*/
  temp_occ_rl90=0;
  temp_occ_rl45=0;
  temp_occ_rr45=0;
  for (i=0;i<64;i++) {
    if (PieceOnSquare(ply,i)) {
      temp_occ_rl90=Or(temp_occ_rl90,set_mask_rl90[i]);
      temp_occ_rl45=Or(temp_occ_rl45,set_mask_rl45[i]);
      temp_occ_rr45=Or(temp_occ_rr45,set_mask_rr45[i]);
    }
  }
  if(Xor(OccupiedRL90(ply),temp_occ_rl90)) {
    Print(1,"ERROR occupied squares (rotated left 90) is bad!\n");
    Display2BitBoards(temp_occ_rl90,OccupiedRL90(ply));
    error=1;
  }
  if(Xor(OccupiedRL45(ply),temp_occ_rl45)) {
    Print(1,"ERROR occupied squares (rotated left 45) is bad!\n");
    Display2BitBoards(temp_occ_rl45,OccupiedRL45(ply));
    error=1;
  }
  if(Xor(OccupiedRR45(ply),temp_occ_rr45)) {
    Print(1,"ERROR occupied squares (rotated right 45) is bad!\n");
    Display2BitBoards(temp_occ_rr45,OccupiedRR45(ply));
    error=1;
  }
/*
  now test bishops_queens and rooks_queens
*/
  temp_occ=Or(Or(Or(WhiteBishops(ply),WhiteQueens(ply)),BlackBishops(ply)),
              BlackQueens(ply));
  if(Xor(BishopsQueens(ply),temp_occ)) {
    Print(1,"ERROR bishops_queens is bad!\n");
    Display2BitBoards(temp_occ,BishopsQueens(ply));
    error=1;
  }
    temp_occ=Or(Or(Or(WhiteRooks(ply),WhiteQueens(ply)),BlackRooks(ply)),
                BlackQueens(ply));
  if(Xor(RooksQueens(ply),temp_occ)) {
    Print(1,"ERROR rooks_queens is bad!\n");
    Display2BitBoards(temp_occ,RooksQueens(ply));
    error=1;
    DisplayBitBoard(Xor(RooksQueens(ply),temp_occ));
  }
/*
  check individual piece bit-boards to make sure two pieces
  don't occupy the same square (bit)
*/
    temp_occ=Xor(Xor(Xor(Xor(Xor(Xor(Xor(Xor(Xor(Xor(Xor(
       WhitePawns(ply),WhiteKnights(ply)),WhiteBishops(ply)),WhiteRooks(ply)),
       WhiteQueens(ply)),BlackPawns(ply)),BlackKnights(ply)),BlackBishops(ply)),
       BlackRooks(ply)),BlackQueens(ply)),WhiteKing(ply)),BlackKing(ply));
    temp_occx=Or(Or(Or(Or(Or(Or(Or(Or(Or(Or(Or(
       WhitePawns(ply),WhiteKnights(ply)),WhiteBishops(ply)),WhiteRooks(ply)),
       WhiteQueens(ply)),BlackPawns(ply)),BlackKnights(ply)),BlackBishops(ply)),
       BlackRooks(ply)),BlackQueens(ply)),WhiteKing(ply)),BlackKing(ply));
    if(Xor(temp_occ,temp_occx)) {
      Print(1,"ERROR two pieces on same square\n");
      error=1;
    }
/*
  test material_evaluation
*/
  temp_score=Popcnt(WhitePawns(ply))*PAWN_VALUE;
  temp_score-=Popcnt(BlackPawns(ply))*PAWN_VALUE;
  temp_score+=Popcnt(WhiteKnights(ply))*KNIGHT_VALUE;
  temp_score-=Popcnt(BlackKnights(ply))*KNIGHT_VALUE;
  temp_score+=Popcnt(WhiteBishops(ply))*BISHOP_VALUE;
  temp_score-=Popcnt(BlackBishops(ply))*BISHOP_VALUE;
  temp_score+=Popcnt(WhiteRooks(ply))*ROOK_VALUE;
  temp_score-=Popcnt(BlackRooks(ply))*ROOK_VALUE;
  temp_score+=Popcnt(WhiteQueens(ply))*QUEEN_VALUE;
  temp_score-=Popcnt(BlackQueens(ply))*QUEEN_VALUE;
  if(temp_score != Material(ply)) {
    Print(1,"ERROR  material_evaluation is wrong, good=%d, bad=%d\n",
           temp_score,Material(ply));
    error=1;
  }
  temp_score=Popcnt(WhiteKnights(ply))*knight_v;
  temp_score+=Popcnt(WhiteBishops(ply))*bishop_v;
  temp_score+=Popcnt(WhiteRooks(ply))*rook_v;
  temp_score+=Popcnt(WhiteQueens(ply))*queen_v;
  if(temp_score != TotalWhitePieces(ply)) {
    Print(1,"ERROR  white_pieces is wrong, good=%d, bad=%d\n",
           temp_score,TotalWhitePieces(ply));
    error=1;
  }
  temp_score=Popcnt(WhitePawns(ply));
  if(temp_score != TotalWhitePawns(ply)) {
    Print(1,"ERROR  white_pawns is wrong, good=%d, bad=%d\n",
           temp_score,TotalWhitePawns(ply));
    error=1;
  }
  temp_score=Popcnt(BlackKnights(ply))*knight_v;
  temp_score+=Popcnt(BlackBishops(ply))*bishop_v;
  temp_score+=Popcnt(BlackRooks(ply))*rook_v;
  temp_score+=Popcnt(BlackQueens(ply))*queen_v;
  if(temp_score != TotalBlackPieces(ply)) {
    Print(1,"ERROR  black_pieces is wrong, good=%d, bad=%d\n",
           temp_score,TotalBlackPieces(ply));
    error=1;
  }
  temp_score=Popcnt(BlackPawns(ply));
  if(temp_score != TotalBlackPawns(ply)) {
    Print(1,"ERROR  black_pawns is wrong, good=%d, bad=%d\n",
           temp_score,TotalBlackPawns(ply));
    error=1;
  }
/*
  now test the board[...] to make sure piece values are correct.
*/
/*
   test pawn locations
*/
    temp=WhitePawns(ply);
    while(temp) {
      square=FirstOne(temp);
      if (PieceOnSquare(ply,square) != pawn) {
        Print(1,"ERROR!  board[%d]=%d, should be 1\n",square,
              PieceOnSquare(ply,square));
        error=1;
      }
      Clear(square,temp);
    }
    temp=BlackPawns(ply);
    while(temp) {
      square=FirstOne(temp);
      if (PieceOnSquare(ply,square) != -pawn) {
        Print(1,"ERROR!  board[%d]=%d, should be -1\n",square,
              PieceOnSquare(ply,square));
        error=1;
      }
      Clear(square,temp);
    }
/*
   test knight locations
*/
    temp=WhiteKnights(ply);
    while(temp) {
      square=FirstOne(temp);
      if (PieceOnSquare(ply,square) != knight) {
        Print(1,"ERROR!  board[%d]=%d, should be 2\n",square,
              PieceOnSquare(ply,square));
        error=1;
      }
      Clear(square,temp);
    }
    temp=BlackKnights(ply);
    while(temp) {
      square=FirstOne(temp);
      if (PieceOnSquare(ply,square) != -knight) {
        Print(1,"ERROR!  board[%d]=%d, should be -2\n",square,
              PieceOnSquare(ply,square));
        error=1;
      }
      Clear(square,temp);
    }
/*
   test bishop locations
*/
    temp=WhiteBishops(ply);
    while(temp) {
      square=FirstOne(temp);
      if (PieceOnSquare(ply,square) != bishop) {
        Print(1,"ERROR!  board[%d]=%d, should be 3\n",square,
              PieceOnSquare(ply,square));
        error=1;
      }
      rattacks=AttacksBishop(square);
      cattacks=ValidateComputeBishopAttacks(square,ply);
      if (rattacks != cattacks) {
        Print(1,"ERROR!  bishop attacks wrong, square=%d\n",square);
        Display2BitBoards(rattacks,cattacks);
        error=1;
      }
      Clear(square,temp);
    }
    temp=BlackBishops(ply);
    while(temp) {
      square=FirstOne(temp);
      if (PieceOnSquare(ply,square) != -bishop) {
        Print(1,"ERROR!  board[%d]=%d, should be -3\n",square,
              PieceOnSquare(ply,square));
        error=1;
      }
      rattacks=AttacksBishop(square);
      cattacks=ValidateComputeBishopAttacks(square,ply);
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
    temp=WhiteRooks(ply);
    while(temp) {
      square=FirstOne(temp);
      if (PieceOnSquare(ply,square) != rook) {
        Print(1,"ERROR!  board[%d]=%d, should be 4\n",square,
              PieceOnSquare(ply,square));
        error=1;
      }
      rattacks=AttacksRook(square);
      cattacks=ValidateComputeRookAttacks(square,ply);
      if (rattacks != cattacks) {
        Print(1,"ERROR!  Rook attacks wrong, square=%d\n",square);
        Display2BitBoards(rattacks,cattacks);
        error=1;
      }
      Clear(square,temp);
    }
    temp=BlackRooks(ply);
    while(temp) {
      square=FirstOne(temp);
      if (PieceOnSquare(ply,square) != -rook) {
        Print(1,"ERROR!  board[%d]=%d, should be -4\n",square,
              PieceOnSquare(ply,square));
        error=1;
      }
      rattacks=AttacksRook(square);
      cattacks=ValidateComputeRookAttacks(square,ply);
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
    temp=WhiteQueens(ply);
    while(temp) {
      square=FirstOne(temp);
      if (PieceOnSquare(ply,square) != queen) {
        Print(1,"ERROR!  board[%d]=%d, should be 5\n",square,
              PieceOnSquare(ply,square));
        error=1;
      }
      rattacks=AttacksQueen(square);
      cattacks=Or(ValidateComputeRookAttacks(square,ply),ValidateComputeBishopAttacks(square,ply));
      if (rattacks != cattacks) {
        Print(1,"ERROR!  queen attacks wrong, square=%d\n",square);
        Display2BitBoards(rattacks,cattacks);
        error=1;
      }
      Clear(square,temp);
    }
    temp=BlackQueens(ply);
    while(temp) {
      square=FirstOne(temp);
      if (PieceOnSquare(ply,square) != -queen) {
        Print(1,"ERROR!  board[%d]=%d, should be -5\n",square,
              PieceOnSquare(ply,square));
        error=1;
      }
      rattacks=AttacksQueen(square);
      cattacks=Or(ValidateComputeRookAttacks(square,ply),ValidateComputeBishopAttacks(square,ply));
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
    temp=WhiteKing(ply);
    while(temp) {
      square=FirstOne(temp);
      if (PieceOnSquare(ply,square) != king) {
        Print(1,"ERROR!  board[%d]=%d, should be 6\n",square,
              PieceOnSquare(ply,square));
        error=1;
      }
      if (WhiteKingSQ(ply) != square) {
        Print(1,"ERROR!  white_king is %d, should be %d\n",
              WhiteKingSQ(ply),square);
        error=1;
      }
      Clear(square,temp);
    }
    temp=BlackKing(ply);
    while(temp) {
      square=FirstOne(temp);
      if (PieceOnSquare(ply,square) != -king) {
        Print(1,"ERROR!  board[%d]=%d, should be -6\n",square,
              PieceOnSquare(ply,square));
        error=1;
      }
      if (BlackKingSQ(ply) != square) {
        Print(1,"ERROR!  black_king is %d, should be %d\n",
              BlackKingSQ(ply),square);
        error=1;
      }
      Clear(square,temp);
    }
/*
   test board[i] fully now.
*/
    for (i=0;i<64;i++)
    switch (PieceOnSquare(ply,i)) {
      case -king:
        if (!And(BlackKing(ply),set_mask[i])) {
          Print(1,"ERROR!  b_king/board[%d] don't agree!\n",i);
          error=1;
        }
        break;
      case -queen:
        if (!And(BlackQueens(ply),set_mask[i])) {
          Print(1,"ERROR!  b_queen/board[%d] don't agree!\n",i);
          error=1;
        }
        break;
      case -rook:
        if (!And(BlackRooks(ply),set_mask[i])) {
          Print(1,"ERROR!  b_rook/board[%d] don't agree!\n",i);
          error=1;
        }
        break;
      case -bishop:
        if (!And(BlackBishops(ply),set_mask[i])) {
          Print(1,"ERROR!  b_bishop/board[%d] don't agree!\n",i);
          error=1;
        }
        break;
      case -knight:
        if (!And(BlackKnights(ply),set_mask[i])) {
          Print(1,"ERROR!  b_knight/board[%d] don't agree!\n",i);
          error=1;
        }
        break;
      case -pawn:
        if (!And(BlackPawns(ply),set_mask[i])) {
          Print(1,"ERROR!  b_pawn/board[%d] don't agree!\n",i);
          error=1;
        }
        break;
      case king:
        if (!And(WhiteKing(ply),set_mask[i])) {
          Print(1,"ERROR!  w_king/board[%d] don't agree!\n",i);
          error=1;
        }
        break;
      case queen:
        if (!And(WhiteQueens(ply),set_mask[i])) {
          Print(1,"ERROR!  w_queen/board[%d] don't agree!\n",i);
          error=1;
        }
        break;
      case rook:
        if (!And(WhiteRooks(ply),set_mask[i])) {
          Print(1,"ERROR!  w_rook/board[%d] don't agree!\n",i);
          error=1;
        }
        break;
      case bishop:
        if (!And(WhiteBishops(ply),set_mask[i])) {
          Print(1,"ERROR!  w_bishop/board[%d] don't agree!\n",i);
          error=1;
        }
        break;
      case knight:
        if (!And(WhiteKnights(ply),set_mask[i])) {
          Print(1,"ERROR!  w_knight/board[%d] don't agree!\n",i);
          error=1;
        }
        break;
      case pawn:
        if (!And(WhitePawns(ply),set_mask[i])) {
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
      if (PieceOnSquare(ply,square)) {
        Print(1,"ERROR!  board[%d]=%d, should be 0\n",square,
              PieceOnSquare(ply,square));
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
    switch (PieceOnSquare(ply,i)) {
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
  if (EnPassantTarget(ply)) HashEP(FirstOne(EnPassantTarget(ply)),temp);
  if (!(WhiteCastle(ply)&1)) HashCastleW(0,temp);
  if (!(WhiteCastle(ply)&2)) HashCastleW(1,temp);
  if (!(BlackCastle(ply)&1)) HashCastleB(0,temp);
  if (!(BlackCastle(ply)&2)) HashCastleB(1,temp);
  if(Xor(temp,HashKey(ply))) {
    Print(1,"ERROR!  hash_key is bad.\n");
    error=1;
  }
  if(Xor(temp1,PawnHashKey(ply))) {
    Print(1,"ERROR!  pawn_hash_key is bad.\n");
    error=1;
  }
  if (error) exit(1);
}

BITBOARD ValidateComputeBishopAttacks(int square,int ply)
{
  BITBOARD attacks, temp_attacks;
  BITBOARD temp7, temp9;
  attacks=bishop_attacks[square];
  temp_attacks=And(attacks,Compl(Occupied(ply)));
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

BITBOARD ValidateComputeRookAttacks(int square,int ply)
{
  BITBOARD attacks, temp_attacks;
  BITBOARD temp1, temp8;
  attacks=rook_attacks[square];
  temp_attacks=And(attacks,Compl(Occupied(ply)));
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
