#include <stdio.h>
#include <stdlib.h>
#include "types.h"
#include "function.h"
#include "data.h"
#include "evaluate.h"
#if defined(UNIX)
  #include <unistd.h>
#endif

int init_r90[64] = { 56, 48, 40, 32, 24, 16,  8,  0, 
                     57, 49, 41, 33, 25, 17,  9,  1, 
                     58, 50, 42, 34, 26, 18, 10,  2, 
                     59, 51, 43, 35, 27, 19, 11,  3, 
                     60, 52, 44, 36, 28, 20, 12,  4, 
                     61, 53, 45, 37, 29, 21, 13,  5, 
                     62, 54, 46, 38, 30, 22, 14,  6, 
                     63, 55, 47, 39, 31, 23, 15,  7 };

int init_l90[64] = {  7, 15, 23, 31, 39, 47, 55, 63, 
                      6, 14, 22, 30, 38, 46, 54, 62, 
                      5, 13, 21, 29, 37, 45, 53, 61, 
                      4, 12, 20, 28, 36, 44, 52, 60, 
                      3, 11, 19, 27, 35, 43, 51, 59, 
                      2, 10, 18, 26, 34, 42, 50, 58, 
                      1,  9, 17, 25, 33, 41, 49, 57, 
                      0,  8, 16, 24, 32, 40, 48, 56 };

int init_l45[64] = {                0,
                                  2,  5,
                                9, 14, 20,
                             27, 35,  1,  4,
                            8, 13, 19, 26, 34,
                         42,  3,  7, 12, 18, 25,
                       33, 41, 48,  6, 11, 17, 24,
                     32, 40, 47, 53, 10, 16, 23, 31,
                       39, 46, 52, 57, 15, 22, 30,
                         38, 45, 51, 56, 60, 21,
                           29, 37, 44, 50, 55,
                             59, 62, 28, 36,
                               43, 49, 54,
                                 58, 61,
                                   63 };

int init_ul45[64] = {               0,
                                  8,  1,
                               16,  9,  2,
                             24, 17, 10,  3,
                           32, 25, 18, 11,  4,
                         40, 33, 26, 19, 12,  5,
                       48, 41, 34, 27, 20, 13,  6,
                     56, 49, 42, 35, 28, 21, 14,  7,
                       57, 50, 43, 36, 29, 22, 15,
                         58, 51, 44, 37, 30, 23,
                           59, 52, 45, 38, 31,
                             60, 53, 46, 39,
                               61, 54, 47,
                                 62, 55,
                                   63 };

int init_r45[64] = {               28,
                                 21, 15,
                               10,  6,  3,
                              1,  0, 36, 29,
                           22, 16, 11,  7,  4,
                          2, 43, 37, 30, 23, 17,
                       12,  8,  5, 49, 44, 38, 31,
                     24, 18, 13,  9, 54, 50, 45, 39,
                       32, 25, 19, 14, 58, 55, 51,
                         46, 40, 33, 26, 20, 61,
                           59, 56, 52, 47, 41,
                             34, 27, 63, 62,
                               60, 57, 53,
                                 48, 42,
                                   35  };

int init_ur45[64] = {               7, 
                                  6, 15, 
                                5, 14, 23, 
                              4, 13, 22, 31, 
                            3, 12, 21, 30, 39, 
                          2, 11, 20, 29, 38, 47, 
                        1, 10, 19, 28, 37, 46, 55, 
                      0,  9, 18, 27, 36, 45, 54, 63, 
                        8, 17, 26, 35, 44, 53, 62, 
                         16, 25, 34, 43, 52, 61, 
                           24, 33, 42, 51, 60, 
                             32, 41, 50, 59, 
                               40, 49, 58, 
                                 48, 57, 
                                   56 };

int diagonal_length[64] = {         1,
                                  2,  2,
                                3,  3,  3,
                              4,  4,  4,  4,
                            5,  5,  5,  5,  5,
                          6,  6,  6,  6,  6,  6,
                        7,  7,  7,  7,  7,  7,  7,
                      8,  8,  8,  8,  8,  8,  8,  8,
                        7,  7,  7,  7,  7,  7,  7,
                          6,  6,  6,  6,  6,  6,
                            5,  5,  5,  5,  5,
                              4,  4,  4,  4,
                                3,  3,  3,
                                  2,  2,
                                    1 };

void Initialize(int continuing) {
/*
 ----------------------------------------------------------
|                                                          |
|   perform routine initialization.                        |
|                                                          |
 ----------------------------------------------------------
*/
  char log_filename[64];
  char history_filename[64];
  char command[80];

#if !defined(HAS_64BITS)
  Initialize_Zero_Masks();
  Initialize_Population_Count();
#endif

  Initialize_Masks();
  Initialize_Random_Hash();
  Initialize_Attack_Boards();
  Initialize_Pawn_Masks();
  Initialize_Piece_Masks();

  Initialize_Chess_Board(&position[0]);

  book_file=fopen("book.bin","rb");
  if (!book_file) printf("unable to open book file [book.bin].\n");
  books_file=fopen("books.bin","rb");
  if (!books_file) printf("unable to open book file [books.bin].\n");

  for (log_id=1;log_id <300;log_id++) {
    sprintf(log_filename,"log.%d",log_id);
    sprintf(history_filename,"game.%d",log_id);
    log_file=fopen(log_filename,"r");
    if (!log_file) break;
    fclose(log_file);
  }
  if (continuing) {
    log_id--;
    sprintf(log_filename,"log.%d",log_id);
    sprintf(history_filename,"game.%d",log_id);
    log_file=fopen(log_filename,"r+");
    history_file=fopen(history_filename,"r+");
    if (!log_file || !history_file) {
      printf("\nsorry.  nothing to continue.\n\n");
      log_file=fopen("log.1","w");
      history_file=fopen("game.1","w+");
    }
    else {
      sprintf(command,"read=game.%d",log_id);
      (void) Option(command);
    }
  }
  else {
    log_file=fopen(log_filename,"w");
    history_file=fopen(history_filename,"w+");
  }

  trans_ref_w=malloc(16*(hash_table_size+4096));
  trans_ref_b=malloc(16*(hash_table_size+4096));
  pawn_hash_table=malloc(8*pawn_hash_table_size);
  pawn_hash_table_x=malloc(4*pawn_hash_table_size);
  king_hash_table=malloc(8*king_hash_table_size);
  Initialize_Hash_Tables();
  if (!trans_ref_w || !trans_ref_b) {
    printf("malloc() failed, not enough memory.\n");
    free(trans_ref_w);
    free(trans_ref_b);
    free(pawn_hash_table);
    free(pawn_hash_table_x);
    free(king_hash_table);
    hash_table_size=0;
    pawn_hash_table_size=0;
    log_hash_table_size=0;
    log_pawn_hash_table_size=0;
    trans_ref_w=0;
    trans_ref_b=0;
    pawn_hash_table=0;
    pawn_hash_table_x=0;
    king_hash_table=0;
  }
  hash_mask=Mask(128-log_hash_table_size);
  pawn_hash_mask=Mask(128-log_pawn_hash_table_size);
  king_hash_mask=Mask(128-log_king_hash_table_size);
}

