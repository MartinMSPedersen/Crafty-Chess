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
 *   UNIX:  define this if the program is being run on a unix-based system,    *
 *   which causes the executable to use unix-specific runtime utilities.       *
 *                                                                             *
 *   CPUS=N:  this sets up data structures to the appropriate size to support  *
 *   up to N simultaneous search engines.  note that you can set this to a     *
 *   value larger than the max processors you currently have, because the mt=n *
 *   command (added to the command line or your crafty.rc/.craftyrc file) will *
 *   control how many threads are actually spawned.                            *
 *                                                                             *
 *******************************************************************************
 */
/* *INDENT-OFF* */

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#if !defined(IPHONE)
#  include <malloc.h>
#endif
#include <string.h>
#if !defined(TYPES_INCLUDED)
#  include "lock.h"
#  if defined (_MSC_VER) && (_MSC_VER >= 1300) && \
    (!defined(_M_IX86) || (_MSC_VER >= 1400))
#    define RESTRICT __restrict
#  else
#    define RESTRICT
#  endif
#  if !defined(CPUS)
#    define CPUS 1
#  endif
#  if defined(NT_i386)
#    include <windows.h>
#    include <process.h>
#  endif
#  define TYPES_INCLUDED
#  define CDECL
#  define STDCALL
/* Provide reasonable defaults for UNIX systems. */
#  undef  HAS_64BITS    /* machine has 64-bit integers / operators    */
#  define UNIX  /* system is unix-based                       */
/* Architecture-specific definitions */
#  if defined(AIX)
#    undef  HAS_64BITS  /* machine has 64-bit integers / operators    */
#    define UNIX        /* system is unix-based                       */
#  endif
#  if defined(AMIGA)
#    undef  HAS_64BITS  /* machine has 64-bit integers / operators    */
#    undef  UNIX        /* system is unix-based                       */
#  endif
#  if defined(FreeBSD)
#    undef  HAS_64BITS  /* machine has 64-bit integers / operators    */
#    define UNIX        /* system is unix-based                       */
#  endif
#  if defined(HP)
#    undef  HAS_64BITS  /* machine has 64-bit integers / operators    */
#    define UNIX        /* system is unix-based                       */
#  endif
#  if defined(LINUX)
#    define HAS_64BITS  /* machine has 64-bit integers / operators    */
#    define UNIX        /* system is unix-based                       */
#  endif
#  if defined(MIPS)
#    undef  HAS_64BITS  /* machine has 64-bit integers / operators    */
#    define UNIX        /* system is unix-based                       */
#  endif
#  if defined(NetBSD)
#    if defined(__alpha__)
#      define HAS_64BITS        /* machine has 64-bit integers / operators   */
#      define UNIX      /* system is unix-based                      */
#    else
#      undef  HAS_64BITS        /* machine has 64-bit integers / operators   */
#      define UNIX      /* system is unix-based                      */
#    endif
#  endif
#  if defined(NEXT)
#    undef  HAS_64BITS  /* machine has 64-bit integers / operators    */
#    define UNIX        /* system is unix-based                       */
#  endif
#  if defined(NT_i386)
#    undef  HAS_64BITS  /* machine has 64-bit integers / operators    */
#    undef  UNIX        /* system is unix-based                       */
#    undef  STDCALL
#    define STDCALL __stdcall
#    ifdef  VC_INLINE32
#      undef  CDECL
#      define CDECL __cdecl
#    endif
#  endif
#  if defined(OS2)
#    undef  HAS_64BITS  /* machine has 64-bit integers / operators    */
#    define UNIX        /* system is unix-based                       */
#  endif
#  if defined(SGI)
#    undef  HAS_64BITS  /* machine has 64-bit integers / operators    */
#    define UNIX        /* system is unix-based                       */
#  endif
#  if defined(SUN)
#    undef  HAS_64BITS  /* machine has 64-bit integers / operators    */
#    define UNIX        /* system is unix-based                       */
#  endif
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
#  define MAXPLY                                  65
#  define MAX_TC_NODES                       1000000
#  define MAX_BLOCKS_PER_CPU                      64
#  define MAX_BLOCKS         MAX_BLOCKS_PER_CPU*CPUS
#  define BOOK_CLUSTER_SIZE                     8000
#  define BOOK_POSITION_SIZE                      16
#  define MERGE_BLOCK                           1000
#  define SORT_BLOCK                         4000000
#  define LEARN_INTERVAL                          10
#  define LEARN_COUNTER_BAD                      -80
#  define LEARN_COUNTER_GOOD                    +100
#  define MATE                                 32768
#  define PAWN_VALUE                             100
#  define KNIGHT_VALUE                           325
#  define BISHOP_VALUE                           325
#  define ROOK_VALUE                             500
#  define QUEEN_VALUE                           1050
#  define KING_VALUE                           40000
#  define MAX_DRAFT                              256
#  if defined(HAS_64BITS)
typedef unsigned long BITBOARD;
#  elif defined(NT_i386)
typedef unsigned __int64 BITBOARD;
#  else
typedef unsigned long long BITBOARD;
#  endif
#  if defined(NT_i386)
#    define BMF   "%I64u"
#    define BMF6  "%6I64u"
#    define BMF10 "%10I64u"
#  else
#    define BMF   "%llu"
#    define BMF6  "%6llu"
#    define BMF10 "%10llu"
#  endif
#  if defined(UNIX) & (CPUS > 1)
#    include <pthread.h>
#  endif
#  include <time.h>
#  if !defined(CLOCKS_PER_SEC)
#    define CLOCKS_PER_SEC 1000000
#  endif
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
typedef enum { empty = 0, occupied = 0, pawn = 1, knight = 2, bishop = 3,
  rook = 4, queen = 5, king = 6
} PIECE;
typedef enum { black = 0, white = 1 } COLOR;
typedef enum { mg = 0, eg = 1 } PHASE;
typedef enum { empty_v = 0, pawn_v = 1, knight_v = 3,
  bishop_v = 3, rook_v = 5, queen_v = 9, king_v = 99
} PIECE_V;
typedef enum { think = 1, puzzle = 2, book = 3, annotate = 4 } SEARCH_TYPE;
typedef enum { normal_mode, tournament_mode } PLAYING_MODE;
typedef struct {
  unsigned char enpassant_target;
  signed char castle[2];
  unsigned char rule_50_moves;
} SEARCH_POSITION;
typedef struct {
  int move1;
  int move2;
} KILLER;
typedef struct {
  BITBOARD pieces[7];
} BB_PIECES;
typedef struct {
  BB_PIECES color[2];
  BITBOARD hash_key;
  BITBOARD pawn_hash_key;
  int material_evaluation;
  int kingsq[2];
  signed char board[64];
  signed char pieces[2][7];
  signed char majors[2];
  signed char minors[2];
  signed char total_all_pieces;
} POSITION;
typedef struct {
  BITBOARD word1;
  BITBOARD word2;
} HASH_ENTRY;
typedef struct {
  BITBOARD key;
  int score_mg, score_eg;
  unsigned char defects_k[2];
  unsigned char defects_e[2];
  unsigned char defects_d[2];
  unsigned char defects_q[2];
  unsigned char all[2];
  unsigned char passed[2];
  unsigned char candidates[2];
  unsigned char open_files;
  unsigned char filler;
} PAWN_HASH_ENTRY;
typedef struct {
  BITBOARD entry[4];
} PXOR;
typedef struct {
  int path[MAXPLY];
  unsigned char pathh;
  unsigned char pathl;
  unsigned char pathd;
} PATH;
typedef struct {
  BITBOARD path_sig;
  int hash_pathl;
  int  hash_path_age;
  int hash_path[MAXPLY];
} HPATH_ENTRY;
typedef struct {
  int phase;
  int remaining;
  int *last;
} NEXT_MOVE;
typedef struct {
  BITBOARD nodes;
  int move;
/*
   x..xx xxxx xxx1 = failed low once
   x..xx xxxx xx1x = failed low twice
   x..xx xxxx x1xx = failed low three times
   x..xx xxxx 1xxx = failed high once
   x..xx xxx1 xxxx = failed high twice
   x..xx xx1x xxxx = failed high three times
   x..xx x1xx xxxx = don't search in parallel
   x..xx 1xxx xxxx = do not reduce this move
   x..x1 xxxx xxxx = move has been searched
 */
  unsigned int status;
} ROOT_MOVE;
#  if defined(NT_i386)
#    pragma pack(4)
#  endif
typedef struct {
  BITBOARD position;
  unsigned int status_played;
  float learn;
} BOOK_POSITION;
#  if defined(NT_i386)
#    pragma pack()
#  endif
typedef struct {
  unsigned char position[8];
  unsigned char status;
  unsigned char percent_play;
} BB_POSITION;
struct personality_term {
  char *description;
  int type;
  int size;
  int *value;
};
struct tree {
  POSITION pos;
  BITBOARD save_hash_key[MAXPLY + 2];
  BITBOARD rep_list[2][128];
  BITBOARD all_pawns;
  BITBOARD nodes_searched;
  BITBOARD save_pawn_hash_key[MAXPLY + 2];
  BITBOARD cache_n[64];
  PAWN_HASH_ENTRY pawn_score;
  SEARCH_POSITION position[MAXPLY + 2];
  NEXT_MOVE next_status[MAXPLY];
  PATH pv[MAXPLY];
  int cache_n_mobility[64];
  int rep_index[2];
  int curmv[MAXPLY];
  int hash_move[MAXPLY];
  int *last[MAXPLY];
  unsigned int fail_high;
  unsigned int fail_high_first;
  unsigned int evaluations;
  unsigned int egtb_probes;
  unsigned int egtb_probes_successful;
  unsigned int extensions_done;
  unsigned int qchecks_done;
  unsigned int reductions_done;
  unsigned int moves_pruned;
  KILLER killers[MAXPLY];
  int move_list[5120];
  int sort_value[256];
  unsigned char inchk[MAXPLY];
  unsigned char phase[MAXPLY];
  int search_value;
  int tropism[2];
  int dangerous[2];
  int score_mg, score_eg;
  int root_move;
#  if (CPUS > 1)
  lock_t lock;
#  endif
  long thread_id;
  volatile int stop;
  char root_move_text[16];
  char remaining_moves_text[16];
  struct tree *volatile siblings[CPUS], *parent;
  volatile int nprocs;
  int alpha;
  int beta;
  int value;
  int wtm;
  int depth;
  int ply;
  int cutmove;
  int moves_searched;
  volatile int used;
};
typedef struct tree TREE;
/*
   DO NOT modify these.  these are constants, used in multiple modules.
   modification may corrupt the search in any number of ways, all bad.
 */
