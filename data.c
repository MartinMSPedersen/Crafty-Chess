#include <stdio.h>
#include <stdlib.h>
#include "chess.h"
#include "evaluate.h"
FILE     *input_stream;
FILE     *book_file;
FILE     *books_file;
FILE     *normal_bs_file;
FILE     *computer_bs_file;
FILE     *history_file;
FILE     *log_file;
FILE     *auto_file;
FILE     *book_lrn_file;
FILE     *position_file;
FILE     *position_lrn_file;
char      kibitz_text[512];
int       kibitz_depth;
int       done = 0;
BITBOARD  total_moves;
int       last_mate_score;
int       time_abort;
signed char abort_search;
char      log_filename[64];
char      history_filename[64];
int       number_of_solutions;
int       solutions[10];
int       solution_type;
char      cmd_buffer[4096];
char     *args[32];
char      buffer[512];
int       nargs;
int       iteration_depth;
int       root_alpha;
int       root_beta;
int       last_root_value;
int       ponder_value;
int       root_value;
int       root_wtm;
int       root_print_ok;
int       move_actually_played;
ROOT_MOVE root_moves[256];
int       n_root_moves;
int       cpu_percent;
int       easy_move;
int       absolute_time_limit;
int       search_time_limit;
int       burp;
int       ponder_move;
int       ponder_moves[220];
int       num_ponder_moves;
volatile int quit;
unsigned int opponent_start_time, opponent_end_time;
unsigned int program_start_time, program_end_time;
unsigned int start_time, end_time;
unsigned int elapsed_start, elapsed_end;
int       book_move;
int       book_learn_eval[LEARN_INTERVAL];
int       book_learn_depth[LEARN_INTERVAL];
int       hash_mask;
unsigned int pawn_hash_mask;
HASH_ENTRY *trans_ref;
PAWN_HASH_ENTRY *pawn_hash_table;
HASH_ENTRY *trans_ref_orig;
size_t    cb_trans_ref;
PAWN_HASH_ENTRY *pawn_hash_table_orig;
size_t    cb_pawn_hash_table;
PATH      last_pv;
int       last_value;
int       temper_b[64], temper_w[64];
signed char pval_b[64];
signed char nval_b[64];
signed char bval_b[64];
signed char rval_b[64];
signed char qval_b[64];
signed char kval_bn[64];
signed char kval_bk[64];
signed char kval_bq[64];
signed char black_outpost[64];
signed char king_defects_b[64];
signed char directions[64][64];
int       tropism[128];
char      pawn_rams[9];
BITBOARD  w_pawn_attacks[64];
BITBOARD  b_pawn_attacks[64];
BITBOARD  knight_attacks[64];
BITBOARD  bishop_attacks[64];
BITBOARD  bishop_attacks_rl45[64][64];
BITBOARD  bishop_attacks_rr45[64][64];
BITBOARD  rook_attacks_r0[64][64];
BITBOARD  rook_attacks_rl90[64][64];
int       bishop_mobility_rl45[64][64];
int       bishop_mobility_rr45[64][64];
int       rook_mobility_r0[64][64];
int       rook_mobility_rl90[64][64];
BITBOARD  rook_attacks[64];
POSITION  display;
BITBOARD  queen_attacks[64];
BITBOARD  king_attacks[64];
BITBOARD  king_attacks_1[64];
BITBOARD  king_attacks_2[64];
BITBOARD  obstructed[64][64];
BITBOARD  w_pawn_random[64];
BITBOARD  b_pawn_random[64];
BITBOARD  w_knight_random[64];
BITBOARD  b_knight_random[64];
BITBOARD  w_bishop_random[64];
BITBOARD  b_bishop_random[64];
BITBOARD  w_rook_random[64];
BITBOARD  b_rook_random[64];
BITBOARD  w_queen_random[64];
BITBOARD  b_queen_random[64];
BITBOARD  w_king_random[64];
BITBOARD  b_king_random[64];
BITBOARD  stalemate_sqs[64];
BITBOARD  edge_moves[64];
BITBOARD  enpassant_random[65];
BITBOARD  castle_random_w[2];
BITBOARD  castle_random_b[2];
BITBOARD  wtm_random[2];
BITBOARD  clear_mask[65];
BITBOARD  clear_mask_rl90[65];
BITBOARD  clear_mask_rl45[65];
BITBOARD  clear_mask_rr45[65];
BITBOARD  set_mask[65];
BITBOARD  set_mask_rl90[65];
BITBOARD  set_mask_rl45[65];
BITBOARD  set_mask_rr45[65];
BITBOARD  file_mask[8];
BITBOARD  rank_mask[8];
BITBOARD  right_side_mask[8];
BITBOARD  left_side_mask[8];
BITBOARD  right_side_empty_mask[8];
BITBOARD  left_side_empty_mask[8];
BITBOARD  mask_efgh, mask_fgh, mask_abc, mask_abcd;
BITBOARD  mask_abs7_w, mask_abs7_b;
BITBOARD  mask_advance_2_w;
BITBOARD  mask_advance_2_b;
BITBOARD  mask_left_edge;
BITBOARD  mask_right_edge;
BITBOARD  mask_not_edge;
BITBOARD  mask_WBT;
BITBOARD  mask_BBT;
BITBOARD  mask_A3B3;
BITBOARD  mask_B3C3;
BITBOARD  mask_F3G3;
BITBOARD  mask_G3H3;
BITBOARD  mask_A6B6;
BITBOARD  mask_B6C6;
BITBOARD  mask_F6G6;
BITBOARD  mask_G6H6;
BITBOARD  mask_white_OO;
BITBOARD  mask_white_OOO;
BITBOARD  mask_black_OO;
BITBOARD  mask_black_OOO;
BITBOARD  stonewall_white;
BITBOARD  stonewall_black;
BITBOARD  e7_e6;
BITBOARD  e2_e3;
BITBOARD  closed_white;
BITBOARD  closed_black;
BITBOARD  mask_kr_trapped_w[3];
BITBOARD  mask_qr_trapped_w[3];
BITBOARD  mask_kr_trapped_b[3];
BITBOARD  mask_qr_trapped_b[3];
BITBOARD  good_bishop_kw;
BITBOARD  good_bishop_qw;
BITBOARD  good_bishop_kb;
BITBOARD  good_bishop_qb;
BITBOARD  light_squares;
BITBOARD  dark_squares;
BITBOARD  not_rook_pawns;
BITBOARD  plus1dir[65];
BITBOARD  plus7dir[65];
BITBOARD  plus8dir[65];
BITBOARD  plus9dir[65];
BITBOARD  minus1dir[65];
BITBOARD  minus7dir[65];
BITBOARD  minus8dir[65];
BITBOARD  minus9dir[65];
BITBOARD  mask_eptest[64];
# if !defined(ALPHA)
BITBOARD  mask_1;
BITBOARD  mask_2;
BITBOARD  mask_3;
BITBOARD  mask_8;
BITBOARD  mask_16;
BITBOARD  mask_112;
BITBOARD  mask_120;
# endif
BITBOARD  mask_clear_entry;
# if (!defined(_M_AMD64) && !defined(INLINE_ASM)) || defined(VC_INLINE_ASM)
unsigned char first_one[65536];
unsigned char last_one[65536];
# endif
unsigned char first_one_8bit[256];
unsigned char last_one_8bit[256];
unsigned char pop_cnt_8bit[256];
unsigned char connected_passed[256];
unsigned char file_spread[256];
signed char is_outside[256][256];
signed char is_outside_c[256][256];
BITBOARD  mask_pawn_protected_b[64];
BITBOARD  mask_pawn_protected_w[64];
BITBOARD  mask_pawn_duo[64];
BITBOARD  mask_pawn_isolated[64];
BITBOARD  mask_pawn_passed_w[64];
BITBOARD  mask_pawn_passed_b[64];
BITBOARD  mask_no_pattacks_w[64];
BITBOARD  mask_no_pattacks_b[64];
BITBOARD  white_minor_pieces;
BITBOARD  black_minor_pieces;
BITBOARD  white_pawn_race_wtm[64];
BITBOARD  white_pawn_race_btm[64];
BITBOARD  black_pawn_race_wtm[64];
BITBOARD  black_pawn_race_btm[64];
BITBOARD  mask_wk_4th, mask_wq_4th, mask_bk_4th, mask_bq_4th;
BOOK_POSITION book_buffer[BOOK_CLUSTER_SIZE];
BOOK_POSITION books_buffer[BOOK_CLUSTER_SIZE];
unsigned int thread_start_time[CPUS];
unsigned int pids[CPUS];
# if defined(SMP)
TREE     *local[MAX_BLOCKS + 1];
TREE     *volatile thread[CPUS];
lock_t    lock_smp, lock_io, lock_root;
#   if defined(POSIX)
pthread_attr_t pthread_attr;
#   endif
# else
TREE     *local[1];
#  endif
unsigned int parallel_splits;
unsigned int parallel_stops;
unsigned int max_split_blocks;
volatile unsigned int splitting;
# define    VERSION                             "19.13"
char      version[6] = { VERSION };
PLAYING_MODE mode = normal_mode;
#if defined(SMP)
volatile int smp_idle = 0;
volatile int smp_threads = 0;
#endif
int       batch_mode = 0;       /* no asynch reads */
int       swindle_mode = 1;     /* try to swindle */
int       call_flag = 0;
int       crafty_rating = 2500;
int       opponent_rating = 2500;
int       last_search_value = 0;
int       DGT_active = 0;
int       to_dgt = 0;
int       from_dgt = 0;
int       pgn_suggested_percent = 0;
char      pgn_event[32] = { "?" };
char      pgn_site[32] = { "?" };
char      pgn_date[32] = { "????.??.??" };
char      pgn_round[32] = { "?" };
char      pgn_white[64] = { "unknown" };
char      pgn_white_elo[32] = { "" };
char      pgn_black[64] = { "Crafty " VERSION };
char      pgn_black_elo[32] = { "" };
char      pgn_result[32] = { "*" };
int       number_to_play = 0;
char      toplay_list[512][20] = {
  {""}
};
int       number_of_blockers = 0;
char      blocker_list[512][20] = {
  {""}
};
int       number_auto_kibitzers = 7;
char      auto_kibitz_list[64][20] = {
  {"diepx"},
  {"ferret"},
  {"hossa"},
  {"lambchop"},
  {"moron"},
  {"otter"},
  {"zarkovx"}
};
int       number_of_computers = 0;
char      computer_list[512][20] = {
  {""}
};
int       number_of_GMs = 9;
char      GM_list[512][20] = {
  {"astr"},
  {"dreev"},
  {"garompon"},
  {"jean-reno"},
  {"lorenzo"},
  {"mecking"},
  {"nikephoros"},
  {"ruslam"},
  {"sweere"}
};
int       number_of_IMs = 0;
char      IM_list[512][20] = {
  {""}
};
int       number_of_special_openings = 0;
char      opening_list[16][20] = {
  {""}
};
char      opening_filenames[16][64] = {
  {""}
};
int       ics = 0;
int       output_format = 0;
int       EGTBlimit = 0;
int       EGTB_use = 0;
int       EGTB_draw = 0;
int       EGTB_search = 0;
size_t    EGTB_cache_size = EGTB_CACHE_DEFAULT;
void     *EGTB_cache = (void *) 0;
int       EGTB_setup = 0;
int       xboard = 0;
int       pong = 0;
int       channel = 0;
int       early_exit = 99;
int       new_game = 0;
char      channel_title[32] = { "" };
char      book_path[128] = { BOOKDIR };
char      log_path[128] = { LOGDIR };
char      tb_path[128] = { TBDIR };
char      rc_path[128] = { RCDIR };
int       initialized = 0;
int       kibitz = 0;
int       post = 0;
int       log_id = 1;
int       move_number = 1;
int       wtm = 1;
int       crafty_is_white = 0;
int       last_opponent_move = 0;
int       average_nps = 0;
int       incheck_depth = INCPLY;
int       onerep_depth = 3 * INCPLY / 4;
int       recap_depth = 3 * INCPLY / 4;
int       pushpp_depth = 3 * INCPLY / 4;
int       mate_depth = 3 * INCPLY / 4;
int       null_min = 3 * INCPLY;        /* R=2 */
int       null_max = 4 * INCPLY;        /* R=3 */
int       largest_positional_score = 300;
int       lazy_eval_cutoff = 200;
int       search_depth = 0;
unsigned int search_nodes = 0;
int       search_move = 0;
TIME_TYPE time_type = elapsed;
int       nodes_between_time_checks = 10000;
int       nodes_per_second = 10000;
int       predicted = 0;
int       time_used = 0;
int       time_used_opponent = 0;
int       cpu_time_used = 0;
signed char transposition_id = 0;
int       opening = 1;
int       middle_game = 0;
int       end_game = 0;
signed char thinking = 0;
signed char pondering = 0;
signed char puzzling = 0;
signed char booking = 0;
int       analyze_mode = 0;
int       annotate_mode = 0;
int       test_mode = 0;
int       input_status = 0;
signed char resign = 9;
signed char resign_counter = 0;
signed char resign_count = 5;
signed char draw_counter = 0;
signed char draw_count = 5;
signed char draw_offer_pending = 0;
int       offer_draws = 1;
int       adaptive_hash = 0;
size_t    adaptive_hash_min = 0;
size_t    adaptive_hash_max = 0;
size_t    adaptive_hashp_min = 0;
size_t    adaptive_hashp_max = 0;
int       tc_moves = 60;
int       tc_time = 180000;
int       tc_time_remaining = 180000;
int       tc_time_remaining_opponent = 180000;
int       tc_moves_remaining = 60;
int       tc_secondary_moves = 30;
int       tc_secondary_time = 90000;
int       tc_increment = 0;
int       tc_sudden_death = 0;
int       tc_operator_time = 0;
int       tc_safety_margin = 0;
int       time_limit = 100;
int       force = 0;
char      initial_position[80] = { "" };
char      hint[512] = { "" };
char      book_hint[512] = { "" };
int       over = 0;
int       trojan_check = 0;
int       computer_opponent = 0;
int       use_asymmetry = 1;
int       usage_level = 0;
char      audible_alarm = 0x07;
char      speech = 0;
int       ansi = 1;
int       book_accept_mask = ~03;
int       book_reject_mask = 3;
int       book_random = 1;
float     book_weight_learn = 1.0;
float     book_weight_freq = 1.0;
float     book_weight_CAP = 0.7;
float     book_weight_eval = 0.5;
int       book_search_trigger = 20;
int       learning = 7;
int       learning_cutoff = -2*PAWN_VALUE;
int       learning_trigger = PAWN_VALUE/3;
int       moves_out_of_book = 0;
int       show_book = 0;
int       book_selection_width = 5;
int       ponder = 1;
int       trace_level = 0;
int       display_options = 4095 - 256 - 512;
int       max_threads = 0;
int       min_thread_depth = 3 * INCPLY;
int       max_thread_group = CPUS;
int       split_at_root = 1;
unsigned int noise_level = 100000;
size_t    hash_table_size = 65536;
int       log_hash = 16;
size_t    pawn_hash_table_size = 32768;
int       log_pawn_hash = 15;
int       draw_score[2] = { 0, 0 };
int       abs_draw_score = 1;
int       accept_draws = 1;
unsigned char bishop_shift_rl45[64] = {
  64, 62, 59, 55, 50, 44, 37, 29,
  62, 59, 55, 50, 44, 37, 29, 22,
  59, 55, 50, 44, 37, 29, 22, 16,
  55, 50, 44, 37, 29, 22, 16, 11,
  50, 44, 37, 29, 22, 16, 11,  7,
  44, 37, 29, 22, 16, 11,  7,  4,
  37, 29, 22, 16, 11,  7,  4,  2,
  29, 22, 16, 11,  7,  4,  2,  1
};
unsigned char bishop_shift_rr45[64] = {
  29, 37, 44, 50, 55, 59, 62, 64,
  22, 29, 37, 44, 50, 55, 59, 62,
  16, 22, 29, 37, 44, 50, 55, 59,
  11, 16, 22, 29, 37, 44, 50, 55,
   7, 11, 16, 22, 29, 37, 44, 50,
   4,  7, 11, 16, 22, 29, 37, 44,
   2,  4,  7, 11, 16, 22, 29, 37,
   1,  2,  4,  7, 11, 16, 22, 29
};
const char xlate[15] =
    { 'q', 'r', 'b', 0, 'k', 'n', 'p', 0, 'P', 'N', 'K', 0, 'B', 'R', 'Q' };
