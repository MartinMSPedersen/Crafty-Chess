#include <math.h>
#include "chess.h"
#include "data.h"
#include "epdglue.h"
#if defined(UNIX) || defined(AMIGA)
#  include <unistd.h>
#  include <sys/types.h>
#endif
/* last modified 11/11/13 */
/*
 *******************************************************************************
 *                                                                             *
 *   Iterate() is the routine used to drive the iterated search.  It           *
 *   repeatedly calls search, incrementing the search depth after each call,   *
 *   until either time is exhausted or the maximum set search depth is         *
 *   reached.                                                                  *
 *                                                                             *
 *******************************************************************************
 */
int Iterate(int wtm, int search_type, int root_list_done) {
  TREE *const tree = block[0];
  PATH savepv;
  ROOT_MOVE temp_rm;
  int savevalue = 0, print_ok = 0, bm_changes;
  int i, fail_delta, old_root_alpha, old_root_beta;
  int value = 0, twtm, correct, correct_count;
  char *fl_indicator, *fh_indicator;

#if (CPUS > 1)
  pthread_t pt;
#endif
/*
 ************************************************************
 *                                                          *
 *  Produce the root move list, which is ordered and kept   *
 *  for the duration of this search (the order may change   *
 *  as new best moves are backed up to the root of course.) *
 *                                                          *
 ************************************************************
 */
  tree->curmv[0] = 0;
  if (wtm) {
    draw_score[0] = -abs_draw_score;
    draw_score[1] = abs_draw_score;
  } else {
    draw_score[0] = abs_draw_score;
    draw_score[1] = -abs_draw_score;
  }
#if defined(NODES)
  temp_search_nodes = search_nodes;
#endif
  abort_search = 0;
  book_move = 0;
  program_start_time = ReadClock();
  start_time = ReadClock();
  elapsed_start = ReadClock();
  root_wtm = wtm;
  kibitz_depth = 0;
  tree->nodes_searched = 0;
  tree->fail_highs = 0;
  tree->fail_high_number = 0;
  parallel_splits = 0;
  parallel_aborts = 0;
  max_split_blocks = 0;
  if (booking || !Book(tree, wtm, root_list_done))
    do {
      if (abort_search)
        break;
      if (!root_list_done)
        RootMoveList(wtm);
#if !defined(NOEGTB)
      if (EGTB_draw && !puzzling && swindle_mode)
        EGTB_use = 0;
      else
        EGTB_use = EGTBlimit;
      if (EGTBlimit && !EGTB_use)
        Print(128, "Drawn at root, trying for swindle.\n");
#endif
      correct_count = 0;
      burp = 15 * 100;
      transposition_age = (transposition_age + 1) & 0x1ff;
      next_time_check = nodes_between_time_checks;
      tree->evaluations = 0;
      tree->egtb_probes = 0;
      tree->egtb_probes_successful = 0;
      tree->extensions_done = 0;
      tree->qchecks_done = 0;
      tree->reductions_done = 0;
      tree->moves_pruned = 0;
      root_wtm = wtm;
/*
 ************************************************************
 *                                                          *
 *   If there are no legal moves, it is either mate or draw *
 *   depending on whether the side to move is in check or   *
 *   not (mate or stalemate.)                               *
 *                                                          *
 ************************************************************
 */
      if (n_root_moves == 0) {
        program_end_time = ReadClock();
        tree->pv[0].pathl = 0;
        tree->pv[0].pathd = 0;
        if (Check(wtm))
          root_value = -(MATE - 1);
        else
          root_value = DrawScore(wtm);
        Print(6, "        depth     time       score   variation\n");
        Print(6, "                                     (no moves)\n");
        tree->nodes_searched = 1;
        if (!puzzling)
          last_root_value = root_value;
        return (root_value);
      }
/*
 ************************************************************
 *                                                          *
 *   Now set the search time and iteratively call Search()  *
 *   to analyze the position deeper and deeper.  Note that  *
 *   Search() is called with an alpha/beta window roughly   *
 *   1/3 of a pawn on either side of the score last         *
 *   returned by search.  Also, after the first root move   *
 *   is searched, this window is collapsed to n and n+1     *
 *   (where n is the value for the first root move.)  Often *
 *   a better move is found, which causes search to return  *
 *   <beta> as the score.  We then relax beta depending on  *
 *   its value:  if beta = alpha+1, set beta to alpha+1/3   *
 *   of a pawn;  if beta = alpha+1/3 pawn, then set beta to *
 *   + infinity.                                            *
 *                                                          *
 ************************************************************
 */
      TimeSet(tree, search_type);
      iteration_depth = 1;
      if (last_pv.pathd > 1)
        iteration_depth = last_pv.pathd + 1;
      else
        difficulty = 100;
      Print(6, "        depth     time       score   variation (%d)\n",
          iteration_depth);
      abort_search = 0;
/*
 ************************************************************
 *                                                          *
 *   Set the initial search bounds based on the last search *
 *   or default values.                                     *
 *                                                          *
 ************************************************************
 */
      tree->pv[0] = last_pv;
      if (iteration_depth > 1) {
        root_alpha = last_value - 16;
        root_value = last_value - 16;
        root_beta = last_value + 16;
      } else {
        root_alpha = -MATE - 1;
        root_value = -MATE - 1;
        root_beta = MATE + 1;
      }
/*
 ************************************************************
 *                                                          *
 *   If we are using multiple threads, and they have not    *
 *   been started yet, then start them now as the search    *
 *   is ready to begin.                                     *
 *                                                          *
 ************************************************************
 */
#if (CPUS > 1)
      if (smp_max_threads > smp_idle + 1) {
        long proc;

        initialized_threads = 1;
        Print(128, "starting thread");
        for (proc = smp_threads + 1; proc < smp_max_threads; proc++) {
          Print(128, " %d", proc);
          thread[proc] = 0;
#  if defined(_WIN32) || defined(_WIN64)
          NumaStartThread(ThreadInit, (void *) proc);
#  else
          pthread_create(&pt, &attributes, ThreadInit, (void *) proc);
#  endif
          Lock(lock_smp);
          smp_threads++;
          Unlock(lock_smp);
        }
        Print(128, " <done>\n");
      }
      WaitForAllThreadsInitialized();
#endif
      root_print_ok = 0;
      if (search_nodes)
        nodes_between_time_checks = search_nodes;
      for (; iteration_depth <= MAXPLY - 5; iteration_depth++) {
/*
 ************************************************************
 *                                                          *
 *   Now install the old PV into the hash table so that     *
 *   these moves will be followed first.                    *
 *                                                          *
 ************************************************************
 */
        fail_delta = 16;
        twtm = wtm;
        for (i = 1; i < (int) tree->pv[0].pathl; i++) {
          if (!VerifyMove(tree, i, twtm, tree->pv[0].path[i])) {
            Print(4095, "ERROR, not installing bogus pv info at ply=%d\n", i);
            Print(4095, "not installing from=%d  to=%d  piece=%d\n",
                From(tree->pv[0].path[i]), To(tree->pv[0].path[i]),
                Piece(tree->pv[0].path[i]));
            Print(4095, "pathlen=%d\n", tree->pv[0].pathl);
            break;
          }
          HashStorePV(tree, twtm, i);
          MakeMove(tree, i, tree->pv[0].path[i], twtm);
          twtm = Flip(twtm);
        }
        for (i--; i > 0; i--) {
          twtm = Flip(twtm);
          UnmakeMove(tree, i, tree->pv[0].path[i], twtm);
        }
/*
 ************************************************************
 *                                                          *
 *   Now we call Search() and start the next search         *
 *   iteration.  We already have solid alpha/beta bounds    *
 *   set up for the aspiration search.  When each iteration *
 *   completes, these aspiration values are recomputed and  *
 *   used for the next iteration.                           *
 *                                                          *
 ************************************************************
 */
        if (trace_level) {
          printf("==================================\n");
          printf("=      search iteration %2d       =\n", iteration_depth);
          printf("==================================\n");
        }
        if (tree->nodes_searched) {
          nodes_between_time_checks = nodes_per_second / 10;
          if (!analyze_mode) {
            if (time_limit > 300);
            else if (time_limit > 50)
              nodes_between_time_checks /= 10;
            else
              nodes_between_time_checks /= 100;
          } else
            nodes_between_time_checks = Min(nodes_per_second, 100000);
#if defined(SKILL)
          if (skill > 50)
#endif
            nodes_between_time_checks = Max(nodes_between_time_checks, 10000);
        }
        if (search_nodes)
          nodes_between_time_checks = search_nodes - tree->nodes_searched;
        nodes_between_time_checks =
            Min(nodes_between_time_checks, MAX_TC_NODES);
        next_time_check = nodes_between_time_checks;
        while (1) {
          old_root_alpha = root_alpha;
          old_root_beta = root_beta;
          thread[0] = block[0];
          tree->inchk[1] = Check(wtm);
          value =
              Search(tree, root_alpha, root_beta, wtm, iteration_depth, 1, 0);
          end_time = ReadClock();
          root_print_ok = tree->nodes_searched > noise_level;
          if (abort_search)
            break;
/*
 ************************************************************
 *                                                          *
 *   Check for the case where we get a score back that is   *
 *   greater than or equal to beta.  This is called a fail  *
 *   high condition and requires a re-search with a better  *
 *   (more optimistic) beta value so that we can discover   *
 *   just how good this move really is.                     *
 *                                                          *
 ************************************************************
 */
          if (value >= root_beta) {
            root_moves[0].status |= 2;
            root_value = root_alpha;
            root_beta =
                SetRootBeta(root_moves[0].status, root_beta, &fail_delta);
            root_moves[0].status &= 255 - 16;
            if (root_print_ok) {
              if (wtm)
                fh_indicator = "++";
              else
                fh_indicator = "--";
              Print(2, "         %2i   %s     %2s   ", iteration_depth,
                  Display2Times(end_time - start_time), fh_indicator);
              if (display_options & 64)
                Print(2, "%d. ", move_number);
              if ((display_options & 64) && !wtm)
                Print(2, "... ");
              Print(2, "%s! ", OutputMove(tree, tree->pv[1].path[1], 1, wtm));
              Print(2, "(%c%s)                  \n", (wtm) ? '>' : '<',
                  DisplayEvaluationKibitz(old_root_beta, wtm));
              kibitz_text[0] = 0;
              if (display_options & 64)
                sprintf(kibitz_text, " %d.", move_number);
              if ((display_options & 64) && !wtm)
                sprintf(kibitz_text + strlen(kibitz_text), " ...");
              sprintf(kibitz_text + strlen(kibitz_text), " %s!",
                  OutputMove(tree, tree->pv[1].path[1], 1, wtm));
              Kibitz(6, wtm, iteration_depth, end_time - start_time, value,
                  tree->nodes_searched, tree->egtb_probes_successful,
                  kibitz_text);
            }
/*
 ************************************************************
 *                                                          *
 *   Check for the case where we get a score back that is   *
 *   less than or equal to alpha.  This is called a fail    *
 *   low condition and requires a re-search with a better   *
 *   more pessimistic)) alpha value so that we can discover *
 *   just how bad this move really is.                      *
 *                                                          *
 ************************************************************
 */
          } else if (value <= root_alpha) {
            difficulty = Max(difficulty, 100);
            if (!(root_moves[0].status & 2)) {
              root_moves[0].status |= 1;
              root_alpha =
                  SetRootAlpha(root_moves[0].status, root_alpha, &fail_delta);
              root_value = root_alpha;
              root_moves[0].status &= 255 - 16;
              if (root_print_ok && !abort_search) {
                if (wtm)
                  fl_indicator = "--";
                else
                  fl_indicator = "++";
                Print(4, "         %2i   %s     %2s   ", iteration_depth,
                    Display2Times(ReadClock() - start_time), fl_indicator);
                if (display_options & 64)
                  Print(4, "%d. ", move_number);
                if ((display_options & 64) && !wtm)
                  Print(4, "... ");
                Print(4, "%s? ", OutputMove(tree, root_moves[0].move, 1,
                        wtm));
                Print(4, "(%c%s)                  \n",
                    (Flip(wtm)) ? '>' : '<',
                    DisplayEvaluationKibitz(old_root_alpha, wtm));
              }
            } else
              break;
          } else
            break;
        }
        if (root_value > root_alpha && root_value < root_beta)
          last_root_value = root_value;
/*
 ************************************************************
 *                                                          *
 *   If we are running a test suite, check to see if we can *
 *   exit the search.  This happens when N successive       *
 *   iterations produce the correct solution.  N is set by  *
 *   the test command in Option().                          *
 *                                                          *
 ************************************************************
 */
        correct = solution_type;
        for (i = 0; i < number_of_solutions; i++) {
          if (!solution_type) {
            if (solutions[i] == tree->pv[0].path[1])
              correct = 1;
          } else if (solutions[i] == tree->pv[0].path[1])
            correct = 0;
        }
        if (correct)
          correct_count++;
        else
          correct_count = 0;
/*
 ************************************************************
 *                                                          *
 *   If the search terminated normally, then dump the PV    *
 *   and search statistics (we don't do this if the search  *
 *   aborts because the opponent doesn't make the predicted *
 *   move...)                                               *
 *                                                          *
 ************************************************************
 */
        twtm = wtm;
/*
 ************************************************************
 *                                                          *
 *   A quick kludge to make certain the PV move comes first *
 *   in the move list.  On rare occasions, multiple moves   *
 *   can fail high on the null-window search at the root,   *
 *   and some of them will then fail low on the re-search.  *
 *   The last move to fail high ends up on the top of the   *
 *   move list, while a different move shows up as best by  *
 *   not failing low on the re-search.  This makes certain  *
 *   that the PV move from the previous iteration is always *
 *   searched first.                                        *
 *                                                          *
 ************************************************************
 */
        if (tree->pv[0].path[1] != root_moves[0].move) {
          for (i = 0; i < n_root_moves; i++)
            if (tree->pv[0].path[1] == root_moves[i].move)
              break;
          if (i < n_root_moves) {
            temp_rm = root_moves[i];
            for (; i > 0; i--)
              root_moves[i] = root_moves[i - 1];
            root_moves[0] = temp_rm;
          }
        }
/*
 ************************************************************
 *                                                          *
 *   Notice that we don't search moves that were best over  *
 *   the last 3 iterations in parallel, nor do we reduce    *
 *   those since they are potential best moves again.       *
 *                                                          *
 ************************************************************
 */
        for (i = 0; i < n_root_moves; i++)
          root_moves[i].status = 0;
        root_moves[0].status |= 4 + 8;
        for (i = 0; i < n_root_moves; i++) {
          if (root_moves[i].bm_age)
            root_moves[i].bm_age--;
          if (root_moves[i].bm_age)
            root_moves[i].status |= 4 + 8;
        }
/*
 ************************************************************
 *                                                          *
 *   If requested, print the ply=1 move list along with the *
 *   flags for each move.                                   *
 *                                                          *
 ************************************************************
 */
        if (display_options & 256) {
          uint64_t total_nodes = 0;

          Print(128, "       move  age  R/P\n");
          for (i = 0; i < n_root_moves; i++) {
            Print(128, " %10s   %d   %d %d\n", OutputMove(tree,
                    root_moves[i].move, 1, wtm), root_moves[i].bm_age,
                (root_moves[i].status & 8) == 0,
                (root_moves[i].status & 4) == 0);
          }
          Print(256, "      total  " BMF10 "\n", total_nodes);
        }
/*
 ************************************************************
 *                                                          *
 *   now adjust the "difficulty" value.  The idea here is   *
 *   that a position is easier if the best move does not    *
 *   change during an iteration, and harder if it does.     *
 *   We use this to adjust the target time during the       *
 *   search to spend more or less time when appropriate.    *
 *                                                          *
 *   if the PV did not change, we lower the difficulty      *
 *   setting, but we weigh the past difficulty more, so     *
 *   it takes several iterations with no change to reach    *
 *   the max easy setting.  Ditto for cases where there     *
 *   were changes, except that the more changes in the last *
 *   iteration, the more difficult we consider this search. *
 *                                                          *
 ************************************************************
 */
        bm_changes = 0;
        for (i = 0; i < n_root_moves; i++)
          if (root_moves[i].bm_age == 3)
            bm_changes++;
        if (bm_changes <= 1)
          difficulty = 90 * difficulty / 100;
        else {
          if (difficulty < 100)
            difficulty = 100 + 20 * bm_changes;
          else
            difficulty = difficulty + 20 * bm_changes;
        }
        difficulty = Max(60, Min(difficulty, 200));
        if (end_time - start_time > 10)
          nodes_per_second =
              tree->nodes_searched * 100 / (uint64_t) (end_time - start_time);
        else
          nodes_per_second = 1000000;
        if (!abort_search && value != -(MATE - 1)) {
          if (root_print_ok) {
            DisplayPV(tree, 5, wtm, end_time - start_time, value,
                &tree->pv[0]);
          } else {
            savevalue = value;
            savepv = tree->pv[0];
            print_ok = 1;
          }
        }
        root_alpha = value - 16;
        root_value = root_alpha;
        root_beta = value + 16;
        if (iteration_depth > 3 && value > MATE - 300 &&
            value >= (MATE - iteration_depth - 1)
            && value > last_mate_score)
          break;
        if ((iteration_depth >= search_depth) && search_depth)
          break;
        if (abort_search)
          break;
        end_time = ReadClock() - start_time;
        if (TimeCheck(tree, 1))
          break;
        if (correct_count >= early_exit)
          break;
#if !defined(NOEGTB)
        if (iteration_depth > 10 && TotalAllPieces <= EGTBlimit && EGTB_use &&
            !EGTB_search && EGTBProbe(tree, 1, wtm, &i))
          break;
#endif
        if (search_nodes && tree->nodes_searched >= search_nodes)
          break;
      }
/*
 ************************************************************
 *                                                          *
 *   Search done, now display statistics, depending on the  *
 *   verbosity level set.                                   *
 *                                                          *
 ************************************************************
 */
      end_time = ReadClock();
      elapsed_end = ReadClock() - elapsed_start;
      if (elapsed_end > 10)
        nodes_per_second =
            (uint64_t) tree->nodes_searched * 100 / (uint64_t) elapsed_end;
      if (abort_search != 2 && !puzzling) {
        if (!root_print_ok && print_ok) {
          root_print_ok = 1;
          DisplayPV(tree, 5, wtm, end_time - start_time, savevalue, &savepv);
        }
        tree->evaluations = (tree->evaluations) ? tree->evaluations : 1;
        tree->fail_highs++;
        tree->fail_high_number++;
        Print(8, "        time=%s", DisplayTimeKibitz(end_time - start_time));
        Print(8, "  n=" BMF, tree->nodes_searched);
        Print(8, "  afhm=%.2f",
            (float) tree->fail_high_number / (float) tree->fail_highs);
        Print(8, "  predicted=%d", predicted);
        Print(8, "  50move=%d", Rule50Moves(0));
        Print(8, "  nps=%s\n", DisplayKM(nodes_per_second));
        Print(16, "        extended=%s ", DisplayKM(tree->extensions_done));
        Print(16, "qchks=%s", DisplayKM(tree->qchecks_done));
        Print(16, "  reduced=%s", DisplayKM(tree->reductions_done));
        Print(16, "  pruned=%s", DisplayKM(tree->moves_pruned));
        Print(16, "  evals=%s\n", DisplayKM(tree->evaluations));
        Print(16, "        EGTBprobes=%s  hits=%s",
            DisplayKM(tree->egtb_probes),
            DisplayKM(tree->egtb_probes_successful));
        Print(16, "  splits=%d  aborts=%d  data=%d/%d\n", parallel_splits,
            parallel_aborts, max_split_blocks, MAX_BLOCKS);
      }
    } while (0);
  else {
    last_root_value = 0;
    root_value = 0;
    book_move = 1;
    tree->pv[0] = tree->pv[1];
    if (analyze_mode)
      Kibitz(4, wtm, 0, 0, 0, 0, 0, kibitz_text);
  }
  if (smp_nice && ponder == 0 && smp_threads) {
    int proc;

    Print(128, "terminating SMP processes.\n");
    for (proc = 1; proc < CPUS; proc++)
      thread[proc] = (TREE *) - 1;
    while (smp_threads);
    smp_idle = 0;
  }
  program_end_time = ReadClock();
  search_move = 0;
  if (quit)
    CraftyExit(0);
  return (last_root_value);
}

