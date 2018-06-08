/*
********************************************************************************
*                                                                              *
*   configuration information:  the following variables need to be set to      *
*   indicate the machine configuration/capabilities.                           *
*                                                                              *
*   there are pre-defined machine types for the following machines: (1) SUN    *
*   (2) DOS (3) ALPHA [DEC Alpha] (4) CRAY  (5) LINUX.  defining any of these  *
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

#if defined(AIX)
#  undef  HAS_64BITS           /* machine has 64-bit integers / operators     */
#  define HAS_LONGLONG         /* machine has 32-bit/64-bit integers          */
#  undef  LITTLE_ENDIAN_ARCH   /* machine stores bytes in "PC" order          */
#  define UNIX                 /* system is unix-based                        */
#endif
#if defined(ALPHA)
#  define HAS_64BITS           /* machine has 64-bit integers / operators     */
#  undef  HAS_LONGLONG         /* machine has 32-bit/64-bit integers          */
#  define LITTLE_ENDIAN_ARCH   /* machine stores bytes in "PC" order          */
#  define UNIX                 /* system is unix-based                        */
#endif
#if defined(AMIGA)
#  undef  HAS_64BITS           /* machine has 64-bit integers / operators     */
#  define HAS_LONGLONG         /* machine has 32-bit/64-bit integers          */
#  undef  LITTLE_ENDIAN_ARCH   /* machine stores bytes in "PC" order          */
#  undef  UNIX                 /* system is unix-based                        */
#endif
#if defined(CRAY1)
#  define HAS_64BITS           /* machine has 64-bit integers / operators     */
#  undef  HAS_LONGLONG         /* machine has 32-bit/64-bit integers          */
#  undef  LITTLE_ENDIAN_ARCH   /* machine stores bytes in "PC" order          */
#  define UNIX                 /* system is unix-based                        */
#endif
#if defined(DOS)
#  undef  HAS_64BITS           /* machine has 64-bit integers / operators     */
#  define HAS_LONGLONG         /* machine has 32-bit/64-bit integers          */
#  define LITTLE_ENDIAN_ARCH   /* machine stores bytes in "PC" order          */
#  undef  UNIX                 /* system is unix-based                        */
#endif
#if defined(FreeBSD)
#  undef  HAS_64BITS           /* machine has 64-bit integers / operators     */
#  define HAS_LONGLONG         /* machine has 32-bit/64-bit integers          */
#  define LITTLE_ENDIAN_ARCH   /* machine stores bytes in "PC" order          */
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
#if defined(NEXT)
#  undef  HAS_64BITS          /* machine has 64-bit integers / operators     */
#  define HAS_LONGLONG        /* machine has 32-bit/64-bit integers          */
#  undef  LITTLE_ENDIAN_ARCH  /* machine stores bytes in "PC" order          */
#  define UNIX                /* system is unix-based                        */
#endif
#if defined(NT_AXP)
#  define HAS_64BITS           /* machine has 64-bit integers / operators     */
#  undef  HAS_LONGLONG         /* machine has 32-bit/64-bit integers          */
#  define LITTLE_ENDIAN_ARCH   /* machine stores bytes in "PC" order          */
#  undef  UNIX                 /* system is unix-based                        */
#endif
#if defined(NT_i386)
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

#  define NULL_MOVE_DEPTH           2

#  define MATE                  65536
#  define PAWN_VALUE             1000 
#  define KNIGHT_VALUE           4000 
#  define BISHOP_VALUE           4000 
#  define ROOK_VALUE             6500 
#  define QUEEN_VALUE           12000 
#  define KING_VALUE           100000 

#  define MAXPLY 65
  
#  if defined(HAS_64BITS)
     typedef unsigned long BITBOARD;
#  else
#    if defined(NT_i386)
       typedef unsigned _int64 BITBOARD;
#    else
       typedef unsigned long long BITBOARD;
#    endif
#  endif
    typedef signed char  SMALL;
    typedef signed short MEDIUM;

