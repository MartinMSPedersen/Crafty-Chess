#include "chess.h"
#include "data.h"
/* modified 10/28/09 */
/*
 *******************************************************************************
 *                                                                             *
 *   GenerateCaptures() is used to generate capture and pawn promotion moves   *
 *   from the current position.                                                *
 *                                                                             *
 *   The destination square set is the set of squares occupied by opponent     *
 *   pieces, plus the set of squares on the 8th rank that pawns can advance to *
 *   and promote.                                                              *
 *                                                                             *
 *******************************************************************************
 */
int *GenerateCaptures(TREE * RESTRICT tree, int ply, int wtm, int *move) {
  BITBOARD target, piecebd, moves;
  BITBOARD promotions, pcapturesl, pcapturesr;
  int from, to, temp, common, btm = Flip(wtm);

/*
 ************************************************************
 *                                                          *
 *   We produce knight moves by locating the most advanced  *
 *   knight and then using that <from> square as an index   *
 *   into the precomputed knight_attacks data.  We repeat   *
 *   for each knight.                                       *
 *                                                          *
 ************************************************************
 */
  for (piecebd = Knights(wtm); piecebd; Clear(from, piecebd)) {
    from = Advanced(wtm, piecebd);
    moves = knight_attacks[from] & Occupied(btm);
    temp = from + (knight << 12);
    Unpack(wtm, move, moves, temp);
  }
/*
 ************************************************************
 *                                                          *
 *   We produce bishop moves by locating the most advanced  *
 *   bishop and then using that square in a magic multiply  *
 *   move generation to quickly identify all the squares a  *
 *   bishop can reach.  We repeat for each bishop.          *
 *                                                          *
 ************************************************************
 */
  for (piecebd = Bishops(wtm); piecebd; Clear(from, piecebd)) {
    from = Advanced(wtm, piecebd);
    moves = AttacksBishop(from, OccupiedSquares) & Occupied(btm);
    temp = from + (bishop << 12);
    Unpack(wtm, move, moves, temp);
  }
/*
 ************************************************************
 *                                                          *
 *   We produce rook moves by locating the most advanced    *
 *   rook and then using that square in a magic multiply    *
 *   move generation to quickly identify all the squares    *
 *   rook can reach.  We repeat for each rook.              *
 *                                                          *
 ************************************************************
 */
  for (piecebd = Rooks(wtm); piecebd; Clear(from, piecebd)) {
    from = Advanced(wtm, piecebd);
    moves = AttacksRook(from, OccupiedSquares) & Occupied(btm);
    temp = from + (rook << 12);
    Unpack(wtm, move, moves, temp);
  }
/*
 ************************************************************
 *                                                          *
 *   We produce queen moves by locating the most advanced   *
 *   queen and then using that square in a magic multiply   *
 *   move generation to quickly identify all the squares a  *
 *   queen can reach.  We repeat for each queen.            *
 *                                                          *
 ************************************************************
 */
  for (piecebd = Queens(wtm); piecebd; Clear(from, piecebd)) {
    from = Advanced(wtm, piecebd);
    moves = AttacksQueen(from, OccupiedSquares) & Occupied(btm);
    temp = from + (queen << 12);
    Unpack(wtm, move, moves, temp);
  }
/*
 ************************************************************
 *                                                          *
 *   We produce king moves by locating the only king and    *
 *   then using that <from> square as an index into the     *
 *   precomputed king_attacks data.                         *
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
 *   Now, produce pawn moves.  This is done differently due *
 *   to inconsistencies in the way a pawn moves when it     *
 *   captures as opposed to normal non-capturing moves.     *
 *   Another exception is capturing enpassant.  The first   *
 *   step is to generate all possible pawn promotions.  We  *
 *   do this by removing all pawns but those on the 7th     *
 *   rank and then advancing them if the square in front is *
 *   empty.                                                 *
 *                                                          *
 ************************************************************
 */
  promotions =
      ((wtm) ? (Pawns(white) & rank_mask[RANK7]) << 8 : (Pawns(black) &
          rank_mask[RANK2]) >> 8) & ~OccupiedSquares;
  for (; promotions; Clear(to, promotions)) {
    to = LSB(promotions);
    *move++ = (to + pawnadv1[wtm]) | (to << 6) | (pawn << 12) | (queen << 18);
  }
  target = Occupied(btm) | EnPassantTarget(ply);
  pcapturesl =
      ((wtm) ? (Pawns(wtm) & mask_left_edge) << 7 : (Pawns(wtm) &
          mask_left_edge) >> 9) & target;
  pcapturesr =
      ((wtm) ? (Pawns(wtm) & mask_right_edge) << 9 : (Pawns(wtm) &
          mask_right_edge) >> 7) & target;
  for (; pcapturesl; Clear(to, pcapturesl)) {
    to = Advanced(wtm, pcapturesl);
    common = (to + capleft[wtm]) | (to << 6) | (pawn << 12);
    if ((wtm == 0 && to > 7) || (wtm == 1 && to < 56))
      *move++ = common | (((PcOnSq(to)) ? Abs(PcOnSq(to)) : pawn) << 15);
    else
      *move++ = common | (Abs(PcOnSq(to)) << 15) | (queen << 18);
  }
  for (; pcapturesr; Clear(to, pcapturesr)) {
    to = Advanced(wtm, pcapturesr);
    common = (to + capright[wtm]) | (to << 6) | (pawn << 12);
    if ((wtm == 0 && to > 7) || (wtm == 1 && to < 56))
      *move++ = common | (((PcOnSq(to)) ? Abs(PcOnSq(to)) : pawn) << 15);
    else
      *move++ = common | (Abs(PcOnSq(to)) << 15) | (queen << 18);
  }
  return (move);
}

