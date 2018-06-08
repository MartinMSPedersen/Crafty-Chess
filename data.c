#include <stdio.h>
#include <stdlib.h>
#include "chess.h"
SHARED *shared;
FILE *input_stream;
FILE *dbout;
FILE *book_file;
FILE *books_file;
FILE *normal_bs_file;
FILE *computer_bs_file;
FILE *history_file;
FILE *log_file;
FILE *auto_file;
FILE *position_file;
int done = 0;
BITBOARD total_moves;
int last_mate_score;
char log_filename[64];
char history_filename[64];
int number_of_solutions;
int solutions[10];
int solution_type;
char cmd_buffer[4096];
char *args[256];
char buffer[512];
unsigned char convert_buff[8];
int nargs;
int ponder_value;
int move_actually_played;
int ponder_move;
int ponder_moves[220];
int num_ponder_moves;
int book_move;
int book_learn_eval[LEARN_INTERVAL];
int book_learn_depth[LEARN_INTERVAL];
int learn_positions_count = 0;
int learn_seekto[64];
BITBOARD learn_key[64];
int learn_nmoves[64];
int book_learn_nmoves;
int book_learn_seekto;
BITBOARD book_learn_key;
int hash_mask;
unsigned int pawn_hash_mask;
HASH_ENTRY *trans_ref;
PAWN_HASH_ENTRY *pawn_hash_table;
size_t cb_trans_ref;
size_t cb_pawn_hash_table;
PATH last_pv;
int last_value;
int pval_b[64];
int kval_bn[64];
int kval_bk[64];
int kval_bq[64];
int king_safety[16][16];
int black_outpost[64];
signed char directions[64][64];
BITBOARD w_pawn_attacks[64];
BITBOARD b_pawn_attacks[64];
BITBOARD knight_attacks[64];
BITBOARD bishop_attacks_rl45[64][64];
BITBOARD bishop_attacks_rr45[64][64];
BITBOARD rook_attacks_r0[64][64];
BITBOARD rook_attacks_rr90[64][64];
POSITION display;
BITBOARD king_attacks[64];
BITBOARD obstructed[64][64];
BITBOARD w_pawn_random[64];
BITBOARD b_pawn_random[64];
BITBOARD w_knight_random[64];
BITBOARD b_knight_random[64];
BITBOARD w_bishop_random[64];
BITBOARD b_bishop_random[64];
BITBOARD w_rook_random[64];
BITBOARD b_rook_random[64];
BITBOARD w_queen_random[64];
BITBOARD b_queen_random[64];
BITBOARD w_king_random[64];
BITBOARD b_king_random[64];
BITBOARD stalemate_sqs[64];
BITBOARD edge_moves[64];
BITBOARD enpassant_random[65];
BITBOARD castle_random_w[2];
BITBOARD castle_random_b[2];
BITBOARD wtm_random[2];
BITBOARD clear_mask[65];
BITBOARD clear_mask_rr90[65];
BITBOARD clear_mask_rl45[65];
BITBOARD clear_mask_rr45[65];
BITBOARD set_mask[65];
BITBOARD set_mask_rr90[65];
BITBOARD set_mask_rl45[65];
BITBOARD set_mask_rr45[65];
BITBOARD file_mask[8];
BITBOARD rank_mask[8];
BITBOARD virgin_center_pawns;
BITBOARD mask_efgh, mask_fgh, mask_abc, mask_abcd;
BITBOARD mask_advance_2_w;
BITBOARD mask_advance_2_b;
BITBOARD mask_left_edge;
BITBOARD mask_right_edge;
BITBOARD mask_not_edge;
BITBOARD mask_A3B3;
BITBOARD mask_B3C3;
BITBOARD mask_F3G3;
BITBOARD mask_G3H3;
BITBOARD mask_A6B6;
BITBOARD mask_B6C6;
BITBOARD mask_F6G6;
BITBOARD mask_G6H6;
BITBOARD mask_white_OO;
BITBOARD mask_white_OOO;
BITBOARD mask_black_OO;
BITBOARD mask_black_OOO;
BITBOARD mask_kr_trapped_w[3];
BITBOARD mask_qr_trapped_w[3];
BITBOARD mask_kr_trapped_b[3];
BITBOARD mask_qr_trapped_b[3];
BITBOARD mask_corner_a1, mask_corner_h1;
BITBOARD mask_corner_a8, mask_corner_h8;
BITBOARD mask_center_files;
BITBOARD mask_d3d4d5d6;
BITBOARD mask_e3e4e5e6;
BITBOARD light_squares;
BITBOARD dark_squares;
BITBOARD not_rook_pawns;
BITBOARD plus1dir[65];
BITBOARD plus7dir[65];
BITBOARD plus8dir[65];
BITBOARD plus9dir[65];
BITBOARD minus1dir[65];
BITBOARD minus7dir[65];
BITBOARD minus8dir[65];
BITBOARD minus9dir[65];
BITBOARD mask_eptest[64];
BITBOARD mask_clear_entry;

