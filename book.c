#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "types.h"
#include "function.h"
#include "data.h"
#if defined(UNIX)
#  include <unistd.h>
#endif
#define BOOK_CLUSTER_SIZE   600
#define MERGE_BLOCKSIZE    1000
#define SORT_BLOCKSIZE   200000
/*
********************************************************************************
*                                                                              *
*   book() is used to determine if the current position is in the book data-   *
*   base.  it simply takes the set of moves produced by root_moves() and then  *
*   tries each position's hash key to see if it can be found in the data-      *
*   base.  if so, such a move represents a "book move."  the set of flags is   *
*   used to decide on a sub-set of moves to be used as the "book move pool"    *
*   from which a move is chosen randomly.                                      *
*                                                                              *
*   flag bits:                                                                 *
*      0000 0001  ?? flagged move                                              *
*      0000 0010   ? flagged move                                              *
*      0000 0100   = flagged move                                              *
*      0000 1000   ! flagged move                                              *
*      0001 0000  !! flagged move                                              *
*      0010 0000  white won at least one game                                  *
*      0100 0000  black won at least one game                                  *
*      1000 0000  at least one game was a draw                                 *
*                                                                              *
*      Remainder of the bits are flag bits set by user (the next 16 bits       *
*      only).                                                                  *
*                                                                              *
********************************************************************************
*/
int Book(int wtm, int which_book)
{
  FILE *temp_book_file;
  static BOOK_POSITION buffer[BOOK_CLUSTER_SIZE];
  static int book_moves[200];
  static int selected[200];
  static int selected_order[200], selected_status[200], book_development[200];
  static int book_order[200], book_status[200], evaluations[200];
  int m1_status, status;
  int done, i, j, last_move, temp, which;
  int *mv, in_book;
  int cluster, test;
  BITBOARD temp_hash_key;
  int key, nmoves, num_selected, st;
  int percent_played, total_played, total_moves, distribution;
  int initial_development;
  char *whisper_p;

  static float tvar, variance[10] = {.4, .4, .5, .5, .5, .6, .7, .8, .9, 1.0};
  static char ch[16] = {'0','1','2','3','4','5','6','7',
                        '8','9','A','B','C','D','E','F'};

/*
 ----------------------------------------------------------
|                                                          |
|   if we have been out of book for several moves, return  |
|   and start the normal tree search.                      |
|                                                          |
 ----------------------------------------------------------
*/
  if (move_number > last_move_in_book+3) return(0);
  temp_book_file= (which_book == 1) ? books_file : book_file;
/*
 ----------------------------------------------------------
|                                                          |
|   if the starting position is not in book, don't try to  |
|   transpose back in, which leads to amusing blunders.    |
|   ie, 1. e4 e5  2. Bb5 a6  3. Nf3 and Crafty will find   |
|   3. ... Nc6 in book and play it, eschewing the free     |
|   bishop.                                                |
|                                                          |
 ----------------------------------------------------------
*/
  if (move_number > 1) {
    in_book=0;
    temp_hash_key=Xor(HashKey(1),wtm_random[!wtm]);
    temp_hash_key=BookKey(1,!wtm,temp_hash_key);
    test=temp_hash_key>>49;
    if (book_file) {
      fseek(book_file,test*sizeof(int),SEEK_SET);
      fread(&key,sizeof(int),1,book_file);
      if (key > 0) {
        fseek(book_file,key,SEEK_SET);
        fread(&cluster,sizeof(int),1,book_file);
        fread(buffer,sizeof(BOOK_POSITION),cluster,book_file);
        for (i=0;i<cluster;i++)
          if (!Xor(temp_hash_key,buffer[i].position)) in_book=1;
      }
    }
    if (!in_book) {
      if (books_file) {
        fseek(books_file,test*sizeof(int),SEEK_SET);
        fread(&key,sizeof(int),1,books_file);
        if (key > 0) {
          fseek(books_file,key,SEEK_SET);
          fread(&cluster,sizeof(int),1,books_file);
          fread(buffer,sizeof(BOOK_POSITION),cluster,books_file);
          for (i=0;i<cluster;i++)
            if (!Xor(temp_hash_key,buffer[i].position)) in_book=1;
        }
      }
    }
    if (!in_book) return(0);
  }
/*
 ----------------------------------------------------------
|                                                          |
|   position is known, read in the appropriate cluster.    |
|   note that this cluster will have all possible book     |
|   moves from current position in it (as well as others   |
|   of course.)                                            |
|                                                          |
 ----------------------------------------------------------
*/
  temp_hash_key=Xor(HashKey(1),wtm_random[wtm]);
  temp_hash_key=BookKey(1,wtm,temp_hash_key);
  test=temp_hash_key>>49;
  if (temp_book_file) {
    fseek(temp_book_file,test*sizeof(int),SEEK_SET);
    fread(&key,sizeof(int),1,temp_book_file);
    if (key > 0) {
      fseek(temp_book_file,key,SEEK_SET);
      fread(&cluster,sizeof(int),1,temp_book_file);
      fread(buffer,sizeof(BOOK_POSITION),cluster,temp_book_file);
/*
 ----------------------------------------------------------
|                                                          |
|   first cycle through the root move list, make each      |
|   move, and see if the resulting hash key is in the book |
|   database.                                              |
|                                                          |
 ----------------------------------------------------------
*/
      initial_development=(wtm) ? EvaluateDevelopment(1) : 
                                 -EvaluateDevelopment(1);
      total_moves=0;
      nmoves=0;
      for (mv=first[1];mv<last[1];mv++) {
        MakeMove(1,*mv,wtm);
        if (RepetitionCheck(2)) continue;
        temp_hash_key=Xor(HashKey(2),wtm_random[wtm]);
        temp_hash_key=BookKey(2,wtm,temp_hash_key);
        for (i=0;i<cluster;i++) {
          if (!Xor(temp_hash_key,buffer[i].position)) {
            status=Shiftr(buffer[i].status,32);
            if (!(status & book_reject_mask) &&
                (!status || (status & book_accept_mask)) &&
                ((wtm && ((status & 32) || !(status & 192))) ||
                 (!wtm && ((status & 64) || !(status & 160))))) {
              book_status[nmoves]=status;
              book_order[nmoves]=And(buffer[i].status,mask_96);
              current_move[1]=*mv;
              if (!Captured(*mv)) 
                book_development[nmoves]=((wtm) ? EvaluateDevelopment(2) : 
                      -EvaluateDevelopment(2))-initial_development;
              else book_development[nmoves]=0;
              total_moves+=And(buffer[i].status,mask_96);
              static_eval[2]=0;
              evaluations[nmoves]=Evaluate(2,wtm,-999999,999999);
              evaluations[nmoves]-=(wtm) ? Material(2) : -Material(2);
              book_moves[nmoves++]=*mv;
            }
            break;
          }
        }
      }
      if (nmoves) {
        if (show_book)
          for (i=0;i<nmoves;i++) {
            printf("%3d%% ",100*book_order[i]/total_moves);
            printf("%s ",DisplayEvaluation(evaluations[i]));
            printf("%6s", OutputMove(&book_moves[i],1,wtm));
            st=book_status[i] & book_accept_mask & (~224);
            if (st) {
              if (st & 1) printf("??");
              else if (st & 2) printf("? ");
              else if (st & 4) printf("= ");
              else if (st & 8) printf("! ");
              else if (st & 16) printf("!!");
              else {
                printf("  ");
                st=st>>8;
                printf("/");
                for (j=0;j<16;j++) {
                  if (st & 1) printf("%c",ch[j]);
                  st=st>>1;
                }
              }
            }
            else printf("  ");
            printf(" [played %d times]", book_order[i]);
            if (book_order[i] < book_lower_bound) printf(" (<%d)",
              book_lower_bound);
            if (book_development[i] < 0) printf(" (anti-thematic)");
            printf("\n");
          }
/*
 ----------------------------------------------------------
|                                                          |
|   now select a move from the set of moves just found. do |
|   this in four distinct passes:  (2) look for !! moves;  |
|   (2) look for ! moves;  (3) look for any other moves.   |
|   note: book_accept_mask *should* have a bit set for any |
|   move that is selected, including !! and ! type moves   |
|   so that they *can* be excluded if desired.  note also  |
|   that book_reject_mask should have ?? and ? set (at a   |
|   minimum) to exclude these types of moves.              |
|                                                          |
 ----------------------------------------------------------
*/
/*
   first, check for !! moves
*/
        num_selected=0;
        if (!num_selected)
          if (book_accept_mask & 16)
            for (i=0;i<nmoves;i++)
              if (book_status[i] & 16) {
                selected_status[num_selected]=book_status[i];
                selected_order[num_selected]=book_order[i];
                selected[num_selected++]=book_moves[i];
              }
/*
   if none, then check for ! moves
*/
        if (!num_selected)
          if (book_accept_mask & 8)
            for (i=0;i<nmoves;i++)
              if (book_status[i] & 8) {
                selected_status[num_selected]=book_status[i];
                selected_order[num_selected]=book_order[i];
                selected[num_selected++]=book_moves[i];
              }
/*
   if none, then check for = moves
*/
        if (!num_selected)
          if (book_accept_mask & 4)
            for (i=0;i<nmoves;i++)
              if (book_status[i] & 4)
                if (book_order[i] >= book_lower_bound) {
                  selected_status[num_selected]=book_status[i];
                  selected_order[num_selected]=book_order[i];
                  selected[num_selected++]=book_moves[i];
                }
/*
   if none, then check for any flagged moves
*/
        if (!num_selected)
          for (i=0;i<nmoves;i++)
            if (book_status[i] & book_accept_mask)
              if (book_order[i] >= book_lower_bound) {
                selected_status[num_selected]=book_status[i];
                selected_order[num_selected]=book_order[i];
                selected[num_selected++]=book_moves[i];
              }
/*
   if none, then any book move is acceptable
*/
        if (!num_selected)
          for (i=0;i<nmoves;i++) {
            if (book_development[i] >= 0) {
              selected_status[num_selected]=book_status[i];
              selected_order[num_selected]=book_order[i];
              selected[num_selected++]=book_moves[i];
            }
          }
        if (!num_selected) {
          if (which_book == 2) return(0);
          else return(Book(wtm,2));
        }
/*
   now copy moves to right place and sort 'em.
*/
        for (i=0;i<num_selected;i++) {
          book_status[i]=selected_status[i];
          book_moves[i]=selected[i];
          book_order[i]=selected_order[i];
          book_order[i]=selected_order[i];
        }
        nmoves=num_selected;
        if (nmoves > 1) {
          do {
            done=1;
            for (i=0;i<nmoves-1;i++)
              if (book_order[i] < book_order[i+1]) {
                temp=book_order[i];
                book_order[i]=book_order[i+1];
                book_order[i+1]=temp;
                temp=book_moves[i];
                book_moves[i]=book_moves[i+1];
                book_moves[i+1]=temp;
                temp=book_status[i];
                book_status[i]=book_status[i+1];
                book_status[i+1]=temp;
                done=0;
              }
          } while (!done);
        }
/*
  now determine what type of randomness is wanted, and adjust the book_order
  counts to reflect this.  the options are:  (0) do a short search for each
  move in the list and play the best move found;  (1) take the most frequently 
  played moves, which is usually only 2 or 3 out of the list;  (2) use the
  actual frequency the moves were played as a model for choosing moves;
  (3) use sqrt() of the frequency played, which adds a larger random factor
  into selecting moves;  (4) choose from the known book moves completely
   randomly, without regard to frequency.
*/
        Print(1,"              book moves(");
        for (i=0;i<nmoves;i++) {
          Print(1,"%s", OutputMove(&book_moves[i],1,wtm));
          if (i < nmoves-1) Print(1,", ");
        }
        Print(1,")\n");
        if (!puzzling && ((whisper>2) || (kibitz>3))) {
          whisper_text[0]='\0';
          sprintf(whisper_text,"book moves (");
          whisper_p=whisper_text+strlen(whisper_text);
          for (i=0;i<nmoves;i++) {
            sprintf(whisper_p,"%s",OutputMove(&book_moves[i],1,wtm));
            whisper_p=whisper_text+strlen(whisper_text);
            if (i < nmoves-1) {
              sprintf(whisper_p,", ");
              whisper_p=whisper_text+strlen(whisper_text);
            }
          }
          sprintf(whisper_p,")\n");
          if (kibitz > 2) printf("kibitz %s\n",whisper_text);
          else printf("whisper %s\n",whisper_text);
        }
        if ((which_book == 2) && (book_random == 0)) {
          if (move_number < 10)
            for (i=1;i<nmoves;i++)
              if (book_order[0] > 4*book_order[i]) {
                nmoves=i;
                break;
              }
          Print(1,"              moves considered (");
          for (i=0;i<nmoves;i++) {
            Print(1,"%s", OutputMove(&book_moves[i],1,wtm));
            if (i < nmoves-1) Print(1,", ");
          }
          Print(1,")\n");
          if (nmoves > 1) {
            for (i=0;i<nmoves;i++) *(first[1]+i)=book_moves[i];
            last[1]=first[1]+nmoves;
            pv[0].path_iteration_depth=0;
            booking=1;
            (void) Iterate(wtm,booking);
            booking=0;
          }
          else {
            pv[1].path[1]=book_moves[0];
            pv[1].path_length=1;
          }
          return(1);
        }
  
        for (last_move=0;last_move<nmoves;last_move++)
          if ((book_order[last_move] < book_min_percent_played*total_moves) &&
               !book_status[last_move]) break;
        if (last_move == 0) {
          if (which_book == 2) return(0);
          else return(Book(wtm,2));
        }
        if (book_order[0] >= 10*book_lower_bound) {
          for (last_move=1;last_move<nmoves;last_move++)
            if ((book_order[last_move] < book_lower_bound) && 
                !book_status[last_move]) break;
        }
        else if (book_order[0] >= book_absolute_lower_bound) {
          for (last_move=1;last_move<nmoves;last_move++)
            if ((book_order[last_move] < book_absolute_lower_bound) && 
                !book_status[last_move]) break;
        }
        else return(0);
        j=GetTime(elapsed)/10 % 97;
        for (i=0;i<j;i++) which=Random32();
        switch (book_random) {
          case 1:
            for (i=1;i<last_move;i++)
              if (book_order[0] > 4*book_order[i]) book_order[i]=0;
            tvar=(move_number < 10) ? variance[move_number] : variance[9];
            break;
          case 2:
            tvar=(move_number < 10) ? variance[move_number] : variance[9];
            break;
          case 3:
            for (i=0;i<last_move;i++) book_order[i]=1+sqrt(book_order[i]);
            tvar=1.0;
            break;
          case 4:
            for (i=0;i<last_move;i++) book_order[i]=100;
            tvar=1.0;
            break;
        }
/*
   now randomly choose from the "doctored" random distribution.
*/
        total_moves=0;
        for (i=0;i<last_move;i++) total_moves+=book_order[i];
        distribution=abs(which) % total_moves;
        distribution*=tvar;
        for (which=0;which<last_move;which++) {
          distribution-=book_order[which];
          if (distribution < 0) break;
        }
        which=Min(which,last_move);
        pv[1].path[1]=book_moves[which];
        percent_played=100*book_order[which]/total_moves;
        total_played=book_order[which];
        m1_status=book_status[which];
        pv[1].path_length=1;
        MakeMove(1,book_moves[which],wtm);
        Print(2,"               book   0.0s    %3d%%   ", percent_played);
        Print(2," %s",OutputMove(&pv[1].path[1],1,wtm));
        st=m1_status & book_accept_mask & (~224);
        if (st) {
          if (st & 1) Print(2,"??");
          else if (st & 2) Print(2,"?");
          else if (st & 4) Print(2,"=");
          else if (st & 8) Print(2,"!");
          else if (st & 16) Print(2,"!!");
          else {
            st=st>>8;
            Print(2,"/");
            for (j=0;j<16;j++) {
              if (st & 1) Print(2,"%c",ch[j]);
              st=st>>1;
            }
          }
        }
        Print(2," [played %d times]\n",total_played);
        return(1);
      }
    }
  }
  if (which_book == 2) return(0);
  else return(Book(wtm,2));
}