#  define WORTHLESS                 0
#  define LOWER                     1
#  define UPPER                     2
#  define EXACT                     3
#  define HASH_MISS                 0
#  define HASH_HIT                  1
#  define AVOID_NULL_MOVE           2
#  define NO_NULL                   0
#  define DO_NULL                   1
#  define NONE                      0
#  define EVALUATION                0
#  define NULL_MOVE                 1
#  define HASH_MOVE                 2
#  define GENERATE_CAPTURE_MOVES    3
#  define CAPTURE_MOVES             4
#  define KILLER_MOVE_1             5
#  define KILLER_MOVE_2             6
#  define GENERATE_ALL_MOVES        7
#  define SORT_ALL_MOVES            8
#  define REMAINING_MOVES           9
#  if defined(VC_INLINE32)
#    include "vcinline.h"
#  else
#    if !defined(INLINE64) && !defined(INLINE32)
int CDECL PopCnt(BITBOARD);
int CDECL MSB(BITBOARD);
int CDECL LSB(BITBOARD);
#    endif
#  endif
void AlignedMalloc(void **, int, size_t);
void AlignedRemalloc(void **, int, size_t);
void Analyze(void);
void Annotate(void);
void AnnotateHeaderHTML(char *, FILE *);
void AnnotateFooterHTML(FILE *);
void AnnotatePositionHTML(TREE * RESTRICT, int, FILE *);
char *AnnotateVtoNAG(int, int, int, int);
void AnnotateHeaderTeX(char *, FILE *);
void AnnotateFooterTeX(FILE *);
void AnnotatePositionTeX(TREE *, int, FILE *);
BITBOARD atoiKM(char *);
int Attacks(TREE * RESTRICT, int, int);
BITBOARD AttacksTo(TREE * RESTRICT, int);
void Bench(int);
int Book(TREE * RESTRICT, int, int);
void BookClusterIn(FILE *, int, BOOK_POSITION *);
void BookClusterOut(FILE *, int, BOOK_POSITION *);
int BookIn32(unsigned char *);
float BookIn32f(unsigned char *);
BITBOARD BookIn64(unsigned char *);
int BookMask(char *);
unsigned char *BookOut32(int);
unsigned char *BookOut32f(float);
unsigned char *BookOut64(BITBOARD);
int BookPonderMove(TREE * RESTRICT, int);
void BookUp(TREE * RESTRICT, int, char **);
void BookSort(BB_POSITION *, int, int);
#  if defined(NT_i386)
int _cdecl BookUpCompare(const void *, const void *);
#  else
int BookUpCompare(const void *, const void *);
#  endif
BB_POSITION BookUpNextPosition(int, int);
int CheckInput(void);
void ClearHashTableScores(void);
void CopyFromChild(TREE * RESTRICT, TREE * RESTRICT, int);
TREE *CopyToChild(TREE * RESTRICT, int);
void CraftyExit(int);
void DisplayArray(int *, int);
void DisplayArrayX2(int *, int *, int);
void DisplayBitBoard(BITBOARD);
void Display2BitBoards(BITBOARD, BITBOARD);
void DisplayChessBoard(FILE *, POSITION);
char *DisplayEvaluation(int, int);
char *DisplayEvaluationKibitz(int, int);
void DisplayFT(int, int, int);
char *DisplayHHMM(unsigned int);
char *DisplayHHMMSS(unsigned int);
char *DisplayKM(unsigned int);
void DisplayPV(TREE * RESTRICT, int, int, int, int, PATH *);
char *DisplayTime(unsigned int);
char *DisplayTimeKibitz(unsigned int);
void DisplayTreeState(TREE * RESTRICT, int, int, int);
void DisplayChessMove(char *, int);
int Drawn(TREE * RESTRICT, int);
void DisplayType3(int *, int *);
void DisplayType4(int *, int *);
void DisplayType5(int *, int *, int);
void DisplayType6(int *, int *);
void DisplayType7(int *, int *);
void DisplayType8(int *, int);
void DisplayType9(int *, int *);
void Edit(void);
#  if !defined(NOEGTB)
int EGTBProbe(TREE * RESTRICT, int, int, int *);
void EGTBPV(TREE * RESTRICT, int);
#  endif
int Evaluate(TREE * RESTRICT, int, int, int, int);
void EvaluateBishops(TREE * RESTRICT, int);
void EvaluateDevelopment(TREE * RESTRICT, int, int);
int EvaluateDraws(TREE * RESTRICT, int, int, int);
void EvaluateKings(TREE * RESTRICT, int, int);
int EvaluateKingsFile(TREE * RESTRICT, int, int);
void EvaluateKnights(TREE * RESTRICT, int);
void EvaluateMate(TREE * RESTRICT, int);
void EvaluateMaterial(TREE * RESTRICT, int);
void EvaluatePassedPawns(TREE * RESTRICT, int);
void EvaluatePassedPawnRaces(TREE * RESTRICT, int);
void EvaluatePawns(TREE * RESTRICT, int);
void EvaluateQueens(TREE * RESTRICT, int);
void EvaluateRooks(TREE * RESTRICT, int);
int EvaluateWinningChances(TREE * RESTRICT, int, int);
void EVTest(char *);
int FindBlockID(TREE * RESTRICT);
char *FormatPV(TREE * RESTRICT, int, PATH);
int FTbSetCacheSize(void *, unsigned long);
int GameOver(int);
int *GenerateCaptures(TREE * RESTRICT, int, int, int *);
int *GenerateCheckEvasions(TREE * RESTRICT, int, int, int *);
int *GenerateChecks(TREE * RESTRICT, int, int, int *);
int *GenerateNoncaptures(TREE * RESTRICT, int, int, int *);
int HashProbe(TREE * RESTRICT, int, int, int, int, int, int*);
void HashStore(TREE * RESTRICT, int, int, int, int, int, int);
void HashStorePV(TREE * RESTRICT, int, int);
int EvaluateHasOpposition(int, int, int);
int IInitializeTb(char *);
void Initialize(void);
void InitializeAttackBoards(void);
void InitializeChessBoard(TREE *);
int InitializeGetLogID();
void InitializeHashTables(void);
void InitializeKillers(void);
void InitializeKingSafety(void);
void InitializeMagic(void);
BITBOARD InitializeMagicBishop(int, BITBOARD);
BITBOARD InitializeMagicRook(int, BITBOARD);
BITBOARD InitializeMagicOccupied(int *, int, BITBOARD);
void InitializeMasks(void);
void InitializePawnMasks(void);
void InitializeSMP(void);
int InputMove(TREE * RESTRICT, char *, int, int, int, int);
int InputMoveICS(TREE * RESTRICT, char *, int, int, int, int);
BITBOARD InterposeSquares(int, int, int);
void Interrupt(int);
int InvalidPosition(TREE * RESTRICT);
int Iterate(int, int, int);
void Kibitz(int, int, int, int, int, BITBOARD, int, char *);
void Killer(TREE * RESTRICT, int, int);
int KingPawnSquare(int, int, int, int);
void LearnBook(void);
int LearnFunction(int, int, int, int);
void LearnValue(int, int);
void MakeMove(TREE * RESTRICT, int, int, int);
void MakeMoveRoot(TREE * RESTRICT, int, int);
void NewGame(int);
int NextEvasion(TREE * RESTRICT, int, int);
int NextMove(TREE * RESTRICT, int, int);
int NextRootMove(TREE * RESTRICT, TREE * RESTRICT, int);
int NextRootMoveParallel(void);
int Option(TREE * RESTRICT);
int OptionMatch(char *, char *);
void OptionPerft(TREE * RESTRICT, int, int, int);
void Output(TREE * RESTRICT, int, int);
char *OutputMove(TREE * RESTRICT, int, int, int);
int ParseTime(char *);
void Pass(void);
int PinnedOnKing(TREE * RESTRICT, int, int);
int Ponder(int);
void Print(int, char *, ...);
char *PrintKM(size_t, int);
int Quiesce(TREE * RESTRICT, int, int, int, int, int);
int QuiesceEvasions(TREE * RESTRICT, int, int, int, int);
unsigned int Random32(void);
BITBOARD Random64(void);
int Read(int, char *);
int ReadChessMove(TREE * RESTRICT, FILE *, int, int);
void ReadClear(void);
unsigned int ReadClock(void);
int ReadPGN(FILE *, int);
int ReadNextMove(TREE * RESTRICT, char *, int, int);
int ReadParse(char *, char *args[], char *);
int ReadInput(void);
int RepetitionCheck(TREE * RESTRICT, int, int);
int RepetitionCheckBook(TREE * RESTRICT, int, int);
int RepetitionDraw(TREE * RESTRICT, int);
void ResignOrDraw(TREE * RESTRICT, int);
void RestoreGame(void);
void RootMoveList(int);
int Search(TREE * RESTRICT, int, int, int, int, int, int);
int SearchParallel(TREE * RESTRICT, int, int, int, int, int, int);
void Trace(TREE * RESTRICT, int, int, int, int, int, const char *, int);
void SetBoard(TREE *, int, char **, int);
void SetChessBitBoards(TREE *);
int SetRootAlpha(unsigned char, int);
int SetRootBeta(unsigned char, int);
void SharedFree(void *address);
int StrCnt(char *, char);
int Swap(TREE * RESTRICT, int, int);
int SwapO(TREE * RESTRICT, int, int);
void Test(char *);
void TestEPD(char *);
int Thread(TREE * RESTRICT);
void WaitForAllThreadsInitialized(void);
void *STDCALL ThreadInit(void *);
#  if defined(_WIN32) || defined(_WIN64)
void ThreadMalloc(int);
#  endif
void ThreadStop(TREE * RESTRICT);
int ThreadWait(long, TREE * RESTRICT);
void TimeAdjust(int, int);
int TimeCheck(TREE * RESTRICT, int);
void TimeSet(TREE * RESTRICT, int);
void UnmakeMove(TREE * RESTRICT, int, int, int);
int ValidMove(TREE * RESTRICT, int, int, int);
int VerifyMove(TREE * RESTRICT, int, int, int);
void ValidatePosition(TREE * RESTRICT, int, int, char *);
#  if defined(_WIN32) || defined(_WIN64)
extern void *WinMallocInterleaved(size_t, int);
extern void WinFreeInterleaved(void *, size_t);
#    define MallocInterleaved(cBytes, cThreads)\
    WinMallocInterleaved(cBytes, cThreads)
