#include "chess.h"
#include "data.h"

/* last modified 01/20/08 */
/*
 *******************************************************************************
 *                                                                             *
 *   Evaluate() is used to evaluate the chess board.  broadly, it addresses    *
 *   four (4) distinct areas:  (1) material score which is simply a summing of *
 *   piece types multiplied by piece values;  (2) pawn scoring which considers *
 *   placement of pawns and also evaluates passed pawns, particularly in end-  *
 *   game situations;  (3) piece scoring which evaluates the placement of each *
 *   piece as well as things like piece mobility;  (4) king safety which       *
 *   considers the pawn shelter around the king along with material present to *
 *   facilitate an attack.                                                     *
 *                                                                             *
 *******************************************************************************
 */

int Evaluate(TREE * RESTRICT tree, int ply, int wtm, int alpha, int beta)
{
  register PAWN_HASH_ENTRY *ptable;
  register int score, lscore, totalBR, totalPc, cutoff, can_win = 3;
  register int majors, minors, pscore, wop, bop;

/*
 **********************************************************************
 *                                                                    *
 *   initialize.                                                      *
 *                                                                    *
 **********************************************************************
 */
  tree->endgame = (TotalPieces(white) <= EG_MAT ||
      TotalPieces(black) <= EG_MAT);
  tree->Dangerous[white] = (Queens(white) && TotalPieces(white) > 13) ||
      (TotalRooks(white) > 1 && TotalPieces(white) > 15);
  tree->Dangerous[black] = (Queens(black) && TotalPieces(black) > 13) ||
      (TotalRooks(black) > 1 && TotalPieces(black) > 15);
  tree->evaluations++;
  score = (wtm) ? wtm_bonus : -wtm_bonus;
  score += EvaluateMaterial(tree);
#ifdef DEBUGEV
  printf("score[material]=                  %4d\n", score);
#endif
/*
 **********************************************************************
 *                                                                    *
 *   check for draws due to insufficient material and adjust the      *
 *   score as necessary.  this code also handles a special endgame    *
 *   case where one side has only a lone king, and the king has no    *
 *   legal moves.  this has been shown to break a few evaluation      *
 *   terms such as bishop + wrong color rook pawn.  if this case is   *
 *   detected, a drawscore is returned.                               *
 *                                                                    *
 **********************************************************************
 */
  if (TotalPieces(white) < 13 && TotalPieces(black) < 13)
    do {
/*
 ************************************************************
 *                                                          *
 *   if neither side has any pieces, and both sides have    *
 *   non-rookpawns, then either side can win.               *
 *                                                          *
 ************************************************************
 */
      if (TotalPieces(white) == 0 && TotalPieces(black) == 0 &&
          (Pawns(white) & not_rook_pawns && Pawns(black) & not_rook_pawns))
        break;
/*
 ************************************************************
 *                                                          *
 *   if one side is an exchange up, but has no pawns, then  *
 *   that side can not possibly win.                        *
 *                                                          *
 ************************************************************
 */
      majors =
          TotalRooks(white) + 2 * TotalQueens(white) - TotalRooks(black) -
          2 * TotalQueens(black);
      if (majors) {
        minors =
            TotalKnights(white) + TotalBishops(white) - TotalKnights(black) -
            TotalBishops(black);
        if (majors == -minors) {
          if (TotalPawns(black) == 0)
            can_win &= 1;
          if (TotalPawns(white) == 0)
            can_win &= 2;
        }
        if (can_win == 0)
          break;
      }
/*
 ************************************************************
 *                                                          *
 *   now check several special cases, such as bishop + the  *
 *   wrong rook pawn and adjust can_win accordingly.        *
 *                                                          *
 ************************************************************
 */
      if (!EvaluateWinningChances(tree, white))
        can_win &= 2;
      if (!EvaluateWinningChances(tree, black))
        can_win &= 1;
    } while (0);
/*
 **********************************************************************
 *                                                                    *
 *   determine if this position should be evaluated to force mate     *
 *   (neither side has pawns) or if it should be evaluated normally.  *
 *   call EvaluatePawns() to evaluate the current pawn position.      *
 *   this routine modifies the "passed" pawn bit-vector which         *
 *   indicates whether a pawn on each file is passed or not.          *
 *                                                                    *
 *   note the special case of no pawns, one side is ahead in total    *
 *   material, but the game is a hopeless draw.  KRN vs KR is one     *
 *   example.  If EvaluateWinningChances() determines that the side   *
 *   with extra material can not win, the score is pulled closer to a *
 *   draw although it can not collapse completely to the drawscore as *
 *   it is possible to lose KRB vs KR if the KR side lets the king    *
 *   get trapped on the edge of the board.                            *
 *                                                                    *
 **********************************************************************
 */
  tree->all_pawns = Pawns(black) | Pawns(white);
  if (!tree->all_pawns) {
    if (TotalPieces(white) > 3 || TotalPieces(black) > 3) {
      if (Material > 0)
        score += EvaluateMate(tree, white);
      else if (Material < 0)
        score -= EvaluateMate(tree, black);
      if (score > DrawScore(1) && !(can_win & 1))
        score = score / 4;
      if (score < DrawScore(1) && !(can_win & 2))
        score = score / 4;
      return ((wtm) ? score : -score);
    }
  }
/*
 **********************************************************************
 *                                                                    *
 *   now evaluate pawns.  if the pawn hash signature has not changed  *
 *   from the last entry to Evaluate() then we already have every-    *
 *   thing we need in the pawn hash entry.  in this case, we do not   *
 *   need to call EvaluatePawns() at all.  EvaluatePawns() does all   *
 *   of the analysis for information specifically regarding only      *
 *   pawns.  in many cases, it merely records the presence/absence of *
 *   positional pawn feature because that feature also depends on     *
 *   pieces.  note that anything put into EvaluatePawns() can only    *
 *   consider the placement of pawns.  kings or other pieces can not  *
 *   influence the score because those pieces are not hashed into the *
 *   pawn hash signature.  violating this principle leads to lots of  *
 *   very difficult and challenging debugging problems.               *
 *                                                                    *
 **********************************************************************
 */
  if (PawnHashKey == tree->pawn_score.key)
    score += tree->pawn_score.p_score;
  else {
/*
 ************************************************************
 *                                                          *
 *   first check to see if this position has been handled   *
 *   before.  if so, we can skip the work saved in the pawn *
 *   hash table.                                            *
 *                                                          *
 ************************************************************
 */
    ptable = pawn_hash_table + (PawnHashKey & pawn_hash_mask);
    if (ptable->key == PawnHashKey) {
      tree->pawn_score = *ptable;
      pscore = tree->pawn_score.p_score;
    } else {
      tree->pawn_score.key = PawnHashKey;
      tree->pawn_score.all[white] = 0;
      tree->pawn_score.candidates[white] = 0;
      tree->pawn_score.passed[white] = 0;
      tree->pawn_score.weak[white] = 0;
      tree->pawn_score.all[black] = 0;
      tree->pawn_score.candidates[black] = 0;
      tree->pawn_score.passed[black] = 0;
      tree->pawn_score.weak[black] = 0;
      tree->pawn_score.outside = 0;
      tree->pawn_score.protected = 0;
      tree->pawn_score.open_files = 255;
      tree->pawn_score.protected_count = 64;
      tree->pawn_score.hidden[white] = 0;
      tree->pawn_score.hidden[black] = 0;
      pscore = EvaluatePawns(tree, white) - EvaluatePawns(tree, black);
/*
 ************************************************************
 *                                                          *
 *  evaluate outside passed pawns by analyzing the passed   *
 *  pawns for both sides. we use a pre-computed table that  *
 *  can determine if one side has a passed pawn that is to  *
 *  the left of all other pawns, or to the right of all     *
 *  other pawns.  if both seem to have an outside passed    *
 *  pawn, this means one side has one on one side of the    *
 *  board while the other side has a passed pawn on the     *
 *  opposite side of the board.  we handle this by looking  *
 *  to see which king is closer to the opponent's queening  *
 *  square.  if one is significantly closer, that side has  *
 *  an advantage in terms of time.                          *
 *                                                          *
 *  an outside passer is defined as a pawn closer to one    *
 *  edge of the board than _all_ other pawns on the board.  *
 *  and that pawn must have at least one file between it    *
 *  and any other pawns on the board.  this last limitation *
 *  means we have to do a little work here to remove our    *
 *  own pawn from the test if (say) we have a passed pawn   *
 *  on the a-file, and one of our pawns on the b-file.      *
 *                                                          *
 *  we repeat for candidate passed pawns as well.           *
 *                                                          *
 *  tree->pawn_score.outside is a bitmap with 8 bits:       *
 *                                                          *
 *        xxxx xxx1   (1) -> white has outside passer       *
 *        xxxx xx1x   (2) -> white has 2 outside passers    *
 *        xxxx x1xx   (4) -> black has outside passer       *
 *        xxxx 1xxx   (8) -> black has 2 outside passers    *
 *        xxx1 xxxx  (16) -> white has outside candidate    *
 *        xx1x xxxx  (32) -> white has 2 outside candidates *
 *        x1xx xxxx  (64) -> black has outside candidate    *
 *        1xxx xxxx (128) -> black has 2 outside candidates *
 *                                                          *
 ************************************************************
 */
      wop =
          is_outside[tree->pawn_score.passed[white]][tree->pawn_score.
          all[black]];
      bop =
          is_outside[tree->pawn_score.passed[black]][tree->pawn_score.
          all[white]];
      if (wop || bop) {
        if (wop == 0 || bop == 0) {
          if (wop > 1)
            tree->pawn_score.outside |= 2;
          else if (wop)
            tree->pawn_score.outside |= 1;
          if (bop > 1)
            tree->pawn_score.outside |= 8;
          else if (bop)
            tree->pawn_score.outside |= 4;
        }
      }
      wop =
          is_outside_c[tree->pawn_score.candidates[white]][tree->pawn_score.
          all[black]];
      bop =
          is_outside_c[tree->pawn_score.candidates[black]][tree->pawn_score.
          all[white]];
      if (wop || bop) {
        if (wop == 0 || bop == 0) {
          if (wop > 1)
            tree->pawn_score.outside |= 32;
          else if (wop)
            tree->pawn_score.outside |= 16;
          if (bop > 1)
            tree->pawn_score.outside |= 128;
          else if (bop)
            tree->pawn_score.outside |= 64;
        }
      }
/*
 ************************************************************
 *                                                          *
 *   store the results in the pawn hash table for reuse at  *
 *   a later time as needed.                                *
 *                                                          *
 ************************************************************
 */
      tree->pawn_score.p_score = pscore;
      *ptable = tree->pawn_score;
    }
    score += pscore;
  }
/*
 **********************************************************************
 *                                                                    *
 *  Give protected pawns a small bonus scaled toward end-game.        *
 *                                                                    *
 **********************************************************************
 */
  score += (protected_pawn_eg_bonus * (tree->pawn_score.protected_count - 64))
      * (62 - (TotalPieces(white) + TotalPieces(black))) / 62;
#ifdef DEBUGEV
  printf("score[pawns]=                     %4d\n", score);
#endif
/*
 **********************************************************************
 *                                                                    *
 *   if there are any passed pawns, first call EvaluatePassedPawns()  *
 *   to evaluate them.  then, if one side has a passed pawn and the   *
 *   other side has no pieces, call EvaluatePassedPawnRaces() to see  *
 *   if the passed pawn can be stopped from promoting.  finally, we   *
 *   use tree->pawn_score.outside to see if one side has an outside   *
 *   passed pawn that represents a nearly won endgame advantage. note *
 *   that a protected passed pawn for the opponent negates this       *
 *   scoring term completely.  note that these procedures do not do   *
 *   any hashing, so they can safely consider piece locations in      *
 *   addition to the pawn structure info computed previously.         *
 *                                                                    *
 **********************************************************************
 */
  if (tree->pawn_score.passed[black] || tree->pawn_score.passed[white] ||
      tree->pawn_score.outside) {
    score +=
        ScaleEG(EvaluatePassedPawns(tree, white) - EvaluatePassedPawns(tree,
            black));
    if ((TotalPieces(white) == 0 && tree->pawn_score.passed[black]) ||
        (TotalPieces(black) == 0 && tree->pawn_score.passed[white]))
      score += EvaluatePassedPawnRaces(tree, wtm);
  }
#ifdef DEBUGEV
  printf("score[passed pawns]=              %4d\n", score);
#endif
/*
 **********************************************************************
 *                                                                    *
 *   call EvaluateDevelopment() to evaluate development.  note that   *
 *   we only do this when either side has not castled at the root.    *
 *                                                                    *
 **********************************************************************
 */
  if (Castle(1, white))
    score += ScaleMG(EvaluateDevelopment(tree, ply, white));
  if (Castle(1, black))
    score -= ScaleMG(EvaluateDevelopment(tree, ply, black));
#ifdef DEBUGEV
  printf("score[development]=               %4d\n", score);
#endif
/*
 **********************************************************************
 *                                                                    *
 *   now evaluate pieces.                                             *
 *                                                                    *
 **********************************************************************
 */
  tree->tropism[white] = 0;
  tree->tropism[black] = 0;
  if (can_win != 3) {
    score += EvaluateAll(tree, white) - EvaluateAll(tree, black);
    score += EvaluateKnights(tree, white) - EvaluateKnights(tree, black);
    score += EvaluateBishops(tree, white) - EvaluateBishops(tree, black);
    score += EvaluateRooks(tree, white) - EvaluateRooks(tree, black);
    score += EvaluateQueens(tree, white) - EvaluateQueens(tree, black);
    score += EvaluateKings(tree, ply, white) - EvaluateKings(tree, ply, black);
    score += EvaluateMobility(tree, white) - EvaluateMobility(tree, black);
  } else {
    lscore = (wtm) ? score : -score;
    totalBR =
        TotalBishops(white) + TotalBishops(black) + TotalRooks(white) +
        TotalRooks(black);
    totalPc =
        totalBR + TotalKnights(white) + TotalQueens(white) * 2 +
        TotalKnights(black) + TotalQueens(black) * 2;
    cutoff = 300 + totalPc * 4;
    if (!((lscore - cutoff >= beta) || (lscore + cutoff <= alpha))) {
      score += EvaluateAll(tree, white) - EvaluateAll(tree, black);
      score += EvaluateKnights(tree, white) - EvaluateKnights(tree, black);
      score += EvaluateBishops(tree, white) - EvaluateBishops(tree, black);
      score += EvaluateRooks(tree, white) - EvaluateRooks(tree, black);
      score += EvaluateQueens(tree, white) - EvaluateQueens(tree, black);
      score +=
          EvaluateKings(tree, ply, white) - EvaluateKings(tree, ply, black);
      if (totalBR) {
        lscore = (wtm) ? score : -score;
        cutoff = 12 + totalBR * 16;
        if (TotalBishops(white) > 1 || TotalBishops(black) > 1)
          cutoff += (TotalBishops(white) != TotalBishops(black)) ? 64 : 32;
        if (!((lscore - cutoff >= beta) || (lscore + cutoff <= alpha))) {
          score +=
              EvaluateMobility(tree, white) - EvaluateMobility(tree, black);
        }
      }
    }
  }
#ifdef DEBUGEV
  printf("score[pieces]=                    %4d\n", score);
#endif
/*
 **********************************************************************
 *                                                                    *
 *   now adjust the score if the game is drawish but one side appears *
 *   to be significantly better according to the computed score.      *
 *                                                                    *
 **********************************************************************
 */
  score = EvaluateDraws(tree, ply, can_win, score);
  return ((wtm) ? score : -score);
}

