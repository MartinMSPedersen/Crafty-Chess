/*
 *******************************************************************************
 *                                                                             *
 *   configuration information:  the following variables need to be set to     *
 *   indicate the machine configuration/capabilities.                          *
 *                                                                             *
 *   HAS_64BITS:  define this for a machine that has true 64-bit hardware      *
 *   including leading-zero hardware, population count, etc.  ie, a Cray-like  *
 *   machine.                                                                  *
 *                                                                             *
 *   HAS_LONGLONG:  define this for a 32-bit machine with a compiler that      *
 *   supports the long long (64-bit) integer data and allows bitwise operations*
 *   on this data type.  this provides significantly faster execution time as  *
 *   the bitwise operators are done by the compiler rather than by procedure   *
 *   calls.                                                                    *
 *                                                                             *
 *   UNIX:  define this if the program is being run on a unix-based system,    *
 *   which causes the executable to use unix-specific runtime utilities.       *
 *                                                                             *
 *   SMP:  this enables the symmetric multiprocessing code that allows Crafty  *
 *   to spawn threads and execute a parallel search.  Note that if this is set,*
 *   then the next variable must be defined as well.                           *
 *                                                                             *
 *   CPUS=N:  this sets up data structures to the appropriate size to support  *
 *   up to N simultaneous search engines.  note that you can set this to a     *
 *   value larger than the max processors you currently have, because the mt=n *
 *   command (added to the command line or your crafty.rc/.craftyrc file) will *
 *   control how many threads are actually spawned.                            *
 *                                                                             *
 *******************************************************************************
 */
#if !defined(TYPES_INCLUDED)
#if defined (_MSC_VER) && (_MSC_VER >= 1300) && \
    (!defined(_M_IX86) || (_MSC_VER >= 1400))
