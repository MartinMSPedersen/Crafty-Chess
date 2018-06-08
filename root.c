#include <stdio.h>
#include <stdlib.h>
#include "chess.h"
#include "data.h"

/* last modified 07/14/96 */
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
void RootMoveList(int wtm)
{
  BITBOARD target;
  int *mvp, tempm;
  int square, side, i, done, temp, value;

/*
 ----------------------------------------------------------
|                                                          |
|   first, use GenerateMoves() to generate the set of      |
|   legal moves from the root position.                    |
|                                                          |
 ----------------------------------------------------------
*/
  easy_move=0;
  target=(wtm) ? Compl(WhitePieces) : Compl(BlackPieces);
  last[1]=GenerateMoves(1, 1, wtm, target, 1, last[0]);
  if (last[1] == last[0]+1) return;
/*
 ----------------------------------------------------------
|                                                          |
|   now make each move and use Evaluate() to compute the   |
|   positional evaluation.                                 |
|                                                          |
 ----------------------------------------------------------
*/
  for (mvp=last[0];mvp<last[1];mvp++) {
    value=-4000000;
    MakeMove(1, *mvp, wtm);
    if (!Check(wtm)) {
      current_move[1]=*mvp;
      value=-Evaluate(2,ChangeSide(wtm),-99999,99999);
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
      if (Promote(*mvp) && (Promote(*mvp) != queen)) value-=500;
    }
    root_sort_value[mvp-last[0]]=value;
    UnMakeMove(1, *mvp, wtm);
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
    for (i=0;i<last[1]-last[0]-1;i++) {
      if (root_sort_value[i] < root_sort_value[i+1]) {
        temp=root_sort_value[i];
        root_sort_value[i]=root_sort_value[i+1];
        root_sort_value[i+1]=temp;
        tempm=*(last[0]+i);
        *(last[0]+i)=*(last[0]+i+1);
        *(last[0]+i+1)=tempm;
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
  for (;last[1]>last[0];last[1]--) 
    if (root_sort_value[last[1]-last[0]-1] > -3000000) break;
  if (root_sort_value[0] > 1000000) root_sort_value[0]-=2000000;
  if (root_sort_value[0] > root_sort_value[1]+2000 &&
      ((To(*last[0]) == To(last_opponent_move) &&
        Captured(*last[0]) == Piece(last_opponent_move)) || 
      root_sort_value[0] < PAWN_VALUE)) easy_move=1;
  if (trace_level > 0) {
    printf("produced %d moves at root\n",last[1]-last[0]);
    for (mvp=last[0];mvp<last[1];mvp++) {
      current_move[1]=*mvp;
      printf("%s",OutputMove(mvp,1,wtm));
      MakeMove(1, *mvp, wtm);
      printf("/%d/%d  ",root_sort_value[mvp-last[0]],
             -Evaluate(2,ChangeSide(wtm),-99999,99999));
      if (!((mvp-last[0]+1) % 5)) printf("\n");
      UnMakeMove(1, *mvp, wtm);
    }
  }
  return;
}
