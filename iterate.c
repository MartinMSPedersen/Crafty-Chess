#include "chess.h"
#include "data.h"
#include "epdglue.h"
/* last modified 04/24/14 */
/*
 *******************************************************************************
 *                                                                             *
 *   Iterate() is the routine used to drive the iterated search.  It           *
 *   repeatedly calls search, incrementing the search depth after each call,   *
 *   until either time is exhausted or the maximum set search depth is         *
 *   reached.                                                                  *
 *                                                                             *
 *   Crafty has several specialized modes that influence how moves are chosen  *
 *   and when.                                                                 *
 *                                                                             *
 *   (1) "mode tournament" is a special way of handling book moves.  Here we   *
 *   are dealing with pondering.  We play our move, and then we take all of    *
 *   the known book moves for the opponent (moves where we have an instant     *
 *   reply since they are in the book) and eliminate those from the set of     *
 *   root moves to search.  We do a short search to figure out which of those  *
 *   non-book moves is best, and then we ponder that move.  It will look like  *
 *   we are always out of book, but we are not.  We are just looking for one   *
 *   of two cases:  (i) The opponent's book runs out and he doesn't play the   *
 *   expected book line (which is normally a mistake), where this will give us *
 *   a good chance of pondering the move he will actually play (a non-book     *
 *   move) without sitting around and doing nothing until he takes us out of   *
 *   book;  (ii) Our book doesn't have a reasonable choice, so we do a search  *
 *   and ponder a better choice so again we are not wasting time.  The value   *
 *   of "mode" will be set to "tournament" to enable this.                     *
 *                                                                             *
 *   (2) "book random 0" tells the program to enumerate the list of known book *
 *   moves, but rather than playing one randomly, we do a shortened search and *
 *   use the normal move selection approach (which will, unfortunately, accept *
 *   many gambits that a normal book line would bypass as too risky.  But this *
 *   can also find a better book move in many positions, since many book lines *
 *   are not verified with computer searches.                                  *
 *                                                                             *
 *   Those modes are handled within Book() and Ponder() but they all use the   *
 *   same iterated search as is used for normal moves.                         *
 *                                                                             *
 *******************************************************************************
 */
