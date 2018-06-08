#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "chess.h"
#include "data.h"
#include "epdglue.h"

/* modified 11/26/96 */
/*
********************************************************************************
*                                                                              *
*   SearchRoot() is the recursive routine used to implement the alpha/beta     *
*   negamax search (similar to minimax but simpler to code.)  SearchRoot() is  *
*   only called when ply=1.  it is somewhat different from Search() in that    *
*   some things (null move search, hash lookup, etc.) are not useful at the    *
*   root of the tree.  SearchRoot() calls Search() to search any positions     *
*   that are below ply=1.                                                      *
*                                                                              *
********************************************************************************
*/
int SearchRoot(int alpha, int beta, int wtm, int depth, int ply)
{
  register int first_move=1;
  register int initial_alpha, value;
  register int extensions, i;
/*
 ----------------------------------------------------------
|                                                          |
|   initialize.  set NextMove() status to 0 so it will     |
|   know what has to be done.                              |
|                                                          |
 ----------------------------------------------------------
*/
  nodes_searched++;
  in_check[ply+1]=0;
  extended_reason[ply+1]=no_extension;
  initial_alpha=alpha;
  RepetitionCheck(ply,wtm);
  in_check[ply]=Check(wtm);
  next_status[ply].phase=ROOT_MOVES;
/*
 ----------------------------------------------------------
|                                                          |
|   now iterate through the move list and search the       |
|   resulting positions.  note that Search() culls any     |
|   move that is not legal by using Check().  the special  |
|   case is that we must find one legal move to search to  |
|   confirm that it's not a mate or draw.                  |
|                                                          |
 ----------------------------------------------------------
*/
  while ((current_phase[ply]=NextMove(depth,ply,wtm))) {
    extended_reason[ply]&=check_extension;
#if !defined(FAST)
    if (ply <= trace_level) SearchTrace(ply,depth,wtm,alpha,beta,"SearchRoot",current_phase[ply]);
#endif
/*
 ----------------------------------------------------------
|                                                          |
|   if we push a pawn to the 7th rank, we need to look     |
|   deeper to see if it can promote.                       |
|                                                          |
 ----------------------------------------------------------
*/
    extensions=-INCREMENT_PLY;
    if (Piece(current_move[ply]) == pawn) 
      if (end_game && !FutileAhead(wtm) &&
          ((wtm && TotalBlackPieces<=5 && To(current_move[ply])>H5 &&
            !And(mask_pawn_passed_w[To(current_move[ply])],BlackPawns)) ||
           (!wtm && TotalWhitePieces<=5 && To(current_move[ply])<A4 &&
            !And(mask_pawn_passed_b[To(current_move[ply])],WhitePawns)) ||
           push_extensions[To(current_move[ply])]) &&
           Swap(From(current_move[ply]),To(current_move[ply]),wtm)>=0) {
          extensions+=PASSED_PAWN_PUSH;
          extended_reason[ply]|=passed_pawn_extension;
          passed_pawn_extensions_done++;
        }
/*
 ----------------------------------------------------------
|                                                          |
|   now make the move and search the resulting position.   |
|                                                          |
 ----------------------------------------------------------
*/
    MakeMove(ply,current_move[ply],wtm);
/*
 ----------------------------------------------------------
|                                                          |
|   if the move to be made checks the opponent, then we    |
|   need to remember that he's in check and also extend    |
|   the depth by one ply for him to get out.               |
|                                                          |
 ----------------------------------------------------------
*/
      if (Check(ChangeSide(wtm))) {
        in_check[ply+1]=1;
        extended_reason[ply+1]=check_extension;
        check_extensions_done++;
        extensions+=IN_CHECK;
      }
      else {
        in_check[ply+1]=0;
        extended_reason[ply+1]=no_extension;
      }
/*
 ----------------------------------------------------------
|                                                          |
|   if there's only one legal move, extend the search one  |
|   additional ply since this node is very easy to search. |
|                                                          |
 ----------------------------------------------------------
*/
    if (first_move) {
      if (depth+MaxExtensions(extensions) > INCREMENT_PLY-1)
        value=-Search(-beta,-alpha,ChangeSide(wtm),depth+MaxExtensions(extensions),ply+1,DO_NULL);
      else
        value=-Quiesce(-beta,-alpha,ChangeSide(wtm),ply+1);
      if (abort_search) {
        UnMakeMove(ply,current_move[ply],wtm);
        return(0);
      }
      first_move=0;
    }
    else {
      if (depth+MaxExtensions(extensions) > INCREMENT_PLY-1)
        value=-Search(-alpha-1,-alpha,ChangeSide(wtm),depth+MaxExtensions(extensions),ply+1,DO_NULL);
      else
        value=-Quiesce(-alpha-1,-alpha,ChangeSide(wtm),ply+1);
      if (abort_search) {
        UnMakeMove(ply,current_move[ply],wtm);
        return(0);
      }
      if ((value > alpha) && (value < beta)) {
        if (depth+MaxExtensions(extensions) > INCREMENT_PLY-1)
          value=-Search(-beta,-alpha,ChangeSide(wtm),depth+MaxExtensions(extensions),ply+1,DO_NULL);
        else 
          value=-Quiesce(-beta,-alpha,ChangeSide(wtm),ply+1);
        if (abort_search) {
          UnMakeMove(ply,current_move[ply],wtm);
          return(0);
        }
      }
    }
    if (value > alpha) {
      SearchOutput(value,beta);
      if(value >= beta) {
        HistoryRefutation(ply,depth,wtm);
        UnMakeMove(ply,current_move[ply],wtm);
        StoreRefutation(ply,depth,wtm,beta);
        return(beta);
      }
      alpha=value;
      root_value=alpha;
    }
    UnMakeMove(ply,current_move[ply],wtm);
  }
/*
 ----------------------------------------------------------
|                                                          |
|   all moves have been searched.  if none were legal,     |
|   return either MATE or DRAW depending on whether the    |
|   side to move is in check or not.                       |
|                                                          |
 ----------------------------------------------------------
*/
  if (first_move == 1) {
    value=(Check(wtm)) ? -(MATE-ply) :
                             ((wtm==root_wtm) ? DrawScore() : -DrawScore());
    if(value > beta) value=beta;
    else if (value < alpha) value=alpha;
    if (value >=alpha && value <beta) {
      SavePVS(ply-1,value,0);
#if !defined(FAST)
      if (ply <= trace_level) printf("Search() no moves!  ply=%d\n",ply);
#endif
    }
    return(value);
  }
  else {
    if (alpha != initial_alpha) {
      pv[ply-1]=pv[ply];
      HistoryBest(ply,depth,wtm);
    }
    StoreBest(ply,depth,wtm,alpha,initial_alpha);
    return(alpha);
  }
}