void Initialize_Attack_Boards(void)
{
  int diag_sq[64] = {                0,
                                   1,  0,
                                 2,  1,  0,
                               3,  2,  1,  0,
                             4,  3,  2,  1,  0,
                           5,  4,  3,  2,  1,  0,
                         6,  5,  4,  3,  2,  1,  0,
                       7,  6,  5,  4,  3,  2,  1,  0,
                         6,  5,  4,  3,  2,  1,  0,
                           5,  4,  3,  2,  1,  0,
                             4,  3,  2,  1,  0,
                               3,  2,  1,  0,
                                 2,  1,  0,
                                   1,  0,
                                     0 };

  int bias_rl45[64] = {              0,
                                   1,  1,
                                 3,  3,  3,
                               6,  6,  6,  6,
                            10, 10, 10, 10, 10,
                          15, 15, 15, 15, 15, 15,
                        21, 21, 21, 21, 21, 21, 21,
                      28, 28, 28, 28, 28, 28, 28, 28,
                        36, 36, 36, 36, 36, 36, 36,
                          43, 43, 43, 43, 43, 43,
                            49, 49, 49, 49, 49,
                              54, 54, 54, 54,
                                58, 58, 58,
                                  61, 61,
                                    63 };

  int i, j, frank, ffile, mask, trank, tfile;
  int sq, rsq, tsq, lastsq;
  int square, pcs, attacks;
  int knightsq[8]={-17,-15,-10,-6,6,10,15,17};
  int bishopsq[4]={-9,-7,7,9};
  int rooksq[4]={-8,-1,1,8};
  BITBOARD sqs;
/*
   initialize pawn attack boards
*/
  for(i=0;i<64;i++) {
    w_pawn_attacks[i]=0;
    if (i < 56)
      for(j=2;j<4;j++) {
        sq=i+bishopsq[j];
        if((abs(sq/8-i/8)==1) && 
           (abs((sq&7) - (i&7))==1) &&
              (sq < 64) && (sq > -1)) 
          w_pawn_attacks[i]=Or(w_pawn_attacks[i],Shiftr(mask_1,sq));
      }
    b_pawn_attacks[i]=0;
    if (i > 7)
      for(j=0;j<2;j++) {
        sq=i+bishopsq[j];
        if((abs(sq/8-i/8)==1) && 
           (abs((sq&7)-(i&7))==1) &&
              (sq < 64) && (sq > -1)) 
          b_pawn_attacks[i]=Or(b_pawn_attacks[i],Shiftr(mask_1,sq));
      }
  }
/*
   initialize knight attack board 
*/
  for(i=0;i<64;i++) {
    knight_attacks[i]=0;
    frank=i/8;
    ffile=i&7;
    for(j=0;j<8;j++) {
      sq=i+knightsq[j];
      if((sq < 0) || (sq > 63)) continue;
      trank=sq/8;
      tfile=sq&7;
      if((abs(frank-trank) > 2) || 
         (abs(ffile-tfile) > 2)) continue;
      knight_attacks[i]=Or(knight_attacks[i],Shiftr(mask_1,sq));
    }
  }
/*
   initialize bishop/queen attack boards and masks
*/
  for(i=0;i<64;i++) {
    bishop_attacks[i]=0;
    for(j=0;j<4;j++) {
      sq=i;
      lastsq=sq;
      sq=sq+bishopsq[j];
      while((abs(sq/8-lastsq/8)==1) && 
            (abs((sq&7)-(lastsq&7))==1) &&
            (sq < 64) && (sq > -1)) {
        bishop_attacks[i]=Or(bishop_attacks[i],Shiftr(mask_1,sq));
        queen_attacks[i]=Or(queen_attacks[i],Shiftr(mask_1,sq));
        if(bishopsq[j]==7)
          mask_plus7dir[i]=Or(mask_plus7dir[i],Shiftr(mask_1,sq));
        else if(bishopsq[j]==9)
          mask_plus9dir[i]=Or(mask_plus9dir[i],Shiftr(mask_1,sq));
        else if(bishopsq[j]==-7)
          mask_minus7dir[i]=Or(mask_minus7dir[i],Shiftr(mask_1,sq));
        else
          mask_minus9dir[i]=Or(mask_minus9dir[i],Shiftr(mask_1,sq));
        lastsq=sq;
        sq=sq+bishopsq[j];
      }
    }
  }
  mask_plus1dir[64]=0;
  mask_plus7dir[64]=0;
  mask_plus8dir[64]=0;
  mask_plus9dir[64]=0;
  mask_minus1dir[64]=0;
  mask_minus7dir[64]=0;
  mask_minus8dir[64]=0;
  mask_minus9dir[64]=0;
/*
   initialize rook/queen attack boards
*/
  for(i=0;i<64;i++) {
    rook_attacks[i]=0;
    for(j=0;j<4;j++) {
      sq=i;
      lastsq=sq;
      sq=sq+rooksq[j];
      while((((abs(sq/8-lastsq/8)==1) && 
             (abs((sq&7)-(lastsq&7))==0)) || 
            ((abs(sq/8-lastsq/8)==0) && 
             (abs((sq&7)-(lastsq&7))==1))) &&
            (sq < 64) && (sq > -1)) {
        rook_attacks[i]=Or(rook_attacks[i],Shiftr(mask_1,sq));
        queen_attacks[i]=Or(queen_attacks[i],Shiftr(mask_1,sq));
        if(rooksq[j]==1)
          mask_plus1dir[i]=Or(mask_plus1dir[i],Shiftr(mask_1,sq));
        else if(rooksq[j]==8)
          mask_plus8dir[i]=Or(mask_plus8dir[i],Shiftr(mask_1,sq));
        else if(rooksq[j]==-1)
          mask_minus1dir[i]=Or(mask_minus1dir[i],Shiftr(mask_1,sq));
        else
          mask_minus8dir[i]=Or(mask_minus8dir[i],Shiftr(mask_1,sq));
        lastsq=sq;
        sq=sq+rooksq[j];
      }
    }
  }
/*
   initialize king attack board 
*/
  for(i=0;i<64;i++) {
    king_attacks[i]=0;
    king_attacks_1[i]=0;
    king_attacks_2[i]=0;
    for (j=0;j<64;j++) {
      if (Distance(i,j) == 1) king_attacks[i]=Or(king_attacks[i],
                                                 set_mask[j]);
      if (Distance(i,j) <= 1) king_attacks_1[i]=Or(king_attacks_1[i],
                                                   set_mask[j]);
      if (Distance(i,j) <= 2) king_attacks_2[i]=Or(king_attacks_2[i],
                                                   set_mask[j]);
    }
  }
/*
  direction[sq1][sq2] gives the "move direction" to move from
  sq1 to sq2.  obstructed[sq1][sq2] gives a bit vector that indicates
  which squares must be unoccupied in order for <sq1> to attack <sq2>,
  assuming a sliding piece is involved.  to use this, you simply have
  to Or(obstructed[sq1][sq2],occupied_squares) and if the result is 
  "0" then a sliding piece on sq1 would attack sq2 and vice-versa.
*/
  for (i=0;i<64;i++) {
    for (j=0;j<64;j++)
      obstructed[i][j]=-1;
    sqs=mask_plus1dir[i];
    while (sqs) {
      j=First_One(sqs);
      directions[i][j]=1;
      obstructed[i][j]=Xor(mask_plus1dir[i],mask_plus1dir[j-1]);
      Clear(j,sqs);
    }
    sqs=mask_plus7dir[i];
    while (sqs) {
      j=First_One(sqs);
      directions[i][j]=7;
      obstructed[i][j]=Xor(mask_plus7dir[i],mask_plus7dir[j-7]);
      Clear(j,sqs);
    }
    sqs=mask_plus8dir[i];
    while (sqs) {
      j=First_One(sqs);
      directions[i][j]=8;
      obstructed[i][j]=Xor(mask_plus8dir[i],mask_plus8dir[j-8]);
      Clear(j,sqs);
    }
    sqs=mask_plus9dir[i];
    while (sqs) {
      j=First_One(sqs);
      directions[i][j]=9;
      obstructed[i][j]=Xor(mask_plus9dir[i],mask_plus9dir[j-9]);
      Clear(j,sqs);
    }
    sqs=mask_minus1dir[i];
    while (sqs) {
      j=First_One(sqs);
      directions[i][j]=-1;
      obstructed[i][j]=Xor(mask_minus1dir[i],mask_minus1dir[j+1]);
      Clear(j,sqs);
    }
    sqs=mask_minus7dir[i];
    while (sqs) {
      j=First_One(sqs);
      directions[i][j]=-7;
      obstructed[i][j]=Xor(mask_minus7dir[i],mask_minus7dir[j+7]);
      Clear(j,sqs);
    }
    sqs=mask_minus8dir[i];
    while (sqs) {
      j=First_One(sqs);
      directions[i][j]=-8;
      obstructed[i][j]=Xor(mask_minus8dir[i],mask_minus8dir[j+8]);
      Clear(j,sqs);
    }
    sqs=mask_minus9dir[i];
    while (sqs) {
      j=First_One(sqs);
      directions[i][j]=-9;
      obstructed[i][j]=Xor(mask_minus9dir[i],mask_minus9dir[j+9]);
      Clear(j,sqs);
    }
  }
/*
  initialize the rotated attack board that is based on the
  normal chess board.
*/
  for (square=0;square<64;square++) {
    for (i=0;i<256;i++) {
      rook_attacks_r0[square][i]=0;
      rook_mobility_r0[square][i]=0;
    }
    for (pcs=0;pcs<256;pcs++) {
      attacks=Initialize_Find_Attacks(7-(square&7),pcs,8);
      while (attacks) {
        sq=first_ones_8bit[attacks];
        rook_attacks_r0[square][pcs]=
          Or(rook_attacks_r0[square][pcs],set_mask[(square&56)+sq]);
        attacks=attacks&(~(1<<(7-sq)));
      }
      rook_mobility_r0[square][pcs]=Popcnt(rook_attacks_r0[square][pcs]);
    }
  }
/*
  initialize the rotated attack board that is based on one that
  rotated left 90 degrees (which lines up a file horizontally,
  rather than its normal vertical orientation.)
*/
  for (square=0;square<64;square++) {
    for (i=0;i<256;i++) {
      rook_attacks_rl90[square][i]=0;
      rook_mobility_rl90[square][i]=0;
    }
    for (pcs=0;pcs<256;pcs++) {
      attacks=Initialize_Find_Attacks(square>>3,pcs,8);
      while (attacks) {
        sq=first_ones_8bit[attacks];
        rook_attacks_rl90[square][pcs]=
          Or(rook_attacks_rl90[square][pcs],
             set_mask[init_r90[((square&7)<<3)+sq]]);
        attacks=attacks&(~(1<<(7-sq)));
      }
      rook_mobility_rl90[square][pcs]=Popcnt(rook_attacks_rl90[square][pcs]);
    }
  }
/*
  initialize the rotated attack board that is based on one that is 
  rotated left 45 degrees (which lines up the (a8-h1) diagonal 
  horizontally.
*/
  for (square=0;square<64;square++) {
    for (i=0;i<256;i++) {
      bishop_attacks_rl45[square][i]=0;
      bishop_mobility_rl45[square][i]=0;
    }
    for (pcs=0;pcs<(1<<diagonal_length[init_l45[square]]);pcs++) {
      rsq=init_l45[square];
      tsq=diag_sq[rsq];
      attacks=Initialize_Find_Attacks(tsq,pcs,diagonal_length[rsq])<<
                        (8-diagonal_length[rsq]);
      while (attacks) {
        sq=first_ones_8bit[attacks];
        bishop_attacks_rl45[square][pcs]=
          Or(bishop_attacks_rl45[square][pcs],
             set_mask[init_ul45[sq+bias_rl45[rsq]]]);
        attacks=attacks&(~(1<<(7-sq)));
      }
    }
    mask=(1<<diagonal_length[init_l45[square]])-1;
    for (pcs=0;pcs<256;pcs++) {
      if ((pcs&mask) != pcs)
        bishop_attacks_rl45[square][pcs]=
          bishop_attacks_rl45[square][pcs&mask];
      bishop_mobility_rl45[square][pcs]=
        Popcnt(bishop_attacks_rl45[square][pcs]);
    }
  }
/*
  initialize the rotated attack board that is based on one that is 
  rotated right 45 degrees (which lines up the (a1-h8) diagonal
  horizontally,
*/
  for (square=0;square<64;square++) {
    for (i=0;i<256;i++) {
      bishop_attacks_rr45[square][i]=0;
      bishop_mobility_rr45[square][i]=0;
    }
    for (pcs=0;pcs<(1<<diagonal_length[init_r45[square]]);pcs++) {
      rsq=init_r45[square];
      tsq=diag_sq[rsq];
      attacks=Initialize_Find_Attacks(tsq,pcs,diagonal_length[rsq])<<
                        (8-diagonal_length[rsq]);
      while (attacks) {
        sq=first_ones_8bit[attacks];
        bishop_attacks_rr45[square][pcs]=
          Or(bishop_attacks_rr45[square][pcs],
             set_mask[init_ur45[sq+bias_rl45[rsq]]]);
        attacks=attacks&(~(1<<(7-sq)));
      }
    }
    mask=(1<<diagonal_length[init_r45[square]])-1;
    for (pcs=0;pcs<256;pcs++) {
      if ((pcs&mask) != pcs)
        bishop_attacks_rr45[square][pcs]=
          bishop_attacks_rr45[square][pcs&mask];
      bishop_mobility_rr45[square][pcs]=
        Popcnt(bishop_attacks_rr45[square][pcs]);
    }
  }
}

