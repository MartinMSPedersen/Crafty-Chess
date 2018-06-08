#include "chess.h"
#include "data.h"
/* last modified 09/27/08 */
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
  register int score, can_win = 3;
  register int side, majors, minors;
  register int phase, lscore, totalBR, totalPc, cutoff;

/*
 **********************************************************************
 *                                                                    *
 *   initialize.                                                      *
 *                                                                    *
 **********************************************************************
 */
  tree->dangerous[white] = (Queens(white) && TotalPieces(white, occupied) > 13)
      || (TotalPieces(white, rook) > 1 && TotalPieces(white, occupied) > 15);
  tree->dangerous[black] = (Queens(black) && TotalPieces(black, occupied) > 13)
      || (TotalPieces(black, rook) > 1 && TotalPieces(black, occupied) > 15);
  phase = Min(62, TotalPieces(white, occupied) + TotalPieces(black, occupied));
  tree->evaluations++;
  tree->score_mg = 0;
  tree->score_eg = 0;
  EvaluateMaterial(tree, wtm);
#ifdef DEBUGEV
  printf("score[material] (MG) =              %4d\n", tree->score_mg);
  printf("score[material] (EG) =              %4d\n", tree->score_eg);
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
  if (TotalPieces(white, occupied) < 13 && TotalPieces(black, occupied) < 13)
    do {
/*
 ************************************************************
 *                                                          *
 *   if neither side has any pieces, and both sides have    *
 *   non-rookpawns, then either side can win.               *
 *                                                          *
 ************************************************************
 */
      if (TotalPieces(white, occupied) == 0 && TotalPieces(black, occupied) == 0
          && (Pawns(white) & not_rook_pawns && Pawns(black) & not_rook_pawns))
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
          TotalPieces(white, rook) + 2 * TotalPieces(white,
          queen) - TotalPieces(black, rook) - 2 * TotalPieces(black, queen);
      if (abs(majors) == 1) {
        minors =
            TotalPieces(white, knight) + TotalPieces(white,
            bishop) - TotalPieces(black, knight) - TotalPieces(black, bishop);
        if (majors == -minors) {
          if (TotalPieces(black, pawn) == 0)
            can_win &= 1;
          if (TotalPieces(white, pawn) == 0)
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
    if (TotalPieces(white, occupied) > 3 || TotalPieces(black, occupied) > 3) {
      if (Material > 0)
        EvaluateMate(tree, white);
      else if (Material < 0)
        EvaluateMate(tree, black);
      if (tree->score_eg > DrawScore(1) && !(can_win & 1))
        tree->score_eg = tree->score_eg / 4;
      if (tree->score_eg < DrawScore(1) && !(can_win & 2))
        tree->score_eg = tree->score_eg / 4;
      return ((wtm) ? tree->score_eg : -tree->score_eg);
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
  else {
    if (PawnHashKey == tree->pawn_score.key) {
      tree->score_mg += tree->pawn_score.score_mg;
      tree->score_eg += tree->pawn_score.score_eg;
    }
/*
 ************************************************************
 *                                                          *
 *   first check to see if this position has been handled   *
 *   before.  if so, we can skip the work saved in the pawn *
 *   hash table.                                            *
 *                                                          *
 ************************************************************
 */
    else {
      ptable = pawn_hash_table + (PawnHashKey & pawn_hash_mask);
      if (ptable->key == PawnHashKey)
        tree->pawn_score = *ptable;
      else {
        tree->pawn_score.key = PawnHashKey;
        tree->pawn_score.open_files = 255;
        tree->pawn_score.score_mg = 0;
        tree->pawn_score.score_eg = 0;
        for (side = black; side <= white; side++)
          EvaluatePawns(tree, side);
        *ptable = tree->pawn_score;
      }
      tree->score_mg += tree->pawn_score.score_mg;
      tree->score_eg += tree->pawn_score.score_eg;
    }
#ifdef DEBUGEV
    printf("score[pawns] (MG) =                 %4d\n", tree->score_mg);
    printf("score[pawns] (EG) =                 %4d\n", tree->score_eg);
#endif
/*
 **********************************************************************
 *                                                                    *
 *   if there are any passed pawns, first call EvaluatePassedPawns()  *
 *   to evaluate them.  then, if one side has a passed pawn and the   *
 *   other side has no pieces, call EvaluatePassedPawnRaces() to see  *
 *   if the passed pawn can be stopped from promoting.                *
 *                                                                    *
 **********************************************************************
 */
    if (tree->pawn_score.passed[black] || tree->pawn_score.passed[white]) {
      for (side = black; side <= white; side++)
        if (tree->pawn_score.passed[side])
          EvaluatePassedPawns(tree, side);
      if ((TotalPieces(white, occupied) == 0 && tree->pawn_score.passed[black])
          || (TotalPieces(black, occupied) == 0 &&
              tree->pawn_score.passed[white]))
        EvaluatePassedPawnRaces(tree, wtm);
    }
#ifdef DEBUGEV
    printf("score[passed pawns] (MG) =          %4d\n", tree->score_mg);
    printf("score[passed pawns] (EG) =          %4d\n", tree->score_eg);
#endif
  }
/*
 **********************************************************************
 *                                                                    *
 *   call EvaluateDevelopment() to evaluate development.  note that   *
 *   we only do this when either side has not castled at the root.    *
 *                                                                    *
 **********************************************************************
 */
  for (side = black; side <= white; side++)
    if (Castle(1, side))
      EvaluateDevelopment(tree, ply, side);
#ifdef DEBUGEV
  printf("score[development]=                 %4d\n", tree->score_mg);
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
  score = ((tree->score_mg * phase) + (tree->score_eg * (62 - phase))) / 62;
  lscore = (wtm) ? score : -score;
  totalBR =
      TotalPieces(white, bishop) + TotalPieces(black,
      bishop) + TotalPieces(white, rook) + TotalPieces(black, rook);
  totalPc =
      totalBR + TotalPieces(white, knight) + TotalPieces(white,
      queen) * 2 + TotalPieces(black, knight) + TotalPieces(black, queen) * 2;
  cutoff = lazy_eval_cutoff + totalPc * 4;
  if (!((lscore - cutoff >= beta) || (lscore + cutoff <= alpha))) {
    for (side = black; side <= white; side++) {
      EvaluateKnights(tree, side);
      EvaluateBishops(tree, side);
      EvaluateRooks(tree, side);
      EvaluateQueens(tree, side);
    }
    for (side = black; side <= white; side++)
      EvaluateKings(tree, ply, side);
  }
#ifdef DEBUGEV
  printf("score[pieces]= (MG)                 %4d\n", score);
  printf("score[pieces]= (EG)                 %4d\n", score);
#endif
/*
 **********************************************************************
 *                                                                    *
 *   now adjust the score if the game is drawish but one side appears *
 *   to be significantly better according to the computed score.      *
 *                                                                    *
 **********************************************************************
 */
  score = ((tree->score_mg * phase) + (tree->score_eg * (62 - phase))) / 62;
  score = EvaluateDraws(tree, ply, can_win, score);
#if defined(SKILL)
  score =
      skill * score / 100 + ((100 -
          skill) * PAWN_VALUE * (BITBOARD) Random32() / 0x100000000ULL) / 100;
#endif
  return ((wtm) ? score : -score);
}

/* last modified 09/27/08 */
/*
 *******************************************************************************
 *                                                                             *
 *   EvaluateBishops() is used to evaluate black/white bishops.                *
 *                                                                             *
 *******************************************************************************
 */
void EvaluateBishops(TREE * RESTRICT tree, int side)
{
  register BITBOARD temp, moves;
  register int square, trop;
  register int i;
  register int score_eg = 0, score_mg = 0, enemy = Flip(side);
  int pair = (Bishops(side) & (Bishops(side) - 1)) != 0;

/*
 ************************************************************
 *                                                          *
 *   first, locate each bishop and add in its piece/square  *
 *   score.                                                 *
 *                                                          *
 ************************************************************
 */
  temp = Bishops(side);
  while (temp) {
    square = LSB(temp);
    score_mg += bval[mg][side][square];
    score_eg += bval[eg][side][square];
/*
 ************************************************************
 *                                                          *
 *   now, evaluate for "outposts" which is a bishop that    *
 *   can't be driven off by an enemy pawn, and which is     *
 *   supported by a friendly pawn.                          *
 *                                                          *
 *   if the enemy has NO minor to take this bishop, then    *
 *   increase the bonus.                                    *
 *                                                          *
 ************************************************************
 */
    if (!(mask_no_pattacks[enemy][square] & Pawns(enemy))) {
      if (bishop_outpost[side][square]) {
        score_eg += bishop_outpost[side][square];
        score_mg += bishop_outpost[side][square];
        if (bishop_outpost[side][square] &&
            pawn_attacks[enemy][square] & Pawns(side)) {
          score_eg += bishop_outpost[side][square] / 2;
          score_mg += bishop_outpost[side][square] / 2;
          if (!Knights(enemy) && !(Color(square) & Bishops(enemy))) {
            score_eg += bishop_outpost[side][square];
            score_mg += bishop_outpost[side][square];
          }
        }
      }
    }
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
    if (square == sqflip[side][A7] && SetMask(sqflip[side][B6]) & Pawns(enemy)) {
      score_eg -= bishop_trapped;
      score_mg -= bishop_trapped;
    } else if (square == sqflip[side][B8] &&
        SetMask(sqflip[side][C7]) & Pawns(enemy)) {
      score_eg -= bishop_trapped;
      score_mg -= bishop_trapped;
    } else if (square == sqflip[side][H7] &&
        SetMask(sqflip[side][G6]) & Pawns(enemy)) {
      score_eg -= bishop_trapped;
      score_mg -= bishop_trapped;
    } else if (square == sqflip[side][G8] &&
        SetMask(sqflip[side][F7]) & Pawns(enemy)) {
      score_eg -= bishop_trapped;
      score_mg -= bishop_trapped;
    }
/*
 ************************************************************
 *                                                          *
 *   mobility counts the number of squares the piece        *
 *   attacks, excluding squares with friendly pieces, and   *
 *   weighs each square according to centralization.        *
 *                                                          *
 ************************************************************
 */
    moves = AttacksBishop(square, OccupiedSquares) & ~Occupied(side);
    moves |= SetMask(square);
    score_mg -= lower_b;
    score_eg -= lower_b;
    for (i = 0; i < 4; i++) {
      score_mg +=
          PopCnt(moves & mobility_mask_b[i]) * mobility_score_b[pair][i];
      score_eg +=
          PopCnt(moves & mobility_mask_b[i]) * mobility_score_b[pair][i];
    }
/*
 ************************************************************
 *                                                          *
 *   check for pawns on both wings, which makes a bishop    *
 *   even more valuable against an enemy knight             *
 *                                                          *
 ************************************************************
 */
    if (tree->all_pawns & mask_fgh && tree->all_pawns & mask_abc) {
      score_mg += bishop_with_wing_pawns[mg];
      score_eg += bishop_with_wing_pawns[eg];
    }
/*
 ************************************************************
 *                                                          *
 *   adjust the tropism count for this piece.               *
 *                                                          *
 ************************************************************
 */
    if (tree->dangerous[side]) {
      trop =
          ((AttacksBishop(square,
                  OccupiedSquares & ~(Queens(side)))) &
          king_attacks[KingSQ(enemy)]) ? 1 : Distance(square, KingSQ(enemy));
      tree->tropism[side] += king_tropism_b[trop];
    }
    temp &= temp - 1;
  }
  tree->score_mg += sign[side] * score_mg;
  tree->score_eg += sign[side] * score_eg;
#ifdef DEBUGEV
  printf("score[bishops(%d), MG]=            %4d\n", side, score_mg);
  printf("score[bishops(%d), EG]=            %4d\n", side, score_eg);
  printf("tropism[bishops(%d)]=              %4d\n", side, tree->tropism[side]);
#endif
}

/* last modified 09/27/08 */
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
void EvaluateDevelopment(TREE * RESTRICT tree, int ply, int side)
{
  register int score_mg = 0;
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
      score_mg -= development_thematic;
  }
#ifdef DEBUGDV
  printf("development(%d).1 score_mg=%d\n", side, score_mg);
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
        score_mg -= oq * development_losing_castle;
      else if (Castle(ply, side) > 0)
        score_mg -= (oq * development_losing_castle) / 2;
    } else
      score_mg -= development_not_castled;
  }
#ifdef DEBUGDV
  printf("development(%d).2 score_mg=%d\n", side, score_mg);
#endif
/*
 ************************************************************
 *                                                          *
 *   check for an undeveloped knight/rook combo             *
 *                                                          *
 ************************************************************
 */
  if (PcOnSq(sqflip[side][B1]) == pieces[side][knight] &&
      PcOnSq(sqflip[side][A1]) == pieces[side][rook])
    score_mg -= undeveloped_piece;
  if (PcOnSq(sqflip[side][G1]) == pieces[side][knight] &&
      PcOnSq(sqflip[side][H1]) == pieces[side][rook])
    score_mg -= undeveloped_piece;
  tree->score_mg += sign[side] * score_mg;
}

