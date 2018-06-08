#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include "chess.h"
#include "data.h"
#if defined(UNIX)
#  include <unistd.h>
#endif

/* last modified 12/04/96 */
/*
********************************************************************************
*                                                                              *
*   LearnBook() is used to accumulate the evaluations for the first N moves    *
*   out of book.  after these moves have been played, the evaluations are then *
*   used to decide whether the last book move played was a reasonable choice   *
*   or not.  (N is set by the #define LEARN_INTERVAL definition.)              *
*                                                                              *
*   there are three cases to be handled.  (1) if the evaluation is bad right   *
*   out of book, or it drops enough to be considered a bad line, then the book *
*   move will have its "learn" value reduced to discourage playing this move   *
*   again.  (2) if the evaluation is even after N moves, then the learn        *
*   value will be increased, but by a relatively modest amount, so that a few  *
*   even results will offset one bad result.  (3) if the evaluation is very    *
*   good after N moves, the learn value will be increased by a large amount    *
*   so that this move will be favored the next time the game is played.        *
*                                                                              *
********************************************************************************
*/

void LearnBook(int wtm, int search_value, int search_depth, int lv)
{
/*
 ----------------------------------------------------------
|                                                          |
|   if we have not been "out of book" for N moves, all     |
|   we need to do is take the search evaluation for the    |
|   search just completed and tuck it away in the book     |
|   learning array (book_learn_eval[]) for use later.      |
|                                                          |
 ----------------------------------------------------------
*/
  if (move_number-last_move_in_book-1 < LEARN_INTERVAL) {
    book_learn_eval[move_number-last_move_in_book-1]=search_value;
  }
/*
 ----------------------------------------------------------
|                                                          |
|   check the evaluations we've seen so far.  if they are  |
|   within reason (+/- 1/3 of a pawn or so) we simply keep |
|   playing and leave the book alone.  if the eval is much |
|   better or worse, we need to update the learning count. |
|                                                          |
 ----------------------------------------------------------
*/
  else if (move_number-last_move_in_book-1 == LEARN_INTERVAL) {
    int nmoves, move, movenum, i, learn_value;
    int secs, temp_value, twtm;
    char cmd[32], buff[80], *nextc;
    int best_eval=-999999, best_eval_p=0;
    int worst_eval=999999, worst_eval_p=0;
    int best_after_worst_eval=-999999, worst_after_best_eval=999999;
    struct tm *timestruct;

    for (i=0;i<LEARN_INTERVAL;i++) {
      if (book_learn_eval[i] > best_eval) {
        best_eval=book_learn_eval[i];
        best_eval_p=i;
      }
      if (book_learn_eval[i] < worst_eval) {
        worst_eval=book_learn_eval[i];
        worst_eval_p=i;
      }
    }
    if (best_eval_p < LEARN_INTERVAL-1) {
      for (i=best_eval_p;i<LEARN_INTERVAL;i++)
        if (book_learn_eval[i] < worst_after_best_eval)
          worst_after_best_eval=book_learn_eval[i];
    }
    else worst_after_best_eval=book_learn_eval[LEARN_INTERVAL-1];

    if (worst_eval_p < LEARN_INTERVAL-1) {
      for (i=worst_eval_p;i<LEARN_INTERVAL;i++)
        if (book_learn_eval[i] > best_after_worst_eval)
          best_after_worst_eval=book_learn_eval[i];
    }
    else best_after_worst_eval=book_learn_eval[LEARN_INTERVAL-1];

    Print(1,"Learning analysis ...\n");
    Print(1,"worst=%d  best=%d  baw=%d  wab=%d\n",
          worst_eval, best_eval, best_after_worst_eval, worst_after_best_eval);
    for (i=0;i<LEARN_INTERVAL;i++)
      Print(1,"%d ",book_learn_eval[i]);
    Print(1,"\n");

/*
 ----------------------------------------------------------
|                                                          |
|   we now have the best eval for the first N moves out    |
|   of book, the worst eval for the first N moves out of   |
|   book, and the worst eval that follows the best eval.   |
|   this will be used to recognize the following cases of  |
|   results that follow a book move:                       |
|                                                          |
|   (1) the best score is very good, and it doesn't drop   |
|   after following the game further.  this case detects   |
|   those moves in book that are "killers" and should be   |
|   played whenever possible.                              |
|                                                          |
|   (2) the worst score is bad, and doesn't improve any    |
|   after the worst point, indicating that the book move   |
|   chosen was a "bummer" and should be avoided in the     |
|   future.                                                |
|                                                          |
|   (3) things seem even out of book and remain that way   |
|   for N moves.  no learning is needed.                   |
|                                                          |
 ----------------------------------------------------------
*/
/*
 -------------------------------------------
|                                           |
|   case (1)                                |
|                                           |
 -------------------------------------------
*/
    if (best_eval >= LEARN_WINDOW_UB &&
        worst_after_best_eval >= LEARN_WINDOW_UB)
      learn_value=Min(best_eval,worst_after_best_eval);
/*
 -------------------------------------------
|                                           |
|   case (2)                                |
|                                           |
 -------------------------------------------
*/
    else if (worst_eval <= LEARN_WINDOW_LB &&
             (best_after_worst_eval <= LEARN_WINDOW_LB ||
              book_learn_eval[LEARN_INTERVAL-1] < LEARN_WINDOW_LB))
      learn_value=Min(Max(worst_eval,best_after_worst_eval),
                      book_learn_eval[LEARN_INTERVAL-1]);
/*
 -------------------------------------------
|                                           |
|   case (3)                                |
|                                           |
 -------------------------------------------
*/
    else if (lv) learn_value=search_value;
    else learn_value=0;
    if (learn_value == 0) return;
    if (lv == 0)
      learn_value=LearnFunction(learn_value,search_depth,
                                crafty_rating-opponent_rating);
    else
      learn_value=search_value;
/*
 ----------------------------------------------------------
|                                                          |
|   reset the board position to what it was at the point   |
|   of the last book move, so that we can locate the right |
|   position in the book database.                         |
|                                                          |
 ----------------------------------------------------------
*/
    nmoves=(last_move_in_book)*2-wtm;
    twtm=1;
    movenum=1;
    InitializeChessBoard(&position[0]);
    for (i=0;i<nmoves;i++) {
      fseek(history_file,i*10,SEEK_SET);
      fscanf(history_file,"%s",cmd);
      move=InputMove(cmd,0,twtm,1,0);
/*
 ----------------------------------------------------------
|                                                          |
|   now call LearnBookUpdate() to find this position in    |
|   the book database and update the learn count.  note    |
|   that if this function returns(0), it updated the last  |
|   book move to a noplay status, so that we need to back  |
|   up two plies and update there, and so forth until we   |
|   reach a book position where there will still be a      |
|   playable move left.  this propogates bad lines back up |
|   as all alternatives are tried.                         |
|                                                          |
 ----------------------------------------------------------
*/
      temp_value=learn_value*(i+1)/nmoves;
      if (wtm != twtm) temp_value*=-1;
      LearnBookUpdate(twtm, move, temp_value, search_depth);
      if (move) MakeMoveRoot(move,twtm);
      twtm=ChangeSide(twtm);
    } 
/*
 ----------------------------------------------------------
|                                                          |
|   now update the "book.lrn" file so that this can be     |
|   shared with other crafty users or else saved in case.  |
|   the book must be re-built.                             |
|                                                          |
 ----------------------------------------------------------
*/
      if (wtm) {
        fprintf(learn_file,"[White \"Crafty v%s\"]\n",version);
        fprintf(learn_file,"[Black \"%s\"]\n",opponents_name);
      }
      else {
        fprintf(learn_file,"[White \"%s\"]\n",opponents_name);
        fprintf(learn_file,"[Black \"Crafty v%s\"]\n",version);
      }
      secs=time(0);
      timestruct=localtime((time_t*) &secs);
      fprintf(learn_file,"[Date \"%4d.%02d.%02d\"]\n",timestruct->tm_year+1900,
              timestruct->tm_mon+1,timestruct->tm_mday);
      nmoves=(last_move_in_book-1)*2+1-wtm;
      nextc=buff;
      for (i=0;i<nmoves+1;i++) {
        fseek(history_file,i*10,SEEK_SET);
        fscanf(history_file,"%s",cmd);
        if (strchr(cmd,' ')) *strchr(cmd,' ')=0;
        sprintf(nextc," %s",cmd);
        nextc=buff+strlen(buff);
        if (nextc-buff > 60) {
          fprintf(learn_file,"%s\n",buff);
          nextc=buff;
          strcpy(buff,"");
        }
      }
      fprintf(learn_file,"%s {%d %d %d}\n",buff, learn_value, search_depth, 0);
      fflush(learn_file);
/*
 ----------------------------------------------------------
|                                                          |
|   done.  now use the reset <n> command to reset the      |
|   position to what it was when we started this process.  |
|                                                          |
 ----------------------------------------------------------
*/
    nmoves=(move_number-1)*2+1-wtm;
    wtm=1;
    movenum=1;
    InitializeChessBoard(&position[0]);
    for (i=0;i<nmoves;i++) {
      fseek(history_file,i*10,SEEK_SET);
      fscanf(history_file,"%s",cmd);
      move=InputMove(cmd,0,wtm,1,0);
      if (move) MakeMoveRoot(move,wtm);
      wtm=ChangeSide(wtm);
      if (wtm) movenum++;
    } 
    Phase();
  }
}