#  define RESTRICT __restrict
#else
#  define RESTRICT
#endif
#if !defined(CPUS)
#  define CPUS 1
#endif
#if defined(SMP)
#  if defined(NT_i386)
#    include <windows.h>
#    include <process.h>
#  endif
#endif
#define TYPES_INCLUDED
#define CDECL
#define STDCALL
/* Provide reasonable defaults for UNIX systems. */
#undef  HAS_64BITS      /* machine has 64-bit integers / operators    */
#define HAS_LONGLONG    /* machine has 32-bit/64-bit integers         */
#define UNIX            /* system is unix-based                       */
/* Architecture-specific definitions */
#if defined(AIX)
#  undef  HAS_64BITS    /* machine has 64-bit integers / operators    */
#  define HAS_LONGLONG  /* machine has 32-bit/64-bit integers         */
#  define UNIX          /* system is unix-based                       */
#endif
#if defined(ALPHA)
#  define HAS_64BITS    /* machine has 64-bit integers / operators    */
#  undef  HAS_LONGLONG  /* machine has 32-bit/64-bit integers         */
#  define UNIX          /* system is unix-based                       */
#endif
#if defined(AMIGA)
#  undef  HAS_64BITS    /* machine has 64-bit integers / operators    */
#  define HAS_LONGLONG  /* machine has 32-bit/64-bit integers         */
#  undef  UNIX          /* system is unix-based                       */
#endif
#if defined(FreeBSD)
#  undef  HAS_64BITS    /* machine has 64-bit integers / operators    */
#  define HAS_LONGLONG  /* machine has 32-bit/64-bit integers         */
#  define UNIX          /* system is unix-based                       */
#endif
#if defined(HP)
#  undef  HAS_64BITS    /* machine has 64-bit integers / operators    */
#  define HAS_LONGLONG  /* machine has 32-bit/64-bit integers         */
#  define UNIX          /* system is unix-based                       */
#endif
#if defined(LINUX)
#  undef  HAS_64BITS    /* machine has 64-bit integers / operators    */
#  define HAS_LONGLONG  /* machine has 32-bit/64-bit integers         */
#  define UNIX          /* system is unix-based                       */
#endif
#if defined(MIPS)
#  undef  HAS_64BITS    /* machine has 64-bit integers / operators    */
#  define HAS_LONGLONG  /* machine has 32-bit/64-bit integers         */
#  define UNIX          /* system is unix-based                       */
#endif
#if defined(NetBSD)
#  if defined(__alpha__)
#    define HAS_64BITS   /* machine has 64-bit integers / operators   */
#    undef  HAS_LONGLONG /* machine has 32-bit/64-bit integers        */
#    define UNIX         /* system is unix-based                      */
#  else
#    undef  HAS_64BITS   /* machine has 64-bit integers / operators   */
#    define HAS_LONGLONG /* machine has 32-bit/64-bit integers        */
#    define UNIX         /* system is unix-based                      */
#  endif
#endif
#if defined(NEXT)
#  undef  HAS_64BITS    /* machine has 64-bit integers / operators    */
#  define HAS_LONGLONG  /* machine has 32-bit/64-bit integers         */
#  define UNIX          /* system is unix-based                       */
#endif
#if defined(NT_i386)
#  undef  HAS_64BITS    /* machine has 64-bit integers / operators    */
#  define HAS_LONGLONG  /* machine has 32-bit/64-bit integers         */
#  undef  UNIX          /* system is unix-based                       */
#  undef  STDCALL
#  define STDCALL __stdcall
#  ifdef  VC_INLINE32
#    undef  CDECL
#    define CDECL __cdecl
#  endif
#endif
#if defined(OS2)
#  undef  HAS_64BITS    /* machine has 64-bit integers / operators    */
#  define HAS_LONGLONG  /* machine has 32-bit/64-bit integers         */
#  define UNIX          /* system is unix-based                       */
#endif
#if defined(SGI)
#  undef  HAS_64BITS    /* machine has 64-bit integers / operators    */
#  define HAS_LONGLONG  /* machine has 32-bit/64-bit integers         */
#  define UNIX          /* system is unix-based                       */
#endif
#if defined(SUN)
#  undef  HAS_64BITS    /* machine has 64-bit integers / operators    */
#  define HAS_LONGLONG  /* machine has 32-bit/64-bit integers         */
#  define UNIX          /* system is unix-based                       */
#endif
#if !defined(BOOKDIR)
#  define     BOOKDIR        "."
#endif
#if !defined(PERSDIR)
#  define     PERSDIR        "."
#endif
#if !defined(LOGDIR)
#  define      LOGDIR        "."
#endif
#if !defined(TBDIR)
#  define       TBDIR     "./TB"
#endif
#if !defined(RCDIR)
#  define       RCDIR        "."
#endif
#if !defined(NOEGTB)
#define     EGTB_CACHE_DEFAULT               1024*1024
#endif
#define     MAXPLY                                  65
#define     MAX_BLOCKS_PER_CPU                      64
#define     MAX_BLOCKS         MAX_BLOCKS_PER_CPU*CPUS
#define     MAX_TC_NODES                       3000000
#if !defined(SMP) && !defined(SUN)
#  define lock_t volatile int
#endif
#include "lock.h"
#define      BOOK_CLUSTER_SIZE           8000
#define     BOOK_POSITION_SIZE             20
#define            MERGE_BLOCK           1000
#define             SORT_BLOCK        4000000
#define         LEARN_INTERVAL             10
#define        LEARN_WINDOW_LB            -40
#define        LEARN_WINDOW_UB            +40
#define      LEARN_COUNTER_BAD            -80
#define     LEARN_COUNTER_GOOD           +100
#define         CAP_SCORE_GOOD           +150
#define          CAP_SCORE_BAD           -100
#define PLY                       4
#define MATE                  32768
#define PAWN_VALUE              100
#define KNIGHT_VALUE            300
#define BISHOP_VALUE            300
#define ROOK_VALUE              500
#define QUEEN_VALUE             900
#define KING_VALUE            40000
#define EG_MAT                   14
#define MAX_DRAFT             32000
#if defined(HAS_64BITS)
typedef unsigned long BITBOARD;
#elif defined(NT_i386)
typedef unsigned __int64 BITBOARD;
#else
typedef unsigned long long BITBOARD;
#endif
#if defined(NT_i386)
#   define BMF   "%I64u"
#   define BMF6  "%6I64u"
#   define BMF10 "%10I64u"
#else
#   define BMF   "%llu"
#   define BMF6  "%6llu"
#   define BMF10 "%10llu"
#endif
#include <time.h>
#if !defined(CLOCKS_PER_SEC)
#  define CLOCKS_PER_SEC 1000000
#endif
typedef enum {
  A1, B1, C1, D1, E1, F1, G1, H1,
  A2, B2, C2, D2, E2, F2, G2, H2,
  A3, B3, C3, D3, E3, F3, G3, H3,
  A4, B4, C4, D4, E4, F4, G4, H4,
  A5, B5, C5, D5, E5, F5, G5, H5,
  A6, B6, C6, D6, E6, F6, G6, H6,
  A7, B7, C7, D7, E7, F7, G7, H7,
  A8, B8, C8, D8, E8, F8, G8, H8,
  BAD_SQUARE
} squares;
typedef enum { FILEA, FILEB, FILEC, FILED, FILEE, FILEF, FILEG, FILEH } files;
typedef enum { RANK1, RANK2, RANK3, RANK4, RANK5, RANK6, RANK7, RANK8 } ranks;
typedef enum { none = 0, pawn = 1, knight = 2, king = 3,
  bishop = 5, rook = 6, queen = 7
} PIECE;
typedef enum { empty_v = 0, pawn_v = 1, knight_v = 3,
  bishop_v = 3, rook_v = 5, queen_v = 9, king_v = 99
} PIECE_V;
typedef enum { no_extension = 0, check_extension = 1,
  one_reply_extension = 2, mate_extension = 4
} EXTENSIONS;
typedef enum { think = 1, puzzle = 2, book = 3, annotate = 4 } SEARCH_TYPE;
typedef enum { normal_mode, tournament_mode } PLAYING_MODE;
typedef enum { crafty, opponent } PLAYER;
typedef enum { book_learning = 1, position_learning = 2,
  result_learning = 4
} LEARNING_MODE;
typedef struct {
  unsigned char enpassant_target;
  signed char w_castle;
  signed char b_castle;
  unsigned char rule_50_moves;
} SEARCH_POSITION;
typedef struct {
  int       move1;
  int       move2;
} KILLER;
typedef struct {
  BITBOARD  w_occupied;
  BITBOARD  b_occupied;
  BITBOARD  occupied_rl90;
  BITBOARD  occupied_rl45;
  BITBOARD  occupied_rr45;
  BITBOARD  rooks_queens;
  BITBOARD  bishops_queens;
  BITBOARD  w_pawn;
  BITBOARD  w_knight;
  BITBOARD  w_bishop;
  BITBOARD  w_rook;
  BITBOARD  w_queen;
  BITBOARD  b_pawn;
  BITBOARD  b_knight;
  BITBOARD  b_bishop;
  BITBOARD  b_rook;
  BITBOARD  b_queen;
  BITBOARD  hash_key;
  BITBOARD  pawn_hash_key;
  int       material_evaluation;
  signed char white_king;
  signed char black_king;
  signed char board[64];
  signed char white_pieces;
  signed char white_pawns;
  signed char black_pieces;
  signed char black_pawns;
  signed char total_pieces;
  signed char minors;
  signed char majors;
} POSITION;
typedef struct {
  BITBOARD  word1;
  BITBOARD  word2;
} TABLE_ENTRY;
typedef struct {
  TABLE_ENTRY prefer;
  TABLE_ENTRY always[2];
} HASH_ENTRY;
typedef struct {
  BITBOARD key;
  int      p_score;
  unsigned char allb;
  unsigned char black_defects_k;
  unsigned char black_defects_e;
  unsigned char black_defects_d;
  unsigned char black_defects_q;
  unsigned char passed_b;
  unsigned char candidates_b;
  unsigned char average_b;
  unsigned char allw;
  unsigned char white_defects_k;
  unsigned char white_defects_e;
  unsigned char white_defects_d;
  unsigned char white_defects_q;
  unsigned char passed_w;
  unsigned char candidates_w;
  unsigned char average_w;
  unsigned char protected;
  unsigned char outside;
  unsigned char open_files;
  unsigned char center;
} PAWN_HASH_ENTRY;
typedef struct {
  int       path[MAXPLY];
  unsigned char pathh;
  unsigned char pathl;
  unsigned char pathd;
} PATH;
typedef struct {
  int       phase;
  int       remaining;
  int      *last;
} NEXT_MOVE;
typedef struct {
  BITBOARD  nodes;
  int       move;
 /*
    xxxx xxx1 = failed low once
    xxxx xx1x = failed low twice
    xxxx x1xx = failed low three times
    xxxx 1xxx = failed high once
    xxx1 xxxx = failed high twice
    xx1x xxxx = failed high three times
    x1xx xxxx = don't search in parallel
    1xxx xxxx = move has been searched
  */
  unsigned char status;
} ROOT_MOVE;

