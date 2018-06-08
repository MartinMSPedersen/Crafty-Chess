#include <stdio.h>
#include <stdlib.h>
#include "chess.h"
#include "evaluate.h"
  
   FILE           *input_stream;
   FILE           *book_file;
   FILE           *books_file;
   FILE           *history_file;
   FILE           *log_file;
   FILE           *auto_file;
   FILE           *book_lrn_file;
   FILE           *position_file;
   FILE           *position_lrn_file;
   char           whisper_text[512];
   int            whisper_value;
   int            whisper_depth;
   int            total_moves;
   int            last_mate_score;
   int            time_abort;
   int            auto232;
   int            auto232_delay;
   signed char    abort_search;
   char           log_filename[64];
   char           history_filename[64];
   int            number_of_solutions;
   int            solutions[10];
   int            solution_type;
   char           cmd_buffer[512];
   char           *args[32];
   char           buffer[512];
   int            nargs;
   int            iteration_depth;
   int            previous_search_value;
   int            search_failed_high;
   int            search_failed_low;
   int            root_alpha;
   int            root_beta;
   int            root_value;
   int            root_wtm;
   int            root_move;
   int            cpu_percent;
   int            easy_move;
   int            absolute_time_limit;
   int            search_time_limit;
   int            next_time_check;
   int            burp;
   int            ponder_move;
   int            made_predicted_move;
   int            ponder_moves[220];
   int            num_ponder_moves;
   unsigned int   opponent_start_time, opponent_end_time;
   unsigned int   program_start_time, program_end_time;
   unsigned int   start_time, end_time;
   unsigned int   elapsed_start, elapsed_end;
   int            book_move;
   int            book_learn_eval[LEARN_INTERVAL];
   int            book_learn_depth[LEARN_INTERVAL];
   int            hash_maska;
   int            hash_maskb;
   unsigned int   pawn_hash_mask;
   HASH_ENTRY      *trans_ref_a;
   HASH_ENTRY      *trans_ref_b;
   PAWN_HASH_ENTRY *pawn_hash_table;
   HASH_ENTRY      *trans_ref_a_orig;
   HASH_ENTRY      *trans_ref_b_orig;
   PAWN_HASH_ENTRY *pawn_hash_table_orig;
   int            history_w[4096], history_b[4096];
   PATH           last_pv;
   int            last_value;
   int            pawn_value_b[64];
   int            knight_value_b[64];
   int            bishop_value_b[64];
   int            rook_value_b[64];
   int            queen_value_b[64];
   int            king_value_b[64];
   signed char    directions[64][64];
   BITBOARD       w_pawn_attacks[64];
   BITBOARD       b_pawn_attacks[64];
   BITBOARD       knight_attacks[64];
   BITBOARD       bishop_attacks[64];

#if defined(COMPACT_ATTACKS)
  /* Stuff these into a structure to make the addressing slightly cheaper */
  struct at at;
  BITBOARD       diag_attack_bitboards[NDIAG_ATTACKS];
  BITBOARD       anti_diag_attack_bitboards[NDIAG_ATTACKS];
  DIAG_INFO      diag_info[64];
#else
  BITBOARD       bishop_attacks_rl45[64][256];
  BITBOARD       bishop_attacks_rr45[64][256];
  int            bishop_mobility_rl45[64][256];
  int            bishop_mobility_rr45[64][256];
#endif
  BITBOARD       rook_attacks[64];
#if !defined(COMPACT_ATTACKS)
  BITBOARD       rook_attacks_r0[64][256];
  BITBOARD       rook_attacks_rl90[64][256];
  int            rook_mobility_r0[64][256];
  int            rook_mobility_rl90[64][256];
