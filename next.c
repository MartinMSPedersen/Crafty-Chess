#include <stdio.h>
#include <stdlib.h>
#include "types.h"
#include "function.h"
#include "data.h"
/*
********************************************************************************
*                                                                              *
*   NextMove() is used to select the next move from the current move list.     *
*                                                                              *
********************************************************************************
*/
int NextMove(int depth, int ply, int wtm)
{
  register BITBOARD target;
  register int *mv, *mvp, tempm;
  char remain[10];
  register int history_value, bestval, done, i, index, ndone, temp;
/*
 ----------------------------------------------------------
|                                                          |
|   if in check, use NextEvasion() instead as it is more   |
|   intelligent about the moves it produces when the king  |
|   is in check.                                           |
|                                                          |
 ----------------------------------------------------------
*/
  if ((ply>1) && in_check[ply]) return(NextEvasion(ply,wtm));
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
  case hash_normal_move:
    next_status[ply].phase=capture_moves;
    if (hash_move[ply]) {
      current_move[ply]=hash_move[ply];
      if (ValidMove(ply,wtm,current_move[ply])) return(hash_normal_move);
      else {
        Print(1,"bad move from hash table, ply=%d\n",ply);
        DisplayChessBoard(log_file,position[ply]);
        fprintf(log_file,"bad move: %s\n",OutputMove(&current_move[ply],ply,wtm));
/*
        DisplayChessMove("bad move=",current_move[ply]);
*/
      }
    }
/*
 ----------------------------------------------------------
|                                                          |
|   try the capture moves next.  this phase first uses     |
|   GenerateMoves() with a target of the opponent's        |
|   occupied squares.  after generating the captures, we   |
|   can use Swap() to evaluate the relative gain or loss   |
|   incurred by the capture.                               |
|                                                          |
 ----------------------------------------------------------
*/
  case capture_moves:
    if (next_status[ply].whats_generated != captures_generated) {
      if (wtm) target=BlackPieces(ply);
      else target=WhitePieces(ply);
      next_status[ply].to=target;
      last[ply]=GenerateMoves(ply, depth, wtm, target, 1, first[ply]);
      next_status[ply].whats_generated=captures_generated;
/*
 --------------------------------------------------
|                                                  |
|   moves are generated, now sort best captures    |
|   first.                                         |
|                                                  |
 --------------------------------------------------
*/
      next_status[ply].remaining=0;
      for (mvp=first[ply];mvp<last[ply];mvp++) {
        if (hash_move[ply] == *mvp) {
          *mvp=0;
          sort_value[mvp-first[ply]]=-999999;
        }
        else {
          if (piece_values[Piece(*mvp)] < piece_values[Captured(*mvp)])
          sort_value[mvp-first[ply]]=
            piece_values[Captured(*mvp)]-piece_values[Piece(*mvp)];
          else
            sort_value[mvp-first[ply]]=Swap(ply,From(*mvp),To(*mvp),wtm);
          if (sort_value[mvp-first[ply]] >= 0) next_status[ply].remaining++;
        }
      }
      do {
        done=1;
        for (i=0;i<last[ply]-first[ply]-1;i++) {
          if (sort_value[i] < sort_value[i+1]) {
            temp=sort_value[i];
            sort_value[i]=sort_value[i+1];
            sort_value[i+1]=temp;
            tempm=*(first[ply]+i);
            *(first[ply]+i)=*(first[ply]+i+1);
            *(first[ply]+i+1)=tempm;
            done=0;
          }
        }
      } while(!done);
      next_status[ply].last=first[ply];
    }
/*
 --------------------------------------------------
|                                                  |
|   after the first call, simply drop to here to   |
|   return the next "good" capture move.           |
|                                                  |
 --------------------------------------------------
*/
    if (next_status[ply].remaining) {
      current_move[ply]=*(next_status[ply].last);
      next_status[ply].current=next_status[ply].last;
      *next_status[ply].last++=0;
      next_status[ply].remaining--;
      if (!next_status[ply].remaining) next_status[ply].phase=killer_moves;
      return(capture_moves);
    }
    next_status[ply].phase=killer_moves;
/*
 ----------------------------------------------------------
|                                                          |
|   now, try the killer moves.  this phase tries the two   |
|   killers for the current ply without generating moves,  |
|   which saves time if a cutoff occurs.                   |
|                                                          |
 ----------------------------------------------------------
*/
  case killer_moves:
    if (next_status[ply].remaining==0) {
      if ((hash_move[ply] != killer_move[ply][0]) &&
          ValidMove(ply,wtm,killer_move[ply][0])) {
        current_move[ply]=killer_move[ply][0];
        next_status[ply].remaining=1;
        return(killer_moves);
      }
      if ((hash_move[ply] != killer_move[ply][1]) &&
          ValidMove(ply,wtm,killer_move[ply][1])) {
        current_move[ply]=killer_move[ply][1];
        next_status[ply].phase=history_moves;
        return(killer_moves);
      }
    }
    else {
      if ((hash_move[ply] != killer_move[ply][1]) &&
          ValidMove(ply,wtm,killer_move[ply][1])) {
        current_move[ply]=killer_move[ply][1];
        next_status[ply].phase=history_moves;
        return(killer_moves);
      }
    }
    next_status[ply].phase=history_moves;
/*
 ----------------------------------------------------------
|                                                          |
|   now, try the history moves.  this phase generates the  |
|   complete move list, and then shifts those moves with   |
|   a non-zero history value to the top of the list.       |
|   these moves are then sorted and tried in order before  |
|   trying the rest.                                       |
|                                                          |
 ----------------------------------------------------------
*/
  case history_moves:
    if (next_status[ply].whats_generated != everything) {
      if (wtm) target=And(Compl(WhitePieces(ply)),Compl(next_status[ply].to));
      else target=And(Compl(BlackPieces(ply)),Compl(next_status[ply].to));
      last[ply]=GenerateMoves(ply, depth, wtm, target, 0, last[ply]);
      next_status[ply].whats_generated=everything;
      bestval=0;
      for (mv=first[ply];mv<last[ply];mv++) {
        if ((*mv == hash_move[ply]) || (*mv == killer_move[ply][0]) ||
            (*mv == killer_move[ply][1])) *mv=0;
        if (*mv) {
          index=*mv&4095;
          if (wtm) history_value=history_w[index];
          else history_value=history_b[index];
          if (history_value > bestval) {
            bestval=history_value;
            mvp=mv;
          }
        }
      }
    }
    else {
      bestval=0;
      for (mv=first[ply];mv<last[ply];mv++) {
        if (*mv) {
          index=*mv&4095;
          if (wtm) history_value=history_w[index];
          else history_value=history_b[index];
          if (history_value > bestval) {
            bestval=history_value;
            mvp=mv;
          }
        }
      }
    }
    if (bestval) {
      current_move[ply]=*mvp;
      next_status[ply].current=mvp;
      *mvp=0;
      return(history_moves);
    }
    next_status[ply].phase=remaining_moves;
    next_status[ply].last=first[ply];
/*
 ----------------------------------------------------------
|                                                          |
|   now try the rest of the set of moves.                  |
|                                                          |
 ----------------------------------------------------------
*/
  case remaining_moves:
    for (;next_status[ply].last<last[ply];next_status[ply].last++) {
      if (*next_status[ply].last) {
        current_move[ply]=*next_status[ply].last;
        next_status[ply].current=next_status[ply].last;
        *next_status[ply].last++=0;
        return(remaining_moves);
      }
    }
    return(none);
/*
 ----------------------------------------------------------
|                                                          |
|   for the moves at the root of the tree, the list has    |
|   already been generated and sorted.  on entry, reset    |
|   the searched_this_root_move[] array and then take the  |
|   moves in the order they appear in the move list.       |
|                                                          |
 ----------------------------------------------------------
*/
  case root_moves:
    ndone=0;
    for (mvp=first[1];mvp<last[1];mvp++)
      if (searched_this_root_move[mvp-first[1]]) ndone++;
    if ((ndone==1) && searched_this_root_move[0] &&
        (root_value==root_alpha) && !search_failed_high) return(none);
    for (mvp=first[1];mvp<last[1];mvp++)
      if (!searched_this_root_move[mvp-first[1]]) {
        if (search_move)
          if(*mvp != search_move) {
            searched_this_root_move[mvp-first[1]]=1;
            continue;
          }
        current_move[1]=*mvp;
        next_status[ply].current=mvp;
        searched_this_root_move[mvp-first[1]]=1;
        if ((nodes_searched > noise_level) && (verbosity_level >= 9)) {
          sprintf(remain,"%ld/%ld",mvp-first[ply]+1,last[ply]-first[ply]);
          end_time=GetTime(time_type);
          printf("               %2i   %s %7s   ",iteration_depth,
                 DisplayTime(end_time-start_time),remain);
          printf(" %s      \r",OutputMove(&current_move[1],1,wtm));
          fflush(stdout);
        }
        return(root_moves);
      }
      return(none);
  
  default:
    printf("oops!  next_status.phase is bad! [normal %d]\n",next_status[ply].phase);
    return(none);
  }
}