/* last modified 09/27/08 */
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
  if (TotalPieces(white, occupied) <= 8 && TotalPieces(black, occupied) <= 8) {
    if (TotalPieces(white, bishop) == 1 && TotalPieces(black, bishop) == 1) {
      if (square_color[LSB(Bishops(black))] !=
          square_color[LSB(Bishops(white))]) {
        if (TotalPieces(white, occupied) == 3 &&
            TotalPieces(black, occupied) == 3 && ((TotalPieces(white, pawn) < 4
                    && TotalPieces(black, pawn) < 4) ||
                abs(TotalPieces(white, pawn) - TotalPieces(black, pawn)) < 2))
          score = score / 2;
        else if (TotalPieces(white, occupied) == TotalPieces(black, occupied))
          score = 3 * score / 4;
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
  printf("score[draws] =                      %4d\n", score);
#endif
  return (score);
}

/* last modified 09/27/08 */
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

/* last modified 09/27/08 */
/*
 *******************************************************************************
 *                                                                             *
 *   EvaluateKings() is used to evaluate black/white kings.                    *
 *                                                                             *
 *******************************************************************************
 */
void EvaluateKings(TREE * RESTRICT tree, int ply, int side)
{
  register int score_eg = 0, score_mg = 0, defects;
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
  if (tree->all_pawns) {
    if (tree->all_pawns & mask_efgh && tree->all_pawns & mask_abcd)
      score_eg += kval_n[side][KingSQ(side)];
    else if (tree->all_pawns & mask_efgh)
      score_eg += kval_k[side][KingSQ(side)];
    else
      score_eg += kval_q[side][KingSQ(side)];
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
  if (tree->dangerous[enemy]) {
    if (trojan_check) {
      if (root_wtm == side && File(KingSQ(side)) >= FILEE) {
        if (!(tree->all_pawns & file_mask[FILEH])) {
          if (Rooks(enemy) && Queens(enemy))
            score_mg -= king_safety_mate_threat;
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
    score_mg -= king_safety[defects][tree->tropism[enemy]];
  }
  tree->score_mg += sign[side] * score_mg;
  tree->score_eg += sign[side] * score_eg;
#ifdef DEBUGEV
  printf("score[defects(%d)]=                %4d\n", side, defects);
  printf("score[tropism(%d)]=                %4d\n", enemy,
      tree->tropism[enemy]);
  printf("score[kings(%d) [MG]=              %4d\n", side, score_mg);
  printf("score[kings(%d) [EG]=              %4d\n", side, score_eg);
#endif
}

/* last modified 09/27/08 */
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

/* last modified 09/27/08 */
/*
 *******************************************************************************
 *                                                                             *
 *   EvaluateKnights() is used to evaluate black/white knights.                *
 *                                                                             *
 *******************************************************************************
 */
void EvaluateKnights(TREE * RESTRICT tree, int side)
{
  register BITBOARD temp, moves;
  register int square, i, score_eg = 0, score_mg = 0;
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
    score_mg += nval[mg][side][square];
    score_eg += nval[eg][side][square];
/*
 ************************************************************
 *                                                          *
 *   now, evaluate for "outposts" which is a knight that    *
 *   can't be driven off by an enemy pawn, and which is     *
 *   supported by a friendly pawn.                          *
 *                                                          *
 *   if the enemy has NO minor to take this knight, then    *
 *   increase the bonus.                                    *
 *                                                          *
 ************************************************************
 */
    if (!(mask_no_pattacks[enemy][square] & Pawns(enemy))) {
      if (knight_outpost[side][square]) {
        score_eg += knight_outpost[side][square];
        score_mg += knight_outpost[side][square];
        if (knight_outpost[side][square] &&
            pawn_attacks[enemy][square] & Pawns(side)) {
          score_eg += knight_outpost[side][square] / 2;
          score_mg += knight_outpost[side][square] / 2;
          if (!Knights(enemy) && !(Color(square) & Bishops(enemy))) {
            score_eg += knight_outpost[side][square];
            score_mg += knight_outpost[side][square];
          }
        }
      }
    }
/*
 ************************************************************
 *                                                          *
 *   mobility counts the number of squares the piece        *
 *   attacks, excluding squares with friendly pieces, and   *
 *   weighs each square according to centralization.        *
 *                                                          *
 ************************************************************
 */
    moves = AttacksKnight(square) & ~Occupied(side);
    moves |= SetMask(square);
    score_mg -= lower_n;
    score_eg -= lower_n;
    for (i = 0; i < 4; i++) {
      score_mg += PopCnt(moves & mobility_mask_n[i]) * mobility_score_n[i];
      score_eg += PopCnt(moves & mobility_mask_n[i]) * mobility_score_n[i];
    }
/*
 ************************************************************
 *                                                          *
 *   adjust the tropism count for this piece.               *
 *                                                          *
 ************************************************************
 */
    if (tree->dangerous[side]) {
      i = Distance(square, KingSQ(enemy));
      tree->tropism[side] += king_tropism_n[i];
    }
    temp &= temp - 1;
  }
  tree->score_mg += sign[side] * score_mg;
  tree->score_eg += sign[side] * score_eg;
#ifdef DEBUGEV
  printf("score[knights(%d), MG]=            %4d\n", side, score_mg);
  printf("score[knights(%d), EG]=            %4d\n", side, score_eg);
  printf("tropism[knights(%d)]=              %4d\n", side, tree->tropism[side]);
#endif
}

/* last modified 09/27/08 */
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
void EvaluateMate(TREE * RESTRICT tree, int side)
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
  if (TotalPieces(enemy, occupied) == 0 && TotalPieces(side, occupied) == 6 &&
      TotalPieces(side, bishop) == 1) {
    if (dark_squares & Bishops(side))
      mate_score = b_n_mate_dark_squares[KingSQ(enemy)];
    else
      mate_score = b_n_mate_light_squares[KingSQ(enemy)];
  }
/*
 ************************************************************
 *                                                          *
 *   if one side is winning, force the enemy king to the    *
 *   edge of the board.                                     *
 *                                                          *
 ************************************************************
 */
  else {
    mate_score = mate[KingSQ(enemy)];
    mate_score -=
        (Distance(KingSQ(side), KingSQ(enemy)) - 3) * king_king_tropism;
  }
  tree->score_mg += sign[side] * mate_score;
  tree->score_eg += sign[side] * mate_score;
}

/* last modified 09/27/08 */
/*
 *******************************************************************************
 *                                                                             *
 *   EvaluateMaterial() is used to evaluate material on the board.  it really  *
 *   accomplishes detecting cases where one side has made a 'bad trade' as the *
 *   comments below show.                                                      *
 *                                                                             *
 *******************************************************************************
 */
void EvaluateMaterial(TREE * RESTRICT tree, int wtm)
{
  register int score_mg, score_eg, majors, minors, side;

/*
 **********************************************************************
 *                                                                    *
 *   we start with the raw Material balance for the current position, *
 *   then we add in the dynamic material adjustments that depend on   *
 *   the number of pawns and piece counts.                            *
 *                                                                    *
 **********************************************************************
 */
  score_mg = Material + ((wtm) ? wtm_bonus[mg] : -wtm_bonus[mg]);
  score_eg = Material + ((wtm) ? wtm_bonus[eg] : -wtm_bonus[eg]);
  for (side = black; side <= white; side++)
    EvaluateMaterialDynamic(tree, side);
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
      TotalPieces(white, rook) + 2 * TotalPieces(white,
      queen) - TotalPieces(black, rook) - 2 * TotalPieces(black, queen);
  minors =
      TotalPieces(white, knight) + TotalPieces(white,
      bishop) - TotalPieces(black, knight) - TotalPieces(black, bishop);
  if (majors || minors)
    if (Abs(TotalPieces(white, occupied) - TotalPieces(black, occupied)) != 2) {
      if (TotalPieces(white, occupied) > TotalPieces(black, occupied)) {
        score_mg += bad_trade;
        score_eg += bad_trade;
      } else if (TotalPieces(black, occupied) > TotalPieces(white, occupied)) {
        score_mg -= bad_trade;
        score_eg -= bad_trade;
      }
    }
  tree->score_mg += score_mg;
  tree->score_eg += score_eg;
#ifdef DEBUGM
  printf
      ("Majors=%d  Minors=%d  TotalPieces(white, occupied)=%d  TotalPieces(black, occupied)=%d\n",
      Majors, Minors, TotalPieces(white, occupied), TotalPieces(black,
          occupied));
  printf("score[bad trade] = (MG)             %4d\n", tree->score_mg);
  printf("score[bad trade] = (EG)             %4d\n", tree->score_eg);
#endif
}

/* last modified 09/27/08 */
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
void EvaluateMaterialDynamic(TREE * RESTRICT tree, int side)
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
  tp = TotalPieces(white, pawn) + TotalPieces(black, pawn);
  if (TotalPieces(side, knight)) {
    score += 2 * tp - 16;
    if (TotalPieces(side, knight) > 1)
      score -= 11;
  }
  if (TotalPieces(side, bishop) > 1)
    score += 13;
  if (Queens(side))
    score +=
        tp + TotalPieces(white, knight) + TotalPieces(white,
        bishop) + TotalPieces(black, knight) + TotalPieces(black, bishop);
  score += TotalPieces(side, rook) * (32 - tp * 2);
  tree->score_mg += sign[side] * score;
  tree->score_eg += sign[side] * score;
}

/* last modified 10/29/08 */
/*
 *******************************************************************************
 *                                                                             *
 *   EvaluatePassedPawns() is used to evaluate passed pawns and the danger     *
 *   they produce.  This code considers pieces as well, so it has been         *
 *   separated from the normal EvaluatePawns() code that hashes information    *
 *   based only on pawn positions.                                             *
 *                                                                             *
 *******************************************************************************
 */
void EvaluatePassedPawns(TREE * RESTRICT tree, int side)
{
  register int file, square, score_mg = 0, score_eg = 0;
  register int king_sq, pawns, rank, rank_bonus, blocking;
  register int enemy = Flip(side);

/*
 ************************************************************
 *                                                          *
 *   Initialize.                                            *
 *                                                          *
 ************************************************************
 */
  king_sq = KingSQ(side);
  pawns = tree->pawn_score.passed[side];
  while (pawns) {
    file = LSB8Bit(pawns);
    pawns &= pawns - 1;
    square = Advanced(side, Pawns(side) & file_mask[file]);
    rank = Rank(square);
    rank_bonus =
        Max(0,
        (rankflip[side][rank] - RANK2) * (rankflip[side][rank] - RANK2 - 1));
#ifdef DEBUGPP
    printf("score[passed pawns %s] on square %d\n", (side) ? "white" : "black",
        square);
#endif
/*
 *****************************************************
 *                                                   *
 *   We have located the most advanced pawn on this  *
 *   file, which is the only one that will get any   *
 *   sort of bonus.  add in the MG/EG scores first.  *
 *                                                   *
 *****************************************************
 */
    score_mg += passed_pawn_value[mg][side][rank];
    score_eg += passed_pawn_value[eg][side][rank];
#ifdef DEBUGPP
    printf("score[passed rank %s] = (MG)       %d\n",
        (side) ? "white" : "black", score_mg);
    printf("score[passed rank %s] = (EG)       %d\n",
        (side) ? "white" : "black", score_eg);
#endif
/*
 *****************************************************
 *                                                   *
 *   add in a bonus if the passed pawn is connected  *
 *   with another pawn for support.                  *
 *                                                   *
 *****************************************************
 */
    if (mask_pawn_connected[square] & Pawns(side)) {
      score_mg += connected_passed_pawn_value[mg] * rank_bonus;
      score_eg += connected_passed_pawn_value[eg] * rank_bonus;
    }
#ifdef DEBUGPP
    printf("score[passed connected %s] = (MG)       %d\n",
        (side) ? "white" : "black", score_mg);
    printf("score[passed connected %s] = (EG)       %d\n",
        (side) ? "white" : "black", score_eg);
#endif
/*
 *****************************************************
 *                                                   *
 *   if the pawn is blockaded by an enemy piece, it  *
 *   cannot move and is therefore not nearly as      *
 *   valuable as if it were free to advance.         *
 *                                                   *
 *****************************************************
 */
    blocking = square + direction[side];
    if (PcOnSq(blocking) * sign[side] < 0) {
      score_mg -= blockading_passed_pawn_value[mg][side][Rank(square)];
      score_eg -= blockading_passed_pawn_value[eg][side][Rank(square)];
    }
#ifdef DEBUGPP
    printf("score[passed blocked %s] = (MG)       %d\n",
        (side) ? "white" : "black", score_mg);
    printf("score[passed blocked %s] = (EG)       %d\n",
        (side) ? "white" : "black", score_eg);
#endif
/*
 *****************************************************
 *                                                   *
 *   add in a bonus based on how close the friendly  *
 *   king is, and a penalty based on how close the   *
 *   enemy king is.  the bonus/penalty is based on   *
 *   how advanced the pawn is to attract the kings   *
 *   toward the most advanced (and most dangerous)   *
 *   passed pawn.                                    *
 *                                                   *
 *****************************************************
 */
    score_eg -=
        Distance(blocking, KingSQ(side)) * rank_bonus - Distance(blocking,
        KingSQ(enemy)) * rank_bonus;
#ifdef DEBUGPP
    printf("score[passed supported %s] = (EG)       %d\n",
        (side) ? "white" : "black", score_eg);
#endif
  }
/*
 ************************************************************
 *                                                          *
 *   check to see if side has an outside passed pawn.       *
 *                                                          *
 ************************************************************
 */
  if (tree->pawn_score.passed[side]) {
    if (is_outside[tree->pawn_score.passed[side]][tree->pawn_score.all[enemy]]) {
      score_mg += outside_passed[mg];
      score_eg += outside_passed[eg];
    }
  }
#ifdef DEBUGP
  printf("score_mg.4 after %s outside = %d\n", (side) ? "white" : "black",
      score_mg);
  printf("score_eg.4 after %s outside = %d\n", (side) ? "white" : "black",
      score_eg);
#endif
  tree->score_mg += sign[side] * score_mg;
  tree->score_eg += sign[side] * score_eg;
}

/* last modified 09/27/08 */
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
void EvaluatePassedPawnRaces(TREE * RESTRICT tree, int wtm)
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
    if (Pawns(side) && !Pawns(enemy) && TotalPieces(white, occupied) == 0 &&
        TotalPieces(black, occupied) == 0) {
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
                      sqflip[side][A8]))) {
            tree->score_eg += sign[side] * pawn_can_promote;
            return;
          }
          continue;
        } else if (File(pawnsq) == FILEH) {
          if ((File(KingSQ(side)) == FILEG) &&
              (Distance(KingSQ(side),
                      sqflip[side][H8]) < Distance(KingSQ(enemy),
                      sqflip[side][H8]))) {
            tree->score_eg += sign[side] * pawn_can_promote;
            return;
          }
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
              sign[side] * (Rank(pawnsq) - 1 + 2 * side)) {
            tree->score_eg += sign[side] * pawn_can_promote;
            return;
          }
          if (Rank(KingSQ(side)) == rankflip[side][RANK6]) {
            tree->score_eg += sign[side] * pawn_can_promote;
            return;
          }
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
            EvaluateHasOpposition(wtm == side, KingSQ(side), KingSQ(enemy))) {
          tree->score_eg += sign[side] * pawn_can_promote;
          return;
        }
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
    if (TotalPieces(enemy, occupied) == 0 && tree->pawn_score.passed[side]) {
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
    return;
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
    return;
  if ((queener[white] < 8) && (queener[black] == 8)) {
    tree->score_eg += pawn_can_promote + (5 - queener[white]) * 10;
    return;
  } else if ((queener[black] < 8) && (queener[white] == 8)) {
    tree->score_eg += -(pawn_can_promote + (5 - queener[black]) * 10);
    return;
  }
  if (queener[white] < queener[black] && forced_km[white] && !forced_km[black]) {
    tree->score_eg += pawn_can_promote + (5 - queener[white]) * 10;
    return;
  } else if (queener[black] < queener[white] && forced_km[black] &&
      forced_km[white]) {
    tree->score_eg += -(pawn_can_promote + (5 - queener[black]) * 10);
    return;
  }
}

