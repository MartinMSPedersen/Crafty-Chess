#include "chess.h"
#include "data.h"

/* modified 02/19/08 */
/*
 *******************************************************************************
 *                                                                             *
 *   GenerateCaptures() is used to generate capture and pawn promotion moves   *
 *   from the current position.                                                *
 *                                                                             *
 *   the destination square set is the set of squares occupied by opponent     *
 *   pieces, plus the set of squares on the 8th rank that pawns can advance to *
 *   and promote.                                                              *
 *                                                                             *
 *******************************************************************************
 */
int *GenerateCaptures(TREE * RESTRICT tree, int ply, int wtm, int *move)
{
  register BITBOARD target, piecebd, moves;
  register BITBOARD promotions, pcapturesl, pcapturesr;
  register int from, to, temp, common, btm = Flip(wtm);

/*
 ************************************************************
 *                                                          *
 *   now, produce knight moves by cycling through the       *
 *   *_knight board to locate a [from] square and then      *
 *   cycling through knight_attacks[] to locate to squares  *
 *   that a knight on [from] attacks.                       *
 *                                                          *
 ************************************************************
 */
  piecebd = Knights(wtm);
  while (piecebd) {
    from = Advanced(wtm, piecebd);
    moves = knight_attacks[from] & Occupied(btm);
    temp = from + (knight << 12);
    Unpack(wtm, move, moves, temp);
    Clear(from, piecebd);
  }
/*
 ************************************************************
 *                                                          *
 *   now, produce bishop moves by cycling through the       *
 *   *_bishop board to locate a [from] square and then      *
 *   generate the AttacksFrom() bitmap which supplies the   *
 *   list of valid <to> squares.                            *
 *                                                          *
 ************************************************************
 */
  piecebd = Bishops(wtm);
  while (piecebd) {
    from = Advanced(wtm, piecebd);
    moves = AttacksBishop(from, OccupiedSquares) & Occupied(btm);
    temp = from + (bishop << 12);
    Unpack(wtm, move, moves, temp);
    Clear(from, piecebd);
  }
/*
 ************************************************************
 *                                                          *
 *   now, produce rook moves by cycling through the         *
 *   *_rook board to locate a [from] square and then        *
 *   generate the AttacksFrom() bitmap which supplies the   *
 *   list of valid <to> squares.                            *
 *                                                          *
 ************************************************************
 */
  piecebd = Rooks(wtm);
  while (piecebd) {
    from = Advanced(wtm, piecebd);
    moves = AttacksRook(from, OccupiedSquares) & Occupied(btm);
    temp = from + (rook << 12);
    Unpack(wtm, move, moves, temp);
    Clear(from, piecebd);
  }
/*
 ************************************************************
 *                                                          *
 *   now, produce queen moves by cycling through the        *
 *   *_queen board to locate a [from] square and then       *
 *   generate the AttacksFrom() bitmap which supplies the   *
 *   list of valid <to> squares.                            *
 *                                                          *
 ************************************************************
 */
  piecebd = Queens(wtm);
  while (piecebd) {
    from = Advanced(wtm, piecebd);
    moves = AttacksQueen(from, OccupiedSquares) & Occupied(btm);
    temp = from + (queen << 12);
    Unpack(wtm, move, moves, temp);
    Clear(from, piecebd);
  }
/*
 ************************************************************
 *                                                          *
 *   now, produce king moves by cycling through the         *
 *   *_king board to locate a [from] square and then        *
 *   cycling through king_attacks[] to locate to squares    *
 *   that a king on [from] attacks.                         *
 *                                                          *
 ************************************************************
 */
  from = KingSQ(wtm);
  moves = king_attacks[from] & Occupied(btm);
  temp = from + (king << 12);
  Unpack(wtm, move, moves, temp);
/*
 ************************************************************
 *                                                          *
 *   now, produce pawn moves.  this is done differently due *
 *   to inconsistencies in the way a pawn moves when it     *
 *   captures as opposed to normal non-capturing moves.     *
 *   another exception is capturing enpassant.  the first   *
 *   step is to generate all possible pawn promotions.  we  *
 *   do this by masking  all pawns but those on the 7th     *
 *   rank and then advancing them ahead if the square in    *
 *   front is empty.                                        *
 *                                                          *
 ************************************************************
 */
  promotions =
      ((wtm) ? (Pawns(white) & rank_mask[RANK7]) << 8 : (Pawns(black) &
          rank_mask[RANK2]) >> 8) & ~OccupiedSquares;
  while (promotions) {
    to = LSB(promotions);
    *move++ = (to + pawnadv1[wtm]) | (to << 6) | (pawn << 12) | (queen << 18);
    Clear(to, promotions);
  }
  target = Occupied(btm) | EnPassantTarget(ply);
  pcapturesl =
      ((wtm) ? (Pawns(wtm) & mask_left_edge) << 7 : (Pawns(wtm) &
          mask_left_edge) >> 9) & target;
  pcapturesr =
      ((wtm) ? (Pawns(wtm) & mask_right_edge) << 9 : (Pawns(wtm) &
          mask_right_edge) >> 7) & target;
  while (pcapturesl) {
    to = Advanced(wtm, pcapturesl);
    common = (to + capleft[wtm]) | (to << 6) | (pawn << 12);
    if ((wtm == 0 && to > 7) || (wtm == 1 && to < 56))
      *move++ = common | (((PcOnSq(to)) ? Abs(PcOnSq(to)) : pawn) << 15);
    else
      *move++ = common | (Abs(PcOnSq(to)) << 15) | (queen << 18);
    Clear(to, pcapturesl);
  }
  while (pcapturesr) {
    to = Advanced(wtm, pcapturesr);
    common = (to + capright[wtm]) | (to << 6) | (pawn << 12);
    if ((wtm == 0 && to > 7) || (wtm == 1 && to < 56))
      *move++ = common | (((PcOnSq(to)) ? Abs(PcOnSq(to)) : pawn) << 15);
    else
      *move++ = common | (Abs(PcOnSq(to)) << 15) | (queen << 18);
    Clear(to, pcapturesr);
  }
  return (move);
}

