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

/* Provide reasonable defaults for UNIX systems. */
#undef  HAS_64BITS             /* machine has 64-bit integers / operators     */
#define HAS_LONGLONG           /* machine has 32-bit/64-bit integers          */
#define UNIX                   /* system is unix-based                        */

/* Architecture-specific definitions */
#if defined(AIX)
#  undef  HAS_64BITS           /* machine has 64-bit integers / operators     */
#  define HAS_LONGLONG         /* machine has 32-bit/64-bit integers          */
#  define UNIX                 /* system is unix-based                        */
#endif
#if defined(ALPHA)
#  define HAS_64BITS           /* machine has 64-bit integers / operators     */
#  undef  HAS_LONGLONG         /* machine has 32-bit/64-bit integers          */
#  define UNIX                 /* system is unix-based                        */
#endif
#if defined(AMIGA)
#  undef  HAS_64BITS           /* machine has 64-bit integers / operators     */
#  define HAS_LONGLONG         /* machine has 32-bit/64-bit integers          */
#  undef  UNIX                 /* system is unix-based                        */
#endif
#if defined(CRAY1)
#  define HAS_64BITS           /* machine has 64-bit integers / operators     */
#  undef  HAS_LONGLONG         /* machine has 32-bit/64-bit integers          */
#  define UNIX                 /* system is unix-based                        */
#endif
#if defined(DOS)
#  undef  HAS_64BITS           /* machine has 64-bit integers / operators     */
#  define HAS_LONGLONG         /* machine has 32-bit/64-bit integers          */
#  undef  UNIX                 /* system is unix-based                        */
#endif
#if defined(FreeBSD)
#  undef  HAS_64BITS           /* machine has 64-bit integers / operators     */
#  define HAS_LONGLONG         /* machine has 32-bit/64-bit integers          */
#  define UNIX                 /* system is unix-based                        */
#endif
#if defined(HP)
#  undef  HAS_64BITS           /* machine has 64-bit integers / operators     */
#  define HAS_LONGLONG         /* machine has 32-bit/64-bit integers          */
#  define UNIX                 /* system is unix-based                        */
#endif
#if defined(LINUX)
#  undef  HAS_64BITS           /* machine has 64-bit integers / operators     */
#  define HAS_LONGLONG         /* machine has 32-bit/64-bit integers          */
#  define UNIX                 /* system is unix-based                        */
#endif
#if defined(MIPS)
#  undef  HAS_64BITS           /* machine has 64-bit integers / operators     */
#  define HAS_LONGLONG         /* machine has 32-bit/64-bit integers          */
#  define UNIX                 /* system is unix-based                        */
#endif
#if defined(NetBSD)
#  if defined(__alpha__)
#    define HAS_64BITS         /* machine has 64-bit integers / operators     */
#    undef  HAS_LONGLONG       /* machine has 32-bit/64-bit integers          */
#    define UNIX               /* system is unix-based                        */
#  else
#    undef  HAS_64BITS         /* machine has 64-bit integers / operators     */
#    define HAS_LONGLONG       /* machine has 32-bit/64-bit integers          */
#    define UNIX               /* system is unix-based                        */
#  endif
#endif
#if defined(NEXT)
#  undef  HAS_64BITS          /* machine has 64-bit integers / operators     */
#  define HAS_LONGLONG        /* machine has 32-bit/64-bit integers          */
#  define UNIX                /* system is unix-based                        */
#endif
#if defined(NT_AXP)
#  undef  HAS_64BITS           /* machine has 64-bit integers / operators     */
#  define HAS_LONGLONG         /* machine has 32-bit/64-bit integers          */
#  undef  UNIX                 /* system is unix-based                        */
#endif
#if defined(NT_i386)
#  undef  HAS_64BITS           /* machine has 64-bit integers / operators     */
#  define HAS_LONGLONG         /* machine has 32-bit/64-bit integers          */
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
#  define UNIX                 /* system is unix-based                        */
#endif
#if defined(SGI)
#  undef  HAS_64BITS           /* machine has 64-bit integers / operators     */
#  define HAS_LONGLONG         /* machine has 32-bit/64-bit integers          */
#  define UNIX                 /* system is unix-based                        */
#endif
#if defined(SUN)
#  undef  HAS_64BITS           /* machine has 64-bit integers / operators     */
#  define HAS_LONGLONG         /* machine has 32-bit/64-bit integers          */
#  define UNIX                 /* system is unix-based                        */
#endif
#if defined(SUN_BSD)
#  undef  HAS_64BITS           /* machine has 64-bit integers / operators     */
#  define HAS_LONGLONG         /* machine has 32-bit/64-bit integers          */
#  define UNIX                 /* system is unix-based                        */
#endif

