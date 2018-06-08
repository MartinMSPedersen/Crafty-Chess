#include "chess.h"
#include "data.h"
/* last modified 01/18/09 */
/*
 *******************************************************************************
 *                                                                             *
 *   Analyze() is used to handle the "analyze" command.  This mode basically   *
 *   puts Crafty into a "permanent pondering" state, where it reads a move from*
 *   the input stream, and then "ponders" for the opposite side.  Whenever a   *
 *   move is entered, Crafty reads this move, updates the game board, and then *
 *   starts "pondering" for the other side.                                    *
 *                                                                             *
 *   The purpose of this mode is to force Crafty to follow along in a game,    *
 *   providing analysis continually for the side on move until a move is       *
 *   entered, advancing the game to the next position.                         *
 *                                                                             *
 *******************************************************************************
 */
void Analyze() {
  int i, move, back_number, readstat = 1;
  TREE *const tree = block[0];

/*
 ************************************************************
 *                                                          *
 *  Initialize.                                             *
 *                                                          *
 ************************************************************
 */
  int save_swindle_mode = swindle_mode;

  swindle_mode = 0;
  ponder_move = 0;
  analyze_mode = 1;
  if (!xboard)
    display_options |= 1 + 2 + 4;
  printf("Analyze Mode: type \"exit\" to terminate.\n");
/*
 ************************************************************
 *                                                          *
 *  Now loop waiting on input, searching the current        *
 *  position continually until a move comes in.             *
 *                                                          *
 ************************************************************
 */
  do {
    do {
      last_pv.pathd = 0;
      last_pv.pathl = 0;
      input_status = 0;
      pondering = 1;
      tree->position[1] = tree->position[0];
      (void) Iterate(wtm, think, 0);
      pondering = 0;
      if (book_move)
        moves_out_of_book = 0;
      if (!xboard) {
        if (wtm)
          printf("analyze.White(%d): ", move_number);
        else
          printf("analyze.Black(%d): ", move_number);
        fflush(stdout);
      }
/*
 ************************************************************
 *                                                          *
 *  If we get back to here, something has been typed in and *
 *  is in the command buffer normally, unless the search    *
 *  terminated naturally due to finding a mate or reaching  *
 *  the max depth allowable.                                *
 *                                                          *
 ************************************************************
 */
      if (!input_status)
        do {
          readstat = Read(1, buffer);
          if (readstat < 0)
            break;
          nargs = ReadParse(buffer, args, " 	;");
          Print(128, "%s\n", buffer);
          if (strstr(args[0], "timeleft") && !xboard) {
            if (wtm)
              printf("analyze.White(%d): ", move_number);
            else
              printf("analyze.Black(%d): ", move_number);
            fflush(stdout);
          }
        } while (strstr(args[0], "timeleft"));
      else
        nargs = ReadParse(buffer, args, " 	;");
      if (readstat < 0)
        break;
      move = 0;
      if (!strcmp(args[0], "exit"))
        break;
/*
 ************************************************************
 *                                                          *
 *  First, check for the special analyze command "back n"   *
 *  and handle it if present, otherwise try Option() to see *
 *  if it recognizes the input as a command.                *
 *                                                          *
 ************************************************************
 */
      if (OptionMatch("back", args[0])) {
        if (nargs > 1)
          back_number = atoi(args[1]);
        else
          back_number = 1;
        for (i = 0; i < back_number; i++) {
          wtm = Flip(wtm);
          if (Flip(wtm))
            move_number--;
        }
        if (move_number == 0) {
          move_number = 1;
          wtm = 1;
        }
        sprintf(buffer, "reset %d", move_number);
        (void) Option(tree);
        display = tree->pos;
      } else if (Option(tree)) {
        display = tree->pos;
      }
/*
 ************************************************************
 *                                                          *
 *  If InputMove() can recognize this as a move, make it,   *
 *  swap sides, and return to the top of the loop to call   *
 *  search from this new position.                          *
 *                                                          *
 ************************************************************
 */
      else if ((move = InputMove(tree, buffer, 0, wtm, 1, 0))) {
        char *outmove = OutputMove(tree, move, 0, wtm);

        if (history_file) {
          fseek(history_file, ((move_number - 1) * 2 + 1 - wtm) * 10,
              SEEK_SET);
          fprintf(history_file, "%9s\n", outmove);
        }
        if (wtm)
          Print(128, "White(%d): ", move_number);
        else
          Print(128, "Black(%d): ", move_number);
        Print(128, "%s\n", outmove);
        if (speech) {
          char announce[64];

          strcpy(announce, SPEAK);
          strcat(announce, outmove);
          system(announce);
        }
        MakeMoveRoot(tree, move, wtm);
        display = tree->pos;
        last_mate_score = 0;
        if (log_file)
          DisplayChessBoard(log_file, tree->pos);
      }
/*
 ************************************************************
 *                                                          *
 *  If Option() didn't handle the input, then it is illegal *
 *  and should be reported to the user.                     *
 *                                                          *
 ************************************************************
 */
      else {
        pondering = 0;
        if (Option(tree) == 0)
          printf("illegal move: %s\n", buffer);
        pondering = 1;
        display = tree->pos;
      }
    } while (!move);
    if (readstat < 0 || !strcmp(args[0], "exit"))
      break;
    wtm = Flip(wtm);
    if (wtm)
      move_number++;
  } while (1);
  analyze_mode = 0;
  printf("analyze complete.\n");
  pondering = 0;
  swindle_mode = save_swindle_mode;
}