/* modified 12/10/07 */
/*
 *******************************************************************************
 *                                                                             *
 *   GenerateCheckEvasions() is used to generate moves when the king is in     *
 *   check.                                                                    *
 *                                                                             *
 *   three types of check-evasion moves are generated:                         *
 *                                                                             *
 *   (1) generate king moves to squares that are not attacked by the opponent's*
 *   pieces.  this includes capture and non-capture moves.                     *
 *                                                                             *
 *   (2) generate interpositions along the rank/file that the checking attack  *
 *   is coming along (assuming (a) only one piece is checking the king, and    *
 *   (b) the checking piece is a sliding piece [bishop, rook, queen]).         *
 *                                                                             *
 *   (3) generate capture moves, but only to the square(s) that are giving     *
 *   check.  captures are a special case.  if there is one checking piece, then*
 *   capturing it by any piece is tried.  if there are two pieces checking the *
 *   king, then the only legal capture to try is for the king to capture one of*
 *   the checking pieces that is on an un-attacked square.                     *
 *                                                                             *
 *******************************************************************************
 */
int *GenerateCheckEvasions(TREE * RESTRICT tree, int ply, int wtm, int *move)
{
  register BITBOARD target, targetc, targetp, piecebd, moves;
  register BITBOARD padvances1, padvances2, pcapturesl, pcapturesr;
  register BITBOARD padvances1_all, empty, checksqs, ep;
  register int from, to, temp, common, btm = Flip(wtm);
  register int king_square, checkers, checking_square;
  register int check_direction1 = 0, check_direction2 = 0;

/*
 ************************************************************
 *                                                          *
 *   first, determine how many pieces are attacking the     *
 *   king and where they are, so we can figure out how to   *
 *   legally get out of check.                              *
 *                                                          *
 ************************************************************
 */
  king_square = KingSQ(wtm);
  checksqs = AttacksTo(tree, king_square) & Occupied(btm);
  checkers = PopCnt(checksqs);
  if (checkers == 1) {
    checking_square = LSB(checksqs);
    if (PcOnSq(checking_square) != pieces[btm][pawn])
      check_direction1 = directions[checking_square][king_square];
    target = InterposeSquares(check_direction1, king_square, checking_square);
    target |= checksqs;
    target |= Kings(btm);
  } else {
    target = Kings(btm);
    checking_square = LSB(checksqs);
    if (PcOnSq(checking_square) != pieces[btm][pawn])
      check_direction1 = directions[checking_square][king_square];
    Clear(checking_square, checksqs);
    checking_square = LSB(checksqs);
    if (PcOnSq(checking_square) != pieces[btm][pawn])
      check_direction2 = directions[checking_square][king_square];
  }
/*
 ************************************************************
 *                                                          *
 *   the next step is to produce the set of valid           *
 *   destination squares.  for king moves, this is simply   *
 *   the set of squares that are not attacked by enemy      *
 *   pieces (if there are any such squares.)                *
 *                                                          *
 *   then, if the checking piece is not a knight, we need   *
 *   to know the checking direction so that we can either   *
 *   move the king "off" of that direction, or else "block" *
 *   that direction.                                        *
 *                                                          *
 *   first produce king moves by cycling through the        *
 *   *_king board to locate a [from] square and then        *
 *   cycling through attacks_to[] to locate to squares      *
 *   that the king on [from] attacks.                       *
 *                                                          *
 ************************************************************
 */
  from = king_square;
  moves = king_attacks[from] & ~Occupied(wtm);
  temp = from + (king << 12);
  while (moves) {
    to = Advanced(wtm, moves);
    if (!Attacked(tree, to, btm)
        && (directions[from][to] != check_direction1)
        && (directions[from][to] != check_direction2))
      *move++ = temp | (to << 6) | (Abs(PcOnSq(to)) << 15);
    Clear(to, moves);
  }
/*
 ************************************************************
 *                                                          *
 *   now, produce knight moves by cycling through the       *
 *   *_knight board to locate a [from] square and then      *
 *   cycling through knight_attacks[] to locate to squares  *
 *   that a knight on [from] attacks.                       *
 *                                                          *
 ************************************************************
 */
  if (checkers == 1) {
    piecebd = Knights(wtm);
    while (piecebd) {
      from = Advanced(wtm, piecebd);
      if (!PinnedOnKing(tree, wtm, from)) {
        moves = knight_attacks[from] & target;
        temp = from + (knight << 12);
        Unpack(wtm, move, moves, temp);
      }
      Clear(from, piecebd);
    }
/*
 ************************************************************
 *                                                          *
 *   now, produce bishop moves by cycling through the       *
 *   *_bishop board to locate a [from] square and then      *
 *   generate the AttacksFrom() bitmap which supplies the   *
 *   list of valid <to> squares.                            *
 *                                                          *
 ************************************************************
 */
    piecebd = Bishops(wtm);
    while (piecebd) {
      from = Advanced(wtm, piecebd);
      if (!PinnedOnKing(tree, wtm, from)) {
        moves = AttacksBishop(from, OccupiedSquares) & target;
        temp = from + (bishop << 12);
        Unpack(wtm, move, moves, temp);
      }
      Clear(from, piecebd);
    }
/*
 ************************************************************
 *                                                          *
 *   now, produce rook moves by cycling through the         *
 *   *_rook board to locate a [from] square and then        *
 *   generate the AttacksFrom() bitmap which supplies the   *
 *   list of valid <to> squares.                            *
 *                                                          *
 ************************************************************
 */
    piecebd = Rooks(wtm);
    while (piecebd) {
      from = Advanced(wtm, piecebd);
      if (!PinnedOnKing(tree, wtm, from)) {
        moves = AttacksRook(from, OccupiedSquares) & target;
        temp = from + (rook << 12);
        Unpack(wtm, move, moves, temp);
      }
      Clear(from, piecebd);
    }
/*
 ************************************************************
 *                                                          *
 *   now, produce queen moves by cycling through the        *
 *   *_queen board to locate a [from] square and then       *
 *   generate the AttacksFrom() bitmap which supplies the   *
 *   list of valid <to> squares.                            *
 *                                                          *
 ************************************************************
 */
    piecebd = Queens(wtm);
    while (piecebd) {
      from = Advanced(wtm, piecebd);
      if (!PinnedOnKing(tree, wtm, from)) {
        moves = AttacksQueen(from, OccupiedSquares) & target;
        temp = from + (queen << 12);
        Unpack(wtm, move, moves, temp);
      }
      Clear(from, piecebd);
    }
/*
 ************************************************************
 *                                                          *
 *   now, produce pawn moves.  this is done differently due *
 *   to inconsistencies in the way a pawn moves when it     *
 *   captures as opposed to normal non-capturing moves.     *
 *   another exception is capturing enpassant.  the first   *
 *   step is to generate all possible pawn moves.  we do    *
 *   this in 2 operations:  (1) shift the pawns forward one *
 *   rank then and with empty squares;  (2) shift the pawns *
 *   forward two ranks and then and with empty squares.     *
 *                                                          *
 ************************************************************
 */
    empty = ~OccupiedSquares;
    targetp = target & empty;
    if (wtm) {
      padvances1 = Pawns(white) << 8 & targetp;
      padvances1_all = Pawns(white) << 8 & empty;
      padvances2 = ((padvances1_all & ((BITBOARD) 255 << 16)) << 8) & targetp;
    } else {
      padvances1 = Pawns(black) >> 8 & targetp;
      padvances1_all = Pawns(black) >> 8 & empty;
      padvances2 = ((padvances1_all & ((BITBOARD) 255 << 40)) >> 8) & targetp;
    }
/*
 ************************************************************
 *                                                          *
 *   now that we got 'em, we simply enumerate the to        *
 *   squares as before, but in four steps since we have     *
 *   four sets of potential moves.                          *
 *                                                          *
 ************************************************************
 */
    while (padvances2) {
      to = Advanced(wtm, padvances2);
      if (!PinnedOnKing(tree, wtm, to + pawnadv2[wtm]))
        *move++ = (to + pawnadv2[wtm]) | (to << 6) | (pawn << 12);
      Clear(to, padvances2);
    }
    while (padvances1) {
      to = Advanced(wtm, padvances1);
      if (!PinnedOnKing(tree, wtm, to + pawnadv1[wtm])) {
        common = (to + pawnadv1[wtm]) | (to << 6) | (pawn << 12);
        if ((wtm == 0 && to > 7) || (wtm == 1 && to < 56))
          *move++ = common;
        else {
          *move++ = common | (queen << 18);
          *move++ = common | (rook << 18);
          *move++ = common | (bishop << 18);
          *move++ = common | (knight << 18);
        }
      }
      Clear(to, padvances1);
    }
    targetc = Occupied(btm) | EnPassantTarget(ply);
    targetc = targetc & target;
    ep = Pawns(btm) & target & ((wtm) ? EnPassantTarget(ply) >> 8 :
        EnPassantTarget(ply) << 8);
    if (Pawns(btm) & target & ((wtm) ? EnPassantTarget(ply) >> 8 :
            EnPassantTarget(ply) << 8))
      targetc = targetc | EnPassantTarget(ply);
    if (wtm) {
      pcapturesl = (Pawns(white) & mask_left_edge) << 7 & targetc;
      pcapturesr = (Pawns(white) & mask_right_edge) << 9 & targetc;
    } else {
      pcapturesl = (Pawns(black) & mask_left_edge) >> 9 & targetc;
      pcapturesr = (Pawns(black) & mask_right_edge) >> 7 & targetc;
    }
    while (pcapturesl) {
      to = Advanced(wtm, pcapturesl);
      if (!PinnedOnKing(tree, wtm, to + capleft[wtm])) {
        common = (to + capleft[wtm]) | (to << 6) | (pawn << 12);
        if ((wtm == 0 && to > 7) || (wtm == 1 && to < 56)) {
          *move++ = common | (((PcOnSq(to)) ? Abs(PcOnSq(to)) : pawn) << 15);
        } else {
          *move++ = common | (Abs(PcOnSq(to)) << 15) | (queen << 18);
          *move++ = common | (Abs(PcOnSq(to)) << 15) | (rook << 18);
          *move++ = common | (Abs(PcOnSq(to)) << 15) | (bishop << 18);
          *move++ = common | (Abs(PcOnSq(to)) << 15) | (knight << 18);
        }
      }
      Clear(to, pcapturesl);
    }
    while (pcapturesr) {
      to = Advanced(wtm, pcapturesr);
      if (!PinnedOnKing(tree, wtm, to + capright[wtm])) {
        common = (to + capright[wtm]) | (to << 6) | (pawn << 12);
        if ((wtm == 0 && to > 7) || (wtm == 1 && to < 56)) {
          *move++ = common | (((PcOnSq(to)) ? Abs(PcOnSq(to)) : pawn) << 15);
        } else {
          *move++ = common | (Abs(PcOnSq(to)) << 15) | (queen << 18);
          *move++ = common | (Abs(PcOnSq(to)) << 15) | (rook << 18);
          *move++ = common | (Abs(PcOnSq(to)) << 15) | (bishop << 18);
          *move++ = common | (Abs(PcOnSq(to)) << 15) | (knight << 18);
        }
      }
      Clear(to, pcapturesr);
    }
  }
  return (move);
}

