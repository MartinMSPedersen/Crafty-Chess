#include <stdio.h>
#include <stdlib.h>
#include "types.h"
#include "function.h"
#include "data.h"
#if defined(UNIX) || defined(AMIGA)
#  include <unistd.h>
#endif
#include "epdglue.h"

#if defined(COMPACT_ATTACKS)
extern unsigned char init_l90[];
extern unsigned char init_l45[];
extern unsigned char init_r45[];
#else
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
#endif

void Initialize(int continuing)
{
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

  InitializeZeroMasks();
  InitializeMasks();
  InitializeRandomHash();
  InitializeAttackBoards();
  InitializePawnMasks();
  InitializePieceMasks();

  InitializeChessBoard(&position[0]);

  EGInit();

  last[0]=move_list;

  sprintf(log_filename,"%s/book.bin",BOOKDIR);
  book_file=fopen(log_filename,"rb");
  if (!book_file) printf("unable to open book file [book.bin].\n");
  sprintf(log_filename,"%s/books.bin",BOOKDIR);
  books_file=fopen(log_filename,"rb");
  if (!books_file) printf("unable to open book file [books.bin].\n");

  for (log_id=1;log_id <300;log_id++) {
    sprintf(log_filename,"%s/log.%03d",LOGDIR,log_id);
    sprintf(history_filename,"%s/game.%03d",LOGDIR,log_id);
    log_file=fopen(log_filename,"r");
    if (!log_file) break;
    fclose(log_file);
  }
  if (continuing) {
    log_id--;
    sprintf(log_filename,"%s/log.%03d",LOGDIR,log_id);
    sprintf(history_filename,"%s/game.%03d",LOGDIR,log_id);
    log_file=fopen(log_filename,"r+");
    history_file=fopen(history_filename,"r+");
    if (!log_file || !history_file) {
      printf("\nsorry.  nothing to continue.\n\n");
      sprintf(log_filename,"%s/log.%03d",LOGDIR,1);
      sprintf(history_filename,"%s/game.%03d",LOGDIR,1);
      log_file=fopen("log_filename","w");
      history_file=fopen("history_filename","w+");
    }
    else {
      sprintf(command,"read=game.%03d",log_id);
      (void) Option(command);
    }
  }
  else {
    log_file=fopen(log_filename,"w");
    history_file=fopen(history_filename,"w+");
  }

  trans_ref_wa=malloc(16*hash_table_size);
  trans_ref_wb=malloc(16*2*hash_table_size);
  trans_ref_ba=malloc(16*hash_table_size);
  trans_ref_bb=malloc(16*2*hash_table_size);
  pawn_hash_table=malloc(16*pawn_hash_table_size);
  InitializeHashTables();
  if (!trans_ref_wa || !trans_ref_wb || !trans_ref_ba || !trans_ref_bb ) {
    printf("malloc() failed, not enough memory.\n");
    free(trans_ref_wa);
    free(trans_ref_wb);
    free(trans_ref_ba);
    free(trans_ref_bb);
    free(pawn_hash_table);
    hash_table_size=0;
    pawn_hash_table_size=0;
    log_hash_table_size=0;
    log_pawn_hash_table_size=0;
    trans_ref_wa=0;
    trans_ref_wb=0;
    trans_ref_ba=0;
    trans_ref_bb=0;
    pawn_hash_table=0;
  }
  hash_maska=(1<<log_hash_table_size)-1;
  hash_maskb=(1<<(log_hash_table_size+1))-1;
  pawn_hash_mask=((unsigned int) 037777777777)>>(32-log_pawn_hash_table_size);
}