#if defined(__MWERKS__)
#  define MACOS
#endif

#if defined(MACOS)
#  undef  HAS_64BITS           /* machine has 64-bit integers / operators     */
#  define HAS_LONGLONG         /* machine has 32-bit/64-bit integers          */
#  undef  UNIX                 /* system is unix-based                        */

#  define COMPACT_ATTACKS
#  define USE_ATTACK_FUNCTIONS  
#endif

#if defined(MACOS)
#  if !defined(BOOKDIR)
#    define     BOOKDIR    "Books"
#  endif
#  if !defined(LOGDIR)
#    define      LOGDIR     "Logs"
#  endif
#  if !defined(TBDIR)
#    define       TBDIR       "TB"
#  endif
#  if !defined(RCDIR)
#    define       RCDIR        "."
#  endif
#else
#  if !defined(BOOKDIR)
#    define     BOOKDIR        "."
#  endif
#  if !defined(LOGDIR)
#    define      LOGDIR        "."
#  endif
#  if !defined(TBDIR)
#    define       TBDIR     "./TB"
#  endif
#  if !defined(RCDIR)
#    define       RCDIR        "."
#  endif
#endif

# define    EGTB_CACHE_DEFAULT 1024*1024
#define     MAXPLY            65
#define MAX_BLOCKS       16*CPUS
#define MAX_TC_NODES      300000

#if !defined(SMP) && !defined(SUN)
#  define lock_t int
#endif
#include "lock.h"

#define      BOOK_CLUSTER_SIZE            600
#define            MERGE_BLOCK           1000
#define             SORT_BLOCK        4000000
#define         LEARN_INTERVAL             10
#define        LEARN_WINDOW_LB            -40
#define        LEARN_WINDOW_UB            +40
#define      LEARN_COUNTER_BAD            -80
#define     LEARN_COUNTER_GOOD           +100
#define         CAP_SCORE_GOOD           +150
#define          CAP_SCORE_BAD           -100

/*
  fractional ply extensions.  these should be in units based on the
  value of INCPLY (default is 60).  a value of 60 means this
  extension is exactly one ply.
*/

#define INCPLY                   60  /* 1.00 */

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

typedef enum {none=0, pawn=1, knight=2, king=3, 
              bishop=5, rook=6, queen=7} PIECE;
  
typedef enum {empty_v=0, pawn_v=1, knight_v=3, 
              bishop_v=3, rook_v=5, queen_v=9} PIECE_V;
  
typedef enum {no_extension=0, check_extension=1, recapture_extension=2,
              passed_pawn_extension=4, one_reply_extension=8,
              mate_extension=16} EXTENSIONS;
  
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
  
typedef struct {
  int             move1;
  int             move2;
} KILLER;

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
  unsigned char passed_b;
  unsigned char black_defects_k;
  unsigned char black_defects_q;
  unsigned char passed_w;
  unsigned char white_defects_k;
  unsigned char white_defects_q;
  unsigned char protected;
  unsigned char outside;
  unsigned char candidates_w;
  unsigned char candidates_b;
  unsigned char allw;
  unsigned char allb;
} PAWN_HASH_ENTRY;

typedef struct {
  int path[MAXPLY];
  unsigned char pathh;
  unsigned char pathl;
  unsigned char pathd;
} PATH;
  
typedef struct {
  int phase;
  int remaining;
  int *last;
} NEXT_MOVE;
  
typedef struct {
  BITBOARD nodes;
  int move;
/* 
  xxxx xxx1 = failed low
  xxxx xx1x = failed high
  1xxx xxxx = done (searched)
  x1xx xxxx = don't search in parallel
*/
  unsigned char status;
} ROOT_MOVE;