/* last modified 02/08/08 */
/*
 *******************************************************************************
 *                                                                             *
 *   EvaluateAll() is used to evaluate general board dynamics involving        *
 *   multiple pieces.                                                          *
 *                                                                             *
 *******************************************************************************
 */
int EvaluateAll(TREE * RESTRICT tree, int side)
{
  register int score = 0;

/*
 ************************************************************
 *                                                          *
 *   check for blocked center pawns at D2, E2, D7, or E7    *
 *   as that is very cramping regardless of what piece      *
 *   is blocking it.                                        *
 *                                                          *
 ************************************************************
 */
  if (!tree->endgame) {
    if (tree->all_pawns & virgin_center_pawns) {
      if (PcOnSq(sqflip[side][D2]) == pieces[side][pawn] &&
          PcOnSq(sqflip[side][D3]))
        score -= blocked_center_pawn;
      if (PcOnSq(sqflip[side][E2]) == pieces[side][pawn] &&
          PcOnSq(sqflip[side][E3]))
        score -= blocked_center_pawn;
    }
/*
 ************************************************************
 *                                                          *
 *   check for an undeveloped knight/rook combo             *
 *                                                          *
 ************************************************************
 */
    if (PcOnSq(sqflip[side][B1]) == pieces[side][knight] &&
        PcOnSq(sqflip[side][A1]) == pieces[side][rook])
      score -= undeveloped_piece;
    if (PcOnSq(sqflip[side][G1]) == pieces[side][knight] &&
        PcOnSq(sqflip[side][H1]) == pieces[side][rook])
      score -= undeveloped_piece;
  }
/*
 ************************************************************
 *                                                          *
 *   check for the existance of a slider when pawns are     *
 *   present on both wings.                                 *
 *                                                          *
 ************************************************************
 */
  if (tree->all_pawns & mask_fgh && tree->all_pawns & mask_abc) {
    if (Rooks(side) || Bishops(side))
      score += slider_with_wing_pawns;
  }
  return score;
}

/* last modified 01/16/08 */
/*
 *******************************************************************************
 *                                                                             *
 *   EvaluateBishops() is used to evaluate black/white bishops.                *
 *                                                                             *
 *******************************************************************************
 */
int EvaluateBishops(TREE * RESTRICT tree, int side)
{
  register BITBOARD temp;
  register int square, trop, score = 0;
  register int enemy = Flip(side);

/*
 ************************************************************
 *                                                          *
 *   first, locate each bishop and add in its mobility/     *
 *   square score.                                          *
 *                                                          *
 ************************************************************
 */
  temp = Bishops(side);
  while (temp) {
    square = LSB(temp);
/*
 ************************************************************
 *                                                          *
 *   check to see if the bishop is trapped at a7 or h7 with *
 *   a pawn at b6 or g6 that can advance one square and     *
 *   trap the bishop, or a pawn at b6 or g6 that has        *
 *   trapped the bishop already.  also test for the bishop  *
 *   at b8 or g8 as that might not be an escape.            *
 *                                                          *
 ************************************************************
 */
    if (square == sqflip[side][A7] && SetMask(sqflip[side][B6]) & Pawns(enemy))
      score -= bishop_trapped;
    else if (square == sqflip[side][B8] &&
        SetMask(sqflip[side][C7]) & Pawns(enemy))
      score -= bishop_trapped;
    else if (square == sqflip[side][H7] &&
        SetMask(sqflip[side][G6]) & Pawns(enemy))
      score -= bishop_trapped;
    else if (square == sqflip[side][G8] &&
        SetMask(sqflip[side][F7]) & Pawns(enemy))
      score -= bishop_trapped;
/*
 ************************************************************
 *                                                          *
 *   adjust the tropism count for this piece.               *
 *                                                          *
 ************************************************************
 */
    if (tree->Dangerous[side]) {
      trop =
          (AttacksBishop(square,
              OccupiedSquares & ~(Queens(side) | (Bishops(side)))) &
          king_attacks[KingSQ(enemy)]) ? 1 : Distance(square, KingSQ(enemy));
      score -= trop * gen_trop - gen_trop_mid;
      tree->tropism[side] += king_tropism_b[trop];
    }
    temp &= temp - 1;
  }
#ifdef DEBUGEV
  printf("score[bishops(%d)]=                %4d\n", side, score);
  printf("tropism[bishops(%d)]=              %4d\n", side,
      tree->tropism[white]);
#endif
  return (score);
}

/* last modified 01/21/08 */
/*
 *******************************************************************************
 *                                                                             *
 *   EvaluateDevelopment() is used to encourage the program to develop its     *
 *   pieces before moving its queen.  standard developmental principles are    *
 *   applied.  they include:  (1) don't move the queen until minor pieces are  *
 *   developed;  (2) advance the center pawns as soon as possible;  (3) don't  *
 *   move the king unless its a castling move.                                 *
 *                                                                             *
 *******************************************************************************
 */
