#include <stdio.h>
#include <stdlib.h>
#include "types.h"
#include "function.h"
#include "data.h"
#include "evaluate.h"
/*
********************************************************************************
*                                                                              *
*   Swap() is used to analyze capture moves to see whether or not they appear  *
*   to be profitable.  the basic algorithm is extremely fast since it uses the *
*   from.attack[] bit-boards to determine which squares are attacking [target] *
*   square.                                                                    *
*                                                                              *
*   the algorithm is quite simple.  using from.attacks[], we can enumerate all *
*   the pieces that are attacking [target] for either side.  then we simply    *
*   use the lowest piece (value) for the correct side to capture on [target].  *
*   we continually "flip" sides taking the lowest piece each time.             *
*                                                                              *
*   as a piece is "used", if it is a sliding piece (pawn, bishop, rook or      *
*   queen) we use the from.attacks[square] to see if a sliding piece is        *
*   attacking this piece in the same direction, meaning that the sliding piece *
*   can now be used in the swap sequence.  one final "fix" is that the piece   *
*   on <from> must be used first in the capture sequence (if [from] is a real  *
*   square.)                                                                   *
*                                                                              *
********************************************************************************
*/
int Swap(int ply, int source, int target, int wtm)
{
  BITBOARD white_attackers, black_attackers;
  BITBOARD piece_squares, temp_attacks;
  BITBOARD *pawns[2], *knights[2], *bishops[2], 
           *rooks[2], *queens[2], *kings[2];
  int attacked_piece;
  int square;
  int swap_sign, color, next_capture=1;
  int swap_list[32];
/*
 ----------------------------------------------------------
|                                                          |
|   determine which squares attack <target> for each side. |
|                                                          |
 ----------------------------------------------------------
*/
  temp_attacks=Attacks_To(target,ply);
  white_attackers=And(temp_attacks,White_Pieces(ply));
  black_attackers=And(temp_attacks,Black_Pieces(ply));
/*
 ----------------------------------------------------------
|                                                          |
|   if the side-to-move isn't attacking <target> then we   |
|   are done.                                              |
|                                                          |
 ----------------------------------------------------------
*/
  if (wtm) {
    if (!white_attackers)
      return(0);
  }
  else {
    if (!black_attackers)
      return(0);
  }
/*
 ----------------------------------------------------------
|                                                          |
|   initialize by placing the piece on <target> first in   |
| the list as it is being captured to start things off.    |
|                                                          |
 ----------------------------------------------------------
*/
  color=!wtm;
  attacked_piece=piece_values[abs(Piece_On_Square(ply,target))];
/*
 ----------------------------------------------------------
|                                                          |
|   if the side not-to-move isn't attacking <target> then  |
|   we simply return the value of the piece on <target>    |
|   since it can be ripped with impunity.                  |
|                                                          |
 ----------------------------------------------------------
*/
  if (wtm) {
    if (!black_attackers)
      return(attacked_piece);
  }
  else {
    if (!white_attackers)
      return(attacked_piece);
  }
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
  swap_sign=-1;
  piece_squares=temp_attacks;
/*
 ----------------------------------------------------------
|                                                          |
|   the first piece to capture on <target> is the piece    |
|   standing on <source>.                                  |
|                                                          |
 ----------------------------------------------------------
*/
  color=!wtm;
  swap_list[0]=attacked_piece;
  swap_sign=-1;
  attacked_piece=piece_values[abs(Piece_On_Square(ply,source))];
  Clear(source,piece_squares);
  piece_squares=Swap_Xray(ply,piece_squares,source,target);
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
      break;
/*
 ------------------------------------------------
|                                                |
|  located the least valuable piece bearing on   |
|  <target>.  remove it from the list and then   |
|  find out what's behind it.                    |
|                                                |
 ------------------------------------------------
*/
    swap_list[next_capture]=swap_list[next_capture-1]+
                            swap_sign*attacked_piece;
    attacked_piece=piece_values[abs(Piece_On_Square(ply,square))];
    Clear(square,piece_squares);
    piece_squares=Swap_Xray(ply,piece_squares,square,target);
    next_capture++;
    swap_sign=-swap_sign;
    color=!color;
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

/*
********************************************************************************
*                                                                              *
*   Swap_Xray() is used to determine if a piece is "behind" the piece on       *
*   <from>, and this piece would attack <to> if the piece on <from> were moved *
*   (as in playing out sequences of swaps).  if so, this indirect attacker is  *
*   added to the list of attackers bearing to <to>.                            *
*                                                                              *
********************************************************************************
*/
BITBOARD Swap_Xray(int ply, BITBOARD attacks, int from, int to)
{
  BITBOARD indirect = 0;
  switch (directions[to][from]) {
  case 1: 
    indirect=And(And(Attacks_Rank(from),Rooks_Queens(ply)),
                 mask_plus1dir[from]);
    break;
  case 7: 
    indirect=And(And(Attacks_Diaga1(from),Bishops_Queens(ply)),
                 mask_plus7dir[from]);
    break;
  case 8: 
    indirect=And(And(Attacks_File(from),Rooks_Queens(ply)),
                 mask_plus8dir[from]);
    break;
  case 9: 
    indirect=And(And(Attacks_Diagh1(from),Bishops_Queens(ply)),
                 mask_plus9dir[from]);
    break;
  case -1: 
    indirect=And(And(Attacks_Rank(from),Rooks_Queens(ply)),
                 mask_minus1dir[from]);
    break;
  case -7: 
    indirect=And(And(Attacks_Diaga1(from),Bishops_Queens(ply)),
                 mask_minus7dir[from]);
    break;
  case -8: 
    indirect=And(And(Attacks_File(from),Rooks_Queens(ply)),
                 mask_minus8dir[from]);
    break;
  case -9: 
    indirect=And(And(Attacks_Diagh1(from),Bishops_Queens(ply)),
                 mask_minus9dir[from]);
    break;
  }
  return(Or(attacks,indirect));
}