/* modified 10/28/09 */
/*
 *******************************************************************************
 *                                                                             *
 *   GenerateChecks() is used to generate non-capture moves from the current   *
 *   position.                                                                 *
 *                                                                             *
 *   The first pass produces a bitmap that contains the squares a particular   *
 *   piece type would attack if sitting on the square the enemy king sits on.  *
 *   We then use each of these squares as a source and check to see if the     *
 *   same piece type attacks one of these common targets.  If so, we can move  *
 *   that piece to that square and it will directly attack the king.  We do    *
 *   this for pawns, knights, bishops, rooks and queens to produce the set of  *
 *   "direct checking moves."                                                  *
 *                                                                             *
 *   Then we generate discovered checks in two passes, once for diagonal       *
 *   attacks and once for rank/file attacks (we do it in two passes since a    *
 *   rook can't produce a discovered check along a rank or file since it moves *
 *   in that direction as well.  For diagonals, we first generate the bishop   *
 *   attacks from the enemy king square and mask them with the friendly piece  *
 *   occupied squares bitmap.  This gives us a set of up to 4 "blocking        *
 *   pieces" that could be preventing a check.  We then remove them via the    *
 *   "magic move generation" tricks, and see if we now reach friendly bishops  *
 *   or queens on those diagonals.  If we have a friendly blocker, and a       *
 *   friendly diagonal mover behind that blocker, then moving the blocker is   *
 *   a discovered check (and there could be double-checks included but we do   *
 *   not check for that since a single check is good enough).  We repeat this  *
 *   for the ranks/files and we are done.                                      *
 *                                                                             *
 *   For the present, this code does not produce discovered checks by the      *
 *   king since all king moves are not discovered checks because the king can  *
 *   move in the same direction as the piece it blocks and not uncover the     *
 *   attack.  This might be fixed at some point, but it is rare enough to not  *
 *   be an issue except in far endgames.                                       *
 *                                                                             *
 *******************************************************************************
 */
