#include <stdio.h>
#include <stdlib.h>
#include "chess.h"
#include "data.h"
#include "epdglue.h"

/* last modified 04/09/99 */
/*
********************************************************************************
*                                                                              *
*   RootMoveList() is used to set up the ply one move list.  it is a  more     *
*   accurate ordering of the move list than that done for plies deeper than    *
*   one.  briefly, Evaluate() is used to evaluate the positional/material      *
*   score for each move and then the status of friendly pieces is added in.    *
*   EnPrise() evaluates the safety of each friendly piece by noticing          *
*   attackers and defenders and returning the expected loss. Root_Moves()      *
*   is only called once before the iterated search is started.  as the         *
*   iterations progress, the moves in the root move list are continually       *
*   re-arranged as scores are backed up to the root of the tree.               *
*                                                                              *
********************************************************************************
*/
void RootMoveList(int wtm) {
  int *mvp, *lastm, rmoves[256];
  int square, i, side, done, temp, value;
  TREE * const tree=local[0];
  int tb_value;
/*
 ----------------------------------------------------------
|                                                          |
|   if the position at the root is a draw, based on EGTB   |
|   results, we are going to behave differently.  we will  |
|   extract the root moves that are draws, and toss the    |
|   losers out.  then, we will do a normal search on the   |
|   moves that draw to try and chose the most difficult    |
|   drawing move.                                          |
|                                                          |
 ----------------------------------------------------------
*/
  EGTB_draw=0;
  if (swindle_mode && EGTBlimit && TotalPieces<=5 &&
      EGTBProbe(tree, 1, wtm, &tb_value)) {
    if (tb_value == 0)
      if ((wtm && Material>0) || (!wtm && Material<0)) EGTB_draw=1;
  }
/*
 ----------------------------------------------------------
|                                                          |
|   first, use GenerateMoves() to generate the set of      |
|   legal moves from the root position.                    |
|                                                          |
 ----------------------------------------------------------
*/
  easy_move=0;
  lastm=GenerateCaptures(tree, 1, wtm, rmoves);
  lastm=GenerateNonCaptures(tree, 1, wtm, lastm);
  n_root_moves=lastm-rmoves;
/*
 ----------------------------------------------------------
|                                                          |
|   now make each move and use Evaluate() to compute the   |
|   positional evaluation.                                 |
|                                                          |
 ----------------------------------------------------------
*/
  for (mvp=rmoves;mvp<lastm;mvp++) {
    value=-4000000;
    MakeMove(tree, 1, *mvp, wtm);
    if (!Check(wtm)) do {
      if (EGTBlimit && EGTB_draw) {
        i=EGTBProbe(tree, 2, ChangeSide(wtm), &tb_value);
        if (tb_value != 0) break;
      }
      tree->current_move[1]=*mvp;
      value=-Evaluate(tree,2,ChangeSide(wtm),-99999,99999);
/*
 ----------------------------------------------------------
|                                                          |
|   now use EnPrise() to analyze the state of all the      |
|   friendly pieces to see what appears to be hanging when |
|   we try the current move.                               |
|                                                          |
 ----------------------------------------------------------
*/
      side=(wtm) ? 1 : -1;
      for (square=0;square<64;square++) 
        if (PieceOnSquare(square)*side > 0)
          value-=Max(EnPrise(square,ChangeSide(wtm)),0);
/*
 ----------------------------------------------------------
|                                                          |
|   add in a bonus if this move is part of the previous    |
|   principal variation.  it was good in the search, we    |
|   should try it first now.                               |
|                                                          |
 ----------------------------------------------------------
*/

      if((Piece(*mvp)    == Piece(last_pv.path[1])) &&
         (From(*mvp)     == From(last_pv.path[1])) &&
         (To(*mvp)       == To(last_pv.path[1])) &&
         (Captured(*mvp) == Captured(last_pv.path[1])) &&
         (Promote(*mvp)  == Promote(last_pv.path[1]))) {
        value+=2000000;
    }
/*
 ----------------------------------------------------------
|                                                          |
|   fudge the score for promotions so that promotion to a  |
|   queen is tried first.  since the positional score is   |
|   computed before the piece is removed, under-promoting  |
|   sometimes looks better.                                |
|                                                          |
 ----------------------------------------------------------
*/
      if (Promote(*mvp) && (Promote(*mvp) != queen)) value-=50;
    } while(0);
    tree->sort_value[mvp-rmoves]=value;
    UnMakeMove(tree, 1, *mvp, wtm);
  }
/*
 ----------------------------------------------------------
|                                                          |
|   now sort the moves into order based on the sum of the  |
|   positional score less the possible hung pieces, and    |
|   factoring in a bonus for following the PV move.        |
|                                                          |
 ----------------------------------------------------------
*/
  do {
    done=1;
    for (i=0;i<lastm-rmoves-1;i++) {
      if (tree->sort_value[i] < tree->sort_value[i+1]) {
        temp=tree->sort_value[i];
        tree->sort_value[i]=tree->sort_value[i+1];
        tree->sort_value[i+1]=temp;
        temp=rmoves[i];
        rmoves[i]=rmoves[i+1];
        rmoves[i+1]=temp;
        done=0;
      }
    }
  } while(!done);
/*
 ----------------------------------------------------------
|                                                          |
|   now trim the move list to eliminate those moves that   |
|   "hung" the king and are illegal.                       |
|                                                          |
 ----------------------------------------------------------
*/
  for (;n_root_moves;n_root_moves--)
    if (tree->sort_value[n_root_moves-1] > -3000000) break;
  if (tree->sort_value[0] > 1000000) tree->sort_value[0]-=2000000;
  if (tree->sort_value[0] > tree->sort_value[1]+200 &&
      ((To(rmoves[0]) == To(last_opponent_move) &&
        Captured(rmoves[0]) == Piece(last_opponent_move)) || 
      tree->sort_value[0] < PAWN_VALUE)) easy_move=1;
/*
 ----------------------------------------------------------
|                                                          |
|   debugging output to dump root move list and the stuff  |
|   used to sort them, for testing and debugging.          |
|                                                          |
 ----------------------------------------------------------
*/
  if (display_options & 512) {
    int score;
    int orig_score=Evaluate(tree,1,wtm,-99999,99999)-Material;
    Print(512,"%d moves at root\n",n_root_moves);
    Print(512,"        move   score      eval     (+/-)\n");
    for (i=0;i<n_root_moves;i++) {
      tree->current_move[1]=rmoves[i];
      Print(512,"%12s",OutputMove(tree,rmoves[i],1,wtm));
      MakeMove(tree, 1, rmoves[i], wtm);
      score=-Evaluate(tree,2,ChangeSide(wtm),-99999,99999);
      Print(512,"%8d  %8d  %8d\n",tree->sort_value[i], score,
            score-orig_score-Material);
      UnMakeMove(tree, 1, rmoves[i], wtm);
    }
  }
/*
 ----------------------------------------------------------
|                                                          |
|   now copy the root moves into the root_move structure   |
|   array for use by NextRootMove().                       |
|                                                          |
 ----------------------------------------------------------
*/
  for (i=0;i<n_root_moves;i++) {
    root_moves[i].move=rmoves[i];
    root_moves[i].nodes=0;
    root_moves[i].status=0;
  }
  return;
}
