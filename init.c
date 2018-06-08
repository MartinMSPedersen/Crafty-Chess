#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "chess.h"
#include "data.h"
#if defined(UNIX) || defined(AMIGA)
#  include <unistd.h>
#endif
#include "epdglue.h"
#if defined(NT_i386) || defined(NT_AXP)
#  include <fcntl.h>  /* needed for definition of "_O_BINARY" */
#endif


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

void Initialize(int continuing) {
/*
 ----------------------------------------------------------
|                                                          |
|   perform routine initialization.                        |
|                                                          |
 ----------------------------------------------------------
*/
  int i, j, major, minor;
  TREE *tree;

#if defined(SMP)
  for (i=1;i<MAX_BLOCKS+1;i++) {
    local[i]=(TREE*) malloc(sizeof(TREE));
    local[i]->used=0;
  }
  local[0]->parent=(TREE*)-1;
#endif
  
  tree=local[0];
  i=0;
  InitializeZeroMasks();
#if defined(SMP)
  InitializeSMP();
#endif
  InitializeMasks();
  InitializeRandomHash();
  InitializeAttackBoards();
  InitializePawnMasks();
  InitializePieceMasks();
  InitializeChessBoard(&tree->position[0]);
#if defined(NT_i386) || defined(NT_AXP)
  _fmode = _O_BINARY;  /* set global file mode to binary to avoid text translation */
#endif

  EGInit();

  tree->last[0]=tree->move_list;

#if defined(MACOS)
  sprintf(log_filename,":%s:book.bin",book_path);
#else
  sprintf(log_filename,"%s/book.bin",book_path);
#endif
  book_file=fopen(log_filename,"rb+");
  if (!book_file) {
    book_file=fopen(log_filename,"rb");
    if (!book_file) {
#if defined(MACOS)
      printf("unable to open book file [:%s:book.bin].\n",book_path);
#else
      printf("unable to open book file [%s/book.bin].\n",book_path);
#endif
      printf("book is disabled\n");
    }
    else {
#if defined(MACOS)
      printf("unable to open book file [:%s:book.bin] for \"write\".\n",book_path);
#else
      printf("unable to open book file [%s/book.bin] for \"write\".\n",book_path);
#endif
      printf("learning is disabled\n");
    }
  }
#if defined(MACOS)
  sprintf(log_filename,":%s:books.bin",book_path);
#else
  sprintf(log_filename,"%s/books.bin",book_path);
#endif
  books_file=fopen(log_filename,"rb");
#if defined(MACOS)
  if (!books_file) printf("unable to open book file [:%s:books.bin].\n",book_path);
#else
  if (!books_file) printf("unable to open book file [%s/books.bin].\n",book_path);
#endif
  if (book_file) {
    fseek(book_file,-sizeof(int),SEEK_END);
    fread(&major,sizeof(int),1,book_file);
    minor=major&65535;
    major=major>>16;
    if (major<15 || (major==15 && minor<15)) {
      Print(4095,"\nERROR!  book.bin not made by version 15.15 or later\n");
      book_file=0;
      books_file=0;
    }
  }
#if defined(MACOS)
  sprintf(log_filename,":%s:book.lrn",book_path);
#else
  sprintf(log_filename,"%s/book.lrn",book_path);
#endif
  book_lrn_file=fopen(log_filename,"a");
  if (!book_lrn_file) {
#if defined(MACOS)
    printf("unable to open book learning file [:%s:book.lrn].\n",book_path);
#else
    printf("unable to open book learning file [%s/book.lrn].\n",book_path);
#endif
    printf("learning disabled.\n");
    learning&=~(book_learning+result_learning);
  }
  if (learning&position_learning) {
#if defined(MACOS)
    sprintf(log_filename,":%s:position.bin",book_path);
#else
    sprintf(log_filename,"%s/position.bin",book_path);
#endif
    position_file=fopen(log_filename,"rb+");
    if (position_file) {
      fseek(position_file,0,SEEK_END);
      if (ftell(position_file) == 0) {
        fclose(position_file);
        position_file=0;
      }
    }
    if (!position_file) {
      position_file=fopen(log_filename,"wb+");
      if (position_file) {
        fseek(position_file,0,SEEK_SET);
        fwrite(&i,sizeof(int),1,position_file);
        i--;
        fwrite(&i,sizeof(int),1,position_file);
      }
      else {
#if defined(MACOS)
        printf("unable to open position learning file [:%s:position.bin].\n",book_path);
#else
        printf("unable to open position learning file [%s/position.bin].\n",book_path);
#endif
        printf("learning disabled.\n");
        learning&=~position_learning;
      }
    }
#if defined(MACOS)
    sprintf(log_filename,":%s:position.lrn",book_path);
#else
    sprintf(log_filename,"%s/position.lrn",book_path);
#endif
    position_lrn_file=fopen(log_filename,"r");
    if (!position_lrn_file) {
      position_lrn_file=fopen(log_filename,"a");
      fprintf(position_lrn_file,"position\n");
    }
    else {
      fclose(position_lrn_file);
      position_lrn_file=fopen(log_filename,"a");
    }
  }

  for (log_id=1;log_id <300;log_id++) {
#if defined(MACOS)
    sprintf(log_filename,":%s:log.%03d",log_path,log_id);
    sprintf(history_filename,":%s:game.%03d",log_path,log_id);
#else
    sprintf(log_filename,"%s/log.%03d",log_path,log_id);
    sprintf(history_filename,"%s/game.%03d",log_path,log_id);
#endif
    log_file=fopen(log_filename,"r");
    if (!log_file) break;
    fclose(log_file);
  }
  if (continuing) {
    log_id--;
    sprintf(log_filename,"%s/log.%03d",log_path,log_id);
    sprintf(history_filename,"%s/game.%03d",log_path,log_id);
    log_file=fopen(log_filename,"r+");
    history_file=fopen(history_filename,"r+");
    if (!log_file || !history_file) {
      printf("\nsorry.  nothing to continue.\n\n");
      sprintf(log_filename,"%s/log.%03d",log_path,1);
      sprintf(history_filename,"%s/game.%03d",log_path,1);
      log_file=fopen(log_filename,"w");
      history_file=fopen(history_filename,"w+");
    }
    else {
      sprintf(buffer,"read %s/game.%03d", log_path, log_id);
      (void) Option(tree);
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
  pawn_hash_table=malloc(sizeof(PAWN_HASH_ENTRY)*pawn_hash_table_size);
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
    log_hash=0;
    log_pawn_hash=0;
    trans_ref_wa=0;
    trans_ref_wb=0;
    trans_ref_ba=0;
    trans_ref_bb=0;
    pawn_hash_table=0;
  }
  hash_maska=(1<<log_hash)-1;
  hash_maskb=(1<<(log_hash+1))-1;
  pawn_hash_mask=((unsigned int) 037777777777)>>(32-log_pawn_hash);

  for (i=0;i<8;i++)
    for (j=0;j<8;j++) {
      pawn_value_b[i*8+j]=  pawn_value_w[(7-i)*8+j];
      knight_value_b[i*8+j]=knight_value_w[(7-i)*8+j];
      bishop_value_b[i*8+j]=bishop_value_w[(7-i)*8+j];
      rook_value_b[i*8+j]=  rook_value_w[(7-i)*8+j];
      queen_value_b[i*8+j]= queen_value_w[(7-i)*8+j];
      king_value_b[i*8+j]=  king_value_w[(7-i)*8+j];
    }
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
          plus7dir[i]=Or(plus7dir[i],Shiftr(mask_1,sq));
        else if(bishopsq[j]==9)
          plus9dir[i]=Or(plus9dir[i],Shiftr(mask_1,sq));
        else if(bishopsq[j]==-7)
          minus7dir[i]=Or(minus7dir[i],Shiftr(mask_1,sq));
        else
          minus9dir[i]=Or(minus9dir[i],Shiftr(mask_1,sq));
        lastsq=sq;
        sq=sq+bishopsq[j];
      }
    }
  }
  plus1dir[64]=0;
  plus7dir[64]=0;
  plus8dir[64]=0;
  plus9dir[64]=0;
  minus1dir[64]=0;
  minus7dir[64]=0;
  minus8dir[64]=0;
  minus9dir[64]=0;
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
          plus1dir[i]=Or(plus1dir[i],Shiftr(mask_1,sq));
        else if(rooksq[j]==8)
          plus8dir[i]=Or(plus8dir[i],Shiftr(mask_1,sq));
        else if(rooksq[j]==-1)
          minus1dir[i]=Or(minus1dir[i],Shiftr(mask_1,sq));
        else
          minus8dir[i]=Or(minus8dir[i],Shiftr(mask_1,sq));
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
        king_attacks[i]=Or(king_attacks[i],SetMask(j));
      if (Distance(i,j) <= 1)
        king_attacks_1[i]=Or(king_attacks_1[i],SetMask(j));
      if (Distance(i,j) <= 2)
        king_attacks_2[i]=Or(king_attacks_2[i],SetMask(j));
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
      obstructed[i][j]=(BITBOARD) -1;
    sqs=plus1dir[i];
    while (sqs) {
      j=FirstOne(sqs);
      directions[i][j]=1;
      obstructed[i][j]=Xor(plus1dir[i],plus1dir[j-1]);
      Clear(j,sqs);
    }
    sqs=plus7dir[i];
    while (sqs) {
      j=FirstOne(sqs);
      directions[i][j]=7;
      obstructed[i][j]=Xor(plus7dir[i],plus7dir[j-7]);
      Clear(j,sqs);
    }
    sqs=plus8dir[i];
    while (sqs) {
      j=FirstOne(sqs);
      directions[i][j]=8;
      obstructed[i][j]=Xor(plus8dir[i],plus8dir[j-8]);
      Clear(j,sqs);
    }
    sqs=plus9dir[i];
    while (sqs) {
      j=FirstOne(sqs);
      directions[i][j]=9;
      obstructed[i][j]=Xor(plus9dir[i],plus9dir[j-9]);
      Clear(j,sqs);
    }
    sqs=minus1dir[i];
    while (sqs) {
      j=FirstOne(sqs);
      directions[i][j]=-1;
      obstructed[i][j]=Xor(minus1dir[i],minus1dir[j+1]);
      Clear(j,sqs);
    }
    sqs=minus7dir[i];
    while (sqs) {
      j=FirstOne(sqs);
      directions[i][j]=-7;
      obstructed[i][j]=Xor(minus7dir[i],minus7dir[j+7]);
      Clear(j,sqs);
    }
    sqs=minus8dir[i];
    while (sqs) {
      j=FirstOne(sqs);
      directions[i][j]=-8;
      obstructed[i][j]=Xor(minus8dir[i],minus8dir[j+8]);
      Clear(j,sqs);
    }
    sqs=minus9dir[i];
    while (sqs) {
      j=FirstOne(sqs);
      directions[i][j]=-9;
      obstructed[i][j]=Xor(minus9dir[i],minus9dir[j+9]);
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
            Or(rook_attacks_r0[square][pcs],SetMask((square&56)+sq));
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
               SetMask(init_r90[((square&7)<<3)+sq]));
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
               SetMask(init_ul45[sq+bias_rl45[rsq]]));
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
               SetMask(init_ur45[sq+bias_rl45[rsq]]));
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
  TREE *tree=local[0];

  if (strlen(initial_position)) {
    static char a1[80], a2[16], a3[16], a4[16], a5[16];
    static char *args[16]={a1,a2,a3,a4,a5,a5,a5,a5,a5,a5,a5,a5,a5,a5,a5,a5};
    int nargs;

    nargs=ReadParse(initial_position,args," ;");
    SetBoard(nargs,args,1);
  }
  else {
    for(i=0;i<64;i++) tree->pos.board[i]=empty;
    new_pos->rule_50_moves=0;
    opening=1;
    middle_game=0;
    end_game=0;
    largest_positional_score=100;
/*
   place pawns
*/
    for (i=0;i<8;i++) {
      tree->pos.board[i+8]=pawn;
      tree->pos.board[i+48]=-pawn;
    }
/*
   place knights
*/
    tree->pos.board[B1]=knight;
    tree->pos.board[G1]=knight;
    tree->pos.board[B8]=-knight;
    tree->pos.board[G8]=-knight;
/*
   place bishops
*/
    tree->pos.board[C1]=bishop;
    tree->pos.board[F1]=bishop;
    tree->pos.board[C8]=-bishop;
    tree->pos.board[F8]=-bishop;
/*
   place rooks
*/
    tree->pos.board[A1]=rook;
    tree->pos.board[H1]=rook;
    tree->pos.board[A8]=-rook;
    tree->pos.board[H8]=-rook;
/*
   place queens
*/
    tree->pos.board[D1]=queen;
    tree->pos.board[D8]=-queen;
/*
   place kings
*/
    tree->pos.board[E1]=king;
    tree->pos.board[E8]=-king;
/*
   initialize castling status so all castling is legal.
*/
    new_pos->w_castle=3;
    new_pos->b_castle=3;
/*
   initialize 50 move counter.
*/
    new_pos->rule_50_moves=0;
/*
   initialize enpassant status.
*/
    new_pos->enpassant_target=0;
/*
   now, set the bit-boards.
*/
    SetChessBitBoards(new_pos);
  }
}