int Iterate(int wtm, int search_type, int root_list_done) {
  TREE *const tree = block[0];
  int i, root_alpha, old_root_alpha, old_root_beta;
  int value = 0, twtm, correct, correct_count;
  char *fl_indicator, *fh_indicator;
#if (CPUS > 1)
  pthread_t pt;
#endif

/*
 ************************************************************
 *                                                          *
 *  Initialize draw score.  This has to be done here since  *
 *  we don't know whether we are searching for black or     *
 *  white until we get to this point.                       *
 *                                                          *
 ************************************************************
 */
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
/*
 ************************************************************
 *                                                          *
 *  Initialize statistical counters and such.               *
 *                                                          *
 ************************************************************
 */
  idle_time = 0;
  tree->curmv[0] = 0;
  abort_search = 0;
  book_move = 0;
  program_start_time = ReadClock();
  start_time = ReadClock();
  root_wtm = wtm;
  kibitz_depth = 0;
  tree->nodes_searched = 0;
  tree->fail_highs = 0;
  tree->fail_high_first_move = 0;
  parallel_splits = 0;
  parallel_aborts = 0;
  correct_count = 0;
  burp = 15 * 100;
  transposition_age = (transposition_age + 1) & 0x1ff;
  next_time_check = nodes_between_time_checks;
  tree->evaluations = 0;
  tree->egtb_probes = 0;
  tree->egtb_probes_successful = 0;
  tree->extensions_done = 0;
  tree->qchecks_done = 0;
  tree->moves_fpruned = 0;
  for (i = 0; i < 16; i++)
    tree->LMR_done[i] = 0;
  for (i = 0; i < 32; i++)
    tree->null_done[i] = 0;
  HistoryAge();
  root_wtm = wtm;
/*
 ************************************************************
 *                                                          *
 *  We do a quick check to see if this position has a known *
 *  book reply.  If not, we drop into the main iterated     *
 *  search, otherwise we skip to the bottom and return the  *
 *  move that Book() returned.                              *
 *                                                          *
 *  Note the "booking" exception below.  If you use the     *
 *  "book random 0" you instruct Crafty to enumerate the    *
 *  known set of book moves, and then initiate a normal     *
 *  iterated search, but with just those known book moves   *
 *  included in the root move list.  We therefore choose    *
 *  (based on a normal search / evaluation but with a lower *
 *  time limit) from the book moves given.                  *
 *                                                          *
 ************************************************************
 */
  if (booking || !Book(tree, wtm, root_list_done))
    do {
      if (abort_search)
        break;
#if !defined(NOEGTB)
      if (EGTB_draw && !puzzling && swindle_mode)
        EGTB_use = 0;
      else
        EGTB_use = EGTBlimit;
      if (EGTBlimit && !EGTB_use)
        Print(128, "Drawn at root, trying for swindle.\n");
#endif
/*
 ************************************************************
 *                                                          *
 *  The first action for a real search is to generate the   *
 *  root move list if it has not already been done.  For    *
 *  some circumstances, such as a non-random book move      *
 *  search, we are given the root move list, which only     *
 *  contains the known book moves.  Those are all we need   *
 *  to search.  If there are no legal moves, it is either   *
 *  mate or draw depending on whether the side to move is   *
 *  in check or not (mate or stalemate.)                    *
 *                                                          *
 *  Why would there be already be a root move list?  See    *
 *  the two modes described at the top (mode tournament and *
 *  book random 0) which would have already inserted just   *
 *  the moves that should be searched.                      *
 *                                                          *
 ************************************************************
 */
      if (!root_list_done)
        RootMoveList(wtm);
      if (n_root_moves == 0) {
        program_end_time = ReadClock();
        tree->pv[0].pathl = 0;
        tree->pv[0].pathd = 0;
        if (Check(wtm))
          value = -(MATE - 1);
        else
          value = DrawScore(wtm);
        Print(6, "        depth     time       score   variation\n");
        Print(6, "                                     (no moves)\n");
        tree->nodes_searched = 1;
        if (!puzzling)
          last_root_value = value;
        return value;
      }
/*
 ************************************************************
 *                                                          *
 *  Now set the search time and iteratively call Search()   *
 *  to analyze the position deeper and deeper.  Note that   *
 *  Search() is called with an alpha/beta window roughly    *
 *  1/3 of a pawn wide, centered on the score last returned *
 *  by search.                                              *
 *                                                          *
 ************************************************************
 */
      TimeSet(search_type);
      iteration_depth = 1;
      noise_block = 0;
      if (last_pv.pathd > 1) {
        iteration_depth = last_pv.pathd + 1;
        value = last_root_value;
        tree->pv[0] = last_pv;
        noise_block = 1;
      } else
        difficulty = 100;
      Print(6, "        depth     time       score   variation (%d)\n",
          iteration_depth);
      abort_search = 0;
/*
 ************************************************************
 *                                                          *
 *  Set the initial search bounds based on the last search  *
 *  or default values.                                      *
 *                                                          *
 ************************************************************
 */
      tree->pv[0] = last_pv;
      if (iteration_depth > 1) {
        root_alpha = Max(-MATE, last_value - 16);
        root_beta = Min(MATE, last_value + 16);
      } else {
        root_alpha = -MATE;
        root_beta = MATE;
      }
/*
 ************************************************************
 *                                                          *
 *  If we are using multiple threads, and they have not     *
 *  been started yet, then start them now as the search is  *
 *  ready to begin.                                         *
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
          thread[proc].tree = 0;
#  if defined(UNIX)
          pthread_create(&pt, &attributes, ThreadInit, (void *) proc);
#  else
          NumaStartThread(ThreadInit, (void *) proc);
#  endif
          smp_threads++;
        }
        Print(128, " <done>\n");
      }
      WaitForAllThreadsInitialized();
#endif
      if (search_nodes)
        nodes_between_time_checks = search_nodes;
      for (; iteration_depth <= MAXPLY - 5; iteration_depth++) {
/*
 ************************************************************
 *                                                          *
 *  Now install the old PV into the hash table so that      *
 *  these moves will be followed first.  We do this since   *
 *  the last iteration done could have overwritten the PV   *
 *  as the last few root moves were searched.               *
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
 *  Now we call Search() and start the next search          *
 *  iteration.  We already have solid alpha/beta bounds set *
 *  up for the aspiration search.  When each iteration      *
 *  completes, these aspiration values are recomputed and   *
 *  used for the next iteration.                            *
 *                                                          *
 *  We need to set "nodes_between_time_checks" to a value   *
 *  that will force us to check the time reasonably often   *
 *  without wasting excessive time doing this check.  As    *
 *  the target time limit gets shorter, we shorten the      *
 *  interval between time checks to avoid burning time off  *
 *  of the clock unnecessarily.                             *
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
            nodes_between_time_checks = Min(nodes_per_second, 1000000);
        }
        if (search_nodes)
          nodes_between_time_checks = search_nodes - tree->nodes_searched;
        nodes_between_time_checks =
            Min(nodes_between_time_checks, MAX_TC_NODES);
        next_time_check = nodes_between_time_checks;
/*
 ************************************************************
 *                                                          *
 *  This loop will execute until we either run out of time  *
 *  or complete this iteration.  Since we can return to     *
 *  Iterate() multiple times during this iteration, due to  *
 *  multiple fail highs (and perhaps even an initial fail   *
 *  low) we stick in this loop until we have completed all  *
 *  root moves or TimeCheck() tells us it is time to stop.  *
 *                                                          *
 ************************************************************
 */
        failhi_delta = 16;
        faillo_delta = 16;
        while (1) {
          thread[0].tree = block[0];
          if (smp_max_threads > 1)
            smp_split = 1;
          tree->rep_index--;
          value =
              Search(tree, root_alpha, root_beta, wtm, iteration_depth, 1,
              Check(wtm), 0);
          tree->rep_index++;
          end_time = ReadClock();
          if (abort_search)
            break;
          old_root_alpha = root_alpha;
          old_root_beta = root_beta;
/*
 ************************************************************
 *                                                          *
 *  Check for the case where we get a score back that is    *
 *  greater than or equal to beta.  This is called a fail   *
 *  high condition and requires a re-search with a better   *
 *  (more optimistic) beta value so that we can discover    *
 *  just how good this move really is.                      *
 *                                                          *
 *  Note that each time we return here, we need to increase *
 *  the upper search bound (beta).  We have a value named   *
 *  failhi_delta that is initially set to 16 on the first   *
 *  fail high of a particular move.  We increase beta by    *
 *  this value each time we fail high.  However, each time  *
 *  we fail high, we also double this value so that we      *
 *  increase beta at an ever-increasing rate.  Small jumps  *
 *  at first let us detect marginal score increases while   *
 *  still allowing cutoffs for branches with excessively    *
 *  high scores.  But each re-search sees the margin double *
 *  which quickly increases the bound as needed.            *
 *                                                          *
 *  We also use ComputeDifficulty() to adjust the level of  *
 *  difficulty for this move since we might be changing our *
 *  mind at the root.  (If we are failing high on the first *
 *  root move we skip this update.)                         *
 *                                                          *
 ************************************************************
 */
          if (value >= root_beta) {
            root_beta = Min(old_root_beta + failhi_delta, MATE);
            failhi_delta *= 2;
            if (failhi_delta > 10 * PAWN_VALUE)
              failhi_delta = 99999;
            root_moves[0].status &= 0xf7;
            if ((root_moves[0].status & 2) == 0)
              difficulty = ComputeDifficulty(difficulty, +1);
            root_moves[0].status |= 2;
            if (end_time - start_time >= noise_level) {
              fh_indicator = (wtm) ? "++" : "--";
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
              idle_percent =
                  100 - Min(100,
                  100 * idle_time / (smp_max_threads * (end_time -
                          start_time) + 1));
              Kibitz(6, wtm, iteration_depth, end_time - start_time, value,
                  tree->nodes_searched, idle_percent,
                  tree->egtb_probes_successful, kibitz_text);
            }
/*
 ************************************************************
 *                                                          *
 *  Check for the case where we get a score back that is    *
 *  less than or equal to alpha.  This is called a fail     *
 *  low condition and requires a re-search with a better    *
 *  more pessimistic)) alpha value so that we can discover  *
 *  just how bad this move really is.                       *
 *                                                          *
 *  Note that each time we return here, we need to decrease *
 *  the lower search bound (alpha).  We have a value named  *
 *  faillo_delta that is initially set to 16 on the first   *
 *  fail low of a particular move.  We decrease alpha by    *
 *  this value each time we fail low.  However, each time   *
 *  we fail low, we also double this value so that we       *
 *  decrease alpha at an ever-increasing rate.  Small jumps *
 *  at first let us detect marginal score decreases while   *
 *  still allowing cutoffs for branches with excessively    *
 *  low scores.  But each re-search sees the margin double  *
 *  which quickly decreases the bound as needed.            *
 *                                                          *
 *  We also use ComputeDifficulty() to adjust the level of  *
 *  difficulty for this move since we are failing low on    *
 *  the first move at the root, and we don't want to stop   *
 *  before we have a chance to find a better one.           *
 *                                                          *
 ************************************************************
 */
          } else if (value <= root_alpha) {
            root_alpha = Max(old_root_alpha - faillo_delta, -MATE);
            faillo_delta *= 2;
            if (faillo_delta > 10 * PAWN_VALUE)
              faillo_delta = 99999;
            root_moves[0].status &= 0xf7;
            if ((root_moves[0].status & 1) == 0)
              difficulty = ComputeDifficulty(Max(100, difficulty), -1);
            root_moves[0].status |= 1;
            if (ReadClock() - start_time >= noise_level && !abort_search) {
              fl_indicator = (wtm) ? "--" : "++";
              Print(4, "         %2i   %s     %2s   ", iteration_depth,
                  Display2Times(ReadClock() - start_time), fl_indicator);
              if (display_options & 64)
                Print(4, "%d. ", move_number);
              if ((display_options & 64) && !wtm)
                Print(4, "... ");
              Print(4, "%s? ", OutputMove(tree, root_moves[0].move, 1, wtm));
              Print(4, "(%c%s)                  \n", (Flip(wtm)) ? '>' : '<',
                  DisplayEvaluationKibitz(old_root_alpha, wtm));
            }
          } else
            break;
        }
        if (value > root_alpha && value < root_beta)
          last_root_value = value;
/*
 ************************************************************
 *                                                          *
 *  If we are running a test suite, check to see if we can  *
 *  exit the search.  This happens when N successive        *
 *  iterations produce the correct solution.  N is set by   *
 *  the test command in Option().                           *
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
 *  Notice that we don't search moves that were best over   *
 *  the last 3 iterations in parallel, nor do we reduce     *
 *  those since they are potential best moves again.        *
 *                                                          *
 ************************************************************
 */
        for (i = 0; i < n_root_moves; i++) {
          if (root_moves[i].bm_age)
            root_moves[i].bm_age--;
          if (root_moves[i].bm_age)
            root_moves[i].status &= 0xfb;
          else
            root_moves[i].status |= 4;
        }
        difficulty = ComputeDifficulty(difficulty, 0);
/*
 ************************************************************
 *                                                          *
 *  If requested, print the ply=1 move list along with the  *
 *  flags for each move.  Once we print this (if requested) *
 *  we can then clear all of the status flags (except the   *
 *  "ok to search in parallel or reduce" flag) to prepare   *
 *  for the start of the next iteration, since that is the  *
 *  only flag that needs to be carried forward to the next  *
 *  iteration.                                              *
 *                                                          *
 ************************************************************
 */
        if (display_options & 256) {
          Print(128, "       move  age  R ! ?\n");
          for (i = 0; i < n_root_moves; i++) {
            Print(128, " %10s   %d   %d %d %d\n", OutputMove(tree,
                    root_moves[i].move, 1, wtm), root_moves[i].bm_age,
                (root_moves[i].status & 4) != 0,
                (root_moves[i].status & 2) != 0,
                (root_moves[i].status & 1) != 0);
          }
        }
        for (i = 0; i < n_root_moves; i++)
          root_moves[i].status &= 4;
/*
 ************************************************************
 *                                                          *
 *  Here we simply display the PV from the current search   *
 *  iteration, and then set the aspiration for the next     *
 *  iteration to the current score +/- 16.                  *
 *                                                          *
 ************************************************************
 */
        if (end_time - start_time > 10)
          nodes_per_second =
              tree->nodes_searched * 100 / (uint64_t) (end_time - start_time);
        else
          nodes_per_second = 1000000;
        if (!abort_search && value != -(MATE - 1)) {
          if (end_time - start_time > noise_level) {
            DisplayPV(tree, 5, wtm, end_time - start_time, &tree->pv[0], 0);
            noise_block = 0;
          } else
            noise_block = 1;
        }
        root_alpha = Max(-MATE, value - 16);
        root_beta = Min(MATE, value + 16);
/*
 ************************************************************
 *                                                          *
 *  There are multiple termination criteria that are used.  *
 *  The first and most obvious is that we have exceeded the *
 *  target time limit.  Others include reaching a user-set  *
 *  maximum search depth, finding a mate and we searched so *
 *  deep there is little chance of another iteration find-  *
 *  ing a shorter mate; the search was aborted due to some  *
 *  sort of user input (usually during pondering);  and     *
 *  finally, when running a test suite, we had the correct  *
 *  best move for N successive iterations and the user      *
 *  asked us to stop after that number.                     *
 *                                                          *
 ************************************************************
 */
        if (TimeCheck(tree, 0))
          break;
        if (iteration_depth > 3 && value > 32000 &&
            value >= (MATE - iteration_depth + 3)
            && value > last_mate_score)
          break;
        if ((iteration_depth >= search_depth) && search_depth)
          break;
        if (abort_search)
          break;
        end_time = ReadClock() - start_time;
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
 *  Search done, now display statistics, depending on the   *
 *  display options set (see display command in Option().)  *
 *                                                          *
 *  Simple kludge here.  If the last search was quite deep  *
 *  while we were pondering, we start this iteration at the *
 *  last depth - 1.  Sometimes that will result in a search *
 *  that is deep enough that we do not produce/print a PV   *
 *  before time runs out.  On other occasions, noise_level  *
 *  prevents us from printing anything, leaving us with no  *
 *  output during this PV.  We initialize a variable named  *
 *  noise_block to 1.  If, during this iteration, we do     *
 *  manage to print a PV, we set it to zero until the next  *
 *  iteration starts.  Otherwise this will force us to at   *
 *  display the PV from the last iteration (first two moves *
 *  were removed in main(), so they are not present) so we  *
 *  have some sort of output for this iteration.            *
 *                                                          *
 ************************************************************
 */
      end_time = ReadClock();
      if (end_time > 10)
        nodes_per_second =
            (uint64_t) tree->nodes_searched * 100 / Max((uint64_t) end_time -
            start_time, 1);
      if (abort_search != 2 && !puzzling) {
        if (noise_block)
          DisplayPV(tree, 5, wtm, end_time - start_time, &tree->pv[0], 1);
        tree->evaluations = (tree->evaluations) ? tree->evaluations : 1;
        tree->fail_highs++;
        tree->fail_high_first_move++;
        idle_percent =
            100 - Min(100,
            100 * idle_time / (smp_max_threads * (end_time - start_time) +
                1));
        Print(8, "        time=%s(%d%%)",
            DisplayTimeKibitz(end_time - start_time), idle_percent);
        Print(8, "  n=%" PRIu64 "(%s)", tree->nodes_searched,
            DisplayKMB(tree->nodes_searched, 0));
        Print(8, "  fh1=%d%%",
            tree->fail_high_first_move * 100 / tree->fail_highs);
        Print(8, "  50move=%d", Reversible(0));
        Print(8, "  nps=%s\n", DisplayKMB(nodes_per_second, 0));
        Print(16, "        ext=%s", DisplayKMB(tree->extensions_done, 0));
        Print(16, "  pruned=%s", DisplayKMB(tree->moves_fpruned, 0));
        Print(16, "  qchks=%s", DisplayKMB(tree->qchecks_done, 0));
        Print(16, "  predicted=%d\n", predicted);
        Print(16, "        LMReductions: ");
        for (i = 1; i < 16; i++)
          if (tree->LMR_done[i])
            Print(16, "%d/%s  ", i, DisplayKMB(tree->LMR_done[i], 0));
        Print(16, "\n");
        Print(16, "        null searches (R): ");
        for (i = 1; i < 32; i++)
          if (tree->null_done[i])
            Print(16, "%d/%s  ", i, DisplayKMB(tree->null_done[i], 0));
        Print(16, "\n");
        Print(16, "        splits=%s", DisplayKMB(parallel_splits, 0));
        Print(16, "  aborts=%s", DisplayKMB(parallel_aborts, 0));
        Print(16, "  data=%d%%", 100 * max_split_blocks / Max(MAX_BLOCKS, 1));
        Print(16, "  probes=%s", DisplayKMB(tree->egtb_probes, 0));
        Print(16, "  hits=%s\n", DisplayKMB(tree->egtb_probes_successful, 0));
      }
    } while (0);
/*
 ************************************************************
 *                                                          *
 *  If this is a known book position, Book() has already    *
 *  set the PV/best move so we can return without doing the *
 *  iterated search at all.                                 *
 *                                                          *
 ************************************************************
 */
  else {
    last_root_value = 0;
    value = 0;
    book_move = 1;
    tree->pv[0] = tree->pv[1];
    if (analyze_mode)
      Kibitz(4, wtm, 0, 0, 0, 0, 0, 0, kibitz_text);
  }
/*
 ************************************************************
 *                                                          *
 *  If "smp_nice" is set, and we are not allowed to ponder  *
 *  while waiting on the opponent to move, then terminate   *
 *  the parallel threads so they won't sit in their normal  *
 *  spin-wait loop while waiting for new work which will    *
 *  "burn" smp_max_threads - 1 cpus, penalizing anything    *
 *  else that might be running (such as another chess       *
 *  engine we might be playing in a ponder=off match.)      *
 *                                                          *
 ************************************************************
 */
  if (smp_nice && ponder == 0 && smp_threads) {
    int proc;
    Print(128, "terminating SMP processes.\n");
    for (proc = 1; proc < CPUS; proc++)
      thread[proc].tree = (TREE *) - 1;
    while (smp_threads);
    smp_idle = 0;
    smp_split = 0;
  }
  program_end_time = ReadClock();
  search_move = 0;
  if (quit)
    CraftyExit(0);
  return last_root_value;
}
