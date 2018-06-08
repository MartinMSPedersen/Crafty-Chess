#include <stddef.h>
#include "chess.h"
#include "data.h"
#if defined(UNIX) || defined(AMIGA)
#  include <unistd.h>
#  include <sys/types.h>
#endif
#if defined(UNIX)
#  include <sys/stat.h>
#endif
#include "epdglue.h"
#if defined(NT_i386)
#  include <fcntl.h>    /* needed for definition of "_O_BINARY" */
#endif
/*
 *******************************************************************************
 *                                                                             *
 *   Initialize() performs routine initialization before anything else is      *
 *   attempted.  It uses a group of service routines to initialize various     *
 *   data structures that are needed before the engine can do anything at all. *
 *                                                                             *
 *******************************************************************************
 */
void Initialize()
{
  int i, major, minor, id;
  TREE *tree;
  void *mem;
  int j;

  tree = block[0];
  InitializeMagic();
  InitializeSMP();
  InitializeMasks();
  InitializeRandomHash();
  InitializeAttackBoards();
  InitializePawnMasks();
  InitializeChessBoard(tree);
  InitializeKillers();
#if defined(NT_i386)
  _fmode = _O_BINARY;   /* set file mode binary to avoid text translation */
#endif
#if defined(EPD)
  EGInit();
#endif
  tree->last[0] = tree->move_list;
  tree->last[1] = tree->move_list;
  sprintf(log_filename, "%s/book.bin", book_path);
  book_file = fopen(log_filename, "rb+");
  if (!book_file) {
    book_file = fopen(log_filename, "rb");
    if (!book_file) {
      Print(128, "unable to open book file [%s/book.bin].\n", book_path);
      Print(128, "book is disabled\n");
    } else {
      Print(128, "unable to open book file [%s/book.bin] for \"write\".\n",
          book_path);
      Print(128, "learning is disabled\n");
    }
  }
  sprintf(log_filename, "%s/books.bin", book_path);
  normal_bs_file = fopen(log_filename, "rb");
  books_file = normal_bs_file;
  if (!normal_bs_file)
    Print(128, "unable to open book file [%s/books.bin].\n", book_path);
  sprintf(log_filename, "%s/bookc.bin", book_path);
  computer_bs_file = fopen(log_filename, "rb");
  if (computer_bs_file)
    Print(128, "found computer opening book file [%s/bookc.bin].\n", book_path);
  if (book_file) {
    int maj_min;
    fseek(book_file, -sizeof(int), SEEK_END);
    fread(&maj_min, 4, 1, book_file);
    major = BookIn32((unsigned char *) &maj_min);
    minor = major & 65535;
    major = major >> 16;
    if (major < 20 || (major == 20 && minor < 17)) {
      Print(4095, "\nERROR!  book.bin not made by version 20.17 or later\n");
      fclose(book_file);
      fclose(books_file);
      book_file = 0;
      books_file = 0;
    }
  }
  id = InitializeGetLogID();
  sprintf(log_filename, "%s/log.%03d", log_path, id);
  sprintf(history_filename, "%s/game.%03d", log_path, id);
  log_file = fopen(log_filename, "w");
  history_file = fopen(history_filename, "w+");
  if (!history_file) {
    printf("ERROR, unable to open game history file, exiting\n");
    CraftyExit(1);
  }
  cb_trans_ref = sizeof(HASH_ENTRY) * hash_table_size + 15;
  trans_ref = (HASH_ENTRY *) malloc(cb_trans_ref);
  cb_pawn_hash_table = sizeof(PAWN_HASH_ENTRY) * pawn_hash_table_size + 15;
  pawn_hash_table = (PAWN_HASH_ENTRY *) malloc(cb_pawn_hash_table);
  if (!trans_ref) {
    Print(128, "malloc() failed, not enough memory.\n");
    free(trans_ref);
    free(pawn_hash_table);
    hash_table_size = 0;
    pawn_hash_table_size = 0;
    log_hash = 0;
    log_pawn_hash = 0;
    trans_ref = 0;
    pawn_hash_table = 0;
  }
/*
 ************************************************************
 *                                                          *
 *   now for some NUMA stuff.  we need to allocate the      *
 *   local memory for each processor, but we can't touch it *
 *   here or it will be faulted in and be allocated on the  *
 *   curret CPU, which is not where it should be located    *
 *   for optimal NUMA performance.  ThreadInit() will do    *
 *   the actual initialization after each new process is    *
 *   created, so that the pages of local memory will be     *
 *   faulted in on the correct processor and use local      *
 *   node memory for optimal performance.                   *
 *                                                          *
 ************************************************************
 */
#if defined(_WIN32) || defined(_WIN64)
  ThreadMalloc((int) 0);
#else
  for (i = 0; i < CPUS; i++) {
    mem = malloc(MAX_BLOCKS_PER_CPU * ((sizeof(TREE) + 2047)) & ~2047);
    for (j = 0; j < MAX_BLOCKS_PER_CPU; j++) {
      block[i * MAX_BLOCKS_PER_CPU + j + 1] =
          (TREE *) ((long) mem + j * ((sizeof(TREE) + 2047) & ~2047));
    }
  }
  for (i = 0; i < MAX_BLOCKS_PER_CPU; i++) {
    memset((void *) block[i + 1], 0, sizeof(TREE));
    block[i + 1]->used = 0;
    block[i + 1]->parent = (TREE *) - 1;
    LockInit(block[i + 1]->lock);
  }
#endif
  initialized_threads++;
  InitializeHashTables();
  hash_mask = (1 << log_hash) - 1;
  pawn_hash_mask = (1 << (log_pawn_hash)) - 1;
  InitializeKingSafety();
}
void InitializeAttackBoards(void)
{
  int i, j, frank, ffile, trank, tfile;
  int sq, lastsq;
  static const int knightsq[8] = { -17, -15, -10, -6, 6, 10, 15, 17 };
  static const int bishopsq[4] = { -9, -7, 7, 9 };
  static const int rooksq[4] = { -8, -1, 1, 8 };
  BITBOARD sqs;

/*
 initialize pawn attack boards
 */
  for (i = 0; i < 64; i++) {
    pawn_attacks[white][i] = 0;
    if (i < 56)
      for (j = 2; j < 4; j++) {
        sq = i + bishopsq[j];
        if ((abs(Rank(sq) - Rank(i)) == 1) && (abs(File(sq) - File(i)) == 1) &&
            (sq < 64) && (sq > -1))
          pawn_attacks[white][i] = pawn_attacks[white][i] | (BITBOARD) 1 << sq;
      }
    pawn_attacks[black][i] = 0;
    if (i > 7)
      for (j = 0; j < 2; j++) {
        sq = i + bishopsq[j];
        if ((abs(Rank(sq) - Rank(i)) == 1) && (abs(File(sq) - File(i)) == 1) &&
            (sq < 64) && (sq > -1))
          pawn_attacks[black][i] = pawn_attacks[black][i] | (BITBOARD) 1 << sq;
      }
  }
/*
 initialize knight attack board
 */
  for (i = 0; i < 64; i++) {
    knight_attacks[i] = 0;
    frank = Rank(i);
    ffile = File(i);
    for (j = 0; j < 8; j++) {
      sq = i + knightsq[j];
      if ((sq < 0) || (sq > 63))
        continue;
      trank = Rank(sq);
      tfile = File(sq);
      if ((abs(frank - trank) > 2) || (abs(ffile - tfile) > 2))
        continue;
      knight_attacks[i] = knight_attacks[i] | (BITBOARD) 1 << sq;
    }
  }
/*
 initialize bishop/queen attack boards and masks
 */
  for (i = 0; i < 64; i++) {
    for (j = 0; j < 4; j++) {
      sq = i;
      lastsq = sq;
      sq = sq + bishopsq[j];
      while ((abs(Rank(sq) - Rank(lastsq)) == 1) &&
          (abs(File(sq) - File(lastsq)) == 1) && (sq < 64) && (sq > -1)) {
        if (bishopsq[j] == 7)
          plus7dir[i] = plus7dir[i] | (BITBOARD) 1 << sq;
        else if (bishopsq[j] == 9)
          plus9dir[i] = plus9dir[i] | (BITBOARD) 1 << sq;
        else if (bishopsq[j] == -7)
          minus7dir[i] = minus7dir[i] | (BITBOARD) 1 << sq;
        else
          minus9dir[i] = minus9dir[i] | (BITBOARD) 1 << sq;
        lastsq = sq;
        sq = sq + bishopsq[j];
      }
    }
  }
  plus1dir[64] = 0;
  plus7dir[64] = 0;
  plus8dir[64] = 0;
  plus9dir[64] = 0;
  minus1dir[64] = 0;
  minus7dir[64] = 0;
  minus8dir[64] = 0;
  minus9dir[64] = 0;
/*
 initialize rook/queen attack boards
 */
  for (i = 0; i < 64; i++) {
    for (j = 0; j < 4; j++) {
      sq = i;
      lastsq = sq;
      sq = sq + rooksq[j];
      while ((((abs(Rank(sq) - Rank(lastsq)) == 1) &&
                  (abs(File(sq) - File(lastsq)) == 0)) ||
              ((abs(Rank(sq) - Rank(lastsq)) == 0) &&
                  (abs(File(sq) - File(lastsq)) == 1))) && (sq < 64) &&
          (sq > -1)) {
        if (rooksq[j] == 1)
          plus1dir[i] = plus1dir[i] | (BITBOARD) 1 << sq;
        else if (rooksq[j] == 8)
          plus8dir[i] = plus8dir[i] | (BITBOARD) 1 << sq;
        else if (rooksq[j] == -1)
          minus1dir[i] = minus1dir[i] | (BITBOARD) 1 << sq;
        else
          minus8dir[i] = minus8dir[i] | (BITBOARD) 1 << sq;
        lastsq = sq;
        sq = sq + rooksq[j];
      }
    }
  }
/*
 initialize king attack board
 */
  for (i = 0; i < 64; i++) {
    king_attacks[i] = 0;
    for (j = 0; j < 64; j++) {
      if (Distance(i, j) == 1)
        king_attacks[i] = king_attacks[i] | SetMask(j);
    }
  }
/*
 direction[sq1][sq2] gives the "move direction" to move from
 sq1 to sq2.  obstructed[sq1][sq2] gives a bit vector that indicates
 which squares must be unoccupied in order for <sq1> to attack <sq2>,
 assuming a sliding piece is involved.  to use this, you simply have
 to Or(obstructed[sq1][sq2],occupied_squares) and if the result is
 "0" then a sliding piece on sq1 would attack sq2 and vice-versa.
 */
  for (i = 0; i < 64; i++) {
    for (j = 0; j < 64; j++)
      obstructed[i][j] = (BITBOARD) - 1;
    sqs = plus1dir[i];
    while (sqs) {
      j = LSB(sqs);
      directions[i][j] = 1;
      obstructed[i][j] = plus1dir[i] ^ plus1dir[j - 1];
      sqs &= sqs - 1;
    }
    sqs = plus7dir[i];
    while (sqs) {
      j = LSB(sqs);
      directions[i][j] = 7;
      obstructed[i][j] = plus7dir[i] ^ plus7dir[j - 7];
      sqs &= sqs - 1;
    }
    sqs = plus8dir[i];
    while (sqs) {
      j = LSB(sqs);
      directions[i][j] = 8;
      obstructed[i][j] = plus8dir[i] ^ plus8dir[j - 8];
      sqs &= sqs - 1;
    }
    sqs = plus9dir[i];
    while (sqs) {
      j = LSB(sqs);
      directions[i][j] = 9;
      obstructed[i][j] = plus9dir[i] ^ plus9dir[j - 9];
      sqs &= sqs - 1;
    }
    sqs = minus1dir[i];
    while (sqs) {
      j = LSB(sqs);
      directions[i][j] = -1;
      obstructed[i][j] = minus1dir[i] ^ minus1dir[j + 1];
      sqs &= sqs - 1;
    }
    sqs = minus7dir[i];
    while (sqs) {
      j = LSB(sqs);
      directions[i][j] = -7;
      obstructed[i][j] = minus7dir[i] ^ minus7dir[j + 7];
      sqs &= sqs - 1;
    }
    sqs = minus8dir[i];
    while (sqs) {
      j = LSB(sqs);
      directions[i][j] = -8;
      obstructed[i][j] = minus8dir[i] ^ minus8dir[j + 8];
      sqs &= sqs - 1;
    }
    sqs = minus9dir[i];
    while (sqs) {
      j = LSB(sqs);
      directions[i][j] = -9;
      obstructed[i][j] = minus9dir[i] ^ minus9dir[j + 9];
      sqs &= sqs - 1;
    }
  }
}
void InitializeMagic(void)
{
  int i;
  int initmagicmoves_bitpos64_database[64] = {
    63, 0, 58, 1, 59, 47, 53, 2,
    60, 39, 48, 27, 54, 33, 42, 3,
    61, 51, 37, 40, 49, 18, 28, 20,
    55, 30, 34, 11, 43, 14, 22, 4,
    62, 57, 46, 52, 38, 26, 32, 41,
    50, 36, 17, 19, 29, 10, 13, 21,
    56, 45, 25, 31, 35, 16, 9, 12,
    44, 24, 15, 8, 23, 7, 6, 5
  };
  for (i = 0; i < 64; i++) {
    int squares[64];
    int numsquares = 0;
    BITBOARD temp = magic_bishop_mask[i];

    while (temp) {
      BITBOARD abit = temp & -temp;

      squares[numsquares++] =
          initmagicmoves_bitpos64_database[(abit *
              0x07EDD5E59A4E28C2ULL) >> 58];
      temp ^= abit;
    }
    for (temp = 0; temp < (((BITBOARD) (1)) << numsquares); temp++) {
      BITBOARD tempoccupied =
          InitializeMagicOccupied(squares, numsquares, temp);
      *(magic_bishop_indices[i] +
          (((tempoccupied) * magic_bishop[i]) >> magic_bishop_shift[i])) =
          InitializeMagicBishop(i, tempoccupied);
    }
  }
  for (i = 0; i < 64; i++) {
    int squares[64];
    int numsquares = 0;
    BITBOARD temp = magic_rook_mask[i];

    while (temp) {
      BITBOARD abit = temp & -temp;

      squares[numsquares++] =
          initmagicmoves_bitpos64_database[(abit *
              0x07EDD5E59A4E28C2ULL) >> 58];
      temp ^= abit;
    }
    for (temp = 0; temp < (((BITBOARD) (1)) << numsquares); temp++) {
      BITBOARD tempoccupied =
          InitializeMagicOccupied(squares, numsquares, temp);
      *(magic_rook_indices[i] +
          (((tempoccupied) * magic_rook[i]) >> magic_rook_shift[i])) =
          InitializeMagicRook(i, tempoccupied);
    }
  }
}
BITBOARD InitializeMagicBishop(int square, BITBOARD occupied)
{
  BITBOARD ret = 0;
  BITBOARD abit;
  BITBOARD abit2;
  BITBOARD rowbits = (((BITBOARD) 0xFF) << (8 * (square / 8)));

  abit = (((BITBOARD) (1)) << square);
  abit2 = abit;
  do {
    abit <<= 8 - 1;
    abit2 >>= 1;
    if (abit2 & rowbits)
      ret |= abit;
    else
      break;
  } while (abit && !(abit & occupied));
  abit = (((BITBOARD) (1)) << square);
  abit2 = abit;
  do {
    abit <<= 8 + 1;
    abit2 <<= 1;
    if (abit2 & rowbits)
      ret |= abit;
    else
      break;
  } while (abit && !(abit & occupied));
  abit = (((BITBOARD) (1)) << square);
  abit2 = abit;
  do {
    abit >>= 8 - 1;
    abit2 <<= 1;
    if (abit2 & rowbits)
      ret |= abit;
    else
      break;
  } while (abit && !(abit & occupied));
  abit = (((BITBOARD) (1)) << square);
  abit2 = abit;
  do {
    abit >>= 8 + 1;
    abit2 >>= 1;
    if (abit2 & rowbits)
      ret |= abit;
    else
      break;
  } while (abit && !(abit & occupied));
  return (ret);
}