/*
********************************************************************************
*                                                                              *
*   BookKey() is used to "twiddle" with the hash key, so that the upper 16     *
*   bits are only affected by the side not on move.  this will "cluster" all   *
*   moves for a given position together in the book database when it's sorted  *
*   so that one file read can retrieve all opening move positions, rather than *
*   having to fseek() around which is very slow in a 60+mb file.               *
*                                                                              *
********************************************************************************
*/
BITBOARD BookKey(int ply, int wtm, BITBOARD hash_key)
{
  int i;
  for (i=0;i<64;i++)
    switch (PieceOnSquare(ply,i)) {
      case king:
        if (wtm) hash_key=Xor(hash_key,And(mask_16,w_king_random[i]));
        break;
      case queen:
        if (wtm) hash_key=Xor(hash_key,And(mask_16,w_queen_random[i]));
        break;
      case rook:
        if (wtm) hash_key=Xor(hash_key,And(mask_16,w_rook_random[i]));
        break;
      case bishop:
        if (wtm) hash_key=Xor(hash_key,And(mask_16,w_bishop_random[i]));
        break;
      case knight:
        if (wtm) hash_key=Xor(hash_key,And(mask_16,w_knight_random[i]));
        break;
      case pawn:
        if (wtm) hash_key=Xor(hash_key,And(mask_16,w_pawn_random[i]));
        break;
      case -pawn:
        if (!wtm) hash_key=Xor(hash_key,And(mask_16,b_pawn_random[i]));
        break;
      case -knight:
        if (!wtm) hash_key=Xor(hash_key,And(mask_16,b_knight_random[i]));
        break;
      case -bishop:
        if (!wtm) hash_key=Xor(hash_key,And(mask_16,b_bishop_random[i]));
        break;
      case -rook:
        if (!wtm) hash_key=Xor(hash_key,And(mask_16,b_rook_random[i]));
        break;
      case -queen:
        if (!wtm) hash_key=Xor(hash_key,And(mask_16,b_queen_random[i]));
        break;
      case -king:
        if (!wtm) hash_key=Xor(hash_key,And(mask_16,b_king_random[i]));
        break;
      default:
        break;
    }
  return(hash_key);
}