#if defined(NT_i386)
#pragma pack(4)
#endif
typedef struct {
  BITBOARD  position;
  unsigned int status_played;
  float     learn;
  int       CAP_score;
} BOOK_POSITION;

#if defined(NT_i386)
#pragma pack()
#endif
typedef struct {
  unsigned char position[8];
  unsigned char status;
  unsigned char percent_play;
} BB_POSITION;

struct eval_term {
  char *description;
  int  size;
  int *value;
};
struct tree {
  POSITION  pos;
  BITBOARD  save_hash_key[MAXPLY + 2];
  BITBOARD  rep_list[256];
  BITBOARD  all_pawns;
  BITBOARD  nodes_searched;
  BITBOARD  save_pawn_hash_key[MAXPLY + 2];
  PAWN_HASH_ENTRY pawn_score;
  SEARCH_POSITION position[MAXPLY + 2];
  NEXT_MOVE next_status[MAXPLY];
  PATH      pv[MAXPLY];
  int       rep_game;
  int       current_move[MAXPLY];
  int       hash_move[MAXPLY];
  int      *last[MAXPLY];
 # if !defined(NOFUTILITY)
  unsigned int fprune;
 # endif
 # if !defined(LIMITEXT)
  unsigned int no_limit;
 # endif
  unsigned int fail_high;
  unsigned int fail_high_first;
  unsigned int evaluations;
  unsigned int transposition_probes;
  unsigned int transposition_hits;
  unsigned int transposition_good_hits;
  unsigned int transposition_uppers;
  unsigned int transposition_lowers;
  unsigned int transposition_exacts;
  unsigned int egtb_probes;
  unsigned int egtb_probes_successful;
  unsigned int check_extensions_done;
  unsigned int one_reply_extensions_done;
  unsigned int mate_extensions_done;
  unsigned int reductions_attempted;
  unsigned int reductions_done;
  KILLER    killers[MAXPLY];
  int       move_list[5120];
  int       sort_value[256];
  signed char in_check[MAXPLY];
  signed char phase[MAXPLY];
  int       search_value;
  int       w_safety, b_safety;
  int       w_tropism, b_tropism;
  int       endgame;
  int       root_move;
  lock_t    lock;
  int       thread_id;
  volatile char stop;
  char      root_move_text[16];
  char      remaining_moves_text[16];
  struct tree *volatile siblings[CPUS], *parent;
  volatile int nprocs;
  int       alpha;
  int       beta;
  int       value;
  int       wtm;
  int       depth;
  int       ply;
  int       mate_threat;
  volatile int used;
};

typedef struct tree TREE;

