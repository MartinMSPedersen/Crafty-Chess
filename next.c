#include <stdio.h>
#include <stdlib.h>
#include "chess.h"
#include "data.h"

/* last modified 09/27/96 */
/*
********************************************************************************
*                                                                              *
*   NextMove() is used to select the next move from the current move list.     *
*                                                                              *
********************************************************************************
*/
int NextMove(int depth, int ply, int wtm)
{
  register int *bestp, *movep, *sortv, temp;
  char remaining_moves[10];
  register int history_value, bestval, done, index;
/*
 ----------------------------------------------------------
|                                                          |
|   if in check, use NextEvasion() instead as it is more   |
|   intelligent about the moves it produces when the king  |
|   is in check.                                           |
|                                                          |
 ----------------------------------------------------------
*/
  switch (next_status[ply].phase) {
/*
 ----------------------------------------------------------
|                                                          |
|   first, try the transposition table move (which will be |
|   the principal variation move as we first move down the |
|   tree).                                                 |
|                                                          |
 ----------------------------------------------------------
*/
  case HASH_MOVE:
    next_status[ply].phase=GENERATE_CAPTURE_MOVES;
    if (hash_move[ply]) {
      current_move[ply]=hash_move[ply];
      if (ValidMove(ply,wtm,current_move[ply])) return(HASH_MOVE);
      else Print(1,"bad move from hash table, ply=%d\n",ply);
    }
/*
 ----------------------------------------------------------
|                                                          |
|   generate captures and sort them based on (a) the value |
|   of the captured piece - the value of the capturing     |
|   piece if this is > 0; or, (b) the value returned by    |
|   Swap().                                                |
|                                                          |
 ----------------------------------------------------------
*/
  case GENERATE_CAPTURE_MOVES:
    next_status[ply].phase=CAPTURE_MOVES;
    last[ply]=GenerateMoves(ply, depth, wtm,
                            (wtm) ? BlackPieces : WhitePieces,
                            1, last[ply-1]);
    next_status[ply].remaining=0;
    if (hash_move[ply]) {
      for (movep=last[ply-1],sortv=sort_value;movep<last[ply];movep++,sortv++)
        if (*movep == hash_move[ply]) {
          *sortv=-999999;
          *movep=0;
          hash_move[ply]=0;
        }
        else {
          if (piece_values[Piece(*movep)] < piece_values[Captured(*movep)]) {
            *sortv=piece_values[Captured(*movep)]-piece_values[Piece(*movep)];
            next_status[ply].remaining++;
          }
          else {
            *sortv=Swap(From(*movep),To(*movep),wtm);
            if (*sortv >= 0)  next_status[ply].remaining++;
          }
        }
    }
    else {
      for (movep=last[ply-1],sortv=sort_value;movep<last[ply];movep++,sortv++)
        if (piece_values[Piece(*movep)] < piece_values[Captured(*movep)]) {
          *sortv=piece_values[Captured(*movep)]-piece_values[Piece(*movep)];
          next_status[ply].remaining++;
        }
        else {
          *sortv=Swap(From(*movep),To(*movep),wtm);
          if (*sortv >= 0)  next_status[ply].remaining++;
        }
    }
/*
 ----------------------------------------------------------
|                                                          |
|   don't disdain the lowly bubble sort here.  the list of |
|   captures is always short, and experiments with other   |
|   algorithms are always slightly slower.                 |
|                                                          |
 ----------------------------------------------------------
*/
    do {
      done=1;
      for (movep=last[ply-1],sortv=sort_value;movep<last[ply]-1;movep++,sortv++)
        if (*sortv < *(sortv+1)) {
          temp=*sortv;
          *sortv=*(sortv+1);
          *(sortv+1)=temp;
          temp=*movep;
          *movep=*(movep+1);
          *(movep+1)=temp;
          done=0;
        }
    } while(!done);
    next_status[ply].last=last[ply-1];
/*
 ----------------------------------------------------------
|                                                          |
|   try the captures moves, which are in order based on    |
|   the expected gain of material.  captures that lose     |
|   material have been excluded from this phase.           |
|                                                          |
 ----------------------------------------------------------
*/
  case CAPTURE_MOVES:
    if (next_status[ply].remaining) {
      current_move[ply]=*(next_status[ply].last);
      *next_status[ply].last++=0;
      next_status[ply].remaining--;
      if (!next_status[ply].remaining) next_status[ply].phase=KILLER_MOVE_1;
      return(CAPTURE_MOVES);
    }
    next_status[ply].phase=KILLER_MOVE_1;
/*
 ----------------------------------------------------------
|                                                          |
|   now, try the killer moves.  this phase tries the two   |
|   killers for the current ply without generating moves,  |
|   which saves time if a cutoff occurs.                   |
|                                                          |
 ----------------------------------------------------------
*/
  case KILLER_MOVE_1:
    if ((hash_move[ply] != killer_move[ply][0]) &&
        ValidMove(ply,wtm,killer_move[ply][0])) {
      current_move[ply]=killer_move[ply][0];
      next_status[ply].phase=KILLER_MOVE_2;
      return(KILLER_MOVE_1);
    }
  case KILLER_MOVE_2:
    if ((hash_move[ply] != killer_move[ply][1]) &&
        ValidMove(ply,wtm,killer_move[ply][1])) {
      current_move[ply]=killer_move[ply][1];
      next_status[ply].phase=GENERATE_ALL_MOVES;
      return(KILLER_MOVE_2);
    }
    next_status[ply].phase=GENERATE_ALL_MOVES;
/*
 ----------------------------------------------------------
|                                                          |
|   now, generate all non-capturing moves.                 |
|                                                          |
 ----------------------------------------------------------
*/
  case GENERATE_ALL_MOVES:
    last[ply]=GenerateMoves(ply, depth, wtm, Compl(Occupied),
                            0, last[ply]);
    next_status[ply].phase=HISTORY_MOVES_1;
/*
 ----------------------------------------------------------
|                                                          |
|   now, try the history moves.  this phase takes the      |
|   complete move list, and passes over them in a classic  |
|   selection-sort, choosing the move with the highest     |
|   history score.  this phase is only done one time, as   |
|   it also purges the hash and killer moves from the list.|
|                                                          |
 ----------------------------------------------------------
*/
  case HISTORY_MOVES_1:
    next_status[ply].remaining=1;
    next_status[ply].phase=HISTORY_MOVES_2;
    bestval=0;
    bestp=0;
    for (movep=last[ply-1];movep<last[ply];movep++)
      if (*movep && ((*movep == hash_move[ply]) || (*movep == killer_move[ply][0]) ||
          (*movep == killer_move[ply][1]))) *movep=0;
      else {
        index=*movep&4095;
        history_value= (wtm) ? history_w[index] : history_b[index];
        if (history_value > bestval) {
          bestval=history_value;
          bestp=movep;
        }
      }
    if (bestp) {
      current_move[ply]=*bestp;
      *bestp=0;
      return(HISTORY_MOVES_1);
    }
    goto remaining_moves;
/*
 ----------------------------------------------------------
|                                                          |
|   now, continue with the history moves, but since one    |
|   pass has been made over the complete move list, there  |
|   are no hash/killer moves left in the list, so the test |
|   for these can be avoided.                              |
|                                                          |
 ----------------------------------------------------------
*/
  case HISTORY_MOVES_2:
    bestval=0;
    bestp=0;
    for (movep=last[ply-1];movep<last[ply];movep++)
      if (*movep) {
        index=*movep&4095;
        history_value= (wtm) ? history_w[index] : history_b[index];
        if (history_value > bestval) {
          bestval=history_value;
          bestp=movep;
        }
      }
    if (bestval) {
      current_move[ply]=*bestp;
      *bestp=0;
      next_status[ply].remaining++;
      if (next_status[ply].remaining > 3) {
        next_status[ply].phase=REMAINING_MOVES;
        next_status[ply].last=last[ply-1];
      }
      return(HISTORY_MOVES_2);
    }
  remaining_moves:
    next_status[ply].phase=REMAINING_MOVES;
    next_status[ply].last=last[ply-1];
/*
 ----------------------------------------------------------
|                                                          |
|   now try the rest of the set of moves.                  |
|                                                          |
 ----------------------------------------------------------
*/
  case REMAINING_MOVES:
    for (;next_status[ply].last<last[ply];next_status[ply].last++)
      if (*next_status[ply].last) {
        current_move[ply]=*next_status[ply].last;
        *next_status[ply].last++=0;
        return(REMAINING_MOVES);
      }
    return(NONE);
/*
 ----------------------------------------------------------
|                                                          |
|   for the moves at the root of the tree, the list has    |
|   already been generated and sorted.  on entry, test     |
|   the searched_this_root_move[] array and then take the  |
|   moves in the order they appear in the move list.       |
|                                                          |
 ----------------------------------------------------------
*/
  case ROOT_MOVES:
    time_abort+=TimeCheck(1);
    if (time_abort) {
      abort_search=1;
      return(NONE);
    }
    done=0;
    for (movep=last[0];movep<last[1];movep++)
      if (searched_this_root_move[movep-last[0]]) done++;
    if ((done==1) && searched_this_root_move[0] &&
        (root_value==root_alpha) && !search_failed_high) return(NONE);

    for (movep=last[0];movep<last[1];movep++)
      if (!searched_this_root_move[movep-last[0]]) {
        if (search_move) {
          if (search_move > 0) {
            if(*movep != search_move) {
              searched_this_root_move[movep-last[0]]=1;
              continue;
            }
          }
          else {
            if(*movep == -search_move) {
              searched_this_root_move[movep-last[0]]=1;
              continue;
            }
          }
        }
        current_move[1]=*movep;
        searched_this_root_move[movep-last[0]]=1;
        if ((nodes_searched > noise_level) && (verbosity_level >= 9)) {
          sprintf(remaining_moves,"%d/%d",movep-last[ply-1]+1,last[ply]-last[ply-1]);
          end_time=GetTime(time_type);
          printf("               %2i   %s %7s   ",iteration_depth,
                 DisplayTime(end_time-start_time),remaining_moves);
          printf(" %s      \r",OutputMove(&current_move[1],1,wtm));
          fflush(stdout);
        }
        return(ROOT_MOVES);
      }
      return(NONE);
  
  default:
    printf("oops!  next_status.phase is bad! [normal %d]\n",next_status[ply].phase);
    return(NONE);
  }
}