/* last modified 09/26/08 */
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
void EvaluatePawns(TREE * RESTRICT tree, int side)
{
  register BITBOARD pawns;
  register BITBOARD temp;
  BITBOARD p_moves[2];
  register int pns, square, file, rank, score_eg = 0, score_mg = 0;
  register int defenders, attackers, sq;
  register int enemy = Flip(side);

/*
 ************************************************************
 *                                                          *
 *   initialize.                                            *
 *                                                          *
 ************************************************************
 */
  tree->pawn_score.all[side] = 0;
  tree->pawn_score.candidates[side] = 0;
  tree->pawn_score.passed[side] = 0;
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
    file = File(square);
    rank = Rank(square);
    pawns &= pawns - 1;
/*
 ************************************************************
 *                                                          *
 *   the first thing we do is make a note that the current  *
 *   file can't be open since there is a pawn on it.        *
 *                                                          *
 ************************************************************
 */
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
    score_mg += pval[mg][side][square];
    score_eg += pval[eg][side][square];
#ifdef DEBUGP
    printf("%s pawn[static] file=%d, score_mg=%d\n", (side) ? "white" : "black",
        file, score_mg);
    printf("%s pawn[static] file=%d, score_eg=%d\n", (side) ? "white" : "black",
        file, score_eg);
#endif
/*
 ************************************************************
 *                                                          *
 *   evaluate isolated pawns, which  are penalized based on *
 *   the file, with central isolani being worse than when   *
 *   on the wings.                                          *
 *                                                          *
 ************************************************************
 */
    if (!(mask_pawn_isolated[square] & Pawns(side))) {
      score_mg -= pawn_isolated[mg];
      score_eg -= pawn_isolated[eg];
      if (!(Pawns(enemy) & file_mask[file])) {
        score_mg -= pawn_isolated[mg] / 2;
        score_eg -= pawn_isolated[eg] / 2;
      }
    }
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
          score_mg -= pawn_weak[mg];
          score_eg -= pawn_weak[eg];
          if (!(Pawns(enemy) & file_mask[file])) {
            score_mg -= pawn_weak[mg] / 2;
/* score_eg -= pawn_weak[eg] / 2; */
          }
        }
      } while (0);