void SetChessBitBoards(SEARCH_POSITION *new_pos)
{
  int i;
  TREE *tree=local[0];
  tree->pos.hash_key=0;
  tree->pos.pawn_hash_key=0;
/*
   place pawns
*/
  tree->pos.w_pawn=0;
  tree->pos.b_pawn=0;
  for (i=0;i<64;i++) {
    if(tree->pos.board[i]==pawn) {
      tree->pos.w_pawn=Or(tree->pos.w_pawn,SetMask(i));
      tree->pos.hash_key=Xor(tree->pos.hash_key,w_pawn_random[i]);
      tree->pos.pawn_hash_key=tree->pos.pawn_hash_key^w_pawn_random32[i];
    }
    if(tree->pos.board[i]==-pawn) {
      tree->pos.b_pawn=Or(tree->pos.b_pawn,SetMask(i));
      tree->pos.hash_key=Xor(tree->pos.hash_key,b_pawn_random[i]);
      tree->pos.pawn_hash_key=tree->pos.pawn_hash_key^b_pawn_random32[i];
    }
  }
/*
   place knights
*/
  tree->pos.w_knight=0;
  tree->pos.b_knight=0;
  for (i=0;i<64;i++) {
    if(tree->pos.board[i] == knight) {
      tree->pos.w_knight=Or(tree->pos.w_knight,SetMask(i));
      tree->pos.hash_key=Xor(tree->pos.hash_key,w_knight_random[i]);
    }
    if(tree->pos.board[i] == -knight) {
      tree->pos.b_knight=Or(tree->pos.b_knight,SetMask(i));
      tree->pos.hash_key=Xor(tree->pos.hash_key,b_knight_random[i]);
    }
  }
/*
   place bishops
*/
  tree->pos.w_bishop=0;
  tree->pos.b_bishop=0;
  for (i=0;i<64;i++) {
    if(tree->pos.board[i] == bishop) {
      tree->pos.w_bishop=Or(tree->pos.w_bishop,SetMask(i));
      tree->pos.hash_key=Xor(tree->pos.hash_key,w_bishop_random[i]);
    }
    if(tree->pos.board[i] == -bishop) {
      tree->pos.b_bishop=Or(tree->pos.b_bishop,SetMask(i));
      tree->pos.hash_key=Xor(tree->pos.hash_key,b_bishop_random[i]);
    }
  }
/*
   place rooks
*/
  tree->pos.w_rook=0;
  tree->pos.b_rook=0;
  for (i=0;i<64;i++) {
    if(tree->pos.board[i] == rook) {
      tree->pos.w_rook=Or(tree->pos.w_rook,SetMask(i));
      tree->pos.hash_key=Xor(tree->pos.hash_key,w_rook_random[i]);
    }
    if(tree->pos.board[i] == -rook) {
      tree->pos.b_rook=Or(tree->pos.b_rook,SetMask(i));
      tree->pos.hash_key=Xor(tree->pos.hash_key,b_rook_random[i]);
    }
  }
/*
   place queens
*/
  tree->pos.w_queen=0;
  tree->pos.b_queen=0;
  for (i=0;i<64;i++) {
    if(tree->pos.board[i] == queen) {
      tree->pos.w_queen=Or(tree->pos.w_queen,SetMask(i));
      tree->pos.hash_key=Xor(tree->pos.hash_key,w_queen_random[i]);
    }
    if(tree->pos.board[i] == -queen) {
      tree->pos.b_queen=Or(tree->pos.b_queen,SetMask(i));
      tree->pos.hash_key=Xor(tree->pos.hash_key,b_queen_random[i]);
    }
  }
/*
   place kings
*/
  for (i=0;i<64;i++) {
    if(tree->pos.board[i] == king) {
      tree->pos.white_king=i;
      tree->pos.hash_key=Xor(tree->pos.hash_key,w_king_random[i]);
    }
    if(tree->pos.board[i] == -king) {
      tree->pos.black_king=i;
      tree->pos.hash_key=Xor(tree->pos.hash_key,b_king_random[i]);
    }
  }
  if (new_pos->enpassant_target) 
    HashEP(new_pos->enpassant_target,tree->pos.hash_key);
  if (!(new_pos->w_castle&1)) HashCastleW(0,tree->pos.hash_key);
  if (!(new_pos->w_castle&2)) HashCastleW(1,tree->pos.hash_key);
  if (!(new_pos->b_castle&1)) HashCastleB(0,tree->pos.hash_key);
  if (!(new_pos->b_castle&2)) HashCastleB(1,tree->pos.hash_key);
/*
   initialize combination boards that show multiple pieces.
*/
  tree->pos.bishops_queens=Or(Or(Or(tree->pos.w_bishop,tree->pos.w_queen),tree->pos.b_bishop),tree->pos.b_queen);
  tree->pos.rooks_queens=Or(Or(Or(tree->pos.w_rook,tree->pos.w_queen),tree->pos.b_rook),tree->pos.b_queen);
  tree->pos.w_occupied=Or(Or(Or(Or(Or(tree->pos.w_pawn,tree->pos.w_knight),tree->pos.w_bishop),tree->pos.w_rook),
                                   tree->pos.w_queen),SetMask(tree->pos.white_king));
  tree->pos.b_occupied=Or(Or(Or(Or(Or(tree->pos.b_pawn,tree->pos.b_knight),tree->pos.b_bishop),tree->pos.b_rook),
                                   tree->pos.b_queen),SetMask(tree->pos.black_king));
/*
  now initialize rotated occupied bitboards.
*/
  tree->pos.occupied_rl90=0;
  tree->pos.occupied_rl45=0;
  tree->pos.occupied_rr45=0;
  for (i=0;i<64;i++) {
    if (tree->pos.board[i]) {
      tree->pos.occupied_rl90=Or(tree->pos.occupied_rl90,SetMaskRL90(i));
      tree->pos.occupied_rl45=Or(tree->pos.occupied_rl45,SetMaskRL45(i));
      tree->pos.occupied_rr45=Or(tree->pos.occupied_rr45,SetMaskRR45(i));
    }
  }
/*
   initialize black/white piece counts.
*/
  tree->pos.white_pieces=0;
  tree->pos.white_majors=0;
  tree->pos.white_minors=0;
  tree->pos.white_pawns=0;
  tree->pos.black_pieces=0;
  tree->pos.black_majors=0;
  tree->pos.black_minors=0;
  tree->pos.black_pawns=0;
  tree->pos.material_evaluation=0;
  for (i=0;i<64;i++) {
    switch (tree->pos.board[i]) {
      case pawn:
        tree->pos.material_evaluation+=PAWN_VALUE;
        tree->pos.white_pawns+=pawn_v;
        break;
      case knight:
        tree->pos.material_evaluation+=KNIGHT_VALUE;
        tree->pos.white_pieces+=knight_v;
        tree->pos.white_minors++;
        break;
      case bishop:
        tree->pos.material_evaluation+=BISHOP_VALUE;
        tree->pos.white_pieces+=bishop_v;
        tree->pos.white_minors++;
        break;
      case rook:
        tree->pos.material_evaluation+=ROOK_VALUE;
        tree->pos.white_pieces+=rook_v;
        tree->pos.white_majors++;
        break;
      case queen:
        tree->pos.material_evaluation+=QUEEN_VALUE;
        tree->pos.white_pieces+=queen_v;
        tree->pos.white_majors+=2;
        break;
      case -pawn:
        tree->pos.material_evaluation-=PAWN_VALUE;
        tree->pos.black_pawns+=pawn_v;
        break;
      case -knight:
        tree->pos.material_evaluation-=KNIGHT_VALUE;
        tree->pos.black_pieces+=knight_v;
        tree->pos.black_minors++;
        break;
      case -bishop:
        tree->pos.material_evaluation-=BISHOP_VALUE;
        tree->pos.black_pieces+=bishop_v;
        tree->pos.black_minors++;
        break;
      case -rook:
        tree->pos.material_evaluation-=ROOK_VALUE;
        tree->pos.black_pieces+=rook_v;
        tree->pos.black_majors++;
        break;
      case -queen:
        tree->pos.material_evaluation-=QUEEN_VALUE;
        tree->pos.black_pieces+=queen_v;
        tree->pos.black_majors+=2;
        break;
      default:
        ;
    }
  }
  TotalPieces=PopCnt(Occupied);
  if (new_pos == &tree->position[0]) {
    tree->rephead_b=tree->replist_b;
    tree->rephead_w=tree->replist_w;
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
  transposition_id=0;
  for (i=0;i<hash_table_size;i++) {
    (trans_ref_wa+i)->word1=Shiftl((BITBOARD) 7,61);
    (trans_ref_wa+i)->word2=0;
    (trans_ref_ba+i)->word1=Shiftl((BITBOARD) 7,61);
    (trans_ref_ba+i)->word2=0;
  }
  for (i=0;i<2*hash_table_size;i++) {
    (trans_ref_wb+i)->word1=Shiftl((BITBOARD) 7,61);
    (trans_ref_wb+i)->word2=0;
    (trans_ref_bb+i)->word1=Shiftl((BITBOARD) 7,61);
    (trans_ref_bb+i)->word2=0;
  }
  for (i=0;i<pawn_hash_table_size;i++) {
    (pawn_hash_table+i)->key=0;
    (pawn_hash_table+i)->p_score=0;
    (pawn_hash_table+i)->black_protected=0;
    (pawn_hash_table+i)->white_protected=0;
    (pawn_hash_table+i)->black_pof=0;
    (pawn_hash_table+i)->white_pof=0;
    (pawn_hash_table+i)->weak_b=0;
    (pawn_hash_table+i)->weak_w=0;
    (pawn_hash_table+i)->black_defects_k=0;
    (pawn_hash_table+i)->black_defects_q=0;
    (pawn_hash_table+i)->white_defects_k=0;
    (pawn_hash_table+i)->white_defects_q=0;
    (pawn_hash_table+i)->passed_w=0;
    (pawn_hash_table+i)->passed_w=0;
    (pawn_hash_table+i)->outside=0;
  }
}

void InitializeMasks(void) {
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
  mask_clear_entry=Compl(Or(Shiftl(Mask(109),21),Shiftr(Mask(3),3)));
/*
  masks to set/clear a bit on a specific square
*/
  for (i=0;i<64;i++) {
    ClearMask(i)=Compl(Shiftr(mask_1,i));
    ClearMaskRL45(i)=Compl(Shiftr(mask_1,init_l45[i]));
    ClearMaskRR45(i)=Compl(Shiftr(mask_1,init_r45[i]));
    ClearMaskRL90(i)=Compl(Shiftr(mask_1,init_l90[i]));
    SetMask(i)=Shiftr(mask_1,i);
    SetMaskRL45(i)=Shiftr(mask_1,init_l45[i]);
    SetMaskRR45(i)=Shiftr(mask_1,init_r45[i]);
    SetMaskRL90(i)=Shiftr(mask_1,init_l90[i]);
  }
  ClearMask(BAD_SQUARE)=0;
  ClearMaskRL45(BAD_SQUARE)=0;
  ClearMaskRR45(BAD_SQUARE)=0;
  ClearMaskRL90(BAD_SQUARE)=0;
  SetMask(BAD_SQUARE)=0;
  SetMaskRL45(BAD_SQUARE)=0;
  SetMaskRR45(BAD_SQUARE)=0;
  SetMaskRL90(BAD_SQUARE)=0;
/*
  masks to select bits on a specific rank or file
*/
  rank_mask[0]=mask_8;
  for (i=1;i<8;i++) rank_mask[i]=Shiftr(rank_mask[i-1],8);
  file_mask[FILEA]=mask_1;
  for (i=1;i<8;i++) file_mask[FILEA]=Or(file_mask[FILEA],Shiftr(file_mask[FILEA],8));
  for (i=1;i<8;i++) file_mask[i]=Shiftr(file_mask[i-1],1);
/*
  masks to determine if a pawn is protected by another pawn or not.
  also masks to detect "duos" (pawns side-by-side only).
*/
  for (i=8;i<56;i++) {
    if (File(i)>0 && File(i)<7) {
      mask_pawn_duo[i]=Or(SetMask(i-1),SetMask(i+1));
      mask_pawn_protected_w[i]=Or(SetMask(i-1),SetMask(i+1));
      if (i > 15) mask_pawn_protected_w[i]|=Or(SetMask(i-7),SetMask(i-9));
      mask_pawn_protected_b[i]=Or(SetMask(i-1),SetMask(i+1));
      if (i < 48) mask_pawn_protected_b[i]|=Or(SetMask(i+7),SetMask(i+9));
    }
    else if (File(i) == 0) {
      mask_pawn_duo[i]=SetMask(i+1);
      mask_pawn_protected_w[i]=SetMask(i+1);
      if (i > 15) mask_pawn_protected_w[i]|=SetMask(i-7);
      mask_pawn_protected_b[i]=SetMask(i+1);
      if (i < 48) mask_pawn_protected_b[i]|=SetMask(i+9);
    }
    else if (File(i) == 7) {
      mask_pawn_duo[i]=SetMask(i-1);
      mask_pawn_protected_w[i]=SetMask(i-1);
      if (i > 15) mask_pawn_protected_w[i]|=SetMask(i-9);
      mask_pawn_protected_b[i]=SetMask(i-1);
      if (i < 48) mask_pawn_protected_b[i]|=SetMask(i+7);
    }
  }
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
  mask_efgh=Or(Or(Or(file_mask[FILEE],file_mask[FILEF]),file_mask[FILEG]),file_mask[FILEH]);
  mask_fgh=Or(Or(file_mask[FILEF],file_mask[FILEG]),file_mask[FILEH]);
  mask_abcd=Or(Or(Or(file_mask[FILEA],file_mask[FILEB]),file_mask[FILEC]),file_mask[FILED]);
  mask_abc=Or(Or(file_mask[FILEA],file_mask[FILEB]),file_mask[FILEC]);
  mask_kr_trapped_w[0]=SetMask(H2);
  mask_kr_trapped_w[1]=Or(SetMask(H1),SetMask(H2));
  mask_kr_trapped_w[2]=Or(Or(SetMask(G1),SetMask(H1)),SetMask(H2));
  mask_qr_trapped_w[0]=SetMask(A2);
  mask_qr_trapped_w[1]=Or(SetMask(A1),SetMask(A2));
  mask_qr_trapped_w[2]=Or(Or(SetMask(A1),SetMask(B1)),SetMask(A2));
  mask_kr_trapped_b[0]=SetMask(H7);
  mask_kr_trapped_b[1]=Or(SetMask(H8),SetMask(H7));
  mask_kr_trapped_b[2]=Or(Or(SetMask(H8),SetMask(G8)),SetMask(H7));
  mask_qr_trapped_b[0]=SetMask(A7);
  mask_qr_trapped_b[1]=Or(SetMask(A8),SetMask(A7));
  mask_qr_trapped_b[2]=Or(Or(SetMask(A8),SetMask(B8)),SetMask(A7));

  mask_abs7_w=Xor(rank_mask[RANK7],Or(SetMask(H7),SetMask(A7)));
  mask_abs7_b=Xor(rank_mask[RANK2],Or(SetMask(H2),SetMask(A2)));

  mask_not_rank8=~rank_mask[RANK8];
  mask_not_rank1=~rank_mask[RANK1];

  mask_A7H7=Or(SetMask(A7),SetMask(H7));
  mask_A2H2=Or(SetMask(A2),SetMask(H2));
  center=Or(Or(SetMask(D4),SetMask(E4)),
            Or(SetMask(D5),SetMask(E5)));
  threat_flag=Shiftl((BITBOARD) 1, 58);
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
    if (!(i&7)) mask_pawn_isolated[i]=file_mask[(i&7)+1];
    else if ((i&7) == 7) mask_pawn_isolated[i]=file_mask[(i&7)-1];
    else mask_pawn_isolated[i]=Or(file_mask[(i&7)-1],file_mask[(i&7)+1]);
  }
/*
    initialize connected pawn masks, which are nothing more than 1's on
    files adjacent to the pawn and ranks that are within 1 rank of the
    pawn.
*/
  for (i=8;i<56;i++) {
    if (((i&7)>0) && ((i&7)<7))
      mask_pawn_connected[i]=Or(Or(Or(Or(Or(SetMask(i-9),SetMask(i-7)),
                                         SetMask(i-1)),
                                      SetMask(i+1)),
                                   SetMask(i+7)),
                                SetMask(i+9));
    else if ((i&7)==0)
      mask_pawn_connected[i]=Or(Or(SetMask(i-7),SetMask(i+1)),
                                SetMask(i+9));
    else if ((i&7)==7)
      mask_pawn_connected[i]=Or(Or(SetMask(i-9),SetMask(i-1)),
                                SetMask(i+7));
  }
/*
    initialize passed pawn masks, which are nothing more than 1's on
    the pawn's file and the adjacent files, but only on ranks that are
    in "front" of the pawn.  
*/
  for (i=0;i<64;i++) {
    if (!(i&7)) {
      mask_pawn_passed_w[i]=Or(plus8dir[i],plus8dir[i+1]);
      mask_pawn_passed_b[i]=Or(minus8dir[i],minus8dir[i+1]);
    }
    else if ((i&7) == 7) {
      mask_pawn_passed_w[i]=Or(plus8dir[i-1],plus8dir[i]);
      mask_pawn_passed_b[i]=Or(minus8dir[i-1],minus8dir[i]);
    }
    else {
      mask_pawn_passed_w[i]=Or(Or(plus8dir[i-1],plus8dir[i]),
                               plus8dir[i+1]);
      mask_pawn_passed_b[i]=Or(Or(minus8dir[i-1],minus8dir[i]),
                               minus8dir[i+1]);
    }
  }
/*
    these masks are used to determine if the other side has any pawns
    that can attack [square].
*/
  for (i=8;i<56;i++) {
    if (!(i&7)) {
      mask_no_pawn_attacks_w[i]=minus8dir[i+1];
      mask_no_pawn_attacks_b[i]=plus8dir[i+1];
    }
    else if ((i&7) == 7) {
      mask_no_pawn_attacks_w[i]=minus8dir[i-1];
      mask_no_pawn_attacks_b[i]=plus8dir[i-1];
    }
    else {
      mask_no_pawn_attacks_w[i]=Or(minus8dir[i-1],minus8dir[i+1]);
      mask_no_pawn_attacks_b[i]=Or(plus8dir[i+1],plus8dir[i-1]);
    }
  }
/*
    enpassant pawns are on either file adjacent to the current file, and
    on the same rank.                                          
*/
  for (i=0;i<64;i++) mask_eptest[i]=0;
  for (i=25;i<31;i++) mask_eptest[i]=Or(SetMask(i-1),SetMask(i+1));
  for (i=33;i<39;i++) mask_eptest[i]=Or(SetMask(i-1),SetMask(i+1));
  mask_eptest[A4]=SetMask(B4);
  mask_eptest[H4]=SetMask(G4);
  mask_eptest[A5]=SetMask(B5);
  mask_eptest[H5]=SetMask(G5);

/*
  masks to detect pawns bearing down on the king
*/
  mask_kingside_attack_w1=Or(Or(minus8dir[F5],minus8dir[G5]),
                             minus8dir[H5]);
  mask_kingside_attack_w2=Or(Or(minus8dir[F4],minus8dir[G4]),
                             minus8dir[H4]);
  mask_queenside_attack_w1=Or(Or(minus8dir[A5],minus8dir[B5]),
                              minus8dir[C5]);
  mask_queenside_attack_w2=Or(Or(minus8dir[A4],minus8dir[B4]),
                              minus8dir[C4]);
  mask_kingside_attack_b1=Or(Or(plus8dir[F4],plus8dir[G4]),
                             plus8dir[H4]);
  mask_kingside_attack_b2=Or(Or(plus8dir[F5],plus8dir[G5]),
                             plus8dir[H5]);
  mask_queenside_attack_b1=Or(Or(plus8dir[A4],plus8dir[B4]),
                              plus8dir[C4]);
  mask_queenside_attack_b2=Or(Or(plus8dir[A5],plus8dir[B5]),
                              plus8dir[C5]);
/* 
  pawns at d5/e5/f5 cramp black, and pawns at d4/e4/f4 cramp
  white, especially if there are no pawns that can attack
  these pawns.
*/
  pawns_cramp_black=Or(Or(SetMask(D5),SetMask(E5)),
                       SetMask(F5));
  pawns_cramp_white=Or(Or(SetMask(D4),SetMask(E4)),
                       SetMask(F4));
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
  mask_corner_squares=Or(Or(SetMask(A1),SetMask(H1)),
                         Or(SetMask(A8),SetMask(H8)));
/* 
  these masks have 1's on the squares where it is useful to have a bishop
  when the b or g pawn is missing or pushed one square.
*/
  good_bishop_kw=Or(Or(SetMask(F1),SetMask(H1)),SetMask(G2));
  good_bishop_qw=Or(Or(SetMask(A1),SetMask(C1)),SetMask(B2));
  good_bishop_kb=Or(Or(SetMask(G7),SetMask(F8)),SetMask(H8));
  good_bishop_qb=Or(Or(SetMask(B7),SetMask(A8)),SetMask(C8));
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
      mask_promotion_threat_b[i]=Or(SetMask(B2),SetMask(B3));
    }
    else if ((i&7) == 7) {
      mask_promotion_threat_b[i]=Or(SetMask(G2),SetMask(G3));
    }
    else {
      mask_promotion_threat_b[i]=Or(Or(SetMask((i&7)+7),SetMask((i&7)+9)),
                                    Or(SetMask((i&7)+15),SetMask((i&7)+17)));
    }
  }
  for (i=40;i<56;i++) {
    if (!(i&7)) {
      mask_promotion_threat_w[i]=Or(SetMask(B6),SetMask(B7));
    }
    else if ((i&7) == 7) {
      mask_promotion_threat_w[i]=Or(SetMask(G6),SetMask(G7));
    }
    else {
      mask_promotion_threat_w[i]=Or(Or(SetMask((i&7)+39),SetMask((i&7)+47)),
                                    Or(SetMask((i&7)+41),SetMask((i&7)+49)));
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
  mask_G2G3=Or(SetMask(G2),SetMask(G3));
  mask_B2B3=Or(SetMask(B2),SetMask(B3));
  mask_G6G7=Or(SetMask(G6),SetMask(G7));
  mask_B6B7=Or(SetMask(B6),SetMask(B7));
/*
  these masks are used to detect that opponent pawns are getting very
  close to the king.
*/
  mask_wq_4th=Or(Or(SetMask(A4),SetMask(B4)),
                 SetMask(C4));
  mask_wq_5th=Or(Or(SetMask(A5),SetMask(B5)),
                 SetMask(C5));
  mask_wk_4th=Or(Or(SetMask(F4),SetMask(G4)),
                 SetMask(H4));
  mask_wk_5th=Or(Or(SetMask(F5),SetMask(G5)),
                 SetMask(H5));
  mask_bk_4th=mask_wk_5th;
  mask_bq_4th=mask_wq_5th;
  mask_bk_5th=mask_wk_4th;
  mask_bq_5th=mask_wq_4th;

/*
  these masks are used to detect that the opponent is trying to set up
  a stonewall type pawn formation.
*/
  stonewall_white=Or(Or(SetMask(D4),SetMask(E3)),SetMask(F4));
  closed_white=Or(Or(SetMask(E4),SetMask(D3)),SetMask(C4));
  stonewall_black=Or(Or(SetMask(D5),SetMask(E6)),SetMask(F5));
  closed_black=Or(Or(SetMask(E5),SetMask(D6)),SetMask(C5));
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
          white_pawn_race_wtm[j]=Or(white_pawn_race_wtm[j],SetMask(i));
      }
      else {
        if (KingPawnSquare(j,i,(j&7)+56,1)) 
          white_pawn_race_wtm[j]=Or(white_pawn_race_wtm[j],SetMask(i));
      }
/* white pawn, ChangeSide(wtm) */
      if (j < 16) {
        if (KingPawnSquare(j+8,i,(j&7)+56,0)) 
          white_pawn_race_btm[j]=Or(white_pawn_race_btm[j],SetMask(i));
      }
      else {
        if (KingPawnSquare(j,i,(j&7)+56,0)) 
          white_pawn_race_btm[j]=Or(white_pawn_race_btm[j],SetMask(i));
      }
/* black pawn, wtm */
      if (j > 47) {
        if (KingPawnSquare(j-8,i,j&7,0)) 
          black_pawn_race_wtm[j]=Or(black_pawn_race_wtm[j],SetMask(i));
      }
      else {
        if (KingPawnSquare(j,i,j&7,0)) 
          black_pawn_race_wtm[j]=Or(black_pawn_race_wtm[j],SetMask(i));
      }
/* black pawn, ChangeSide(wtm) */
      if (j > 47) {
        if (KingPawnSquare(j-8,i,j&7,1)) 
          black_pawn_race_btm[j]=Or(black_pawn_race_btm[j],SetMask(i));
      }
      else {
        if (KingPawnSquare(j,i,j&7,1)) 
          black_pawn_race_btm[j]=Or(black_pawn_race_btm[j],SetMask(i));
      }
    }
  }
}

/*
********************************************************************************
*                                                                              *
*   InitializeRandomHash() is called to initialize the tables of random      *
*   numbers used to produce the incrementally-updated hash keys.  note that    *
*   this uses a treendom number generator rather than the C library one    *
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
  for (i=0;i<64;i++) {
    w_pawn_random32[i]=Random32();
    b_pawn_random32[i]=Random32();
  }
}

#if defined(SMP)
/*
********************************************************************************
*                                                                              *
*   InitlializeSMP() is used to initialize the pthread lock variables.         *
*                                                                              *
********************************************************************************
*/
void InitializeSMP(void) {
  int i;
#if defined(POSIX)
  pthread_attr_init(&pthread_attr);
  pthread_attr_setdetachstate(&pthread_attr, PTHREAD_CREATE_DETACHED);
  pthread_attr_setscope(&pthread_attr, PTHREAD_SCOPE_SYSTEM);
#endif
  LockInit(lock_hasha);
  LockInit(lock_hashb);
  LockInit(lock_pawn_hash);
  LockInit(lock_smp);
  LockInit(lock_io);
  for (i=0;i<64+1;i++)
    LockInit(local[i]->lock);
}
#endif

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