BITBOARD InitializeMagicOccupied(int *squares, int numSquares,
    BITBOARD linoccupied)
{
  int i;
  BITBOARD ret = 0;

  for (i = 0; i < numSquares; i++)
    if (linoccupied & (((BITBOARD) (1)) << i))
      ret |= (((BITBOARD) (1)) << squares[i]);
  return (ret);
}

BITBOARD InitializeMagicRook(int square, BITBOARD occupied)
{
  BITBOARD ret = 0;
  BITBOARD abit;
  BITBOARD rowbits = (((BITBOARD) 0xFF) << (8 * (square / 8)));

  abit = (((BITBOARD) (1)) << square);
  do {
    abit <<= 8;
    ret |= abit;
  } while (abit && !(abit & occupied));
  abit = (((BITBOARD) (1)) << square);
  do {
    abit >>= 8;
    ret |= abit;
  } while (abit && !(abit & occupied));
  abit = (((BITBOARD) (1)) << square);
  do {
    abit <<= 1;
    if (abit & rowbits)
      ret |= abit;
    else
      break;
  } while (!(abit & occupied));
  abit = (((BITBOARD) (1)) << square);
  do {
    abit >>= 1;
    if (abit & rowbits)
      ret |= abit;
    else
      break;
  } while (!(abit & occupied));
  return (ret);
}