/*
 *******************************************************************************
 *                                                                             *
 *   InterposeSquares() is used to compute the set of squares that block an    *
 *   attack on the king by a sliding piece, by interposing any piece between   *
 *   the attacking piece and the king on the same ray.                         *
 *                                                                             *
 *******************************************************************************
 */
BITBOARD InterposeSquares(int check_direction, int king_square,
    int checking_square)
{
  register BITBOARD target;

/*
 ************************************************************
 *                                                          *
 *   if this is a check from a single sliding piece, then   *
 *   we can interpose along the checking rank/file/diagonal *
 *   and block the check.  otherwise, interposing is not a  *
 *   possibility.                                           *
 *                                                          *
 ************************************************************
 */
  switch (check_direction) {
  case +1:
    target = plus1dir[king_square - 1] ^ plus1dir[checking_square];
    break;
  case +7:
    target = plus7dir[king_square - 7] ^ plus7dir[checking_square];
    break;
  case +8:
    target = plus8dir[king_square - 8] ^ plus8dir[checking_square];
    break;
  case +9:
    target = plus9dir[king_square - 9] ^ plus9dir[checking_square];
    break;
  case -1:
    target = minus1dir[king_square + 1] ^ minus1dir[checking_square];
    break;
  case -7:
    target = minus7dir[king_square + 7] ^ minus7dir[checking_square];
    break;
  case -8:
    target = minus8dir[king_square + 8] ^ minus8dir[checking_square];
    break;
  case -9:
    target = minus9dir[king_square + 9] ^ minus9dir[checking_square];
    break;
  default:
    target = 0;
    break;
  }
  return (target);
}

