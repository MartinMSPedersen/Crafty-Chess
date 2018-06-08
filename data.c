#include "chess.h"
/* *INDENT-OFF* */
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
char *args[512];
char buffer[4096];
int line_length = 80;
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
int king_safety[16][16];
signed char directions[64][64];
BITBOARD pawn_attacks[2][64];
BITBOARD knight_attacks[64];
BITBOARD king_attacks[64];
BITBOARD obstructed[64][64];
BITBOARD randoms[2][7][64];
BITBOARD castle_random[2][2];
BITBOARD enpassant_random[65];
BITBOARD wtm_random[2];
BITBOARD clear_mask[65];
BITBOARD set_mask[65];
BITBOARD file_mask[8];
BITBOARD rank_mask[8];
BITBOARD OO[2] = { 0x6000000000000000ULL, 0x0000000000000060ULL };
BITBOARD OOO[2] = { 0x0E00000000000000ULL, 0x000000000000000EULL };
BITBOARD mask_efgh, mask_fgh, mask_abc, mask_abcd;
BITBOARD mask_advance_2_w;
BITBOARD mask_advance_2_b;
BITBOARD mask_left_edge;
BITBOARD mask_right_edge;
BITBOARD mask_not_edge;
BITBOARD mask_kr_trapped[2][3];
BITBOARD mask_qr_trapped[2][3];
BITBOARD dark_squares;
BITBOARD not_rook_pawns;
BITBOARD rook_pawns;
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
POSITION display;
#if (!defined(_M_AMD64) && !defined (_M_IA64) && !defined(INLINE32)) || defined(VC_INLINE32)
unsigned char msb[65536];
unsigned char lsb[65536];
#endif
unsigned char msb_8bit[256];
unsigned char lsb_8bit[256];
unsigned char islands[256];
unsigned char pop_cnt_8bit[256];
unsigned char connected_passed[256];
unsigned char file_spread[256];
signed char is_outside[256][256];
BITBOARD mask_pawn_connected[64];
BITBOARD mask_pawn_duo[64];
BITBOARD mask_pawn_isolated[64];
BITBOARD mask_pawn_passed[2][64];
BITBOARD mask_no_pattacks[2][64];
BITBOARD mask_hidden_left[2][8];
BITBOARD mask_hidden_right[2][8];
BITBOARD pawn_race[2][2][64];
BITBOARD pawn_race[2][2][64];
BOOK_POSITION book_buffer[BOOK_CLUSTER_SIZE];
BOOK_POSITION book_buffer_char[BOOK_CLUSTER_SIZE];
int OOsqs[2][3] = { {E8, F8, G8}, {E1, F1, G1} };
int OOOsqs[2][3] = { {E8, D8, C8}, {E1, D1, C1} };
int OOfrom[2] = { E8, E1 };
int OOto[2] = { G8, G1 };
int OOOto[2] = { C8, C1 };
#define    VERSION                             "22.3"
char version[8] = { VERSION };
PLAYING_MODE mode = normal_mode;
int batch_mode = 0;             /* no asynch reads */
int swindle_mode = 1;           /* try to swindle */
int call_flag = 0;
int crafty_rating = 2500;
int opponent_rating = 2500;
int last_search_value = 0;
int lazy_eval_cutoff = 125;
int razor_margin = 300;                        /* Heinz = QUEEN    */
int futility_margin = 125;                     /* Heinz = 2 PAWNS  */
int extended_futility_margin = 300;            /* Heinz = ROOK     */
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
char log_path[128] = { LOGDIR };
char tb_path[128] = { TBDIR };
char rc_path[128] = { RCDIR };
int initialized = 0;
int kibitz = 0;
int post = 0;
int log_id = 0;
int wtm = 1;
int last_opponent_move = 0;
int check_depth = 1;
int null_depth = 3;               /* R=3 */
int LMR_min_depth = 1;            /* leave 1 full ply after reductions */
int LMR_depth = 1;                /* reduce 1 ply */
int search_depth = 0;
unsigned int search_nodes = 0;
unsigned int temp_search_nodes = 0;
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
int draw_offered = 0;
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
float book_weight_eval = 0.1;
int book_search_trigger = 20;
int learning = 7;
int time_abort;
int abort_search;
int iteration_depth;
int root_alpha;
int root_beta;
int root_value;
int root_wtm;
int last_root_value;
ROOT_MOVE root_moves[256];
int n_root_moves;
int easy_move;
int time_limit;
int absolute_time_limit;
int search_time_limit;
int burp;
volatile int quit;
unsigned int opponent_start_time, opponent_end_time;
unsigned int program_start_time, program_end_time;
unsigned int start_time, end_time;
unsigned int elapsed_start, elapsed_end;
TREE *block[MAX_BLOCKS + 1];
TREE *volatile thread[CPUS];
#if (CPUS > 1)
lock_t lock_smp, lock_io, lock_root;
#endif
unsigned int parallel_splits;
unsigned int parallel_aborts;
unsigned int max_split_blocks;
volatile int smp_idle = 0;
volatile int smp_threads = 0;
volatile int initialized_threads = 0;
int crafty_is_white = 0;
int average_nps = 0;
int nodes_between_time_checks = 1000000;
int nodes_per_second = 1000000;
int next_time_check = 100000;
int transposition_id = 0;
int thinking = 0;
int pondering = 0;
int puzzling = 0;
int booking = 0;
int trojan_check = 0;
int computer_opponent = 0;
int display_options = 4095 - 256 - 512;
int max_threads = 0;
int min_thread_depth = 40;
int max_thread_group = 4;
int split_at_root = 1;
unsigned int noise_level = 200000;
int tc_moves = 60;
int tc_time = 180000;
int tc_time_remaining = 180000;
int tc_time_remaining_opponent = 180000;
int tc_moves_remaining = 60;
int tc_secondary_moves = 30;
int tc_secondary_time = 90000;
int tc_increment = 0;
int tc_sudden_death = 0;
int tc_operator_time = 0;
int tc_safety_margin = 0;
int draw_score[2] = { 0, 0 };
char kibitz_text[512];
int kibitz_depth;
int move_number = 1;
int root_print_ok = 0;
int moves_out_of_book = 0;
int first_nonbook_factor = 0;
int first_nonbook_span = 0;
int smp_nice = 1;
#if defined(SKILL)
int skill = 100;
#endif
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
const char translate[13] =
    { 'k', 'q', 'r', 'b', 'n', 'p', 0, 'P', 'N', 'B', 'R', 'Q', 'K' };
