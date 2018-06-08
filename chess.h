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
*   SMP:  this enables the symmetric multiprocessing code that allows Crafty   *
*   to spawn threads and execute a parallel search.  Note that if this is set, *
*   then the next variable must be defined as well.                            *
*                                                                              *
*   CPUS=N:  this sets up data structures to the appropriate size to support   *
*   up to N simultaneous search engines.  note that you can set this to a      *
*   value larger than the max processors you currently have, because the mt=n  *
*   command (added to the command line or your crafty.rc/.craftyrc file) will  *
*   control how many threads are actually spawned.                             *
*                                                                              *
********************************************************************************
*/
#if !defined(TYPES_INCLUDED)

#if !defined(CPUS)
#  define CPUS 1
#endif

#if defined(SMP)
#  if (defined(NT_i386) || defined(NT_AXP))
#    include <windows.h>
#    include <process.h>
#  elif defined(LINUX) || defined(ALPHA) || defined(POSIX)
#    include <pthread.h>
#  endif
#endif

#define TYPES_INCLUDED

#define CDECL
#define STDCALL

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
#  undef  LITTLE_ENDIAN_ARCH   /* machine stores bytes in "PC" order          */
#  define UNIX                 /* system is unix-based                        */
#endif
#if defined(NEXT)
#  undef  HAS_64BITS          /* machine has 64-bit integers / operators     */
#  define HAS_LONGLONG        /* machine has 32-bit/64-bit integers          */
#  undef  LITTLE_ENDIAN_ARCH  /* machine stores bytes in "PC" order          */
#  define UNIX                /* system is unix-based                        */
#endif
#if defined(NT_AXP)
#  undef  HAS_64BITS           /* machine has 64-bit integers / operators     */
#  define HAS_LONGLONG         /* machine has 32-bit/64-bit integers          */
#  define LITTLE_ENDIAN_ARCH   /* machine stores bytes in "PC" order          */
#  undef  UNIX                 /* system is unix-based                        */
#endif
#if defined(NT_i386)
#  undef  HAS_64BITS           /* machine has 64-bit integers / operators     */
#  define HAS_LONGLONG         /* machine has 32-bit/64-bit integers          */
#  define LITTLE_ENDIAN_ARCH   /* machine stores bytes in "PC" order          */
#  undef  UNIX                 /* system is unix-based                        */
#  undef  STDCALL
#  define STDCALL __stdcall
#  ifdef  VC_INLINE_ASM
#    undef  CDECL
#    define CDECL __cdecl
#    define COMPACT_ATTACKS
#    define USE_ATTACK_FUNCTIONS
#    define USE_ASSEMBLY_A
#    define USE_ASSEMBLY_B
#  endif
#endif
#if defined(OS2)
#  undef  HAS_64BITS           /* machine has 64-bit integers / operators     */
#  define HAS_LONGLONG         /* machine has 32-bit/64-bit integers          */
#  define LITTLE_ENDIAN_ARCH   /* machine stores bytes in "PC" order          */
#  define UNIX                 /* system is unix-based                        */
#endif
#if defined(SGI)
#  undef  HAS_64BITS           /* machine has 64-bit integers / operators     */
#  define HAS_LONGLONG         /* machine has 32-bit/64-bit integers          */
#  undef  LITTLE_ENDIAN_ARCH   /* machine stores bytes in "PC" order          */
#  define UNIX                 /* system is unix-based                        */
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

#if defined(__MWERKS__)
#  define MACOS
#endif

#if defined(MACOS)
#  undef  HAS_64BITS           /* machine has 64-bit integers / operators     */
#  define HAS_LONGLONG         /* machine has 32-bit/64-bit integers          */
#  undef  LITTLE_ENDIAN_ARCH   /* machine stores bytes in "PC" order          */
#  undef  UNIX                 /* system is unix-based                        */

#  define COMPACT_ATTACKS
#  define USE_ATTACK_FUNCTIONS  

#  define     BOOKDIR    "Books"
#  define      LOGDIR     "Logs"
#  define       TBDIR       "TB"
#else
#  define     BOOKDIR        "."
#  define      LOGDIR        "."
#  define       TBDIR     "./TB"
#endif

# define    EGTB_CACHE_DEFAULT 1024*1024
#define     MAXPLY            65
#define MAX_BLOCKS            64
#define MAX_TC_NODES      300000

#if !defined(SMP)
#  define lock_t int
#endif
#include "lock.h"