/* modified 01/23/08 */
/*
 *******************************************************************************
 *                                                                             *
 *   PinnedOnKing() is used to determine if the piece on <square> is pinned    *
 *   against the king, so that it's illegal to move it.  this is used to cull  *
 *   potential moves by GenerateCheckEvasions() so that illegal moves are not  *
 *   produced.                                                                 *
 *                                                                             *
 *******************************************************************************
 */
int PinnedOnKing(TREE * RESTRICT tree, int wtm, int square)
{
  register int ray;
  register int btm = Flip(wtm);

/*
 ************************************************************
 *                                                          *
 *   first, determine if the piece being moved is on the    *
 *   same diagonal, rank or file as the king.               *
 *                                                          *
 ************************************************************
 */
  ray = directions[square][KingSQ(wtm)];
  if (!ray)
    return (0);
/*
 ************************************************************
 *                                                          *
 *   if they are on the same ray, then determine if the     *
 *   king blocks a bishop attack in one direction from this *
 *   square and a bishop or queen blocks a bishop attack    *
 *   on the same diagonal in the opposite direction.        *
 *                                                          *
 ************************************************************
 */
  switch (abs(ray)) {
  case 1:
    if (AttacksRank(square) & Kings(wtm))
      return ((AttacksRank(square) & RooksQueens & Occupied(btm)) != 0);
    else
      return (0);
  case 7:
    if (AttacksDiagh1(square) & Kings(wtm))
      return ((AttacksDiagh1(square) & BishopsQueens & Occupied(btm)) != 0);
    else
      return (0);
  case 8:
    if (AttacksFile(square) & Kings(wtm))
      return ((AttacksFile(square) & RooksQueens & Occupied(btm)) != 0);
    else
      return (0);
  case 9:
    if (AttacksDiaga1(square) & Kings(wtm))
      return ((AttacksDiaga1(square) & BishopsQueens & Occupied(btm)) != 0);
    else
      return (0);
  }
  return (0);
}

