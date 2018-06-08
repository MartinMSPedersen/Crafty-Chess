#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "types.h"
#include "function.h"
#include "data.h"
#if defined(UNIX)
#  include <unistd.h>
#endif
#define BOOK_CLUSTER_SIZE 200
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
********************************************************************************
*/
int Book(int wtm, int which_book)
{
  FILE *temp_book_file;
  BOOK_POSITION buffer[BOOK_CLUSTER_SIZE];
  int book_moves[200];
  int selected[200];
  int selected_order[200], selected_status[200], book_development[200];
  int book_order[200], book_status[200], m1_status, status;
  int done, i, j, last_move, temp, which;
  int *mv;
  int cluster, test;
  BITBOARD temp_hash_key;
  int key, nmoves, num_selected, st;
  int percent_played, total_played, total_moves, distribution;
  int initial_development;

  char ch[16] = {'0','1','2','3','4','5','6','7',
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
|   first cycle through the root move list, make each      |
|   move, and see if the resulting hash key is in the book |
|   database.                                              |
|                                                          |
 ----------------------------------------------------------
*/
  initial_development=(wtm) ? Evaluate_Development(1) : -Evaluate_Development(1);
  total_moves=0;
  if (temp_book_file) {
    nmoves=0;
    for (mv=first[1];mv<last[1];mv++) {
      Make_Move(1,*mv,wtm);
      if (Repetition_Check(2)) continue;
      temp_hash_key=Xor(Hash_Key(2),
            enpassant_random[First_One(EnPassant_Target(2))]);
      temp_hash_key=Xor(temp_hash_key,
                        castle_random_w[(int) White_Castle(2)]);
      temp_hash_key=Xor(temp_hash_key,
                        castle_random_b[(int) Black_Castle(2)]);
      temp_hash_key=Xor(temp_hash_key,wtm_random[wtm]);
      test=temp_hash_key>>49;
      fseek(temp_book_file,test*sizeof(int),SEEK_SET);
      fread(&key,sizeof(int),1,temp_book_file);
      if (key > 0) {
        fseek(temp_book_file,key,SEEK_SET);
        fread(&cluster,sizeof(int),1,temp_book_file);
        fread(&buffer,sizeof(BOOK_POSITION),cluster,temp_book_file);
        for (i=0;i<cluster;i++) {
          if (!Xor(temp_hash_key,buffer[i].position)) {
            status=Shiftr(buffer[i].status,32);
            if (!(status & book_reject_mask) &&
                (!status || (status & book_accept_mask))) {
              book_status[nmoves]=status;
              book_order[nmoves]=And(buffer[i].status,mask_96);
              if (!Captured(*mv)) 
                book_development[nmoves]=((wtm) ? Evaluate_Development(2) : 
                      -Evaluate_Development(2))-initial_development;
              else
                book_development[nmoves]=0;
              total_moves+=And(buffer[i].status,mask_96);
              book_moves[nmoves++]=*mv;
            }
            break;
          }
          else
            if (test != (buffer[i].position>>49)) break;
        }
      }
    }
    if (nmoves) {
      if (show_book)
        for (i=0;i<nmoves;i++) {
          printf("%3d%% ",100*book_order[i]/total_moves);
          printf("%6s", Output_Move(&book_moves[i],1,wtm));
          st=book_status[i] & book_accept_mask;
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
  counts to reflect this.  the options are:  (0) take the most frequently 
  played moves, which is usually only 2 or 3 out of the list;  (1) use the
  actual frequency the moves were played as a model for choosing moves;
  (2) use sqrt() of the frequency played, which adds a larger random factor
  into selecting moves;  (3) choose from the known book moves completely
   randomly, without regard to frequency.
*/
      for (last_move=0;last_move<nmoves;last_move++)
        if ((book_order[last_move] < book_min_percent_played*total_moves) &&
             !book_status[last_move]) break;
      if (last_move == 0) {
        if (which_book == 2) return(0);
        else return(Book(wtm,2));
      }
      if (book_order[0] >= book_lower_bound) {
        for (last_move=1;last_move<nmoves;last_move++)
          if ((book_order[last_move] < book_lower_bound) && 
              !book_status[last_move]) break;
      }
      else last_move=nmoves;
      j=Get_Time(elapsed)/10 % 97;
      for (i=0;i<j;i++) which=Random32();
      switch (book_random) {
        case 0:
          for (i=1;i<last_move;i++)
            if (book_order[0] > 4*book_order[i]) book_order[i]=0;
          break;
        case 1:
          break;
        case 2:
          for (i=0;i<last_move;i++) book_order[i]=1+sqrt(book_order[i]);
          break;
        case 3:
          for (i=0;i<last_move;i++) book_order[i]=100;
          break;
      }
/*
   now randomly choose from the "doctored" random distribution.
*/
      total_moves=0;
      for (i=0;i<last_move;i++) total_moves+=book_order[i];
      distribution=abs(which) % total_moves;
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
      Make_Move(1,book_moves[which],wtm);
/*
 ----------------------------------------------------------
|                                                          |
|   after choosing a book move for the program, we now     |
|   need a predicted move for pondering as well.  do the   |
|   same thing again after making the book move, so that   |
|   we will have a predicted move also.                    |
|                                                          |
 ----------------------------------------------------------
*/
      nmoves=0;
      Print(2,"               book   0.0s    %3d%%   ", percent_played);
      Print(2," %s",Output_Move(&pv[1].path[1],1,wtm));
      if (m1_status & book_accept_mask) {
        st=m1_status & book_accept_mask;
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
  if (which_book == 2) return(0);
  else return(Book(wtm,2));
}