int EvaluateDevelopment(TREE * RESTRICT tree, int ply, int side)
{
  register int score = 0;
  register int enemy = Flip(side);

/*
 ************************************************************
 *                                                          *
 *   first, some "thematic" things, which includes don't    *
 *   block the c-pawn in queen-pawn openings.               *
 *                                                          *
 ************************************************************
 */
  if (!(SetMask(sqflip[side][E4]) & Pawns(side)) &&
      SetMask(sqflip[side][D4]) & Pawns(side)) {
    if (SetMask(sqflip[side][C2]) & Pawns(side) &&
        SetMask(sqflip[side][C3]) & (Knights(side) | Bishops(side)))
      score -= development_thematic;
  }
#ifdef DEBUGDV
  printf("development(%d).1 score=%d\n", side, score);
#endif
/*
 ************************************************************
 *                                                          *
 *   if the king hasn't moved at the beginning of the       *
 *   search, but it has moved somewhere in the current      *
 *   search path, make *sure* it's a castle move or else    *
 *   penalize the loss of castling privilege.               *
 *                                                          *
 ************************************************************
 */
  if (Castle(1, side) > 0) {
    if (Castle(ply, side) != Castle(1, side)) {
      register int oq;

      oq = (Queens(enemy)) ? 3 : 1;
      if (Castle(ply, side) == 0)
        score -= oq * development_losing_castle;
      else if (Castle(ply, side) > 0)
        score -= (oq * development_losing_castle) / 2;
    } else
      score -= development_not_castled;
  }
#ifdef DEBUGDV
  printf("development(%d).2 score=%d\n", side, score);
#endif
  return (score);
}

/* last modified 02/08/08 */
/*
 *******************************************************************************
 *                                                                             *
 *   EvaluateDraws() is used to adjust the score based on whether the side     *
 *   that appears to be better according the computed score can actually win   *
 *   the game or not.  if the answer is "no" then the score is reduced         *
 *   significantly to reflect the lack of winning chances.                     *
 *                                                                             *
 *******************************************************************************
 */
int EvaluateDraws(TREE * RESTRICT tree, int ply, int can_win, int score)
{
/*
 ************************************************************
 *                                                          *
 *   if the ending has only bishops of opposite colors, the *
 *   score is pulled closer to a draw.  if the score says   *
 *   one side is winning, but that side doesn't have enough *
 *   material to win, the score is set to DRAW.             *
 *                                                          *
 *   if this is a pure BOC ending, it is very drawish un-   *
 *   less one side has at least 4 pawns.  more pawns makes  *
 *   it harder for a bishop and king to stop them all from  *
 *   advancing.                                             *
 *                                                          *
 ************************************************************
 */
  if (TotalPieces(white) <= 8 && TotalPieces(black) <= 8) {
    if (TotalBishops(white) == 1 && TotalBishops(black) == 1) {
      if (square_color[LSB(Bishops(black))] !=
          square_color[LSB(Bishops(white))]) {
        if (TotalPieces(white) == 3 && TotalPieces(black) == 3 &&
            ((TotalPawns(white) < 4 && TotalPawns(black) < 4) ||
                abs(TotalPawns(white) - TotalPawns(black)) < 2))
          score = score / 2;
        else if (TotalPieces(white) == TotalPieces(black))
          score = ((score - Material) / 2) + Material;
      }
    }
  }
  if (can_win != 3) {
    if (can_win != 1 && score > DrawScore(1))
      score = DrawScore(1);
    else if (can_win != 2 && score < DrawScore(1))
      score = DrawScore(1);
  }
/*
 ************************************************************
 *                                                          *
 *   if we are running into the 50-move rule, then start    *
 *   dragging the score toward draw.  this is the idea of a *
 *   "weariness factor" as mentioned by Dave Slate many     *
 *   times.  this avoids slamming into a draw at move 50    *
 *   and having to move something quickly, rather than      *
 *   slowly discovering that the score is dropping and that *
 *   pushing a pawn or capturing something will cause it to *
 *   go back to its correct value a bit more smoothly.      *
 *                                                          *
 ************************************************************
 */
  if (Rule50Moves(ply) > 80) {
    int scale = 101 - Rule50Moves(ply);

    score = DrawScore(1) + score * scale / 20;
  }
#ifdef DEBUGEV
  printf("score[draws]=                     %4d\n", score);
#endif
  return (score);
}

/* last modified 02/13/08 */
/*
 *******************************************************************************
 *                                                                             *
 *   EvaluateHasOpposition() is used to determine if one king stands in        *
 *   "opposition" to the other.  if the kings are opposed on the same file or  *
 *   else are opposed on the same diagonal, then the side not-to-move has the  *
 *   opposition and the side-to-move must give way.                            *
 *                                                                             *
 *******************************************************************************
 */
int EvaluateHasOpposition(int on_move, int king, int enemy_king)
{
  register int file_distance, rank_distance;

  file_distance = FileDistance(king, enemy_king);
  rank_distance = RankDistance(king, enemy_king);
  if (rank_distance < 2)
    return (1);
  if (on_move) {
    if (rank_distance & 1) {
      rank_distance--;
      if (file_distance & 1)
        file_distance--;
    } else if (file_distance & 1) {
      file_distance--;
      if (rank_distance & 1)
        rank_distance--;
    }
  }
  if (!(file_distance & 1) && !(rank_distance & 1))
    return (1);
  return (0);
}

/* last modified 02/07/08 */
/*
 *******************************************************************************
 *                                                                             *
 *   EvaluateKings() is used to evaluate black/white kings.                    *
 *                                                                             *
 *******************************************************************************
 */
int EvaluateKings(TREE * RESTRICT tree, int ply, int side)
{
  register int score = 0, defects;
  int enemy = Flip(side);

/*
 ************************************************************
 *                                                          *
 *   first, check for where the king should be if this is   *
 *   an endgame.  ie with pawns on one wing, the king needs *
 *   to be on that wing.  with pawns on both wings, the     *
 *   king belongs in the center.                            *
 *                                                          *
 ************************************************************
 */
  if (tree->endgame && tree->all_pawns) {
    if (tree->all_pawns & mask_efgh && tree->all_pawns & mask_abcd)
      score += kval_n[side][KingSQ(side)];
    else if (tree->all_pawns & mask_efgh)
      score += kval_k[side][KingSQ(side)];
    else
      score += kval_q[side][KingSQ(side)];
  }
/*
 ************************************************************
 *                                                          *
 *   check to see if the king has been forced to move and   *
 *   has trapped a rook at a1/a2/b1/g1/h1/h2, if so, then   *
 *   penalize the trapped rook to help extricate it.        *
 *                                                          *
 ************************************************************
 */
  else {
    if (Rank(KingSQ(side)) == rankflip[side][RANK1]) {
      if (File(KingSQ(side)) > FILEE) {
        if (Rooks(side) & mask_kr_trapped[side][FILEH - File(KingSQ(side))])
          score -= rook_trapped;
      } else if (File(KingSQ(side)) < FILED) {
        if (Rooks(side) & mask_qr_trapped[side][File(KingSQ(side))])
          score -= rook_trapped;
      }
    }
/*
 ************************************************************
 *                                                          *
 *   now, check for the "trojan horse" attack where the     *
 *   opponent offers a piece to open the h-file with a very *
 *   difficult to refute attack.                            *
 *                                                          *
 ************************************************************
 */
    if (tree->Dangerous[enemy]) {
      if (shared->trojan_check) {
        if (shared->root_wtm == side && File(KingSQ(side)) >= FILEE) {
          if (!(tree->all_pawns & file_mask[FILEH])) {
            if (Rooks(enemy) && Queens(enemy))
              score -= king_safety_mate_threat;
          }
        }
      }
/*
 ************************************************************
 *                                                          *
 *   Now do castle scoring, if the king has castled, the    *
 *   pawns in front are important.  If not castled yet, the *
 *   pawns on the kingside should be preserved for this.    *
 *                                                          *
 ************************************************************
 */
      defects = 0;
      if (Castle(ply, side) <= 0) {
        if (File(KingSQ(side)) >= FILEE) {
          if (File(KingSQ(side)) > FILEE)
            defects = tree->pawn_score.defects_k[side];
          else
            defects = tree->pawn_score.defects_e[side];
        } else {
          if (File(KingSQ(side)) < FILED)
            defects = tree->pawn_score.defects_q[side];
          else
            defects = tree->pawn_score.defects_d[side];
        }
      } else {
        if (Castle(ply, side) == 3)
          defects =
              Min(Min(tree->pawn_score.defects_k[side],
                  tree->pawn_score.defects_e[side]),
              tree->pawn_score.defects_q[side]);
        else if (Castle(ply, side) == 1)
          defects =
              Min(tree->pawn_score.defects_k[side],
              tree->pawn_score.defects_e[side]);
        else
          defects =
              Min(tree->pawn_score.defects_q[side],
              tree->pawn_score.defects_e[side]);
        if (defects < 3)
          defects = 3;
      }
/*
 ************************************************************
 *                                                          *
 *   now fold in the king tropism and king pawn shelter     *
 *   scores together.  Also add in an enemy pawn tropism    *
 *   score.                                                 *
 *                                                          *
 ************************************************************
 */
      if (Kings(side) & rank12[side] &&
          (king_attacks[KingSQ(side)] | king_attacks[KingSQ(side) - 8 +
                  16 * side]) & Pawns(enemy))
        tree->tropism[enemy] += 3;
      if (tree->tropism[enemy] < 0)
        tree->tropism[enemy] = 0;
      else if (tree->tropism[enemy] > 15)
        tree->tropism[enemy] = 15;
      if (defects > 15)
        defects = 15;
      score -= king_safety[defects][tree->tropism[enemy]];
      score -= defects * (Queens(enemy) ? dented_armor_q : dented_armor);
    }
  }
#ifdef DEBUGEV
  printf("score[defects(%d)]=                %4d\n", side, defects);
  printf("score[tropism(%d)]=                %4d\n", enemy,
      tree->tropism[black]);
  printf("score[kings(%d)]=                  %4d\n", side, score);
#endif
/*
 ************************************************************
 *                                                          *
 *   if we are in an endgame, and there are no pieces on    *
 *   the board, just pawns and kings, then give a bonus for *
 *   the distance between the king and the closest weak     *
 *   opponent pawn.  if one side's king is significantly    *
 *   closer, it will likely win the pawns and then escort   *
 *   its own pawn to promotion.                             *
 *                                                          *
 *   note that this only applies if all remaining pawns are *
 *   on the same side of the board.                         *
 *                                                          *
 ************************************************************
 */
  if (tree->endgame && TotalPieces(white) + TotalPieces(black) == 0 &&
      (!(tree->all_pawns & mask_abcd) || !(tree->all_pawns & mask_efgh))) {
    BITBOARD pawns;
    int dist = 7, square, prot;

    pawns = Pawns(enemy);
    while (pawns) {
      square = LSB(pawns);
      pawns &= pawns - 1;
      if (pawn_attacks[side][square] & Pawns(enemy))
        continue;
      dist = Min(dist, Distance(KingSQ(side), square));
    }
    prot = (side) ? 2 : 1;
    if (!(tree->pawn_score.protected & prot))
      score += won_kp_ending[dist];
#ifdef DEBUGEV
    printf("score[kings(all)] dist=%d\n", dist);
#endif
  }
#ifdef DEBUGEV
  printf("score[kings(all)]=                %4d\n", score);
#endif
  return (score);
}

