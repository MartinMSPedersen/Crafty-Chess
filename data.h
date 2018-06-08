#if !defined(DATA_INCLUDED)
  #define DATA_INCLUDED
  #define MATE          65536
  #define REHASH_EXTRA   4096
  char           version[5];
  
  char           opponents_name[100];
  FILE           *input_stream;
  FILE           *book_file;
  FILE           *books_file;
  FILE           *history_file;
  FILE           *log_file;
  int            log_id;
  char           input[200];
  char           whisper_text[500];
  int            whisper_value;
  int            whisper_depth;
  int            last_mate_score;
  
  int            default_draw_score;
  int            over;
  int            ics;
  int            whisper;
  int            kibitz;
  int            move_number;
  int            wtm;
  int            iteration_depth;
  int            largest_positional_score;
  int            last_search_value;
  int            failed_high;
  int            failed_low;
  int            root_alpha;
  int            root_beta;
  int            root_value;
  int            root_wtm;
  int            nodes_per_second;

  int            null_depth;
  int            check_extensions;
  int            recapture_extensions;
  int            passed_pawn_extensions;
  int            quiescence_checks;
  int            mvv_lva_ordering;
  
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
  char           audible_alarm[2];
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
  int            abort_search;
  int            do_ponder;
  int            ponder_move;
  int            made_predicted_move;
  int            ponder_completed;
  int            force;
  
  unsigned int   opponent_start_time, opponent_end_time;
  unsigned int   program_start_time, program_end_time;
  int            start_time, end_time;
  int            nodes_searched;
  int            nodes_searched_null_move;
  int            nodes_searched_null_move_wasted;
  int            null_moves_tried;
  int            null_moves_wasted;
  int            evaluations;
  int            evaluations_hashed;
  int            max_search_depth;
  int            transposition_hashes;
  int            transposition_hashes_value;
  int            transposition_hashes_bound;
  int            transposition_hashes_cutoff;
  int            pawn_hashes;
  int            king_hashes;
  int            check_extensions_done;
  int            recapture_extensions_done;
  int            passed_pawn_extensions_done;
  
  int            ansi;
  int            trace_level;
  int            verbosity_level;
  int            burp;
  int            noise_level;
  
  int            last_move_in_book;
  int            book_accept_mask;
  int            book_reject_mask;
  int            book_random;
  int            book_lower_bound;
  float          book_min_percent_played;
  int            show_book;

  int            tc_moves;
  int            tc_time;
  int            tc_simple_average_time;
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
  
  int            log_hash_table_size;
  int            log_pawn_hash_table_size;
  int            log_king_hash_table_size;
  int            hash_table_size;
  int            pawn_hash_table_size;
  int            king_hash_table_size;
  
  BITBOARD       hash_mask;
  BITBOARD       pawn_hash_mask;
  BITBOARD       king_hash_mask;
  HASH_ENTRY     *trans_ref_w;
  HASH_ENTRY     *trans_ref_b;
  BITBOARD       *pawn_hash_table;
  BITBOARD       *pawn_hash_table_x;
  BITBOARD       *king_hash_table;
  
  int            hash_move[MAXPLY];
  int            positional_evaluation[MAXPLY];
  int            history_w[4096], history_b[4096];
  int            killer_move[MAXPLY][2];
  int            killer_move_count[MAXPLY][2];
  BITBOARD       repetition_list[100+MAXPLY];
  int            repetition_head;
  
  int            current_move[MAXPLY];
  int            *first[MAXPLY];
  int            *last[MAXPLY];
  int            in_check[MAXPLY];
  EXTENSIONS     extended_reason[MAXPLY];
  int            full[MAXPLY];
  int            current_phase[MAXPLY];
  int            move_list[10000];
  int            sort_value[300];
  int            root_sort_value[300];
  int            searched_this_root_move[300];
  
  CHESS_PATH     pv[MAXPLY];
#if !defined(FAST)
  CHESS_PATH_EXT pv_extensions[MAXPLY];
  int            show_extensions;
