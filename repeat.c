#include <stdio.h>
#include <stdlib.h>
#include "types.h"
#include "function.h"
#include "data.h"
/*
********************************************************************************
*                                                                              *
*   RepetitionCheck() is used to detect a draw by repetition.  it saves the    *
*   current position in the repetition list each time it is called.  the list  *
*   contains all positions encountered since the last irreversible move        *
*   (capture or pawn push).                                                    *
*                                                                              *
*   RepetitionCheck() then scans the list to determine if this position has    *
*   occurred before.  if so, the position will be treated as a draw by         *
*   Search().                                                                  *
*                                                                              *
*   RepetitionCheck() also handles 50-move draws.  the position[] structure    *
*   countains the count of moves since the last capture or pawn push.  when    *
*   this value reaches 100 (plies, which is 50 moves) the score is set to      *
*   DRAW.                                                                      *
*                                                                              *
********************************************************************************
*/
int RepetitionCheck(int ply)
{
  register int i, this_position, more, reps, search_list;
/*
 ----------------------------------------------------------
|                                                          |
|   insert the board into the next slot in the repetition  |
|   list.                                                  |
|                                                          |
 ----------------------------------------------------------
*/
  this_position=repetition_head+ply-1;
  repetition_list[this_position]=HashKey(ply);
  if (ply == 1) return(0);
/*
 ----------------------------------------------------------
|                                                          |
|   if the 50-move rule is drawing close, then adjust the  |
|   score to reflect the impending draw.                   |
|                                                          |
 ----------------------------------------------------------
*/
  if (position[ply].rule_50_moves > 99) return(2);
/*
 ----------------------------------------------------------
|                                                          |
|   check for trivial draws, like insufficient material.   |
|                                                          |
 ----------------------------------------------------------
*/
  if (!(TotalWhitePawns(ply)+TotalBlackPawns(ply)) &&
      (TotalWhitePieces(ply) < 5) && (TotalBlackPieces(ply) < 5)) return(1);
/*
 ----------------------------------------------------------
|                                                          |
|   scan the repetition list to determine if this position |
|   has occurred before.                                   |
|                                                          |
 ----------------------------------------------------------
*/
  reps=0;
  more=position[ply].rule_50_moves>>1;
  search_list=Min(more,(ply-2)>>1);
  more-=search_list;
  for (i=this_position-2;search_list;i-=2,search_list--)
    if(repetition_list[this_position] == repetition_list[i]) return(1);
  for (;more>0;i-=2,more--)
    if(repetition_list[this_position] == repetition_list[i]) reps++;
  if (reps > 1) return(1);
  return(0);
}

/*
********************************************************************************
*                                                                              *
*   RepetitionDraw() is used to detect a draw by repetition.  it saves the     *
*   current position in the repetition list each time it is called.  the list  *
*   contains all positions encountered since the last irreversible move        *
*   (capture or pawn push).                                                    *
*                                                                              *
*   RepetitionDraw() then scans the list to determine if this position has     *
*   been repeated three times (a real draw by repetition, not a "psuedo-draw." *
*                                                                              *
********************************************************************************
*/
int RepetitionDraw(void)
{
  register int i, more, reps;
/*
 ----------------------------------------------------------
|                                                          |
|   if the 50-move rule is drawing close, then adjust the  |
|   score to reflect the impending draw.                   |
|                                                          |
 ----------------------------------------------------------
*/
  if (position[0].rule_50_moves > 99) return(2);
/*
 ----------------------------------------------------------
|                                                          |
|   scan the repetition list to determine if this position |
|   has occurred before.                                   |
|                                                          |
 ----------------------------------------------------------
*/
  reps=0;
  more=position[0].rule_50_moves>>1;
  for (i=repetition_head;more;i-=2,more--)
    if(HashKey(0) == repetition_list[i]) reps++;
  return(reps == 3);
}