void InitializeChessBoard(TREE * tree)
{
  int i;

  if (strlen(initial_position)) {
    int nargs;

    nargs = ReadParse(initial_position, args, " ;");
    SetBoard(tree, nargs, args, 1);
  } else {
    for (i = 0; i < 64; i++)
      PcOnSq(i) = empty;
    Rule50Moves(0) = 0;
    Repetition(black) = 0;
    Repetition(white) = 0;
    wtm = 1;
/*
 place pawns
 */
    for (i = 0; i < 8; i++) {
      PcOnSq(i + 8) = pawn;
      PcOnSq(i + 48) = -pawn;
    }
/*
 place knights
 */
    PcOnSq(B1) = knight;
    PcOnSq(G1) = knight;
    PcOnSq(B8) = -knight;
    PcOnSq(G8) = -knight;
/*
 place bishops
 */
    PcOnSq(C1) = bishop;
    PcOnSq(F1) = bishop;
    PcOnSq(C8) = -bishop;
    PcOnSq(F8) = -bishop;
/*
 place rooks
 */
    PcOnSq(A1) = rook;
    PcOnSq(H1) = rook;
    PcOnSq(A8) = -rook;
    PcOnSq(H8) = -rook;
/*
 place queens
 */
    PcOnSq(D1) = queen;
    PcOnSq(D8) = -queen;
/*
 place kings
 */
    PcOnSq(E1) = king;
    PcOnSq(E8) = -king;
/*
 initialize castling status so all castling is legal.
 */
    Castle(0, black) = 3;
    Castle(0, white) = 3;
/*
 initialize 50 move counter.
 */
    Rule50Moves(black) = 0;
    Rule50Moves(white) = 0;
/*
 initialize enpassant status.
 */
    EnPassant(0) = 0;
/*
 now, set the bit-boards.
 */
    SetChessBitBoards(tree);
  }
}
void SetChessBitBoards(TREE * tree)
{
  int side, piece, square;

  HashKey = 0;
  PawnHashKey = 0;
  Material = 0;
  for (side = black; side <= white; side++)
    for (piece = empty; piece <= king; piece++)
      Pieces(side, piece) = 0;
  for (square = 0; square < 64; square++) {
    if (!PcOnSq(square))
      continue;
    piece = PcOnSq(square);
    side = (piece > 0) ? 1 : 0;
    Pieces(side, Abs(piece)) |= SetMask(square);
    Occupied(side) |= SetMask(square);
    Hash(side, Abs(piece), square);
    if (Abs(piece) == pawn)
      HashP(side, square);
    Material += piece_values[side][Abs(piece)];
  }
  KingSQ(white) = LSB(Pieces(white, king));
  KingSQ(black) = LSB(Pieces(black, king));
  if (EnPassant(0))
    HashEP(EnPassant(0), tree->pos.hash_key);
  if (!(Castle(0, white) & 1))
    HashCastle(0, HashKey, white);
  if (!(Castle(0, white) & 2))
    HashCastle(1, HashKey, white);
  if (!(Castle(0, black) & 1))
    HashCastle(0, HashKey, black);
  if (!(Castle(0, black) & 2))
    HashCastle(1, HashKey, black);
/*
 initialize combination boards that show multiple pieces.
 */
  BishopsQueens =
      Bishops(white) | Bishops(black) | Queens(white) | Queens(black);
  RooksQueens = Rooks(white) | Rooks(black) | Queens(white) | Queens(black);
/*
 initialize black/white piece counts.
 */
  for (side = black; side <= white; side++)
    for (piece = pawn; piece <= king; piece++)
      TotalPieces(side, piece) = PopCnt(Pieces(side, piece));
  for (side = black; side <= white; side++) {
    TotalPieces(side, occupied) = 0;
    for (piece = knight; piece < king; piece++)
      TotalPieces(side, occupied) +=
          PopCnt(Pieces(side, piece)) * p_vals[piece];
  }
  TotalAllPieces = PopCnt(OccupiedSquares);
  Repetition(black) = 0;
  Repetition(white) = 0;
}