#ifdef DEBUGP
      printf("%s pawn[weak] file=%d, score_mg=%d\n", (side) ? "white" : "black",
          file, score_mg);
      printf("%s pawn[weak] file=%d, score_eg=%d\n", (side) ? "white" : "black",
          file, score_eg);
#endif
/*
 ************************************************************
 *                                                          *
 *   evaluate doubled pawns.  if there are other pawns on   *
 *   this file, penalize this pawn.                         *
 *                                                          *
 ************************************************************
 */
      if ((pns = PopCnt(file_mask[file] & Pawns(side))) > 1) {
        score_mg -= doubled_pawn_value[mg];
        score_eg -= doubled_pawn_value[eg];
      }
#ifdef DEBUGP
      printf("%s pawn[doubled] file=%d, score_mg=%d\n",
          (side) ? "white" : "black", file, score_mg);
      printf("%s pawn[doubled] file=%d, score_eg=%d\n",
          (side) ? "white" : "black", file, score_eg);
#endif
/*
 ************************************************************
 *                                                          *
 *  test the pawn to see it if forms a "duo" which is two   *
 *  pawns side-by-side.                                     *
 *                                                          *
 ************************************************************
 */
      if (mask_pawn_duo[square] & Pawns(side)) {
        score_mg += pawn_duo[mg];
        score_eg += pawn_duo[eg];
      }
    }