#if (defined(NT_i386) || defined(NT_AXP))
#pragma pack(4)
#endif
typedef struct {
  BITBOARD position;
  unsigned int status_played;
  float learn;
  int CAP_score;
} BOOK_POSITION;
#if (defined(NT_i386) || defined(NT_AXP))
#pragma pack()
#endif

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
  unsigned int    mate_extensions_done;
  KILLER          killers[MAXPLY];
  int             move_list[5120];
  int             sort_value[256];
  signed char     in_check[MAXPLY];
  signed char     extended_reason[MAXPLY];
  signed char     phase[MAXPLY];
  int             search_value;
  int             w_safety, b_safety;
  int             w_kingsq, b_kingsq;
  int             endgame;
  int             root_move;
  lock_t          lock;
  int             thread_id;
  volatile char   stop;
  volatile char   done;
  char            root_move_text[16];
  char            remaining_moves_text[16];
  struct tree     *volatile siblings[CPUS], *parent;
  volatile int    nprocs;
  int             alpha;
  int             beta;
  int             value;
  int             wtm;
  int             depth;
  int             ply;
  int             mate_threat;
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
#define FAIL_LOW_POS              5

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
 
#if !defined(CRAY1)
  BITBOARD     Mask(int);
#  if defined(VC_INLINE_ASM)
#    include "vcinline.h"
#  else
     int CDECL PopCnt(BITBOARD);
     int CDECL FirstOne(BITBOARD);
     int CDECL LastOne(BITBOARD);
#  endif
#endif
  
void           Analyze();
void           Annotate();
void           AnnotateHeaderHTML(char*, FILE*);
void           AnnotateFooterHTML(FILE*);
void           AnnotatePositionHTML(TREE*, int, FILE*);
char           *AnnotateValueToNAG(int, int, int);
int            Attacked(TREE*, int, int);
BITBOARD       AttacksFrom(TREE*, int, int);
BITBOARD       AttacksTo(TREE*, int);
void           Bench(void);
int            Book(TREE*,int,int);
int            BookMask(char*);
int            BookPonderMove(TREE*,int);
int            BookRejectMove(TREE*,int);
void           BookUp(TREE*, char*, int, char**);
void           BookSort(BB_POSITION*, int, int);
#if defined(NT_i386) || defined(NT_AXP)
  int _cdecl     BookUpCompare(const void *, const void *);
#else
  int            BookUpCompare(const void *, const void *);
#endif
BB_POSITION    BookUpNextPosition(int, int);
int            CheckInput(void);
void           ClearHashTableScores(void);
void           ComputeAttacksAndMobility(void);
void           CopyFromSMP(TREE*, TREE*);
TREE*          CopyToSMP(TREE*);
void           DelayTime(int);
void           DGTInit(int,char**);
int            DGTCheckInput(void);
void           DGTRead(void);
void           DisplayBitBoard(BITBOARD);
void           DisplayChessBoard(FILE*, POSITION);
char*          DisplayEvaluation(int,int);
char*          DisplayEvaluationWhisper(int,int);
void           DisplayFT(int, int, int);
char*          DisplayHHMM(unsigned int);
void           DisplayPieceBoards(signed char*, signed char*);
void           DisplayPV(TREE*, int, int, int, int, PATH*);
char*          DisplaySQ(unsigned int);
char*          DisplayTime(unsigned int);
char*          DisplayTimeWhisper(unsigned int);
void           DisplayTreeState(TREE*, int, int, int);
void           Display2BitBoards(BITBOARD, BITBOARD);
void           DisplayChessMove(char*, int);
int            Drawn(TREE*, int);
void           Edit(void);
int            EGTBProbe(TREE*, int, int, int*);
void           EGTBPV(TREE*, int);
int            EnPrise(int, int);
int            Evaluate(TREE*, int, int, int, int);
int            EvaluateDevelopmentB(TREE*, int);
int            EvaluateDevelopmentW(TREE*, int);
int            EvaluateDraws(TREE*);
int            EvaluateKingSafety(TREE*, int);
int            EvaluateMate(TREE*);
int            EvaluateMaterial(TREE*);
int            EvaluatePassedPawns(TREE*);
int            EvaluatePassedPawnRaces(TREE*, int);
int            EvaluatePawns(TREE*);
void           EVTest(char *);
int            FindBlockID(TREE*);
char*          FormatPV(TREE*,int,PATH);
int            FTbSetCacheSize(void*,unsigned long);
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
void           InitializeHistoryKillers(void);
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
void           LearnImportCAP(TREE*, int, char**);
void           LearnImportPosition(TREE*, int, char**);
void           LearnPosition(TREE*, int, int, int);
void           LearnPositionLoad(void);
int            LegalMove(TREE*, int, int, int);
void           MakeMove(TREE*, int, int, int);
void           MakeMoveRoot(TREE*, int, int);
void           NewGame(int);
int            NextEvasion(TREE*, int, int);
int            NextMove(TREE*, int, int);
int            NextRootMove(TREE*, int, char*);
int            NextRootMoveParallel(void);
char*          Normal(void);
int            Option(TREE*);
int            OptionMatch(char*, char*);
void           OptionPerft(TREE*, int, int, int);
char*          OutputMove(TREE*, int, int, int);
char*          OutputMoveICS(int);
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
int            RepetitionCheckBook(TREE*, int, int);
int            RepetitionDraw(TREE*, int);
void           ResignOrDraw(TREE*, int);
void           RestoreGame(void);
char*          Reverse(void);
void           RootMoveList(int);
int            Search(TREE*, int, int, int, int, int, int);
void           SearchOutput(TREE*, int, int);
int            SearchRoot(TREE*, int, int, int, int);
int            SearchSMP(TREE*, int, int, int, int, int, int, int);
void           SearchTrace(TREE*, int, int, int, int, int, char*, int);
void           SetBoard(SEARCH_POSITION*,int,char**,int);
void           SetChessBitBoards(SEARCH_POSITION*);
int            StrCnt(char*, char);
int            Swap(TREE*, int, int, int);
BITBOARD       SwapXray(TREE*, BITBOARD, int, int);
void           Test(char *);
int            Thread(TREE*);
void* STDCALL  ThreadInit(void*);
void           ThreadStop(TREE*);
int            ThreadWait(int, TREE*);
int            Threat(TREE*, int, int, int, int, int, int);
void           TimeAdjust(int,PLAYER);
int            TimeCheck(TREE*,int);
void           TimeSet(int);
void           UnMakeMove(TREE*, int, int, int);
int            ValidMove(TREE*, int, int, int);
void           ValidatePosition(TREE*, int, int, char*);
void           Whisper(int, int, int, int, int, unsigned int, int, int, char*);
  