/* last modified 11/24/96 */
/*
********************************************************************************
*                                                                              *
*   LearnBookUpdate() is called to find the current position in the book and   *
*   update the learn counter.  if it is supposed to mark a move as not to be   *
*   played, and after marking such a move there are no more left at this point *
*   in the database, it returns (0) which will force LearnBook() to back up    *
*   two plies and update that position as well, since no more choices at the   *
*   current position doesn't really do much for us...                          *
*                                                                              *
********************************************************************************
*/
void LearnBookUpdate(int wtm, int move, int learn_value, int search_depth)
{
  int cluster, test, move_index, key, lc, lv;
  BITBOARD temp_hash_key, common;
/*
 ----------------------------------------------------------
|                                                          |
|   first find the appropriate cluster, make the move we   |
|   were passed, and find the resulting position in the    |
|   database.                                              |
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
      common=And(HashKey,mask_16);
      MakeMove(1,move,wtm);
      temp_hash_key=Xor(HashKey,wtm_random[wtm]);
      temp_hash_key=Or(And(temp_hash_key,Compl(mask_16)),common);
      for (move_index=0;move_index<cluster;move_index++)
        if (!Xor(temp_hash_key,buffer[move_index].position)) break;
      UnMakeMove(1,move,wtm);
      if (move_index >= cluster) return;
      lv=(buffer[move_index].learn&077777777)-037777777;
      lc=(buffer[move_index].learn>>24)&255;
      lv=(lv*lc+learn_value)/(lc+1);
      buffer[move_index].learn=(lv+037777777)|(Min(lc+1,255)<<24);
      fseek(book_file,key+sizeof(int),SEEK_SET);
      fwrite(buffer,sizeof(BOOK_POSITION),cluster,book_file);
      fflush(book_file);
    }
  }
}

/* last modified 11/21/96 */
/*
********************************************************************************
*                                                                              *
*   LearnFunction() is called to compute the adjustment value added to the     *
*   learn counter in the opening book.  it takes three pieces of information   *
*   into consideration to do this:  the search value, the search depth that    *
*   produced this value, and the rating difference (Crafty-opponent) so that   *
*   + numbers means Crafty is expected to win, - numbers mean Crafty is ex-    *
*   pected to lose.                                                            *
*                                                                              *
********************************************************************************
*/
int LearnFunction(int search_value, int search_depth, int rating_difference)
{
  int material[10] = {1, 100, 200, 250, 300, 350, 400, 450, 500, 600};
  int ply_multiplier[20] = { 0,  1,  2,  3,  4,  5,  6,  7,  8,  9,
                            10, 11, 12, 13, 14, 15, 16, 17, 18, 19} ; 
  float rating_multiplier[11] = {6.0, 5.0, 4.0, 3.0, 2.0, 1.0,
                                 0.75, 0.5, 0.25, 0.125, 0.0625};
  int sv, sd, rd, positional_edge;

  if (abs(search_value) < PAWN_VALUE && abs(search_value) >= 10)
    positional_edge=abs(search_value)/10;
  else
    positional_edge=1;
  sv=Min(abs(search_value)/PAWN_VALUE,9);
  sd=Max(Min(search_depth,19),0);
  rd=Max(Min(rating_difference/200,5),-5);
  if (search_value < 0) {
    rd=-rd;
    return(-material[sv]*ply_multiplier[sd]*rating_multiplier[rd+5]*positional_edge);
  }
  else if (search_value > 0) {
    return(+material[sv]*ply_multiplier[sd]*rating_multiplier[rd+5]*positional_edge);
  }
  else return(0);
/*
  while (1) {
    int sv, sd, sr;
    printf("value:0);
    scanf("%d",&sv);
    printf("depth:");
    scanf("%d",&sd);
    printf("rating diff:");
    scanf("%d",&sr);
    printf("adjustment=%d\n",LearnFunction(sv,sd,sr));
  }
*/
}