typedef struct {
  int       time_abort;
  int       abort_search;
  int       iteration_depth;
  int       root_alpha;
  int       root_beta;
  int       root_value;
  int       root_wtm;
  int       last_root_value;
  ROOT_MOVE root_moves[256];
  int       n_root_moves;
  int       easy_move;
  int       time_limit;
  int       absolute_time_limit;
  int       search_time_limit;
  int       burp;
  volatile int quit;
  unsigned int opponent_start_time, opponent_end_time;
  unsigned int program_start_time, program_end_time;
  unsigned int start_time, end_time;
  unsigned int elapsed_start, elapsed_end;
# if defined(SMP)
    TREE     *local[MAX_BLOCKS + 1];
    TREE     *volatile thread[CPUS];
    lock_t    lock_smp, lock_io, lock_root;
# else
    TREE     *local[1];
#  endif
  unsigned int history[8192];
  unsigned int history_fh[8192];
  unsigned int history_count[8192];
  unsigned int parallel_splits;
  unsigned int parallel_aborts;
  unsigned int max_split_blocks;
  volatile unsigned int splitting;
  volatile int smp_idle;
  volatile int smp_threads;
  volatile int initialized_threads;
  int       crafty_is_white;
  int       average_nps;
  int       nodes_between_time_checks;
  int       nodes_per_second;
  int       next_time_check;
  int       transposition_id;
  int       thinking;
  int       pondering;
  int       puzzling;
  int       booking;
  int       trojan_check;
  int       computer_opponent;
  int       display_options;
  int       max_threads;
  int       min_thread_depth;
  int       max_thread_group;
  int       split_at_root;
  unsigned int noise_level;
  int       tc_moves;
  int       tc_time;
  int       tc_time_remaining;
  int       tc_time_remaining_opponent;
  int       tc_moves_remaining;
  int       tc_secondary_moves;
  int       tc_secondary_time;
  int       tc_increment;
  int       tc_sudden_death;
  int       tc_operator_time;
  int       tc_safety_margin;
  int       draw_score[2];
  char      kibitz_text[512];
  int       kibitz_depth;
  int       move_number;
  int       root_print_ok;
  int       moves_out_of_book;
  int       first_nonbook_factor;
  int       first_nonbook_span;
} SHARED;
/*
   DO NOT modify these.  these are constants, used in multiple modules.
   modification may corrupt the search in any number of ways, all bad.
 */
#define WORTHLESS                 0
#define LOWER                     1
#define UPPER                     2
#define EXACT                     3
#define AVOID_NULL_MOVE           4
#define EXACTEGTB                 5
#define NULL_MOVE                 0
#define DO_NULL                   1
#define NO_NULL                   0
#define NONE                      0
#define HASH_MOVE                 1
#define GENERATE_CAPTURE_MOVES    2
#define CAPTURE_MOVES             3
#define KILLER_MOVE_1             4
#define KILLER_MOVE_2             5
#define GENERATE_ALL_MOVES        6
#define SORT_ALL_MOVES            7
#define HISTORY_MOVES_1           8
#define HISTORY_MOVES_2           9
#define REMAINING_MOVES          10
#define ROOT_MOVES               11

#if defined(VC_INLINE32)
#  include "vcinline.h"
#else
#  if !defined(INLINE64) && !defined(INLINE32)
int CDECL PopCnt(BITBOARD);
int CDECL FirstOne(BITBOARD);
int CDECL LastOne(BITBOARD);
#  endif
#endif
void      Analyze(void);
void      Annotate(void);
void      AnnotateHeaderHTML(char *, FILE *);
void      AnnotateFooterHTML(FILE *);
void      AnnotatePositionHTML(TREE * RESTRICT, int, FILE *);
char     *AnnotateVtoNAG(int, int, int, int);
void      AnnotateHeaderTeX(char *, FILE *);
void      AnnotateFooterTeX(FILE *);
void      AnnotatePositionTeX(TREE *, int, FILE *);
int       Attacked(TREE * RESTRICT, int, int);
BITBOARD  AttacksTo(TREE * RESTRICT, int);
void      Bench(void);
int       Book(TREE * RESTRICT, int, int);
void      BookClusterIn(FILE *, int , BOOK_POSITION *);
void      BookClusterOut(FILE *, int , BOOK_POSITION *);
int       BookIn32(unsigned char *ch);
float     BookIn32f(unsigned char *ch);
BITBOARD  BookIn64(unsigned char *ch);
int       BookMask(char *);
unsigned char *BookOut32(int val);
unsigned char *BookOut32f(float val);
unsigned char *BookOut64(BITBOARD val);
int       BookPonderMove(TREE * RESTRICT, int);
void      BookUp(TREE * RESTRICT, int, char **);
void      BookSort(BB_POSITION *, int, int);

