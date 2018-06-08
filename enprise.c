#include <stdio.h>
#include <stdlib.h>
#include "types.h"
#include "function.h"
#include "data.h"
#include "evaluate.h"
/*
********************************************************************************
*                                                                              *
*   EnPrise() is used to analyze pieces (at the root of the search) to see if  *
*   they can be captured.  this information is then used to order the moves at *
*   the root of the tree to search moves that don't hang pieces first.         *
*                                                                              *
*   the algorithm is quite simple.  using Attacks_To(), we can enumerate all   *
*   the pieces that are attacking [target] for either side.  then we simply    *
*   use the lowest piece (value) for the correct side to capture on [target].  *
*   we continually "flip" sides taking the lowest piece each time.             *
*                                                                              *
*   as a piece is "used", if it is a sliding piece (pawn, bishop, rook or      *
*   queen) we use the Attacks_To(square) to see if a sliding piece is          *
*   attacking this piece in the same direction, meaning that the sliding piece *
*   can now be used in the swap sequence.  one final "fix" is that the piece   *
*   on <from> must be used first in the capture sequence (if [from] is a real  *
*   square.)                                                                   *
*                                                                              *
********************************************************************************
*/
int EnPrise(int ply, int target, int wtm)
{
  BITBOARD white_attackers, black_attackers;
  BITBOARD piece_squares, temp_attacks;
  BITBOARD *pawns[2], *knights[2], *bishops[2], 
           *rooks[2], *queens[2], *kings[2];
  int attacked_piece;
  int square;
  int swap_sign, color, next_capture=0;
  int swap_list[32];
/*
 ----------------------------------------------------------
|                                                          |
|   determine which squares attack <target> for each side. |
|                                                          |
 ----------------------------------------------------------
*/
  temp_attacks=Attacks_To(target,ply);
  white_attackers=And(temp_attacks,
                      White_Pieces(ply));
  black_attackers=And(temp_attacks,
                      Black_Pieces(ply));
/*
 ----------------------------------------------------------
|                                                          |
|   if the side-to-move isn't attacking <target> then we   |
|   are done.                                              |
|                                                          |
 ----------------------------------------------------------
*/
  if (wtm) {
    if (!white_attackers) return(0);
  }
  else {
    if (!black_attackers) return(0);
  }
/*
 ----------------------------------------------------------
|                                                          |
|   initialize by placing the piece on <target> first in   |
| the list as it is being captured to start things off.    |
|                                                          |
 ----------------------------------------------------------
*/
  swap_list[0]=0;
  attacked_piece=piece_values[abs(Piece_On_Square(ply,target))];
/*
 ----------------------------------------------------------
|                                                          |
|   no quick exit.  set up for scanning the list of pieces |
|   that attack <target> and play the "swaps" out.         |
|                                                          |
 ----------------------------------------------------------
*/
  pawns[0]=&Black_Pawns(ply);
  pawns[1]=&White_Pawns(ply);
  knights[0]=&Black_Knights(ply);
  knights[1]=&White_Knights(ply);
  bishops[0]=&Black_Bishops(ply);
  bishops[1]=&White_Bishops(ply);
  rooks[0]=&Black_Rooks(ply);
  rooks[1]=&White_Rooks(ply);
  queens[0]=&Black_Queens(ply);
  queens[1]=&White_Queens(ply);
  kings[0]=&Black_King(ply);
  kings[1]=&White_King(ply);
  swap_sign=1;
  piece_squares=Attacks_To(target,ply);
  color=wtm;
/*
 ----------------------------------------------------------
|                                                          |
|   now pick out the least valuable piece for the correct  |
|   side that is bearing on <target>.  as we find one, we  |
|   call Swap_Xray() to add the piece behind this piece    |
|   that is indirectly bearing on <target> (if any).       |
|                                                          |
 ----------------------------------------------------------
*/
  while (piece_squares) {
    if (And(*pawns[color],piece_squares))
      square=First_One(And(*pawns[color],piece_squares));
    else if (And(*knights[color],piece_squares))
      square=First_One(And(*knights[color],piece_squares));
    else if (And(*bishops[color],piece_squares))
      square=First_One(And(*bishops[color],piece_squares));
    else if (And(*rooks[color],piece_squares))
      square=First_One(And(*rooks[color],piece_squares));
    else if (And(*queens[color],piece_squares))
      square=First_One(And(*queens[color],piece_squares));
    else if (And(*kings[color],piece_squares))
      square=First_One(And(*kings[color],piece_squares));
    else 
      square=-1;
/*
 ------------------------------------------------
|                                                |
|  located the least valuable piece bearing on   |
|  <target>.  remove it from the list and then   |
|  find out what's behind it.                    |
|                                                |
 ------------------------------------------------
*/
    if (square < 0) break;
    if (next_capture)
      swap_list[next_capture]=swap_list[next_capture-1]+
                              swap_sign*attacked_piece;
    else
      swap_list[next_capture]=attacked_piece;
    attacked_piece=piece_values[abs(Piece_On_Square(ply,square))];
    Clear(square,piece_squares);
    piece_squares=Swap_Xray(ply,piece_squares,square,target);
    next_capture++;
    swap_sign=-swap_sign;
    color=!color;
  }
/*
  for (i=0;i<next_capture;i++)
    printf("swap_list[%d]=%d\n",i,swap_list[i]);
*/
/*
 ----------------------------------------------------------
|                                                          |
|   starting at the end of the sequence of values, use a   |
|   "minimax" like procedure to decide where the captures  |
|   will stop.                                             |
|                                                          |
 ----------------------------------------------------------
*/
  next_capture--;
  if(next_capture&1) 
    swap_sign=-1;
  else
    swap_sign=1;
  while (next_capture) {
    if (swap_sign < 0) {
      if(swap_list[next_capture] <= swap_list[next_capture-1])
         swap_list[next_capture-1]=swap_list[next_capture];
    }
    else {
      if(swap_list[next_capture] >= swap_list[next_capture-1])
       swap_list[next_capture-1]=swap_list[next_capture];
    }
    next_capture--;
    swap_sign=-swap_sign;
  }
  return (swap_list[0]);
}