const char empty[9] = { 0, '1', '2', '3', '4', '5', '6', '7', '8' };
const char king_tropism_n[8] = { 3, 3, 3, 2, 1, 0, 0, 0 };
const char king_tropism_b[8] = { 3, 3, 3, 2, 1, 0, 0, 0 };
const char king_tropism_r[8] = { 3, 3, 3, 2, 1, 0, 0, 0 };
const char king_tropism_at_r[8] = { 4, 3, 1, 0, 0, 0, 0, 0 };
const char king_tropism_q[8] = { 4, 4, 4, 2, 1, 0, 0, 0 };
const char king_tropism_at_q[8] = { 5, 5, 3, 0, 0, 0, 0, 0 };
const int king_tropism[128] = {
  -25, -25, -20, -20, -15, -15, -10,  -5,
    0,   2,   5,  12,  15,  20,  30,  40,
   50,  60,  70,  80,  90, 100, 110, 120,
  125, 125, 130, 130, 135, 135, 140, 145,
  150, 160, 170, 180, 180, 180, 180, 180,
  180, 180, 180, 180, 180, 180, 180, 180,
  180, 180, 180, 180, 180, 180, 180, 180,
  180, 180, 180, 180, 180, 180, 180, 180,
  180, 180, 180, 180, 180, 180, 180, 180,
  180, 180, 180, 180, 180, 180, 180, 180,
  180, 180, 180, 180, 180, 180, 180, 180,
  180, 180, 180, 180, 180, 180, 180, 180,
  180, 180, 180, 180, 180, 180, 180, 180,
  180, 180, 180, 180, 180, 180, 180, 180,
  180, 180, 180, 180, 180, 180, 180, 180,
  180, 180, 180, 180, 180, 180, 180, 180
};
const char connected_passed_pawn_value[8] = { 0, 0, 8, 12, 24, 48, 100, 0 };
const int hidden_passed_pawn_value[8] = { 0, 0, 0, 0, 0, 16, 24, 0 };
const int passed_pawn_value[8] = { 0, 24, 32, 48, 60, 120, 160, 0 };
const char blockading_passed_pawn_value[8] = { 0, 8, 12, 16, 30, 45, 60, 0 };
const char isolated_pawn_value[9] = { 0, 12, 24, 56, 66, 80, 90, 100, 110 };
const char isolated_pawn_of_value[9] = { 0, 5, 10, 15, 20, 25, 30, 35, 40 };
const char doubled_pawn_value[7] = { 0, 0, 5, 10, 20, 25, 30 };
const char pawn_rams_v[9] = { 0, 0, 6, 24, 40, 64, 80, 94, 96 };
const char supported_passer[8] = { 0, 0, 0, 0, 12, 60, 100, 0 };
const char outside_passed[128] = {
  96, 48, 48, 48, 45, 42, 40, 40,
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
  12, 12, 12, 12, 12, 12, 12, 12
};
const char majority[128] = {
  72, 36, 36, 36, 34, 31, 30, 30,
  27, 27, 24, 24, 21, 21, 18, 18,
  15, 15, 12, 12, 10, 10, 10, 10,
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
  10, 10, 10, 10, 10, 10, 10, 10,
  10, 10, 10, 10, 10, 10, 10, 10
};
/*
 the following values are for king safety.  The 'temper' array is used
 to scale the number of defects found into some sort of 'sane' score so that
 small pertubations in king safety can produce signficant scoring changes,
 but large pertubations won't cause Crafty to sacrifice pieces.
 */