#if (!defined(_M_AMD64) && !defined (_M_IA64) && !defined(INLINE32)) || defined(VC_INLINE32)
unsigned char msb[65536];
unsigned char lsb[65536];
#endif
unsigned char msb_8bit[256];
unsigned char lsb_8bit[256];
unsigned char pop_cnt_8bit[256];
unsigned char connected_passed[256];
unsigned char file_spread[256];
signed char is_outside[256][256];
signed char is_outside_c[256][256];
BITBOARD mask_pawn_protected_b[64];
BITBOARD mask_pawn_protected_w[64];
BITBOARD mask_pawn_duo[64];
BITBOARD mask_pawn_isolated[64];
BITBOARD mask_pawn_passed_w[64];
BITBOARD mask_pawn_passed_b[64];
BITBOARD mask_no_pattacks_w[64];
BITBOARD mask_no_pattacks_b[64];
BITBOARD white_pawn_race_wtm[64];
BITBOARD white_pawn_race_btm[64];
BITBOARD black_pawn_race_wtm[64];
BITBOARD black_pawn_race_btm[64];
BOOK_POSITION book_buffer[BOOK_CLUSTER_SIZE];
BOOK_POSITION book_buffer_char[BOOK_CLUSTER_SIZE];

#define    VERSION                             "21.1"
char version[8] = { VERSION };
PLAYING_MODE mode = normal_mode;
int batch_mode = 0;             /* no asynch reads */
int swindle_mode = 1;           /* try to swindle */
int call_flag = 0;
int crafty_rating = 2500;
int opponent_rating = 2500;
int last_search_value = 0;
int lazy_eval_cutoff = 350;
int DGT_active = 0;
int to_dgt = 0;
int from_dgt = 0;
int pgn_suggested_percent = 0;
char pgn_event[128] = { "?" };
char pgn_site[128] = { "?" };
char pgn_date[128] = { "????.??.??" };
char pgn_round[128] = { "?" };
char pgn_white[128] = { "unknown" };
char pgn_white_elo[128] = { "" };
char pgn_black[128] = { "Crafty " VERSION };
char pgn_black_elo[128] = { "" };
char pgn_result[128] = { "*" };
char *B_list[128];
char *AK_list[128];
char *C_list[128];
char *GM_list[128];
char *IM_list[128];
char *SP_list[128];
char *SP_opening_filename[128];
char *SP_personality_filename[128];
int ics = 0;
int output_format = 0;