BITBOARD magic_bishop[64] = {
  0x0002020202020200ULL, 0x0002020202020000ULL, 0x0004010202000000ULL,
  0x0004040080000000ULL, 0x0001104000000000ULL, 0x0000821040000000ULL,
  0x0000410410400000ULL, 0x0000104104104000ULL, 0x0000040404040400ULL,
  0x0000020202020200ULL, 0x0000040102020000ULL, 0x0000040400800000ULL,
  0x0000011040000000ULL, 0x0000008210400000ULL, 0x0000004104104000ULL,
  0x0000002082082000ULL, 0x0004000808080800ULL, 0x0002000404040400ULL,
  0x0001000202020200ULL, 0x0000800802004000ULL, 0x0000800400A00000ULL,
  0x0000200100884000ULL, 0x0000400082082000ULL, 0x0000200041041000ULL,
  0x0002080010101000ULL, 0x0001040008080800ULL, 0x0000208004010400ULL,
  0x0000404004010200ULL, 0x0000840000802000ULL, 0x0000404002011000ULL,
  0x0000808001041000ULL, 0x0000404000820800ULL, 0x0001041000202000ULL,
  0x0000820800101000ULL, 0x0000104400080800ULL, 0x0000020080080080ULL,
  0x0000404040040100ULL, 0x0000808100020100ULL, 0x0001010100020800ULL,
  0x0000808080010400ULL, 0x0000820820004000ULL, 0x0000410410002000ULL,
  0x0000082088001000ULL, 0x0000002011000800ULL, 0x0000080100400400ULL,
  0x0001010101000200ULL, 0x0002020202000400ULL, 0x0001010101000200ULL,
  0x0000410410400000ULL, 0x0000208208200000ULL, 0x0000002084100000ULL,
  0x0000000020880000ULL, 0x0000001002020000ULL, 0x0000040408020000ULL,
  0x0004040404040000ULL, 0x0002020202020000ULL, 0x0000104104104000ULL,
  0x0000002082082000ULL, 0x0000000020841000ULL, 0x0000000000208800ULL,
  0x0000000010020200ULL, 0x0000000404080200ULL, 0x0000040404040400ULL,
  0x0002020202020200ULL
};
BITBOARD magic_bishop_mask[64] = {
  0x0040201008040200ULL, 0x0000402010080400ULL, 0x0000004020100A00ULL,
  0x0000000040221400ULL, 0x0000000002442800ULL, 0x0000000204085000ULL,
  0x0000020408102000ULL, 0x0002040810204000ULL, 0x0020100804020000ULL,
  0x0040201008040000ULL, 0x00004020100A0000ULL, 0x0000004022140000ULL,
  0x0000000244280000ULL, 0x0000020408500000ULL, 0x0002040810200000ULL,
  0x0004081020400000ULL, 0x0010080402000200ULL, 0x0020100804000400ULL,
  0x004020100A000A00ULL, 0x0000402214001400ULL, 0x0000024428002800ULL,
  0x0002040850005000ULL, 0x0004081020002000ULL, 0x0008102040004000ULL,
  0x0008040200020400ULL, 0x0010080400040800ULL, 0x0020100A000A1000ULL,
  0x0040221400142200ULL, 0x0002442800284400ULL, 0x0004085000500800ULL,
  0x0008102000201000ULL, 0x0010204000402000ULL, 0x0004020002040800ULL,
  0x0008040004081000ULL, 0x00100A000A102000ULL, 0x0022140014224000ULL,
  0x0044280028440200ULL, 0x0008500050080400ULL, 0x0010200020100800ULL,
  0x0020400040201000ULL, 0x0002000204081000ULL, 0x0004000408102000ULL,
  0x000A000A10204000ULL, 0x0014001422400000ULL, 0x0028002844020000ULL,
  0x0050005008040200ULL, 0x0020002010080400ULL, 0x0040004020100800ULL,
  0x0000020408102000ULL, 0x0000040810204000ULL, 0x00000A1020400000ULL,
  0x0000142240000000ULL, 0x0000284402000000ULL, 0x0000500804020000ULL,
  0x0000201008040200ULL, 0x0000402010080400ULL, 0x0002040810204000ULL,
  0x0004081020400000ULL, 0x000A102040000000ULL, 0x0014224000000000ULL,
  0x0028440200000000ULL, 0x0050080402000000ULL, 0x0020100804020000ULL,
  0x0040201008040200ULL
};
unsigned int magic_bishop_shift[64] = {
  58, 59, 59, 59, 59, 59, 59, 58,
  59, 59, 59, 59, 59, 59, 59, 59,
  59, 59, 57, 57, 57, 57, 59, 59,
  59, 59, 57, 55, 55, 57, 59, 59,
  59, 59, 57, 55, 55, 57, 59, 59,
  59, 59, 57, 57, 57, 57, 59, 59,
  59, 59, 59, 59, 59, 59, 59, 59,
  58, 59, 59, 59, 59, 59, 59, 58
};
BITBOARD magic_bishop_table[5248];
BITBOARD *magic_bishop_indices[64] = {
  magic_bishop_table + 4992, magic_bishop_table + 2624,
  magic_bishop_table + 256, magic_bishop_table + 896,
  magic_bishop_table + 1280, magic_bishop_table + 1664,
  magic_bishop_table + 4800, magic_bishop_table + 5120,
  magic_bishop_table + 2560, magic_bishop_table + 2656,
  magic_bishop_table + 288, magic_bishop_table + 928,
  magic_bishop_table + 1312, magic_bishop_table + 1696,
  magic_bishop_table + 4832, magic_bishop_table + 4928,
  magic_bishop_table + 0, magic_bishop_table + 128,
  magic_bishop_table + 320, magic_bishop_table + 960,
  magic_bishop_table + 1344, magic_bishop_table + 1728,
  magic_bishop_table + 2304, magic_bishop_table + 2432,
  magic_bishop_table + 32, magic_bishop_table + 160,
  magic_bishop_table + 448, magic_bishop_table + 2752,
  magic_bishop_table + 3776, magic_bishop_table + 1856,
  magic_bishop_table + 2336, magic_bishop_table + 2464,
  magic_bishop_table + 64, magic_bishop_table + 192,
  magic_bishop_table + 576, magic_bishop_table + 3264,
  magic_bishop_table + 4288, magic_bishop_table + 1984,
  magic_bishop_table + 2368, magic_bishop_table + 2496,
  magic_bishop_table + 96, magic_bishop_table + 224,
  magic_bishop_table + 704, magic_bishop_table + 1088,
  magic_bishop_table + 1472, magic_bishop_table + 2112,
  magic_bishop_table + 2400, magic_bishop_table + 2528,
  magic_bishop_table + 2592, magic_bishop_table + 2688,
  magic_bishop_table + 832, magic_bishop_table + 1216,
  magic_bishop_table + 1600, magic_bishop_table + 2240,
  magic_bishop_table + 4864, magic_bishop_table + 4960,
  magic_bishop_table + 5056, magic_bishop_table + 2720,
  magic_bishop_table + 864, magic_bishop_table + 1248,
  magic_bishop_table + 1632, magic_bishop_table + 2272,
  magic_bishop_table + 4896, magic_bishop_table + 5184
};
BITBOARD magic_rook_table[102400];
BITBOARD *magic_rook_indices[64] = {
  magic_rook_table + 86016, magic_rook_table + 73728,
  magic_rook_table + 36864, magic_rook_table + 43008,
  magic_rook_table + 47104, magic_rook_table + 51200,
  magic_rook_table + 77824, magic_rook_table + 94208,
  magic_rook_table + 69632, magic_rook_table + 32768,
  magic_rook_table + 38912, magic_rook_table + 10240,
  magic_rook_table + 14336, magic_rook_table + 53248,
  magic_rook_table + 57344, magic_rook_table + 81920,
  magic_rook_table + 24576, magic_rook_table + 33792,
  magic_rook_table + 6144, magic_rook_table + 11264,
  magic_rook_table + 15360, magic_rook_table + 18432,
  magic_rook_table + 58368, magic_rook_table + 61440,
  magic_rook_table + 26624, magic_rook_table + 4096,
  magic_rook_table + 7168, magic_rook_table + 0,
  magic_rook_table + 2048, magic_rook_table + 19456,
  magic_rook_table + 22528, magic_rook_table + 63488,
  magic_rook_table + 28672, magic_rook_table + 5120,
  magic_rook_table + 8192, magic_rook_table + 1024,
  magic_rook_table + 3072, magic_rook_table + 20480,
  magic_rook_table + 23552, magic_rook_table + 65536,
  magic_rook_table + 30720, magic_rook_table + 34816,
  magic_rook_table + 9216, magic_rook_table + 12288,
  magic_rook_table + 16384, magic_rook_table + 21504,
  magic_rook_table + 59392, magic_rook_table + 67584,
  magic_rook_table + 71680, magic_rook_table + 35840,
  magic_rook_table + 39936, magic_rook_table + 13312,
  magic_rook_table + 17408, magic_rook_table + 54272,
  magic_rook_table + 60416, magic_rook_table + 83968,
  magic_rook_table + 90112, magic_rook_table + 75776,
  magic_rook_table + 40960, magic_rook_table + 45056,
  magic_rook_table + 49152, magic_rook_table + 55296,
  magic_rook_table + 79872, magic_rook_table + 98304
};
BITBOARD magic_rook[64] = {
  0x0080001020400080ULL, 0x0040001000200040ULL, 0x0080081000200080ULL,
  0x0080040800100080ULL, 0x0080020400080080ULL, 0x0080010200040080ULL,
  0x0080008001000200ULL, 0x0080002040800100ULL, 0x0000800020400080ULL,
  0x0000400020005000ULL, 0x0000801000200080ULL, 0x0000800800100080ULL,
  0x0000800400080080ULL, 0x0000800200040080ULL, 0x0000800100020080ULL,
  0x0000800040800100ULL, 0x0000208000400080ULL, 0x0000404000201000ULL,
  0x0000808010002000ULL, 0x0000808008001000ULL, 0x0000808004000800ULL,
  0x0000808002000400ULL, 0x0000010100020004ULL, 0x0000020000408104ULL,
  0x0000208080004000ULL, 0x0000200040005000ULL, 0x0000100080200080ULL,
  0x0000080080100080ULL, 0x0000040080080080ULL, 0x0000020080040080ULL,
  0x0000010080800200ULL, 0x0000800080004100ULL, 0x0000204000800080ULL,
  0x0000200040401000ULL, 0x0000100080802000ULL, 0x0000080080801000ULL,
  0x0000040080800800ULL, 0x0000020080800400ULL, 0x0000020001010004ULL,
  0x0000800040800100ULL, 0x0000204000808000ULL, 0x0000200040008080ULL,
  0x0000100020008080ULL, 0x0000080010008080ULL, 0x0000040008008080ULL,
  0x0000020004008080ULL, 0x0000010002008080ULL, 0x0000004081020004ULL,
  0x0000204000800080ULL, 0x0000200040008080ULL, 0x0000100020008080ULL,
  0x0000080010008080ULL, 0x0000040008008080ULL, 0x0000020004008080ULL,
  0x0000800100020080ULL, 0x0000800041000080ULL, 0x00FFFCDDFCED714AULL,
  0x007FFCDDFCED714AULL, 0x003FFFCDFFD88096ULL, 0x0000040810002101ULL,
  0x0001000204080011ULL, 0x0001000204000801ULL, 0x0001000082000401ULL,
  0x0001FFFAABFAD1A2ULL
};
BITBOARD magic_rook_mask[64] = {
  0x000101010101017EULL, 0x000202020202027CULL, 0x000404040404047AULL,
  0x0008080808080876ULL, 0x001010101010106EULL, 0x002020202020205EULL,
  0x004040404040403EULL, 0x008080808080807EULL, 0x0001010101017E00ULL,
  0x0002020202027C00ULL, 0x0004040404047A00ULL, 0x0008080808087600ULL,
  0x0010101010106E00ULL, 0x0020202020205E00ULL, 0x0040404040403E00ULL,
  0x0080808080807E00ULL, 0x00010101017E0100ULL, 0x00020202027C0200ULL,
  0x00040404047A0400ULL, 0x0008080808760800ULL, 0x00101010106E1000ULL,
  0x00202020205E2000ULL, 0x00404040403E4000ULL, 0x00808080807E8000ULL,
  0x000101017E010100ULL, 0x000202027C020200ULL, 0x000404047A040400ULL,
  0x0008080876080800ULL, 0x001010106E101000ULL, 0x002020205E202000ULL,
  0x004040403E404000ULL, 0x008080807E808000ULL, 0x0001017E01010100ULL,
  0x0002027C02020200ULL, 0x0004047A04040400ULL, 0x0008087608080800ULL,
  0x0010106E10101000ULL, 0x0020205E20202000ULL, 0x0040403E40404000ULL,
  0x0080807E80808000ULL, 0x00017E0101010100ULL, 0x00027C0202020200ULL,
  0x00047A0404040400ULL, 0x0008760808080800ULL, 0x00106E1010101000ULL,
  0x00205E2020202000ULL, 0x00403E4040404000ULL, 0x00807E8080808000ULL,
  0x007E010101010100ULL, 0x007C020202020200ULL, 0x007A040404040400ULL,
  0x0076080808080800ULL, 0x006E101010101000ULL, 0x005E202020202000ULL,
  0x003E404040404000ULL, 0x007E808080808000ULL, 0x7E01010101010100ULL,
  0x7C02020202020200ULL, 0x7A04040404040400ULL, 0x7608080808080800ULL,
  0x6E10101010101000ULL, 0x5E20202020202000ULL, 0x3E40404040404000ULL,
  0x7E80808080808000ULL
};
unsigned int magic_rook_shift[64] = {
  52, 53, 53, 53, 53, 53, 53, 52,
  53, 54, 54, 54, 54, 54, 54, 53,
  53, 54, 54, 54, 54, 54, 54, 53,
  53, 54, 54, 54, 54, 54, 54, 53,
  53, 54, 54, 54, 54, 54, 54, 53,
  53, 54, 54, 54, 54, 54, 54, 53,
  53, 54, 54, 54, 54, 54, 54, 53,
  53, 54, 54, 53, 53, 53, 53, 53
};
BITBOARD mobility_mask_n[4] = {
  0xFF818181818181FFULL, 0x007E424242427E00ULL,
  0x00003C24243C0000ULL, 0x0000001818000000ULL
};
BITBOARD mobility_mask_b[4] = {
  0xFF818181818181FFULL, 0x007E424242427E00ULL,
  0x00003C24243C0000ULL, 0x0000001818000000ULL
};
BITBOARD mobility_mask_r[4] = {
  0x8181818181818181ULL, 0x4242424242424242ULL,
  0x2424242424242424ULL, 0x1818181818181818ULL
};
BITBOARD mobility_mask_q[4] = {
  0xFF818181818181FFULL, 0x007E424242427E00ULL,
  0x00003C24243C0000ULL, 0x0000001818000000ULL
};
/*
  values use to deal with white/black independently
 */
