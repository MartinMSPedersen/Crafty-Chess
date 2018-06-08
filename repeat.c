#include "chess.h"
#include "data.h"
/* last modified 01/29/08 */
/*
 *******************************************************************************
 *                                                                             *
 *   RepetitionCheck() is used to detect a draw by repetition.  the repetition *
 *   list is a 2d array indexed by wtm (0,1) and the repetition indices which  *
 *   are also indexed by wtm.  this keeps positions with wtm separate from the *
 *   positions where it is btm, so that we can easily check the positions for  *
 *   the right side on move without having to wade through the positions with  *
 *   the wrong side on move as well.  positions are only added to this list in *
 *   MakeMove() which alsi increments the proper index.  UnmakeMove() then     *
 *   decrements the proper index when the move is retracted, effectively       *
 *   removing the position from the list.                                      *
 *                                                                             *
 *   RepetitionCheck() scans the list to determine if this position has        *
 *   occurred before.  if so, the position will be treated as a draw by        *
 *   Search().                                                                 *
 *                                                                             *
 *   RepetitionCheck() also handles 50-move draws.  the position[] structure   *
 *   countains the count of moves since the last capture or pawn push.  when   *
 *   this value reaches 100 (plies, which is 50 moves) the score is set to     *
 *   DRAW.                                                                     *
 *                                                                             *
 *******************************************************************************
 */
int RepetitionCheck(TREE * RESTRICT tree, int ply, int wtm)
{
  register int where, loops;

/*
 ************************************************************
 *                                                          *
 *   if the 50-move rule has been reached, then adjust the  *
 *   score to reflect the impending draw.                   *
 *                                                          *
 ************************************************************
 */
  if (Rule50Moves(ply) > 99)
    return (1);
/*
 ************************************************************
 *                                                          *
 *   now we scan the right part of the repetition list, and *
 *   stop when we reach the current repetition index value  *
 *   since positions beyond that index are not valid.  at   *
 *   plies 2 and 3 (this is never called at ply=1) we look  *
 *   for 3 repetitions (a real draw) where at plies deeper  *
 *   than 3, two repetitions are considered a draw.  this   *
 *   avoids very short PVs with 2-move repetitions cutting  *
 *   them off, although deeper PVs can still be truncated   *
 *   by the second repetition.                              *
 *                                                          *
 ************************************************************
 */
  loops = (Rule50Moves(ply) + 1) >> 1;
  for (where = tree->rep_index[wtm] - 1; where >= 0; where--) {
    if (loops-- <= 0)
      break;
    if (HashKey == tree->rep_list[wtm][where])
      return (1);
  }
  return (0);
}

/* last modified 01/28/08 */
/*
 *******************************************************************************
 *                                                                             *
 *   RepetitionCheckBook() is used to detect a draw by repetition while still  *
 *   in book.  It simply counts two repeats as a draw as there is little sense *
 *   in repeating a position at all in the opening.                            *
 *                                                                             *
 *******************************************************************************
 */
int RepetitionCheckBook(TREE * RESTRICT tree, int ply, int wtm)
{
  register int where;

/*
 ************************************************************
 *                                                          *
 *   if the 50-move rule has been reached, then adjust the  *
 *   score to reflect the impending draw.                   *
 *                                                          *
 ************************************************************
 */
  if (Rule50Moves(ply) > 99)
    return (1);
/*
 ************************************************************
 *                                                          *
 *   scan the list.  we look for the case where the         *
 *   position has been seen one time before.                *
 *                                                          *
 ************************************************************
 */
  for (where = 0; where < tree->rep_index[wtm]; where++)
    if (HashKey == tree->rep_list[wtm][where])
      return (1);
  return (0);
}

/* last modified 01/28/08 */
/*
 *******************************************************************************
 *                                                                             *
 *   RepetitionDraw() is used to detect a draw by repetition.  this routine is *
 *   only called from Main() and simply scans the complete list searching for  *
 *   exactly three repetitions (two additional repetitions of the current      *
 *   position.)  this is used to actually claim a draw by repetition or by the *
 *   50 move rule.                                                             *
 *                                                                             *
 *******************************************************************************
 */
int RepetitionDraw(TREE * RESTRICT tree, int ply, int wtm)
{
  register int reps;
  int where;

/*
 ************************************************************
 *                                                          *
 *   if the 50-move rule has been reached, then adjust the  *
 *   score to reflect the impending draw.                   *
 *                                                          *
 ************************************************************
 */
  if (Rule50Moves(ply) > 99)
    return (2);
/*
 ************************************************************
 *                                                          *
 *   scan the repetition list to determine if this position *
 *   has occurred before.                                   *
 *                                                          *
 ************************************************************
 */
  reps = 0;
  for (where = 0; where < tree->rep_index[wtm]; where++)
    if (HashKey == tree->rep_list[wtm][where])
      reps++;
  return (reps == 2);
}