int *GenerateChecks(TREE * RESTRICT tree, int ply, int wtm, int *move) {
  BITBOARD temp_target, target, piecebd, moves;
  BITBOARD padvances1, blockers, checkers;
  int from, to, promote, temp, btm = Flip(wtm);

/*
 *********************************************************************
 *                                                                   *
 *  First pass:  produce direct checks.  For each piece type, we     *
 *  pretend that a piece of that type stands on the square of the    *
 *  king and we generate attacks from that square for that piece.    *
 *  Now, if we can find any piece of that type that attacks one of   *
 *  those squares, then that piece move would deliver a direct       *
 *  check to the enemy king.  Easy, wasn't it?                       *
 *                                                                   *
 *********************************************************************
 */
  target = ~OccupiedSquares;
/*
 ************************************************************
 *                                                          *
 *  Knight direct checks.                                   *
 *                                                          *
 ************************************************************
 */
  temp_target = target & knight_attacks[KingSQ(btm)];
  for (piecebd = Knights(wtm); piecebd; Clear(from, piecebd)) {
    from = Advanced(wtm, piecebd);
    moves = knight_attacks[from] & temp_target;
    temp = from + (knight << 12);
    Unpack(wtm, move, moves, temp);
  }
/*
 ************************************************************
 *                                                          *
 *  Bishop direct checks.                                   *
 *                                                          *
 ************************************************************
 */
  temp_target = target & AttacksBishop(KingSQ(btm), OccupiedSquares);
  for (piecebd = Bishops(wtm); piecebd; Clear(from, piecebd)) {
    from = Advanced(wtm, piecebd);
    moves = AttacksBishop(from, OccupiedSquares) & temp_target;
    temp = from + (bishop << 12);
    Unpack(wtm, move, moves, temp);
  }
/*
 ************************************************************
 *                                                          *
 *  Rook direct checks.                                     *
 *                                                          *
 ************************************************************
 */
  temp_target = target & AttacksRook(KingSQ(btm), OccupiedSquares);
  for (piecebd = Rooks(wtm); piecebd; Clear(from, piecebd)) {
    from = Advanced(wtm, piecebd);
    moves = AttacksRook(from, OccupiedSquares) & temp_target;
    temp = from + (rook << 12);
    Unpack(wtm, move, moves, temp);
  }
/*
 ************************************************************
 *                                                          *
 *  Queen direct checks.                                    *
 *                                                          *
 ************************************************************
 */
  temp_target = target & AttacksQueen(KingSQ(btm), OccupiedSquares);
  for (piecebd = Queens(wtm); piecebd; Clear(from, piecebd)) {
    from = Advanced(wtm, piecebd);
    moves = AttacksQueen(from, OccupiedSquares) & temp_target;
    temp = from + (queen << 12);
    Unpack(wtm, move, moves, temp);
  }
/*
 ************************************************************
 *                                                          *
 *   Pawn direct checks.                                    *
 *                                                          *
 ************************************************************
 */
  temp_target = target & pawn_attacks[btm][KingSQ(btm)];
  padvances1 = ((wtm) ? Pawns(wtm) << 8 : Pawns(wtm) >> 8) & temp_target;
  for (; padvances1; Clear(to, padvances1)) {
    to = Advanced(wtm, padvances1);
    *move++ = (to + pawnadv1[wtm]) | (to << 6) | (pawn << 12);
  }
/*
 *********************************************************************
 *                                                                   *
 *  Second pass:  produce discovered checks.  Here we do things a    *
 *  bit differently.  We first take diagonal movers.  From the enemy *
 *  king's position, we generate diagonal moves to see if any of     *
 *  them end at one of our pieces that does not slide diagonally,    *
 *  such as a rook, knight or pawn.  If we find one, we look on down *
 *  that diagonal to see if we now find a diagonal mover (queen or   *
 *  bishop).  If so, any legal move by this piece (except captures   *
 *  which have already been generated) will be a discovered check    *
 *  that needs to be searched.  We do the same for vertical /        *
 *  horizontal rays that are blocked by pawns, bishops, knights or   *
 *  kings that would hide a discovered check by a rook or queen.     *
 *                                                                   *
 *********************************************************************
 */
/*
 ************************************************************
 *                                                          *
 *   First we look for diagonal discovered attacks.  Once   *
 *   we know which squares hold pieces that create a        *
 *   discovered check when they move, we generate them      *
 *   piece type by piece type.                              *
 *                                                          *
 ************************************************************
 */
  blockers =
      AttacksBishop(KingSQ(btm),
      OccupiedSquares) & (Rooks(wtm) | Knights(wtm) | Pawns(wtm));
  if (blockers) {
    checkers =
        AttacksBishop(KingSQ(btm),
        OccupiedSquares & ~blockers) & (Bishops(wtm) | Queens(wtm));
    if (checkers) {
      if ((plus7dir[KingSQ(btm)] & blockers) &&
          !(plus7dir[KingSQ(btm)] & checkers))
        blockers &= ~plus7dir[KingSQ(btm)];
      if ((plus9dir[KingSQ(btm)] & blockers) &&
          !(plus9dir[KingSQ(btm)] & checkers))
        blockers &= ~plus9dir[KingSQ(btm)];
      if ((minus7dir[KingSQ(btm)] & blockers) &&
          !(minus7dir[KingSQ(btm)] & checkers))
        blockers &= ~minus7dir[KingSQ(btm)];
      if ((minus9dir[KingSQ(btm)] & blockers) &&
          !(minus9dir[KingSQ(btm)] & checkers))
        blockers &= ~minus9dir[KingSQ(btm)];
/*
 ************************************************************
 *                                                          *
 *   Knight discovered checks.                              *
 *                                                          *
 ************************************************************
 */
      target = ~OccupiedSquares;
      temp_target = target & ~knight_attacks[KingSQ(btm)];
      for (piecebd = Knights(wtm) & blockers; piecebd; Clear(from, piecebd)) {
        from = Advanced(wtm, piecebd);
        moves = knight_attacks[from] & temp_target;
        temp = from + (knight << 12);
        Unpack(wtm, move, moves, temp);
      }
/*
 ************************************************************
 *                                                          *
 *   Rook discovered checks.                                *
 *                                                          *
 ************************************************************
 */
      target = ~OccupiedSquares;
      temp_target = target & ~AttacksRook(KingSQ(btm), OccupiedSquares);
      for (piecebd = Rooks(wtm) & blockers; piecebd; Clear(from, piecebd)) {
        from = Advanced(wtm, piecebd);
        moves = AttacksRook(from, OccupiedSquares) & temp_target;
        temp = from + (rook << 12);
        Unpack(wtm, move, moves, temp);
      }
/*
 ************************************************************
 *                                                          *
 *   Pawn discovered checks.                                *
 *                                                          *
 ************************************************************
 */
      piecebd =
          Pawns(wtm) & blockers & ((wtm) ? ~OccupiedSquares >> 8 :
          ~OccupiedSquares << 8);
      for (; piecebd; Clear(from, piecebd)) {
        from = Advanced(wtm, piecebd);
        to = from + pawnadv1[btm];
        if ((wtm && to > 55) || (btm && to < 8))
          promote = queen;
        else
          promote = 0;
        *move++ = from | (to << 6) | (pawn << 12) | (promote << 18);
      }
    }
  }
/*
 ************************************************************
 *                                                          *
 *   Next, we look for rank/file discovered attacks.  Once  *
 *   we know which squares hold pieces that create a        *
 *   discovered check when they move, we generate them      *
 *   piece type by piece type.                              *
 *                                                          *
 ************************************************************
 */
  blockers =
      AttacksRook(KingSQ(btm),
      OccupiedSquares) & (Bishops(wtm) | Knights(wtm) | (Pawns(wtm) &
          rank_mask[Rank(KingSQ(btm))]));
  if (blockers) {
    checkers =
        AttacksRook(KingSQ(btm),
        OccupiedSquares & ~blockers) & (Rooks(wtm) | Queens(wtm));
    if (checkers) {
      if ((plus1dir[KingSQ(btm)] & blockers) &&
          !(plus1dir[KingSQ(btm)] & checkers))
        blockers &= ~plus1dir[KingSQ(btm)];
      if ((plus8dir[KingSQ(btm)] & blockers) &&
          !(plus8dir[KingSQ(btm)] & checkers))
        blockers &= ~plus8dir[KingSQ(btm)];
      if ((minus1dir[KingSQ(btm)] & blockers) &&
          !(minus1dir[KingSQ(btm)] & checkers))
        blockers &= ~minus1dir[KingSQ(btm)];
      if ((minus8dir[KingSQ(btm)] & blockers) &&
          !(minus8dir[KingSQ(btm)] & checkers))
        blockers &= ~minus8dir[KingSQ(btm)];
/*
 ************************************************************
 *                                                          *
 *   Knight discovered checks.                              *
 *                                                          *
 ************************************************************
 */
      target = ~OccupiedSquares;
      temp_target = target & ~knight_attacks[KingSQ(btm)];
      for (piecebd = Knights(wtm) & blockers; piecebd; Clear(from, piecebd)) {
        from = Advanced(wtm, piecebd);
        moves = knight_attacks[from] & temp_target;
        temp = from + (knight << 12);
        Unpack(wtm, move, moves, temp);
      }
/*
 ************************************************************
 *                                                          *
 *   Bishop discovered checks.                              *
 *                                                          *
 ************************************************************
 */
      target = ~OccupiedSquares;
      temp_target = target & ~AttacksBishop(KingSQ(btm), OccupiedSquares);
      for (piecebd = Bishops(wtm) & blockers; piecebd; Clear(from, piecebd)) {
        from = Advanced(wtm, piecebd);
        moves = AttacksBishop(from, OccupiedSquares) & temp_target;
        temp = from + (bishop << 12);
        Unpack(wtm, move, moves, temp);
      }
/*
 ************************************************************
 *                                                          *
 *   Pawn discovered checks.                                *
 *                                                          *
 ************************************************************
 */
      piecebd =
          Pawns(wtm) & blockers & ((wtm) ? ~OccupiedSquares >> 8 :
          ~OccupiedSquares << 8);
      for (; piecebd; Clear(from, piecebd)) {
        from = Advanced(wtm, piecebd);
        to = from + pawnadv1[btm];
        if ((wtm && to > 55) || (btm && to < 8))
          promote = queen;
        else
          promote = 0;
        *move++ = from | (to << 6) | (pawn << 12) | (promote << 18);
      }
    }
  }
  return (move);
}