/* last modified 01/20/08 */
/*
 *******************************************************************************
 *                                                                             *
 *   EvaluateKingsFile computes defects for a file, based on whether the file  *
 *   is open or half-open.  if there are friendly pawns still on the file,     *
 *   they are penalized for advancing in front of the king.                    *
 *                                                                             *
 *******************************************************************************
 */
int EvaluateKingsFile(TREE * RESTRICT tree, int whichfile, int side)
{
  register int defects = 0, file;
  register int enemy = Flip(side);

  for (file = whichfile - 1; file <= whichfile + 1; file++) {
    if (!(file_mask[file] & tree->all_pawns))
      defects += open_file[file];
    else {
      if (!(file_mask[file] & Pawns(enemy)))
        defects += half_open_file[file] / 2;
      else
        defects +=
            pawn_defects[side][Rank(Advanced(enemy,
                    file_mask[file] & Pawns(enemy)))];
      if (!(file_mask[file] & Pawns(side)))
        defects += half_open_file[file];
      else {
        if (!(Pawns(side) & SetMask(sqflip[side][A2] + file))) {
          defects++;
          if (!(Pawns(side) & SetMask(sqflip[side][A3] + file)))
            defects++;
        }
      }
    }
  }
  return (defects);
}

/* last modified 01/24/08 */
/*
 *******************************************************************************
 *                                                                             *
 *   EvaluateKnights() is used to evaluate black/white knights.                *
 *                                                                             *
 *******************************************************************************
 */
int EvaluateKnights(TREE * RESTRICT tree, int side)
{
  register BITBOARD temp;
  register int square, t, score = 0;
  register int enemy = Flip(side);

/*
 ************************************************************
 *                                                          *
 *   first fold in centralization score from the piece/     *
 *   square table "nval".                                   *
 *                                                          *
 ************************************************************
 */
  temp = Knights(side);
  while (temp) {
    square = LSB(temp);
    t = nval[square];
    score += t;
/*
 ************************************************************
 *                                                          *
 *   now, evaluate for "outposts" which is a knight that    *
 *   can't be driven off by an enemy pawn, and which is     *
 *   supported by a friendly pawn.                          *
 *                                                          *
 *   a knight on the edge gets a further penalty determined *
 *   by three factors:                                      *
 *     1) is it on the opposite side of the enemy king      *
 *     2) does it have "bad" mobility                       *
 *     3) the number of enemy pawns on that side of the     *
 *       board (which can potentially hinder mobility more, *
 *       or eventaully trap the knight)                     *
 *                                                          *
 ************************************************************
 */
    if (t > -30) {
      if (outpost[side][square]
          && !(mask_no_pattacks[enemy][square] & Pawns(enemy))
          && pawn_attacks[enemy][square] & Pawns(side))
        score += outpost[side][square];
    } else {
      if (File(square) == FILEA) {
        score -= ((mask_efgh & KingSQ(enemy) ? 3 : 2)
            * (5 - PopCnt(knight_attacks[square] & ~(Occupied(side))))
            * (1 + PopCnt(mask_abcd & Pawns(enemy)))
            ) / 5 - 2;
      } else {
        score -= ((mask_abcd & KingSQ(enemy) ? 3 : 2)
            * (5 - PopCnt(knight_attacks[square] & ~(Occupied(side))))
            * (1 + PopCnt(mask_efgh & Pawns(enemy)))
            ) / 5 - 2;
      }
    }
/*
 ************************************************************
 *                                                          *
 *   adjust the tropism count for this piece.               *
 *                                                          *
 ************************************************************
 */
    if (tree->Dangerous[side]) {
      t = Distance(square, KingSQ(enemy));
      score -= t * gen_trop - gen_trop_mid;
      tree->tropism[side] += king_tropism_n[t];
    }
    temp &= temp - 1;
  }
#ifdef DEBUGEV
  printf("score[knights(%d)]=                %4d\n", side, score);
  printf("tropism[knights(%d)]=              %4d\n", side, tree->tropism[side]);
#endif
  return (score);
}

/* last modified 02/08/08 */
/*
 *******************************************************************************
 *                                                                             *
 *   EvaluateMate() is used to evaluate positions where neither side has pawns *
 *   and one side has enough material to force checkmate.  it simply trys to   *
 *   force the losing king to the edge of the board, and then to the corner    *
 *   where mates are easier to find.                                           *
 *                                                                             *
 *******************************************************************************
 */
int EvaluateMate(TREE * RESTRICT tree, int side)
{
  register int mate_score = 0;
  register int enemy = Flip(side);

/*
 ************************************************************
 *                                                          *
 *   if one side has a bishop+knight and the other side has *
 *   no pieces or pawns, then use the special bishop_knight *
 *   scoring board for the losing king to force it to the   *
 *   right corner for mate.                                 *
 *                                                          *
 ************************************************************
 */
  if (TotalPieces(enemy) == 0 && TotalPieces(side) == 6 &&
      TotalBishops(side) == 1) {
    if (dark_squares & Bishops(side))
      mate_score = b_n_mate_dark_squares[KingSQ(enemy)];
    else
      mate_score = b_n_mate_light_squares[KingSQ(enemy)];
  }
/*
 ************************************************************
 *                                                          *
 *   if one side is winning, force the black king to the    *
 *   edge of the board.                                     *
 *                                                          *
 ************************************************************
 */
  else {
    mate_score = mate[KingSQ(enemy)];
    mate_score -=
        (Distance(KingSQ(side), KingSQ(enemy)) - 3) * king_king_tropism;
  }
  return (mate_score);
}

/* last modified 02/08/08 */
/*
 *******************************************************************************
 *                                                                             *
 *   EvaluateMaterial() is used to evaluate material on the board.  it really  *
 *   accomplishes detecting cases where one side has made a 'bad trade' as the *
 *   comments below show.                                                      *
 *                                                                             *
 *******************************************************************************
 */
int EvaluateMaterial(TREE * RESTRICT tree)
{
  register int score, majors, minors;

/*
 **********************************************************************
 *                                                                    *
 *   we start with the raw Material balance for the current position, *
 *   then we add in the dynamic material adjustments that depend on   *
 *   the number of pawns and piece counts.                            *
 *                                                                    *
 **********************************************************************
 */
  score = Material;
  score +=
      EvaluateMaterialDynamic(tree, white) - EvaluateMaterialDynamic(tree,
      black);
/*
 **********************************************************************
 *                                                                    *
 *   test 1.  if Majors or Minors are not balanced, then if one side  *
 *   is only an exchange up or down, we do not give any sort of bad   *
 *   trade penalty/bonus.                                             *
 *                                                                    *
 *   test 2.  if Majors or Minors are not balanced, then if one side  *
 *   has more piece material points than the other (using normal      *
 *   piece values of 3, 3, 5, 9 for N, B, R and Q) then the side that *
 *   is behind in piece material gets a penalty.                      *
 *                                                                    *
 **********************************************************************
 */
  majors =
      TotalRooks(white) + 2 * TotalQueens(white) - TotalRooks(black) -
      2 * TotalQueens(black);
  minors =
      TotalKnights(white) + TotalBishops(white) - TotalKnights(black) -
      TotalBishops(black);
  if (majors || minors)
    if (Abs(TotalPieces(white) - TotalPieces(black)) != 2) {
      if (TotalPieces(white) > TotalPieces(black))
        score += bad_trade;
      else if (TotalPieces(black) > TotalPieces(white))
        score -= bad_trade;
    }
#ifdef DEBUGM
  printf("Majors=%d  Minors=%d  TotalPieces(white)=%d  TotalPieces(black)=%d\n",
      Majors, Minors, TotalPieces(white), TotalPieces(black));
  printf("score[bad trade]=                 %4d\n", score);
  printf("score[material]=                  %4d\n", Material);
#endif
  return (score);
}

/* last modified 02/08/08 */
/*
 *******************************************************************************
 *                                                                             *
 *   EvaluateMaterialDynamic() adds bonuses/penalties for each piece based on  *
 *   the number of pawns on the board, and the number of pieces of each type.  *
 *   this comes from IM Larry Kaufman's ideas on how piece values change with  *
 *   relation to what other pieces are present to complement or overlap with   *
 *   the specific piece type.                                                  *
 *                                                                             *
 *******************************************************************************
 */
int EvaluateMaterialDynamic(TREE * RESTRICT tree, int side)
{
  register int score = 0, tp;

/*
 **********************************************************************
 *                                                                    *
 *   we start by counting the number of pawns on the board and then   *
 *   computing each type of piece bonus based on this number.         *
 *                                                                    *
 **********************************************************************
 */
  tp = TotalPawns(white) + TotalPawns(black);
  if (TotalKnights(side)) {
    score += 2 * tp - 16;
    if (TotalKnights(side) > 1)
      score -= 11;
  }
  if (TotalBishops(side) > 1)
    score += 13;
  if (Queens(side))
    score +=
        tp + TotalKnights(white) + TotalBishops(white) + TotalKnights(black) +
        TotalBishops(black);
  score += TotalRooks(side) * (32 - tp * 2);
  return (score);
}

