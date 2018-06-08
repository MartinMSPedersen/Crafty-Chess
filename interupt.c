#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "types.h"
#include "function.h"
#include "data.h"
/*
********************************************************************************
*                                                                              *
*   Interrupt() is used to read in a move when the operator types something    *
*   while a search is in progress (during pondering as one example.)  this     *
*   routine reads in a command (move) and then makes two attempts to use this  *
*   input:  (1) call Option() to see if the command can be executed;  (2) try  *
*   Input_Move() to see if this input is a legal move;  if so, and we are      *
*   pondering see if it matches the move we are pondering.                     *
*                                                                              *
********************************************************************************
*/
void Interrupt(int ply)
{
  int temp, *mvp;
  int i, left, result, time_used;

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
  else if (!ics) {
    do {
      scanf("%s",input);
      Print(1,"ok.\n");
      if (!strcmp(input,".")) {
        end_time=Get_Time(time_type);
        time_used=(end_time-start_time);
        printf("time:%s ",Display_Time(time_used));
        printf("nodes:%ld ",nodes_searched);
        printf("maxd:%ld\n",max_search_depth);
        for (left=0,mvp=first[1];mvp<last[1];mvp++) 
          if (!searched_this_root_move[mvp-first[1]]) left++;
        printf("%d:%d/%d  ",1,left,last[1]-first[1]);
        for (i=2;i<=ply;i++) {
          left=0;
          for (mvp=first[i];mvp<last[i];mvp++) 
            if (*mvp) left++;
          printf("%d:%d/%d  ",i,left,last[i]-first[i]);
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
            temp=Input_Move(input,0,!root_wtm,1);
            if (temp) {
              if ((From(temp) == From(ponder_move)) &&
                  (To(temp) == To(ponder_move)) &&
                  (Piece(temp) == Piece(ponder_move)) &&
                  (Captured(temp) == Captured(ponder_move)) &&
                  (Promote(temp) == Promote(ponder_move))) {
                made_predicted_move=1;
                pondering=0;
                thinking=1;
                opponent_end_time=Get_Time(elapsed);
                program_start_time=Get_Time(time_type);
              }
              else
                abort_search=1;
            }
            else
              Print(0,"illegal move.\n");
          }
          else
           Print(0,"unknown command/command not legal now.\n");
        }
      }
    } while (Check_Input());
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
  else {
    for (i=0;i<3;i++) {
      fgets(input,80,input_stream);
      input[strlen(input)-1]='\0';
      if (!strlen(input)) return;
      fprintf(log_file,"%s\n",input);
      if (input[4] == ' ') input[4]='=';
      result=Option(input);
      if (result == 2) {
        abort_search=1;
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
          temp=Input_Move_ICS(input,0,!root_wtm,1);
          if (temp) {
            if ((From(temp) == From(ponder_move)) &&
                (To(temp) == To(ponder_move)) &&
                (Piece(temp) == Piece(ponder_move)) &&
                (Captured(temp) == Captured(ponder_move)) &&
                (Promote(temp) == Promote(ponder_move))) {
              made_predicted_move=1;
              pondering=0;
              thinking=1;
              opponent_end_time=Get_Time(elapsed);
              program_start_time=Get_Time(time_type);
            }
            else
              abort_search=1;
          }
          else
            Print(0,"illegal move.\n");
        }
      }
    }
  }
}