#define      BOOK_CLUSTER_SIZE            600
#define            MERGE_BLOCK           1000
#define             SORT_BLOCK         400000
#define         LEARN_INTERVAL             10
#define        LEARN_WINDOW_LB            -40
#define        LEARN_WINDOW_UB            +40
#define      LEARN_COUNTER_BAD            -80
#define     LEARN_COUNTER_GOOD           +100

/*
  fractional ply extensions.  these should be in units based on the
  value of INCREMENT_PLY (default is 60).  a value of 60 means this
  extension is exactly one ply.
*/

#define INCREMENT_PLY            60  /* 1.00 */
#define NULL_MOVE_DEPTH         120  /* 2.00 */
#define RAZORING_DEPTH           60  /* 1.00 */

#define MATE                  32768
#define PAWN_VALUE              100
#define KNIGHT_VALUE            300
#define BISHOP_VALUE            300
#define ROOK_VALUE              500
#define QUEEN_VALUE             900
#define KING_VALUE            40000
#define EG_MAT                   14
  
#if defined(HAS_64BITS)
  typedef unsigned long BITBOARD;
#else
#  if defined(NT_i386) || defined(NT_AXP)
    typedef unsigned __int64 BITBOARD;
#  else
    typedef unsigned long long BITBOARD;
#  endif
#endif

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

typedef enum {think=1, puzzle=2, book=3, annotate=4} SEARCH_TYPE;

typedef enum {normal_mode, tournament_mode} PLAYING_MODE;

typedef enum {crafty, opponent} PLAYER;

typedef enum {book_learning=1, position_learning=2,
              result_learning=4} LEARNING_MODE;
  
typedef struct {
  unsigned char enpassant_target;
  signed   char w_castle;
  signed   char b_castle;
  unsigned char rule_50_moves;
} SEARCH_POSITION;

typedef  struct {
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
  BITBOARD       b_pawn;
  BITBOARD       b_knight;
  BITBOARD       b_bishop;
  BITBOARD       b_rook;
  BITBOARD       b_queen;
  BITBOARD       hash_key;
  unsigned int   pawn_hash_key;
  int            material_evaluation;
  signed char    white_king;
  signed char    black_king;
  signed char    board[64];
  signed char    white_pieces;
  signed char    white_minors;
  signed char    white_majors;
  signed char    white_pawns;
  signed char    black_pieces;
  signed char    black_minors;
  signed char    black_majors;
  signed char    black_pawns;
  signed char    total_pieces;
} POSITION;
  
typedef struct {
  BITBOARD word1;
  BITBOARD word2;
} HASH_ENTRY;

typedef struct {
  unsigned int key;
  short    p_score;
  unsigned char black_protected;
  unsigned char passed_b;
  unsigned char black_defects_k;
  unsigned char black_defects_q;
  unsigned char white_protected;
  unsigned char passed_w;
  unsigned char white_defects_k;
  unsigned char white_defects_q;
  unsigned char outside;
} PAWN_HASH_ENTRY;
  
typedef struct {
  int path[MAXPLY];
  unsigned char path_hashed;
  unsigned char path_length;
  unsigned char path_iteration_depth;
} PATH;
  
typedef struct {
  int phase;
  int remaining;
  int *last;
} NEXT_MOVE;

typedef struct {
  BITBOARD position;
  unsigned int status_played;
  float learn;
} BOOK_POSITION;

typedef struct {
  unsigned char position[8];
  unsigned char status;
  unsigned char percent_play;
} BB_POSITION;