/*
 *******************************************************************************
 *                                                                             *
 *   SetRootAlpha() is used to set the root alpha value by looking at the move *
 *   status to see how many times this move has failed low.  The first fail    *
 *   drops alpha by -1.0.  The second fail low drops it by another -2.0, and   *
 *   the third fail low drops it to -infinity.                                 *
 *                                                                             *
 *******************************************************************************
 */
int SetRootAlpha(unsigned char status, int old_root_alpha, int *fail_delta) {
  int new_root_value;

  new_root_value = old_root_alpha - *fail_delta;
  *fail_delta *= 2;
  if (*fail_delta > 10 * PAWN_VALUE)
    *fail_delta = 99999;

  return (Max(new_root_value, -MATE));
}

/*
 *******************************************************************************
 *                                                                             *
 *   SetRootBeta() is used to set the root beta value by looking at the move   *
 *   status to see how many times this move has failed high.  The first fail   *
 *   raises alpha by 1.0.  The second fail raises it by another 2.0, and       *
 *   the third fail raises it to +infinity.                                    *
 *                                                                             *
 *******************************************************************************
 */
int SetRootBeta(unsigned char status, int old_root_beta, int *fail_delta) {
  int new_root_value;

  new_root_value = old_root_beta + *fail_delta;
  *fail_delta *= 2;
  if (*fail_delta > 10 * PAWN_VALUE)
    *fail_delta = 99999;
  return (Min(new_root_value, MATE));
}
