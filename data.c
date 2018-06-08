#include <stdio.h>
#include <stdlib.h>
#include "types.h"
#include "evaluate.h"
  
  char      version[6] =                    {"10.18"};

  PLAYING_MODE mode =                     normal_mode;

  int       number_auto_kibitzers =                 8;

  char      auto_kibitz_list[100][20] = {
                                      {"ferret"},
                                      {"kerrigan"},
                                      {"nowx"},
                                      {"ratbert"},
                                      {"roborvl"},
                                      {"wchess"},
                                      {"wchessx"},
                                      {"zarkovx"}};
  

  int       number_of_computers =                  32;
  char      computer_list[100][20] = {
                                      {"amyii"},
                                      {"ban"},
                                      {"biggenius"},
                                      {"centius"},
                                      {"chameleon"},
                                      {"chesst"},
                                      {"crazyknight"},
                                      {"cstal"},
                                      {"deepviolet"},
                                      {"destroyer"},
                                      {"doctorwho"},
                                      {"ferret"},
                                      {"fitter"},
                                      {"fritzpentium"},
                                      {"gnusurf"},
                                      {"kerrigan"},
                                      {"kingtwoft"},
                                      {"lightpurple"},
                                      {"lonnie"},
                                      {"mephistoiii"},
                                      {"megadiep"},
                                      {"mnemonic"},
                                      {"netsurfer"},
                                      {"nowx"},
                                      {"ratbert"},
                                      {"robocop"},
                                      {"roborvl"},
                                      {"shredder"},
                                      {"turbogm"},
                                      {"wchess"},
                                      {"wchessx"},
                                      {"zarkovx"}};

  int       number_of_GMs =                        24;
  char      GM_list[100][20] =       {
                                      {"anat"},
                                      {"devin"},
                                      {"dgurevich"},
                                      {"dlugy"},
                                      {"flamingskull"},
                                      {"gum"},
                                      {"gmsoffer"},
                                      {"junior"},
                                      {"kaidanov"},
                                      {"kc"},
                                      {"kevlar"},
                                      {"kingloek"},
                                      {"kudrin"},
                                      {"lombardy"},
                                      {"psakhis"},
                                      {"roman"},
                                      {"sagalchik"},
                                      {"securitron"},
                                      {"smirin"},
                                      {"stefansson"},
                                      {"taxi"},
                                      {"tigre"},
                                      {"vivre"},
                                      {"yofe"}};

  int       number_of_IMs =                        10;
  char      IM_list[100][20] =       {
                                      {"adolf"},
                                      {"badviking"},
                                      {"bandora"},
                                      {"imorlov"},
                                      {"impolzin"},
                                      {"laflair"},
                                      {"lsokol"},
                                      {"thutters"},
                                      {"thumpster"},
                                      {"tim"}};

  int       ics =                                   0;
  int       xboard =                                0;
  int       whisper =                               0;
  int       kibitz =                                0;
  int       post =                                  0;
  int       log_id =                                1;
  int       move_number =                           1;
  int       wtm =                                   1;
  int       last_opponent_move =                    0;
  int       largest_positional_score =           1000;
  int       search_depth =                          0;
  int       search_move =                           0;
  int       time_type =                       elapsed;
  int       nodes_between_time_checks =          4000;
  int       nodes_per_second =                   4000;

  int       time_used =                             0;
  int       time_used_opponent =                    0;
  int       auto_kibitzing =                        0;
  int       transposition_id =                      0;
  int       check_extensions =                      1;
  int       recapture_extensions =                  1;
  int       passed_pawn_extensions =                1;
  int       one_reply_extensions =                  1;

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
  int       move_limit =                          200;
  int       time_divisor=                           2;
  float     increment_factor=                       1;
  int       tc_moves =                             60;
  int       tc_time =                          180000;
  int       tc_time_remaining =                180000;
  int       tc_time_remaining_opponent =       180000;
  int       tc_moves_remaining =                   60;
  int       tc_secondary_moves =                   30;
  int       tc_secondary_time =                 90000;
  int       tc_increment =                          0;
  int       tc_sudden_death =                       0;
  int       tc_clock_start =                        0;
  int       tc_operator_time =                      0;
  int       tc_safety_margin =                      0;
  int       time_limit =                           10;
  int       force =                                 0;
  int       over =                                  0;
  float     zero_inc_factor =                       8;
  float     inc_time_multiplier =                   9;
  int       draw_score_is_zero =                    0;
  float     usage_level =                           0;
  float     usage_time =                            0;
  float     u_time=                               300;
  float     u_otime=                              300;

  char      audible_alarm =                      0x07;
#if defined(NT_i386) || defined(NT_AXP)
  int       ansi =                                  0;
#else
  int       ansi =                                  1;
