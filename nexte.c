#include "chess.h"
#include "data.h"
/* last modified 01/22/04 */
/*
 *******************************************************************************
 *                                                                             *
 *   NextEvasion() is used to select the next move from the current move list  *
 *   when the king is in check.  we use GenerateEvasions() (in movgen.c) to    *
 *   generate a list of moves that get us out of check.  The only unusual      *
 *   feature is that these moves are all legal and do not need to be vetted    *
 *   with the usual Check() function to test for legality.                     *
 *                                                                             *
 *******************************************************************************
 */
int NextEvasion(TREE * RESTRICT tree, int ply, int wtm)
{
  register int *movep, *sortv;

  switch (tree->next_status[ply].phase) {
/*
 ************************************************************
 *                                                          *
 *   first generate all legal moves by using the special    *
 *   GenerateCheckEvasions() function, so we can determine  *
 *   if this is a one-legal-reply-to-check position.        *
 *                                                          *
 ************************************************************
 */
  case HASH_MOVE:
    tree->last[ply] =
        GenerateCheckEvasions(tree, ply, wtm, tree->last[ply - 1]);
/*
 ************************************************************
 *                                                          *
 *   now try the transposition table move (which might be   *
 *   the principal variation move as we first move down the *
 *   tree).                                                 *
 *                                                          *
 ************************************************************
 */
    if (tree->hash_move[ply]) {
      tree->next_status[ply].phase = SORT_ALL_MOVES;
      tree->curmv[ply] = tree->hash_move[ply];
      if (ValidMove(tree, ply, wtm, tree->curmv[ply]))
        return (HASH_MOVE);
#if defined(DEBUG)
      else
        Print(128, "bad move from hash table, ply=%d\n", ply);
#endif
    }
/*
 ************************************************************
 *                                                          *
 *   now sort the moves based on the expected gain or loss. *
 *   this is deferred until now to see if the hash move is  *
 *   good enough to produce a cutoff and avoid this work.   *
 *                                                          *
 ************************************************************
 */
  case SORT_ALL_MOVES:
    tree->next_status[ply].phase = REMAINING_MOVES;
    if (tree->hash_move[ply]) {
      for (movep = tree->last[ply - 1], sortv = tree->sort_value;
          movep < tree->last[ply]; movep++, sortv++)
        if (*movep == tree->hash_move[ply]) {
          *sortv = -999999;
          *movep = 0;
        } else {
          if (pc_values[Piece(*movep)] < pc_values[Captured(*movep)])
            *sortv = pc_values[Captured(*movep)] - pc_values[Piece(*movep)];
          else
            *sortv = Swap(tree, From(*movep), To(*movep), wtm);
        }
    } else {
      for (movep = tree->last[ply - 1], sortv = tree->sort_value;
          movep < tree->last[ply]; movep++, sortv++)
        if (pc_values[Piece(*movep)] < pc_values[Captured(*movep)])
          *sortv = pc_values[Captured(*movep)] - pc_values[Piece(*movep)];
        else
          *sortv = Swap(tree, From(*movep), To(*movep), wtm);
    }
/*
 ************************************************************
 *                                                          *
 *   this is a simple insertion sort algorithm.  it seems   *
 *   be no faster than a normal bubble sort, but using this *
 *   eliminated a lot of explaining about "why?". :)        *
 *                                                          *
 ************************************************************
 */
    if (tree->last[ply] > tree->last[ply - 1] + 1) {
      int temp1, temp2, *tmovep, *tsortv;
      int *end;

      sortv = tree->sort_value + 1;
      end = tree->last[ply];
      for (movep = tree->last[ply - 1] + 1; movep < end; movep++, sortv++) {
        temp1 = *movep;
        temp2 = *sortv;
        tmovep = movep - 1;
        tsortv = sortv - 1;
        while (tmovep >= tree->last[ply - 1] && *tsortv < temp2) {
          *(tsortv + 1) = *tsortv;
          *(tmovep + 1) = *tmovep;
          tmovep--;
          tsortv--;
        }
        *(tmovep + 1) = temp1;
        *(tsortv + 1) = temp2;
      }
    }
    tree->next_status[ply].last = tree->last[ply - 1];
/*
 ************************************************************
 *                                                          *
 *   now try the rest of the set of moves.                  *
 *                                                          *
 ************************************************************
 */
  case REMAINING_MOVES:
    for (; tree->next_status[ply].last < tree->last[ply];
        tree->next_status[ply].last++)
      if ((*tree->next_status[ply].last)) {
        tree->curmv[ply] = *tree->next_status[ply].last++;
        return (REMAINING_MOVES);
      }
    return (NONE);
  default:
    printf("oops!  next_status.phase is bad! [evasion %d]\n",
        tree->next_status[ply].phase);
    return (NONE);
  }
}
