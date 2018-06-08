#include <stdio.h>
#include <stdlib.h>
#include "types.h"
#include "function.h"
#include "data.h"
#include "evaluate.h"
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
  int i, more, ndone, value, time_used;
  int twtm, used_w, used_b, draft;
  int cpu_start, cpu_end;
  int elapsed_start, elapsed_end;
  int material;
  char ext_char[5] = {"-x=."};

/*
 ----------------------------------------------------------
|                                                          |
|   first run through the transposition table and set the  |
|   "old" flag, so that left-over positions will be        |
|   overwritten as needed.                                 |
|                                                          |
 ----------------------------------------------------------
*/
  for (i=0;i<hash_table_size;i++) {
    (trans_ref_b+i)->word1=Or((trans_ref_b+i)->word1,mask_1);
    (trans_ref_w+i)->word1=Or((trans_ref_w+i)->word1,mask_1);
  }
/*
 ----------------------------------------------------------
|                                                          |
|   initialize.                                            |
|                                                          |
 ----------------------------------------------------------
*/
  time_abort=0;
  abort_search=0;
  search_failed_high=0;
  search_failed_low=0;
  program_start_time=GetTime(time_type);
  start_time=GetTime(time_type);
  cpu_start=GetTime(cpu);
  cpu_percent=0;
  elapsed_start=GetTime(elapsed);
  nodes_searched=0;
  next_time_check=nodes_between_time_checks;
  evaluations=0;
  evaluations_hashed=0;
  max_search_depth=0;
  transposition_hashes=0;
  transposition_hashes_value=0;
  transposition_hashes_bound=0;
  transposition_hashes_cutoff=0;
  pawn_hashes=0;
  king_hashes=0;
  check_extensions_done=0;
  recapture_extensions_done=0;
  passed_pawn_extensions_done=0;
  nodes_searched_null_move=0;
  nodes_searched_null_move_wasted=0;
  null_moves_tried=0;
  null_moves_wasted=0;
  root_alpha=-MATE-1;
  root_value=-MATE-1;
  root_beta=MATE+1;
  root_wtm=wtm;
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
  first[0]=move_list;
  last[0]=move_list;
  if (search_type != booking) RootMoveList(wtm);
  if (first[1] == last[1]) {
    program_end_time=GetTime(time_type);
    pv[1].path_length=0;
    if (Check(1,wtm)) {
      root_value=-(MATE-1);
      last_search_value=-(MATE-1);
    }
    else {
      root_value=DrawScore();
      last_search_value=DrawScore();
    }
    nodes_searched=1;
    return(root_value);
  }
  if (first[1] == (last[1]-1)) {
    program_end_time=GetTime(time_type);
    pv[1].path_length=1;
    pv[1].path_hashed=0;
    pv[1].path_iteration_depth=0;
    pv[1].path[1]=*first[1];
    nodes_searched=1;
    return(root_value);
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
  nodes_between_time_checks=nodes_per_second;
  nodes_between_time_checks=Min(nodes_between_time_checks,50000);
  if (time_limit < 10) nodes_between_time_checks/=20;
  nodes_between_time_checks=Max(nodes_between_time_checks,1000);
  iteration_depth=1;
  if (pv[0].path_iteration_depth > 1)
    iteration_depth=pv[0].path_iteration_depth+1;
  Print(2,"              depth   time   value    variation (%d)\n",
        iteration_depth);
  if ((search_type==booking) || !Book(wtm,1)) {
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
    draft=iteration_depth-1;
    if (draft) {
      twtm=wtm;
      for (i=1;i<=pv[0].path_length;i++) {
        pv[i]=pv[i-1];
        MakeMove(i,pv[0].path[i],twtm);
        StorePV(i, twtm);
        twtm=!twtm;
      }
    }
    for (;iteration_depth<=60;iteration_depth++) {
      if (trace_level) {
        printf("==================================\n");
        printf("=      search iteration %2d       =\n",iteration_depth);
        printf("==================================\n");
      }
      for (mvp=first[1];mvp<last[1];mvp++)
        searched_this_root_move[mvp-first[1]]=0;
      search_failed_high=0;
      search_failed_low=0;
      while (!time_abort && !abort_search) {
        value=Search(root_alpha, root_beta, wtm, iteration_depth, 1, 1, 1);
        if (value >= root_beta) {
          search_failed_high=1;
          search_failed_low=0;
          root_alpha=root_beta-1;
          root_value=root_alpha;
          root_beta=100000;
          searched_this_root_move[0]=0;
        }
        else if (value <= root_alpha) {
          ndone=0;
          for (i=0;i<last[1]-first[1];i++)
            if (searched_this_root_move[i]) ndone++;
          if ((ndone == (last[1]-first[1])) && (last[1] > first[1]+1)) break;
          if ((ndone == 1) && (!search_failed_high)) {
            search_failed_high=0;
            search_failed_low=1;
            root_alpha=-100000;
            root_value=root_alpha;
            searched_this_root_move[0]=0;
            easy_move=0;
            if ((nodes_searched > noise_level) && (!time_abort && !abort_search)) {
              Print(2,"               %2i   %s      --   ",iteration_depth,
                    DisplayTime(GetTime(time_type)-start_time));
              Print(2," %s\n",OutputMove(&current_move[1],1,wtm));
            }
          }
        }
        else
          break;
      }
      if ((root_value > root_alpha) && (root_value < root_beta)) {
        last_search_value=root_value;
      }
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
      if (!time_abort  && !abort_search && (nodes_searched > noise_level)) {
        end_time=GetTime(time_type);
        Print(3,"               %2i-> %s%s   ",iteration_depth,
              DisplayTime(end_time-start_time), DisplayEvaluation(value));
      }
      if (value != -(MATE-1)) {
        more=1;
        draft=iteration_depth;
        for (i=1;i<=pv[1].path_length;i++) {
          pv[i+1]=pv[i];
          if (!time_abort && !abort_search && (nodes_searched > noise_level)) {
            if (!more) Print(3,"                                     ");
            Print(3," %s",OutputMove(&pv[1].path[i],i,twtm));
#if !defined(FAST)
            if (show_extensions && pv_extensions[1].extensions[i]) {
              int j,k;
              for (j=1,k=0;j<16;j=j<<1,k++)
                if (pv_extensions[1].extensions[i]&j) Print(2,"%c",ext_char[k]);
            }
#endif
            more=1;
            if (!(i&7)) {
              Print(3,"\n");
              more=0;
            }
          }
          MakeMove(i,pv[1].path[i],twtm);
          material=Material(i+1)/PAWN_VALUE;
          if(!time_abort && !abort_search) StorePV(i, twtm);
          twtm=!twtm;
        }
      }
      if (!time_abort && !abort_search && (nodes_searched > noise_level)) {
        if(pv[1].path_hashed) {
          if (!more) Print(3,"                                     ");
          Print(3," ...");
          more=1;
        }
        if (more) Print(3,"  \n");
      }
/*
      Print(11,"              time:%s  n:%d/%d  maxd:%d\n",
            DisplayTime(end_time-start_time), nodes_searched, 
            evaluations, max_search_depth);
*/
      root_alpha=value-400;
      root_value=root_alpha;
      root_beta=value+400;
      if ((value > MATE-100) && (value > last_mate_score)) break;
      if ((iteration_depth >= search_depth) && search_depth) break;
      if (time_abort || abort_search) break;
      end_time=GetTime(time_type)-start_time;
      if (thinking) {
        if (end_time >= time_limit) break;
      }
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
    for (i=0;i<hash_table_size;i++) {
      if (!And((trans_ref_b+i)->word1,mask_1)) used_w++;
      if (!And((trans_ref_w+i)->word1,mask_1)) used_b++;
    }
    end_time=GetTime(time_type);
    time_used=(end_time-start_time);
    if (time_used < 10) time_used=10;
    cpu_end=GetTime(cpu)-cpu_start;
    cpu_end=(cpu_end > 0) ? cpu_end : 1;
    elapsed_end=GetTime(elapsed)-elapsed_start;
    if (elapsed_end) {
      cpu_percent=Min(100*cpu_end/elapsed_end,100);
    }
    else {
      cpu_percent=100;
    }
    if (time_used > 10)
      nodes_per_second=nodes_searched*10/cpu_end;
    if (!evaluations) evaluations=1;
    if (!abort_search || time_abort) {
      if (!wtm) material=-material;
      Print(4,"              time:%s  cpu:%d%%  mat:%d",
            DisplayTime(end_time-start_time), cpu_percent, material); 
      Print(4,"  n:%d/%d  maxd:%d  nps:%d\n",
            nodes_searched, evaluations, max_search_depth, nodes_per_second);
      Print(5,"              ext-> checks:%d recaps:%d pawns:%d\n",
            check_extensions_done, recapture_extensions_done,
            passed_pawn_extensions_done);
      Print(6,"              hashing-> trans/ref:%d%%  pawn:%d%%  king:%d%%",
            100*transposition_hashes/nodes_searched,
            100*pawn_hashes/evaluations,100*king_hashes/(2*evaluations));
      Print(6,"  evals:%d%%\n",
            100*evaluations_hashed/(evaluations+evaluations_hashed));
      Print(6,"              hashing-> value:%d%%  bound:%d%%  cutoff:%d%%",
            transposition_hashes_value*100/nodes_searched,
            transposition_hashes_bound*100/nodes_searched,
            transposition_hashes_cutoff*100/nodes_searched);
      Print(6,"  used:w%d%% b%d%%\n",
            used_w*100/(hash_table_size+1),used_b*100/(hash_table_size+1));
      Print(7,"              nulls used[%d/%d] wasted[%d/%d]\n",
            null_moves_tried-null_moves_wasted,
            nodes_searched_null_move-nodes_searched_null_move_wasted,
            null_moves_wasted,nodes_searched_null_move_wasted);
    }
  }
  else {
    root_value=0;
    last_search_value=0;
    last_move_in_book=move_number;
  }
  program_end_time=GetTime(time_type);
  if (abs(last_search_value) > (MATE-100)) last_mate_score=last_search_value;
  return(last_search_value);
}