/*
********************************************************************************
*                                                                              *
*   BookUp() is used to create/add to the opening book file.  typing "book     *
*   create" will erase the old book file and start from scratch, typing "book  *
*   add" will simply add more moves to the existing file.                      *
*                                                                              *
*   the format of the input data is a left bracket ("[") followed by any title *
*   information desired, followed by a right bracket ("]") followed by a       *
*   sequence of moves.  the sequence of moves is assumed to start at ply=1,    *
*   with white-to-move (normal opening position) and can contain as many moves *
*   as desired (no limit on the depth of each variation.)  the file *must* be  *
*   terminated with a line that begins with "end", since handling the EOF      *
*   condition makes portable code difficult.                                   *
*                                                                              *
*   book moves can either be typed in by hand, directly into book_add(), by    *
*   using the "book create/add" command.  using the command "book add/create   *
*   filename" will cause book_add() to read its opening text moves from        *
*   filename rather than from the key                                    *
*                                                                              *
*   in addition to the normal text for a move (reduced or full algebraic is    *
*   accepted, ie, e4, ed, exd4, e3d4, etc. are all acceptable) some special    *
*   characters can be appended to a move.  the move must be immediately        *
*   followed by either a "/" or a "\" followed by one or more of the following *
*   mask characters with no intervening spaces.  if "/" preceeds the flags,    *
*   the flags are added (or'ed) to any already existing flags that might be    *
*   from other book variations that pass through this position.  if "\" is     *
*   used, these flags replace any existing flags, which is the easy way to     *
*   clear incorrect flags and/or replace them with new ones.                   *
*                                                                              *
*      ?? ->  never play this move.  since the same book is used for both      *
*             black and white, you can enter moves in that white might play,   *
*             but prevent the program from choosing them on its own.           *
*      ?  ->  avoid this move except for non-important games.  these openings  *
*             are historically those that the program doesn't play very well,  *
*             but which aren't outright losing.                                *
*      =  ->  drawish move, only play this move if drawish moves are allowed   *
*             by the operator.  this is used to encourage the program to play  *
*             drawish openings (Petrov's comes to mind) when the program needs *
*             to draw or is facing a formidable opponent (deep thought comes   *
*             to mind.)                                                        *
*      !  ->  always play this move, if there isn't a move with the !! flag    *
*             set also.  this is a strong move, but not as strong as a !!      *
*             moveing traps.                                                   *
*      !! ->  always play this move.  this can be used to make the program     *
*             favor particular lines, or to mark a strong move for certain     *
*             opening traps.                                                   *
*     0-f ->  16 user-defined flags.  the program will ignore these flags      *
*             unless the operator sets the "book mask" to contain them which   *
*             will "require" the program to play from the set of moves with    *
*             at least one of the flags in "book mask" set.                    *
*                                                                              *
********************************************************************************
*/
void BookUp(char output_filename[])
{
  BOOK_POSITION *buffer;
  int move;
  BITBOARD temp_hash_key;
  FILE *book_input, *output_file;
  char flags[40], fname[64], text[30], nextc, which_mask[20];
  int white_won, black_won, drawn, i, mask_word, total_moves;
  int move_num, wtm, book_positions, rarely_played;
  int cluster, max_cluster, discarded, errors, data_read;
  int start_cpu_time, start_elapsed_time, following, ply, max_ply=256;
  int files, buffered=0;
  BOOK_POSITION current, next;
  int last, cluster_seek, next_cluster;
  int counter, *index, start;

/*
 ----------------------------------------------------------
|                                                          |
|   determine if we should read the book moves from a file |
|   or from the operator (which is normally used to add/   |
|   delete moves just before a game.)                      |
|                                                          |
 ----------------------------------------------------------
*/
  InitializeChessBoard(&position[1]);
  scanf("%s",text);
  book_input=input_stream;
  if (!strcmp(text,"add") || !strcmp(text,"create")) {
    nextc=getc(input_stream);
    if (nextc == ' ') {
      data_read=fscanf(input_stream,"%s",fname);
      if (!(book_input=fopen(fname,"r"))) {
        printf("file %s does not exist.\n",fname);
        return;
      }
      nextc=getc(input_stream);
      if (nextc == ' ') {
        data_read=fscanf(input_stream,"%d",&max_ply);
      }
    }
  }
/*
 ----------------------------------------------------------
|                                                          |
|   open the correct book file for writing/reading         |
|                                                          |
 ----------------------------------------------------------
*/
  if (!strcmp(text,"create")) {
    if (book_file) fclose(book_file);
    book_file=fopen(output_filename,"wb+");
    buffer=(BOOK_POSITION *) malloc(sizeof(BOOK_POSITION)*100);
    if (!buffer) {
      printf("not enough memory for buffer.\n");
      return;
    }
    fseek(book_file,0,SEEK_SET);
  }
  else if (!strcmp(text,"off")) {
    if (book_file) fclose(book_file);
    book_file=0;
    books_file=0;
    Print(0,"book file disabled.\n");
    return;
  }
  else if (!strcmp(text,"on")) {
    if (!book_file)
      book_file=fopen("book.bin","rb+");
      books_file=fopen("books.bin","rb+");
    Print(0,"book file enabled.\n");
    return;
  }
  else if (!strcmp(text,"played")) {
    scanf("%f",&book_min_percent_played);
    book_min_percent_played/=100.0;
    printf("moves must be played at least %6.2f to be used.\n",
           book_min_percent_played*100);
    return;
  }
  else if (!strcmp(text,"random")) {
    data_read=fscanf(input_stream,"%d",&book_random);
    switch (book_random) {
      case 0:
        Print(0,"play best book line after search.\n");
        break;
      case 1: 
        Print(0,"play best book lines.\n");
        break;
      case 2:
        Print(0,"play most popular book lines.\n");
        break;
      case 3:
        Print(0,"play most popular book lines, but vary more.\n");
        break;
      case 4:
        Print(0,"play random book lines.\n");
        break;
      default:
        Print(0,"valid options are 0-4.\n");
        break;
    }
    return;
  }
  else if (!strcmp(text,"mask")) {
    data_read=fscanf(input_stream,"%s",which_mask);
    data_read=fscanf(input_stream,"%s",flags);
    if (!strcmp(which_mask,"accept")) {
      book_accept_mask=BookMask(flags);
      book_reject_mask=book_reject_mask & ~book_accept_mask;
    }
    else if (!strcmp(which_mask,"reject")) {
      book_reject_mask=BookMask(flags);
      book_accept_mask=book_accept_mask & ~book_reject_mask;
    }
    else {
      printf("usage:  book mask accept|reject <chars>\n");
    }
    return;
  }
  else if (!strcmp(text,"lower")) {
    data_read=fscanf(input_stream,"%d",&book_lower_bound);
    return;
  }
  else {
    printf("usage:  book edit/create/off [filename]\n");
    return;
  }
/*
 ----------------------------------------------------------
|                                                          |
|   now, read in a series of moves (terminated by the "["  |
|   of the next title or by "end" for end of the file)     |
|   and make them.  after each MakeMove(), we can grab     |
|   the hash key, and use it to access the book data file  |
|   to add this position.  note that we have to check the  |
|   last character of a move for the special flags and     |
|   set the correct bit in the status for this position.   |
|   when we reach the end of a book line, we back up to    |
|   the starting position and start over.                  |
|                                                          |
 ----------------------------------------------------------
*/
  start=strcmp(output_filename,"book.bin");
  printf("parsing pgn move file (1000 moves/dot)\n");
  start_cpu_time=GetTime(cpu);
  start_elapsed_time=GetTime(elapsed);
  if (book_file) {
    total_moves=0;
    max_search_depth=0;
    discarded=0;
    errors=0;
    if (book_input == stdin) {
      printf("enter book text, first line must be [title information],\n");
      printf("type \"end\" to exit.\n");
      printf("title(1): ");
    }
    do 
      data_read=fscanf(book_input,"%s",text);
    while ((text[0] != '[') && strcmp(text,"end") && (data_read>0));
    do {
      if (book_input != stdin) if (verbosity_level) printf("%s ", text);
      white_won=0;
      black_won=0;
      drawn=0;
      if (start) {
        white_won=1;
        black_won=1;
      }
      while ((text[strlen(text)-1] != ']') && 
             strcmp(text,"end") && (data_read>0)) {
        data_read=fscanf(book_input,"%s",text);
        if (strstr(text,"1-0")) white_won=1;
        else if (strstr(text,"0-1")) black_won=1;
        else if (strstr(text,"1/2-1/2")) drawn=1;
        if ((book_input != stdin) && verbosity_level) printf("%s ",text);
      }
      if ((book_input != stdin) && verbosity_level) printf("\n");
      if (!strcmp(text,"end") || (data_read<=0)) break;
      wtm=1;
      move_num=1;
      position[2]=position[1];
      ply=0;
      following=1;
      while (data_read>0) {
        if (book_input == stdin)
          if (wtm)
            printf("WhitePieces(%d): ",move_num);
          else
            printf("BlackPieces(%d): ",move_num);
        do
          data_read=fscanf(book_input,"%s",text);
        while ((text[0] >= '0') && (text[00] <= '9') && (data_read>0));
        mask_word=0;
        if (!strcmp(text,"end") || (data_read<=0)) {
          if (book_input != stdin) fclose(book_input);
          break;
        }
        else if (strchr(text,'/')) {
          strcpy(flags, strchr(text,'/')+1);
          *strchr(text,'/')='\0';
          mask_word=BookMask(flags);
        }
        else {
          for (i=0;i<(int)strlen(text);i++)
            if (strchr("?!",text[i])) {
              strcpy(flags,&text[i]);
              text[i]='\0';
              mask_word=BookMask(flags);
              break;
            }
        }
        mask_word|=(white_won<<5)+(black_won<<6)+(drawn<<7);
        if (text[0] == '[') break;
        if (!strchr(text,'$') && !strchr(text,'*')) {
          move=InputMove(text,2,wtm,0);
          if (move) {
            ply++;
            max_search_depth=Max(max_search_depth,ply);
            total_moves++;
            if (!(total_moves % 1000)) {
              printf(".");
              if (!(total_moves % 60000)) printf(" (%d)\n",total_moves);
              fflush(stdout);
            }
            MakeMove(2,move,wtm);
            position[2]=position[3];
            if ((ply <= max_ply) || (following && (Captured(move) || Promote(move)))) {
              temp_hash_key=Xor(HashKey(2),wtm_random[wtm]);
              temp_hash_key=BookKey(2,wtm,temp_hash_key);
              buffer[buffered].position=temp_hash_key;
              buffer[buffered++].status=Shiftl((BITBOARD) mask_word,32);
              if (buffered > 99) {
                fwrite(buffer,sizeof(BOOK_POSITION),100,book_file);
                buffered=0;
              }
            }
            else {
              following=0;
              discarded++;
            }
          }
          else {
            errors++;
            Print(0,"ERROR!  move %d: %s is illegal\n",move_num,text);
            DisplayChessBoard(stdout,position[2]);
            break;
          }
          wtm=!wtm;
          if (wtm) move_num++;
        } 
      }
    } while (strcmp(text,"end") && (data_read>0));
    if (buffered) {
      fwrite(buffer,sizeof(BOOK_POSITION),buffered,book_file);
    }
/*
 ----------------------------------------------------------
|                                                          |
|   done, now we have to sort this mess into ascending     |
|   order, move by move, to get moves that belong in the   |
|   same record adjacent to each other so we can "collapse |
|   and count" easily.                                     |
|                                                          |
 ----------------------------------------------------------
*/
    printf("\nsorting %d moves (%dK/dot) ",total_moves,SORT_BLOCKSIZE/1000);
    fflush(stdout);
    free(buffer);
    buffer=(BOOK_POSITION *) malloc(sizeof(BOOK_POSITION)*SORT_BLOCKSIZE);
    fseek(book_file,0,SEEK_SET);
    data_read=SORT_BLOCKSIZE;
    for (files=1;files < 10000; files++) {
      if (data_read < SORT_BLOCKSIZE) break;
      data_read=fread(buffer,sizeof(BOOK_POSITION),SORT_BLOCKSIZE,book_file);
      if (!data_read) break;
      qsort((char *) buffer,data_read,sizeof(BOOK_POSITION),BookUpCompare);
      sprintf(fname,"sort.%d",files);
      if(!(output_file=fopen(fname,"wb+"))) 
        printf("ERROR.  unable to open sort output file\n");
      fwrite(buffer,sizeof(BOOK_POSITION),data_read,output_file);
      fclose(output_file);
      if (files%10) printf(".");
      else printf("(%d)",files);
      fflush(stdout);
    }
    fclose(book_file);
    free(buffer);
    printf("<done>\n");
/*
 ----------------------------------------------------------
|                                                          |
|   now merge these "chunks" into book.bin, keeping all of |
|   the "flags" as well as counting the number of times    |
|   that each move was played.                             |
|                                                          |
 ----------------------------------------------------------
*/
    printf("merging sorted files (%d) (10K/dot)\n",files-1);
    counter=0;
    index=malloc(32768*sizeof(int));
    for (i=0;i<32768;i++) index[i]=-1;
    current=BookUpNextPosition(files);
    if (start) current.status=And(current.status,mask_32)+100;
    else current.status++;
    book_file=fopen(output_filename,"wb+");
    fseek(book_file,sizeof(int)*32768,SEEK_SET);
    last=current.position>>49;
    index[last]=ftell(book_file);
    book_positions=0;
    rarely_played=0;
    cluster=0;
    cluster_seek=sizeof(int)*32768;
    fseek(book_file,cluster_seek+sizeof(int),SEEK_SET);
    max_cluster=0;
    while (1) {
      next=BookUpNextPosition(files);
      counter++;
      if (counter%10000 == 0) {
        printf(".");
        if (counter%600000 == 0) printf("(%d)\n",counter);
        fflush(stdout);
      }
      if (current.position == next.position) {
        if (!start) current.status++;
        current.status=Or(current.status,next.status);
      }
      else {
        book_positions++;
        cluster++;
        max_cluster=Max(max_cluster,cluster);
        fwrite(&current,sizeof(BOOK_POSITION),1,book_file);
        if (last != (next.position>>49)) {
          next_cluster=ftell(book_file);
          fseek(book_file,cluster_seek,SEEK_SET);
          fwrite(&cluster,sizeof(int),1,book_file);
          if (next.position == 0) break;
          fseek(book_file,next_cluster+sizeof(int),SEEK_SET);
          cluster_seek=next_cluster;
          last=next.position>>49;
          index[last]=next_cluster;
          cluster=0;
        }
        if (And(current.status,mask_32) < book_lower_bound) rarely_played++;
        current=next;
        if (start) current.status=And(current.status,mask_32)+100;
        else current.status++;
      }
      if (next.position == 0) break;
    }
    fseek(book_file,0,SEEK_SET);
    fwrite(index,sizeof(int),32768,book_file);
/*
 ----------------------------------------------------------
|                                                          |
|   now clean up, remove the sort.n files, and print the   |
|   statistics for building the book.                      |
|                                                          |
 ----------------------------------------------------------
*/
/*
    for (i=1;i<files;i++) {
      sprintf(fname,"sort.%d",i);
      remove(fname);
    }
*/
    start_cpu_time=GetTime(cpu)-start_cpu_time;
    start_elapsed_time=GetTime(elapsed)-start_elapsed_time;
    Print(0,"\n\nparsed %d moves.\n",total_moves);
    Print(0,"found %d errors during parsing.\n",errors);
    Print(0,"discarded %d moves (maxply=%d).\n",discarded,max_ply);
    Print(0,"book contains %d unique positions.\n",book_positions);
    Print(0,"deepest book line was %d plies.\n",max_search_depth);
    Print(0,"longest cluster of moves was %d.\n",max_cluster);
    Print(0,"%d positions were played less than %d times.\n",
          rarely_played, book_lower_bound);
    Print(0,"time used:  %s cpu  ", DisplayTime(start_cpu_time));
    Print(0,"  %s elapsed.\n", DisplayTime(start_elapsed_time));
  }
}