const int rankflip[2][8] = {
  {RANK8, RANK7, RANK6, RANK5, RANK4, RANK3, RANK2, RANK1},
  {RANK1, RANK2, RANK3, RANK4, RANK5, RANK6, RANK7, RANK8}
};
const int sqflip[2][64] = {
   A8, B8, C8, D8, E8, F8, G8, H8,
   A7, B7, C7, D7, E7, F7, G7, H7,
   A6, B6, C6, D6, E6, F6, G6, H6,
   A5, B5, C5, D5, E5, F5, G5, H5,
   A4, B4, C4, D4, E4, F4, G4, H4,   /* black */
   A3, B3, C3, D3, E3, F3, G3, H3,
   A2, B2, C2, D2, E2, F2, G2, H2,
   A1, B1, C1, D1, E1, F1, G1, H1,

   A1, B1, C1, D1, E1, F1, G1, H1,
   A2, B2, C2, D2, E2, F2, G2, H2,
   A3, B3, C3, D3, E3, F3, G3, H3,
   A4, B4, C4, D4, E4, F4, G4, H4,
   A5, B5, C5, D5, E5, F5, G5, H5,   /* white */
   A6, B6, C6, D6, E6, F6, G6, H6,
   A7, B7, C7, D7, E7, F7, G7, H7,
   A8, B8, C8, D8, E8, F8, G8, H8
};
const BITBOARD rank12[2] = { 0xffff000000000000ULL, 0x000000000000ffffULL };
const int sign[2] = { -1, 1 };
int direction[2] = { -8, 8 };
int dark_corner[2] = { FILEA, FILEH };
int light_corner[2] = { FILEH, FILEA };
int epsq[2] = { +8, -8 };
int rook_A[2] = { A8, A1 };
int rook_D[2] = { D8, D1 };
int rook_F[2] = { F8, F1 };
int rook_G[2] = { G8, G1 };
int rook_H[2] = { H8, H1 };
int pawnadv1[2] = { +8, -8 };
int pawnadv2[2] = { +16, -16 };
int capleft[2] = { +9, -7 };
int capright[2] = { +7, -9 };
const char empty_sqs[9] = { 0, '1', '2', '3', '4', '5', '6', '7', '8' };
int king_tropism_n[8] = { 0, 3, 3, 2, 1, 0, 0, 0 };
int king_tropism_b[8] = { 0, 2, 2, 1, 0, 0, 0, 0 };
int king_tropism_r[8] = { 0, 4, 3, 2, 1, 1, 1, 1 };
int king_tropism_q[8] = { 0, 6, 5, 4, 3, 2, 2, 2 };
int passed_pawn_candidate[2][2][8] = {
  0,  0, 36, 16,  8,  4,  4, 0,         /* [mg][black][8] */
  0,  4,  4,  8, 16, 36,  0, 0,         /* [mg][white][8] */
  0,  0, 48, 32, 16,  9,  9, 0,         /* [eg][black][8] */
  0,  9,  9, 16, 32, 48,  0, 0          /* [eg][white][8] */
};
int passed_pawn_value[2][2][8] = {
  0, 200, 120,  80,  20,   0,   0, 0,   /* [mg][black][8] */
  0,   0,   0,  20,  80, 120, 200, 0,   /* [mg][white][8] */
  0, 200, 120,  80,  20,   0,   0, 0,   /* [eg][black][8] */
  0,   0,   0,  20,  80, 120, 200, 0,   /* [eg][white][8] */
};
int connected_passed_pawn_value[2] = { 1, 3 };
int passed_pawn_hidden[2] = {0, 40};
int blockading_passed_pawn_value[2][2][8] = {
  0, 100, 60, 40, 10,  0,   0, 0,       /* [mg][black][8] */
  0,   0,  0, 10, 40, 60, 100, 0,       /* [mg][white][8] */
  0, 100, 60, 40, 10,  0,   0, 0,       /* [eg][black][8] */
  0,   0,  0, 10, 40, 60, 100, 0,       /* [eg][white][8] */
};
int doubled_pawn_value[2] = {5, 6};
int outside_passed[2] = {20, 60};
int pawn_defects[2][8] = {
  {0, 0, 0, 1, 2, 3, 0, 0},             /* [black][8] */
  {0, 0, 3, 2, 1, 0, 0, 0}              /* [white][8] */
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
  99, 90, 80, 70, 60, 50, 40, 30,
  90, 80, 70, 60, 50, 40, 30, 40,
  80, 70, 60, 50, 40, 30, 40, 50,
  70, 60, 50, 40, 30, 40, 50, 60,
  60, 50, 40, 30, 40, 50, 60, 70,
  50, 40, 30, 40, 50, 60, 70, 80,
  40, 30, 40, 50, 60, 70, 80, 90,
  30, 40, 50, 60, 70, 80, 90, 99
};
const char b_n_mate_light_squares[64] = {
  30, 40, 50, 60, 70, 80, 90, 99,
  40, 30, 40, 50, 60, 70, 80, 90,
  50, 40, 30, 40, 50, 60, 70, 80,
  60, 50, 40, 30, 40, 50, 60, 70,
  70, 60, 50, 40, 30, 40, 50, 60,
  80, 70, 60, 50, 40, 30, 40, 50,
  90, 80, 70, 60, 50, 40, 30, 40,
  99, 90, 80, 70, 60, 50, 40, 30
};
const int mate[64] = {
  200, 180, 160, 140, 140, 160, 180, 200,
  180, 160, 140, 120, 120, 140, 160, 180,
  160, 140, 120, 100, 100, 120, 140, 160,
  140, 120, 100, 100, 100, 100, 120, 140,
  140, 120, 100, 100, 100, 100, 120, 140,
  160, 140, 120, 100, 100, 120, 140, 160,
  180, 160, 140, 120, 120, 140, 160, 180,
  200, 180, 160, 140, 140, 160, 180, 200
};
int knight_outpost[2][64] = {
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 1, 4, 4, 4, 4, 1, 0,
    0, 2, 6, 8, 8, 6, 2, 0,
    0, 1, 4, 4, 4, 4, 1, 0,   /* [black][64] */
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,

    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 1, 4, 4, 4, 4, 1, 0,
    0, 2, 6, 8, 8, 6, 2, 0,   /* [white][64] */
    0, 1, 4, 4, 4, 4, 1, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0
};
int bishop_outpost[2][64] = {
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 1, 1, 1, 1, 0, 0,
    0, 1, 3, 3, 3, 3, 1, 0,
    0, 3, 5, 5, 5, 5, 3, 0,   /* [black][64] */
    0, 1, 2, 2, 2, 2, 1, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,

    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 1, 2, 2, 2, 2, 1, 0,
    0, 3, 5, 5, 5, 5, 3, 0,
    0, 1, 3, 3, 3, 3, 1, 0,   /* [white][64] */
    0, 0, 1, 1, 1, 1, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0
};
int pval[2][2][64] = {
      0,   0,   0,   0,   0,   0,   0,   0,
     70,  70,  70,  70,  70,  70,  70,  70,
     10,  10,  10,  30,  30,  10,  10,  10,
      6,   6,   6,  16,  16,   6,   6,   6,
      3,   3,   3,  13,  13,   3,   3,   3,   /* [mg][black][64] */
      1,   1,   1,  10,  10,   1,   1,   1,
      0,   0,   0, -12, -12,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,

      0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0, -12, -12,   0,   0,   0,
      1,   1,   1,  10,  10,   1,   1,   1,
      3,   3,   3,  13,  13,   3,   3,   3,
      6,   6,   6,  16,  16,   6,   6,   6,   /* [mg][white][64] */
     10,  10,  10,  30,  30,  10,  10,  10,
     70,  70,  70,  70,  70,  70,  70,  70,
      0,   0,   0,   0,   0,   0,   0,   0,

      0,   0,   0,   0,   0,   0,   0,   0,
     70,  70,  70,  70,  70,  70,  70,  70,
     10,  10,  10,  30,  30,  10,  10,  10,
      6,   6,   6,  16,  16,   6,   6,   6,
      3,   3,   3,  13,  13,   3,   3,   3,   /* [eg][black][64] */
      1,   1,   1,  10,  10,   1,   1,   1,
      0,   0,   0, -12, -12,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,

      0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0, -12, -12,   0,   0,   0,
      1,   1,   1,  10,  10,   1,   1,   1,
      3,   3,   3,  13,  13,   3,   3,   3,
      6,   6,   6,  16,  16,   6,   6,   6,   /* [eg][white][64] */
     10,  10,  10,  30,  30,  10,  10,  10,
     70,  70,  70,  70,  70,  70,  70,  70,
      0,   0,   0,   0,   0,   0,   0,   0
};
int nval[2][2][64] = {
    -30, -20, -20, -10, -10, -20, -20, -30,
      0,  10,  16,  20,  20,  16,  10,   0,
      0,  12,  20,  24,  24,  20,  12,   0,
      0,  12,  20,  24,  24,  20,  12,   0,
      0,  10,  18,  20,  20,  18,  10,   0,  /* [mg][black][64] */
      0,   0,  16,  14,  14,  16,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,
    -20, -20, -20, -20, -20, -20, -20, -20,

    -20, -20, -20, -20, -20, -20, -20, -20,
      0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,  16,  14,  14,  16,   0,   0,
      0,  10,  18,  20,  20,  18,  10,   0,
      0,  12,  20,  24,  24,  20,  12,   0,  /* [mg][white][64] */
      0,  12,  20,  24,  24,  20,  12,   0,
      0,  10,  16,  20,  20,  16,  10,   0,
    -30, -20, -20, -10, -10, -20, -20, -30,

    -30, -20, -20, -10, -10, -20, -20, -30,
      0,  10,  16,  20,  20,  16,  10,   0,
      0,  12,  20,  24,  24,  20,  12,   0,
      0,  12,  20,  24,  24,  20,  12,   0,
      0,  10,  18,  20,  20,  18,  10,   0,  /* [eg][black][64] */
      0,   0,  16,  14,  14,  16,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,
    -20, -20, -20, -20, -20, -20, -20, -20,

    -20, -20, -20, -20, -20, -20, -20, -20,
      0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,  16,  14,  14,  16,   0,   0,
      0,  10,  18,  20,  20,  18,  10,   0,
      0,  12,  20,  24,  24,  20,  12,   0,  /* [eg][white][64] */
      0,  12,  20,  24,  24,  20,  12,   0,
      0,  10,  16,  20,  20,  16,  10,   0,
    -30, -20, -20, -10, -10, -20, -20, -30
};
int bval[2][2][64] = {
      0,   0,   2,   4,   4,   2,   0,   0,
      0,   8,   6,   8,   8,   6,   8,   0,
      2,   6,  12,  10,  10,  12,   6,   2,
      4,   8,  10,  16,  16,  10,   8,   4,
      4,   8,  10,  16,  16,  10,   8,   4,   /* [mg][black][64] */
      2,   6,  12,  10,  10,  12,   6,   2,
      0,   8,   6,   8,   8,   6,   8,   0,
    -10, -10,  -8,  -6,  -6,  -8, -10, -10,

    -10, -10,  -8,  -6,  -6,  -8, -10, -10,
      0,   8,   6,   8,   8,   6,   8,   0,
      2,   6,  12,  10,  10,  12,   6,   2,
      4,   8,  10,  16,  16,  10,   8,   4,
      4,   8,  10,  16,  16,  10,   8,   4,   /* [mg][white][64] */
      2,   6,  12,  10,  10,  12,   6,   2,
      0,   8,   6,   8,   8,   6,   8,   0,
      0,   0,   2,   4,   4,   2,   0,   0,

      0,   0,   2,   4,   4,   2,   0,   0,
      0,   8,   6,   8,   8,   6,   8,   0,
      2,   6,  12,  10,  10,  12,   6,   2,
      4,   8,  10,  16,  16,  10,   8,   4,
      4,   8,  10,  16,  16,  10,   8,   4,   /* [eg][black][64] */
      2,   6,  12,  10,  10,  12,   6,   2,
      0,   8,   6,   8,   8,   6,   8,   0,
    -10, -10,  -8,  -6,  -6,  -8, -10, -10,

    -10, -10,  -8,  -6,  -6,  -8, -10, -10,
      0,   8,   6,   8,   8,   6,   8,   0,
      2,   6,  12,  10,  10,  12,   6,   2,
      4,   8,  10,  16,  16,  10,   8,   4,
      4,   8,  10,  16,  16,  10,   8,   4,   /* [eg][white][64] */
      2,   6,  12,  10,  10,  12,   6,   2,
      0,   8,   6,   8,   8,   6,   8,   0,
      0,   0,   2,   4,   4,   2,   0,   0
};
int qval[2][2][64] = {
      0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   4,   4,   4,   4,   0,   0,
      0,   4,   4,   6,   6,   4,   4,   0,
      0,   4,   6,   8,   8,   6,   4,   0,
      0,   4,   6,   8,   8,   6,   4,   0,   /* [mg][black][64] */
      0,   4,   4,   6,   6,   4,   4,   0,
      0,   0,   4,   4,   4,   4,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,

      0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   4,   4,   4,   4,   0,   0,
      0,   4,   4,   6,   6,   4,   4,   0,
      0,   4,   6,   8,   8,   6,   4,   0,
      0,   4,   6,   8,   8,   6,   4,   0,   /* [mg][white][64] */
      0,   4,   4,   6,   6,   4,   4,   0,
      0,   0,   4,   4,   4,   4,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,

      0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   4,   4,   4,   4,   0,   0,
      0,   4,   4,   6,   6,   4,   4,   0,
      0,   4,   6,   8,   8,   6,   4,   0,
      0,   4,   6,   8,   8,   6,   4,   0,   /* [eg][black][64] */
      0,   4,   4,   6,   6,   4,   4,   0,
      0,   0,   4,   4,   4,   4,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,

      0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   4,   4,   4,   4,   0,   0,
      0,   4,   4,   6,   6,   4,   4,   0,
      0,   4,   6,   8,   8,   6,   4,   0,
      0,   4,   6,   8,   8,   6,   4,   0,   /* [eg][white][64] */
      0,   4,   4,   6,   6,   4,   4,   0,
      0,   0,   4,   4,   4,   4,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0
};
int kval_n[2][64] = {
    -40, -40, -40, -40, -40, -40, -40, -40,
    -40, -10, -10, -10, -10, -10, -10, -40,
    -40, -10,  60,  60,  60,  60, -10, -40,
    -40, -10,  60,  60,  60,  60, -10, -40,
    -40, -10,  40,  40,  40,  40, -10, -40,   /* [black][64] */
    -40, -10,  20,  20,  20,  20, -10, -40,
    -40, -10, -10, -10, -10, -10, -10, -40,
    -40, -40, -40, -40, -40, -40, -40, -40,

    -40, -40, -40, -40, -40, -40, -40, -40,
    -40, -10, -10, -10, -10, -10, -10, -40,
    -40, -10,  20,  20,  20,  20, -10, -40,
    -40, -10,  40,  40,  40,  40, -10, -40,
    -40, -10,  60,  60,  60,  60, -10, -40,   /* [white][64] */
    -40, -10,  60,  60,  60,  60, -10, -40,
    -40, -10, -10, -10, -10, -10, -10, -40,
    -40, -40, -40, -40, -40, -40, -40, -40
};
int kval_k[2][64] = {
    -60, -40, -20, -20, -20, -20, -20, -20,
    -60, -40, -20,  20,  40,  40,  40,  40,
    -60, -40, -20,  20,  60,  60,  60,  40,
    -60, -40, -20,  20,  60,  60,  60,  40,
    -60, -40, -20,  20,  40,  40,  40,  40,   /* [black][64] */
    -60, -40, -20,  20,  20,  20,  20,  20,
    -60, -40, -20,   0,   0,   0,   0,   0,
    -60, -40, -20, -20, -20, -20, -20, -20,

    -60, -40, -20, -20, -20, -20, -20, -20,
    -60, -40, -20,   0,   0,   0,   0,   0,
    -60, -40, -20,  20,  20,  20,  20,  20,
    -60, -40, -20,  20,  40,  40,  40,  40,
    -60, -40, -20,  20,  60,  60,  60,  40,   /* [white][64] */
    -60, -40, -20,  20,  60,  60,  60,  40,
    -60, -40, -20,  20,  40,  40,  40,  40,
    -60, -40, -20, -20, -20, -20, -20, -20
};
int kval_q[2][64] = {
    -20, -20, -20, -20, -20, -20, -40, -60,
     40,  40,  40,  40,  20, -20, -40, -60,
     40,  60,  60,  60,  20, -20, -40, -60,
     40,  60,  60,  60,  20, -20, -40, -60,
     40,  40,  40,  40,  20, -20, -40, -60,   /* [black][64] */
     20,  20,  20,  20,  20, -20, -40, -60,
      0,   0,   0,   0,   0, -20, -40, -60,
    -20, -20, -20, -20, -20, -20, -40, -60,

    -20, -20, -20, -20, -20, -20, -40, -60,
      0,   0,   0,   0,   0, -20, -40, -60,
     20,  20,  20,  20,  20, -20, -40, -60,
     40,  40,  40,  40,  20, -20, -40, -60,
     40,  60,  60,  60,  20, -20, -40, -60,   /* [white][64] */
     40,  60,  60,  60,  20, -20, -40, -60,
     40,  40,  40,  40,  20, -20, -40, -60,
    -20, -20, -20, -20, -20, -20, -40, -60
};
int safety_vector[16] = {
  0, 7, 14, 21, 28, 35, 42, 49,
  56, 63, 70, 77, 84, 91, 98, 105
};
int tropism_vector[16] = {
  0, 1, 2, 3, 4, 5, 11, 20,
  32, 47, 65, 86, 110, 137, 167, 200
};
const int p_values[13] = { 10000, 900, 500, 300, 300, 100, 0,
  100, 300, 300, 500, 900, 9900
};
const int pc_values[7] = { 0, 100, 300, 300, 500, 900, 9900 };
const int p_vals[7] = { 0, 1, 3, 3, 5, 9, 99 };
const int pieces[2][7] = {
  {0, -1, -2, -3, -4, -5, -6},
  {0, +1, +2, +3, +4, +5, +6}
};
int pawn_value = PAWN_VALUE;
int knight_value = KNIGHT_VALUE;
int bishop_value = BISHOP_VALUE;
int rook_value = ROOK_VALUE;
int queen_value = QUEEN_VALUE;
int king_value = KING_VALUE;
int piece_values[2][7] = {
  {0, -PAWN_VALUE, -KNIGHT_VALUE, -BISHOP_VALUE,
   -ROOK_VALUE, -QUEEN_VALUE, -KING_VALUE},
  {0, PAWN_VALUE, KNIGHT_VALUE, BISHOP_VALUE,
   ROOK_VALUE, QUEEN_VALUE, KING_VALUE}
};
int pawn_can_promote = 525;
int bad_trade = 90;
int wtm_bonus[2] = {5, 8};
int lower_n = 12;
int lower_b = 16;
int lower_r = 16;
int mobility_score_b[2][4] = {{1, 2, 3, 4}, {2, 3, 4, 5}};
int mobility_score_n[4] = {1, 2, 3, 4};
int mobility_score_r[4] = {1, 2, 3, 4};
int mobility_score_q[4] = {1, 2, 3, 4};
int undeveloped_piece = 12;
int friendly_queen[8] = {2, 2, 2, 1, 0, 0, -1, -1};
int pawn_duo[2] = {4, 8};
int pawn_isolated[2] = {12, 18};
int pawn_weak[2] = {16, 24};
int pawn_islands[2][5] = { 0, 0, 0, 10, 20,
                           0, 0, 0, 20, 30 };
