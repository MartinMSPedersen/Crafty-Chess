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
  BITBOARD total_nodes;

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
  time_abort += TimeCheck(tree, 1);
  if (time_abort)
    return (NONE);
  if (!annotate_mode && !pondering && !booking && n_root_moves == 1 &&
      iteration_depth > 4) {
    abort_search = 1;
    return (NONE);
  }
  done = 0;
  for (which = 0; which < n_root_moves; which++)
    if (root_moves[which].status & 256)
      done++;
  if (done == 1 && (root_moves[0].status & 256) && root_value == root_alpha &&
      !(root_moves[0].status & 0x38))
    return (NONE);
  for (which = 0; which < n_root_moves; which++)
    if (!(root_moves[which].status & 256)) {
      if (search_move) {
        if (root_moves[which].move != search_move) {
          root_moves[which].status |= 256;
          continue;
        }
      }
      tree->curmv[1] = root_moves[which].move;
      tree->root_move = which;
      root_moves[which].status |= 256;
      if ((tree->nodes_searched > noise_level) && (display_options & 32)) {
        Lock(lock_io);
        sprintf(mytree->remaining_moves_text, "%d/%d", which + 1, n_root_moves);
        end_time = ReadClock();
        if (pondering)
          printf("               %2i   %s%7s?  ", iteration_depth,
              DisplayTime(end_time - start_time), mytree->remaining_moves_text);
        else
          printf("               %2i   %s%7s*  ", iteration_depth,
              DisplayTime(end_time - start_time), mytree->remaining_moves_text);
        if (display_options & 32 && display_options & 64)
          printf("%d. ", move_number);
        if ((display_options & 32) && (display_options & 64) && Flip(wtm))
          printf("... ");
        strcpy(mytree->root_move_text, OutputMove(tree, tree->curmv[1], 1,
                wtm));
        total_nodes = block[0]->nodes_searched;
        for (i = 1; i < MAX_BLOCKS; i++)
          if (block[i] && block[i]->used)
            total_nodes += block[i]->nodes_searched;
        nodes_per_second = total_nodes * 100 / Max(end_time - start_time, 1);
        i = strlen(mytree->root_move_text);
        i = (i < 8) ? i : 8;
        strncat(mytree->root_move_text, "          ", 8 - i);
        printf("%s", mytree->root_move_text);
        printf("(%snps)             \r", DisplayKM(nodes_per_second));
        fflush(stdout);
        Unlock(lock_io);
      }
      if (!(root_moves[which].status & 128))
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
  for (which = 0; which < n_root_moves; which++)
    if (!(root_moves[which].status & 256))
      break;
  if (which < n_root_moves && root_moves[which].status & 64)
    return (0);
  if (root_value >= last_root_value - 33 || which > n_root_moves / 3)
    return (1);
  return (0);
}
