#include "chess.h"
#include "data.h"
#include "epdglue.h"
#if defined(UNIX) || defined(AMIGA)
#  include <unistd.h>
#  include <sys/types.h>
#endif
/* last modified 06/07/09 */
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
  ROOT_MOVE temp;
  int i;
  int value = 0, twtm;
  int correct, correct_count, material = 0, sorted;
  char *fh_indicator, *fl_indicator;
  TREE *const tree = block[0];
  int savevalue = 0;
  PATH savepv;
  int print_ok = 0;

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
  abort_after_ply1 = 0;
  abort_search = 0;
  book_move = 0;
  program_start_time = ReadClock();
  start_time = ReadClock();
  elapsed_start = ReadClock();
  root_wtm = wtm;
  kibitz_depth = 0;
  tree->nodes_searched = 0;
  tree->fail_high = 0;
  tree->fail_high_first = 0;
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
        Print(6, "              depth   time  score   variation\n");
        Print(6, "                                    (no moves)\n");
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
      Print(6, "              depth   time  score   variation (%d)\n",
          iteration_depth);
      abort_after_ply1 = 0;
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
        root_alpha = last_value - 40;
        root_value = last_value - 40;
        root_beta = last_value + 40;
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
          HashStorePV(tree, twtm, tree->pv[0].path[i]);
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
 *   Now we call SearchRoot() and start the next search     *
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
          nodes_between_time_checks = Max(nodes_between_time_checks, 10000);
        }
        if (search_nodes)
          nodes_between_time_checks = search_nodes - tree->nodes_searched;
        nodes_between_time_checks =
            Min(nodes_between_time_checks, MAX_TC_NODES);
        while (1) {
          thread[0] = block[0];
          for (i = 0; i < n_root_moves; i++)
            if (!(root_moves[i].status & 256))
              break;
          root_moves[i].status &= 4095 - 128;
          tree->inchk[1] = Check(wtm);
          value =
              Search(tree, root_alpha, root_beta, wtm, iteration_depth, 1, 0);
          root_print_ok = tree->nodes_searched > noise_level;
          if (abort_search || abort_after_ply1)
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
            if (!(root_moves[0].status & 8))
              root_moves[0].status |= 8;
            else if (!(root_moves[0].status & 16))
              root_moves[0].status |= 16;
            else
              root_moves[0].status |= 32;
            root_alpha = SetRootAlpha(root_moves[0].status, root_alpha);
            root_value = root_alpha;
            root_beta = SetRootBeta(root_moves[0].status, root_beta);
            root_moves[0].status &= 4095 - 256;
            root_moves[0].nodes = 0;
            if (root_print_ok) {
              if (wtm) {
                fh_indicator = "+1";
                if (root_moves[0].status & 16)
                  fh_indicator = "+3";
                if (root_moves[0].status & 32)
                  fh_indicator = "+M";
              } else {
                fh_indicator = "-1";
                if (root_moves[0].status & 16)
                  fh_indicator = "-3";
                if (root_moves[0].status & 32)
                  fh_indicator = "-M";
              }
              Print(2, "               %2i   %s     %2s   ", iteration_depth,
                  DisplayTime(end_time - start_time), fh_indicator);
              if (display_options & 64)
                Print(2, "%d. ", move_number);
              if ((display_options & 64) && !wtm)
                Print(2, "... ");
              Print(2, "%s!                    \n", OutputMove(tree,
                      tree->pv[1].path[1], 1, wtm));
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
            if (!(root_moves[0].status & 0x38)) {
              if (!(root_moves[0].status & 1))
                root_moves[0].status |= 1;
              else if (!(root_moves[0].status & 2))
                root_moves[0].status |= 2;
              else
                root_moves[0].status |= 4;
              root_alpha = SetRootAlpha(root_moves[0].status, root_alpha);
              root_value = root_alpha;
              root_beta = SetRootBeta(root_moves[0].status, root_beta);
              root_moves[0].status &= 4095 - 256;
              root_moves[0].nodes = 0;
              easy_move = 0;
              if (root_print_ok && !abort_after_ply1 && !abort_search) {
                if (wtm) {
                  fl_indicator = "-1";
                  if (root_moves[0].status & 2)
                    fl_indicator = "-3";
                  if (root_moves[0].status & 4)
                    fl_indicator = "-M";
                } else {
                  fl_indicator = "+1";
                  if (root_moves[0].status & 2)
                    fl_indicator = "+3";
                  if (root_moves[0].status & 4)
                    fl_indicator = "+M";
                }
                Print(4, "               %2i   %s     %2s   ",
                    iteration_depth, DisplayTime(ReadClock() - start_time),
                    fl_indicator);
                if (display_options & 64)
                  Print(4, "%d. ", move_number);
                if ((display_options & 64) && !wtm)
                  Print(4, "... ");
                Print(4, "%s?                    \n", OutputMove(tree,
                        root_moves[0].move, 1, wtm));
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
        end_time = ReadClock();
        do {
          sorted = 1;
          for (i = 1; i < n_root_moves - 1; i++) {
            if (root_moves[i].nodes < root_moves[i + 1].nodes) {
              temp = root_moves[i];
              root_moves[i] = root_moves[i + 1];
              root_moves[i + 1] = temp;
              sorted = 0;
            }
          }
        } while (!sorted);
/*
 ************************************************************
 *                                                          *
 *   Notice if there are multiple moves that are producing  *
 *   large trees.  If so, don't search those in parallel by *
 *   setting the flag to avoid this.  We also don't reduce  *
 *   the first 1/4 of the root moves, period, since after   *
 *   deep searches, several of them might have been best    *
 *   moves in previous iterations and need a chance to pop  *
 *   back to the top again.                                 *
 *                                                          *
 ************************************************************
 */
        for (i = 0; i < n_root_moves; i++)
          root_moves[i].status = 0;
        root_moves[0].status |= 64 + 128;
        if (root_moves[0].nodes >= 3)
          for (i = 0; i < n_root_moves; i++) {
            if (i < Min(n_root_moves, 16) &&
                root_moves[i].nodes > root_moves[0].nodes / 3)
              root_moves[i].status |= 64;
            if (i < n_root_moves / 4)
              root_moves[i].status |= 128;
          }
/*
 ************************************************************
 *                                                          *
 *   If requested, print the ply=1 move list along with the *
 *   node counts for the tree each move produced.           *
 *                                                          *
 ************************************************************
 */
        if (display_options & 256) {
          BITBOARD total_nodes = 0;

          Print(128, "       move       nodes     R/P\n");
          for (i = 0; i < n_root_moves; i++) {
            total_nodes += root_moves[i].nodes;
            Print(128, " %10s  " BMF10 "     %d %d\n", OutputMove(tree,
                    root_moves[i].move, 1, wtm), root_moves[i].nodes,
                (root_moves[i].status & 128) == 0,
                (root_moves[i].status & 64) == 0);
          }
          Print(256, "      total  " BMF10 "\n", total_nodes);
        }
        for (i = 0; i < n_root_moves; i++)
          root_moves[i].nodes = 0;
        if (end_time - start_time > 10)
          nodes_per_second =
              tree->nodes_searched * 100 / (BITBOARD) (end_time - start_time);
        else
          nodes_per_second = 1000000;
        if (!abort_after_ply1 && !abort_search && value != -(MATE - 1)) {
          if (root_print_ok) {
            DisplayPV(tree, 5, wtm, end_time - start_time, value,
                &tree->pv[0]);
          } else {
            savevalue = value;
            savepv = tree->pv[0];
            print_ok = 1;
          }
        }
        root_alpha = value - 40;
        root_value = root_alpha;
        root_beta = value + 40;
        if (iteration_depth > 3 && value > MATE - 300 &&
            value >= (MATE - iteration_depth - 1) && value > last_mate_score)
          break;
        if ((iteration_depth >= search_depth) && search_depth)
          break;
        if (abort_after_ply1 || abort_search)
          break;
        end_time = ReadClock() - start_time;
        if (thinking && (int) end_time >= time_limit)
          break;
        if (correct_count >= early_exit)
          break;
#if !defined(NOEGTB)
        if (iteration_depth > 3 && TotalAllPieces <= EGTBlimit && EGTB_use &&
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
            (BITBOARD) tree->nodes_searched * 100 / (BITBOARD) elapsed_end;
      if (!root_print_ok && print_ok) {
        root_print_ok = 1;
        DisplayPV(tree, 5, wtm, end_time - start_time, savevalue, &savepv);
      }
      tree->evaluations = (tree->evaluations) ? tree->evaluations : 1;
      if ((!abort_search || abort_after_ply1) && !puzzling) {
        tree->fail_high++;
        tree->fail_high_first++;
        material = Material / PieceValues(white, pawn);
        Print(8, "              time=%s  mat=%d",
            DisplayTimeKibitz(end_time - start_time), material);
        Print(8, "  n=" BMF, tree->nodes_searched);
        Print(8, "  fh=%u%%",
            (int) ((BITBOARD) tree->fail_high_first * 100 /
                (BITBOARD) tree->fail_high));
        Print(8, "  nps=%s\n", DisplayKM(nodes_per_second));
        Print(16, "              extensions=%s ",
            DisplayKM(tree->extensions_done));
        Print(16, "qchecks=%s ", DisplayKM(tree->qchecks_done));
        Print(16, "reduced=%s ", DisplayKM(tree->reductions_done));
        Print(16, "pruned=%s\n", DisplayKM(tree->moves_pruned));
        Print(16, "              predicted=%d  evals=%s  50move=%d",
            predicted, DisplayKM(tree->evaluations), Rule50Moves(0));
        Print(16, "  EGTBprobes=%s  hits=%s\n", DisplayKM(tree->egtb_probes),
            DisplayKM(tree->egtb_probes_successful));
        Print(16, "              SMP->  splits=%d  aborts=%d  data=%d/%d  ",
            parallel_splits, parallel_aborts, max_split_blocks, MAX_BLOCKS);
        Print(16, "elap=%s\n", DisplayTimeKibitz(elapsed_end));
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
int SetRootAlpha(unsigned char status, int old_root_alpha) {
  if (status & 4)
    return (-MATE - 1);
  if (status & 2)
    return (old_root_alpha - 200);
  if (status & 1)
    return (old_root_alpha - 100);
  return (old_root_alpha);
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
int SetRootBeta(unsigned char status, int old_root_beta) {
  if (status & 32)
    return (MATE + 1);
  if (status & 16)
    return (old_root_beta + 200);
  if (status & 8)
    return (old_root_beta + 100);
  return (old_root_beta);
}