/* modified 02/19/08 */
/*
 *******************************************************************************
 *                                                                             *
 *   GenerateNonCaptures() is used to generate non-capture moves from the      *
 *   current position.                                                         *
 *                                                                             *
 *   once the valid destination squares are known, we have to locate a friendly*
 *   piece to get a attacks_to[] entry.  we then produce the moves for this    *
 *   piece by using the source square as [from] and enumerating each square it *
 *   attacks into [to].                                                        *
 *                                                                             *
 *   pawns are handled differently.  regular pawn moves are produced by        *
 *   shifting the pawn bitmap 8 bits "forward" and anding this with the        *
 *   complement of the occupied squares bitmap  double advances are then       *
 *   produced by anding the pawn bitmap with a mask containing 1's on the      *
 *   second rank, shifting this 16 bits "forward" and then anding this with    *
 *   the complement of the occupied squares bitmap as before.  if [to]         *
 *   reaches the 8th rank, we produce a set of four moves, promoting the pawn  *
 *   to knight, bishop, rook and queen.                                        *
 *                                                                             *
 *******************************************************************************
 */
int *GenerateNonCaptures(TREE * RESTRICT tree, int ply, int wtm, int *move)
{
  register BITBOARD target, piecebd, moves;
  register BITBOARD padvances1, padvances2, pcapturesl, pcapturesr;
  register int from, to, temp, common, btm = Flip(wtm);

/*
 ************************************************************
 *                                                          *
 *   first, produce castling moves if it is legal.          *
 *                                                          *
 ************************************************************
 */
  if (Castle(ply, wtm) > 0) {
    if ((Castle(ply, wtm) & 1) && !(OccupiedSquares & OO[wtm]) &&
        !Attacked(tree, OOsqs[wtm][0], btm) &&
        !Attacked(tree, OOsqs[wtm][1], btm) &&
        !Attacked(tree, OOsqs[wtm][2], btm)) {
      *move++ = (king << 12) + (OOto[wtm] << 6) + OOfrom[wtm];
    }
    if ((Castle(ply, wtm) & 2) && !(OccupiedSquares & OOO[wtm]) &&
        !Attacked(tree, OOOsqs[wtm][0], btm) &&
        !Attacked(tree, OOOsqs[wtm][1], btm) &&
        !Attacked(tree, OOOsqs[wtm][2], btm)) {
      *move++ = (king << 12) + (OOOto[wtm] << 6) + OOfrom[wtm];
    }
  }
/*
 ************************************************************
 *                                                          *
 *   now, produce knight moves by cycling through the       *
 *   *_knight board to locate a [from] square and then      *
 *   cycling through knight_attacks[] to locate to squares  *
 *   that a knight on [from] attacks.                       *
 *                                                          *
 ************************************************************
 */
  target = ~OccupiedSquares;
  piecebd = Knights(wtm);
  while (piecebd) {
    from = Advanced(wtm, piecebd);
    moves = knight_attacks[from] & target;
    temp = from + (knight << 12);
    Unpack(wtm, move, moves, temp);
    Clear(from, piecebd);
  }
/*
 ************************************************************
 *                                                          *
 *   now, produce bishop moves by cycling through the       *
 *   *_bishop board to locate a [from] square and then      *
 *   generate the AttacksFrom() bitmap which supplies the   *
 *   list of valid <to> squares.                            *
 *                                                          *
 ************************************************************
 */
  piecebd = Bishops(wtm);
  while (piecebd) {
    from = Advanced(wtm, piecebd);
    moves = AttacksBishop(from, OccupiedSquares) & target;
    temp = from + (bishop << 12);
    Unpack(wtm, move, moves, temp);
    Clear(from, piecebd);
  }
/*
 ************************************************************
 *                                                          *
 *   now, produce rook moves by cycling through the         *
 *   *_rook board to locate a [from] square and then        *
 *   generate the AttacksFrom() bitmap which supplies the   *
 *   list of valid <to> squares.                            *
 *                                                          *
 ************************************************************
 */
  piecebd = Rooks(wtm);
  while (piecebd) {
    from = Advanced(wtm, piecebd);
    moves = AttacksRook(from, OccupiedSquares) & target;
    temp = from + (rook << 12);
    Unpack(wtm, move, moves, temp);
    Clear(from, piecebd);
  }
/*
 ************************************************************
 *                                                          *
 *   now, produce queen moves by cycling through the        *
 *   *_queen board to locate a [from] square and then       *
 *   generate the AttacksFrom() bitmap which supplies the   *
 *   list of valid <to> squares.                            *
 *                                                          *
 ************************************************************
 */
  piecebd = Queens(wtm);
  while (piecebd) {
    from = Advanced(wtm, piecebd);
    moves = AttacksQueen(from, OccupiedSquares) & target;
    temp = from + (queen << 12);
    Unpack(wtm, move, moves, temp);
    Clear(from, piecebd);
  }
/*
 ************************************************************
 *                                                          *
 *   now, produce king moves by cycling through the         *
 *   *_king board to locate a [from] square and then        *
 *   cycling through king_attacks[] to locate to squares    *
 *   that a king on [from] attacks.                         *
 *                                                          *
 ************************************************************
 */
  from = KingSQ(wtm);
  moves = king_attacks[from] & target;
  temp = from + (king << 12);
  Unpack(wtm, move, moves, temp);
/*
 ************************************************************
 *                                                          *
 *   now, produce pawn moves.  this is done differently due *
 *   to inconsistencies in the way a pawn moves when it     *
 *   captures as opposed to normal non-capturing moves.     *
 *   first we generate all possible pawn moves.  we do      *
 *   this in 4 operations:  (1) shift the pawns forward one *
 *   rank then and with empty squares;  (2) shift the pawns *
 *   forward two ranks and then and with empty squares;     *
 *   (3) remove the a-pawn(s) then shift the pawns          *
 *   diagonally left then and with enemy occupied squares;  *
 *   (4) remove the h-pawn(s) then shift the pawns          *
 *   diagonally right then and with enemy occupied squares. *
 *   note that the only captures produced are under-        *
 *   promotions, because the rest were done in GenCap.      *
 *                                                          *
 ************************************************************
 */
  padvances1 = ((wtm) ? Pawns(wtm) << 8 : Pawns(wtm) >> 8) & target;
  padvances2 =
      ((wtm) ? (padvances1 & mask_advance_2_w) << 8 : (padvances1 &
          mask_advance_2_b) >> 8) & target;
/*
 ************************************************************
 *                                                          *
 *   now that we got 'em, we simply enumerate the to        *
 *   squares as before, but in four steps since we have     *
 *   four sets of potential moves.                          *
 *                                                          *
 ************************************************************
 */
  while (padvances2) {
    to = Advanced(wtm, padvances2);
    *move++ = (to + pawnadv2[wtm]) | (to << 6) | (pawn << 12);
    Clear(to, padvances2);
  }
  while (padvances1) {
    to = Advanced(wtm, padvances1);
    common = (to + pawnadv1[wtm]) | (to << 6) | (pawn << 12);
    if ((wtm == 0 && to > 7) || (wtm == 1 && to < 56))
      *move++ = common;
    else {
      *move++ = common | (rook << 18);
      *move++ = common | (bishop << 18);
      *move++ = common | (knight << 18);
    }
    Clear(to, padvances1);
  }
/*
 ************************************************************
 *                                                          *
 *   generate the rest of the capture/promotions here since *
 *   GenerateCaptures() only generates captures that are    *
 *   promotions to a queen.                                 *
 *                                                          *
 ************************************************************
 */
  target = Occupied(btm) & rank_mask[RANK1 + wtm * 7];
  pcapturesl =
      ((wtm) ? (Pawns(white) & mask_left_edge) << 7 : (Pawns(black) &
          mask_left_edge) >> 9) & target;
  pcapturesr =
      ((wtm) ? (Pawns(white) & mask_right_edge) << 9 : (Pawns(black) &
          mask_right_edge) >> 7) & target;
  while (pcapturesl) {
    to = Advanced(wtm, pcapturesl);
    common = (to + capleft[wtm]) | (to << 6) | (pawn << 12);
    *move++ = common | (Abs(PcOnSq(to)) << 15) | (rook << 18);
    *move++ = common | (Abs(PcOnSq(to)) << 15) | (bishop << 18);
    *move++ = common | (Abs(PcOnSq(to)) << 15) | (knight << 18);
    Clear(to, pcapturesl);
  }
  while (pcapturesr) {
    to = Advanced(wtm, pcapturesr);
    common = (to + capright[wtm]) | (to << 6) | (pawn << 12);
    *move++ = common | (Abs(PcOnSq(to)) << 15) | (rook << 18);
    *move++ = common | (Abs(PcOnSq(to)) << 15) | (bishop << 18);
    *move++ = common | (Abs(PcOnSq(to)) << 15) | (knight << 18);
    Clear(to, pcapturesr);
  }
  return (move);
}