#    define FreeInterleaved(pMemory, cBytes)\
    WinFreeInterleaved(pMemory, cBytes)
#  else
#    if defined(NUMA)
#      define MallocInterleaved(cBytes, cThreads) numa_alloc_interleaved(cBytes)
#      define FreeInterleaved(pMemory, cBytes)    numa_free(pMemory, 1)
#    else
#      define MallocInterleaved(cBytes, cThreads) malloc(cBytes)
#      define FreeInterleaved(pMemory, cBytes)    free(pMemory)
#    endif
#  endif
#  define Abs(a)    (((a) > 0) ? (a) : -(a))
#  define Max(a,b)  (((a) > (b)) ? (a) : (b))
#  define Min(a,b)  (((a) < (b)) ? (a) : (b))
#  define FileDistance(a,b) abs(File(a) - File(b))
#  define RankDistance(a,b) abs(Rank(a) - Rank(b))
#  define Distance(a,b) Max(FileDistance(a,b), RankDistance(a,b))
#  define DrawScore(side)                 (draw_score[side])
#  define PopCnt8Bit(a) (pop_cnt_8bit[a])
#  define MSB8Bit(a) (msb_8bit[a])
#  define LSB8Bit(a) (lsb_8bit[a])
/*
  side = side to move
  mptr = pointer into move list
  m = bit vector of to squares to unpack
  t = pre-computed from + moving piece
 */
