#include <stdio.h>
#include <stdlib.h>
#include "chess.h"
#include "data.h"

/* last modified 02/17/97 */
/*
********************************************************************************
*                                                                              *
*   EnPrise() is used to analyze pieces (at the root of the search) to see if  *
*   they can be captured.  this information is then used to order the moves at *
*   the root of the tree to search moves that don't hang pieces first.         *
*                                                                              *
*   the algorithm is quite simple.  using AttacksTo(), we can enumerate all    *
*   the pieces that are attacking [target] for either side.  then we simply    *
*   use the lowest piece (value) for the correct side to capture on [target].  *
*   we continually "flip" sides taking the lowest piece each time.             *
*                                                                              *
*   as a piece is "used", if it is a sliding piece (pawn, bishop, rook or      *
*   queen) we use the AttacksTo(square) to see if a sliding piece is           *
*   attacking this piece in the same direction, meaning that the sliding piece *
*   can now be used in the swap sequence.  one final "fix" is that the piece   *
*   on <from> must be used first in the capture sequence (if [from] is a real  *
*   square.)                                                                   *
*                                                                              *
********************************************************************************
*/
int EnPrise(int target, int wtm)
{
  BITBOARD white_attackers, black_attackers;
  BITBOARD attacks, temp_attacks;
  BITBOARD *pawns[2], *knights[2], *bishops[2], 
           *rooks[2], *queens[2], *kings[2];
  int attacked_piece;
  int square, direction;
  int swap_sign, color, next_capture=0;
  int swap_list[32];
  TREE *tree=local[0];
/*
 ----------------------------------------------------------
|                                                          |
|   determine which squares attack <target> for each side. |
|                                                          |
 ----------------------------------------------------------
*/
  temp_attacks=AttacksTo(tree,target);
  white_attackers=And(temp_attacks,WhitePieces);
  black_attackers=And(temp_attacks,BlackPieces);
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
  attacked_piece=p_values[PieceOnSquare(target)+7];
/*
 ----------------------------------------------------------
|                                                          |
|   no quick exit.  set up for scanning the list of pieces |
|   that attack <target> and play the "swaps" out.         |
|                                                          |
 ----------------------------------------------------------
*/
  pawns[0]=&BlackPawns;
  pawns[1]=&WhitePawns;
  knights[0]=&BlackKnights;
  knights[1]=&WhiteKnights;
  bishops[0]=&BlackBishops;
  bishops[1]=&WhiteBishops;
  rooks[0]=&BlackRooks;
  rooks[1]=&WhiteRooks;
  queens[0]=&BlackQueens;
  queens[1]=&WhiteQueens;
  kings[0]=&BlackKing;
  kings[1]=&WhiteKing;
  swap_sign=1;
  attacks=AttacksTo(tree,target);
  color=wtm;
/*
 ----------------------------------------------------------
|                                                          |
|   now pick out the least valuable piece for the correct  |
|   side that is bearing on <target>.  as we find one, we  |
|   call SwapXray() to add the piece behind this piece     |
|   that is indirectly bearing on <target> (if any).       |
|                                                          |
 ----------------------------------------------------------
*/
  while (attacks) {
    if (And(*pawns[color],attacks))
      square=FirstOne(And(*pawns[color],attacks));
    else if (And(*knights[color],attacks))
      square=FirstOne(And(*knights[color],attacks));
    else if (And(*bishops[color],attacks))
      square=FirstOne(And(*bishops[color],attacks));
    else if (And(*rooks[color],attacks))
      square=FirstOne(And(*rooks[color],attacks));
    else if (And(*queens[color],attacks))
      square=FirstOne(And(*queens[color],attacks));
    else if (And(*kings[color],attacks))
      square=FirstOne(And(*kings[color],attacks));
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
    attacked_piece=p_values[PieceOnSquare(square)+7];
    Clear(square,attacks);
    direction=directions[target][square];
    if (direction) attacks=SwapXray(tree,attacks,square,direction);
    next_capture++;
    swap_sign=-swap_sign;
    color=ChangeSide(color);
  }
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
