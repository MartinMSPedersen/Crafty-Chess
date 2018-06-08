#include <stdio.h>
#include <stdlib.h>
#include "types.h"
#include "function.h"
#include "data.h"
/*
********************************************************************************
*                                                                              *
*   Next_Evasion() is used to select the next move from the current move list  *
*   when the king is in check.  it tries the following things to get out of    *
*   check:                                                                     *
*                                                                              *
*     1.  if only piece is attacking the king (aren't bitboard attack vectors  *
*         wonderful?) try to capture the checking piece first.  we use the     *
*         normal capture logic that tries winning or even captures first, and  *
*         postpones losing captures until other safe moves have been tried.    *
*                                                                              *
*     2.  try moving the king to a safe square (one that is not already under  *
*         attack which would do nothing...)                                    *
*                                                                              *
*     3.  If more than one piece is attacking the king, we can give up as we   *
*         can't do anything but move the king, which we've already tried.      *
*         therefore, assuming one attacker, if it's a knight, again we are     *
*         done as we can't interpose anything.  if it's a bishop, rook, or     *
*         queen, try interposing.  we simply have to find the attacker (easy)  *
*         the attackee (the king's square, also easy) and use the precomputed  *
*         mask to produce a bit vector of the squares between these two        *
*         squares.  then pass this to Generate_Check_Evasions() as targets     *
*         and we have all interposing moves.  of course, we still try them     *
*         in "sane" order of safe followed by any.                             *
*                                                                              *
********************************************************************************
*/
int Next_Evasion(int ply, int wtm)
{
  BITBOARD target;
  int *mv, *mvp, tempm;
  int history_value, bestval, done, i, index, temp;
  int checkers, king_square;
  int checking_square, check_direction;
  switch (next_status[ply].phase) {
/*
 ----------------------------------------------------------
|                                                          |
|   first try the transposition table move (which will be  |
|   the principal variation move as we first move down the |
|   tree).                                                 |
|                                                          |
 ----------------------------------------------------------
*/
  case hash_normal_move:
    next_status[ply].phase=capture_moves;
    if (hash_move[ply]) {
      current_move[ply]=hash_move[ply];
      if (Valid_Move(ply,wtm,current_move[ply]))
        return(hash_normal_move);
      else
        Print(1,"bad move from hash table, ply=%d\n",ply);
    }
/*
 ----------------------------------------------------------
|                                                          |
|   try the capturing moves next.  this phase uses the     |
|   special move generator Generate_Check_Evasions() to    |
|   generate moves that evade the current check.  this     |
|   routine generates the moves as described previously.   |
|                                                          |
 ----------------------------------------------------------
*/
  case capture_moves:
    if (next_status[ply].whats_generated != everything) {
      next_status[ply].remaining=0;
      check_direction=0;
      if (wtm) {
        king_square=White_King_SQ(ply);
        checkers=Popcnt(And(Attacks_To(king_square,ply),
                            Black_Pieces(ply)));
        if (checkers == 1) {
          checking_square=First_One(And(Attacks_To(king_square,ply),
                                        Black_Pieces(ply)));
          if (Piece_On_Square(ply,checking_square) != -1)
            check_direction=directions[checking_square][king_square];
          else
            check_direction=0;
        }
        target=Next_Evasion_Interpose_Squares(ply,wtm,checkers,check_direction,
                                              king_square,checking_square);
      }
      else {
        king_square=Black_King_SQ(ply);
        checkers=Popcnt(And(Attacks_To(king_square,ply),
                            White_Pieces(ply)));
        if (checkers == 1) {
          checking_square=First_One(And(Attacks_To(king_square,ply),
                                        White_Pieces(ply)));
          if (Piece_On_Square(ply,checking_square) != 1)
            check_direction=directions[checking_square][king_square];
          else
            check_direction=0;
        }
        target=Next_Evasion_Interpose_Squares(ply,wtm,checkers,check_direction,
                                              king_square,checking_square);
      }
      next_status[ply].to=target;
      last[ply]=Generate_Check_Evasions(ply, wtm, target, checkers,
                                        check_direction, king_square,
                                        first[ply]);
      next_status[ply].whats_generated=everything;
/*
 --------------------------------------------------
|                                                  |
|   moves are generated, now sort best captures    |
|   first.                                         |
|                                                  |
 --------------------------------------------------
*/
      next_status[ply].remaining=0;
      for (mvp=first[ply];mvp<last[ply];mvp++) {
        if (hash_move[ply] == *mvp) {
          *mvp=0;
          sort_value[mvp-first[ply]]=-999999;
        }
        else
          sort_value[mvp-first[ply]]=
            Swap(ply,From(*mvp),To(*mvp),wtm);
        if (sort_value[mvp-first[ply]] >= 0) next_status[ply].remaining++;
      }
      do {
        done=1;
        for (i=0;i<last[ply]-first[ply]-1;i++) {
          if (sort_value[i] < sort_value[i+1]) {
            temp=sort_value[i];
            sort_value[i]=sort_value[i+1];
            sort_value[i+1]=temp;
            tempm=*(first[ply]+i);
            *(first[ply]+i)=*(first[ply]+i+1);
            *(first[ply]+i+1)=tempm;
            done=0;
          }
        }
      } while(!done);
      next_status[ply].last=first[ply];
    }
/*
 --------------------------------------------------
|                                                  |
|   after the first call, simply drop to here to   |
|   return the next "good" capture move.           |
|                                                  |
 --------------------------------------------------
*/
    if (next_status[ply].remaining) {
      current_move[ply]=*(next_status[ply].last);
      next_status[ply].current=next_status[ply].last;
      *next_status[ply].last++=0;
      next_status[ply].remaining--;
      if (!next_status[ply].remaining) 
        next_status[ply].phase=history_moves;
      return(capture_moves);
    }
    next_status[ply].phase=history_moves;
/*
 ----------------------------------------------------------
|                                                          |
|   now, try the history moves.  this phase generates the  |
|   complete move list, and then shifts those moves with   |
|   a non-zero history value to the top of the list.       |
|   these moves are then sorted and tried in order before  |
|   trying the rest.                                       |
|                                                          |
 ----------------------------------------------------------
*/
  case history_moves:
    bestval=0;
    for (mv=first[ply];mv<last[ply];mv++) {
      if (*mv == hash_move[ply]) *mv=0;
      index=*mv&4095;
      if (wtm)
        history_value=history_w[index];
      else
        history_value=history_b[index];
      if ((history_value > bestval) && *mv) {
        bestval=history_value;
        mvp=mv;
      }
    }
    if (bestval) {
      current_move[ply]=*mvp;
      next_status[ply].current=mvp;
      *mvp=0;
      return(history_moves);
    }
    next_status[ply].phase=remaining_moves;
/*
 ----------------------------------------------------------
|                                                          |
|   now try the rest of the set of moves. try 'em in a     |
|   couple of passes (for now.)  pass 1: move attacked     |
|   pieces to unattacked squares;  pass 2: move any piece  |
|   to an unattacked square;  pass 3: any moves left.      |
|                                                          |
 ----------------------------------------------------------
*/
  case remaining_moves:
    for (next_status[ply].last=first[ply];
         next_status[ply].last<last[ply];
         next_status[ply].last++) {
      if ((*next_status[ply].last)) {
        current_move[ply]=*next_status[ply].last;
        next_status[ply].current=next_status[ply].last;
        *next_status[ply].last++=0;
        return(remaining_moves);
      }
    }
    break;
  case all_done:
    return(none);
  default:
    printf("oops!  next_status.phase is bad! [evasion %d]\n",
           next_status[ply].phase);
  }
/*
 ----------------------------------------------------------
|                                                          |
|   done, return (none) since nothing was found.           |
|                                                          |
 ----------------------------------------------------------
*/
  return(none);
}

