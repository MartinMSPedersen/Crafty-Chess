#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "types.h"
#include "function.h"
#include "data.h"

/* last modified 08/15/96 */
/*
********************************************************************************
*                                                                              *
*   Interrupt() is used to read in a move when the operator types something    *
*   while a search is in progress (during pondering as one example.)  this     *
*   routine reads in a command (move) and then makes two attempts to use this  *
*   input:  (1) call Option() to see if the command can be executed;  (2) try  *
*   InputMove() to see if this input is a legal move;  if so, and we are       *
*   pondering see if it matches the move we are pondering.                     *
*                                                                              *
********************************************************************************
*/
void Interrupt(int ply)
{
  int temp, *mvp;
  int i, left, result, time_used;
  static char save_command[64];
  int deferred=0;

/*
 ----------------------------------------------------------
|                                                          |
|   if trying to find a move to ponder, and the operator   |
|   types a command, exit a.s.a.p.                         |
|                                                          |
 ----------------------------------------------------------
*/
  if (puzzling) 
    abort_search=1;
/*
 ----------------------------------------------------------
|                                                          |
|   first check to see if this is a command by calling     |
|   Option().  Option() will return a 0 if it didn't       |
|   recognize the command; otherwise it returns a 1 if     |
|   the command was executed, or a 2 if we need to abort   |
|   the search to execute the command.                     |
|                                                          |
 ----------------------------------------------------------
*/
  else if (!xboard && !ics) {
    do {
      scanf("%s",input);
      Print(1,"ok.\n");
      if (!strcmp(input,".")) {
        end_time=GetTime(time_type);
        time_used=(end_time-start_time);
        printf("time:%s ",DisplayTime(time_used));
        printf("nodes:%d\n",nodes_searched);
        for (left=0,mvp=last[0];mvp<last[1];mvp++) 
          if (!searched_this_root_move[mvp-last[0]]) left++;
        printf("%d:%d/%d  ",1,left,last[1]-last[0]);
        for (i=2;i<=ply;i++) {
          left=0;
          for (mvp=last[i-1];mvp<last[i];mvp++) 
            if (*mvp) left++;
          printf("%d:%d/%d  ",i,left,last[i]-last[i-1]);
          if (!(i%8)) printf("\n");
        }
        printf("\n");
      }
      else if (!strcmp(input,"?")) {
        if (thinking) {
          time_abort=1;
          abort_search=1;
        }
      }
      else {
        result=Option(input);
        if (result == 2) {
          if (thinking)
            Print(0,"command not legal now.\n");
          else {
            abort_search=1;
            analyze_move_read=1;
          }
        }
        else if ((result != 1) && analyze_mode) {
          abort_search=1;
          analyze_move_read=1;
        }
        else if (!result) {
/*
 ----------------------------------------------------------
|                                                          |
|   now, check to see if the operator typed a move.  if    |
|   so, and it matched the predicted move, switch from     |
|   pondering to thinking to start the timer.  if the      |
|   is a move, but not the predicted move, abort the       |
|   search, and start over with the right move.            |
|                                                          |
 ----------------------------------------------------------
*/
          if (pondering) {
            temp=InputMove(input,0,ChangeSide(root_wtm),1,1);
            if (temp) {
              if ((From(temp) == From(ponder_move)) &&
                  (To(temp) == To(ponder_move)) &&
                  (Piece(temp) == Piece(ponder_move)) &&
                  (Captured(temp) == Captured(ponder_move)) &&
                  (Promote(temp) == Promote(ponder_move))) {
                made_predicted_move=1;
                pondering=0;
                thinking=1;
                opponent_end_time=GetTime(elapsed);
                program_start_time=GetTime(time_type);
              }
              else
                abort_search=1;
            }
            else if (!strcmp(input,"go") || !strcmp(input,"move")) {
              abort_search=1;
            }
            else Print(0,"illegal move.\n");
          }
          else
           Print(0,"unknown command/command not legal now.\n");
        }
      }
    } while (CheckInput());
  }
/*
 ----------------------------------------------------------
|                                                          |
|   first check to see if this is a command by calling     |
|   Option().  Option() will return a 0 if it didn't       |
|   recognize the command; otherwise it returns a 1 if     |
|   the command was executed, or a 2 if we need to abort   |
|   the search to execute the command.                     |
|                                                          |
 ----------------------------------------------------------
*/
  else if (xboard) {
    for (i=0;i<3;i++) {
      fgets(input,80,input_stream);
      input[strlen(input)-1]='\0';
      if (!strlen(input)) {
        i--;
        continue;
      }
      if (log_file) fprintf(log_file,"%s\n",input);
      if (input[4] == ' ') input[4]='=';
      result=Option(input);
      if (result == 2) {
        if (thinking)
          Print(0,"command not legal now.\n");
        else {
          deferred=1;
          strcpy(save_command,input);
          abort_search=1;
          analyze_move_read=1;
        }
      }
      else if ((result != 1) && analyze_mode) {
        abort_search=1;
        analyze_move_read=1;
      }
  /*
   ----------------------------------------------------------
  |                                                          |
  |   now, check to see if the operator typed a move.  if    |
  |   so, and it matched the predicted move, switch from     |
  |   pondering to thinking to start the timer.  if the      |
  |   is a move, but not the predicted move, abort the       |
  |   search, and start over with the right move.            |
  |                                                          |
   ----------------------------------------------------------
  */
      else if (!result) {
        if (pondering) {
          temp=InputMoveICS(input,0,ChangeSide(root_wtm),1,1);
          if (temp) {
            if ((From(temp) == From(ponder_move)) &&
                (To(temp) == To(ponder_move)) &&
                (Piece(temp) == Piece(ponder_move)) &&
                (Captured(temp) == Captured(ponder_move)) &&
                (Promote(temp) == Promote(ponder_move))) {
              made_predicted_move=1;
              pondering=0;
              thinking=1;
              opponent_end_time=GetTime(elapsed);
              program_start_time=GetTime(time_type);
            }
            else
              abort_search=1;
          }
          else if (!strcmp(input,"go") || !strcmp(input,"move")) {
            abort_search=1;
          }
          else Print(0,"illegal move.\n");
        }
      }
      if (!strstr(input,"otim") && !strstr(input,"time")) break;
    }
    if (deferred) strcpy(input,save_command);
  }
  else if (ics) {
    for (i=0;i<2;i++) {
      scanf("%s",input);
      if (!strcmp(input,"?")) {
        if (thinking) {
          time_abort=1;
          abort_search=1;
        }
      }
      else {
        result=Option(input);
        if (result == 2) {
          if (thinking)
            Print(0,"command not legal now.\n");
          else {
            abort_search=1;
            analyze_move_read=1;
          }
        }
        else if ((result != 1) && analyze_mode) {
          abort_search=1;
          analyze_move_read=1;
        }
        else if (!result) {
/*
 ----------------------------------------------------------
|                                                          |
|   now, check to see if the operator typed a move.  if    |
|   so, and it matched the predicted move, switch from     |
|   pondering to thinking to start the timer.  if the      |
|   is a move, but not the predicted move, abort the       |
|   search, and start over with the right move.            |
|                                                          |
 ----------------------------------------------------------
*/
          if (pondering) {
            temp=InputMove(input,0,ChangeSide(root_wtm),1,1);
            if (temp) {
              if ((From(temp) == From(ponder_move)) &&
                  (To(temp) == To(ponder_move)) &&
                  (Piece(temp) == Piece(ponder_move)) &&
                  (Captured(temp) == Captured(ponder_move)) &&
                  (Promote(temp) == Promote(ponder_move))) {
                made_predicted_move=1;
                pondering=0;
                thinking=1;
                opponent_end_time=GetTime(elapsed);
                program_start_time=GetTime(time_type);
              }
              else
                abort_search=1;
            }
            else if (!strcmp(input,"go") || !strcmp(input,"move")) {
              abort_search=1;
            }
            else Print(0,"illegal move.\n");
          }
          else
           Print(0,"unknown command/command not legal now.\n");
        }
      }
    }
  }
}
/*
********************************************************************************
*                                                                              *
*   InterruptSignal() is used to catch SIGINT which is used by xboard when the *
*   operator wants crafty to "move right now".  if not pondering, all that's   *
*   necessary is to set the appropriate abort flag(s) and exit, resetting the  *
*   signal handler to trap the next SIGINT.                                    *
*                                                                              *
********************************************************************************
*/
void InterruptSignal(int sig_type)
{
  if (thinking) {
    time_abort=1;
    abort_search=1;
  }

  signal(SIGINT,InterruptSignal);
}