/* last modified 01/20/08 */
/*
 *******************************************************************************
 *                                                                             *
 *   EvaluateMobility() is used to evaluate bishops/rooks with respect to      *
 *   their mobility.  bishops also have the PAIR scoring done here but the     *
 *   score depends on the mobility of each bishop so that a bad bishop kills   *
 *   the value of the pair.                                                    *
 *                                                                             *
 *   the basic components considered are:                                      *
 *                                                                             *
 *     (1) a piece gets a bonus for each square it can move to, where some     *
 *         squares get a bigger bonus because they are move useful (center)    *
 *         as opposed to corner squares for example.                           *
 *                                                                             *
 *     (2) a bishop behind a queen or a rook behind a rook/queen offers        *
 *         support to the piece and get credit for moving through such a piece *
 *         and continuing down the board.                                      *
 *                                                                             *
 *     (3) any slider that attacks a more valuable piece gets an additional    *
 *         bonus since it forces the opponent to respond.                      *
 *                                                                             *
 *******************************************************************************
 */
int EvaluateMobility(TREE * RESTRICT tree, int side)
{
  register BITBOARD bishops, rooks, moves;
  register int square, score = 0, tscore, i;
  register int score1 = 0, score2 = 0;
  register int enemy = Flip(side);

/*
 **********************************************************************
 *                                                                    *
 *   Bishop mobility scoring.                                         *
 *                                                                    *
 **********************************************************************
 */
  bishops = Bishops(side);
  while (bishops) {
    square = LSB(bishops);
    bishops &= bishops - 1;
    moves = AttacksBishop(square, OccupiedSquares ^ Queens(side));
    tscore =
        attacks_enemy *
        PopCnt(moves & (Rooks(enemy) | Kings(enemy) | Queens(enemy)));
    tscore += supports_slider * PopCnt(moves & Queens(side));
    moves &= ~(Occupied(side) ^ Queens(side));
    moves |= SetMask(square);
    for (i = 0; i < 4; i++) {
      tscore += PopCnt(moves & mobility_mask_b[i]) * mobility_score_b[i];
    }
    if (!score1)
      score1 = tscore;
    else if (!score2)
      score2 = tscore;
    else
      score += tscore - lower_b1;
  }
  if (score2)
    score +=
        score1 + score2 + Min((score1 * score2 / 16) + pair_b_min,
        max_pair_b) - lower_b2;
  else if (score1)
    score += score1 - lower_b1;
/*
 **********************************************************************
 *                                                                    *
 *   Rook mobility scoring.                                           *
 *                                                                    *
 **********************************************************************
 */
  tscore = 0;
  rooks = Rooks(side);
  while (rooks) {
    square = LSB(rooks);
    rooks &= rooks - 1;
    moves = AttacksRook(square, OccupiedSquares ^ Queens(side) ^ Rooks(side));
    tscore += attacks_enemy * PopCnt(moves & (Kings(enemy) | Queens(enemy)));
    tscore += supports_slider * PopCnt(moves & (Queens(side) | Rooks(side)));
    moves &= ~(Occupied(side) ^ Queens(side) ^ Rooks(side));
    moves |= SetMask(square);
    for (i = 0; i < 4; i++) {
      tscore += PopCnt(moves & mobility_mask_r[i]) * mobility_score_r[i];
    }
    tscore -= lower_r;
  }
  score += tscore * lower_r_percent / 10;
  return (score);
}

/* last modified 02/12/08 */
/*
 *******************************************************************************
 *                                                                             *
 *   EvaluatePassedPawns() is used to evaluate passed pawns and the danger     *
 *   they produce.  EvaluatePawns() has thoughtfully provided us with the      *
 *   following bit flags to give us a starting point for evaluation.           *
 *                                                                             *
 *   pawn_score.outside:                                                       *
 *                                                                             *
 *        xxxx xxx1   (1) -> white has outside passer                          *
 *        xxxx xx1x   (2) -> white has 2 outside passers                       *
 *        xxxx x1xx   (4) -> black has outside passer                          *
 *        xxxx 1xxx   (8) -> black has 2 outside passers                       *
 *        xxx1 xxxx  (16) -> white has outside candidate                       *
 *        xx1x xxxx  (32) -> white has 2 outside candidates                    *
 *        x1xx xxxx  (64) -> black has outside candidate                       *
 *        1xxx xxxx (128) -> black has 2 outside candidates                    *
 *                                                                             *
 *   pawn_score.protected:                                                     *
 *                                                                             *
 *        xxxx xxx1   (1) -> white has protected passed pawn                   *
 *        xxxx xx1x   (2) -> black has protected passed pawn                   *
 *                                                                             *
 *******************************************************************************
 */
int EvaluatePassedPawns(TREE * RESTRICT tree, int side)
{
  register int file, square, score = 0;
  register int king_sq, pawns;
  register int enemy = Flip(side);
  int sign[2] = { -1, 1 };

/*
 ************************************************************
 *                                                          *
 *   check to see if side has any passed pawns.  if so,     *
 *   then the following things are evaluated:               *
 *                                                          *
 *   1.  A passed pawn is given a bonus depending on how    *
 *       far it has advanced.                               *
 *                                                          *
 *   2.  If the pawn is passed, but is blockaded by an      *
 *       enemy piece, it is not as valuable as if it were   *
 *       unblocked.                                         *
 *                                                          *
 *   3.  if the pawn is supported by the king, it is more   *
 *       valuable since it is easier to force its adance    *
 *       in endgames if the king can help.                  *
 *                                                          *
 ************************************************************
 */
  if (tree->pawn_score.passed[side]) {
    king_sq = KingSQ(side);
    pawns = tree->pawn_score.passed[side];
    while (pawns) {
      file = LSB8Bit(pawns);
      pawns &= pawns - 1;
      square = Advanced(side, Pawns(side) & file_mask[file]);
      score += passed_pawn_value[side][Rank(square)];
      if (PcOnSq(square - 8 + side * 16) * sign[side] < 0)
        score -= blockading_passed_pawn_value[side][Rank(square)];
      if (tree->endgame && FileDistance(square, king_sq) == 1 &&
          (Rank(king_sq) - Rank(square)) * sign[side] >= 0)
        score += supported_passer[side][Rank(square)];
    }
#ifdef DEBUGPP
    printf("score.1 after %s passers = %d\n", (side) ? "white" : "black",
        score);
#endif
/*
 ************************************************************
 *                                                          *
 *  if side has any "hidden passed pawns" then we factor    *
 *  the score here.                                         *
 *                                                          *
 ************************************************************
 */
    if (tree->pawn_score.hidden[side])
      score +=
          PopCnt8Bit(tree->pawn_score.hidden[side]) * hidden_passed_pawn_value;
#ifdef DEBUGPP
    printf("score.2 after %s passers = %d\n", (side) ? "white" : "black",
        score);
#endif
/*
 ************************************************************
 *                                                          *
 *   check to see if side has any connected passed pawns.   *
 *   if so, and they have both reached the 6th/7th rank,    *
 *   then they are very dangerous.                          *
 *                                                          *
 ************************************************************
 */
    pawns = tree->pawn_score.passed[side];
    while ((file = connected_passed[pawns])) {
      register int square1, square2;

      pawns &= ~(1 << (file - 1));
      square1 = Advanced(side, Pawns(side) & file_mask[file - 1]);
      square2 = Advanced(side, Pawns(side) & file_mask[file]);
      score +=
          connected_passed_pawn_value[side][MinMax(side, Rank(square1),
              Rank(square2))];
    }
#ifdef DEBUGPP
    printf("score.3 after %s passers = %d\n", (side) ? "white" : "black",
        score);
#endif
  }
/*
 ************************************************************
 *                                                          *
 *   check to see if side has an outside passed pawn.       *
 *                                                          *
 *   note that an outside candidate is worth 1/2 as much as *
 *   a real outside passer, since it takes more time to     *
 *   convert a candidate into a true passed pawn.           *
 *                                                          *
 ************************************************************
 */
  if (tree->pawn_score.outside) {
    int oneoutside[2] = { 4, 1 };
    int twooutside[2] = { 8, 2 };
    int onecandidate[2] = { 64, 16 };
    int twocandidates[2] = { 128, 32 };
    int otherprotected[2] = { 1, 2 };
    if (tree->pawn_score.outside & twooutside[side])
      score += 2 * outside_passed;
    else if ((tree->pawn_score.outside & oneoutside[side]) &&
        (!(tree->pawn_score.protected & otherprotected[side]) ||
            TotalPieces(side)))
      score += outside_passed;
    if (tree->pawn_score.outside & twocandidates[side])
      score += outside_passed;
    else if ((tree->pawn_score.outside & onecandidate[side]) &&
        (!(tree->pawn_score.protected & otherprotected[side]) ||
            TotalPieces(side)))
      score += outside_passed / 2;
  }
#ifdef DEBUGPP
  printf("score.4 after %s passers = %d\n", (side) ? "white" : "black", score);
#endif
/*
 ************************************************************
 *                                                          *
 *   check to see if side has "split" passed pawns, as      *
 *   these are much harder for a lone king to stop than two *
 *   connected passers are.                                 *
 *                                                          *
 ************************************************************
 */
  if (TotalPieces(enemy) == 0) {
    int spread;

    spread = file_spread[tree->pawn_score.passed[side]];
    if (spread > 0)
      score += (spread - 1) * split_passed;
  }
#ifdef DEBUGPP
  printf("score[passed pawns %s] = %d\n", (side) ? "white" : "black", score);
#endif
  return (score);
}

/* last modified 02/16/08 */
/*
 *******************************************************************************
 *                                                                             *
 *   EvaluatePassedPawnRaces() is used to evaluate passed pawns when one       *
 *   side has passed pawns and the other side (or neither) has pieces.  in     *
 *   such a case, the critical question is can the defending king stop the pawn*
 *   from queening or is it too far away?  if only one side has pawns that can *
 *   "run" then the situation is simple.  when both sides have pawns that can  *
 *   "run" it becomes more complex as it then becomes necessary to see if      *
 *   one side can use a forced king move to stop the other side, while the     *
 *   other side doesn't have the same ability to stop ours.                    *
 *                                                                             *
 *   in the case of king and pawn endings with exactly one pawn, the simple    *
 *   evaluation rules are used:  if the king is two squares in front of the    *
 *   pawn then it is a win, if the king is one one square in front with the    *
 *   opposition, then it is a win,  if the king is on the 6th rank with the    *
 *   pawn close by, it is a win.  rook pawns are handled separately and are    *
 *   more difficult to queen because the king can get trapped in front of the  *
 *   pawn blocking promotion.                                                  *
 *                                                                             *
 *******************************************************************************
 */
