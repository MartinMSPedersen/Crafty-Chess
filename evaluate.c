#include <stdio.h>
#include <stdlib.h>
#include "chess.h"
#include "data.h"

/* last modified 05/23/06 */
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

int Evaluate(TREE * RESTRICT tree, int ply, int wtm)
{
  register int score;
  register int can_win = 3;

/*
 **********************************************************************
 *                                                                    *
 *   initialize.                                                      *
 *                                                                    *
 **********************************************************************
 */
  tree->endgame = (TotalWhitePieces <= EG_MAT || TotalBlackPieces <= EG_MAT);
  tree->w_safety = 0;
  tree->b_safety = 0;
  tree->evaluations++;
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
  if (TotalWhitePieces < 13 && TotalBlackPieces < 13) {
    can_win = EvaluateWinningChances(tree);
    if (EvaluateStalemate(tree, wtm))
      can_win = 0;
  }
  score = EvaluateMaterial(tree);
#ifdef DEBUGEV
  printf("score[material]=                  %4d\n", score);
#endif
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
  tree->all_pawns = BlackPawns | WhitePawns;
  if ((TotalWhitePawns + TotalBlackPawns) == 0) {
    if (TotalWhitePieces > 3 || TotalBlackPieces > 3) {
      score += EvaluateMate(tree);
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
  else
    score += EvaluatePawns(tree);
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
  if (tree->pawn_score.passed_b || tree->pawn_score.passed_w ||
      tree->pawn_score.candidates_b || tree->pawn_score.candidates_w) {
    int pscore = EvaluatePassedPawns(tree, wtm);

    if ((TotalWhitePieces == 0 && tree->pawn_score.passed_b) ||
        (TotalBlackPieces == 0 && tree->pawn_score.passed_w))
      pscore += EvaluatePassedPawnRaces(tree, wtm);
    score += pscore;
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
  if (WhiteCastle(1))
    score += EvaluateDevelopmentW(tree, ply);
  if (BlackCastle(1))
    score += EvaluateDevelopmentB(tree, ply);
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
  tree->w_tropism = 0;
  tree->b_tropism = 0;
  score += EvaluateKnights(tree);
  score += EvaluateBishops(tree);
  score += EvaluateRooks(tree);
  score += EvaluateQueens(tree);
  score += EvaluateKings(tree, wtm, ply);
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

/* last modified 05/23/06 */
/*
 *******************************************************************************
 *                                                                             *
 *   EvaluateBishops() is used to evaluate black/white bishops.                *
 *                                                                             *
 *******************************************************************************
 */
int EvaluateBishops(TREE * RESTRICT tree)
{
  register BITBOARD temp;
  register int square, score = 0;

/*
 ************************************************************
 *                                                          *
 *   white bishops                                          *
 *                                                          *
 *   first, locate each bishop and add in its static score  *
 *   from the bishop piece/square table.                    *
 *                                                          *
 ************************************************************
 */
  temp = WhiteBishops;
  while (temp) {
    square = FirstOne(temp);
    score += bval[square];
/*
 ************************************************************
 *                                                          *
 *   now add in a bonus for a bishop blocking a center pawn *
 *   at D6/E6 as that is very cramping.                     *
 *                                                          *
 ************************************************************
 */
    if ((square == D6 || square == E6) && PcOnSq(square + 8) == -pawn)
      score += blocked_center_pawn;
/*
 ************************************************************
 *                                                          *
 *   also add in a bonus for a bishop, if there are pawns   *
 *   on both sides of the board in an endgame, because a    *
 *   bishop is much more valuable there.                    *
 *                                                          *
 ************************************************************
 */
    if (TotalWhitePieces < 7 && !BlackBishops && tree->all_pawns & mask_fgh &&
        tree->all_pawns & mask_abc)
      score += bishop_over_knight_endgame;
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
    if (square == A7 && SetMask(B6) & BlackPawns)
      score -= bishop_trapped;
    else if (square == B8 && SetMask(C7) & BlackPawns)
      score -= bishop_trapped;
    else if (square == H7 && SetMask(G6) & BlackPawns)
      score -= bishop_trapped;
    else if (square == G8 && SetMask(F7) & BlackPawns)
      score -= bishop_trapped;
/*
 ************************************************************
 *                                                          *
 *   check to see if the bishop is bad, which in this very  *
 *   simple case simply means that it is attacking one or   *
 *   two friendly pawns from the rear, one or two squares   *
 *   away.                                                  *
 *                                                          *
 ************************************************************
 */
    if (SetMask(square) & mask_corner_a1)
      score -= PopCnt(plus9dir[square] & WhitePawns) * bishop_bad;
    else if (SetMask(square) & mask_corner_h1)
      score -= PopCnt(plus7dir[square] & WhitePawns) * bishop_bad;
/*
 ************************************************************
 *                                                          *
 *   adjust the tropism count for this piece.               *
 *                                                          *
 ************************************************************
 */
    tree->w_tropism += king_tropism_b[Distance(square, BlackKingSQ)];
    Clear(square, temp);
  }
/*
 ************************************************************
 *                                                          *
 *   give the white side a bonus for having two bishops.    *
 *                                                          *
 ************************************************************
 */
  if (WhiteBishops & (WhiteBishops - 1))
    score += bishop_pair[tree->pawn_score.center];
#ifdef DEBUGEV
  printf("score[bishops(white)]=            %4d\n", score);
  printf("tropism[bishops(white)]=          %4d\n", tree->w_tropism);
#endif
/*
 ************************************************************
 *                                                          *
 *   black bishops                                          *
 *                                                          *
 *   first, locate each bishop and add in its static score  *
 *   from the bishop piece/square table.                    *
 *                                                          *
 ************************************************************
 */
  temp = BlackBishops;
  while (temp) {
    square = FirstOne(temp);
    score -= bval[square];
/*
 ************************************************************
 *                                                          *
 *   now add in a bonus for a bishop blocking a center pawn *
 *   at D3/E3 as that is very cramping.                     *
 *                                                          *
 ************************************************************
 */
    if ((square == D3 || square == E3) && PcOnSq(square - 8) == pawn)
      score -= blocked_center_pawn;
/*
 ************************************************************
 *                                                          *
 *   add in a bonus for a bishop, if there are pawns on     *
 *   both sides of the board in an endgame, because a       *
 *   bishop is much more valuable there.                    *
 *                                                          *
 ************************************************************
 */
    if (TotalBlackPieces < 7 && !WhiteBishops && tree->all_pawns & mask_fgh &&
        tree->all_pawns & mask_abc)
      score -= bishop_over_knight_endgame;
/*
 ************************************************************
 *                                                          *
 *   check to see if the bishop is trapped at a2 or h2 with *
 *   a pawn at b2 or g2 that can advance one square and     *
 *   trap the bishop, or a pawn at b3 or g3 that has        *
 *   trapped the bishop already.  also test for the bishop  *
 *   at b1 or g1 as that might not be an escape.            *
 *                                                          *
 ************************************************************
 */
    if (square == A2 && SetMask(B3) & WhitePawns)
      score += bishop_trapped;
    else if (square == B1 && SetMask(C2) & WhitePawns)
      score += bishop_trapped;
    else if (square == H2 && SetMask(G3) & WhitePawns)
      score += bishop_trapped;
    else if (square == G1 && SetMask(F2) & WhitePawns)
      score += bishop_trapped;
/*
 ************************************************************
 *                                                          *
 *   check to see if the bishop is bad, which in this very  *
 *   simple case simply means that it is attacking one or   *
 *   two friendly pawns from the rear, one or two squares   *
 *   away.                                                  *
 *                                                          *
 ************************************************************
 */
    if (SetMask(square) & mask_corner_a8)
      score += PopCnt(minus7dir[square] & WhitePawns) * bishop_bad;
    else if (SetMask(square) & mask_corner_h8)
      score += PopCnt(minus9dir[square] & WhitePawns) * bishop_bad;
/*
 ************************************************************
 *                                                          *
 *   adjust the tropism count for this piece.               *
 *                                                          *
 ************************************************************
 */
    tree->b_tropism += king_tropism_b[Distance(square, WhiteKingSQ)];
    Clear(square, temp);
  }
/*
 ************************************************************
 *                                                          *
 *   give the black side a bonus for having two bishops.    *
 *                                                          *
 ************************************************************
 */
  if (BlackBishops & (BlackBishops - 1))
    score -= bishop_pair[tree->pawn_score.center];
#ifdef DEBUGEV
  printf("score[bishops(black)]=            %4d\n", score);
  printf("tropism[bishops(black)]=          %4d\n", tree->b_tropism);
#endif
  return (score);
}

/* last modified 05/23/06 */
/*
 *******************************************************************************
 *                                                                             *
 *   EvaluateDevelopmentB() is used to encourage the program to develop its    *
 *   pieces before moving its queen.  standard developmental principles are    *
 *   applied.  they include:  (1) don't move the queen until minor pieces are  *
 *   developed;  (2) advance the center pawns as soon as possible;  (3) don't  *
 *   move the king unless its a castling move.                                 *
 *                                                                             *
 *******************************************************************************
 */
int EvaluateDevelopmentB(TREE * RESTRICT tree, int ply)
{
  register int score = 0;

/*
 ************************************************************
 *                                                          *
 *   first, some "thematic" things, which includes don't    *
 *   block the c-pawn in queen-pawn openings, then also     *
 *   check to see if center pawns are blocked.              *
 *                                                          *
 ************************************************************
 */
  if (!(SetMask(E5) & BlackPawns) && SetMask(D5) & BlackPawns) {
    if (SetMask(C7) & BlackPawns && SetMask(C6) & (BlackKnights | BlackBishops))
      score += development_thematic;
  }
  if (Occupied & ((BlackPawns & rank_mask[RANK7]) << 8) & (file_mask[FILED]
          | file_mask[FILEE]))
    score += blocked_center_pawn;
#ifdef DEBUGDV
  printf("developmentB.1 score=%d\n", score);
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
  if (BlackCastle(1) > 0) {
    if (BlackCastle(ply) != BlackCastle(1)) {
      register int wq;

      wq = (WhiteQueens) ? 3 : 1;
      if (BlackCastle(ply) == 0)
        score += wq * development_losing_castle;
      else if (BlackCastle(ply) > 0)
        score += (wq * development_losing_castle) / 2;
    } else
      score += development_not_castled;
  }
#ifdef DEBUGDV
  printf("developmentB.3 score=%d\n", score);
#endif
  return (score);
}

/* last modified 05/23/06 */
/*
 *******************************************************************************
 *                                                                             *
 *   EvaluateDevelopmentW() is used to encourage the program to develop its    *
 *   pieces before moving its queen.  standard developmental principles are    *
 *   applied.  they include:  (1) don't move the queen until minor pieces are  *
 *   developed;  (2) advance the center pawns as soon as possible;  (3) don't  *
 *   move the king unless its a castling move.                                 *
 *                                                                             *
 *******************************************************************************
 */
int EvaluateDevelopmentW(TREE * RESTRICT tree, int ply)
{
  register int score = 0;

/*
 ************************************************************
 *                                                          *
 *   first, some "thematic" things, which includes don't    *
 *   block the c-pawn in queen-pawn openings, then also     *
 *   check to see if center pawns are blocked.              *
 *                                                          *
 ************************************************************
 */
  if (!(SetMask(E4) & WhitePawns) && SetMask(D4) & WhitePawns) {
    if (SetMask(C2) & WhitePawns && SetMask(C3) & (WhiteKnights | WhiteBishops))
      score -= development_thematic;
  }
  if (Occupied & ((WhitePawns & rank_mask[RANK2]) >> 8) & (file_mask[FILED]
          | file_mask[FILEE]))
    score -= blocked_center_pawn;
#ifdef DEBUGDV
  printf("developmentW.1 score=%d\n", score);
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
  if (WhiteCastle(1) > 0) {
    if (WhiteCastle(ply) != WhiteCastle(1)) {
      register int bq;

      bq = (BlackQueens) ? 3 : 1;
      if (WhiteCastle(ply) == 0)
        score -= bq * development_losing_castle;
      else if (WhiteCastle(ply) > 0)
        score -= (bq * development_losing_castle) / 2;
    } else
      score -= development_not_castled;
  }
#ifdef DEBUGDV
  printf("developmentW.3 score=%d\n", score);
#endif
  return (score);
}

/* last modified 05/23/06 */
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
  if (TotalWhitePieces <= 8 && TotalBlackPieces <= 8) {
    if (PopCnt(WhiteBishops) == 1 && PopCnt(BlackBishops) == 1) {
      if (square_color[FirstOne(BlackBishops)] !=
          square_color[FirstOne(WhiteBishops)]) {
        if (TotalWhitePieces == 3 && TotalBlackPieces == 3 &&
            ((TotalWhitePawns < 4 && TotalBlackPawns < 4) ||
                abs(TotalWhitePawns - TotalBlackPawns) < 2))
          score = score / 2;
        else if (TotalWhitePieces == TotalBlackPieces)
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

/* last modified 05/23/06 */
/*
 *******************************************************************************
 *                                                                             *
 *   EvaluateKings() is used to evaluate black/white kings.                    *
 *                                                                             *
 *******************************************************************************
 */
int EvaluateKings(TREE * RESTRICT tree, int wtm, int ply)
{
  register int score = 0;

/*
 ************************************************************
 *                                                          *
 *   white king.                                            *
 *                                                          *
 *   first, check for where the king should be if this is   *
 *   an endgame.  ie with pawns on one wing, the king needs *
 *   to be on that wing.  with pawns on both wings, the     *
 *   king belongs in the center.                            *
 *                                                          *
 ************************************************************
 */
  if (tree->endgame) {
    if (tree->all_pawns & mask_efgh && tree->all_pawns & mask_abcd)
      score += kval_wn[WhiteKingSQ];
    else if (tree->all_pawns & mask_efgh)
      score += kval_wk[WhiteKingSQ];
    else if (tree->all_pawns)
      score += kval_wq[WhiteKingSQ];
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
    if (WhiteKingSQ < A2) {
      if (WhiteKingSQ > E1) {
        if (WhiteRooks & mask_kr_trapped_w[FILEH - WhiteKingSQ])
          score -= rook_trapped;
      } else if (WhiteKingSQ < D1) {
        if (WhiteRooks & mask_qr_trapped_w[WhiteKingSQ])
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
    if (shared->trojan_check) {
      if (shared->root_wtm && File(WhiteKingSQ) >= FILEE) {
        if (!(tree->all_pawns & file_mask[FILEH])) {
          if (BlackRooks && BlackQueens)
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
    tree->w_safety = 0;
    if (WhiteCastle(ply) <= 0) {
      if (File(WhiteKingSQ) >= FILEE) {
        if (File(WhiteKingSQ) > FILEE)
          tree->w_safety += tree->pawn_score.white_defects_k;
        else
          tree->w_safety += tree->pawn_score.white_defects_e;
      } else if (File(WhiteKingSQ) <= FILED) {
        if (File(WhiteKingSQ) < FILED)
          tree->w_safety += tree->pawn_score.white_defects_q;
        else
          tree->w_safety += tree->pawn_score.white_defects_d;
      }
    } else {
      if (WhiteCastle(ply) == 3)
        tree->w_safety =
            Min(Min(tree->pawn_score.white_defects_k,
                tree->pawn_score.white_defects_e),
            tree->pawn_score.white_defects_q);
      else if (WhiteCastle(ply) == 1)
        tree->w_safety =
            Min(tree->pawn_score.white_defects_k,
            tree->pawn_score.white_defects_e);
      else
        tree->w_safety =
            Min(tree->pawn_score.white_defects_q,
            tree->pawn_score.white_defects_e);
      tree->w_safety = Max(tree->w_safety, 3);
    }
    if (!BlackQueens) {
      tree->w_safety /= 2;
      tree->b_tropism /= 2;
    }
/*
 ************************************************************
 *                                                          *
 *   now fold in the king tropism and king pawn shelter     *
 *   scores together.                                       *
 *                                                          *
 ************************************************************
 */
if (tree->w_safety > 15) {
  printf("ERROR, w_safety = %d\n", tree->w_safety);
  exit(1);
}
    score -= king_safety[Min(tree->w_safety, 15)][Min(tree->b_tropism, 15)];
  }
#ifdef DEBUGEV
  printf("score[safety(white)]=             %4d\n", tree->w_safety);
  printf("score[tropism(black)]=            %4d\n", tree->b_tropism);
  printf("score[kings(white)]=              %4d\n", score);
#endif
/*
 ************************************************************
 *                                                          *
 *   black king.                                            *
 *                                                          *
 *   first, check for where the king should be if this is   *
 *   an endgame.  ie with pawns on one wing, the king needs *
 *   to be on that wing.  with pawns on both wings, the     *
 *   king belongs in the center.                            *
 *                                                          *
 ************************************************************
 */
  if (tree->endgame) {
    if (tree->all_pawns & mask_efgh && tree->all_pawns & mask_abcd)
      score -= kval_bn[BlackKingSQ];
    else if (tree->all_pawns & mask_efgh)
      score -= kval_bk[BlackKingSQ];
    else if (tree->all_pawns)
      score -= kval_bq[BlackKingSQ];
  }
/*
 ************************************************************
 *                                                          *
 *   check to see if the king has been forced to move and   *
 *   has trapped a rook at a8/a7/b8/g7/h8/h7, if so, then   *
 *   penalize the trapped rook to help extricate it.        *
 *                                                          *
 ************************************************************
 */
  else {
    if (BlackKingSQ > H7) {
      if (BlackKingSQ > E8) {
        if (BlackRooks & mask_kr_trapped_b[FILEH - File(BlackKingSQ)])
          score += rook_trapped;
      } else if (BlackKingSQ < D8) {
        if (BlackRooks & mask_qr_trapped_b[File(BlackKingSQ)])
          score += rook_trapped;
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
    if (shared->trojan_check) {
      if (Flip(shared->root_wtm) && File(BlackKingSQ) >= FILEE) {
        if (!(tree->all_pawns & file_mask[FILEH])) {
          if (WhiteRooks && WhiteQueens)
            score += king_safety_mate_threat;
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
    tree->b_safety = 0;
    if (BlackCastle(ply) <= 0) {
      if (File(BlackKingSQ) >= FILEE) {
        if (File(BlackKingSQ) > FILEE)
          tree->b_safety += tree->pawn_score.black_defects_k;
        else
          tree->b_safety += tree->pawn_score.black_defects_e;
      } else if (File(BlackKingSQ) <= FILED) {
        if (File(BlackKingSQ) < FILED)
          tree->b_safety += tree->pawn_score.black_defects_q;
        else
          tree->b_safety += tree->pawn_score.black_defects_d;
      }
    } else {
      if (BlackCastle(ply) == 3)
        tree->b_safety =
            Min(Min(tree->pawn_score.black_defects_k,
                tree->pawn_score.black_defects_e),
            tree->pawn_score.black_defects_q);
      else if (BlackCastle(ply) == 1)
        tree->b_safety =
            Min(tree->pawn_score.black_defects_k,
            tree->pawn_score.black_defects_e);
      else
        tree->b_safety =
            Min(tree->pawn_score.black_defects_q,
            tree->pawn_score.black_defects_e);
      tree->b_safety = Max(tree->b_safety, 3);
    }
    if (!WhiteQueens) {
      tree->b_safety /= 2;
      tree->w_tropism /= 2;
    }
/*
 ************************************************************
 *                                                          *
 *   now fold in the king tropism and king pawn shelter     *
 *   scores together.                                       *
 *                                                          *
 ************************************************************
 */
if (tree->b_safety > 15) {
  printf("ERROR, b_safety = %d\n", tree->b_safety);
  exit(1);
}
    score += king_safety[tree->b_safety][Min(tree->w_tropism, 15)];
  }
#ifdef DEBUGEV
  printf("score[safety(black)]=             %4d\n", tree->b_safety);
  printf("score[tropism(white)]=            %4d\n", tree->w_tropism);
  printf("score[kings(black)]=              %4d\n", score);
#endif
/*
 ************************************************************
 *                                                          *
 *   both kings.                                            *
 *                                                          *
 *   if we are in an endgame, and there are no pieces on    *
 *   the board, just pawns and kings, then check to see if  *
 *   we are in a won KP ending.  this kind of position is   *
 *   one that occurs when one side has an outside passed    *
 *   pawn, and pushes it until the opposing king has to run *
 *   over and capture it, leaving itself out of play and    *
 *   too far from the pawns on the other side of the board, *
 *   losing the game.                                       *
 *                                                          *
 *   the basic idea is that if one king is closer to a weak *
 *   opponent pawn than the other king, then that side will *
 *   win pawns and probably the game.                       *
 *                                                          *
 ************************************************************
 */
  if (tree->endgame && TotalWhitePieces + TotalBlackPieces == 0 &&
      (!(tree->all_pawns & mask_abcd) || !(tree->all_pawns & mask_efgh))) {
    BITBOARD pawns;
    int wdist = 99, bdist = 99, square;

    pawns = WhitePawns;
    while (pawns) {
      square = LastOne(pawns);
      pawns = pawns & (pawns - 1);
      if (b_pawn_attacks[square] & WhitePawns)
        continue;
      if (bdist > Distance(BlackKingSQ, square))
        bdist = Distance(BlackKingSQ, square);
    }
    bdist -= Flip(wtm);
    pawns = BlackPawns;
    while (pawns) {
      square = LastOne(pawns);
      pawns = pawns & (pawns - 1);
      if (w_pawn_attacks[square] & BlackPawns)
        continue;
      if (wdist > Distance(WhiteKingSQ, square))
        wdist = Distance(WhiteKingSQ, square);
    }
    wdist -= wtm;
    if (!(tree->pawn_score.protected & 2)) {
      if (wdist < bdist)
        score += won_kp_ending;
    }
    if (!(tree->pawn_score.protected & 1)) {
      if (bdist < wdist)
        score -= won_kp_ending;
    }
#ifdef DEBUGEV
    printf("wdist=%d  bdist=%d\n", wdist, bdist);
#endif
  }
#ifdef DEBUGEV
  printf("score[kings(all)]=                %4d\n", score);
#endif
  return (score);
}

/* last modified 05/23/06 */
/*
 *******************************************************************************
 *                                                                             *
 *   EvaluateKingsFile[WB] computes defects for a file, based on whether  *
 *   the file is open or half-open.  if there are friendly pawns still on the  *
 *   file, they are penalized for advancing in front of the king.              *
 *                                                                             *
 *******************************************************************************
 */
int EvaluateKingsFileB(TREE * RESTRICT tree, int whichfile)
{
  int defects = 0, file;

  for (file = whichfile - 1; file <= whichfile + 1; file++) {
    if (!(file_mask[file] & tree->all_pawns))
      defects += open_file;
    else {
      if (!(file_mask[file] & WhitePawns))
        defects += half_open_file / 2;
      else
        defects +=
            Max(0, (Rank(LastOne(file_mask[file] & WhitePawns)) - 2)) & 3;
      if (!(file_mask[file] & BlackPawns))
        defects += half_open_file;
      else {
        if (!(BlackPawns & SetMask(A7 + file))) {
          defects++;
          if (!(BlackPawns & SetMask(A6 + file)))
            defects++;
        }
      }
    }
  }
  return (Min(defects, 15));
}

int EvaluateKingsFileW(TREE * RESTRICT tree, int whichfile)
{
  int defects = 0, file;

  for (file = whichfile - 1; file <= whichfile + 1; file++) {
    if (!(file_mask[file] & tree->all_pawns))
      defects += open_file;
    else {
      if (!(file_mask[file] & BlackPawns))
        defects += half_open_file / 2;
      else
        defects +=
            Max(0, (5 - Rank(FirstOne(file_mask[file] & BlackPawns)))) & 3;
      if (!(file_mask[file] & WhitePawns))
        defects += half_open_file;
      else {
        if (!(WhitePawns & SetMask(A2 + file))) {
          defects++;
          if (!(WhitePawns & SetMask(A3 + file)))
            defects++;
        }
      }
    }
  }
  return (Min(defects, 15));
}

/* last modified 05/23/06 */
/*
 *******************************************************************************
 *                                                                             *
 *   EvaluateKnights() is used to evaluate black/white knights.                *
 *                                                                             *
 *******************************************************************************
 */
int EvaluateKnights(TREE * RESTRICT tree)
{
  register BITBOARD temp, temp2;
  register int square, score = 0;

/*
 ************************************************************
 *                                                          *
 *   white knights.                                         *
 *                                                          *
 *   first fold in centralization score from the piece/     *
 *   square table "nval".                                   *
 *                                                          *
 ************************************************************
 */
  temp = WhiteKnights;
  while (temp) {
    square = FirstOne(temp);
    score += nval[square];
/*
 ************************************************************
 *                                                          *
 *   now, evaluate for "outposts" which is a knight that    *
 *   can't be driven off by an enemy pawn, and which is     *
 *   supported by a friendly pawn.                          *
 *                                                          *
 ************************************************************
 */
    if (white_outpost[square] && !(mask_no_pattacks_b[square] & BlackPawns)
        && b_pawn_attacks[square] & WhitePawns)
      score += white_outpost[square];
/*
 ************************************************************
 *                                                          *
 *   if the knight is blocking an unmoved center pawn, then *
 *   this really cramps the opponent and should be given a  *
 *   reward.                                                *
 *                                                          *
 ************************************************************
 */
    if ((square == D6 || square == E6) && PcOnSq(square + 8) == -pawn)
      score += blocked_center_pawn;
/*
 ************************************************************
 *                                                          *
 *   adjust the tropism count for this piece.               *
 *                                                          *
 ************************************************************
 */
    tree->w_tropism += king_tropism_n[Distance(square, BlackKingSQ)];
    Clear(square, temp);
  }
#ifdef DEBUGEV
  printf("score[knights(white)]=            %4d\n", score);
  printf("tropism[knights(white)]=          %4d\n", tree->w_tropism);
#endif
/*
 ************************************************************
 *                                                          *
 *   black knights.                                         *
 *                                                          *
 *   first fold in centralization score from the piece/     *
 *   square table "nval".                                   *
 *                                                          *
 ************************************************************
 */
  temp = BlackKnights;
  while (temp) {
    square = FirstOne(temp);
    score -= nval[square];
/*
 ************************************************************
 *                                                          *
 *   now, evaluate for "outposts" which is a knight that    *
 *   can't be driven off by an enemy pawn, and which is     *
 *   supported by a friendly pawn.                          *
 *                                                          *
 ************************************************************
 */
    if (black_outpost[square] && !(mask_no_pattacks_w[square] & WhitePawns)
        && w_pawn_attacks[square] & BlackPawns)
      score -= black_outpost[square];
/*
 ************************************************************
 *                                                          *
 *   if the knight is blocking an unmoved center pawn, then *
 *   this really cramps the opponent and should be given a  *
 *   reward.                                                *
 *                                                          *
 ************************************************************
 */
    if ((square == D3 || square == E3) && PcOnSq(square - 8) == pawn)
      score -= blocked_center_pawn;
/*
 ************************************************************
 *                                                          *
 *   adjust the tropism count for this piece.               *
 *                                                          *
 ************************************************************
 */
    tree->b_tropism += king_tropism_n[Distance(square, WhiteKingSQ)];
    Clear(square, temp);
  }
#ifdef DEBUGEV
  printf("score[knights(black)]=            %4d\n", score);
  printf("tropism[knights(black)]=          %4d\n", tree->b_tropism);
#endif
  return (score);
}

/* last modified 09/16/05 */
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
int EvaluateMate(TREE * RESTRICT tree)
{
  register int mate_score = DrawScore(1);

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
  if ((TotalBlackPieces == 0) && (TotalWhitePieces == 6) && (!WhitePawns) &&
      (!BlackPawns) && WhiteBishops && WhiteKnights) {
    if (dark_squares & WhiteBishops)
      mate_score = b_n_mate_dark_squares[BlackKingSQ];
    else
      mate_score = b_n_mate_light_squares[BlackKingSQ];
  }
  if ((TotalBlackPieces == 6) && (TotalWhitePieces == 0) && (!WhitePawns) &&
      (!BlackPawns) && BlackBishops && BlackKnights) {
    if (dark_squares & BlackBishops)
      mate_score = -b_n_mate_dark_squares[WhiteKingSQ];
    else
      mate_score = -b_n_mate_light_squares[WhiteKingSQ];
  }
/*
 ************************************************************
 *                                                          *
 *   if white is winning, force the black king to the edge  *
 *   of the board.                                          *
 *                                                          *
 ************************************************************
 */
  if (!mate_score) {
    if (Material > 0) {
      mate_score = mate[BlackKingSQ];
      mate_score -=
          (Distance(WhiteKingSQ, BlackKingSQ) - 3) * king_king_tropism;
    }
/*
 ************************************************************
 *                                                          *
 *   if black is winning, force the white king to the edge  *
 *   of the board.                                          *
 *                                                          *
 ************************************************************
 */
    else if (Material < 0) {
      mate_score = -mate[WhiteKingSQ];
      mate_score +=
          (Distance(WhiteKingSQ, BlackKingSQ) - 3) * king_king_tropism;
    }
  }
  return (mate_score);
}

/* last modified 05/23/06 */
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
  register int score;

/*
 **********************************************************************
 *                                                                    *
 *   we start with the raw Material balance for the current position. *
 *                                                                    *
 **********************************************************************
 */
  score = Material;
/*
 **********************************************************************
 *                                                                    *
 *   test 1.  if Majors or Minors are not balanced, then if one side  *
 *   has more piece material points than the other (using normal      *
 *   piece values of 3, 3, 5, 9 for N, B, R and Q) then the side that *
 *   is behind in piece material gets a penalty.                      *
 *                                                                    *
 *   test 2.  if Majors and Minors are balanced, then if one side has *
 *   a queen but the other side does not, the queen gets a bonus.     *
 *                                                                    *
 **********************************************************************
 */
  if (Majors != 0 || Minors != 0) {
    if (abs(Majors) != 1 || Majors != -Minors) {
      if (TotalWhitePieces > TotalBlackPieces)
        score += bad_trade;
      else if (TotalBlackPieces > TotalWhitePieces)
        score -= bad_trade;
    }
  }
  if (WhiteQueens && !BlackQueens)
    score += queen_vs_2_rooks;
  else if (!WhiteQueens && BlackQueens)
    score -= queen_vs_2_rooks;
#ifdef DEBUGM
  printf("Majors=%d  Minors=%d  TotalWhitePieces=%d  TotalBlackPieces=%d\n",
      Majors, Minors, TotalWhitePieces, TotalBlackPieces);
  printf("score[bad trade]=                 %4d\n", score);
  printf("score[material]=                  %4d\n", Material);
#endif
  return (score);
}

/* last modified 05/23/06 */
/*
 *******************************************************************************
 *                                                                             *
 *   EvaluatePassedPawns() is used to evaluate passed pawns and the danger     *
 *   they produce.  EvaluatePawns() has thoughtfully provided us with the      *
 *   following bit flags to give us a starting point for evaluation.           *
 *                                                                             *
 *   pawn_score.outside:                                                       *
 *                                                                             *
 *        xxxx xxx1   (1) -> white has outside candidate or passer             *
 *        xxxx xx1x   (2) -> white has 2 outside candidate or passer           *
 *        xxxx x1xx   (4) -> black has outside candidate or passer             *
 *        xxxx 1xxx   (8) -> black has 2 outside candidate or passer           *
 *                                                                             *
 *   pawn_score.protected:                                                     *
 *                                                                             *
 *        xxxx xxx1   (1) -> white has protected passed pawn                   *
 *        xxxx xx1x   (2) -> black has protected passed pawn                   *
 *                                                                             *
 *******************************************************************************
 */
int EvaluatePassedPawns(TREE * RESTRICT tree, int wtm)
{
  register int file, square, score = 0;
  register int white_king_sq, black_king_sq;
  register int pawns;

/*
 ************************************************************
 *                                                          *
 *   check to see if black has any passed pawns.  if so,    *
 *   and the king supports the pawn, then the pawn is even  *
 *   more valuable.  at the same time, check to see if it   *
 *   is blockaded by an enemy piece.  if so then the pawn   *
 *   is less valuable since it can't advance easily.  as    *
 *   material is removed, passed pawns also become more     *
 *   valuable.                                              *
 *                                                          *
 ************************************************************
 */
  if (tree->pawn_score.passed_b) {
    black_king_sq = BlackKingSQ;
    pawns = tree->pawn_score.passed_b;
    while (pawns) {
      file = FirstOne8Bit(pawns);
      pawns &= ~(128 >> file);
      square = FirstOne(BlackPawns & file_mask[file]);
      if (tree->endgame && FileDistance(square, black_king_sq) == 1 &&
          Rank(black_king_sq) <= Rank(square))
        score -= supported_passer[RANK8 - Rank(square)];
      if (Rank(square) == RANK2 && AttacksRank(square) & BlackRooks)
        score -= supported_passer[RANK7];
      if (PcOnSq(square - 8) > 0)
        score += blockading_passed_pawn_value[RANK8 - Rank(square)];
    }
#ifdef DEBUGPP
    printf("score.1 after black passers = %d\n", score);
#endif
/*
 ************************************************************
 *                                                          *
 *   check to see if black has any connected passed pawns.  *
 *   if so, and they have both reached the 6th/7th rank,    *
 *   then they are very dangerous.                          *
 *                                                          *
 ************************************************************
 */
    pawns = tree->pawn_score.passed_b;
    while ((file = connected_passed[pawns])) {
      register int square1, square2;

      pawns &= ~(128 >> file);
      square1 = FirstOne(BlackPawns & file_mask[file - 1]);
      square2 = FirstOne(BlackPawns & file_mask[file]);
      score -=
          connected_passed_pawn_value[RANK8 - Max(Rank(square1),
              Rank(square2))];
    }
  }
#ifdef DEBUGPP
  printf("score.2 after black passers = %d\n", score);
#endif
/*
 ************************************************************
 *                                                          *
 *   check to see if white has any passed pawns.  if so,    *
 *   and the king supports the pawn, then the pawn is even  *
 *   more valuable.  at the same time, check to see if it   *
 *   is blockaded by an enemy piece.  if so then the pawn   *
 *   is less valuable since it can't advance easily.  as    *
 *   material is removed, passed pawns also become more     *
 *   valuable.                                              *
 *                                                          *
 ************************************************************
 */
  if (tree->pawn_score.passed_w) {
    white_king_sq = WhiteKingSQ;
    pawns = tree->pawn_score.passed_w;
    while (pawns) {
      file = FirstOne8Bit(pawns);
      pawns &= ~(128 >> file);
      square = LastOne(WhitePawns & file_mask[file]);
      if (tree->endgame && FileDistance(square, white_king_sq) == 1 &&
          Rank(white_king_sq) >= Rank(square))
        score += supported_passer[Rank(square)];
      if (Rank(square) == RANK7 && AttacksRank(square) & WhiteRooks)
        score += supported_passer[RANK7];
      if (PcOnSq(square + 8) < 0)
        score -= blockading_passed_pawn_value[Rank(square)];
    }
#ifdef DEBUGPP
    printf("score.1 after white passers = %d\n", score);
#endif
/*
 ************************************************************
 *                                                          *
 *   check to see if white has any connected passed pawns.  *
 *   if so, and they have both reached the 6th/7th rank,    *
 *   then they are very dangerous.                          *
 *                                                          *
 ************************************************************
 */
    pawns = tree->pawn_score.passed_w;
    while ((file = connected_passed[pawns])) {
      register int square1, square2;

      pawns &= ~(128 >> file);
      square1 = LastOne(WhitePawns & file_mask[file - 1]);
      square2 = LastOne(WhitePawns & file_mask[file]);
      score += connected_passed_pawn_value[Min(Rank(square1), Rank(square2))];
    }
  }
#ifdef DEBUGPP
  printf("score.2 after white passers = %d\n", score);
#endif
/*
 ************************************************************
 *                                                          *
 *   check to see if this is a KRP vs KR type ending.  if   *
 *   so, and the losing king is in front of the passer,     *
 *   then this is a drawn ending.                           *
 *                                                          *
 ************************************************************
 */
  if (TotalBlackPawns == 1 && TotalWhitePawns == 0 && TotalBlackPieces == 5 &&
      TotalWhitePieces == 5) {
    square = FirstOne(BlackPawns);
    if (FileDistance(WhiteKingSQ, square) <= 1 &&
        Rank(WhiteKingSQ) < Rank(square))
      return (0);
    if (Rank(BlackKingSQ) > Rank(square) ||
        FileDistance(BlackKingSQ, square) > 1)
      return (0);
  }
  if (TotalWhitePawns == 1 && TotalBlackPawns == 0 && TotalWhitePieces == 5 &&
      TotalBlackPieces == 5) {
    square = FirstOne(WhitePawns);
    if (FileDistance(BlackKingSQ, square) <= 1 &&
        Rank(BlackKingSQ) > Rank(square))
      return (0);
    if (Rank(WhiteKingSQ) < Rank(square) ||
        FileDistance(WhiteKingSQ, square) > 1)
      return (0);
  }
  if (tree->pawn_score.outside) {
    int pscore = 0;

    if (tree->pawn_score.outside & 2)
      pscore += 2 * outside_passed[(unsigned) TotalBlackPieces];
    else if ((tree->pawn_score.outside & 1) &&
        (!(tree->pawn_score.protected & 2)
            || TotalWhitePieces))
      pscore += outside_passed[(unsigned) TotalBlackPieces];
    if (tree->pawn_score.outside & 8)
      pscore -= 2 * outside_passed[(unsigned) TotalWhitePieces];
    else if ((tree->pawn_score.outside & 4) &&
        (!(tree->pawn_score.protected & 1) || TotalBlackPieces))
      pscore -= outside_passed[(unsigned) TotalWhitePieces];
    score += pscore;
  }
#ifdef DEBUGPP
  printf("score.4 after outside passer test = %d\n", score);
#endif
  if (TotalWhitePieces == 0 || TotalBlackPieces == 0) {
    int pscore = 0;
    int w_spread, b_spread;

    if (TotalBlackPieces == 0) {
      w_spread =
          file_spread[tree->pawn_score.passed_w | tree->pawn_score.
          candidates_w];
      if (w_spread > 1)
        pscore += (w_spread - 1) * split_passed;
    }
    if (TotalWhitePieces == 0) {
      b_spread =
          file_spread[tree->pawn_score.passed_b | tree->pawn_score.
          candidates_b];
      if (b_spread > 1)
        pscore -= (b_spread - 1) * split_passed;
    }
    score += pscore;
  }
#ifdef DEBUGPP
  printf("score[passed pawns]=              %4d\n", score);
  printf("score[passed pawns] (flags) =     %4x\n", tree->pawn_score.outside);
#endif
  return (score);
}

/* last modified 09/16/05 */
/*
 *******************************************************************************
 *                                                                             *
 *   EvaluatePassedPawnRaces() is used to evaluate passed pawns when one       *
 *   side has passed pawns and the other side (or neither) has pieces.  in     *
 *   such a case, the critical question is can the defending king stop the pawn*
 *   from queening or is it too far away?  if only one side has pawns that can *
 *   "run" then the situation is simple.  when both sides have pawns that can  *
 *   "run" it becomes more complex as it then becomes necessary to see if      *
 *   pawn queens with check, or if either pawn queens and simultaneously       *
 *   attacks the opposing side's queening square.  for the special case of a   *
 *   single pawn, the simple evaluation rules are used:  king two squares in   *
 *   front of the pawn=win, one square in front with opposition=win,  king on  *
 *   6th pawn close by is a win.  rook pawns are handled separately and are    *
 *   harder to queen.                                                          *
 *                                                                             *
 *******************************************************************************
 */
int EvaluatePassedPawnRaces(TREE * RESTRICT tree, int wtm)
{
  register int file, square;
  register int white_queener = 8, white_square = 0;
  register int black_queener = 8, black_square = 0;
  register int white_pawn = 0, black_pawn = 0, queen_distance;
  register int pawnsq;
  register BITBOARD tempw, tempb;
  register int passed, realw, realb;

/*
 ************************************************************
 *                                                          *
 *   check to see if white has one pawn and neither side    *
 *   has any pieces.  if so, use the simple pawn evaluation *
 *   logic.                                                 *
 *                                                          *
 ************************************************************
 */
  if (WhitePawns && !BlackPawns && TotalWhitePieces == 0 &&
      TotalBlackPieces == 0)
    do {
      pawnsq = LastOne(WhitePawns);
/*
 **************************************************
 *                                                *
 *   king must be in front of the pawn or we      *
 *   go no further.                               *
 *                                                *
 **************************************************
 */
      if (Rank(WhiteKingSQ) <= Rank(pawnsq))
        break;
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
        if ((File(WhiteKingSQ) == FILEB) &&
            (Distance(WhiteKingSQ, A8) < Distance(BlackKingSQ, A8)))
          return (pawn_can_promote);
        break;
      } else if (File(pawnsq) == FILEH) {
        if ((File(WhiteKingSQ) == FILEG) &&
            (Distance(WhiteKingSQ, H8) < Distance(BlackKingSQ, H8)))
          return (pawn_can_promote);
        break;
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
      if (Distance(WhiteKingSQ, pawnsq) < Distance(BlackKingSQ, pawnsq)) {
        if (Rank(WhiteKingSQ) > Rank(pawnsq) + 1)
          return (pawn_can_promote);
        if (Rank(WhiteKingSQ) == RANK6)
          return (pawn_can_promote);
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
      if ((Rank(WhiteKingSQ) == Rank(pawnsq) + 1) &&
          HasOpposition(wtm, WhiteKingSQ, BlackKingSQ))
        return (pawn_can_promote);
    }
    while (0);
/*
 ************************************************************
 *                                                          *
 *   check to see if black has one pawn and neither side    *
 *   has any pieces.  if so, use the simple pawn evaluation *
 *   logic.                                                 *
 *                                                          *
 ************************************************************
 */
  if (BlackPawns && !WhitePawns && TotalWhitePieces == 0 &&
      TotalBlackPieces == 0)
    do {
      pawnsq = FirstOne(BlackPawns);
/*
 **************************************************
 *                                                *
 *   king must be in front of the pawn or we      *
 *   go no further.                               *
 *                                                *
 **************************************************
 */
      if (Rank(BlackKingSQ) >= Rank(pawnsq))
        break;
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
        if ((File(BlackKingSQ) == FILEB) &&
            (Distance(BlackKingSQ, A1) < Distance(WhiteKingSQ, A1)))
          return (-(pawn_can_promote));
        break;
      } else if (File(pawnsq) == FILEH) {
        if ((File(BlackKingSQ) == FILEG) &&
            (Distance(BlackKingSQ, H1) < Distance(WhiteKingSQ, H1)))
          return (-(pawn_can_promote));
        break;
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
      if (Distance(BlackKingSQ, pawnsq) < Distance(WhiteKingSQ, pawnsq)) {
        if (Rank(BlackKingSQ) < Rank(pawnsq) - 1)
          return (-(pawn_can_promote));
        if (Rank(BlackKingSQ) == RANK3)
          return (-(pawn_can_promote));
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
      if ((Rank(BlackKingSQ) == Rank(pawnsq) - 1) &&
          HasOpposition(Flip(wtm), BlackKingSQ, WhiteKingSQ))
        return (-(pawn_can_promote));
    }
    while (0);
/*
 ************************************************************
 *                                                          *
 *   check to see if white is out of pieces and black has   *
 *   passed pawns.  if so, see if any of these passed pawns *
 *   can outrun the defending king and promote.             *
 *                                                          *
 ************************************************************
 */
  realb = 0;
  if (TotalWhitePieces == 0 && tree->pawn_score.passed_b) {
    passed = tree->pawn_score.passed_b;
    while ((file = FirstOne8Bit(passed)) != 8) {
      passed &= ~(128 >> file);
      square = FirstOne(BlackPawns & file_mask[file]);
      if ((wtm && !(black_pawn_race_wtm[square] & WhiteKing))
          || (Flip(wtm) && !(black_pawn_race_btm[square] & WhiteKing))) {
        queen_distance = Rank(square);
        if (BlackKing & minus8dir[square]) {
          if (file == FILEA || file == FILEH)
            queen_distance = 99;
          queen_distance++;
        }
        if (Rank(square) == RANK7)
          queen_distance--;
        if (queen_distance < black_queener) {
          black_queener = queen_distance;
          black_square = file;
          black_pawn = square;
          realb = 1;
        }
      }
    }
    if (PopCnt8Bit(tree->pawn_score.passed_b) > 1) {
      int left =
          FirstOne(file_mask[FirstOne8Bit(tree->pawn_score.
                  passed_b)] & BlackPawns);
      int right =
          FirstOne(file_mask[LastOne8Bit(tree->pawn_score.
                  passed_b)] & BlackPawns);
      if (File(right) - File(left) > 1) {
        if (!(black_pawn_race_btm[left] & SetMask(right))) {
          queen_distance = Rank(left);
          if (Rank(left) == RANK7)
            queen_distance--;
          if (queen_distance < black_queener) {
            black_queener = queen_distance;
            black_square = File(left);
            black_pawn = left;
          }
        }
        if (!(black_pawn_race_btm[right] & SetMask(left))) {
          queen_distance = Rank(right);
          if (Rank(right) == RANK7)
            queen_distance--;
          if (queen_distance < black_queener) {
            black_queener = queen_distance;
            black_square = File(right);
            black_pawn = right;
          }
        }
      }
    }
  }
#ifdef DEBUGPP
  printf("black pawn on %d can promote at %d in %d moves.\n", black_pawn,
      black_square, black_queener);
#endif
/*
 ************************************************************
 *                                                          *
 *   check to see if black is out of pieces and white has   *
 *   passed pawns.  if so, see if any of these passed pawns *
 *   can outrun the defending king and promote.             *
 *                                                          *
 ************************************************************
 */
  realw = 0;
  if (TotalBlackPieces == 0 && tree->pawn_score.passed_w) {
    passed = tree->pawn_score.passed_w;
    while ((file = FirstOne8Bit(passed)) != 8) {
      passed &= ~(128 >> file);
      square = LastOne(WhitePawns & file_mask[file]);
      if ((wtm && !(white_pawn_race_wtm[square] & BlackKing))
          || (Flip(wtm) && !(white_pawn_race_btm[square] & BlackKing))) {
        queen_distance = RANK8 - Rank(square);
        if (WhiteKing & plus8dir[square]) {
          if (file == FILEA || file == FILEH)
            queen_distance = 99;
          queen_distance++;
        }
        if (Rank(square) == RANK2)
          queen_distance--;
        if (queen_distance < white_queener) {
          white_queener = queen_distance;
          white_square = file + A8;
          white_pawn = square;
          realw = 1;
        }
      }
    }
    if (PopCnt8Bit(tree->pawn_score.passed_w) > 1) {
      int left =
          LastOne(file_mask[FirstOne8Bit(tree->pawn_score.
                  passed_w)] & WhitePawns);
      int right =
          LastOne(file_mask[LastOne8Bit(tree->pawn_score.
                  passed_w)] & WhitePawns);
      if (File(right) - File(left) > 1) {
        if (!(white_pawn_race_wtm[left] & SetMask(right))) {
          queen_distance = RANK8 - Rank(left);
          if (Rank(left) == RANK2)
            queen_distance--;
          if (queen_distance < white_queener) {
            white_queener = queen_distance;
            white_square = A8 + File(left);
            white_pawn = left;
          }
        }
        if (!(white_pawn_race_wtm[right] & SetMask(left))) {
          queen_distance = RANK8 - Rank(right);
          if (Rank(right) == RANK2)
            queen_distance--;
          if (queen_distance < white_queener) {
            white_queener = queen_distance;
            white_square = A8 + File(right);
            white_pawn = right;
          }
        }
      }
    }
  }
#ifdef DEBUGPP
  printf("white pawn on %d can promote at %d in %d moves.\n", white_pawn,
      white_square, white_queener);
#endif
  if (realw && realb == 0) {
    black_queener = 8;
    black_square = 0;
  }
  if (realb && realw == 0) {
    white_queener = 8;
    white_square = 0;
  }
  do {
    if ((white_queener == 8) && (black_queener == 8))
      break;
/*
 ************************************************************
 *                                                          *
 *   now that we know which pawns can outrun the kings for  *
 *   each side, we need to do the following:  if one side   *
 *   queens before the other (two moves or more) then that  *
 *   side wins.                                             *
 *                                                          *
 ************************************************************
 */
    if ((white_queener < 8) && (black_queener == 8))
      return (pawn_can_promote + (5 - white_queener) * pawn_value / 10);
    else if ((black_queener < 8) && (white_queener == 8))
      return (-(pawn_can_promote + (5 - black_queener) * pawn_value / 10));
    if (Flip(wtm))
      black_queener--;
    if (white_queener < black_queener)
      return (pawn_can_promote + (5 - white_queener) * pawn_value / 10);
    else if (black_queener < white_queener - 1)
      return (-(pawn_can_promote + (5 - black_queener) * pawn_value / 10));
    if ((white_queener == 8) || (black_queener == 8))
      break;
/*
 ************************************************************
 *                                                          *
 *   if the white pawn queens one move before black, then   *
 *   if the new queen checks the black king, or the new     *
 *   queen attacks the queening square of black, white wins *
 *   unless the black king is protecting the black queening *
 *   square in which case it's a draw.                      *
 *                                                          *
 ************************************************************
 */
    if (white_queener == black_queener) {
      tempw = WhitePieces;
      Clear(white_pawn, WhitePieces);
      WhitePieces = WhitePieces | SetMask(white_square);
      tempb = BlackPieces;
      Clear(black_pawn, BlackPieces);
      BlackPieces = BlackPieces | SetMask(black_square);
      if (Attack(BlackKingSQ, white_square)) {
        WhitePieces = tempw;
        BlackPieces = tempb;
        return (pawn_can_promote + (5 - white_queener) * pawn_value / 10);
      }
      if (Attack(black_square, white_square) &&
          !(king_attacks[black_square] & BlackKing)) {
        WhitePieces = tempw;
        BlackPieces = tempb;
        return (pawn_can_promote + (5 - white_queener) * pawn_value / 10);
      }
      WhitePieces = tempw;
      BlackPieces = tempb;
    }
/*
 ************************************************************
 *                                                          *
 *   if the black pawn queens one move before white, then   *
 *   if the new queen checks the white king, or the new     *
 *   queen attacks the queening square of white, black wins *
 *   unless the white king is protecting the white queening *
 *   square in which case it's a draw.                      *
 *                                                          *
 ************************************************************
 */
    if (black_queener == white_queener - 1) {
      tempw = WhitePieces;
      Clear(white_pawn, WhitePieces);
      WhitePieces = WhitePieces | SetMask(white_square);
      tempb = BlackPieces;
      Clear(black_pawn, BlackPieces);
      BlackPieces = BlackPieces | SetMask(black_square);
      if (Attack(WhiteKingSQ, black_square)) {
        WhitePieces = tempw;
        BlackPieces = tempb;
        return (-(pawn_can_promote + (5 - black_queener) * pawn_value / 10));
      }
      if (Attack(white_square, black_square) &&
          !(king_attacks[white_square] & WhiteKing)) {
        WhitePieces = tempw;
        BlackPieces = tempb;
        return (-(pawn_can_promote + (5 - black_queener) * pawn_value / 10));
      }
      WhitePieces = tempw;
      BlackPieces = tempb;
    }
  }
  while (0);
  return (0);
}

/* last modified 05/23/06 */
/*
 *******************************************************************************
 *                                                                             *
 *   EvaluatePawns() is used to evaluate pawns.  broadly, it addresses three   *
 *   distinct actions:  (1) basic pawn scoring is hashed, the first thing done *
 *   is to see if the score is in the pawn hash table;  (2) passed pawn scoring*
 *   for positions with pieces left;  (3) passed pawns where one side has a    *
 *   passer and the other side has no pieces, so that the pawn can potentially *
 *   outrun the opposing king and promote.                                     *
 *                                                                             *
 *******************************************************************************
 */
int EvaluatePawns(TREE * RESTRICT tree)
{
  register PAWN_HASH_ENTRY *ptable;
  register BITBOARD pawns;
  register BITBOARD temp;
  register BITBOARD wp_moves, bp_moves;
  register int score = 0;
  register int pns, square, file;
  register int w_isolated, b_isolated;
  register int w_isolated_of, b_isolated_of;
  register int wop, bop;
  register int defenders, attackers, weakness, sq;
  register int average_br = 0, average_wr = 0;
  register int average_bf = 0, average_wf = 0;

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
    return (tree->pawn_score.p_score);
  }
  tree->pawn_score.key = PawnHashKey;
  tree->pawn_score.allw = 0;
  tree->pawn_score.white_defects_k = 0;
  tree->pawn_score.white_defects_e = 0;
  tree->pawn_score.white_defects_d = 0;
  tree->pawn_score.white_defects_q = 0;
  tree->pawn_score.candidates_w = 0;
  tree->pawn_score.passed_w = 0;
  tree->pawn_score.average_w = 0;
  tree->pawn_score.allb = 0;
  tree->pawn_score.black_defects_k = 0;
  tree->pawn_score.black_defects_e = 0;
  tree->pawn_score.black_defects_d = 0;
  tree->pawn_score.black_defects_q = 0;
  tree->pawn_score.candidates_b = 0;
  tree->pawn_score.passed_b = 0;
  tree->pawn_score.average_b = 0;
  tree->pawn_score.outside = 0;
  tree->pawn_score.protected = 0;
  tree->pawn_score.open_files = 255;
  tree->pawn_score.center = 0;
  w_isolated = 0;
  w_isolated_of = 0;
  b_isolated = 0;
  b_isolated_of = 0;
/*
 ************************************************************
 *                                                          *
 *   first, determine which squares pawns can reach.        *
 *                                                          *
 ************************************************************
 */
  pawns = WhitePawns;
  wp_moves = 0;
  while (pawns) {
    square = FirstOne(pawns);
    average_wf += File(square);
    average_wr += Rank(square);
    tree->pawn_score.allw |= 128 >> File(square);
    for (sq = square; sq < A7; sq += 8) {
      wp_moves |= SetMask(sq);
      if (SetMask(sq + 8) & tree->all_pawns)
        break;
      defenders = PopCnt(b_pawn_attacks[sq + 8] & WhitePawns);
      attackers = PopCnt(w_pawn_attacks[sq + 8] & BlackPawns);
      if (attackers - defenders > 0)
        break;
    }
    Clear(square, pawns);
  }
  pawns = BlackPawns;
  bp_moves = 0;
  while (pawns) {
    square = FirstOne(pawns);
    average_bf += File(square);
    average_br += Rank(square);
    tree->pawn_score.allb |= 128 >> File(square);
    for (sq = square; sq > H2; sq -= 8) {
      bp_moves |= SetMask(sq);
      if (SetMask(sq - 8) & tree->all_pawns)
        break;
      attackers = PopCnt(b_pawn_attacks[sq - 8] & WhitePawns);
      defenders = PopCnt(w_pawn_attacks[sq - 8] & BlackPawns);
      if (attackers - defenders > 0)
        break;
    }
    Clear(square, pawns);
  }
  if (TotalWhitePawns) {
    average_wr /= TotalWhitePawns;
    average_wf /= TotalWhitePawns;
    tree->pawn_score.average_w = (average_wr << 3) + average_wf;
  }
  if (TotalBlackPawns) {
    average_br /= TotalBlackPawns;
    average_bf /= TotalBlackPawns;
    tree->pawn_score.average_b = (average_br << 3) + average_bf;
  }
/*
 ************************************************************
 *                                                          *
 *   now determine how blocked the center of the board is   *
 *   so that the bishop pair score will reflect this.       *
 *                                                          *
 ************************************************************
 */
  if (PopCnt(mask_center_files & tree->all_pawns) <= 1)
    tree->pawn_score.center = 2;
  else {
    if (!(((mask_d3d4d5d6 & WhitePawns) >> 8) & BlackPawns) ||
        !(((mask_d3d4d5d6 & WhitePawns) >> 8) & BlackPawns) ||
        Rank(LastOne(mask_d3d4d5d6 & WhitePawns)) ==
        Rank(LastOne(mask_e3e4e5e6 & WhitePawns)))
      tree->pawn_score.center = 1;

  }
/*
 ************************************************************
 *                                                          *
 *   white pawns.                                           *
 *                                                          *
 ************************************************************
 */
  pawns = WhitePawns;
  while (pawns) {
    square = LastOne(pawns);
    file = File(square);
    tree->pawn_score.open_files &= (~128 >> file);
/*
 ************************************************************
 *                                                          *
 *   evaluate pawn advances.  center pawns are encouraged   *
 *   to advance, while wing pawns are pretty much neutral.  *
 *                                                          *
 ************************************************************
 */
    score += pval_w[square];
#ifdef DEBUGP
    printf("white pawn[static] file=%d,   score=%d\n", file, score);
#endif
/*
 ************************************************************
 *                                                          *
 *   evaluate isolated pawns, which also detects that the   *
 *   file is half-open making it easier to attack from the  *
 *   front if rooks are on the board.  if the pawn is       *
 *   isolated, all the weak pawn analysis is skipped.       *
 *                                                          *
 ************************************************************
 */
    if (!(mask_pawn_isolated[square] & WhitePawns)) {
      w_isolated++;
      if ((pns = PopCnt(file_mask[file] & WhitePawns)) > 1) {
        score -= doubled_isolated_pawn_value[pns];
      }
      if (!(plus8dir[square] & BlackPawns))
        w_isolated_of++;
    } else {
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
 *  test the pawn where it is, and as it advances.  note    *
 *  whether it can end up with more pawn defenders than     *
 *  pawn attackers as it advances.                          *
 *                                                          *
 ************************************************************
 */
      do {
        weakness = 0;
        if (plus8dir[square] & BlackPawns)
          break;
        defenders = PopCnt(b_pawn_attacks[square] & WhitePawns);
        attackers = PopCnt(w_pawn_attacks[square] & BlackPawns);
        if (defenders > attackers)
          break;
        defenders = PopCnt(b_pawn_attacks[square + 8] & WhitePawns);
        attackers = PopCnt(w_pawn_attacks[square + 8] & BlackPawns);
        if (attackers && attackers >= defenders)
          weakness = attackers - defenders + 1;
        else if (attackers || defenders)
          break;
        if (weakness == 0)
          break;
/*
 ************************************************************
 *                                                          *
 *  if the pawn can be defended by a pawn, and that pawn    *
 *  can safely advance, then this pawn is not weak.         *
 *                                                          *
 ************************************************************
 */
        if ((temp = mask_no_pattacks_w[square] & WhitePawns)) {
          if (file > FILEA) {
            const BITBOARD temp1 = temp & file_mask[file - 1];

            attackers = 1;
            if (temp1) {
              const int defend_sq = LastOne(temp1);

              for (sq = defend_sq; sq < (Rank(square) << 3); sq += 8) {
                attackers = 1;
                if (sq != defend_sq && tree->all_pawns & SetMask(sq))
                  break;
                attackers = PopCnt(w_pawn_attacks[sq] & BlackPawns);
                if (attackers)
                  break;
              }
              if (attackers == 0)
                weakness = 0;
            }
            if (weakness == 0)
              break;
          }
          if (file < FILEH) {
            const BITBOARD temp1 = temp & file_mask[file + 1];

            if (temp1) {
              const int defend_sq = LastOne(temp1);

              for (sq = defend_sq; sq < (Rank(square) << 3); sq += 8) {
                attackers = 1;
                if (sq != defend_sq && tree->all_pawns & SetMask(sq))
                  break;
                attackers = PopCnt(w_pawn_attacks[sq] & BlackPawns);
                if (attackers)
                  break;
              }
              if (attackers == 0)
                weakness = 0;
            }
          }
        }
        if (weakness > 0) {
          if (weakness == 3)
            score -= pawn_weak_p2;
          else if (weakness)
            score -= pawn_weak_p1;
        }
      }
      while (0);
#ifdef DEBUGP
      printf("white pawn[weak] file=%d,     score=%d\n", file, score);
#endif
/*
 ************************************************************
 *                                                          *
 *   evaluate doubled pawns.  if there are other pawns on   *
 *   this file, penalize this pawn.                         *
 *                                                          *
 ************************************************************
 */
      if ((pns = PopCnt(file_mask[file] & WhitePawns)) > 1) {
        score -= doubled_pawn_value[pns];
      }
#ifdef DEBUGP
      printf("white pawn[doubled] file=%d,  score=%d\n", file, score);
#endif
/*
 ************************************************************
 *                                                          *
 *  test the pawn to see it if forms a "duo" which is two   *
 *  pawns side-by-side.                                     *
 *                                                          *
 ************************************************************
 */
      if (mask_pawn_duo[square] & WhitePawns)
        score += pawn_duo;
#ifdef DEBUGP
      printf("white pawn[duo] file=%d,      score=%d\n", file, score);
#endif
    }
/*
 ************************************************************
 *                                                          *
 *   evaluate passed pawns.                                 *
 *                                                          *
 ************************************************************
 */
    if (!(mask_pawn_passed_w[square] & BlackPawns)) {
      if (LastOne(WhitePawns & file_mask[file]) == square)
        score += passed_pawn_value[Rank(square)];
      if (mask_pawn_protected_w[square] & WhitePawns)
        tree->pawn_score.protected |= 1;
      tree->pawn_score.passed_w |= 128 >> file;
#ifdef DEBUGP
      printf("white pawn[passed]            file=%d\n", file);
      printf("white pawn[passed]            score=%d\n", score);
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
      if (!(file_mask[File(square)] & BlackPawns) &&
          mask_pawn_isolated[square] & WhitePawns &&
          !(w_pawn_attacks[square] & BlackPawns)) {
        attackers = 1;
        defenders = 0;
        for (sq = square; sq < A7; sq += 8) {
          if (SetMask(sq + 8) & tree->all_pawns)
            break;
          defenders = PopCnt(b_pawn_attacks[sq] & wp_moves);
          attackers = PopCnt(w_pawn_attacks[sq] & BlackPawns);
          if (attackers)
            break;
        }
        if (attackers <= defenders) {
          if (!(mask_pawn_passed_w[sq + 8] & BlackPawns)) {
            tree->pawn_score.candidates_w |= 128 >> file;
          }
        }
      }
      if (!(tree->pawn_score.candidates_w & (128 >> file))) {
        if (file <= FILED) {
          if (WhitePawns & file_mask[file + 1]
              && WhitePawns & file_mask[file + 2]) {
            if (!(BlackPawns & file_mask[file])
                && !(BlackPawns & file_mask[file + 2])
                && !(BlackPawns & file_mask[file + 3])
                && PopCnt(BlackPawns & file_mask[file + 1]) <= 2)
              tree->pawn_score.candidates_w |= 128 >> file;
          }
        } else {
          if (WhitePawns & file_mask[file - 1]
              && WhitePawns & file_mask[file - 2]) {
            if (!(BlackPawns & file_mask[file])
                && !(BlackPawns & file_mask[file - 2])
                && !(BlackPawns & file_mask[file - 3])
                && PopCnt(BlackPawns & file_mask[file - 1]) <= 2)
              tree->pawn_score.candidates_w |= 128 >> file;
          }
        }
      }
#ifdef DEBUGP
      if (tree->pawn_score.candidates_w & (128 >> file))
        printf("white pawn[candidate]       square=%d\n", square);
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
      if (Rank(square) > RANK5 && SetMask(square + 8) & BlackPawns &&
          !(mask_pawn_passed_w[square + 8] & BlackPawns) &&
          ((File(square) < FILEH && SetMask(square - 7) & WhitePawns &&
                  !(plus8dir[square - 7] & BlackPawns)
                  && (File(square) == FILEG ||
                      !(plus8dir[square - 6] & BlackPawns)))
              || (File(square) > FILEA && SetMask(square - 9) & WhitePawns &&
                  !(plus8dir[square - 9] & BlackPawns)
                  && (File(square) == FILEB ||
                      !(plus8dir[square - 10] & BlackPawns)))))
        score += hidden_passed_pawn_value[Rank(square)];
#ifdef DEBUGP
      printf("white pawn[hidden] file=%d,   score=%d\n", file, score);
#endif
    }
    Clear(square, pawns);
  }
#ifdef DEBUGP
  printf("white pawn[space]             score=%d\n", score);
#endif
/*
 ************************************************************
 *                                                          *
 *   black pawns.                                           *
 *                                                          *
 ************************************************************
 */
  pawns = BlackPawns;
  while (pawns) {
    square = FirstOne(pawns);
    file = File(square);
    tree->pawn_score.open_files &= (~128 >> file);
/*
 ************************************************************
 *                                                          *
 *   evaluate pawn advances.  center pawns are encouraged   *
 *   to advance, while wing pawns are pretty much neutral.  *
 *                                                          *
 ************************************************************
 */
    score -= pval_b[square];
#ifdef DEBUGP
    printf("black pawn[static] file=%d,   score=%d\n", file, score);
#endif
/*
 ************************************************************
 *                                                          *
 *   evaluate isolated pawns, which also detects that the   *
 *   file is half-open making it easier to attack from the  *
 *   front if rooks are on the board.  if the pawn is       *
 *   isolated, all the weak pawn analysis is skipped.       *
 *                                                          *
 ************************************************************
 */
    if (!(mask_pawn_isolated[square] & BlackPawns)) {
      b_isolated++;
      if ((pns = PopCnt(file_mask[file] & BlackPawns)) > 1) {
        score += doubled_isolated_pawn_value[pns];
      }
      if (!(minus8dir[square] & WhitePawns))
        b_isolated_of++;
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
 *  test the pawn where it is, and as it advances.  note    *
 *  whether it can end up with more pawn defenders than     *
 *  pawn attackers as it advances.                          *
 *                                                          *
 ************************************************************
 */
    else {
      do {
        weakness = 0;
        if (minus8dir[square] & WhitePawns)
          break;
        attackers = PopCnt(b_pawn_attacks[square] & WhitePawns);
        defenders = PopCnt(w_pawn_attacks[square] & BlackPawns);
        if (defenders > attackers)
          break;
        attackers = PopCnt(b_pawn_attacks[square - 8] & WhitePawns);
        defenders = PopCnt(w_pawn_attacks[square - 8] & BlackPawns);
        if (attackers && attackers >= defenders)
          weakness = attackers - defenders + 1;
        else if (attackers || defenders)
          break;
        if (weakness == 0)
          break;
/*
 ************************************************************
 *                                                          *
 *  if the pawn can be defended by a pawn, and that pawn    *
 *  can safely advance, then this pawn is not weak.         *
 *                                                          *
 ************************************************************
 */
        if ((temp = mask_no_pattacks_b[square] & BlackPawns)) {
          if (file > FILEA) {
            const BITBOARD temp1 = temp & file_mask[file - 1];

            attackers = 1;
            if (temp1) {
              const int defend_sq = FirstOne(temp1);

              for (sq = defend_sq; sq >= ((Rank(square) + 1) << 3); sq -= 8) {
                attackers = 1;
                if (sq != defend_sq && tree->all_pawns & SetMask(sq))
                  break;
                attackers = PopCnt(b_pawn_attacks[sq] & WhitePawns);
                if (attackers)
                  break;
              }
              if (attackers == 0)
                weakness = 0;
            }
            if (weakness == 0)
              break;
          }
          if (file < FILEH) {
            const BITBOARD temp1 = temp & file_mask[file + 1];

            if (temp1) {
              const int defend_sq = FirstOne(temp1);

              for (sq = defend_sq; sq >= ((Rank(square) + 1) << 3); sq -= 8) {
                attackers = 1;
                if (sq != defend_sq && tree->all_pawns & SetMask(sq))
                  break;
                attackers = PopCnt(b_pawn_attacks[sq] & WhitePawns);
                if (attackers)
                  break;
              }
              if (attackers == 0)
                weakness = 0;
            }
          }
        }

        if (weakness > 0) {
          if (weakness == 3)
            score += pawn_weak_p2;
          else if (weakness)
            score += pawn_weak_p1;
        }
      }
      while (0);
#ifdef DEBUGP
      printf("black pawn[weak] file=%d,     score=%d\n", file, score);
#endif
/*
 ************************************************************
 *                                                          *
 *   evaluate doubled pawns.  if there are other pawns on   *
 *   this file, penalize this pawn.                         *
 *                                                          *
 ************************************************************
 */
      if ((pns = PopCnt(file_mask[file] & BlackPawns)) > 1) {
        score += doubled_pawn_value[pns];
      }
#ifdef DEBUGP
      printf("black pawn[doubled] file=%d,  score=%d\n", file, score);
#endif
/*
 ************************************************************
 *                                                          *
 *  test the pawn to see it if forms a "duo" which is two   *
 *  pawns side-by-side.                                     *
 *                                                          *
 ************************************************************
 */
      if (mask_pawn_duo[square] & BlackPawns)
        score -= pawn_duo;
#ifdef DEBUGP
      printf("black pawn[duo] file=%d,      score=%d\n", file, score);
#endif
    }
/*
 ************************************************************
 *                                                          *
 *   evaluate passed pawns.                                 *
 *                                                          *
 ************************************************************
 */
    if (!(mask_pawn_passed_b[square] & WhitePawns)) {
      if (FirstOne(BlackPawns & file_mask[file]) == square)
        score -= passed_pawn_value[(RANK8 - Rank(square))];
      if (mask_pawn_protected_b[square] & BlackPawns)
        tree->pawn_score.protected |= 2;
      tree->pawn_score.passed_b |= 128 >> file;
#ifdef DEBUGP
      printf("black pawn[passed]            file=%d\n", file);
      printf("black pawn[passed]            score=%d\n", score);
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
      if (!(file_mask[File(square)] & WhitePawns) &&
          mask_pawn_isolated[square] & BlackPawns &&
          !(b_pawn_attacks[square] & WhitePawns)) {
        attackers = 1;
        defenders = 0;
        for (sq = square; sq > H2; sq -= 8) {
          if (SetMask(sq - 8) & tree->all_pawns)
            break;
          attackers = PopCnt(b_pawn_attacks[sq] & WhitePawns);
          defenders = PopCnt(w_pawn_attacks[sq] & bp_moves);
          if (attackers)
            break;
        }
        if (attackers <= defenders) {
          if (!(mask_pawn_passed_b[sq - 8] & WhitePawns)) {
            tree->pawn_score.candidates_b |= 128 >> file;
          }
        }
      }
      if (!(tree->pawn_score.candidates_b & (128 >> file))) {
        if (file <= FILED) {
          if (BlackPawns & file_mask[file + 1]
              && BlackPawns & file_mask[file + 2]) {
            if (!(WhitePawns & file_mask[file])
                && !(WhitePawns & file_mask[file + 2])
                && !(WhitePawns & file_mask[file + 3])
                && PopCnt(WhitePawns & file_mask[file + 1]) <= 2)
              tree->pawn_score.candidates_b |= 128 >> file;
          }
        } else {
          if (BlackPawns & file_mask[file - 1]
              && BlackPawns & file_mask[file - 2]) {
            if (!(WhitePawns & file_mask[file])
                && !(WhitePawns & file_mask[file - 2])
                && !(WhitePawns & file_mask[file - 3])
                && PopCnt(WhitePawns & file_mask[file - 1]) <= 2)
              tree->pawn_score.candidates_b |= 128 >> file;
          }
        }
      }
#ifdef DEBUGP
      if (tree->pawn_score.candidates_b & (128 >> file))
        printf("black pawn[candidate]       square=%d\n", square);
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
      if (Rank(square) < RANK4 && SetMask(square - 8) & WhitePawns &&
          !(mask_pawn_passed_b[square - 8] & WhitePawns) &&
          ((File(square) < FILEH && SetMask(square + 9) & BlackPawns &&
                  !(minus8dir[square + 9] & WhitePawns) &&
                  (File(square) == FILEG ||
                      !(minus8dir[square + 10] & WhitePawns)))
              || (File(square) > FILEA && SetMask(square + 7) & BlackPawns &&
                  !(minus8dir[square + 7] & WhitePawns)
                  && (File(square) == FILEB ||
                      !(minus8dir[square + 6] & WhitePawns))))) {
        score -= hidden_passed_pawn_value[(RANK8 - Rank(square))];
      }
#ifdef DEBUGP
      printf("black pawn[hidden] file=%d,   score=%d\n", file, score);
#endif
    }
    Clear(square, pawns);
  }
#ifdef DEBUGP
  printf("black pawn[space]             score=%d\n", score);
#endif
/*
 ************************************************************
 *                                                          *
 *   now fold in the penalty for isolated pawns, which is   *
 *   non-linear to penalize more isolani more severely.     *
 *   note that the penalty penalizes the side with the      *
 *   most isolated pawns, in an exponential rate.           *
 *                                                          *
 ************************************************************
 */
  score -= isolated_pawn_value[w_isolated];
  score -= isolated_pawn_of_value[w_isolated_of];
  score += isolated_pawn_value[b_isolated];
  score += isolated_pawn_of_value[b_isolated_of];
#ifdef DEBUGP
  printf("pawn[isolated]          score=%d\n", score);
#endif
/*
 ************************************************************
 *                                                          *
 *   now evaluate king safety.                              *
 *                                                          *
 *   the first step is to step across the board and note    *
 *   which files are open/half open.  since this is common  *
 *   to both kings, this is only done once since it is the  *
 *   same for both players.                                 *
 *                                                          *
 *   at the same time we note if any of the three pawns in  *
 *   front of the king have moved, and count those as well. *
 *                                                          *
 ************************************************************
 */
  tree->pawn_score.white_defects_q += EvaluateKingsFileW(tree, FILEB);
  tree->pawn_score.black_defects_q += EvaluateKingsFileB(tree, FILEB);
  tree->pawn_score.white_defects_d += EvaluateKingsFileW(tree, FILED);
  tree->pawn_score.black_defects_d += EvaluateKingsFileB(tree, FILED);
  tree->pawn_score.white_defects_e += EvaluateKingsFileW(tree, FILEE);
  tree->pawn_score.black_defects_e += EvaluateKingsFileB(tree, FILEE);
  tree->pawn_score.white_defects_k += EvaluateKingsFileW(tree, FILEG);
  tree->pawn_score.black_defects_k += EvaluateKingsFileB(tree, FILEG);
#if defined(DEBUGK)
  printf("white: kdefects=%d  qdefects=%d\n", tree->pawn_score.white_defects_k,
      tree->pawn_score.white_defects_q);
  printf("black: kdefects=%d  qdefects=%d\n", tree->pawn_score.black_defects_k,
      tree->pawn_score.black_defects_q);
#endif
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
 *  opposite side of the board.  this case is treated as    *
 *  neither having an outside passer.                       *
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
 *         (c/p -> candidate or passer)                     *
 *                                                          *
 *        xxxx xxx1   (1) -> white has outside c/p          *
 *        xxxx xx1x   (2) -> white has 2 outside c/p        *
 *        xxxx x1xx   (4) -> black has outside c/p          *
 *        xxxx 1xxx   (8) -> black has 2 outside c/p        *
 *                                                          *
 ************************************************************
 */
  if (TotalBlackPawns)
    wop =
        is_outside[tree->pawn_score.passed_w][tree->pawn_score.
        allb] | is_outside_c[tree->pawn_score.candidates_w]
        [tree->pawn_score.allb];
  else
    wop = 0;
  if (TotalWhitePawns)
    bop =
        is_outside[tree->pawn_score.passed_b][tree->pawn_score.
        allw] | is_outside_c[tree->pawn_score.candidates_b]
        [tree->pawn_score.allw];
  else
    bop = 0;
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
/*
  if (tree->pawn_score.outside) {
    printf(">>>>>>>>>>>>>>>>>>>  outside=%x\n",tree->pawn_score.outside);
    printf("w_passed=%x  b_passed=%x\n",
    tree->pawn_score.passed_w,tree->pawn_score.passed_b);
    printf("w_candidates=%x  b_candidates=%x\n",
    tree->pawn_score.candidates_w,tree->pawn_score.candidates_b);
    wop=is_outside[tree->pawn_score.passed_w][tree->pawn_score.allb];
    bop=is_outside[tree->pawn_score.passed_b][tree->pawn_score.allw];
    printf("allb=%x  allw=%x\n",tree->pawn_score.allb,tree->pawn_score.allw);
    printf("passed, wop=%d  bop=%d\n",wop,bop);
    wop=is_outside_c[tree->pawn_score.candidates_w][tree->pawn_score.allb];
    bop=is_outside_c[tree->pawn_score.candidates_b][tree->pawn_score.allw];
    printf("candidates, wop=%d  bop=%d\n",wop,bop);
    printf("protected=%x\n",tree->pawn_score.protected);
  }
*/
/*
 ************************************************************
 *                                                          *
 *   store the results in the pawn hash table for reuse at  *
 *   a later time as needed.                                *
 *                                                          *
 ************************************************************
 */
  tree->pawn_score.p_score = score;
  *ptable = tree->pawn_score;
  return (score);
}

/* last modified 05/23/06 */
/*
 *******************************************************************************
 *                                                                             *
 *   EvaluateQueens() is used to evaluate black/white queens.                  *
 *                                                                             *
 *******************************************************************************
 */
int EvaluateQueens(TREE * RESTRICT tree)
{
  register BITBOARD temp;
  register int square, trop;
  register int score = 0;

/*
 ************************************************************
 *                                                          *
 *   white queens                                           *
 *                                                          *
 *   first locate each queen and obtain it's centralization *
 *   score from the static piece/square table for queens.   *
 *                                                          *
 ************************************************************
 */
  temp = WhiteQueens;
  while (temp) {
    square = FirstOne(temp);
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
    if (Rank(square) == RANK7 && (BlackPawns & rank_mask[RANK7] ||
            (BlackKingSQ > H7))) {
      if (AttacksRank(square) & WhiteRooks)
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
    if (TotalWhitePawns > 4) {
      if ((File(square) < FILEC && File(BlackKingSQ) > FILEE) ||
          (File(square) > FILEF && File(BlackKingSQ) < FILED))
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
    trop = 7;
    if (!(plus8dir[square] & WhitePawns))
      trop = FileDistance(square, BlackKingSQ);
    tree->w_tropism +=
        Max(king_tropism_q[Distance(square, BlackKingSQ)],
        king_tropism_at_q[trop]);
    Clear(square, temp);
  }
#ifdef DEBUGEV
  printf("score[queens(white)]=             %4d\n", score);
  printf("tropism[queens(white)]=           %4d\n", tree->w_tropism);
#endif
/*
 ************************************************************
 *                                                          *
 *   black queens                                           *
 *                                                          *
 *   first locate each queen and obtain it's centralization *
 *   score from the static piece/square table for queens.   *
 *                                                          *
 ************************************************************
 */
  temp = BlackQueens;
  while (temp) {
    square = FirstOne(temp);
    score -= qval[square];
/*
 ************************************************************
 *                                                          *
 *   check to see if the queen is in a strong position on   *
 *   the 7th rank supported by a rook on the 7th.  if so,   *
 *   the positional advantage is almost overwhelming.       *
 *                                                          *
 ************************************************************
 */
    if (Rank(square) == RANK2 && (WhitePawns & rank_mask[RANK2] ||
            (WhiteKingSQ < A2))) {
      if (AttacksRank(square) & BlackRooks)
        score -= queen_rook_on_7th_rank;
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
    if (TotalBlackPawns > 4) {
      if ((File(square) < FILEC && File(WhiteKingSQ) > FILEE) ||
          (File(square) > FILEF && File(WhiteKingSQ) < FILED))
        score += queen_offside;
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
    trop = 7;
    if (!(minus8dir[square] & BlackPawns))
      trop = FileDistance(square, WhiteKingSQ);
    tree->b_tropism +=
        Max(king_tropism_q[Distance(square, WhiteKingSQ)],
        king_tropism_at_q[trop]);
    Clear(square, temp);
  }
#ifdef DEBUGEV
  printf("score[queens(black)]=             %4d\n", score);
  printf("tropism[queens(black)]=           %4d\n", tree->b_tropism);
#endif
  return (score);
}

/* last modified 05/23/06 */
/*
 *******************************************************************************
 *                                                                             *
 *   EvaluateRooks() is used to evaluate black/white rooks.                    *
 *                                                                             *
 *******************************************************************************
 */
int EvaluateRooks(TREE * RESTRICT tree)
{
  register BITBOARD temp;
  register int square, file, trop, open_files;
  register int score = 0;

  open_files = PopCnt8Bit(tree->pawn_score.open_files);
/*
 ************************************************************
 *                                                          *
 *   white rooks                                            *
 *                                                          *
 *   first fold in the piece/square value from rval[].      *
 *                                                          *
 ************************************************************
 */
  temp = WhiteRooks;
  while (temp) {
    square = FirstOne(temp);
    file = File(square);
    score += rval[square];
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
    trop = 7;
    if (tree->pawn_score.open_files & 128 >> file) {
      score += rook_open_file[open_files][file];
      trop = FileDistance(square, BlackKingSQ);
    } else {
      if (open_files) {
        unsigned char rankmvs = AttacksRank(square) >> (56 - (square & 0x38));

        if (!(rankmvs & tree->pawn_score.open_files))
          score -= rook_reaches_open_file;
      }
      if (!(file_mask[file] & WhitePawns)) {
        score += rook_half_open_file;
        trop = FileDistance(square, BlackKingSQ);
      } else if (!(plus8dir[square] & WhitePawns)) {
        trop = FileDistance(square, BlackKingSQ);
      }
    }
/*
 ************************************************************
 *                                                          *
 *   see if the rook is behind a passed pawn.  if it is,    *
 *   it is given a bonus.                                   *
 *                                                          *
 ************************************************************
 */
    if (128 >> file & tree->pawn_score.passed_w) {
      register const int pawnsq = LastOne(WhitePawns & file_mask[file]);

      if (square < pawnsq)
        score += rook_behind_passed_pawn;
    }
    if (128 >> file & tree->pawn_score.passed_b) {
      register const int pawnsq = FirstOne(BlackPawns & file_mask[file]);

      if (pawnsq < square)
        score += rook_behind_passed_pawn;
    }
/*
 ************************************************************
 *                                                          *
 *   finally check to see if any rooks are on the 7th rank, *
 *   with the opponent having pawns on that rank and the    *
 *   opponent's king being hemmed in on the 7th/8th rank.   *
 *   if so, and another rook or queen is also on the 7th,   *
 *   then this is a *strong* positional advantage.          *
 *                                                          *
 ************************************************************
 */
    if (Rank(square) == RANK7 && (BlackKingSQ > H7 ||
            BlackPawns & rank_mask[RANK7])) {
      score += rook_on_7th;
      if (AttacksRank(square) & (WhiteRooks | WhiteQueens))
        score += rook_connected_7th_rank;
    }
/*
 ************************************************************
 *                                                          *
 *   adjust the tropism count for this piece.               *
 *                                                          *
 ************************************************************
 */
    tree->w_tropism +=
        Max(king_tropism_at_r[trop], king_tropism_r[Distance(square,
                BlackKingSQ)]);
    Clear(square, temp);
  }
#ifdef DEBUGEV
  printf("score[rooks(white)]=              %4d\n", score);
  printf("tropism[rooks(white)]=            %4d\n", tree->w_tropism);
#endif
/*
 ************************************************************
 *                                                          *
 *   black rooks                                            *
 *                                                          *
 *   first fold in the piece/square value from rval[].      *
 *                                                          *
 ************************************************************
 */
  temp = BlackRooks;
  while (temp) {
    square = FirstOne(temp);
    file = File(square);
    score -= rval[square];
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
    trop = 7;
    if (tree->pawn_score.open_files & 128 >> file) {
      score -= rook_open_file[open_files][file];
      trop = FileDistance(square, WhiteKingSQ);
    } else {
      if (open_files) {
        unsigned char rankmvs = AttacksRank(square) >> (56 - (square & 0x38));

        if (!(rankmvs & tree->pawn_score.open_files))
          score += rook_reaches_open_file;
      }
      if (!(file_mask[file] & BlackPawns)) {
        score -= rook_half_open_file;
        trop = FileDistance(square, WhiteKingSQ);
      } else if (!(minus8dir[square] & BlackPawns)) {
        trop = FileDistance(square, WhiteKingSQ);
      }
    }
/*
 ************************************************************
 *                                                          *
 *   see if the rook is behind a passed pawn.  if it is,    *
 *   it is given a bonus.                                   *
 *                                                          *
 ************************************************************
 */
    if (128 >> file & tree->pawn_score.passed_b) {
      register const int pawnsq = FirstOne(BlackPawns & file_mask[file]);

      if (square > pawnsq)
        score -= rook_behind_passed_pawn;
    }
    if (128 >> file & tree->pawn_score.passed_w) {
      register const int pawnsq = LastOne(WhitePawns & file_mask[file]);

      if (pawnsq > square)
        score -= rook_behind_passed_pawn;
    }
/*
 ************************************************************
 *                                                          *
 *   finally check to see if any rooks are on the 7th rank, *
 *   with the opponent having pawns on that rank and the    *
 *   opponent's king being hemmed in on the 7th/8th rank.   *
 *   if so, and another rook or queen is also on the 7th,   *
 *   then this is a *strong* positional advantage.          *
 *                                                          *
 ************************************************************
 */
    if (Rank(square) == RANK2 && (WhiteKingSQ < A2 ||
            WhitePawns & rank_mask[RANK2])) {
      score -= rook_on_7th;
      if (AttacksRank(square) & (BlackRooks | BlackQueens))
        score -= rook_connected_7th_rank;
    }
/*
 ************************************************************
 *                                                          *
 *   adjust the tropism count for this piece.               *
 *                                                          *
 ************************************************************
 */
    tree->b_tropism +=
        Max(king_tropism_at_r[trop], king_tropism_r[Distance(square,
                WhiteKingSQ)]);
    Clear(square, temp);
  }
#ifdef DEBUGEV
  printf("score[rooks(black)]=              %4d\n", score);
  printf("tropism[rooks(black)]=            %4d\n", tree->b_tropism);
#endif
  return (score);
}

/* last modified 10/09/05 */
/*
 *******************************************************************************
 *                                                                             *
 *   EvaluateStaleMate() is used to determine if one side has only a king and  *
 *   the king has no legal moves.  this is used to correct a codition that can *
 *   confuse the "bishop + wrong rook pawn = draw" code (as well as others that*
 *   are similar.  if the side with the lone king is on move and has no legal  *
 *   moves, then a draw score is returned.                                     *
 *                                                                             *
 *******************************************************************************
 */
int EvaluateStalemate(TREE * RESTRICT tree, int wtm)
{
  int stalemate = 0;

/*
 ************************************************************
 *                                                          *
 *   if white has only a king and it can't legally move,    *
 *   and white is on move, then this is a stalemate.        *
 *                                                          *
 ************************************************************
 */
  if (wtm) {
    if (TotalWhitePieces + TotalWhitePawns == 0) {
      BITBOARD sm_possible = stalemate_sqs[WhiteKingSQ] & BlackKing;

      if (sm_possible) {
        BITBOARD moves = edge_moves[WhiteKingSQ] & ~WhitePieces;

        stalemate = 1;
        while (moves) {
          int square = FirstOne(moves);

          if (!Attacked(tree, square, 0)) {
            stalemate = 0;
            break;
          }
          Clear(square, moves);
        }
      }
    }
  }
/*
 ************************************************************
 *                                                          *
 *   if black has only a king and it can't legally move,    *
 *   and white is on move, then this is a stalemate.        *
 *                                                          *
 ************************************************************
 */
  else {
    if (TotalBlackPieces + TotalBlackPawns == 0) {
      BITBOARD sm_possible = stalemate_sqs[BlackKingSQ] & WhiteKing;

      if (sm_possible) {
        BITBOARD moves = edge_moves[BlackKingSQ] & ~BlackPieces;

        stalemate = 1;
        while (moves) {
          int square = FirstOne(moves);

          if (!Attacked(tree, square, 1)) {
            stalemate = 0;
            break;
          }
          Clear(square, moves);
        }
      }
    }
  }
  return (stalemate);
}

/* last modified 09/16/05 */
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
int EvaluateWinningChances(TREE * RESTRICT tree)
{
  register int can_win = 3;

/*
 ************************************************************
 *                                                          *
 *   if one side is a piece up, but has no pawns, then that *
 *   side can not possibly win.  we recognize an exception  *
 *   where the weaker side's king is trapped on the edge    *
 *   where it might be checkmated.                          *
 *                                                          *
 ************************************************************
 */
  if (TotalWhitePawns == 0 && TotalWhitePieces - TotalBlackPieces <= 3) {
    if ((mask_not_edge & BlackKing) || TotalWhitePieces <= 3)
      can_win &= 2;
  }
  if (TotalBlackPawns == 0 && TotalBlackPieces - TotalWhitePieces <= 3) {
    if ((mask_not_edge & WhiteKing) || TotalBlackPieces <= 3)
      can_win &= 1;
  }
  if (can_win == 0)
    return (can_win);
/*
 ************************************************************
 *                                                          *
 *   if one side is an exchange up, but has no pawns, then  *
 *   that side can not possibly win.                        *
 *                                                          *
 ************************************************************
 */
  if (Majors) {
    if (Majors == -Minors) {
      if (TotalBlackPawns == 0)
        can_win &= 1;
      if (TotalWhitePawns == 0)
        can_win &= 2;
    }
    if (can_win == 0)
      return (can_win);
  }
/*
 ************************************************************
 *                                                          *
 *   if neither side has any pieces, and both sides have    *
 *   non-rookpawns, then either side can win.  also, if one *
 *   has a piece and the other side has one pawn, then that *
 *   piece can sac itself for the pawn so that the side     *
 *   with a pawn can't win.                                 *
 *                                                          *
 ************************************************************
 */
  if (TotalWhitePieces == 0 && TotalBlackPieces == 0) {
    if (WhitePawns & not_rook_pawns && BlackPawns & not_rook_pawns)
      return (can_win);
  }
  if (TotalBlackPieces == 0) {
    if (TotalBlackPawns == 0)
      can_win &= 1;
    if (TotalBlackPawns == 1 && TotalWhitePieces)
      can_win &= 1;
  }
  if (TotalWhitePieces == 0) {
    if (TotalWhitePawns == 0)
      can_win &= 2;
    if (TotalWhitePawns == 1 && TotalBlackPieces)
      can_win &= 2;
  }
/*
 ************************************************************
 *                                                          *
 *   if white has a pawn, then either the pawn had better   *
 *   not be a rook pawn, or else white had better have the  *
 *   right color bishop or any other piece, otherwise it is *
 *   not winnable if the black king can get to the queening *
 *   square first.                                          *
 *                                                          *
 ************************************************************
 */
  if (TotalWhitePawns)
    do {
      if (WhitePawns & not_rook_pawns)
        continue;
      if (TotalWhitePieces > 3 || (TotalWhitePieces == 3 && WhiteKnights))
        continue;
      if (TotalWhitePieces == 0) {
        if (file_mask[FILEA] & WhitePawns && file_mask[FILEH] & WhitePawns)
          continue;
      }
      if (!(WhitePawns & not_rook_pawns)) {
        if (WhiteBishops) {
          if (!BlackBishops) {
            if (WhiteBishops & dark_squares) {
              if (file_mask[FILEH] & WhitePawns)
                continue;
            } else if (file_mask[FILEA] & WhitePawns)
              continue;
          } else {
            if (WhiteBishops & dark_squares && !(BlackBishops & dark_squares)) {
              if (file_mask[FILEH] & WhitePawns)
                continue;
            } else if (file_mask[FILEA] & WhitePawns)
              continue;
          }
        }
        if (!(WhitePawns & file_mask[FILEA]) ||
            !(WhitePawns & file_mask[FILEH])) {
          if (WhitePawns & file_mask[FILEA]) {
            int bkd, wkd, pd;

            bkd = Distance(BlackKingSQ, A8) - Flip(wtm);
            if (bkd <= 1)
              can_win &= 2;
            else {
              wkd = Distance(WhiteKingSQ, A8) - wtm;
              pd = Distance(LastOne(WhitePawns & file_mask[FILEA]), A8) - wtm;
              if (bkd < wkd && bkd <= pd)
                can_win &= 2;
            }
            continue;
          } else {
            int bkd, wkd, pd;

            bkd = Distance(BlackKingSQ, H8) - Flip(wtm);
            if (bkd <= 1)
              can_win &= 2;
            else {
              wkd = Distance(WhiteKingSQ, H8) - wtm;
              pd = Distance(LastOne(WhitePawns & file_mask[FILEH]), H8) - wtm;
              if (bkd < wkd && bkd <= pd)
                can_win &= 2;
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
 *   if black has a pawn, then either the pawn had better   *
 *   not be a rook pawn, or else black had better have the  *
 *   right color bishop or any other piece, otherwise it is *
 *   not winnable if the white king can get to the queening *
 *   square first.                                          *
 *                                                          *
 ************************************************************
 */
  if (TotalBlackPawns)
    do {
      if (BlackPawns & not_rook_pawns)
        continue;
      if (TotalBlackPieces > 3 || (TotalBlackPieces == 3 && BlackKnights))
        continue;
      if (TotalBlackPieces == 0) {
        if (file_mask[FILEA] & BlackPawns && file_mask[FILEH] & BlackPawns)
          continue;
      }
      if (!(BlackPawns & not_rook_pawns)) {
        if (BlackBishops) {
          if (!WhiteBishops) {
            if (BlackBishops & dark_squares) {
              if (file_mask[FILEA] & BlackPawns)
                continue;
            } else if (file_mask[FILEH] & BlackPawns)
              continue;
          } else {
            if (BlackBishops & dark_squares && !(WhiteBishops & dark_squares)) {
              if (file_mask[FILEA] & BlackPawns)
                continue;
            } else if (file_mask[FILEH] & BlackPawns)
              continue;
          }
        }
        if (!(BlackPawns & file_mask[FILEA]) ||
            !(BlackPawns & file_mask[FILEH])) {
          if (BlackPawns & file_mask[FILEA]) {
            int bkd, wkd, pd;

            wkd = Distance(WhiteKingSQ, A1) - wtm;
            if (wkd <= 1)
              can_win &= 1;
            else {
              bkd = Distance(BlackKingSQ, A1) - Flip(wtm);
              pd = Distance(FirstOne(BlackPawns & file_mask[FILEA]),
                  A1) - Flip(wtm);
              if (wkd < bkd && wkd <= pd)
                can_win &= 1;
            }
            continue;
          } else {
            int bkd, wkd, pd;

            wkd = Distance(WhiteKingSQ, H1) - wtm;
            if (wkd <= 1)
              can_win &= 1;
            else {
              bkd = Distance(BlackKingSQ, H1) - Flip(wtm);
              pd = Distance(FirstOne(BlackPawns & file_mask[FILEH]),
                  H1) - Flip(wtm);
              if (wkd < bkd && wkd <= pd)
                can_win &= 1;
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
 *   if one side has a rook, while the other side has a     *
 *   minor + pawns, then the rook can't possibly win.       *
 *                                                          *
 ************************************************************
 */
  if (TotalWhitePawns && TotalBlackPawns)
    return (can_win);
/*
 ************************************************************
 *                                                          *
 *   if one side has two bishops, and the other side has    *
 *   a single kinght, the two bishops win.                  *
 *                                                          *
 ************************************************************
 */
  if (TotalWhitePawns == 0 && TotalWhitePieces == 6 && TotalBlackPieces == 3) {
    if (WhiteKnights || !BlackKnights)
      can_win &= 2;
  } else if (TotalBlackPawns == 0 && TotalBlackPieces == 6 &&
      TotalWhitePieces == 3) {
    if (BlackKnights || !WhiteKnights)
      can_win &= 1;
  }
/*
 ************************************************************
 *                                                          *
 *   if one side is two knights ahead and the opponent has  *
 *   no remaining material, it is a draw.                   *
 *                                                          *
 ************************************************************
 */
  if (TotalWhitePawns == 0 && TotalWhitePieces == 6 && !WhiteBishops &&
      TotalBlackPieces + TotalBlackPawns == 0)
    can_win &= 2;
  if (TotalBlackPawns == 0 && TotalBlackPieces == 6 && !BlackBishops &&
      TotalWhitePieces + TotalWhitePawns == 0)
    can_win &= 1;
  return (can_win);
}