#if defined(NT_i386)
int _cdecl BookUpCompare(const void *, const void *);
#else
int       BookUpCompare(const void *, const void *);
#endif
BB_POSITION BookUpNextPosition(int, int);
int       CheckInput(void);
void      ClearHashTableScores(int);
void      ComputeAttacksAndMobility(void);
void      CopyFromSMP(TREE * RESTRICT, TREE * RESTRICT, int);
TREE     *CopyToSMP(TREE * RESTRICT, int);
void      CraftyExit(int);
void      DGTInit(int, char **);
int       DGTCheckInput(void);
void      DGTRead(void);
void      DisplayArray(int*, int);
void      DisplayBitBoard(BITBOARD);
void      Display64BitWord(BITBOARD);
void      DisplayChessBoard(FILE *, POSITION);
char     *DisplayEvaluation(int, int);
char     *DisplayEvaluationKibitz(int, int);
void      DisplayFT(int, int, int);
char     *DisplayHHMM(unsigned int);
char     *DisplayKM(unsigned int);
void      DisplayPieceBoards(int *, int *);
void      DisplayPV(TREE * RESTRICT, int, int, int, int, PATH *);
char     *DisplaySQ(unsigned int);
char     *DisplayTime(unsigned int);
char     *DisplayTimeKibitz(unsigned int);
void      DisplayTreeState(TREE * RESTRICT, int, int, int);
void      Display2BitBoards(BITBOARD, BITBOARD);
void      DisplayChessMove(char *, int);
int       Drawn(TREE * RESTRICT, int);
void      Edit(void);
#if !defined(NOEGTB)
int       EGTBProbe(TREE * RESTRICT, int, int, int *);
void      EGTBPV(TREE * RESTRICT, int);
#endif
int       EnPrise(int, int);
int       Evaluate(TREE * RESTRICT, int, int, int, int);
int       EvaluateBishops(TREE * RESTRICT);
int       EvaluateDevelopmentB(TREE * RESTRICT, int);
int       EvaluateDevelopmentW(TREE * RESTRICT, int);
int       EvaluateDraws(TREE * RESTRICT, int, int, int);
int       EvaluateKings(TREE * RESTRICT, int, int);
int       EvaluateKingsFileB(TREE * RESTRICT, int);
int       EvaluateKingsFileW(TREE * RESTRICT, int);
int       EvaluateKnights(TREE * RESTRICT);
int       EvaluateMate(TREE * RESTRICT);
int       EvaluateMaterial(TREE * RESTRICT);
int       EvaluatePassedPawns(TREE * RESTRICT, int);
int       EvaluatePassedPawnRaces(TREE * RESTRICT, int);
int       EvaluatePawns(TREE * RESTRICT);
int       EvaluateQueens(TREE * RESTRICT);
int       EvaluateRooks(TREE * RESTRICT);
int       EvaluateStalemate(TREE * RESTRICT, int);
int       EvaluateWinningChances(TREE * RESTRICT);
void      EVTest(char *);
int       FindBlockID(TREE * RESTRICT);
char     *FormatPV(TREE * RESTRICT, int, PATH);
int       FTbSetCacheSize(void *, unsigned long);
int      *GenerateCaptures(TREE * RESTRICT, int, int, int *);
int      *GenerateCheckEvasions(TREE * RESTRICT, int, int, int *);
int      *GenerateNonCaptures(TREE * RESTRICT, int, int, int *);
int       HashProbe(TREE * RESTRICT, int, int, int, int *, int, int *);
void      HashStore(TREE * RESTRICT, int, int, int, int, int, int);
void      HashStorePV(TREE * RESTRICT, int, int);
int       HasOpposition(int, int, int);
void      History(TREE * RESTRICT, int, int, int, int);
void      HistoryAge(TREE * RESTRICT);
void      HistoryUpdateFH(TREE * RESTRICT, int, int*, int);
int       IInitializeTb(char *);
void      Initialize(void);
void      InitializeAttackBoards(void);
void      InitializeChessBoard(SEARCH_POSITION *);
void      InitializeEvaluation(void);
int       InitializeFindAttacks(int, int, int);
int       InitializeGetLogID();
void      InitializeHashTables(void);
void      InitializeHistoryKillers(void);
void      InitializeKingSafety(void);
void      InitializeMasks(void);
void      InitializePawnMasks(void);
void      InitializePieceMasks(void);
void      InitializeRandomHash(void);
void      InitializeSharedData(void);
void      InitializeSMP(void);
void      InitializeZeroMasks(void);
int       InputMove(TREE * RESTRICT, char *, int, int, int, int);
int       InputMoveICS(TREE * RESTRICT, char *, int, int, int, int);
BITBOARD  InterposeSquares(int, int, int);
void      Interrupt(int);
int       InvalidPosition(TREE * RESTRICT);
int       Iterate(int, int, int);
int       KingPawnSquare(int, int, int, int);
void      LearnBook(TREE * RESTRICT, int, int, int, int, int);
void      LearnBookUpdate(TREE * RESTRICT, int, int, float);
int       LearnFunction(int, int, int, int);
void      LearnImport(TREE * RESTRICT, int, char **);
void      LearnImportBook(TREE * RESTRICT, int, char **);
void      LearnImportCAP(TREE * RESTRICT, int, char **);
void      LearnImportPosition(TREE * RESTRICT, int, char **);
void      LearnPosition(TREE * RESTRICT, int, int, int);
void      LearnPositionLoad(void);
int       LegalMove(TREE * RESTRICT, int, int, int);
void      MakeMove(TREE * RESTRICT, int, int, int);
void      MakeMoveRoot(TREE * RESTRICT, int, int);
void      NewGame(int);
int       NextEvasion(TREE * RESTRICT, int, int);
int       NextMove(TREE * RESTRICT, int, int);
int       NextRootMove(TREE * RESTRICT, TREE * RESTRICT, int);
int       NextRootMoveParallel(void);
char     *Normal(void);
int       Option(TREE * RESTRICT);
int       OptionMatch(char *, char *);
void      OptionPerft(TREE * RESTRICT, int, int, int);
char     *OutputMove(TREE * RESTRICT, int, int, int);
char     *OutputMoveICS(int);
int       OutputGood(TREE * RESTRICT, char *, int, int);
int       ParseTime(char *);
void      Pass(void);
int       PinnedOnKing(TREE * RESTRICT, int, int);
int       Ponder(int);
void      PreEvaluate(TREE * RESTRICT);
void      Print(int, char *, ...);
char     *PrintKM(size_t, int);
int       Quiesce(TREE * RESTRICT, int, int, int, int);
unsigned int Random32(void);
BITBOARD  Random64(void);
int       Read(int, char *);
int       ReadChessMove(TREE * RESTRICT, FILE *, int, int);
void      ReadClear(void);
unsigned int ReadClock(void);
int       ReadPGN(FILE *, int);
int       ReadNextMove(TREE * RESTRICT, char *, int, int);
int       ReadParse(char *, char *args[], char *);
int       ReadInput(void);
int       RepetitionCheck(TREE * RESTRICT, int);
int       RepetitionCheckBook(TREE * RESTRICT, int);
int       RepetitionDraw(TREE * RESTRICT, int ply);
void      ResignOrDraw(TREE * RESTRICT, int);
void      RestoreGame(void);
char     *Reverse(void);
void      RootMoveList(int);
int       Search(TREE * RESTRICT, int, int, int, int, int, int);
int       SearchControl(TREE * RESTRICT, int, int, int, int);
void      SearchOutput(TREE * RESTRICT, int, int);
int       SearchRoot(TREE * RESTRICT, int, int, int, int);
int       SearchSMP(TREE * RESTRICT, int, int, int, int, int, int, int);
void      SearchTrace(TREE * RESTRICT, int, int, int, int, int, char *, int);
void      SetBoard(SEARCH_POSITION *, int, char **, int);
void      SetChessBitBoards(SEARCH_POSITION *);
int       SetRootAlpha(unsigned char, int);
int       SetRootBeta(unsigned char, int);
void     *SharedMalloc(size_t, int);
void      SharedFree(void *address);
void      SignalInterrupt(int);
int       StrCnt(char *, char);
int       Swap(TREE * RESTRICT, int, int, int);
BITBOARD  SwapXray(TREE * RESTRICT, BITBOARD, int, int);
void      Test(char *);
void      TestEPD(char *);
int       Thread(TREE * RESTRICT);
void      WaitForAllThreadsInitialized(void);
void     *STDCALL ThreadInit(void *);
#if (defined(_WIN32) || defined(_WIN64)) && defined(SMP)
void      ThreadMalloc(int);
#endif
void      ThreadStop(TREE * RESTRICT);
int       ThreadWait(int, TREE * RESTRICT);
int       Threat(TREE * RESTRICT, int, int, int, int, int, int);
void      TimeAdjust(int, PLAYER);
int       TimeCheck(TREE * RESTRICT, int);
void      TimeSet(int);
void      UnmakeMove(TREE * RESTRICT, int, int, int);
int       ValidMove(TREE * RESTRICT, int, int, int);
void      ValidatePosition(TREE * RESTRICT, int, int, char *);
void      Kibitz(int, int, int, int, int, BITBOARD, int, char *);