/*
 *******************************************************************************
 *                                                                             *
 *   InitlializeFindAttacks() is used to find the attacks from <square> that   *
 *   exist on the 8-bit vector supplied as <pieces>.  <pieces> represents a    *
 *   rank, file or diagonal, based on the rotated bit-boards.                  *
 *                                                                             *
 *******************************************************************************
 */
int InitializeFindAttacks(int square, int pieces, int length)
{
  int result, start;

  result = 0;
/*
 ************************************************************
 *                                                          *
 *   find attacks to left of <square>.                      *
 *                                                          *
 ************************************************************
 */
  if (square < 7) {
    start = 1 << (square + 1);
    while (start < 256) {
      result = result | start;
      if (pieces & start)
        break;
      start = start << 1;
    }
  }
/*
 ************************************************************
 *                                                          *
 *   find attacks to right of <square>.                     *
 *                                                          *
 ************************************************************
 */
  if (square > 0) {
    start = 1 << (square - 1);
    while (start > 0) {
      result = result | start;
      if (pieces & start)
        break;
      start = start >> 1;
    }
  }
  return (result & ((1 << length) - 1));
}
int InitializeGetLogID(void)
{
#if defined(UNIX)
  struct stat *fileinfo = malloc(sizeof(struct stat));
#endif
  int t;

  if (!log_id) {
    for (log_id = 1; log_id < 300; log_id++) {
      sprintf(log_filename, "%s/log.%03d", log_path, log_id);
      sprintf(history_filename, "%s/game.%03d", log_path, log_id);
      log_file = fopen(log_filename, "r");
      if (!log_file)
        break;
      fclose(log_file);
    }
#if defined(UNIX)
/*  a kludge to work around an xboard 4.2.3 problem.  It sends two "quit"
   commands, which causes every other log.nnn file to be empty.  this code
   looks for a very small log.nnn file as the last one, and if it is small,
   then we simply overwrite it to solve this problem temporarily.  this will
   be removed when the nexto xboard version comes out to fix this extra quit
   problem.                                                               */
    {
      char tfn[128];
      FILE *tlog;
      int i;

      sprintf(tfn, "%s/log.%03d", log_path, log_id - 1);
      tlog = fopen(tfn, "r+");
      if (tlog) {
        i = fstat(fileno(tlog), fileinfo);
        if (fileinfo->st_size < 1300)
          log_id--;
      }
    }
#endif
  }
  t = log_id++;
  return (t);
}
void InitializeHashTables(void)
{
  int i, side;

  transposition_id = 0;
  if (!trans_ref)
    return;
  for (i = 0; i < hash_table_size; i++) {
    (trans_ref + i)->prefer.word1 = (BITBOARD) 7 << 61;
    (trans_ref + i)->prefer.word2 = 0;
    (trans_ref + i)->always[0].word1 = (BITBOARD) 7 << 61;
    (trans_ref + i)->always[0].word2 = 0;
    (trans_ref + i)->always[1].word1 = (BITBOARD) 7 << 61;
    (trans_ref + i)->always[1].word2 = 0;
  }
  if (!pawn_hash_table)
    return;
  for (i = 0; i < pawn_hash_table_size; i++) {
    (pawn_hash_table + i)->key = 0;
    (pawn_hash_table + i)->score_mg = 0;
    (pawn_hash_table + i)->score_eg = 0;
    (pawn_hash_table + i)->open_files = 0;
    for (side = black; side <= white; side++) {
      (pawn_hash_table + i)->candidates[side] = 0;
      (pawn_hash_table + i)->defects_k[side] = 0;
      (pawn_hash_table + i)->defects_e[side] = 0;
      (pawn_hash_table + i)->defects_d[side] = 0;
      (pawn_hash_table + i)->defects_q[side] = 0;
      (pawn_hash_table + i)->all[side] = 0;
      (pawn_hash_table + i)->passed[side] = 0;
      (pawn_hash_table + i)->candidates[side] = 0;
    }
  }
}
void InitializeKillers(void)
{
  int i, j;

  for (i = 0; i < MAXPLY; i++) {
    block[0]->killers[i].move1 = 0;
    block[0]->killers[i].move2 = 0;
  }
  for (i = 0; i < 2; i++)
    for (j = 0; j < 128; j++)
      block[0]->rep_list[i][j] = 0;
}