void Initialize_Chess_Board(CHESS_POSITION *new_pos)
{
  int i;

  for(i=0;i<64;i++) new_pos->board.board[i]=0;
  new_pos->moves_since_cap_or_push=0;
  opening=1;
  middle_game=0;
  end_game=0;
/*
   place pawns
*/
  for (i=0;i<8;i++) {
    new_pos->board.board[i+8]=1;
    new_pos->board.board[i+48]=-1;
  }
/*
   place knights
*/
  new_pos->board.board[1]=2;
  new_pos->board.board[6]=2;
  new_pos->board.board[57]=-2;
  new_pos->board.board[62]=-2;
/*
   place bishops
*/
  new_pos->board.board[2]=3;
  new_pos->board.board[5]=3;
  new_pos->board.board[58]=-3;
  new_pos->board.board[61]=-3;
/*
   place rooks
*/
  new_pos->board.board[0]=4;
  new_pos->board.board[7]=4;
  new_pos->board.board[56]=-4;
  new_pos->board.board[63]=-4;
/*
   place queens
*/
  new_pos->board.board[3]=5;
  new_pos->board.board[59]=-5;
/*
   place kings
*/
  new_pos->board.board[4]=6;
  new_pos->board.board[60]=-6;
/*
   initialize castling status so all castling is legal.
*/
  new_pos->board.w_castle=3;
  new_pos->board.b_castle=3;
/*
   initialize enpassant status.
*/
  new_pos->board.enpassant_target=0;
/*
   now, set the bit-boards.
*/
  Set_Chess_Bit_Boards(new_pos);
}