struct tree {
  POSITION        pos;
  PAWN_HASH_ENTRY pawn_score;
  NEXT_MOVE       next_status[MAXPLY];
  BITBOARD        save_hash_key[MAXPLY+2];
  BITBOARD        replist_w[128];
  BITBOARD        replist_b[128];
  BITBOARD        *rephead_w;
  BITBOARD        *rephead_b;
  BITBOARD        all_pawns;
  SEARCH_POSITION position[MAXPLY+2];
  unsigned int    save_pawn_hash_key[MAXPLY+2];
  int             current_move[MAXPLY];
  int             hash_move[MAXPLY];
  int             *last[MAXPLY];
  PATH            pv[MAXPLY];
  unsigned int    nodes_searched;
  unsigned int    fail_high;
  unsigned int    fail_high_first;
  unsigned int    evaluations;
  unsigned int    transposition_probes;
  unsigned int    transposition_hits;
  unsigned int    pawn_probes;
  unsigned int    pawn_hits;
  unsigned int    egtb_probes;
  unsigned int    egtb_probes_successful;
  unsigned int    check_extensions_done;
  unsigned int    recapture_extensions_done;
  unsigned int    passed_pawn_extensions_done;
  unsigned int    one_reply_extensions_done;
  int             killer_move1[MAXPLY];
  int             killer_move2[MAXPLY];
  int             move_list[5120];
  int             sort_value[256];
  unsigned int    root_nodes[256];
  signed char     in_check[MAXPLY];
  signed char     extended_reason[MAXPLY];
  signed char     current_phase[MAXPLY];
  signed char     searched_this_root_move[256];
  int             search_value;
  int             w_safety, b_safety;
  int             w_kingsq, b_kingsq;
  lock_t          lock;
  int             thread_id;
  volatile char   stop;
  volatile char   done;
  struct tree     *volatile siblings[CPUS], *parent;
  volatile int    nprocs;
  int             alpha;
  int             beta;
  int             value;
  int             wtm;
  int             depth;
  int             ply;
  int             threat;
  int             used;
};
  
typedef struct tree TREE;

#if defined(COMPACT_ATTACKS)
#  define NDIAG_ATTACKS   296
#  define NRANK_ATTACKS    70
#  define NFILE_ATTACKS    70

#  define NSHORT_MOBILITY  116

#  define MAX_ATTACKS_FROM_SQUARE 12

  struct at {
    unsigned char which_attack[8][64];
    BITBOARD      file_attack_bitboards[8][MAX_ATTACKS_FROM_SQUARE];
    unsigned char rank_attack_bitboards[8][MAX_ATTACKS_FROM_SQUARE];
    unsigned char length8_mobility[8][MAX_ATTACKS_FROM_SQUARE];
    unsigned char short_mobility[NSHORT_MOBILITY];
  };
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

/*  
    DO NOT modify these.  these are constants, used in multiple modules.
    modification may corrupt the search in any number of ways, all bad.
*/

#define WORTHLESS                 0
#define LOWER                     1
#define UPPER                     2
#define EXACT                     3
#define AVOID_NULL_MOVE           4

#define NULL_MOVE                 0
#define DO_NULL                   1
#define NO_NULL                   0

#define NONE                      0
#define FIRST_PHASE               1
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
 
#if !defined(CRAY1)
  BITBOARD     Mask(int);
  int CDECL    PopCnt(BITBOARD);
  int CDECL    FirstOne(BITBOARD);
  int CDECL    LastOne(BITBOARD);
#endif
  
void           Analyze();
void           Annotate();
int            Attacked(TREE*, int, int);
BITBOARD       AttacksFrom(TREE*, int, int);
BITBOARD       AttacksTo(TREE*, int);
void           Bench(void);
int            Book(TREE*,int,int);
int            BookMask(char*);
void           BookUp(TREE*, char*, int, char**);
void           BookSort(BB_POSITION*, int, int);
#if defined(NT_i386) || defined(NT_AXP)
  int _cdecl     BookUpCompare(const void *, const void *);
#else
  int            BookUpCompare(const void *, const void *);
