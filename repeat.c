#include "chess.h"
#include "data.h"
/* last modified 11/05/10 */
/*
 *******************************************************************************
 *                                                                             *
 *   RepetitionCheck() is used to detect a draw by repetition.  The repetition *
 *   list is a 2d array indexed by wtm (0,1) and the repetition indices which  *
 *   are also indexed by wtm.  This keeps positions with wtm separate from the *
 *   positions where it is btm, so that we can easily check the positions for  *
 *   the right side on move without having to wade through the positions with  *
 *   the wrong side on move as well.  Positions are only added to this list in *
 *   MakeMove() which alsi increments the proper index.  UnmakeMove() then     *
 *   decrements the proper index when the move is retracted, effectively       *
 *   removing the position from the list.                                      *
 *                                                                             *
 *   RepetitionCheck() scans the list to determine if this position has        *
 *   occurred before.  If so, the position will be treated as a draw by        *
 *   Search().                                                                 *
 *                                                                             *
 *   RepetitionCheck() also handles 50-move draws.  The position[] structure   *
 *   countains the count of moves since the last capture or pawn push.  When   *
 *   this value reaches 100 (plies, which is 50 moves) the score is set to     *
 *   DRAW.                                                                     *
 *                                                                             *
 *******************************************************************************
 */
int RepetitionCheck(TREE * RESTRICT tree, int ply, int wtm) {
  int where;

/*
 ************************************************************
 *                                                          *
 *   If the 50-move rule has been reached, then adjust the  *
 *   score to reflect the impending draw.  If we have not   *
 *   made 2 moves for each side (or more) since the last    *
 *   irreversible move, there is no way to repeat a prior   *
 *   position.                                              *
 *                                                          *
 ************************************************************
 */
  if (Rule50Moves(ply) < 4)
    return (0);
  if (Rule50Moves(ply) > 99)
    return (2);
/*
 ************************************************************
 *                                                          *
 *   Now we scan the right part of the repetition list, and *
 *   stop when we reach the current repetition index value  *
 *   since positions beyond that index are not valid.       *
 *                                                          *
 ************************************************************
 */
  for (where = Repetition(wtm) - 2 + (ply - 1) / 2; where >= 0; where--)
    if (HashKey == tree->rep_list[wtm][where])
      return (1);
  return (0);
}

/* last modified 11/05/10 */
/*
 *******************************************************************************
 *                                                                             *
 *   RepetitionCheckBook() is used to detect a draw by repetition while still  *
 *   in book.  It simply counts two repeats as a draw as there is little sense *
 *   in repeating a position at all in the opening.                            *
 *                                                                             *
 *******************************************************************************
 */
int RepetitionCheckBook(TREE * RESTRICT tree, int ply, int wtm) {
  int where;

/*
 ************************************************************
 *                                                          *
 *   If the 50-move rule has been reached, then adjust the  *
 *   score to reflect the impending draw.                   *
 *                                                          *
 ************************************************************
 */
  if (Rule50Moves(ply) > 99)
    return (1);
/*
 ************************************************************
 *                                                          *
 *   Scan the list.  We look for the case where the         *
 *   position has been seen one time before.                *
 *                                                          *
 ************************************************************
 */
  for (where = 0; where < Repetition(wtm); where++)
    if (HashKey == tree->rep_list[wtm][where])
      return (1);
  return (0);
}

/* last modified 11/05/10 */
/*
 *******************************************************************************
 *                                                                             *
 *   RepetitionDraw() is used to detect a draw by repetition.  This routine is *
 *   only called from Main() and simply scans the complete list searching for  *
 *   exactly three repetitions (two additional repetitions of the current      *
 *   position.)  This is used to actually claim a draw by repetition or by the *
 *   50 move rule.                                                             *
 *                                                                             *
 *******************************************************************************
 */
int RepetitionDraw(TREE * RESTRICT tree, int wtm) {
  int reps;
  int where;

/*
 ************************************************************
 *                                                          *
 *   If the 50-move rule has been reached, then adjust the  *
 *   score to reflect the impending draw.                   *
 *                                                          *
 ************************************************************
 */
  if (Rule50Moves(0) > 99)
    return (2);
/*
 ************************************************************
 *                                                          *
 *   Scan the repetition list to determine if this position *
 *   has occurred before.                                   *
 *                                                          *
 ************************************************************
 */
  reps = 0;
  for (where = 0; where < Repetition(wtm); where++)
    if (HashKey == tree->rep_list[wtm][where])
      reps++;
  return (reps == 2);
}