int split_passed[2] = {0, 50};
int king_king_tropism = 10;
int bishop_trapped = 174;
int bishop_with_wing_pawns[2] = {18, 36};
int rook_on_7th[2] = {20, 40};
/*
    each row contains values for a rook occupying a specific open
    file starting with file A and going through file H.  Row 0 is
    not used, row 1 is used when there is only one open file on
    the board, row 2 is used when there are two openfiles on the
    board, etc.  obviously control of a file is more important if
    there is only one or two files available.
*/
int rook_open_file[2][9][8] = {
    0,  0,  0,  0,  0,  0,  0,  0,
   12, 16, 24, 24, 24, 24, 16, 12,
   12, 16, 20, 20, 20, 20, 16, 12,
   12, 16, 20, 20, 20, 20, 16, 12,
   12, 16, 20, 20, 20, 20, 16, 12,
   12, 16, 20, 20, 20, 20, 16, 12,
   12, 16, 20, 20, 20, 20, 16, 12,
   12, 16, 20, 20, 20, 20, 16, 12,
   12, 16, 20, 20, 20, 20, 16, 12,

    0,  0,  0,  0,  0,  0,  0,  0,
   12, 16, 24, 24, 24, 24, 16, 12,
   12, 16, 20, 20, 20, 20, 16, 12,
   12, 16, 20, 20, 20, 20, 16, 12,
   12, 16, 20, 20, 20, 20, 16, 12,
   12, 16, 20, 20, 20, 20, 16, 12,
   12, 16, 20, 20, 20, 20, 16, 12,
   12, 16, 20, 20, 20, 20, 16, 12,
   12, 16, 20, 20, 20, 20, 16, 12
};
int rook_half_open_file[2] = {10, 10};
int rook_behind_passed_pawn[2] = {10, 36};
int rook_trapped = 60;
int open_file[8] = { 6, 5, 4, 4, 4, 4, 5, 6 };
int half_open_file[8] = { 4, 4, 3, 3, 3, 3, 4, 4 };
int king_safety_mate_threat = 600;
int development_thematic = 12;
int development_losing_castle = 20;
int development_not_castled = 20;
/*
   First term is a character string explaining what the eval
   term is used for.

  Second term is the "type" of the value.  
     0 = heading entry (no values, just a category name to display).
     1 = scalar value.
     2 = mg/eg two-element array
     3 = array[mg][side][64].  These values are displayed as 8x8
       boards with white values at the bottom, and the ranks/files
       labeled to make them easier to read.
     4 = array[side][64]
     5 = array[side][N!=64]
     6 = array[mg][side][8]
     7 = array[mg][small#]
     8 = array[n]

   Third term is the "size" of the scoring term, where 0 is a
     scalar value, otherwise it is the actual number of elements in
     an array of values.

   Fourth term is a pointer to the data value(s).
*/
struct personality_term personality_packet[256] = {
  {"search options                       ", 0, 0, NULL},        /* 0 */
  {"check extension                      ", 1, 0, &check_depth},
  {"null-move reduction                  ", 1, 0, &null_depth},
  {"razoring margin                      ", 1, 0, &razor_margin},
  {"futility margin                      ", 1, 0, &futility_margin},
  {"extended futility margin             ", 1, 0, &extended_futility_margin},
  {"LMR min distance to frontier         ", 1, 0, &LMR_min_depth},
  {"LMR reduction                        ", 1, 0, &LMR_depth},
  {NULL, 0, 0, NULL},
  {NULL, 0, 0, NULL},
  {"raw piece values                     ", 0, 0, NULL},        /* 10 */
  {"pawn value                           ", 1, 0, &pawn_value},
  {"knight value                         ", 1, 0, &knight_value},
  {"bishop value                         ", 1, 0, &bishop_value},
  {"rook value                           ", 1, 0, &rook_value},
  {"queen value                          ", 1, 0, &queen_value},
  {NULL, 0, 0, NULL},
  {NULL, 0, 0, NULL},
  {NULL, 0, 0, NULL},
  {NULL, 0, 0, NULL},
  {"miscellaneous scoring values         ", 0, 0, NULL},        /* 20 */
  {"bad trade bonus/penalty              ", 1, 0, &bad_trade},
  {"wtm bonus                            ", 2, 2, wtm_bonus},
  {"draw score                           ", 1, 0, &abs_draw_score},
#if defined(SKILL)
  {"skill level setting                  ", 1, 0, &skill},
#else
  {NULL, 0, 0, NULL},
#endif
  {NULL, 0, 0, NULL},
  {NULL, 0, 0, NULL},
  {NULL, 0, 0, NULL},
  {NULL, 0, 0, NULL},
  {NULL, 0, 0, NULL},
  {"pawn evaluation                      ", 0, 0, NULL},        /* 30 */
  {"pawn piece/square table (white)      ", 3, 256, (int *) pval},
  {"connected passed pawn [rank]         ", 2, 2, connected_passed_pawn_value},
  {"passed pawn [rank]                   ", 6, 32, (int *) passed_pawn_value},
  {"blockading a passed pawn [rank]      ", 6, 32, (int *) blockading_passed_pawn_value},
  {"pawn duo                             ", 2, 2, pawn_duo},
  {"pawn isolated                        ", 2, 2, pawn_isolated},
  {"pawn weak                            ", 2, 2, pawn_weak},
  {"pawn islands [0-4]                   ", 7, 10, (int *) pawn_islands},
  {"pawn can promote                     ", 1, 0, &pawn_can_promote},
  {"split passed pawn bonus              ", 2, 2, split_passed},
  {"outside passed pawn                  ", 2, 2, outside_passed},
  {"hidden passed pawn                   ", 2, 2, passed_pawn_hidden},
  {"doubled pawn                         ", 2, 2, doubled_pawn_value},
  {NULL, 0, 0, NULL},
  {NULL, 0, 0, NULL},
  {NULL, 0, 0, NULL},
  {NULL, 0, 0, NULL},
  {NULL, 0, 0, NULL},
  {NULL, 0, 0, NULL},
  {"knight scoring                       ", 0, 0, NULL},        /* 50 */
  {"knight piece/square table (white)    ", 3, 256, (int *) nval},
  {"knight outpost [square]              ", 4, 128, (int *) knight_outpost},
  {"knight king tropism [distance]       ", 8, 8, king_tropism_n},
  {"knight mobility                      ", 8, 4, mobility_score_n},
  {NULL, 0, 0, NULL},
  {NULL, 0, 0, NULL},
  {NULL, 0, 0, NULL},
  {NULL, 0, 0, NULL},
  {NULL, 0, 0, NULL},
  {"bishop scoring                       ", 0, 0, NULL},        /* 60 */
  {"bishop piece/square table (white)    ", 3, 256, (int *) bval},
  {"bishop king tropism [distance]       ", 8, 8, king_tropism_b},
  {"bishop mobility/square table         ", 8, 4, (int *) mobility_score_b},
  {"bishop mobility/square (pair)        ", 8, 4, (int *) mobility_score_b},
  {"bishop with wing pawns               ", 2, 2, bishop_with_wing_pawns},
  {"bishop trapped                       ", 1, 0, &bishop_trapped},
  {NULL, 0, 0, NULL},
  {NULL, 0, 0, NULL},
  {NULL, 0, 0, NULL},
  {"rook scoring                         ", 0, 0, NULL},        /* 70 */
  {"rook open file [2[[9][8]             ", 5, 144, (int *) rook_open_file},
  {"rook king tropism [distance]         ", 8, 8, king_tropism_r},
  {"rook mobility/square table           ", 8, 4, mobility_score_r},
  {"rook half open file                  ", 2, 2, rook_half_open_file},
  {"rook on 7th                          ", 2, 2, rook_on_7th},
  {"rook behind passed pawn              ", 2, 2, rook_behind_passed_pawn},
  {"rook trapped                         ", 1, 0, &rook_trapped},
  {NULL, 0, 0, NULL},
  {NULL, 0, 0, NULL},
  {"queen scoring                        ", 0, 0, NULL},        /* 80 */
  {"queen piece/square table (white)     ", 3, 256, (int *) qval},
  {"queen king tropism [distance]        ", 8, 8, king_tropism_q},
  {NULL, 0, 0, NULL},
  {NULL, 0, 0, NULL},
  {NULL, 0, 0, NULL},
  {NULL, 0, 0, NULL},
  {NULL, 0, 0, NULL},
  {NULL, 0, 0, NULL},
  {NULL, 0, 0, NULL},
  {"king scoring                         ", 0, 0, NULL},        /* 90 */
  {"king piece/square normal             ", 4, 128, (int *)kval_n},
  {"king piece/square kside pawns        ", 4, 128, (int *)kval_k},
  {"king piece/square qside pawns        ", 4, 128, (int *)kval_q},
  {"king safety pawn-shield vector       ", 8, 16, safety_vector},
  {"king safety tropism vector           ", 8, 16, tropism_vector},
  {"king safe open file [file]           ", 8, 8, open_file},
  {"king safe half-open file [file]      ", 8, 8, half_open_file},
  {"king king tropism (endgame)          ", 1, 0, &king_king_tropism},
  {"king safety trojan horse threat      ", 1, 0, &king_safety_mate_threat},
  {NULL, 0, 0, NULL},
  {NULL, 0, 0, NULL},
  {NULL, 0, 0, NULL},
  {NULL, 0, 0, NULL},
  {NULL, 0, 0, NULL},
  {NULL, 0, 0, NULL},
  {NULL, 0, 0, NULL},
  {NULL, 0, 0, NULL},
  {NULL, 0, 0, NULL},
  {NULL, 0, 0, NULL},
  {"development scoring                  ", 0, 0, NULL},        /* 110 */
  {"development thematic                 ", 1, 0, &development_thematic},
  {"development losing castle rights     ", 1, 0, &development_losing_castle},
  {"development not castled              ", 1, 0, &development_not_castled},
};
/* *INDENT-ON* */
