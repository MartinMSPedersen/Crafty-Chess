#include "chess.h"
#include "data.h"
#include "epdglue.h"
/* last modified 01/17/09 */
/*
 *******************************************************************************
 *                                                                             *
 *   RootMoveList() is used to set up the ply one move list.  It is a  more    *
 *   accurate ordering of the move list than that done for plies deeper than   *
 *   one.  Briefly, Quiesce() is used to obtain the positional score plus the  *
 *   expected gain/loss for pieces that can be captured.                       *
 *                                                                             *
 *******************************************************************************
 */
void RootMoveList(int wtm) {
  int *mvp, *lastm, rmoves[256], sort_value[256];
  int i, done, temp, value;
  TREE *const tree = block[0];
  int tb_value;
  int mating_via_tb = 0;

/*
 ************************************************************
 *                                                          *
 *   If the position at the root is a draw, based on EGTB   *
 *   results, we are going to behave differently.  We will  *
 *   extract the root moves that are draws, and toss the    *
 *   losers out.  Then, we will do a normal search on the   *
 *   moves that draw to try and chose the drawing move that *
 *   gives our opponent the best chance to make an error.   *
 *                                                          *
 ************************************************************
 */
#if !defined(NOEGTB)
  EGTB_draw = 0;
  if (EGTBlimit && TotalAllPieces <= EGTBlimit &&
      Castle(1, white) + Castle(1, black) == 0 &&
      EGTBProbe(tree, 1, wtm, &tb_value)) {
    if (swindle_mode && (tb_value == DrawScore(wtm)))
      if ((wtm && Material > 0) || (!wtm && Material < 0))
        EGTB_draw = 1;
    if (tb_value > MATE - 300)
      mating_via_tb = -tb_value - 1;
  }
#endif
/*
 ************************************************************
 *                                                          *
 *   First, use GenerateMoves() to generate the set of      *
 *   legal moves from the root position.                    *
 *                                                          *
 ************************************************************
 */
  easy_move = 0;
  lastm = GenerateCaptures(tree, 1, wtm, rmoves);
  lastm = GenerateNoncaptures(tree, 1, wtm, lastm);
  n_root_moves = lastm - rmoves;
/*
 ************************************************************
 *                                                          *
 *   Now make each move and use Evaluate() to compute the   *
 *   positional evaluation.                                 *
 *                                                          *
 ************************************************************
 */
  for (mvp = rmoves; mvp < lastm; mvp++) {
    value = -4000000;
#if defined(TRACE)
    if (trace_level >= 1) {
      tree->curmv[1] = *mvp;
      tree->phase[1] = HASH_MOVE;
      Trace(tree, 1, 0, wtm, -MATE, MATE, "RootMoves()", tree->phase[1]);
    }
#endif
    MakeMove(tree, 1, *mvp, wtm);
    tree->nodes_searched++;
    if (!Check(wtm))
      do {
        tree->curmv[1] = *mvp;
#if !defined(NOEGTB)
        if (TotalAllPieces <= EGTBlimit && EGTB_draw &&
            Castle(1, white) + Castle(1, black) == 0) {
          i = EGTBProbe(tree, 2, Flip(wtm), &tb_value);
          if (i && tb_value != DrawScore(Flip(wtm)))
            break;
        }
        if (mating_via_tb && TotalAllPieces <= EGTBlimit &&
            Castle(1, white) + Castle(1, black) == 0) {
          i = EGTBProbe(tree, 2, Flip(wtm), &tb_value);
          if (i && ((mating_via_tb > DrawScore(Flip(wtm)) &&
                      tb_value < mating_via_tb) ||
                  (mating_via_tb < DrawScore(Flip(wtm)) &&
                      tb_value > mating_via_tb)))
            break;
        }
#endif
        value = -Quiesce(tree, -MATE, MATE, Flip(wtm), 2, 0);
/*
 ************************************************************
 *                                                          *
 *   Add in a bonus if this move is part of the previous    *
 *   principal variation.  It was good in the search, we    *
 *   should try it first now.                               *
 *                                                          *
 ************************************************************
 */
        if ((Piece(*mvp) == Piece(last_pv.path[1])) &&
            (From(*mvp) == From(last_pv.path[1])) &&
            (To(*mvp) == To(last_pv.path[1])) &&
            (Captured(*mvp) == Captured(last_pv.path[1])) &&
            (Promote(*mvp) == Promote(last_pv.path[1])))
          value += 2000000;
/*
 ************************************************************
 *                                                          *
 *   Fudge the score for promotions so that promotion to a  *
 *   queen is tried first.                                  *
 *                                                          *
 ************************************************************
 */
        if (Promote(*mvp) && (Promote(*mvp) != queen))
          value -= 50;
      } while (0);
    sort_value[mvp - rmoves] = value;
    UnmakeMove(tree, 1, *mvp, wtm);
  }
/*
 ************************************************************
 *                                                          *
 *   Sort the moves into order based on the scores returned *
 *   by Quiesce() which includes evaluation + captures.     *
 *                                                          *
 ************************************************************
 */
  do {
    done = 1;
    for (i = 0; i < lastm - rmoves - 1; i++) {
      if (sort_value[i] < sort_value[i + 1]) {
        temp = sort_value[i];
        sort_value[i] = sort_value[i + 1];
        sort_value[i + 1] = temp;
        temp = rmoves[i];
        rmoves[i] = rmoves[i + 1];
        rmoves[i + 1] = temp;
        done = 0;
      }
    }
  } while (!done);
/*
 ************************************************************
 *                                                          *
 *   Trim the move list to eliminate those moves that hang  *
 *   the king and are illegal.                              *
 *                                                          *
 *   If the first move in the list is better than the next  *
 *   move by at least two pawns, we set the "easy move"     *
 *   flag which will let us terminate the search early, if  *
 *   we don't have any fail lows on the move before we run  *
 *   out of time.                                           *
 *                                                          *
 ************************************************************
 */
  for (; n_root_moves; n_root_moves--)
    if (sort_value[n_root_moves - 1] > -3000000)
      break;
  if (sort_value[0] > 1000000)
    sort_value[0] -= 2000000;
  if (sort_value[0] > sort_value[1] + 200 &&
      ((To(rmoves[0]) == To(last_opponent_move) &&
              Captured(rmoves[0]) == Piece(last_opponent_move)) ||
          sort_value[0] < PAWN_VALUE))
    easy_move = 1;
/*
 ************************************************************
 *                                                          *
 *   Debugging output to dump root move list and the stuff  *
 *   used to sort them, for testing and debugging.          *
 *                                                          *
 ************************************************************
 */
  if (display_options & 512) {
    Print(512, "%d moves at root\n", n_root_moves);
    Print(512, "        move   score\n");
    for (i = 0; i < n_root_moves; i++) {
      tree->curmv[1] = rmoves[i];
      Print(512, "%12s", OutputMove(tree, rmoves[i], 1, wtm));
      Print(512, "%8d\n", sort_value[i]);
    }
  }
/*
 ************************************************************
 *                                                          *
 *   check to see if we are in the special mode where moves *
 *   need to be searched because of missing EGTBs.          *
 *                                                          *
 ************************************************************
 */
#if !defined(NOEGTB)
  if (mating_via_tb) {
    for (i = 0; i < n_root_moves; i++) {
      tree->curmv[1] = rmoves[i];
      MakeMove(tree, 1, rmoves[i], wtm);
      if (mating_via_tb && TotalAllPieces <= EGTBlimit &&
          Castle(1, white) + Castle(1, black) == 0)
        temp =
            (EGTBProbe(tree, 2, Flip(wtm),
                &tb_value) != DrawScore(Flip(wtm)));
      else
        temp = 0;
      UnmakeMove(tree, 1, rmoves[i], wtm);
      if (temp)
        break;
    }
    EGTB_search = (i == n_root_moves);
  } else
    EGTB_search = 0;
#endif
/*
 ************************************************************
 *                                                          *
 *   Copy the root moves into the root_move structure array *
 *   for use by NextRootMove().                             *
 *                                                          *
 ************************************************************
 */
  for (i = 0; i < n_root_moves; i++) {
    root_moves[i].move = rmoves[i];
    root_moves[i].nodes = 0;
    root_moves[i].status = 128;
  }
  root_moves[0].status = 0;
  return;
}