#if !defined(NOEGTB)
int EGTBlimit = 0;
int EGTB_use = 0;
int EGTB_draw = 0;
int EGTB_search = 0;
size_t EGTB_cache_size = EGTB_CACHE_DEFAULT;
void *EGTB_cache = (void *) 0;
int EGTB_setup = 0;
#endif
int xboard = 0;
int pong = 0;
int channel = 0;
int early_exit = 99;
int new_game = 0;
char channel_title[32] = { "" };
char book_path[128] = { BOOKDIR };
char personality_path[128] = { PERSDIR };
char log_path[128] = { LOGDIR };
char tb_path[128] = { TBDIR };
char rc_path[128] = { RCDIR };
int initialized = 0;
int kibitz = 0;
int post = 0;
int log_id = 0;
int wtm = 1;
int last_opponent_move = 0;
int incheck_depth = 4;
int onerep_depth = 3;
int mate_depth = 3;
int null_min = 3 * PLY;         /* R=2 */
int null_max = 4 * PLY;         /* R=3 */
int reduce_min_depth = PLY;     /* leave 1 good ply after reductions */
int reduce_value = PLY;         /* reduce 1 ply */
int reduce_hist = 80;           /* ok if move fails high < 80% of time */
int search_depth = 0;
unsigned int search_nodes = 0;
int search_move = 0;
int predicted = 0;
int time_used = 0;
int time_used_opponent = 0;
int analyze_mode = 0;
int annotate_mode = 0;
int input_status = 0;
int resign = 9;
int resign_counter = 0;
int resign_count = 5;
int draw_counter = 0;
int draw_count = 5;
int draw_offer_pending = 0;
int offer_draws = 1;
int adaptive_hash = 0;
size_t adaptive_hash_min = 0;
size_t adaptive_hash_max = 0;
size_t adaptive_hashp_min = 0;
size_t adaptive_hashp_max = 0;
int time_limit = 100;
int force = 0;
char initial_position[80] = { "" };
char hint[512] = { "" };
char book_hint[512] = { "" };
int over = 0;
int silent = 0;
int usage_level = 0;
char audible_alarm = 0x07;
char speech = 0;
int ansi = 1;
int book_accept_mask = ~03;
int book_reject_mask = 3;
int book_random = 1;
float book_weight_learn = 1.0;
float book_weight_freq = 1.0;
float book_weight_CAP = 0.7;
float book_weight_eval = 0.5;
int book_search_trigger = 20;
int learning = 7;
int learning_cutoff = -2 * PAWN_VALUE;
int learning_trigger = PAWN_VALUE / 3;
int show_book = 0;
int book_selection_width = 5;
int ponder = 1;
int trace_level = 0;
size_t hash_table_size = 65536;
int log_hash = 16;
size_t pawn_hash_table_size = 32768;
int log_pawn_hash = 15;
int abs_draw_score = 1;
int accept_draws = 1;
unsigned char bishop_shift_rr45[64] = {
   1,  2,  4,  7, 11, 16, 22, 29,
   2,  4,  7, 11, 16, 22, 29, 37,
   4,  7, 11, 16, 22, 29, 37, 44,
   7, 11, 16, 22, 29, 37, 44, 50,
  11, 16, 22, 29, 37, 44, 50, 55,
  16, 22, 29, 37, 44, 50, 55, 59,
  22, 29, 37, 44, 50, 55, 59, 62,
  29, 37, 44, 50, 55, 59, 62, 64
};
unsigned char bishop_shift_rl45[64] = {
  29, 22, 16, 11,  7,  4,  2,  1,
  37, 29, 22, 16, 11,  7,  4,  2,
  44, 37, 29, 22, 16, 11,  7,  4,
  50, 44, 37, 29, 22, 16, 11,  7,
  55, 50, 44, 37, 29, 22, 16, 11,
  59, 55, 50, 44, 37, 29, 22, 16,
  62, 59, 55, 50, 44, 37, 29, 22,
  64, 62, 59, 55, 50, 44, 37, 29
};
const char xlate[15] =
    { 'q', 'r', 'b', 0, 'k', 'n', 'p', 0, 'P', 'N', 'K', 0, 'B', 'R', 'Q' };
const char empty[9] = { 0, '1', '2', '3', '4', '5', '6', '7', '8' };
int king_tropism_n[8] = { 0, 3, 3, 2, 1, 0, 0, 0 };
int king_tropism_b[8] = { 0, 2, 2, 1, 0, 0, 0, 0 };
int king_tropism_r[8] = { 0, 4, 3, 2, 1, 1, 1, 1 };
int king_tropism_at_r[8] = { 4, 3, 2, 2, 2, 2, 2, 2 };
int king_tropism_q[8] = { 0, 6, 5, 4, 3, 2, 2, 2 };
int king_tropism_at_q[8] = { 6, 5, 4, 3, 3, 3, 3, 3 };

