#include "chess.h"
#include "data.h"

/* last modified 10/31/07 */
/*
 *******************************************************************************
 *                                                                             *
 *   NextRootMove() is used to select the next move from the root move list.   *
 *                                                                             *
 *******************************************************************************
 */
int NextRootMove(TREE * RESTRICT tree, TREE * RESTRICT mytree, int wtm)
{
  register int done, which, i;

/*
 ************************************************************
 *                                                          *
 *   for the moves at the root of the tree, the list has    *
 *   already been generated and sorted.  on entry, test     *
 *   the searched_this_root_move[] array and then take the  *
 *   moves in the order they appear in the move list.       *
 *                                                          *
 ************************************************************
 */
  shared->time_abort += TimeCheck(tree, 1);
  if (shared->time_abort)
    return (NONE);
  if (!annotate_mode && !shared->pondering && !shared->booking &&
      shared->n_root_moves == 1 && shared->iteration_depth > 4) {
    shared->abort_search = 1;
    return (NONE);
  }
  done = 0;
  for (which = 0; which < shared->n_root_moves; which++)
    if (shared->root_moves[which].status & 256)
      done++;
  if (done == 1 && (shared->root_moves[0].status & 256) &&
      shared->root_value == shared->root_alpha &&
      !(shared->root_moves[0].status & 0x38))
    return (NONE);
  for (which = 0; which < shared->n_root_moves; which++)
    if (!(shared->root_moves[which].status & 256)) {
      if (search_move) {
        if (shared->root_moves[which].move != search_move) {
          shared->root_moves[which].status |= 256;
          continue;
        }
      }
      tree->curmv[1] = shared->root_moves[which].move;
      tree->root_move = which;
      shared->root_moves[which].status |= 256;
      if ((tree->nodes_searched > shared->noise_level) &&
          (shared->display_options & 32)) {
        Lock(shared->lock_io);
        sprintf(mytree->remaining_moves_text, "%d/%d", which + 1,
            shared->n_root_moves);
        shared->end_time = ReadClock();
        if (shared->pondering)
          printf("               %2i   %s%7s?  ", shared->iteration_depth,
              DisplayTime(shared->end_time - shared->start_time),
              mytree->remaining_moves_text);
        else
          printf("               %2i   %s%7s*  ", shared->iteration_depth,
              DisplayTime(shared->end_time - shared->start_time),
              mytree->remaining_moves_text);
        if (shared->display_options & 32 && shared->display_options & 64)
          printf("%d. ", shared->move_number);
        if ((shared->display_options & 32) && (shared->display_options & 64) &&
            Flip(wtm))
          printf("... ");
        strcpy(mytree->root_move_text, OutputMove(tree, tree->curmv[1], 1,
                wtm));
        shared->nodes_per_second =
            tree->nodes_searched * 100 / Max(shared->end_time -
            shared->start_time, 1);
        i = strlen(mytree->root_move_text);
        i = (i < 8) ? i : 8;
        strncat(mytree->root_move_text, "          ", 8 - i);
        printf("%s", mytree->root_move_text);
        printf("(%snps)             \r", DisplayKM(shared->nodes_per_second));
        fflush(stdout);
        Unlock(shared->lock_io);
      }
      if (!(shared->root_moves[which].status & 128))
        return (HASH_MOVE);
      else
        return (REMAINING_MOVES);
    }
  return (NONE);
}

/* last modified 08/07/05 */
/*
 *******************************************************************************
 *                                                                             *
 *   NextRootMoveParallel() is used to determine if the next root move can be  *
 *   searched in parallel.  if it appears to Iterate() that one of the moves   *
 *   following the first move might become the best move, the 'no parallel'    *
 *   flag is set to speed up finding the new best move.                        *
 *                                                                             *
 *******************************************************************************
 */
int NextRootMoveParallel(void)
{
  register int which;

/*
 ************************************************************
 *                                                          *
 *   first, find out how far down the list we have searched *
 *   already.  if the next move is flagged as "do not       *
 *   search in parallel" then return 1 unless the score has *
 *   dropped significantly.  if the score has dropped, then *
 *   we search serially to find a better move quickly.      *
 *                                                          *
 ************************************************************
 */
  for (which = 0; which < shared->n_root_moves; which++)
    if (!(shared->root_moves[which].status & 256))
      break;
  if (which < shared->n_root_moves && shared->root_moves[which].status & 64)
    return (0);
  if (shared->root_value >= shared->last_root_value - 33 ||
      which > shared->n_root_moves / 3)
    return (1);
  return (0);
}