int EvaluatePassedPawnRaces(TREE * RESTRICT tree, int wtm)
{
  register int file, square;
  register int queen_distance;
  register int pawnsq;
  register BITBOARD pawns;
  register int passed;
  register int side, enemy;
  BITBOARD runners[2] = { 0, 0 };
  int queener[2] = { 8, 8 };
  int psquare[2] = { 0, 0 };
  int ppawn[2] = { 0, 0 };
  int forced_km[2] = { 0, 0 };

/*
 ************************************************************
 *                                                          *
 *   check to see if side has one pawn and neither side     *
 *   has any pieces.  if so, use the simple pawn evaluation *
 *   logic.                                                 *
 *                                                          *
 ************************************************************
 */
  for (side = black; side <= white; side++) {
    enemy = Flip(side);
    if (Pawns(side) && !Pawns(enemy) && TotalPieces(white) == 0 &&
        TotalPieces(black) == 0) {
      pawns = Pawns(side);
      while (pawns) {
        pawnsq = LSB(pawns);
        pawns &= pawns - 1;
/*
 **************************************************
 *                                                *
 *   king must be in front of the pawn or we      *
 *   go no further.                               *
 *                                                *
 **************************************************
 */
        if (sign[side] * Rank(KingSQ(side)) <= sign[side] * Rank(pawnsq))
          continue;
/*
 **************************************************
 *                                                *
 *   first a special case.  if this is a rook     *
 *   pawn, then the king must be on the adjacent  *
 *   file, and be closer to the queening square   *
 *   than the opposing king.                      *
 *                                                *
 **************************************************
 */
        if (File(pawnsq) == FILEA) {
          if ((File(KingSQ(side)) == FILEB) &&
              (Distance(KingSQ(side),
                      sqflip[side][A8]) < Distance(KingSQ(enemy),
                      sqflip[side][A8])))
            return (sign[side] * pawn_can_promote);
          continue;
        } else if (File(pawnsq) == FILEH) {
          if ((File(KingSQ(side)) == FILEG) &&
              (Distance(KingSQ(side),
                      sqflip[side][H8]) < Distance(KingSQ(enemy),
                      sqflip[side][H8])))
            return (sign[side] * pawn_can_promote);
          continue;
        }
/*
 **************************************************
 *                                                *
 *   if king is two squares in front of the pawn  *
 *   then it's a win immediately.  if the king is *
 *   on the 6th rank and closer to the pawn than  *
 *   the opposing king, it's also a win.          *
 *                                                *
 **************************************************
 */
        if (Distance(KingSQ(side), pawnsq) < Distance(KingSQ(enemy), pawnsq)) {
          if (sign[side] * Rank(KingSQ(side)) >
              sign[side] * (Rank(pawnsq) - 1 + 2 * side))
            return (sign[side] * pawn_can_promote);
          if (Rank(KingSQ(side)) == rankflip[side][RANK6])
            return (sign[side] * pawn_can_promote);
        }
/*
 **************************************************
 *                                                *
 *   last chance:  if the king is one square in   *
 *   front of the pawn and has the opposition,    *
 *   then it's still a win.                       *
 *                                                *
 **************************************************
 */
        if ((Rank(KingSQ(side)) == Rank(pawnsq) - 1 + 2 * side) &&
            EvaluateHasOpposition(wtm == side, KingSQ(side), KingSQ(enemy)))
          return (sign[side] * pawn_can_promote);
      }
    }
/*
 ************************************************************
 *                                                          *
 *   check to see if black is out of pieces and white has   *
 *   passed pawns.  if so, see if any of these passed pawns *
 *   can outrun the defending king and promote.             *
 *                                                          *
 ************************************************************
 */
    if (TotalPieces(enemy) == 0 && tree->pawn_score.passed[side]) {
      passed = tree->pawn_score.passed[side];
      while (passed) {
        file = LSB8Bit(passed);
        passed &= passed - 1;
        square = Advanced(side, Pawns(side) & file_mask[file]);
        forced_km[enemy] =
            (pawn_race[side][wtm][square] & Kings(enemy)) !=
            (pawn_race[side][Flip(wtm)][square] & Kings(enemy));
        if (!(pawn_race[side][wtm][square] & Kings(enemy))) {
          queen_distance = Abs(rankflip[side][RANK8] - Rank(square));
          if (Kings(side) & ((side) ? plus8dir[square] : minus8dir[square])) {
            if (file == FILEA || file == FILEH)
              queen_distance = 99;
            queen_distance++;
          }
          if (Rank(square) == rankflip[side][RANK2])
            queen_distance--;
          if (queen_distance < queener[side]) {
            runners[side] = SetMask(square);
            queener[side] = queen_distance;
            psquare[side] = file + (rankflip[side][RANK8] << 3);
            ppawn[side] = square;
          } else if (queen_distance == queener[side])
            runners[side] |= SetMask(square);
        }
      }
    }
#ifdef DEBUGPP
    printf("%s pawn on %d can promote at %d in %d moves.\n",
        (side) ? "white" : "black", ppawn[side], psquare[side], queener[side]);
#endif
  }
  if ((queener[white] == 8) && (queener[black] == 8))
    return (0);
/*
 ************************************************************
 *                                                          *
 *   now that we know which pawns can outrun the kings for  *
 *   each side, we need to do the following:                *
 *                                                          *
 *     (1) if both sides are forced to move their king to   *
 *         prevent the opponent from promoting, we let the  *
 *         search resolve this as the depth increases.      *
 *                                                          *
 *     (2) if white can run while black can not, or then    *
 *         white wins, or vice-versa.                       *
 *                                                          *
 *     (3) if white queens and black's king can't stop it   *
 *         no matter who moves first, while black has a     *
 *         pawn that white can stop if a king move is made  *
 *         immediately, then white wins, and vice-versa.    *
 *                                                          *
 *     (4) other situations are left to the search to       *
 *         resolve.                                         *
 *                                                          *
 ************************************************************
 */
  if (forced_km[white] & forced_km[black])
    return (0);
  if ((queener[white] < 8) && (queener[black] == 8))
    return (pawn_can_promote + (5 - queener[white]) * 10);
  else if ((queener[black] < 8) && (queener[white] == 8))
    return (-(pawn_can_promote + (5 - queener[black]) * 10));
  if (queener[white] < queener[black] && forced_km[white] && !forced_km[black])
    return (pawn_can_promote + (5 - queener[white]) * 10);
  else if (queener[black] < queener[white] && forced_km[black] &&
      forced_km[white])
    return (-(pawn_can_promote + (5 - queener[black]) * 10));
  return (0);
}

/* last modified 02/12/08 */
/*
 *******************************************************************************
 *                                                                             *
 *   EvaluatePawns() is used to evaluate pawns.  it evaluates pawns for only   *
 *   one side, and fills in the pawn hash entry information.  it requires two  *
 *   calls to evaluate all pawns on the board.  comments below indicate the    *
 *   particular pawn structure features that are evaluated.                    *
 *                                                                             *
 *   this procedure also fills in information (without scoring) that other     *
 *   evaluation procedures use, such as which pawns are passed or candidates,  *
 *   which pawns are weak, which files are open, and so forth.                 *
 *                                                                             *
 *******************************************************************************
 */