/*
********************************************************************************
*                                                                              *
*   BookMask() is used to convert the flags for a book move into a 32 bit      *
*   mask that is either kept in the file, or is set by the operator to select  *
*   which opening(s) the program is allowed to play.                           *
*                                                                              *
********************************************************************************
*/
int BookMask(char *flags)
{
  int f, i, mask;
  mask=0;
  for (i=0;i<(int) strlen(flags);i++) {
    if (flags[i] == '*')
      mask=-1;
    else if (flags[i] == '?') {
      if (flags[i+1] == '?') {
        mask=mask | 1;
        i++;
      }
      else
        mask=mask | 2;
    }
    else if (flags[i] == '=') {
      mask=mask | 4;
    }
    else if (flags[i] == '!') {
      if (flags[i+1] == '!') {
        mask=mask | 16;
        i++;
      }
      else
        mask=mask | 8;
    }
    else {
      f=flags[i]-'0';
      if ((f >= 0) && (f <= 9))
        mask=mask | (1 << (f+8));
      else {
        f=flags[i]-'a';
        if ((f >= 0) && (f <= 5))
          mask=mask | (1 << (f+18));
        else {
          f=flags[i]-'A';
          if ((f >= 0) && (f <= 5))
            mask=mask | (1 << (f+18));
          else
            printf("unknown book mask character -%c- ignored.\n",flags[i]);
        }
      }
    }
  }
  return(mask);
}

