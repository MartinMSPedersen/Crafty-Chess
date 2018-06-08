#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "chess.h"
#include "data.h"

/* last modified 04/30/97 */
/*
********************************************************************************
*                                                                              *
*   EVTest() is used to test the program against a suite of test positions to  *
*   measure its performance on a particular machine, or to evaluate its skill  *
*   after modifying it in some way.                                            *
*                                                                              *
*   the test is initiated by using the "evtest <filename>" command to read in  *
*   the suite of problems from file <filename>.  the format of this file is    *
*   as follows:                                                                *
*                                                                              *
*   setboard <forsythe-string>:  this sets the board position using the usual  *
*   forsythe notation (see module SetBoard() in setc for a full explanation    *
*   planation of the syntax).                                                  *
*                                                                              *
*   after reading this position, the program then calls Evaluate() to produce  *
*   a positional evaluation, along with any debug output from Evaluate(), and  *
*   then goes on to the next position.                                         *
*                                                                              *
********************************************************************************
*/
void EVTest(char *filename)
{
  FILE *test_input;
  int i, len;
  char *eof;
  TREE * const tree=local[0];
/*
 ----------------------------------------------------------
|                                                          |
|   read in the position and then the solutions.  after    |
|   executing a search to find the best move (according    |
|   to the program, anyway) compare it against the list    |
|   of solutions and count it right or wrong.              |
|                                                          |
 ----------------------------------------------------------
*/
  if (!(test_input=fopen(filename,"r"))) {
    printf("file %s does not exist.\n",filename);
    return;
  }
  test_mode=1;
  while (1) {
    eof=fgets(buffer,512,test_input);
    if (eof) {
      char *delim;
      delim=strchr(buffer,'\n');
      if (delim) *delim=0;
      delim=strchr(buffer,'\r');
      if (delim) *delim=' ';
    }
    else break;
    nargs=ReadParse(buffer,args," ;");
    if (!strcmp(args[0],"end")) break;
    else if (!strcmp(args[0],"title")) {
      Print(4095,"======================================================================\n");
      Print(4095,"! ");
      len=0;
      for (i=1;i<nargs;i++) {
        Print(4095,"%s ",args[i]);
        len+=strlen(args[i])+1;
        if (len > 65) break;
      }
      for (i=len;i<67;i++) printf(" ");
      Print(4095,"!\n");
      Print(4095,"======================================================================\n");
    }
    else if (!strcmp(args[0],"setboard")) {
      SetBoard(nargs-1,args+1,0);
      PreEvaluate(tree,wtm);
      DisplayChessBoard(stdout,tree->pos);
      Print(4095,"Evaluation=%d\n",Evaluate(tree,0,wtm,-999999,999999));
    }
  }
  input_stream=stdin;
  test_mode=0;
}
