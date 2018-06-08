#include <stdio.h>
#include <stdlib.h>
#include "chess.h"
#include "data.h"

/* last modified 04/04/00 */
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
int Swap(TREE * RESTRICT tree, int source, int target, int wtm) {
  register BITBOARD attacks;
  register int attacked_piece;
  register int square, direction;
  register int color, nc=1;
  int swap_list[32];
/*
 ----------------------------------------------------------
|                                                          |
|   determine which squares attack <target> for each side. |
|   initialize by placing the piece on <target> first in   |
|   the list as it is being captured to start things off.  |
|                                                          |
 ----------------------------------------------------------
*/
  attacks=AttacksTo(tree,target);
  attacked_piece=p_values[PcOnSq(target)+7];
/*
 ----------------------------------------------------------
|                                                          |
|   the first piece to capture on <target> is the piece    |
|   standing on <source>.                                  |
|                                                          |
 ----------------------------------------------------------
*/
  color=Flip(wtm);
  swap_list[0]=attacked_piece;
  attacked_piece=p_values[PcOnSq(source)+7];
  Clear(source,attacks);
  direction=directions[target][source];
  if (direction) attacks=SwapXray(tree,attacks,source,direction);
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
      if (WhitePawns & attacks)
        square=FirstOne(WhitePawns & attacks);
      else if (WhiteKnights & attacks)
        square=FirstOne(WhiteKnights & attacks);
      else if (WhiteBishops & attacks)
        square=FirstOne(WhiteBishops & attacks);
      else if (WhiteRooks & attacks)
        square=FirstOne(WhiteRooks & attacks);
      else if (WhiteQueens & attacks)
        square=FirstOne(WhiteQueens & attacks);
      else if (WhiteKing & attacks)
        square=WhiteKingSQ;
      else break;
    }
    else {
      if (BlackPawns & attacks)
        square=FirstOne(BlackPawns & attacks);
      else if (BlackKnights & attacks)
        square=FirstOne(BlackKnights & attacks);
      else if (BlackBishops & attacks)
        square=FirstOne(BlackBishops & attacks);
      else if (BlackRooks & attacks)
        square=FirstOne(BlackRooks & attacks);
      else if (BlackQueens & attacks)
        square=FirstOne(BlackQueens & attacks);
      else if (BlackKing & attacks)
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
    swap_list[nc]=-swap_list[nc-1]+attacked_piece;
    attacked_piece=p_values[PcOnSq(square)+7];
    Clear(square,attacks);
    direction=directions[target][square];
    if (direction) attacks=SwapXray(tree,attacks,square,direction);
    nc++;
    color=Flip(color);
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
  while(--nc)
    if(swap_list[nc] > -swap_list[nc-1]) swap_list[nc-1]=-swap_list[nc];
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
BITBOARD SwapXray(TREE * RESTRICT tree, BITBOARD attacks, int from, int direction) {
  switch (direction) {
  case 1: 
    return(attacks | (AttacksRank(from) & RooksQueens & plus1dir[from]));
  case 7: 
    return(attacks | (AttacksDiaga1(from) & BishopsQueens & plus7dir[from]));
  case 8: 
    return(attacks | (AttacksFile(from) & RooksQueens & plus8dir[from]));
  case 9: 
    return(attacks | (AttacksDiagh1(from) & BishopsQueens & plus9dir[from]));
  case -1: 
    return(attacks | (AttacksRank(from) & RooksQueens & minus1dir[from]));
  case -7: 
    return(attacks | (AttacksDiaga1(from) & BishopsQueens & minus7dir[from]));
  case -8: 
    return(attacks | (AttacksFile(from) & RooksQueens & minus8dir[from]));
  case -9: 
    return(attacks | (AttacksDiagh1(from) & BishopsQueens & minus9dir[from]));
  }
  return(attacks);
}
