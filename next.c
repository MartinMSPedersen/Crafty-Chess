#include "chess.h"
#include "data.h"
/* last modified 08/20/10 */
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
 *   First try the transposition table move (which might be *
 *   the principal variation move as we first move down the *
 *   tree).  If it is good enough to cause a cutoff, we     *
 *   avoided the overhead of generating legal moves.        *
 *                                                          *
 ************************************************************
 */
    case HASH_MOVE:
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
 *   Now generate all legal moves by using the special      *
 *   GenerateCheckEvasions() procedure.  Then sort the      *
 *   moves based on the expected gain or loss.  this is     *
 *   deferred until now to see if the hash move is good     *
 *   enough to produce a cutoff and avoid this effort.      *
 *                                                          *
 *   Once we confirm that the move does not lose any        *
 *   material, we sort these non-losing moves into MVV/LVA  *
 *   order which appears to be a slightly faster move       *
 *   ordering idea.  Unsafe evasion moves are sorted using  *
 *   the original Swap() score to keep them last in the     *
 *   move list.  Note that this move list contains both     *
 *   captures and non-captures.  We try the safe captures   *
 *   first due to the way the sort score is computed.       *
 *                                                          *
 ************************************************************
 */
    case SORT_ALL_MOVES:
      tree->last[ply] =
          GenerateCheckEvasions(tree, ply, wtm, tree->last[ply - 1]);
      tree->next_status[ply].phase = REMAINING_MOVES;
      for (movep = tree->last[ply - 1], sortv = tree->sort_value;
          movep < tree->last[ply]; movep++, sortv++)
        if (tree->hash_move[ply] && *movep == tree->hash_move[ply]) {
          *sortv = -999999;
          *movep = 0;
        } else {
          if (pc_values[Piece(*movep)] <= pc_values[Captured(*movep)])
            *sortv =
                128 * pc_values[Captured(*movep)] - pc_values[Piece(*movep)];
          else {
            *sortv = Swap(tree, *movep, wtm);
            if (*sortv >= 0)
              *sortv =
                  128 * pc_values[Captured(*movep)] -
                  pc_values[Piece(*movep)];
          }
        }
/*
 ************************************************************
 *                                                          *
 *   This is a simple insertion sort algorithm.  It seems   *
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
 *   Now try the moves in sorted order.                     *
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
  }
  return (NONE);
}

/* last modified 07/24/09 */
/*
 *******************************************************************************
 *                                                                             *
 *   NextMove() is used to select the next move from the current move list.    *
 *                                                                             *
 *******************************************************************************
 */
int NextMove(TREE * RESTRICT tree, int ply, int wtm) {
  int *movep, *sortv;

  switch (tree->next_status[ply].phase) {
/*
 ************************************************************
 *                                                          *
 *   First, try the transposition table move (which will be *
 *   the principal variation move as we first move down the *
 *   tree).                                                 *
 *                                                          *
 ************************************************************
 */
    case HASH_MOVE:
      tree->next_status[ply].phase = GENERATE_CAPTURE_MOVES;
      if (tree->hash_move[ply]) {
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
 *   Generate captures and sort them based on the simple    *
 *   MVV/LVA ordering where we try to capture the most      *
 *   valuable victim piece possible, using the least        *
 *   valuable attacking piece possible.  Later we will test *
 *   to see if the capture appears to lose material and we  *
 *   will defer searching it until later.                   *
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
        } else {
          *sortv =
              128 * pc_values[Captured(*movep)] - pc_values[Piece(*movep)];
          tree->next_status[ply].remaining++;
        }
/*
 ************************************************************
 *                                                          *
 *   This is a simple insertion sort algorithm.  It seems   *
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
 *   Try the captures moves, which are in order based on    *
 *   the expected gain of material.  Captures that lose     *
 *   material have been excluded from this phase.           *
 *                                                          *
 ************************************************************
 */
    case CAPTURE_MOVES:
      while (tree->next_status[ply].remaining) {
        tree->curmv[ply] = *(tree->next_status[ply].last++);
        tree->next_status[ply].remaining--;
        if (!tree->next_status[ply].remaining)
          tree->next_status[ply].phase = KILLER_MOVE_1;
        if (pc_values[Piece(tree->curmv[ply])] >
            pc_values[Captured(tree->curmv[ply])] &&
            Swap(tree, tree->curmv[ply], wtm) < 0)
          continue;
        *(tree->next_status[ply].last - 1) = 0;
        return (CAPTURE_MOVES);
      }
      tree->next_status[ply].phase = KILLER_MOVE_1;
/*
 ************************************************************
 *                                                          *
 *   Now, try the killer moves.  This phase tries the two   *
 *   killers for the current ply without generating moves,  *
 *   which saves time if a cutoff occurs.                   *
 *                                                          *
 ************************************************************
 */
    case KILLER_MOVE_1:
      if ((tree->hash_move[ply] != tree->killers[ply].move1) &&
          ValidMove(tree, ply, wtm, tree->killers[ply].move1)) {
        tree->curmv[ply] = tree->killers[ply].move1;
        tree->next_status[ply].phase = KILLER_MOVE_2;
        return (KILLER_MOVE_1);
      }
    case KILLER_MOVE_2:
      if ((tree->hash_move[ply] != tree->killers[ply].move2) &&
          ValidMove(tree, ply, wtm, tree->killers[ply].move2)) {
        tree->curmv[ply] = tree->killers[ply].move2;
        tree->next_status[ply].phase = GENERATE_ALL_MOVES;
        return (KILLER_MOVE_2);
      }
      tree->next_status[ply].phase = GENERATE_ALL_MOVES;
/*
 ************************************************************
 *                                                          *
 *   Now, generate all non-capturing moves.                 *
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
 *   Then we try the rest of the set of moves.              *
 *                                                          *
 ************************************************************
 */
    case REMAINING_MOVES:
      for (; tree->next_status[ply].last < tree->last[ply];
          tree->next_status[ply].last++)
        if (*tree->next_status[ply].last &&
            *tree->next_status[ply].last != tree->hash_move[ply] &&
            *tree->next_status[ply].last != tree->killers[ply].move1 &&
            *tree->next_status[ply].last != tree->killers[ply].move2) {
          tree->curmv[ply] = *tree->next_status[ply].last;
          *tree->next_status[ply].last++ = 0;
          return (REMAINING_MOVES);
        }
      return (NONE);
    default:
      Print(4095, "oops!  next_status.phase is bad! [normal %d]\n",
          tree->next_status[ply].phase);
  }
  return (NONE);
}

