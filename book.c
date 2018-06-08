#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "chess.h"
#include "data.h"
#if defined(UNIX)
#  include <unistd.h>
#endif

/* last modified 12/02/96 */
/*
********************************************************************************
*                                                                              *
*   Book() is used to determine if the current position is in the book data-   *
*   base.  it simply takes the set of moves produced by root_moves() and then  *
*   tries each position's hash key to see if it can be found in the data-      *
*   base.  if so, such a move represents a "book move."  the set of flags is   *
*   used to decide on a sub-set of moves to be used as the "book move pool"    *
*   from which a move is chosen randomly.                                      *
*                                                                              *
*   the format of a book position is as follows:                               *
*                                                                              *
*   64 bits:  hash key for this position.                                      *
*                                                                              *
*   16 bits:  flag bits defined as  follows:                                   *
*                                                                              *
*      0000 0001  ?? flagged move                (0001)                        *
*      0000 0010   ? flagged move                (0002)                        *
*      0000 0100   = flagged move                (0004)                        *
*      0000 1000   ! flagged move                (0010)                        *
*      0001 0000  !! flagged move                (0020)                        *
*                                                                              *
*      Remainder of the bits are flag bits set by user (the next 11 bits       *
*      only).                                                                  *
*                                                                              *
*   16 bits:  number of games won by white after playing this move.            *
*                                                                              *
*   16 bits:  number of games drawn after playing this move.                   *
*                                                                              *
*   16 bits:  number of games won by black after playing this move.            *
*                                                                              *
*    8 bits:  number of games used in "learned value" for this move.           *
*                                                                              *
*   24 bits:  learned value + 037777777 to make it positive.                   *
*                                                                              *
*     (note:  counts are clamped to 65535 in case they overflow)               *
*                                                                              *
********************************************************************************
*/
#define BAD_MOVE  002
#define GOOD_MOVE 010