/*
 *******************************************************************************
 *                                                                             *
 *   InitlializeKingSafety() is used to initialize the king safety matrix.     *
 *   this is set so that the matrix, indexed by king safety pawn structure     *
 *   index and by king safety piece tropism, combines the two indices to       *
 *   produce a single score.  as either index rises, the king safety score     *
 *   tracks along, but as both rise, the king safety score rises much more     *
 *   quickly.                                                                  *
 *                                                                             *
 *******************************************************************************
 */
void InitializeKingSafety()
{
  int safety, tropism;

  for (safety = 0; safety < 16; safety++) {
    for (tropism = 0; tropism < 16; tropism++) {
      king_safety[safety][tropism] =
          180 * ((safety_vector[safety] + 100) * (tropism_vector[tropism] +
              100) / 100 - 100) / 100;
    }
  }
/*
  for (safety = 0; safety < 16; safety++) {
    for (tropism = 0; tropism < 16; tropism++) {
      printf("%4d", king_safety[safety][tropism]);
    }
    printf("\n");
  }
*/
}
void InitializeMasks(void)
{
  int i, j;

  mask_clear_entry = (BITBOARD) 0xe7fffffffffe0000ull;
/*
 masks to set/clear a bit on a specific square
 */
  for (i = 0; i < 64; i++) {
    ClearMask(i) = ~((BITBOARD) 1 << i);
    SetMask(i) = (BITBOARD) 1 << i;
  }
  ClearMask(BAD_SQUARE) = 0;
  SetMask(BAD_SQUARE) = 0;
/*
 masks to select bits on a specific rank or file
 */
  rank_mask[0] = (BITBOARD) 255;
  for (i = 1; i < 8; i++)
    rank_mask[i] = rank_mask[i - 1] << 8;
  file_mask[FILEA] = (BITBOARD) 1;
  for (i = 1; i < 8; i++)
    file_mask[FILEA] = file_mask[FILEA] | file_mask[FILEA] << 8;
  for (i = 1; i < 8; i++)
    file_mask[i] = file_mask[i - 1] << 1;
/*
 masks to determine if a pawn is protected by another pawn or not.
 also masks to detect "duos" (pawns side-by-side only).
 */
  for (i = 8; i < 56; i++) {
    if (File(i) > 0 && File(i) < 7) {
      mask_pawn_duo[i] = SetMask(i - 1) | SetMask(i + 1);
      mask_pawn_connected[i] =
          SetMask(i - 1) | SetMask(i + 1) | SetMask(i - 9) | SetMask(i -
          7) | SetMask(i + 7) | SetMask(i + 9);
    } else if (File(i) == 0) {
      mask_pawn_duo[i] = SetMask(i + 1);
      mask_pawn_connected[i] = SetMask(i + 1) | SetMask(i - 7) | SetMask(i + 9);
    } else if (File(i) == 7) {
      mask_pawn_duo[i] = SetMask(i - 1);
      mask_pawn_connected[i] = SetMask(i - 1) | SetMask(i - 9) | SetMask(i + 7);
    }
  }
  mask_fgh = file_mask[FILEF] | file_mask[FILEG] | file_mask[FILEH];
  mask_efgh =
      file_mask[FILEE] | file_mask[FILEF] | file_mask[FILEG] | file_mask[FILEH];
  mask_abc = file_mask[FILEA] | file_mask[FILEB] | file_mask[FILEC];
  mask_abcd =
      file_mask[FILEA] | file_mask[FILEB] | file_mask[FILEC] | file_mask[FILED];
  mask_kr_trapped[black][0] = SetMask(H7);
  mask_kr_trapped[black][1] = SetMask(H8) | SetMask(H7);
  mask_kr_trapped[black][2] = SetMask(H8) | SetMask(G8) | SetMask(H7);
  mask_qr_trapped[black][0] = SetMask(A7);
  mask_qr_trapped[black][1] = SetMask(A8) | SetMask(A7);
  mask_qr_trapped[black][2] = SetMask(A8) | SetMask(B8) | SetMask(A7);
  mask_kr_trapped[white][0] = SetMask(H2);
  mask_kr_trapped[white][1] = SetMask(H1) | SetMask(H2);
  mask_kr_trapped[white][2] = SetMask(G1) | SetMask(H1) | SetMask(H2);
  mask_qr_trapped[white][0] = SetMask(A2);
  mask_qr_trapped[white][1] = SetMask(A1) | SetMask(A2);
  mask_qr_trapped[white][2] = SetMask(A1) | SetMask(B1) | SetMask(A2);
#if !defined(_M_AMD64) && !defined (_M_IA64) && !defined(INLINE32)
  msb[0] = 16;
  lsb[0] = 16;
  for (i = 1; i < 65536; i++) {
    lsb[i] = 16;
    for (j = 0; j < 16; j++)
      if (i & (1 << j)) {
        msb[i] = j;
        if (lsb[i] == 16)
          lsb[i] = j;
      }
  }
#endif
  msb_8bit[0] = 8;
  lsb_8bit[0] = 8;
  pop_cnt_8bit[0] = 0;
  connected_passed[0] = 0;
  for (i = 1; i < 256; i++) {
    pop_cnt_8bit[i] = 0;
    for (j = 0; j < 8; j++)
      if (i & (1 << j))
        pop_cnt_8bit[i]++;
    lsb_8bit[i] = 8;
    for (j = 0; j < 8; j++) {
      if (i & (1 << j)) {
        msb_8bit[i] = j;
        if (lsb_8bit[i] == 8)
          lsb_8bit[i] = j;
      }
    }
  }
}
void InitializePawnMasks(void)
{
  int i, j, k, file;
  BITBOARD m1, m2;

/*
 initialize isolated pawn masks, which are nothing more than 1's on
 the files adjacent to the pawn file.
 */
  for (i = 0; i < 64; i++) {
    if (!File(i))
      mask_pawn_isolated[i] = file_mask[File(i) + 1];
    else if (File(i) == 7)
      mask_pawn_isolated[i] = file_mask[File(i) - 1];
    else
      mask_pawn_isolated[i] = file_mask[File(i) - 1] | file_mask[File(i) + 1];
  }
/*
 initialize passed pawn masks, which are nothing more than 1's on
 the pawn's file and the adjacent files, but only on ranks that are
 in "front" of the pawn.
 */
  for (i = 0; i < 64; i++) {
    if (!File(i)) {
      mask_pawn_passed[white][i] = plus8dir[i] | plus8dir[i + 1];
      mask_pawn_passed[black][i] = minus8dir[i] | minus8dir[i + 1];
    } else if (File(i) == 7) {
      mask_pawn_passed[white][i] = plus8dir[i - 1] | plus8dir[i];
      mask_pawn_passed[black][i] = minus8dir[i - 1] | minus8dir[i];
    } else {
      mask_pawn_passed[white][i] =
          plus8dir[i - 1] | plus8dir[i] | plus8dir[i + 1];
      mask_pawn_passed[black][i] =
          minus8dir[i - 1] | minus8dir[i] | minus8dir[i + 1];
    }
  }
/*
 initialize hidden passed pawn masks, which are nothing more than 1's on
 squares where the opponent can't have pawns so that our "hidden" passed
 will work (say we have pawns on h6 and g5, and our opponent has a pawn on
 h7.  he can't have pawns at g6 or f7 or we can't play g6 and free up our h
 pawn.
 */
  for (i = 0; i < 8; i++) {
    mask_hidden_left[black][i] = 0;
    mask_hidden_right[black][i] = 0;
    mask_hidden_left[white][i] = 0;
    mask_hidden_right[white][i] = 0;
    if (i > 0) {
      mask_hidden_left[white][i] |= SetMask(39 + i) | SetMask(47 + i);
      mask_hidden_left[black][i] |= SetMask(15 + i) | SetMask(7 + i);
    }
    if (i > 1) {
      mask_hidden_left[white][i] |= SetMask(46 + i) | SetMask(38 + i);
      mask_hidden_left[black][i] |= SetMask(6 + i) | SetMask(14 + i);
    }
    if (i < 6) {
      mask_hidden_right[white][i] |= SetMask(50 + i) | SetMask(42 + i);
      mask_hidden_right[black][i] |= SetMask(10 + i) | SetMask(18 + i);
    }
    if (i < 7) {
      mask_hidden_right[white][i] |= SetMask(41 + i) | SetMask(49 + i);
      mask_hidden_right[black][i] |= SetMask(17 + i) | SetMask(9 + i);
    }
  }
/*
 these masks are used to determine if the other side has any pawns
 that can attack [square].
 */
  for (i = 8; i < 56; i++) {
    if (!File(i)) {
      mask_no_pattacks[white][i] = minus8dir[i + 1];
      mask_no_pattacks[black][i] = plus8dir[i + 1];
    } else if (File(i) == 7) {
      mask_no_pattacks[white][i] = minus8dir[i - 1];
      mask_no_pattacks[black][i] = plus8dir[i - 1];
    } else {
      mask_no_pattacks[white][i] = minus8dir[i - 1] | minus8dir[i + 1];
      mask_no_pattacks[black][i] = plus8dir[i + 1] | plus8dir[i - 1];
    }
  }
/*
 enpassant pawns are on either file adjacent to the current file, and
 on the same rank.
 */
  for (i = 0; i < 64; i++)
    mask_eptest[i] = 0;
  for (i = 25; i < 31; i++)
    mask_eptest[i] = SetMask(i - 1) | SetMask(i + 1);
  for (i = 33; i < 39; i++)
    mask_eptest[i] = SetMask(i - 1) | SetMask(i + 1);
  mask_eptest[A4] = SetMask(B4);
  mask_eptest[H4] = SetMask(G4);
  mask_eptest[A5] = SetMask(B5);
  mask_eptest[H5] = SetMask(G5);
/*
   these two masks have 1's on dark squares and light squares
   to test to see if pawns/bishops are on them.
 */
  m1 = (BITBOARD) 1 << 63;
  m2 = m1 >> 1;
  for (i = 1; i < 4; i++) {
    m1 = m1 | m1 >> 2;
    m2 = m2 | m2 >> 2;
  }
  for (i = 0; i < 64; i += 8)
    if ((Rank(i)) & 1) {
      dark_squares = dark_squares | m2 >> i;
    } else {
      dark_squares = dark_squares | m1 >> i;
    }
/*
   these masks are used to detect that one side has pawns, but all
   are rook pawns, or all are now rook pawns.
 */
  not_rook_pawns =
      file_mask[FILEB] | file_mask[FILEC] | file_mask[FILED] | file_mask[FILEE]
      | file_mask[FILEF] | file_mask[FILEG];
  rook_pawns = ~not_rook_pawns;
/*
   these two masks have 1's on everywhere but the left or right
   files, used to prevent pawns from capturing off the edge of
   the board and wrapping around.rom capturing off the edge of
 */
  mask_left_edge = ~file_mask[FILEA];
  mask_right_edge = ~file_mask[FILEH];
  mask_advance_2_w = rank_mask[RANK3];
  mask_advance_2_b = rank_mask[RANK6];
/*
   this mask has 1's everywhere except the a/h file and
   the first/last rank.
 */
  mask_not_edge =
      ~(rank_mask[RANK1] | rank_mask[RANK8] | file_mask[FILEA] |
      file_mask[FILEH]);
/*
   this array is indexed by an 8-bit value that has a one for
   each file with a pawn of a specific color.  it returns the
   number of "pawn islands" based on that file status.
 */
  for (i = 0; i < 256; i++) {
    islands[i] = 0;
    file = 1;
    while (file <= 128) {
      while (!(i & file)) {
        file <<= 1;
        if (file > 128)
          break;
      }
      if (file <= 128) {
        islands[i]++;
        while (i & file) {
          file <<= 1;
          if (file > 128)
            break;
        }
      }
    }
  }
/*
 initialize masks used to evaluate pawn races.  these masks are
 used to determine if the opposing king is in a position to stop a
 passed pawn from racing down and queening.  the data is organized
 as pawn_race[side][onmove][square], where side is black or white,
 and onmove indicates which side is to move for proper tempo 
 evaluation.
 */
  for (i = 0; i < 64; i++) {
    pawn_race[white][white][i] = 0;
    pawn_race[white][black][i] = 0;
    pawn_race[black][white][i] = 0;
    pawn_race[black][black][i] = 0;
  }
  for (j = 8; j < 56; j++) {
    for (i = 0; i < 64; i++) {
/* white pawn, wtm */
      if (j < 16) {
        if (KingPawnSquare(j + 8, i, File(j) + 56, 1))
          pawn_race[white][white][j] |= SetMask(i);
      } else {
        if (KingPawnSquare(j, i, File(j) + 56, 1))
          pawn_race[white][white][j] |= SetMask(i);
      }
/* white pawn, btm */
      if (j < 16) {
        if (KingPawnSquare(j + 8, i, File(j) + 56, 0))
          pawn_race[white][black][j] |= SetMask(i);
      } else {
        if (KingPawnSquare(j, i, File(j) + 56, 0))
          pawn_race[white][black][j] |= SetMask(i);
      }
/* black pawn, wtm */
      if (j > 47) {
        if (KingPawnSquare(j - 8, i, File(j), 0))
          pawn_race[black][white][j] |= SetMask(i);
      } else {
        if (KingPawnSquare(j, i, File(j), 0))
          pawn_race[black][white][j] |= SetMask(i);
      }
/* black pawn, btm */
      if (j > 47) {
        if (KingPawnSquare(j - 8, i, File(j), 1))
          pawn_race[black][black][j] |= SetMask(i);
      } else {
        if (KingPawnSquare(j, i, File(j), 1))
          pawn_race[black][black][j] |= SetMask(i);
      }
    }
  }
/*
  table to detect connected pawns and to determine how far apart
  two "split" passed pawns are.
 */
  for (i = 1; i < 256; i++) {
    connected_passed[i] = 0;
    for (j = 0; j < 8; j++)
      if ((i & (3 << j)) == (3 << j)) {
        connected_passed[i] = j + 1;
        break;
      }
    for (j = 0; j < 8; j++)
      if (i & (1 << j))
        break;
    for (k = 7; k >= 0; k--)
      if (i & (1 << k))
        break;
    if (k > j)
      file_spread[i] = k - j - 1;
    else
      file_spread[i] = 0;
  }
/*
 is_outside[p][a]
 p=8 bit mask for passed pawns
 a=8 bit mask for all pawns on board
 p must have left-most or right-most bit set when compared to
 mask 'a'.  and this bit must be separated from the next bit
 by at least one file (ie the outside passed pawn is 2 files
 from the rest of the pawns, at least.
 ppsq = square that contains a (potential) passed pawn.
 psql = leftmost pawn, period.
 psqr = rightmost pawn, period.
 0 -> passed pawn is not 'outside'
 1 -> passed pawn is 'outside'
 2 -> passed pawn is 'outside' on both sides of board
 */
  for (i = 0; i < 256; i++) {
    for (j = 0; j < 256; j++) {
      int ppsq1, ppsq2, psql, psqr;

      is_outside[i][j] = 0;
      ppsq1 = lsb_8bit[i];
      if (ppsq1 < 8) {
        psql = lsb_8bit[j];
        if (ppsq1 < psql - 1 || psql == 8)
          is_outside[i][j] += 1;
      }
      ppsq2 = msb_8bit[i];
      if (ppsq2 < 8) {
        psqr = msb_8bit[j];
        if (ppsq2 > psqr + 1 || psqr == 8)
          is_outside[i][j] += 1;
      }
      if (ppsq1 == ppsq2 && is_outside[i][j] > 0)
        is_outside[i][j] = 1;
    }
  }
}