/*
********************************************************************************
*                                                                              *
*   Next_Evasion_Interpose_Mask() is used to compute the set of squares that   *
*   blocks the one-and-only check.                                             *
*                                                                              *
********************************************************************************
*/
BITBOARD Next_Evasion_Interpose_Squares(int ply, int wtm, int checkers, 
                                        int check_direction, int king_square,
                                        int checking_square)
{
  BITBOARD target;
/*
 ----------------------------------------------------------
|                                                          |
|   if this is a check from a single sliding piece, then   |
|   we can interpose along the checking rank/file/diagonal |
|   and block the check.  otherwise, interposing is not a  |
|   possibility.                                           |
|                                                          |
 ----------------------------------------------------------
*/
  if (wtm) {
    if ((checkers == 1) && And(Attacks_To(king_square,ply),
                                Black_Knights(ply)))
      checkers=2;
  }
  else {
    if ((checkers == 1) && And(Attacks_To(king_square,ply),
                                White_Knights(ply)))
      checkers=2;
  }

  if (checkers == 1) {
    switch (check_direction) {
      case +1:
        target=Xor(mask_plus1dir[king_square-1],
                   mask_plus1dir[checking_square]);
        break;
      case +7:
        target=Xor(mask_plus7dir[king_square-7],
                   mask_plus7dir[checking_square]);
        break;
      case +8:
        target=Xor(mask_plus8dir[king_square-8],
                   mask_plus8dir[checking_square]);
        break;
      case +9:
        target=Xor(mask_plus9dir[king_square-9],
                   mask_plus9dir[checking_square]);
        break;
      case -1:
        target=Xor(mask_minus1dir[king_square+1],
                   mask_minus1dir[checking_square]);
        break;
      case -7:
        target=Xor(mask_minus7dir[king_square+7],
                   mask_minus7dir[checking_square]);
        break;
      case -8:
        target=Xor(mask_minus8dir[king_square+8],
                   mask_minus8dir[checking_square]);
        break;
      case -9:
        target=Xor(mask_minus9dir[king_square+9],
                   mask_minus9dir[checking_square]);
        break;
      default:
        target=0;
        break;
    }
  }
  else
    target=0;
  return(target);
}