void InitializeAttackBoards(void)
{

  int i, j, frank, ffile, trank, tfile;
  int sq, lastsq;
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
      if (Distance(i,j) == 1)
        king_attacks[i]=Or(king_attacks[i],set_mask[j]);
      if (Distance(i,j) <= 1)
        king_attacks_1[i]=Or(king_attacks_1[i],set_mask[j]);
      if (Distance(i,j) <= 2)
        king_attacks_2[i]=Or(king_attacks_2[i],set_mask[j]);
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
      j=FirstOne(sqs);
      directions[i][j]=1;
      obstructed[i][j]=Xor(mask_plus1dir[i],mask_plus1dir[j-1]);
      Clear(j,sqs);
    }
    sqs=mask_plus7dir[i];
    while (sqs) {
      j=FirstOne(sqs);
      directions[i][j]=7;
      obstructed[i][j]=Xor(mask_plus7dir[i],mask_plus7dir[j-7]);
      Clear(j,sqs);
    }
    sqs=mask_plus8dir[i];
    while (sqs) {
      j=FirstOne(sqs);
      directions[i][j]=8;
      obstructed[i][j]=Xor(mask_plus8dir[i],mask_plus8dir[j-8]);
      Clear(j,sqs);
    }
    sqs=mask_plus9dir[i];
    while (sqs) {
      j=FirstOne(sqs);
      directions[i][j]=9;
      obstructed[i][j]=Xor(mask_plus9dir[i],mask_plus9dir[j-9]);
      Clear(j,sqs);
    }
    sqs=mask_minus1dir[i];
    while (sqs) {
      j=FirstOne(sqs);
      directions[i][j]=-1;
      obstructed[i][j]=Xor(mask_minus1dir[i],mask_minus1dir[j+1]);
      Clear(j,sqs);
    }
    sqs=mask_minus7dir[i];
    while (sqs) {
      j=FirstOne(sqs);
      directions[i][j]=-7;
      obstructed[i][j]=Xor(mask_minus7dir[i],mask_minus7dir[j+7]);
      Clear(j,sqs);
    }
    sqs=mask_minus8dir[i];
    while (sqs) {
      j=FirstOne(sqs);
      directions[i][j]=-8;
      obstructed[i][j]=Xor(mask_minus8dir[i],mask_minus8dir[j+8]);
      Clear(j,sqs);
    }
    sqs=mask_minus9dir[i];
    while (sqs) {
      j=FirstOne(sqs);
      directions[i][j]=-9;
      obstructed[i][j]=Xor(mask_minus9dir[i],mask_minus9dir[j+9]);
      Clear(j,sqs);
    }
  }
#if defined(COMPACT_ATTACKS)
  ComputeAttacksAndMobility();
#else 
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
    int square, pcs, attacks;
    int rsq, tsq;
    int mask;

/*
  initialize the rotated attack board that is based on the
  normal chess 
*/
    for (square=0;square<64;square++) {
      for (i=0;i<256;i++) {
        rook_attacks_r0[square][i]=0;
        rook_mobility_r0[square][i]=0;
      }
      for (pcs=0;pcs<256;pcs++) {
        attacks=InitializeFindAttacks(7-File(square),pcs,8);
        while (attacks) {
          sq=first_ones_8bit[attacks];
          rook_attacks_r0[square][pcs]=
            Or(rook_attacks_r0[square][pcs],set_mask[(square&56)+sq]);
          attacks=attacks&(~(1<<(7-sq)));
        }
        rook_mobility_r0[square][pcs]=PopCnt(rook_attacks_r0[square][pcs]);
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
        attacks=InitializeFindAttacks(Rank(square),pcs,8);
        while (attacks) {
          sq=first_ones_8bit[attacks];
          rook_attacks_rl90[square][pcs]=
            Or(rook_attacks_rl90[square][pcs],
               set_mask[init_r90[((square&7)<<3)+sq]]);
          attacks=attacks&(~(1<<(7-sq)));
        }
        rook_mobility_rl90[square][pcs]=PopCnt(rook_attacks_rl90[square][pcs]);
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
        attacks=InitializeFindAttacks(tsq,pcs,diagonal_length[rsq])<<
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
          PopCnt(bishop_attacks_rl45[square][pcs]);
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
        attacks=InitializeFindAttacks(tsq,pcs,diagonal_length[rsq])<<
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
          PopCnt(bishop_attacks_rr45[square][pcs]);
      }
    }
  }
#endif
}

