#include <stdio.h>
#include <stdlib.h>
#include "chess.h"
#include "data.h"

/* last modified 11/11/96 */
/*
********************************************************************************
*                                                                              *
*   Swap() is used to analyze capture moves to see whether or not they appear  *
*   to be profitable.  the basic algorithm is extremely fast since it uses the *
*   bitmaps to determine which squares are attacking the [target] square.      *
*                                                                              *
*   the algorithm is quite simple.  using the attack bitmaps, we enumerate all *
*   the pieces that are attacking [target] for either side.  then we simply    *
*   use the lowest piece (value) for the correct side to capture on [target].  *
*   we continually "flip" sides taking the lowest piece each time.             *
*                                                                              *
*   as a piece is used, if it is a sliding piece (pawn, bishop, rook or queen) *
*   we "peek" behind it to see if it is attacked by a sliding piece in the     *
*   direction away from the piece being captured.  if so, and that sliding     *
*   piece moves in this direction, then it is added to the list of attackers   *
*   since its attack has been "uncovered" by moving the capturing piece.       *
*                                                                              *
********************************************************************************
*/
int Swap(int source, int target, int wtm)
{
  register BITBOARD attacks;
  register int attacked_piece;
  register int square;
  register int sign, color, next_capture=1;
  int swap_list[32];
/*
 ----------------------------------------------------------
|                                                          |
|   determine which squares attack <target> for each side. |
|                                                          |
 ----------------------------------------------------------
*/
  attacks=AttacksTo(target);
/*
 ----------------------------------------------------------
|                                                          |
|   if the side-to-move isn't attacking <target> then we   |
|   are done.                                              |
|                                                          |
 ----------------------------------------------------------
*/
  if (!And(attacks, wtm ? WhitePieces: BlackPieces)) return(0);
/*
 ----------------------------------------------------------
|                                                          |
|   initialize by placing the piece on <target> first in   |
| the list as it is being captured to start things off.    |
|                                                          |
 ----------------------------------------------------------
*/
  attacked_piece=piece_values[abs(PieceOnSquare(target))];
/*
 ----------------------------------------------------------
|                                                          |
|   the first piece to capture on <target> is the piece    |
|   standing on <source>.                                  |
|                                                          |
 ----------------------------------------------------------
*/
  color=ChangeSide(wtm);
  swap_list[0]=attacked_piece;
  sign=-1;
  attacked_piece=piece_values[abs(PieceOnSquare(source))];
  Clear(source,attacks);
  if (directions[target][source]) attacks=SwapXray(attacks,source,target);
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
    if (color) {
      if (And(WhitePawns,attacks))
        square=FirstOne(And(WhitePawns,attacks));
      else if (And(WhiteKnights,attacks))
        square=FirstOne(And(WhiteKnights,attacks));
      else if (And(WhiteBishops,attacks))
        square=FirstOne(And(WhiteBishops,attacks));
      else if (And(WhiteRooks,attacks))
        square=FirstOne(And(WhiteRooks,attacks));
      else if (And(WhiteQueens,attacks))
        square=FirstOne(And(WhiteQueens,attacks));
      else if (And(WhiteKing,attacks))
        square=WhiteKingSQ;
      else break;
    }
    else {
      if (And(BlackPawns,attacks))
        square=FirstOne(And(BlackPawns,attacks));
      else if (And(BlackKnights,attacks))
        square=FirstOne(And(BlackKnights,attacks));
      else if (And(BlackBishops,attacks))
        square=FirstOne(And(BlackBishops,attacks));
      else if (And(BlackRooks,attacks))
        square=FirstOne(And(BlackRooks,attacks));
      else if (And(BlackQueens,attacks))
        square=FirstOne(And(BlackQueens,attacks));
      else if (And(BlackKing,attacks))
        square=BlackKingSQ;
      else break;
    }
/*
 ------------------------------------------------
|                                                |
|  located the least valuable piece bearing on   |
|  <target>.  remove it from the list and then   |
|  find out if a sliding piece behind it attacks |
|  through this piece.                           |
|                                                |
 ------------------------------------------------
*/
    swap_list[next_capture]=swap_list[next_capture-1]+sign*attacked_piece;
    attacked_piece=piece_values[abs(PieceOnSquare(square))];
    Clear(square,attacks);
    if (directions[target][square]) attacks=SwapXray(attacks,square,target);
    next_capture++;
    sign=-sign;
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
  if(next_capture&1) sign=-1;
  else sign=1;
  while (next_capture) {
    if (sign < 0) {
      if(swap_list[next_capture] <= swap_list[next_capture-1])
         swap_list[next_capture-1]=swap_list[next_capture];
    }
    else {
      if(swap_list[next_capture] >= swap_list[next_capture-1])
       swap_list[next_capture-1]=swap_list[next_capture];
    }
    next_capture--;
    sign=-sign;
  }
  return (swap_list[0]);
}

/*
********************************************************************************
*                                                                              *
*   SwapXray() is used to determine if a piece is "behind" the piece on        *
*   <from>, and this piece would attack <to> if the piece on <from> were moved *
*   (as in playing out sequences of swaps).  if so, this indirect attacker is  *
*   added to the list of attackers bearing to <to>.                            *
*                                                                              *
********************************************************************************
*/
BITBOARD SwapXray(BITBOARD attacks, int from, int to)
{
  register BITBOARD indirect;
  switch (directions[to][from]) {
  case 1: 
    indirect=And(And(AttacksRank(from),RooksQueens),mask_plus1dir[from]);
    return(Or(attacks,indirect));
  case 7: 
    indirect=And(And(AttacksDiaga1(from),BishopsQueens),mask_plus7dir[from]);
    return(Or(attacks,indirect));
  case 8: 
    indirect=And(And(AttacksFile(from),RooksQueens),mask_plus8dir[from]);
    return(Or(attacks,indirect));
  case 9: 
    indirect=And(And(AttacksDiagh1(from),BishopsQueens),mask_plus9dir[from]);
    return(Or(attacks,indirect));
  case -1: 
    indirect=And(And(AttacksRank(from),RooksQueens),mask_minus1dir[from]);
    return(Or(attacks,indirect));
  case -7: 
    indirect=And(And(AttacksDiaga1(from),BishopsQueens),mask_minus7dir[from]);
    return(Or(attacks,indirect));
  case -8: 
    indirect=And(And(AttacksFile(from),RooksQueens),mask_minus8dir[from]);
    return(Or(attacks,indirect));
  case -9: 
    indirect=And(And(AttacksDiagh1(from),BishopsQueens),mask_minus9dir[from]);
    return(Or(attacks,indirect));
  }
  return(attacks);
}