#if defined(HAS_64BITS) || defined(HAS_LONGLONG)
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
#  if defined(ALPHA)
#    include <machine/builtins.h>
#    define MaskL(a)      (((unsigned long)(-1L))<<(a))
#    define MaskR(a)      (((unsigned long)(-1L))>>(a))

#    define mask_0        0
#    define mask_1        MaskL(63)
#    define mask_2        MaskL(62)
#    define mask_3        MaskL(61)
#    define mask_4        MaskL(60)
#    define mask_5        MaskL(59)
#    define mask_6        MaskL(58)
#    define mask_7        MaskL(57)
#    define mask_8        MaskL(56)
#    define mask_9        MaskL(55)
#    define mask_10       MaskL(54)
#    define mask_11       MaskL(53)
#    define mask_12       MaskL(52)
#    define mask_13       MaskL(51)
#    define mask_14       MaskL(50)
#    define mask_15       MaskL(49)
#    define mask_16       MaskL(48)
#    define mask_17       MaskL(47)
#    define mask_18       MaskL(46)
#    define mask_19       MaskL(45)
#    define mask_20       MaskL(44)
#    define mask_21       MaskL(43)
#    define mask_22       MaskL(42)
#    define mask_23       MaskL(41)
#    define mask_24       MaskL(40)
#    define mask_25       MaskL(39)
#    define mask_26       MaskL(38)
#    define mask_27       MaskL(37)
#    define mask_28       MaskL(36)
#    define mask_29       MaskL(35)
#    define mask_30       MaskL(34)
#    define mask_31       MaskL(33)
#    define mask_32       MaskL(32)
#    define mask_33       MaskL(31)
#    define mask_34       MaskL(30)
#    define mask_35       MaskL(29)
#    define mask_36       MaskL(28)
#    define mask_37       MaskL(27)
#    define mask_38       MaskL(26)
#    define mask_39       MaskL(25)
#    define mask_40       MaskL(24)
#    define mask_41       MaskL(23)
#    define mask_42       MaskL(22)
#    define mask_43       MaskL(21)
#    define mask_44       MaskL(20)
#    define mask_45       MaskL(19)
#    define mask_46       MaskL(18)
#    define mask_47       MaskL(17)
#    define mask_48       MaskL(16)
#    define mask_49       MaskL(15)
#    define mask_50       MaskL(14)
#    define mask_51       MaskL(13)
#    define mask_52       MaskL(12)
#    define mask_53       MaskL(11)
#    define mask_54       MaskL(10)
#    define mask_55       MaskL(9)
#    define mask_56       MaskL(8)
#    define mask_57       MaskL(7)
#    define mask_58       MaskL(6)
#    define mask_59       MaskL(5)
#    define mask_60       MaskL(4)
#    define mask_61       MaskL(3)
#    define mask_62       MaskL(2)
#    define mask_63       MaskL(1)
#    define mask_64       MaskR(0)
#    define mask_65       MaskR(1)
#    define mask_66       MaskR(2)
#    define mask_67       MaskR(3)
#    define mask_68       MaskR(4)
#    define mask_69       MaskR(5)
#    define mask_70       MaskR(6)
#    define mask_71       MaskR(7)
#    define mask_72       MaskR(8)
#    define mask_73       MaskR(9)
#    define mask_74       MaskR(10)
#    define mask_75       MaskR(11)
#    define mask_76       MaskR(12)
#    define mask_77       MaskR(13)
#    define mask_78       MaskR(14)
#    define mask_79       MaskR(15)
#    define mask_80       MaskR(16)
#    define mask_81       MaskR(17)
#    define mask_82       MaskR(18)
#    define mask_83       MaskR(19)
#    define mask_84       MaskR(20)
#    define mask_85       MaskR(21)
#    define mask_86       MaskR(22)
#    define mask_87       MaskR(23)
#    define mask_88       MaskR(24)
#    define mask_89       MaskR(25)
#    define mask_90       MaskR(26)
#    define mask_91       MaskR(27)
#    define mask_92       MaskR(28)
#    define mask_93       MaskR(29)
#    define mask_94       MaskR(30)
#    define mask_95       MaskR(31)
#    define mask_96       MaskR(32)
#    define mask_97       MaskR(33)
#    define mask_98       MaskR(34)
#    define mask_99       MaskR(35)
#    define mask_100      MaskR(36)
#    define mask_101      MaskR(37)
#    define mask_102      MaskR(38)
#    define mask_103      MaskR(39)
#    define mask_104      MaskR(40)
#    define mask_105      MaskR(41)
#    define mask_106      MaskR(42)
#    define mask_107      MaskR(43)
#    define mask_108      MaskR(44)
#    define mask_109      MaskR(45)
#    define mask_110      MaskR(46)
#    define mask_111      MaskR(47)
#    define mask_112      MaskR(48)
#    define mask_113      MaskR(49)
#    define mask_114      MaskR(50)
#    define mask_115      MaskR(51)
#    define mask_116      MaskR(52)
#    define mask_117      MaskR(53)
#    define mask_118      MaskR(54)
#    define mask_119      MaskR(55)
#    define mask_120      MaskR(56)
#    define mask_121      MaskR(57)
#    define mask_122      MaskR(58)
#    define mask_123      MaskR(59)
#    define mask_124      MaskR(60)
#    define mask_125      MaskR(61)
#    define mask_126      MaskR(62)
#    define mask_127      MaskR(63)
#    define mask_128      0
/* The following are defined only on Unix 4.0E and later. */
#    ifdef _int_mult_upper /* kludge to identify version of builtins.h */
#      define PopCnt(a)     _popcnt(a)
#      define FirstOne(a)   _leadz(a)
#      define LastOne(a)    (63 - _trailz(a))
#    endif
#  endif
#endif

