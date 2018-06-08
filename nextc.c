#include <stdio.h>
#include <stdlib.h>
#include "types.h"
#include "function.h"
#include "data.h"
/*
********************************************************************************
*                                                                              *
*   NextCapture() is used to select the next move in the capture search.  it   *
*   also may include checking moves under certain tightly constrained rules.   *
*                                                                              *
********************************************************************************
*/
int NextCapture(int ply, int wtm, int depth)
{
  register BITBOARD target;
  register int *mvp, tempm;
  register int done, do_checks, nchecks, i, temp;

  switch (next_status[ply].phase) {
/*
 ----------------------------------------------------------
|                                                          |
|   first, try the transposition table move (which will be |
|   the principal variation move as we first move down the |
|   tree).  note that we only allow a capture or pawn pro- |
|   motion move at this point.                             |
|                                                          |
 ----------------------------------------------------------
*/
  case hash_capture_move:
    next_status[ply].phase=capture_moves;
    if (hash_move[ply]) {
      if (Captured(hash_move[ply]) || Promote(hash_move[ply])) {
        current_move[ply]=hash_move[ply];
        if (ValidMove(ply,wtm,current_move[ply])) return(hash_capture_move);
        else {
          Print(1,"bad move from hash table, ply=%d\n",ply);
          DisplayChessBoard(log_file,position[ply]);
          fprintf(log_file,"bad move: %s\n",OutputMove(&current_move[ply],ply,wtm));
/*
          DisplayChessMove("bad move=",current_move[ply]);
*/
        }
      }
      else hash_move[ply]=0;
    }
/*
 ----------------------------------------------------------
|                                                          |
|   try the capture moves next.  this phase first uses     |
|   GenerateMoves() with a target of the opponent's        |
|   occupied squares.                                      |
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
        if (*mvp == hash_move[ply]) {
          *mvp=0;
          sort_value[mvp-first[ply]]=-9999;
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
      if (!next_status[ply].remaining) next_status[ply].phase=hash_checking_move;
      return(capture_moves);
    }
    next_status[ply].phase=hash_checking_move;
/*
 ----------------------------------------------------------
|                                                          |
|   next, try the transposition table move (which will be  |
|   the principal variation move as we first move down the |
|   tree).  note that we only allow a checking move at     |
|   this point.                                            |
|                                                          |
 ----------------------------------------------------------
*/
  case hash_checking_move:
    if (!quiescence_checks || !in_check[ply-1]) return(none);
    next_status[ply].phase=checking_moves;
    do_checks=0;
    if (root_wtm == wtm) {
      for (i=2;i<ply-abs(depth)+1;i+=2) if(in_check[i]) do_checks++;
    }
    else {
      for (i=3;i<ply-abs(depth)+1;i+=2) if(in_check[i]) do_checks++;
    }
    nchecks=(abs(depth)+1)/2;
    if (!do_checks || (nchecks >= do_checks) || (nchecks > quiescence_checks)) 
      return(none);
    if (hash_move[ply]) {
      if (GiveCheck(ply,wtm,&hash_move[ply])) {
        current_move[ply]=hash_move[ply];
        if (ValidMove(ply,wtm,current_move[ply])) return(hash_checking_move);
        else {
          Print(1,"bad move from hash table, ply=%d\n",ply);
          DisplayChessBoard(log_file,position[ply]);
          fprintf(log_file,"bad move: %s\n",OutputMove(&current_move[ply],ply,wtm));
/*
          DisplayChessMove("bad move=",current_move[ply]);
*/
        }
      }
      else hash_move[ply]=0;
    }
/*
 ----------------------------------------------------------
|                                                          |
|   now try the checking moves.  we allow one check in the |
|   quiescence search for each check in the basic search,  |
|   limited by the value of "quiescence_checks" which is   |
|   normally set to "2".                                   |
|                                                          |
 ----------------------------------------------------------
*/
  case checking_moves:
    if (next_status[ply].whats_generated != everything) {
      next_status[ply].last=last[ply];
      if (wtm) target=And(Compl(WhitePieces(ply)),Compl(next_status[ply].to));
      else target=And(Compl(BlackPieces(ply)),Compl(next_status[ply].to));
      next_status[ply].last=first[ply];
      last[ply]=GenerateMoves(ply, depth, wtm, target, 0, last[ply]);
      next_status[ply].whats_generated=everything;
    }
    for (mvp=next_status[ply].last;mvp<last[ply];mvp++) {
      if (*mvp == hash_move[ply]) *mvp=0;
      else if (*mvp && GiveCheck(ply,wtm,mvp) &&
               Swap(ply,From(*mvp),To(*mvp),wtm) >= 0) break;
    }
    if (mvp < last[ply]) {
      current_move[ply]=*mvp;
      next_status[ply].current=mvp;
      next_status[ply].last=mvp+1;
      *next_status[ply].current=0;
      check_extensions_done++;
      return(checking_moves);
    }
    return(none);
  default:
    printf("oops!  next_status.phase is bad! [capture %d]\n",next_status[ply].phase);
    return(none);
  }
}