void InitializeChessBoard(SEARCH_POSITION *new_pos)
{
  int i;

  for(i=0;i<64;i++) search.board[i]=empty;
  new_pos->rule_50_moves=0;
  opening=1;
  middle_game=0;
  end_game=0;
/*
   place pawns
*/
  for (i=0;i<8;i++) {
    search.board[i+8]=pawn;
    search.board[i+48]=-pawn;
  }
/*
   place knights
*/
  search.board[1]=knight;
  search.board[6]=knight;
  search.board[57]=-knight;
  search.board[62]=-knight;
/*
   place bishops
*/
  search.board[2]=bishop;
  search.board[5]=bishop;
  search.board[58]=-bishop;
  search.board[61]=-bishop;
/*
   place rooks
*/
  search.board[0]=rook;
  search.board[7]=rook;
  search.board[56]=-rook;
  search.board[63]=-rook;
/*
   place queens
*/
  search.board[3]=queen;
  search.board[59]=-queen;
/*
   place kings
*/
  search.board[4]=king;
  search.board[60]=-king;
/*
   initialize castling status so all castling is legal.
*/
  new_pos->w_castle=3;
  new_pos->b_castle=3;
/*
   initialize enpassant status.
*/
  new_pos->enpassant_target=0;
/*
   now, set the bit-boards.
*/
  SetChessBitBoards(new_pos);
}