#  define Unpack(side, mptr, m, t)                                         \
  for ( ; m ; Clear(to, m)) {                                              \
    to = Advanced(side, m);                                                \
    *mptr++ = t | (to << 6) | (Abs(PcOnSq(to)) << 15);                     \
  }
#  define Check(side) Attacks(tree, KingSQ(side), Flip(side))
#  define Attack(from,to) (!(intervening[from][to] & OccupiedSquares))
#  define AttacksBishop(square, occ) *(magic_bishop_indices[square]+((((occ)&magic_bishop_mask[square])*magic_bishop[square])>>magic_bishop_shift[square]))
#  define MobilityBishop(square, occ) *(magic_bishop_mobility_indices[square]+((((occ)&magic_bishop_mask[square])*magic_bishop[square])>>magic_bishop_shift[square]))
#  define AttacksKnight(square) knight_attacks[square]
#  define AttacksRook(square, occ) *(magic_rook_indices[square]+((((occ)&magic_rook_mask[square])*magic_rook[square])>>magic_rook_shift[square]))
#  define MobilityRook(square, occ) *(magic_rook_mobility_indices[square]+((((occ)&magic_rook_mask[square])*magic_rook[square])>>magic_rook_shift[square]))
#  define AttacksQueen(square, occ)   (AttacksBishop(square, occ)|AttacksRook(square, occ))
#  define AttacksQueen(square, occ)   (AttacksBishop(square, occ)|AttacksRook(square, occ))
#  define Rank(x)       ((x)>>3)
#  define File(x)       ((x)&7)
#  define Flip(x)       ((x)^1)
#  define PawnAttacks(side, x)   (pawn_attacks[Flip(side)][(x)] & Pawns(side))
#  define Advanced(side, squares) ((side) ? MSB(squares) : LSB(squares))
#  define MinMax(side, v1, v2) ((side) ? Min((v1), (v2)) : Max((v1), (v2)))
#  define InFront(side, k, p) ((side) ? k > p : k < p)
#  define Behind(side, k, p) ((side) ? k < p : k > p)
#  define AttacksRank(a) (AttacksRook(a, OccupiedSquares) & rank_mask[Rank(a)])
#  define AttacksFile(a) (AttacksRook(a, OccupiedSquares) & file_mask[File(a)])
#  define AttacksDiaga1(a) (AttacksBishop(a, OccupiedSquares) & (plus9dir[a] | minus9dir[a]))
#  define AttacksDiagh1(a) (AttacksBishop(a, OccupiedSquares) & (plus7dir[a] | minus7dir[a]))
#  define InterposeSquares(kingsq, checksq) intervening[kingsq][checksq]
/*
   the following macros are used to extract the pieces of a move that are
   kept compressed into the rightmost 21 bits of a simple integer.
 */