void Set_Chess_Bit_Boards(CHESS_POSITION *new_pos)
{
  int i;
  new_pos->board.hash_key=0;
  new_pos->board.pawn_hash_key=0;
/*
   place pawns
*/
  new_pos->board.w_pawn=0;
  new_pos->board.b_pawn=0;
  for (i=0;i<64;i++) {
    if(new_pos->board.board[i]==1) {
      new_pos->board.w_pawn=Or(new_pos->board.w_pawn,set_mask[i]);
      new_pos->board.hash_key=Xor(new_pos->board.hash_key,w_pawn_random[i]);
      new_pos->board.pawn_hash_key=Xor(new_pos->board.pawn_hash_key,
                                        w_pawn_random[i]);
    }
    if(new_pos->board.board[i]==-1) {
      new_pos->board.b_pawn=Or(new_pos->board.b_pawn,set_mask[i]);
      new_pos->board.hash_key=Xor(new_pos->board.hash_key,b_pawn_random[i]);
      new_pos->board.pawn_hash_key=Xor(new_pos->board.pawn_hash_key,
                                        b_pawn_random[i]);
    }
  }
/*
   place knights
*/
  new_pos->board.w_knight=0;
  new_pos->board.b_knight=0;
  for (i=0;i<64;i++) {
    if(new_pos->board.board[i] == 2) {
      new_pos->board.w_knight=Or(new_pos->board.w_knight,set_mask[i]);
      new_pos->board.hash_key=Xor(new_pos->board.hash_key,w_knight_random[i]);
    }
    if(new_pos->board.board[i] == -2) {
      new_pos->board.b_knight=Or(new_pos->board.b_knight,set_mask[i]);
      new_pos->board.hash_key=Xor(new_pos->board.hash_key,b_knight_random[i]);
    }
  }
/*
   place bishops
*/
  new_pos->board.w_bishop=0;
  new_pos->board.b_bishop=0;
  for (i=0;i<64;i++) {
    if(new_pos->board.board[i] == 3) {
      new_pos->board.w_bishop=Or(new_pos->board.w_bishop,set_mask[i]);
      new_pos->board.hash_key=Xor(new_pos->board.hash_key,w_bishop_random[i]);
    }
    if(new_pos->board.board[i] == -3) {
      new_pos->board.b_bishop=Or(new_pos->board.b_bishop,set_mask[i]);
      new_pos->board.hash_key=Xor(new_pos->board.hash_key,b_bishop_random[i]);
    }
  }
/*
   place rooks
*/
  new_pos->board.w_rook=0;
  new_pos->board.b_rook=0;
  for (i=0;i<64;i++) {
    if(new_pos->board.board[i] == 4) {
      new_pos->board.w_rook=Or(new_pos->board.w_rook,set_mask[i]);
      new_pos->board.hash_key=Xor(new_pos->board.hash_key,w_rook_random[i]);
    }
    if(new_pos->board.board[i] == -4) {
      new_pos->board.b_rook=Or(new_pos->board.b_rook,set_mask[i]);
      new_pos->board.hash_key=Xor(new_pos->board.hash_key,b_rook_random[i]);
    }
  }
/*
   place queens
*/
  new_pos->board.w_queen=0;
  new_pos->board.b_queen=0;
  for (i=0;i<64;i++) {
    if(new_pos->board.board[i] == 5) {
      new_pos->board.w_queen=Or(new_pos->board.w_queen,set_mask[i]);
      new_pos->board.hash_key=Xor(new_pos->board.hash_key,w_queen_random[i]);
    }
    if(new_pos->board.board[i] == -5) {
      new_pos->board.b_queen=Or(new_pos->board.b_queen,set_mask[i]);
      new_pos->board.hash_key=Xor(new_pos->board.hash_key,b_queen_random[i]);
    }
  }
/*
   place kings
*/
  new_pos->board.w_king=0;
  new_pos->board.b_king=0;
  for (i=0;i<64;i++) {
    if(new_pos->board.board[i] == 6) {
      new_pos->board.w_king=Or(new_pos->board.w_king,set_mask[i]);
      new_pos->board.white_king=i;
      new_pos->board.hash_key=Xor(new_pos->board.hash_key,w_king_random[i]);
    }
    if(new_pos->board.board[i] == -6) {
      new_pos->board.b_king=Or(new_pos->board.b_king,set_mask[i]);
      new_pos->board.black_king=i;
      new_pos->board.hash_key=Xor(new_pos->board.hash_key,b_king_random[i]);
    }
  }
/*
   initialize combination boards that show multiple pieces.
*/
  new_pos->board.bishops_queens=Or(Or(Or(new_pos->board.w_bishop,
                                          new_pos->board.w_queen),
                                       new_pos->board.b_bishop),
                                    new_pos->board.b_queen);
  new_pos->board.rooks_queens=Or(Or(Or(new_pos->board.w_rook,
                                        new_pos->board.w_queen),
                                     new_pos->board.b_rook),
                                  new_pos->board.b_queen);
  new_pos->board.w_occupied=Or(Or(Or(Or(Or(new_pos->board.w_pawn,
                                            new_pos->board.w_knight),
                                         new_pos->board.w_bishop),
                                      new_pos->board.w_rook),
                                   new_pos->board.w_queen),                                                    new_pos->board.w_king);
  new_pos->board.b_occupied=Or(Or(Or(Or(Or(new_pos->board.b_pawn,
                                            new_pos->board.b_knight),
                                         new_pos->board.b_bishop),
                                      new_pos->board.b_rook),
                                   new_pos->board.b_queen),
                                new_pos->board.b_king);
/*
  now initialize rotated occupied bitboards.
*/
  new_pos->board.occupied_rl90=0;
  new_pos->board.occupied_rl45=0;
  new_pos->board.occupied_rr45=0;
  for (i=0;i<64;i++) {
    if (new_pos->board.board[i]) {
      new_pos->board.occupied_rl90=
        Or(new_pos->board.occupied_rl90,
           set_mask_rl90[i]);
      new_pos->board.occupied_rl45=
        Or(new_pos->board.occupied_rl45,
           set_mask_rl45[i]);
      new_pos->board.occupied_rr45=
        Or(new_pos->board.occupied_rr45,
           set_mask_rr45[i]);
    }
  }
/*
   initialize black/white piece counts.
*/
  new_pos->board.white_pieces=0;
  new_pos->board.white_pawns=0;
  new_pos->board.black_pieces=0;
  new_pos->board.black_pawns=0;
  new_pos->board.material_evaluation=0;
  for (i=0;i<64;i++) {
    switch (new_pos->board.board[i]) {
      case pawn:
        new_pos->board.material_evaluation+=PAWN_VALUE;
        new_pos->board.white_pawns+=1;
        break;
      case knight:
        new_pos->board.material_evaluation+=KNIGHT_VALUE;
        new_pos->board.white_pieces+=3;
        break;
      case bishop:
        new_pos->board.material_evaluation+=BISHOP_VALUE;
        new_pos->board.white_pieces+=3;
        break;
      case rook:
        new_pos->board.material_evaluation+=ROOK_VALUE;
        new_pos->board.white_pieces+=5;
        break;
      case queen:
        new_pos->board.material_evaluation+=QUEEN_VALUE;
        new_pos->board.white_pieces+=9;
          break;
      case -pawn:
        new_pos->board.material_evaluation-=PAWN_VALUE;
        new_pos->board.black_pawns+=1;
        break;
      case -knight:
        new_pos->board.material_evaluation-=KNIGHT_VALUE;
        new_pos->board.black_pieces+=3;
        break;
      case -bishop:
        new_pos->board.material_evaluation-=BISHOP_VALUE;
        new_pos->board.black_pieces+=3;
        break;
      case -rook:
        new_pos->board.material_evaluation-=ROOK_VALUE;
        new_pos->board.black_pieces+=5;
        break;
      case -queen:
        new_pos->board.material_evaluation-=QUEEN_VALUE;
        new_pos->board.black_pieces+=9;
        break;
      default:
        ;
    }
    if (new_pos == &position[0]) repetition_head=0;
  }
}

