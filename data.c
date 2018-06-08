#include <stdio.h>
#include <stdlib.h>
#include "types.h"
#include "evaluate.h"
  
  char      version[5] =                     {"8.22"};

  int       number_of_computers =                  12;
  char      computer_list[100][20] = {
                                      {"biggenius"},
                                      {"ferret"},
                                      {"fitter"},
                                      {"fritzpentium"},
                                      {"kerrigan"},
                                      {"mephistoiii"},
                                      {"megadiep"},
                                      {"nowx"},
                                      {"robocop"},
                                      {"wchess"},
                                      {"wchessx"},
                                      {"zarkovx"}};

  int       number_of_GMs =                        14;
  char      GM_list[100][20] =       {
                                      {"anat"},
                                      {"dgurevich"},
                                      {"dlugy"},
                                      {"flamingskull"},
                                      {"junior"},
                                      {"kaidanov"},
                                      {"kingloek"},
                                      {"kudrin"},
                                      {"lombardy"},
                                      {"psakhis"},
                                      {"roman"},
                                      {"sagalchik"},
                                      {"securitron"},
                                      {"yofe"}};

  int       number_of_IMs =                         9;
  char      IM_list[100][20] =       {
                                      {"adolf"},
                                      {"badviking"},
                                      {"bandora"},
                                      {"impolzin"},
                                      {"kevlar"},
                                      {"laflair"},
                                      {"lsokol"},
                                      {"thutters"},
                                      {"tim"}};

  int       ics =                                   0;
  int       whisper =                               0;
  int       kibitz =                                0;
  int       log_id =                                1;
  int       move_number =                           1;
  int       wtm =                                   1;
  int       largest_positional_score =           1000;
  int       search_depth =                          0;
  int       search_move =                           0;
  int       repetition_head =                       0;
  int       time_type =                       elapsed;
  int       nodes_between_time_checks =          1000;
  int       nodes_per_second =                   1000;

  int       null_depth =                            2;
  int       check_extensions =                      1;
  int       recapture_extensions =                  1;
  int       passed_pawn_extensions =                1;
  int       quiescence_checks =                     2;
#if !defined(FAST)
  int       show_extensions =                       0;
