#if !defined(DATA_INCLUDED)

#  define DATA_INCLUDED

   char           version[6];

   PLAYING_MODE   mode;
   int            crafty_rating;
   int            opponent_rating;
   int            computer_opponent;
   int            number_auto_kibitzers;
   int            number_of_computers;
   int            number_of_GMs;
   int            number_of_IMs;
   int            time_used;
   int            time_used_opponent;
   int            auto_kibitzing;
   int            total_moves;
   char           auto_kibitz_list[100][20];
   char           GM_list[100][20];
   char           IM_list[100][20];
   char           computer_list[100][20];
   char           opponents_name[100];
   FILE           *input_stream;
   FILE           *book_file;
   FILE           *books_file;
   FILE           *history_file;
   FILE           *log_file;
   FILE           *auto_file;
   FILE           *learn_file;
   int            log_id;
   char           input[200];
   char           whisper_text[500];
   int            whisper_value;
   int            whisper_depth;
   int            last_mate_score;
   int            last_opponent_move;

   char           pgn_event[32];
   char           pgn_site[32];
   char           pgn_date[32];
   char           pgn_round[32];
   char           pgn_site[32];
   char           pgn_white[32];
   char           pgn_white_elo[32];
   char           pgn_black[32];
   char           pgn_black_elo[32];
   char           pgn_result[32];

   int            number_of_solutions;
   int            solutions[10];
   int            solution_type;
   int            default_draw_score;
   int            over;
   int            ics;
   int            autoplay;
   int            xboard;
   int            whisper;
   int            kibitz;
   int            move_number;
   int            wtm;
   int            iteration_depth;
   int            largest_positional_score;
   int            last_search_value;
   int            search_failed_high;
   int            search_failed_low;
   int            root_alpha;
   int            root_beta;
   int            root_value;
   int            root_wtm;
   int            root_white_pieces,root_white_pawns;
   int            root_black_pieces,root_black_pawns;
   int            nodes_per_second;
   int            cpu_percent;

   int            tb_probes;
   int            tb_probes_successful;

   int            opening;
   int            middle_game;
   int            end_game;
   int            analyze_mode;
   int            analyze_move_read;
   int            resign;
   int            resign_counter;
   int            resign_count;
   int            draw;
   int            draw_counter;
   int            draw_count;
   char           audible_alarm;
   char           hint[16];
   int            post;
   int            search_depth;
   int            search_move;
   int            easy_move;
   int            time_type;
   int            time_limit;
   int            absolute_time_limit;
   int            search_time_limit;
   int            nodes_between_time_checks;
   int            next_time_check;

   int            time_abort;
   int            pondering;   /* program is thinking on opponent's time */
   int            thinking;    /* program is searching on its time       */
   int            puzzling;    /* program is puzzling about a move to ponder */
   int            booking;     /* program is searching, following book moves */
   int            abort_search;
   int            do_ponder;
   int            ponder_move;
   int            made_predicted_move;
   int            ponder_completed;
   int            force;

   int            ponder_moves[220];
   int            num_ponder_moves;

   unsigned int   opponent_start_time, opponent_end_time;
   unsigned int   program_start_time, program_end_time;
   unsigned int   start_time, end_time;
   unsigned int   elapsed_start, elapsed_end;
   unsigned int   nodes_searched;
   unsigned int   q_nodes_searched;
   unsigned int   evaluations;
   int            transposition_id;
   int            transposition_hashes;
   int            pawn_hashes;
   int            check_extensions_done;
   int            recapture_extensions_done;
   int            passed_pawn_extensions_done;
   int            one_reply_extensions_done;

   int            ansi;
   int            trace_level;
   int            verbosity_level;
   int            burp;
   int            noise_level;

   int            book_move;
   int            last_move_in_book;
   int            book_accept_mask;
   int            book_reject_mask;
   int            book_random;
   int            book_selection_width;
   int            show_book;
   int            book_learning;
   int            book_learn_eval[LEARN_INTERVAL];

   float          increment_factor;
   int            time_divisor;
   int            move_limit;
   int            tc_moves;
   int            tc_time;
   int            tc_time_remaining;
   int            tc_time_remaining_opponent;
   int            tc_moves_remaining;
   int            tc_secondary_moves;
   int            tc_secondary_time;
   int            tc_increment;
   int            tc_sudden_death;
   int            tc_clock_start;
   int            tc_operator_time;
   int            tc_safety_margin;
   float          zero_inc_factor;
   float          inc_time_multiplier;
   int            draw_score_is_zero;
   int            out_of_book;
   float          usage_level;
   float          usage_time;
   float          u_time;
   float          u_otime;

   int            log_hash_table_size;
   int            log_pawn_hash_table_size;
   int            hash_table_size;
   int            pawn_hash_table_size;

   int            hash_maska;
   int            hash_maskb;
   unsigned int   pawn_hash_mask;
   HASH_ENTRY     *trans_ref_wa;
   HASH_ENTRY     *trans_ref_wb;
   HASH_ENTRY     *trans_ref_ba;
   HASH_ENTRY     *trans_ref_bb;
   HASH_ENTRY     *pawn_hash_table;

   int            hash_move[MAXPLY];
   int            history_w[4096], history_b[4096];
   int            killer_move[MAXPLY][2];
   int            killer_move_count[MAXPLY][2];
   BITBOARD       repetition_list_w[50+MAXPLY/2];
   BITBOARD       repetition_list_b[50+MAXPLY/2];
   BITBOARD       *repetition_head_w;
   BITBOARD       *repetition_head_b;

   int            current_move[MAXPLY];
   int            *last[MAXPLY];
   int            in_check[MAXPLY];
   EXTENSIONS     extended_reason[MAXPLY];
   int            current_phase[MAXPLY];
   int            move_list[10000];
   int            sort_value[220];
   int            root_sort_value[220];
   int            searched_this_root_move[220];
   CHESS_PATH     pv[MAXPLY];
   CHESS_PATH     last_pv;
   int            last_value;
   NEXT_MOVE      next_status[MAXPLY];
   SEARCH_POSITION position[MAXPLY+2];
   BITBOARD       save_hash_key[MAXPLY+2];
   BITBOARD       save_pawn_hash_key[MAXPLY+2];

   char           white_outpost[64];
   char           black_outpost[64];
   char           square_color[64];
   int            connected_passer[8];
   int            supported_passer[8];
   int            pawn_advance[8];
   int            outside_passed[128];
   char           king_safety_c[128];
   char           king_safety_o[128];
   int            pawn_rams[9];
   int            cramping_pawn_rams[9];
   int            bad_pawn_rams[9];
   int            pawns_isolated[9];
   int            piece_values[8];
   int            pawn_value_w[64];
   int            pawn_value_b[64];
   int            knight_value_w[64];
   int            knight_value_b[64];
   int            bishop_value_w[64];
   int            bishop_value_b[64];
   int            rook_value_w[64];
   int            rook_value_b[64];
   int            queen_value_w[64];
   int            queen_value_b[64];
   int            king_value_w[64];
   int            king_value_b[64];
   char           king_defects_w[64];
   char           king_defects_b[64];

   int            b_n_mate_dark_squares[64];
   int            b_n_mate_light_squares[64];
   int            mate[64];

   char           is_corner[64];
   int            flight_sq[64][2];

   signed char    directions[64][64];
   BITBOARD       w_pawn_attacks[64];
   BITBOARD       b_pawn_attacks[64];
   BITBOARD       knight_attacks[64];
   BITBOARD       bishop_attacks[64];
