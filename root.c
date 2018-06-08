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
#if !defined(NOEGTB)
  int mating_via_tb = 0, tb_value;
#endif

/*
 ************************************************************
 *                                                          *
 *  If the position at the root is a draw, based on EGTB    *
 *  results, we are going to behave differently.  We will   *
 *  extract the root moves that are draws, and toss the     *
 *  losers out.  Then, we will do a normal search on the    *
 *  moves that draw to try and chose the drawing move that  *
 *  gives our opponent the best chance to make an error.    *
 *  This search will be done sans EGTB probes since we al-  *
 *  ready know this is a draw at the root.  We simply find  *
 *  the best move (based on search/eval) that preserves the *
 *  draw.                                                   *
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
    if (tb_value > 32000)
      mating_via_tb = -tb_value - 1;
  }
#endif
/*
 ************************************************************
 *                                                          *
 *  First, use GenerateMoves() to generate the set of legal *
 *  moves from the root position.                           *
 *                                                          *
 ************************************************************
 */
  lastm = GenerateCaptures(tree, 1, wtm, rmoves);
  lastm = GenerateNoncaptures(tree, 1, wtm, lastm);
  n_root_moves = lastm - rmoves;
/*
 ************************************************************
 *                                                          *
 *  Now make each move and use Quiesce() to analyze the     *
 *  potential captures and return a minimax score.          *
 *                                                          *
 *  Special case:  if this is an egtb draw at the root,     *
 *  then this is where we cull the non-drawing moves by     *
 *  doing an EGTB probe for each move.  Any moves that lose *
 *  are left with a very bad sorting score to move them to  *
 *  the end of the root move list.                          *
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
          if (i && ((mating_via_tb > DrawScore(Flip(wtm))
                      && tb_value < mating_via_tb)
                  || (mating_via_tb < DrawScore(Flip(wtm))
                      && tb_value > mating_via_tb)))
            break;
        }
#endif
        value = -Quiesce(tree, -MATE, MATE, Flip(wtm), 2, 0);
/*
 ************************************************************
 *                                                          *
 *  Add in a bonus if this move is part of the previous     *
 *  principal variation.  It was good in the search, we     *
 *  should try it first now.                                *
 *                                                          *
 ************************************************************
 */
        if ((Piece(*mvp) == Piece(last_pv.path[1]))
            && (From(*mvp) == From(last_pv.path[1]))
            && (To(*mvp) == To(last_pv.path[1]))
            && (Captured(*mvp) == Captured(last_pv.path[1]))
            && (Promote(*mvp) == Promote(last_pv.path[1])))
          value += 2000000;
/*
 ************************************************************
 *                                                          *
 *  Fudge the score for promotions so that promotion to a   *
 *  queen is tried first.                                   *
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
 *  Sort the moves into order based on the scores returned  *
 *  by Quiesce() which includes evaluation + captures.      *
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
 *  Trim the move list to eliminate those moves that hang   *
 *  the king and are illegal.  This also culls any non-     *
 *  drawing moves when we are in the swindle-mode situation *
 *  and want to do a normal search but only on moves that   *
 *  preserve the draw.                                      *
 *                                                          *
 ************************************************************
 */
  for (; n_root_moves; n_root_moves--)
    if (sort_value[n_root_moves - 1] > -3000000)
      break;
  if (sort_value[0] > 1000000)
    sort_value[0] -= 2000000;
/*
 ************************************************************
 *                                                          *
 *  Debugging output to dump root move list and the stuff   *
 *  used to sort them, for testing and debugging.           *
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
 *  Check to see if we are in the special mode where moves  *
 *  need to be searched because of missing EGTBs.  This is  *
 *  sorto fo a hack that handles the case where we have an  *
 *  EGTB file like KRPKR, but we don't have the files for   *
 *  promotions for the pawn.  The program would avoid any   *
 *  pawn promotion since it likely could not see the mate,  *
 *  because the EGTB position does have a mate score.  In   *
 *  this case, we turn EGTBs off for this search so that we *
 *  can see a reason to promote the pawn and make progress  *
 *  rather than just sitting on our pawn advantage.         *
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
 *  Copy the root moves into the root_move structure array  *
 *  for use by NextRootMove().                              *
 *                                                          *
 ************************************************************
 */
  for (i = 0; i < n_root_moves; i++) {
    root_moves[i].move = rmoves[i];
    root_moves[i].status = 4;
    root_moves[i].bm_age = 0;
  }
  root_moves[0].status = 0;
  return;
}