#define HistoryIndex(move, wtm) ((move & 4095) + wtm * 4096)

#if (defined(_WIN32) || defined(_WIN64)) && defined(SMP)
extern void *WinMallocInterleaved(size_t, int);
extern void WinFreeInterleaved(void *, size_t);

#define MallocInterleaved(cBytes, cThreads)\
    WinMallocInterleaved(cBytes, cThreads)
#define FreeInterleaved(pMemory, cBytes)\
    WinFreeInterleaved(pMemory, cBytes)
#else
#  if defined(NUMA)
#    define MallocInterleaved(cBytes, cThreads) numa_alloc_interleaved(cBytes)
#    define FreeInterleaved(pMemory, cBytes)    numa_free(pMemory, 1)
#  else
#    define MallocInterleaved(cBytes, cThreads) malloc(cBytes)
#    define FreeInterleaved(pMemory, cBytes)    free(pMemory)
#  endif
#endif
#if defined(HAS_64BITS) || defined(HAS_LONGLONG)
#  if defined(ALPHA)
#    include <machine/builtins.h>
/* The following are defined only on Unix 4.0E and later. */
#    ifdef _int_mult_upper      /* kludge to identify version of builtins.h */
#      define PopCnt(a)     _popcnt(a)
#      define FirstOne(a)   _leadz(a)
#      define LastOne(a)    (63 - _trailz(a))
#    endif
#  endif
#endif
#define Max(a,b)  (((a) > (b)) ? (a) : (b))
#define Min(a,b)  (((a) < (b)) ? (a) : (b))
#define FileDistance(a,b) abs(File(a) - File(b))
#define RankDistance(a,b) abs(Rank(a) - Rank(b))
#define Distance(a,b) Max(FileDistance(a,b),RankDistance(a,b))
#define DrawScore(wtm)                 (shared->draw_score[wtm])
#define PopCnt8Bit(a) (pop_cnt_8bit[a])
#define FirstOne8Bit(a) (first_one_8bit[a])
#define LastOne8Bit(a) (last_one_8bit[a])
/*
   the following macro is used to limit the search extensions based on the
   current iteration depth and current ply in the tree.
 */
# if !defined(LIMITEXT)
#define LimitExtensions(extended,ply)                                        \
      extended=Min(extended,PLY);                                            \
      if (ply > 2*shared->iteration_depth && !tree->no_limit) {              \
        if (ply <= 4*shared->iteration_depth)                                \
          extended=extended*(4*shared->iteration_depth-ply)/                 \
                   (2*shared->iteration_depth);                              \
        else                                                                 \
          extended=0;                                                        \
      }
