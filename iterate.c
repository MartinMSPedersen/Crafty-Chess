#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "chess.h"
#include "data.h"

#define EARLY_EXIT 2  /* correct 2 iterations causes an exit */

/* last modified 11/20/96 */
/*
********************************************************************************
*                                                                              *
*   Iterate() is the routine used to drive the iterated search.  it repeatedly *
*   calls search, incrementing the search depth after each call, until either  *
*   time is exhausted or the maximum set search depth is reached.              *
*                                                                              *
********************************************************************************
*/
int Iterate(int wtm, int search_type)
{
  int *mvp;
  char buffer[500], *buffp, *bufftemp;
  int i, value=0, time_used;
  int twtm, used_w, used_b;
  int cpu_start, cpu_end;
  int correct, correct_count, material=0;

/*
 ----------------------------------------------------------
|                                                          |
|   initialize.                                            |
|                                                          |
 ----------------------------------------------------------
*/
  correct_count=0;
  burp=15*100;
  transposition_id=(++transposition_id)&3;
  time_abort=0;
  abort_search=0;
  program_start_time=GetTime(time_type);
  start_time=GetTime(time_type);
  cpu_start=GetTime(cpu);
  cpu_percent=0;
  elapsed_start=GetTime(elapsed);
  nodes_searched=0;
  q_nodes_searched=0;
  next_time_check=nodes_between_time_checks;
  evaluations=0;
#if !defined(FAST)
  transposition_hashes=0;
  pawn_hashes=0;
#endif
  tb_probes=0;
  tb_probes_successful=0;
  check_extensions_done=0;
  recapture_extensions_done=0;
  passed_pawn_extensions_done=0;
  one_reply_extensions_done=0;
  root_wtm=wtm;
  root_white_pieces=TotalWhitePieces;
  root_white_pawns=TotalWhitePawns;
  root_black_pieces=TotalBlackPieces;
  root_black_pawns=TotalBlackPawns;
  PreEvaluate(wtm);
/*
 ----------------------------------------------------------
|                                                          |
|  produce the root move list, which is ordered and kept   |
|  for the duration of this search (the order may change   |
|  as new best moves are backed up to the root of course.) |
|  if there are no legal moves, it is either mate or draw  |
|  depending on whether the side to move is in check or    |
|  not (mate or stalemate.)                                |
|                                                          |
 ----------------------------------------------------------
*/
  if (search_type != booking) RootMoveList(wtm);
  if (last[0] == last[1]) {
    program_end_time=GetTime(time_type);
    pv[0].path_length=0;
    if (Check(wtm)) {
      root_value=-MATE;
      last_search_value=-(MATE-1);
    }
    else {
      root_value=DrawScore();
      last_search_value=DrawScore();
    }
    Print(2,"              depth   time   score    variation\n");
    Print(0,"                                      (no moves)\n");
    nodes_searched=1;
    q_nodes_searched=0;
    return(root_value);
  }
  if (last[0] == (last[1]-1) && search_type != booking) {
    program_end_time=GetTime(time_type);
    pv[0].path_length=1;
    pv[0].path_hashed=0;
    pv[0].path_iteration_depth=0;
    pv[0].path[1]=*last[0];
    nodes_searched=1;
    q_nodes_searched=0;
    Print(2,"              depth   time   score    variation\n");
    if (analyze_mode && xboard) {
      Print(0,"                       (only move)   ");
      Print(0," %s\n",OutputMove(last[0],1,wtm));
    }
    else {
      Print(2,"                       (only move)   ");
      Print(2," %s\n",OutputMove(last[0],1,wtm));
    }
    return(0);
  }
/*
 ----------------------------------------------------------
|                                                          |
|   now set the search time and iteratively call Search()  |
|   to analyze the position deeper and deeper.  note that  |
|   Search() is called with an alpha/beta window roughly   |
|   1/3 of a pawn on either side of the score last         |
|   returned by search.  also, after the first root move   |
|   is searched, this window is collapsed to n and n+1     |
|   (where n is the value for the first root move.)  often |
|   a better move is found, which causes search to return  |
|   <beta> as the score.  we then relax beta depending on  |
|   its value:  if beta = alpha+1, set beta to alpha+1/3   |
|   of a pawn;  if beta = alpha+1/3 pawn, then set beta to |
|   + infinity.                                            |
|                                                          |
 ----------------------------------------------------------
*/

  TimeSet(search_type);
  iteration_depth=1;
  if (last_pv.path_iteration_depth > 1)
    iteration_depth=last_pv.path_iteration_depth+1;
  Print(2,"              depth   time   score    variation (%d)\n",
        iteration_depth);
  book_move=0;
  if ((search_type==booking) || !Book(wtm)) {
    program_start_time=GetTime(time_type);
    start_time=GetTime(time_type);
    cpu_start=GetTime(cpu);
    elapsed_start=GetTime(elapsed);
/*
 ----------------------------------------------------------
|                                                          |
|   now install the old PV into the hash table so that     |
|   these moves will be followed first.                    |
|                                                          |
 ----------------------------------------------------------
*/
    if (iteration_depth > 1) {
      twtm=wtm;
      pv[0]=last_pv;
      for (i=1;i<=last_pv.path_length;i++) {
        pv[i]=pv[i-1];
        StorePV(i, twtm);
        MakeMove(i,last_pv.path[i],twtm);
        twtm=ChangeSide(twtm);
      }
      for (i=last_pv.path_length;i>0;i--) {
        twtm=ChangeSide(twtm);
        UnMakeMove(i,last_pv.path[i],twtm);
      }
      root_alpha=last_value-400;
      root_value=last_value-400;
      root_beta=last_value+400;
    }
    else {
      root_alpha=-MATE-1;
      root_value=-MATE-1;
      root_beta=MATE+1;
    }
    for (;iteration_depth<=60;iteration_depth++) {
      if (trace_level) {
        printf("==================================\n");
        printf("=      search iteration %2d       =\n",iteration_depth);
        printf("==================================\n");
      }
      for (mvp=last[0];mvp<last[1];mvp++)
        searched_this_root_move[mvp-last[0]]=0;
      search_failed_high=0;
      search_failed_low=0;
      if (nodes_searched) {
        nodes_between_time_checks=nodes_per_second/
                                  (Max(q_nodes_searched/nodes_searched,1)+1);
        nodes_between_time_checks=Min(nodes_between_time_checks,200000);
        if (time_limit > 3000) nodes_between_time_checks*=3;
        else if (time_limit > 200) nodes_between_time_checks=nodes_between_time_checks;
        else if (time_limit > 50) nodes_between_time_checks/=20;
        else nodes_between_time_checks/=100;
        nodes_between_time_checks=Max(nodes_between_time_checks,2000);
      }
      while (!time_abort && !abort_search) {
        value=SearchRoot(root_alpha, root_beta, wtm,
                         iteration_depth*INCREMENT_PLY, 1);
        if (value >= root_beta) {
          search_failed_high=1;
          root_alpha=root_beta-1;
          root_value=root_alpha;
          root_beta=MATE+1;
          searched_this_root_move[0]=0;
        }
        else if (value <= root_alpha) {
          if (!search_failed_high) {
            for (mvp=last[0];mvp<last[1];mvp++)
              searched_this_root_move[mvp-last[0]]=0;
            search_failed_low=1;
            root_alpha=-MATE-1;
            root_value=root_alpha;
            easy_move=0;
            if (((nodes_searched+q_nodes_searched) > noise_level) && 
                (!time_abort && !abort_search)) {
              Print(2,"               %2i   %s      --   ",iteration_depth,
                    DisplayTime(GetTime(time_type)-start_time));
              Print(2," %s\n",OutputMove(last[0],1,wtm));
            }
          }
          else break;
        }
        else
          break;
      }
      if ((root_value > root_alpha) && (root_value < root_beta)) 
        last_search_value=root_value;
/*
 ----------------------------------------------------------
|                                                          |
|   if we are running a test suite, check to see if we can |
|   exit the search.  this happens when N successive       |
|   iterations produce the correct solution.  N is set by  |
|   the #define EARLY_EXIT above.                          |
|                                                          |
 ----------------------------------------------------------
*/
      correct=solution_type;
      for (i=0;i<number_of_solutions;i++) {
        if (!solution_type) {
          if (solutions[i] == pv[1].path[1]) correct=1;
        }
        else
          if (solutions[i] == pv[1].path[1]) correct=0;
      }
      if (correct) correct_count++;
      else correct_count=0;
/*
 ----------------------------------------------------------
|                                                          |
|   if the search terminated normally, then dump the PV    |
|   and search statistics (we don't do this if the search  |
|   aborts because the opponent doesn't make the predicted |
|   move...)                                               |
|                                                          |
 ----------------------------------------------------------
*/
      twtm=wtm;
      end_time=GetTime(time_type);
      if (value != -(MATE-1)) {
        buffer[0]=0;
        for (i=1;i<=pv[1].path_length;i++) {
          pv[i+1]=pv[i];
          if (!time_abort && !abort_search && 
              (nodes_searched+q_nodes_searched > noise_level ||
               value > MATE-100 ||
               correct_count >= EARLY_EXIT)) {
            sprintf(buffer+strlen(buffer)," %s",OutputMove(&pv[1].path[i],i,twtm));
          }
          if(!time_abort && !abort_search) StorePV(i, twtm);
          MakeMove(i,pv[1].path[i],twtm);
          material=Material/PAWN_VALUE;
          twtm=ChangeSide(twtm);
        }
        for (i=pv[1].path_length;i>0;i--) {
          twtm=ChangeSide(twtm);
          UnMakeMove(i,pv[1].path[i],twtm);
        }
      }
      if (pv[1].path_hashed == 1)
        sprintf(buffer+strlen(buffer)," <HT>");
      else if(pv[1].path_hashed == 2) 
        sprintf(buffer+strlen(buffer)," <EGTB>");
      if (!time_abort && !abort_search && 
          (nodes_searched+q_nodes_searched > noise_level ||
           correct_count >= EARLY_EXIT ||
           value > MATE-100 || pv[1].path_hashed==2)) {
        Whisper(5,iteration_depth,end_time-start_time,whisper_value,
                nodes_searched+q_nodes_searched,0,buffer);
        Print(3,"               %2i-> %s%s   ",iteration_depth,
                  DisplayTime(end_time-start_time),DisplayEvaluation(value));
        buffp=buffer;
        do {
          if ((int) strlen(buffp) > 34) 
            bufftemp=strchr(buffp+34,' ');
          else 
            bufftemp=0;
          if (bufftemp) *bufftemp=0;
          Print(2,"%s\n",buffp);
          buffp=bufftemp+1;
          if (bufftemp) 
            Print(2,"                                      ");
        } while(bufftemp);
      }
      root_alpha=value-400;
      root_value=root_alpha;
      root_beta=value+400;
      if ((iteration_depth > 1) && (value > MATE-100) &&
          (value > last_mate_score)) break;
      if ((iteration_depth >= search_depth) && search_depth) break;
      if (time_abort || abort_search) break;
      end_time=GetTime(time_type)-start_time;
      if (thinking && (end_time >= time_limit)) break;
      if (correct_count >= EARLY_EXIT) break;
    }
/*
 ----------------------------------------------------------
|                                                          |
|   PV done, now display statistics, depending on the      |
|   verbosity level set.                                   |
|                                                          |
 ----------------------------------------------------------
*/
    used_w=0;
    used_b=0;
#if !defined(FAST)
    for (i=0;i<hash_table_size;i++) {
      if (Shiftr((trans_ref_ba+i)->word1,62) == transposition_id) used_b++;
      if (Shiftr((trans_ref_wa+i)->word1,62) == transposition_id) used_w++;
    }
#endif
    end_time=GetTime(time_type);
    time_used=(end_time-start_time);
    if (time_used < 10) time_used=10;
    cpu_end=GetTime(cpu)-cpu_start;
    cpu_end=(cpu_end > 0) ? cpu_end : 1;
    elapsed_end=GetTime(elapsed)-elapsed_start;
    if (elapsed_end) cpu_percent=Min(100*cpu_end/elapsed_end,100);
    else cpu_percent=100;
    if (time_used > 100)
      nodes_per_second=((BITBOARD) nodes_searched +
                        (BITBOARD) q_nodes_searched)*100/(BITBOARD) cpu_end;
    evaluations=(evaluations) ? evaluations : 1;
    if ((!abort_search || time_abort) && !puzzling) {
      if (ChangeSide(wtm)) material=-material;
      Print(4,"              time:%s  cpu:%d%%  mat:%d",
            DisplayTime(end_time-start_time), cpu_percent, material); 
      Print(4,"  n:%u", nodes_searched+q_nodes_searched);
      Print(4,"  nps:%d\n", nodes_per_second);
      Print(5,"              ext-> checks:%d recaps:%d pawns:%d 1rep:%d\n",
            check_extensions_done, recapture_extensions_done,
            passed_pawn_extensions_done,one_reply_extensions_done);
      Print(6,"              nodes  full:%u  quiescence:%u  evals:%u\n", 
             nodes_searched, q_nodes_searched, evaluations);
      Print(6,"              endgame tablebase-> probes done: %d  successful: %d\n",
            tb_probes, tb_probes_successful);
#if !defined(FAST)
      Print(6,"              hashing-> trans/ref:%d%%  pawn:%d%%\n",
            100*transposition_hashes/(nodes_searched+q_nodes_searched),
            100*pawn_hashes/evaluations);
      Print(6,"              hashing-> used:w%d%% b%d%%\n",
            used_w*100/(hash_table_size+1), used_b*100/(hash_table_size+1));
#endif
    }
  }
  else {
    root_value=0;
    last_search_value=0;
    book_move=1;
  }
  program_end_time=GetTime(time_type);
  pv[0]=pv[1];
  return(last_search_value);
}