void SetChessBitBoards(SEARCH_POSITION *new_pos)
{
  int i;
  search.hash_key=0;
  search.pawn_hash_key=0;
/*
   place pawns
*/
  search.w_pawn=0;
  search.b_pawn=0;
  for (i=0;i<64;i++) {
    if(search.board[i]==pawn) {
      search.w_pawn=Or(search.w_pawn,set_mask[i]);
      search.hash_key=Xor(search.hash_key,w_pawn_random[i]);
      search.pawn_hash_key=Xor(search.pawn_hash_key,w_pawn_random[i]);
    }
    if(search.board[i]==-pawn) {
      search.b_pawn=Or(search.b_pawn,set_mask[i]);
      search.hash_key=Xor(search.hash_key,b_pawn_random[i]);
      search.pawn_hash_key=Xor(search.pawn_hash_key,b_pawn_random[i]);
    }
  }
/*
   place knights
*/
  search.w_knight=0;
  search.b_knight=0;
  for (i=0;i<64;i++) {
    if(search.board[i] == knight) {
      search.w_knight=Or(search.w_knight,set_mask[i]);
      search.hash_key=Xor(search.hash_key,w_knight_random[i]);
    }
    if(search.board[i] == -knight) {
      search.b_knight=Or(search.b_knight,set_mask[i]);
      search.hash_key=Xor(search.hash_key,b_knight_random[i]);
    }
  }
/*
   place bishops
*/
  search.w_bishop=0;
  search.b_bishop=0;
  for (i=0;i<64;i++) {
    if(search.board[i] == bishop) {
      search.w_bishop=Or(search.w_bishop,set_mask[i]);
      search.hash_key=Xor(search.hash_key,w_bishop_random[i]);
    }
    if(search.board[i] == -bishop) {
      search.b_bishop=Or(search.b_bishop,set_mask[i]);
      search.hash_key=Xor(search.hash_key,b_bishop_random[i]);
    }
  }
/*
   place rooks
*/
  search.w_rook=0;
  search.b_rook=0;
  for (i=0;i<64;i++) {
    if(search.board[i] == rook) {
      search.w_rook=Or(search.w_rook,set_mask[i]);
      search.hash_key=Xor(search.hash_key,w_rook_random[i]);
    }
    if(search.board[i] == -rook) {
      search.b_rook=Or(search.b_rook,set_mask[i]);
      search.hash_key=Xor(search.hash_key,b_rook_random[i]);
    }
  }
/*
   place queens
*/
  search.w_queen=0;
  search.b_queen=0;
  for (i=0;i<64;i++) {
    if(search.board[i] == queen) {
      search.w_queen=Or(search.w_queen,set_mask[i]);
      search.hash_key=Xor(search.hash_key,w_queen_random[i]);
    }
    if(search.board[i] == -queen) {
      search.b_queen=Or(search.b_queen,set_mask[i]);
      search.hash_key=Xor(search.hash_key,b_queen_random[i]);
    }
  }
/*
   place kings
*/
  for (i=0;i<64;i++) {
    if(search.board[i] == king) {
      search.white_king=i;
      search.hash_key=Xor(search.hash_key,w_king_random[i]);
    }
    if(search.board[i] == -king) {
      search.black_king=i;
      search.hash_key=Xor(search.hash_key,b_king_random[i]);
    }
  }
  if (new_pos->enpassant_target) 
    HashEP(new_pos->enpassant_target,search.hash_key);
  if (!(new_pos->w_castle&1)) HashCastleW(0,search.hash_key);
  if (!(new_pos->w_castle&2)) HashCastleW(1,search.hash_key);
  if (!(new_pos->b_castle&1)) HashCastleB(0,search.hash_key);
  if (!(new_pos->b_castle&2)) HashCastleB(1,search.hash_key);
/*
   initialize combination boards that show multiple pieces.
*/
  search.bishops_queens=Or(Or(Or(search.w_bishop,search.w_queen),search.b_bishop),search.b_queen);
  search.rooks_queens=Or(Or(Or(search.w_rook,search.w_queen),search.b_rook),search.b_queen);
  search.w_occupied=Or(Or(Or(Or(Or(search.w_pawn,search.w_knight),search.w_bishop),search.w_rook),
                                   search.w_queen),set_mask[search.white_king]);
  search.b_occupied=Or(Or(Or(Or(Or(search.b_pawn,search.b_knight),search.b_bishop),search.b_rook),
                                   search.b_queen),set_mask[search.black_king]);
/*
  now initialize rotated occupied bitboards.
*/
  search.occupied_rl90=0;
  search.occupied_rl45=0;
  search.occupied_rr45=0;
  for (i=0;i<64;i++) {
    if (search.board[i]) {
      search.occupied_rl90=Or(search.occupied_rl90,set_mask_rl90[i]);
      search.occupied_rl45=Or(search.occupied_rl45,set_mask_rl45[i]);
      search.occupied_rr45=Or(search.occupied_rr45,set_mask_rr45[i]);
    }
  }
/*
   initialize black/white piece counts.
*/
  search.white_pieces=0;
  search.white_pawns=0;
  search.black_pieces=0;
  search.black_pawns=0;
  search.material_evaluation=0;
  for (i=0;i<64;i++) {
    switch (search.board[i]) {
      case pawn:
        search.material_evaluation+=PAWN_VALUE;
        search.white_pawns+=pawn_v;
        break;
      case knight:
        search.material_evaluation+=KNIGHT_VALUE;
        search.white_pieces+=knight_v;
        break;
      case bishop:
        search.material_evaluation+=BISHOP_VALUE;
        search.white_pieces+=bishop_v;
        break;
      case rook:
        search.material_evaluation+=ROOK_VALUE;
        search.white_pieces+=rook_v;
        break;
      case queen:
        search.material_evaluation+=QUEEN_VALUE;
        search.white_pieces+=queen_v;
        break;
      case -pawn:
        search.material_evaluation-=PAWN_VALUE;
        search.black_pawns+=pawn_v;
        break;
      case -knight:
        search.material_evaluation-=KNIGHT_VALUE;
        search.black_pieces+=knight_v;
        break;
      case -bishop:
        search.material_evaluation-=BISHOP_VALUE;
        search.black_pieces+=bishop_v;
        break;
      case -rook:
        search.material_evaluation-=ROOK_VALUE;
        search.black_pieces+=rook_v;
        break;
      case -queen:
        search.material_evaluation-=QUEEN_VALUE;
        search.black_pieces+=queen_v;
        break;
      default:
        ;
    }
  }
  if (new_pos == &position[0]) {
    repetition_head_b=repetition_list_b;
    repetition_head_w=repetition_list_w;
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
int InitializeFindAttacks(int square, int pieces, int length)
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

void InitializeHashTables(void)
{
  int i;
  for (i=0;i<hash_table_size;i++) {
    (trans_ref_wa+i)->word1=0;
    (trans_ref_wa+i)->word2=0;
    (trans_ref_ba+i)->word1=0;
    (trans_ref_ba+i)->word2=0;
  }
  for (i=0;i<2*hash_table_size;i++) {
    (trans_ref_wb+i)->word1=0;
    (trans_ref_wb+i)->word2=0;
    (trans_ref_bb+i)->word1=0;
    (trans_ref_bb+i)->word2=0;
  }
  for (i=0;i<pawn_hash_table_size;i++) {
    (pawn_hash_table+i)->word1=0;
    (pawn_hash_table+i)->word2=0;
  }
}

void InitializeMasks(void)
{

  int i, j;
/*
  specific masks to avoid Mask() procedure call if possible.
*/
#  if !defined(CRAY1)
    mask_1=Mask(1);
    mask_2=Mask(2);
    mask_3=Mask(3);
    mask_4=Mask(4);
    mask_8=Mask(8);
    mask_16=Mask(16);
    mask_32=Mask(32);
    mask_72=Mask(72);
    mask_80=Mask(80);
    mask_85=Mask(85);
    mask_96=Mask(96);
    mask_107=Mask(107);
    mask_108=Mask(108);
    mask_112=Mask(112);
    mask_118=Mask(118);
    mask_120=Mask(120);
    mask_121=Mask(121);
    mask_127=Mask(127);
#  endif
  mask_clear_entry=Compl(Or(Shiftl(Mask(108),21),Shiftr(Mask(2),2)));
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
  clear_mask[BAD_SQUARE]=0;
  clear_mask_rl45[BAD_SQUARE]=0;
  clear_mask_rr45[BAD_SQUARE]=0;
  clear_mask_rl90[BAD_SQUARE]=0;
  set_mask[BAD_SQUARE]=0;
  set_mask_rl45[BAD_SQUARE]=0;
  set_mask_rr45[BAD_SQUARE]=0;
  set_mask_rl90[BAD_SQUARE]=0;
/*
  masks to select bits on a specific rank or file
*/
  rank_mask[0]=mask_8;
  for (i=1;i<8;i++) rank_mask[i]=Shiftr(rank_mask[i-1],8);
  file_mask[FILEA]=mask_1;
  for (i=1;i<8;i++) file_mask[FILEA]=Or(file_mask[FILEA],Shiftr(file_mask[FILEA],8));
  for (i=1;i<8;i++) file_mask[i]=Shiftr(file_mask[i-1],1);
/*
  masks to select bits on either white or black side of board
  note that white is skewed 1 rank because of the way rams are
  computed by advancing white pawns one rank and then Or'ing with
  black pawns.
*/
  mask_black_half=Or(Or(rank_mask[4],rank_mask[5]),
                     Or(rank_mask[6],rank_mask[7]));
  mask_white_half=Or(Or(rank_mask[1],rank_mask[2]),
                     Or(rank_mask[3],rank_mask[4]));
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
  right_half_mask=Or(Or(Or(file_mask[FILEE],file_mask[FILEF]),file_mask[FILEG]),file_mask[FILEH]);
  left_half_mask=Or(Or(Or(file_mask[FILEA],file_mask[FILEB]),file_mask[FILEC]),file_mask[FILED]);
  mask_kr_trapped_w[0]=set_mask[H2];
  mask_kr_trapped_w[1]=Or(set_mask[H1],set_mask[H2]);
  mask_kr_trapped_w[2]=Or(Or(set_mask[G1],set_mask[H1]),set_mask[H2]);
  mask_qr_trapped_w[0]=set_mask[A2];
  mask_qr_trapped_w[1]=Or(set_mask[A1],set_mask[A2]);
  mask_qr_trapped_w[2]=Or(Or(set_mask[A1],set_mask[B1]),set_mask[A2]);
  mask_kr_trapped_b[0]=set_mask[H7];
  mask_kr_trapped_b[1]=Or(set_mask[H8],set_mask[H7]);
  mask_kr_trapped_b[2]=Or(Or(set_mask[H8],set_mask[G8]),set_mask[H7]);
  mask_qr_trapped_b[0]=set_mask[A7];
  mask_qr_trapped_b[1]=Or(set_mask[A8],set_mask[A7]);
  mask_qr_trapped_b[2]=Or(Or(set_mask[A8],set_mask[B8]),set_mask[A7]);
}

void InitializePawnMasks(void)
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
    initialize connected pawn masks, which are nothing more than 1's on
    files adjacent to the pawn and ranks that are within 1 rank of the
    pawn.
*/
  for (i=8;i<56;i++) {
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
  mask_enpassant_test[A4]=set_mask[B4];
  mask_enpassant_test[H4]=set_mask[G4];
  mask_enpassant_test[A5]=set_mask[B5];
  mask_enpassant_test[H5]=set_mask[G5];

/*
  masks to detect pawns bearing down on the king
*/
  mask_kingside_attack_w1=Or(Or(mask_minus8dir[F5],mask_minus8dir[G5]),
                             mask_minus8dir[H5]);
  mask_kingside_attack_w2=Or(Or(mask_minus8dir[F4],mask_minus8dir[G4]),
                             mask_minus8dir[H4]);
  mask_queenside_attack_w1=Or(Or(mask_minus8dir[A5],mask_minus8dir[B5]),
                              mask_minus8dir[C5]);
  mask_queenside_attack_w2=Or(Or(mask_minus8dir[A4],mask_minus8dir[B4]),
                              mask_minus8dir[C4]);
  mask_kingside_attack_b1=Or(Or(mask_plus8dir[F4],mask_plus8dir[G4]),
                             mask_plus8dir[H4]);
  mask_kingside_attack_b2=Or(Or(mask_plus8dir[F5],mask_plus8dir[G5]),
                             mask_plus8dir[H5]);
  mask_queenside_attack_b1=Or(Or(mask_plus8dir[A4],mask_plus8dir[B4]),
                              mask_plus8dir[C4]);
  mask_queenside_attack_b2=Or(Or(mask_plus8dir[A5],mask_plus8dir[B5]),
                              mask_plus8dir[C5]);
/* 
  pawns at d5/e5/f5 cramp black, and pawns at d4/e4/f4 cramp
  white, especially if there are no pawns that can attack
  these pawns.
*/
  pawns_cramp_black=Or(Or(set_mask[D5],set_mask[E5]),
                       set_mask[F5]);
  pawns_cramp_white=Or(Or(set_mask[D4],set_mask[E4]),
                       set_mask[F4]);
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
  this mask is used to detect that one side has pawns, but all
  are rook pawns.                                                
*/
  not_rook_pawns=Or(Or(Or(file_mask[FILEB],file_mask[FILEC]),
                       Or(file_mask[FILED],file_mask[FILEE])),
                    Or(file_mask[FILEF],file_mask[FILEG]));
/* 
  these two masks have 1's on everywhere but the left or right
  files, used to prevent pawns from capturing off the edge of
  the board and wrapping around.rom capturing off the edge of
*/
  mask_left_edge=Compl(file_mask[FILEA]);
  mask_right_edge=Compl(file_mask[FILEH]);
  mask_advance_2_w=rank_mask[RANK3];
  mask_advance_2_b=rank_mask[RANK6];
/* 
  this mask has 1's on the 4 corner squares, and is used to detect
  the king sitting right in the corner where it's easier to get
  mated.
*/
  mask_corner_squares=Or(Or(set_mask[A1],set_mask[H1]),
                         Or(set_mask[A8],set_mask[H8]));
/* 
  these masks have 1's on the squares where it is useful to have a bishop
  when the b or g pawn is missing or pushed one square.
*/
  good_bishop_kw=Or(Or(set_mask[F1],set_mask[H1]),set_mask[G2]);
  good_bishop_qw=Or(Or(set_mask[A1],set_mask[C1]),set_mask[B2]);
  good_bishop_kb=Or(Or(set_mask[G7],set_mask[F8]),set_mask[H8]);
  good_bishop_qb=Or(Or(set_mask[B7],set_mask[A8]),set_mask[C8]);
/*
    these masks are used to detect when a passed pawn reaches the 6th or
    7th rank with a connected neighboring pawn also on the 6th or 7th rank
    so that the threat to promote is really significant.
*/
  for (i=0;i<64;i++) {
    mask_promotion_threat_w[i]=0;
    mask_promotion_threat_b[i]=0;
  }
  for (i=8;i<24;i++) {
    if (!(i&7)) {
      mask_promotion_threat_b[i]=Or(set_mask[B2],set_mask[B3]);
    }
    else if ((i&7) == 7) {
      mask_promotion_threat_b[i]=Or(set_mask[G2],set_mask[G3]);
    }
    else {
      mask_promotion_threat_b[i]=Or(Or(set_mask[(i&7)+7],set_mask[(i&7)+9]),
                                    Or(set_mask[(i&7)+15],set_mask[(i&7)+17]));
    }
  }
  for (i=40;i<56;i++) {
    if (!(i&7)) {
      mask_promotion_threat_w[i]=Or(set_mask[B6],set_mask[B7]);
    }
    else if ((i&7) == 7) {
      mask_promotion_threat_w[i]=Or(set_mask[G6],set_mask[G7]);
    }
    else {
      mask_promotion_threat_w[i]=Or(Or(set_mask[(i&7)+39],set_mask[(i&7)+47]),
                                    Or(set_mask[(i&7)+41],set_mask[(i&7)+49]));
    }
  }
/*
  these two masks are for generating passed pawn pushes and are used to
  select the 6th-7th rank squares as targets.
*/
  promote_mask_w=Compl(Or(rank_mask[5],rank_mask[6]));
  promote_mask_b=Compl(Or(rank_mask[1],rank_mask[2]));
/*
  these masks are used to test for the presence of a pawn at g2/g3, etc.
  and are used in evaluating a bishop potentially trapped at h2, etc.
*/
  mask_g2g3=Or(set_mask[G2],set_mask[G3]);
  mask_b2b3=Or(set_mask[B2],set_mask[B3]);
  mask_g6g7=Or(set_mask[G6],set_mask[G7]);
  mask_b6b7=Or(set_mask[B6],set_mask[B7]);
/*
  these masks are used to detect that opponent pawns are getting very
  close to the king.
*/
  mask_wq_3rd=Or(Or(set_mask[A3],set_mask[B3]),set_mask[C3]);
  mask_wk_3rd=Or(Or(set_mask[F3],set_mask[G3]),set_mask[H3]);
  mask_wq_4th=Or(Or(set_mask[A4],set_mask[B4]),set_mask[C4]);
  mask_wk_4th=Or(Or(set_mask[F4],set_mask[G4]),set_mask[H4]);

  mask_bq_3rd=Or(Or(set_mask[A6],set_mask[B6]),set_mask[C6]);
  mask_bk_3rd=Or(Or(set_mask[F6],set_mask[G6]),set_mask[H6]);
  mask_bq_4th=Or(Or(set_mask[A5],set_mask[B5]),set_mask[C5]);
  mask_bk_4th=Or(Or(set_mask[F5],set_mask[G5]),set_mask[H5]);

}

void InitializePieceMasks(void)
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
        if (KingPawnSquare(j+8,i,(j&7)+56,1)) 
          white_pawn_race_wtm[j]=Or(white_pawn_race_wtm[j],set_mask[i]);
      }
      else {
        if (KingPawnSquare(j,i,(j&7)+56,1)) 
          white_pawn_race_wtm[j]=Or(white_pawn_race_wtm[j],set_mask[i]);
      }
/* white pawn, ChangeSide(wtm) */
      if (j < 16) {
        if (KingPawnSquare(j+8,i,(j&7)+56,0)) 
          white_pawn_race_btm[j]=Or(white_pawn_race_btm[j],set_mask[i]);
      }
      else {
        if (KingPawnSquare(j,i,(j&7)+56,0)) 
          white_pawn_race_btm[j]=Or(white_pawn_race_btm[j],set_mask[i]);
      }
/* black pawn, wtm */
      if (j > 47) {
        if (KingPawnSquare(j-8,i,j&7,0)) 
          black_pawn_race_wtm[j]=Or(black_pawn_race_wtm[j],set_mask[i]);
      }
      else {
        if (KingPawnSquare(j,i,j&7,0)) 
          black_pawn_race_wtm[j]=Or(black_pawn_race_wtm[j],set_mask[i]);
      }
/* black pawn, ChangeSide(wtm) */
      if (j > 47) {
        if (KingPawnSquare(j-8,i,j&7,1)) 
          black_pawn_race_btm[j]=Or(black_pawn_race_btm[j],set_mask[i]);
      }
      else {
        if (KingPawnSquare(j,i,j&7,1)) 
          black_pawn_race_btm[j]=Or(black_pawn_race_btm[j],set_mask[i]);
      }
    }
  }
}

/*
********************************************************************************
*                                                                              *
*   InitializeRandomHash() is called to initialize the tables of random      *
*   numbers used to produce the incrementally-updated hash keys.  note that    *
*   this uses a local random number generator rather than the C library one    *
*   since there is no uniformity in the number of bits returned by the         *
*   standard library routines, it varies from 16 bits to 64.                   *
*                                                                              *
********************************************************************************
*/
void InitializeRandomHash(void)
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
  for (i=0;i<2;i++) {
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

void InitializeZeroMasks(void)
{
  int i,j,maskl,maskr;
#if !defined(CRAY1)
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
  connected_passed[0]=0;
  for (i=0;i<256;i++){
    connected_passed[i]=0;
    for (j=0;j<8;j++){
      if (i & (1<<(7-j))){
        first_ones_8bit[i]=j;
        break;
      }
    }
    for (j=7;j>=0;j--){
      if (i & (1<<(7-j))){
        last_ones_8bit[i]=j;
        break;
      }
    }
    for (j=7;j>0;j--){
      if ((i & (3<<(7-j))) == (3<<(7-j))){
        connected_passed[i]=j;
        break;
      }
    }
  }
}
