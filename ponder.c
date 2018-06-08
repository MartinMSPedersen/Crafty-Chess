#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "types.h"
#include "function.h"
#include "data.h"

/* last modified 05/18/96 */
/*
********************************************************************************
*                                                                              *
*   Ponder() is the driver for "pondering" (thinking on the opponent's time.)  *
*   its operation is simple:  find a predicted move by (a) taking the second   *
*   move from the principal variation, or (b) call lookup to see if it finds   *
*   a suggested move from the transposition table.  then, make this move and   *
*   do a search from the resulting position.  while pondering, one of three    *
*   things can happen:  (1) a move is entered, and it matches the predicted    *
*   move.  we then switch from pondering to thinking and search as normal;     *
*   (2) a move is entered, but it does not match the predicted move.  we then  *
*   abort the search, unmake the pondered move, and then restart with the move *
*   entered.  (3) a command is entered.  if it is a simple command, it can be  *
*   done without aborting the search or losing time.  if not, we abort the     *
*   search, execute the command, and then attempt to restart pondering if the  *
*   command didn't make that impossible.                                       *
*                                                                              *
********************************************************************************
*/
int Ponder(int wtm)
{
  int dummy=0, i;
  BITBOARD target;

/*
 ----------------------------------------------------------
|                                                          |
|   if we don't have a predicted move to ponder, try two   |
|   sources:  (1) look up the current position in the      |
|   transposition table and see if it has a suggested best |
|   move;  (2) do a short tree search to calculate a move  |
|   that we should ponder.                                 |
|                                                          |
 ----------------------------------------------------------
*/
  ponder_completed=0;
  if (!ponder_move) {
    (void) LookUp(0,0,wtm,&dummy,dummy,dummy);
    if (hash_move[0]) ponder_move=hash_move[0];
  }
  if (!ponder_move) {
    TimeSet(puzzle);
    if (time_limit < 3) return(0);
    puzzling=1;
    position[1]=position[0];
    Print(2,"              puzzling over a move to ponder.\n");
    last_pv.path_length=0;
    last_pv.path_iteration_depth=0;
    for (i=0;i<MAXPLY;i++) {
      killer_move[i][0]=0;
      killer_move[i][1]=0;
      killer_move_count[i][0]=0;
      killer_move_count[i][1]=0;
    }
    (void) Iterate(wtm,puzzle);
    for (i=0;i<MAXPLY;i++) {
      killer_move[i][0]=0;
      killer_move[i][1]=0;
      killer_move_count[i][0]=0;
      killer_move_count[i][1]=0;
    }
    puzzling=0;
    if (pv[0].path_length) ponder_move=pv[0].path[1];
    if (!ponder_move) return(0);
    for (i=1;i<pv[0].path_length-1;i++) last_pv.path[i]=pv[0].path[i+1];
    last_pv.path_length=pv[0].path_length-1;
    last_pv.path_iteration_depth=0;
    if (!ValidMove(1,wtm,ponder_move)) {
      printf("puzzle returned an illegal move!\n");
      DisplayChessMove("move= ",ponder_move);
      ponder_move=0;
      return(0);
    }
  }
  if (!ponder_move) {
    strcpy(hint,"none");
    return(0);
  }
/*
 ----------------------------------------------------------
|                                                          |
|   display the move we are going to "ponder".             |
|                                                          |
 ----------------------------------------------------------
*/
  if (wtm)
    Print(2,"White(%d): %s [pondering]\n",
          move_number,OutputMove(&ponder_move,0,wtm));
  else
    Print(2,"Black(%d): %s [pondering]\n",
          move_number,OutputMove(&ponder_move,0,wtm));
  sprintf(hint,"%s",OutputMove(&ponder_move,0,wtm));
  target=(wtm) ? Compl(WhitePieces) : Compl(BlackPieces);
  num_ponder_moves=GenerateMoves(0, 1, wtm, target, 1, ponder_moves)-ponder_moves;
/*
 ----------------------------------------------------------
|                                                          |
|   now, perform an iterated search, but with the special  |
|   "pondering" flag set which changes the time controls   |
|   since there is no need to stop searching until the     |
|   opponent makes a move.                                 |
|                                                          |
 ----------------------------------------------------------
*/
  MakeMove(0,ponder_move,wtm);
  last_opponent_move=ponder_move;
  if (ChangeSide(wtm))
    *repetition_head_w++=HashKey;
  else
    *repetition_head_b++=HashKey;
  if (RepetitionDraw(wtm)) Print(0,"game is a draw by repetition\n");
  if (whisper) strcpy(whisper_text,"n/a");
  pondering=1;
  (void) Iterate(ChangeSide(wtm),think);
  pondering=0;
  if (!abort_search) ponder_completed=1;
  if (ChangeSide(wtm))
    repetition_head_w--;
  else
    repetition_head_b--;
  last_opponent_move=0;
  UnMakeMove(0,ponder_move,wtm);
/*
 ----------------------------------------------------------
|                                                          |
|   search completed.  there are two possible reasons for  |
|   us to get here.  (1) the opponent made the predicted   |
|   move and we have used enough time on the search, or;   |
|   the operator typed in a command (or not-predicted      |
|   move) that requires the search to abort and restart.   |
|                                                          |
 ----------------------------------------------------------
*/
  if (made_predicted_move) return(1);
  return(0);
}
