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
void EVTest(char *filename) {
  FILE *test_input;
  int i, len;
  char *eof;
  TREE * const tree=local[0];
/*
 ----------------------------------------------------------
|                                                          |
|   read in the position and then the solutions.  then do  |
|   a call to Evaluate() which will normally display the   |
|   debugging stuff that is enabled.                       |
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
      int s1,s2,s3,s4;
      SetBoard(nargs-1,args+1,0);
      WhiteCastle(0)=0;
      BlackCastle(0)=0;
      root_wtm=wtm;
      PreEvaluate(tree,wtm);
      tree->pawn_score.key=0;
      DisplayChessBoard(stdout,tree->pos);
      s1=Evaluate(tree,0,wtm,-999999,999999);
      strcpy(buffer,"flop");
      Option(tree);
      root_wtm=wtm;
      PreEvaluate(tree,wtm);
      tree->pawn_score.key=0;
      s2=Evaluate(tree,0,wtm,-999999,999999);
      strcpy(buffer,"flip");
      Option(tree);
      root_wtm=wtm;
      PreEvaluate(tree,ChangeSide(wtm));
      tree->pawn_score.key=0;
      s3=Evaluate(tree,0,ChangeSide(wtm),-999999,999999);
      strcpy(buffer,"flop");
      Option(tree);
      root_wtm=wtm;
      PreEvaluate(tree,ChangeSide(wtm));
      tree->pawn_score.key=0;
      s4=Evaluate(tree,0,ChangeSide(wtm),-999999,999999);
      if (s1 != s2) Print(4095,"test 1 failed, s1=%d s2=%d\n",s1,s2);
      if (s2 != s3) Print(4095,"test 2 failed, s2=%d s3=%d\n",s2,s3);
      if (s3 != s4) Print(4095,"test 3 failed, s3=%d s4=%d\n",s3,s4);
      if (s4 != s1) Print(4095,"test 4 failed, s4=%d s1=%d\n",s4,s1);
    }
  }
  input_stream=stdin;
  test_mode=0;
}