#ifdef DEBUGP
    printf("%s pawn[duo] file=%d, score_mg=%d\n", (side) ? "white" : "black",
        file, score_mg);
    printf("%s pawn[duo] file=%d, score_eg=%d\n", (side) ? "white" : "black",
        file, score_eg);
#endif
/*
 ************************************************************
 *                                                          *
 *   discover and flag passed pawns for use later.          *
 *                                                          *
 ************************************************************
 */
    if (!(mask_pawn_passed[side][square] & Pawns(enemy))) {
      tree->pawn_score.passed[side] |= 1 << file;
#ifdef DEBUGP
      printf("%s pawn[passed] file=%d\n", (side) ? "white" : "black", file);
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
            score_mg += passed_pawn_candidate[mg][side][rank];
            score_eg += passed_pawn_candidate[eg][side][rank];
          }
        }
      }
    }
#ifdef DEBUGP
    if (tree->pawn_score.candidates[side] & (1 << file))
      printf("%s pawn[candidate] square=%d\n", (side) ? "white" : "black",
          square);
    printf("%s pawn[candidate] file=%d, score_mg=%d\n",
        (side) ? "white" : "black", file, score_mg);
    printf("%s pawn[candidate] file=%d, score_eg=%d\n",
        (side) ? "white" : "black", file, score_eg);
