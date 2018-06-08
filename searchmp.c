#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "chess.h"
#include "data.h"
#include "epdglue.h"

/* modified 10/10/01 */
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
              int depth, int ply, int mate_threat, int lp_recapture) {
  register int extensions, extended, recapture;
  BITBOARD begin_root_nodes;
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
      tree->phase[ply]=NextRootMove(tree->parent,tree,wtm);
      tree->root_move=tree->parent->root_move;
    }
    else
      tree->phase[ply]=(tree->in_check[ply]) ? 
                               NextEvasion((TREE*)tree->parent,ply,wtm) : 
                               NextMove((TREE*)tree->parent,ply,wtm);
    tree->current_move[ply]=tree->parent->current_move[ply];
    UnLock(tree->parent->lock);
    if (!tree->phase[ply]) break;
    extended=0;
#if defined(TRACE)
    if (ply <= trace_level)
      SearchTrace(tree,ply,depth,wtm,alpha,beta,"SearchSMP",tree->phase[ply]);
#endif
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
        tree->check_extensions_done++;
        extended+=incheck_depth;
      }
      else tree->in_check[ply+1]=0;
/*
 ----------------------------------------------------------
|                                                          |
|   if the null move found that the side on move gets      |
|   mated by not moving, then there must be some strong    |
|   threat at this position.  extend the search to make    |
|   sure it is analyzed carefully.                         |
|                                                          |
 ----------------------------------------------------------
*/
      if (mate_threat) {
        extended+=mate_depth;
        tree->mate_extensions_done++;
      }
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
      recapture=0;
      if (ply>1 && Captured(tree->current_move[ply]) &&
          Captured(tree->current_move[ply-1]) &&
          To(tree->current_move[ply-1]) == To(tree->current_move[ply]) &&
          (p_values[Captured(tree->current_move[ply-1])+7] == 
           p_values[Captured(tree->current_move[ply])+7] ||
           Promote(tree->current_move[ply-1])) &&
          !lp_recapture) {
        tree->recapture_extensions_done++;
        extended+=recap_depth;
        recapture=1;
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
        tree->passed_pawn_extensions_done++;
        extended+=pushpp_depth;
      }
/*
 ----------------------------------------------------------
|                                                          |
|   now it is time to call Search()/Quiesce to find out if |
|   this move is reasonable or not.                        |
|                                                          |
 ----------------------------------------------------------
*/
      begin_root_nodes=tree->nodes_searched;
      if (extended) {
        if (extended > INCPLY) extended=INCPLY;
        if (ply > 2*iteration_depth) extended>>=1;
      }
      extensions=extended-INCPLY;
      if (depth+extensions >= INCPLY)
        value=-Search(tree,-alpha-1,-alpha,ChangeSide(wtm),
                      depth+extensions,ply+1,DO_NULL,recapture);
      else
        value=-Quiesce(tree,-alpha-1,-alpha,ChangeSide(wtm),ply+1);
      if (abort_search || tree->stop) {
        UnmakeMove(tree,ply,tree->current_move[ply],wtm);
        break;
      }
      if (value>alpha && value<beta) {
        if (depth+extensions >= INCPLY)
          value=-Search(tree,-beta,-alpha,ChangeSide(wtm),
                        depth+extensions,ply+1,DO_NULL,recapture);
        else
          value=-Quiesce(tree,-beta,-alpha,ChangeSide(wtm),ply+1);
        if (abort_search || tree->stop) {
          UnmakeMove(tree,ply,tree->current_move[ply],wtm);
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
            root_value=value;
          }
          UnLock(lock_root);
        }
        if(value >= beta) {
          register int proc;
          parallel_stops++;
          UnmakeMove(tree,ply,tree->current_move[ply],wtm);
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
    UnmakeMove(tree,ply,tree->current_move[ply],wtm);
    tree->search_value=alpha;
  }
  tree->parent->done=1;
  if (tree->stop && ply==1) root_moves[tree->root_move].status&=255-128;
  return(0);
}
#endif
