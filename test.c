#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "chess.h"
#include "data.h"

/* last modified 11/20/01 */
/*
********************************************************************************
*                                                                              *
*   Test() is used to test the program against a suite of test positions to    *
*   measure its performance on a particular machine, or to evaluate its skill  *
*   after modifying it in some way.                                            *
*                                                                              *
*   the test is initiated by using the "test <filename>" command to read in    *
*   the suite of problems from file <filename>.  the format of this file is    *
*   as follows:                                                                *
*                                                                              *
*   setboard <forsythe-string>:  this sets the board position using the usual  *
*   forsythe notation (see module SetBoard() in setc for a full ex-      *
*   planation of the syntax).                                                  *
*                                                                              *
*   solution <move1> <move2> ... <moven>:  this provides a solution move (or   *
*   set of solution moves if more than one is correct).  if the search finds   *
*   one of these moves, then the prblem is counted as correct, otherwise it    *
*   is counted wrong.                                                          *
*                                                                              *
*   after reading these two lines, the program then searches to whatever time  *
*   or depth limit has been set, when it reaches the end-of-file condition or  *
*   when it reads a record containing the string "end" it then displays the    *
*   number correct and the number missed.                                      *
*                                                                              *
********************************************************************************
*/
void Test(char *filename) {
  FILE *test_input;
  int i, move, right=0, wrong=0, correct;
  int time=0, len;
  BITBOARD nodes=0;
  char *eof;
  float avg_depth=0.0;
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
  Print(4095,"\n");
  eof=fgets(buffer,512,test_input);
  if (strstr(buffer,"title"));
  else {
    fclose(test_input);
    TestEPD(filename);
    return;
  }
  
  test_mode=1;
  if (book_file) {
    fclose(book_file);
    fclose(books_file);
    book_file=0;
    books_file=0;
  }
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
    else if (strcmp(args[0],"solution")) {
      Option(tree);
    }
    else {
      number_of_solutions=0;
      solution_type=0;
      Print(4095,"solution ");
      for (i=1;i<nargs;i++) {
        if (args[i][strlen(args[i])-1] == '?') {
          solution_type=1;
          args[i][strlen(args[i])-1]='\0';
        }
        else if (*(args+i)[strlen(args[i])-1] == '!') {
          solution_type=0;
          args[i][strlen(args[i])-1]='\0';
        }
        move=InputMove(tree,args[i],0,wtm,0,0);
        if (move) {
          solutions[number_of_solutions]=move;
          Print(4095,"%d. %s",(number_of_solutions++)+1,OutputMove(tree,move,0,wtm));
          if (solution_type==1) Print(4095,"? ");
          else Print(4095,"  ");
        }
        else DisplayChessBoard(stdout,tree->pos);
      }
      Print(4095,"\n");
      InitializeHashTables();
      last_pv.pathd=0;
      lazy_eval_cutoff=200;
      largest_positional_score=300;
      thinking=1;
      tree->position[1]=tree->position[0];
      (void) Iterate(wtm,think,0);
      thinking=0;
      nodes+=tree->nodes_searched;
      avg_depth+=(float)iteration_depth;
      time+=(end_time-start_time);
      correct=solution_type;
      for (i=0;i<number_of_solutions;i++) {
        if (!solution_type) {
          if (solutions[i] == tree->pv[1].path[1]) correct=1;
        }
        else
          if (solutions[i] == tree->pv[1].path[1]) correct=0;
      }
      if (correct) {
        right++;
        Print(4095,"----------------------> solution correct (%d/%d).\n",
              right,right+wrong);
      }
      else {
        wrong++;
        Print(4095,"----------------------> solution incorrect (%d/%d).\n",
              right,right+wrong);
      }
    }
  }
/*
 ----------------------------------------------------------
|                                                          |
|   now print the results.                                 |
|                                                          |
 ----------------------------------------------------------
*/
  if (right+wrong) {
    Print(4095,"\n\n\n");
    Print(4095,"test results summary:\n\n");
    Print(4095,"total positions searched..........%12d\n",right+wrong);
    Print(4095,"number right......................%12d\n",right);
    Print(4095,"number wrong......................%12d\n",wrong);
    Print(4095,"percentage right..................%12d\n",right*100/(right+wrong));
    Print(4095,"percentage wrong..................%12d\n",wrong*100/(right+wrong));
    Print(4095,"total nodes searched..............%12llu\n",nodes);
    Print(4095,"average search depth..............%12.1f\n",avg_depth/(right+wrong));
    Print(4095,"nodes per second..................%12d\n",(int) ((float) nodes/(float) time*100.0));
  }
  input_stream=stdin;
  early_exit=99;
  test_mode=0;
}
