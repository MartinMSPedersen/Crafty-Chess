#include <stdio.h>
#include <stdlib.h>
#include "chess.h"
SHARED   *shared;
FILE     *input_stream;
FILE     *dbout;
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
int       done = 0;
BITBOARD  total_moves;
int       last_mate_score;
char      log_filename[64];
char      history_filename[64];
int       number_of_solutions;
int       solutions[10];
int       solution_type;
char      cmd_buffer[4096];
char     *args[256];
char      buffer[512];
unsigned char convert_buff[8];
int       nargs;
int       ponder_value;
int       move_actually_played;
int       ponder_move;
int       ponder_moves[220];
int       num_ponder_moves;
int       book_move;
int       book_learn_eval[LEARN_INTERVAL];
int       book_learn_depth[LEARN_INTERVAL];
int       hash_mask;
unsigned int pawn_hash_mask;
HASH_ENTRY *trans_ref;
PAWN_HASH_ENTRY *pawn_hash_table;
size_t    cb_trans_ref;
size_t    cb_pawn_hash_table;
PATH      last_pv;
int       last_value;
int       temper_b[64], temper_w[64];
int pval_b[64];
int nval_b[64];
int bval_b[64];
int rval_b[64];
int qval_b[64];
int kval_bn[64];
int kval_bk[64];
int kval_bq[64];
int black_outpost[64];
int king_defects_b[64];
signed char directions[64][64];
int       tropism[128];
int       pawn_rams[9];
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
# if (!defined(_M_AMD64) && !defined (_M_IA64) && !defined(INLINE_ASM)) || defined(VC_INLINE_ASM)
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
BOOK_POSITION book_buffer[BOOK_CLUSTER_SIZE];
BOOK_POSITION book_buffer_char[BOOK_CLUSTER_SIZE];
# define    VERSION                             "20.0"
char      version[6] = { VERSION };
PLAYING_MODE mode = normal_mode;
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
char      pgn_event[128] = { "?" };
char      pgn_site[128] = { "?" };
char      pgn_date[128] = { "????.??.??" };
char      pgn_round[128] = { "?" };
char      pgn_white[128] = { "unknown" };
char      pgn_white_elo[128] = { "" };
char      pgn_black[128] = { "Crafty " VERSION };
char      pgn_black_elo[128] = { "" };
char      pgn_result[128] = { "*" };
char      *B_list[128];
char      *AK_list[128];
char      *C_list[128];
char      *GM_list[128];
char      *IM_list[128];
char      *SP_list[128];
char      *SP_opening_filename[128];
char      *SP_personality_filename[128];
int       ics = 0;
int       output_format = 0;
#if !defined(NOEGTB)
int       EGTBlimit = 0;
int       EGTB_use = 0;
int       EGTB_draw = 0;
int       EGTB_search = 0;
size_t    EGTB_cache_size = EGTB_CACHE_DEFAULT;
void     *EGTB_cache = (void *) 0;
int       EGTB_setup = 0;
#endif
int       xboard = 0;
int       pong = 0;
int       channel = 0;
int       early_exit = 99;
int       new_game = 0;
char      channel_title[32] = { "" };
char      book_path[128] = { BOOKDIR };
char      personality_path[128] = { PERSDIR };
char      log_path[128] = { LOGDIR };
char      tb_path[128] = { TBDIR };
char      rc_path[128] = { RCDIR };
int       initialized = 0;
int       kibitz = 0;
int       post = 0;
int       log_id = 1;
int       wtm = 1;
int       last_opponent_move = 0;
int       incheck_depth = 60;
int       onerep_depth = 45;
int       recap_depth = 45;
int       mate_depth = 30;
int       null_min = 3 * INCPLY;        /* R=2 */
int       null_max = 4 * INCPLY;        /* R=3 */
int       search_depth = 0;
unsigned int search_nodes = 0;
int       search_move = 0;
int       predicted = 0;
int       time_used = 0;
int       time_used_opponent = 0;
int       analyze_mode = 0;
int       annotate_mode = 0;
int       test_mode = 0;
int       input_status = 0;
int       resign = 9;
int       resign_counter = 0;
int       resign_count = 5;
int       draw_counter = 0;
int       draw_count = 5;
int       draw_offer_pending = 0;
int       offer_draws = 1;
int       adaptive_hash = 0;
size_t    adaptive_hash_min = 0;
size_t    adaptive_hash_max = 0;
size_t    adaptive_hashp_min = 0;
size_t    adaptive_hashp_max = 0;
int       time_limit = 100;
int       force = 0;
char      initial_position[80] = { "" };
char      hint[512] = { "" };
char      book_hint[512] = { "" };
int       over = 0;
int       silent = 0;
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
size_t    hash_table_size = 65536;
int       log_hash = 16;
size_t    pawn_hash_table_size = 32768;
int       log_pawn_hash = 15;
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
int king_tropism_n[8] = { 3, 3, 3, 2, 1, 0, 0, 0 };
int king_tropism_b[8] = { 3, 3, 3, 2, 1, 0, 0, 0 };
int king_tropism_r[8] = { 3, 3, 3, 2, 1, 0, 0, 0 };
int king_tropism_at_r[8] = { 4, 3, 1, 0, 0, 0, 0, 0 };
int king_tropism_q[8] = { 4, 4, 4, 2, 1, 0, 0, 0 };
int king_tropism_at_q[8] = { 5, 5, 3, 0, 0, 0, 0, 0 };
int king_tropism[128] = {
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
int connected_passed_pawn_value[8] = { 0, -6, -6, 0, 100, 200, 300, 0 };
int hidden_passed_pawn_value[8] = { 0, 0, 0, 0, 20, 40, 0, 0 };
int passed_pawn_value[8] = { 0, 12, 20, 32, 48, 96, 150, 0 };
int blockading_passed_pawn_value[8] = { 0, 4, 7, 10, 16, 32, 50, 0 };
int isolated_pawn_value[9] = { 0, 8, 20, 40, 60, 70, 80, 80, 80 };
int isolated_pawn_of_value[9] = { 0, 4, 10, 16, 24, 24, 24, 24, 24 };
int doubled_pawn_value[9] = { 0, 0, 4, 7, 10, 10, 10, 10, 10};
int pawn_rams_v[9] = { 0, 0, 4, 8, 16, 24, 32, 40, 48 };
int pawn_space[8] = {0, 0, 0, 0, 1, 2, 2, 2};
int supported_passer[8] = { 0, 0, 0, 10, 40, 60, 100, 0 };
int outside_passed[128] = {
  72, 64, 64, 64, 56, 56, 52, 52,
  48, 48, 44, 44, 42, 42, 40, 40,
  36, 36, 30, 30, 24, 24, 24, 12,
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
int majority[128] = {
  60, 40, 40, 40, 34, 32, 28, 28,
  26, 26, 24, 24, 21, 21, 18, 18,
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
int temper[64] = {
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
int ttemper[64] = {
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
int openf[4] = { 0, 6, 8, 16 };
/*
 penalty for a half-open (one side's pawn is missing) file in front of
 the castled king.
 */
int hopenf[4] = { 0, 1, 8, 8 };
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
int king_defects_w[64] = {
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
int white_outpost[64] = {
  0, 0,  0,  0,  0,  0, 0, 0,
  0, 0,  0,  0,  0,  0, 0, 0,
  0, 0,  0,  0,  0,  0, 0, 0,
  0, 0,  0, 12, 12,  0, 0, 0,
  0, 0, 24, 30, 30, 24, 0, 0,
  0, 0, 24, 30, 30, 24, 0, 0,
  0, 0, 12, 20, 20, 12, 0, 0,
  0, 0,  0,  0,  0,  0, 0, 0
};
int pval_w[64] = {
 0,  0,  0,  0,  0,  0,  0,  0,
 0,  0,  0, -8, -8,  0,  0,  0,
 6,  6,  6,  6,  6,  6,  6,  6,
12, 12, 12, 12, 12, 12, 12, 12,
18, 18, 18, 18, 18, 18, 18, 18,
24, 24, 24, 24, 24, 24, 24, 24,
30, 30, 30, 30, 30, 30, 30, 30,
 0,  0,  0,  0,  0,  0,  0,  0
};
int nval_w[64] = {
  -32, -8, -8, -8, -8, -8, -8,-32,
   -8, -8,  0,  0,  0,  0, -8, -8,
   -8,  2,  2,  2,  2,  2,  2, -8,
   -8,  2,  4,  4,  4,  4,  2, -8,
   -8,  2,  6,  6,  6,  6,  2, -8,
   -8,  2,  6,  6,  6,  6,  2, -8,
  -24,-24,  4,  4,  4,  4,-24,-24,
  -32,-16, -8, -8, -8, -8,-16,-32
};
int bval_w[64] = {
  -16, -16, -8, -8, -8, -8,-16,-16,
  -16,   2,  0,  0,  0,  0,  2,-16,
   -4,   2,  2,  2,  2,  2,  2, -4,
   -4,   2,  4,  4,  4,  4,  2, -4,
   -4,   2,  6,  6,  6,  6,  2, -4,
   -4,   2,  6,  6,  6,  6,  2, -4,
  -16,   2,  4,  4,  4,  4,  2,-16,
  -16, -16,  0,  0,  0,  0,-16,-16
};
int rval_w[64] = {
 -3,-3, 1, 3, 3, 1,-3,-3,
 -3,-3, 1, 3, 3, 1,-3,-3,
 -3,-3, 1, 3, 3, 1,-3,-3,
 -3,-3, 1, 3, 3, 1,-3,-3,
 -3,-3, 1, 3, 3, 1,-3,-3,
 -3,-3, 1, 3, 3, 1,-3,-3,
 -3,-3, 1, 3, 3, 1,-3,-3,
 -3,-3, 1, 3, 3, 1,-3,-3
};
int qval_w[64] = {
  -10,  -8,  0,  0,  0, 0,  -8, -10,
  -10,   2,  8,  8,  8, 8,   2, -10,
  -10, -10,  4, 10, 10, 4, -10, -10,
  -10,   2, 10, 12, 12, 10,  2, -10,
    0,   2, 10, 12, 12, 10,  2,   0,
    0,   2,  4, 10, 10,  4,  2,   0,
  -15,   0,  4,  5,  5,  4,  0, -15,
    0,   0,  0,  0,  0,  0,  0, 0
};
int kval_wn[64] = {
  -20, -10, -10, -10, -10, -10, -10, -20,
  -10,   0,   0,   0,   0,   0,   0, -10,
  -10,  10,  10,  10,  10,  10,  10, -10,
  -10,  10,  20,  20,  20,  20,  10, -10,
  -10,  10,  20,  30,  30,  20,  10, -10,
  -10,  10,  20,  30,  30,  20,  10, -10,
  -10,  10,  20,  20,  20,  20,  10, -10,
  -20, -10, -10, -10, -10, -10, -10, -20
};
int kval_wk[64] = {
  -30, -20, -10, -10, -10, -10, -10, -10,
  -30, -20, -10,   0,   0,   0,   0,   0,
  -30, -20, -10,  10,  10,  10,  10,  10,
  -30, -20, -10,  10,  20,  20,  20,  20,
  -30, -20, -10,  10,  20,  30,  30,  20,
  -30, -20, -10,  10,  20,  30,  30,  20,
  -30, -20, -10,  10,  20,  20,  20,  20,
  -30, -20, -10, -10, -10, -10, -10, -10
};
int kval_wq[64] = {
  -10, -10, -10, -10, -10, -10, -20, -30,
    0,   0,   0,   0,   0, -10, -20, -30,
   10,  10,  10,  10,  10, -10, -20, -30,
   20,  20,  20,  20,  10, -10, -20, -30,
   20,  30,  30,  20,  10, -10, -20, -30,
   20,  30,  30,  20,  10, -10, -20, -30,
   20,  20,  20,  20,  10, -10, -20, -30,
  -10, -10, -10, -10, -10, -10, -20, -30
};
/*  score for bishop pair varies depending on how many pawns are
   on the board (0-8)                                           */
int bishop_pair[9] = { 30, 30, 30, 30, 30, 30, 30, 12, 0 };
/* note that black piece/square values are copied from white, but
   reflected */
const int p_values[15] = { QUEEN_VALUE, ROOK_VALUE, BISHOP_VALUE, 0,
  KING_VALUE, KNIGHT_VALUE, PAWN_VALUE, 0, PAWN_VALUE, KNIGHT_VALUE,
  KING_VALUE, 0, BISHOP_VALUE, ROOK_VALUE, QUEEN_VALUE
};
int pawn_value = PAWN_VALUE;
int knight_value = KNIGHT_VALUE;
int bishop_value = BISHOP_VALUE;
int rook_value = ROOK_VALUE;
int queen_value = QUEEN_VALUE;
int king_value = KING_VALUE;
int pawn_can_promote = 525;
int bad_trade = 120;
int eight_pawns = 10;
int pawns_blocked = 4;
int center_pawn_unmoved = 16;
int pawn_duo = 2;
int pawn_weak_p1 = 12;
int pawn_weak_p2 = 20;
int pawn_protected_passer_wins = 50;
int won_kp_ending = 150;
int split_passed = 50;
int pawn_base[8] = {0, 3, 6, 9, 9, 6, 3, 0};
int pawn_advance[8] = {3, 3, 3, 3, 3, 3, 3, 3};
int king_king_tropism = 15;
int bishop_trapped = 175;
int bishop_plus_pawns_on_color = 2;
int bishop_over_knight_endgame = 30;
int bishop_mobility = 3;
int bishop_king_safety = 10;
int rook_on_7th = 30;
int rook_absolute_7th = 20;
int rook_connected_7th_rank = 16;
int rook_open_file[8] = {3, 3, 4, 5, 5, 4, 3, 3};
int rook_half_open_file[8] = {1, 1, 2, 3, 3, 2, 1, 1};
int rook_behind_passed_pawn = 30;
int rook_trapped = 30;
int rook_limited = 20;
int queen_rook_on_7th_rank = 10;
int queen_king_safety = 6;
int queen_vs_2_rooks = 50;
int queen_is_strong = 30;
int queen_offside = -40;
int king_safety_mate_g2g7 = 3;
int king_safety_mate_threat = 600;
int king_safety_open_file = 4;
int castle_opposite = 3;
int development_thematic = 6;
int development_unmoved = 7;
int blocked_center_pawn = 12;
int development_losing_castle = 20;
int development_not_castled = 10;
int development_queen_early = 20;
int development_castle_bonus = 20;
int *evalterm_value[256] =
    { NULL,                        &pawn_value,
      &knight_value,               &bishop_value,
      &rook_value,                 &queen_value,
      NULL,                        NULL,
      NULL,                        NULL,
      NULL,                        &blocked_scale,
      &king_safety_asymmetry,      &king_safety_scale,
      &king_safety_tropism,        &passed_scale,
      &pawn_scale,                 &bad_trade,
      NULL,                        NULL,
      NULL,                        &eight_pawns,
      &pawns_blocked,              &center_pawn_unmoved,
      &pawn_duo,                   &pawn_protected_passer_wins,
      &pawn_weak_p1,               &pawn_weak_p2,
      &pawn_can_promote,           &won_kp_ending,
      &split_passed,               pval_w,                      
      connected_passed_pawn_value, hidden_passed_pawn_value,
      passed_pawn_value,           blockading_passed_pawn_value,
      isolated_pawn_value,         isolated_pawn_of_value,
      doubled_pawn_value,          pawn_rams_v,
      supported_passer,            outside_passed,
      majority,                    pawn_space,
      NULL,                        NULL,
      NULL,                        NULL,
      NULL,                        NULL,
      NULL,                        king_tropism_n,
      nval_w,                      white_outpost,
      NULL,                        NULL,
      NULL,                        NULL,
      NULL,                        NULL,
      NULL,                        &bishop_king_safety,
      &bishop_mobility,            &bishop_over_knight_endgame,
      &bishop_plus_pawns_on_color, &bishop_trapped,
      bishop_pair,                 king_tropism_b,
      bval_w,                      NULL,
      NULL,                        &rook_on_7th,
      &rook_absolute_7th,          &rook_connected_7th_rank,
      &rook_trapped,               &rook_limited,
      &rook_behind_passed_pawn,    rook_half_open_file,
      rook_open_file,              king_tropism_r,
      king_tropism_at_r,           rval_w,
      NULL,                        NULL,
      NULL,                        NULL,
      NULL,                        NULL,
      NULL,                        NULL,
      NULL,                        &queen_rook_on_7th_rank,
      &queen_king_safety,          &queen_vs_2_rooks,
      &queen_is_strong,            &queen_offside_tropism,
      king_tropism_q,              king_tropism_at_q,
      qval_w,                      NULL,
      NULL,                        &king_king_tropism,
      &king_safety_mate_g2g7,      &king_safety_mate_threat,
      &king_safety_open_file,      &castle_opposite,
      king_tropism,                king_defects_w,
      kval_wn,                     kval_wk,
      kval_wq,                     hopenf,
      openf,                       temper,
      ttemper,                     NULL,
      NULL,                        NULL,
      NULL,                        NULL,
      NULL,                        &development_thematic,
      &development_unmoved,        &blocked_center_pawn,
      &development_losing_castle,  &development_not_castled,
      &development_queen_early,    &development_castle_bonus,
      };
char *evalterm_description[256] =
    { "piece values--------------------", "pawn value                      ",
      "knight value                    ", "bishop value                    ",
      "rook value                      ", "queen value                     ",
      NULL,                               NULL,
      NULL,                               NULL,
      "evaluation scale factors--------", "blocked pawn scale factor       ",
      "king safety asymmetry           ", "king safety scale factor        ",
      "king safety tropism scale factor", "passed pawn scoring scale factor",
      "pawn scoring scale factor       ", "bad trade bonus/penalty         ",
      NULL,                               NULL,
      "pawn evaluation-----------------", "eight pawns penalty             ",
      "center pawn blocked             ", "center pawn unmoved             ",
      "pawn duo                        ", "protected passed pawn wins      ",
      "pawn weak (one pawn blocking)   ", "pawn weak (two pawns blocking)  ",
      "pawn can promote                ", "won kp ending                   ",
      "split passed pawn bonus         ", "pawn piece/square table         ",
      "connected passed pawn [rank]    ", "hidden passed pawn [rank]       ",
      "passed pawn [rank]              ", "blockading a passed pawn [rank] ",
      "isolated pawn [n]               ", "isolated pawn on open file [n]  ",
      "doubled pawn [n]                ", "pawn ram [n]                    ",
      "supported passed pawn [rank]    ", "outside passed pawn [matrl]     ",
      "pawn majority [matrl]           ", "pawn space [rank]               ",
      NULL,                               NULL,
      NULL,                               NULL,
      NULL,                               NULL,
      "knight scoring------------------", "king tropism [distance]         ",
      "knight piece/square table       ", "outpost [square]                ",
      NULL,                               NULL,
      NULL,                               NULL,
      NULL,                               NULL,
      "bishop scoring------------------", "fianchettoed bishop value       ",
      "bishop mobility                 ", "bishop over knight endgame      ",
      "bishop plus pawns on color      ", "bishop trapped                  ",
      "bishop pair [npawns]            ", "king tropism [distance]         ",
      "bishop piece/square table       ", NULL,
      "rook scoring--------------------", "rook on 7th                     ",
      "rook absolute 7th               ", "rook connected 7th rank         ",
      "rook trapped                    ", "rook limited mobility           ",
      "rook behind passed pawn         ", "rook half open file             ",
      "rook open file                  ", "king tropism [distance]         ",
      "king file tropism [distance]    ", "rook piece/square table         ",
      NULL,                               NULL,
      NULL,                               NULL,
      NULL,                               NULL,
      NULL,                               NULL,
      "queen scoring-------------------", "queen rook on 7th rank          ",
      "queen king tropism              ", "queen vs 2 rooks                ",
      "queen is strong                 ", "queen offside tropism           ",
      "king tropism [distance]         ", "king file tropism [distance]    ",
      "queen piece/square table        ", NULL,
      "king scoring--------------------", "king king tropism (endgame)     ",
      "king safety mate g2g7           ", "king safety trojan horse threat ",
      "king safety open file           ", "king safety castle opposite     ",
      "king tropism [defects]          ", "king defects (white - mirror B) ",
      "king piece/square normal        ", "king piece/square kside pawns   ",
      "king piece/square qside pawns   ", "king safety half-open file def  ",
      "king safety open file defects   ", "king safety indirect temper     ",
      "king safety tropism temper      ", NULL,
      NULL,                               NULL,
      NULL,                               NULL,
      "development scoring-------------", "development thematic            ",
      "development unmoved             ", "development blocked center pawn ",
      "development losing castle       ", "development not castled         ",
      "development moved queen early   ", "development castle bonus        ",
      };
int evalterm_size[256] = {
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
  0,-64,  8,  8,  8,  8,  9,  9,  9,  9,
  8,128,128,  8,  0,  0,  0,  0,  0,  0,
  0,  8,-64,-64,  0,  0,  0,  0,  0,  0, 
  0,  0,  0,  0,  0,  0,  9,  8,-64,  0,
  0,  0,  0,  0,  0,  0,  0,  8,  8,  8, 
  8,-64,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  8,  8,-64,  0, 
  0,  0,  0,  0,  0,  0,128,-64,-64,-64,
-64,  4,  4, 64, 64,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
  0,  0,  0,  0,  0,  0};
