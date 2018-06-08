#if !defined(DATA_INCLUDED)
# define DATA_INCLUDED

  extern char           version[6];
  extern PLAYING_MODE   mode;
  extern int            batch_mode;
  extern int            swindle_mode;
  extern int            call_flag;
  extern int            crafty_rating;
  extern int            opponent_rating;
  extern int            number_auto_kibitzers;
  extern int            number_of_computers;
  extern int            number_of_GMs;
  extern int            number_of_IMs;
  extern int            number_to_play;
  extern int            number_of_blockers;
  extern int            number_of_special_openings;
  extern int            time_used;
  extern int            time_used_opponent;
  extern int            cpu_time_used;
  extern int            next_time_check;
  extern BITBOARD       total_moves;
  extern int            initialized;
  extern int            early_exit;
  extern int            new_game;
  extern char           auto_kibitz_list[64][20];
  extern char           GM_list[512][20];
  extern char           IM_list[512][20];
  extern char           computer_list[512][20];
  extern char           blocker_list[512][20];
  extern char           toplay_list[512][20];
  extern char           opening_list[16][20];
  extern char           opening_filenames[16][64];
  extern FILE           *input_stream;
  extern FILE           *book_file;
  extern FILE           *books_file;
  extern FILE           *normal_bs_file;
  extern FILE           *computer_bs_file;
  extern FILE           *history_file;
  extern FILE           *log_file;
  extern FILE           *auto_file;
  extern FILE           *book_lrn_file;
  extern FILE           *position_file;
  extern FILE           *position_lrn_file;
  extern int            log_id;
  extern int            output_format;
  extern int            EGTBlimit;
  extern int            EGTB_draw;
  extern int            EGTB_search;
  extern int            EGTB_use;
  extern void           *EGTB_cache;
  extern int            EGTB_cache_size;
  extern int            EGTB_setup;
  extern int            DGT_active;
  extern int            to_dgt;
  extern int            from_dgt;
  extern char           whisper_text[512];
  extern int            whisper_depth;
  extern int            done;
  extern int            last_mate_score;
  extern int            last_opponent_move;
  extern int            average_nps;

  extern int            incheck_depth;
  extern int            onerep_depth;
  extern int            pushpp_depth;
  extern int            recap_depth;
  extern int            mate_depth;

  extern int            null_min;
  extern int            null_max;

  extern int            pgn_suggested_percent;
  extern char           pgn_event[32];
  extern char           pgn_date[32];
  extern char           pgn_round[32];
  extern char           pgn_site[32];
  extern char           pgn_white[64];
  extern char           pgn_white_elo[32];
  extern char           pgn_black[64];
  extern char           pgn_black_elo[32];
  extern char           pgn_result[32];
  extern char           log_filename[64];
  extern char           history_filename[64];

  extern int            number_of_solutions;
  extern int            solutions[10];
  extern int            solution_type;
  extern int            draw_score[2];
  extern int            abs_draw_score;
  extern int            accept_draws;
  extern int            offer_draws;
  extern int            over;
  extern int            ics;
  extern int            auto232;
  extern int            auto232_delay;
  extern int            xboard;
  extern int            pong;
  extern int            whisper;
  extern int            channel;
  extern char           channel_title[32];
  extern char           book_path[128];
  extern char           log_path[128];
  extern char           tb_path[128];
  extern char           rc_path[128];
  extern char           cmd_buffer[512];
  extern char           *args[32];
  extern char           buffer[512];
  extern int            nargs;
  extern int            kibitz;
  extern int            move_number;
  extern int            wtm;
  extern int            crafty_is_white;
  extern int            iteration_depth;
  extern int            last_search_value;
  extern int            search_failed_high;
  extern int            search_failed_low;
  extern int            largest_positional_score;
  extern int            lazy_eval_cutoff;
  extern int            root_alpha;
  extern int            root_beta;
  extern int            last_root_value;
  extern int            ponder_value;
  extern int            root_value;
  extern int            root_wtm;
  extern int            root_print_ok;
  extern int            move_actually_played;
  extern ROOT_MOVE      root_moves[256];
  extern int            n_root_moves;
  extern int            nodes_per_second;
  extern int            cpu_percent;
  extern int            opening;
  extern int            middle_game;
  extern int            end_game;
  extern int            analyze_mode;
  extern int            annotate_mode;
  extern int            test_mode;
  extern int            input_status;  /* 0=no input; 
                                          1=predicted move read;
                                          2=unpredicted move read;
                                          3=something read, not executed. */
  extern signed char    resign;
  extern signed char    resign_counter;
  extern signed char    resign_count;
  extern signed char    draw_counter;
  extern signed char    draw_count;
  extern signed char    draw_offer_pending;
  extern char           audible_alarm;
  extern char           hint[512];
  extern char           book_hint[512];
  extern int            post;
  extern int            search_depth;
  extern unsigned int   search_nodes;
  extern int            search_move;
  extern int            easy_move;
  extern TIME_TYPE      time_type;
  extern int            time_limit;
  extern int            absolute_time_limit;
  extern int            search_time_limit;
  extern int            nodes_between_time_checks;
  extern int            burp;

  extern int            time_abort;
  extern volatile int   quit;
  extern signed char    pondering;   /* thinking on opponent's time     */
  extern signed char    thinking;    /* searching on its time           */
  extern signed char    puzzling;    /* puzzling about a move to ponder */
  extern signed char    booking;     /* searching, following book moves */
  extern signed char    abort_search;
  extern int            ponder;
  extern int            ponder_move;
  extern int            force;
  extern int            ponder_moves[220];
  extern int            num_ponder_moves;
  extern char           initial_position[80];

  extern unsigned int   opponent_start_time, opponent_end_time;
  extern unsigned int   program_start_time, program_end_time;
  extern unsigned int   start_time, end_time;
  extern unsigned int   elapsed_start, elapsed_end;
  extern int            predicted;
  extern signed char    transposition_id;

  extern int            ansi;
  extern int            trace_level;
  extern int            max_threads;
  extern int            min_thread_depth;
  extern int            max_thread_group;
  extern int            split_at_root;
  extern int            thread_depth;
  extern int            display_options;
  extern unsigned int   noise_level;

  extern int            book_move;
  extern int            moves_out_of_book;
  extern int            book_accept_mask;
  extern int            book_reject_mask;
  extern int            book_random;
  extern float          book_weight_freq;
  extern float          book_weight_eval;
  extern float          book_weight_learn;
  extern float          book_weight_CAP;
  extern int            book_search_trigger;
  extern int            book_selection_width;
  extern int            show_book;
  extern int            learning;
  extern int            book_learn_eval[LEARN_INTERVAL];
  extern int            book_learn_depth[LEARN_INTERVAL];

  extern int            tc_moves;
  extern int            tc_time;
  extern int            tc_time_remaining;
  extern int            tc_time_remaining_opponent;
  extern int            tc_moves_remaining;
  extern int            tc_secondary_moves;
  extern int            tc_secondary_time;
  extern int            tc_increment;
  extern int            tc_sudden_death;
  extern int            tc_operator_time;
  extern int            tc_safety_margin;
  extern int            trojan_check;
  extern int            computer_opponent;
  extern int            use_asymmetry;
  extern int            usage_level;
  extern int            log_hash;
  extern int            log_pawn_hash;
  extern int            hash_table_size;
  extern int            pawn_hash_table_size;
  extern int            hash_mask;
  extern unsigned int   pawn_hash_mask;
  extern HASH_ENTRY      *trans_ref;
  extern PAWN_HASH_ENTRY *pawn_hash_table;
  extern HASH_ENTRY      *trans_ref_orig;
  extern PAWN_HASH_ENTRY *pawn_hash_table_orig;

  extern int            history_w[4096], history_b[4096];

  extern const int      p_values[15];
  extern PATH           last_pv;
  extern int            last_value;

  extern const char     xlate[15];
  extern const char     empty[9];

  extern signed char    white_outpost[64];
  extern signed char    black_outpost[64];
  extern const char     square_color[64];
  extern const char     connected_passed_pawn_value[8];
  extern const int      hidden_passed_pawn_value[8];
  extern const int      passed_pawn_value[8];
  extern const char     blockading_passed_pawn_value[8];
  extern const char     isolated_pawn_value[9];
  extern const char     isolated_pawn_of_value[9];
  extern const char     doubled_pawn_value[7];
  extern const char     pawn_rams_v[9];
  extern char           pawn_rams[9];
  extern const char     supported_passer[8];
  extern const char     outside_passed[128];
  extern const char     majority[128];
  extern const int      temper[64];
  extern int            temper_b[64], temper_w[64];
  extern const char     ttemper[64];
  extern int            king_safety_asymmetry;
  extern int            king_safety_scale;
  extern int            king_safety_tropism;
  extern int            blocked_scale;
  extern int            pawn_scale;
  extern int            passed_scale;
  extern const char     missing[8];
  extern const char     openf[4];
  extern const char     hopenf[4];
  extern const char     king_tropism_n[8];
  extern const char     king_tropism_b[8];
  extern const char     king_tropism_r[8];
  extern const char     king_tropism_at_r[8];
  extern const char     king_tropism_q[8];
  extern const char     king_tropism_at_q[8];
  extern const int      king_tropism[128];
  extern int            tropism[128];

  extern signed char    pval_w[64];
  extern signed char    pval_b[64];
  extern signed char    nval_w[64];
  extern signed char    nval_b[64];
  extern signed char    bval_w[64];
  extern signed char    bval_b[64];
  extern signed char    rval_w[64];
  extern signed char    rval_b[64];
  extern signed char    qval_w[64];
  extern signed char    qval_b[64];
  extern signed char    kval_wn[64];
  extern signed char    kval_wk[64];
  extern signed char    kval_wq[64];
  extern signed char    kval_bn[64];
  extern signed char    kval_bk[64];
  extern signed char    kval_bq[64];
  extern signed char    king_defects_w[64];
  extern signed char    king_defects_b[64];

  extern signed char    bishop_pair[9];

  extern const char     b_n_mate_dark_squares[64];
  extern const char     b_n_mate_light_squares[64];
  extern const char     mate[64];

  extern const char     push_extensions[64];

  extern signed char    directions[64][64];
  extern BITBOARD       w_pawn_attacks[64];
  extern BITBOARD       b_pawn_attacks[64];
  extern BITBOARD       knight_attacks[64];
  extern BITBOARD       bishop_attacks[64];
