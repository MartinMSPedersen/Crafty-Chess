#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "chess.h"
#include "data.h"

/* last modified 02/27/96 */
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
void Test(void)
{
  int i, move, right=0, wrong=0, correct;
  char command[64], nextc;
  int nodes=0;
  int time=0;
  int temp_draw_score_is_zero;
  float avg_depth=0.0;
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
  temp_draw_score_is_zero=draw_score_is_zero;
  draw_score_is_zero=1;
  if (book_file) {
    fclose(book_file);
    book_file=0;
  }
  while (1) {
    InitializeHashTables();
    last_pv.path_iteration_depth=0;
    largest_positional_score=1000;
    if (fscanf(input_stream,"%s",command) == EOF) break;
    if (!strcmp(command,"end")) break;
    else if (!strcmp(command,"title")) {
      fgets(command,80,input_stream);
      command[strlen(command)-1]='\0';
      Print(0,"=======================================================\n");
      Print(0,"!  %-50s !\n",command);
      Print(0,"=======================================================\n");
    }
    else if (!strcmp(command,"setboard")) SetBoard();
    else if (!strcmp(command,"solution")) {
      number_of_solutions=0;
      solution_type=0;
      Print(0,"solution ");
      do {
        fscanf(input_stream,"%s",command);
        Print(0,"%d. %s ",number_of_solutions+1,command);
        if (command[strlen(command)-1] == '?') {
          solution_type=1;
          command[strlen(command)-1]='\0';
        }
        else if (command[strlen(command)-1] == '!') {
          solution_type=0;
          command[strlen(command)-1]='\0';
        }
        move=InputMove(command,0,wtm,0,0);
        if (move)
          solutions[number_of_solutions++]=move;
        nextc=getc(input_stream);
      } while (nextc == ' ');
      Print(0,"\n");
      thinking=1;
      position[1]=position[0];
      (void) Iterate(wtm,think);
      thinking=0;
      nodes+=(nodes_searched+q_nodes_searched);
      avg_depth+=(float)iteration_depth;
      time+=(end_time-start_time);
      correct=solution_type;
      for (i=0;i<number_of_solutions;i++) {
        if (!solution_type) {
          if (solutions[i] == pv[1].path[1]) correct=1;
        }
        else
          if (solutions[i] == pv[1].path[1]) correct=0;
      }
      if (correct) {
        right++;
        Print(0,"----------------------> solution correct.\n");
      }
      else {
        wrong++;
        Print(0,"----------------------> solution incorrect.\n");
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
  Print(0,"\n\n\n");
  Print(0,"test results summary:\n\n");
  Print(0,"total positions searched..........%10d\n",right+wrong);
  Print(0,"number right......................%10d\n",right);
  Print(0,"number wrong......................%10d\n",wrong);
  Print(0,"percentage right..................%10d\n",right*100/(right+wrong));
  Print(0,"percentage wrong..................%10d\n",wrong*100/(right+wrong));
  Print(0,"total nodes searched..............%10d\n",nodes);
  Print(0,"average search depth..............%10.1f\n",avg_depth/(right+wrong));
  Print(0,"nodes per second..................%10d\n",(int) ((float) nodes/(float) time*100.0));
  input_stream=stdin;
  draw_score_is_zero=temp_draw_score_is_zero;
}