/* modified 10/28/09 */
/*
 *******************************************************************************
 *                                                                             *
 *   GenerateCheckEvasions() is used to generate moves when the king is in     *
 *   check.                                                                    *
 *                                                                             *
 *   Three types of check-evasion moves are generated:                         *
 *                                                                             *
 *   (1) Generate king moves to squares that are not attacked by the           *
 *   opponent's pieces.  This includes capture and non-capture moves.          *
 *                                                                             *
 *   (2) Generate interpositions along the rank/file that the checking attack  *
 *   is coming along (assuming (a) only one piece is checking the king, and    *
 *   (b) the checking piece is a sliding piece [bishop, rook, queen]).         *
 *                                                                             *
 *   (3) Generate capture moves, but only to the square(s) that are giving     *
 *   check.  Captures are a special case.  If there is one checking piece,     *
 *   then capturing it by any piece is tried.  If there are two pieces         *
 *   checking the king, then the only legal capture to try is for the king to  *
 *   capture one of the checking pieces that is on an un-attacked square.      *
 *                                                                             *
 *******************************************************************************
 */
int *GenerateCheckEvasions(TREE * RESTRICT tree, int ply, int wtm, int *move) {
  BITBOARD target, targetc, targetp, piecebd, moves;
  BITBOARD padvances1, padvances2, pcapturesl, pcapturesr;
  BITBOARD padvances1_all, empty, checksqs;
  int from, to, temp, common, btm = Flip(wtm);
  int king_square, checkers, checking_square;
  int check_direction1 = 0, check_direction2 = 0;

/*
 ************************************************************
 *                                                          *
 *   First, determine how many pieces are attacking the     *
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
    target = InterposeSquares(king_square, checking_square);
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
 *   The next step is to produce the set of valid           *
 *   destination squares.  For king moves, this is simply   *
 *   the set of squares that are not attacked by enemy      *
 *   pieces (if there are any such squares.)                *
 *                                                          *
 *   Then, if the checking piece is not a knight, we need   *
 *   to know the checking direction so that we can either   *
 *   move the king off of that ray, or else block that ray. *
 *                                                          *
 *   We produce king moves by locating the only king and    *
 *   then using that <from> square as an index into the     *
 *   precomputed king_attacks data.                         *
 *                                                          *
 ************************************************************
 */
  from = king_square;
  temp = from + (king << 12);
  for (moves = king_attacks[from] & ~Occupied(wtm); moves; Clear(to, moves)) {
    to = Advanced(wtm, moves);
    if (!Attacks(tree, to, btm)
        && (directions[from][to] != check_direction1)
        && (directions[from][to] != check_direction2))
      *move++ = temp | (to << 6) | (Abs(PcOnSq(to)) << 15);
  }
/*
 ************************************************************
 *                                                          *
 *   We produce knight moves by locating the most advanced  *
 *   knight and then using that <from> square as an index   *
 *   into the precomputed knight_attacks data.  We repeat   *
 *   for each knight.                                       *
 *                                                          *
 ************************************************************
 */
  if (checkers == 1) {
    for (piecebd = Knights(wtm); piecebd; Clear(from, piecebd)) {
      from = Advanced(wtm, piecebd);
      if (!PinnedOnKing(tree, wtm, from)) {
        moves = knight_attacks[from] & target;
        temp = from + (knight << 12);
        Unpack(wtm, move, moves, temp);
      }
    }
/*
 ************************************************************
 *                                                          *
 *   We produce bishop moves by locating the most advanced  *
 *   bishop and then using that square in a magic multiply  *
 *   move generation to quickly identify all the squares a  *
 *   bishop can reach.  We repeat for each bishop.          *
 *                                                          *
 ************************************************************
 */
    for (piecebd = Bishops(wtm); piecebd; Clear(from, piecebd)) {
      from = Advanced(wtm, piecebd);
      if (!PinnedOnKing(tree, wtm, from)) {
        moves = AttacksBishop(from, OccupiedSquares) & target;
        temp = from + (bishop << 12);
        Unpack(wtm, move, moves, temp);
      }
    }
/*
 ************************************************************
 *                                                          *
 *   We produce rook moves by locating the most advanced    *
 *   rook and then using that square in a magic multiply    *
 *   move generation to quickly identify all the squares    *
 *   rook can reach.  We repeat for each rook.              *
 *                                                          *
 ************************************************************
 */
    for (piecebd = Rooks(wtm); piecebd; Clear(from, piecebd)) {
      from = Advanced(wtm, piecebd);
      if (!PinnedOnKing(tree, wtm, from)) {
        moves = AttacksRook(from, OccupiedSquares) & target;
        temp = from + (rook << 12);
        Unpack(wtm, move, moves, temp);
      }
    }
/*
 ************************************************************
 *                                                          *
 *   We produce queen moves by locating the most advanced   *
 *   queen and then using that square in a magic multiply   *
 *   move generation to quickly identify all the squares a  *
 *   queen can reach.  We repeat for each queen.            *
 *                                                          *
 ************************************************************
 */
    for (piecebd = Queens(wtm); piecebd; Clear(from, piecebd)) {
      from = Advanced(wtm, piecebd);
      if (!PinnedOnKing(tree, wtm, from)) {
        moves = AttacksQueen(from, OccupiedSquares) & target;
        temp = from + (queen << 12);
        Unpack(wtm, move, moves, temp);
      }
    }
/*
 ************************************************************
 *                                                          *
 *   Now, produce pawn moves.  This is done differently due *
 *   to inconsistencies in the way a pawn moves when it     *
 *   captures as opposed to normal non-capturing moves.     *
 *   another exception is capturing enpassant.  The first   *
 *   step is to generate all possible pawn moves.  We do    *
 *   this in 2 operations:  (1) shift the pawns forward one *
 *   rank then AND with empty squares;  (2) shift the pawns *
 *   forward two ranks and then AND with empty squares.     *
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
 *   Now that we got 'em, we simply enumerate the to        *
 *   squares as before, but in four steps since we have     *
 *   four sets of potential moves.                          *
 *                                                          *
 ************************************************************
 */
    for (; padvances2; Clear(to, padvances2)) {
      to = Advanced(wtm, padvances2);
      if (!PinnedOnKing(tree, wtm, to + pawnadv2[wtm]))
        *move++ = (to + pawnadv2[wtm]) | (to << 6) | (pawn << 12);
    }
    for (; padvances1; Clear(to, padvances1)) {
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
    }
    targetc = Occupied(btm) | EnPassantTarget(ply);
    targetc = targetc & target;
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
    for (; pcapturesl; Clear(to, pcapturesl)) {
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
    }
    for (; pcapturesr; Clear(to, pcapturesr)) {
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
    }
  }
  return (move);
}