#if defined(COMPACT_ATTACKS)
  /* Stuff these into a structure to make the addressing slightly cheaper */
  extern struct at at;

  extern BITBOARD       diag_attack_bitboards[NDIAG_ATTACKS];
  extern BITBOARD       anti_diag_attack_bitboards[NDIAG_ATTACKS];
  extern DIAG_INFO      diag_info[64];
  extern const unsigned char  bishop_shift_rl45[64];
  extern const unsigned char  bishop_shift_rr45[64];
#else
  extern BITBOARD       bishop_attacks_rl45[64][256];
  extern BITBOARD       bishop_attacks_rr45[64][256];
  extern BITBOARD       rook_attacks_r0[64][256];
  extern BITBOARD       rook_attacks_rl90[64][256];
  extern int            bishop_mobility_rl45[64][256];
  extern int            bishop_mobility_rr45[64][256];
  extern char           bishop_shift_rl45[64];
  extern char           bishop_shift_rr45[64];
  extern int            rook_mobility_r0[64][256];
  extern int            rook_mobility_rl90[64][256];
#endif
  extern BITBOARD       rook_attacks[64];

  extern POSITION display;

  extern BITBOARD       queen_attacks[64];
  extern BITBOARD       king_attacks[64];
  extern BITBOARD       king_attacks_1[64];
  extern BITBOARD       king_attacks_2[64];
  extern BITBOARD       obstructed[64][64];

  extern BITBOARD       w_pawn_random[64];
  extern BITBOARD       b_pawn_random[64];
  extern BITBOARD       w_knight_random[64];
  extern BITBOARD       b_knight_random[64];
  extern BITBOARD       w_bishop_random[64];
  extern BITBOARD       b_bishop_random[64];
  extern BITBOARD       w_rook_random[64];
  extern BITBOARD       b_rook_random[64];
  extern BITBOARD       w_queen_random[64];
  extern BITBOARD       b_queen_random[64];
  extern BITBOARD       w_king_random[64];
  extern BITBOARD       b_king_random[64];
  extern BITBOARD       stalemate_sqs[64];
  extern BITBOARD       edge_moves[64];
  extern BITBOARD       enpassant_random[65];
  extern BITBOARD       castle_random_w[2];
  extern BITBOARD       castle_random_b[2];
  extern BITBOARD       wtm_random[2];

  extern BITBOARD       clear_mask[65];
  extern BITBOARD       clear_mask_rl90[65];
  extern BITBOARD       clear_mask_rl45[65];
  extern BITBOARD       clear_mask_rr45[65];
  extern BITBOARD       set_mask[65];
  extern BITBOARD       set_mask_rl90[65];
  extern BITBOARD       set_mask_rl45[65];
  extern BITBOARD       set_mask_rr45[65];
  extern BITBOARD       file_mask[8];
  extern BITBOARD       rank_mask[8];
  extern BITBOARD       right_side_mask[8];
  extern BITBOARD       left_side_mask[8];
  extern BITBOARD       right_side_empty_mask[8];
  extern BITBOARD       left_side_empty_mask[8];
  extern BITBOARD       mask_efgh, mask_fgh, mask_abc, mask_abcd;
  extern BITBOARD       mask_abs7_w, mask_abs7_b;
  extern BITBOARD       mask_advance_2_w;
  extern BITBOARD       mask_advance_2_b;
  extern BITBOARD       mask_left_edge;
  extern BITBOARD       mask_right_edge;
  extern BITBOARD       mask_WBT;
  extern BITBOARD       mask_BBT;
  extern BITBOARD       mask_A3B3;
  extern BITBOARD       mask_B3C3;
  extern BITBOARD       mask_F3G3;
  extern BITBOARD       mask_G3H3;
  extern BITBOARD       mask_A6B6;
  extern BITBOARD       mask_B6C6;
  extern BITBOARD       mask_F6G6;
  extern BITBOARD       mask_G6H6;
  extern BITBOARD       mask_white_OO;
  extern BITBOARD       mask_white_OOO;
  extern BITBOARD       mask_black_OO;
  extern BITBOARD       mask_black_OOO;

  extern BITBOARD       stonewall_white;
  extern BITBOARD       stonewall_black;
  extern BITBOARD       e7_e6;
  extern BITBOARD       e2_e3;
  extern BITBOARD       closed_white;
  extern BITBOARD       closed_black;

  extern BITBOARD       mask_kr_trapped_w[3];
  extern BITBOARD       mask_qr_trapped_w[3];
  extern BITBOARD       mask_kr_trapped_b[3];
  extern BITBOARD       mask_qr_trapped_b[3];

  extern BITBOARD       good_bishop_kw;
  extern BITBOARD       good_bishop_qw;
  extern BITBOARD       good_bishop_kb;
  extern BITBOARD       good_bishop_qb;

  extern BITBOARD       light_squares;
  extern BITBOARD       dark_squares;
  extern BITBOARD       not_rook_pawns;

  extern BITBOARD       plus1dir[65];
  extern BITBOARD       plus7dir[65];
  extern BITBOARD       plus8dir[65];
  extern BITBOARD       plus9dir[65];
  extern BITBOARD       minus1dir[65];
  extern BITBOARD       minus7dir[65];
  extern BITBOARD       minus8dir[65];
  extern BITBOARD       minus9dir[65];

  extern BITBOARD       mask_eptest[64];