#endif

  int       opening =                               1;
  int       middle_game =                           0;
  int       end_game =                              0;
  int       thinking =                              0;
  int       pondering =                             0;
  int       puzzling =                              0;
  int       booking =                               0;
  int       analyze_mode =                          0;
  int       analyze_move_read =                     0;
  int       resign =                                5;
  int       resign_counter =                        0;
  int       resign_count =                         10;
  int       draw =                                  5;
  int       draw_counter =                          0;
  int       draw_count =                           10;
  int       tc_moves =                             60;
  int       tc_time =                            1800;
  int       tc_simple_average_time =               30;
  int       tc_time_remaining =                  1800;
  int       tc_moves_remaining =                   60;
  int       tc_secondary_moves =                   30;
  int       tc_secondary_time =                   900;
  int       tc_increment =                          0;
  int       tc_sudden_death =                       0;
  int       tc_clock_start =                        0;
  int       tc_operator_time =                      0;
  int       tc_safety_margin =                      0;
  int       force =                                 0;
  int       over =                                  0;
  
  char      audible_alarm =                      0x07;
  int       ansi =                                  1;
  int       book_accept_mask =                     ~3;
  int       book_reject_mask =                      3;
  int       book_random =                           3;
  int       book_lower_bound =                      2;
  int       book_absolute_lower_bound =             2;
  int       show_book =                             0;
  float     book_min_percent_played =             .05;
  int       do_ponder =                             1;
  int       trace_level =                           0;
  int       verbosity_level =                       9;
  int       burp =                                 15;
  int       noise_level =                         500;
 
  int       hash_table_size =                   16384;
  int       log_hash_table_size =                  14;
  int       pawn_hash_table_size =               4096;
  int       log_pawn_hash_table_size =             12;
  int       king_hash_table_size =               4096;
  int       log_king_hash_table_size =             12;

  int       default_draw_score =                 DRAW;

  int       connected_passer[8] =  { 0,
                                     PAWN_CONNECTED_PASSED_RANK2,
                                     PAWN_CONNECTED_PASSED_RANK3,
                                     PAWN_CONNECTED_PASSED_RANK4,
                                     PAWN_CONNECTED_PASSED_RANK5,
                                     PAWN_CONNECTED_PASSED_RANK6,
                                     PAWN_CONNECTED_PASSED_RANK7,
                                     0 };
  
  int       supported_passer[8] =  { 0,
                                     PAWN_SUPPORTED_PASSED_RANK2,
                                     PAWN_SUPPORTED_PASSED_RANK3,
                                     PAWN_SUPPORTED_PASSED_RANK4,
                                     PAWN_SUPPORTED_PASSED_RANK5,
                                     PAWN_SUPPORTED_PASSED_RANK6,
                                     PAWN_SUPPORTED_PASSED_RANK7,
                                     0 };
  
  int       outside_passed[128] = { 500, 500, 500, 300, 300, 200, 200, 200,
                                    200, 100, 100, 100, 100, 100, 100, 100,
                                    100, 100, 100, 100, 100, 100, 100, 100,
                                    100, 100, 100, 100, 100, 100, 100, 100,
                                    100, 100, 100, 100, 100, 100, 100, 100,
                                    100, 100, 100, 100, 100, 100, 100, 100,
                                    100, 100, 100, 100, 100, 100, 100, 100,
                                    100, 100, 100, 100, 100, 100, 100, 100,
                                    100, 100, 100, 100, 100, 100, 100, 100,
                                    100, 100, 100, 100, 100, 100, 100, 100,
                                    100, 100, 100, 100, 100, 100, 100, 100,
                                    100, 100, 100, 100, 100, 100, 100, 100,
                                    100, 100, 100, 100, 100, 100, 100, 100,
                                    100, 100, 100, 100, 100, 100, 100, 100,
                                    100, 100, 100, 100, 100, 100, 100, 100,
                                    100, 100, 100, 100, 100, 100, 100, 100};
    
  int       piece_values[8] =        {0,PAWN_VALUE,KNIGHT_VALUE,KING_VALUE,
                                      0, BISHOP_VALUE,ROOK_VALUE,QUEEN_VALUE};
  int       pawn_ram[9] =            {0,0,PAWN_RAM*2,PAWN_RAM*4,
                                      PAWN_RAM*6,PAWN_RAM*10,PAWN_RAM*13,
                                      PAWN_RAM*15,PAWN_RAM*18};
  int       isolated[9] =            { 0,PAWN_ISOLATED, 3*PAWN_ISOLATED,
                                       6*PAWN_ISOLATED,10*PAWN_ISOLATED,
                                      13*PAWN_ISOLATED,14*PAWN_ISOLATED,
                                      15*PAWN_ISOLATED,16*PAWN_ISOLATED};

  int       b_n_mate_dark_squares[64] = 
                             { 2000, 1500, 1000,  500, -500,-1000,-1500,-2000,
                               1500, 1500, 1000,  500, -500,-1000,-1500,-1500,
                               1000, 1000, 1000,  500, -500,-1000,-1000,-1000,
                                500,  500,  500,  500, -500, -500, -500, -500,
                               -500, -500, -500, -500,  500,  500,  500,  500,
                              -1000,-1000,-1000, -500,  500, 1000, 1000, 1000,
                              -1500,-1500,-1000, -500,  500, 1000, 1500, 1500,
                              -2000,-1500,-1000, -500,  500, 1000, 1500, 2000};

  int       b_n_mate_light_squares[64] =
                             {-2000,-1500,-1000, -500,  500, 1000, 1500, 2000,
                              -1500,-1500,-1000, -500,  500, 1000, 1500, 1500,
                              -1000,-1000,-1000, -500,  500, 1000, 1000, 1000,
                               -500, -500, -500, -500,  500,  500,  500,  500,
                                500,  500,  500,  500, -500, -500, -500, -500,
                               1000, 1000, 1000,  500, -500,-1000,-1000,-1000,
                               1500, 1500, 1000,  500, -500,-1000,-1500,-1500,
                               2000, 1500, 1000,  500, -500,-1000,-1500,-2000};

  int       mate[64] =           {4800,4600,4400,4200,4200,4400,4600,4800,
                                  4600,3600,3400,3200,3200,3400,3600,4600,
                                  4400,3400,2400,2200,2200,2400,3400,4400,
                                  4200,3200,2200,1200,1200,2200,3200,4200,
                                  4200,3200,2200,1200,1200,2200,3200,4200,
                                  4400,3400,2400,2200,2200,2400,3400,4400,
                                  4600,3600,3400,3200,3200,3400,3600,4600,
                                  4800,4600,4400,4200,4200,4400,4600,4800};


