/*
 *******************************************************************************
 *                                                                             *
 *   configuration information:  the following variables need to be set to     *
 *   indicate the machine configuration/capabilities.                          *
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

#if defined(AFFINITY)
#  define _GNU_SOURCE
#  include <sched.h>
#endif
#if defined(UNIX)
#  define _GNU_SOURCE
#  if (CPUS > 1)
#    include <pthread.h>
#  endif
#  include <unistd.h>
#  include <sys/types.h>
#endif
#include <stdint.h>
#include <inttypes.h>
#include <time.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#if !defined(TYPES_INCLUDED)
#  define TYPES_INCLUDED
#  if !defined (UNIX)
#    define RESTRICT __restrict
#  else
#    define RESTRICT
#  endif
#  if !defined(CPUS)
#    define CPUS 1
#  endif
#  if !defined(UNIX)
#    include <windows.h>
#    include <process.h>
#  endif
#  define CDECL
#  define STDCALL
/* Provide reasonable defaults for UNIX systems. */
#  if !defined(UNIX)
#    undef  STDCALL
#    define STDCALL __stdcall
#    ifdef  VC_INLINE32
#      undef  CDECL
#      define CDECL __cdecl
#    endif
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
#  include "lock.h"
#  define MAXPLY                                 129
#  define MAX_TC_NODES                      10000000
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
  int8_t castle[2];
  uint8_t enpassant_target;
  uint8_t reversible;
} SEARCH_POSITION;
typedef struct {
  int move1;
  int move2;
} KILLER;
typedef struct {
  uint64_t pieces[7];
} BB_PIECES;
typedef struct {
  BB_PIECES color[2];
  uint64_t hash_key;
  uint64_t pawn_hash_key;
  int material_evaluation;
  int kingsq[2];
  int8_t board[64];
  char pieces[2][7];
  char majors[2];
  char minors[2];
  char total_all_pieces;
} POSITION;
typedef struct {
  uint64_t word1;
  uint64_t word2;
} HASH_ENTRY;
typedef struct {
  uint64_t key;
  int score_mg, score_eg;
  unsigned char defects_k[2];
  unsigned char defects_e[2];
  unsigned char defects_d[2];
  unsigned char defects_q[2];
  unsigned char all[2];
  unsigned char passed[2];
  unsigned char filler[4];
} PAWN_HASH_ENTRY;
typedef struct {
  uint64_t entry[4];
} PXOR;
typedef struct {
  int path[MAXPLY];
  int pathh;
  int pathl;
  int pathd;
  int pathv;
} PATH;
typedef struct {
  uint64_t path_sig;
  int hash_pathl;
  int hash_path_age;
  int hash_path_moves[MAXPLY];
} HPATH_ENTRY;
typedef struct {
  int phase;
  int remaining;
  int *last;
  int excluded_moves[5];
  int num_excluded;
} NEXT_MOVE;
typedef struct {
  int move;
/*
   x..xx xxxx xxx1 = failed low this iteration
   x..xx xxxx xx1x = failed high this iteration
   x..xx xxxx x1xx = don't search in parallel or reduce
   x..xx xxxx 1xxx = move has been searched
 */
  unsigned int status;
  int bm_age;
} ROOT_MOVE;
#  if !defined(UNIX)
#    pragma pack(4)
#  endif
typedef struct {
  uint64_t position;
  unsigned int status_played;
  float learn;
} BOOK_POSITION;
#  if !defined(UNIX)
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
  POSITION position;
  uint64_t save_hash_key[MAXPLY + 3];
  uint64_t save_pawn_hash_key[MAXPLY + 3];
  int rep_index;
  uint64_t rep_list[256];
  uint64_t all_pawns;
  uint64_t nodes_searched;
  uint64_t cache_n[64];
  PAWN_HASH_ENTRY pawn_score;
  SEARCH_POSITION status[MAXPLY + 3];
  NEXT_MOVE next_status[MAXPLY];
  PATH pv[MAXPLY];
  int cache_n_mobility[64];
  int curmv[MAXPLY];
  int hash_move[MAXPLY];
  int *last[MAXPLY];
  uint64_t fail_highs;
  uint64_t fail_high_first_move;
  unsigned int evaluations;
  unsigned int egtb_probes;
  unsigned int egtb_probes_successful;
  unsigned int extensions_done;
  unsigned int qchecks_done;
  unsigned int reductions_done;
  unsigned int moves_fpruned;
  KILLER killers[MAXPLY];
  int move_list[5120];
  int sort_value[256];
  unsigned char inchk[MAXPLY];
  int phase[MAXPLY];
  int tropism[2];
  int dangerous[2];
  int score_mg, score_eg;