int connected_passed_pawn_value[8] = { 0, 0, 10, 20, 40, 100, 200, 0 };
int hidden_passed_pawn_value[8] = { 0, 0, 0, 0, 20, 40, 0, 0 };
int passed_pawn_value[8] = { 0, 12, 20, 48, 72, 120, 150, 0 };
int blockading_passed_pawn_value[8] = { 0, 6, 10, 24, 36, 60, 75, 0 };
int isolated_pawn_value[9] = { 0, 8, 20, 40, 60, 70, 80, 80, 80 };
int isolated_pawn_of_value[9] = { 0, 4, 10, 16, 24, 24, 24, 24, 24 };
int doubled_pawn_value[9] = { 0, 0, 4, 7, 10, 10, 10, 10, 10 };
int doubled_isolated_pawn_value[9] = { 0, 5, 10, 15, 15, 15, 15, 15, 15 };
int supported_passer[8] = { 0, 0, 0, 20, 40, 60, 100, 0 };
int outside_passed[128] = {
  160, 100, 100, 100, 80, 80, 80, 70,
   60,  50,  40,  40, 30, 30, 20, 20,
   16,  16,  10,  10,  0,  0,  0,  0,
    0,   0,   0,   0,  0,  0,  0,  0,
    0,   0,   0,   0,  0,  0,  0,  0,
    0,   0,   0,   0,  0,  0,  0,  0,
    0,   0,   0,   0,  0,  0,  0,  0,
    0,   0,   0,   0,  0,  0,  0,  0,
    0,   0,   0,   0,  0,  0,  0,  0,
    0,   0,   0,   0,  0,  0,  0,  0,
    0,   0,   0,   0,  0,  0,  0,  0,
    0,   0,   0,   0,  0,  0,  0,  0,
    0,   0,   0,   0,  0,  0,  0,  0,
    0,   0,   0,   0,  0,  0,  0,  0,
    0,   0,   0,   0,  0,  0,  0,  0,
    0,   0,   0,   0,  0,  0,  0,  0
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
   30, 40, 50, 60, 70, 80, 90, 100,
   40, 30, 40, 50, 60, 70, 80,  90,
   50, 40, 30, 40, 50, 60, 70,  80,
   60, 50, 40, 30, 40, 50, 60,  70,
   70, 60, 50, 40, 30, 40, 50,  60,
   80, 70, 60, 50, 40, 30, 40,  50,
   90, 80, 70, 60, 50, 40, 30,  40,
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
int sqrvalB[64] = {
    1,   1,   1,   1,   1,   1,   1,   1,
    1,   2,   2,   2,   2,   2,   2,   1,
    1,   2,   3,   3,   3,   3,   2,   1,
    1,   2,   3,   4,   4,   3,   2,   1,
    1,   2,   3,   4,   4,   3,   2,   1,
    1,   2,   3,   3,   3,   3,   2,   1,
    1,   2,   2,   2,   2,   2,   2,   1,
    1,   1,   1,   1,   1,   1,   1,   1
};
int sqrvalR[64] = {
    1,   2,   3,   4,   4,   3,   2,   1,
    1,   2,   3,   4,   4,   3,   2,   1,
    1,   2,   3,   4,   4,   3,   2,   1,
    1,   2,   3,   4,   4,   3,   2,   1,
    1,   2,   3,   4,   4,   3,   2,   1,
    1,   2,   3,   4,   4,   3,   2,   1,
    1,   2,   3,   4,   4,   3,   2,   1,
    1,   2,   3,   4,   4,   3,   2,   1
};
int white_outpost[64] = {
  0, 0,  0,  0,  0,  0, 0, 0,
  0, 0,  0,  0,  0,  0, 0, 0,
  0, 0,  0,  0,  0,  0, 0, 0,
  0, 5, 10, 20, 20, 10, 5, 0,
  0, 5, 10, 24, 24, 10, 5, 0,
  0, 0, 10, 24, 24, 10, 0, 0,
  0, 0,  0,  0,  0,  0, 0, 0,
  0, 0,  0,  0,  0,  0, 0, 0
};
int pval_w[64] = {
   0,  0,  0,   0,   0,  0,  0,  0,
   0,  0,  0, -10, -10,  0,  0,  0,
   1,  1,  1,  10,  10,  1,  1,  1,
   3,  3,  3,  13,  13,  3,  3,  3,
   6,  6,  6,  16,  16,  6,  6,  6,
  10, 10, 10,  30,  30, 10, 10, 10,
  40, 40, 40,  40,  40, 40, 40, 40,
   0,  0,  0,   0,   0,  0,  0,  0
};
int nval[64] = {
  -60, -30, -30, -30, -30, -30, -30, -60,
  -30, -24, -10, -10, -10, -10, -24, -30,
  -30,  -6,  -6,  -6,  -6,  -6,  -6, -30,
  -30,  -6,  -2,   0,   0,  -2,  -6, -30,
  -30,  -6,  -2,   0,   0,  -2,  -6, -30,
  -30,  -6,  -6,  -6,  -6,  -6,  -6, -30,
  -30, -24, -10, -10, -10, -10, -24, -30,
  -60, -30, -30, -30, -30, -30, -30, -60,
};
int qval[64] = {
  -20, -20,   0,   0,   0,   0, -20, -20,
  -20,   0,   8,   8,   8,   8,   0, -20,
     0,  8,   8,  12,  12,   8,   8,   0,
     0,  8,  12,  16,  16,  12,   8,   0,
     0,  8,  12,  16,  16,  12,   8,   0,
     0,  8,   8,  12,  12,   8,   8,   0,
  -20,   0,   8,   8,   8,   8,   0, -20,
  -20, -20,   0,   0,   0,   0, -20, -20,
};
int kval_wn[64] = {
  -40, -20, -20, -20, -20, -20, -20, -40,
  -20,   0,   0,   0,   0,   0,   0, -20,
  -20,  20,  20,  20,  20,  20,  20, -20,
  -20,  20,  40,  40,  40,  40,  20, -20,
  -20,  30,  60,  60,  60,  60,  30, -20,
  -20,  30,  60,  60,  60,  60,  30, -20,
  -20,  30,  40,  40,  40,  40,  30, -20,
  -40, -20, -20, -20, -20, -20, -20, -40
};
int kval_wk[64] = {
  -60, -40, -20, -20, -20, -20, -20, -20,
  -60, -40, -20,   0,   0,   0,   0,   0,
  -60, -40, -20,  20,  20,  20,  20,  20,
  -60, -40, -20,  20,  40,  40,  40,  40,
  -60, -40, -20,  20,  60,  60,  60,  40,
  -60, -40, -20,  20,  60,  60,  60,  40,
  -60, -40, -20,  20,  40,  40,  40,  40,
  -60, -40, -20, -20, -20, -20, -20, -20
};
int kval_wq[64] = {
  -20, -20, -20, -20, -20, -20, -40, -60,
    0,   0,   0,   0,   0, -20, -40, -60,
   20,  20,  20,  20,  20, -20, -40, -60,
   40,  40,  40,  40,  20, -20, -40, -60,
   40,  60,  60,  60,  20, -20, -40, -60,
   40,  60,  60,  60,  20, -20, -40, -60,
   40,  40,  40,  40,  20, -20, -40, -60,
  -20, -20, -20, -20, -20, -20, -40, -60
};
int safety_vector[16] = {
    0,  7, 14, 21, 28, 35, 42, 49,
   56, 63, 70, 77, 84, 91, 98, 105
};
int tropism_vector[16] = {
   0,  1,  2,  3,   4,   5,  11,  20,
  32, 47, 65, 86, 110, 137, 167, 200
};

/* note that black piece/square values are copied from white, but
   reflected */
const int p_values[15] = { QUEEN_VALUE, ROOK_VALUE, BISHOP_VALUE, 0,
  KING_VALUE, KNIGHT_VALUE, PAWN_VALUE, 0, PAWN_VALUE, KNIGHT_VALUE,
  KING_VALUE, 0, BISHOP_VALUE, ROOK_VALUE, QUEEN_VALUE
};
const int p_vals[8] =
    { 0, pawn_v, knight_v, king_v, 0, bishop_v, rook_v, queen_v
};
int pawn_value = PAWN_VALUE;
int knight_value = KNIGHT_VALUE;
int bishop_value = BISHOP_VALUE;
int rook_value = ROOK_VALUE;
int queen_value = QUEEN_VALUE;
int king_value = KING_VALUE;
int pawn_can_promote = 525;
int bad_trade = 150;
int pawns_blocked = 2;
int center_pawn_unmoved = 8;
int pawn_duo = 2;
int pawn_weak_p1 = 12;
int pawn_weak_p2 = 20;
int pawn_protected_passer_wins = 50;
int won_kp_ending = 200;
int split_passed = 50;
int king_king_tropism = 15;
int bishop_trapped = 174;
int slider_with_wing_pawns = 36;
int pinOrDiscoveredCheck = 4;
int weakPawnScore[4] = {4, 6, 5, 9};
int weakPawnScoreVert[4] = {5, 9, 5, 9};
/*
    this is indexed by the center status (open, ..., blocked)
    as a bishop pair is more important when the board has opened
    up so that mobility is high.  0 is blocked, 60 is open, middle
    value is in-between.
*/
int bishop_pair[3] = { 0, 30, 60 };
int rook_on_7th = 24;
int rook_connected_7th_rank = 10;

/*
    each row contains values for a rook occupying a specific open
    file starting with file A and going through file H.  Row 0 is
    not used, row 1 is used when there is only one open file on
    the board, row 2 is used when there are two openfiles on the
    board, etc.  obviously control of a file is more important if
    there is only one or two files available.
*/
int rook_open_file[9][8] = {
  {  0,  0,  0,  0,  0,  0,  0,  0 },
  { 20, 27, 35, 40, 40, 35, 27, 20 },
  { 13, 18, 23, 26, 26, 23, 18, 13 },
  { 10, 13, 17, 20, 20, 17, 13, 10 },
  { 10, 13, 17, 20, 20, 17, 13, 10 },
  { 10, 13, 17, 20, 20, 17, 13, 10 },
  { 10, 13, 17, 20, 20, 17, 13, 10 },
  { 10, 13, 17, 20, 20, 17, 13, 10 },
  { 10, 13, 17, 20, 20, 17, 13, 10 }
};
int rook_reaches_open_file = 16;
int rook_half_open_file = 10;
int rook_behind_passed_pawn = 24;
int rook_trapped = 40;
int queen_rook_on_7th_rank = 50;
int queen_offside = 30;
int open_file[8] = { 6, 5, 4, 4, 4, 4, 5, 6 };
int half_open_file[8] = { 4, 4, 3, 3, 3, 3, 4, 4 };
int king_safety_mate_threat = 600;
int development_thematic = 12;
int blocked_center_pawn = 12;
int development_losing_castle = 20;
int development_not_castled = 20;

// Directions.
int UP = +8;
int UP_RIGHT = +9;
int RIGHT = +1;
int DOWN_RIGHT = -7;
int DOWN = -8;
int DOWN_LEFT = -9;
int LEFT = -1;
int UP_LEFT = +7;

/*
   First term is a character string explaining what the eval
   term is used for.  Second term is the "size" of the thing.
   0 means scalar value, + numbers mean an array, -64 means an
   array of 64 but displayed as a 8 x 8 chess board, to make it
   easier to understand.  When entering this data, a1 is first
   value, etc, down to h8.  When the data is displayed, however,
   it is displayed as an 8 x 8 matrix with a1 = lower left corner,
   for easier visualization.  Third term is a pointer to the
   data value(s).
*/
struct eval_term eval_packet[256] = {
  {"raw piece values----------------", 0, NULL},        /* 0 */
  {"pawn value                      ", 0, &pawn_value},
  {"knight value                    ", 0, &knight_value},
  {"bishop value                    ", 0, &bishop_value},
  {"rook value                      ", 0, &rook_value},
  {"queen value                     ", 0, &queen_value},
  {"bad trade bonus/penalty         ", 0, &bad_trade},
  {NULL, 0, NULL},
  {NULL, 0, NULL},
  {NULL, 0, NULL},
  {NULL, 0, NULL},
  {NULL, 0, NULL},
  {NULL, 0, NULL},
  {NULL, 0, NULL},
  {NULL, 0, NULL},
  {NULL, 0, NULL},
  {NULL, 0, NULL},
  {NULL, 0, NULL},
  {NULL, 0, NULL},
  {NULL, 0, NULL},
  {"pawn evaluation-----------------", 0, NULL},        /* 20 */
  {"center pawn blocked             ", 0, &pawns_blocked},
  {"center pawn unmoved             ", 0, &center_pawn_unmoved},
  {"pawn duo                        ", 0, &pawn_duo},
  {"protected passed pawn wins      ", 0, &pawn_protected_passer_wins},
  {"pawn weak (one pawn blocking)   ", 0, &pawn_weak_p1},
  {"pawn weak (two pawns blocking)  ", 0, &pawn_weak_p2},
  {"pawn can promote                ", 0, &pawn_can_promote},
  {"won kp ending                   ", 0, &won_kp_ending},
  {"split passed pawn bonus         ", 0, &split_passed},
  {"pawn piece/square table         ", -64, pval_w},
  {"connected passed pawn [rank]    ", 8, connected_passed_pawn_value},
  {"hidden passed pawn [rank]       ", 8, hidden_passed_pawn_value},
  {"passed pawn [rank]              ", 8, passed_pawn_value},
  {"blockading a passed pawn [rank] ", 8, blockading_passed_pawn_value},
  {"isolated pawn [n]               ", 9, isolated_pawn_value},
  {"isolated pawn on open file [n]  ", 9, isolated_pawn_of_value},
  {"doubled pawn [n]                ", 9, doubled_pawn_value},
  {"doubled isolated pawn [n]       ", 9, doubled_isolated_pawn_value},
  {"supported passed pawn [rank]    ", 8, supported_passer},
  {"outside passed pawn [matrl]     ", 128, outside_passed},
  {NULL, 0, NULL},
  {NULL, 0, NULL},
  {NULL, 0, NULL},
  {NULL, 0, NULL},
  {NULL, 0, NULL},
  {NULL, 0, NULL},
  {NULL, 0, NULL},
  {NULL, 0, NULL},
  {NULL, 0, NULL},
  {"knight scoring------------------", 0, NULL},        /* 50 */
  {"king tropism [distance]         ", 8, king_tropism_n},
  {"knight piece/square table       ", -64, nval},
  {"outpost [square]                ", -64, white_outpost},
  {NULL, 0, NULL},
  {NULL, 0, NULL},
  {NULL, 0, NULL},
  {NULL, 0, NULL},
  {NULL, 0, NULL},
  {NULL, 0, NULL},
  {"bishop scoring------------------", 0, NULL},        /* 60 */
  {"bishop over knight endgame      ", 0, &slider_with_wing_pawns},
  {"bishop trapped                  ", 0, &bishop_trapped},
  {"bishop pair                     ", 3, bishop_pair},
  {"king tropism [distance]         ", 8, king_tropism_b},
  {"bishop mobility/square table    ", -64, sqrvalB},
  {NULL, 0, NULL},
  {NULL, 0, NULL},
  {NULL, 0, NULL},
  {NULL, 0, NULL},
  {"rook scoring--------------------", 0, NULL},        /* 70 */
  {"rook on 7th                     ", 0, &rook_on_7th},
  {"rook connected 7th rank         ", 0, &rook_connected_7th_rank},
  {"rook trapped                    ", 0, &rook_trapped},
  {"rook behind passed pawn         ", 0, &rook_behind_passed_pawn},
  {"rook half open file             ", 0, &rook_half_open_file},
  {"rook open file [9][8]           ", 72, &rook_open_file[0][0]},
  {"rook reaches open file          ", 0, &rook_reaches_open_file},
  {"king tropism [distance]         ", 8, king_tropism_r},
  {"king file tropism [distance]    ", 8, king_tropism_at_r},
  {"rook mobility/square table      ", -64, sqrvalR},
  {NULL, 0, NULL},
  {NULL, 0, NULL},
  {NULL, 0, NULL},
  {NULL, 0, NULL},
  {NULL, 0, NULL},
  {NULL, 0, NULL},
  {NULL, 0, NULL},
  {NULL, 0, NULL},
  {NULL, 0, NULL},
  {"queen scoring-------------------", 0, NULL},        /* 90 */
  {"queen rook on 7th rank          ", 0, &queen_rook_on_7th_rank},
  {"queen offside                   ", 0, &queen_offside},
  {"king tropism [distance]         ", 8, king_tropism_q},
  {"king file tropism [distance]    ", 8, king_tropism_at_q},
  {"queen piece/square table        ", -64, qval},
  {NULL, 0, NULL},
  {NULL, 0, NULL},
  {NULL, 0, NULL},
  {NULL, 0, NULL},
  {"king scoring--------------------", 0, NULL},        /* 100 */
  {"king king tropism (endgame)     ", 0, &king_king_tropism},
  {"king safety trojan horse threat ", 0, &king_safety_mate_threat},
  {"king piece/square normal        ", -64, kval_wn},
  {"king piece/square kside pawns   ", -64, kval_wk},
  {"king piece/square qside pawns   ", -64, kval_wq},
  {"king safe open file [file]      ", 8, open_file},
  {"king safe half-open file [file] ", 8, half_open_file},
  {"king safety pawn-shield vector  ", 16, safety_vector},
  {"king safety tropism vector      ", 16, tropism_vector},
  {NULL, 0, NULL},
  {NULL, 0, NULL},
  {NULL, 0, NULL},
  {NULL, 0, NULL},
  {NULL, 0, NULL},
  {NULL, 0, NULL},
  {NULL, 0, NULL},
  {NULL, 0, NULL},
  {NULL, 0, NULL},
  {NULL, 0, NULL},
  {"development scoring-------------", 0, NULL},        /* 120 */
  {"development thematic            ", 0, &development_thematic},
  {"development blocked center pawn ", 0, &blocked_center_pawn},
  {"development losing castle       ", 0, &development_losing_castle},
  {"development not castled         ", 0, &development_not_castled},
};