#endif
BB_POSITION    BookUpNextPosition(int, int);
int            CheckInput(void);
void           ClearHashTables(void);
void           ComputeAttacksAndMobility(void);
void           CopyFromSMP(TREE*, TREE*);
TREE*          CopyToSMP(TREE*);
void           Delay(int);
void           DGTInit(int,char**);
int            DGTCheckInput(void);
void           DGTRead(void);
void           DisplayBitBoard(BITBOARD);
void           DisplayChessBoard(FILE*, POSITION);
char*          DisplayEvaluation(int);
char*          DisplayEvaluationWhisper(int);
void           DisplayFT(int, int, int);
char*          DisplayHHMM(unsigned int);
void           DisplayPieceBoards(int*, int*);
void           DisplayPV(TREE*, int, int, int, int, PATH*);
char*          DisplaySQ(unsigned int);
char*          DisplayTime(unsigned int);
char*          DisplayTimeWhisper(unsigned int);
void           DisplayTreeState(TREE*, int, int, int);
void           Display2BitBoards(BITBOARD, BITBOARD);
void           DisplayChessMove(char*, int);
int            DrawScore(int);
int            Drawn(TREE*, int);
void           Edit(void);
int            EGTBProbe(TREE*, int, int, int*);
int            EnPrise(int, int);
int            Evaluate(TREE*, int, int, int, int);
int            EvaluateDevelopment(TREE*, int);
int            EvaluateDraws(TREE*);
int            EvaluateKingSafety(TREE*, int);
int            EvaluateMate(TREE*);
int            EvaluatePassedPawns(TREE*);
int            EvaluatePassedPawnRaces(TREE*, int);
int            EvaluatePawns(TREE*);
void           EVTest(char *);
int            FindBlockID(TREE*);
char*          FormatPV(TREE*,int,PATH);
void           FTbSetCacheSize(void*,int);
int*           GenerateCaptures(TREE*, int, int, int*);
int*           GenerateCheckEvasions(TREE*, int, int, int*);
int*           GenerateNonCaptures(TREE*, int, int, int*);
int            HashProbe(TREE*, int, int, int, int*, int*, int*);
void           HashStore(TREE*, int, int, int, int, int, int);
void           HashStorePV(TREE*, int,int);
int            HasOpposition(int, int, int);
void           History(TREE*, int, int, int, int);
int            IInitializeTb(char*);
void           Initialize(int);
void           InitializeAttackBoards(void);
void           InitializeChessBoard(SEARCH_POSITION*);
int            InitializeFindAttacks(int, int, int);
void           InitializeHashTables(void);
void           InitializeMasks(void);
void           InitializePawnMasks(void);
void           InitializePieceMasks(void);
void           InitializeRandomHash(void);
void           InitializeSMP(void);
void           InitializeZeroMasks(void);
int            InputMove(TREE*, char*, int, int, int, int);
int            InputMoveICS(TREE*, char*, int, int, int, int);
BITBOARD       InterposeSquares(int, int, int);
void           Interrupt(int);
int            Iterate(int, int, int);
int            KingPawnSquare(int, int, int, int);
void           LearnBook(TREE*, int, int, int, int, int);
void           LearnBookUpdate(TREE*, int, int, float);
int            LearnFunction(int, int, int, int);
void           LearnImport(TREE*, int, char**);
void           LearnImportBook(TREE*, int, char**);
void           LearnImportPosition(TREE*, int, char**);
void           LearnPosition(TREE*, int, int, int);
void           LearnPositionLoad(TREE*);
void           LearnResult(TREE*, int);
int            LegalMove(TREE*, int, int, int);
void           MakeMove(TREE*, int, int, int);
void           MakeMoveRoot(TREE*, int, int);
void           NewGame(int);
int            NextEvasion(TREE*, int, int);
int            NextMove(TREE*, int, int);
int            NextRootMove(TREE*, int);
char*          Normal(void);
int            Option(TREE*);
int            OptionMatch(char*, char*);
void           OptionPerft(TREE*, int, int, int);
char*          OutputMove(TREE*, int, int, int);
char*          OutputMoveICS(TREE*, int);
int            OutputGood(TREE*, char*, int, int);
int            ParseTime(char*);
void           Pass(void);
void           Phase(void);
int            PinnedOnKing(TREE*, int, int);
int            Ponder(int);
void           PreEvaluate(TREE*,int);
void           Print(int, char*, ...);
int            Quiesce(TREE*, int, int, int, int);
unsigned int   Random32(void);
BITBOARD       Random64(void);
int            Read(int, char*);
int            ReadChessMove(TREE*, FILE*, int, int);
void           ReadClear();
unsigned int   ReadClock(TIME_TYPE);
int            ReadPGN(FILE*,int);
int            ReadNextMove(TREE*, char*, int, int);
int            ReadParse(char*, char *args[], char*);
int            ReadInput();
int            RepetitionCheck(TREE*, int, int);
int            RepetitionDraw(TREE*, int);
void           ResignOrDraw(TREE*, int, int);
void           RestoreGame(void);
char*          Reverse(void);
void           RootMoveList(int);
int            Search(TREE*, int, int, int, int, int, int);
void           SearchOutput(TREE*, int, int);
int            SearchRoot(TREE*, int, int, int, int);
int            SearchSMP(TREE*, int, int, int, int, int, int, int);
void           SearchTrace(TREE*, int, int, int, int, int, char*, int);
void           SetBoard(int,char**,int);
void           SetChessBitBoards(SEARCH_POSITION*);
int            Swap(TREE*, int, int, int);
BITBOARD       SwapXray(TREE*, BITBOARD, int, int);
void           Test(char *);
int            Thread(TREE*);
void*          ThreadInit(void*);
void           ThreadStop(TREE*);
int            ThreadWait(int, TREE*);
void           TimeAdjust(int,PLAYER);
int            TimeCheck(int);
void           TimeSet(int);
void           UnMakeMove(TREE*, int, int, int);
int            ValidMove(TREE*, int, int, int);
void           ValidatePosition(TREE*, int, int, char*);
BITBOARD       ValidateComputeBishopAttacks(TREE*, int);
BITBOARD       ValidateComputeRookAttacks(TREE*, int);
void           Whisper(int, int, int, int, unsigned int, int, int, int, char*);
  
