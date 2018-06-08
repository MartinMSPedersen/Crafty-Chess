#include "chess.h"
#include "data.h"
/* last modified 03/03/08 */
/*
 *******************************************************************************
 *                                                                             *
 *   ValidatePosition() is a debugging tool that is enabled by using the       *
 *   -DDEBUG compilation flag.  This procedure tests the various data          *
 *   structures used in Crafty related to the chess board and incrementally    *
 *   updated values like hash signatures and so forth.  It simply looks for    *
 *   consistency between the various bitboards, and recomputes the hash        *
 *   signatures to determine if they are correct.  If anything fails to pass   *
 *   the validation test, we print out a dump of the moves made in this path   *
 *   through the tree, and then exit since things are corrupted.               *
 *                                                                             *
 *   This greatly slows the program down, because ValidatePosition() is called *
 *   after each Make()/Unmake() (these are the functions that modify the       *
 *   primary data structures).  In general, this will not be used by users     *
 *   unless they are modifying the source code themselves.                     *
 *                                                                             *
 *******************************************************************************
 */
void ValidatePosition(TREE * RESTRICT tree, int ply, int move, char *caller) {
  BITBOARD temp, temp1, temp_occ;
  BITBOARD temp_occx;
  int i, square, error;
  int side, piece, temp_score;

/*
 ************************************************************
 *                                                          *
 *  First, test occupied[side] which should match the OR    *
 *  result of all pieces[side].                             *
 *                                                          *
 ************************************************************
 */
  error = 0;
  for (side = black; side <= white; side++) {
    temp_occ =
        Pawns(side) | Knights(side) | Bishops(side) | Rooks(side) |
        Queens(side)
        | Kings(side);
    if (Occupied(side) ^ temp_occ) {
      Print(128, "ERROR %s occupied squares is bad!\n",
          (side) ? "white" : "black");
      Display2BitBoards(temp_occ, Occupied(white));
      error = 1;
    }
  }
/*
 ************************************************************
 *                                                          *
 *  Then test the bishops&queens and rooks&queens bitmaps   *
 *  by ORing all of the correct piece type bitmaps.         *
 *                                                          *
 ************************************************************
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
 ************************************************************
 *                                                          *
 *  Now we do some sanity tests on the actual chess board   *
 *  information.  The first test is to make sure that no    *
 *  bitmap square is set in more than one bitmap, which     *
 *  would imply two different pieces on the same square.    *
 *                                                          *
 ************************************************************
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
 ************************************************************
 *                                                          *
 *  Add up all the pieces (material values) to see if this  *
 *  matches the incrementally updated value.                *
 *                                                          *
 ************************************************************
 */
  temp_score = 0;
  for (side = black; side <= white; side++)
    for (piece = pawn; piece < king; piece++)
      temp_score += PopCnt(Pieces(side, piece)) * piece_values[side][piece];
  if (temp_score != Material) {
    Print(128, "ERROR  material_evaluation is wrong, good=%d, bad=%d\n",
        temp_score, Material);
    error = 1;
  }
/*
 ************************************************************
 *                                                          *
 *  Next, check the incrementally updated piece counts for  *
 *  both sides.  ditto for pawn counts.                     *
 *                                                          *
 ************************************************************
 */
  for (side = black; side <= white; side++) {
    temp_score = 0;
    for (piece = knight; piece < king; piece++)
      temp_score += PopCnt(Pieces(side, piece)) * p_vals[piece];
    if (temp_score != TotalPieces(side, occupied)) {
      Print(128, "ERROR  %s pieces is wrong, good=%d, bad=%d\n",
          (side) ? "white" : "black", temp_score, TotalPieces(side,
              occupied));
      error = 1;
    }
  }
  for (side = black; side <= white; side++) {
    temp_score = PopCnt(Pawns(side));
    if (temp_score != TotalPieces(side, pawn)) {
      Print(128, "ERROR  %s pawns is wrong, good=%d, bad=%d\n",
          (side) ? "white" : "black", temp_score, TotalPieces(side, pawn));
      error = 1;
    }
  }
  i = PopCnt(OccupiedSquares);
  if (i != TotalAllPieces) {
    Print(128, "ERROR!  TotalAllPieces is wrong, correct=%d  bad=%d\n", i,
        TotalAllPieces);
    error = 1;
  }
/*
 ************************************************************
 *                                                          *
 *  Now we cycle through each different chessboard bitmap   *
 *  and verify that each piece in a bitmap matches the same *
 *  piece type in the board[64] array.                      *
 *                                                          *
 ************************************************************
 */
  for (side = black; side <= white; side++)
    for (piece = pawn; piece <= king; piece++) {
      temp = Pieces(side, piece);
      while (temp) {
        square = LSB(temp);
        if (PcOnSq(square) != pieces[side][piece]) {
          Print(128, "ERROR!  board[%d]=%d, should be %d\n", square,
              PcOnSq(square), pieces[side][piece]);
          error = 1;
        }
        temp &= temp - 1;
      }
    }
/*
 ************************************************************
 *                                                          *
 *  And then we look at the board[64] array and make sure   *
 *  that any non-zero piece matches the proper bitmap for   *
 *  that particular piece type.                             *
 *                                                          *
 ************************************************************
 */
  for (i = 0; i < 64; i++) {
    if (!PcOnSq(i))
      continue;
    side = (PcOnSq(i) > 0) ? 1 : 0;
    if (SetMask(i) & Pieces(side, Abs(PcOnSq(i))))
      continue;
    Print(128, "ERROR!  bitboards/board[%d] don't agree!\n", i);
    error = 1;
    break;
  }
/*
 ************************************************************
 *                                                          *
 *  The last chess board test is to make sure that any      *
 *  square that is empty according to board[64] is also     *
 *  empty according to the occupied squares bitmap.         *
 *                                                          *
 ************************************************************
 */
  temp = ~(temp_occ | temp_occx);
  while (temp) {
    square = LSB(temp);
    if (PcOnSq(square)) {
      Print(128, "ERROR!  board[%d]=%d, should be 0\n", square,
          PcOnSq(square));
      error = 1;
    }
    temp &= temp - 1;
  }
/*
 ************************************************************
 *                                                          *
 *  Finally, we re-compute the pawn hash signature and the  *
 *  normal hash signature and verify that they match the    *
 *  incrementally updated values.                           *
 *                                                          *
 ************************************************************
 */
  temp = 0;
  temp1 = 0;
  for (i = 0; i < 64; i++) {
    side = (PcOnSq(i) > 0) ? 1 : 0;
    temp ^= randoms[side][Abs(PcOnSq(i))][i];
    if (Abs(PcOnSq(i)) == pawn)
      temp1 ^= randoms[side][Abs(PcOnSq(i))][i];
  }
  if (EnPassant(ply))
    HashEP(EnPassant(ply), temp);
  for (side = black; side <= white; side++) {
    if (Castle(ply, side) < 0 || !(Castle(ply, side) & 1))
      HashCastle(0, temp, side);
    if (Castle(ply, side) < 0 || !(Castle(ply, side) & 2))
      HashCastle(1, temp, side);
  }
  if (temp ^ HashKey) {
    Print(128, "ERROR!  hash_key is bad.\n");
    error = 1;
  }
  if (temp1 ^ PawnHashKey) {
    Print(128, "ERROR!  pawn_hash_key is bad.\n");
    error = 1;
  }
/*
 ************************************************************
 *                                                          *
 *  If any inconsistencies/errors were found, we are going  *
 *  to dump as much debugging information as possible to    *
 *  help pinpoint the source of the problem.                *
 *                                                          *
 ************************************************************
 */
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
