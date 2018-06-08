#include "chess.h"
#include "data.h"
#include "epdglue.h"
/* last modified 04/21/15 */
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
  TREE *const tree = block[0];
  unsigned mvp, *lastm, rmoves[256];
  int temp, value;
#if !defined(NOEGTB)
  int tb_value;
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
  for (mvp = 0; mvp < n_root_moves; mvp++)
    root_moves[mvp].move = rmoves[mvp];
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
  abort_search = 0;
  for (mvp = 0; mvp < n_root_moves; mvp++) {
    value = -4000000;
#if defined(TRACE)
    if (trace_level >= 1) {
      tree->curmv[1] = root_moves[mvp].move;
      Trace(tree, 1, 0, wtm, -MATE, MATE, "RootMoves()", serial, HASH,
          mvp + 1);
    }
#endif
    MakeMove(tree, 1, wtm, root_moves[mvp].move);
    tree->nodes_searched++;
    if (!Check(wtm))
      do {
        tree->curmv[1] = root_moves[mvp].move;
#if !defined(NOEGTB)
        if (TotalAllPieces <= EGTBlimit && EGTB_draw &&
            Castle(1, white) + Castle(1, black) == 0) {
          temp = EGTBProbe(tree, 2, Flip(wtm), &tb_value);
          if (temp && tb_value != DrawScore(Flip(wtm)))
            break;
        }
#endif
        value = -Quiesce(tree, 2, Flip(wtm), -MATE, MATE, 0);
/*
 ************************************************************
 *                                                          *
 *  Add in a bonus if this move is part of the previous     *
 *  principal variation.  It was good in the search, we     *
 *  should try it first now.                                *
 *                                                          *
 ************************************************************
 */
        if ((Piece(root_moves[mvp].move) == Piece(last_pv.path[1]))
            && (From(root_moves[mvp].move) == From(last_pv.path[1]))
            && (To(root_moves[mvp].move) == To(last_pv.path[1]))
            && (Captured(root_moves[mvp].move) == Captured(last_pv.path[1]))
            && (Promote(root_moves[mvp].move) == Promote(last_pv.path[1])))
          value += 2000000;
/*
 ************************************************************
 *                                                          *
 *  Fudge the score for promotions so that promotion to a   *
 *  queen is tried first.                                   *
 *                                                          *
 ************************************************************
 */
        if (Promote(root_moves[mvp].move) &&
            (Promote(root_moves[mvp].move) != queen))
          value -= 50;
      } while (0);
    root_moves[mvp].path = tree->pv[1];
    root_moves[mvp].path.pathv = value;
    root_moves[mvp].status = 0;
    root_moves[mvp].bm_age = 0;
    UnmakeMove(tree, 1, wtm, root_moves[mvp].move);
  }
/*
 ************************************************************
 *                                                          *
 *  Sort the moves into order based on the scores returned  *
 *  by Quiesce() which includes evaluation + captures.      *
 *                                                          *
 ************************************************************
 */
  SortRootMoves();
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
    if (root_moves[n_root_moves - 1].path.pathv > -3000000)
      break;
  if (root_moves[0].path.pathv > 1000000)
    root_moves[0].path.pathv -= 2000000;
/*
 ************************************************************
 *                                                          *
 *  Debugging output to dump root move list and the stuff   *
 *  used to sort them, for testing and debugging.           *
 *                                                          *
 ************************************************************
 */
  if (display_options & 128) {
    Print(128, "%d moves at root\n", n_root_moves);
    Print(128, "     score    move/pv\n");
    for (mvp = 0; mvp < n_root_moves; mvp++)
      Print(128, "%10s    %s\n", DisplayEvaluation(root_moves[mvp].path.pathv,
              wtm), DisplayPath(tree, wtm, &root_moves[mvp].path));
  }
/*
 ************************************************************
 *                                                          *
 *  Finally, before we return the list of moves, we need to *
 *  set the values to an impossible negative value so that  *
 *  as we sort the root move list after fail highs and lows *
 *  the un-searched moves won't pop to the top of the list. *
 *                                                          *
 ************************************************************
 */
  for (mvp = 1; mvp < n_root_moves; mvp++)
    root_moves[mvp].path.pathv = -99999999;
  return;
}