/*
********************************************************************************
*                                                                              *
*   Initlialize_Find_Attacks() is used to find the attacks from <square> that  *
*   exist on the 8-bit vector supplied as <pieces>.  <pieces> represents a     *
*   rank, file or diagonal, based on the rotated bit-boards.                   *
*                                                                              *
********************************************************************************
*/
int Initialize_Find_Attacks(int square, int pieces, int length)
{
  int result, start;
  result=0;
/*
 ----------------------------------------------------------
|                                                          |
|   find attacks to left of <square>.                      |
|                                                          |
 ----------------------------------------------------------
*/
  if (square < 7) {
    start=1<<(square+1);
    while (start < 256) {
      result=result | start;
      if (pieces & start) break;
      start=start<<1;
    }
  }
/*
 ----------------------------------------------------------
|                                                          |
|   find attacks to left of <square>.                      |
|                                                          |
 ----------------------------------------------------------
*/
  if (square > 0) {
    start=1<<(square-1);
    while (start > 0) {
      result=result | start;
      if (pieces & start) break;
      start=start>>1;
    }
  }
/*
  printf("square=%d  pieces=%d\n",square,pieces);
  printf("result=%d\n",result);
  printf("length=%d  result=%d\n",length,result&(1<<length)-1);
*/
  return(result&((1<<length)-1));
}

void Initialize_Hash_Tables(void)
{
  int i;
  for (i=0;i<(hash_table_size+REHASH_EXTRA);i++) {
    (trans_ref_w+i)->word1=0;
    (trans_ref_w+i)->word2=0;
    (trans_ref_b+i)->word1=0;
    (trans_ref_b+i)->word2=0;
  }
  for (i=0;i<pawn_hash_table_size;i++) {
    *(pawn_hash_table+i)=0;
  }
  for (i=0;i<pawn_hash_table_size/2;i++) {
    *(pawn_hash_table_x+i)=0;
  }
  for (i=0;i<king_hash_table_size;i++) {
    *(king_hash_table+i)=0;
  }
}