/*
 *******************************************************************************
 *                                                                             *
 *   InitializeRandomHash() is called to initialize the tables of random       *
 *   numbers used to produce the incrementally-updated hash keys.  note that   *
 *   this uses a treendom number generator rather than the C library one       *
 *   since there is no uniformity in the number of bits returned by the        *
 *   standard library routines, it varies from 16 bits to 64.                  *
 *                                                                             *
 *******************************************************************************
 */
void InitializeRandomHash(void)
{
  int piece, color, square, i;

  for (square = 0; square < 64; square++)
    for (piece = pawn; piece <= king; piece++)
      for (color = white; color >= black; color--)
        randoms[color][piece][square] = Random64();
  for (i = 0; i < 2; i++) {
    castle_random[1][i] = Random64();
    castle_random[0][i] = Random64();
  }
  enpassant_random[0] = 0;
  for (i = 1; i < 65; i++) {
    enpassant_random[i] = Random64();
  }
  for (i = 0; i < 2; i++) {
    wtm_random[i] = Random64();
  }
}

/*
 *******************************************************************************
 *                                                                             *
 *   InitlializeSMP() is used to initialize the pthread lock variables.        *
 *                                                                             *
 *******************************************************************************
 */
void InitializeSMP(void)
{
  LockInit(lock_smp);
  LockInit(lock_io);
  LockInit(lock_root);
  LockInit(block[0]->lock);
}