#endif
  int       book_accept_mask =                    ~03;
  int       book_reject_mask =                      3;
  int       book_random =                           2;
  int       show_book =                             0;
  int       book_selection_width =                  5;
  int       do_ponder =                             1;
  int       trace_level =                           0;
  int       verbosity_level =                       9;
  int       noise_level =                       10000;
 
  int       hash_table_size =                    8192;
  int       log_hash_table_size =                  12;
  int       pawn_hash_table_size =               4096;
  int       log_pawn_hash_table_size =             12;

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

  char      king_safety_x[128] =  {   0,   0,   0,   0,   0,   0,   0,   0,
                                      0,   0,   0,   0,   0,   0,   0,   0,
                                      2,   4,   6,   8,  10,  12,  14,  16,
                                     18,  20,  22,  24,  26,  28,  30,  32,
                                     32,  32,  32,  32,  32,  32,  32,  32,
                                     32,  32,  32,  32,  32,  32,  32,  32,
                                     32,  32,  32,  32,  32,  32,  32,  32,
                                     32,  32,  32,  32,  32,  32,  32,  32,
                                     32,  32,  32,  32,  32,  32,  32,  32,
                                     32,  32,  32,  32,  32,  32,  32,  32,
                                     32,  32,  32,  32,  32,  32,  32,  32,
                                     32,  32,  32,  32,  32,  32,  32,  32,
                                     32,  32,  32,  32,  32,  32,  32,  32,
                                     32,  32,  32,  32,  32,  32,  32,  32,
                                     32,  32,  32,  32,  32,  32,  32,  32,
                                     32,  32,  32,  32,  32,  32,  32,  32};
  
  int       outside_passed[128] = { 800, 800, 800, 600, 600, 500, 500, 500,
                                    400, 400, 200, 200, 200, 200, 100,  50,
                                     50,  50,  50,  50,  50,  50,  50,  50,
                                     50,  50,  50,  50,  50,  50,  50,  50,
                                     50,  50,  50,  50,  50,  50,  50,  50,
                                     50,  50,  50,  50,  50,  50,  50,  50,
                                     50,  50,  50,  50,  50,  50,  50,  50,
                                     50,  50,  50,  50,  50,  50,  50,  50,
                                     50,  50,  50,  50,  50,  50,  50,  50,
                                     50,  50,  50,  50,  50,  50,  50,  50,
                                     50,  50,  50,  50,  50,  50,  50,  50,
                                     50,  50,  50,  50,  50,  50,  50,  50,
                                     50,  50,  50,  50,  50,  50,  50,  50,
                                     50,  50,  50,  50,  50,  50,  50,  50,
                                     50,  50,  50,  50,  50,  50,  50,  50,
                                     50,  50,  50,  50,  50,  50,  50,  50};
    
  int       piece_values[8] =        {0,PAWN_VALUE,KNIGHT_VALUE,KING_VALUE,
                                      0, BISHOP_VALUE,ROOK_VALUE,QUEEN_VALUE};

  int       pawn_rams[9] =           {0,0,PAWN_RAM*2,PAWN_RAM*4,
                                      PAWN_RAM*6,PAWN_RAM*10,PAWN_RAM*13,
                                      PAWN_RAM*15,PAWN_RAM*18};
  int       cramping_pawn_rams[9] =  {0,PAWN_RAM*3,PAWN_RAM*6,PAWN_RAM*9,
                                      PAWN_RAM*12,PAWN_RAM*15,PAWN_RAM*18,
                                      PAWN_RAM*21,PAWN_RAM*24};
  int       bad_pawn_rams[9] =       {0,BAD_PAWN_RAM*3,BAD_PAWN_RAM*6,BAD_PAWN_RAM*9,
                                      BAD_PAWN_RAM*12,BAD_PAWN_RAM*15,BAD_PAWN_RAM*18,
                                      BAD_PAWN_RAM*21,BAD_PAWN_RAM*24};
  int       pawns_isolated[9] =      { 0,PAWN_ISOLATED, 3*PAWN_ISOLATED,
                                       6*PAWN_ISOLATED,10*PAWN_ISOLATED,
                                      14*PAWN_ISOLATED,17*PAWN_ISOLATED,
                                      20*PAWN_ISOLATED,20*PAWN_ISOLATED};

  char      square_color[64]  = { 1, 0, 1, 0, 1, 0, 1, 0,
                                  0, 1, 0, 1, 0, 1, 0, 1,
                                  1, 0, 1, 0, 1, 0, 1, 0,
                                  0, 1, 0, 1, 0, 1, 0, 1,
                                  1, 0, 1, 0, 1, 0, 1, 0,
                                  0, 1, 0, 1, 0, 1, 0, 1,
                                  1, 0, 1, 0, 1, 0, 1, 0,
                                  0, 1, 0, 1, 0, 1, 0, 1 };

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

  int       mate[64] =           {2800,2600,2400,2200,2200,2400,2600,2800,
                                  2600,1600,1400,1200,1200,1400,1600,2600,
                                  2400,1400, 400, 200, 200, 400,1400,2400,
                                  2200,1200, 200,   0,   0, 200,1200,2200,
                                  2200,1200, 200,   0,   0, 200,1200,2200,
                                  2400,1400, 400, 200, 200, 400,1400,2400,
                                  2600,1600,1400,1200,1200,1400,1600,2600,
                                  2800,2600,2400,2200,2200,2400,2600,2800};


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
#endif

  char            white_outpost[64] = { 0, 0, 0, 0, 0, 0, 0, 0,
                                        0, 0, 0, 0, 0, 0, 0, 0,
                                        0, 0, 0, 0, 0, 0, 0, 0,
                                        0, 0, 0, 0, 0, 0, 0, 0,
                                        0, 0, 2, 3, 3, 2, 0, 0,
                                        0, 1, 2, 3, 3, 2, 1, 0,
                                        0, 0, 0, 1, 1, 0, 0, 0,
                                        0, 0, 0, 0, 0, 0, 0, 0 };

  char            black_outpost[64] = { 0, 0, 0, 0, 0, 0, 0, 0,
                                        0, 0, 0, 1, 1, 0, 0, 0,
                                        0, 1, 2, 3, 3, 2, 1, 0,
                                        0, 0, 2, 3, 3, 2, 0, 0,
                                        0, 0, 0, 0, 0, 0, 0, 0,
                                        0, 0, 0, 0, 0, 0, 0, 0,
                                        0, 0, 0, 0, 0, 0, 0, 0,
                                        0, 0, 0, 0, 0, 0, 0, 0 };

  char           push_extensions[64] = { 0, 0, 0, 0, 0, 0, 0, 0,
                                         1, 1, 1, 1, 1, 1, 1, 1,
                                         0, 0, 0, 0, 0, 0, 0, 0,
                                         0, 0, 0, 0, 0, 0, 0, 0,
                                         0, 0, 0, 0, 0, 0, 0, 0,
                                         0, 0, 0, 0, 0, 0, 0, 0,
                                         1, 1, 1, 1, 1, 1, 1, 1,
                                         0, 0, 0, 0, 0, 0, 0, 0 };

  int  knight_value_w[64] = {-100,  -50,  -50,  -50,  -50,  -50,  -50, -100,
                              -50,  -50,    0,    0,    0,    0,  -50,  -50,
                              -50,    0,   50,   50,   50,   50,    0,  -50,
                              -50,    0,   50,   80,   80,   50,    0,  -50,
                              -50,    0,   80,   80,   80,   80,    0,  -50,
                              -50,    0,   80,   80,   80,   80,    0,  -50,
                             -500, -300,    0,    0,    0,    0, -300, -500,
                             -500, -400,  -50,  -50,  -50,  -50, -400, -500};

  int  knight_value_b[64] = {-500, -400,  -50,  -50,  -50,  -50, -400, -500,
                             -500, -300,    0,    0,    0,    0, -300, -500,
                              -50,    0,   80,   80,   80,   80,    0,  -50,
                              -50,    0,   80,   80,   80,   80,    0,  -50,
                              -50,    0,   50,   80,   80,   50,    0,  -50,
                              -50,    0,   50,   50,   50,   50,    0,  -50,
                              -50,  -50,    0,    0,    0,    0,  -50,  -50,
                             -100,  -50,  -50,  -50,  -50,  -50,  -50, -100};

  int  bishop_value_w[64] = { -50,    0,    0,    0,    0,    0,    0,  -50,
                                0,   50,   50,   50,   50,   50,   50,    0,
                               25,   50,   70,   70,   70,   70,   50,   25,
                               25,   50,   70,  100,  100,   70,   50,   25,
                               50,   50,   70,  100,  100,   70,   50,   50,
                               50,   50,   70,   70,   70,   70,   50,   50,
                               50,   50,   50,   50,   50,   50,   50,   50,
                               50,   50,   50,   50,   50,   50,   50,   50};

  int  bishop_value_b[64] = { 
                               50,   50,   50,   50,   50,   50,   50,   50,
                               50,   50,   50,   50,   50,   50,   50,   50,
                               50,   50,   70,   70,   70,   70,   50,   50,
                               50,   50,   70,  100,  100,   70,   50,   50,
                               25,   50,   70,  100,  100,   70,   50,   25,
                               25,   50,   70,   70,   70,   70,   50,   25,
                                0,   50,   50,   50,   50,   50,   50,    0,
                              -50,    0,    0,    0,    0,    0,    0,  -50};
 
  int    rook_value_w[64] = {   0,    0,    0,    0,    0,    0,    0,    0,
                             -100, -100,    0,    0,    0,    0, -100, -100,
                             -100, -100,    0,    0,    0,    0, -100, -100,
                             -100, -100,    0,    0,    0,    0, -100, -100,
                             -100, -100,    0,    0,    0,    0, -100, -100,
                             -100, -100,    0,    0,    0,    0, -100, -100,
                                0,    0,    0,    0,    0,    0,    0,    0,
                                0,    0,    0,    0,    0,    0,    0,    0};
 
  int    rook_value_b[64] = {   0,    0,    0,    0,    0,    0,    0,    0,
                                0,    0,    0,    0,    0,    0,    0,    0,
                             -100, -100,    0,    0,    0,    0, -100, -100,
                             -100, -100,    0,    0,    0,    0, -100, -100,
                             -100, -100,    0,    0,    0,    0, -100, -100,
                             -100, -100,    0,    0,    0,    0, -100, -100,
                             -100, -100,    0,    0,    0,    0, -100, -100,
                                0,    0,    0,    0,    0,    0,    0,    0};

  int   queen_value_w[64] = {-30, -30, -30, -30, -30, -30, -30, -30,
                             -30,   0,   0,   0,   0,   0,   0, -30,
                             -30,   0,  30,  30,  30,  30,   0, -30,
                             -30,   0,  30,  60,  60,  30,   0, -30,
                             -30,   0,  30,  60,  60,  30,   0, -30,
                             -30,   0,  30,  30,  30,  30,   0, -30,
                             -30,   0,   0,   0,   0,   0,   0, -30,
                             -30, -30, -30, -30, -30, -30, -30, -30};

  int   queen_value_b[64] = {-30, -30, -30, -30, -30, -30, -30, -30,
                             -30,   0,   0,   0,   0,   0,   0, -30,
                             -30,   0,  30,  30,  30,  30,   0, -30,
                             -30,   0,  30,  60,  60,  30,   0, -30,
                             -30,   0,  30,  60,  60,  30,   0, -30,
                             -30,   0,  30,  30,  30,  30,   0, -30,
                             -30,   0,   0,   0,   0,   0,   0, -30,
                             -30, -30, -30, -30, -30, -30, -30, -30};

  int    king_value_w[64] = {-300,-200,-100,-100,-100,-100,-200,-300,
                             -200,-200, -50, -50, -50, -50,-200,-200,
                             -150,-100,   0, 100, 100,   0,-100,-150,
                              -60,   0, 100, 200, 200, 100,   0, -60,
                              -40, 100, 300, 300, 300, 300, 100, -40,
                                0, 100, 400, 400, 400, 400, 100,   0,
                              -50, 100, 150, 150, 150, 150, 100, -50,
                             -150,-100,-100,-100,-100,-100,-100,-150};

  int    king_value_b[64] = {-150,-100,-100,-100,-100,-100,-100,-150,
                              -50, 100, 150, 150, 150, 150, 100, -50,
                                0, 100, 400, 400, 400, 400, 100,   0,
                              -40, 100, 300, 300, 300, 300, 100, -40,
                              -60,   0, 100, 200, 200, 100,   0, -60,
                             -150,-100,   0, 100, 100,   0,-100,-150,
                             -200,-200, -50, -50, -50, -50,-200,-200,
                             -300,-200,-200,-200,-200,-200,-200,-300};

   char king_defects_w[64]= { 4, 1, 3, 6, 6, 3, 0, 4,
                              5, 2, 5, 7, 7, 5, 1, 3,
                              6, 4, 6, 7, 7, 6, 4, 6,
                              8, 5, 6, 8, 8, 6, 5, 8,
                              8, 5, 6, 8, 8, 6, 5, 8,
                              8, 5, 6, 8, 8, 6, 5, 8,
                              8, 5, 6, 8, 8, 6, 5, 8,
                              8, 5, 6, 8, 8, 6, 5, 8};

   char king_defects_b[64]= { 8, 5, 6, 8, 8, 6, 5, 8,
                              8, 5, 6, 8, 8, 6, 5, 8,
                              8, 5, 6, 8, 8, 6, 5, 8,
                              8, 5, 6, 8, 8, 6, 5, 8,
                              8, 5, 6, 8, 8, 6, 5, 8,
                              6, 4, 6, 7, 7, 6, 4, 6,
                              5, 2, 5, 7, 7, 5, 1, 3,
                              4, 1, 3, 6, 6, 3, 0, 4};

   char is_corner[64] =      {1, 0, 0, 0, 0, 0, 0, 1,
                              0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0,
                              1, 0, 0, 0, 0, 0, 0, 1};

   int flight_sq[64][2] =   {{10,17},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{13,22},
                               {0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
                               {0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
                               {0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
                               {0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
                               {0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
                               {0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
                             {41,50},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{46,53}};