#if defined(HAS_64BITS) || defined(HAS_LONGLONG)
#  define And(a,b)    ((a) & (b))
#  define Or(a,b)     ((a) | (b))
#  define Xor(a,b)    ((a) ^ (b))
#  define Compl(a)    (~(a))
#  define Shiftl(a,b) ((a) << (b))
#  define Shiftr(a,b) ((a) >> (b))
#  if defined(CRAY1)
#    define PopCnt(a)     _popcnt(a)
#    define FirstOne(a)   _leadz(a)
#    define LastOne(a)    _leadz(a&~((a)&(a-1)))
#    define Mask(a)       _mask(a)
#    define mask_1        _mask(1)
#    define mask_2        _mask(2)
#    define mask_3        _mask(3)
#    define mask_4        _mask(4)
#    define mask_8        _mask(8)
#    define mask_16       _mask(16)
#    define mask_32       _mask(32)
#    define mask_72       _mask(72)
#    define mask_80       _mask(80)
#    define mask_85       _mask(85)
#    define mask_96       _mask(96)
#    define mask_107      _mask(107)
#    define mask_108      _mask(108)
#    define mask_112      _mask(112)
#    define mask_118      _mask(118)
#    define mask_120      _mask(120)
#    define mask_121      _mask(121)
#    define mask_127      _mask(127)
#  endif
#endif

#define ABSearch(tree,alpha,beta,wtm,depth,ply,donull)        \
        (((depth) >= INCREMENT_PLY) ?                         \
        Search(tree,alpha,beta,wtm,depth,ply,donull) :        \
        Quiesce(tree,alpha,beta,wtm,ply))

#define Max(a,b)  (((a) > (b)) ? (a) : (b))
#define Min(a,b)  (((a) < (b)) ? (a) : (b))
#define FileDistance(a,b) abs(((a)&7) - ((b)&7))
#define RankDistance(a,b) abs(((a)>>3) - ((b)>>3))
#define Distance(a,b) Max(FileDistance(a,b),RankDistance(a,b))
#define KingSafety(s,p)                ((s)*scale_down[p])
#define ScaleDown(s,m)                 ((s)*scale_down[m]/12)
#define ScaleUp(s,m)                   ((s)*scale_up[m]/12)

/*  
    the following macro is used to determine if one side is in check.  it
    simply returns the result of Attacked().
*/
#define Check(wtm)                                                     \
  Attacked(tree, (wtm)?WhiteKingSQ:BlackKingSQ,ChangeSide(wtm))
/*  
    Attack() is used to determine if a newly promoted pawn (queen)
    attacks <square>.  normally <square> will be the location of the opposing
    king, but it can also be the location of the opposing side's queening
    square in case this pawn prevents the other pawn from safely queening on
    the next move.
*/
#define Attack(square,queen,ply) !And(obstructed[square][queen],Occupied)
/*  
    the following macros are used to construct the attacks from a square.
    the attacks are computed as four separate bit vectors, one for each of the
    two diagonals, and one for the ranks and one for the files.  these can be
    Or'ed together to produce the attack bitmaps for bishops, rooks and queens.
*/
#if defined(COMPACT_ATTACKS) && defined(USE_ATTACK_FUNCTIONS)
  extern BITBOARD CDECL AttacksRookFunc(int, POSITION *);
  extern BITBOARD CDECL AttacksBishopFunc(DIAG_INFO *, POSITION *);
#  define AttacksRook(a) AttacksRookFunc(a,&tree->pos)
#  define AttacksBishop(a) AttacksBishopFunc(&diag_info[a],&tree->pos)
#else
#  define AttacksRook(a)    Or(AttacksRank(a),AttacksFile(a))
#  define AttacksBishop(a)  Or(AttacksDiaga1(a),AttacksDiagh1(a))
#endif

#define AttacksQueen(a)   Or(AttacksBishop(a),AttacksRook(a))
#define Rank(x)       (((x)>>3)&7)
#define File(x)       ((x)&7)
#define ChangeSide(x) ((x)^1)