#define Max(a,b)  (((a) > (b)) ? (a) : (b))
#define Min(a,b)  (((a) < (b)) ? (a) : (b))
#define FileDistance(a,b) abs(((a)&7) - ((b)&7))
#define RankDistance(a,b) abs(((a)>>3) - ((b)>>3))
#define Distance(a,b) Max(FileDistance(a,b),RankDistance(a,b))
#define DrawScore(wtm)                 (draw_score[wtm])

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
#define Attack(square,queen) !(obstructed[square][queen] & Occupied)
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
#  define AttacksRook(a)    (AttacksRank(a)|AttacksFile(a))
#  define AttacksBishop(a)  (AttacksDiaga1(a)|AttacksDiagh1(a))
#endif

#define AttacksQueen(a)   (AttacksBishop(a)|AttacksRook(a))
#define Rank(x)       (((x)>>3)&7)
#define File(x)       ((x)&7)
#define ChangeSide(x) ((x)^1)

#if defined(COMPACT_ATTACKS)
#  define AttacksDiaga1Int(diagp,boardp)                                  \
     (diagp)->ad_attacks[(diagp)->ad_which_attack[                        \
      ((boardp)->occupied_rl45>>(diagp)->ad_shift) & (diagp)->ad_mask]]

#  define AttacksDiagh1Int(diagp,boardp)                                  \
     (diagp)->d_attacks[(diagp)->d_which_attack[                          \
      ((boardp)->occupied_rr45>>(diagp)->d_shift) & (diagp)->d_mask]]