#  if (CPUS > 1)
  lock_t lock;
#  endif
  int thread_id;
  volatile int stop;
  char root_move_text[16];
  char remaining_moves_text[16];
  struct tree *volatile siblings[CPUS], *parent;
  volatile int nprocs;
  int alpha;
  int beta;
  volatile int value;
  int side;
  int depth;
  int ply;
  int cutmove;
  volatile int used;
  volatile int moves_searched;
};
typedef struct tree TREE;
struct thread {
  TREE *volatile tree;
  volatile int idle;
  char filler[52];
};
typedef struct thread THREAD;
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
#  define KILLER_MOVE_3             7
#  define KILLER_MOVE_4             8
#  define GENERATE_ALL_MOVES        9
#  define REMAINING_MOVES          10
#if defined(UNIX) && !defined(INLINEASM)
int CDECL PopCnt(uint64_t);
int CDECL MSB(uint64_t);
int CDECL LSB(uint64_t);
#endif
void AlignedMalloc(void **, int, size_t);
void AlignedRemalloc(void **, int, size_t);
void Analyze(void);
void Annotate(void);
void AnnotateHeaderHTML(char *, FILE *);
void AnnotateFooterHTML(FILE *);
void AnnotatePositionHTML(TREE *RESTRICT, int, FILE *);
char *AnnotateVtoNAG(int, int, int, int);
void AnnotateHeaderTeX(FILE *);
void AnnotateFooterTeX(FILE *);
void AnnotatePositionTeX(TREE *, int, FILE *);
uint64_t atoiKM(char *);
int Attacks(TREE *RESTRICT, int, int);
uint64_t AttacksFrom(TREE *RESTRICT, int, int);
uint64_t AttacksTo(TREE *RESTRICT, int);
void Bench(int);
int Book(TREE *RESTRICT, int, int);
void BookClusterIn(FILE *, int, BOOK_POSITION *);
void BookClusterOut(FILE *, int, BOOK_POSITION *);
int BookIn32(unsigned char *);
float BookIn32f(unsigned char *);
uint64_t BookIn64(unsigned char *);
int BookMask(char *);
unsigned char *BookOut32(int);
unsigned char *BookOut32f(float);
unsigned char *BookOut64(uint64_t);
int BookPonderMove(TREE *RESTRICT, int);
void BookUp(TREE *RESTRICT, int, char **);
void BookSort(BB_POSITION *, int, int);
int BookUpCompare(const void *, const void *);
BB_POSITION BookUpNextPosition(int, int);
int CheckInput(void);
void ClearHashTableScores(void);
int ComputeDifficulty(int, int);
void CopyToParent(TREE *RESTRICT, TREE *RESTRICT, int);
void CopyToChild(TREE *RESTRICT, int);
void CraftyExit(int);
void DisplayArray(int *, int);
void DisplayArrayX2(int *, int *, int);
void DisplayBitBoard(uint64_t);
void Display2BitBoards(uint64_t, uint64_t);
void DisplayChessBoard(FILE *, POSITION);
char *DisplayEvaluation(int, int);
char *DisplayEvaluationKibitz(int, int);
void DisplayFT(int, int, int);
char *DisplayHHMM(unsigned int);
char *DisplayHHMMSS(unsigned int);
char *DisplayKMB(uint64_t);
void DisplayPV(TREE *RESTRICT, int, int, int, PATH *);
char *DisplayTime(unsigned int);
char *Display2Times(unsigned int);
char *DisplayTimeKibitz(unsigned int);
void DisplayTreeState(TREE *RESTRICT, int, int, int);
void DisplayChessMove(char *, int);
int Drawn(TREE *RESTRICT, int);
void DisplayType3(int *, int *);
void DisplayType4(int *, int *);
void DisplayType5(int *, int);
void DisplayType6(int *);
void Edit(void);
#  if !defined(NOEGTB)
int EGTBProbe(TREE *RESTRICT, int, int, int *);
void EGTBPV(TREE *RESTRICT, int);
#  endif
int Evaluate(TREE *RESTRICT, int, int, int, int);
void EvaluateBishops(TREE *RESTRICT, int);
void EvaluateDevelopment(TREE *RESTRICT, int, int);
int EvaluateDraws(TREE *RESTRICT, int, int, int);
int EvaluateHasOpposition(int, int, int);
void EvaluateKings(TREE *RESTRICT, int, int);
int EvaluateKingsFile(TREE *RESTRICT, int, int);
void EvaluateKnights(TREE *RESTRICT, int);
void EvaluateMate(TREE *RESTRICT, int);
void EvaluateMaterial(TREE *RESTRICT, int);
void EvaluatePassedPawns(TREE *RESTRICT, int, int);
void EvaluatePassedPawnRaces(TREE *RESTRICT, int);
void EvaluatePawns(TREE *RESTRICT, int);
void EvaluateQueens(TREE *RESTRICT, int);
void EvaluateRooks(TREE *RESTRICT, int);
int EvaluateWinningChances(TREE *RESTRICT, int, int);
void EVTest(char *);
int Exclude(TREE *RESTRICT, int, int);
int FindBlockID(TREE *RESTRICT);
char *FormatPV(TREE *RESTRICT, int, PATH);
int FTbSetCacheSize(void *, unsigned long);
int GameOver(int);
int *GenerateCaptures(TREE *RESTRICT, int, int, int *);
int *GenerateCheckEvasions(TREE *RESTRICT, int, int, int *);
int *GenerateChecks(TREE *RESTRICT, int, int *);
int *GenerateNoncaptures(TREE *RESTRICT, int, int, int *);
int HashProbe(TREE *RESTRICT, int, int, int, int, int, int*);
void HashStore(TREE *RESTRICT, int, int, int, int, int, int);
void HashStorePV(TREE *RESTRICT, int, int);
int IInitializeTb(char *);
void Initialize(void);
void InitializeAttackBoards(void);
void InitializeChessBoard(TREE *);
int InitializeGetLogID();
void InitializeHashTables(void);
void InitializeKillers(void);
void InitializeKingSafety(void);
void InitializeMagic(void);
uint64_t InitializeMagicBishop(int, uint64_t);
uint64_t InitializeMagicRook(int, uint64_t);
uint64_t InitializeMagicOccupied(int *, int, uint64_t);
void InitializeMasks(void);
void InitializePawnMasks(void);
void InitializeSMP(void);
int InputMove(TREE *RESTRICT, char *, int, int, int, int);
int InputMoveICS(TREE *RESTRICT, char *, int, int, int, int);
uint64_t InterposeSquares(int, int, int);
void Interrupt(int);
int InvalidPosition(TREE *RESTRICT);
int Iterate(int, int, int);
void Kibitz(int, int, int, int, int, uint64_t, int, int, char *);
void Killer(TREE *RESTRICT, int, int);
int KingPawnSquare(int, int, int, int);
int LearnAdjust(int);
void LearnBook(void);
int LearnFunction(int, int, int, int);
void LearnValue(int, int);
void MakeMove(TREE *RESTRICT, int, int, int);
void MakeMoveRoot(TREE *RESTRICT, int, int);
void NewGame(int);
int NextEvasion(TREE *RESTRICT, int, int);
int NextMove(TREE *RESTRICT, int, int);
int NextRootMove(TREE *RESTRICT, TREE *RESTRICT, int);
int NextRootMoveParallel(void);
int Option(TREE *RESTRICT);
int OptionMatch(char *, char *);
void OptionPerft(TREE *RESTRICT, int, int, int);
void Output(TREE *RESTRICT, int);
char *OutputMove(TREE *RESTRICT, int, int, int);
int ParseTime(char *);
void Pass(void);
int PinnedOnKing(TREE *RESTRICT, int, int);
int Ponder(int);
void Print(int, char *, ...);
char *PrintKM(size_t, int);
int Quiesce(TREE *RESTRICT, int, int, int, int, int);
int QuiesceEvasions(TREE *RESTRICT, int, int, int, int);
unsigned int Random32(void);
uint64_t Random64(void);
int Read(int, char *);
int ReadChessMove(TREE *RESTRICT, FILE *, int, int);
void ReadClear(void);
unsigned int ReadClock(void);
int ReadPGN(FILE *, int);
int ReadNextMove(TREE *RESTRICT, char *, int, int);
int ReadParse(char *, char *args[], char *);
int ReadInput(void);
int Repeat(TREE *RESTRICT, int);
int Repeat3x(TREE *RESTRICT, int);
void ResignOrDraw(TREE *RESTRICT, int);
void RestoreGame(void);
void RootMoveList(int);
int Search(TREE *RESTRICT, int, int, int, int, int, int);
int SearchParallel(TREE *RESTRICT, int, int, int, int, int, int);
void Trace(TREE *RESTRICT, int, int, int, int, int, const char *, int);
void SetBoard(TREE *, int, char **, int);
void SetChessBitBoards(TREE *);
void SharedFree(void *address);
int StrCnt(char *, char);
int Swap(TREE *RESTRICT, int, int);
int SwapO(TREE *RESTRICT, int, int);
void Test(char *);
void TestEPD(char *);
int Thread(TREE *RESTRICT);
void WaitForAllThreadsInitialized(void);
void *STDCALL ThreadInit(void *);
#  if !defined(UNIX)
void ThreadMalloc(int64_t);
#  endif
void ThreadStop(TREE *RESTRICT);
int ThreadWait(int64_t, TREE *RESTRICT);
void TimeAdjust(int, int);
int TimeCheck(TREE *RESTRICT, int);
void TimeSet(int);
void UnmakeMove(TREE *RESTRICT, int, int, int);
int ValidMove(TREE *RESTRICT, int, int, int);
int VerifyMove(TREE *RESTRICT, int, int, int);
void ValidatePosition(TREE *RESTRICT, int, int, char *);
#  if !defined(UNIX)
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
#  define Extract(side, mptr, m, t)                                        \
  for ( ; m ; Clear(to, m)) {                                              \
    to = Advanced(side, m);                                                \
    *mptr++ = t | (to << 6) | (Abs(PcOnSq(to)) << 15);                     \
  }