#if defined(COMPACT_ATTACKS)
/*
  On a 32 bit machine optimizes the right shift of a long long
  where it is known that desired piece lies completely in one of
  the 32 bit words.
*/
#  if defined(USE_SPLIT_SHIFTS)
#    define SplitShiftr(value,shift) ((shift) >= 32 ?             \
       Shiftr ((unsigned long) Shiftr((value), 32), (shift)-32) : \
       Shiftr ((unsigned long) (value), (shift)))
#  else
#    define SplitShiftr(value,shift) Shiftr(value,shift)
#  endif

#  define AttacksDiaga1Int(diagp,boardp) \
     (diagp)->ad_attacks[(diagp)->ad_which_attack[                \
      And (SplitShiftr((boardp)->occupied_rl45,(diagp)->ad_shift),\
      (diagp)->ad_mask)]]

#  define AttacksDiagh1Int(diagp,boardp)                          \
     (diagp)->d_attacks[(diagp)->d_which_attack[                  \
      And (SplitShiftr((boardp)->occupied_rr45,(diagp)->d_shift), \
      (diagp)->d_mask)]]

/*
  On a 32 bit machine optimizes promoting a smaller value to a long
  long where it is known that the smaller piece will be completely
  in one of the 32 bit words.
*/
#  if defined(USE_SPLIT_SHIFTS)
#    define SplitShiftl(value,shift)                              \
       ((shift) >= 32 ? Shiftl((BITBOARD)                         \
        Shiftl((unsigned long) (value), (shift)-32), 32) :        \
       (BITBOARD) Shiftl((unsigned long) (value), (shift)))
#  else
#    define SplitShiftl(value,shift) Shiftl((BITBOARD) value,shift)
#  endif

/*
  The length of the following macro is a little excessive as the
  rank_attacks lookup is duplicated.  Making it a function, or passing
  in a temporary variable would simplify it significantly, although
  hopefully the compiler recognizes the common subexpression.
*/
#  define AttacksRankInt(a,boardp)                          \
     SplitShiftl(at.rank_attack_bitboards[File(a)][         \
     at.which_attack[File(a)][And(SplitShiftr(              \
     Or((boardp)->w_occupied,(boardp)->b_occupied),         \
     (Rank(~(a))<<3)+1),0x3f)]],Rank(~(a))<<3)

/* 
  The final left shift in this is optimizable, but the optimization is
  a little ugly to express.  There is no information that crosses the
  word boundary in the shift so it can be implemented as two separate
  word shifts that are joined together in a long long.
*/
#  define AttacksFileInt(a,boardp)                          \
     Shiftl(at.file_attack_bitboards[Rank(a)] [             \
     at.which_attack[Rank(a)] [                             \
     And(SplitShiftr((boardp)->occupied_rl90,               \
     (File(~(a))<<3)+1),0x3f)]],File(~(a)) )

#  if defined(USE_ATTACK_FUNCTIONS)
    extern BITBOARD CDECL AttacksRankFunc(int, POSITION *);
    extern BITBOARD CDECL AttacksFileFunc(int, POSITION *);
    extern BITBOARD CDECL AttacksDiaga1Func(DIAG_INFO *, POSITION *);
    extern BITBOARD CDECL AttacksDiagh1Func(DIAG_INFO *, POSITION *);

#    define AttacksRank(a) AttacksRankFunc(a,&tree->pos)
#    define AttacksFile(a) AttacksFileFunc(a,&tree->pos)
#    define AttacksDiaga1(a) AttacksDiaga1Func(&diag_info[a],&tree->pos)
#    define AttacksDiagh1(a) AttacksDiagh1Func(&diag_info[a],&tree->pos)
#  else
#    define AttacksRank(a) AttacksRankInt(a,&tree->pos)
#    define AttacksFile(a) AttacksFileInt(a,&tree->pos)
#    define AttacksDiaga1(a) AttacksDiaga1Int(&diag_info[a],&tree->pos)
#    define AttacksDiagh1(a) AttacksDiagh1Int(&diag_info[a],&tree->pos)
#  endif

#else

#  define AttacksRank(a)                                               \
      rook_attacks_r0[(a)][And(Shiftr(Or(tree->pos.w_occupied,         \
                                         tree->pos.b_occupied),        \
                                      56-((a)&56)),255)]
#  define AttacksFile(a)                                               \
      rook_attacks_rl90[(a)][And(Shiftr(tree->pos.occupied_rl90,       \
                                        56-(((a)&7)<<3)),255)]
