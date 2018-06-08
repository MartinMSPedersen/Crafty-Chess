/*
********************************************************************************
*                                                                              *
*   configuration information:  the following variables need to be set to      *
*   indicate the machine configuration/capabilities.                           *
*                                                                              *
*   there are pre-defined machine types for the following machines: (1) SUN    *
*   (2) PC  (3) ALPHA [DEC Alpha] (4) CRAY  (5) LINUX.  defining any of these  *
*   names will produce a runnable executable.  for other machines, the names   *
*   explained below must be individually DEFINED or UNDEFINED as needed.       *
*                                                                              *
*   HAS_64BITS:  define this for a machine that has true 64-bit hardware       *
*   including leading-zero hardware, population count, etc.  ie, a Cray-like   *
*   machine.                                                                   *
*                                                                              *
*   HAS_LONGLONG:  define this for a 32-bit machine with a compiler that       *
*   supports the long long (64-bit) integer data and allows bitwise operations *
*   on this data type.  this provides significantly faster execution time as   *
*   the bitwise operators are done by the compiler rather than by procedure    *
*   calls.                                                                     *
*                                                                              *
*   LITTLE_ENDIAN_ARCH:  define for a 32-bit machine that mangles the way data *
*   is stored within a word.  This is currently true for all PC class machines *
*   and false for other processors used in current workstations (SUN, etc.)    *
*                                                                              *
*   UNIX:  define this if the program is being run on a unix-based system,     *
*   which causes the executable to use unix-specific runtime utilities.        *
*                                                                              *
********************************************************************************
*/
#if !defined(TYPES_INCLUDED)
#  define TYPES_INCLUDED
#if defined(ALPHA)
#  undef  HAS_64BITS           /* machine has 64-bit integers / operators     */
#  define HAS_LONGLONG         /* machine has 32-bit/64-bit integers          */
#  define LITTLE_ENDIAN_ARCH   /* machine stores bytes in "PC" order          */
#  define UNIX                 /* system is unix-based                        */
#endif
#if defined(CRAY1)
#  define HAS_64BITS           /* machine has 64-bit integers / operators     */
#  undef  HAS_LONGLONG         /* machine has 32-bit/64-bit integers          */
#  undef  LITTLE_ENDIAN_ARCH   /* machine stores bytes in "PC" order          */
#  define UNIX                 /* system is unix-based                        */
#endif
#if defined(HP)
#  undef  HAS_64BITS           /* machine has 64-bit integers / operators     */
#  define HAS_LONGLONG         /* machine has 32-bit/64-bit integers          */
#  undef  LITTLE_ENDIAN_ARCH   /* machine stores bytes in "PC" order          */
#  define UNIX                 /* system is unix-based                        */
#endif
#if defined(LINUX)
#  undef  HAS_64BITS           /* machine has 64-bit integers / operators     */
#  define HAS_LONGLONG         /* machine has 32-bit/64-bit integers          */
#  define LITTLE_ENDIAN_ARCH   /* machine stores bytes in "PC" order          */
#  define UNIX                 /* system is unix-based                        */
#endif
#if defined(MIPS)
#  undef  HAS_64BITS           /* machine has 64-bit integers / operators     */
#  define HAS_LONGLONG         /* machine has 32-bit/64-bit integers          */
#  define LITTLE_ENDIAN_ARCH   /* machine stores bytes in "PC" order          */
#  define UNIX                 /* system is unix-based                        */
#endif
#if defined(PC)
#  undef  HAS_64BITS           /* machine has 64-bit integers / operators     */
#  define HAS_LONGLONG         /* machine has 32-bit/64-bit integers          */
#  define LITTLE_ENDIAN_ARCH   /* machine stores bytes in "PC" order          */
#  undef  UNIX                 /* system is unix-based                        */
#endif
#if defined(SUN)
#  undef  HAS_64BITS           /* machine has 64-bit integers / operators     */
#  define HAS_LONGLONG         /* machine has 32-bit/64-bit integers          */
#  undef  LITTLE_ENDIAN_ARCH   /* machine stores bytes in "PC" order          */
#  define UNIX                 /* system is unix-based                        */
#endif
#if defined(SUN_BSD)
#  undef  HAS_64BITS           /* machine has 64-bit integers / operators     */
#  define HAS_LONGLONG         /* machine has 32-bit/64-bit integers          */
#  undef  LITTLE_ENDIAN_ARCH   /* machine stores bytes in "PC" order          */
#  define UNIX                 /* system is unix-based                        */
#endif