#  define Check(side) Attacks(tree, Flip(side), KingSQ(side))
#  define Attack(from,to) (!(intervening[from][to] & OccupiedSquares))
#  define BishopAttacks(square, occ) *(magic_bishop_indices[square]+((((occ)&magic_bishop_mask[square])*magic_bishop[square])>>magic_bishop_shift[square]))
#  define BishopMobility(square, occ) *(magic_bishop_mobility_indices[square]+((((occ)&magic_bishop_mask[square])*magic_bishop[square])>>magic_bishop_shift[square]))
#  define KingAttacks(square) king_attacks[square]
#  define KnightAttacks(square) knight_attacks[square]
#  define PawnAttacks(side, square)   pawn_attacks[side][square]
#  define Reversible(p)               (tree->status[p].reversible)
#  define RookAttacks(square, occ) *(magic_rook_indices[square]+((((occ)&magic_rook_mask[square])*magic_rook[square])>>magic_rook_shift[square]))
#  define RookMobility(square, occ) *(magic_rook_mobility_indices[square]+((((occ)&magic_rook_mask[square])*magic_rook[square])>>magic_rook_shift[square]))
#  define QueenAttacks(square, occ)   (BishopAttacks(square, occ)|RookAttacks(square, occ))
#  define Rank(x)       ((x)>>3)
#  define File(x)       ((x)&7)
#  define Flip(x)       ((x)^1)
#  define Advanced(side, squares) ((side) ? MSB(squares) : LSB(squares))
#  define MinMax(side, v1, v2) ((side) ? Min((v1), (v2)) : Max((v1), (v2)))
#  define InFront(side, k, p) ((side) ? k > p : k < p)
#  define Behind(side, k, p) ((side) ? k < p : k > p)
#  define RankAttacks(a) (RookAttacks(a, OccupiedSquares) & rank_mask[Rank(a)])
#  define FileAttacks(a) (RookAttacks(a, OccupiedSquares) & file_mask[File(a)])
#  define Diaga1Attacks(a) (BishopAttacks(a, OccupiedSquares) & (plus9dir[a] | minus9dir[a]))
#  define Diagh1Attacks(a) (BishopAttacks(a, OccupiedSquares) & (plus7dir[a] | minus7dir[a]))
#  define InterposeSquares(kingsq, checksq) intervening[kingsq][checksq]
/*
   the following macros are used to extract the pieces of a move that are
   kept compressed into the rightmost 21 bits of a simple integer.
 */
