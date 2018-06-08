#include <stdio.h>
#include <stdlib.h>
#include "chess.h"
#include "data.h"

/* last modified 10/18/99 */
/*
********************************************************************************
*                                                                              *
*   NextRootMove() is used to select the next move from the root move list.    *
*                                                                              *
********************************************************************************
*/
int NextRootMove(TREE *tree, int wtm) {
  register int done, i;
  char remaining_moves[10];
/*
 ----------------------------------------------------------
|                                                          |
|   for the moves at the root of the tree, the list has    |
|   already been generated and sorted.  on entry, test     |
|   the searched_this_root_move[] array and then take the  |
|   moves in the order they appear in the move list.       |
|                                                          |
 ----------------------------------------------------------
*/
  time_abort+=TimeCheck(1);
  if (time_abort) return(NONE);
  if (!annotate_mode && !pondering && !booking &&
      n_root_moves==1 && iteration_depth>4) {
    abort_search=1;
    return(NONE);
  }
  done=0;
  for (i=0;i<n_root_moves;i++) if (root_moves[i].status&128) done++;
  if (done==1 && (root_moves[0].status&128) && root_value==root_alpha &&
      !(root_moves[0].status&2)) return(NONE);
  for (i=0;i<n_root_moves;i++)
    if (!(root_moves[i].status&128)) {
      if (search_move) {
        if (search_move > 0) {
          if(root_moves[i].move != search_move) {
            root_moves[i].status|=128;
            continue;
          }
        }
        else {
          if(root_moves[i].move == -search_move) {
            root_moves[i].status|=128;
            continue;
          }
        }
      }
      tree->current_move[1]=root_moves[i].move;
      tree->root_move=i;
      root_moves[i].status|=128;
      if ((tree->nodes_searched > noise_level) && (display_options&32)) {
        Lock(lock_io);
        sprintf(remaining_moves,"%d/%d",i+1,n_root_moves);
        end_time=ReadClock(time_type);
        printf("               %2i   %s%7s   ",iteration_depth,
               DisplayTime(end_time-start_time),remaining_moves);
        if (display_options&32 && display_options&64)
          printf("%d. ",move_number);
        if ((display_options&32) && (display_options&64) && !wtm)
          printf("... ");
#if defined(MACOS)
        printf("%s      \n",OutputMove(tree,tree->current_move[1],1,wtm));
#else
        printf("%s      \r",OutputMove(tree,tree->current_move[1],1,wtm));
#endif
        fflush(stdout);
        UnLock(lock_io);
      }
      return(ROOT_MOVES);
    }
  return(NONE);
}

/* last modified 05/10/99 */
/*
********************************************************************************
*                                                                              *
*   NextRootMoveParallel() is used to determine if the next root move can be   *
*   searched in parallel.  if it appears to Iterate() that one of the moves    *
*   following the first move might become the best move, the 'no parallel'     *
*   flag is set to speed up finding the new best move.                         *
*                                                                              *
********************************************************************************
*/
int NextRootMoveParallel(void) {
  register int which;
/*
 ----------------------------------------------------------
|                                                          |
|   for the moves at the root of the tree, the list has    |
|   already been generated and sorted.  on entry, test     |
|   the searched_this_root_move[] array and then take the  |
|   moves in the order they appear in the move list.       |
|                                                          |
 ----------------------------------------------------------
*/
  for (which=0;which<n_root_moves;which++)
    if (!(root_moves[which].status&128)) break;
  if (which < n_root_moves &&
      root_moves[which].status&64) return(0);
  if (root_value>=last_search_value-33 ||
      which > n_root_moves/3) return(1);
  return(0);
}