#if defined(COMPACT_ATTACKS)
  /* Stuff these into a structure to make the addressing slightly cheaper */
  struct at {
    unsigned char which_attack[8][64];
    BITBOARD      file_attack_bitboards[8][MAX_ATTACKS_FROM_SQUARE];
    unsigned char rank_attack_bitboards[8][MAX_ATTACKS_FROM_SQUARE];
    unsigned char length8_mobility[8][MAX_ATTACKS_FROM_SQUARE];
    unsigned char short_mobility[NSHORT_MOBILITY];
  } at;

  BITBOARD       diag_attack_bitboards[NDIAG_ATTACKS];
  BITBOARD       anti_diag_attack_bitboards[NDIAG_ATTACKS];
  DIAG_INFO      diag_info[64];
  unsigned char  bishop_shift_rl45[64];
  unsigned char  bishop_shift_rr45[64];
#else
  BITBOARD       bishop_attacks_rl45[64][256];
  BITBOARD       bishop_attacks_rr45[64][256];
  int            bishop_mobility_rl45[64][256];
  int            bishop_mobility_rr45[64][256];
  int            bishop_shift_rl45[64];
  int            bishop_shift_rr45[64];
#endif
  BITBOARD       rook_attacks[64];
#if !defined(COMPACT_ATTACKS)
  BITBOARD       rook_attacks_r0[64][256];
  BITBOARD       rook_attacks_rl90[64][256];
  int            rook_mobility_r0[64][256];
  int            rook_mobility_rl90[64][256];
