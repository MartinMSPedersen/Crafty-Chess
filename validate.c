#include "chess.h"
#include "data.h"

void ValidatePosition(TREE * RESTRICT tree, int ply, int move, char *caller)
{
  BITBOARD temp, temp1, temp_occ;
  BITBOARD temp_occx;
  int i, square, error;
  int temp_score;

/*
 first, test occupied[1] and occupied[0]
 */
  error = 0;
  temp_occ =
      Pawns(white) | Knights(white) | Bishops(white) | Rooks(white) |
      Queens(white) | Kings(white);
  if (Occupied(white) ^ temp_occ) {
    Print(128, "ERROR white occupied squares is bad!\n");
    Display2BitBoards(temp_occ, Occupied(white));
    error = 1;
  }
  temp_occ =
      Pawns(black) | Knights(black) | Bishops(black) | Rooks(black) |
      Queens(black) | Kings(black);
  if (Occupied(black) ^ temp_occ) {
    Print(128, "ERROR black occupied squares is bad!\n");
    Display2BitBoards(temp_occ, Occupied(black));
    error = 1;
  }
/*
 now test bishops_queens and rooks_queens
 */
  temp_occ = Bishops(white) | Queens(white) | Bishops(black) | Queens(black);
  if (BishopsQueens ^ temp_occ) {
    Print(128, "ERROR bishops_queens is bad!\n");
    Display2BitBoards(temp_occ, BishopsQueens);
    error = 1;
  }
  temp_occ = Rooks(white) | Queens(white) | Rooks(black) | Queens(black);
  if (RooksQueens ^ temp_occ) {
    Print(128, "ERROR rooks_queens is bad!\n");
    Display2BitBoards(temp_occ, RooksQueens);
    error = 1;
  }
/*
 check individual piece bit-boards to make sure two pieces
 don't occupy the same square (bit)
 */
  temp_occ =
      Pawns(white) ^ Knights(white) ^ Bishops(white) ^ Rooks(white) ^
      Queens(white) ^ Pawns(black) ^ Knights(black) ^ Bishops(black) ^
      Rooks(black) ^ Queens(black) ^ Kings(white) ^ Kings(black);
  temp_occx =
      Pawns(white) | Knights(white) | Bishops(white) | Rooks(white) |
      Queens(white) | Pawns(black) | Knights(black) | Bishops(black) |
      Rooks(black) | Queens(black) | Kings(white) | Kings(black);
  if (temp_occ ^ temp_occx) {
    Print(128, "ERROR two pieces on same square\n");
    error = 1;
  }
/*
 test material_evaluation
 */
  temp_score = PopCnt(Pawns(white)) * pawn_value;
  temp_score -= PopCnt(Pawns(black)) * pawn_value;
  temp_score += PopCnt(Knights(white)) * knight_value;
  temp_score -= PopCnt(Knights(black)) * knight_value;
  temp_score += PopCnt(Bishops(white)) * bishop_value;
  temp_score -= PopCnt(Bishops(black)) * bishop_value;
  temp_score += PopCnt(Rooks(white)) * rook_value;
  temp_score -= PopCnt(Rooks(black)) * rook_value;
  temp_score += PopCnt(Queens(white)) * queen_value;
  temp_score -= PopCnt(Queens(black)) * queen_value;
  if (temp_score != Material) {
    Print(128, "ERROR  material_evaluation is wrong, good=%d, bad=%d\n",
        temp_score, Material);
    error = 1;
  }
  temp_score = PopCnt(Knights(white)) * knight_v;
  temp_score += PopCnt(Bishops(white)) * bishop_v;
  temp_score += PopCnt(Rooks(white)) * rook_v;
  temp_score += PopCnt(Queens(white)) * queen_v;
  if (temp_score != TotalPieces(white)) {
    Print(128, "ERROR  white_pieces is wrong, good=%d, bad=%d\n", temp_score,
        TotalPieces(white));
    error = 1;
  }
  temp_score = PopCnt(Knights(white));
  if (temp_score != TotalKnights(white)) {
    Print(128, "ERROR  TotalKnights(white) is wrong, good=%d, bad=%d\n",
        temp_score, TotalKnights(white));
    error = 1;
  }
  temp_score = PopCnt(Knights(black));
  if (temp_score != TotalKnights(black)) {
    Print(128, "ERROR  TotalKnights(black) is wrong, good=%d, bad=%d\n",
        temp_score, TotalKnights(black));
    error = 1;
  }
  temp_score = PopCnt(Bishops(white));
  if (temp_score != TotalBishops(white)) {
    Print(128, "ERROR  TotalBishops(white) is wrong, good=%d, bad=%d\n",
        temp_score, TotalBishops(white));
    error = 1;
  }
  temp_score = PopCnt(Bishops(black));
  if (temp_score != TotalBishops(black)) {
    Print(128, "ERROR  TotalBishops(black) is wrong, good=%d, bad=%d\n",
        temp_score, TotalBishops(black));
    error = 1;
  }
  temp_score = PopCnt(Rooks(white));
  if (temp_score != TotalRooks(white)) {
    Print(128, "ERROR  TotalRooks(white) is wrong, good=%d, bad=%d\n",
        temp_score, TotalRooks(white));
    error = 1;
  }
  temp_score = PopCnt(Rooks(black));
  if (temp_score != TotalRooks(black)) {
    Print(128, "ERROR  TotalRooks(black) is wrong, good=%d, bad=%d\n",
        temp_score, TotalRooks(black));
    error = 1;
  }
  temp_score = PopCnt(Queens(white));
  if (temp_score != TotalQueens(white)) {
    Print(128, "ERROR  TotalQueens(white) is wrong, good=%d, bad=%d\n",
        temp_score, TotalQueens(white));
    error = 1;
  }
  temp_score = PopCnt(Queens(black));
  if (temp_score != TotalQueens(black)) {
    Print(128, "ERROR  TotalQueens(black) is wrong, good=%d, bad=%d\n",
        temp_score, TotalQueens(black));
    error = 1;
  }

  temp_score = PopCnt(Pawns(white));
  if (temp_score != TotalPawns(white)) {
    Print(128, "ERROR  white_pawns is wrong, good=%d, bad=%d\n", temp_score,
        TotalPawns(white));
    error = 1;
  }
  temp_score = PopCnt(Knights(black)) * knight_v;
  temp_score += PopCnt(Bishops(black)) * bishop_v;
  temp_score += PopCnt(Rooks(black)) * rook_v;
  temp_score += PopCnt(Queens(black)) * queen_v;
  if (temp_score != TotalPieces(black)) {
    Print(128, "ERROR  black_pieces is wrong, good=%d, bad=%d\n", temp_score,
        TotalPieces(black));
    error = 1;
  }
  temp_score = PopCnt(Pawns(black));
  if (temp_score != TotalPawns(black)) {
    Print(128, "ERROR  black_pawns is wrong, good=%d, bad=%d\n", temp_score,
        TotalPawns(black));
    error = 1;
  }
/*
 now test the board[...] to make sure piece values are correct.
 */
/*
 test pawn locations
 */
  temp = Pawns(white);
  while (temp) {
    square = LSB(temp);
    if (PcOnSq(square) != pawn) {
      Print(128, "ERROR!  board[%d]=%d, should be 1\n", square, PcOnSq(square));
      error = 1;
    }
    temp &= temp - 1;
  }
  temp = Pawns(black);
  while (temp) {
    square = LSB(temp);
    if (PcOnSq(square) != -pawn) {
      Print(128, "ERROR!  board[%d]=%d, should be -1\n", square,
          PcOnSq(square));
      error = 1;
    }
    temp &= temp - 1;
  }
/*
 test knight locations
 */
  temp = Knights(white);
  while (temp) {
    square = LSB(temp);
    if (PcOnSq(square) != knight) {
      Print(128, "ERROR!  board[%d]=%d, should be 2\n", square, PcOnSq(square));
      error = 1;
    }
    temp &= temp - 1;
  }
  temp = Knights(black);
  while (temp) {
    square = LSB(temp);
    if (PcOnSq(square) != -knight) {
      Print(128, "ERROR!  board[%d]=%d, should be -2\n", square,
          PcOnSq(square));
      error = 1;
    }
    temp &= temp - 1;
  }
/*
 test bishop locations
 */
  temp = Bishops(white);
  while (temp) {
    square = LSB(temp);
    if (PcOnSq(square) != bishop) {
      Print(128, "ERROR!  board[%d]=%d, should be 3\n", square, PcOnSq(square));
      error = 1;
    }
    temp &= temp - 1;
  }
  temp = Bishops(black);
  while (temp) {
    square = LSB(temp);
    if (PcOnSq(square) != -bishop) {
      Print(128, "ERROR!  board[%d]=%d, should be -3\n", square,
          PcOnSq(square));
      error = 1;
    }
    temp &= temp - 1;
  }
/*
 test rook locations
 */
  temp = Rooks(white);
  while (temp) {
    square = LSB(temp);
    if (PcOnSq(square) != rook) {
      Print(128, "ERROR!  board[%d]=%d, should be 4\n", square, PcOnSq(square));
      error = 1;
    }
    temp &= temp - 1;
  }
  temp = Rooks(black);
  while (temp) {
    square = LSB(temp);
    if (PcOnSq(square) != -rook) {
      Print(128, "ERROR!  board[%d]=%d, should be -4\n", square,
          PcOnSq(square));
      error = 1;
    }
    temp &= temp - 1;
  }
/*
 test queen locations
 */
  temp = Queens(white);
  while (temp) {
    square = LSB(temp);
    if (PcOnSq(square) != queen) {
      Print(128, "ERROR!  board[%d]=%d, should be 5\n", square, PcOnSq(square));
      error = 1;
    }
    temp &= temp - 1;
  }
  temp = Queens(black);
  while (temp) {
    square = LSB(temp);
    if (PcOnSq(square) != -queen) {
      Print(128, "ERROR!  board[%d]=%d, should be -5\n", square,
          PcOnSq(square));
      error = 1;
    }
    temp &= temp - 1;
  }
/*
 test king locations
 */
  temp = Kings(white);
  while (temp) {
    square = LSB(temp);
    if (PcOnSq(square) != king) {
      Print(128, "ERROR!  board[%d]=%d, should be 6\n", square, PcOnSq(square));
      error = 1;
    }
    if (KingSQ(white) != square) {
      Print(128, "ERROR!  white_king is %d, should be %d\n", KingSQ(white),
          square);
      error = 1;
    }
    temp &= temp - 1;
  }
  temp = Kings(black);
  while (temp) {
    square = LSB(temp);
    if (PcOnSq(square) != -king) {
      Print(128, "ERROR!  board[%d]=%d, should be -6\n", square,
          PcOnSq(square));
      error = 1;
    }
    if (KingSQ(black) != square) {
      Print(128, "ERROR!  black_king is %d, should be %d\n", KingSQ(black),
          square);
      error = 1;
    }
    temp &= temp - 1;
  }
/*
 test board[i] fully now.
 */
  for (i = 0; i < 64; i++)
    switch (PcOnSq(i)) {
    case -king:
      if (!(Kings(black) & SetMask(i))) {
        Print(128, "ERROR!  b_king/board[%d] don't agree!\n", i);
        error = 1;
      }
      break;
    case -queen:
      if (!(Queens(black) & SetMask(i))) {
        Print(128, "ERROR!  b_queen/board[%d] don't agree!\n", i);
        error = 1;
      }
      break;
    case -rook:
      if (!(Rooks(black) & SetMask(i))) {
        Print(128, "ERROR!  b_rook/board[%d] don't agree!\n", i);
        error = 1;
      }
      break;
    case -bishop:
      if (!(Bishops(black) & SetMask(i))) {
        Print(128, "ERROR!  b_bishop/board[%d] don't agree!\n", i);
        error = 1;
      }
      break;
    case -knight:
      if (!(Knights(black) & SetMask(i))) {
        Print(128, "ERROR!  b_knight/board[%d] don't agree!\n", i);
        error = 1;
      }
      break;
    case -pawn:
      if (!(Pawns(black) & SetMask(i))) {
        Print(128, "ERROR!  b_pawn/board[%d] don't agree!\n", i);
        error = 1;
      }
      break;
    case king:
      if (!(Kings(white) & SetMask(i))) {
        Print(128, "ERROR!  w_king/board[%d] don't agree!\n", i);
        error = 1;
      }
      break;
    case queen:
      if (!(Queens(white) & SetMask(i))) {
        Print(128, "ERROR!  w_queen/board[%d] don't agree!\n", i);
        error = 1;
      }
      break;
    case rook:
      if (!(Rooks(white) & SetMask(i))) {
        Print(128, "ERROR!  w_rook/board[%d] don't agree!\n", i);
        error = 1;
      }
      break;
    case bishop:
      if (!(Bishops(white) & SetMask(i))) {
        Print(128, "ERROR!  w_bishop/board[%d] don't agree!\n", i);
        error = 1;
      }
      break;
    case knight:
      if (!(Knights(white) & SetMask(i))) {
        Print(128, "ERROR!  w_knight/board[%d] don't agree!\n", i);
        error = 1;
      }
      break;
    case pawn:
      if (!(Pawns(white) & SetMask(i))) {
        Print(128, "ERROR!  w_pawn/board[%d] don't agree!\n", i);
        error = 1;
      }
      break;
    }
/*
 test empty squares now
 */
  temp = ~(temp_occ | temp_occx);
  while (temp) {
    square = LSB(temp);
    if (PcOnSq(square)) {
      Print(128, "ERROR!  board[%d]=%d, should be 0\n", square, PcOnSq(square));
      error = 1;
    }
    temp &= temp - 1;
  }
/*
 test total piece count now
 */
  i = PopCnt(OccupiedSquares);
  if (i != TotalAllPieces) {
    Print(128, "ERROR!  TotalAllPieces is wrong, correct=%d  bad=%d\n", i,
        TotalAllPieces);
    error = 1;
  }
/*
 test hash key
 */
  temp = 0;
  temp1 = 0;
  for (i = 0; i < 64; i++) {
    switch (PcOnSq(i)) {
    case king:
      temp = temp ^ randoms[1][king][i];
      break;
    case queen:
      temp = temp ^ randoms[1][queen][i];
      break;
    case rook:
      temp = temp ^ randoms[1][rook][i];
      break;
    case bishop:
      temp = temp ^ randoms[1][bishop][i];
      break;
    case knight:
      temp = temp ^ randoms[1][knight][i];
      break;
    case pawn:
      temp = temp ^ randoms[1][pawn][i];
      temp1 = temp1 ^ randoms[1][pawn][i];
      break;
    case -pawn:
      temp = temp ^ randoms[0][pawn][i];
      temp1 = temp1 ^ randoms[0][pawn][i];
      break;
    case -knight:
      temp = temp ^ randoms[0][knight][i];
      break;
    case -bishop:
      temp = temp ^ randoms[0][bishop][i];
      break;
    case -rook:
      temp = temp ^ randoms[0][rook][i];
      break;
    case -queen:
      temp = temp ^ randoms[0][queen][i];
      break;
    case -king:
      temp = temp ^ randoms[0][king][i];
      break;
    default:
      break;
    }
  }
  if (EnPassant(ply))
    HashEP(EnPassant(ply), temp);
  if (Castle(ply, white) < 0 || !(Castle(ply, white) & 1))
    HashCastle(0, temp, white);
  if (Castle(ply, white) < 0 || !(Castle(ply, white) & 2))
    HashCastle(1, temp, white);
  if (Castle(ply, black) < 0 || !(Castle(ply, black) & 1))
    HashCastle(0, temp, black);
  if (Castle(ply, black) < 0 || !(Castle(ply, black) & 2))
    HashCastle(1, temp, black);
  if (temp ^ HashKey) {
    Print(128, "ERROR!  hash_key is bad.\n");
    error = 1;
  }
  if (temp1 ^ PawnHashKey) {
    Print(128, "ERROR!  pawn_hash_key is bad.\n");
    error = 1;
  }
  if (error) {
    Print(4095, "processor id: cpu-%d\n", tree->thread_id);
    Print(4095, "current move:\n");
    DisplayChessMove("move=", move);
    DisplayChessBoard(stdout, tree->pos);
    Print(4095, "called from %s, ply=%d\n", caller, ply);
    Print(4095, "node=" BMF "\n", tree->nodes_searched);
    Print(4095, "active path:\n");
    for (i = 1; i <= ply; i++)
      DisplayChessMove("move=", tree->curmv[i]);
    CraftyExit(1);
  }
}
