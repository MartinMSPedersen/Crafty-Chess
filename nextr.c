#include <stdio.h>
#include <stdlib.h>
#include "chess.h"
#include "data.h"

/* last modified 01/18/99 */
/*
********************************************************************************
*                                                                              *
*   NextRootMove() is used to select the next move from the root move list.    *
*                                                                              *
********************************************************************************
*/
int NextRootMove(TREE *tree, int wtm)
{
  register int nodes_per_second, done, *movep;
  char remaining_moves[10];
  static int adjusted=0;
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
  if (time_abort) {
    abort_search=1;
    return(NONE);
  }
  done=0;
  if (!annotate_mode && !pondering && !booking &&
      tree->last[1]-tree->last[0]==1 && iteration_depth>4) {
    abort_search=1;
    return(NONE);
  }
  if (!adjusted) {
    if (iteration_depth>4 && tree->egtb_probes) {
      time_used=ReadClock(time_type)-program_start_time+1;
      nodes_per_second=(BITBOARD) tree->nodes_searched*100/time_used;
      if (nodes_per_second<average_nps/8) {
        EGTB_maxdepth=Max(EGTB_maxdepth-3,6);
        adjusted=3;
      }
      else if (nodes_per_second<average_nps/4) {
        EGTB_maxdepth=Max(EGTB_maxdepth-2,6);
        adjusted=3;
      }
      else if (nodes_per_second<average_nps/2) {
        EGTB_maxdepth=Max(EGTB_maxdepth-1,6);
        adjusted=3;
      }
      else if (EGTB_maxdepth<32 && nodes_per_second>3*average_nps/4) {
        if (adjusted >= 0) {
          EGTB_maxdepth=Min(EGTB_maxdepth+1,iteration_depth+4);
          adjusted=3;
        }
        else adjusted=0;
      }
      else adjusted=0;
#if defined(DEBUG)
      if (adjusted) {
        Print(128,"nps=%d  avgnps=%d\n",nodes_per_second,average_nps);
        Print(128,"limiting EGTB probes to first %d plies.\n",EGTB_maxdepth);
      }
#endif
    }
  }
  else adjusted--;
  for (movep=tree->last[0];movep<tree->last[1];movep++)
    if (tree->searched_this_root_move[movep-tree->last[0]]) done++;
  if ((done==1) && tree->searched_this_root_move[0] &&
      (root_value==root_alpha) && !search_failed_high) return(NONE);

  for (movep=tree->last[0];movep<tree->last[1];movep++)
    if (!tree->searched_this_root_move[movep-tree->last[0]]) {
      if (search_move) {
        if (search_move > 0) {
          if(*movep != search_move) {
            tree->searched_this_root_move[movep-tree->last[0]]=1;
            continue;
          }
        }
        else {
          if(*movep == -search_move) {
            tree->searched_this_root_move[movep-tree->last[0]]=1;
            continue;
          }
        }
      }
      tree->current_move[1]=*movep;
      root_move=movep-tree->last[0];
      tree->searched_this_root_move[root_move]=1;
      if ((tree->nodes_searched > noise_level) && (display_options&32)) {
        Lock(lock_io);
        sprintf(remaining_moves,"%d/%d",movep-tree->last[0]+1,tree->last[1]-tree->last[0]);
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

