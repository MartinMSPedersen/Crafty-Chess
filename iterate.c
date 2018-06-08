#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "chess.h"
#include "data.h"
#include "epdglue.h"

/* last modified 08/27/99 */
/*
********************************************************************************
*                                                                              *
*   Iterate() is the routine used to drive the iterated search.  it repeatedly *
*   calls search, incrementing the search depth after each call, until either  *
*   time is exhausted or the maximum set search depth is reached.              *
*                                                                              *
********************************************************************************
*/
int Iterate(int wtm, int search_type, int root_list_done) {
  ROOT_MOVE temp;
  int wpawn, bpawn, TB_use_ok;
  int i, value=0, time_used;
  int twtm, used;
  int correct, correct_count, material=0, sorted;
  TREE * const tree=local[0];
/*
 ----------------------------------------------------------
|                                                          |
|  initialize.                                             |
|                                                          |
|  produce the root move list, which is ordered and kept   |
|  for the duration of this search (the order may change   |
|  as new best moves are backed up to the root of course.) |
|                                                          |
 ----------------------------------------------------------
*/
  largest_positional_score=Max(largest_positional_score>>1,100);
  if (average_nps == 0) average_nps=150000*max_threads;
  if (wtm) {
    draw_score[0]=-abs_draw_score;
    draw_score[1]=abs_draw_score;
  }
  else {
    draw_score[0]=abs_draw_score;
    draw_score[1]=-abs_draw_score;
  }
  time_abort=0;
  abort_search=0;
  book_move=0;
  program_start_time=ReadClock(time_type);
  start_time=ReadClock(time_type);
  cpu_time_used=0;
  elapsed_start=ReadClock(elapsed);
  root_wtm=wtm;
  PreEvaluate(tree,wtm);
  tree->nodes_searched=0;
  tree->fail_high=0;
  tree->fail_high_first=0;
  parallel_splits=0;
  parallel_stops=0;
  max_split_blocks=0;
  TB_use_ok=1;
  if (TotalWhitePawns && TotalBlackPawns) {
    wpawn=FirstOne(WhitePawns);
    bpawn=FirstOne(BlackPawns);
    if (FileDistance(wpawn,bpawn) == 1) {
      if(((Rank(wpawn)==RANK2) && (Rank(bpawn)>RANK3)) ||
         ((Rank(bpawn)==RANK7) && (Rank(wpawn)<RANK6)) || 
         EnPassant(1)) TB_use_ok=0;
    }
  }
  if (booking || !Book(tree,wtm,root_list_done)) do {
    if (abort_search) break;
    if (!root_list_done) RootMoveList(wtm);
    if (EGTB_draw && !puzzling && swindle_mode) EGTB_use=0;
    else EGTB_use=EGTBlimit;
    if (EGTBlimit && !EGTB_use)
      Print(128,"Drawn at root, trying for swindle.\n");
    correct_count=0;
    burp=15*100;
    transposition_id=(transposition_id+1)&7;
    if (!transposition_id) transposition_id++;
    program_start_time=ReadClock(time_type);
    start_time=ReadClock(time_type);
    cpu_percent=0;
    elapsed_start=ReadClock(elapsed);
    next_time_check=nodes_between_time_checks;
    tree->evaluations=0;
#if !defined(FAST)
    tree->transposition_hits=0;
    tree->transposition_probes=0;
    tree->pawn_probes=0;
    tree->pawn_hits=0;
#endif
    tree->egtb_probes=0;
    tree->egtb_probes_successful=0;
    tree->check_extensions_done=0;
    tree->threat_extensions_done=0;
    tree->recapture_extensions_done=0;
    tree->passed_pawn_extensions_done=0;
    tree->one_reply_extensions_done=0;
    root_wtm=wtm;
/*
 ----------------------------------------------------------
|                                                          |
|  if there are no legal moves, it is either mate or draw  |
|  depending on whether the side to move is in check or    |
|  not (mate or stalemate.)                                |
|                                                          |
 ----------------------------------------------------------
*/
    if (n_root_moves == 0) {
      program_end_time=ReadClock(time_type);
      tree->pv[0].pathl=0;
      tree->pv[0].pathd=10;
      if (Check(wtm)) {
        root_value=-(MATE-1);
      }
      else {
        root_value=DrawScore(wtm);
      }
      Print(6,"              depth   time  score   variation\n");
      Print(6,"                                    (no moves)\n");
      tree->nodes_searched=1;
      if (!puzzling) last_root_value=root_value;
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
    iteration_depth=1;
    if (last_pv.pathd > 1)
      iteration_depth=last_pv.pathd+1;
    Print(6,"         nss  depth   time  score   variation (%d)\n",
          iteration_depth);
    time_abort=0;
    abort_search=0;
    program_start_time=ReadClock(time_type);
    start_time=ReadClock(time_type);
    elapsed_start=ReadClock(elapsed);
/*
 ----------------------------------------------------------
|                                                          |
|   now install the learned position information in the    |
|   hash table.                                            |
|                                                          |
 ----------------------------------------------------------
*/
    LearnPositionLoad();
/*
 ----------------------------------------------------------
|                                                          |
|   set the initial search bounds based on the last search |
|   or default values.                                     |
|                                                          |
 ----------------------------------------------------------
*/
    tree->pv[0]=last_pv;
    if (iteration_depth > 1) {
      root_alpha=last_value-40;
      root_value=last_value-40;
      root_beta=last_value+40;
    }
    else {
      root_alpha=-MATE-1;
      root_value=-MATE-1;
      root_beta=MATE+1;
    }
/*
 ----------------------------------------------------------
|                                                          |
|   if we are using multiple threads, and they have not    |
|   been started yet, then start them now as the search    |
|   is ready to begin.                                     |
|                                                          |
 ----------------------------------------------------------
*/
#if defined(SMP)
    if (max_threads>smp_idle+1) {
      pthread_t pt;
      int proc;
      for (proc=smp_threads+1;proc<max_threads;proc++) {
        Print(128,"starting thread %d\n",proc);
        thread[proc]=0;
        tfork(pt,ThreadInit,proc);
        smp_threads++;
      }
    }
#endif
    root_print_ok=0;
    for (;iteration_depth<=60;iteration_depth++) {
/*
 ----------------------------------------------------------
|                                                          |
|   now install the old PV into the hash table so that     |
|   these moves will be followed first.                    |
|                                                          |
 ----------------------------------------------------------
*/
      twtm=wtm;
      for (i=1;i<=(int) tree->pv[0].pathl;i++) {
        tree->pv[i]=tree->pv[i-1];
        if (!LegalMove(tree,i,twtm,tree->pv[i].path[i])) {
          Print(4095,"ERROR, not installing bogus move at ply=%d\n",i);
          Print(4095,"not installing from=%d  to=%d  piece=%d\n",
                From(tree->pv[i].path[i]), To(tree->pv[i].path[i]),
                Piece(tree->pv[i].path[i]));
          break;
        }
        HashStorePV(tree, i, twtm);
        MakeMove(tree, i,tree->pv[0].path[i],twtm);
        twtm=ChangeSide(twtm);
      }
      for (i--;i>0;i--) {
        twtm=ChangeSide(twtm);
        UnMakeMove(tree, i,tree->pv[0].path[i],twtm);
      }
      if (trace_level) {
        printf("==================================\n");
        printf("=      search iteration %2d       =\n",iteration_depth);
        printf("==================================\n");
      }
      if (tree->nodes_searched) {
        nodes_between_time_checks=nodes_per_second;
        nodes_between_time_checks=Min(nodes_between_time_checks,MAX_TC_NODES);
        nodes_between_time_checks=Max(nodes_between_time_checks,5000);
        if (!analyze_mode) {
          if (time_limit>300 && !auto232);
          else if (time_limit>100 || auto232) nodes_between_time_checks/=10;
          else if (time_limit > 50) nodes_between_time_checks/=20;
          else nodes_between_time_checks/=100;
        } else nodes_between_time_checks=5000;
      }
      while (!time_abort && !abort_search) {
#if defined(SMP)
        thread[0]=local[0];
#endif
        thread_start_time[0]=ReadClock(cpu);
        value=SearchRoot(tree,root_alpha, root_beta, wtm,
                         iteration_depth*INCPLY+45);
        root_print_ok=tree->nodes_searched > noise_level;
        cpu_time_used+=ReadClock(cpu)-thread_start_time[0];
        if (value >= root_beta) {
          root_moves[0].status|=2;
          root_moves[0].status&=255-128;
          if (root_moves[0].status&1) root_alpha=-MATE-1;
          else root_alpha=root_beta-1;
          root_value=root_alpha;
          root_beta=MATE+1;
          root_moves[0].nodes=0;
        }
        else if (value <= root_alpha) {
          if (!(root_moves[0].status&2)) {
            root_moves[0].status&=255-128;
            root_moves[0].nodes=0;
            root_moves[0].status|=1;
            if (root_moves[0].status&2) root_beta=MATE+1;
            root_alpha=-MATE-1;
            root_value=root_alpha;
            easy_move=0;
            if (root_print_ok && !time_abort && !abort_search) {
              Print(4,"               %2i   %s     --   ",iteration_depth,
                    DisplayTime(ReadClock(time_type)-start_time));
              if (display_options&64) Print(4,"%d. ",move_number);
              if ((display_options&64) && !wtm) Print(4,"... ");
              Print(4,"%s\n",OutputMove(tree,root_moves[0].move,1,wtm));
            }
          }
          else break;
        }
        else break;
      }
      if (root_value>root_alpha && root_value<root_beta)
        last_root_value=root_value;
/*
 ----------------------------------------------------------
|                                                          |
|   if we are running a test suite, check to see if we can |
|   exit the search.  this happens when N successive       |
|   iterations produce the correct solution.  N is set by  |
|   the test command in Option().                          |
|                                                          |
 ----------------------------------------------------------
*/
      correct=solution_type;
      for (i=0;i<number_of_solutions;i++) {
        if (!solution_type) {
          if (solutions[i] == tree->pv[0].path[1]) correct=1;
        }
        else
          if (solutions[i] == tree->pv[0].path[1]) correct=0;
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
      end_time=ReadClock(time_type);
      do {
        sorted=1;
        for (i=1;i<n_root_moves-1;i++) {
          if (root_moves[i].nodes < root_moves[i+1].nodes) {
            temp=root_moves[i];
            root_moves[i]=root_moves[i+1];
            root_moves[i+1]=temp;
            sorted=0;
          }
        }
      } while(!sorted);
      for (i=0;i<n_root_moves;i++) root_moves[i].status=0;
      if (root_moves[0].nodes > 1000)
        for (i=0;i<Min(n_root_moves,4);i++) {
          if (root_moves[i].nodes > root_moves[0].nodes/2)
            root_moves[i].status|=64;
        }
      if (display_options&256) {
        unsigned int total_nodes=0;
        Print(4095,"       move       nodes      hi/low\n");
        for (i=0;i<n_root_moves;i++) {
          total_nodes+=root_moves[i].nodes;
          Print(4095," %10s  %10d       %d   %d\n",
            OutputMove(tree,root_moves[i].move,1,wtm),
                       (unsigned int)root_moves[i].nodes,
                       (root_moves[i].status&2)>0,(root_moves[i].status&1)>0);
        }
        Print(256,"      total  %10d\n",total_nodes);
      }
/*
 ----------------------------------------------------------
|                                                          |
|   notice if there are multiple moves that are producing  |
|   large trees.  if so, don't search those in parallel by |
|   setting the flag to avoid this.                        |
|                                                          |
 ----------------------------------------------------------
*/
      for (i=0;i<n_root_moves;i++) root_moves[i].nodes=0;
      if (end_time-start_time > 10)
        nodes_per_second=(BITBOARD) tree->nodes_searched*100/(BITBOARD) (end_time-start_time);
      else
        nodes_per_second=10000;
      if (!time_abort && !abort_search && (root_print_ok ||
          correct_count>=early_exit || value>MATE-300 || tree->pv[0].pathh==2)) {
        if (value != -(MATE-1))
          DisplayPV(tree,5,wtm,end_time-start_time,value,&tree->pv[0]);
      }
      root_alpha=value-40;
      root_value=root_alpha;
      root_beta=value+40;
      if (iteration_depth>3 && value>MATE-300 &&
          value>=(MATE-iteration_depth-1) && value>last_mate_score) break;
      if ((iteration_depth >= search_depth) && search_depth) break;
      if (time_abort || abort_search) break;
      end_time=ReadClock(time_type)-start_time;
      if (thinking && (int)end_time>=time_limit) break;
      if (correct_count >= early_exit) break;
      if (iteration_depth>3 && TotalPieces<=EGTBlimit && TB_use_ok &&
          EGTB_use && !EGTB_search && EGTBProbe(tree, 1, wtm, &i)) break;
      if (search_nodes && tree->nodes_searched>search_nodes) break;
    }
/*
 ----------------------------------------------------------
|                                                          |
|   search done, now display statistics, depending on the  |
|   verbosity level set.                                   |
|                                                          |
 ----------------------------------------------------------
*/
    used=0;
#if !defined(FAST)
    for (i=0;i<hash_table_size;i++) {
      if ((trans_ref_a+i)->word1>>61 == transposition_id) used++;
    }
    for (i=0;i<hash_table_size*2;i++) {
      if ((trans_ref_b+i)->word1>>61 == transposition_id) used++;
    }
#endif
    end_time=ReadClock(time_type);
    time_used=(end_time-start_time);
    if (time_used < 10) time_used=10;
    elapsed_end=ReadClock(elapsed)-elapsed_start;
    if (elapsed_end)
      cpu_percent=100*cpu_time_used/elapsed_end;
    else cpu_percent=100;
    if (elapsed_end > 10)
      nodes_per_second=(BITBOARD)tree->nodes_searched*100/(BITBOARD) elapsed_end;
    if (!tree->egtb_probes) {
      if (average_nps) average_nps=(9*average_nps+nodes_per_second)/10;
      else average_nps=nodes_per_second;
    }
    tree->evaluations=(tree->evaluations) ? tree->evaluations : 1;
    if ((!abort_search || time_abort) && !puzzling) {
      tree->fail_high++;
      tree->fail_high_first++;
      material=Material/PAWN_VALUE;
      if (ChangeSide(wtm)) material=-material;
      Print(8,"              time=%s  cpu=%d%%  mat=%d",
            DisplayTimeWhisper(end_time-start_time), cpu_percent, material); 
      Print(8,"  n=%u", tree->nodes_searched);
      Print(8,"  fh=%u%%", tree->fail_high_first*100/tree->fail_high);
      Print(8,"  nps=%d\n", nodes_per_second);
      Print(16,"              ext-> checks=%d recaps=%d pawns=%d 1rep=%d thrt:%d\n",
            tree->check_extensions_done, tree->recapture_extensions_done,
            tree->passed_pawn_extensions_done, tree->one_reply_extensions_done,
            tree->threat_extensions_done);
      Print(16,"              predicted=%d  nodes=%u  evals=%u\n", 
             predicted, tree->nodes_searched, tree->evaluations);
      Print(16,"              endgame tablebase-> probes done=%d  successful=%d\n",
            tree->egtb_probes, tree->egtb_probes_successful);
#if !defined(FAST)
      Print(16,"              hashing-> trans/ref=%d%%  pawn=%d%%  used=%d%%\n",
            100*tree->transposition_hits/(tree->transposition_probes+1),
            100*tree->pawn_hits/(tree->pawn_probes+1),
            used*100/(3*hash_table_size+1));
#endif
#if defined(SMP)
      Print(16,"              SMP->  split=%d  stop=%d  data=%d/%d  ",
            parallel_splits, parallel_stops,max_split_blocks,MAX_BLOCKS);
      Print(16,"cpu=%s  ", DisplayTimeWhisper(cpu_time_used));
      Print(16,"elap=%s\n", DisplayTimeWhisper(elapsed_end));
#endif
    }
  } while(0);
  else {
    last_root_value=0;
    root_value=0;
    book_move=1;
    tree->pv[0]=tree->pv[1];
    if (analyze_mode) Whisper(4,0,0,0,0,0,0,whisper_text);
  }
  program_end_time=ReadClock(time_type);
  search_move=0;
  return(last_root_value);
}
