#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "chess.h"
#include "data.h"
#include "epdglue.h"

/* last modified 12/19/98 */
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
int Search(TREE *tree, int alpha, int beta, int wtm, int depth,
           int ply, int do_null) {
  register int moves_searched=0;
  register BITBOARD save_hash_key;
  register int o_alpha, value=0;
  register int extensions, pieces;
  int threat=0;
/*
 ----------------------------------------------------------
|                                                          |
|   check to see if we have searched enough nodes that it  |
|   is time to peek at how much time has been used, or if  |
|   is time to check for operator keyboard input.  this is |
|   usually enough nodes to force a time/input check about |
|   once per second, except when the target time per move  |
|   is very small, in which case we try to check the time  |
|   at least 10 times during the search.                   |
|                                                          |
 ----------------------------------------------------------
*/
  tree->nodes_searched++;
  if (--next_time_check <= 0) {
    next_time_check=nodes_between_time_checks;
    if (CheckInput()) Interrupt(ply);
    time_abort+=TimeCheck(0);
    if (time_abort) {
      abort_search=1;
      return(0);
    }
  }
  if (ply >= MAXPLY-1) return(beta);
/*
 ----------------------------------------------------------
|                                                          |
|   check for draw by repetition.                          |
|                                                          |
 ----------------------------------------------------------
*/
  if (RepetitionCheck(tree,ply,wtm)) {
    value=DrawScore(root_wtm==wtm);
    if (value < beta) SavePV(tree,ply,value,0);
#if !defined(FAST)
    if(ply <= trace_level) printf("draw by repetition detected, ply=%d.\n",ply);
#endif
    return(value);
  }
/*
 ----------------------------------------------------------
|                                                          |
|   now call HashProbe() to see if this position has been  |
|   searched before.  if so, we may get a real score,      |
|   produce a cutoff, or get nothing more than a good move |
|   to try first.  there are four cases to handle:         |
|                                                          |
|   1. HashProbe() returned "EXACT" if this score is       |
|   greater than beta, return beta.  otherwise, return the |
|   score.  In either case, no further searching is needed |
|   from this position.  note that lookup verified that    |
|   the table position has sufficient "draft" to meet the  |
|   requirements of the current search depth remaining.    |
|                                                          |
|   2. HashProbe() returned "UPPER" which means that       |
|   when this position was searched previously, every move |
|   was "refuted" by one of its descendents.  as a result, |
|   when the search was completed, we returned alpha at    |
|   that point.  we simply return alpha here as well.      |
|                                                          |
|   3. HashProbe() returned "LOWER" which means that       |
|   when we encountered this position before, we searched  |
|   one branch (probably) which promptly refuted the move  |
|   at the previous ply.                                   |
|                                                          |
|   4. HashProbe() returned "AVOID_NULL_MOVE" which means  |
|   the hashed score/bound was no good, but it indicated   |
|   that trying a null-move in this position will be a     |
|   waste of time.                                         |
|                                                          |
 ----------------------------------------------------------
*/
  switch (HashProbe(tree,ply,depth,wtm,&alpha,&beta,&threat)) {
    case EXACT:
      if(alpha < beta) SavePV(tree,ply,alpha,1);
      return(alpha);
    case LOWER:
      return(beta);
    case UPPER:
      return(alpha);
    case AVOID_NULL_MOVE:
      do_null=0;
  }
/*
 ----------------------------------------------------------
|                                                          |
|   now it's time to try a probe into the endgame table-   |
|   base files.  this is done if (a) the previous move was |
|   a capture or promotion, unless we are at very shallow  |
|   plies (<4) in the search; (b) there are less than 5    |
|   pieces left (currently all interesting 4 piece endings |
|   are available.)                                        |
|                                                          |
 ----------------------------------------------------------
*/
  if (TotalPieces<=EGTB_use && ply<EGTB_maxdepth) {
    int egtb_value;
    tree->egtb_probes++;
    if (EGTBProbe(tree, ply, wtm, &egtb_value)) {
      tree->egtb_probes_successful++;
      alpha=egtb_value;
      if (abs(alpha) > MATE-300) alpha+=(alpha > 0) ? -(ply-1) : +(ply);
      else if (alpha == 0) alpha=DrawScore(root_wtm==wtm);
      if(alpha < beta) SavePV(tree,ply,alpha,2);
      return(alpha);
    }
  }
/*
 ----------------------------------------------------------
|                                                          |
|   initialize.                                            |
|                                                          |
 ----------------------------------------------------------
*/
  tree->in_check[ply+1]=0;
  tree->extended_reason[ply+1]=no_extension;
  o_alpha=alpha;
  tree->last[ply]=tree->last[ply-1];
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
|                                                          |
|  the null-move search is also used to detect certain     |
|  types of threats.  the original idea of using the value |
|  returned by the null-move search was reported by C.     |
|  donninger, but was modified by Bruce Moreland (ferret)  |
|  in the following way:  if the null-move search returns  |
|  a score that says "mated in N" then this position is a  |
|  dangerous one, because not moving gets the side to move |
|  mated.  we extend the search one ply in this case, al-  |
|  though, as always, no more than one ply of extensions   |
|  are allowed at any one level in the tree.  note also    |
|  that this "threat" condition is hashed so that later,   |
|  if the hash table says "don't try the null move because |
|  it likely will fail low, we still know that this is a   |
|  threat position and should be extended.                 |
|                                                          |
 ----------------------------------------------------------
*/
# if defined(NULL_MOVE_DEPTH)
  pieces=(wtm) ? TotalWhitePieces : TotalBlackPieces;
  if (do_null && !tree->in_check[ply] && pieces && (pieces>5 || depth<421)) {
    tree->current_move[ply]=0;
    tree->current_phase[ply]=NULL_MOVE;
#if !defined(FAST)
    if (ply <= trace_level)
      SearchTrace(tree,ply,depth,wtm,beta-1,beta,"Search",0);
#endif
    tree->position[ply+1]=tree->position[ply];
    Rule50Moves(ply+1)++;
    save_hash_key=HashKey;
    if (EnPassant(ply)) {
      HashEP(EnPassant(ply+1),HashKey);
      EnPassant(ply+1)=0;
    }
    value=-ABSearch(tree,-beta,-beta+1,ChangeSide(wtm),
                    depth-NULL_MOVE_DEPTH-INCPLY,ply+1,NO_NULL);
    HashKey=save_hash_key;
    if (abort_search || tree->stop) return(0);
    if (value >= beta) {
      HashStore(tree,ply,depth,wtm,LOWER,value,threat);
      return(value);
    }
    if (value < -MATE+30) threat=1;
  }
# endif
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
  tree->next_status[ply].phase=FIRST_PHASE;
  if (tree->hash_move[ply]==0 && (depth > 2*INCPLY) &&
      (((ply & 1) && alpha == root_alpha && beta == root_beta) ||
      (!(ply & 1) && alpha == -root_beta && beta == -root_alpha))) {
    tree->current_move[ply]=0;
    value=ABSearch(tree,alpha,beta,wtm,depth-2*INCPLY,ply,0);
    if (abort_search || tree->stop) return(0);
    if (value <= alpha) {
      value=ABSearch(tree,-MATE,beta,wtm,depth-2*INCPLY,ply,0);
      if (abort_search || tree->stop) return(0);
      if (value > -MATE && (int) tree->pv[ply-1].pathl >= ply) 
        tree->hash_move[ply]=tree->pv[ply-1].path[ply];
    }
    else if (value < beta) {
      if ((int) tree->pv[ply-1].pathl >= ply) 
        tree->hash_move[ply]=tree->pv[ply-1].path[ply];
    }
    else tree->hash_move[ply]=tree->current_move[ply];
    tree->last[ply]=tree->last[ply-1];
    tree->next_status[ply].phase=FIRST_PHASE;
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
  while ((tree->current_phase[ply]=(tree->in_check[ply]) ?
         NextEvasion(tree,ply,wtm) : NextMove(tree,ply,wtm))) {
    tree->extended_reason[ply]&=check_extension;
#if !defined(FAST)
    if (ply <= trace_level)
      SearchTrace(tree,ply,depth,wtm,alpha,beta,"Search",tree->current_phase[ply]);
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
    if (threat) 
      extensions+=(ply<=2*iteration_depth) ? threat_depth : threat_depth>>1;
    if (Captured(tree->current_move[ply]) && Captured(tree->current_move[ply-1]) &&
        To(tree->current_move[ply-1]) == To(tree->current_move[ply]) &&
        (p_values[Captured(tree->current_move[ply-1])+7] == 
         p_values[Captured(tree->current_move[ply])+7] ||
         Promote(tree->current_move[ply-1])) &&
        !(tree->extended_reason[ply-1]&recapture_extension)) {
      tree->extended_reason[ply]|=recapture_extension;
      tree->recapture_extensions_done++;
      extensions+=(ply<=2*iteration_depth) ? recap_depth : recap_depth>>1;
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
      extensions+=(ply<=2*iteration_depth) ? pushpp_depth : pushpp_depth>>1;
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
        extensions+=(ply<=2*iteration_depth) ? incheck_depth : incheck_depth>>1;
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
        register const int value=-Evaluate(tree,ply+1,ChangeSide(wtm),
                                           -(beta+51),-(alpha-51));
        if (value+50 < alpha) extensions-=60;
      }
/*
 ----------------------------------------------------------
|                                                          |
|   if there's only one legal move, extend the search one  |
|   additional ply since this node is very easy to search. |
|                                                          |
 ----------------------------------------------------------
*/
      if (!moves_searched) {
        if (tree->in_check[ply] && tree->last[ply]-tree->last[ply-1] == 1) {
          tree->extended_reason[ply]|=one_reply_extension;
          tree->one_reply_extensions_done++;
          extensions+=(ply<=2*iteration_depth) ? onerep_depth : onerep_depth>>1;
        }
        extensions=Min(extensions,0);
        value=-ABSearch(tree,-beta,-alpha,ChangeSide(wtm),
                        depth+extensions,ply+1,DO_NULL);
        if (abort_search || tree->stop) {
          UnMakeMove(tree,ply,tree->current_move[ply],wtm);
          return(0);
        }
      }
      else {
        extensions=Min(extensions,0);
        value=-ABSearch(tree,-alpha-1,-alpha,ChangeSide(wtm),
                        depth+extensions,ply+1,DO_NULL);
        if (abort_search || tree->stop) {
          UnMakeMove(tree,ply,tree->current_move[ply],wtm);
          return(0);
        }
        if (value>alpha && value<beta) {
          value=-ABSearch(tree,-beta,-alpha,ChangeSide(wtm),
                          depth+extensions,ply+1,DO_NULL);
          if (abort_search || tree->stop) {
            UnMakeMove(tree,ply,tree->current_move[ply],wtm);
            return(0);
          }
        }
      }
      if (value > alpha) {
        if(value >= beta) {
          History(tree,ply,depth,wtm,tree->current_move[ply]);
          UnMakeMove(tree,ply,tree->current_move[ply],wtm);
          HashStore(tree,ply,depth,wtm,LOWER,value,threat);
          tree->fail_high++;
          if (!moves_searched) tree->fail_high_first++;
          return(value);
        }
        alpha=value;
      }
      moves_searched++;
    } else tree->nodes_searched++;
    UnMakeMove(tree,ply,tree->current_move[ply],wtm);
#if defined(SMP)
    if (smp_idle) {
      if (moves_searched && min_thread_depth<=depth && 
        (!tree->in_check[ply] || tree->last[ply]-tree->last[ply-1]>1)) {
        tree->alpha=alpha;
        tree->beta=beta;
        tree->value=value;
        tree->wtm=wtm;
        tree->ply=ply;
        tree->depth=depth;
        tree->threat=threat;
        if(Thread(tree)) {
          if (CheckInput()) Interrupt(ply);
          value=tree->search_value;
          if (value > alpha) {
            if(value >= beta) {
              History(tree,ply,depth,wtm,tree->current_move[ply]);
              HashStore(tree,ply,depth,wtm,LOWER,value,threat);
              tree->fail_high++;
              return(value);
            }
            alpha=value;
            break;
          }
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
  if (moves_searched == 0) {
    value=(Check(wtm)) ? -(MATE-ply) : DrawScore(root_wtm==wtm);
    if (value>=alpha && value<beta) {
      SavePV(tree,ply,value,0);
#if !defined(FAST)
      if (ply <= trace_level) printf("Search() no moves!  ply=%d\n",ply);
#endif
    }
    return(value);
  }
  else {
    if (alpha != o_alpha) {
      memcpy(&tree->pv[ply-1].path[ply],&tree->pv[ply].path[ply],(tree->pv[ply].pathl-ply+1)*4);
      memcpy(&tree->pv[ply-1].pathh,&tree->pv[ply].pathh,3);
      tree->pv[ply-1].path[ply-1]=tree->current_move[ply-1];
      History(tree,ply,depth,wtm,tree->pv[ply].path[ply]);
    }
    HashStore(tree,ply,depth,wtm,(alpha==o_alpha)?UPPER:EXACT,alpha,threat);
    return(alpha);
  }
}