#  define From(a)               ((a) & 63)
#  define To(a)                 (((a)>>6) & 63)
#  define Piece(a)              (((a)>>12) & 7)
#  define Captured(a)           (((a)>>15) & 7)
#  define Promote(a)            (((a)>>18) & 7)
#  define CaptureOrPromote(a)   (((a)>>15) & 63)
#  define SetMask(a)            (set_mask[a])
#  define ClearMask(a)          (clear_mask[a])
#  define Pawns(c)              (tree->pos.color[c].pieces[pawn])
#  define Knights(c)            (tree->pos.color[c].pieces[knight])
#  define Bishops(c)            (tree->pos.color[c].pieces[bishop])
#  define Rooks(c)              (tree->pos.color[c].pieces[rook])
#  define Queens(c)             (tree->pos.color[c].pieces[queen])
#  define Kings(c)              (tree->pos.color[c].pieces[king])
#  define KingSQ(c)             (tree->pos.kingsq[c])
#  define Occupied(c)           (tree->pos.color[c].pieces[occupied])
#  define Pieces(c, p)          (tree->pos.color[c].pieces[p])
#  define TotalPieces(c, p)     (tree->pos.pieces[c][p])
#  define PieceValues(c, p)     (piece_values[p][c])
#  define TotalAllPieces        (tree->pos.total_all_pieces)
#  define Material              (tree->pos.material_evaluation)
#  define MaterialSTM(side)     ((side) ? Material : -Material)
#  define Castle(ply, c)        (tree->position[ply].castle[c])
#  define Rule50Moves(ply)      (tree->position[ply].rule_50_moves)
#  define Repetition(side)      (tree->rep_index[side])
#  define HashKey               (tree->pos.hash_key)
#  define PawnHashKey           (tree->pos.pawn_hash_key)
#  define EnPassant(ply)        (tree->position[ply].enpassant_target)
#  define EnPassantTarget(ply)  (EnPassant(ply) ? SetMask(EnPassant(ply)) : 0)
#  define PcOnSq(sq)            (tree->pos.board[sq])
#  define OccupiedSquares       (Occupied(white) | Occupied(black))
#  define Color(square)         (square_color[square] ? dark_squares : ~dark_squares)
#  define SideToMove(c)         ((c) ? "White" : "Black")
/*
   the following macros are used to Set and Clear a specific bit in the
   second argument.  this is done to make the code more readable, rather
   than to make it faster.
 */