/* last modified 08/24/10 */
/*
 *******************************************************************************
 *                                                                             *
 *   NextRootMove() is used to select the next move from the root move list.   *
 *                                                                             *
 *******************************************************************************
 */
int NextRootMove(TREE * RESTRICT tree, TREE * RESTRICT mytree, int wtm) {
  int done, which, i;
  BITBOARD total_nodes;

/*
 ************************************************************
 *                                                          *
 *   First, we check to see if we are out of time.  We try  *
 *   to complete any "current" root moves being searched,   *
 *   prior to ending the search, so it is possible that     *
 *   time has already expired, but we let the search finish *
 *   current root moves that are being searched (there may  *
 *   be more than one, thanks to the parallel search) so    *
 *   that we don't abort just before a new best move might  *
 *   be discovered.                                         *
 *                                                          *
 ************************************************************
 */
  abort_after_ply1 += TimeCheck(tree, 1);
  if (abort_after_ply1)
    return (NONE);
  if (!annotate_mode && !pondering && !booking && n_root_moves == 1 &&
      iteration_depth > 4) {
    abort_search = 1;
    return (NONE);
  }
/*
 ************************************************************
 *                                                          *
 *   For the moves at the root of the tree, the list has    *
 *   already been generated and sorted.  On entry, test     *
 *   the searched_this_root_move[] array to determine the   *
 *   first move in the list that has not yet been searched. *
 *   We select that move and search it next.                *
 *                                                          *
 ************************************************************
 */
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
/*
 ************************************************************
 *                                                          *
 *   We have found a move to search.  If appropriate, we    *
 *   display this move, along with the time and information *
 *   such as which move this is in the list and how many    *
 *   are left to search before this iteration is done, and  *
 *   a "status" character that shows the state of the       *
 *   current search ("?" means we are pondering, waiting on *
 *   a move to be entered, "*" means we are searching and   *
 *   our clock is running).  We also display the NPS for    *
 *   the search, simply for information about how fast the  *
 *   machine is running.                                    *
 *                                                          *
 ************************************************************
 */
      if ((tree->nodes_searched > noise_level) && (display_options & 32)) {
        Lock(lock_io);
        sprintf(mytree->remaining_moves_text, "%d/%d", which + 1,
            n_root_moves);
        end_time = ReadClock();
        if (pondering)
          printf("               %2i   %s%7s?  ", iteration_depth,
              DisplayTime(end_time - start_time),
              mytree->remaining_moves_text);
        else
          printf("               %2i   %s%7s*  ", iteration_depth,
              DisplayTime(end_time - start_time),
              mytree->remaining_moves_text);
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
/*
 ************************************************************
 *                                                          *
 *   Bit of a tricky exit.  If the move is flagged as "do   *
 *   not reduce" or "do not search in parallel" then we     *
 *   return "HASH_MOVE" which will prevent Search() from    *
 *   reducing the move (LMR).  Otherwise we return the more *
 *   common "REMAINING_MOVES" value which allows LMR to be  *
 *   used on those root moves.                              *
 *                                                          *
 ************************************************************
 */
      if (root_moves[which].status & 0xc0)
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
 *   searched in parallel.  If it appears to Iterate() that one of the moves   *
 *   following the first move might become the best move, the 'no parallel'    *
 *   flag is set to speed up finding the new best move.  This flag is set if   *
 *   any root move has an exceptionally large node count when compared to      *
 *   the other moves at the root.  Such moves might just lead to complex and   *
 *   tactical positions with a large tree, or they might be about to rise to   *
 *   the top and become the best move.  We want to search these moves one at   *
 *   time using all processors, so that we can find the best move as quickly   *
 *   as possible.                                                              *
 *                                                                             *
 *   We only allow this for at most 1/3 of the root moves before we start to   *
 *   split at the root and search in parallel, because this is a much more     *
 *   efficient way to search with no overhead whatsoever.                      *
 *                                                                             *
 *******************************************************************************
 */
int NextRootMoveParallel(void) {
  int which;

/*
 ************************************************************
 *                                                          *
 *   First, find out how far down the list we have searched *
 *   already.  if the next move is flagged as "do not       *
 *   search in parallel" then return 1 unless the score has *
 *   dropped significantly.  If the score has dropped, then *
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
