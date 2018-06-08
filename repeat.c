#include <stdio.h>
#include <stdlib.h>
#include "types.h"
#include "function.h"
#include "data.h"
/*
********************************************************************************
*                                                                              *
*   Repetition_Check() is used to detect a draw by repetition.  it saves the   *
*   current position in the repetition list each time it is called.  the list  *
*   contains all positions encountered since the last irreversible move        *
*   (capture or pawn push).                                                    *
*                                                                              *
*   Repetition_Check() then scans the list to determine if this position has   *
*   occurred before.  if so, the position will be treated as a draw by         *
*   Search().                                                                  *
*                                                                              *
*   Repetition_Check() also handles 50-move draws.  the position[] structure   *
*   countains the count of moves since the last capture or pawn push.  when    *
*   this value reaches 100 (plies, which is 50 moves) the score is set to      *
*   DRAW.                                                                      *
*                                                                              *
********************************************************************************
*/
int Repetition_Check(int ply)
{
  int i, this_position, more;
/*
 ----------------------------------------------------------
|                                                          |
|   insert the board into the next slot in the repetition  |
|   list.                                                  |
|                                                          |
 ----------------------------------------------------------
*/
  this_position=repetition_head+ply-1;
  repetition_list[this_position]=Hash_Key(ply);
  if (ply == 1) return(0);
/*
 ----------------------------------------------------------
|                                                          |
|   if the 50-move rule is drawing close, then adjust the  |
|   score to reflect the impending draw.                   |
|                                                          |
 ----------------------------------------------------------
*/
  if (position[ply].moves_since_cap_or_push > 99) return(2);
/*
 ----------------------------------------------------------
|                                                          |
|   check for trivial draws, like insufficient material.   |
|                                                          |
 ----------------------------------------------------------
*/
  if (!(Total_White_Pawns(ply)+Total_Black_Pawns(ply))) {
    if ((Total_White_Pieces(ply) < 5) && 
        (Total_Black_Pieces(ply) < 5)) return(1);
  }
/*
 ----------------------------------------------------------
|                                                          |
|   scan the repetition list to determine if this position |
|   has occurred before.                                   |
|                                                          |
 ----------------------------------------------------------
*/
  more=position[ply].moves_since_cap_or_push>>1;
  for (i=this_position-2;more;i-=2,more--)
      if(repetition_list[this_position] == repetition_list[i]) return(1);
  return(0);
}

/*
********************************************************************************
*                                                                              *
*   Repetition_Draw() is used to detect a draw by repetition.  it saves the    *
*   current position in the repetition list each time it is called.  the list  *
*   contains all positions encountered since the last irreversible move        *
*   (capture or pawn push).                                                    *
*                                                                              *
*   Repetition_Draw() then scans the list to determine if this position has    *
*   been repeated three times (a real draw by repetition, not a "psuedo-draw." *
*                                                                              *
********************************************************************************
*/
int Repetition_Draw(void)
{
  int i, more, rep;
/*
 ----------------------------------------------------------
|                                                          |
|   if the 50-move rule is drawing close, then adjust the  |
|   score to reflect the impending draw.                   |
|                                                          |
 ----------------------------------------------------------
*/
  if (position[0].moves_since_cap_or_push > 99) return(2);
/*
 ----------------------------------------------------------
|                                                          |
|   scan the repetition list to determine if this position |
|   has occurred before.                                   |
|                                                          |
 ----------------------------------------------------------
*/
  rep=0;
  more=position[0].moves_since_cap_or_push>>1;
  for (i=repetition_head;more;i-=2,more--)
    if(Hash_Key(0) == repetition_list[i]) rep++;
  return(rep == 3);
}
