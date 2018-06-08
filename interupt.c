#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "chess.h"
#include "data.h"

/* last modified 10/18/99 */
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
void Interrupt(int ply) {
  int temp, i, left=0, readstat, result, time_used;
  int save_move_number;
  TREE * const tree=local[0];
  static int busy=0;
/*
 ----------------------------------------------------------
|                                                          |
|   if another thread is already reading the input, this   |
|   thread can exit now.                                   |
|                                                          |
 ----------------------------------------------------------
*/
#if defined(SMP)
  Lock(lock_io);
  if (busy==1) {
    Unlock(lock_io);
    return;
  }
  busy=1;
  Unlock(lock_io);
#endif
  if (abort_search) {
    busy=0;
    return;
  }
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
  else do {
    readstat=Read(0,buffer);
    if (readstat <= 0) break;
    nargs=ReadParse(buffer,args," 	;");
    if (nargs == 0) {
      Print(128,"ok.\n");
      break;
    }
    if (strcmp(args[0],".")) {
      save_move_number=move_number;
      if (!wtm) move_number--;
      if (wtm)
        Print(128,"White(%d): %s\n",move_number,buffer);
      else
        Print(128,"Black(%d): %s\n",move_number,buffer);
      move_number=save_move_number;
    }
/*
 ----------------------------------------------------------
|                                                          |
|   "." command displays status of current search.         |
|                                                          |
 ----------------------------------------------------------
*/
    if (!strcmp(args[0],".")) {
      if (xboard) {
        end_time=ReadClock(time_type);
        time_used=(end_time-start_time);
        printf("stat01: %d ",time_used);
        printf(BMF " ",tree->nodes_searched);
        printf("%d ",iteration_depth); 
        for (i=0;i<n_root_moves;i++)
          if (!(root_moves[i].status&128)) left++;
        printf("%d %d\n",left,n_root_moves);
        fflush(stdout);
        break;
      }
      else {
        end_time=ReadClock(time_type);
        time_used=(end_time-start_time);
        printf("time:%s ",DisplayTime(time_used));
        printf("nodes:" BMF "\n",tree->nodes_searched);
        DisplayTreeState(local[0],1,0,ply);
      }
    }
/*
 ----------------------------------------------------------
|                                                          |
|   "mn" command is used to set the move number to a       |
|   specific value...                                      |
|                                                          |
 ----------------------------------------------------------
*/
    else if (!strcmp("mn",args[0])) {
      if (nargs == 2) {
        move_number=atoi(args[1]);
        Print(128,"move number set to %d\n",move_number);
      }
    }
/*
 ----------------------------------------------------------
|                                                          |
|   "?" command says "move now!"                           |
|                                                          |
 ----------------------------------------------------------
*/
    else if (!strcmp(args[0],"?")) {
      if (thinking) {
        time_abort=1;
        abort_search=1;
      }
    }
/*
 ----------------------------------------------------------
|                                                          |
|   next see if Option() recognizes this as a command.     |
|                                                          |
 ----------------------------------------------------------
*/
    else {
      save_move_number=move_number;
      if (!analyze_mode && !wtm) move_number--;
      result=Option(tree);
      move_number=save_move_number;
      if (result >= 2) {
        if (thinking && result!=3)
          Print(128,"command not legal now.\n");
        else {
          abort_search=1;
          input_status=2;
          break;
        }
      }
      else if ((result != 1) && analyze_mode) {
        abort_search=1;
        input_status=2;
        break;
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
          nargs=ReadParse(buffer,args," 	;");
          temp=InputMove(tree,args[0],0,ChangeSide(root_wtm),1,1);
          if (temp) {
            if (auto232) {
              const char *mv=OutputMoveICS(temp);
              DelayTime(auto232_delay);
              if (!wtm) fprintf(auto_file,"\t");
              fprintf(auto_file, " %c%c-%c%c", mv[0], mv[1], mv[2], mv[3]);
              if ((mv[4] != ' ') && (mv[4] != 0))
                fprintf(auto_file, "/%c", mv[4]);
              fprintf(auto_file, "\n");
              fflush(auto_file);
            }
            if ((From(temp) == From(ponder_move)) &&
                (To(temp) == To(ponder_move)) &&
                (Piece(temp) == Piece(ponder_move)) &&
                (Captured(temp) == Captured(ponder_move)) &&
                (Promote(temp) == Promote(ponder_move))) {
              predicted++;
              input_status=1;
              pondering=0;
              thinking=1;
              opponent_end_time=ReadClock(elapsed);
              program_start_time=ReadClock(time_type);
              Print(128,"predicted move made.\n");
            }
            else {
              input_status=2;
              abort_search=1;
              break;
            }
          }
          else if (!strcmp(args[0],"go") || !strcmp(args[0],"move") ||
                   !strcmp(args[0],"SP")) {
            abort_search=1;
            break;
          }
          else Print(4095,"Illegal move: %s\n", args[0]);
        }
        else Print(4095,"unrecognized/illegal command: %s\n", args[0]);
      }
    }
  } while (1);
  if (log_file) fflush(log_file);
  busy=0;
}
