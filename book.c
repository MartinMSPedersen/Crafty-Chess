#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "chess.h"
#include "data.h"
#if defined(UNIX)
#  include <unistd.h>
#endif

/* last modified 09/16/98 */
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
*    8 bits:  flag bits defined as  follows:                                   *
*                                                                              *
*      0000 0001  ?? flagged move                (0001) (0x01)                 *
*      0000 0010   ? flagged move                (0002) (0x02)                 *
*      0000 0100   = flagged move                (0004) (0x04)                 *
*      0000 1000   ! flagged move                (0010) (0x08)                 *
*      0001 0000  !! flagged move                (0020) (0x10)                 *
*      0010 0000     black won at least 1 game   (0040) (0x20)                 *
*      0100 0000     at least one game was drawn (0100) (0x40)                 *
*      1000 0000     white won at least 1 game   (0200) (0x80)                 *
*                                                                              *
*   24 bits:  number of games this move was played.                            *
*                                                                              *
*   32 bits:  learned value (floating point).                                  *
*                                                                              *
*     (note:  counts are normalized to a max of 255.                           *
*                                                                              *
********************************************************************************
*/
#define BAD_MOVE  002
#define GOOD_MOVE 010

int Book(TREE *tree, int wtm, int root_list_done) {
  static int book_moves[200];
  static BOOK_POSITION start_moves[200];
  static int selected[200];
  static int selected_order_played[200], selected_value[200];
  static int selected_status[200], selected_percent[200], book_development[200];
  static int bs_played[200], bs_percent[200];
  static int book_status[200], evaluations[200], bs_learn[200];
  static float bs_value[200], total_value;
  int m1_status, forced=0, total_percent;
  float tempr;
  int done, i, j, last_move, temp, which, minlv=999999, maxlv=-999999;
  int maxp=-999999, minev=999999, maxev=-999999;
  int *mv, value, np;
  int cluster, scluster, test;
  BITBOARD temp_hash_key, common;
  int key, nmoves, num_selected, st;
  int percent_played, total_played, total_moves, smoves;
  int distribution;
  int initial_development;
  char *whisper_p;
/*
 ----------------------------------------------------------
|                                                          |
|   if we have been out of book for several moves, return  |
|   and start the normal tree search.                      |
|                                                          |
 ----------------------------------------------------------
*/
  if (moves_out_of_book > 3) return(0);
/*
 ----------------------------------------------------------
|                                                          |
|   position is known, read the start book file and save   |
|   each move found.  these will be used later to augment  |
|   the flags in the normal book to offer better control.  |
|                                                          |
 ----------------------------------------------------------
*/
  if (!root_list_done) RootMoveList(wtm);
  test=HashKey>>49;
  smoves=0;
  if (books_file) {
    fseek(books_file,test*sizeof(int),SEEK_SET);
    fread(&key,sizeof(int),1,books_file);
    if (key > 0) {
      fseek(books_file,key,SEEK_SET);
      fread(&scluster,sizeof(int),1,books_file);
      fread(books_buffer,sizeof(BOOK_POSITION),scluster,books_file);
      for (mv=tree->last[0];mv<tree->last[1];mv++) {
        common=And(HashKey,mask_16);
        MakeMove(tree,1,*mv,wtm);
        if (RepetitionCheck(tree,2,ChangeSide(wtm))) {
          UnMakeMove(tree,1,*mv,wtm);
          return(0);
        }
        temp_hash_key=Xor(HashKey,wtm_random[wtm]);
        temp_hash_key=Or(And(temp_hash_key,Compl(mask_16)),common);
        for (i=0;i<scluster;i++)
          if (!Xor(temp_hash_key,books_buffer[i].position)) {
            start_moves[smoves++]=books_buffer[i];
            break;
          }
        UnMakeMove(tree,1,*mv,wtm);
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
      fread(book_buffer,sizeof(BOOK_POSITION),cluster,book_file);
    }
    else cluster=0;
    if (!cluster) return(0);
/*
 ----------------------------------------------------------
|                                                          |
|   now add any moves from books.bin to the end of the     |
|   cluster so that they will be played even if not in the |
|   regular database of moves.                             |
|                                                          |
 ----------------------------------------------------------
*/
    for (i=0;i<smoves;i++) {
      for (j=0;j<cluster;j++) 
        if (!Xor(book_buffer[j].position,start_moves[i].position)) break;
      if (j >= cluster) {
        book_buffer[cluster]=start_moves[i];
        book_buffer[cluster].status_played=
          book_buffer[cluster].status_played&037700000000;
        cluster++;
      }
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
    initial_development=(wtm) ? EvaluateDevelopment(tree,1) : 
                               -EvaluateDevelopment(tree,1);
    total_moves=0;
    nmoves=0;
    for (mv=tree->last[0];mv<tree->last[1];mv++) {
      common=And(HashKey,mask_16);
      MakeMove(tree,1,*mv,wtm);
      if (RepetitionCheck(tree,2,ChangeSide(wtm))) {
        UnMakeMove(tree,1,*mv,wtm);
        return(0);
      }
      temp_hash_key=Xor(HashKey,wtm_random[wtm]);
      temp_hash_key=Or(And(temp_hash_key,Compl(mask_16)),common);
      for (i=0;i<cluster;i++) {
        if (!Xor(temp_hash_key,book_buffer[i].position)) {
          book_status[nmoves]=book_buffer[i].status_played>>24;
          bs_played[nmoves]=book_buffer[i].status_played&077777777;
          bs_learn[nmoves]=(int) (book_buffer[i].learn*100.0);
          if (puzzling) bs_played[nmoves]+=1;
          tree->current_move[1]=*mv;
          if (!Captured(*mv)) 
            book_development[nmoves]=((wtm) ? EvaluateDevelopment(tree,2) : 
                  -EvaluateDevelopment(tree,2))-initial_development;
          else book_development[nmoves]=0;
          total_moves+=bs_played[nmoves];
          evaluations[nmoves]=Evaluate(tree,2,wtm,-999999,999999);
          evaluations[nmoves]-=(wtm) ? Material : -Material;
          bs_percent[nmoves]=0;
          for (j=0;j<smoves;j++) {
            if (!Xor(book_buffer[i].position,start_moves[j].position)) {
              book_status[nmoves]|=start_moves[j].status_played>>24;
              bs_percent[nmoves]=start_moves[j].status_played&077777777;
              break;
            }
          }
          book_moves[nmoves++]=*mv;
          break;
        }
      }
      UnMakeMove(tree,1,*mv,wtm);
    }
    if (!nmoves) return(0);
/*
 ----------------------------------------------------------
|                                                          |
|   we have the book moves, now it's time to decide how    |
|   they are supposed to be sorted and compute the sort    |
|   key.                                                   |
|                                                          |
 ----------------------------------------------------------
*/
    for (i=0;i<nmoves;i++) {
      minlv=Min(minlv,bs_learn[i]);
      maxlv=Max(maxlv,bs_learn[i]);
      minev=Min(minev,evaluations[i]);
      maxev=Max(maxev,evaluations[i]);
      maxp=Max(maxp,bs_played[i]);
    }
    maxp++;
    for (i=0;i<nmoves;i++) {
      bs_value[i]=1;
      bs_value[i]+=bs_played[i]/(float) maxp*1000.0*book_weight_freq;
      if (minlv < maxlv)
        bs_value[i]+=(bs_learn[i]-minlv)/
                     (float) (Max(maxlv-minlv,50))*1000.0*book_weight_learn;
      if (minev < maxev)
        bs_value[i]+=(evaluations[i]-minev)/(float)(Max(maxev-minev,50))*
                     1000.0*book_weight_eval;
    }
    total_played=total_moves;
/*
 ----------------------------------------------------------
|                                                          |
|   if any moves have a very bad or a very good learn      |
|   value, set the appropriate ? or ! flag so the move     |
|   be played or avoided as appropriate.                   |
|                                                          |
 ----------------------------------------------------------
*/
    for (i=0;i<nmoves;i++) {
      if (bs_learn[i] <= LEARN_COUNTER_BAD && !bs_percent[i] &&
          !(book_status[i] & 030)) book_status[i]|=BAD_MOVE;
      if (wtm && !(book_status[i]&0200) && !bs_percent[i] &&
          !(book_status[i] & 030)) book_status[i]|=BAD_MOVE;
      if (!wtm && !(book_status[i]&040) && !bs_percent[i] &&
          !(book_status[i] & 030)) book_status[i]|=BAD_MOVE;
      if (bs_played[i] < maxp/10 && !bs_percent[i] && book_random &&
          !(book_status[i] & 030)) book_status[i]|=BAD_MOVE;
      if (bs_learn[i] >= LEARN_COUNTER_GOOD &&
          !(book_status[i] & 003)) book_status[i]|=GOOD_MOVE;
      if (bs_percent[i]) book_status[i]|=GOOD_MOVE;
    }
/*
 ----------------------------------------------------------
|                                                          |
|   if there are any ! moves, make their popularity count  |
|   huge since they have to be considered.                 |
|                                                          |
 ----------------------------------------------------------
*/
    for (i=0;i<nmoves;i++) 
      if (book_status[i] & 030) break;
    if (i < nmoves){
      for (i=0;i<nmoves;i++) {
        if (book_status[i] & 030) bs_value[i]+=8000.0;
        if (!(book_status[i] & 003)) bs_value[i]+=4000.0;
      }
    }
/*
 ----------------------------------------------------------
|                                                          |
|   now sort the moves based on the complete sort value.   |
|                                                          |
 ----------------------------------------------------------
*/
    if (nmoves) 
      do {
        done=1;
        for (i=0;i<nmoves-1;i++) {
          if (bs_percent[i]<bs_percent[i+1] ||
              (bs_percent[i]==bs_percent[i+1] && bs_value[i]<bs_value[i+1])) {
            tempr=bs_played[i];
            bs_played[i]=bs_played[i+1];
            bs_played[i+1]=tempr;
            tempr=bs_value[i];
            bs_value[i]=bs_value[i+1];
            bs_value[i+1]=tempr;
            temp=evaluations[i];
            evaluations[i]=evaluations[i+1];
            evaluations[i+1]=temp;
            temp=bs_learn[i];
            bs_learn[i]=bs_learn[i+1];
            bs_learn[i+1]=temp;
            temp=book_development[i];
            book_development[i]=book_development[i+1];
            book_development[i+1]=temp;
            temp=book_moves[i];
            book_moves[i]=book_moves[i+1];
            book_moves[i+1]=temp;
            temp=book_status[i];
            book_status[i]=book_status[i+1];
            book_status[i+1]=temp;
            temp=bs_percent[i];
            bs_percent[i]=bs_percent[i+1];
            bs_percent[i+1]=temp;
            done=0;
          }
        }
      } while (!done);
/*
 ----------------------------------------------------------
|                                                          |
|   display the book moves, and total counts, etc. if the  |
|   operator has requested it.                             |
|                                                          |
 ----------------------------------------------------------
*/
    if (show_book) {
      Print(128,"  after screening, the following moves can be played\n");
      Print(128,"  move     played    %%  score    learn     sortv  P%%  P\n");
      for (i=0;i<nmoves;i++) {
        Print(128,"%6s", OutputMove(tree,book_moves[i],1,wtm));
        st=book_status[i];
        if (st & 037) {
          if (st & 1) Print(128,"??");
          else if (st & 2) Print(128,"? ");
          else if (st & 4) Print(128,"= ");
          else if (st & 8) Print(128,"! ");
          else if (st & 16) Print(128,"!!");
        }
        else Print(128,"  ");
        Print(128,"   %6d",bs_played[i]);
        Print(128,"  %3d",100*bs_played[i]/Max(total_moves,1));
        Print(128,"%s",DisplayEvaluation(evaluations[i]));
        Print(128,"%9.2f",(float)bs_learn[i]/100.0);
        Print(128," %9.1f",bs_value[i]);
        Print(128," %3d",bs_percent[i]);
        if ((book_status[i]&book_accept_mask &&
             !(book_status[i]&book_reject_mask)) || 
              (!(book_status[i]&book_reject_mask) && 
              (bs_percent[i] || book_status[i]&030 ||
               (wtm && book_status[i]&0200) ||
               (!wtm && book_status[i]&040))))
          Print(128,"  Y");
        else Print(128,"  N");
        Print(128,"\n");
      }
    }
/*
 ----------------------------------------------------------
|                                                          |
|   delete ? and ?? moves first, which includes those      |
|   moves with bad learned results.                        |
|                                                          |
 ----------------------------------------------------------
*/
    num_selected=0;
    for (i=0;i<nmoves;i++)
      if (!(book_status[i] & 003) || bs_percent[i]) {
        selected_status[num_selected]=book_status[i];
        selected_order_played[num_selected]=bs_played[i];
        selected_value[num_selected]=bs_value[i];
        selected_percent[num_selected]=bs_percent[i];
        selected[num_selected++]=book_moves[i];
      }
    for (i=0;i<num_selected;i++) {
      book_status[i]=selected_status[i];
      bs_played[i]=selected_order_played[i];
      bs_value[i]=selected_value[i];
      bs_percent[i]=selected_percent[i];
      book_moves[i]=selected[i];
    }
    nmoves=num_selected;
/*
 ----------------------------------------------------------
|                                                          |
|   if this is a real search (not a puzzling search to     |
|   find a move by the opponent to ponder) then we need to |
|   set up the whisper info for later.                     |
|                                                          |
 ----------------------------------------------------------
*/
    if (!puzzling) do {
      whisper_text[0]='\0';
      if (!nmoves) break;
      sprintf(whisper_text,"book moves (");
      whisper_p=whisper_text+strlen(whisper_text);
      for (i=0;i<nmoves;i++) {
        sprintf(whisper_p,"%s %d%%",OutputMove(tree,book_moves[i],1,wtm),
                                    100*bs_played[i]/Max(total_played,1));
        whisper_p=whisper_text+strlen(whisper_text);
        if (i < nmoves-1) {
          sprintf(whisper_p,", ");
          whisper_p=whisper_text+strlen(whisper_text);
        }
      }
      sprintf(whisper_p,")\n");
    } while(0);
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
            selected_order_played[num_selected]=bs_played[i];
            selected_value[num_selected]=bs_value[i];
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
            selected_order_played[num_selected]=bs_played[i];
            selected_value[num_selected]=bs_value[i];
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
            selected_order_played[num_selected]=bs_played[i];
            selected_value[num_selected]=bs_value[i];
            selected[num_selected++]=book_moves[i];
          }
/*
   if none, then check for any flagged moves
*/
    if (!num_selected && !puzzling)
      for (i=0;i<nmoves;i++)
        if (book_status[i] & book_accept_mask) {
          selected_status[num_selected]=book_status[i];
          selected_order_played[num_selected]=bs_played[i];
          selected_value[num_selected]=bs_value[i];
          selected[num_selected++]=book_moves[i];
        }
/*
   if none, then any book move is acceptable
*/
    if (!num_selected)
      for (i=0;i<nmoves;i++) {
        selected_status[num_selected]=book_status[i];
        selected_order_played[num_selected]=bs_played[i];
        selected_value[num_selected]=bs_value[i];
        selected[num_selected++]=book_moves[i];
      }
    if (!num_selected) return(0);
/*
   now copy moves to the right place.
*/
    for (i=0;i<num_selected;i++) {
      book_status[i]=selected_status[i];
      book_moves[i]=selected[i];
      bs_played[i]=selected_order_played[i];
      bs_value[i]=selected_value[i];
    }
    nmoves=num_selected;
    if (nmoves == 0) return(0);

    Print(128,"               book moves {");
    for (i=0;i<nmoves;i++) {
      Print(128,"%s", OutputMove(tree,book_moves[i],1,wtm));
      if (i < nmoves-1) Print(128,", ");
    }
    Print(128,"}\n");
    nmoves=Min(nmoves,book_selection_width);
    if (show_book) {
      Print(128,"               moves considered {");
      for (i=0;i<nmoves;i++) {
        Print(128,"%s", OutputMove(tree,book_moves[i],1,wtm));
        if (i < nmoves-1) Print(128,", ");
      }
      Print(128,"}\n");
    }
/*
 ----------------------------------------------------------
|                                                          |
|   we have the book moves, if any have specified percents |
|   for play, then adjust the bs_value[] to reflect this   |
|   percentage.                                            |
|                                                          |
 ----------------------------------------------------------
*/
    total_value=0.0;
    total_percent=0;
    for (i=0;i<nmoves;i++) {
      if (!bs_percent[i]) total_value+=bs_value[i];
      total_percent+=bs_percent[i];
    }
    if (total_value == 0.0) total_value=1000.0;
    total_percent=(total_percent>99) ? 99 : total_percent;
    for (i=0;i<nmoves;i++) 
      if (bs_percent[i])
        bs_value[i]=total_value/(1.0-(float)total_percent/100.0)*
                                     (float)bs_percent[i]/100.0;
/*
 ----------------------------------------------------------
|                                                          |
|   display the book moves, and total counts, etc. if the  |
|   operator has requested it.                             |
|                                                          |
 ----------------------------------------------------------
*/
    if (show_book) {
      Print(128,"  move     played    %%  score    learn     sortv  P%%  P\n");
      for (i=0;i<nmoves;i++) {
        Print(128,"%6s", OutputMove(tree,book_moves[i],1,wtm));
        st=book_status[i];
        if (st & 037) {
          if (st & 1) Print(128,"??");
          else if (st & 2) Print(128,"? ");
          else if (st & 4) Print(128,"= ");
          else if (st & 8) Print(128,"! ");
          else if (st & 16) Print(128,"!!");
        }
        else Print(128,"  ");
        Print(128,"   %6d",bs_played[i]);
        Print(128,"  %3d",100*bs_played[i]/Max(total_moves,1));
        Print(128,"%s",DisplayEvaluation(evaluations[i]));
        Print(128,"%9.2f",(float)bs_learn[i]/100.0);
        Print(128," %9.1f",bs_value[i]);
        Print(128," %3d",bs_percent[i]);
        if ((book_status[i]&book_accept_mask &&
             !(book_status[i]&book_reject_mask)) || 
              (!(book_status[i]&book_reject_mask) && 
              ((wtm && book_status[i]&0200) ||
               (!wtm && book_status[i]&040))))
          Print(128,"  Y");
        else Print(128,"  N");
        Print(128,"\n");
      }
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
    if (nmoves && (!puzzling || mode!=tournament_mode)) {
      np=bs_played[nmoves-1];
      if (!puzzling && (!book_random ||
                       (mode==tournament_mode && np<book_search_trigger))) {
        if (!forced) {
          for (i=0;i<nmoves;i++) *(tree->last[0]+i)=book_moves[i];
          tree->last[1]=tree->last[0]+nmoves;
          last_pv.pathd=0;
          booking=1;
          value=Iterate(wtm,booking,1);
          booking=0;
          if (value <- 50) {
            last_pv.pathd=0;
            return(0);
          }
        }
        else {
          tree->pv[1].path[1]=book_moves[0];
          tree->pv[1].pathl=1;
        }
        return(1);
      }
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
    else if (mode==tournament_mode && puzzling && !auto232) {
      RootMoveList(wtm);
      for (i=0;i<(tree->last[1]-tree->last[0]);i++)
        for (j=0;j<nmoves;j++)
          if (*(tree->last[0]+i)==book_moves[j]) *(tree->last[0]+i)=0;
      for (i=0,j=0;i<(tree->last[1]-tree->last[0]);i++)
        if (*(tree->last[0]+i) != 0) *(tree->last[0]+j++)=*(tree->last[0]+i);
      tree->last[1]=tree->last[0]+j;
      Print(128,"               moves considered {only non-book moves}\n");
      nmoves=j;
      if (nmoves > 1) {
        last_pv.pathd=0;
        booking=1;
        (void) Iterate(wtm,booking,1);
        booking=0;
      }
      else {
        tree->pv[1].path[1]=book_moves[0];
        tree->pv[1].pathl=1;
      }
      return(1);
    }
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
    j=ReadClock(microseconds)/100 % 13;
    for (i=0;i<j;i++) which=Random32();
    total_moves=0;
    for (i=0;i<last_move;i++) {
      if (bs_percent[0]) total_moves+=bs_value[i];
      else total_moves+=bs_value[i]*bs_value[i];
    }
    distribution=abs(which) % Max(total_moves,1);
    for (which=0;which<last_move;which++) {
      if (bs_percent[0]) distribution-=bs_value[which];
      else distribution-=bs_value[which]*bs_value[which];
      if (distribution < 0) break;
    }
    which=Min(which,last_move-1);
    tree->pv[1].path[1]=book_moves[which];
    percent_played=100*bs_played[which]/Max(total_played,1);
    total_played=bs_played[which];
    m1_status=book_status[which];
    tree->pv[1].pathl=1;
    MakeMove(tree,1,book_moves[which],wtm);
    UnMakeMove(tree,1,book_moves[which],wtm);
    Print(128,"               book   0.0s    %3d%%   ", percent_played);
    Print(128," %s",OutputMove(tree,tree->pv[1].path[1],1,wtm));
    st=m1_status & book_accept_mask & (~224);
    if (st) {
      if (st & 1) Print(128,"??");
      else if (st & 2) Print(128,"?");
      else if (st & 4) Print(128,"=");
      else if (st & 8) Print(128,"!");
      else if (st & 16) Print(128,"!!");
    }
    Print(128,"\n");
    return(1);
  }
  return(0);
}

/* last modified 08/22/98 */
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
*   characters can be appended to a move.                                      *
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
*                                                                              *
*  {play nn%} is used to force this specific book move to be played a specific *
*             percentage of the time, and override the frequency of play that  *
*             comes from the large pgn database.                               *
*                                                                              *
********************************************************************************
*/
void BookUp(TREE *tree, char *output_filename, int nargs, char **args) {
  BB_POSITION *bbuffer;
  BITBOARD temp_hash_key, common;
  FILE *book_input;
  char fname[64], *start, *ch;
  static char schar[2]={"."};
  int result=0, played, i, mask_word, total_moves;
  int move, move_num, wtm, book_positions, major, minor;
  int cluster, max_cluster, discarded=0, discarded_mp=0, discarded_lose=0;
  int errors, data_read;
  int start_cpu_time, start_elapsed_time, ply, max_ply=256;
  int stat, files=0, buffered=0, min_played=0, games_parsed=0;
  int wins, losses;
  BOOK_POSITION current, next;
  BB_POSITION temp;
  int last, cluster_seek, next_cluster;
  int counter, *index, max_search_depth;
  POSITION cp_save;
  SEARCH_POSITION sp_save;
  double wl_percent=0.0;

/*
 ----------------------------------------------------------
|                                                          |
|   open the correct book file for writing/reading         |
|                                                          |
 ----------------------------------------------------------
*/
  if (!strcmp(args[0],"create")) {
    if (nargs < 3) {
      Print(4095,"usage:  book|books create filename ");
      Print(4095,"maxply [minplay] [win/lose %]\n");
      return;
    }
    max_ply=atoi(args[2]);
    if (nargs >= 4) {
      min_played=atoi(args[3]);
    }
    if (nargs > 4) {
      wl_percent=atof(args[4])/100.0;
    }
  }
  else if (!strcmp(args[0],"off")) {
    if (book_file) fclose(book_file);
    if (books_file) fclose(books_file);
    book_file=0;
    books_file=0;
    Print(4095,"book file disabled.\n");
    return;
  }
  else if (!strcmp(args[0],"on")) {
    if (!book_file) {
#if defined (MACOS)
      sprintf(fname,":%s:book.bin",book_path);
      book_file=fopen(fname,"rb+");
      sprintf(fname,":%s:books.bin",book_path);
      books_file=fopen(fname,"rb+");
#else
      sprintf(fname,"%s/book.bin",book_path);
      book_file=fopen(fname,"rb+");
      sprintf(fname,"%s/books.bin",book_path);
      books_file=fopen(fname,"rb+");
#endif
      Print(4095,"book file enabled.\n");
    }
    return;
  }
  else if (!strcmp(args[0],"mask")) {
    if (nargs < 3) {
      Print(4095,"usage:  book mask accept|reject value\n");
      return;
    }
    else if (!strcmp(args[1],"accept")) {
      book_accept_mask=BookMask(args[2]);
      book_reject_mask=book_reject_mask & ~book_accept_mask;
    }
    else if (!strcmp(args[1],"reject")) {
      book_reject_mask=BookMask(args[2]);
      book_accept_mask=book_accept_mask & ~book_reject_mask;
    }
  }
  else if (!strcmp(args[0],"random")) {
    if (nargs < 2) {
      Print(4095,"usage:  book random <n>\n");
      return;
    }
    book_random=atoi(args[1]);
    switch (book_random) {
      case 0:
        Print(4095,"play best book line after search.\n");
        break;
      case 1: 
        Print(4095,"choose from book moves randomly (using weights.)\n");
        break;
      default:
        Print(4095,"valid options are 0-1.\n");
        break;
    }
    return;
  }
  else if (!strcmp(args[0],"trigger")) {
    if (nargs < 2) {
      Print(4095,"usage:  book trigger <n>\n");
      return;
    }
    book_search_trigger=atoi(args[1]);
    Print(4095,"search book moves if the most popular was not played\n");
    Print(4095,"at least %d times.\n", book_search_trigger);
    return;
  }
  else if (!strcmp(args[0],"width")) {
    if (nargs < 2) {
      Print(4095,"usage:  book width <n>\n");
      return;
    }
    book_selection_width=atoi(args[1]);
    Print(4095,"choose from %d best moves.\n", book_selection_width);
    return;
  }
  else {
    Print(4095,"usage:  book [option] [filename] [maxply] [minplay]\n");
    return;
  }
  if (!(book_input=fopen(args[1],"r"))) {
    printf("file %s does not exist.\n",args[1]);
    return;
  }
  InitializeChessBoard(&tree->position[1]);
  cp_save=tree->pos;
  sp_save=tree->position[1];
  if (book_file) fclose(book_file);
  book_file=fopen(output_filename,"wb+");
  bbuffer=(BB_POSITION *) malloc(sizeof(BB_POSITION)*SORT_BLOCK);
  if (!bbuffer) {
    Print(4095,"Unable to malloc() sort buffer, aborting\n");
    exit(1);
  }
  fseek(book_file,0,SEEK_SET);
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
  printf("parsing pgn move file (10000 moves/dot)\n");
  start_cpu_time=ReadClock(cpu);
  start_elapsed_time=ReadClock(elapsed);
  if (book_file) {
    total_moves=0;
    max_search_depth=0;
    errors=0;
    do {
      data_read=ReadPGN(book_input,0);
      if (data_read == -1) Print(4095,"end-of-file reached\n");
    } while (data_read == 0);
    do {
      if (data_read < 0) {
        Print(4095,"end-of-file reached\n");
        break;
      }
      if (data_read == 1) {
        if (strstr(buffer,"Site")) {
          games_parsed++;
          result=3;
        }
        else if (strstr(buffer,"esult")) {
          if (strstr(buffer,"1-0")) result=2;
          else if (strstr(buffer,"0-1")) result=1;
          else if (strstr(buffer,"1/2-1/2")) result=0;
          else if (strstr(buffer,"*")) result=3;
        }
        data_read=ReadPGN(book_input,0);
      }
      else do {
        wtm=1;
        move_num=1;
        tree->position[2]=tree->position[1];
        ply=0;
        data_read=0;
        while (data_read==0) {
          mask_word=0;
          if ((ch=strpbrk(buffer,"?!"))) {
            mask_word=BookMask(ch);
            *ch=0;
          }
          if (!strchr(buffer,'$') && !strchr(buffer,'*')) {
            if (ply < max_ply)
              move=ReadNextMove(tree,buffer,2,wtm);
            else move=0;
            if (move) {
              ply++;
              max_search_depth=Max(max_search_depth,ply);
              total_moves++;
              common=And(HashKey,mask_16);
              MakeMove(tree,2,move,wtm);
              tree->position[2]=tree->position[3];
              if (ply <= max_ply) {
                temp_hash_key=Xor(HashKey,wtm_random[wtm]);
                temp_hash_key=Or(And(temp_hash_key,Compl(mask_16)),common);
                memcpy(bbuffer[buffered].position,(char*)&temp_hash_key,8);
                if (result == 0) mask_word|=0100;
                else if (result&2) mask_word|=0200;
                else if (result&1) mask_word|=040;
                bbuffer[buffered].status=mask_word;
                bbuffer[buffered++].percent_play=pgn_suggested_percent+(wtm<<7);
                if (buffered >= SORT_BLOCK) {
                  BookSort(bbuffer,buffered,++files);
                  buffered=0;
                  strcpy(schar,"S");
                }
              }
              else {
                discarded++;
              }
              if (!(total_moves % 10000)) {
                printf(schar);
                strcpy(schar,".");
                if (!(total_moves % 600000)) printf(" (%d)\n",total_moves);
                fflush(stdout);
              }
              wtm=ChangeSide(wtm);
              if (wtm) move_num++;
            }
            else if (strspn(buffer,"0123456789/-.*") != strlen(buffer) &&
                     ply < max_ply) {
              errors++;
              Print(4095,"ERROR!  move %d: %s is illegal (line %d)\n",
                    move_num,buffer,ReadPGN(book_input,-2));
              ReadPGN(book_input,-1);
              DisplayChessBoard(stdout,tree->pos);
              do {
                data_read=ReadPGN(book_input,0);
                if (data_read == -1) Print(4095,"end-of-file reached\n");
              } while (data_read == 0);
              break;
            }
          }
          data_read=ReadPGN(book_input,0);
        }
      } while(0);
      InitializeChessBoard(&tree->position[1]);
      tree->pos=cp_save;
      tree->position[1]=sp_save;
    } while (strcmp(buffer,"end") && data_read!=-1);
    if (book_input != stdin) fclose(book_input);
    if (buffered) BookSort(bbuffer,buffered,++files);
    free(bbuffer);
    printf("S  <done>\n");
/*
 ----------------------------------------------------------
|                                                          |
|   now merge these "chunks" into book.bin, keeping all of |
|   the "flags" as well as counting the number of times    |
|   that each move was played.                             |
|                                                          |
 ----------------------------------------------------------
*/
    printf("merging sorted files (%d) (10K/dot)\n",files);
    counter=0;
    index=(int *) malloc(32768*sizeof(int));
    if (!index) {
      Print(4095,"Unable to malloc() index block, aborting\n");
      exit(1);
    }
    for (i=0;i<32768;i++) index[i]=-1;
    temp=BookUpNextPosition(files,1);
    memcpy((char*)&current.position,temp.position,8);
    current.status_played=temp.status<<24;
    if (start) current.status_played+=temp.percent_play;
    current.learn=0.0;
    played=1;
    fclose(book_file);
    book_file=fopen(output_filename,"wb+");
    fseek(book_file,sizeof(int)*32768,SEEK_SET);
    last=current.position>>49;
    index[last]=ftell(book_file);
    book_positions=0;
    cluster=0;
    cluster_seek=sizeof(int)*32768;
    fseek(book_file,cluster_seek+sizeof(int),SEEK_SET);
    max_cluster=0;
    wins=0;
    losses=0;
    if (temp.status&128  && temp.percent_play&128) wins++;
    if (temp.status&128  && !(temp.percent_play&128)) losses++;
    if (temp.status&32  && !(temp.percent_play&128)) wins++;
    if (temp.status&32  && temp.percent_play&128) losses++;
    while (1) {
      temp=BookUpNextPosition(files,0);
      memcpy((char*)&next.position,temp.position,8);
      next.status_played=temp.status<<24;
      if (start) next.status_played+=temp.percent_play&127;
      next.learn=0.0;
      counter++;
      if (counter%10000 == 0) {
        printf(".");
        if (counter%600000 == 0) printf(" (%d)\n",counter);
        fflush(stdout);
      }
      if (current.position == next.position) {
        current.status_played=current.status_played|next.status_played;
        played++;
        if (temp.status&128  && temp.percent_play&128) wins++;
        if (temp.status&128  && !(temp.percent_play&128)) losses++;
        if (temp.status&32  && !(temp.percent_play&128)) wins++;
        if (temp.status&32  && temp.percent_play&128) losses++;
      }
      else {
        if (played>=min_played && wins>=(losses*wl_percent)) {
          book_positions++;
          cluster++;
          max_cluster=Max(max_cluster,cluster);
          if (!start) current.status_played+=played;
          current.learn=0.0;
          stat=fwrite(&current,sizeof(BOOK_POSITION),1,book_file);
          if (stat != 1)
            Print(4095,"ERROR!  write failed, disk probably full.\n");
        }
        else if (played < min_played) discarded_mp++;
        else discarded_lose++;
        if (last != (int) (next.position>>49)) {
          next_cluster=ftell(book_file);
          fseek(book_file,cluster_seek,SEEK_SET);
          stat=fwrite(&cluster,sizeof(int),1,book_file);
          if (stat != 1)
            Print(4095,"ERROR!  write failed, disk probably full.\n");
          if (next.position == 0) break;
          fseek(book_file,next_cluster+sizeof(int),SEEK_SET);
          cluster_seek=next_cluster;
          last=next.position>>49;
          index[last]=next_cluster;
          cluster=0;
        }
        wins=0;
        losses=0;
        if (temp.status&128  && temp.percent_play&128) wins++;
        if (temp.status&128  && !(temp.percent_play&128)) losses++;
        if (temp.status&32  && !(temp.percent_play&128)) wins++;
        if (temp.status&32  && temp.percent_play&128) losses++;
        current=next;
        played=1;
        if (next.position == 0) break;
      }
    }
    fseek(book_file,0,SEEK_SET);
    fwrite(index,sizeof(int),32768,book_file);
    fseek(book_file,0,SEEK_END);
    major=atoi(version);
    minor=atoi(strchr(version,'.')+1);
    major=(major<<16)+minor;
    fwrite(&major,sizeof(int),1,book_file);
/*
 ----------------------------------------------------------
|                                                          |
|   now clean up, remove the sort.n files, and print the   |
|   statistics for building the book.                      |
|                                                          |
 ----------------------------------------------------------
*/
    for (i=1;i<=files;i++) {
      sprintf(fname,"sort.%d",i);
      remove(fname);
    }
    free(index);
    start_cpu_time=ReadClock(cpu)-start_cpu_time;
    start_elapsed_time=ReadClock(elapsed)-start_elapsed_time;
    Print(4095,"\n\nparsed %d moves (%d games).\n",total_moves,games_parsed);
    Print(4095,"found %d errors during parsing.\n",errors);
    Print(4095,"discarded %d moves (maxply=%d).\n",discarded,max_ply);
    Print(4095,"discarded %d moves (minplayed=%d).\n",discarded_mp,min_played);
    Print(4095,"discarded %d moves (win/lose=%.1f%%).\n",discarded_lose,
          wl_percent*100);
    Print(4095,"book contains %d unique positions.\n",book_positions);
    Print(4095,"deepest book line was %d plies.\n",max_search_depth);
    Print(4095,"longest cluster of moves was %d.\n",max_cluster);
    Print(4095,"time used:  %s cpu  ", DisplayTime(start_cpu_time));
    Print(4095,"  %s elapsed.\n", DisplayTime(start_elapsed_time));
  }
}

/* last modified 06/18/98 */
/*
********************************************************************************
*                                                                              *
*   BookMask() is used to convert the flags for a book move into an 8 bit      *
*   mask that is either kept in the file, or is set by the operator to select  *
*   which opening(s) the program is allowed to play.                           *
*                                                                              *
********************************************************************************
*/
int BookMask(char *flags) {
  int i, mask;
  mask=0;
  for (i=0;i<(int) strlen(flags);i++) {
    if (flags[i] == '?') {
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
  }
  return(mask);
}


/* last modified 06/19/98 */
/*
********************************************************************************
*                                                                              *
*   BookSort() is called to sort a block of moves after they have been parsed  *
*   and converted to hash keys.                                                *
*                                                                              *
********************************************************************************
*/
void BookSort(BB_POSITION *buffer, int number, int fileno) {
  char fname[16];
  FILE *output_file;
  int stat;

  qsort((char *) buffer,number,sizeof(BB_POSITION),BookUpCompare);
  sprintf(fname,"sort.%d",fileno);
  if(!(output_file=fopen(fname,"wb+"))) 
    printf("ERROR.  unable to open sort output file\n");
  stat=fwrite(buffer,sizeof(BB_POSITION),number,output_file);
  if (stat != number)
    Print(4095,"ERROR!  write failed, disk probably full.\n");
  fclose(output_file);
}

/* last modified 06/19/98 */
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
BB_POSITION BookUpNextPosition(int files, int init) {
  char fname[20];
  static FILE *input_file[100];
  static BB_POSITION *buffer[100];
  static int data_read[100], next[100];
  int i, used;
  BB_POSITION least;
  if (init) {
    for (i=1;i<=files;i++) {
      sprintf(fname,"sort.%d",i);
      if (!(input_file[i]=fopen(fname,"rb"))) {
        printf("unable to open sort.%d file, may be too many files open.\n",i);
        exit(1);
      }
      buffer[i]=(BB_POSITION *) malloc(sizeof(BB_POSITION)*MERGE_BLOCK);
      if (!buffer[i]) {
        printf("out of memory.  aborting. \n");
        exit(1);
      }
      fseek(input_file[i],0,SEEK_SET);
      data_read[i]=fread(buffer[i],sizeof(BB_POSITION),MERGE_BLOCK,
                         input_file[i]);
      next[i]=0;
    }
  }
  for (i=0;i<8;i++) least.position[i]=0;
  used=-1;
  for (i=1;i<=files;i++)
    if (data_read[i]) {
      least=buffer[i][next[i]];
      used=i;
      break;
    }
  if (i > files) {
    for (i=1;i<=files;i++) fclose(input_file[i]);
    return(least);
  }
  for (i++;i<=files;i++) {
    if (data_read[i]) {
      BITBOARD p1, p2;
      memcpy((char*)&p1,least.position,8);
      memcpy((char*)&p2,buffer[i][next[i]].position,8);
      if (p1 > p2) {
        least=buffer[i][next[i]];
        used=i;
      }
    }
  }
  if (--data_read[used] == 0) {
    data_read[used]=fread(buffer[used],sizeof(BB_POSITION),
                          MERGE_BLOCK,input_file[used]);
    next[used]=0;
  }
  else
    next[used]++;
  return(least);
}

#if defined(NT_i386)
int _cdecl BookUpCompare(const void *pos1, const void *pos2) {
#else
int BookUpCompare(const void *pos1, const void *pos2) {
#endif
  static BITBOARD p1, p2;
  memcpy((char*)&p1,((BB_POSITION *)pos1)->position,8);
  memcpy((char*)&p2,((BB_POSITION *)pos2)->position,8);
  if (p1 < p2) return(-1);
  if (p1 > p2) return(+1);
  return(0);
}
