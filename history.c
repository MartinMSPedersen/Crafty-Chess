#include <stdio.h>
#include <stdlib.h>
#include "chess.h"
#include "data.h"

/* last modified 03/01/06 */
/*
 *******************************************************************************
 *                                                                             *
 *   History() is used to maintain the history database.  this is a set of     *
 *   counts, indexed by the 12-bit value [to,from], that contains the relative *
 *   effectiveness of this move as a refutation.  this effectiveness is simply *
 *   an arbitrary value that varies significantly and inversely to the depth of*
 *   the sub-tree refuted by each move.                                        *
 *                                                                             *
 *   these routines also maintain the killer move lists as well, since they are*
 *   similar in nature.                                                        *
 *                                                                             *
 *******************************************************************************
 */
void History(TREE * RESTRICT tree, int ply, int depth, int wtm, int move)
{
  register int index;

/*
 ************************************************************
 *                                                          *
 *   if the best move so far is a capture or a promotion,   *
 *   return, since we try good captures and promotions      *
 *   before searching history heuristic moves anyway.       *
 *                                                          *
 ************************************************************
 */
  if (CaptureOrPromote(move))
    return;
/*
 ************************************************************
 *                                                          *
 *   otherwise, use the [to,from] as an index and increment *
 *   the appropriate history table/entry.                   *
 *                                                          *
 ************************************************************
 */
  index = HistoryIndex(move, wtm);
  shared->history[index] += depth * depth;
  if (shared->history[index] > 32767) {
    for (index = 0; index < 8192; index++)
      shared->history[index] = shared->history[index] >> 1;
  }
/*
 ************************************************************
 *                                                          *
 *   now, add the same move to the current killer moves if  *
 *   it is not already there.                               *
 *                                                          *
 ************************************************************
 */
  if (tree->killers[ply].move1 != move) {
    tree->killers[ply].move2 = tree->killers[ply].move1;
    tree->killers[ply].move1 = move;
  }
}

/* last modified 03/23/06 */
/*
 *******************************************************************************
 *                                                                             *
 *   HistoryUpdateFH() is used to update the failed-high history information   *
 *   that is used for the late move reduction search enhancement.  the move    *
 *   causing a fail-high or producing a PV move gets both the use counter and  *
 *   the fail-high counters updated, while the rest of the moves that did not  *
 *   produce a fail-high or PV move just get the counters updated, showing     *
 *   that they were searched but did not produce anything useful.              *
 *                                                                             *
 *   if a counter goes beyond 32767, all values are divided by 2 to prevent    *
 *   the potential for overflow later on.  since this data is shared among all *
 *   processes, only thread_id==0 will do the division, otherwise all could    *
 *   do this at the same time and divide by 4 (with two threads) or by 16      *
 *   (with four threads).                                                      *
 *                                                                             *
 *******************************************************************************
 */
void HistoryUpdateFH(TREE * RESTRICT tree, int wtm, int searched[], int count)
{
  int i, age = 0;

  if (!CaptureOrPromote(searched[count - 1])) {
    shared->history_fh[HistoryIndex(searched[count - 1], wtm)] += 100;
    if (++shared->history_count[HistoryIndex(searched[count - 1], wtm)] > 32767)
      age++;
  }
  for (i = 0; i < count - 1; i++)
    if (!CaptureOrPromote(searched[i]))
      if (++shared->history_count[HistoryIndex(searched[i], wtm)] > 32767)
        age++;
  if (tree->thread_id == 0) {
    if (age) {
      for (i = 0; i < 8192; i++) {
        shared->history_fh[i] >>= 1;
        shared->history_count[i] = (shared->history_count[i] >> 1) + 1;
      }
    }
  }
  return;
}