# if !defined(CRAY1) && !defined(ALPHA)
    extern BITBOARD       mask_1;
    extern BITBOARD       mask_2;
    extern BITBOARD       mask_3;
    extern BITBOARD       mask_8;
    extern BITBOARD       mask_16;
    extern BITBOARD       mask_112;
    extern BITBOARD       mask_120;
# endif
  extern BITBOARD       mask_clear_entry;

# if !defined(CRAY1) && !defined(USE_ASSEMBLY_B)
    extern unsigned char  first_one[65536];
    extern unsigned char  last_one[65536];
# endif
  extern unsigned char  first_one_8bit[256];
  extern unsigned char  last_one_8bit[256];
  extern unsigned char  pop_cnt_8bit[256];
  extern unsigned char  connected_passed[256];
  extern unsigned char  file_spread[256];
  extern signed char    is_outside[256][256];
  extern signed char    is_outside_c[256][256];

  extern BITBOARD       mask_pawn_protected_b[64];
  extern BITBOARD       mask_pawn_protected_w[64];
  extern BITBOARD       mask_pawn_duo[64];
  extern BITBOARD       mask_pawn_isolated[64];
  extern BITBOARD       mask_pawn_passed_w[64];
  extern BITBOARD       mask_pawn_passed_b[64];
  extern BITBOARD       mask_no_pawn_attacks_w[64];
  extern BITBOARD       mask_no_pawn_attacks_b[64];
  extern BITBOARD       white_minor_pieces;
  extern BITBOARD       black_minor_pieces;
  extern BITBOARD       white_pawn_race_wtm[64];
  extern BITBOARD       white_pawn_race_btm[64];
  extern BITBOARD       black_pawn_race_wtm[64];
  extern BITBOARD       black_pawn_race_btm[64];

  extern BITBOARD       mask_wk_4th, mask_wq_4th, mask_bk_4th, mask_bq_4th;
  extern BOOK_POSITION  book_buffer[BOOK_CLUSTER_SIZE];
  extern BOOK_POSITION  books_buffer[BOOK_CLUSTER_SIZE];

  extern unsigned int   thread_start_time[CPUS];
  extern unsigned int   pids[CPUS];
# if defined(SMP)
  extern TREE           *local[MAX_BLOCKS+1];
  extern TREE           *volatile thread[CPUS];
  extern lock_t         lock_smp, lock_io, lock_root;
  extern volatile int   smp_idle;
  extern volatile int   smp_threads;
# if !defined(CLONE)
  extern pthread_attr_t pthread_attr;
# endif
# else
  extern TREE           local_data[1], *local[1];
# endif
  extern unsigned int   parallel_splits;
  extern unsigned int   parallel_stops;
  extern unsigned int   max_split_blocks;
  extern volatile unsigned int   splitting;
  extern int cbEGTBCompBytes;
#endif