/* modified 11/26/96 */
/*
********************************************************************************
*                                                                              *
*   SearchOutput() is used to print the principal variation whenever it        *
*   changes.  one additional feature is that SearchOutput() will try to do     *
*   something about variations truncated by the transposition table.  if the   *
*   variation was cut short by a transposition table hit, then we can make the *
*   last move, add it to the end of the variation and extend the depth of the  *
*   variation to cover it.                                                     *
*                                                                              *
********************************************************************************
*/
void SearchOutput(int value, int bound)
{
#define PrintOK() (((nodes_searched+q_nodes_searched) > noise_level) || \
                   (value > (MATE-100)))
  char buffer[500], *buffp, *bufftemp;
  register int *mv, *mvp;
  register int i, j, tv,  wtm;
  int dummy;

/*
 ----------------------------------------------------------
|                                                          |
|   first, move the best move to the top of the ply-1 move |
|   list if it's not already there, so that it will be the |
|   first move tried in the next iteration.                |
|                                                          |
 ----------------------------------------------------------
*/
  wtm=root_wtm;
  if (!abort_search) {
    whisper_value=(analyze_mode && !root_wtm) ? -value : value;
    whisper_depth=iteration_depth;
    for (mvp=last[0];mvp<last[1];mvp++) if(current_move[1]== *mvp) break;
    if (mvp != last[0]) {
      tv=root_sort_value[mvp-last[0]];
      for (mv=mvp;mv>last[0];mv--) {
        *mv=*(mv-1);
        root_sort_value[mv-last[0]]=root_sort_value[mv-last[0]-1];
      }
      *last[0]=current_move[1];
      root_sort_value[0]=tv;
      easy_move=0;
    }
    end_time=GetTime(time_type);
/*
 ----------------------------------------------------------
|                                                          |
|   if this is not a fail-high move, then output the PV    |
|   by walking down the path being backed up.              |
|                                                          |
 ----------------------------------------------------------
*/
    if(value < bound) {
      buffer[0]=0;
      UnMakeMove(1,pv[1].path[1],root_wtm);
      for (i=1;i<=pv[1].path_length;i++) {
        sprintf(buffer+strlen(buffer)," %s",OutputMove(&pv[1].path[i],i,wtm));
        MakeMove(i,pv[1].path[i],wtm);
        wtm=ChangeSide(wtm);
      }
/*
 ----------------------------------------------------------
|                                                          |
|   if the pv was terminated prematurely by a trans/ref    |
|   hit, see if any more moves are in the trans/ref table  |
|   and if so, add 'em to the end of the PV so we will     |
|   have better move ordering next iteration.              |
|                                                          |
 ----------------------------------------------------------
*/
      if(pv[1].path_hashed == 1) {
        for (i=pv[1].path_length+1;i<=MAXPLY;i++) {
          LookUp(i,0,wtm,&dummy,dummy);
          if (hash_move[i] && ValidMove(i,wtm,hash_move[i])) {
            pv[1].path[i]=hash_move[i];
            for (j=1;j<i;j++) 
              if (pv[1].path[i] == pv[1].path[j]) break;
            if (j < i) break;
            pv[1].path_length++;
            sprintf(buffer+strlen(buffer)," %s",OutputMove(&pv[1].path[i],i,wtm));
            MakeMove(i,pv[1].path[i],wtm);
          }
          else break;
          wtm=ChangeSide(wtm);
        }
        sprintf(buffer+strlen(buffer)," <HT>");
      }
      else if(pv[1].path_hashed == 2) 
        sprintf(buffer+strlen(buffer)," <EGTB>");
      strcpy(whisper_text,buffer);
      if (PrintOK()) {
        Print(2,"               %2i   %s%s   ",iteration_depth,
              DisplayTime(end_time-start_time),DisplayEvaluation(value));
        buffp=buffer;
        do {
          if ((int) strlen(buffp) > 34) 
            bufftemp=strchr(buffp+34,' ');
          else 
            bufftemp=0;
          if (bufftemp) *bufftemp=0;
          Print(2,"%s\n",buffp);
          buffp=bufftemp+1;
          if (bufftemp) Print(2,"                                      ");
        } while(bufftemp);
        Whisper(6,iteration_depth,end_time-start_time,whisper_value,
                nodes_searched+q_nodes_searched,0,whisper_text);
      }
      for (i=pv[1].path_length;i>1;i--) {
        wtm=ChangeSide(wtm);
        UnMakeMove(i,pv[1].path[i],wtm);
      }
    }
    else if (PrintOK()) {
      Print(2,"               %2i   %s      ++   ",iteration_depth,
      DisplayTime(end_time-start_time));
      UnMakeMove(1,current_move[1],wtm);
      Print(2," %s!!\n",OutputMove(&current_move[1],1,wtm));
      sprintf(whisper_text," %s!!",OutputMove(&current_move[1],1,wtm));
      MakeMove(1,current_move[1],wtm);
      if (current_move[1] != pv[1].path[1]) {
        pv[1].path[1]=current_move[1];
        pv[1].path_length=1;
        pv[1].path_hashed=0;
        pv[1].path_iteration_depth=iteration_depth;
      }
      Whisper(6,iteration_depth,end_time-start_time,whisper_value,
              nodes_searched+q_nodes_searched,-1,whisper_text);
    }
  }
}

/* modified 05/20/96 */
/*
********************************************************************************
*                                                                              *
*   SearchTrace() is used to print the search trace output each time a node is *
*   traversed in the tree.                                                     *
*                                                                              *
********************************************************************************
*/
void SearchTrace(int ply, int depth, int wtm, int alpha, int beta, char* name,
                 int phase)
{
  int i;
  for (i=1;i<ply;i++) printf("  ");
  printf("%d  %s d:%5.2f [%s,",ply,OutputMove(&current_move[ply],ply,wtm),
         (float) depth/ (float) INCREMENT_PLY,DisplayEvaluation(alpha));
  printf("%s] n:%d %s(%d)\n", DisplayEvaluation(beta),
         (nodes_searched+q_nodes_searched),name,phase);
}