const int temper[64] = {
   0,    4,  15,  24,  36,  42,  54,  72,       /*   0-   7 */
   81,  90,  99, 108, 117, 126, 135, 144,       /*   8-  15 */
  148, 152, 156, 160, 164, 168, 172, 175,       /*  16-  23 */
  180, 180, 180, 180, 180, 180, 180, 180,       /*  24-  31 */
  180, 180, 180, 180, 180, 180, 180, 180,       /*  32-  39 */
  180, 180, 180, 180, 180, 180, 180, 180,       /*  47-  47 */
  180, 180, 180, 180, 180, 180, 180, 180,       /*  48-  55 */
  180, 180, 180, 180, 180, 180, 180, 180        /*  56-  63 */
};
/*
 the following array cuts the king safety term down to a value that can
 be used in the king tropism calculation.  this lets the safety of a king
 influence how important getting pieces closer to the king really is.  and
 is primarily used to help when both sides castle to the same side of the
 board which means that if I disrupt my kingside, I am also disrupting my
 opponent's kingside.  now I can figure out who comes out better in such
 situations. These values are in units of 1/16th.  Which means that a value
 of 16 will use the entire tropism value, a value of 32 will use 2x the
 normal tropism value, and a value of 8 will use 1/2 the normal value.
 this array is indexed by the tempered king safety value, and lower-
 numbered elements correspond to safe king positions, while higher-numbered
 elements represent disrupted kingsides.
 */
