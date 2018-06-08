#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "chess.h"
#include "data.h"
#include "epdglue.h"

/* modified 10/18/99 */
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
int SearchRoot(TREE *tree, int alpha, int beta, int wtm, int depth) {
  register int first_move=1;
  register int initial_alpha, value;
  register int extensions, begin_root_nodes;
/*
 ----------------------------------------------------------
|                                                          |
|   initialize.  set NextMove() status to 0 so it will     |
|   know what has to be done.                              |
|                                                          |
 ----------------------------------------------------------
*/
  tree->in_check[2]=0;
  tree->extended_reason[2]=no_extension;
  initial_alpha=alpha;
  tree->in_check[1]=Check(wtm);
  tree->next_status[1].phase=ROOT_MOVES;
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
  while ((tree->phase[1]=NextRootMove(tree,wtm))) {
    tree->extended_reason[1]=0;
#if defined(TRACE)
    if (1 <= trace_level)
      SearchTrace(tree,1,depth,wtm,alpha,beta,"SearchRoot",tree->phase[1]);
#endif
/*
 ----------------------------------------------------------
|                                                          |
|   if we push a pawn to the 7th rank, we need to look     |
|   deeper to see if it can promote.                       |
|                                                          |
 ----------------------------------------------------------
*/
    extensions=-60;
    if (Piece(tree->current_move[1])==pawn &&
      push_extensions[To(tree->current_move[1])]) {
      tree->extended_reason[1]|=passed_pawn_extension;
      tree->passed_pawn_extensions_done++;
      extensions+=pushpp_depth;
    }
/*
 ----------------------------------------------------------
|                                                          |
|   now make the move and search the resulting position.   |
|                                                          |
 ----------------------------------------------------------
*/
    MakeMove(tree,1,tree->current_move[1],wtm);
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
      tree->in_check[2]=1;
      tree->extended_reason[2]=check_extension;
      tree->check_extensions_done++;
      extensions+=incheck_depth;
    }
    else {
      tree->in_check[2]=0;
      tree->extended_reason[2]=no_extension;
    }
/*
 ----------------------------------------------------------
|                                                          |
|   now call Search to produce a value for this move.      |
|                                                          |
 ----------------------------------------------------------
*/
    begin_root_nodes=tree->nodes_searched;
    extensions=Min(extensions,0);
    if (first_move) {
      if (depth+extensions >= INCPLY)
        value=-Search(tree,-beta,-alpha,ChangeSide(wtm),
                      depth+extensions,2,DO_NULL);
      else
        value=-Quiesce(tree,-beta,-alpha,ChangeSide(wtm),2);
      if (abort_search) {
        UnMakeMove(tree,1,tree->current_move[1],wtm);
        return(alpha);
      }
      first_move=0;
    }
    else {
      if (depth+extensions >= INCPLY)
        value=-Search(tree,-alpha-1,-alpha,ChangeSide(wtm),
                      depth+extensions,2,DO_NULL);
      else
        value=-Quiesce(tree,-alpha-1,-alpha,ChangeSide(wtm),2);
      if (abort_search) {
        UnMakeMove(tree,1,tree->current_move[1],wtm);
        return(alpha);
      }
      if ((value > alpha) && (value < beta)) {
      if (depth+extensions >= INCPLY)
        value=-Search(tree,-beta,-alpha,ChangeSide(wtm),
                      depth+extensions,2,DO_NULL);
      else
        value=-Quiesce(tree,-beta,-alpha,ChangeSide(wtm),2);
        if (abort_search) {
          UnMakeMove(tree,1,tree->current_move[1],wtm);
          return(alpha);
        }
      }
    }
    root_moves[tree->root_move].nodes=tree->nodes_searched-begin_root_nodes;
    if (value > alpha) {
      SearchOutput(tree,value,beta);
      root_value=alpha;
      if(value >= beta) {
        History(tree,1,depth,wtm,tree->current_move[1]);
        UnMakeMove(tree,1,tree->current_move[1],wtm);
        return(value);
      }
      alpha=value;
      beta=value+40;
      root_beta=beta;
    }
    root_value=alpha;
    UnMakeMove(tree,1,tree->current_move[1],wtm);
#if defined(SMP)
    if (split_at_root && smp_idle && NextRootMoveParallel()) {
      tree->alpha=alpha;
      tree->beta=beta;
      tree->value=alpha;
      tree->wtm=wtm;
      tree->ply=1;
      tree->depth=depth;
      tree->threat=0;
      if(Thread(tree)) {
        if (abort_search || tree->stop) return(0);
        if (CheckInput()) Interrupt(1);
        value=tree->search_value;
        if (value > alpha) {
          if(value >= beta) {
            History(tree,1,depth,wtm,tree->current_move[1]);
            tree->fail_high++;
            return(value);
          }
          alpha=value;
          break;
        }
      }
    }
#endif
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
  if (abort_search || time_abort) return(0);
  if (first_move == 1) {
    value=(Check(wtm)) ? -(MATE-1) : DrawScore(wtm);
    if (value >=alpha && value <beta) {
      tree->pv[0].pathl=0;
      tree->pv[0].pathh=0;
      tree->pv[0].pathd=iteration_depth;
      SearchOutput(tree,value,beta);
#if defined(TRACE)
      if (1 <= trace_level) printf("Search() no moves!  ply=1\n");
#endif
    }
    return(value);
  }
  else {
    History(tree,1,depth,wtm,tree->pv[1].path[1]);
    return(alpha);
  }
}

