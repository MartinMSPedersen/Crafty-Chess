#include "chess.h"
#include "data.h"
/* last modified 05/15/14 */
/*
 *******************************************************************************
 *                                                                             *
 *   NextEvasion() is used to select the next move from the current move list  *
 *   when the king is in check.  We use GenerateEvasions() (in movgen.c) to    *
 *   generate a list of moves that get us out of check.  The only unusual      *
 *   feature is that these moves are all legal and do not need to be vetted    *
 *   with the usual Check() function to test for legality.                     *
 *                                                                             *
 *******************************************************************************
 */
int NextEvasion(TREE * RESTRICT tree, int ply, int wtm) {
  int *movep, *sortv;

  switch (tree->next_status[ply].phase) {
/*
 ************************************************************
 *                                                          *
 *  First try the transposition table move (which might be  *
 *  the principal variation move as we first move down the  *
 *  tree).  If it is good enough to cause a cutoff, we      *
 *  avoided the overhead of generating legal moves.         *
 *                                                          *
 ************************************************************
 */
    case HASH_MOVE:
      if (tree->hash_move[ply]) {
        tree->next_status[ply].phase = GENERATE_ALL_MOVES;
        tree->curmv[ply] = tree->hash_move[ply];
        if (ValidMove(tree, ply, wtm, tree->curmv[ply]))
          return HASH_MOVE;
#if defined(DEBUG)
        else
          Print(128, "bad move from hash table, ply=%d\n", ply);
#endif
      }
/*
 ************************************************************
 *                                                          *
 *  Now generate all legal moves by using the special       *
 *  GenerateCheckEvasions() procedure.  Then sort the moves *
 *  based on the expected gain or loss.  this is deferred   *
 *  until now to see if the hash move is good enough to     *
 *  produce a cutoff and avoid this effort.                 *
 *                                                          *
 *  Once we confirm that the move does not lose any         *
 *  material, we sort these non-losing moves into MVV/LVA   *
 *  order which appears to be a slightly faster move        *
 *  ordering idea.  Unsafe evasion moves are sorted using   *
 *  the original Swap() score to keep them last in the move *
 *  list.  Note that this move list contains both captures  *
 *  and non-captures.  We try the safe captures first due   *
 *  to the way the sort score is computed.                  *
 *                                                          *
 ************************************************************
 */
    case GENERATE_ALL_MOVES:
      tree->last[ply] =
          GenerateCheckEvasions(tree, ply, wtm, tree->last[ply - 1]);
      tree->next_status[ply].phase = REMAINING_MOVES;
      for (movep = tree->last[ply - 1], sortv = tree->sort_value;
          movep < tree->last[ply]; movep++, sortv++)
        if (tree->hash_move[ply] && *movep == tree->hash_move[ply]) {
          *sortv = -999999;
          *movep = 0;
        } else {
          if (pcval[Piece(*movep)] <= pcval[Captured(*movep)])
            *sortv = 128 * pcval[Captured(*movep)] - pcval[Piece(*movep)];
          else {
            *sortv = Swap(tree, *movep, wtm);
            if (*sortv >= 0)
              *sortv = 128 * pcval[Captured(*movep)] - pcval[Piece(*movep)];
          }
        }
/*
 ************************************************************
 *                                                          *
 *  This is a simple insertion sort algorithm.  It seems be *
 *  be no faster than a normal bubble sort, but using this  *
 *  eliminated a lot of explaining about "why?". :)         *
 *                                                          *
 ************************************************************
 */
      if (tree->last[ply] > tree->last[ply - 1] + 1) {
        int temp1, temp2, *tmovep, *tsortv, *end;

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
 *  Now try the moves in sorted order.                      *
 *                                                          *
 ************************************************************
 */
    case REMAINING_MOVES:
      for (; tree->next_status[ply].last < tree->last[ply];
          tree->next_status[ply].last++)
        if (*tree->next_status[ply].last) {
          tree->curmv[ply] = *tree->next_status[ply].last++;
          return REMAINING_MOVES;
        }
      return NONE;
    default:
      printf("oops!  next_status.phase is bad! [evasion %d]\n",
          tree->next_status[ply].phase);
  }
  return NONE;
}

/* last modified 05/15/14 */
/*
 *******************************************************************************
 *                                                                             *
 *   NextMove() is used to select the next move from the current move list.    *
 *                                                                             *
 *   The "excluded move" code below simply collects any moves that were        *
 *   searched without being generated (hash move and up to 4 killers).  We     *
 *   save them in the NEXT structure and make sure to exclude them when        *
 *   searching after a move generation to avoid the duplicated effort.         *
 *                                                                             *
 *******************************************************************************
 */
int NextMove(TREE * RESTRICT tree, int ply, int wtm) {
  int *movep, *sortv;

  switch (tree->next_status[ply].phase) {
/*
 ************************************************************
 *                                                          *
 *  First, try the transposition table move (which will be  *
 *  the principal variation move as we first move down the  *
 *  tree).                                                  *
 *                                                          *
 ************************************************************
 */
    case HASH_MOVE:
      tree->next_status[ply].num_excluded = 0;
      tree->next_status[ply].phase = GENERATE_CAPTURE_MOVES;
      if (tree->hash_move[ply]) {
        tree->curmv[ply] = tree->hash_move[ply];
        tree->next_status[ply].excluded_moves[tree->next_status[ply].
            num_excluded++]
            = tree->curmv[ply];
        if (ValidMove(tree, ply, wtm, tree->curmv[ply]))
          return HASH_MOVE;
#if defined(DEBUG)
        else
          Print(128, "bad move from hash table, ply=%d\n", ply);
#endif
      }
/*
 ************************************************************
 *                                                          *
 *  Generate captures and sort them based on the simple     *
 *  MVV/LVA ordering where we try to capture the most       *
 *  valuable victim piece possible, using the least         *
 *  valuable attacking piece possible.  Later we will test  *
 *  to see if the capture appears to lose material and we   *
 *  will defer searching it until later.                    *
 *                                                          *
 ************************************************************
 */
    case GENERATE_CAPTURE_MOVES:
      tree->next_status[ply].phase = CAPTURE_MOVES;
      tree->last[ply] = GenerateCaptures(tree, ply, wtm, tree->last[ply - 1]);
      tree->next_status[ply].remaining = 0;
      for (movep = tree->last[ply - 1], sortv = tree->sort_value;
          movep < tree->last[ply]; movep++, sortv++)
        if (*movep == tree->hash_move[ply]) {
          *sortv = -999999;
          *movep = 0;
          tree->next_status[ply].num_excluded = 0;
        } else {
          *sortv = 128 * pcval[Captured(*movep)] - pcval[Piece(*movep)];
          tree->next_status[ply].remaining++;
        }
/*
 ************************************************************
 *                                                          *
 *  This is a simple insertion sort algorithm.  It seems to *
 *  be no faster than a normal bubble sort, but using this  *
 *  eliminated a lot of explaining about "why?". :)         *
 *                                                          *
 ************************************************************
 */
      if (tree->last[ply] > tree->last[ply - 1] + 1) {
        int temp1, temp2, *tmovep, *tsortv, *end;

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
 *  Try the captures moves, which are in order based on     *
 *  MVV/LVA ordering.  If a larger-valued piece captures a  *
 *  lesser-valued piece, and Swap() says it loses material, *
 *  this capture will be deferred until later.              *
 *                                                          *
 ************************************************************
 */
    case CAPTURE_MOVES:
      while (tree->next_status[ply].remaining) {
        tree->curmv[ply] = *(tree->next_status[ply].last++);
        if (!--tree->next_status[ply].remaining)
          tree->next_status[ply].phase = KILLER_MOVE_1;
        if (pcval[Piece(tree->curmv[ply])] > pcval[Captured(tree->curmv[ply])]
            && Swap(tree, tree->curmv[ply], wtm) < 0)
          continue;
        *(tree->next_status[ply].last - 1) = 0;
        return CAPTURE_MOVES;
      }
/*
 ************************************************************
 *                                                          *
 *  Now, try the killer moves.  This phase tries the two    *
 *  killers for the current ply without generating moves,   *
 *  which saves time if a cutoff occurs.  After those two   *
 *  killers are searched, we try the killers from two plies *
 *  back since they have greater depth and might produce a  *
 *  cutoff if the current two do not.                       *
 *                                                          *
 ************************************************************
 */
    case KILLER_MOVE_1:
      if (!Exclude(tree, ply, tree->killers[ply].move1) &&
          ValidMove(tree, ply, wtm, tree->killers[ply].move1)) {
        tree->curmv[ply] = tree->killers[ply].move1;
        tree->next_status[ply].excluded_moves[tree->next_status[ply].
            num_excluded++]
            = tree->curmv[ply];
        tree->next_status[ply].phase = KILLER_MOVE_2;
        return KILLER_MOVE_1;
      }
    case KILLER_MOVE_2:
      if (!Exclude(tree, ply, tree->killers[ply].move2) &&
          ValidMove(tree, ply, wtm, tree->killers[ply].move2)) {
        tree->curmv[ply] = tree->killers[ply].move2;
        tree->next_status[ply].excluded_moves[tree->next_status[ply].
            num_excluded++]
            = tree->curmv[ply];
        if (ply < 3) {
          tree->next_status[ply].phase = GENERATE_ALL_MOVES;
        } else
          tree->next_status[ply].phase = KILLER_MOVE_3;
        return KILLER_MOVE_2;
      }
    case KILLER_MOVE_3:
      if (!Exclude(tree, ply, tree->killers[ply - 2].move1) &&
          ValidMove(tree, ply, wtm, tree->killers[ply - 2].move1)) {
        tree->curmv[ply] = tree->killers[ply - 2].move1;
        tree->next_status[ply].excluded_moves[tree->next_status[ply].
            num_excluded++]
            = tree->curmv[ply];
        tree->next_status[ply].phase = KILLER_MOVE_4;
        return KILLER_MOVE_3;
      }
    case KILLER_MOVE_4:
      if (!Exclude(tree, ply, tree->killers[ply - 2].move2) &&
          ValidMove(tree, ply, wtm, tree->killers[ply - 2].move2)) {
        tree->curmv[ply] = tree->killers[ply - 2].move2;
        tree->next_status[ply].excluded_moves[tree->next_status[ply].
            num_excluded++]
            = tree->curmv[ply];
        tree->next_status[ply].phase = GENERATE_ALL_MOVES;
        return KILLER_MOVE_4;
      }
/*
 ************************************************************
 *                                                          *
 *  Now, generate all non-capturing moves, which get added  *
 *  to the move list behind any captures we did not search. *
 *                                                          *
 ************************************************************
 */
    case GENERATE_ALL_MOVES:
      tree->last[ply] = GenerateNoncaptures(tree, ply, wtm, tree->last[ply]);
      tree->next_status[ply].phase = REMAINING_MOVES;
      tree->next_status[ply].last = tree->last[ply - 1];
/*
 ************************************************************
 *                                                          *
 *  Then we try the rest of the set of moves, but we use    *
 *  Exclude() function to skip any moves we have already    *
 *  searched (hash or killers).                             *
 *                                                          *
 ************************************************************
 */
    case REMAINING_MOVES:
      for (; tree->next_status[ply].last < tree->last[ply];
          tree->next_status[ply].last++)
        if (*tree->next_status[ply].last) {
          if (!Exclude(tree, ply, *tree->next_status[ply].last)) {
            tree->curmv[ply] = *tree->next_status[ply].last++;
            return REMAINING_MOVES;
          }
        }
      return NONE;
    default:
      Print(4095, "oops!  next_status.phase is bad! [normal %d]\n",
          tree->next_status[ply].phase);
  }
  return NONE;
}

/* last modified 05/15/14 */
/*
 *******************************************************************************
 *                                                                             *
 *   NextRootMove() is used to select the next move from the root move list.   *
 *                                                                             *
 *******************************************************************************
 */
int NextRootMove(TREE * RESTRICT tree, TREE * RESTRICT mytree, int wtm) {
  int which, i;
  uint64_t total_nodes;

/*
 ************************************************************
 *                                                          *
 *  First, we check to see if we only have one legal move.  *
 *  If so, and we are not pondering, we stop after a short  *
 *  search, saving time, but making sure we have something  *
 *  to ponder.                                              *
 *                                                          *
 ************************************************************
 */
  if (!annotate_mode && !pondering && !booking && n_root_moves == 1 &&
      iteration_depth > 4) {
    abort_search = 1;
    return NONE;
  }
/*
 ************************************************************
 *                                                          *
 *  For the moves at the root of the tree, the list has     *
 *  already been generated and sorted.                      *
 *                                                          *
 *  We simply have to find the first move that has a zero   *
 *  "already searched" flag and choose that one.  We do set *
 *  the "already searched" flag for this move before we     *
 *  return so that it won't be searched again in another    *
 *  thread.                                                 *
 *                                                          *
 ************************************************************
 */
  for (which = 0; which < n_root_moves; which++)
    if (!(root_moves[which].status & 8)) {
      if (search_move) {
        if (root_moves[which].move != search_move) {
          root_moves[which].status |= 8;
          continue;
        }
      }
      tree->curmv[1] = root_moves[which].move;
      root_moves[which].status |= 8;
/*
 ************************************************************
 *                                                          *
 *  We have found a move to search.  If appropriate, we     *
 *  display this move, along with the time and information  *
 *  such as which move this is in the list and how many     *
 *  are left to search before this iteration is done, and   *
 *  a "status" character that shows the state of the        *
 *  current search ("?" means we are pondering, waiting on  *
 *  a move to be entered, "*" means we are searching and    *
 *  our clock is running).  We also display the NPS for     *
 *  the search, simply for information about how fast the   *
 *  machine is running.                                     *
 *                                                          *
 ************************************************************
 */
      if (tree->nodes_searched > noise_level && display_options & 32) {
        Lock(lock_io);
        sprintf(mytree->remaining_moves_text, "%d/%d", which + 1,
            n_root_moves);
        end_time = ReadClock();
        if (pondering)
          printf("         %2i   %s%7s?  ", iteration_depth,
              Display2Times(end_time - start_time),
              mytree->remaining_moves_text);
        else
          printf("         %2i   %s%7s*  ", iteration_depth,
              Display2Times(end_time - start_time),
              mytree->remaining_moves_text);
        if (display_options & 32 && display_options & 64)
          printf("%d. ", move_number);
        if (display_options & 32 && display_options & 64 && Flip(wtm))
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
        printf("(%snps)             \r", DisplayKMB(nodes_per_second));
        fflush(stdout);
        Unlock(lock_io);
      }
/*
 ************************************************************
 *                                                          *
 *  Bit of a tricky exit.  If the move is not flagged as    *
 *  "OK to search in parallel or reduce" then we return     *
 *  "HASH_MOVE" which will prevent Search() from reducing   *
 *  the move (LMR).  Otherwise we return the more common    *
 *  "REMAINING_MOVES" value which allows LMR to be used on  *
 *  those root moves.                                       *
 *                                                          *
 ************************************************************
 */
      if ((root_moves[which].status & 4) == 0)
        return HASH_MOVE;
      else
        return REMAINING_MOVES;
    }
  return NONE;
}

/* last modified 05/15/14 */
/*
 *******************************************************************************
 *                                                                             *
 *   NextRootMoveParallel() is used to determine if the next root move can be  *
 *   searched in parallel.  If it appears to Iterate() that one of the moves   *
 *   following the first move might become the best move, the 'no parallel'    *
 *   flag is set to speed up finding the new best move.  This flag is set if   *
 *   this root move has an "age" value > 0 which indicates this move was the   *
 *   "best move" within the previous 3 search depths.  We want to search such  *
 *   moves as quickly as possible, prior to starting a parallel search at the  *
 *   root, in case this move once again becomes the best move and provides a   *
 *   better alpha bound.                                                       *
 *                                                                             *
 *******************************************************************************
 */
int NextRootMoveParallel(void) {
  int which;

/*
 ************************************************************
 *                                                          *
 *  Here we simply check the root_move status flag that is  *
 *  set in Iterate() after each iteration is completed.  A  *
 *  value of "1" indicates this move has to be searched by  *
 *  all processors, splitting must wait until after all     *
 *  such moves have been searched individually.             *
 *                                                          *
 ************************************************************
 */
  for (which = 0; which < n_root_moves; which++)
    if (!(root_moves[which].status & 8))
      break;
  if (which < n_root_moves && root_moves[which].status & 4)
    return 1;
  return 0;
}

/* last modified 05/15/14 */
/*
 *******************************************************************************
 *                                                                             *
 *   Exclude() searches the list of moves searched prior to generating a move  *
 *   list to exclude those that were searched via a hash table best move or    *
 *   through the killer moves for the current ply and two plies back.          *
 *                                                                             *
 *   The variable next_status[].num_excluded is the total number of non-       *
 *   generated moves we searched.  next_status[].remaining is initially set to *
 *   num_excluded, but each time an excluded move is found, the counter is     *
 *   decremented.  Once all excluded moves have been found, we avoid running   *
 *   through the list of excluded moves on each call and simply return.        *
 *                                                                             *
 *******************************************************************************
 */
int Exclude(TREE * RESTRICT tree, int ply, int move) {
  int i;

  if (tree->next_status[ply].num_excluded)
    for (i = 0; i < tree->next_status[ply].num_excluded; i++)
      if (move == tree->next_status[ply].excluded_moves[i]) {
        tree->next_status[ply].remaining--;
        return 1;
      }
  return 0;
}