const char ttemper[64] = {
  16, 16, 16, 16, 17, 17, 18, 18,       /*   0-   7 */
  19, 19, 20, 20, 21, 21, 22, 22,       /*   8-  15 */
  23, 23, 24, 24, 25, 25, 26, 26,       /*  16-  23 */
  27, 27, 28, 28, 29, 29, 30, 30,       /*  24-  31 */
  31, 31, 32, 32, 32, 32, 32, 32,       /*  32-  39 */
  32, 32, 32, 32, 32, 32, 32, 32,       /*  40-  47 */
  32, 32, 32, 32, 32, 32, 32, 32,       /*  48-  55 */
  32, 32, 32, 32, 32, 32, 32, 32        /*  56-  63 */
};
/*
 penalty for a fully open file in front of the castled king.
 */
const char openf[4] = { 0, 8, 13, 16 };
/*
 penalty for a half-open (one side's pawn is missing) file in front of
 the castled king.
 */
const char hopenf[4] = { 0, 3, 7, 9 };
/*
 this value controls king safety asymmetry.  0 is fully symmetrical.
 this works by modifying the king safety scores for the opponent by
 the amount specified.  -20 reduces the opponent's king safety scores,
 making crafty's king safety more important.  +20 increases the opponent's
 king safety, making Crafty's less important (this will tend to increase
 aggressiveness while - values will make Crafty more passive/defensive.)
 */