/* modified 10/28/09 */
/*
 *******************************************************************************
 *                                                                             *
 *   GenerateNoncaptures() is used to generate non-capture moves from the      *
 *   current position.                                                         *
 *                                                                             *
 *   Once the valid destination squares are known, we have to locate a friendly*
 *   piece to get a attacks_to[] entry.  We then produce the moves for this    *
 *   piece by using the source square as [from] and enumerating each square it *
 *   attacks into [to].                                                        *
 *                                                                             *
 *   Pawns are handled differently.  Regular pawn moves are produced by        *
 *   shifting the pawn bitmap 8 bits "forward" and anding this with the        *
 *   complement of the occupied squares bitmap  double advances are then       *
 *   produced by anding the pawn bitmap with a mask containing 1's on the      *
 *   second rank, shifting this 16 bits "forward" and then anding this with    *
 *   the complement of the occupied squares bitmap as before.  If [to]         *
 *   reaches the 8th rank, we produce a set of four moves, promoting the pawn  *
 *   to knight, bishop, rook and queen.                                        *
 *                                                                             *
 *******************************************************************************
 */
int *GenerateNoncaptures(TREE * RESTRICT tree, int ply, int wtm, int *move) {
  BITBOARD target, piecebd, moves;
  BITBOARD padvances1, padvances2, pcapturesl, pcapturesr;
  int from, to, temp, common, btm = Flip(wtm);

/*
 ************************************************************
 *                                                          *
 *   First, produce castling moves if it is legal.          *
 *                                                          *
 ************************************************************
 */
  if (Castle(ply, wtm) > 0) {
    if ((Castle(ply, wtm) & 1) && !(OccupiedSquares & OO[wtm]) &&
        !Attacks(tree, OOsqs[wtm][0], btm) &&
        !Attacks(tree, OOsqs[wtm][1], btm)
        && !Attacks(tree, OOsqs[wtm][2], btm)) {
      *move++ = (king << 12) + (OOto[wtm] << 6) + OOfrom[wtm];
    }
    if ((Castle(ply, wtm) & 2) && !(OccupiedSquares & OOO[wtm]) &&
        !Attacks(tree, OOOsqs[wtm][0], btm) &&
        !Attacks(tree, OOOsqs[wtm][1], btm) &&
        !Attacks(tree, OOOsqs[wtm][2], btm)) {
      *move++ = (king << 12) + (OOOto[wtm] << 6) + OOfrom[wtm];
    }
  }
/*
 ************************************************************
 *                                                          *
 *   We produce knight moves by locating the most advanced  *
 *   knight and then using that <from> square as an index   *
 *   into the precomputed knight_attacks data.  We repeat   *
 *   for each knight.                                       *
 *                                                          *
 ************************************************************
 */
  target = ~OccupiedSquares;
  for (piecebd = Knights(wtm); piecebd; Clear(from, piecebd)) {
    from = Advanced(wtm, piecebd);
    moves = knight_attacks[from] & target;
    temp = from + (knight << 12);
    Unpack(wtm, move, moves, temp);
  }
/*
 ************************************************************
 *                                                          *
 *   We produce bishop moves by locating the most advanced  *
 *   bishop and then using that square in a magic multiply  *
 *   move generation to quickly identify all the squares a  *
 *   bishop can reach.  We repeat for each bishop.          *
 *                                                          *
 ************************************************************
 */
  for (piecebd = Bishops(wtm); piecebd; Clear(from, piecebd)) {
    from = Advanced(wtm, piecebd);
    moves = AttacksBishop(from, OccupiedSquares) & target;
    temp = from + (bishop << 12);
    Unpack(wtm, move, moves, temp);
  }
/*
 ************************************************************
 *                                                          *
 *   We produce rook moves by locating the most advanced    *
 *   rook and then using that square in a magic multiply    *
 *   move generation to quickly identify all the squares    *
 *   rook can reach.  We repeat for each rook.              *
 *                                                          *
 ************************************************************
 */
  for (piecebd = Rooks(wtm); piecebd; Clear(from, piecebd)) {
    from = Advanced(wtm, piecebd);
    moves = AttacksRook(from, OccupiedSquares) & target;
    temp = from + (rook << 12);
    Unpack(wtm, move, moves, temp);
  }
/*
 ************************************************************
 *                                                          *
 *   We produce queen moves by locating the most advanced   *
 *   queen and then using that square in a magic multiply   *
 *   move generation to quickly identify all the squares a  *
 *   queen can reach.  We repeat for each queen.            *
 *                                                          *
 ************************************************************
 */
  for (piecebd = Queens(wtm); piecebd; Clear(from, piecebd)) {
    from = Advanced(wtm, piecebd);
    moves = AttacksQueen(from, OccupiedSquares) & target;
    temp = from + (queen << 12);
    Unpack(wtm, move, moves, temp);
  }
/*
 ************************************************************
 *                                                          *
 *   We produce king moves by locating the only king and    *
 *   then using that <from> square as an index into the     *
 *   precomputed king_attacks data.                         *
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
 *   Now, produce pawn moves.  This is done differently due *
 *   to inconsistencies in the way a pawn moves when it     *
 *   captures as opposed to normal non-capturing moves.     *
 *   First we generate all possible pawn moves.  We do      *
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
 *   Now that we got 'em, we simply enumerate the to        *
 *   squares as before, but in four steps since we have     *
 *   four sets of potential moves.                          *
 *                                                          *
 ************************************************************
 */
  for (; padvances2; Clear(to, padvances2)) {
    to = Advanced(wtm, padvances2);
    *move++ = (to + pawnadv2[wtm]) | (to << 6) | (pawn << 12);
  }
  for (; padvances1; Clear(to, padvances1)) {
    to = Advanced(wtm, padvances1);
    common = (to + pawnadv1[wtm]) | (to << 6) | (pawn << 12);
    if ((wtm == 0 && to > 7) || (wtm == 1 && to < 56))
      *move++ = common;
    else {
      *move++ = common | (rook << 18);
      *move++ = common | (bishop << 18);
      *move++ = common | (knight << 18);
    }
  }
/*
 ************************************************************
 *                                                          *
 *   Generate the rest of the capture/promotions here since *
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
  for (; pcapturesl; Clear(to, pcapturesl)) {
    to = Advanced(wtm, pcapturesl);
    common = (to + capleft[wtm]) | (to << 6) | (pawn << 12);
    *move++ = common | (Abs(PcOnSq(to)) << 15) | (rook << 18);
    *move++ = common | (Abs(PcOnSq(to)) << 15) | (bishop << 18);
    *move++ = common | (Abs(PcOnSq(to)) << 15) | (knight << 18);
  }
  for (; pcapturesr; Clear(to, pcapturesr)) {
    to = Advanced(wtm, pcapturesr);
    common = (to + capright[wtm]) | (to << 6) | (pawn << 12);
    *move++ = common | (Abs(PcOnSq(to)) << 15) | (rook << 18);
    *move++ = common | (Abs(PcOnSq(to)) << 15) | (bishop << 18);
    *move++ = common | (Abs(PcOnSq(to)) << 15) | (knight << 18);
  }
  return (move);
}

/* modified 09/23/09 */
/*
 *******************************************************************************
 *                                                                             *
 *   PinnedOnKing() is used to determine if the piece on <square> is pinned    *
 *   against the king, so that it's illegal to move it.  This is used to cull  *
 *   potential moves by GenerateCheckEvasions() so that illegal moves are not  *
 *   produced.                                                                 *
 *                                                                             *
 *******************************************************************************
 */
int PinnedOnKing(TREE * RESTRICT tree, int wtm, int square) {
  int ray;
  int btm = Flip(wtm);

/*
 ************************************************************
 *                                                          *
 *   First, determine if the piece being moved is on the    *
 *   same diagonal, rank or file as the king.  If not, then *
 *   it can't be pinned and we return (0).                  *
 *                                                          *
 ************************************************************
 */
  ray = directions[square][KingSQ(wtm)];
  if (!ray)
    return (0);
/*
 ************************************************************
 *                                                          *
 *   If they are on the same ray, then determine if the     *
 *   king blocks a bishop attack in one direction from this *
 *   square and a bishop or queen blocks a bishop attack    *
 *   on the same diagonal in the opposite direction (or the *
 *   same rank/file for rook/queen.)                        *
 *                                                          *
 ************************************************************
 */
  switch (Abs(ray)) {
    case 1:
      if (AttacksRank(square) & Kings(wtm))
        return ((AttacksRank(square) & (Rooks(btm) | Queens(btm))) != 0);
      else
        return (0);
    case 7:
      if (AttacksDiagh1(square) & Kings(wtm))
        return ((AttacksDiagh1(square) & (Bishops(btm) | Queens(btm))) != 0);
      else
        return (0);
    case 8:
      if (AttacksFile(square) & Kings(wtm))
        return ((AttacksFile(square) & (Rooks(btm) | Queens(btm))) != 0);
      else
        return (0);
    case 9:
      if (AttacksDiaga1(square) & Kings(wtm))
        return ((AttacksDiaga1(square) & (Bishops(btm) | Queens(btm))) != 0);
      else
        return (0);
  }
  return (0);
}
