#include "chess.h"
#include "data.h"
/* last modified 01/14/09 */
/*
 *******************************************************************************
 *                                                                             *
 *   Killer() is used to maintain the two killer moves for each ply.  the most *
 *   recently used killer is always first in the list.                         *
 *                                                                             *
 *******************************************************************************
 */
void Killer(TREE * RESTRICT tree, int ply, int move) {
/*
 ************************************************************
 *                                                          *
 *   If the best move so far is a capture or a promotion,   *
 *   return, since we try good captures and promotions      *
 *   before searching killer heuristic moves anyway.        *
 *                                                          *
 ************************************************************
 */
  if (CaptureOrPromote(move))
    return;
/*
 ************************************************************
 *                                                          *
 *   Now, add this move to the current killer moves if it   *
 *   is not already there.  If the move is already first in *
 *   the list, leave it there, otherwise move the first one *
 *   down to slot two and insert this move into slot one.   *
 *                                                          *
 ************************************************************
 */
  if (tree->killers[ply].move1 != move) {
    tree->killers[ply].move2 = tree->killers[ply].move1;
    tree->killers[ply].move1 = move;
  }
}
