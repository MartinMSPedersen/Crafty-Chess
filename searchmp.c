#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "chess.h"
#include "data.h"
#include "epdglue.h"

/* modified 04/19/99 */
/*
********************************************************************************
*                                                                              *
*   SearchSMP() is the recursive routine used to implement the alpha/beta      *
*   negamax search using parallel threads.  when this code is called, the      *
*   first move has already been searched, so all that is left is to search the *
*   remainder of the moves and then return.  note that the hash table and such *
*   can't be modified here since this only represents a part of the search at  *
*   this ply.                                                                  *
*                                                                              *
********************************************************************************
*/
#if defined(SMP)
int SearchSMP(TREE *tree, int alpha, int beta, int value, int wtm,
              int depth, int ply, int threat) {
  register int extensions, begin_root_nodes;
  register int full_extension=ply<=2*iteration_depth;
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
  while (1) {
    Lock(tree->parent->lock);
    if (ply == 1) {
      tree->current_phase[ply]=NextRootMove((TREE*)tree->parent,wtm);
      tree->root_move=tree->parent->root_move;
    }
    else
      tree->current_phase[ply]=(tree->in_check[ply]) ? 
                               NextEvasion((TREE*)tree->parent,ply,wtm) : 
                               NextMove((TREE*)tree->parent,ply,wtm);
    tree->current_move[ply]=tree->parent->current_move[ply];
    UnLock(tree->parent->lock);
    if (!tree->current_phase[ply]) break;
    tree->extended_reason[ply]&=check_extension;
#if defined(TRACE)
    if (ply <= trace_level)
      SearchTrace(tree,ply,depth,wtm,alpha,beta,"SearchSMP",tree->current_phase[ply]);
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
    extensions=-60;
    if (threat) {
      extensions+=threat_depth;
      tree->threat_extensions_done++;
    }
    if (ply>1 && Captured(tree->current_move[ply]) &&
        Captured(tree->current_move[ply-1]) &&
        To(tree->current_move[ply-1]) == To(tree->current_move[ply]) &&
        (p_values[Captured(tree->current_move[ply-1])+7] == 
         p_values[Captured(tree->current_move[ply])+7] ||
         Promote(tree->current_move[ply-1])) &&
        !(tree->extended_reason[ply-1]&recapture_extension)) {
      tree->extended_reason[ply]|=recapture_extension;
      tree->recapture_extensions_done++;
      extensions+=(full_extension) ? recap_depth : recap_depth>>1;
    }
/*
 ----------------------------------------------------------
|                                                          |
|   if we push a passed pawn, we need to look deeper to    |
|   see if it is a legitimate threat.                      |
|                                                          |
 ----------------------------------------------------------
*/
    if (Piece(tree->current_move[ply])==pawn &&
      push_extensions[To(tree->current_move[ply])]) {
      tree->extended_reason[ply]|=passed_pawn_extension;
      tree->passed_pawn_extensions_done++;
      extensions+=(full_extension) ? pushpp_depth : pushpp_depth>>1;
    }
/*
 ----------------------------------------------------------
|                                                          |
|   now make the move and search the resulting position.   |
|   if we are in check, the current move must be legal     |
|   since NextEvasion ensures this, otherwise we have to   |
|   make sure the side-on-move is not in check after the   |
|   move to weed out illegal moves and save time.          |
|                                                          |
 ----------------------------------------------------------
*/
    MakeMove(tree,ply,tree->current_move[ply],wtm);
    if (tree->in_check[ply] || !Check(wtm)) {
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
        tree->in_check[ply+1]=1;
        tree->extended_reason[ply+1]=check_extension;
        tree->check_extensions_done++;
        extensions+=(full_extension) ? incheck_depth : incheck_depth>>1;
      }
      else {
        tree->in_check[ply+1]=0;
        tree->extended_reason[ply+1]=no_extension;
      }
/*
 ----------------------------------------------------------
|                                                          |
|   now we toss in the "razoring" trick, which simply says |
|   if we are doing fairly badly, we can reduce the depth  |
|   an additional ply, if there was nothing at the current |
|   ply that caused an extension.                          |
|                                                          |
 ----------------------------------------------------------
*/
      if (depth<3*INCPLY && depth>=2*INCPLY &&
          !tree->in_check[ply] && extensions == -60) {
        register int value=-Evaluate(tree,ply+1,ChangeSide(wtm),
                                     -(beta+51),-(alpha-51));
        if (value+50 < alpha) extensions-=60;
      }
      begin_root_nodes=tree->nodes_searched;
      extensions=Min(extensions,0);
      value=-ABSearch(tree,-alpha-1,-alpha,ChangeSide(wtm),
                      depth+extensions,ply+1,DO_NULL);
      if (abort_search || tree->stop) {
        UnMakeMove(tree,ply,tree->current_move[ply],wtm);
        break;
      }
      if (value>alpha && value<beta) {
        value=-ABSearch(tree,-beta,-alpha,ChangeSide(wtm),
                        depth+extensions,ply+1,DO_NULL);
        if (abort_search || tree->stop) {
          UnMakeMove(tree,ply,tree->current_move[ply],wtm);
          break;
        }
      }
/*
 ----------------------------------------------------------
|                                                          |
|   now we check for an undesirable case, that of failing  |
|   high while doing a parallel (threaded) search.  this   |
|   means our 'helpers' are doing stuff that is not needed |
|   so we 'stop' them now.                                 |
|                                                          |
 ----------------------------------------------------------
*/
      if (ply == 1)
        root_moves[tree->root_move].nodes=tree->nodes_searched-begin_root_nodes;
      if (value > alpha) {
        if (ply == 1) {
          Lock(lock_root);
          if (value > root_value) {
            SearchOutput(tree,value,beta);
            tree->parent->pv[0]=tree->pv[0];
            root_value=value;
          }
          UnLock(lock_root);
        }
        if(value >= beta) {
          register int proc;
          parallel_stops++;
          UnMakeMove(tree,ply,tree->current_move[ply],wtm);
          tree->search_value=value;
          Lock(lock_smp);
          Lock(tree->parent->lock);
          if (!tree->stop) {
            for (proc=0;proc<max_threads;proc++)
              if (tree->parent->siblings[proc] && proc != tree->thread_id)
                ThreadStop(tree->parent->siblings[proc]);
          }
          UnLock(tree->parent->lock);
          UnLock(lock_smp);
          break;
        }
        alpha=value;
      }
    }
    UnMakeMove(tree,ply,tree->current_move[ply],wtm);
    tree->search_value=alpha;
  }
  tree->parent->done=1;
  if (tree->stop && ply==1) root_moves[tree->root_move].status&=255-128;
  return(0);
}
#endif
