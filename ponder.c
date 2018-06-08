#include "chess.h"
#include "data.h"
/* last modified 01/18/09 */
/*
 *******************************************************************************
 *                                                                             *
 *   Ponder() is the driver for "pondering" (thinking on the opponent's time.) *
 *   its operation is simple:  Find a predicted move by (a) taking the second  *
 *   move from the principal variation, or (b) call lookup to see if it finds  *
 *   a suggested move from the transposition table.  Then, make this move and  *
 *   do a search from the resulting position.  While pondering, one of three   *
 *   things can happen:  (1) A move is entered, and it matches the predicted   *
 *   move.  We then switch from pondering to thinking and search as normal;    *
 *   (2) A move is entered, but it does not match the predicted move.  We then *
 *   abort the search, unmake the pondered move, and then restart with the move*
 *   entered.  (3) A command is entered.  If it is a simple command, it can be *
 *   done without aborting the search or losing time.  If not, we abort the    *
 *   search, execute the command, and then attempt to restart pondering if the *
 *   command didn't make that impossible.                                      *
 *                                                                             *
 *******************************************************************************
 */
int Ponder(int wtm) {
  int dalpha = -999999, dbeta = 999999, i, *n_ponder_moves, *mv;
  int save_move_number, tlom, value;
  TREE *const tree = block[0];

/*
 ************************************************************
 *                                                          *
 *   First, let's check to see if pondering is allowed, or  *
 *   if we should avoid pondering on this move since it is  *
 *   the first move of a game, or if the game is over, or   *
 *   "force" mode is active, or there is input in the queue *
 *   that needs to be read and processed.                   *
 *                                                          *
 ************************************************************
 */
  if (!ponder || force || over || CheckInput())
    return (0);
  save_move_number = move_number;
/*
 ************************************************************
 *                                                          *
 *   If we don't have a predicted move to ponder, try two   *
 *   sources:  (1) Look up the current position in the      *
 *   transposition table and see if it has a suggested best *
 *   move;  (2) Do a short tree search to calculate a move  *
 *   that we should ponder.                                 *
 *                                                          *
 ************************************************************
 */
  strcpy(hint, "none");
  if (ponder_move) {
    if (!VerifyMove(tree, 1, wtm, ponder_move)) {
      ponder_move = 0;
      Print(4095, "ERROR.  ponder_move is illegal (1).\n");
      Print(4095, "ERROR.  PV pathl=%d\n", last_pv.pathl);
      Print(4095, "ERROR.  move=%d  %x\n", ponder_move, ponder_move);
    }
  }
  if (!ponder_move) {
    (void) HashProbe(tree, 0, 0, wtm, dalpha, dbeta, &value);
    if (tree->hash_move[0])
      ponder_move = tree->hash_move[0];
    if (ponder_move) {
      if (!VerifyMove(tree, 1, wtm, ponder_move)) {
        Print(4095, "ERROR.  ponder_move is illegal (2).\n");
        Print(4095, "ERROR.  move=%d  %x\n", ponder_move, ponder_move);
        ponder_move = 0;
      }
    }
  }
  if (!ponder_move) {
    TimeSet(tree, puzzle);
    if (time_limit < 20)
      return (0);
    puzzling = 1;
    tree->position[1] = tree->position[0];
    Print(128, "              puzzling over a move to ponder.\n");
    last_pv.pathl = 0;
    last_pv.pathd = 0;
    for (i = 0; i < MAXPLY; i++) {
      tree->killers[i].move1 = 0;
      tree->killers[i].move2 = 0;
    }
    (void) Iterate(wtm, puzzle, 0);
    for (i = 0; i < MAXPLY; i++) {
      tree->killers[i].move1 = 0;
      tree->killers[i].move2 = 0;
    }
    puzzling = 0;
    if (tree->pv[0].pathl)
      ponder_move = tree->pv[0].path[1];
    if (!ponder_move)
      return (0);
    for (i = 1; i < (int) tree->pv[0].pathl; i++)
      last_pv.path[i] = tree->pv[0].path[i + 1];
    last_pv.pathl = tree->pv[0].pathl - 1;
    last_pv.pathd = 0;
    if (!VerifyMove(tree, 1, wtm, ponder_move)) {
      ponder_move = 0;
      Print(4095, "ERROR.  ponder_move is illegal (3).\n");
      Print(4095, "ERROR.  PV pathl=%d\n", last_pv.pathl);
      return (0);
    }
  }
/*
 ************************************************************
 *                                                          *
 *   Display the move we are going to "ponder".             *
 *                                                          *
 ************************************************************
 */
  if (wtm)
    Print(128, "White(%d): %s [pondering]\n", move_number, OutputMove(tree,
            ponder_move, 0, wtm));
  else
    Print(128, "Black(%d): %s [pondering]\n", move_number, OutputMove(tree,
            ponder_move, 0, wtm));
  sprintf(hint, "%s", OutputMove(tree, ponder_move, 0, wtm));
  if (post)
    printf("Hint: %s\n", hint);
/*
 ************************************************************
 *                                                          *
 *   Set the ponder move list and eliminate illegal moves.  *
 *                                                          *
 ************************************************************
 */
  n_ponder_moves = GenerateCaptures(tree, 0, wtm, ponder_moves);
  num_ponder_moves =
      GenerateNoncaptures(tree, 0, wtm, n_ponder_moves) - ponder_moves;
  for (mv = ponder_moves; mv < ponder_moves + num_ponder_moves; mv++) {
    MakeMove(tree, 0, *mv, wtm);
    if (Check(wtm)) {
      UnmakeMove(tree, 0, *mv, wtm);
      *mv = 0;
    } else
      UnmakeMove(tree, 0, *mv, wtm);
  }
/*
 ************************************************************
 *                                                          *
 *   Now, perform an iterated search, but with the special  *
 *   "pondering" flag set which changes the time controls   *
 *   since there is no need to stop searching until the     *
 *   opponent makes a move.                                 *
 *                                                          *
 ************************************************************
 */
  MakeMove(tree, 0, ponder_move, wtm);
  tlom = last_opponent_move;
  last_opponent_move = ponder_move;
  if (kibitz)
    strcpy(kibitz_text, "n/a");
  thinking = 0;
  pondering = 1;
  if (!wtm)
    move_number++;
  ponder_value = Iterate(Flip(wtm), think, 0);
  move_number = save_move_number;
  pondering = 0;
  thinking = 0;
  last_opponent_move = tlom;
  UnmakeMove(tree, 0, ponder_move, wtm);
/*
 ************************************************************
 *                                                          *
 *   Search completed. the possible return values are:      *
 *                                                          *
 *   (0) No pondering was done, period.                     *
 *                                                          *
 *   (1) Pondering was done, opponent made the predicted    *
 *       move, and we searched until time ran out in a      *
 *       normal manner.                                     *
 *                                                          *
 *   (2) Pondering was done, but the ponder search          *
 *       terminated due to either finding a mate, or the    *
 *       maximum search depth was reached.  The result of   *
 *       this ponder search are valid, but only if the      *
 *       opponent makes the correct (predicted) move.       *
 *                                                          *
 *   (3) Pondering was done, but the opponent either made   *
 *       a different move, or entered a command that has to *
 *       interrupt the pondering search before the command  *
 *       (or move) can be processed.  This forces Main() to *
 *       avoid reading in a move/command since one has been *
 *       read into the command buffer already.              *
 *                                                          *
 ************************************************************
 */
  if (input_status == 1)
    return (1);
  if (input_status == 2)
    return (3);
  return (2);
}