void Initialize_Masks(void)
{

  int i, j;
/*
  specific masks to avoid Mask() procedure call if possible.
*/
  #if !defined(HAS_64BITS)
    mask_1=Mask(1);
    mask_2=Mask(2);
    mask_3=Mask(3);
    mask_4=Mask(4);
    mask_8=Mask(8);
    mask_32=Mask(32);
    mask_72=Mask(72);
    mask_80=Mask(80);
    mask_96=Mask(96);
    mask_107=Mask(107);
    mask_108=Mask(108);
    mask_112=Mask(112);
    mask_118=Mask(118);
    mask_120=Mask(120);
    mask_121=Mask(121);
    mask_127=Mask(127);
  #endif
  mask_clear_entry=Compl(Or(Shiftl(Mask(108),21),Shiftr(Mask(2),1)));
/*
  masks to set/clear a bit on a specific square
*/
  for (i=0;i<64;i++) {
    clear_mask[i]=Compl(Shiftr(mask_1,i));
    clear_mask_rl45[i]=Compl(Shiftr(mask_1,init_l45[i]));
    clear_mask_rr45[i]=Compl(Shiftr(mask_1,init_r45[i]));
    clear_mask_rl90[i]=Compl(Shiftr(mask_1,init_l90[i]));
    set_mask[i]=Shiftr(mask_1,i);
    set_mask_rl45[i]=Shiftr(mask_1,init_l45[i]);
    set_mask_rr45[i]=Shiftr(mask_1,init_r45[i]);
    set_mask_rl90[i]=Shiftr(mask_1,init_l90[i]);
  }
  clear_mask[64]=0;
  clear_mask_rl45[64]=0;
  clear_mask_rr45[64]=0;
  clear_mask_rl90[64]=0;
  set_mask[64]=0;
  set_mask_rl45[64]=0;
  set_mask_rr45[64]=0;
  set_mask_rl90[64]=0;
/*
  do {
    scanf("%d",&i);
    printf("set right 45 degrees\n");
    Display_2_Bit_Boards(set_mask[i],set_mask_r45[i]);
    printf("clear right 45 degrees\n");
    Display_2_Bit_Boards(clear_mask[i],clear_mask_r45[i]);
  } while (i);
*/
/*
  masks to select bits on a specific rank or file
*/
  rank_mask[0]=mask_8;
  for (i=1;i<8;i++) rank_mask[i]=Shiftr(rank_mask[i-1],8);
  file_mask[0]=mask_1;
  for (i=1;i<8;i++) file_mask[0]=Or(file_mask[0],Shiftr(file_mask[0],8));
  for (i=1;i<8;i++) file_mask[i]=Shiftr(file_mask[i-1],1);
/*
  masks to select bits on either half of board
*/
  for (i=0;i<8;i++) {
    right_side_mask[i]=0;
    for (j=i+2;j<8;j++)
      right_side_mask[i]=Or(right_side_mask[i],file_mask[j]);
    left_side_mask[i]=0;
    for (j=i-2;j>=0;j--)
      left_side_mask[i]=Or(left_side_mask[i],file_mask[j]);
  }
  for (i=0;i<8;i++) {
    right_side_empty_mask[i]=0;
    for (j=i+1;j<8;j++)
      right_side_empty_mask[i]=Or(right_side_empty_mask[i],file_mask[j]);
    left_side_empty_mask[i]=0;
    for (j=i-1;j>=0;j--)
      left_side_empty_mask[i]=Or(left_side_empty_mask[i],file_mask[j]);
  }
  right_half_mask=Or(Or(Or(file_mask[4],file_mask[5]),
                        file_mask[6]),
                     file_mask[7]);
  left_half_mask=Or(Or(Or(file_mask[0],file_mask[1]),
                       file_mask[2]),
                    file_mask[3]);
   mask_white_space=Or(Or(Or(rank_mask[4],rank_mask[5]),
                          rank_mask[6]),
                       rank_mask[7]);
   mask_white_pawns_space=Or(Or(Or(Or(rank_mask[3],rank_mask[4]),
                                   rank_mask[5]),
                                rank_mask[6]),
                             rank_mask[7]);
   mask_black_space=Or(Or(Or(rank_mask[0],rank_mask[1]),
                          rank_mask[2]),
                       rank_mask[3]);
   mask_black_pawns_space=Or(Or(Or(Or(rank_mask[0],rank_mask[1]),
                                   rank_mask[2]),
                                rank_mask[3]),
                             rank_mask[4]);
}

