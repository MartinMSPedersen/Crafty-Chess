#include <stdio.h>
#include <stdlib.h>
#include "types.h"
#include "function.h"
#include "data.h"

/* last modified 02/12/96 */
/*
********************************************************************************
*                                                                              *
*   History_*() is used to maintain the history database.  this is a set of    *
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
void HistoryBest(int ply, int depth, int wtm)
{
  register int index, temp;
/*
 ----------------------------------------------------------
|                                                          |
|   if the best move so far is a capture or a promotion,   |
|   return, since we try good captures and promotions      |
|   before searching history heuristic moves anyway.       |
|                                                          |
 ----------------------------------------------------------
*/
  if (CaptureOrPromote(pv[ply].path[ply])) return;
/*
 ----------------------------------------------------------
|                                                          |
|   otherwise, use the [to,from] as an index and increment |
|   the appropriate history table/entry.                   |
|                                                          |
 ----------------------------------------------------------
*/
  index=pv[ply].path[ply] & 4095;
  if (wtm) history_w[index]+=depth*depth;
  else history_b[index]+=depth*depth;
/*
 ----------------------------------------------------------
|                                                          |
|   now, add the same move to the current killer moves if  |
|   it is not already there, otherwise, increment the      |
|   appropriate counter and sort if needed.                |
|                                                          |
 ----------------------------------------------------------
*/
  if (killer_move[ply][0] == pv[ply].path[ply]) killer_move_count[ply][0]++;
  else if (killer_move[ply][1] == pv[ply].path[ply]) {
    killer_move_count[ply][1]++;
    if (killer_move_count[ply][0] < killer_move_count[ply][1]) {
      temp=killer_move_count[ply][0];
      killer_move_count[ply][0]=killer_move_count[ply][1];
      killer_move_count[ply][1]=temp;
      temp=killer_move[ply][0];
      killer_move[ply][0]=killer_move[ply][1];
      killer_move[ply][1]=temp;
    }
  }
  else {
    killer_move_count[ply][1]=1;
    killer_move[ply][1]=pv[ply].path[ply];
  }
}

void HistoryRefutation(int ply, int depth, int wtm)
{
  register int index, temp;
/*
 ----------------------------------------------------------
|                                                          |
|   if the best move so far is a capture or a promotion,   |
|   return, since we try good captures and promotions      |
|   before searching history heuristic moves anyway.       |
|                                                          |
 ----------------------------------------------------------
*/
  if (CaptureOrPromote(current_move[ply])) return;
/*
 ----------------------------------------------------------
|                                                          |
|   otherwise, use the [to,from] as an index and increment |
|   the appropriate history table/entry.                   |
|                                                          |
 ----------------------------------------------------------
*/
  index=current_move[ply] & 4095;
  if (wtm) history_w[index]+=depth*depth;
  else history_b[index]+=depth*depth;
/*
 ----------------------------------------------------------
|                                                          |
|   now, add the same move to the current killer moves if  |
|   it is not already there, otherwise, increment the      |
|   appropriate counter and sort if needed.                |
|                                                          |
 ----------------------------------------------------------
*/
  if (killer_move[ply][0] == current_move[ply]) killer_move_count[ply][0]++;
  else if (killer_move[ply][1] == current_move[ply]) {
    killer_move_count[ply][1]++;
    if (killer_move_count[ply][0] < killer_move_count[ply][1]) {
      temp=killer_move_count[ply][0];
      killer_move_count[ply][0]=killer_move_count[ply][1];
      killer_move_count[ply][1]=temp;
      temp=killer_move[ply][0];
      killer_move[ply][0]=killer_move[ply][1];
      killer_move[ply][1]=temp;
    }
  }
  else {
    killer_move_count[ply][1]=1;
    killer_move[ply][1]=current_move[ply];
  }
}