#  define Passed(sq, wtm)       (!(mask_passed[wtm][sq] & Pawns(Flip(wtm))))
#  define From(a)               ((a) & 63)
#  define To(a)                 (((a)>>6) & 63)
#  define Piece(a)              (((a)>>12) & 7)
#  define Captured(a)           (((a)>>15) & 7)
#  define Promote(a)            (((a)>>18) & 7)
#  define CaptureOrPromote(a)   (((a)>>15) & 63)
#  define SetMask(a)            (set_mask[a])
#  define ClearMask(a)          (clear_mask[a])
#  define Pawns(c)              (tree->position.color[c].pieces[pawn])
#  define Knights(c)            (tree->position.color[c].pieces[knight])
#  define Bishops(c)            (tree->position.color[c].pieces[bishop])
#  define Rooks(c)              (tree->position.color[c].pieces[rook])
#  define Queens(c)             (tree->position.color[c].pieces[queen])
#  define Kings(c)              (tree->position.color[c].pieces[king])
#  define KingSQ(c)             (tree->position.kingsq[c])
#  define Occupied(c)           (tree->position.color[c].pieces[occupied])
#  define Pieces(c, p)          (tree->position.color[c].pieces[p])
#  define TotalPieces(c, p)     (tree->position.pieces[c][p])
#  define TotalMinors(c)        (tree->position.minors[c])
#  define TotalMajors(c)        (tree->position.majors[c])
#  define PieceValues(c, p)     (piece_values[p][c])
#  define TotalAllPieces        (tree->position.total_all_pieces)
#  define Material              (tree->position.material_evaluation)
#  define MaterialSTM(side)     ((side) ? Material : -Material)
#  define MateScore(s)          (Abs(s) > 32000)
#  define Castle(ply, c)        (tree->status[ply].castle[c])
#  define HashKey               (tree->position.hash_key)
#  define PawnHashKey           (tree->position.pawn_hash_key)
#  define EnPassant(ply)        (tree->status[ply].enpassant_target)
#  define EnPassantTarget(ply)  (EnPassant(ply) ? SetMask(EnPassant(ply)) : 0)
#  define PcOnSq(sq)            (tree->position.board[sq])
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
#  define HashEP(sq)                 (HashKey^=enpassant_random[sq])
#  define SavePV(tree,ply,ph)   do {                                        \
        tree->pv[ply-1].path[ply-1]=tree->curmv[ply-1];                     \
        tree->pv[ply-1].pathl=ply;                          \
        tree->pv[ply-1].pathh=ph;                           \
        tree->pv[ply-1].pathd=iteration_depth;} while(0)
#  if defined(INLINEASM)
#    include "inline.h"
#  endif
#  if defined(UNIX)
#    define SPEAK "./speak "
#  else
#    define SPEAK ".\\Speak.exe "
#  endif
#endif
/* *INDENT-ON* */