void Initialize_Pawn_Masks(void)
{
  int i;
  BITBOARD m1,m2;
/*
    initialize isolated pawn masks, which are nothing more than 1's on
    the files adjacent to the pawn file.
*/
  for (i=0;i<64;i++) {
    if (!(i&7)) {
      mask_pawn_isolated[i]=file_mask[(i&7)+1];
    }
    else if ((i&7) == 7) {
      mask_pawn_isolated[i]=file_mask[(i&7)-1];
    }
    else {
      mask_pawn_isolated[i]=Or(file_mask[(i&7)-1],file_mask[(i&7)+1]);
    }
  }
/*
    initialize artificially isolated pawn masks, which are nothing 
    more than 1's on the files adjacent to the pawn file.  however,
    these 1's are used to test if a pawn is 1 move away from
    supporting this pawn.
*/
  for (i=0;i<64;i++) {
    mask_pawn_artificially_isolated_w[i]=0;
    if (i > 7) {
      if (!(i&7)) {
        mask_pawn_artificially_isolated_w[i]=mask_minus8dir[i+1];
        if (i > 15)
          mask_pawn_artificially_isolated_w[i]=
            Xor(mask_pawn_artificially_isolated_w[i],mask_minus8dir[i-15]);
      }
      else if ((i&7) == 7) {
        mask_pawn_artificially_isolated_w[i]=mask_minus8dir[i-1];
        if (i > 15)
          mask_pawn_artificially_isolated_w[i]=
            Xor(mask_pawn_artificially_isolated_w[i],mask_minus8dir[i-17]);
      }
      else {
        mask_pawn_artificially_isolated_w[i]=Or(mask_minus8dir[i-1],
                                                mask_minus8dir[i+1]);
        if (i > 15)
          mask_pawn_artificially_isolated_w[i]=
            Xor(mask_pawn_artificially_isolated_w[i],mask_minus8dir[i-15]);
        if (i > 15)
          mask_pawn_artificially_isolated_w[i]=
            Xor(mask_pawn_artificially_isolated_w[i],mask_minus8dir[i-17]);
      }
    }
  }
  for (i=0;i<64;i++) {
    mask_pawn_artificially_isolated_b[i]=0;
    if (i < 56) {
      if (!(i&7)) {
        mask_pawn_artificially_isolated_b[i]=mask_plus8dir[i+1];
        if (i < 49)
          mask_pawn_artificially_isolated_b[i]=
            Xor(mask_pawn_artificially_isolated_b[i],mask_plus8dir[i+17]);
      }
      else if ((i&7) == 7) {
        mask_pawn_artificially_isolated_b[i]=mask_plus8dir[i-1];
        if (i < 49)
          mask_pawn_artificially_isolated_b[i]=
            Xor(mask_pawn_artificially_isolated_b[i],mask_plus8dir[i+15]);
      }
      else {
        mask_pawn_artificially_isolated_b[i]=Or(mask_plus8dir[i-1],
                                                mask_plus8dir[i+1]);
        if (i < 49)
          mask_pawn_artificially_isolated_b[i]=
            Xor(mask_pawn_artificially_isolated_b[i],mask_plus8dir[i+15]);
        if (i < 49)
          mask_pawn_artificially_isolated_b[i]=
            Xor(mask_pawn_artificially_isolated_b[i],mask_plus8dir[i+17]);
      }
    }
  }
/*
  do {
    scanf("%d",&i);
    printf("black\n");
    Display_Bit_Board(mask_pawn_artificially_isolated_b[i]);
  } while (i);
*/
/*
    initialize connected pawn masks, which are nothing more than 1's on
    files adjacent to the pawn and ranks that are within 1 rank of the
    pawn.
*/
  for (i=8;i<57;i++) {
    if (((i&7)>0) && ((i&7)<7))
      mask_pawn_connected[i]=Or(Or(Or(Or(Or(set_mask[i-9],set_mask[i-7]),
                                         set_mask[i-1]),
                                      set_mask[i+1]),
                                   set_mask[i+7]),
                                set_mask[i+9]);
    else if ((i&7)==0)
      mask_pawn_connected[i]=Or(Or(set_mask[i-7],set_mask[i+1]),
                                set_mask[i+9]);
    else if ((i&7)==7)
      mask_pawn_connected[i]=Or(Or(set_mask[i-9],set_mask[i-1]),
                                set_mask[i+7]);
  }
/*
    initialize passed pawn masks, which are nothing more than 1's on
    the pawn's file and the adjacent files, but only on ranks that are
    in "front" of the pawn.  
*/
  for (i=0;i<64;i++) {
    if (!(i&7)) {
      mask_pawn_passed_w[i]=Or(mask_plus8dir[i],mask_plus8dir[i+1]);
      mask_pawn_passed_b[i]=Or(mask_minus8dir[i],mask_minus8dir[i+1]);
    }
    else if ((i&7) == 7) {
      mask_pawn_passed_w[i]=Or(mask_plus8dir[i-1],mask_plus8dir[i]);
      mask_pawn_passed_b[i]=Or(mask_minus8dir[i-1],mask_minus8dir[i]);
    }
    else {
      mask_pawn_passed_w[i]=Or(Or(mask_plus8dir[i-1],mask_plus8dir[i]),
                               mask_plus8dir[i+1]);
      mask_pawn_passed_b[i]=Or(Or(mask_minus8dir[i-1],mask_minus8dir[i]),
                               mask_minus8dir[i+1]);
    }
  }
/*
    these masks are used to determine if the other side has any pawns
    that can attack [square].
*/
  for (i=8;i<56;i++) {
    if (!(i&7)) {
      mask_no_pawn_attacks_w[i]=mask_minus8dir[i+1];
      mask_no_pawn_attacks_b[i]=mask_plus8dir[i+1];
    }
    else if ((i&7) == 7) {
      mask_no_pawn_attacks_w[i]=mask_minus8dir[i-1];
      mask_no_pawn_attacks_b[i]=mask_plus8dir[i-1];
    }
    else {
      mask_no_pawn_attacks_w[i]=Or(mask_minus8dir[i-1],mask_minus8dir[i+1]);
      mask_no_pawn_attacks_b[i]=Or(mask_plus8dir[i+1],mask_plus8dir[i-1]);
    }
  }
/*
    backward pawns are masked by almost the exact opposite, there must
    be one friendly pawn even or behind on an adjacent file.
*/
  for (i=8;i<56;i++) {
    if (!(i&7)) {
      mask_pawn_backward_w[i]=mask_minus8dir[i+1];
      mask_pawn_backward_b[i]=mask_plus8dir[i+1];
    }
    else if ((i&7) == 7) {
      mask_pawn_backward_w[i]=mask_minus8dir[i-1];
      mask_pawn_backward_b[i]=mask_plus8dir[i-1];
    }
    else {
      mask_pawn_backward_w[i]=Or(mask_minus8dir[i-1],mask_minus8dir[i+1]);
      mask_pawn_backward_b[i]=Or(mask_plus8dir[i+1],mask_plus8dir[i-1]);
    }
  }
  for (i=24;i<56;i++) {
    if (!(i&7)) {
      mask_pawn_backward_w[i]=Xor(mask_pawn_backward_w[i],
                                  mask_minus8dir[i-15]);
    }
    else if ((i&7) == 7) {
      mask_pawn_backward_w[i]=Xor(mask_pawn_backward_w[i],
                                  mask_minus8dir[i-17]);
    }
    else {
      mask_pawn_backward_w[i]=Xor(mask_pawn_backward_w[i],
                                  Or(mask_minus8dir[i-15],
                                     mask_minus8dir[i-17]));
    }
  }
  for (i=8;i<40;i++) {
    if (!(i&7)) {
      mask_pawn_backward_b[i]=Xor(mask_pawn_backward_b[i],
                                  mask_plus8dir[i+17]);
    }
    else if ((i&7) == 7) {
      mask_pawn_backward_b[i]=Xor(mask_pawn_backward_b[i],
                                  mask_plus8dir[i+15]);
    }
    else {
      mask_pawn_backward_b[i]=Xor(mask_pawn_backward_b[i],
                                  Or(mask_plus8dir[i+15],
                                     mask_plus8dir[i+17]));
    }
  }
/*
    enpassant pawns are on either file adjacent to the current file, and
    on the same rank.                                          
*/
  for (i=0;i<64;i++) mask_enpassant_test[i]=0;
  for (i=25;i<31;i++) mask_enpassant_test[i]=Or(set_mask[i-1],set_mask[i+1]);
  for (i=33;i<39;i++) mask_enpassant_test[i]=Or(set_mask[i-1],set_mask[i+1]);
  mask_enpassant_test[24]=set_mask[25];
  mask_enpassant_test[31]=set_mask[30];
  mask_enpassant_test[32]=set_mask[33];
  mask_enpassant_test[39]=set_mask[38];

/*
  masks to detect pawns bearing down on the king
*/
  mask_kingside_attack_w1=Or(Or(mask_minus8dir[37],mask_minus8dir[38]),
                             mask_minus8dir[39]);
  mask_kingside_attack_w2=Or(Or(mask_minus8dir[29],mask_minus8dir[30]),
                             mask_minus8dir[31]);
  mask_queenside_attack_w1=Or(Or(mask_minus8dir[32],mask_minus8dir[33]),
                              mask_minus8dir[34]);
  mask_queenside_attack_w2=Or(Or(mask_minus8dir[24],mask_minus8dir[25]),
                              mask_minus8dir[26]);
  mask_kingside_attack_b1=Or(Or(mask_plus8dir[29],mask_plus8dir[30]),
                             mask_plus8dir[31]);
  mask_kingside_attack_b2=Or(Or(mask_plus8dir[37],mask_plus8dir[38]),
                             mask_plus8dir[39]);
  mask_queenside_attack_b1=Or(Or(mask_plus8dir[24],mask_plus8dir[25]),
                              mask_plus8dir[26]);
  mask_queenside_attack_b2=Or(Or(mask_plus8dir[32],mask_plus8dir[33]),
                              mask_plus8dir[34]);
/* 
  pawns at d5/e5/f5 cramp black, and pawns at d4/e4/f4 cramp
  white, especially if there are no pawns that can attack
  these pawns.
*/
  pawns_cramp_black=Or(Or(set_mask[35],set_mask[36]),
                       set_mask[37]);
  pawns_cramp_white=Or(Or(set_mask[27],set_mask[28]),
                       set_mask[29]);
/* 
  these two masks have 1's on dark squares and light squares
  to test to see if pawns/bishops are on them.
*/
  m1=Mask(1);
  m2=Shiftr(m1,1);
  for (i=1;i<4;i++) {
    m1=Or(m1,Shiftr(m1,2));
    m2=Or(m2,Shiftr(m2,2));
  }
  for (i=0;i<64;i+=8) {
    if ((i/8)&1) {
      dark_squares=Or(dark_squares,Shiftr(m2,i));
      light_squares=Or(light_squares,Shiftr(m1,i));
    }
    else {
      dark_squares=Or(dark_squares,Shiftr(m1,i));
      light_squares=Or(light_squares,Shiftr(m2,i));
    }
  }
/* 
  these two masks have 1's on everywhere but the left or right
  files, used to prevent pawns from capturing off the edge of
  the board and wrapping around.rom capturing off the edge of
*/
  mask_left_edge=Compl(file_mask[0]);
  mask_right_edge=Compl(file_mask[7]);
  mask_advance_2_w=rank_mask[2];
  mask_advance_2_b=rank_mask[5];
/* 
  this mask has 1's on the 4 corner squares, and is used to detect
  the king sitting right in the corner where it's easier to get
  mated.
*/
  mask_corner_squares=Or(Or(set_mask[0],set_mask[7]),
                         Or(set_mask[56],set_mask[63]));
}