/* modified 03/11/98 */
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
void SearchOutput(TREE *tree, int value, int bound) {
  register int wtm;
  int i;
  ROOT_MOVE temp_rm;
/*
 ----------------------------------------------------------
|                                                          |
|   first, move the best move to the top of the ply-1 move |
|   list if it's not already there, so that it will be the |
|   first move tried in the next iteration.                |
|                                                          |
 ----------------------------------------------------------
*/
  root_print_ok=root_print_ok || tree->nodes_searched>noise_level ||
                abs(value)>MATE-300;
  wtm=root_wtm;
  if (!abort_search) {
    whisper_value=(analyze_mode && !root_wtm) ? -value : value;
    whisper_depth=iteration_depth;
    for (i=0;i<n_root_moves;i++)
      if (tree->current_move[1] == root_moves[i].move) break;
    if (i < n_root_moves) {
      temp_rm=root_moves[i];
      for (;i>0;i--) root_moves[i]=root_moves[i-1];
      root_moves[0]=temp_rm;
      easy_move=0;
    }
    end_time=ReadClock(time_type);
/*
 ----------------------------------------------------------
|                                                          |
|   if this is not a fail-high move, then output the PV    |
|   by walking down the path being backed up.              |
|                                                          |
 ----------------------------------------------------------
*/
    if(value < bound) {
      UnMakeMove(tree,1,tree->pv[1].path[1],root_wtm);
      DisplayPV(tree,6, wtm, end_time-start_time, value, &tree->pv[1]);
      MakeMove(tree,1,tree->pv[1].path[1],root_wtm);
    }
    else {
      if (tree->current_move[1] != tree->pv[1].path[1]) {
        tree->pv[1].path[1]=tree->current_move[1];
        tree->pv[1].pathl=1;
        tree->pv[1].pathh=0;
        tree->pv[1].pathd=iteration_depth;
      }
    }
    tree->pv[0]=tree->pv[1];
    local[0]->pv[0]=tree->pv[1];
  }
}

/* modified 03/11/98 */
/*
********************************************************************************
*                                                                              *
*   SearchTrace() is used to print the search trace output each time a node is *
*   traversed in the tree.                                                     *
*                                                                              *
********************************************************************************
*/
void SearchTrace(TREE *tree, int ply, int depth, int wtm,
                 int alpha, int beta, char* name, int phase) {
  int i;
  Lock(lock_io);
  for (i=1;i<ply;i++) printf("  ");
  printf("%d  %s d:%5.2f [%s,",
         ply,OutputMove(tree,tree->current_move[ply],ply,wtm),
         (float) depth/ (float) INCPLY,DisplayEvaluation(alpha));
  printf("%s] n:%d %s(%d)", DisplayEvaluation(beta),
         (tree->nodes_searched),name,phase);
  if (max_threads > 1) printf(" (t=%d) ",tree->thread_id);
  printf("\n");
  UnLock(lock_io);
}