# else
#define LimitExtensions(extended,ply)                                        \
      extended=Min(extended,PLY);                                            \
      if (ply > 2*shared->iteration_depth) {                                 \
        if (ply <= 4*shared->iteration_depth)                                \
          extended=extended*(4*shared->iteration_depth-ply)/                 \
                   (2*shared->iteration_depth);                              \
        else                                                                 \
          extended=0;                                                        \
      }
# endif

/*
   the following macro is used to determine if one side is in check.  it
   simply returns the result of Attacked().
 */
#define Check(wtm)                                                           \
  Attacked(tree, (wtm)?WhiteKingSQ:BlackKingSQ,Flip(wtm))
/*
   Attack() is used to determine if a sliding piece on 'from' can reach
   'to'.  the only requirement is that there be no pieces along the pathway
   connecting from and to.
 */
#define Attack(from,to) (!(obstructed[from][to] & Occupied))
/*
   the following macros are used to construct the attacks from a square.
   the attacks are computed as four separate bit vectors, one for each of the
   two diagonals, and one for the ranks and one for the files.  these can be
   Or'ed together to produce the attack bitmaps for bishops, rooks and queens.
 */
#  define AttacksRook(a)    (AttacksRank(a)|AttacksFile(a))
#  define AttacksBishop(a)  (AttacksDiaga1(a)|AttacksDiagh1(a))
#define AttacksQueen(a)   (AttacksBishop(a)|AttacksRook(a))
#define Rank(x)       ((x)>>3)
#define File(x)       ((x)&7)
#define Flip(x)       ((x)^1)
#  define AttacksRank(a)                                                   \
      rook_attacks_r0[(a)][((tree->pos.w_occupied|tree->pos.b_occupied)>>  \
                           (57-((a)&56)))&63]
#  define AttacksFile(a)                                                   \
      rook_attacks_rl90[(a)][(tree->pos.occupied_rl90>>                    \
                             (57-(File(a)<<3)))&63]
#  define AttacksDiaga1(a)                                                 \
      bishop_attacks_rl45[(a)][(tree->pos.occupied_rl45>>                  \
                                bishop_shift_rl45[(a)])&63]
#  define AttacksDiagh1(a)                                                 \
      bishop_attacks_rr45[(a)][(tree->pos.occupied_rr45>>                  \
                                bishop_shift_rr45[(a)])&63]
/*
   the following macros are used to compute the mobility for a sliding piece.
   The basic idea is the same as the attack vectors above, but the result is
   an integer mobility factor rather than a bitboard.  this saves having to
   do a PopCnt() on the attack bit vector, which is much slower.
 */
#define MobilityRook(a)   (MobilityRank(a)+MobilityFile(a))
#define MobilityBishop(a) (MobilityDiaga1(a)+MobilityDiagh1(a))
#define MobilityQueen(a)  (MobilityBishop(a)+MobilityRook(a))
#  define MobilityRank(a)                                                   \
     (rook_mobility_r0[(a)][(tree->pos.w_occupied|tree->pos.b_occupied)>>   \
                            (57-((a)&56))&63])
#  define MobilityFile(a)                                                   \
     (rook_mobility_rl90[(a)][tree->pos.occupied_rl90>>                     \
                             (57-(File(a)<<3))&63])
#  define MobilityDiaga1(a)                                                 \
     (bishop_mobility_rl45[(a)][(tree->pos.occupied_rl45>>                  \
                                bishop_shift_rl45[(a)])&63])
#  define MobilityDiagh1(a)                                                 \
     (bishop_mobility_rr45[(a)][(tree->pos.occupied_rr45>>                  \
                                bishop_shift_rr45[(a)])&63])
/*
   the following macros are used to extract the pieces of a move that are
   kept compressed into the rightmost 21 bits of a simple integer.
 */
#define From(a)             ((a)&63)
#define To(a)               (((a)>>6)&63)
#define Piece(a)            (((a)>>12)&7)
#define Captured(a)         (((a)>>15)&7)
#define Promote(a)          (((a)>>18)&7)
#define CaptureOrPromote(a) (((a)>>15)&63)
#define SetMask(a)          (set_mask[a])
#define SetMaskRL90(a)      (set_mask_rl90[a])
#define SetMaskRL45(a)      (set_mask_rl45[a])
#define SetMaskRR45(a)      (set_mask_rr45[a])
#define ClearMask(a)        (clear_mask[a])
#define ClearMaskRL90(a)    (clear_mask_rl90[a])
#define ClearMaskRL45(a)    (clear_mask_rl45[a])
#define ClearMaskRR45(a)    (clear_mask_rr45[a])
/*
   the following macros are used to extract the correct bits for the piece
   type desired.
 */