void Initialize_Piece_Masks(void)
{
  int i, j;
/*
    initialize king corner masks, which are 1's on the four squares in
    each corner.
*/
  mask_a1_corner=Or(mask_2,Shiftr(mask_2,8));
  mask_h1_corner=Or(Shiftr(mask_2,6),Shiftr(mask_2,14));
  mask_a8_corner=Or(Shiftr(mask_2,48),Shiftr(mask_2,56));
  mask_h8_corner=Or(Shiftr(mask_2,54),Shiftr(mask_2,62));
/*
    initialize masks used to evaluate development, which includes
    minor piece squares and center pawn squares.
*/
  white_minor_pieces=Or(Shiftr(mask_2,1),Shiftr(mask_2,5));
  black_minor_pieces=Or(Shiftr(mask_2,57),Shiftr(mask_2,61));
  white_center_pawns=Shiftr(mask_2,11);
  black_center_pawns=Shiftr(mask_2,51);
/*
    initialize masks used to evaluate pawn races.  these masks are
    used to determine if the opposing king is in a position to stop a
    passed pawn from racing down and queening.
*/
  for (i=0;i<64;i++) {
    white_pawn_race_wtm[i]=0;
    white_pawn_race_btm[i]=0;
    black_pawn_race_wtm[i]=0;
    black_pawn_race_btm[i]=0;
  }
  for (j=8;j<56;j++) {
    for (i=0;i<64;i++) {
/* white pawn, wtm */
      if (j < 16) {
        if (King_Pawn_Square(j+8,i,(j&7)+56,1)) 
          white_pawn_race_wtm[j]=Or(white_pawn_race_wtm[j],set_mask[i]);
      }
      else {
        if (King_Pawn_Square(j,i,(j&7)+56,1)) 
          white_pawn_race_wtm[j]=Or(white_pawn_race_wtm[j],set_mask[i]);
      }
/* white pawn, !wtm */
      if (j < 16) {
        if (King_Pawn_Square(j+8,i,(j&7)+56,0)) 
          white_pawn_race_btm[j]=Or(white_pawn_race_btm[j],set_mask[i]);
      }
      else {
        if (King_Pawn_Square(j,i,(j&7)+56,0)) 
          white_pawn_race_btm[j]=Or(white_pawn_race_btm[j],set_mask[i]);
      }
/* black pawn, wtm */
      if (j > 47) {
        if (King_Pawn_Square(j-8,i,j&7,0)) 
          black_pawn_race_wtm[j]=Or(black_pawn_race_wtm[j],set_mask[i]);
      }
      else {
        if (King_Pawn_Square(j,i,j&7,0)) 
          black_pawn_race_wtm[j]=Or(black_pawn_race_wtm[j],set_mask[i]);
      }
/* black pawn, !wtm */
      if (j > 47) {
        if (King_Pawn_Square(j-8,i,j&7,1)) 
          black_pawn_race_btm[j]=Or(black_pawn_race_btm[j],set_mask[i]);
      }
      else {
        if (King_Pawn_Square(j,i,j&7,1)) 
          black_pawn_race_btm[j]=Or(black_pawn_race_btm[j],set_mask[i]);
      }
    }
  }
/*
  do {
    scanf("%d",&i);
    printf("white pawns\n");
    Display_2_Bit_Boards(white_pawn_race_wtm[i],white_pawn_race_btm[i]);
    printf("black pawns\n");
    Display_2_Bit_Boards(black_pawn_race_btm[i],black_pawn_race_wtm[i]);
  } while (i);
*/
}

#if !defined(HAS_64BITS)
void Initialize_Population_Count(void)
{
  int i,j,mask,count;
  for (i=0;i<65536;i++){
    mask=32768;
    count=0;
    for(j=0;j<16;j++){
      if ((mask & i)) count++;
      mask=mask>>1;
    }
    population_count[i]=count;
  }
}
#endif

/*
********************************************************************************
*                                                                              *
*   Initialize_Random_Hash() is called to initialize the tables of random      *
*   numbers used to produce the incrementally-updated hash keys.  note that    *
*   this uses a local random number generator rather than the C library one    *
*   since there is no uniformity in the number of bits returned by the         *
*   standard library routines, it varies from 16 bits to 64.                   *
*                                                                              *
********************************************************************************
*/
void Initialize_Random_Hash(void)
{
  int i;
  for (i=0;i<64;i++) {
    w_pawn_random[i]=Random64();
    b_pawn_random[i]=Random64();
    w_knight_random[i]=Random64();
    b_knight_random[i]=Random64();
    w_bishop_random[i]=Random64();
    b_bishop_random[i]=Random64();
    w_rook_random[i]=Random64();
    b_rook_random[i]=Random64();
    w_queen_random[i]=Random64();
    b_queen_random[i]=Random64();
    w_king_random[i]=Random64();
    b_king_random[i]=Random64();
  }
  castle_random_w[0]=0;
  castle_random_b[0]=0;
  for (i=1;i<4;i++) {
    castle_random_w[i]=Random64();
    castle_random_b[i]=Random64();
  }
  enpassant_random[0]=0;
  for (i=1;i<65;i++) {
    enpassant_random[i]=Random64();
  }
  for (i=0;i<2;i++) {
    wtm_random[i]=Random64();
  }
  endgame_random_w=Random64();
  endgame_random_b=Random64();
  w_rooks_random=Random64();
  b_rooks_random=Random64();
}

void Initialize_Zero_Masks(void)
{
  int i,j,maskl,maskr;
#if !defined(HAS_64BITS)
  first_ones[0]=16;
  last_ones[0]=16;
  for (i=1;i<65536;i++){
    maskl=32768;
    for(j=0;j<16;j++){
      if ((maskl & i)){
        first_ones[i]=j;
        break;
      }
      maskl=maskl>>1;
    }
    maskr=1;
    for(j=0;j<16;j++){
      if ((maskr & i)){
        last_ones[i]=15-j;
        break;
      }
      maskr=maskr<<1;
    }
  }
#endif
  first_ones_8bit[0]=8;
  last_ones_8bit[0]=8;
  for (i=1;i<256;i++){
    for(j=0;j<8;j++){
      if (i & (1<<(7-j))){
        first_ones_8bit[i]=j;
        break;
      }
    }
    for(j=7;j>=0;j--){
      if (i & (1<<(7-j))){
        last_ones_8bit[i]=j;
        break;
      }
    }
  }
}
