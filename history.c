#include <stdio.h>
#include <stdlib.h>
#include "chess.h"
#include "data.h"

/* last modified 12/26/03 */
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
void History(TREE * RESTRICT tree, int ply, int histval, int wtm, int move)
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
  index = move & 4095;
  if (wtm)
    tree->history_w[index] += histval;
  else
    tree->history_b[index] += histval;
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

/* last modified 02/09/06 */
/*
 *******************************************************************************
 *                                                                             *
 *   HistoryAge() is called to "age" the history values over time.  this idea  *
 *   is similar to the way the Unix operating system "ages" priorities by      *
 *   basing them on accumulated CPU time that is periodically divided by two   *
 *   to cause unused values to drift toward zero while values that are being   *
 *   updated regularly recover from the aging as they are used.                *
 *                                                                             *
 *   this is called from search whenever a "time check" is done, although it   *
 *   is never done more than once per second to avoid collapsing all values    *
 *   toward zero too quickly.                                                  *
 *                                                                             *
 *******************************************************************************
 */
void HistoryAge(TREE * RESTRICT tree)
{
  int i;

  for (i = 0; i < 4096; i++) {
    tree->history_w[i] = tree->history_w[i] >> 1;
    tree->history_b[i] = tree->history_b[i] >> 1;
  }
}
