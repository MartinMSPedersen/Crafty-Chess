#include <stdio.h>
#include <stdlib.h>
#include "types.h"
#include "function.h"
#include "data.h"
/*
********************************************************************************
*                                                                              *
*   Root_Move_List() is used to set up the ply one move list.  it is a  more   *
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
void Root_Move_List(int wtm)
{
  BITBOARD target;
  CHESS_BOARD board;
  int *mvp, tempm;
  int square, i, done, temp, value;

/*
 ----------------------------------------------------------
|                                                          |
|   first, use Generate_Moves() to generate the set of     |
|   legal moves from the root position.                    |
|                                                          |
 ----------------------------------------------------------
*/
  easy_move=0;
  first[1]=last[0];
  last[1]=first[1];
  if (wtm)
    target=Compl(White_Pieces(1));
  else
    target=Compl(Black_Pieces(1));
  last[1]=Generate_Moves(1, 1, wtm, target, 1, last[1]);
  if (last[1] == first[1]+1) return;
/*
 ----------------------------------------------------------
|                                                          |
|   now make each move and use Evaluate() to compute the   |
|   positional evaluation.                                 |
|                                                          |
 ----------------------------------------------------------
*/
  for (mvp=first[1];mvp<last[1];mvp++) {
    value=-4000000;
    Make_Move(1, *mvp, wtm);
    positional_evaluation[2]=0;
    if (!Check(2,wtm)) {
      value=Evaluate(2,wtm,-99999,99999);
      board=position[2].board;
/*
 ----------------------------------------------------------
|                                                          |
|   now use Sort() to analyze the state of all the         |
|   friendly pieces to see what appears to be hanging when |
|   we try the current move.                               |
|                                                          |
 ----------------------------------------------------------
*/
/*
   now check queens
*/
      if (wtm)
        while(board.w_queen) {
          square=First_One(board.w_queen);
          value-=Max(EnPrise(2,square,!wtm),0);
          Clear(square,board.w_queen);
        }
      else
        while(board.b_queen) {
          square=First_One(board.b_queen);
          value-=Max(EnPrise(2,square,!wtm),0);
          Clear(square,board.b_queen);
        }
/*
   now check rooks
*/
      if (wtm)
        while(board.w_rook) {
          square=First_One(board.w_rook);
          value-=Max(EnPrise(2,square,!wtm),0);
          Clear(square,board.w_rook);
        }
      else
        while(board.b_rook) {
          square=First_One(board.b_rook);
          value-=Max(EnPrise(2,square,!wtm),0);
          Clear(square,board.b_rook);
        }
/*
   now check bishops
*/
      if (wtm)
        while(board.w_bishop) {
          square=First_One(board.w_bishop);
          value-=Max(EnPrise(2,square,!wtm),0);
          Clear(square,board.w_bishop);
        }
      else
        while(board.b_bishop) {
          square=First_One(board.b_bishop);
          value-=Max(EnPrise(2,square,!wtm),0);
          Clear(square,board.b_bishop);
        }
/*
   now check knights
*/
      if (wtm)
        while(board.w_knight) {
          square=First_One(board.w_knight);
          value-=Max(EnPrise(2,square,!wtm),0);
          Clear(square,board.w_knight);
        }
      else
        while(board.b_knight) {
          square=First_One(board.b_knight);
          value-=Max(EnPrise(2,square,!wtm),0);
          Clear(square,board.b_knight);
        }
/*
   now check pawns
*/
      if (wtm)
        while(board.w_pawn) {
          square=First_One(board.w_pawn);
          value-=Max(EnPrise(2,square,!wtm),0);
          Clear(square,board.w_pawn);
        }
      else
        while(board.b_pawn) {
          square=First_One(board.b_pawn);
          value-=Max(EnPrise(2,square,!wtm),0);
          Clear(square,board.b_pawn);
        }
/*
 ----------------------------------------------------------
|                                                          |
|   add in a bonus if this move is part of the previous    |
|   principal variation.  it was good in the search, we    |
|   should try it first now.                               |
|                                                          |
 ----------------------------------------------------------
*/

      if((Piece(*mvp) == Piece(pv[1].path[1])) &&
         (From(*mvp) == From(pv[1].path[1])) &&
         (To(*mvp) == To(pv[1].path[1])) &&
         (Captured(*mvp) == 
          Captured(pv[1].path[1])) &&
         (Promote(*mvp) == Promote(pv[1].path[1])))
        value+=2000000;
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
      if (Promote(*mvp) && (Promote(*mvp) != 5)) value-=50;
    }
    root_sort_value[mvp-first[1]]=value;
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
    for (i=0;i<last[1]-first[1]-1;i++) {
      if (root_sort_value[i] < root_sort_value[i+1]) {
        temp=root_sort_value[i];
        root_sort_value[i]=root_sort_value[i+1];
        root_sort_value[i+1]=temp;
        tempm=*(first[1]+i);
        *(first[1]+i)=*(first[1]+i+1);
        *(first[1]+i+1)=tempm;
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
  for (;last[1]>first[1];last[1]--) 
    if (root_sort_value[last[1]-first[1]-1] > -3000000) break;
  if (root_sort_value[0] > 1000000) root_sort_value[0]-=2000000;
  if ((root_sort_value[0] > root_sort_value[1]+800)) easy_move=1;
  if (trace_level > 0) {
    printf("produced %d moves at root\n",last[1]-first[1]);
    for (mvp=first[1];mvp<last[1];mvp++) {
      Make_Move(1, *mvp, wtm);
      positional_evaluation[2]=0;
      printf("%s/%d/%d  ",Output_Move(mvp,1,wtm),root_sort_value[mvp-first[1]],
             Evaluate(2,wtm,-99999,99999));
      if (!((mvp-first[1]+1) % 5)) printf("\n");
    }
  }
  return;
}
