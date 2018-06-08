#include <stdio.h>
#include <stdlib.h>
#include "types.h"
#include "function.h"
#include "data.h"
/*
********************************************************************************
*                                                                              *
*   Quiesce() is the recursive routine used to implement the alpha/beta        *
*   negamax search (similar to minimax but simpler to code.)  Quiesce() is     *
*   called whenever there is no "depth" remaining so that only capture moves   *
*   (and other "tactical" moves (like passed pawn pushes, for one example).    *
*                                                                              *
********************************************************************************
*/
int Quiesce(int alpha, int beta, int wtm, int depth, int ply)
{
  register int save_alpha, value;
  register int i, result;
/*
 ----------------------------------------------------------
|                                                          |
|   first, check to see if we should check the time to see |
|   if the search should terminate.                        |
|                                                          |
 ----------------------------------------------------------
*/
  nodes_searched++;
  full[ply]=0;
  in_check[ply]=0;
  extended_reason[ply]=no_extension;
  static_eval[ply]=0;
  if (nodes_searched > next_time_check) {
    next_time_check=nodes_searched+nodes_between_time_checks;
    if (CheckInput()) Interrupt(ply);
    time_abort+=TimeCheck();
    if (time_abort) {
      abort_search=1;
      return(0);
    }
  }
  save_alpha=alpha;
  max_search_depth=(ply>max_search_depth) ? ply : max_search_depth;
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
  if ((result=Lookup(ply,depth,wtm,&alpha,alpha,beta))) {
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
/*
 ----------------------------------------------------------
|                                                          |
|   if in check, use QuiesceFull() search code to escape.  |
|   make sure that all moves in the quiescence search have |
|   been checks, otherwise any mates found aren't forced   |
|   and this would waste time avoiding something that is   |
|   not forced.                                            |
|                                                          |
 ----------------------------------------------------------
*/
  if ((depth >= -1) || in_check[ply-2])
    if (Check(ply,wtm)) {
      in_check[ply]=1;
      value=QuiesceFull(alpha,beta,wtm,depth,ply);
      return(value);
    }
  repetition_list[repetition_head+ply]=HashKey(ply);
/*
 ----------------------------------------------------------
|                                                          |
|   now do a fast evaluation (material score only) to see  |
|   if that's enough to cause a cutoff.  otherwise do a    |
|   full evaluation and check again.  this positional eval |
|   then becomes the "stand pat" option unless a good      |
|   tactical move is found.                                |
|                                                          |
 ----------------------------------------------------------
*/
  value=Material(ply);
  if (!wtm) value=-value;
  if(value-largest_positional_score >= beta) return(beta);
  else if(value+largest_positional_score <= alpha) value=alpha;
  else value=Evaluate(ply,wtm,alpha,beta);
  if (value > alpha) {
    if (value >= beta) return(beta);
    alpha=value;
    for (i=1;i<ply;i++) pv[ply].path[i]=current_move[i];
#if !defined(FAST)
    if (show_extensions)
      for (i=1;i<ply;i++) pv_extensions[ply].extensions[i]=extended_reason[i];
#endif
    pv[ply].path_length=ply-1;
    pv[ply].path_hashed=0;
    pv[ply].path_iteration_depth=iteration_depth;
  }
/*
 ----------------------------------------------------------
|                                                          |
|   now iterate through the move list and search the       |
|   resulting positions.  note that Quiesce() culls any    |
|   move that is not legal by using Check().  the special  |
|   case is that we must find one legal move to search to  |
|   confirm that it's not a mate or draw.                  |
|                                                          |
 ----------------------------------------------------------
*/
  next_status[ply].whats_generated=nothing;
  next_status[ply].phase=hash_capture_move;
  first[ply]=last[ply-1]+1;
  last[ply]=first[ply];
  while ((current_phase[ply]=NextCapture(ply,wtm,depth))) {
#if !defined(FAST)
    if (ply <= trace_level) {
      for (i=1;i<ply;i++) printf("  ");
      printf("%d  %s d:%d [%s,",ply,
             OutputMove(&current_move[ply],ply,wtm),depth,
             DisplayEvaluation(alpha));
      printf("%s] n:%d [q-%d]\n", DisplayEvaluation(beta),
             nodes_searched,current_phase[ply]);
    }
#endif
    MakeMove(ply,current_move[ply],wtm);
    if (!Check(ply+1,wtm)) {
#if defined(DEBUG)
      if (!ValidMove(ply,wtm,current_move[ply]))
        printf("Quiesce() searching an illegal move at ply %d\n",ply);
      ValidatePosition(ply+1);
#endif
      value=-Quiesce(-beta,-alpha,!wtm,depth-1,ply+1);
      if (abort_search) return(0);
      if (value > alpha) {
        if(value >= beta) {
          StoreRefutation(ply,depth,wtm,beta);
          return(beta);
        }
        alpha=value;
      }
    }
  }
  if (alpha != save_alpha) {
    pv[ply-1]=pv[ply];
#if !defined(FAST)
    if (show_extensions) pv_extensions[ply-1]=pv_extensions[ply];
#endif
  }
  if (ply == 2) SearchOutput(wtm,-alpha,-save_alpha);
  StoreBest(ply,depth,wtm,alpha,save_alpha);
  return(alpha);
}

/*
********************************************************************************
*                                                                              *
*   QuiesceFull() is the recursive routine used to implement the alpha/beta    *
*   negamax search (similar to minimax but simpler to code.)  QuiesceFull()    *
*   is called from Quiesce() whenever the side-to-move is in check.            *
*   QuiesceFull() searches all successor positions rather than choosing        *
*   all successor branches to avoid a potential checkmate condition.           *
*                                                                              *
********************************************************************************
*/
int QuiesceFull(int alpha, int beta, int wtm, int depth, int ply)
{
  register int no_legal_moves = 1;
  register int save_alpha, value;
  register int i;
/*
 ----------------------------------------------------------
|                                                          |
|   initialize.                                            |
|                                                          |
 ----------------------------------------------------------
*/
  full[ply]=1;
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
  next_status[ply].phase=hash_normal_move;
  first[ply]=last[ply-1]+1;
  last[ply]=first[ply];
/*
 ----------------------------------------------------------
|                                                          |
|   now iterate through the move list and search the       |
|   resulting positions.  note that Quiesce() culls any    |
|   move that is not legal by using Check().  the special  |
|   case is that we must find one legal move to search to  |
|   confirm that it's not a mate or draw.                  |
|                                                          |
 ----------------------------------------------------------
*/
  while ((current_phase[ply]=NextMove(depth,ply,wtm))) {
#if !defined(FAST)
    if (ply <= trace_level) {
      for (i=1;i<ply;i++) printf("  ");
      printf("%d  %s d:%d [%s,",ply,
             OutputMove(&current_move[ply],ply,wtm),depth,
             DisplayEvaluation(alpha));
      printf("%s] n:%d [qf-%d]\n", DisplayEvaluation(beta),
             nodes_searched,current_phase[ply]);
    }
#endif
    MakeMove(ply,current_move[ply],wtm);
    if (!Check(ply+1,wtm)) {
#if defined(DEBUG)
      if (!ValidMove(ply,wtm,current_move[ply]))
        printf("QuiesceFull() searching an illegal move at ply %d\n",ply);
      ValidatePosition(ply+1);
#endif
      value=-Quiesce(-beta,-alpha,!wtm,depth-1,ply+1);
      if (abort_search) return(0);
      no_legal_moves=0;
      if (value > alpha) {
        if(value >= beta) {
          HistoryRefutation(ply,depth,wtm);
          StoreRefutation(ply,depth,wtm,beta);
          return(beta);
        }
        alpha=value;
      }
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
      if (ply <= trace_level) printf("QuiesceFull() checkmate!  ply=%d\n",ply);
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
    if(ply <= trace_level) printf("stalemate!  ply=%d.\n",ply);
#endif
    return(value);
  }
  else {
    pv[ply-1]=pv[ply];
#if !defined(FAST)
    if (show_extensions) pv_extensions[ply-1]=pv_extensions[ply];
#endif
    if (ply == 2) SearchOutput(wtm,-alpha,-save_alpha);
    if (alpha != save_alpha) HistoryBest(ply,depth,wtm);
    StoreBest(ply,depth,wtm,alpha,save_alpha);
    return(alpha);
  }
}
