#include <stdio.h>
#include <stdlib.h>
#include "chess.h"
#include "data.h"

/* last modified 12/23/12 */
/*
 *******************************************************************************
 *                                                                             *
 *   History() is used to update the failed-high history information that is   *
 *   used for the late move reduction search enhancement.  the move causing a  *
 *   fail-high or producing a PV move gets both the history counter and the    *
 *   fail-high counters updated, while the rest of the moves that did not      *
 *   produce a fail-high or PV move just get the fail-low counters updated,    *
 *   showing that they were searched but did not produce anything useful.      *
 *                                                                             *
 *   if a counter goes beyond 65536, all values are divided by 2 to prevent    *
 *   the potential for overflow later on.  since this data is shared among all *
 *   processes, only thread_id==0 will do the division, otherwise all could    *
 *   do this at the same time and divide by 4 (with two threads) or by 16      *
 *   (with four threads).                                                      *
 *                                                                             *
 *******************************************************************************
 */
/*
void History(TREE * RESTRICT tree, int wtm, int depth, int searched[],
    int count) {
  int i;

  history[wtm][HistoryIndex(searched[count - 1])].fh += depth * depth;
  if (history[wtm][HistoryIndex(searched[count - 1])].fh > 65536)
    history_age++;
  for (i = 0; i < count - 1; i++)
    history[wtm][HistoryIndex(searched[i])].fl += depth * depth;
  if (tree->thread_id == 0 && history_age) {
    history_age = 0;
    for (wtm = 0; wtm < 2; wtm++)
      for (i = 0; i < 512; i++) {
        history[wtm][i].fh >>= 2;
        history[wtm][i].fl >>= 2;
      }
  }
  return;
}
*/