#define BlackPawns            (tree->pos.b_pawn)
#define BlackKnights          (tree->pos.b_knight)
#define BlackBishops          (tree->pos.b_bishop)
#define BlackRooks            (tree->pos.b_rook)
#define BlackQueens           (tree->pos.b_queen)
#define BlackKing             (SetMask(tree->pos.black_king))
#define BlackKingSQ           (tree->pos.black_king)
#define BlackCastle(ply)      (tree->position[ply].b_castle)
#define TotalBlackPawns       (tree->pos.black_pawns)
#define TotalBlackPieces      (tree->pos.black_pieces)
#define TotalBlackMaterial    (tree->pos.black_pieces+tree->black_pawns)
#define BlackPieces           (tree->pos.b_occupied)
#define Minors                (tree->pos.minors)
#define Majors                (tree->pos.majors)
#define WhitePawns            (tree->pos.w_pawn)
#define WhiteKnights          (tree->pos.w_knight)
#define WhiteBishops          (tree->pos.w_bishop)
#define WhiteRooks            (tree->pos.w_rook)
#define WhiteQueens           (tree->pos.w_queen)
#define WhiteKing             (SetMask(tree->pos.white_king))
#define WhiteKingSQ           (tree->pos.white_king)
#define WhiteCastle(ply)      (tree->position[ply].w_castle)
#define TotalWhitePawns       (tree->pos.white_pawns)
#define TotalWhitePieces      (tree->pos.white_pieces)
#define TotalWhiteMaterial    (tree->pos.white_pieces+tree->white_pawns)
#define WhitePieces           (tree->pos.w_occupied)
#define TotalPieces           (tree->pos.total_pieces)
#define Material              (tree->pos.material_evaluation)
#define Rule50Moves(ply)      (tree->position[ply].rule_50_moves)
#define HashKey               (tree->pos.hash_key)
#define PawnHashKey           (tree->pos.pawn_hash_key)
#define EnPassant(ply)        (tree->position[ply].enpassant_target)
#define EnPassantTarget(ply)  (EnPassant(ply) ? SetMask(EnPassant(ply)) : 0)
#define PcOnSq(sq)            (tree->pos.board[sq])
#define BishopsQueens         (tree->pos.bishops_queens)
#define RooksQueens           (tree->pos.rooks_queens)
#define Occupied              (tree->pos.w_occupied|tree->pos.b_occupied)
#define OccupiedRL90          (tree->pos.occupied_rl90)
#define OccupiedRL45          (tree->pos.occupied_rl45)
#define OccupiedRR45          (tree->pos.occupied_rr45)
#define Sliding(piece)        ((piece) & 4)
#define SlidingDiag(piece)    (((piece) & 5) == 5)
#define SlidingRow(piece)     (((piece) & 6) == 6)
/*
   the following macros are used to Set and Clear a specific bit in the
   second argument.  this is done to make the code more readable, rather
   than to make it faster.
 */
#define ClearSet(a,b)       b=((a)^(b))
#define Clear(a,b)          b=ClearMask(a)&(b)
#define ClearRL90(a,b)      b=ClearMaskRL90(a)&(b)
#define ClearRL45(a,b)      b=ClearMaskRL45(a)&(b)
#define ClearRR45(a,b)      b=ClearMaskRR45(a)&(b)
#define Set(a,b)            b=SetMask(a)|(b)
#define SetRL90(a,b)        b=SetMaskRL90(a)|(b)
#define SetRL45(a,b)        b=SetMaskRL45(a)|(b)
#define SetRR45(a,b)        b=SetMaskRR45(a)|(b)
#define HashPB(a,b)         b=b_pawn_random[a]^(b)
#define HashPW(a,b)         b=w_pawn_random[a]^(b)
#define HashNB(a,b)         b=b_knight_random[a]^(b)
#define HashNW(a,b)         b=w_knight_random[a]^(b)
#define HashBB(a,b)         b=b_bishop_random[a]^(b)
#define HashBW(a,b)         b=w_bishop_random[a]^(b)
#define HashRB(a,b)         b=b_rook_random[a]^(b)
#define HashRW(a,b)         b=w_rook_random[a]^(b)
#define HashQB(a,b)         b=b_queen_random[a]^(b)
#define HashQW(a,b)         b=w_queen_random[a]^(b)
#define HashKB(a,b)         b=b_king_random[a]^(b)
#define HashKW(a,b)         b=w_king_random[a]^(b)
#define HashEP(a,b)         b=enpassant_random[a]^(b)
#define HashCastleW(a,b)    b=castle_random_w[a]^(b)
#define HashCastleB(a,b)    b=castle_random_b[a]^(b)
#define SavePV(tree,ply,ph) do {                                            \
          tree->pv[ply-1].path[ply-1]=tree->current_move[ply-1];            \
          tree->pv[ply-1].pathl=ply-1;                                      \
          tree->pv[ply-1].pathh=ph;                                         \
          tree->pv[ply-1].pathd=shared->iteration_depth;} while(0)
/*
   Service macro - initialize squares of the particular piece as well as
   counter for that piece. Note: dual initialization saves some time when
   TB is present, but waste for non-present TB. If we often will call
   probing function for an absent TB, maybe we shall split that code.
 */
#define  VInitSqCtr(rgCtr, rgSquares, piece, bitboard) {         \
  int  cPieces=0;                                                \
  BITBOARD bbTemp=(bitboard);                                    \
  while (bbTemp) {                                               \
    const squaret sq=FirstOne (bbTemp);                          \
    (rgSquares)[(piece)*C_PIECES+cPieces]=sq;                    \
    cPieces++;                                                   \
    Clear(sq, bbTemp);                                           \
  }                                                              \
  (rgCtr)[(piece)]=cPieces;                                      \
}
#define mask_120      MaskR(56)

#if defined(INLINE64)
#  include "inline64.h"
#endif
#if defined(INLINE32)
#  include "inline32.h"
#endif
#if defined(UNIX)
#  define SPEAK "./speak "
#else
#  define SPEAK ".\\Speak.exe "
#endif
#endif                          /* if defined(TYPES_INCLUDED) */