#  define AttacksDiaga1(a)                                             \
      bishop_attacks_rl45[(a)][And(Shiftr(tree->pos.occupied_rl45,     \
                                          bishop_shift_rl45[(a)]),255)]
#  define AttacksDiagh1(a)                                             \
      bishop_attacks_rr45[(a)][And(Shiftr(tree->pos.occupied_rr45,     \
                                          bishop_shift_rr45[(a)]),255)]
#endif /* defined(COMPACT_ATTACKS) */
/*  
    the following macros are used to compute the mobility for a sliding piece.
    The basic idea is the same as the attack vectors above, but the result is 
    an integer mobility factor rather than a bitboard.  this saves having to 
    do a PopCnt() on the attack bit vector, which is much slower.
*/
#define MobilityRook(a)   (MobilityRank(a)+MobilityFile(a))

#define MobilityBishop(a) (MobilityDiaga1(a)+MobilityDiagh1(a))

#define MobilityQueen(a)  (MobilityBishop(a)+MobilityRook(a))

#if defined(COMPACT_ATTACKS)

#  define MobilityDiaga1Int(diagp,boardp)                                   \
     (diagp)->ad_mobility[(diagp)->ad_which_attack[                         \
     And (SplitShiftr ((boardp)->occupied_rl45,                             \
     (diagp)->ad_shift),(diagp)->ad_mask)]]

#  define MobilityDiagh1Int(diagp,boardp)                                   \
     (diagp)->d_mobility[(diagp)->d_which_attack [                          \
     And (SplitShiftr ((boardp)->occupied_rr45,                             \
     (diagp)->d_shift),(diagp)->d_mask)]]

#  define MobilityRankInt(a,boardp)                                         \
     at.length8_mobility[File(a)][                                          \
     at.which_attack[File(a)][                                              \
     And(SplitShiftr(Or((boardp)->w_occupied,                               \
     (boardp)->b_occupied),(Rank(~(a))<<3)+1),0x3f)]]

#  define MobilityFileInt(a,boardp)                                         \
     at.length8_mobility[Rank(a)][                                          \
     at.which_attack[Rank(a)] [                                             \
     And(SplitShiftr((boardp)->occupied_rl90,(File(~(a))<<3)+1),0x3f)]]

#  if defined(USE_ATTACK_FUNCTIONS)
    extern unsigned CDECL MobilityRankFunc(int, POSITION *);
    extern unsigned CDECL MobilityFileFunc(int, POSITION *);
    extern unsigned CDECL MobilityDiaga1Func(DIAG_INFO *, POSITION *);
    extern unsigned CDECL MobilityDiagh1Func(DIAG_INFO *, POSITION *);

#    define MobilityRank(a) MobilityRankFunc(a,&tree->pos)
#    define MobilityFile(a) MobilityFileFunc(a,&tree->pos)
#    define MobilityDiaga1(a) MobilityDiaga1Func(&diag_info[a],&tree->pos)
#    define MobilityDiagh1(a) MobilityDiagh1Func(&diag_info[a],&tree->pos)
#  else
#    define MobilityRank(a) MobilityRankInt(a,&tree->pos)
#    define MobilityFile(a) MobilityFileInt(a,&tree->pos)
#    define MobilityDiaga1(a) MobilityDiaga1Int(&diag_info[a],&tree->pos)
#    define MobilityDiagh1(a) MobilityDiagh1Int(&diag_info[a],&tree->pos)
#  endif

#else

#  define MobilityRank(a)                                                   \
     rook_mobility_r0[(a)][And(Shiftr(Or(tree->pos.w_occupied,              \
                                         tree->pos.b_occupied),             \
                                      56-((a)&56)),255)]
#  define MobilityFile(a)                                                   \
     rook_mobility_rl90[(a)][And(Shiftr(tree->pos.occupied_rl90,            \
                                        56-(((a)&7)<<3)),255)]
#  define MobilityDiaga1(a)                                                 \
     bishop_mobility_rl45[(a)][And(Shiftr(tree->pos.occupied_rl45,          \
                                          bishop_shift_rl45[(a)]),255)]
#  define MobilityDiagh1(a)                                                 \
     bishop_mobility_rr45[(a)][And(Shiftr(tree->pos.occupied_rr45,          \
                                          bishop_shift_rr45[(a)]),255)]