#endif

   CHESS_POSITION search;
   CHESS_POSITION display;

   BITBOARD       queen_attacks[64];
   BITBOARD       king_attacks[64];
   BITBOARD       king_attacks_1[64];
   BITBOARD       king_attacks_2[64];
   BITBOARD       obstructed[64][64];

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
   BITBOARD       endgame_random_w;
   BITBOARD       endgame_random_b;
   BITBOARD       w_rooks_random;
   BITBOARD       b_rooks_random;

   BITBOARD       clear_mask[65];
   BITBOARD       clear_mask_rl45[65];
   BITBOARD       clear_mask_rr45[65];
   BITBOARD       clear_mask_rl90[65];
   BITBOARD       set_mask[65];
   BITBOARD       set_mask_rl45[65];
   BITBOARD       set_mask_rr45[65];
   BITBOARD       set_mask_rl90[65];
   BITBOARD       file_mask[8];
   BITBOARD       rank_mask[8];
   BITBOARD       mask_not_rank8;
   BITBOARD       mask_not_rank1;
   BITBOARD       right_side_mask[8];
   BITBOARD       left_side_mask[8];
   BITBOARD       right_side_empty_mask[8];
   BITBOARD       left_side_empty_mask[8];
   BITBOARD       right_half_mask, left_half_mask;
   BITBOARD       mask_black_half, mask_white_half;
   BITBOARD       mask_abs7_w, mask_abs7_b;
   BITBOARD       pawns_cramp_black;
   BITBOARD       pawns_cramp_white;
   BITBOARD       mask_advance_2_w;
   BITBOARD       mask_advance_2_b;
   BITBOARD       mask_left_edge;
   BITBOARD       mask_right_edge;
   BITBOARD       mask_corner_squares;
   BITBOARD       promote_mask_w;
   BITBOARD       promote_mask_b;
   BITBOARD       mask_G2G3;
   BITBOARD       mask_B2B3;
   BITBOARD       mask_G6G7;
   BITBOARD       mask_B6B7;
   BITBOARD       mask_F3H3;
   BITBOARD       mask_F6H6;
   BITBOARD       mask_A3C3;
   BITBOARD       mask_A6C6;

   BITBOARD       mask_kr_trapped_w[3];
   BITBOARD       mask_qr_trapped_w[3];
   BITBOARD       mask_kr_trapped_b[3];
   BITBOARD       mask_qr_trapped_b[3];

   BITBOARD       good_bishop_kw;
   BITBOARD       good_bishop_qw;
   BITBOARD       good_bishop_kb;
   BITBOARD       good_bishop_qb;
   char           push_extensions[64];

   BITBOARD       light_squares;
   BITBOARD       dark_squares;
   BITBOARD       not_rook_pawns;

   BITBOARD       mask_plus1dir[65];
   BITBOARD       mask_plus7dir[65];
   BITBOARD       mask_plus8dir[65];
   BITBOARD       mask_plus9dir[65];
   BITBOARD       mask_minus1dir[65];
   BITBOARD       mask_minus7dir[65];
   BITBOARD       mask_minus8dir[65];
   BITBOARD       mask_minus9dir[65];

   BITBOARD       mask_enpassant_test[64];
#  if !defined(CRAY1)
     BITBOARD       mask_1;
     BITBOARD       mask_2;
     BITBOARD       mask_3;
     BITBOARD       mask_4;
     BITBOARD       mask_8;
     BITBOARD       mask_16;
     BITBOARD       mask_32;
     BITBOARD       mask_72;
     BITBOARD       mask_80;
     BITBOARD       mask_85;
     BITBOARD       mask_96;
     BITBOARD       mask_107;
     BITBOARD       mask_108;
     BITBOARD       mask_112;
     BITBOARD       mask_118;
     BITBOARD       mask_120;
     BITBOARD       mask_121;
     BITBOARD       mask_127;
#  endif
   BITBOARD       mask_clear_entry;

#  if !defined(CRAY1)
     unsigned char  first_ones[65536];
     unsigned char  last_ones[65536];
#  endif
   unsigned char  first_ones_8bit[256];
   unsigned char  last_ones_8bit[256];
   unsigned char  connected_passed[256];

   BITBOARD       mask_kingside_attack_w1;
   BITBOARD       mask_kingside_attack_w2;
   BITBOARD       mask_kingside_attack_b1;
   BITBOARD       mask_kingside_attack_b2;
   BITBOARD       mask_queenside_attack_w1;
   BITBOARD       mask_queenside_attack_w2;
   BITBOARD       mask_queenside_attack_b1;
   BITBOARD       mask_queenside_attack_b2;

   BITBOARD       mask_pawn_isolated[64];
   BITBOARD       mask_pawn_passed_w[64];
   BITBOARD       mask_pawn_passed_b[64];
   BITBOARD       mask_promotion_threat_w[64];
   BITBOARD       mask_promotion_threat_b[64];
   BITBOARD       mask_pawn_backward_w[64];
   BITBOARD       mask_pawn_backward_b[64];
   BITBOARD       mask_pawn_connected[64];
   BITBOARD       mask_no_pawn_attacks_w[64];
   BITBOARD       mask_no_pawn_attacks_b[64];
   BITBOARD       mask_a1_corner;
   BITBOARD       mask_a8_corner;
   BITBOARD       mask_h1_corner;
   BITBOARD       mask_h8_corner;
   BITBOARD       white_minor_pieces;
   BITBOARD       black_minor_pieces;
   BITBOARD       white_center_pawns;
   BITBOARD       black_center_pawns;
   BITBOARD       white_pawn_race_wtm[64];
   BITBOARD       white_pawn_race_btm[64];
   BITBOARD       black_pawn_race_wtm[64];
   BITBOARD       black_pawn_race_btm[64];

   BITBOARD       mask_wk_3rd, mask_wk_4th, mask_wq_3rd, mask_wq_4th;
   BITBOARD       mask_bk_3rd, mask_bk_4th, mask_bq_3rd, mask_bq_4th;
   BOOK_POSITION  buffer[BOOK_CLUSTER_SIZE];
#endif
