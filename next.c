#include <stdio.h>
#include <stdlib.h>
#include "chess.h"
#include "data.h"

/* last modified 12/26/03 */
/*
 *******************************************************************************
 *                                                                             *
 *   NextMove() is used to select the next move from the current move list.    *
 *                                                                             *
 *******************************************************************************
 */
int NextMove(TREE * RESTRICT tree, int ply, int wtm)
{
  register int *bestp, *movep, *sortv;
  register int history_value, bestval, index;

  switch (tree->next_status[ply].phase) {
/*
 ************************************************************
 *                                                          *
 *   first, try the transposition table move (which will be *
 *   the principal variation move as we first move down the *
 *   tree).                                                 *
 *                                                          *
 ************************************************************
 */
  case HASH_MOVE:
    tree->next_status[ply].phase = GENERATE_CAPTURE_MOVES;
    if (tree->hash_move[ply]) {
      tree->current_move[ply] = tree->hash_move[ply];
#if defined(DEBUG)
      if (ValidMove(tree, ply, wtm, tree->current_move[ply]))
#endif
        return (HASH_MOVE);
#if defined(DEBUG)
      else
        Print(128, "bad move from hash table, ply=%d\n", ply);
#endif
    }
/*
 ************************************************************
 *                                                          *
 *   generate captures and sort them based on (a) the value *
 *   of the captured piece - the value of the capturing     *
 *   piece if this is > 0; or, (b) the value returned by    *
 *   Swap().                                                *
 *                                                          *
 ************************************************************
 */
  case GENERATE_CAPTURE_MOVES:
    tree->next_status[ply].phase = CAPTURE_MOVES;
    tree->last[ply] = GenerateCaptures(tree, ply, wtm, tree->last[ply - 1]);
    tree->next_status[ply].remaining = 0;
    if (tree->hash_move[ply]) {
      for (movep = tree->last[ply - 1], sortv = tree->sort_value;
          movep < tree->last[ply]; movep++, sortv++)
        if (*movep == tree->hash_move[ply]) {
          *sortv = -999999;
          *movep = 0;
          tree->hash_move[ply] = 0;
        } else {
          if (p_values[Piece(*movep) + 7] < p_values[Captured(*movep) + 7]) {
            *sortv =
                p_values[Captured(*movep) + 7] - p_values[Piece(*movep) + 7];
            tree->next_status[ply].remaining++;
          } else {
            *sortv = Swap(tree, From(*movep), To(*movep), wtm);
            if (*sortv >= 0)
              tree->next_status[ply].remaining++;
          }
        }
    } else {
      for (movep = tree->last[ply - 1], sortv = tree->sort_value;
          movep < tree->last[ply]; movep++, sortv++)
        if (p_values[Piece(*movep) + 7] < p_values[Captured(*movep) + 7]) {
          *sortv = p_values[Captured(*movep) + 7] - p_values[Piece(*movep) + 7];
          tree->next_status[ply].remaining++;
        } else {
          *sortv = Swap(tree, From(*movep), To(*movep), wtm);
          if (*sortv >= 0)
            tree->next_status[ply].remaining++;
        }
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
 *   try the captures moves, which are in order based on    *
 *   the expected gain of material.  captures that lose     *
 *   material have been excluded from this phase.           *
 *                                                          *
 ************************************************************
 */
  case CAPTURE_MOVES:
    if (tree->next_status[ply].remaining) {
      tree->current_move[ply] = *(tree->next_status[ply].last);
      *tree->next_status[ply].last++ = 0;
      tree->next_status[ply].remaining--;
      if (!tree->next_status[ply].remaining)
        tree->next_status[ply].phase = KILLER_MOVE_1;
      return (CAPTURE_MOVES);
    }
    tree->next_status[ply].phase = KILLER_MOVE_1;
/*
 ************************************************************
 *                                                          *
 *   now, try the killer moves.  this phase tries the two   *
 *   killers for the current ply without generating moves,  *
 *   which saves time if a cutoff occurs.                   *
 *                                                          *
 ************************************************************
 */
  case KILLER_MOVE_1:
    if ((tree->hash_move[ply] != tree->killers[ply].move1) &&
        ValidMove(tree, ply, wtm, tree->killers[ply].move1)) {
      tree->current_move[ply] = tree->killers[ply].move1;
      tree->next_status[ply].phase = KILLER_MOVE_2;
      return (KILLER_MOVE_1);
    }
  case KILLER_MOVE_2:
    if ((tree->hash_move[ply] != tree->killers[ply].move2) &&
        ValidMove(tree, ply, wtm, tree->killers[ply].move2)) {
      tree->current_move[ply] = tree->killers[ply].move2;
      tree->next_status[ply].phase = GENERATE_ALL_MOVES;
      return (KILLER_MOVE_2);
    }
    tree->next_status[ply].phase = GENERATE_ALL_MOVES;
/*
 ************************************************************
 *                                                          *
 *   now, generate all non-capturing moves.                 *
 *                                                          *
 ************************************************************
 */
  case GENERATE_ALL_MOVES:
    tree->last[ply] = GenerateNonCaptures(tree, ply, wtm, tree->last[ply]);
    tree->next_status[ply].phase = HISTORY_MOVES_1;
/*
 ************************************************************
 *                                                          *
 *   now, try the history moves.  this phase takes the      *
 *   complete move list, and passes over them in a classic  *
 *   selection-sort, choosing the move with the highest     *
 *   history score.  this phase is only done one time, as   *
 *   it also purges the hash and killer moves from the list.*
 *                                                          *
 ************************************************************
 */
  case HISTORY_MOVES_1:
    tree->next_status[ply].remaining = 1;
    tree->next_status[ply].phase = HISTORY_MOVES_2;
    bestval = 0;
    bestp = 0;
    for (movep = tree->last[ply - 1]; movep < tree->last[ply]; movep++)
      if (*movep) {
        if (*movep == tree->hash_move[ply] || *movep == tree->killers[ply].move1
            || *movep == tree->killers[ply].move2)
          *movep = 0;
        else {
          index = *movep & 4095;
          history_value =
              (wtm) ? tree->history_w[index] : tree->history_b[index];
          if (history_value > bestval) {
            bestval = history_value;
            bestp = movep;
          }
        }
      }
    if (bestp) {
      tree->current_move[ply] = *bestp;
      *bestp = 0;
      return (HISTORY_MOVES_1);
    }
    goto remaining_moves;
/*
 ************************************************************
 *                                                          *
 *   now, continue with the history moves, but since one    *
 *   pass has been made over the complete move list, there  *
 *   are no hash/killer moves left in the list, so the test *
 *   for these can be avoided.                              *
 *                                                          *
 ************************************************************
 */
  case HISTORY_MOVES_2:
    bestval = 0;
    bestp = 0;
    for (movep = tree->last[ply - 1]; movep < tree->last[ply]; movep++)
      if (*movep) {
        index = *movep & 4095;
        history_value = (wtm) ? tree->history_w[index] : tree->history_b[index];
        if (history_value > bestval) {
          bestval = history_value;
          bestp = movep;
        }
      }
    if (bestval) {
      tree->current_move[ply] = *bestp;
      *bestp = 0;
      tree->next_status[ply].remaining++;
      if (tree->next_status[ply].remaining > 3) {
        tree->next_status[ply].phase = REMAINING_MOVES;
        tree->next_status[ply].last = tree->last[ply - 1];
      }
      return (HISTORY_MOVES_2);
    }
  remaining_moves:
    tree->next_status[ply].phase = REMAINING_MOVES;
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
      if (*tree->next_status[ply].last) {
        tree->current_move[ply] = *tree->next_status[ply].last;
        *tree->next_status[ply].last++ = 0;
        return (REMAINING_MOVES);
      }
    return (NONE);

  default:
    Print(4095, "oops!  next_status.phase is bad! [normal %d]\n",
        tree->next_status[ply].phase);
    return (NONE);
  }
}
