#include <stdio.h>
#include <stdlib.h>
#include "chess.h"
#include "data.h"

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
    shared->history[HistoryIndex(searched[count - 1], wtm)].fh += 100;
    if (++shared->history[HistoryIndex(searched[count - 1], wtm)].count > 32767)
      age++;
  }
  for (i = 0; i < count - 1; i++)
    if (!CaptureOrPromote(searched[i]))
      if (++shared->history[HistoryIndex(searched[i], wtm)].count > 32767)
        age++;
  if (tree->thread_id == 0) {
    if (age) {
      for (i = 0; i < 8192; i++) {
        shared->history[i].fh >>= 1;
        shared->history[i].count = (shared->history[i].count >> 1) + 1;
      }
    }
  }
  return;
}
