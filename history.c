#include <stdio.h>
#include <stdlib.h>
#include "chess.h"
#include "data.h"

/* last modified 05/03/99 */
/*
********************************************************************************
*                                                                              *
*   History() is used to maintain the history database.  this is a set of      *
*   counts, indexed by the 12-bit value [to,from], that contains the relative  *
*   effectiveness of this move as a refutation.  this effectiveness is simply  *
*   an arbitrary value that varies significantly and inversely to the depth of *
*   the sub-tree refuted by each move.                                         *
*                                                                              *
*   these routines also maintain the killer move lists as well, since they are *
*   similar in nature.                                                         *
*                                                                              *
********************************************************************************
*/
void History(TREE *tree, int ply, int depth, int wtm, int move) {
  register int index;
/*
 ----------------------------------------------------------
|                                                          |
|   if the best move so far is a capture or a promotion,   |
|   return, since we try good captures and promotions      |
|   before searching history heuristic moves anyway.       |
|                                                          |
 ----------------------------------------------------------
*/
  if (CaptureOrPromote(move)) return;
/*
 ----------------------------------------------------------
|                                                          |
|   otherwise, use the [to,from] as an index and increment |
|   the appropriate history table/entry.                   |
|                                                          |
 ----------------------------------------------------------
*/
  index=move & 4095;
  if (wtm) history_w[index]+=depth*depth;
  else history_b[index]+=depth*depth;
/*
 ----------------------------------------------------------
|                                                          |
|   now, add the same move to the current killer moves if  |
|   it is not already there.                               |
|                                                          |
 ----------------------------------------------------------
*/
  if (tree->killers[ply].move1 != move) {
    tree->killers[ply].move2=tree->killers[ply].move1;
    tree->killers[ply].move1=move;
  }
}