#endif
  
  NEXT_MOVE      next_status[MAXPLY];
  
  CHESS_POSITION position[MAXPLY+2];
  
  BITBOARD       open_files;

  int            white_outpost[64];
  int            black_outpost[64];
  int            connected_passer[8];
  int            supported_passer[8];
  int            pawn_advance[8];
  int            outside_passed[128];
  int            pawn_ram[9];
  int            isolated[9];
  int            piece_values[7];
  int            aggressor_order[7];
  int            pawn_value_w[64];
  int            pawn_value_b[64];
  int            knight_value_w[64];
  int            knight_value_b[64];
  int            bishop_value_w[64];
  int            bishop_value_b[64];
  int            queen_value_w[64];
  int            queen_value_b[64];
  int            king_value_w[64];
  int            king_value_b[64];

  int            b_n_mate_dark_squares[64];
  int            b_n_mate_light_squares[64];
  int            mate[64];

  int            directions[64][64];
  
  BITBOARD       w_pawn_attacks[64];
  BITBOARD       b_pawn_attacks[64];
  BITBOARD       knight_attacks[64];
  BITBOARD       bishop_attacks[64];
  BITBOARD       bishop_attacks_rl45[64][256];
  BITBOARD       bishop_attacks_rr45[64][256];
  int            bishop_mobility_rl45[64][256];
  int            bishop_mobility_rr45[64][256];
  int            bishop_shift_rl45[64];
  int            bishop_shift_rr45[64];
  BITBOARD       rook_attacks[64];
  BITBOARD       rook_attacks_r0[64][256];
  BITBOARD       rook_attacks_rl90[64][256];
  int            rook_mobility_r0[64][256];
  int            rook_mobility_rl90[64][256];
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
  BITBOARD       castle_random_w[4];
  BITBOARD       castle_random_b[4];
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
  BITBOARD       right_side_mask[8];
  BITBOARD       left_side_mask[8];
  BITBOARD       right_side_empty_mask[8];
  BITBOARD       left_side_empty_mask[8];
  BITBOARD       right_half_mask, left_half_mask;
  BITBOARD       pawns_cramp_black;
  BITBOARD       pawns_cramp_white;
  BITBOARD       mask_white_space;
  BITBOARD       mask_white_pawns_space;
  BITBOARD       mask_black_space;
  BITBOARD       mask_black_pawns_space;
  BITBOARD       mask_advance_2_w;
  BITBOARD       mask_advance_2_b;
  BITBOARD       mask_left_edge;
  BITBOARD       mask_right_edge;
  BITBOARD       mask_corner_squares;
  int            push_extensions[64];

  BITBOARD       light_squares;
  BITBOARD       dark_squares;
  
  BITBOARD       mask_plus1dir[65];
  BITBOARD       mask_plus7dir[65];
  BITBOARD       mask_plus8dir[65];
  BITBOARD       mask_plus9dir[65];
  BITBOARD       mask_minus1dir[65];
  BITBOARD       mask_minus7dir[65];
  BITBOARD       mask_minus8dir[65];
  BITBOARD       mask_minus9dir[65];
  
  BITBOARD       mask_enpassant_test[64];
  #if !defined(HAS_64BITS)
    BITBOARD       mask_1;
    BITBOARD       mask_2;
    BITBOARD       mask_3;
    BITBOARD       mask_4;
    BITBOARD       mask_8;
    BITBOARD       mask_32;
    BITBOARD       mask_72;
    BITBOARD       mask_80;
    BITBOARD       mask_96;
    BITBOARD       mask_107;
    BITBOARD       mask_108;
    BITBOARD       mask_112;
    BITBOARD       mask_118;
    BITBOARD       mask_120;
    BITBOARD       mask_121;
    BITBOARD       mask_127;
  #endif
  BITBOARD       mask_clear_entry;

  #if !defined(HAS_64BITS)
    int  first_ones[65536];
    int  last_ones[65536];
    int  population_count[65536];
  #endif
  int  first_ones_8bit[256];
  int  last_ones_8bit[256];

  BITBOARD       mask_kingside_attack_w1;
  BITBOARD       mask_kingside_attack_w2;
  BITBOARD       mask_kingside_attack_b1;
  BITBOARD       mask_kingside_attack_b2;
  BITBOARD       mask_queenside_attack_w1;
  BITBOARD       mask_queenside_attack_w2;
  BITBOARD       mask_queenside_attack_b1;
  BITBOARD       mask_queenside_attack_b2;

  BITBOARD       mask_pawn_isolated[64];
  BITBOARD       mask_pawn_artificially_isolated_w[64];
  BITBOARD       mask_pawn_artificially_isolated_b[64];
  BITBOARD       mask_pawn_passed_w[64];
  BITBOARD       mask_pawn_passed_b[64];
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
#endif
