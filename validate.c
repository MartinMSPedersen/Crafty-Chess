#include <stdio.h>
#include <stdlib.h>
#include "chess.h"
#include "data.h"

void ValidatePosition(TREE * RESTRICT tree, int ply, int move, char *caller)
{
  BITBOARD temp, temp1, temp_occ, temp_occ_rl90, temp_occ_rl45;
  BITBOARD temp_occ_rr45, temp_occx;
  int i, square, error;
  int temp_score;

/*
 first, test w_occupied and b_occupied
 */
  error = 0;
  temp_occ =
      WhitePawns | WhiteKnights | WhiteBishops | WhiteRooks | WhiteQueens |
      WhiteKing;
  if (WhitePieces ^ temp_occ) {
    Print(128, "ERROR white occupied squares is bad!\n");
    Display2BitBoards(temp_occ, WhitePieces);
    error = 1;
  }
  temp_occ =
      BlackPawns | BlackKnights | BlackBishops | BlackRooks | BlackQueens |
      BlackKing;
  if (BlackPieces ^ temp_occ) {
    Print(128, "ERROR black occupied squares is bad!\n");
    Display2BitBoards(temp_occ, BlackPieces);
    error = 1;
  }
/*
 now test rotated occupied bitboards.
 */
  temp_occ_rl90 = 0;
  temp_occ_rl45 = 0;
  temp_occ_rr45 = 0;
  for (i = 0; i < 64; i++) {
    if (PcOnSq(i)) {
      temp_occ_rl90 = temp_occ_rl90 | SetMaskRL90(i);
      temp_occ_rl45 = temp_occ_rl45 | SetMaskRL45(i);
      temp_occ_rr45 = temp_occ_rr45 | SetMaskRR45(i);
    }
  }
  if (OccupiedRL90 ^ temp_occ_rl90) {
    Print(128, "ERROR occupied squares (rotated left 90) is bad!\n");
    Display2BitBoards(temp_occ_rl90, OccupiedRL90);
    error = 1;
  }
  if (OccupiedRL45 ^ temp_occ_rl45) {
    Print(128, "ERROR occupied squares (rotated left 45) is bad!\n");
    Display2BitBoards(temp_occ_rl45, OccupiedRL45);
    error = 1;
  }
  if (OccupiedRR45 ^ temp_occ_rr45) {
    Print(128, "ERROR occupied squares (rotated right 45) is bad!\n");
    Display2BitBoards(temp_occ_rr45, OccupiedRR45);
    error = 1;
  }
/*
 now test bishops_queens and rooks_queens
 */
  temp_occ = WhiteBishops | WhiteQueens | BlackBishops | BlackQueens;
  if (BishopsQueens ^ temp_occ) {
    Print(128, "ERROR bishops_queens is bad!\n");
    Display2BitBoards(temp_occ, BishopsQueens);
    error = 1;
  }
  temp_occ = WhiteRooks | WhiteQueens | BlackRooks | BlackQueens;
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
      WhitePawns ^ WhiteKnights ^ WhiteBishops ^ WhiteRooks ^ WhiteQueens ^
      BlackPawns ^ BlackKnights ^ BlackBishops ^ BlackRooks ^ BlackQueens ^
      WhiteKing ^ BlackKing;
  temp_occx =
      WhitePawns | WhiteKnights | WhiteBishops | WhiteRooks | WhiteQueens |
      BlackPawns | BlackKnights | BlackBishops | BlackRooks | BlackQueens |
      WhiteKing | BlackKing;
  if (temp_occ ^ temp_occx) {
    Print(128, "ERROR two pieces on same square\n");
    error = 1;
  }
/*
 test material_evaluation
 */
  temp_score = PopCnt(WhitePawns) * pawn_value;
  temp_score -= PopCnt(BlackPawns) * pawn_value;
  temp_score += PopCnt(WhiteKnights) * knight_value;
  temp_score -= PopCnt(BlackKnights) * knight_value;
  temp_score += PopCnt(WhiteBishops) * bishop_value;
  temp_score -= PopCnt(BlackBishops) * bishop_value;
  temp_score += PopCnt(WhiteRooks) * rook_value;
  temp_score -= PopCnt(BlackRooks) * rook_value;
  temp_score += PopCnt(WhiteQueens) * queen_value;
  temp_score -= PopCnt(BlackQueens) * queen_value;
  if (temp_score != Material) {
    Print(128, "ERROR  material_evaluation is wrong, good=%d, bad=%d\n",
        temp_score, Material);
    error = 1;
  }
  temp_score = PopCnt(WhiteKnights) * knight_v;
  temp_score += PopCnt(WhiteBishops) * bishop_v;
  temp_score += PopCnt(WhiteRooks) * rook_v;
  temp_score += PopCnt(WhiteQueens) * queen_v;
  if (temp_score != TotalWhitePieces) {
    Print(128, "ERROR  white_pieces is wrong, good=%d, bad=%d\n", temp_score,
        TotalWhitePieces);
    error = 1;
  }
  temp_score = PopCnt(WhiteKnights);
  temp_score += PopCnt(WhiteBishops);
  if (temp_score != WhiteMinors) {
    Print(128, "ERROR  white_minors is wrong, good=%d, bad=%d\n", temp_score,
        WhiteMinors);
    error = 1;
  }
  temp_score = PopCnt(WhiteRooks);
  temp_score += PopCnt(WhiteQueens) * 2;
  if (temp_score != WhiteMajors) {
    Print(128, "ERROR  white_majors is wrong, good=%d, bad=%d\n", temp_score,
        WhiteMajors);
    error = 1;
  }
  temp_score = PopCnt(WhitePawns);
  if (temp_score != TotalWhitePawns) {
    Print(128, "ERROR  white_pawns is wrong, good=%d, bad=%d\n", temp_score,
        TotalWhitePawns);
    error = 1;
  }
  temp_score = PopCnt(BlackKnights) * knight_v;
  temp_score += PopCnt(BlackBishops) * bishop_v;
  temp_score += PopCnt(BlackRooks) * rook_v;
  temp_score += PopCnt(BlackQueens) * queen_v;
  if (temp_score != TotalBlackPieces) {
    Print(128, "ERROR  black_pieces is wrong, good=%d, bad=%d\n", temp_score,
        TotalBlackPieces);
    error = 1;
  }
  temp_score = PopCnt(BlackKnights);
  temp_score += PopCnt(BlackBishops);
  if (temp_score != BlackMinors) {
    Print(128, "ERROR  black_minors is wrong, good=%d, bad=%d\n", temp_score,
        BlackMinors);
    error = 1;
  }
  temp_score = PopCnt(BlackRooks);
  temp_score += PopCnt(BlackQueens) * 2;
  if (temp_score != BlackMajors) {
    Print(128, "ERROR  black_majors is wrong, good=%d, bad=%d\n", temp_score,
        BlackMajors);
    error = 1;
  }
  temp_score = PopCnt(BlackPawns);
  if (temp_score != TotalBlackPawns) {
    Print(128, "ERROR  black_pawns is wrong, good=%d, bad=%d\n", temp_score,
        TotalBlackPawns);
    error = 1;
  }
/*
 now test the board[...] to make sure piece values are correct.
 */
/*
 test pawn locations
 */
  temp = WhitePawns;
  while (temp) {
    square = FirstOne(temp);
    if (PcOnSq(square) != pawn) {
      Print(128, "ERROR!  board[%d]=%d, should be 1\n", square, PcOnSq(square));
      error = 1;
    }
    Clear(square, temp);
  }
  temp = BlackPawns;
  while (temp) {
    square = FirstOne(temp);
    if (PcOnSq(square) != -pawn) {
      Print(128, "ERROR!  board[%d]=%d, should be -1\n", square,
          PcOnSq(square));
      error = 1;
    }
    Clear(square, temp);
  }
/*
 test knight locations
 */
  temp = WhiteKnights;
  while (temp) {
    square = FirstOne(temp);
    if (PcOnSq(square) != knight) {
      Print(128, "ERROR!  board[%d]=%d, should be 2\n", square, PcOnSq(square));
      error = 1;
    }
    Clear(square, temp);
  }
  temp = BlackKnights;
  while (temp) {
    square = FirstOne(temp);
    if (PcOnSq(square) != -knight) {
      Print(128, "ERROR!  board[%d]=%d, should be -2\n", square,
          PcOnSq(square));
      error = 1;
    }
    Clear(square, temp);
  }
/*
 test bishop locations
 */
  temp = WhiteBishops;
  while (temp) {
    square = FirstOne(temp);
    if (PcOnSq(square) != bishop) {
      Print(128, "ERROR!  board[%d]=%d, should be 3\n", square, PcOnSq(square));
      error = 1;
    }
    Clear(square, temp);
  }
  temp = BlackBishops;
  while (temp) {
    square = FirstOne(temp);
    if (PcOnSq(square) != -bishop) {
      Print(128, "ERROR!  board[%d]=%d, should be -3\n", square,
          PcOnSq(square));
      error = 1;
    }
    Clear(square, temp);
  }
/*
 test rook locations
 */
  temp = WhiteRooks;
  while (temp) {
    square = FirstOne(temp);
    if (PcOnSq(square) != rook) {
      Print(128, "ERROR!  board[%d]=%d, should be 4\n", square, PcOnSq(square));
      error = 1;
    }
    Clear(square, temp);
  }
  temp = BlackRooks;
  while (temp) {
    square = FirstOne(temp);
    if (PcOnSq(square) != -rook) {
      Print(128, "ERROR!  board[%d]=%d, should be -4\n", square,
          PcOnSq(square));
      error = 1;
    }
    Clear(square, temp);
  }
/*
 test queen locations
 */
  temp = WhiteQueens;
  while (temp) {
    square = FirstOne(temp);
    if (PcOnSq(square) != queen) {
      Print(128, "ERROR!  board[%d]=%d, should be 5\n", square, PcOnSq(square));
      error = 1;
    }
    Clear(square, temp);
  }
  temp = BlackQueens;
  while (temp) {
    square = FirstOne(temp);
    if (PcOnSq(square) != -queen) {
      Print(128, "ERROR!  board[%d]=%d, should be -5\n", square,
          PcOnSq(square));
      error = 1;
    }
    Clear(square, temp);
  }
/*
 test king locations
 */
  temp = WhiteKing;
  while (temp) {
    square = FirstOne(temp);
    if (PcOnSq(square) != king) {
      Print(128, "ERROR!  board[%d]=%d, should be 6\n", square, PcOnSq(square));
      error = 1;
    }
    if (WhiteKingSQ != square) {
      Print(128, "ERROR!  white_king is %d, should be %d\n", WhiteKingSQ,
          square);
      error = 1;
    }
    Clear(square, temp);
  }
  temp = BlackKing;
  while (temp) {
    square = FirstOne(temp);
    if (PcOnSq(square) != -king) {
      Print(128, "ERROR!  board[%d]=%d, should be -6\n", square,
          PcOnSq(square));
      error = 1;
    }
    if (BlackKingSQ != square) {
      Print(128, "ERROR!  black_king is %d, should be %d\n", BlackKingSQ,
          square);
      error = 1;
    }
    Clear(square, temp);
  }
/*
 test board[i] fully now.
 */
  for (i = 0; i < 64; i++)
    switch (PcOnSq(i)) {
    case -king:
      if (!(BlackKing & SetMask(i))) {
        Print(128, "ERROR!  b_king/board[%d] don't agree!\n", i);
        error = 1;
      }
      break;
    case -queen:
      if (!(BlackQueens & SetMask(i))) {
        Print(128, "ERROR!  b_queen/board[%d] don't agree!\n", i);
        error = 1;
      }
      break;
    case -rook:
      if (!(BlackRooks & SetMask(i))) {
        Print(128, "ERROR!  b_rook/board[%d] don't agree!\n", i);
        error = 1;
      }
      break;
    case -bishop:
      if (!(BlackBishops & SetMask(i))) {
        Print(128, "ERROR!  b_bishop/board[%d] don't agree!\n", i);
        error = 1;
      }
      break;
    case -knight:
      if (!(BlackKnights & SetMask(i))) {
        Print(128, "ERROR!  b_knight/board[%d] don't agree!\n", i);
        error = 1;
      }
      break;
    case -pawn:
      if (!(BlackPawns & SetMask(i))) {
        Print(128, "ERROR!  b_pawn/board[%d] don't agree!\n", i);
        error = 1;
      }
      break;
    case king:
      if (!(WhiteKing & SetMask(i))) {
        Print(128, "ERROR!  w_king/board[%d] don't agree!\n", i);
        error = 1;
      }
      break;
    case queen:
      if (!(WhiteQueens & SetMask(i))) {
        Print(128, "ERROR!  w_queen/board[%d] don't agree!\n", i);
        error = 1;
      }
      break;
    case rook:
      if (!(WhiteRooks & SetMask(i))) {
        Print(128, "ERROR!  w_rook/board[%d] don't agree!\n", i);
        error = 1;
      }
      break;
    case bishop:
      if (!(WhiteBishops & SetMask(i))) {
        Print(128, "ERROR!  w_bishop/board[%d] don't agree!\n", i);
        error = 1;
      }
      break;
    case knight:
      if (!(WhiteKnights & SetMask(i))) {
        Print(128, "ERROR!  w_knight/board[%d] don't agree!\n", i);
        error = 1;
      }
      break;
    case pawn:
      if (!(WhitePawns & SetMask(i))) {
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
    square = FirstOne(temp);
    if (PcOnSq(square)) {
      Print(128, "ERROR!  board[%d]=%d, should be 0\n", square, PcOnSq(square));
      error = 1;
    }
    Clear(square, temp);
  }
/*
 test total piece count now
 */
  i = PopCnt(Occupied);
  if (i != TotalPieces) {
    Print(128, "ERROR!  TotalPieces is wrong, correct=%d  bad=%d\n", i,
        TotalPieces);
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
      temp = temp ^ w_king_random[i];
      break;
    case queen:
      temp = temp ^ w_queen_random[i];
      break;
    case rook:
      temp = temp ^ w_rook_random[i];
      break;
    case bishop:
      temp = temp ^ w_bishop_random[i];
      break;
    case knight:
      temp = temp ^ w_knight_random[i];
      break;
    case pawn:
      temp = temp ^ w_pawn_random[i];
      temp1 = temp1 ^ w_pawn_random[i];
      break;
    case -pawn:
      temp = temp ^ b_pawn_random[i];
      temp1 = temp1 ^ b_pawn_random[i];
      break;
    case -knight:
      temp = temp ^ b_knight_random[i];
      break;
    case -bishop:
      temp = temp ^ b_bishop_random[i];
      break;
    case -rook:
      temp = temp ^ b_rook_random[i];
      break;
    case -queen:
      temp = temp ^ b_queen_random[i];
      break;
    case -king:
      temp = temp ^ b_king_random[i];
      break;
    default:
      break;
    }
  }
  if (EnPassant(ply))
    HashEP(EnPassant(ply), temp);
  if (WhiteCastle(ply) < 0 || !(WhiteCastle(ply) & 1))
    HashCastleW(0, temp);
  if (WhiteCastle(ply) < 0 || !(WhiteCastle(ply) & 2))
    HashCastleW(1, temp);
  if (BlackCastle(ply) < 0 || !(BlackCastle(ply) & 1))
    HashCastleB(0, temp);
  if (BlackCastle(ply) < 0 || !(BlackCastle(ply) & 2))
    HashCastleB(1, temp);
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
      DisplayChessMove("move=", move);
    i = 99999999;
    tree->move_list[i] = 0;
    exit(1);
  }
}