#endif
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
                !(mask_hidden_left[side][File(square)] & Pawns(enemy))))) {
      score_mg += passed_pawn_hidden[mg];
      score_eg += passed_pawn_hidden[eg];
    }
#ifdef DEBUGP
    printf("%s pawn[hidden] file=%d, score_mg=%d\n", (side) ? "white" : "black",
        file, score_mg);
    printf("%s pawn[hidden] file=%d, score_eg=%d\n", (side) ? "white" : "black",
        file, score_eg);
#endif
  }
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
  tree->pawn_score.score_mg += sign[side] * score_mg;
  tree->pawn_score.score_eg += sign[side] * score_eg;
}

/* last modified 09/18/08 */
/*
 *******************************************************************************
 *                                                                             *
 *   EvaluateQueens() is used to evaluate black/white queens.                  *
 *                                                                             *
 *******************************************************************************
 */
void EvaluateQueens(TREE * RESTRICT tree, int side)
{
  register BITBOARD temp;
  register int square, score_mg = 0, score_eg = 0;
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
/*
 ************************************************************
 *                                                          *
 *   first, add in the piece/square table value for the     *
 *   queen                                                  *
 *                                                          *
 ************************************************************
*/
    score_mg += qval[mg][side][square];
    score_eg += qval[eg][side][square];
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
    if (tree->dangerous[side]) {
      trop = Distance(square, KingSQ(enemy));
      tree->tropism[side] += king_tropism_q[trop];
    }
    tree->tropism[enemy] -= friendly_queen[Distance(square, KingSQ(side))];
    temp &= temp - 1;
  }
  tree->score_mg += sign[side] * score_mg;
  tree->score_eg += sign[side] * score_eg;