int       king_safety_asymmetry = -20;
/*
 this value scales king safety up or down equally for both sides.  A
 value of 100 leaves the values as they are.  values below 100 drop
 the king safety scores for both sides proportionally.
 */
int       king_safety_scale = 100;
/*
 this value scales blocked pawn scores up/down.  Increasing this value
 will make Crafty try harder to not block pawn positions, which will aim
 for more open positions.
 */
int       blocked_scale = 100;
/*
 this value scales passed pawn scores up/down.  Increasing this value
 will make Crafty evaluate some passed pawn scores higher, such as the
 "outside passed pawn scoring."
 */
int       passed_scale = 100;
/*
 this value scales pawn scores up/down.  Increasing this value will
 make Crafty try harder to preserve its pawn structure / wreck the
 opponent's pawn structure.
 */
int       pawn_scale = 100;
/*
 this value scales king tropism up or down.  the default is 100 which
 uses the built-in scores.  150 will increase tropism scores by 50%
 which will make the program more aggressive, but probably less
 positionally aware as a result.
 */
int       king_safety_tropism = 100;
/*
 the following is basically a 'defect' table for kings on a specific
 square.  this lets it figure out that the king doesn't want to go to
 (say f3 or e4 or anywhere near the center) with heavy pieces still on
 the board.
 */