#endif

   POSITION       display;
   BITBOARD       queen_attacks[64];
   BITBOARD       king_attacks[64];
   BITBOARD       king_attacks_1[64];
   BITBOARD       king_attacks_2[64];
   BITBOARD       obstructed[64][64];
   unsigned int   w_pawn_random32[64];
   unsigned int   b_pawn_random32[64];
   BITBOARD       w_pawn_random[64];
   BITBOARD       b_pawn_random[64];
   BITBOARD       w_knight_random[64];
   BITBOARD       b_knight_random[64];
   BITBOARD       w_bishop_random[64];
   BITBOARD       b_bishop_random[64];
   BITBOARD       w_rook_random[64];
   BITBOARD       b_rook_random[64];
   BITBOARD       w_queen_random[64];
   BITBOARD       b_queen_random[64];
   BITBOARD       w_king_random[64];
   BITBOARD       b_king_random[64];
   BITBOARD       enpassant_random[65];
   BITBOARD       castle_random_w[2];
   BITBOARD       castle_random_b[2];
   BITBOARD       wtm_random[2];
   BITBOARD       clear_mask[65];
   BITBOARD       clear_mask_rl90[65];
   BITBOARD       clear_mask_rl45[65];
   BITBOARD       clear_mask_rr45[65];
   BITBOARD       set_mask[65];
   BITBOARD       set_mask_rl90[65];
   BITBOARD       set_mask_rl45[65];
   BITBOARD       set_mask_rr45[65];
   BITBOARD       file_mask[8];
   BITBOARD       rank_mask[8];
   BITBOARD       right_side_mask[8];
   BITBOARD       left_side_mask[8];
   BITBOARD       right_side_empty_mask[8];
   BITBOARD       left_side_empty_mask[8];
   BITBOARD       mask_fgh, mask_abc;
   BITBOARD       mask_abs7_w, mask_abs7_b;
   BITBOARD       mask_advance_2_w;
   BITBOARD       mask_advance_2_b;
   BITBOARD       mask_left_edge;
   BITBOARD       mask_right_edge;
   BITBOARD       mask_G2G3;
   BITBOARD       mask_B2B3;
   BITBOARD       mask_G6G7;
   BITBOARD       mask_B6B7;
   BITBOARD       mask_A7H7;
   BITBOARD       mask_A2H2;
   BITBOARD       stonewall_white;
   BITBOARD       stonewall_black;
   BITBOARD       closed_white;
   BITBOARD       closed_black;
   BITBOARD       mask_kr_trapped_w[3];
   BITBOARD       mask_qr_trapped_w[3];
   BITBOARD       mask_kr_trapped_b[3];
   BITBOARD       mask_qr_trapped_b[3];
   BITBOARD       good_bishop_kw;
   BITBOARD       good_bishop_qw;
   BITBOARD       good_bishop_kb;
   BITBOARD       good_bishop_qb;
   BITBOARD       light_squares;
   BITBOARD       dark_squares;
   BITBOARD       not_rook_pawns;
   BITBOARD       plus1dir[65];
   BITBOARD       plus7dir[65];
   BITBOARD       plus8dir[65];
   BITBOARD       plus9dir[65];
   BITBOARD       minus1dir[65];
   BITBOARD       minus7dir[65];
   BITBOARD       minus8dir[65];
   BITBOARD       minus9dir[65];
   BITBOARD       mask_eptest[64];

#  if !defined(CRAY1)
     BITBOARD       mask_1;
     BITBOARD       mask_2;
     BITBOARD       mask_3;
     BITBOARD       mask_8;
     BITBOARD       mask_16;
     BITBOARD       mask_112;
     BITBOARD       mask_120;
#  endif

   BITBOARD       mask_clear_entry;

#  if !defined(CRAY1) && !defined(USE_ASSEMBLY_B)
     unsigned char  first_ones[65536];
     unsigned char  last_ones[65536];
#  endif

   unsigned char  first_ones_8bit[256];
   unsigned char  last_ones_8bit[256];
   unsigned char  connected_passed[256];
   BITBOARD       mask_pawn_protected_b[64];
   BITBOARD       mask_pawn_protected_w[64];
   BITBOARD       mask_pawn_duo[64];
   BITBOARD       mask_pawn_isolated[64];
   BITBOARD       mask_pawn_passed_w[64];
   BITBOARD       mask_pawn_passed_b[64];
   BITBOARD       mask_no_pawn_attacks_w[64];
   BITBOARD       mask_no_pawn_attacks_b[64];
   BITBOARD       white_minor_pieces;
   BITBOARD       black_minor_pieces;
   BITBOARD       white_pawn_race_wtm[64];
   BITBOARD       white_pawn_race_btm[64];
   BITBOARD       black_pawn_race_wtm[64];
   BITBOARD       black_pawn_race_btm[64];
   BITBOARD       mask_wk_4th, mask_wq_4th, mask_bk_4th, mask_bq_4th;
   BOOK_POSITION  book_buffer[BOOK_CLUSTER_SIZE];
   BOOK_POSITION  books_buffer[BOOK_CLUSTER_SIZE];
   unsigned int   thread_start_time[CPUS];