/*
********************************************************************************
*                                                                              *
*   BookUpNextPosition() is the heart of the "merge" operation that is done    *
*   after the chunks of the parsed/hashed move file are sorted.  this code     *
*   opens the sort.n files, and returns the least (lexically) position key to  *
*   counted/merged into the main book database.                                *
*                                                                              *
********************************************************************************
*/
BOOK_POSITION BookUpNextPosition(int files)
{
  static int init=0;
  char fname[20];
  static FILE *input_file[100];
  static BOOK_POSITION *buffer[100];
  static int data_read[100], next[100];
  int i, used;
  BOOK_POSITION least;
  if (!init) {
    init=1;
    for (i=1;i<files;i++) {
      sprintf(fname,"sort.%d",i);
      if (!(input_file[i]=fopen(fname,"rb"))) {
        printf("unable to open sort.%d file, may be too many files open.\n",i);
      }
      buffer[i]=malloc(sizeof(BOOK_POSITION)*MERGE_BLOCKSIZE);
      if (!buffer[i]) {
        printf("out of memory.  aborting. \n");
        exit(1);
      }
      fseek(input_file[i],0,SEEK_SET);
      data_read[i]=fread(buffer[i],sizeof(BOOK_POSITION),MERGE_BLOCKSIZE,input_file[i]);
      next[i]=0;
    }
  }
  least.position=0;
  used=-1;
  for (i=1;i<files;i++)
    if (data_read[i]) {
      least=buffer[i][next[i]];
      used=i;
      break;
    }
  if (least.position == 0) {
    printf("no more data, file=%d\n",used);
    for (i=1;i<files;i++) fclose(input_file[i]);
    return(least);
  }
  for (i++;i<files;i++) {
    if (data_read[i]) {
      if (least.position > buffer[i][next[i]].position) {
        least=buffer[i][next[i]];
        used=i;
      }
    }
  }
  if (--data_read[used] == 0) {
    data_read[used]=fread(buffer[used],sizeof(BOOK_POSITION),MERGE_BLOCKSIZE,input_file[used]);
    next[used]=0;
  }
  else
    next[used]++;
  return(least);
}

int BookUpCompare(const void *pos1, const void *pos2)
{
  if (((BOOK_POSITION *)pos1)->position < ((BOOK_POSITION *)pos2)->position) return(-1);
  else if (((BOOK_POSITION *)pos1)->position > ((BOOK_POSITION *)pos2)->position) return(1);
  return(0);
}