#  define ClearSet(a,b)         b=((a) ^ (b))
#  define Clear(a,b)            b=ClearMask(a) & (b)
#  define Set(a,b)              b=SetMask(a) | (b)
/*
   the following macros are used to update the hash signatures.
 */
#  define Hash(stm,piece,square)     (HashKey^=randoms[stm][piece][square])
#  define HashP(stm,square)          (PawnHashKey^=randoms[stm][pawn][square])
#  define HashCastle(stm,direction)  (HashKey^=castle_random[stm][direction])
#  define HashEP(stm)                (HashKey^=enpassant_random[stm])
#  define SavePV(tree,ply,ph)   do {                                        \
        tree->pv[ply-1].path[ply-1]=tree->curmv[ply-1];                     \
        tree->pv[ply-1].pathl=ply;                                        \
        tree->pv[ply-1].pathh=ph;                                           \
        tree->pv[ply-1].pathd=iteration_depth;} while(0)
#  if defined(INLINE64)
#    include "inline64.h"
#  endif
#  if defined(INLINE32)
#    include "inline32.h"
#  endif
#  if defined(UNIX)
#    define SPEAK "./speak "
#  else
#    define SPEAK ".\\Speak.exe "
#  endif
#endif                          /* if defined(TYPES_INCLUDED) */
/* *INDENT-ON* */