int EvaluatePawns(TREE * RESTRICT tree, int side)
{
  register BITBOARD pawns;
  register BITBOARD temp;
  BITBOARD p_moves[2];
  register int score = 0;
  register int pns, square, file;
  register int defenders, attackers, sq;
  register int enemy = Flip(side);

/*
 ************************************************************
 *                                                          *
 *   first, determine which squares pawns can reach.        *
 *                                                          *
 ************************************************************
 */
  pawns = Pawns(side);
  p_moves[side] = 0;
  while (pawns) {
    square = LSB(pawns);
    tree->pawn_score.all[side] |= 1 << File(square);
    for (sq = square; sq != File(square) + ((side) ? RANK7 << 3 : RANK2 << 3);
        sq += direction[side]) {
      p_moves[side] |= SetMask(sq);
      if (SetMask(sq + direction[side]) & tree->all_pawns)
        break;
      defenders =
          PopCnt(pawn_attacks[enemy][sq + direction[side]] & Pawns(side));
      attackers =
          PopCnt(pawn_attacks[side][sq + direction[side]] & Pawns(enemy));
      if (attackers - defenders > 0)
        break;
    }
    pawns &= pawns - 1;
  }
/*
 ************************************************************
 *                                                          *
 *   now loop through all pawns for this side.              *
 *                                                          *
 ************************************************************
 */
  pawns = Pawns(side);
  while (pawns) {
    square = LSB(pawns);
    pawns &= pawns - 1;
/*
 ************************************************************
 *                                                          *
 *   the first thing we do is count the number of pawns     *
 *   that are protected by friendly pawns, which is a       *
 *   value used in endgame scoring.  we also flag open      *
 *   files for use by other scoring components.             *
 *                                                          *
 ************************************************************
 */
    if (PawnAttacks(side, square))
      tree->pawn_score.protected_count += 1 - 2 * enemy;
    file = File(square);
    tree->pawn_score.open_files &= ~(1 << file);
/*
 ************************************************************
 *                                                          *
 *   evaluate pawn advances.  center pawns are encouraged   *
 *   to advance, while wing pawns are pretty much neutral.  *
 *   this is a simple piece/square value.                   *
 *                                                          *
 ************************************************************
 */
    score += pval[side][square];
#ifdef DEBUGP
    printf("%s pawn[static] file=%d,   score=%d\n", (side) ? "white" : "black",
        file, score);
#endif
/*
 ************************************************************
 *                                                          *
 *   evaluate isolated pawns, which also means that the     *
 *   pawn is weak.  we simply lump this pawn in with the    *
 *   other weak pawns and then factor in the score later.   *
 *                                                          *
 ************************************************************
 */
    if (!(mask_pawn_isolated[square] & Pawns(side)))
      tree->pawn_score.weak[side]++;
/*
 ************************************************************
 *                                                          *
 *  evaluate weak pawns.  weak pawns are evaluated by the   *
 *  following rules:  (1) if a pawn is defended by a pawn,  *
 *  it isn't weak;  (2) if a pawn is undefended by a pawn   *
 *  and advances one (or two if it hasn't moved yet) ranks  *
 *  and is defended fewer times than it is attacked, it is  *
 *  weak.  note that the penalty is greater if the pawn is  *
 *  on an open file.  note that an isolated pawn is just    *
 *  another case of a weak pawn, since it can never be      *
 *  defended by a pawn.                                     *
 *                                                          *
 *  first, test the pawn where it sits to determine if it   *
 *  is defended more times than attacked.  if so, it is not *
 *  weak and we are done.  if it is weak where it sits, can *
 *  it advance one square and become not weak.  if so we    *
 *  are again finished with this pawn.  otherwise we fall   *
 *  into the next test.                                     *
 *                                                          *
 ************************************************************
 */
    else {
      do {
        attackers = 0;
        defenders = 0;
        temp = p_moves[side] & ((side) ? plus8dir[square] : minus8dir[square]);
        while (temp) {
          sq = LSB(temp);
          temp &= temp - 1;
          defenders = PopCnt(pawn_attacks[enemy][sq] & Pawns(side));
          attackers = PopCnt(pawn_attacks[side][sq] & Pawns(enemy));
          if (defenders && defenders >= attackers)
            break;
        }
        if (defenders && defenders >= attackers)
          break;
/*
 ************************************************************
 *                                                          *
 *  if the pawn can be defended by a pawn, and that pawn    *
 *  can safely advance to actually defend this pawn, then   *
 *  this pawn is not weak.                                  *
 *                                                          *
 ************************************************************
 */
        if (!(pawn_attacks[enemy][square] & p_moves[side])) {
          tree->pawn_score.weak[side]++;
        }
      }
      while (0);
#ifdef DEBUGP
      printf("%s pawn[weak] file=%d,     score=%d\n",
          (side) ? "white" : "black", file, score);
#endif
/*
 ************************************************************
 *                                                          *
 *   evaluate doubled pawns.  if there are other pawns on   *
 *   this file, penalize this pawn.                         *
 *                                                          *
 ************************************************************
 */
      if ((pns = PopCnt(file_mask[file] & Pawns(side))) > 1)
        score -= doubled_pawn_value[pns];
#ifdef DEBUGP
      printf("%s pawn[doubled] file=%d,  score=%d\n",
          (side) ? "white" : "black", file, score);
#endif
/*
 ************************************************************
 *                                                          *
 *  test the pawn to see it if forms a "duo" which is two   *
 *  pawns side-by-side.                                     *
 *                                                          *
 ************************************************************
 */
      if (mask_pawn_duo[square] & Pawns(side))
        score += pawn_duo;
#ifdef DEBUGP
      printf("%s pawn[duo] file=%d,      score=%d\n",
          (side) ? "white" : "black", file, score);
#endif
    }
/*
 ************************************************************
 *                                                          *
 *   discover and flag passed pawns for use later.          *
 *                                                          *
 ************************************************************
 */
    if (!(mask_pawn_passed[side][square] & Pawns(enemy))) {
      if (mask_pawn_protected[side][square] & Pawns(side))
        tree->pawn_score.protected |= (side) ? 1 : 2;
      tree->pawn_score.passed[side] |= 1 << file;
#ifdef DEBUGP
      printf("%s pawn[passed]            file=%d\n", (side) ? "white" : "black",
          file);
#endif
    }
/*
 ************************************************************
 *                                                          *
 *   now determine if this pawn is a candidate passer,      *
 *   since we now know it isn't passed.  a candidate is a   *
 *   pawn on a file with no enemy pawns in front of it, and *
 *   if it advances until it contacts an enemy pawn, and it *
 *   is defended as many times as it is attacked when it    *
 *   reaches that pawn, then all that is left is to see if  *
 *   it is passed when the attacker(s) get removed.         *
 *                                                          *
 ************************************************************
 */
    else {
      if (!(file_mask[File(square)] & Pawns(enemy)) &&
          mask_pawn_isolated[square] & Pawns(side) &&
          !(pawn_attacks[side][square] & Pawns(enemy))) {
        attackers = 1;
        defenders = 0;
        for (sq = square;
            sq != File(square) + ((side) ? RANK7 << 3 : RANK2 << 3);
            sq += direction[side]) {
          if (SetMask(sq + direction[side]) & tree->all_pawns)
            break;
          defenders = PopCnt(pawn_attacks[enemy][sq] & p_moves[side]);
          attackers = PopCnt(pawn_attacks[side][sq] & Pawns(enemy));
          if (attackers)
            break;
        }
        if (attackers <= defenders) {
          if (!(mask_pawn_passed[side][sq + direction[side]] & Pawns(enemy))) {
            tree->pawn_score.candidates[side] |= 1 << file;
          }
        }
      }
      if (!(tree->pawn_score.candidates[side] & (1 << file))) {
        if (file <= FILED) {
          if (Pawns(side) & file_mask[file + 1]
              && Pawns(side) & file_mask[file + 2]) {
            if (!(Pawns(enemy) & file_mask[file])
                && !(Pawns(enemy) & file_mask[file + 2])
                && !(Pawns(enemy) & file_mask[file + 3])
                && PopCnt(Pawns(enemy) & file_mask[file + 1]) <= 2)
              tree->pawn_score.candidates[side] |= 1 << file;
          }
        } else {
          if (Pawns(side) & file_mask[file - 1]
              && Pawns(side) & file_mask[file - 2]) {
            if (!(Pawns(enemy) & file_mask[file])
                && !(Pawns(enemy) & file_mask[file - 2])
                && !(Pawns(enemy) & file_mask[file - 3])
                && PopCnt(Pawns(enemy) & file_mask[file - 1]) <= 2)
              tree->pawn_score.candidates[side] |= 1 << file;
          }
        }
      }
#ifdef DEBUGP
      if (tree->pawn_score.candidates[white] & (1 << file))
        printf("%s pawn[candidate]       square=%d\n",
            (side) ? "white" : "black", square);
#endif
    }
/*
 ************************************************************
 *                                                          *
 *   evaluate "hidden" passed pawns.  simple case is a pawn *
 *   chain (white) at b5, a6, with a black pawn at a7.      *
 *   it appears the b-pawn is backward, with a ram at a6/a7 *
 *   but this is misleading, because the pawn at a6 is      *
 *   really passed when white plays b6.                     *
 *                                                          *
 ************************************************************
 */
    if (Rank(square) == ((side) ? RANK6 : RANK3) &&
        SetMask(square + direction[side]) & Pawns(enemy) &&
        ((File(square) < FILEH && SetMask(square + 9 - 16 * side) & Pawns(side)
                && !(mask_hidden_right[side][File(square)] & Pawns(enemy))) ||
            (File(square) > FILEA &&
                SetMask(square + 7 - 16 * side) & Pawns(side) &&
                !(mask_hidden_left[side][File(square)] & Pawns(enemy)))))
      tree->pawn_score.hidden[side] |= 1 << file;
  }
/*
 ************************************************************
 *                                                          *
 *   next, count the number of pawn islands for each side   *
 *   and add a penalty for each to avoid creating so many   *
 *   weaknesses the endgame can't be held.                  *
 *                                                          *
 ************************************************************
 */
  score -= pawn_islands[islands[tree->pawn_score.all[side]]];
#ifdef DEBUGP
  printf("%s pawn[islands]           score=%d\n", (side) ? "white" : "black",
      score);
#endif
/*
 ************************************************************
 *                                                          *
 *   now fold in the penalty for weak pawns, which is       *
 *   non-linear to penalize more isolani more severely.     *
 *   note that the penalty penalizes the side with the      *
 *   most weak pawns, in an exponential rate.               *
 *                                                          *
 ************************************************************
 */
  score -= pawn_weak[tree->pawn_score.weak[side]];
#ifdef DEBUGP
  printf("%s pawn[weak]              score=%d\n", (side) ? "white" : "black",
      score);
#endif
/*
 ************************************************************
 *                                                          *
 *   now evaluate king safety.                              *
 *                                                          *
 *   this uses the function EvaluateKingsFile() which       *
 *   looks at four possible positions for the king, either  *
 *   castled kingside, queenside or else standing on the    *
 *   d or e file stuck in the middle.  this essentially is  *
 *   about the pawns in front of the king and what kind of  *
 *   "shelter" they provide for the king during the         *
 *   middlegame.                                            *
 *                                                          *
 ************************************************************
 */
  tree->pawn_score.defects_q[side] = EvaluateKingsFile(tree, FILEB, side);
  tree->pawn_score.defects_d[side] = EvaluateKingsFile(tree, FILED, side);
  tree->pawn_score.defects_e[side] = EvaluateKingsFile(tree, FILEE, side);
  tree->pawn_score.defects_k[side] = EvaluateKingsFile(tree, FILEG, side);
  return (score);
}

/* last modified 01/17/08 */
/*
 *******************************************************************************
 *                                                                             *
 *   EvaluateQueens() is used to evaluate black/white queens.                  *
 *                                                                             *
 *******************************************************************************
 */
int EvaluateQueens(TREE * RESTRICT tree, int side)
{
  register BITBOARD temp;
  register int square, score = 0;
  register int enemy = Flip(side);
  int trop;

/*
 ************************************************************
 *                                                          *
 *   first locate each queen and obtain it's centralization *
 *   score from the static piece/square table for queens.   *
 *                                                          *
 ************************************************************
 */
  temp = Queens(side);
  while (temp) {
    square = LSB(temp);
    score += qval[square];
/*
 ************************************************************
 *                                                          *
 *   check to see if the queen is in a strong position on   *
 *   the 7th rank supported by a rook on the 7th.  if so,   *
 *   the positional advantage is almost overwhelming.       *
 *                                                          *
 ************************************************************
 */
    if (Rank(square) == rankflip[side][RANK7] &&
        (Pawns(enemy) & rank_mask[rankflip[side][RANK7]] ||
            (Rank(KingSQ(enemy)) == rankflip[side][RANK8]))) {
      if (AttacksRank(square) & Rooks(side))
        score += queen_rook_on_7th_rank;
    }
/*
 ************************************************************
 *                                                          *
 *   if the queen is on the wrong side of the board, which  *
 *   is the side away from the opponent's king, then a      *
 *   penalty is in order.                                   *
 *                                                          *
 ************************************************************
 */
    if (TotalPawns(side) > 4) {
      if ((File(square) < FILEC && File(KingSQ(enemy)) > FILEE) ||
          (File(square) > FILEF && File(KingSQ(enemy)) < FILED))
        score -= queen_offside;
    }
/*
 ************************************************************
 *                                                          *
 *   adjust the tropism count for this piece.               *
 *                                                          *
 *   now we notice whether the queen is on a file that is   *
 *   bearing on the enemy king and adjust tropism if so.    *
 *                                                          *
 ************************************************************
 */
    if (tree->Dangerous[side]) {
      trop = Distance(square, KingSQ(enemy));
      score -= trop * gen_trop - gen_trop_mid;
      tree->tropism[side] += king_tropism_q[trop];
    }
    tree->tropism[enemy] -= friendly_queen[Distance(square, KingSQ(side))];
    temp &= temp - 1;
  }
#ifdef DEBUGEV
  printf("score[queens(%d)]=                 %4d\n", side, score);
  printf("tropism[queens(%d)]=               %4d\n", side, tree->tropism[side]);
#endif
  return (score);
}

