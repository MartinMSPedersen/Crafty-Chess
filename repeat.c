#include <stdio.h>
#include <stdlib.h>
#include "chess.h"
#include "data.h"

/* last modified 01/14/99 */
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
int RepetitionCheck(TREE *tree, int ply, int wtm)
{
  register int entries;
  register BITBOARD *replist, *thispos;
/*
 ----------------------------------------------------------
|                                                          |
|   if the 50-move rule has been reached, then adjust the  |
|   score to reflect the impending draw.                   |
|                                                          |
 ----------------------------------------------------------
*/
  if (Rule50Moves(ply) > 99) return(1);
/*
 ----------------------------------------------------------
|                                                          |
|   insert the board into the next slot in the repetition  |
|   list.  then scan the list.  we look for the case where |
|   the position has been seen one time before.            |
|                                                          |
 ----------------------------------------------------------
*/
  entries=Rule50Moves(ply)>>1;
  thispos=((wtm)?tree->rephead_w:tree->rephead_b)+((ply-1)>>1);
  *thispos=HashKey;
  for (replist=thispos-1;entries;replist--,entries--)
    if(HashKey == *replist) return(1);
  return(0);
}

/* last modified 03/11/98 */
/*
********************************************************************************
*                                                                              *
*   RepetitionDraw() is used to detect a draw by repetition.  this routine is  *
*   only called from Main() and simply scans the complete list searching for   *
*   exactly three repetitions (two additional repetitions of the current       *
*   position.)                                                                 *
*                                                                              *
********************************************************************************
*/
int RepetitionDraw(TREE *tree, int wtm)
{
  register int reps;
  BITBOARD *thispos;
/*
 ----------------------------------------------------------
|                                                          |
|   if the 50-move rule has been reached, then adjust the  |
|   score to reflect the impending draw.                   |
|                                                          |
 ----------------------------------------------------------
*/
  if (Rule50Moves(0) > 99) return(2);
/*
 ----------------------------------------------------------
|                                                          |
|   scan the repetition list to determine if this position |
|   has occurred before.                                   |
|                                                          |
 ----------------------------------------------------------
*/
  reps=0;
  if (wtm) {
    for (thispos=tree->replist_w;thispos<tree->rephead_w;thispos++)
      if(HashKey == *thispos) reps++;
  }
  else {
    for (thispos=tree->replist_b;thispos<tree->rephead_b;thispos++)
      if(HashKey == *thispos) reps++;
  }
  return(reps == 3);
}