#  if defined(SMP)
   TREE           *local[MAX_BLOCKS+1];
   TREE           *volatile thread[CPUS];
   lock_t         lock_hasha, lock_hashb, lock_pawn_hash;
   lock_t         lock_smp, lock_io;
   pthread_attr_t pthread_attr;
#  else
   TREE           *local[1];
#  endif
   unsigned int   parallel_splits;
   unsigned int   parallel_stops;
   unsigned int   max_split_blocks;
   volatile unsigned int   splitting;

# define    VERSION                             "16.4"
  char      version[6] =                    {VERSION};
  PLAYING_MODE mode =                     normal_mode;

#if defined(SMP)
  volatile int smp_idle =                           0;
  volatile int smp_threads =                        0;
#endif

  int       batch_mode =                            0; /* no asynch reads */
  int       swindle_mode =                          1; /* try to swindle draws */
  int       call_flag =                             0;
  int       crafty_rating =                      2500;
  int       opponent_rating =                    2500;
  int       last_search_value =                     0;
  int       DGT_active =                            0;
  int       to_dgt =                                0;
  int       from_dgt =                              0;
  int       pgn_suggested_percent =                 0;
  char      pgn_event[32] = {"?"};
  char      pgn_site[32] = {"?"};
  char      pgn_date[32] = {"????.??.??"};
  char      pgn_round[32] = {"?"};
  char      pgn_white[64] = {"unknown"};
  char      pgn_white_elo[32] = {""};
  char      pgn_black[64] = {"Crafty " VERSION};
  char      pgn_black_elo[32] = {""};
  char      pgn_result[32] = {"*"};

  int       number_of_specials =                    3;

  char      special_list[512][20] = {
                                      {"bughousemansion"},
                                      {"mercilous"},
                                      {"crissaegrim"}};

  int       number_auto_kibitzers =                11;

  char      auto_kibitz_list[64][20] = {
                                      {"diepx"},
                                      {"ferret"},
                                      {"hossa"},
                                      {"judgeturpin"},
                                      {"knightc"},
                                      {"knightcap"},
                                      {"mink"},
                                      {"otter"},
                                      {"rajah"},
                                      {"tcb"},
                                      {"zarkovx"}};
  

  int       number_of_computers =                   0;
  char      computer_list[512][20] = {
                                      {""}};

  int       number_of_GMs =                       125;
  char      GM_list[512][20] =       {{"agdestein"},
                                      {"airgun"},
                                      {"alexandrel"},
                                      {"anat"},
                                      {"andersson"},
                                      {"anxions"},
                                      {"ariela"},
                                      {"attackgm"},
                                      {"babyboss"},
                                      {"badviking"},
                                      {"bareev"},
                                      {"baskmask"},
                                      {"benyehuda"},
                                      {"berliner"},
                                      {"bigd"},
                                      {"blatny"},
                                      {"blow"},
                                      {"bluedog"},
                                      {"buzzcut"},
                                      {"cambala"},
                                      {"conguito"},
                                      {"davenogood"},
                                      {"davinci"},
                                      {"deep-blue"},
                                      {"dgurevich"},
                                      {"dirtyharry"},
                                      {"dixon"},
                                      {"dlugy"},
                                      {"dr"},
                                      {"dranov"},
                                      {"dumbo"},
                                      {"feeai"},
                                      {"figo"},
                                      {"flamingskull"},
                                      {"fosti"},
                                      {"fright"},
                                      {"gasch"},
                                      {"gatakamsky"},
                                      {"gatotkaca"},
                                      {"gelfand"},
                                      {"geysir"},
                                      {"gmalex"},
                                      {"gmdavies"},
                                      {"gmsoffer"},
                                      {"gref"},
                                      {"gulko"},
                                      {"handokoe"},
                                      {"harzvi"},
                                      {"heine"},
                                      {"helgi"},
                                      {"henley"},
                                      {"hernandez"},
                                      {"hugo"},
                                      {"infochess"},
                                      {"inov"},
                                      {"jacaglaca"},
                                      {"jantonio"},
                                      {"jaque13"},
                                      {"judgedredd"},
                                      {"juditpolgar"},
                                      {"juliogranda"},
                                      {"julius"},
                                      {"junior"},
                                      {"kaidanov"},
                                      {"karpov"},
                                      {"kasparov"},
                                      {"kc"},
                                      {"kempka"},
                                      {"kevlar"},
                                      {"kingloek"},
                                      {"knez"},
                                      {"kotronias"},
                                      {"kramnik"},
                                      {"krogius"},
                                      {"kudrin"},
                                      {"lein"},
                                      {"leon"},
                                      {"leop"},
                                      {"lev"},
                                      {"levalburt"},
                                      {"lombardy"},
                                      {"lycan"},
                                      {"margeir"},
                                      {"mgur"},
                                      {"moggy"},
                                      {"mquinteros"},
                                      {"mysko"},
                                      {"nikolaidis"},
                                      {"olafsson"},
                                      {"phips"},
                                      {"psakhis"},
                                      {"racp"},
                                      {"ree"},
                                      {"repref"},
                                      {"ricardi"},
                                      {"rohde"},
                                      {"sagalchik"},
                                      {"sagdestein"},
                                      {"scratchy"},
                                      {"securitron"},
                                      {"seinfeld"},
                                      {"serper"},
                                      {"shamkovich"},
                                      {"shirov"},
                                      {"short"},
                                      {"sorokin"},
                                      {"spangenberg"},
                                      {"spanish-champ"},
                                      {"spicegirl"},
                                      {"stefansson"},
                                      {"sweere"},
                                      {"taimanov"},
                                      {"tigre"},
                                      {"tioro"},
                                      {"tisdall"},
                                      {"topalov"},
                                      {"ttivel"},
                                      {"uandersson"},
                                      {"vagr"},
                                      {"wbrowne"},
                                      {"wilder"},
                                      {"wojtkiewicz"},
                                      {"yan"},
                                      {"yermo"},
                                      {"younglasker"}};

  int       number_of_IMs =                        11;
  char      IM_list[512][20] =       {
                                      {"adolf"},
                                      {"badviking"},
                                      {"bandora"},
                                      {"imorlov"},
                                      {"impolzin"},
                                      {"laflair"},
                                      {"lsokol"},
                                      {"oed"},
                                      {"thutters"},
                                      {"thumpster"},
                                      {"tim"}};

  int       ics =                                   0;
  int       output_format =                         0;
  int       EGTBlimit =                             0;
  int       EGTB_use =                              0;
  int       EGTB_draw =                             0;
  int       EGTB_cache_size =      EGTB_CACHE_DEFAULT;
  void      *EGTB_cache =                    (void*)0;
  int       EGTB_maxdepth =                        32;
  int       xboard =                                0;
  int       whisper =                               0;
  int       channel =                               0;
  int       early_exit =                           99;
  int       new_game =                              0;
  char      channel_title[32] =                  {""};
  char      book_path[128] =                {BOOKDIR};
  char      log_path[128] =                  {LOGDIR};
  char      tb_path[128] =                    {TBDIR};
  int       initialized =                           0;
  int       kibitz =                                0;
  int       post =                                  0;
  int       log_id =                                1;
  int       move_number =                           1;
  int       wtm =                                   1;
  int       crafty_is_white =                       0;
  int       last_opponent_move =                    0;
  int       average_nps =                           0;
  int       incheck_depth =                        60;
  int       onerep_depth =                         45;
  int       recap_depth =                          45;
  int       pushpp_depth =                         45;
  int       threat_depth =                         45;
  int       singular_depth =                       45;
  int       largest_positional_score =            100;
  int       search_depth =                          0;
  int       search_move =                           0;
  TIME_TYPE time_type =                       elapsed;
  int       nodes_between_time_checks =         10000;
  int       nodes_per_second =                  10000;
  int       predicted =                             0;

  int       time_used =                             0;
  int       time_used_opponent =                    0;
  int       cpu_time_used =                         0;
  signed char transposition_id =                    0;

  int       opening =                               1;
  int       middle_game =                           0;
  int       end_game =                              0;
  signed char thinking =                            0;
  signed char pondering =                           0;
  signed char puzzling =                            0;
  signed char booking =                             0;
  int       analyze_mode =                          0;
  int       annotate_mode =                         0;
  int       test_mode =                             0;
  int       analyze_move_read =                     0;
  signed char resign =                              9;
  signed char resign_counter =                      0;
  signed char resign_count =                        5;
  signed char draw_counter =                        0;
  signed char draw_count =                         10;
  int       tc_moves =                             60;
  int       tc_time =                          180000;
  int       tc_time_remaining =                180000;
  int       tc_time_remaining_opponent =       180000;
  int       tc_moves_remaining =                   60;
  int       tc_secondary_moves =                   30;
  int       tc_secondary_time =                 90000;
  int       tc_increment =                          0;
  int       tc_sudden_death =                       0;
  int       tc_operator_time =                      0;
  int       tc_safety_margin =                      0;
  int       time_limit =                          100;
  int       force =                                 0;
  char      initial_position[80] =               {""};
  char      hint[512] =                          {""};
  char      book_hint[512] =                     {""};
  int       over =                                  0;
  int       trojan_check =                          0;
  int       computer_opponent =                     0;
  int       usage_level =                           0;
  char      audible_alarm =                      0x07;
