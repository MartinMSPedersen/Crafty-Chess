#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "types.h"
#include "function.h"
#include "data.h"
/*
********************************************************************************
*                                                                              *
*   Search() is the recursive routine used to implement the alpha/beta         *
*   negamax search (similar to minimax but simpler to code.)  Search() is      *
*   called whenever there is "depth" remaining so that all moves are subject   *
*   to searching, or when the side to move is in check, to make sure that this *
*   side isn't mated.  Search() recursively calls itself until depth is ex-    *
*   hausted, at which time it calls Quiesce() instead.                         *
*                                                                              *
********************************************************************************
*/
int Search(int alpha, int beta, int wtm, int depth, int ply, 
           int do_null, NODES node_type)
{
  register int no_legal_moves=1, first_move=1;
  register int save_alpha, value;
  register int tdepth, i, new_mev, result;
  register int tnodes;
/*
 ----------------------------------------------------------
|                                                          |
|   first, set the current node type to 1, 2 or 3 (Knuth & |
|   Moore) so that we can decide whether or not it is use- |
|   ful to try a null-move.                                |
|                                                          |
 ----------------------------------------------------------
*/
  nodes_searched++;
  if (node_type != PV) node_type=(node_type==CUT)?ALL:CUT;
/*
 ----------------------------------------------------------
|                                                          |
|   now, check to see if we should check the time to see   |
|   if the search should terminate.                        |
|                                                          |
 ----------------------------------------------------------
*/
  if (nodes_searched > next_time_check) {
    next_time_check=nodes_searched+nodes_between_time_checks;
    if (CheckInput()) Interrupt(ply);
    time_abort+=TimeCheck();
    if (time_abort) {
      abort_search=1;
      return(0);
    }
  }
  full[ply]=0;
  in_check[ply]=0;
  extended_reason[ply]=no_extension;
  static_eval[ply]=0;
  save_alpha=alpha;
/*
 ----------------------------------------------------------
|                                                          |
|   check for draw by repetition.                          |
|                                                          |
 ----------------------------------------------------------
*/
  if (RepetitionCheck(ply)) {
    if (wtm == root_wtm) value=DrawScore();
    else value=-DrawScore();
    if (value < beta) {
      for (i=1;i<ply;i++) pv[ply-1].path[i]=current_move[i];
#if !defined(FAST)
      if (show_extensions)
        for (i=1;i<ply;i++) pv_extensions[ply-1].extensions[i]=extended_reason[i];
#endif
      pv[ply-1].path_length=ply-1;
      pv[ply-1].path_hashed=0;
      pv[ply-1].path_iteration_depth=iteration_depth;
      if (ply == 2) SearchOutput(wtm,-value,-save_alpha);
    }
#if !defined(FAST)
    if(ply <= trace_level) printf("draw by repetition detected, ply=%d.\n",ply);
#endif
    return(value);
  }
/*
 ----------------------------------------------------------
|                                                          |
|   initialize.  set NextMove() status to 0 so it will     |
|   know what has to be done.                              |
|                                                          |
 ----------------------------------------------------------
*/
  next_status[ply].whats_generated=nothing;
  if (ply > 1) {
    next_status[ply].phase=hash_normal_move;
    first[ply]=last[ply-1]+1;
    last[ply]=first[ply];
  }
  else {
    next_status[ply].phase=root_moves;
    no_legal_moves=0;
  }
/*
 ----------------------------------------------------------
|                                                          |
|   if the side to move is in check, extend the search by  |
|   one ply since there are few legal responses and we     |
|   should follow them a little deeper.                    |
|                                                          |
 ----------------------------------------------------------
*/
  if (Check(ply,wtm)) {
    in_check[ply]=1;
    if (check_extensions && (ply > 1) && ((ply+depth) <= 2*iteration_depth)) {
      extended_reason[ply]|=check_extension;
      check_extensions_done++;
      depth++;
    }
  }
  tdepth=depth;
/*
 ----------------------------------------------------------
|                                                          |
|   now call Lookup() to see if this position has been     |
|   searched before.  if so, we may get a real score, a    |
|   better alpha or beta value, or, at least a good move   |
|   to try first.  there are three cases to handle:        |
|                                                          |
|   1.  lookup returned "good_score" which means the       |
|   value returned is a good score.  if this good score is |
|   greater than beta, return beta.  otherwise, return the |
|   score.  In either case, no further searching is needed |
|   from this position.  note that lookup verified that    |
|   the table position has sufficient "draft" to meet the  |
|   requirements of the current search depth remaining.    |
|   2.  lookup returned "failed_low" which means that      |
|   when this position was searched previously, every move |
|   was "refuted" by one of its descendents.  as a result, |
|   when the search was completed, we returned alpha at    |
|   that point.  since we know that this position is going |
|   to return a value <= that "old" alpha, we simply need  |
|   to make sure that the "old" alpha is <= the current    |
|   alpha to make sure that we can still stop searching.   |
|   3.  lookup returned "failed_high" which means that     |
|   when we encountered this position before, we searched  |
|   one branch (probably) which promptly refuted the move  |
|   at the previous ply.  we returned beta the last time   |
|   this happened.  we simply have to make sure that the   |
|   "old" beta is >= the current beta, so that we can      |
|   declare that this position is >= current beta.  if the |
|   current beta is > the old beta, we don't learn         |
|   anything except that we can now set alpha=old beta     |
|   because we know that the current position will produce |
|   a value of at least old beta.                          |
|                                                          |
 ----------------------------------------------------------
*/
  if (ply > 1) {
    if ((result=Lookup(ply,depth,wtm,&alpha,alpha,beta))) {
      no_legal_moves=0;
#if !defined(FAST)
      if(ply <= trace_level)
        printf("Lookup() returned %d at ply=%d\n",result,ply);
#endif
      switch (result) {
        case good_score:
          if(alpha >= beta) return(beta);
          else {
            for (i=1;i<ply;i++) pv[ply-1].path[i]=current_move[i];
#if !defined(FAST)
            if (show_extensions)
              for (i=1;i<ply;i++) pv_extensions[ply-1].extensions[i]=extended_reason[i];
#endif
            pv[ply-1].path_length=ply-1;
            pv[ply-1].path_hashed=1;
            pv[ply-1].path_iteration_depth=iteration_depth;
            if (ply == 2) SearchOutput(wtm,-alpha,-save_alpha);
            return(alpha);
          }
        case failed_low:
          if (ply == 2) SearchOutput(wtm,-alpha,-save_alpha);
          return(alpha);
        case failed_high:
          return(beta);
      }
    }
  }
/*
 ----------------------------------------------------------
|                                                          |
|  first, we try a null move to see if we can get a quick  |
|  cutoff with only a little work.  this operates as       |
|  follows.  instead of making a legal move, the side on   |
|  move 'passes' and does nothing.  the resulting position |
|  is searched to a shallower depth than normal (usually   |
|  one ply less but settable by the operator) this should  |
|  result in a cutoff or at least should set the lower     |
|  bound better since anything should be better than not   |
|  doing anything.                                         |
|                                                          |
|  this is skipped for any of the following reasons:       |
|                                                          |
|  1.  the side on move is in check.  the null move        |
|      results in an illegal position.                     |
|  2.  no more than one null move can appear in succession |
|      or else the search will degenerate into nothing.    |
|  3.  the side on move has little material left making    |
|      zugzwang positions more likely.                     |
|  4.  the move at the previous ply was a capture where    |
|      trying the null move simply allows him to move the  |
|      capturing piece to safety.                          |
|  5.  this is a PV node, where a null is illegal.         |
|                                                          |
 ----------------------------------------------------------
*/
  if ((ply > 1) && do_null && !in_check[ply] &&
      ((TotalBlackPieces(ply) && !wtm) || (TotalWhitePieces(ply) && wtm)) &&
      !Captured(current_move[ply-1]) && (node_type != PV)) {
    current_move[ply]=0;
    current_phase[ply]=null_move;
#if !defined(FAST)
    if (ply <= trace_level) {
      for (i=1;i<ply;i++) printf("  ");
      printf("%d  %s d:%d [%s,",ply,OutputMove(&current_move[ply],ply,wtm),
             depth,DisplayEvaluation(alpha));
      printf("%s] n:%d [s-%d] nt=%d\n", DisplayEvaluation(beta),
             nodes_searched,current_phase[ply],node_type);
    }
#endif
    null_moves_tried++;
    tnodes=nodes_searched;
    position[ply+1]=position[ply];
    if (EnPassantTarget(ply)) {
      HashEP(FirstOne(EnPassantTarget(ply+1)),HashKey(ply+1));
      EnPassantTarget(ply+1)=0;
    }
    if (depth > 3)
      value=-Search(-beta,-alpha,!wtm,depth-null_depth-1,ply+1,0,node_type);
    else if (depth-2 > 0)
      value=-Search(-beta,-alpha,!wtm,depth-2,ply+1,0,node_type);
    else
      value=-Quiesce(-beta,-alpha,!wtm,depth-2,ply+1);
    if (abort_search) return(0);
    nodes_searched_null_move+=nodes_searched-tnodes;
    if (value < beta) {
      nodes_searched_null_move_wasted+=nodes_searched-tnodes;
      null_moves_wasted++;
    }
    if (value > alpha) {
      if(value >= beta) {
        StoreRefutation(ply,depth,wtm,beta);
        return(beta);
      }
    }
  }
/*
 ----------------------------------------------------------
|                                                          |
|   if there is no best move from the hash table, and this |
|   is a PV node, then we need a good move to search       |
|   first.  while killers and history moves are good, they |
|   are not "good enough".  the simplest action is to try  |
|   a shallow search (depth-2) to get a move.  note that   |
|   when we call Search() with depth-2, it, too, will      |
|   not have a hash move, and will therefore recursively   |
|   continue this process, hence the name "internal        |
|   iterative deepening."                                  |
|                                                          |
 ----------------------------------------------------------
*/
  if ((node_type == PV) && (ply > 2) && !hash_move[ply] && (depth > 2)) {
    value=Search(alpha,beta,wtm,depth-2,ply,1,node_type);
    if (abort_search) return(0);
    if (value <= alpha) {
      value=Search(-MATE,beta,wtm,depth-2,ply,1,node_type);
      if (abort_search) return(0);
    }
    if (value < beta) {
      if (pv[ply-1].path_length >= ply) hash_move[ply]=pv[ply-1].path[ply];
    }
    else hash_move[ply]=current_move[ply];
    last[ply]=first[ply];
    next_status[ply].phase=hash_normal_move;
    next_status[ply].whats_generated=nothing;
  }
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
    extended_reason[ply]&=255-recapture_extension-passed_pawn_extension;
#if !defined(FAST)
    if (ply <= trace_level) {
      for (i=1;i<ply;i++) printf("  ");
      printf("%d  %s d:%d [%s,",ply,
             OutputMove(&current_move[ply],ply,wtm),depth,
             DisplayEvaluation(alpha));
      printf("%s] n:%d [s-%d] nt=%d\n", DisplayEvaluation(beta),
             nodes_searched,current_phase[ply],node_type);
    }
#endif
/*
 ----------------------------------------------------------
|                                                          |
|   if two successive moves are capture / re-capture so    |
|   that the material score is restored, extend the search |
|   by one ply on the re-capture since it is pretty much   |
|   forced and easy to analyze.                            |
|                                                          |
 ----------------------------------------------------------
*/
    depth=tdepth;
    if (wtm)
      new_mev=Material(ply)+piece_values[Captured(current_move[ply])]+
                            piece_values[Promote(current_move[ply])];
    else
      new_mev=Material(ply)-piece_values[Captured(current_move[ply])]-
                            piece_values[Promote(current_move[ply])];
    if (recapture_extensions && ((ply+depth) <= 2*iteration_depth) &&
        Captured(current_move[ply-1]) && Captured(current_move[ply]) &&
        (To(current_move[ply-1]) == To(current_move[ply])) &&
        (Material(ply-1) == new_mev) &&
        !(extended_reason[ply-1]&recapture_extension)) {
      extended_reason[ply]|=recapture_extension;
      recapture_extensions_done++;
      depth++;
    }
/*
 ----------------------------------------------------------
|                                                          |
|   if we push a pawn to the 7th rank, we need to look     |
|   deeper to see if it can promote.                       |
|                                                          |
 ----------------------------------------------------------
*/
    if (!passed_pawn_extensions && 
        ((ply+depth) <= 2*iteration_depth) && end_game) 
      if ((Piece(current_move[ply]) == pawn) &&
          push_extensions[To(current_move[ply])] &&
          (Swap(ply,From(current_move[ply]),To(current_move[ply]),wtm) >= 0)) {
        depth++;
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
    if (!Check(ply+1,wtm)) {
      no_legal_moves=0;
#if defined(DEBUG)
      if (!ValidMove(ply,wtm,current_move[ply]))
        printf("Search() searching an illegal move at ply %d\n",ply);
      ValidatePosition(ply+1);
#endif
      if (first_move) {
        if (depth-1 > 0)
          value=-Search(-beta,-alpha,!wtm,depth-1,ply+1,1,node_type);
        else
          value=-Quiesce(-beta,-alpha,!wtm,depth-1,ply+1);
        if (abort_search) return(0);
        first_move=0;
      }
      else {
        if (depth-1 > 0)
          value=-Search(-alpha-1,-alpha,!wtm,depth-1,ply+1,1,node_type);
        else
          value=-Quiesce(-alpha-1,-alpha,!wtm,depth-1,ply+1);
        if (abort_search) return(0);
        if ((value > alpha) && (value < beta)) {
          if (depth-1 > 0)
            value=-Search(-beta,-alpha,!wtm,depth-1,ply+1,1,node_type);
          else
            value=-Quiesce(-beta,-alpha,!wtm,depth-1,ply+1);
          if (abort_search) return(0);
        }
      }
      if (value > alpha) {
        if(value >= beta) {
          HistoryRefutation(ply,depth,wtm);
          StoreRefutation(ply,depth,wtm,beta);
          return(beta);
        }
        alpha=value;
        if (ply == 1) {
          root_value=alpha;
          search_failed_high=0;
          search_failed_low=0;
        }
      }
      node_type=ALL;
    }
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
  if (no_legal_moves) {
/*
 ------------------------------------------------
|                                                |
|   checkmate if in check.                       |
|                                                |
 ------------------------------------------------
*/
    if (Check(ply,wtm)) {
      for (i=1;i<ply;i++) pv[ply-1].path[i]=current_move[i];
#if !defined(FAST)
      if (show_extensions)
        for (i=1;i<ply;i++) pv_extensions[ply-1].extensions[i]=extended_reason[i];
#endif
      pv[ply-1].path_length=ply-1;
      pv[ply-1].path_hashed=0;
      pv[ply-1].path_iteration_depth=iteration_depth;
      if (ply == 2) SearchOutput(wtm,MATE-ply,-save_alpha);
#if !defined(FAST)
      if (ply <= trace_level) printf("search() checkmate!  ply=%d\n",ply);
#endif
      return(-(MATE-ply));
    }
/*
 ------------------------------------------------
|                                                |
|   otherwise draw (stalemate).                  |
|                                                |
 ------------------------------------------------
*/
    if (wtm == root_wtm) value=DrawScore();
    else value=-DrawScore();
    if(value > beta) value=beta;
    else if (value < alpha) value=alpha;
    if (value >=alpha && value <beta) {
      for (i=1;i<ply;i++) pv[ply-1].path[i]=current_move[i];
#if !defined(FAST)
      if (show_extensions)
        for (i=1;i<ply;i++) pv_extensions[ply-1].extensions[i]=extended_reason[i];
#endif
      pv[ply-1].path_length=ply-1;
      pv[ply-1].path_hashed=0;
      pv[ply-1].path_iteration_depth=iteration_depth;
      if (ply == 2) SearchOutput(wtm,-value,-save_alpha);
    }
#if !defined(FAST)
    if(ply <= trace_level) 
      printf("stalemate!  ply=%d.\n",ply);
#endif
    return(value);
  }
  else {
    if (alpha != save_alpha) {
      pv[ply-1]=pv[ply];
#if !defined(FAST)
      if (show_extensions) pv_extensions[ply-1]=pv_extensions[ply];
#endif
      HistoryBest(ply,depth,wtm);
    }
    if (ply == 2) SearchOutput(wtm,-alpha,-save_alpha);
    StoreBest(ply,depth,wtm,alpha,save_alpha);
    return(alpha);
  }
}
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
void SearchOutput(int wtm, int value, int bound)
{
  char ext_char[5] = {"-x=."};
  register int *mv, *mvp;
  register char *whisper_p;
  register int i, j, more, twtm, tv;
  int dummy;

  if (!abort_search) {
    for (mvp=first[1];mvp<last[1];mvp++) if(current_move[1]== *mvp) break;
    if (mvp != first[1]) {
      tv=root_sort_value[mvp-first[1]];
      for (mv=mvp;mv>first[1];mv--) {
        *mv=*(mv-1);
        root_sort_value[mv-first[1]]=root_sort_value[mv-first[1]-1];
      }
      *first[1]=current_move[1];
      root_sort_value[0]=tv;
      easy_move=0;
    }
    if ((nodes_searched > noise_level) || (value > (MATE-100))) {
      end_time=GetTime(time_type);
      if(value < bound) {
        Print(2,"               %2i   %s%s   ",iteration_depth,
               DisplayTime(end_time-start_time),DisplayEvaluation(value));
        more=1;
        for (i=1;i<=pv[1].path_length;i++) {
          wtm=!wtm;
          if (!more) Print(2,"                                     ");
          Print(2," %s",OutputMove(&pv[1].path[i],i,wtm));
#if !defined(FAST)
          if (show_extensions && pv_extensions[1].extensions[i]) {
            int j,k;
            for (j=1,k=0;j<16;j=j<<1,k++)
              if (pv_extensions[1].extensions[i]&j)
                Print(2,"%c",ext_char[k]);
          }
#endif
          more=1;
          if (!(i&7)) {
            Print(2,"\n");
            more=0;
          }
          MakeMove(i,pv[1].path[i],wtm);
        }
        if(pv[1].path_hashed) {
          for (i=pv[1].path_length+1;i<=MAXPLY;i++) {
            wtm=!wtm;
            if (Lookup(i,0,wtm,&dummy,dummy,dummy)) {
              if (hash_move[i]) {
                pv[1].path[i]=hash_move[i];
                pv[1].path_length++;
#if !defined(FAST)
                pv_extensions[1].extensions[i]=16;
#endif
                for (j=1;j<i;j++) 
                  if (pv[1].path[i] == pv[1].path[j]) break;
                if (j < i) break;
              }
              else break;
            }
            else break;
            if (!more) Print(2,"                                     ");
#if !defined(FAST)
            if (!show_extensions)
              Print(2," %s",OutputMove(&pv[1].path[i],i,wtm));
            else
              Print(2," %s.",OutputMove(&pv[1].path[i],i,wtm));
#endif
            more=1;
            if (!(i&7)) {
              Print(2,"\n");
              more=0;
            }
            MakeMove(i,pv[1].path[i],wtm);
          }
          if (!more) Print(2,"                                     ");
          Print(2," ...");
          more=1;
        }
        if (more) Print(2,"   \n");
      }
      else {
        Print(2,"               %2i   %s      ++   ",iteration_depth,
        DisplayTime(end_time-start_time));
        Print(2," %s\n",OutputMove(&current_move[1],1,!wtm));
        if (current_move[1] != pv[1].path[1]) {
          pv[1].path[1]=current_move[1];
          pv[1].path_length=1;
          pv[1].path_hashed=0;
          pv[1].path_iteration_depth=iteration_depth;
        }
      }
    }
/*
 ----------------------------------------------------------
|                                                          |
|   if "whisper" is enabled (ICS whisper command) then we  |
|   need to stuff the PV into whisper_text for output when |
|   the search ends.                                       |
|                                                          |
 ----------------------------------------------------------
*/
    if ((whisper || kibitz) && !puzzling) {
      if (booking)
        sprintf(whisper_text,"(book)");
      else
        whisper_text[0]=0;
      whisper_p=whisper_text+strlen(whisper_text);
      whisper_value=value;
      whisper_depth=iteration_depth;
      twtm=root_wtm;
      for (i=1;i<=pv[1].path_length;i++) {
        sprintf(whisper_p," %s",OutputMove(&pv[1].path[i],i,twtm));
        whisper_p=whisper_text+strlen(whisper_text);
        MakeMove(i,pv[1].path[i],twtm);
        twtm=!twtm;
      }
    }
  }
}