#endif  /* if defined(COMPACT_ATTACKS) */

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
#define BlackMinors           (tree->pos.black_minors)
#define BlackMajors           (tree->pos.black_majors)

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
#define WhiteMinors           (tree->pos.white_minors)
#define WhiteMajors           (tree->pos.white_majors)

#define TotalPieces           (tree->pos.total_pieces)

#define Material              (tree->pos.material_evaluation)
#define Rule50Moves(ply)      (tree->position[ply].rule_50_moves)
#define HashKey               (tree->pos.hash_key)
#define PawnHashKey           (tree->pos.pawn_hash_key)
#define EnPassant(ply)        (tree->position[ply].enpassant_target)
#define EnPassantTarget(ply)  (EnPassant(ply) ? SetMask(EnPassant(ply)) : 0)
#define PieceOnSquare(sq)     (tree->pos.board[sq])
#define BishopsQueens         (tree->pos.bishops_queens)
#define RooksQueens           (tree->pos.rooks_queens)
#define Occupied              (Or(tree->pos.w_occupied,tree->pos.b_occupied))
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
#define ClearSet(a,b)       b=Xor(a,b)
#define Clear(a,b)          b=And(ClearMask(a),b)
#define ClearRL90(a,b)      b=And(ClearMaskRL90(a),b)
#define ClearRL45(a,b)      b=And(ClearMaskRL45(a),b)
#define ClearRR45(a,b)      b=And(ClearMaskRR45(a),b)
#define Set(a,b)            b=Or(SetMask(a),b)
#define SetRL90(a,b)        b=Or(SetMaskRL90(a),b)
#define SetRL45(a,b)        b=Or(SetMaskRL45(a),b)
#define SetRR45(a,b)        b=Or(SetMaskRR45(a),b)

#define HashPB32(a,b)       b=b_pawn_random32[a]^(b)
#define HashPW32(a,b)       b=w_pawn_random32[a]^(b)
#define HashPB(a,b)         b=Xor(b_pawn_random[a],b)
#define HashPW(a,b)         b=Xor(w_pawn_random[a],b)
#define HashNB(a,b)         b=Xor(b_knight_random[a],b)
#define HashNW(a,b)         b=Xor(w_knight_random[a],b)
#define HashBB(a,b)         b=Xor(b_bishop_random[a],b)
#define HashBW(a,b)         b=Xor(w_bishop_random[a],b)
#define HashRB(a,b)         b=Xor(b_rook_random[a],b)
#define HashRW(a,b)         b=Xor(w_rook_random[a],b)
#define HashQB(a,b)         b=Xor(b_queen_random[a],b)
#define HashQW(a,b)         b=Xor(w_queen_random[a],b)
#define HashKB(a,b)         b=Xor(b_king_random[a],b)
#define HashKW(a,b)         b=Xor(w_king_random[a],b)
#define HashEP(a,b)         b=Xor(enpassant_random[a],b)
#define HashCastleW(a,b)    b=Xor(castle_random_w[a],b);
#define HashCastleB(a,b)    b=Xor(castle_random_b[a],b);
#define SavePV(tree,ply,value,ph) do {                                      \
          tree->pv[ply-1].path[ply-1]=tree->current_move[ply-1];            \
          tree->pv[ply-1].path_length=ply-1;                                \
          tree->pv[ply-1].path_hashed=ph;                                   \
          tree->pv[ply-1].path_iteration_depth=iteration_depth;} while(0)
#define SavePVS(tree,ply,value,ph) do {                                     \
          tree->pv[ply-1].path[ply-1]=tree->current_move[ply-1];            \
          tree->pv[ply-1].path_length=ply-1;                                \
          tree->pv[ply-1].path_hashed=ph;                                   \
          tree->pv[ply-1].path_iteration_depth=iteration_depth;             \
          SearchOutput(tree,value,beta);} while(0)

/*
  Service macro - initialize squares of the particular piece as well as
  counter for that piece. Note: dual initialization saves some time when
  TB is present, but waste for non-present TB. If we often will call
  probing function for an absent TB, maybe we shall split that code.
*/

#define  VInitSqCtr(rgCtr, rgSquares, piece, bitboard) {\
  int  cPieces = 0;\
  BITBOARD bbTemp = (bitboard);\
  while (bbTemp) {\
    square sq = FirstOne (bbTemp);\
    (rgSquares)[(piece)*C_PIECES+cPieces] = sq;\
    cPieces ++;\
    Clear (sq, bbTemp);\
  }\
  (rgCtr)[(piece)] = cPieces;\
}


#endif /* if defined(TYPES_INCLUDED) */
