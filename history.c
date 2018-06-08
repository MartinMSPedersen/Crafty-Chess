#include "chess.h"
#include "data.h"
/* last modified 06/23/14 */
/*
 *******************************************************************************
 *                                                                             *
 *   History() is used to maintain the two killer moves for each ply.  The     *
 *   most recently used killer is always first in the list.                    *
 *                                                                             *
 *   History() also maintains the history counters.  Each time a move fails    *
 *   high, it's history count is incremented by depth * depth.  All the moves  *
 *   prior to that move have their history counters decremented by the 2x that *
 *   amount, so that likely fail-high moves get searched first.                *
 *                                                                             *
 *******************************************************************************
 */
void History(TREE * RESTRICT tree, int ply, int depth, int side, int move,
    int searched[], int count) {
  int i, index, age = 0;
/*
 ************************************************************
 *                                                          *
 *  If the best move so far is a capture or a promotion,    *
 *  return, since we try good captures and promotions       *
 *  before searching killer heuristic moves anyway.         *
 *                                                          *
 ************************************************************
 */
  if (CaptureOrPromote(move))
    return;
/*
 ************************************************************
 *                                                          *
 *  Now, add this move to the current killer moves if it is *
 *  not already there.  If the move is already first in the *
 *  list, leave it there, otherwise move the first one down *
 *  to slot two and insert this move into slot one.         *
 *                                                          *
 ************************************************************
 */
  if (tree->killers[ply].move1 != move) {
    tree->killers[ply].move2 = tree->killers[ply].move1;
    tree->killers[ply].move1 = move;
  }
/*
 ************************************************************
 *                                                          *
 *  Adjust the history counter for the move that caused the *
 *  fail-high, limiting the max value to 2^20.              *
 *                                                          *
 ************************************************************
 */
  if (depth > 5) {
    index = HistoryIndex(move);
    history[side][index] += depth * depth;
    if (history[side][index] > (1 << 20))
      age = 1;
/*
 ************************************************************
 *                                                          *
 *  Adjust the history counters for the moves that were     *
 *  searched but did not cause a fail-high, limiting the    *
 *  min value to -2^20.                                     *
 *                                                          *
 ************************************************************
 */
    for (i = 0; i < count; i++) {
      index = HistoryIndex(searched[i]);
      history[side][index] -= depth * depth * 2;
      if (history[side][index] <= (-1 << 20))
        age = 1;
    }
/*
 ************************************************************
 *                                                          *
 *  If any value reached +/-2^20, we divide all entries for *
 *  this side by 4 so that they don't threaten to wrap      *
 *  around which would wreck the counters.                  *
 *                                                          *
 ************************************************************
 */
    if (age)
      for (i = 0; i < 512; i++)
        history[side][i] = history[side][i] >> 4;
  }
}

/* last modified 06/23/14 */
/*
 *******************************************************************************
 *                                                                             *
 *   HistoryAge() is used to age the history data.  Since we use saturating    *
 *   counters (counters reach 2^16 and stop increasing, or -2^16 and stop      *
 *   decreasing, we want to prevent counters from reaching some high value     *
 *   and being accessed rarely, but then giving a bogus impression about how   *
 *   good or bad the move is.  We do this at the start of a new search (not a  *
 *   new iteration) and we divide every value by 256.                          *
 *                                                                             *
 *******************************************************************************
 */
void HistoryAge() {
  int side, i;

  for (side = black; side <= white; side++)
    for (i = 0; i < 512; i++)
      history[side][i] = history[side][i] / 256;
}
