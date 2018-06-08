#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "chess.h"
#include "data.h"

/* last modified 09/10/02 */
/*
 *******************************************************************************
 *                                                                             *
 *   EVTest() is used to test the program against a suite of test positions to *
 *   measure its performance on a particular machine, or to evaluate its skill *
 *   after modifying it in some way.                                           *
 *                                                                             *
 *   the test is initiated by using the "evtest <filename>" command to read in *
 *   the suite of problems from file <filename>.  the format of this file is   *
 *   as follows:                                                               *
 *                                                                             *
 *   setboard <forsythe-string>:  this sets the board position using the usual *
 *   forsythe notation (see module SetBoard() in setc for a full explanation   *
 *   planation of the syntax).                                                 *
 *                                                                             *
 *   after reading this position, the program then calls Evaluate() to produce *
 *   a positional evaluation, along with any debug output from Evaluate(), and *
 *   then goes on to the next position.                                        *
 *                                                                             *
 *******************************************************************************
 */
void EVTest(char *filename)
{
  FILE *test_input;
  char *eof;
  TREE *const tree = local[0];

/*
 ************************************************************
 *                                                          *
 *   read in the position and then the solutions.  then do  *
 *   a call to Evaluate() which will normally display the   *
 *   debugging stuff that is enabled.                       *
 *                                                          *
 ************************************************************
 */
  if (!(test_input = fopen(filename, "r"))) {
    printf("file %s does not exist.\n", filename);
    return;
  }
  test_mode = 1;
  while (1) {
    eof = fgets(buffer, 512, test_input);
    if (eof) {
      char *delim;

      delim = strchr(buffer, '\n');
      if (delim)
        *delim = 0;
      delim = strchr(buffer, '\r');
      if (delim)
        *delim = ' ';
    } else
      break;
    nargs = ReadParse(buffer, args, " ;");
    if (!strcmp(args[0], "end"))
      break;
    else {
      int s, id;

      for (id=2;id < nargs; id++)
        if (!strcmp(args[id],"id")) break;
      if (id >= nargs) id=0;
      SetBoard(&tree->position[0], nargs, args, 0);
      WhiteCastle(0) = 0;
      BlackCastle(0) = 0;
      root_wtm = wtm;
      PreEvaluate(tree, wtm);
      tree->pawn_score.key = 0;
/*
      DisplayChessBoard(stdout, tree->pos);
*/
      s = Evaluate(tree, 0, 1, -999999, 999999);
      if (id) Print(4095, "id=%s  ",args[id+1]);
      Print(4095, "score=%d\n", s);
    }
  }
  input_stream = stdin;
  test_mode = 0;
}