#if defined(MACOS)
  int       ansi =                                  0;
#else
  int       ansi =                                  1;
#endif
  int       book_accept_mask =                    ~03;
  int       book_reject_mask =                      3;
  int       book_random =                           1;
  float     book_weight_freq =                    1.0;
  float     book_weight_eval =                    0.1;
  float     book_weight_learn =                   0.3;
  int       book_search_trigger =                  20;
  int       learning =                              7;
  int       moves_out_of_book =                     0;
  int       show_book =                             0;
  int       book_selection_width =                  5;
  int       ponder =                                1;
  int       trace_level =                           0;
  int       display_options =                    4095;
  int       max_threads =                           0;
  int       min_thread_depth =        2*INCPLY;
  unsigned int noise_level =                    10000;
 
  int       hash_table_size =                   65536;
  int       log_hash =                             16;
  int       pawn_hash_table_size =              32768;
  int       log_pawn_hash =                        15;

  int       draw_score =                            0;
  int       accept_draws =                          0;

  const char xlate[15] = {'q','r','b',0,'k','n','p',0,'P','N','K',0,'B','R','Q'};
  const char empty[9]  = {' ','1','2','3','4','5','6','7','8'};

  const int passed_pawn_value[8] = { 0,
                                     PAWN_PASSED, PAWN_PASSED*2,
                                     PAWN_PASSED*3, PAWN_PASSED*4,
                                     PAWN_PASSED*6,PAWN_PASSED*9,
                                     0};

  const int isolated_pawn_value[9] = { 0,
                                       PAWN_ISOLATED,   PAWN_ISOLATED*2,
                                       PAWN_ISOLATED*4, PAWN_ISOLATED*6,
                                       PAWN_ISOLATED*9,PAWN_ISOLATED*12,
                                       PAWN_ISOLATED*15,PAWN_ISOLATED*20};

  const int supported_passer[8] =  { 0,
                                     PAWN_SUPPORTED_PASSED_RANK2,
                                     PAWN_SUPPORTED_PASSED_RANK3,
                                     PAWN_SUPPORTED_PASSED_RANK4,
                                     PAWN_SUPPORTED_PASSED_RANK5,
                                     PAWN_SUPPORTED_PASSED_RANK6,
                                     PAWN_SUPPORTED_PASSED_RANK7,
                                     0};

  const int reduced_material_passer[20] = { 10,10,9,9,8,8,7,7,6,6,
                                             5,5,4,4,3,3,2,2,1,1};

  const int outside_passed[128] = { 80, 50, 50, 50, 45, 42, 40, 40,
                                    36, 36, 32, 32, 28, 28, 24, 24,
                                    20, 20, 16, 16, 12, 12, 12, 12,
                                    12, 12, 12, 12, 12, 12, 12, 12,
                                    12, 12, 12, 12, 12, 12, 12, 12,
                                    12, 12, 12, 12, 12, 12, 12, 12,
                                    12, 12, 12, 12, 12, 12, 12, 12,
                                    12, 12, 12, 12, 12, 12, 12, 12,
                                    12, 12, 12, 12, 12, 12, 12, 12,
                                    12, 12, 12, 12, 12, 12, 12, 12,
                                    12, 12, 12, 12, 12, 12, 12, 12,
                                    12, 12, 12, 12, 12, 12, 12, 12,
                                    12, 12, 12, 12, 12, 12, 12, 12,
                                    12, 12, 12, 12, 12, 12, 12, 12,
                                    12, 12, 12, 12, 12, 12, 12, 12,
                                    12, 12, 12, 12, 12, 12, 12, 12};

  const int scale_up[128] =       { 12, 12, 12, 11, 11, 11, 10, 10,
                                    10,  9,  9,  9,  8,  8,  8,  7,
                                     6,  6,  6,  6,  6,  6,  6,  6,
                                     6,  6,  6,  6,  6,  6,  6,  6,
                                     6,  6,  6,  6,  6,  6,  6,  6,
                                     6,  6,  6,  6,  6,  6,  6,  6,
                                     6,  6,  6,  6,  6,  6,  6,  6,
                                     6,  6,  6,  6,  6,  6,  6,  6,
                                     6,  6,  6,  6,  6,  6,  6,  6,
                                     6,  6,  6,  6,  6,  6,  6,  6,
                                     6,  6,  6,  6,  6,  6,  6,  6,
                                     6,  6,  6,  6,  6,  6,  6,  6,
                                     6,  6,  6,  6,  6,  6,  6,  6,
                                     6,  6,  6,  6,  6,  6,  6,  6,
                                     6,  6,  6,  6,  6,  6,  6,  6,
                                     6,  6,  6,  6,  6,  6,  6,  6};

  const int scale_down[128] =     {  1,  1,  1,  1,  1,  1,  1,  1,
                                     1,  1,  1,  1,  2,  3,  4,  5,
                                     5,  6,  6,  7,  7,  8,  8,  8,
                                     9,  9,  9, 10, 10, 10, 10, 10,
                                    10, 10, 10, 10, 10, 10, 10, 10,
                                    10, 10, 10, 10, 10, 10, 10, 10,
                                    10, 10, 10, 10, 10, 10, 10, 10,
                                    10, 10, 10, 10, 10, 10, 10, 10,
                                    10, 10, 10, 10, 10, 10, 10, 10,
                                    10, 10, 10, 10, 10, 10, 10, 10,
                                    10, 10, 10, 10, 10, 10, 10, 10,
                                    10, 10, 10, 10, 10, 10, 10, 10,
                                    10, 10, 10, 10, 10, 10, 10, 10,
                                    10, 10, 10, 10, 10, 10, 10, 10,
                                    10, 10, 10, 10, 10, 10, 10, 10,
                                    10, 10, 10, 10, 10, 10, 10, 10};

  const int temper[128] =         {  0,  1,  2,  5,  8, 10, 10, 11,
                                    11, 12, 12, 13, 13, 14, 14, 15,
                                    15, 16, 16, 17, 17, 18, 18, 19,
                                    19, 20, 20, 20, 20, 20, 20, 20,
                                    20, 20, 20, 20, 20, 20, 20, 20,
                                    20, 20, 20, 20, 20, 20, 20, 20,
                                    20, 20, 20, 20, 20, 20, 20, 20,
                                    20, 20, 20, 20, 20, 20, 20, 20,
                                    20, 20, 20, 20, 20, 20, 20, 20,
                                    20, 20, 20, 20, 20, 20, 20, 20,
                                    20, 20, 20, 20, 20, 20, 20, 20,
                                    20, 20, 20, 20, 20, 20, 20, 20,
                                    20, 20, 20, 20, 20, 20, 20, 20,
                                    20, 20, 20, 20, 20, 20, 20, 20,
                                    20, 20, 20, 20, 20, 20, 20, 20,
                                    20, 20, 20, 20, 20, 20, 20, 20};

  const char square_color[64]  = { 1, 0, 1, 0, 1, 0, 1, 0,
                                   0, 1, 0, 1, 0, 1, 0, 1,
                                   1, 0, 1, 0, 1, 0, 1, 0,
                                   0, 1, 0, 1, 0, 1, 0, 1,
                                   1, 0, 1, 0, 1, 0, 1, 0,
                                   0, 1, 0, 1, 0, 1, 0, 1,
                                   1, 0, 1, 0, 1, 0, 1, 0,
                                   0, 1, 0, 1, 0, 1, 0, 1 };

  const int b_n_mate_dark_squares[64] = 
                             { 100,  90,  80,  70,  60,  50,  40,  30,
                                90,  80,  70,  60,  50,  40,  30,  40,
                                80,  70,  60,  50,  40,  30,  40,  50,
                                70,  60,  50,  40,  30,  40,  50,  60,
                                60,  50,  40,  30,  40,  50,  60,  70,
                                50,  40,  30,  40,  50,  60,  70,  80,
                                40,  30,  40,  50,  60,  70,  80,  90,
                                30,  40,  50,  60,  70,  80,  90, 100};

  const int b_n_mate_light_squares[64] =
                             {  30,  40,  50,  60,  70,  80,  90, 100,
                                40,  30,  40,  50,  60,  70,  80,  90,
                                50,  40,  30,  40,  50,  60,  70,  80,
                                60,  50,  40,  30,  40,  50,  60,  70,
                                70,  60,  50,  40,  30,  40,  50,  60,
                                80,  70,  60,  50,  40,  30,  40,  50,
                                90,  80,  70,  60,  50,  40,  30,  40,
                               100,  90,  80,  70,  60,  50,  40,  30};

  const int mate[64] =        {100, 96, 93, 90, 90, 93, 96,100,
                                96, 80, 70, 60, 60, 70, 80, 96,
                                93, 70, 60, 50, 50, 60, 70, 93,
                                90, 60, 50, 40, 40, 50, 60, 90,
                                90, 60, 50, 40, 40, 50, 60, 90,
                                93, 70, 60, 50, 50, 60, 70, 93,
                                96, 80, 70, 60, 60, 70, 80, 96,
                               100, 96, 93, 90, 90, 93, 96,100};

  const char      white_outpost[64] = { 0, 0, 0, 0, 0, 0, 0, 0,
                                        0, 0, 0, 0, 0, 0, 0, 0,
                                        0, 0, 0, 0, 0, 0, 0, 0,
                                        0, 0, 0, 2, 2, 0, 0, 0,
                                        0, 0, 4, 5, 5, 4, 0, 0,
                                        0, 0, 3, 6, 6, 3, 0, 0,
                                        0, 0, 0, 1, 1, 0, 0, 0,
                                        0, 0, 0, 0, 0, 0, 0, 0 };

  const char      black_outpost[64] = { 0, 0, 0, 0, 0, 0, 0, 0,
                                        0, 0, 0, 1, 1, 0, 0, 0,
                                        0, 0, 3, 6, 6, 3, 0, 0,
                                        0, 0, 4, 5, 5, 4, 0, 0,
                                        0, 0, 0, 2, 2, 0, 0, 0,
                                        0, 0, 0, 0, 0, 0, 0, 0,
                                        0, 0, 0, 0, 0, 0, 0, 0,
                                        0, 0, 0, 0, 0, 0, 0, 0 };

  const char     push_extensions[64] = { 0, 0, 0, 0, 0, 0, 0, 0,
                                         1, 1, 1, 1, 1, 1, 1, 1,
                                         0, 0, 0, 0, 0, 0, 0, 0,
                                         0, 0, 0, 0, 0, 0, 0, 0,
                                         0, 0, 0, 0, 0, 0, 0, 0,
                                         0, 0, 0, 0, 0, 0, 0, 0,
                                         1, 1, 1, 1, 1, 1, 1, 1,
                                         0, 0, 0, 0, 0, 0, 0, 0 };

  int    pawn_value_w[64] = { 0,  0,  0,  0,  0,  0,  0,  0,
                              0,  0,  0,  0,  0,  0,  0,  0,
                              0,  0,  0,  0,  0,  0,  0,  0,
                              0,  0,  0,  6,  6,  0,  0,  0,
                              0,  0,  3,  6,  6,  0,  0,  0,
                              0,  0,  3,  3,  3,  0,  0,  0,
                              0,  0,  0,  0,  0,  0,  0,  0,
                              0,  0,  0,  0,  0,  0,  0,  0};

  int  knight_value_w[64] = {-15,-15, -3, -3, -3, -3,-15,-15,
                             -15,-15,  0,  0,  0,  0,-15,-15,
                              -5,  2,  2,  2,  2,  2,  2, -5,
                               0,  2,  4,  4,  4,  4,  2,  0,
                               0,  2,  6,  6,  6,  6,  2,  0,
                               0,  2,  6,  6,  6,  6,  2,  0,
                             -25,-25,  4,  4,  4,  4,-25,-25,
                             -25,-25,  2,  2,  2,  2,-25,-25};

  int  bishop_value_w[64] = {-15,-15, -3, -3, -3, -3,-15,-15,
                             -15,  2,  0,  0,  0,  0,  2,-15,
                               0,  2,  2,  2,  2,  2,  2,  0,
                               0,  2,  4,  4,  4,  4,  2,  0,
                               0,  2,  6,  6,  6,  6,  2,  0,
                               0,  2,  6,  6,  6,  6,  2,  0,
                             -15,  2,  4,  4,  4,  4,  2,-15,
                             -15,-15,  0,  0,  0,  0,-15,-15};

  int    rook_value_w[64] = {  1,  2,  4,  6,  6,  4,  2,  1,
                              -8,  2,  4,  6,  6,  4,  2, -8,
                              -8,  2,  4,  6,  6,  4,  2, -8,
                              -8,  2,  4,  6,  6,  4,  2, -8,
                               0,  2,  4,  6,  6,  4,  2,  0,
                               0,  2,  4,  6,  6,  4,  2,  0,
                               0,  2,  4,  6,  6,  4,  2,  0,
                               0,  2,  4,  6,  6,  4,  2,  0};

  int   queen_value_w[64] = {-10, -8,  0,  0,  0,  0, -8,-10,
                             -10,  2,  8,  8,  8,  8,  2,-10,
                             -10,-10,  4, 10, 10,  4,-10,-10,
                             -10,  2, 10, 12, 12, 10,  2,-10,
                               0,  2, 10, 12, 12, 10,  2,  0,
                               0,  2,  4, 10, 10,  4,  2,  0,
                               0,  2,  4,  5,  5,  4,  2,  0,
                               0,  0,  0,  0,  0,  0,  0,  0};

  int    king_value_w[64] = {-50,-40,-30,-20,-20,-30,-40,-50,
                             -30,-20,-10,  0,  0,-10,-20,-30,
                             -30,-10,  0, 10, 10,  0,-10,-30,
                             -30,-10, 10, 20, 20, 10,-10,-30,
                             -30,-10, 20, 30, 30, 20,-10,-30,
                             -30,-10, 20, 30, 30, 20,-10,-30,
                             -30,-30,-10,-10,-10,-10,-30,-30,
                             -50,-30,-30,-30,-30,-30,-30,-50};