#include <time.h>
#if !defined(CLOCKS_PER_SEC)
#  define CLOCKS_PER_SEC 1000000
#endif
  typedef enum { A1,B1,C1,D1,E1,F1,G1,H1,
                 A2,B2,C2,D2,E2,F2,G2,H2,
                 A3,B3,C3,D3,E3,F3,G3,H3,
                 A4,B4,C4,D4,E4,F4,G4,H4,
                 A5,B5,C5,D5,E5,F5,G5,H5,
                 A6,B6,C6,D6,E6,F6,G6,H6,
                 A7,B7,C7,D7,E7,F7,G7,H7,
                 A8,B8,C8,D8,E8,F8,G8,H8,
                 BAD_SQUARE } squares;

  typedef enum {FILEA, FILEB, FILEC, FILED, FILEE, FILEF, FILEG, FILEH} files;

  typedef enum {RANK1, RANK2, RANK3, RANK4, RANK5, RANK6, RANK7, RANK8} ranks;

  typedef enum {empty=0, pawn=1, knight=2, king=3, 
                bishop=5, rook=6, queen=7} PIECE;
  
  typedef enum {empty_v=0, pawn_v=1, knight_v=2, 
                bishop_v=3, rook_v=5, queen_v=9} PIECE_V;
  
  typedef enum {no_extension=0, check_extension=1, recapture_extension=2,
                passed_pawn_extension=4, one_reply_extension=8} EXTENSIONS;
  
  typedef enum {cpu, elapsed, microseconds} TIME_TYPE;

  typedef enum {think=1, puzzle=2, book=3} SEARCH_TYPE;

  typedef enum {normal_mode, tournament_mode} PLAYING_MODE;

  typedef enum {crafty, opponent} PLAYER;
  
  typedef struct {
    unsigned char enpassant_target;
    signed   char w_castle;
    signed   char b_castle;
    unsigned char rule_50_moves;
  } SEARCH_POSITION;

  typedef  struct {
    BITBOARD       hash_key;
    BITBOARD       pawn_hash_key;
    BITBOARD       w_occupied;
    BITBOARD       b_occupied;
    BITBOARD       occupied_rl90;
    BITBOARD       occupied_rl45;
    BITBOARD       occupied_rr45;
    BITBOARD       rooks_queens;
    BITBOARD       bishops_queens;
    BITBOARD       w_pawn;
    BITBOARD       w_knight;
    BITBOARD       w_bishop;
    BITBOARD       w_rook;
    BITBOARD       w_queen;
    int            white_king;
    BITBOARD       b_pawn;
    BITBOARD       b_knight;
    BITBOARD       b_bishop;
    BITBOARD       b_rook;
    BITBOARD       b_queen;
    int            black_king;
    signed char    board[64];
    int            material_evaluation;
    int            white_pieces;
    int            white_pawns;
    int            black_pieces;
    int            black_pawns;
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
    int phase;
    int remaining;
    int *last;
  } NEXT_MOVE;

  typedef struct {
    BITBOARD position;
    BITBOARD status;
  } BOOK_POSITION;

#  if defined(COMPACT_ATTACKS)
#    define NDIAG_ATTACKS	   296
#    define NRANK_ATTACKS	    70
#    define NFILE_ATTACKS	    70

#    define NSHORT_MOBILITY  116

#    define MAX_ATTACKS_FROM_SQUARE 12

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
#  endif  

/*  
    DO NOT modify these.  these are constants, used in multiple modules.
    modification may corrupt the search in any number of ways, all bad.
*/

#  define WORTHLESS                 0
#  define LOWER_BOUND               1
#  define UPPER_BOUND               2
#  define EXACT_SCORE               3
#  define AVOID_NULL_MOVE           4

#  define NULL_MOVE                 0
#  define DO_NULL                   1
#  define NO_NULL                   0

#  define NONE                      0
#  define FIRST_PHASE               1
#  define HASH_MOVE                 1
#  define GENERATE_CAPTURE_MOVES    2
#  define CAPTURE_MOVES             3
#  define KILLER_MOVE_1             4
#  define KILLER_MOVE_2             5
#  define GENERATE_ALL_MOVES        6
#  define SORT_ALL_MOVES            7  
#  define HISTORY_MOVES_1           8
#  define HISTORY_MOVES_2           9
#  define REMAINING_MOVES          10
#  define ROOT_MOVES               11
 
#endif