/* last modified 01/16/08 */
/*
 *******************************************************************************
 *                                                                             *
 *   EvaluateRooks() is used to evaluate black/white rooks.                    *
 *                                                                             *
 *******************************************************************************
 */
int EvaluateRooks(TREE * RESTRICT tree, int side)
{
  register BITBOARD temp;
  register int square, file, open_files, trop, score = 0;
  register int enemy = Flip(side);

/*
 ************************************************************
 *                                                          *
 *   Initialize.                                            *
 *                                                          *
 ************************************************************
 */
  open_files = PopCnt8Bit(tree->pawn_score.open_files);
  temp = Rooks(side);
  while (temp) {
    square = LSB(temp);
    file = File(square);
/*
 ************************************************************
 *                                                          *
 *   determine if the rook is on an open file.  if it is,   *
 *   determine if this rook attacks another friendly rook,  *
 *   making it difficult to drive the rooks off the file.   *
 *                                                          *
 *   if the rook is not on an open file, but there are open *
 *   files on the board, penalize the rook if it can not    *
 *   move horizontally to reach the open file immediately.  *
 *                                                          *
 *   while evaluating files, notice if the file is bearing  *
 *   on either king and adjust the tropism accordingly.     *
 *                                                          *
 ************************************************************
 */
    if (tree->pawn_score.open_files & 1 << file)
      score += rook_open_file[open_files][file];
    else {
      if (open_files) {
        unsigned int rankmvs = AttacksRank(square) >> (square & 0x38);

        if (!(rankmvs & tree->pawn_score.open_files))
          score -= rook_reaches_open_file;
      }
      if (!(file_mask[file] & Pawns(side)))
        score += rook_half_open_file;
    }
/*
 ************************************************************
 *                                                          *
 *   see if the rook is behind a passed pawn.  if it is,    *
 *   it is given a bonus.                                   *
 *                                                          *
 ************************************************************
 */
    if (1 << file & tree->pawn_score.passed[white]) {
      register int pawnsq = MSB(Pawns(white) & file_mask[file]);

      if (pawnsq > square)
        score += rook_behind_passed_pawn;
    }
    if (1 << file & tree->pawn_score.passed[black]) {
      register int pawnsq = LSB(Pawns(black) & file_mask[file]);

      if (pawnsq < square)
        score += rook_behind_passed_pawn;
    }
/*
 ************************************************************
 *                                                          *
 *   finally check to see if any rooks are on the 7th rank, *
 *   with the opponent having pawns on that rank and the    *
 *   opponent's king being hemmed in on the 7th/8th rank.   *
 *   if so, and another rook is also on the 7th rank, then  *
 *   this is a *strong* positional advantage.               *
 *                                                          *
 ************************************************************
 */
    if (Rank(square) == rankflip[side][RANK7] &&
        (Rank(KingSQ(enemy)) == rankflip[side][RANK8]
            || Pawns(enemy) & rank_mask[rankflip[side][RANK7]])) {
      score += rook_on_7th;
      if (AttacksRank(square) & Rooks(side))
        score += rook_connected_7th_rank;
    }
/*
 ************************************************************
 *                                                          *
 *   adjust the tropism count for this piece.               *
 *                                                          *
 ************************************************************
 */
    if (tree->Dangerous[side]) {
      trop =
          (AttacksRook(square,
              OccupiedSquares & ~(Queens(side) | (Rooks(side)))) &
          king_attacks[KingSQ(enemy)]) ? 1 : Distance(square, KingSQ(enemy));
      score -= trop * gen_trop - gen_trop_mid;
      tree->tropism[side] += king_tropism_r[trop];
    }
    temp &= temp - 1;
  }
#ifdef DEBUGEV
  printf("score[rooks(%d)]=                  %4d\n", side, score);
#endif
  return (score);
}

/* last modified 02/08/08 */
/*
 *******************************************************************************
 *                                                                             *
 *   EvaluateWinningChances() is used to determine if one side (or both) are   *
 *   in a position where winning is impossible.                                *
 *                                                                             *
 *   return values:                                                            *
 *        0    ->     neither side can win, this is a dead drawn position.     *
 *        1    ->     white can win, black can not win.                        *
 *        2    ->     white can not win, black can win.                        *
 *        3    ->     both white and black can win.                            *
 *                                                                             *
 *******************************************************************************
 */
int EvaluateWinningChances(TREE * RESTRICT tree, int side)
{
  register int square;
  int enemy = Flip(side);

/*
 ************************************************************
 *                                                          *
 *   if one side is a piece up, but has no pawns, then that *
 *   side can not possibly win.                             *
 *                                                          *
 ************************************************************
 */
  if (TotalPawns(side) == 0 && TotalPieces(side) - TotalPieces(enemy) <= 3 &&
      ((mask_not_edge & Kings(enemy)) || TotalPieces(side) <= 3))
    return (0);
/*
 ************************************************************
 *                                                          *
 *   if one side has a piece and the other side has one     *
 *   pawn, then that piece can sac itself for the pawn so   *
 *   that the side with a pawn can't win.                   *
 *                                                          *
 ************************************************************
 */
  if (TotalPieces(side) == 0) {
    if (TotalPawns(side) == 0)
      return (0);
    if (TotalPawns(side) == 1 && TotalPieces(enemy))
      return (0);
  }
/*
 ************************************************************
 *                                                          *
 *   if "side" has a pawn, then either the pawn had better  *
 *   not be a rook pawn, or else white had better have the  *
 *   right color bishop or any other piece, otherwise it is *
 *   not winnable if the black king can get to the queening *
 *   square first.                                          *
 *                                                          *
 ************************************************************
 */
  if (TotalPawns(side))
    do {
      if (Pawns(side) & not_rook_pawns)
        continue;
      if (TotalPieces(side) > 3 || (TotalPieces(side) == 3 && Knights(side)))
        continue;
      if (TotalPieces(side) == 0) {
        if (file_mask[FILEA] & Pawns(side) && file_mask[FILEH] & Pawns(side))
          continue;
      }
      if (!(Pawns(side) & not_rook_pawns)) {
        if (Bishops(side)) {
          if (!Bishops(enemy)) {
            if (Bishops(side) & dark_squares) {
              if (file_mask[dark_corner[side]] & Pawns(side))
                continue;
            } else if (file_mask[light_corner[side]] & Pawns(side))
              continue;
          } else {
            if (Bishops(side) & dark_squares &&
                !(Bishops(enemy) & dark_squares)) {
              if (file_mask[dark_corner[side]] & Pawns(side))
                continue;
            } else if (file_mask[light_corner[side]] & Pawns(side))
              continue;
          }
        }
        if (!(Pawns(side) & file_mask[FILEA]) ||
            !(Pawns(side) & file_mask[FILEH])) {
          if (Pawns(side) & file_mask[FILEA]) {
            int fkd, ekd, pd;

            ekd = Distance(KingSQ(enemy), sqflip[side][A8]) - (wtm != side);
            if (ekd <= 1)
              return (0);
            else {
              fkd = Distance(KingSQ(side), sqflip[side][A8]) - (wtm == side);
              pd = Distance(Advanced(side, Pawns(side) & file_mask[FILEA]),
                  sqflip[side][A8]) - (wtm == side);
              if (ekd < fkd && ekd < pd)
                return (0);
            }
            continue;
          } else {
            int fkd, ekd, pd;

            ekd = Distance(KingSQ(enemy), sqflip[side][H8]) - (wtm != side);
            if (ekd <= 1)
              return (0);
            else {
              fkd = Distance(KingSQ(side), sqflip[side][H8]) - (wtm == side);
              pd = Distance(Advanced(side, Pawns(side) & file_mask[FILEH]),
                  sqflip[side][H8]) - (wtm == side);
              if (ekd < fkd && ekd < pd)
                return (0);
            }
            continue;
          }
        }
      }
    }
    while (0);
/*
 ************************************************************
 *                                                          *
 *   if both sides have pawns, the game is not a draw for   *
 *   lack of material.  also, if one side has at least a    *
 *   B+N, then it's not a drawn position.                   *
 *                                                          *
 ************************************************************
 */
  if (TotalPawns(side) && TotalPawns(enemy))
    return (1);
/*
 ************************************************************
 *                                                          *
 *   if one side has two bishops, and the other side has    *
 *   a single kinght, the two bishops win.                  *
 *                                                          *
 ************************************************************
 */
  if (TotalPawns(side) == 0 && TotalPieces(side) == 6 && TotalPieces(enemy) == 3
      && (Knights(side) || !Knights(enemy)))
    return (0);
/*
 ************************************************************
 *                                                          *
 *   if one side is two knights ahead and the opponent has  *
 *   no remaining material, it is a draw.                   *
 *                                                          *
 ************************************************************
 */
  if (TotalPawns(side) == 0 && TotalPieces(side) == 6 && !Bishops(side) &&
      TotalPieces(enemy) + TotalPawns(enemy) == 0)
    return (0);
/*
 ************************************************************
 *                                                          *
 *   check to see if this is a KRP vs KR type ending.  if   *
 *   so, and the losing king is in front of the passer,     *
 *   then this is a drawn ending.                           *
 *                                                          *
 ************************************************************
 */
  if (TotalPawns(side) == 1 && TotalPawns(enemy) == 0 && TotalPieces(side) == 5
      && TotalPieces(enemy) == 5) {
    square = LSB(Pawns(side));
    if (FileDistance(KingSQ(enemy), square) <= 1 &&
        FrontOf(side, Rank(KingSQ(enemy)), Rank(square)))
      return (0);
    else if (FileDistance(KingSQ(side), square) > 1 ||
        Behind(side, Rank(KingSQ(side)), Rank(square)))
      return (0);
  }
  return (1);
}