/*
  The length of the following macro is a little excessive as the
  rank_attacks lookup is duplicated.  Making it a function, or passing
  in a temporary variable would simplify it significantly, although
  hopefully the compiler recognizes the common subexpression.
*/
#  define AttacksRankInt(a,boardp)                          \
     ((BITBOARD)(at.rank_attack_bitboards[File(a)]          \
     [at.which_attack[File(a)]                              \
     [((((boardp)->w_occupied|(boardp)->b_occupied)>>       \
     ((Rank(~(a))<<3)+1))&0x3f)]])<<(Rank(~(a))<<3))

/* 
  The final left shift in this is optimizable, but the optimization is
  a little ugly to express.  There is no information that crosses the
  word boundary in the shift so it can be implemented as two separate
  word shifts that are joined together in a long long.
*/
#  define AttacksFileInt(a,boardp)                          \
     ((BITBOARD)(at.file_attack_bitboards[Rank(a)]          \
     [at.which_attack[Rank(a)]                              \
     [(((boardp)->occupied_rl90>>                           \
     ((File(~(a))<<3)+1))&0x3f)]])<<(File(~(a))))

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

#  define AttacksRank(a)                                                  \
      rook_attacks_r0[(a)][((tree->pos.w_occupied|tree->pos.b_occupied)>> \
                           (56-((a)&56)))&255]
#  define AttacksFile(a)                                                  \
      rook_attacks_rl90[(a)][(tree->pos.occupied_rl90>>                   \
                             (56-(((a)&7)<<3)))&255]
#  define AttacksDiaga1(a)                                                \
      bishop_attacks_rl45[(a)][(tree->pos.occupied_rl45>>                 \
                                bishop_shift_rl45[(a)])&255]
#  define AttacksDiagh1(a)                                                \
      bishop_attacks_rr45[(a)][(tree->pos.occupied_rr45>>                 \
                                bishop_shift_rr45[(a)])&255]
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
     ((boardp)->occupied_rl45>> (diagp)->ad_shift)&(diagp)->ad_mask]]

#  define MobilityDiagh1Int(diagp,boardp)                                   \
     (diagp)->d_mobility[(diagp)->d_which_attack [                          \
     ((boardp)->occupied_rr45>> (diagp)->d_shift)&(diagp)->d_mask]]

#  define MobilityRankInt(a,boardp)                                         \
     at.length8_mobility[File(a)][                                          \
     at.which_attack[File(a)][                                              \
     ((boardp)->w_occupied|(boardp)->b_occupied>>((Rank(~(a))<<3)+1))&0x3f]]

#  define MobilityFileInt(a,boardp)                                         \
     at.length8_mobility[Rank(a)][                                          \
     at.which_attack[Rank(a)] [                                             \
     ((boardp)->occupied_rl90>>((File(~(a))<<3)+1))&0x3f]]

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
     rook_mobility_r0[(a)][tree->pos.w_occupied|tree->pos.b_occupied)>>     \
                           (56-((a)&56))&255]
#  define MobilityFile(a)                                                   \
     rook_mobility_rl90[(a)][tree->pos.occupied_rl90>>                      \
                             (56-(((a)&7)<<3))&255]
#  define MobilityDiaga1(a)                                                 \
     bishop_mobility_rl45[(a)][tree->pos.occupied_rl45>>                    \
                               bishop_shift_rl45[(a)]&255]
#  define MobilityDiagh1(a)                                                 \
     bishop_mobility_rr45[(a)][tree->pos.occupied_rr45>>                    \
                               bishop_shift_rr45[(a)]&255]
#endif

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
#define PcOnSq(sq)     (tree->pos.board[sq])
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

#define HashPB32(a,b)       b=b_pawn_random32[a]^(b)
#define HashPW32(a,b)       b=w_pawn_random32[a]^(b)
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
#define SavePV(tree,ply,value,ph) do {                                      \
          tree->pv[ply-1].path[ply-1]=tree->current_move[ply-1];            \
          tree->pv[ply-1].pathl=ply-1;                                      \
          tree->pv[ply-1].pathh=ph;                                         \
          tree->pv[ply-1].pathd=iteration_depth;} while(0)

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

#endif /* if defined(TYPES_INCLUDED) */
