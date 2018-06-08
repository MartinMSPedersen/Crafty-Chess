#include <stdio.h>
#include <stdlib.h>
#include "chess.h"
#include "data.h"

/* last modified 03/09/00 */
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
*   Search().   note that for a repetition to happen in the first two plies    *
*   of the tree, the position has to be repeated three times, while for plies  *
*   beyond two, two repetitions trigger a draw score.                          *
*                                                                              *
*   RepetitionCheck() also handles 50-move draws.  the position[] structure    *
*   countains the count of moves since the last capture or pawn push.  when    *
*   this value reaches 100 (plies, which is 50 moves) the score is set to      *
*   DRAW.                                                                      *
*                                                                              *
********************************************************************************
*/
int RepetitionCheck(TREE *tree, int ply, int wtm) {
  register BITBOARD *replist, *thispos, *lastpos;
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
|   the position has been seen one time before, unless we  |
|   are at ply 1 or 2, where we must have seen the same    |
|   position twice prior to this.                          |
|                                                          |
 ----------------------------------------------------------
*/
  if (wtm) {
    thispos=tree->rephead_w+((ply-2)>>1);
    lastpos=tree->replist_w;
  }
  else {
    thispos=tree->rephead_b+((ply-2)>>1);
    lastpos=tree->replist_b;
  }
  *thispos=HashKey;
  if (ply > 3) {
    for (replist=thispos-1;replist>=lastpos;replist--)
      if(HashKey == *replist) return(1);
  }
  else {
    int count=0;
    for (replist=thispos-1;replist>=lastpos;replist--)
      if(HashKey == *replist) count++;
    if (count > 1) return(1);
  }
  return(0);
}

/* last modified 03/28/00 */
/*
********************************************************************************
*                                                                              *
*   RepetitionCheckBook() is used to detect a draw by repetition while still   *
*   in book.  It simply counts two repeats as a draw as there is little sense  *
*   in repeating a position at all in the opening.                             *
*                                                                              *
********************************************************************************
*/
int RepetitionCheckBook(TREE *tree, int ply, int wtm) {
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
|   scan the list.  we look for the case where the         |
|   position has been seen one time before.                |
|                                                          |
 ----------------------------------------------------------
*/
  entries=Rule50Moves(ply)>>1;
  thispos=(wtm)?tree->rephead_w:tree->rephead_b;
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
int RepetitionDraw(TREE *tree, int wtm) {
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
