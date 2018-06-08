#include <stdio.h>
#include <stdlib.h>
#include "types.h"
#include "function.h"
#include "data.h"
#include "evaluate.h"
void Validate_Position(int ply)
{
  BITBOARD temp, temp1, temp_occ, temp_occ_rl90, temp_occ_rl45;
  BITBOARD temp_occ_rr45, temp_occx;
  int i,square,error;
  int temp_score;
/*
  first, test w_occupied and b_occupied
*/
  error=0;
  temp_occ=Or(Or(Or(Or(Or(White_Pawns(ply),
                          White_Knights(ply)),
                       White_Bishops(ply)),
                    White_Rooks(ply)),
                 White_Queens(ply)),
              White_King(ply));
  if(Xor(White_Pieces(ply),temp_occ)) {
    Print(1,"ERROR white occupied squares is bad!\n");
    Display_2_Bit_Boards(temp_occ,White_Pieces(ply));
    error=1;
  }
  temp_occ=Or(Or(Or(Or(Or(Black_Pawns(ply),
                          Black_Knights(ply)),
                       Black_Bishops(ply)),
                    Black_Rooks(ply)),
                 Black_Queens(ply)),
              Black_King(ply));
  if(Xor(Black_Pieces(ply),temp_occ)) {
    Print(1,"ERROR black occupied squares is bad!\n");
    Display_2_Bit_Boards(temp_occ,Black_Pieces(ply));
    error=1;
  }
/*
  now test rotated occupied bitboards.
*/
  temp_occ_rl90=0;
  temp_occ_rl45=0;
  temp_occ_rr45=0;
  for (i=0;i<64;i++) {
    if (Piece_On_Square(ply,i)) {
      temp_occ_rl90=
        Or(temp_occ_rl90,
           set_mask_rl90[i]);
      temp_occ_rl45=
        Or(temp_occ_rl45,
           set_mask_rl45[i]);
      temp_occ_rr45=
        Or(temp_occ_rr45,
           set_mask_rr45[i]);
    }
  }
  if(Xor(Occupied_RL90(ply),temp_occ_rl90)) {
    Print(1,"ERROR occupied squares (rotated left 90) is bad!\n");
    Display_2_Bit_Boards(temp_occ_rl90,Occupied_RL90(ply));
    error=1;
  }
  if(Xor(Occupied_RL45(ply),temp_occ_rl45)) {
    Print(1,"ERROR occupied squares (rotated left 45) is bad!\n");
    Display_2_Bit_Boards(temp_occ_rl45,Occupied_RL45(ply));
    error=1;
  }
  if(Xor(Occupied_RR45(ply),temp_occ_rr45)) {
    Print(1,"ERROR occupied squares (rotated right 45) is bad!\n");
    Display_2_Bit_Boards(temp_occ_rr45,Occupied_RR45(ply));
    error=1;
  }
/*
  now test bishops_queens and rooks_queens
*/
  temp_occ=Or(Or(Or(White_Bishops(ply),
                    White_Queens(ply)),
                 Black_Bishops(ply)),
              Black_Queens(ply));
  if(Xor(Bishops_Queens(ply),temp_occ)) {
    Print(1,"ERROR bishops_queens is bad!\n");
    Display_2_Bit_Boards(temp_occ,Bishops_Queens(ply));
    error=1;
}
    temp_occ=Or(Or(Or(White_Rooks(ply),
                      White_Queens(ply)),
                   Black_Rooks(ply)),
                Black_Queens(ply));
  if(Xor(Rooks_Queens(ply),temp_occ)) {
    Print(1,"ERROR rooks_queens is bad!\n");
    Display_2_Bit_Boards(temp_occ,Rooks_Queens(ply));
    error=1;
    Display_Bit_Board(Xor(Rooks_Queens(ply),temp_occ));
}
/*
  check individual piece bit-boards to make sure two pieces
  don't occupy the same square (bit)
*/
    temp_occ=Xor(Xor(Xor(Xor(Xor(Xor(Xor(Xor(Xor(Xor(Xor(
       White_Pawns(ply),White_Knights(ply)),
       White_Bishops(ply)),White_Rooks(ply)),
       White_Queens(ply)),Black_Pawns(ply)),
       Black_Knights(ply)),Black_Bishops(ply)),
       Black_Rooks(ply)),Black_Queens(ply)),
       White_King(ply)),Black_King(ply));
    temp_occx=Or(Or(Or(Or(Or(Or(Or(Or(Or(Or(Or(
       White_Pawns(ply),White_Knights(ply)),
       White_Bishops(ply)),White_Rooks(ply)),
       White_Queens(ply)),Black_Pawns(ply)),
       Black_Knights(ply)),Black_Bishops(ply)),
       Black_Rooks(ply)),Black_Queens(ply)),
       White_King(ply)),Black_King(ply));
    if(Xor(temp_occ,temp_occx)) {
      Print(1,"ERROR two pieces on same square\n");
      error=1;
    }
/*
  test material_evaluation
*/
  temp_score=Popcnt(White_Pawns(ply))*PAWN_VALUE;
  temp_score-=Popcnt(Black_Pawns(ply))*PAWN_VALUE;
  temp_score+=Popcnt(White_Knights(ply))*KNIGHT_VALUE;
  temp_score-=Popcnt(Black_Knights(ply))*KNIGHT_VALUE;
  temp_score+=Popcnt(White_Bishops(ply))*BISHOP_VALUE;
  temp_score-=Popcnt(Black_Bishops(ply))*BISHOP_VALUE;
  temp_score+=Popcnt(White_Rooks(ply))*ROOK_VALUE;
  temp_score-=Popcnt(Black_Rooks(ply))*ROOK_VALUE;
  temp_score+=Popcnt(White_Queens(ply))*QUEEN_VALUE;
  temp_score-=Popcnt(Black_Queens(ply))*QUEEN_VALUE;
  if(temp_score != Material(ply)) {
    Print(1,"ERROR  material_evaluation is wrong, good=%d, bad=%d\n",
           temp_score,Material(ply));
    error=1;
  }
  temp_score=Popcnt(White_Knights(ply))*3;
  temp_score+=Popcnt(White_Bishops(ply))*3;
  temp_score+=Popcnt(White_Rooks(ply))*5;
  temp_score+=Popcnt(White_Queens(ply))*9;
  if(temp_score != Total_White_Pieces(ply)) {
    Print(1,"ERROR  white_pieces is wrong, good=%d, bad=%d\n",
           temp_score,Total_White_Pieces(ply));
    error=1;
  }
  temp_score=Popcnt(White_Pawns(ply));
  if(temp_score != Total_White_Pawns(ply)) {
    Print(1,"ERROR  white_pawns is wrong, good=%d, bad=%d\n",
           temp_score,Total_White_Pawns(ply));
    error=1;
  }
  temp_score=Popcnt(Black_Knights(ply))*3;
  temp_score+=Popcnt(Black_Bishops(ply))*3;
  temp_score+=Popcnt(Black_Rooks(ply))*5;
  temp_score+=Popcnt(Black_Queens(ply))*9;
  if(temp_score != Total_Black_Pieces(ply)) {
    Print(1,"ERROR  black_pieces is wrong, good=%d, bad=%d\n",
           temp_score,Total_Black_Pieces(ply));
    error=1;
  }
  temp_score=Popcnt(Black_Pawns(ply));
  if(temp_score != Total_Black_Pawns(ply)) {
    Print(1,"ERROR  black_pawns is wrong, good=%d, bad=%d\n",
           temp_score,Total_Black_Pawns(ply));
    error=1;
  }
/*
  now test the board[...] to make sure piece values are correct.
*/
/*
   test pawn locations
*/
    temp=White_Pawns(ply);
    while(temp) {
      square=First_One(temp);
      if (Piece_On_Square(ply,square) != 1) {
        Print(1,"ERROR!  board[%d]=%d, should be 1\n",square,
              Piece_On_Square(ply,square));
        error=1;
      }
      Clear(square,temp);
    }
    temp=Black_Pawns(ply);
    while(temp) {
      square=First_One(temp);
      if (Piece_On_Square(ply,square) != -1) {
        Print(1,"ERROR!  board[%d]=%d, should be -1\n",square,
              Piece_On_Square(ply,square));
        error=1;
      }
      Clear(square,temp);
    }
/*
   test knight locations
*/
    temp=White_Knights(ply);
    while(temp) {
      square=First_One(temp);
      if (Piece_On_Square(ply,square) != 2) {
        Print(1,"ERROR!  board[%d]=%d, should be 2\n",square,
              Piece_On_Square(ply,square));
        error=1;
      }
      Clear(square,temp);
    }
    temp=Black_Knights(ply);
    while(temp) {
      square=First_One(temp);
      if (Piece_On_Square(ply,square) != -2) {
        Print(1,"ERROR!  board[%d]=%d, should be -2\n",square,
              Piece_On_Square(ply,square));
        error=1;
      }
      Clear(square,temp);
    }
/*
   test bishop locations
*/
    temp=White_Bishops(ply);
    while(temp) {
      square=First_One(temp);
      if (Piece_On_Square(ply,square) != 3) {
        Print(1,"ERROR!  board[%d]=%d, should be 3\n",square,
              Piece_On_Square(ply,square));
        error=1;
      }
      Clear(square,temp);
    }
    temp=Black_Bishops(ply);
    while(temp) {
      square=First_One(temp);
      if (Piece_On_Square(ply,square) != -3) {
        Print(1,"ERROR!  board[%d]=%d, should be -3\n",square,
              Piece_On_Square(ply,square));
        error=1;
      }
      Clear(square,temp);
    }
/*
   test rook locations
*/
    temp=White_Rooks(ply);
    while(temp) {
      square=First_One(temp);
      if (Piece_On_Square(ply,square) != 4) {
        Print(1,"ERROR!  board[%d]=%d, should be 4\n",square,
              Piece_On_Square(ply,square));
        error=1;
      }
      Clear(square,temp);
    }
    temp=Black_Rooks(ply);
    while(temp) {
      square=First_One(temp);
      if (Piece_On_Square(ply,square) != -4) {
        Print(1,"ERROR!  board[%d]=%d, should be -4\n",square,
              Piece_On_Square(ply,square));
        error=1;
      }
      Clear(square,temp);
    }
/*
   test queen locations
*/
    temp=White_Queens(ply);
    while(temp) {
      square=First_One(temp);
      if (Piece_On_Square(ply,square) != 5) {
        Print(1,"ERROR!  board[%d]=%d, should be 5\n",square,
              Piece_On_Square(ply,square));
        error=1;
      }
      Clear(square,temp);
    }
    temp=Black_Queens(ply);
    while(temp) {
      square=First_One(temp);
      if (Piece_On_Square(ply,square) != -5) {
        Print(1,"ERROR!  board[%d]=%d, should be -5\n",square,
              Piece_On_Square(ply,square));
        error=1;
      }
      Clear(square,temp);
    }
/*
   test king locations
*/
    temp=White_King(ply);
    while(temp) {
      square=First_One(temp);
      if (Piece_On_Square(ply,square) != 6) {
        Print(1,"ERROR!  board[%d]=%d, should be 6\n",square,
              Piece_On_Square(ply,square));
        error=1;
      }
      if (White_King_SQ(ply) != square) {
        Print(1,"ERROR!  white_king is %d, should be %d\n",
              White_King_SQ(ply),square);
        error=1;
      }
      Clear(square,temp);
    }
    temp=Black_King(ply);
    while(temp) {
      square=First_One(temp);
      if (Piece_On_Square(ply,square) != -6) {
        Print(1,"ERROR!  board[%d]=%d, should be -6\n",square,
              Piece_On_Square(ply,square));
        error=1;
      }
      if (Black_King_SQ(ply) != square) {
        Print(1,"ERROR!  black_king is %d, should be %d\n",
              Black_King_SQ(ply),square);
        error=1;
      }
      Clear(square,temp);
    }
/*
   test board[i] fully now.
*/
    for (i=0;i<64;i++)
    switch (Piece_On_Square(ply,i)) {
      case -6:
        if (!And(Black_King(ply),set_mask[i])) {
          Print(1,"ERROR!  b_king/board[%d] don't agree!\n",i);
          error=1;
        }
        break;
      case -5:
        if (!And(Black_Queens(ply),set_mask[i])) {
          Print(1,"ERROR!  b_queen/board[%d] don't agree!\n",i);
          error=1;
        }
        break;
      case -4:
        if (!And(Black_Rooks(ply),set_mask[i])) {
          Print(1,"ERROR!  b_rook/board[%d] don't agree!\n",i);
          error=1;
        }
        break;
      case -3:
        if (!And(Black_Bishops(ply),set_mask[i])) {
          Print(1,"ERROR!  b_bishop/board[%d] don't agree!\n",i);
          error=1;
        }
        break;
      case -2:
        if (!And(Black_Knights(ply),set_mask[i])) {
          Print(1,"ERROR!  b_knight/board[%d] don't agree!\n",i);
          error=1;
        }
        break;
      case -1:
        if (!And(Black_Pawns(ply),set_mask[i])) {
          Print(1,"ERROR!  b_pawn/board[%d] don't agree!\n",i);
          error=1;
        }
        break;
      case 6:
        if (!And(White_King(ply),set_mask[i])) {
          Print(1,"ERROR!  w_king/board[%d] don't agree!\n",i);
          error=1;
        }
        break;
      case 5:
        if (!And(White_Queens(ply),set_mask[i])) {
          Print(1,"ERROR!  w_queen/board[%d] don't agree!\n",i);
          error=1;
        }
        break;
      case 4:
        if (!And(White_Rooks(ply),set_mask[i])) {
          Print(1,"ERROR!  w_rook/board[%d] don't agree!\n",i);
          error=1;
        }
        break;
      case 3:
        if (!And(White_Bishops(ply),set_mask[i])) {
          Print(1,"ERROR!  w_bishop/board[%d] don't agree!\n",i);
          error=1;
        }
        break;
      case 2:
        if (!And(White_Knights(ply),set_mask[i])) {
          Print(1,"ERROR!  w_knight/board[%d] don't agree!\n",i);
          error=1;
        }
        break;
      case 1:
        if (!And(White_Pawns(ply),set_mask[i])) {
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
      square=First_One(temp);
      if (Piece_On_Square(ply,square)) {
        Print(1,"ERROR!  board[%d]=%d, should be 0\n",square,
              Piece_On_Square(ply,square));
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
    switch (Piece_On_Square(ply,i)) {
      case 6:
        temp=Xor(temp,w_king_random[i]);
        break;
      case 5:
        temp=Xor(temp,w_queen_random[i]);
        break;
      case 4:
        temp=Xor(temp,w_rook_random[i]);
        break;
      case 3:
        temp=Xor(temp,w_bishop_random[i]);
        break;
      case 2:
        temp=Xor(temp,w_knight_random[i]);
        break;
      case 1:
        temp=Xor(temp,w_pawn_random[i]);
        temp1=Xor(temp1,w_pawn_random[i]);
        break;
      case -1:
        temp=Xor(temp,b_pawn_random[i]);
        temp1=Xor(temp1,b_pawn_random[i]);
        break;
      case -2:
        temp=Xor(temp,b_knight_random[i]);
        break;
      case -3:
        temp=Xor(temp,b_bishop_random[i]);
        break;
      case -4:
        temp=Xor(temp,b_rook_random[i]);
        break;
      case -5:
        temp=Xor(temp,b_queen_random[i]);
        break;
      case -6:
        temp=Xor(temp,b_king_random[i]);
        break;
      default:
        break;
    }
  }
  if(Xor(temp,Hash_Key(ply))) {
    Print(1,"ERROR!  hash_key is bad.\n");
    error=1;
  }
  if(Xor(temp1,Pawn_Hash_Key(ply))) {
    Print(1,"ERROR!  pawn_hash_key is bad.\n");
    error=1;
  }
  if (error) exit(1);
}
void Validate_Hash_Key(int ply)
{
  int i,j;
  for (i=0;i<ply;i++)
    for (j=i+1;j<ply;j++)
      if (!Xor(Hash_Key(i),Hash_Key(j)))
        if (!(Xor(White_Pawns(i),White_Pawns(j)) ||
            Xor(White_Knights(i),White_Knights(j)) ||
            Xor(White_Bishops(i),White_Bishops(j)) ||
            Xor(White_Rooks(i),White_Rooks(j)) ||
            Xor(White_Queens(i),White_Queens(j)) ||
            Xor(White_King(i),White_King(j)) ||
            Xor(Black_Pawns(i),Black_Pawns(j)) ||
            Xor(Black_Knights(i),Black_Knights(j)) ||
            Xor(Black_Bishops(i),Black_Bishops(j)) ||
            Xor(Black_Rooks(i),Black_Rooks(j)) ||
            Xor(Black_Queens(i),Black_Queens(j)) ||
            Xor(Black_King(i),Black_King(j))))
            Print(1,"hash collision!  key=key, board!=board\n");
}