#if !defined(COMPACT_ATTACKS)
  int            bishop_shift_rl45[64] = { 63, 61, 58, 54, 49, 43, 36, 28,
                                           61, 58, 54, 49, 43, 36, 28, 21,
                                           58, 54, 49, 43, 36, 28, 21, 15,
                                           54, 49, 43, 36, 28, 21, 15, 10,
                                           49, 43, 36, 28, 21, 15, 10,  6,
                                           43, 36, 28, 21, 15, 10,  6,  3,
                                           36, 28, 21, 15, 10,  6,  3,  1,
                                           28, 21, 15, 10,  6,  3,  1,  0 };

  int            bishop_shift_rr45[64] = { 
                                           28, 36, 43, 49, 54, 58, 61, 63,
                                           21, 28, 36, 43, 49, 54, 58, 61,
                                           15, 21, 28, 36, 43, 49, 54, 58,
                                           10, 15, 21, 28, 36, 43, 49, 54,
                                            6, 10, 15, 21, 28, 36, 43, 49,
                                            3,  6, 10, 15, 21, 28, 36, 43,
                                            1,  3,  6, 10, 15, 21, 28, 36,
                                            0,  1,  3,  6, 10, 15, 21, 28 };
#endif COMPACT_ATTACKS

  int            white_outpost[64] =  { 0, 0, 0, 0, 0, 0, 0, 0,
                                        0, 0, 0, 0, 0, 0, 0, 0,
                                        0, 0, 0, 0, 0, 0, 0, 0,
                                        0, 0, 0, 0, 0, 0, 0, 0,
                                        0, 0, 2, 2, 2, 2, 0, 0,
                                        0, 2, 2, 2, 2, 2, 2, 0,
                                        0, 1, 1, 1, 1, 1, 1, 0,
                                        0, 0, 0, 0, 0, 0, 0, 0 };

  int            black_outpost[64] =  { 0, 0, 0, 0, 0, 0, 0, 0,
                                        0, 1, 1, 1, 1, 1, 1, 0,
                                        0, 2, 2, 2, 2, 2, 2, 0,
                                        0, 0, 2, 2, 2, 2, 0, 0,
                                        0, 0, 0, 0, 0, 0, 0, 0,
                                        0, 0, 0, 0, 0, 0, 0, 0,
                                        0, 0, 0, 0, 0, 0, 0, 0,
                                        0, 0, 0, 0, 0, 0, 0, 0 };

  int            push_extensions[64] = { 0, 0, 0, 0, 0, 0, 0, 0,
                                         1, 1, 1, 1, 1, 1, 1, 1,
                                         0, 0, 0, 0, 0, 0, 0, 0,
                                         0, 0, 0, 0, 0, 0, 0, 0,
                                         0, 0, 0, 0, 0, 0, 0, 0,
                                         0, 0, 0, 0, 0, 0, 0, 0,
                                         1, 1, 1, 1, 1, 1, 1, 1,
                                         0, 0, 0, 0, 0, 0, 0, 0 };

  int  knight_value_w[64] = {-100, -100, -100, -100, -100, -100, -100, -100,
                             -100,    0,    0,    0,    0,    0,    0, -100,
                             -100,    0,   20,   20,   20,   20,    0, -100,
                             -100,    0,   20,   40,   40,   20,    0, -100,
                             -100,    0,   20,   40,   40,   20,    0, -100,
                             -100,    0,   20,   20,   20,   20,    0, -100,
                             -100,    0,    0,    0,    0,    0,    0, -100,
                             -100, -100, -100, -100, -100, -100, -100, -100};

  int  knight_value_b[64] = {-100, -100, -100, -100, -100, -100, -100, -100,
                             -100,    0,    0,    0,    0,    0,    0, -100,
                             -100,    0,   20,   20,   20,   20,    0, -100,
                             -100,    0,   20,   40,   40,   20,    0, -100,
                             -100,    0,   20,   40,   40,   20,    0, -100,
                             -100,    0,   20,   20,   20,   20,    0, -100,
                             -100,    0,    0,    0,    0,    0,    0, -100,
                             -100, -100, -100, -100, -100, -100, -100, -100};

  int  bishop_value_w[64] = { -5,  -5,  -5,  -5,  -5,  -5,  -5,  -5,
                              -5,   0,   0,   0,   0,   0,   0,  -5,
                              -5,   0,   5,   5,   5,   5,   0,  -5,
                              -5,   0,   5,  10,  10,   5,   0,  -5,
                              -5,   0,   5,  10,  10,   5,   0,  -5,
                              -5,   0,   5,   5,   5,   5,   0,  -5,
                              -5,   0,   0,   0,   0,   0,   0,  -5,
                              -5,  -5,  -5,  -5,  -5,  -5,  -5,  -5};

  int  bishop_value_b[64] = { -5,  -5,  -5,  -5,  -5,  -5,  -5,  -5,
                              -5,   0,   0,   0,   0,   0,   0,  -5,
                              -5,   0,   5,   5,   5,   5,   0,  -5,
                              -5,   0,   5,  10,  10,   5,   0,  -5,
                              -5,   0,   5,  10,  10,   5,   0,  -5,
                              -5,   0,   5,   5,   5,   5,   0,  -5,
                              -5,   0,   0,   0,   0,   0,   0,  -5,
                              -5,  -5,  -5,  -5,  -5,  -5,  -5,  -5};

  int   queen_value_w[64] = {-10, -10, -10, -10, -10, -10, -10, -10,
                             -10,   0,   0,   0,   0,   0,   0, -10,
                             -10,   0,  10,  10,  10,  10,   0, -10,
                             -10,   0,  10,  20,  20,  10,   0, -10,
                             -10,   0,  10,  20,  20,  10,   0, -10,
                             -10,   0,  10,  10,  10,  10,   0, -10,
                             -10,   0,   0,   0,   0,   0,   0, -10,
                             -10, -10, -10, -10, -10, -10, -10, -10};

  int   queen_value_b[64] = {-10, -10, -10, -10, -10, -10, -10, -10,
                             -10,   0,   0,   0,   0,   0,   0, -10,
                             -10,   0,  10,  10,  10,  10,   0, -10,
                             -10,   0,  10,  20,  20,  10,   0, -10,
                             -10,   0,  10,  20,  20,  10,   0, -10,
                             -10,   0,  10,  10,  10,  10,   0, -10,
                             -10,   0,   0,   0,   0,   0,   0, -10,
                             -10, -10, -10, -10, -10, -10, -10, -10};

  int    king_value_w[64] = {-80, -40, -40, -40, -40, -40, -40, -80,
                             -40, -40,   0,   0,   0,   0, -40, -40,
                             -30,   0,   0,  40,  40,   0,   0, -30,
                             -20,   0,  40,  80,  80,  40,   0, -20,
                             -10,  40,  80,  80,  80,  80,  40, -10,
                              40,  80, 120, 120, 120, 120,  80,  40,
                              40,  80,  80,  80,  80,  80,  80,  40,
                             -80, -40, -40, -40, -40, -40, -40, -80};

  int    king_value_b[64] = {-80, -40, -40, -40, -40, -40, -40, -80,
                              40,  80,  80,  80,  80,  80,  80,  40,
                              40,  80, 120, 120, 120, 120,  80,  40,
                             -10,  40,  80,  80,  80,  80,  40, -10,
                             -20,   0,  40,  80,  80,  40,   0, -20,
                             -30,   0,   0,  40,  40,   0,   0, -30,
                             -40, -40,   0,   0,   0,   0, -40, -40,
                             -80, -40, -40, -40, -40, -40, -40, -80};