signed char king_defects_w[64] = {
  1, 0, 2, 3, 3, 2, 0, 1,
  1, 1, 2, 3, 3, 2, 1, 1,
  3, 3, 3, 3, 3, 3, 3, 3,
  4, 4, 4, 4, 4, 4, 4, 4,
  5, 5, 5, 5, 5, 5, 5, 5,
  6, 6, 6, 6, 6, 6, 6, 6,
  7, 7, 7, 7, 7, 7, 7, 7,
  8, 8, 8, 8, 8, 8, 8, 8
};
const char square_color[64] = {
  1, 0, 1, 0, 1, 0, 1, 0,
  0, 1, 0, 1, 0, 1, 0, 1,
  1, 0, 1, 0, 1, 0, 1, 0,
  0, 1, 0, 1, 0, 1, 0, 1,
  1, 0, 1, 0, 1, 0, 1, 0,
  0, 1, 0, 1, 0, 1, 0, 1,
  1, 0, 1, 0, 1, 0, 1, 0,
  0, 1, 0, 1, 0, 1, 0, 1
};
const char b_n_mate_dark_squares[64] = {
 100, 90, 80, 70, 60, 50, 40,  30,
  90, 80, 70, 60, 50, 40, 30,  40,
  80, 70, 60, 50, 40, 30, 40,  50,
  70, 60, 50, 40, 30, 40, 50,  60,
  60, 50, 40, 30, 40, 50, 60,  70,
  50, 40, 30, 40, 50, 60, 70,  80,
  40, 30, 40, 50, 60, 70, 80,  90,
  30, 40, 50, 60, 70, 80, 90, 100
};
const char b_n_mate_light_squares[64] = {
  30,  40, 50, 60, 70, 80, 90, 100,
  40,  30, 40, 50, 60, 70, 80,  90,
  50,  40, 30, 40, 50, 60, 70,  80,
  60,  50, 40, 30, 40, 50, 60,  70,
  70,  60, 50, 40, 30, 40, 50,  60,
  80,  70, 60, 50, 40, 30, 40,  50,
  90,  80, 70, 60, 50, 40, 30,  40,
  100, 90, 80, 70, 60, 50, 40,  30
};
const char mate[64] = {
  100, 96, 93, 90, 90, 93, 96, 100,
   96, 80, 70, 60, 60, 70, 80,  96,
   93, 70, 60, 50, 50, 60, 70,  93,
   90, 60, 50, 40, 40, 50, 60,  90,
   90, 60, 50, 40, 40, 50, 60,  90,
   93, 70, 60, 50, 50, 60, 70,  93,
   96, 80, 70, 60, 60, 70, 80,  96,
  100, 96, 93, 90, 90, 93, 96, 100
};
signed char white_outpost[64] = {
  0, 0,  0,  0,  0,  0, 0, 0,
  0, 0,  0,  0,  0,  0, 0, 0,
  0, 0,  0,  0,  0,  0, 0, 0,
  0, 0,  0,  6,  6,  0, 0, 0,
  0, 0, 12, 15, 15, 12, 0, 0,
  0, 0, 12, 18, 18, 12, 0, 0,
  0, 0,  6, 12, 12,  6, 0, 0,
  0, 0,  0,  0,  0,  0, 0, 0
};
const char push_extensions[64] = {
  0, 0, 0, 0, 0, 0, 0, 0,
  1, 1, 1, 1, 1, 1, 1, 1,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  1, 1, 1, 1, 1, 1, 1, 1,
  0, 0, 0, 0, 0, 0, 0, 0
};
signed char pval_w[64] = {
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 3, 3, 0, 0, 0,
  0, 0, 2, 3, 3, 0, 0, 0,
  0, 0, 2, 2, 2, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0
};
signed char nval_w[64] = {
   -8, -8, -8, -8, -8, -8, -8, -8,
   -8, -8,  0,  0,  0,  0, -8, -8,
   -8,  2,  2,  2,  2,  2,  2, -8,
   -8,  2,  4,  4,  4,  4,  2, -8,
   -8,  2,  6,  6,  6,  6,  2, -8,
   -8,  2,  6,  6,  6,  6,  2, -8,
  -24,-24,  4,  4,  4,  4,-24,-24,
  -32,-16, -8, -8, -8, -8,-16,-32
};
signed char bval_w[64] = {
  -16, -16, -8, -8, -8, -8,-16,-16,
  -16,   2,  0,  0,  0,  0,  2,-16,
   -4,   2,  2,  2,  2,  2,  2, -4,
   -4,   2,  4,  4,  4,  4,  2, -4,
   -4,   2,  6,  6,  6,  6,  2, -4,
   -4,   2,  6,  6,  6,  6,  2, -4,
  -16,   2,  4,  4,  4,  4,  2,-16,
  -16, -16,  0,  0,  0,  0,-16,-16
};
signed char rval_w[64] = {
  0, 0, 4, 6, 6, 4, 0, 0,
  0, 0, 4, 6, 6, 4, 0, 0,
  0, 0, 4, 6, 6, 4, 0, 0,
  0, 0, 4, 6, 6, 4, 0, 0,
  0, 0, 4, 6, 6, 4, 0, 0,
  0, 0, 4, 6, 6, 4, 0, 0,
  0, 0, 4, 6, 6, 4, 0, 0,
  0, 0, 4, 6, 6, 4, 0, 0
};
signed char qval_w[64] = {
  -10,  -8,  0,  0,  0, 0,  -8, -10,
  -10,   2,  8,  8,  8, 8,   2, -10,
  -10, -10,  4, 10, 10, 4, -10, -10,
  -10,   2, 10, 12, 12, 10,  2, -10,
    0,   2, 10, 12, 12, 10,  2,   0,
    0,   2,  4, 10, 10,  4,  2,   0,
  -15,   0,  4,  5,  5,  4,  0, -15,
    0,   0,  0,  0,  0,  0,  0, 0
};
signed char kval_wn[64] = {
  -50, -40, -30, -20, -20, -30, -40, -50,
  -30, -20, -10,   0,   0, -10, -20, -30,
  -30, -10,   0,  10,  10,   0, -10, -30,
  -30, -10,  20,  30,  30,  20, -10, -30,
  -30, -10,  30,  40,  40,  30, -10, -30,
  -30, -10,  30,  40,  40,  30, -10, -30,
  -30, -30,   0,   0,   0,   0, -30, -30,
  -50, -30, -30, -30, -30, -30, -30, -50
};
signed char kval_wk[64] = {
  -90, -70, -50, -40, -30, -20, -20, -30,
  -70, -50, -30, -20, -10, -10, -10, -10,
  -70, -50, -30, -10,  20,  20,  20,   0,
  -70, -50, -30, -10,  30,  30,  30,   0,
  -70, -50, -30, -10,  30,  30,  30,   0,
  -70, -50, -30, -10,  30,  30,  30,   0,
  -70, -50, -30, -30,   0,   0,   0, -10,
  -90, -70, -50, -30, -30, -30, -30, -30
};
signed char kval_wq[64] = {
  -30, -20, -20, -30, -40, -50, -70, -90,
  -10, -10, -10, -10, -20, -30, -50, -70,
    0,  20,  20,  20, -10, -30, -50, -70,
    0,  30,  30,  30, -10, -30, -50, -70,
    0,  30,  30,  30, -10, -30, -50, -70,
    0,  30,  30,  30, -10, -30, -50, -70,
  -10,   0,   0,   0, -20, -30, -50, -70,
  -30, -30, -30, -30, -30, -50, -70, -90
};
/*  score for bishop pair varies depending on how many pawns are
   on the board (0-8)                                           */
signed char bishop_pair[9] = { 20, 20, 20, 20, 20, 20, 20, 15, 8 };
/* note that black piece/square values are copied from white, but
   reflected */
const int p_values[15] = { QUEEN_VALUE, ROOK_VALUE, BISHOP_VALUE, 0,
  KING_VALUE, KNIGHT_VALUE, PAWN_VALUE,
  0, PAWN_VALUE, KNIGHT_VALUE, KING_VALUE,
  0, BISHOP_VALUE, ROOK_VALUE, QUEEN_VALUE
};