/* note that black piece/square values are copied from white, but
   reflected */

   const char king_defects_w[64]= { 1, 0, 1, 3, 3, 1, 0, 1,
                                    1, 1, 1, 3, 3, 1, 1, 1,
                                    3, 3, 3, 3, 3, 3, 3, 3,
                                    4, 4, 4, 4, 4, 4, 4, 4,
                                    5, 5, 5, 5, 5, 5, 5, 5,
                                    6, 6, 6, 6, 6, 6, 6, 6,
                                    7, 7, 7, 7, 7, 7, 7, 7,
                                    8, 8, 8, 8, 8, 8, 8, 8};

   const char king_defects_b[64]= { 8, 8, 8, 8, 8, 8, 8, 8,
                                    7, 7, 7, 7, 7, 7, 7, 7,
                                    6, 6, 6, 6, 6, 6, 6, 6,
                                    5, 5, 5, 5, 5, 5, 5, 5,
                                    4, 4, 4, 4, 4, 4, 4, 4,
                                    3, 3, 3, 3, 3, 3, 3, 3,
                                    1, 1, 1, 3, 3, 1, 1, 1,
                                    1, 0, 1, 3, 3, 1, 0, 1};

  const int p_values[15] =       {QUEEN_VALUE,ROOK_VALUE,BISHOP_VALUE,0,
                                  KING_VALUE,KNIGHT_VALUE,PAWN_VALUE,
                                  0,PAWN_VALUE,KNIGHT_VALUE,KING_VALUE,
                                  0, BISHOP_VALUE,ROOK_VALUE,QUEEN_VALUE};

  const int unblocked_pawns[9] = {-PAWN_UNBLOCKED*2,0,PAWN_UNBLOCKED*2,
                                   PAWN_UNBLOCKED*3,  PAWN_UNBLOCKED*4,
                                   PAWN_UNBLOCKED*4,  PAWN_UNBLOCKED*4,
                                   PAWN_UNBLOCKED*4,  PAWN_UNBLOCKED*4};

#if !defined(COMPACT_ATTACKS)
  int            bishop_shift_rl45[64] = { 63, 61, 58, 54, 49, 43, 36, 28,
                                           61, 58, 54, 49, 43, 36, 28, 21,
                                           58, 54, 49, 43, 36, 28, 21, 15,
                                           54, 49, 43, 36, 28, 21, 15, 10,
                                           49, 43, 36, 28, 21, 15, 10,  6,
                                           43, 36, 28, 21, 15, 10,  6,  3,
                                           36, 28, 21, 15, 10,  6,  3,  1,
                                           28, 21, 15, 10,  6,  3,  1,  0 };

  int            bishop_shift_rr45[64] = { 28, 36, 43, 49, 54, 58, 61, 63,
                                           21, 28, 36, 43, 49, 54, 58, 61,
                                           15, 21, 28, 36, 43, 49, 54, 58,
                                           10, 15, 21, 28, 36, 43, 49, 54,
                                            6, 10, 15, 21, 28, 36, 43, 49,
                                            3,  6, 10, 15, 21, 28, 36, 43,
                                            1,  3,  6, 10, 15, 21, 28, 36,
                                            0,  1,  3,  6, 10, 15, 21, 28 };
#endif