int Book(int wtm)
{
  static int book_moves[200];
  static BOOK_POSITION start_moves[20];
  static int selected[200];
  static int selected_order_won[200], selected_order_drawn[200],
         selected_order_lost[200];
  static int selected_status[200], book_development[200];
  static int book_order_won[200], book_order_drawn[200], book_order_lost[200];
  static int book_status[200], evaluations[200], book_order_learn[200];
  static int book_order_learn_count[200];
  static float book_sort_value[200];
  int m1_status, status, forced=0;
  float won, lost, tempr;
  int done, i, j, last_move, temp, which, minv=999999, maxv=-999999;
  int *mv, mp, value;
  int cluster, test;
  BITBOARD temp_hash_key, common;
  int key, nmoves, num_selected, st;
  int percent_played, total_played, total_moves, smoves;
  int distribution;
  int initial_development;
  char *whisper_p;

  static char ch[11] = {'0','1','2','3','4','5','6','7',
                        '8','9','A'};

/*
 ----------------------------------------------------------
|                                                          |
|   if we have been out of book for several moves, return  |
|   and start the normal tree search.                      |
|                                                          |
 ----------------------------------------------------------
*/
  if (move_number > last_move_in_book+3) return(0);
/*
 ----------------------------------------------------------
|                                                          |
|   position is known, read the start book file and save   |
|   each move found.  these will be used later to augment  |
|   the flags in the normal book to offer better control.  |
|                                                          |
 ----------------------------------------------------------
*/
  test=HashKey>>49;
  smoves=0;
  if (books_file) {
    fseek(books_file,test*sizeof(int),SEEK_SET);
    fread(&key,sizeof(int),1,books_file);
    if (key > 0) {
      fseek(books_file,key,SEEK_SET);
      fread(&cluster,sizeof(int),1,books_file);
      fread(buffer,sizeof(BOOK_POSITION),cluster,books_file);
      for (mv=last[0];mv<last[1];mv++) {
        common=And(HashKey,mask_16);
        MakeMove(1,*mv,wtm);
        if (RepetitionCheck(2,ChangeSide(wtm))) {
          UnMakeMove(1,*mv,wtm);
          return(0);
        }
        temp_hash_key=Xor(HashKey,wtm_random[wtm]);
        temp_hash_key=Or(And(temp_hash_key,Compl(mask_16)),common);
        for (i=0;i<cluster;i++)
          if (!Xor(temp_hash_key,buffer[i].position)) {
            start_moves[smoves++]=buffer[i];
            break;
          }
        UnMakeMove(1,*mv,wtm);
      }
    }
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
  test=HashKey>>49;
  if (book_file) {
    fseek(book_file,test*sizeof(int),SEEK_SET);
    fread(&key,sizeof(int),1,book_file);
    if (key > 0) {
      fseek(book_file,key,SEEK_SET);
      fread(&cluster,sizeof(int),1,book_file);
      fread(buffer,sizeof(BOOK_POSITION),cluster,book_file);
/*
 ----------------------------------------------------------
|                                                          |
|   cycle through the main book position list to see if    |
|   the start positions are in there.  if so, merge the    |
|   status bits, otherwise add the start position to the   |
|   end of the known positions so we will play them in     |
|   any circumstance where they are valid.                 |
|                                                          |
 ----------------------------------------------------------
*/
      for (i=0;i<smoves;i++) {
        for (j=0;j<cluster;j++)
          if (!Xor(buffer[j].position,start_moves[i].position)) {
            buffer[j].status=Or(buffer[j].status,
                                And(start_moves[i].status,mask_16));
            break;
          }
        if (j == cluster) buffer[cluster++]=start_moves[i];
      }
/*
 ----------------------------------------------------------
|                                                          |
|   if any moves have a very bad or a very good learn      |
|   value, set the appropriate ? or ! flag so the move     |
|   be played or avoided as appropriate.                   |
|                                                          |
 ----------------------------------------------------------
*/
      for (j=0;j<cluster;j++) {
        if (((int) (buffer[j].learn&077777777)-037777777) < LEARN_COUNTER_BAD) 
          buffer[j].status|=Shiftl((BITBOARD) BAD_MOVE,48);
        if (((int) (buffer[j].learn&077777777)-037777777) > LEARN_COUNTER_GOOD) 
          buffer[j].status|=Shiftl((BITBOARD) GOOD_MOVE,48);
      }
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
      for (mv=last[0];mv<last[1];mv++) {
        common=And(HashKey,mask_16);
        MakeMove(1,*mv,wtm);
        if (RepetitionCheck(2,ChangeSide(wtm))) {
          UnMakeMove(1,*mv,wtm);
          return(0);
        }
        temp_hash_key=Xor(HashKey,wtm_random[wtm]);
        temp_hash_key=Or(And(temp_hash_key,Compl(mask_16)),common);
        for (i=0;i<cluster;i++) {
          if (!Xor(temp_hash_key,buffer[i].position)) {
            status=Shiftr(buffer[i].status,48);
            if (puzzling || (!(status & book_reject_mask) && 
                            ((status & book_accept_mask) ||
                ((wtm && ((unsigned int) Shiftr(buffer[i].status,32))&65535) ||
                 (!wtm && ((unsigned int) buffer[i].status)&65535))))) {
              book_status[nmoves]=status;
              if (wtm) {
                book_order_won[nmoves]=Shiftr(buffer[i].status,32)&65535;
                book_order_drawn[nmoves]=Shiftr(buffer[i].status,16)&65535;
                book_order_lost[nmoves]=buffer[i].status&65535;
              }
              else {
                book_order_lost[nmoves]=Shiftr(buffer[i].status,32)&65535;
                book_order_drawn[nmoves]=Shiftr(buffer[i].status,16)&65535;
                book_order_won[nmoves]=buffer[i].status&65535;
              }
              book_order_learn[nmoves]=(int) (buffer[i].learn&077777777)-037777777;
              book_order_learn_count[nmoves]=((buffer[i].learn>>24)&255);
              if (puzzling) book_order_won[nmoves]+=1;
              current_move[1]=*mv;
              if (!Captured(*mv)) 
                book_development[nmoves]=((wtm) ? EvaluateDevelopment(2) : 
                      -EvaluateDevelopment(2))-initial_development;
              else book_development[nmoves]=0;
              total_moves+=book_order_won[nmoves];
              total_moves+=book_order_drawn[nmoves];
              total_moves+=book_order_lost[nmoves];
              evaluations[nmoves]=Evaluate(2,wtm,-999999,999999);
              evaluations[nmoves]-=(wtm) ? Material : -Material;
              book_moves[nmoves++]=*mv;
            }
            break;
          }
        }
        UnMakeMove(1,*mv,wtm);
      }

/*
 ----------------------------------------------------------
|                                                          |
|   we have the book moves, now it's time to decide how    |
|   they are supposed to be sorted and compute the sort    |
|   key.                                                   |
|                                                          |
 ----------------------------------------------------------
*/
      switch (book_random) {
      case 0: /* tree search all book moves to choose. */
        for (i=0;i<nmoves;i++) 
          book_sort_value[i]=book_order_won[i]+book_order_drawn[i]+book_order_lost[i];
        break;
      case 1: /* book moves sorted by win:loss ratio. */
        for (i=0;i<nmoves;i++) {
          won=book_order_won[i];
          lost=Max(book_order_lost[i],4);
          book_sort_value[i]=(won*won)/(lost*lost);
        }
        break;
      case 2: /* book moves sorted by popularity (times played.) */
        for (i=0;i<nmoves;i++) 
          book_sort_value[i]=book_order_won[i]+book_order_drawn[i]+book_order_lost[i];
        break;
      case 3: /* book moves sorted by learned results. */
        for (i=0;i<nmoves;i++) 
          book_sort_value[i]=book_order_learn[i];
        for (i=0;i<nmoves;i++) 
          if (book_order_learn[i] > 0) break;
        if (i < nmoves) break;
        for (i=0;i<nmoves;i++) 
          book_sort_value[i]=book_order_won[i]+book_order_drawn[i]+book_order_lost[i];
        break;
      case 4: /* book moves sorted by square(learned_results + Min(results)+1). */
        for (i=0;i<nmoves;i++) {
          minv=Min(minv,book_order_learn[i]);
          maxv=Max(maxv,book_order_learn[i]);
        }
        minv=(minv < 0) ? minv : 0;
        for (i=0;i<nmoves;i++) 
          book_sort_value[i]=(book_order_learn[i]-minv+1)*(book_order_learn[i]-minv+1);
        if (minv!=0 || maxv!=0) break;
        for (i=0;i<nmoves;i++)
          book_sort_value[i]=book_order_won[i]+book_order_drawn[i]+book_order_lost[i];
        break;
      case 5: /* book moves sorted by Evaluate() */
        for (i=0;i<nmoves;i++)
          book_sort_value[i]=evaluations[i];
        break;
      case 6: /* book moves chosen completely at random. */
        for (i=0;i<nmoves;i++) 
          book_sort_value[i]=100;
        break;
      }
      total_played=total_moves;
/*
 ----------------------------------------------------------
|                                                          |
|   if there are any ! moves, make their popularity count  |
|   huge since they have to be considered.                 |
|                                                          |
 ----------------------------------------------------------
*/
      mp=0;
      for (i=0;i<nmoves;i++) mp=Max(mp,book_sort_value[i]);
      for (i=0;i<nmoves;i++)
        if (book_status[i] & 030) book_sort_value[i]=mp+1;
/*
 ----------------------------------------------------------
|                                                          |
|   now sort the moves based on the criteria chosen by     |
|   the "book random" command.                             |
|                                                          |
 ----------------------------------------------------------
*/
      if (nmoves) {
        do {
          done=1;
          for (i=0;i<nmoves-1;i++) {
            if (book_sort_value[i] < book_sort_value[i+1]) {
              tempr=book_sort_value[i];
              book_sort_value[i]=book_sort_value[i+1];
              book_sort_value[i+1]=tempr;
              temp=evaluations[i];
              evaluations[i]=evaluations[i+1];
              evaluations[i+1]=temp;
              temp=book_order_learn[i];
              book_order_learn[i]=book_order_learn[i+1];
              book_order_learn[i+1]=temp;
              temp=book_order_learn_count[i];
              book_order_learn_count[i]=book_order_learn_count[i+1];
              book_order_learn_count[i+1]=temp;
              temp=book_order_won[i];
              book_order_won[i]=book_order_won[i+1];
              book_order_won[i+1]=temp;
              temp=book_order_drawn[i];
              book_order_drawn[i]=book_order_drawn[i+1];
              book_order_drawn[i+1]=temp;
              temp=book_order_lost[i];
              book_order_lost[i]=book_order_lost[i+1];
              book_order_lost[i+1]=temp;
              temp=book_development[i];
              book_development[i]=book_development[i+1];
              book_development[i+1]=temp;
              temp=book_moves[i];
              book_moves[i]=book_moves[i+1];
              book_moves[i+1]=temp;
              temp=book_status[i];
              book_status[i]=book_status[i+1];
              book_status[i+1]=temp;
              done=0;
            }
          }
        } while (!done);
/*
 ----------------------------------------------------------
|                                                          |
|   if choosing based on frequency of play, then we want   |
|   to cull moves that were hardly played, if there is     |
|   one (or more) that was (were) played significantly     |
|   more times, meaning that moves that were not played    |
|   frequently are "suspect."                              |
|                                                          |
 ----------------------------------------------------------
*/
        if (book_random == 2) 
          for (i=1;i<nmoves;i++)
            if (book_sort_value[0] > 5*book_sort_value[i] &&
                book_sort_value[i] < 100 &&
                !(book_status[i]&030)) {
              nmoves=i;
              break;
            }
/*
 ----------------------------------------------------------
|                                                          |
|   if choosing based on learned results, we only want to  |
|   play moves that led to favorable (+) positions.        |
|                                                          |
 ----------------------------------------------------------
*/
        if (book_random == 3) 
          for (i=1;i<nmoves;i++)
            if (book_sort_value[i] <= 0) {
              nmoves=i;
              break;
            }
/*
 ----------------------------------------------------------
|                                                          |
|   display the book moves, and total counts, etc. if the  |
|   operator has requested it.                             |
|                                                          |
 ----------------------------------------------------------
*/
        if (show_book) {
          printf("  move      wins   draws  losses %%play  w/l ratio    score   learn\n");
          for (i=0;i<nmoves;i++) {
            printf("%6s", OutputMove(&book_moves[i],1,wtm));
            st=book_status[i] & book_accept_mask;
            if (st) {
              if (st & 1) printf("??");
              else if (st & 2) printf("? ");
              else if (st & 4) printf("= ");
              else if (st & 8) printf("! ");
              else if (st & 16) printf("!!");
              else {
                printf("  ");
                st=st>>5;
                printf("/");
                for (j=0;j<11;j++) {
                  if (st & 1) printf("%c",ch[j]);
                  st=st>>1;
                }
              }
            }
            else printf("  ");
            printf("  %6d  %6d  %6d",book_order_won[i], book_order_drawn[i],
                                     book_order_lost[i]);
            printf("  %3d%% ",100*(book_order_won[i]+book_order_drawn[i]+
                                   book_order_lost[i])/Max(total_moves,1));
            printf("  %5.1f  ",((float) book_order_won[i]/
                                Max(book_order_lost[i],1)));
            printf("  %s ",DisplayEvaluation(evaluations[i]));
            printf("%6d/%d",book_order_learn[i],book_order_learn_count[i]);
            if (book_development[i] < 0) printf(" (anti-thematic)");
            printf("\n");
          }
        }
/*
 ----------------------------------------------------------
|                                                          |
|   if this is a real search (not a puzzling search to     |
|   find a move by the opponent to ponder) then we need to |
|   set up the whisper info for later.                     |
|                                                          |
 ----------------------------------------------------------
*/
        if (!puzzling) {
          whisper_text[0]='\0';
          sprintf(whisper_text,"book moves (");
          whisper_p=whisper_text+strlen(whisper_text);
          for (i=0;i<nmoves;i++) {
            sprintf(whisper_p,"%s %d%%",OutputMove(&book_moves[i],1,wtm),
                                        100*(book_order_won[i]+book_order_drawn[i]+
                                             book_order_lost[i])/Max(total_played,1));
            whisper_p=whisper_text+strlen(whisper_text);
            if (book_random == 2 &&
                (book_order_won[i]+book_order_drawn[i]+book_order_lost[i])*100/
                Max(total_moves,1) == 0) break;
            if (i < nmoves-1) {
              sprintf(whisper_p,", ");
              whisper_p=whisper_text+strlen(whisper_text);
            }
          }
          sprintf(whisper_p,")\n");
          Whisper(4,0,0,0,0,0,whisper_text);
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
        if (!num_selected && !puzzling)
          if (book_accept_mask & 16)
            for (i=0;i<nmoves;i++)
              if (book_status[i] & 16) {
                forced=1;
                selected_status[num_selected]=book_status[i];
                selected_order_won[num_selected]=book_order_won[i];
                selected_order_drawn[num_selected]=book_order_drawn[i];
                selected_order_lost[num_selected]=book_order_lost[i];
                selected[num_selected++]=book_moves[i];
              }
/*
   if none, then check for ! moves
*/
        if (!num_selected && !puzzling)
          if (book_accept_mask & 8)
            for (i=0;i<nmoves;i++)
              if (book_status[i] & 8) {
                forced=1;
                selected_status[num_selected]=book_status[i];
                selected_order_won[num_selected]=book_order_won[i];
                selected_order_drawn[num_selected]=book_order_drawn[i];
                selected_order_lost[num_selected]=book_order_lost[i];
                selected[num_selected++]=book_moves[i];
              }
/*
   if none, then check for = moves
*/
        if (!num_selected && !puzzling)
          if (book_accept_mask & 4)
            for (i=0;i<nmoves;i++)
              if (book_status[i] & 4) {
                selected_status[num_selected]=book_status[i];
                selected_order_won[num_selected]=book_order_won[i];
                selected_order_drawn[num_selected]=book_order_drawn[i];
                selected_order_lost[num_selected]=book_order_lost[i];
                selected[num_selected++]=book_moves[i];
              }
/*
   if none, then check for any flagged moves
*/
        if (!num_selected && !puzzling)
          for (i=0;i<nmoves;i++)
            if (book_status[i] & book_accept_mask) {
              selected_status[num_selected]=book_status[i];
              selected_order_won[num_selected]=book_order_won[i];
              selected_order_drawn[num_selected]=book_order_drawn[i];
              selected_order_lost[num_selected]=book_order_lost[i];
              selected[num_selected++]=book_moves[i];
            }
/*
   if none, then any book move is acceptable
*/
        if (!num_selected)
          for (i=0;i<nmoves;i++) {
            selected_status[num_selected]=book_status[i];
            selected_order_won[num_selected]=book_order_won[i];
            selected_order_drawn[num_selected]=book_order_drawn[i];
            selected_order_lost[num_selected]=book_order_lost[i];
            selected[num_selected++]=book_moves[i];
          }
        if (!num_selected) return(0);
/*
   now copy moves to the right place.
*/
        for (i=0;i<num_selected;i++) {
          book_status[i]=selected_status[i];
          book_moves[i]=selected[i];
          book_order_won[i]=selected_order_won[i];
          book_order_drawn[i]=selected_order_drawn[i];
          book_order_lost[i]=selected_order_lost[i];
        }
        nmoves=num_selected;

        Print(1,"               book moves {");
        for (i=0;i<nmoves;i++) {
          Print(1,"%s", OutputMove(&book_moves[i],1,wtm));
          if (i < nmoves-1) Print(1,", ");
        }
        Print(1,"}\n");
        nmoves=Min(nmoves,book_selection_width);
        if (show_book) {
          Print(1,"               moves considered {");
          for (i=0;i<nmoves;i++) {
            Print(1,"%s", OutputMove(&book_moves[i],1,wtm));
            if (i < nmoves-1) Print(1,", ");
          }
          Print(1,"}\n");
        }
/*
 ----------------------------------------------------------
|                                                          |
|   if random=0, then we search the set of legal book      |
|   moves with the normal search engine (but with a short  |
|   time limit) to choose among them.                      |
|                                                          |
 ----------------------------------------------------------
*/
        if (book_random == 0 && !puzzling) {
          if (nmoves > 0 && !forced) {
            for (i=0;i<nmoves;i++) *(last[0]+i)=book_moves[i];
            last[1]=last[0]+nmoves;
            last_pv.path_iteration_depth=0;
            booking=1;
            value=Iterate(wtm,booking);
            booking=0;
            if (value < -800) return(0);
          }
          else {
            pv[1].path[1]=book_moves[0];
            pv[1].path_length=1;
          }
          return(1);
        }
/*
 ----------------------------------------------------------
|                                                          |
|   if puzzling, in tournament mode we try to find the     |
|   best non-book move, because a book move will produce   |
|   a quick move anyway.  we therefore would rather search |
|   for a non-book move, just in case the opponent goes    |
|   out of book here.                                      |
|                                                          |
 ----------------------------------------------------------
*/
        else if (mode==tournament_mode && puzzling) {
          RootMoveList(wtm);
          for (i=0;i<(last[1]-last[0]);i++)
            for (j=0;j<nmoves;j++)
              if (*(last[0]+i)==book_moves[j]) *(last[0]+i)=0;
          for (i=0,j=0;i<(last[1]-last[0]);i++)
            if (*(last[0]+i) != 0) *(last[0]+j++)=*(last[0]+i);
          last[1]=last[0]+j;
          Print(1,"               moves considered {only non-book moves}\n");
          nmoves=j;
          if (nmoves > 1) {
            last_pv.path_iteration_depth=0;
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
  
        if (nmoves == 0) return(0);
        last_move=nmoves;
/*
 ----------------------------------------------------------
|                                                          |
|   compute a random value and use this to generate a      |
|   book move based on a probability distribution of       |
|   the number of games won by each book move.             |
|                                                          |
 ----------------------------------------------------------
*/
        which=Random32();
        j=GetTime(microseconds)/100 % 13;
        for (i=0;i<j;i++) which=Random32();
        total_moves=0;
        if (book_random==3 || book_random==4)
          for (i=0;i<last_move;i++)
            total_moves+=(book_order_learn[i]-minv+1)*
                         (book_order_learn[i]-minv+1);
        else
          for (i=0;i<last_move;i++) total_moves+=book_order_won[i];
        distribution=abs(which) % Max(total_moves,1);
        for (which=0;which<last_move;which++) {
          if (book_random==3 || book_random==4)
            distribution-=(book_order_learn[which]-minv+1)*
                          (book_order_learn[which]-minv+1);
          else
            distribution-=book_order_won[which];
          if (distribution < 0) break;
        }
        which=Min(which,last_move-1);
        pv[1].path[1]=book_moves[which];
        percent_played=100*(book_order_won[which]+book_order_drawn[which]+
                            book_order_lost[which])/Max(total_played,1);
        total_played=book_order_won[which]+book_order_drawn[which]+
                                           book_order_lost[which];
        m1_status=book_status[which];
        pv[1].path_length=1;
        MakeMove(1,book_moves[which],wtm);
        UnMakeMove(1,book_moves[which],wtm);
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
            st=st>>5;
            Print(2,"/");
            for (j=0;j<11;j++) {
              if (st & 1) Print(2,"%c",ch[j]);
              st=st>>1;
            }
          }
        }
        Print(2,"\n");
        return(1);
      }
    }
  }
  return(0);
}

/* last modified 12/04/96 */
/*
********************************************************************************
*                                                                              *
*  "book learn" command is used to read in a book.lrn file and apply the data  *
*  contained in that file to the current book.bin file.  the intent for this   *
*  is to allow users to create a new book.bin at any time, adding more games   *
*  as needed, without losing all of the "learned" openings in the database.    *
*                                                                              *
*  the second intent is to allow users to "share" book.lrn files, and to allow *
*  me to keep several of them on the ftp machine, so that anyone can use that  *
*  file(s) and have their version of Crafty (or any other program that wants   *
*  to participate in this) "learn" what other crafty's have already found out  *
*  about which openings are good and bad.                                      *
*                                                                              *
*  the basic idea is to (a) stuff each opening line into the game history file *
*  for LearnBook(), then set things up so that LearnBook() can be called and   *
*  it will behave just as though this book line was just "learned".            *
*                                                                              *
********************************************************************************
*/
void BookLearnCMD() {

  FILE *learn_in;
  char nextc, text[128], *eof;
  int wtm, learn_value, depth, rating_difference, move, i;

/*
 ----------------------------------------------------------
|                                                          |
|   first, get the name of the file that contains the      |
|   learned book lines.                                    |
|                                                          |
 ----------------------------------------------------------
*/
  fscanf(input_stream,"%s",text);
  learn_in = fopen(text,"r");
  if (learn_in == NULL) {
    Print(0,"unable to open %s for input\n", text);
    return;
  }
/*
 ----------------------------------------------------------
|                                                          |
|   if the <clear> option was given, first we cycle thru   |
|   the entire book and clear every learned value.         |
|                                                          |
 ----------------------------------------------------------
*/
  nextc=fgetc(input_stream);
  if (nextc == ' ') fscanf(input_stream,"%s",text);
  else (strcpy(text,""));
  if (!strcmp(text,"clear")) {
    int index[32768], i, j, cluster;

    fseek(book_file,0,SEEK_SET);
    fread(index,sizeof(int),32768,book_file);
    for (i=0;i<32768;i++)
      if (index[i] > 0) {
        fseek(book_file,index[i],SEEK_SET);
        fread(&cluster,sizeof(int),1,book_file);
        fread(buffer,sizeof(BOOK_POSITION),cluster,book_file);
        for (j=0;j<cluster;j++) buffer[j].learn=037777777;
        fseek(book_file,index[i]+sizeof(int),SEEK_SET);
        fwrite(buffer,sizeof(BOOK_POSITION),cluster,book_file);
      }
  }
/*
 ----------------------------------------------------------
|                                                          |
|   outer loop loops thru the games (opening lines) one by |
|   one, while the inner loop stuffs the game history file |
|   with moves that were played.  the series of moves in a |
|   line is terminated by the {x y z} data values.         |
|                                                          |
 ----------------------------------------------------------
*/
  while (1) {
    InitializeChessBoard(&position[0]);
    wtm=0;
    move_number=0;
    for (i=0;i<3;i++) {
      eof=fgets(text,80,learn_in);
      if (eof == 0) break;
      *strchr(text,'\n')=0;
      if (strchr(text,'[')) do {
        char *bracket1, *bracket2;
        char value[32];
  
        bracket1=strchr(text,'\"');
        bracket2=strchr(bracket1+1,'\"');
        if (bracket1 == 0 || bracket2 == 0) break;
        *bracket2=0;
        strcpy(value,bracket1+1);
        if (bracket2 == 0) break;
        if (strstr(text,"Black") || strstr(text,"White")) {
          if (!strstr(value,"Crafty")) strcpy(opponents_name,value);
        }
      } while(0);
    }
    if (eof == 0) break;
    do {
      wtm=ChangeSide(wtm);
      if (wtm) move_number++;
      do {
        nextc=fgetc(learn_in);
      } while(nextc == ' ' || nextc == '\n');
      if (nextc == '{') break;
      ungetc(nextc,learn_in);
      move=ReadChessMove(learn_in,wtm);
      if (move < 0) break;
      strcpy(text,OutputMove(&move,0,wtm));
      fseek(history_file,((move_number-1)*2+1-wtm)*10,SEEK_SET);
      fprintf(history_file,"%10s ",text);
      last_move_in_book=move_number;
      MakeMoveRoot(move,wtm);
    } while (1);
    if (move < 0) break;
    wtm=ChangeSide(wtm);
    fscanf(learn_in,"%d %d %d",&learn_value, &depth, &rating_difference);
    move_number=last_move_in_book+1+LEARN_INTERVAL;
    for (i=0;i<LEARN_INTERVAL;i++) book_learn_eval[i]=learn_value;
    crafty_rating=rating_difference;
    opponent_rating=0;
    LearnBook(wtm, learn_value, depth, 1);
  }
}

/* last modified 07/20/96 */
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
*     0-A ->  11 user-defined flags.  the program will ignore these flags      *
*             unless the operator sets the "book mask" to contain them which   *
*             will "require" the program to play from the set of moves with    *
*             at least one of the flags in "book mask" set.                    *
*                                                                              *
********************************************************************************
*/
void BookUp(char *output_filename)
{
  BOOK_POSITION *buffer;
  int move, result_found=0;
  BITBOARD temp_hash_key, common;
  FILE *book_input, *output_file;
  char flags[40], fname[64], text[30], nextc, which_mask[20], *start;
  int white_won=0, black_won=0, drawn=0, i, mask_word, total_moves;
  int move_num, wtm, book_positions;
  int cluster, max_cluster, discarded, errors, data_read;
  int start_cpu_time, start_elapsed_time, following, ply, max_ply=256;
  int files, buffered=0;
  BOOK_POSITION current, next;
  int last, cluster_seek, next_cluster;
  int counter, *index, max_search_depth;
  CHESS_POSITION cp_save;
  SEARCH_POSITION sp_save;

/*
 ----------------------------------------------------------
|                                                          |
|   determine if we should read the book moves from a file |
|   or from the operator (which is normally used to add/   |
|   delete moves just before a game.)                      |
|                                                          |
 ----------------------------------------------------------
*/
  fscanf(input_stream,"%s",text);
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
    if (books_file) fclose(books_file);
    book_file=0;
    books_file=0;
    Print(0,"book file disabled.\n");
    return;
  }
  else if (!strcmp(text,"on")) {
    if (!book_file) {
      sprintf(fname,"%s/book.bin",".");
      book_file=fopen(fname,"rb+");
      sprintf(fname,"%s/books.bin",".");
      books_file=fopen(fname,"rb+");
      Print(0,"book file enabled.\n");
    }
    return;
  }
  else if (!strcmp(text,"learn")) {
    BookLearnCMD();
    return;
  }
  else if (!strcmp(text,"learning")) {
    fscanf(input_stream,"%s",text);
    if (!strcmp(text,"on")) {
      book_learning=1;
      printf("book learning enabled\n");
    }
    else if (!strcmp(text,"off")) {
      book_learning=0;
      printf("book learning disabled\n");
    }
    else {
      printf("usage: book learning on|off\n");
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
  else if (!strcmp(text,"random")) {
    data_read=fscanf(input_stream,"%d",&book_random);
    switch (book_random) {
      case 0:
        Print(0,"play best book line after search.\n");
        break;
      case 1: 
        Print(0,"choose from moves with best winning percentage\n");
        break;
      case 2:
        Print(0,"choose from moves that are played most frequently.\n");
        break;
      case 3:
        Print(0,"choose from moves that have the best learned result.\n");
        break;
      case 4:
        Print(0,"choose from all moves, favoring those with better learned results.\n");
        break;
      case 5: 
        Print(0,"choose from moves that produce the best static evaluation.\n");
        break;
      case 6:
        Print(0,"choose from book moves completely randomly.\n");
        break;
      default:
        Print(0,"valid options are 0-6.\n");
        break;
    }
    return;
  }
  else if (!strcmp(text,"width")) {
    fscanf(input_stream,"%d",&book_selection_width);
    printf("choose from %d winningest moves.\n", book_selection_width);
    return;
  }
  else {
    printf("usage:  book edit/create/off [filename]\n");
    return;
  }
  InitializeChessBoard(&position[1]);
  cp_save=search;
  sp_save=position[1];
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
  start=strstr(output_filename,"books.bin");
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
    do {
      data_read=fscanf(book_input,"%s",text);
      if (data_read == 0) printf("end-of-file reached\n");
      if (strcmp(text,"end") == 0) printf("end record read\n");
    } while ((text[0] != '[') && strcmp(text,"end") && (data_read>0));
    do {
      if (book_input != stdin) if (verbosity_level) printf("%s ", text);
      white_won=1;
      black_won=1;
      drawn=0;
      while ((text[strlen(text)-1] != ']') && 
             strcmp(text,"end") && (data_read>0)) {
        if (strstr(text,"esult")) {
          result_found=1;
          data_read=fscanf(book_input,"%s",text);
          if (data_read == 0) printf("end-of-file reached\n");
          if (strcmp(text,"end") == 0) printf("end record read\n");
          if (result_found) {
            white_won=1;
            black_won=1;
          }
          else {
            white_won=0;
            black_won=0;
          }
          drawn=0;
          if (strstr(text,"1-0")) {
            white_won=1;
            black_won=0;
          }
          else if (strstr(text,"0-1")) {
            white_won=0;
            black_won=1;
          }
          else if (strstr(text,"1/2-1/2")) {
            white_won=0;
            black_won=0;
            drawn=1;
          }
          if (strchr(text,']')) break;
        }
        data_read=fscanf(book_input,"%s",text);
        if (data_read == 0) printf("end-of-file reached\n");
        if (strcmp(text,"end") == 0) printf("end record read\n");
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
          if (wtm) printf("WhitePieces(%d): ",move_num);
          else printf("BlackPieces(%d): ",move_num);
        do {
          data_read=fscanf(book_input,"%s",text);
          if (data_read == 0) printf("end-of-file reached\n");
          if (strcmp(text,"end") == 0) printf("end record read\n");
        } while ((text[0] >= '0') && (text[00] <= '9') && (data_read>0));
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
        if (text[0] == '[') break;
        if (!strchr(text,'$') && !strchr(text,'*')) {
          move=InputMove(text,2,wtm,0,0);
          if (move) {
            ply++;
            max_search_depth=Max(max_search_depth,ply);
            total_moves++;
            if (!(total_moves % 1000)) {
              printf(".");
              if (!(total_moves % 60000)) printf(" (%d)\n",total_moves);
              fflush(stdout);
            }
            common=And(HashKey,mask_16);
            MakeMove(2,move,wtm);
            position[2]=position[3];
            if ((ply <= max_ply) || (following && (Captured(move) || Promote(move)))) {
              temp_hash_key=Xor(HashKey,wtm_random[wtm]);
              temp_hash_key=Or(And(temp_hash_key,Compl(mask_16)),common);
              buffer[buffered].position=temp_hash_key;
              buffer[buffered++].status=Or(Shiftl((BITBOARD) ((mask_word<<16)+
                                                              white_won),32),
                                          (BITBOARD) ((drawn<<16)+black_won));
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
            DisplayChessBoard(stdout,search);
            break;
          }
          wtm=ChangeSide(wtm);
          if (wtm) move_num++;
        } 
      }
      InitializeChessBoard(&position[1]);
      search=cp_save;
      position[1]=sp_save;
    } while (strcmp(text,"end") && (data_read>0));
    if (buffered) fwrite(buffer,sizeof(BOOK_POSITION),buffered,book_file);
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
    current=BookUpNextPosition(files,1);
    if (start) current.status=And(current.status,mask_32)+100;
    else current.status++;
    book_file=fopen(output_filename,"wb+");
    fseek(book_file,sizeof(int)*32768,SEEK_SET);
    last=current.position>>49;
    index[last]=ftell(book_file);
    book_positions=0;
    cluster=0;
    cluster_seek=sizeof(int)*32768;
    fseek(book_file,cluster_seek+sizeof(int),SEEK_SET);
    max_cluster=0;
    white_won=0;
    drawn=0;
    black_won=0;
    while (1) {
      next=BookUpNextPosition(files,0);
      counter++;
      if (counter%10000 == 0) {
        printf(".");
        if (counter%600000 == 0) printf("(%d)\n",counter);
        fflush(stdout);
      }
      if (current.position == next.position) {
        if (!start) current.status++;
        current.status=Or(current.status,And(next.status,mask_16));
        if (white_won < 65535)
          white_won+=Shiftr(next.status,32)&65535;
        if (drawn < 65535)
          drawn+=Shiftr(next.status,16)&65535;
        if (black_won < 65535)
          black_won+=next.status&65535;
      }
      else {
        book_positions++;
        cluster++;
        max_cluster=Max(max_cluster,cluster);
        current.status=Or(And(current.status,mask_16),
                          Shiftl((BITBOARD) white_won,32));
        current.status=Or(current.status,Shiftl((BITBOARD) drawn,16));
        current.status=Or(current.status,(BITBOARD) black_won);
        current.learn=037777777;
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
        current=next;
        white_won=Shiftr(current.status,32)&65535;
        drawn=Shiftr(current.status,16)&65535;
        black_won=current.status&65535;
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
    for (i=1;i<files;i++) {
      sprintf(fname,"sort.%d",i);
      remove(fname);
    }
    start_cpu_time=GetTime(cpu)-start_cpu_time;
    start_elapsed_time=GetTime(elapsed)-start_elapsed_time;
    Print(0,"\n\nparsed %d moves.\n",total_moves);
    Print(0,"found %d errors during parsing.\n",errors);
    Print(0,"discarded %d moves (maxply=%d).\n",discarded,max_ply);
    Print(0,"book contains %d unique positions.\n",book_positions);
    Print(0,"deepest book line was %d plies.\n",max_search_depth);
    Print(0,"longest cluster of moves was %d.\n",max_cluster);
    Print(0,"time used:  %s cpu  ", DisplayTime(start_cpu_time));
    Print(0,"  %s elapsed.\n", DisplayTime(start_elapsed_time));
  }
}

/* last modified 07/20/96 */
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

/* last modified 07/20/96 */
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
BOOK_POSITION BookUpNextPosition(int files, int init)
{
  char fname[20];
  static FILE *input_file[100];
  static BOOK_POSITION *buffer[100];
  static int data_read[100], next[100];
  int i, used;
  BOOK_POSITION least;
  if (init) {
    for (i=1;i<files;i++) {
      sprintf(fname,"sort.%d",i);
      if (!(input_file[i]=fopen(fname,"rb")))
        printf("unable to open sort.%d file, may be too many files open.\n",i);
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
