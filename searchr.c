#include "chess.h"
#include "data.h"
#include "epdglue.h"
/* modified 11/02/07 */
/*
 *******************************************************************************
 *                                                                             *
 *   SearchRoot() is the recursive routine used to implement the alpha/beta    *
 *   negamax search (similar to minimax but simpler to code.)  SearchRoot() is *
 *   only called when ply=1.  it is somewhat different from Search() in that   *
 *   some things (null move search, hash lookup, etc.) are not useful at the   *
 *   root of the tree.  SearchRoot() calls Search() to search any positions    *
 *   that are below ply=1.                                                     *
 *                                                                             *
 *******************************************************************************
 */
int SearchRoot(TREE * RESTRICT tree, int alpha, int beta, int wtm, int depth)
{
  register int first_move = 1;
  register int value;
  register int extensions;
  BITBOARD begin_root_nodes;

/*
 ************************************************************
 *                                                          *
 *   now iterate through the move list and search the       *
 *   resulting positions.  note that SearchRoot() does not  *
 *   search illegal moves since RootMoves() screened each   *
 *   move before adding it to the permanent ply-1 move list *
 *   earlier.                                               *
 *                                                          *
 ************************************************************
 */
  tree->inchk[1] = Check(wtm);
  while ((tree->phase[1] = NextRootMove(tree, tree, wtm))) {
#if defined(TRACE)
    if (trace_level > 0)
      Trace(tree, 1, depth, wtm, alpha, beta, "SearchRoot", tree->phase[1]);
#endif
    MakeMove(tree, 1, tree->curmv[1], wtm);
    do {
      extensions = SearchExtensions(tree, wtm, 1, depth);
      begin_root_nodes = tree->nodes_searched;
/*
 ************************************************************
 *                                                          *
 *   if this is the first move searched at ply=1, we search *
 *   using the normal alpha/beta bounds.  if not, we first  *
 *   search with alpha, alpha+1 (PVS search).  if this      *
 *   search fails high, we re-search using the normal       *
 *   bounds.  if allowed by inter-iteration analysis, some  *
 *   root moves might be searched with a reduced depth (as  *
 *   in the LMR idea).  if a reduced move fails high, we    *
 *   always re-search it with the normal depth before we    *
 *   accept the fail-high as a real one and re-search with  *
 *   the original alpha/beta bounds.  so a single move      *
 *   might fail high on the reduced search, get re-searched *
 *   with the original depth, and if it fails high here, we *
 *   then re-search it a third time with the original       *
 *   search bound to see if it is really a new best move.   *
 *                                                          *
 ************************************************************
 */
      if (first_move) {
        if (depth - 1 + extensions > 0)
          value =
              -Search(tree, -beta, -alpha, Flip(wtm), depth - 1 + extensions, 2,
              DO_NULL);
        else
          value = -QuiesceChecks(tree, -beta, -alpha, Flip(wtm), 2);
        first_move = 0;
      } else {
        if (depth - 1 + extensions > 0) {
          value =
              -Search(tree, -alpha - 1, -alpha, Flip(wtm),
              depth - 1 + extensions, 2, DO_NULL);
          if (value > alpha && extensions < 0)
            value =
                -Search(tree, -alpha - 1, -alpha, Flip(wtm), depth - 1, 2,
                DO_NULL);
        } else
          value = -QuiesceChecks(tree, -alpha - 1, -alpha, Flip(wtm), 2);
        if (abort_search)
          break;
        if ((value > alpha) && (value < beta)) {
          if (depth - 1 + extensions > 0)
            value =
                -Search(tree, -beta, -alpha, Flip(wtm), depth - 1 + extensions,
                2, DO_NULL);
          else
            value = -QuiesceChecks(tree, -beta, -alpha, Flip(wtm), 2);
        }
      }
/*
 ************************************************************
 *                                                          *
 *   now the move has been searched to a satisfactory       *
 *   conclusion.  we check to see if the search was aborted *
 *   due to time constraints, and if so, we just return and *
 *   do not modify the best move or anything.  if not, we   *
 *   then test for a beta cutoff and return to the control  *
 *   for the iterated search (Iterate()) to alter the       *
 *   aspiration window if needed.                           *
 *                                                          *
 ************************************************************
 */
      if (abort_search)
        break;
      root_moves[tree->root_move].nodes =
          tree->nodes_searched - begin_root_nodes;
      if (value > alpha) {
        Output(tree, value, beta);
        root_value = alpha;
        if (value >= beta) {
          Killer(tree, 1, tree->curmv[1]);
          UnmakeMove(tree, 1, tree->curmv[1], wtm);
          return (value);
        }
        alpha = value;
      }
      root_value = alpha;
    } while (0);
    UnmakeMove(tree, 1, tree->curmv[1], wtm);
    if (abort_search)
      return (0);
/*
 ************************************************************
 *                                                          *
 *   after searching the first move, we can now begin a     *
 *   parallel search at the root if root splitting is       *
 *   allowed, and the next move is not flagged as a "serial *
 *   search only" type move because we think it might be a  *
 *   new best move when searched further.  otherwise, back  *
 *   to the top of the NextRootMove() loop to search the    *
 *   remaining root moves one at a time.                    *
 *                                                          *
 ************************************************************
 */
#if (CPUS > 1)
    if (split_at_root && smp_idle && NextRootMoveParallel()) {
      tree->alpha = alpha;
      tree->beta = beta;
      tree->value = alpha;
      tree->wtm = wtm;
      tree->ply = 1;
      tree->depth = depth;
      if (Thread(tree)) {
        if (abort_search)
          return (0);
        value = tree->search_value;
        if (value > alpha) {
          if (value >= beta) {
            Killer(tree, 1, tree->curmv[1]);
            tree->fail_high++;
            return (value);
          }
          alpha = value;
          break;
        }
      }
    }
#endif
  }
/*
 ************************************************************
 *                                                          *
 *   all moves have been searched.  if none were legal,     *
 *   return either MATE or DRAW depending on whether the    *
 *   side to move is in check or not.                       *
 *                                                          *
 ************************************************************
 */
  if (abort_search || time_abort)
    return (0);
  if (first_move == 1) {
    value = (Check(wtm)) ? -(MATE - 1) : DrawScore(wtm);
    if (value >= alpha && value < beta) {
      tree->pv[0].pathl = 0;
      tree->pv[0].pathh = 0;
      tree->pv[0].pathd = iteration_depth;
      Output(tree, value, beta);
#if defined(TRACE)
      if (trace_level > 0)
        printf("Search() no moves!  ply=1\n");
#endif
    }
    return (value);
  } else {
    Killer(tree, 1, tree->pv[1].path[1]);
    return (alpha);
  }
}