#  define MAXPLY 65
  
#  if defined(HAS_64BITS)
    typedef unsigned long BITBOARD;
#  else
    typedef unsigned long long BITBOARD;
#  endif
    typedef signed char  SMALL;
    typedef signed short MEDIUM;

#include <time.h>
#if !defined(CLOCKS_PER_SEC)
#  define CLOCKS_PER_SEC 1000000
#endif
  
  typedef enum {none, null_move, hash_normal_move, hash_capture_move,
                capture_moves, killer_moves, history_moves, hash_checking_move, 
                checking_moves, remaining_moves, root_moves, all_done} PHASES;

  typedef enum {empty=0, pawn=1, knight=2, king=3, 
                bishop=5, rook=6, queen=7} PIECE;
  
  typedef enum {empty_v=0, pawn_v=1, knight_v=3, 
                bishop_v=3, rook_v=5, queen_v=9} PIECE_V;
  
  typedef enum {nothing, captures_generated, everything} WHATS_GENERATED;
  
  typedef enum {no_extension=0, check_extension=1, recapture_extension=2,
                passed_pawn_extension=4, threat_extension=8} EXTENSIONS;
  
  typedef enum {worthless=0, good_score=1, failed_low=2, 
                failed_high=3} HASH_ENTRY_TYPES;
  
  typedef enum {cpu, elapsed} TIME_TYPE;
 
  typedef enum {PV=1, CUT, ALL} NODES;

  typedef enum {think=1, puzzle=2, book=3} SEARCH_TYPE;
  
  typedef struct {
    BITBOARD w_pawn;
    BITBOARD w_knight;
    BITBOARD w_bishop;
    BITBOARD w_rook;
    BITBOARD w_queen;
    BITBOARD w_king;
    BITBOARD w_occupied;
  
    BITBOARD b_pawn;
    BITBOARD b_knight;
    BITBOARD b_bishop;
    BITBOARD b_rook;
    BITBOARD b_queen;
    BITBOARD b_king;
    BITBOARD b_occupied;
  
    BITBOARD occupied_rl90;
    BITBOARD occupied_rl45;
    BITBOARD occupied_rr45;

    BITBOARD hash_key;
    BITBOARD pawn_hash_key;
  
    BITBOARD rooks_queens;
    BITBOARD bishops_queens;
  
    BITBOARD enpassant_target;
  
    int material_evaluation;
    signed char board[64];
    signed char w_castle;
    signed char b_castle;
    signed char white_pieces;
    signed char white_pawns;
    signed char black_pieces;
    signed char black_pawns;
    signed char white_king;
    signed char black_king;
    signed char rule_50_moves;
  } CHESS_POSITION;
  
  typedef struct {
    BITBOARD word1;
    BITBOARD word2;
  } HASH_ENTRY;
  
  typedef struct {
    int path_hashed;
    int path_length;
    int path_iteration_depth;
    int path[MAXPLY];
  } CHESS_PATH;
  
  typedef struct {
    int extensions[MAXPLY];
  } CHESS_PATH_EXT;
  
  typedef struct {
    BITBOARD to;
    PHASES phase;
    WHATS_GENERATED whats_generated;
    int remaining;
    int *last;
    int *temp;
    int *current;
  } NEXT_MOVE;

  typedef struct {
    BITBOARD position;
    BITBOARD status;
  } BOOK_POSITION;

#if defined(COMPACT_ATTACKS)
  #define NDIAG_ATTACKS	   296
  #define NRANK_ATTACKS	    70
  #define NFILE_ATTACKS	    70

  #define NSHORT_MOBILITY  116

  #define MAX_ATTACKS_FROM_SQUARE 12

  typedef struct {
    /* Fields for the diagonal */
    BITBOARD *d_attacks;
    unsigned char *d_mobility;
    unsigned char *d_which_attack;
    unsigned char d_shift;
    unsigned char d_mask;

    /* Fields for the anti diagonal */
    unsigned char ad_shift;
    unsigned char ad_mask;
    unsigned char *ad_which_attack;
    unsigned char *ad_mobility;
    BITBOARD *ad_attacks;
  } DIAG_INFO;
#endif  
#endif
