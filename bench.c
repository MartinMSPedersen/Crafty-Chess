#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "chess.h"
#include "data.h"

/* last modified 8/6/99 */
/*
********************************************************************************
*                                                                              *
*   Bench() runs a simple six-position benchmark to gauge crafty's             *
*   performance.  The test positons are hard-coded, and the benchmark is       *
*   calculated much like it would with an external "test" file.  The test      *
*   is a mix of opening, middlegame, and endgame positions, with both tactical *
*   and positional aspects.  (For those interested, the positions chosen are   *
*   Bratko-Kopec 2, 4, 8, 12, 22 and 23.)  This test is a speed measure only;  *
*   the actual solutions to the positions are ignored.                         *
*                                                                              *
********************************************************************************
*/

void Bench(void) {
  double nodes=0.0;
  int old_do, old_st, old_sd, total_time_used;
  TREE * const tree=local[0];

  test_mode=1;
  total_time_used=0;
  old_st=search_time_limit;  /* save old time limit and display settings */
  old_sd=search_depth;
  old_do=display_options;
  search_time_limit=90000;   /* maximum of 15 minutes per position */
  display_options=1;         /* turn off display while running benchmark */

  if (book_file) {
    fclose(book_file);
    book_file=0;
  }
  if (books_file) {
    fclose(books_file);
    books_file=0;
  }

  Print(4095, "Running benchmark. . .\n");

  printf(".");
  fflush(stdout);
  strcpy(args[0],"3r1k2/4npp1/1ppr3p/p6P/P2PPPP1/1NR5/5K2/2R5");
  strcpy(args[1],"w");
  SetBoard(2,args,0);
  search_depth=11;

  InitializeHashTables();
  last_pv.pathd=0;
  largest_positional_score=100;
  thinking=1;
  tree->position[1]=tree->position[0];
  (void) Iterate(wtm,think,0);
  thinking=0;
  nodes+=(float)(tree->nodes_searched);
  total_time_used+=(program_end_time-program_start_time);
  printf(".");
  fflush(stdout);

  strcpy(args[0],"rnbqkb1r/p3pppp/1p6/2ppP3/3N4/2P5/PPP1QPPP/R1B1KB1R");
  strcpy(args[1],"w");
  strcpy(args[2],"KQkq");
  SetBoard(3,args,0);
  search_depth=11;

  InitializeHashTables();
  last_pv.pathd=0;
  largest_positional_score=100;
  thinking=1;
  tree->position[1]=tree->position[0];
  (void) Iterate(wtm,think,0);
  thinking=0;
  nodes+=(float)(tree->nodes_searched);
  total_time_used+=(program_end_time-program_start_time);
  printf(".");
  fflush(stdout);

  strcpy(args[0],"4b3/p3kp2/6p1/3pP2p/2pP1P2/4K1P1/P3N2P/8");
  strcpy(args[1],"w");
  SetBoard(2,args,0);
  search_depth=14;

  InitializeHashTables();
  last_pv.pathd=0;
  largest_positional_score=100;
  thinking=1;
  tree->position[1]=tree->position[0];
  (void) Iterate(wtm,think,0);
  thinking=0;
  nodes+=(float)(tree->nodes_searched);
  total_time_used+=(program_end_time-program_start_time);
  printf(".");
  fflush(stdout);

  strcpy(args[0],"r3r1k1/ppqb1ppp/8/4p1NQ/8/2P5/PP3PPP/R3R1K1");
  strcpy(args[1],"b");
  SetBoard(2,args,0);
  search_depth=11;

  InitializeHashTables();
  last_pv.pathd=0;
  largest_positional_score=100;
  thinking=1;
  tree->position[1]=tree->position[0];
  (void) Iterate(wtm,think,0);
  thinking=0;
  nodes+=(float)(tree->nodes_searched);
  total_time_used+=(program_end_time-program_start_time);
  printf(".");
  fflush(stdout);

  strcpy(args[0],"2r2rk1/1bqnbpp1/1p1ppn1p/pP6/N1P1P3/P2B1N1P/1B2QPP1/R2R2K1");
  strcpy(args[1],"b");
  SetBoard(2,args,0);
  search_depth=12;

  InitializeHashTables();
  last_pv.pathd=0;
  largest_positional_score=100;
  thinking=1;
  tree->position[1]=tree->position[0];
  (void) Iterate(wtm,think,0);
  thinking=0;
  nodes+=(float)(tree->nodes_searched);
  total_time_used+=(program_end_time-program_start_time);
  printf(".");
  fflush(stdout);

  strcpy(args[0],"r1bqk2r/pp2bppp/2p5/3pP3/P2Q1P2/2N1B3/1PP3PP/R4RK1");
  strcpy(args[1],"b");
  strcpy(args[2],"kq");
  SetBoard(3,args,0);
  search_depth=11;

  InitializeHashTables();
  last_pv.pathd=0;
  largest_positional_score=100;
  thinking=1;
  tree->position[1]=tree->position[0];
  (void) Iterate(wtm,think,0);
  thinking=0;
  nodes+=(float)(tree->nodes_searched);
  total_time_used+=(program_end_time-program_start_time);

  printf("\n");
  Print(4095,"Total nodes: %d\n", (int)nodes);
  Print(4095,"Raw nodes per second: %d\n", (int)(nodes / (total_time_used / 100)));
  Print(4095,"Total elapsed time: %d\n", (total_time_used / 100));
  Print(4095,"SMP time-to-ply measurement: %f\n", (640.0 / (total_time_used / 100)));
  input_stream=stdin;
  early_exit=99;
  test_mode=0;

  display_options=old_do;    /* reset old values and restart game */
  search_time_limit=old_st;
  search_depth=old_sd;
  NewGame(0);
}


