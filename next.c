#include <stdio.h>
#include <stdlib.h>
#include "types.h"
#include "function.h"
#include "data.h"
/*
********************************************************************************
*                                                                              *
*   Next_Move() is used to select the next move from the current move list.    *
*                                                                              *
********************************************************************************
*/
int Next_Move(int depth, int ply, int wtm)
{
  BITBOARD target;
  int *mv, *mvp, tempm;
  char remain[10];
  int history_value, bestval, done, i, index, ndone, temp;
/*
 ----------------------------------------------------------
|                                                          |
|   if in check, use Next_Evasion() instead as it is more  |
|   intelligent about the moves it produces when the king  |
|   is in check.                                           |
|                                                          |
 ----------------------------------------------------------
*/
  if ((ply>1) && in_check[ply]) return(Next_Evasion(ply,wtm));
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
      if (Valid_Move(ply,wtm,current_move[ply]))
        return(hash_normal_move);
      else
        Print(1,"bad move from hash table, ply=%d\n",ply);
    }
/*
 ----------------------------------------------------------
|                                                          |
|   try the capture moves next.  this phase first uses     |
|   Generate_Moves() with a target of the opponent's       |
|   occupied squares.  after generating the captures, we   |
|   can use Swap() to evaluate the relative gain or loss   |
|   incurred by the capture.                               |
|                                                          |
|   the type of capture ordering is determined by the      |
|   setting of the mvv_lva_ordering variable, which can be |
|   set by the mvv_lva=n (n=0 or 1) command.  if zero, we  |
|   use normal SEE ordering, if non-zero, we use MVV/LVA   |
|   ordering.                                              |
|                                                          |
 ----------------------------------------------------------
*/
  case capture_moves:
    if (next_status[ply].whats_generated != captures_generated) {
      if (wtm)
        target=Black_Pieces(ply);
      else
        target=White_Pieces(ply);
      next_status[ply].to=target;
      last[ply]=Generate_Moves(ply, depth, wtm, target, 1, first[ply]);
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
          if (!mvv_lva_ordering) {
            sort_value[mvp-first[ply]]=
              Swap(ply,From(*mvp),To(*mvp),wtm);
            if (sort_value[mvp-first[ply]] >= 0) next_status[ply].remaining++;
          }
          else {
            sort_value[mvp-first[ply]]=
              piece_values[Captured(*mvp)]+
              aggressor_order[Piece(*mvp)];
            next_status[ply].remaining++;
          }
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
      if (!next_status[ply].remaining) 
        next_status[ply].phase=killer_moves;
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
          Valid_Move(ply,wtm,killer_move[ply][0])) {
        current_move[ply]=killer_move[ply][0];
        next_status[ply].remaining=1;
        return(killer_moves);
      }
      if ((hash_move[ply] != killer_move[ply][1]) &&
          Valid_Move(ply,wtm,killer_move[ply][1])) {
        current_move[ply]=killer_move[ply][1];
        next_status[ply].phase=history_moves;
        return(killer_moves);
      }
    }
    else {
      if ((hash_move[ply] != killer_move[ply][1]) &&
          Valid_Move(ply,wtm,killer_move[ply][1])) {
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
      if (wtm)
        target=And(Compl(White_Pieces(ply)),
                   Compl(next_status[ply].to));
      else
        target=And(Compl(Black_Pieces(ply)),
                   Compl(next_status[ply].to));
      next_status[ply].last=first[ply];
      last[ply]=Generate_Moves(ply, depth, wtm, target, 0, last[ply]);
      next_status[ply].whats_generated=everything;
    }
    bestval=0;
    for (mv=first[ply];mv<last[ply];mv++) {
      if ((*mv == hash_move[ply]) || (*mv == killer_move[ply][0]) ||
          (*mv == killer_move[ply][1])) *mv=0;
      if (*mv) {
        index=*mv&4095;
        if (wtm)
          history_value=history_w[index];
        else
          history_value=history_b[index];
        if (history_value > bestval) {
          bestval=history_value;
          mvp=mv;
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
    break;
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
        (root_value==root_alpha) && !failed_high) return(none);
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
          end_time=Get_Time(time_type);
          printf("               %2i   %s %7s   ",iteration_depth,
                 Display_Time(end_time-start_time),remain);
          printf(" %s      \r",Output_Move(&current_move[1],1,wtm));
          fflush(stdout);
        }
        return(root_moves);
      }
      break;
/*
 ----------------------------------------------------------
|                                                          |
|   this handles the special way we treat mates.  If a     |
|   move leads to mate, then we don't look at any other    |
|   moves since the mate is forced and one is as good as   |
|   another since the iterated search will find the        |
|   shortest mate first anyway.                            |
|                                                          |
 ----------------------------------------------------------
*/
  case all_done:
    return(none);
  
  default:
    printf("oops!  next_status.phase is bad! [normal %d]\n",
           next_status[ply].phase);
  }
/*
 ----------------------------------------------------------
|                                                          |
|   done, return (none) since nothing was found.           |
|                                                          |
 ----------------------------------------------------------
*/
  return(none);
}