#ifdef DEBUGEV
  printf("score[queens(%d) [MG]]=            %4d\n", side, score_mg);
  printf("score[queens(%d) [EG]]=            %4d\n", side, score_eg);
  printf("tropism[queens(%d)]=               %4d\n", side, tree->tropism[side]);
#endif
}

/* last modified 09/10/08 */
/*
 *******************************************************************************
 *                                                                             *
 *   EvaluateRooks() is used to evaluate black/white rooks.                    *
 *                                                                             *
 *******************************************************************************
 */
void EvaluateRooks(TREE * RESTRICT tree, int side)
{
  register BITBOARD temp, moves;
  register int square, file, trop, tscore = 0, i;
  register int score_mg = 0, score_eg = 0, pawnsq;
  register int enemy = Flip(side);

/*
 ************************************************************
 *                                                          *
 *   Initialize.                                            *
 *                                                          *
 ************************************************************
 */
/*
  open_files = PopCnt8Bit(tree->pawn_score.open_files);
*/
  temp = Rooks(side);
  while (temp) {
    square = LSB(temp);
    file = File(square);
/*
 ************************************************************
 *                                                          *
 *   determine if the rook is on an open file or on a half- *
 *   open file, either of which increases its ability to    *
 *   attack important squares.                              *
 *                                                          *
 ************************************************************
 */
    if (!(file_mask[file] & Pawns(side))) {
      if (!(file_mask[file] & Pawns(enemy))) {
        score_mg += rook_open_file[mg];
        score_eg += rook_open_file[eg];
      } else {
        score_mg += rook_half_open_file[mg];
        score_eg += rook_half_open_file[eg];
      }
    }
/*
 ************************************************************
 *                                                          *
 *   determine if the rook is on the 7th rank, with the     *
 *   enemy king trapped on the 8th rank.  if so the rook    *
 *   exerts a "cramping" effect that is valuable.           *
 *                                                          *
 ************************************************************
 */
    if (rankflip[side][Rank(square)] == RANK7) {
      score_mg += rook_on_7th[mg];
      score_eg += rook_on_7th[eg];
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
      pawnsq = MSB(Pawns(white) & file_mask[file]);
      if (MSB(AttacksFile(square) & plus8dir[square]) == pawnsq) {
        score_mg += rook_behind_passed_pawn[mg];
        score_eg += rook_behind_passed_pawn[eg];
      }
    }
    if (1 << file & tree->pawn_score.passed[black]) {
      pawnsq = LSB(Pawns(black) & file_mask[file]);
      if (LSB(AttacksFile(square) & minus8dir[square]) == pawnsq) {
        score_mg += rook_behind_passed_pawn[mg];
        score_eg += rook_behind_passed_pawn[eg];
      }
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
    if (Rank(KingSQ(side)) == rankflip[side][RANK1]) {
      if (File(KingSQ(side)) < FILED) {
        if (sqflip[side][square] < sqflip[side][KingSQ(side)]) {
          score_mg -= rook_trapped;
          score_eg -= rook_trapped;
        }
      } else if (File(KingSQ(side)) > FILEE) {
        if (sqflip[side][square] > sqflip[side][KingSQ(side)] &&
            sqflip[side][square] < 8) {
          score_mg -= rook_trapped;
          score_eg -= rook_trapped;
        }
      }
    }
/*
 ************************************************************
 *                                                          *
 *   mobility counts the number of squares the piece        *
 *   attacks, excluding squares with friendly pieces, and   *
 *   weighs each square according to centralization (file). *
 *                                                          *
 ************************************************************
 */
    moves = AttacksRook(square, OccupiedSquares) & ~Occupied(side);
    moves |= SetMask(square);
    for (i = 0; i < 4; i++) {
      tscore += PopCnt(moves & mobility_mask_r[i]) * mobility_score_r[i];
    }
    tscore -= lower_r;
/*
 ************************************************************
 *                                                          *
 *   adjust the tropism count for this piece.               *
 *                                                          *
 ************************************************************
 */
    if (tree->dangerous[side]) {
      trop =
          (AttacksRook(square,
              OccupiedSquares & ~(Queens(side) | (Rooks(side)))) &
          king_attacks[KingSQ(enemy)]) ? 1 : Distance(square, KingSQ(enemy));
      tree->tropism[side] += king_tropism_r[trop];
    }
    temp &= temp - 1;
  }
  score_mg += 154 * tscore / 256;
  score_eg += 154 * tscore / 256;
  tree->score_mg += sign[side] * score_mg;
  tree->score_eg += sign[side] * score_eg;
#ifdef DEBUGEV
  printf("score[rooks(%d) [MG]]=             %4d\n", side, score_mg);
  printf("score[rooks(%d) [EG]]=             %4d\n", side, score_eg);
  printf("tropism[rooks(%d)]=                %4d\n", side, tree->tropism[side]);
#endif
}

/* last modified 10/02/08 */
/*
 *******************************************************************************
 *                                                                             *
 *   EvaluateWinningChances() is used to determine if one side has reached a   *
 *   position which can not be won, period, even though side may be ahead in   *
 *   material in some way.                                                     *
 *                                                                             *
 *   return values:                                                            *
 *        0    ->     side on move can not win.                                *
 *        1    ->     side on move can win.                                    *
 *                                                                             *
 *******************************************************************************
 */
int EvaluateWinningChances(TREE * RESTRICT tree, int side)
{
  register int square, fkd, ekd, pd, promote;
  int enemy = Flip(side);

/*
 ************************************************************
 *                                                          *
 *   if side has a piece and no pawn, it can not possibly   *
 *   win.  if side is a piece ahead, the only way it can    *
 *   win is if the enemy is already trapped on the edge of  *
 *   the board (special case to handle KRB vs KR which can  *
 *   be won if the king gets trapped).                      *
 *                                                          *
 ************************************************************
 */
  if (TotalPieces(side, pawn) == 0) {
    if (TotalPieces(side, occupied) <= 3)
      return (0);
    if (TotalPieces(side, occupied) - TotalPieces(enemy, occupied) <= 3 &&
        mask_not_edge & Kings(enemy))
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
  if (TotalPieces(side, pawn) && !(Pawns(side) & not_rook_pawns))
    do {
      if (TotalPieces(side, occupied) > 3 || (TotalPieces(side, occupied) == 3
              && Knights(side)))
        continue;
      if (TotalPieces(side, occupied) == 0 && file_mask[FILEA] & Pawns(side) &&
          file_mask[FILEH] & Pawns(side))
        continue;
      if (Bishops(side)) {
        if (!TotalPieces(enemy, occupied)) {
          if (Bishops(side) & dark_squares) {
            if (file_mask[dark_corner[side]] & Pawns(side))
              continue;
          } else if (file_mask[light_corner[side]] & Pawns(side))
            continue;
        } else
          continue;
      }
      if (!(Pawns(side) & file_mask[FILEA]) ||
          !(Pawns(side) & file_mask[FILEH])) {
        if (Pawns(side) & file_mask[FILEA])
          promote = A8;
        else
          promote = H8;
        ekd = Distance(KingSQ(enemy), sqflip[side][promote]) - (wtm != side);
        if (ekd <= 1)
          return (0);
        else {
          fkd = Distance(KingSQ(side), sqflip[side][promote]) - (wtm == side);
          pd = Distance(Advanced(side, Pawns(side) & file_mask[File(promote)]),
              sqflip[side][promote]) - (wtm == side);
          if ((ekd - 1) <= fkd && (ekd - 1) <= pd)
            return (0);
        }
      }
    } while (0);
/*
 ************************************************************
 *                                                          *
 *   if both sides have pawns, and we have made it through  *
 *   the previous tests, then this side has winning         *
 *   chances.                                               *
 *                                                          *
 ************************************************************
 */
  if (TotalPieces(side, pawn))
    return (1);
/*
 ************************************************************
 *                                                          *
 *   if side has two bishops, and the enemy has only a      *
 *   single kinght, the two bishops win.                    *
 *                                                          *
 ************************************************************
 */
  if (TotalPieces(side, pawn) == 0 && TotalPieces(side, occupied) == 6 &&
      TotalPieces(enemy, occupied) == 3 && (Knights(side) || !Knights(enemy)))
    return (0);
/*
 ************************************************************
 *                                                          *
 *   if one side is two knights ahead and the opponent has  *
 *   no remaining material, it is a draw.                   *
 *                                                          *
 ************************************************************
 */
  if (TotalPieces(side, pawn) == 0 && TotalPieces(side, occupied) == 6 &&
      !Bishops(side) &&
      TotalPieces(enemy, occupied) + TotalPieces(enemy, pawn) == 0)
    return (0);
/*
 ************************************************************
 *                                                          *
 *   check to see if this is a KRP vs KR or KQP vs KQ type  *
 *   ending.  if so, and the losing king is in front of the *
 *   passer, then this is a drawish ending.                 *
 *                                                          *
 ************************************************************
 */
  if (TotalPieces(side, pawn) == 1 && TotalPieces(enemy, pawn) == 0 &&
      ((TotalPieces(side, occupied) == 5 && TotalPieces(enemy, occupied) == 5)
          || ((TotalPieces(side, occupied) == 9 &&
                  TotalPieces(enemy, occupied) == 9)))) {
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
