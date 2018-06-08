#include <stdio.h>
#include <stdlib.h>
#include "chess.h"
#include "data.h"

/* last modified 08/07/05 */
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
  register BITBOARD temp;
  register int square, file, score, tscore, w_tropism = 0, b_tropism = 0;
  register int w_spread, b_spread, trop, can_win = 3;

#if defined(DETECTDRAW)
  int drawing = 0;
#endif
#if defined(DEBUGEV)
  int lastsc;
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
  if (TotalWhitePieces < 13 && TotalBlackPieces < 13) {
    can_win = EvaluateWinner(tree);
    if (EvaluateStalemate(tree, wtm))
      can_win = 0;
  }
  if (can_win == 0 && TotalWhitePawns + TotalBlackPawns == 0)
    return (DrawScore(wtm));
#if defined(DETECTDRAW)
  if (TotalWhitePawns > 2 && TotalBlackPawns > 2 &&
      !(WhiteKnights | BlackKnights))
    drawing = EvaluateDraws(tree, wtm);
  if (drawing)
    return (DrawScore(wtm));
#endif
  score = EvaluateMaterial(tree);
#ifdef DEBUGEV
  printf("score[material]=                  %4d\n", score);
  lastsc = score;
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
 **********************************************************************
 */
  tree->all_pawns = BlackPawns | WhitePawns;
  if ((TotalWhitePawns + TotalBlackPawns) == 0)
    do {
      int ms = EvaluateMate(tree);

      score += ms;
#ifdef DEBUGEV
      printf("score[mater]=                     %4d (%+d)\n", score,
          score - lastsc);
#endif
      if (score > DrawScore(1) && !(can_win & 1))
        score = score >> 2;
      if (score < DrawScore(1) && !(can_win & 2))
        score = score >> 2;
      return ((wtm) ? score : -score);
    } while (0);
#if !defined(FAST)
  tree->pawn_probes++;
#endif
  if (PawnHashKey == tree->pawn_score.key) {
#if !defined(FAST)
    tree->pawn_hits++;
#endif
    score += tree->pawn_score.p_score;
  } else
    score += EvaluatePawns(tree);
#ifdef DEBUGEV
  if (score != lastsc)
    printf("score[pawns]=                     %4d (%+d)\n", score,
        score - lastsc);
  lastsc = score;
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
 *   scoring term completely.                                         *
 *                                                                    *
 *        xxxx xxx1   (1) -> white has outside c/p                    *
 *        xxxx xx1x   (2) -> white has 2 outside c/p                  *
 *        xxxx x1xx   (4) -> black has outside c/p                    *
 *        xxxx 1xxx   (8) -> black has 2 outside c/p                  *
 *                                                                    *
 **********************************************************************
 */
  if (tree->pawn_score.passed_b || tree->pawn_score.passed_w) {
    int pscore = EvaluatePassedPawns(tree);

    if ((TotalWhitePieces == 0 && tree->pawn_score.passed_b) ||
        (TotalBlackPieces == 0 && tree->pawn_score.passed_w))
      pscore += EvaluatePassedPawnRaces(tree, wtm);
    score += pscore * passed_scale / 100;
  }
  if (tree->pawn_score.outside) {
    int pscore = 0;

    if (tree->pawn_score.outside & 2)
      pscore += 2 * outside_passed[(unsigned) TotalBlackPieces];
    else if ((tree->pawn_score.outside & 1) &&
        (!(tree->pawn_score.protected & 2) || TotalWhitePieces))
      pscore += outside_passed[(unsigned) TotalBlackPieces];
    if (tree->pawn_score.outside & 8)
      pscore -= 2 * outside_passed[(unsigned) TotalWhitePieces];
    else if ((tree->pawn_score.outside & 4) &&
        (!(tree->pawn_score.protected & 1) || TotalBlackPieces))
      pscore -= outside_passed[(unsigned) TotalWhitePieces];
    score += pscore * passed_scale / 100;
  }
  if (TotalWhitePieces + TotalBlackPieces == 0) {
    int pscore = 0;
    int wfile = File(WhiteKingSQ);
    int bfile = File(BlackKingSQ);

    if (!(tree->pawn_score.protected & 2)) {
      if (wfile + wtm > bfile + 1) {
        if (wfile < FirstOne8Bit(tree->pawn_score.allb))
          pscore += won_kp_ending;
      } else if (wfile - wtm < bfile - 1) {
        if (wfile > LastOne8Bit(tree->pawn_score.allb))
          pscore += won_kp_ending;
      }
    }
    if (!(tree->pawn_score.protected & 1)) {
      if (bfile > wfile + wtm) {
        if (bfile < FirstOne8Bit(tree->pawn_score.allw))
          pscore += -won_kp_ending;
      } else if (bfile < wfile - wtm) {
        if (bfile > LastOne8Bit(tree->pawn_score.allw))
          pscore += -won_kp_ending;
      }
    }
    score += pscore * passed_scale / 100;
  } else {
    int pscore = 0;

    if (!TotalBlackPieces) {
      w_spread =
          file_spread[tree->pawn_score.passed_w | tree->pawn_score.
          candidates_w];
      if (w_spread > 1)
        pscore += (w_spread - 1) * split_passed;
    }
    if (!TotalWhitePieces) {
      b_spread =
          file_spread[tree->pawn_score.passed_b | tree->pawn_score.
          candidates_b];
      if (b_spread > 1)
        pscore -= (b_spread - 1) * split_passed;
    }
    score += pscore * passed_scale / 100;
  }
#ifdef DEBUGEV
  if (score != lastsc) {
    printf("score[passed pawns]=              %4d (%+d)\n", score,
        score - lastsc);
    printf("score[passed pawns] (flags) =     %4x\n", tree->pawn_score.outside);
    lastsc = score;
  }
#endif
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
  if (WhiteBishops) {
    if (WhiteBishops & mask_WBT) {
      if (WhiteBishops & SetMask(A7) && SetMask(B6) & BlackPawns)
        score -= bishop_trapped;
      else if (WhiteBishops & SetMask(B8) && SetMask(C7) & BlackPawns)
        score -= bishop_trapped;
      else if (WhiteBishops & SetMask(H7) && SetMask(G6) & BlackPawns)
        score -= bishop_trapped;
      else if (WhiteBishops & SetMask(G8) && SetMask(F7) & BlackPawns)
        score -= bishop_trapped;
    }
  }
  if (BlackBishops) {
    if (BlackBishops & mask_BBT) {
      if (BlackBishops & SetMask(A2) && SetMask(B3) & WhitePawns)
        score += bishop_trapped;
      else if (BlackBishops & SetMask(B1) && SetMask(C2) & WhitePawns)
        score += bishop_trapped;
      else if (BlackBishops & SetMask(H2) && SetMask(G3) & WhitePawns)
        score += bishop_trapped;
      else if (BlackBishops & SetMask(G1) && SetMask(F2) & WhitePawns)
        score += bishop_trapped;
    }
  }
/*
 **********************************************************************
 *                                                                    *
 *   call EvaluateDevelopment() to evaluate development.  note that   *
 *   we only do this when either side has not castled.                *
 *                                                                    *
 **********************************************************************
 */
  if (WhiteCastle(1))
    score += EvaluateDevelopmentW(tree, ply);
  if (BlackCastle(1))
    score += EvaluateDevelopmentB(tree, ply);
#ifdef DEBUGEV
  if (score != lastsc)
    printf("score[development]=               %4d (%+d)\n", score,
        score - lastsc);
  lastsc = score;
#endif
/*
 **********************************************************************
 *                                                                    *
 *   now evaluate king safety by analyzing the pawn shelter in front  *
 *   of the king to determine if there are holes, open files or other *
 *   weaknesses that will make an attack difficult to repel.  one     *
 *   special case is to detect the presence of a bishop that protects *
 *   squares left after the b/g pawn has been advanced, leaving holes *
 *   around the king, and pawns/bishops and queen threatening on g2   *
 *   and related squares.                                             *
 *                                                                    *
 **********************************************************************
 */
  tree->endgame = (TotalWhitePieces <= EG_MAT || TotalBlackPieces <= EG_MAT);
  tree->w_kingsq = WhiteKingSQ;
  tree->b_kingsq = BlackKingSQ;
  tree->w_safety = 0;
  tree->b_safety = 0;
  if (!tree->endgame)
    score += EvaluateKingSafety(tree, ply);
#ifdef DEBUGEV
  if (score != lastsc)
    printf("score[king safety]=               %4d (%+d)\n", score,
        score - lastsc);
  lastsc = score;
#endif
/*
 **********************************************************************
 *                                                                    *
 *  check to see if we can take a lazy exit and avoid the time-       *
 *  consuming part of the evaluation code.                            *
 *                                                                    *
 **********************************************************************
 */
  if (can_win == 3) {
    register const int tscore = (wtm) ? score : -score;

    if (tscore - shared->lazy_eval_cutoff >= beta)
      return (beta);
    if (tscore + shared->lazy_eval_cutoff <= alpha)
      return (alpha);
  }
  tscore = score;
  tree->evaluations++;
/*
 **********************************************************************
 *                                                                    *
 *  now evaluate the kings.  these tests are done dynamically, since  *
 *  they depend on various pieces that can't be hashed in the king    *
 *  safety defect counts kept in the pawn hash table.                 *
 *                                                                    *
 **********************************************************************
 */
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
  if (WhiteKingSQ < A2) {
    if (WhiteKingSQ > E1) {
      if (WhiteRooks & mask_kr_trapped_w[FILEH - WhiteKingSQ])
        score -= rook_trapped;
    } else if (WhiteKingSQ < D1) {
      if (WhiteRooks & mask_qr_trapped_w[WhiteKingSQ])
        score -= rook_trapped;
    }
  }
#ifdef DEBUGEV
  if (score != lastsc)
    printf("score[kings(white)]=              %4d (%+d)\n", score,
        score - lastsc);
  lastsc = score;
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
  if (BlackKingSQ > H7) {
    if (BlackKingSQ > E8) {
      if (BlackRooks & mask_kr_trapped_b[FILEH - File(BlackKingSQ)])
        score += rook_trapped;
    } else if (BlackKingSQ < D8) {
      if (BlackRooks & mask_qr_trapped_b[File(BlackKingSQ)])
        score += rook_trapped;
    }
  }
#ifdef DEBUGEV
  if (score != lastsc)
    printf("score[kings(black)]=              %4d (%+d)\n", score,
        score - lastsc);
  lastsc = score;
#endif

/*
 **********************************************************************
 *                                                                    *
 *   knight evaluation includes centralization and "outposts".        *
 *                                                                    *
 **********************************************************************
 */
/*
 ************************************************************
 *                                                          *
 *   white knights.                                         *
 *                                                          *
 *   first, evaluate for "outposts" which is a knight that  *
 *   can't be driven off by an enemy pawn, and which is     *
 *   supported by a friendly pawn.  an outpost is much      *
 *   stronger if it is supported by two pawns (capturing it *
 *   results in a protected passed pawn) or if the opponent *
 *   has no minor piece that can attack it.                 *
 *                                                          *
 ************************************************************
 */
  temp = WhiteKnights;
  while (temp) {
    square = FirstOne(temp);
    if (white_outpost[square] && !(mask_no_pattacks_b[square] & BlackPawns))
      score += white_outpost[square];
    if ((square == D6 || square == E6) && PcOnSq(square + 8) == -pawn)
      score += blocked_center_pawn;
/*
 ************************************************************
 *                                                          *
 *   now fold in centralization score from the piece/square *
 *   table "nval_*".                                        *
 *                                                          *
 ************************************************************
 */
    score += nval_w[square];
/*
 ************************************************************
 *                                                          *
 *   adjust the white tropism count for this piece.         *
 *                                                          *
 ************************************************************
 */
    w_tropism += king_tropism_n[Distance(square, tree->b_kingsq)];
    Clear(square, temp);
  }
#ifdef DEBUGEV
  if (score != lastsc)
    printf("score[knights(white)]=            %4d (%+d)\n", score,
        score - lastsc);
  lastsc = score;
#endif
/*
 ************************************************************
 *                                                          *
 *   black knights.                                         *
 *                                                          *
 *   first, evaluate for "outposts" which is a knight that  *
 *   can't be driven off by an enemy pawn, and which is     *
 *   supported by a friendly pawn.  an outpost is much      *
 *   stronger if it is supported by two pawns (capturing it *
 *   results in a protected passed pawn) or if the opponent *
 *   has no minor piece that can attack it.                 *
 *                                                          *
 ************************************************************
 */
  temp = BlackKnights;
  while (temp) {
    square = FirstOne(temp);
    if (black_outpost[square] && !(mask_no_pattacks_w[square] & WhitePawns))
      score -= black_outpost[square];
    if ((square == D3 || square == E3) && PcOnSq(square - 8) == pawn)
      score -= blocked_center_pawn;
/*
 ************************************************************
 *                                                          *
 *   now fold in centralization score from the piece/square *
 *   table "nval_*".                                        *
 *                                                          *
 ************************************************************
 */
    score -= nval_b[square];
/*
 ************************************************************
 *                                                          *
 *   adjust the black tropism count for this piece.         *
 *                                                          *
 ************************************************************
 */
    b_tropism += king_tropism_n[Distance(square, tree->w_kingsq)];
    Clear(square, temp);
  }
#ifdef DEBUGEV
  if (score != lastsc)
    printf("score[knights(black)]=            %4d (%+d)\n", score,
        score - lastsc);
  lastsc = score;
#endif
/*
 **********************************************************************
 *                                                                    *
 *   bishop evaluation includes mobility, centralization as well as   *
 *   a bonus for having the bishop pair.  a special case is a bishop  *
 *   that is fianchettoed in front of a castled king.  this bishop is *
 *   defending very weak squares and is therefore more valuable and   *
 *   should not be traded.                                            *
 *                                                                    *
 **********************************************************************
 */
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
    score += bval_w[square];
/*
 ************************************************************
 *                                                          *
 *   then fold in the mobility score.                       *
 *                                                          *
 ************************************************************
 */
    score += bishop_mobility * MobilityBishop(square);
/*
 ************************************************************
 *                                                          *
 *   now add in a bonus for a bishop blocking a center pawn *
 *   at D6/E6 as that is very cramping.                     *
 *                                                          *
 ************************************************************
 */
    if (white_outpost[square] && !(mask_no_pattacks_b[square] & BlackPawns)) {
      if ((square == D6 || square == E6) && PcOnSq(square + 8) == -pawn)
        score += blocked_center_pawn;
    }
/*
 ************************************************************
 *                                                          *
 *   adjust the white tropism count for this piece.         *
 *                                                          *
 ************************************************************
 */
    w_tropism += king_tropism_b[Distance(square, tree->b_kingsq)];
    Clear(square, temp);
  }
#ifdef DEBUGEV
  if (score != lastsc)
    printf("score[bishops(white)]=            %4d (%+d)\n", score,
        score - lastsc);
  lastsc = score;
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
    score -= bval_b[square];
/*
 ************************************************************
 *                                                          *
 *   then fold in the mobility score.                       *
 *                                                          *
 ************************************************************
 */
    score -= bishop_mobility * MobilityBishop(square);
/*
 ************************************************************
 *                                                          *
 *   now add in a bonus for a bishop blocking a center pawn *
 *   at D3/E3 as that is very cramping.                     *
 *                                                          *
 ************************************************************
 */
    if (black_outpost[square] && !(mask_no_pattacks_w[square] & WhitePawns)) {
      if ((square == D3 || square == E3) && PcOnSq(square - 8) == pawn)
        score -= blocked_center_pawn;
    }
/*
 ************************************************************
 *                                                          *
 *   adjust the black tropism count for this piece.         *
 *                                                          *
 ************************************************************
 */
    b_tropism += king_tropism_b[Distance(square, tree->w_kingsq)];
    Clear(square, temp);
  }
#ifdef DEBUGEV
  if (score != lastsc)
    printf("score[bishops(black)]=            %4d (%+d)\n", score,
        score - lastsc);
  lastsc = score;
#endif
/*
 ************************************************************
 *                                                          *
 *   now, give either side a bonus for having two bishops.  *
 *   and penalize pawns on the same color as the bishop, if *
 *   one side has only one bishop.                          *
 *                                                          *
 *   also add in a bonus for a bishop, if there are pawns   *
 *   on both sides of the board in an endgame, because a    *
 *   bishop is much more valuable there.                    *
 *                                                          *
 *   add in a bonus for filling holes when the king has     *
 *   castled and played b3/g3/b6/g6.                        *
 *                                                          *
 ************************************************************
 */
  if (WhiteBishops) {
    if (WhiteBishops & (WhiteBishops - 1))
      score += bishop_pair[TotalWhitePawns];
    else {
      if (WhiteBishops & light_squares)
        score -=
            PopCnt(WhitePawns & light_squares) * bishop_plus_pawns_on_color;
      else
        score -= PopCnt(WhitePawns & dark_squares) * bishop_plus_pawns_on_color;
      if (TotalWhitePieces < 7 && !BlackBishops && tree->all_pawns & mask_fgh &&
          tree->all_pawns & mask_abc)
        score += bishop_over_knight_endgame;
    }
    if (!tree->endgame) {
      if (File(tree->w_kingsq) > FILEE) {
        if (!(WhitePawns & SetMask(G2)) && WhitePawns & SetMask(G3) &&
            Distance(tree->w_kingsq, G2) == 1 && WhiteBishops & good_bishop_kw)
          score += bishop_king_safety;
      } else if (File(tree->w_kingsq) < FILED) {
        if (!(WhitePawns & SetMask(B2)) && WhitePawns & SetMask(B3) &&
            Distance(tree->w_kingsq, B2) == 1 && WhiteBishops & good_bishop_qw)
          score += bishop_king_safety;
      }
    }
  }
  if (BlackBishops) {
    if (BlackBishops & (BlackBishops - 1)) {
      score -= bishop_pair[TotalBlackPawns];
    } else {
      if (BlackBishops & light_squares)
        score +=
            PopCnt(BlackPawns & light_squares) * bishop_plus_pawns_on_color;
      else
        score += PopCnt(BlackPawns & dark_squares) * bishop_plus_pawns_on_color;
      if (TotalBlackPieces < 7 && !WhiteBishops && tree->all_pawns & mask_fgh &&
          tree->all_pawns & mask_abc)
        score -= bishop_over_knight_endgame;
    }
    if (!tree->endgame) {
      if (File(tree->b_kingsq) > FILEE) {
        if (!(BlackPawns & SetMask(G7)) && BlackPawns & SetMask(G6) &&
            Distance(tree->b_kingsq, G7) == 1 && BlackBishops & good_bishop_kb)
          score -= bishop_king_safety;
      } else if (File(tree->b_kingsq) < FILED) {
        if (!(BlackPawns & SetMask(B7)) && BlackPawns & SetMask(B6) &&
            Distance(tree->b_kingsq, B7) == 1 && BlackBishops & good_bishop_qb)
          score -= bishop_king_safety;
      }
    }
  }
#ifdef DEBUGEV
  if (score != lastsc)
    printf("score[bishop pair]=               %4d (%+d)\n", score,
        score - lastsc);
  lastsc = score;
#endif
/*
 **********************************************************************
 *                                                                    *
 *   rook evaluation includes several simple cases, including open    *
 *   files, 7th rank, connected, etc.                                 *
 *                                                                    *
 **********************************************************************
 */
/*
 ************************************************************
 *                                                          *
 *   white rooks                                            *
 *                                                          *
 ************************************************************
 */
  temp = WhiteRooks;
  while (temp) {
    register int mobility;

    square = FirstOne(temp);
    file = File(square);
    score += rval_w[square];
/*
 ************************************************************
 *                                                          *
 *   determine if the rook is on an open file.  if it is,   *
 *   determine if this rook attacks another friendly rook,  *
 *   making it difficult to drive the rooks off the file.   *
 *                                                          *
 ************************************************************
 */
    trop = 7;
    if (!(file_mask[file] & tree->all_pawns)) {
      score += rook_open_file[file] * MobilityFile(square);
      trop = FileDistance(square, tree->b_kingsq);
    } else {
      if (tree->pawn_score.open_files) {
        unsigned char rankmvs = AttacksRank(square) >> (56 - (square & 0x38));

        if (!(rankmvs & tree->pawn_score.open_files))
          score -=
              (rook_open_file[FILED] >> 1) * MobilityFile((square & 0x38) +
              FILED);
      }
      if (!(file_mask[file] & WhitePawns)) {
        score += rook_half_open_file[file] * MobilityFile(square);
        trop = FileDistance(square, tree->b_kingsq);
      } else if (!(plus8dir[square] & WhitePawns)) {
        trop = FileDistance(square, tree->b_kingsq);
      }
    }
/*
 ************************************************************
 *                                                          *
 *   see if the rook can move horizontally.  if not, it is  *
 *   somewhat precarious in how it is situated.             *
 *                                                          *
 ************************************************************
 */
    mobility = 0;
    if (File(square) < FILEH && PcOnSq(square + 1) <= 0)
      mobility++;
    if (File(square) > FILEA && PcOnSq(square - 1) <= 0)
      mobility++;
    if (!mobility)
      score -= rook_limited;
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
      if (tree->pawn_score.passed_w && BlackKingSQ > H7 &&
          !(BlackPieces & mask_abs7_w))
        score += rook_absolute_7th;
      if (AttacksRank(square) & (WhiteRooks | WhiteQueens))
        score += rook_connected_7th_rank;
    }
/*
 ************************************************************
 *                                                          *
 *   adjust the white tropism count for this piece.         *
 *                                                          *
 ************************************************************
 */
    w_tropism +=
        Max(king_tropism_at_r[trop], king_tropism_r[Distance(square,
                tree->b_kingsq)]);
    Clear(square, temp);
  }
#ifdef DEBUGEV
  if (score != lastsc)
    printf("score[rooks(white)]=              %4d (%+d)\n", score,
        score - lastsc);
  lastsc = score;
#endif
/*
 ************************************************************
 *                                                          *
 *   black rooks                                            *
 *                                                          *
 ************************************************************
 */
  temp = BlackRooks;
  while (temp) {
    register int mobility;

    square = FirstOne(temp);
    file = File(square);
    score -= rval_b[square];
/*
 ************************************************************
 *                                                          *
 *   determine if the rook is on an open file.  if it is,   *
 *   determine if this rook attacks another friendly rook,  *
 *   making it difficult to drive the rooks off the file.   *
 *                                                          *
 ************************************************************
 */
    trop = 7;
    if (!(file_mask[file] & tree->all_pawns)) {
      score -= rook_open_file[file] * MobilityFile(square);
      trop = FileDistance(square, tree->w_kingsq);
    } else {
      if (tree->pawn_score.open_files) {
        unsigned char rankmvs = AttacksRank(square) >> (56 - (square & 0x38));

        if (!(rankmvs & tree->pawn_score.open_files))
          score +=
              (rook_open_file[FILED] >> 1) * MobilityFile((square & 0x38) +
              FILED);
      }
      if (!(file_mask[file] & BlackPawns)) {
        score -= rook_half_open_file[file] * MobilityFile(square);
        trop = FileDistance(square, tree->w_kingsq);
      } else if (!(minus8dir[square] & BlackPawns)) {
        trop = FileDistance(square, tree->w_kingsq);
      }
    }
/*
 ************************************************************
 *                                                          *
 *   see if the rook can move horizontally.  if not, it is  *
 *   somewhat precarious in how it is situated.             *
 *                                                          *
 ************************************************************
 */
    mobility = 0;
    if (File(square) < FILEH && PcOnSq(square + 1) >= 0)
      mobility++;
    if (File(square) > FILEA && PcOnSq(square - 1) >= 0)
      mobility++;
    if (!mobility)
      score += rook_limited;
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
      if (tree->pawn_score.passed_b && WhiteKingSQ < A2 &&
          !(WhitePieces & mask_abs7_b))
        score -= rook_absolute_7th;
      if (AttacksRank(square) & (BlackRooks | BlackQueens))
        score -= rook_connected_7th_rank;
    }
/*
 ************************************************************
 *                                                          *
 *   adjust the black tropism count for this piece.         *
 *                                                          *
 ************************************************************
 */
    b_tropism +=
        Max(king_tropism_at_r[trop], king_tropism_r[Distance(square,
                tree->w_kingsq)]);
    Clear(square, temp);
  }
#ifdef DEBUGEV
  if (score != lastsc)
    printf("score[rooks(black)]=              %4d (%+d)\n", score,
        score - lastsc);
  lastsc = score;
#endif
/*
 **********************************************************************
 *                                                                    *
 *   queen evaluation includes centralization, plus some bonuses that *
 *   affect the opponent's king safety.                               *
 *                                                                    *
 **********************************************************************
 */
/*
 ************************************************************
 *                                                          *
 *   white queens                                           *
 *                                                          *
 *   first locate each queen and obtain it's centralization *
 *   score from the static piece/square table for queens.   *
 *                                                          *
 *   then, if the opposing side's king safety is much worse *
 *   than the king safety for this side, add in a bonus to  *
 *   keep the queen around.                                 *
 *                                                          *
 ************************************************************
 */
  temp = WhiteQueens;
  while (temp) {
    square = FirstOne(temp);
    score += qval_w[square];
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
 *   adjust the white tropism count for this piece.         *
 *                                                          *
 ************************************************************
 */
    trop = 7;
    if (!(WhitePawns & plus8dir[square]))
      trop = FileDistance(square, tree->b_kingsq);
    w_tropism +=
        Max(king_tropism_q[Distance(square, tree->b_kingsq)],
        king_tropism_at_q[trop]);
/*
 ************************************************************
 *                                                          *
 *   add in a bonus if the opponent's king position is not  *
 *   safe, since the queen is a strong attacking piece.     *
 *                                                          *
 ************************************************************
 */
    if (tree->b_safety > tree->w_safety + 4)
      score += queen_is_strong;
/*
 ************************************************************
 *                                                          *
 *   add in a penalty for being on the wrong side of the    *
 *   board, which can lead to attacks by the opponent.      *
 *                                                          *
 ************************************************************
 */
    if (TotalWhitePawns > 4) {
      int offside = (File(square) < FILEC && File(tree->b_kingsq) > FILEE)
          || (File(square) > FILEF && File(tree->b_kingsq) < FILED);

      if (offside) {
        w_tropism -= queen_offside_tropism;
        w_tropism = Max(w_tropism, -8);
      }
    }
    Clear(square, temp);
  }
#ifdef DEBUGEV
  if (score != lastsc)
    printf("score[queens(white)]=             %4d (%+d)\n", score,
        score - lastsc);
  lastsc = score;
#endif
/*
 ************************************************************
 *                                                          *
 *   black queens                                           *
 *                                                          *
 *   first locate each queen and obtain it's centralization *
 *   score from the static piece/square table for queens.   *
 *                                                          *
 *   then, if the opposing side's king safety is much worse *
 *   than the king safety for this side, add in a bonus to  *
 *   keep the queen around.                                 *
 *                                                          *
 ************************************************************
 */
  temp = BlackQueens;
  while (temp) {
    square = FirstOne(temp);
    score -= qval_b[square];
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
 *   adjust the black tropism count for this piece.         *
 *                                                          *
 ************************************************************
 */
    trop = 7;
    if (!(BlackPawns & minus8dir[square]))
      trop = FileDistance(square, tree->w_kingsq);
    b_tropism +=
        Max(king_tropism_q[Distance(square, tree->w_kingsq)],
        king_tropism_at_q[trop]);
/*
 ************************************************************
 *                                                          *
 *   add in a bonus if the opponent's king position is not  *
 *   safe, since the queen is a strong attacking piece.     *
 *                                                          *
 ************************************************************
 */
    if (tree->w_safety > tree->b_safety + 4)
      score -= queen_is_strong;
/*
 ************************************************************
 *                                                          *
 *   add in a penalty for being on the wrong side of the    *
 *   board, which can lead to attacks by the opponent.      *
 *                                                          *
 ************************************************************
 */
    if (TotalBlackPawns > 4) {
      int offside = (File(square) < FILEC && File(tree->w_kingsq) > FILEE)
          || (File(square) > FILEF && File(tree->w_kingsq) < FILED);

      if (offside) {
        b_tropism -= queen_offside_tropism;
        b_tropism = Max(b_tropism, -8);
      }
    }
    Clear(square, temp);
  }
#ifdef DEBUGEV
  if (score != lastsc)
    printf("score[queens(black)]=             %4d (%+d)\n", score,
        score - lastsc);
  lastsc = score;
#endif
/*
 ************************************************************
 *                                                          *
 *   now fold in the king tropism score, which take into    *
 *   account _all_ pieces for each side that are close to   *
 *   the opponent's king. the calculation uses three values *
 *   for each side.  the 'total tropism' number, multiplied *
 *   by the opponent's king safety, but this is first run   *
 *   through an indirect table probe to get a multiplier    *
 *   value.  the ttemper[] array provides a number between  *
 *   16 and N, where 16 says use all the tropism numbers,   *
 *   32 says to double the tropism scores (ie this array is *
 *   specified in units of 1/16th).                         *
 *                                                          *
 ************************************************************
 */
  if (!tree->endgame)
    score +=
        ((tropism[w_tropism + 8] * ttemper[tree->b_safety]) >> 4) -
        ((tropism[b_tropism + 8] * ttemper[tree->w_safety]) >> 4);
#ifdef DEBUGEV
  if (score != lastsc) {
    printf("score[king tropism]=              %4d (%+d)\n", score,
        score - lastsc);
    printf("w_safety=%d  b_safety=%d\n", tree->w_safety, tree->b_safety);
    printf("w_tropism=%d  b_tropism=%d\n", w_tropism, b_tropism);
    printf("wtrop.score=%d\n",
        (tropism[w_tropism + 8] * ttemper[tree->b_safety]) >> 4);
    printf("btrop.score=%d\n",
        (tropism[b_tropism + 8] * ttemper[tree->w_safety]) >> 4);
  }
  lastsc = score;
#endif
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
  if (TotalWhitePieces <= 11 && TotalBlackPieces <= 11) {
    if (PopCnt(WhiteBishops) == 1 && PopCnt(BlackBishops) == 1) {
      if (square_color[FirstOne(BlackBishops)] !=
          square_color[FirstOne(WhiteBishops)]) {
        if (TotalWhitePieces == 3 && TotalBlackPieces == 3 &&
            ((TotalWhitePawns < 4 && TotalBlackPawns < 4) ||
                abs(TotalWhitePawns - TotalBlackPawns) < 2))
          score = score >> 1;
        else if (TotalWhitePieces == TotalBlackPieces)
          score = ((score - Material) >> 1) + Material;
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
 *   pushing a pawn or capturing something will cause the   *
 *   go back to its correct state a bit more smoothly.      *
 *                                                          *
 ************************************************************
 */
  if (Rule50Moves(ply) > 80) {
    int scale = 101 - Rule50Moves(ply);

    score = DrawScore(1) + score * scale / 20;
  }
#if defined(DETECTDRAW)
  if (drawing == 1)
    score /= 4;
  else if (drawing == 3)
    score *= 2;
#endif
#ifdef DEBUGEV
  if (score != lastsc)
    printf("score[draws]=                     %4d (%+d)\n", score,
        score - lastsc);
  lastsc = score;
#endif
  if (abs(score - tscore) > shared->lazy_eval_cutoff)
    shared->lazy_eval_cutoff = abs(score - tscore);
  if (score > Material) {
    if (score - Material > shared->largest_positional_score)
      shared->largest_positional_score = score - Material;
  } else {
    if (Material - score > shared->largest_positional_score)
      shared->largest_positional_score = Material - score;
  }
  return ((wtm) ? score : -score);
}

/* last modified 08/07/05 */
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
  register int possible, real, score = 0;

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
  if (Occupied & ((BlackPawns & rank_mask[RANK7]) << 8) & (file_mask[FILED] |
          file_mask[FILEE]))
    score += blocked_center_pawn;
#ifdef DEBUGDV
  printf("developmentB.1 score=%d\n", score);
#endif
/*
 ************************************************************
 *                                                          *
 *   if all minor pieces aren't developed, then penalize    *
 *   the queen if it has moved.                             *
 *                                                          *
 ************************************************************
 */
  if (BlackQueens && BlackCastle(ply) > 0 && !(BlackQueens & SetMask(D8)))
    score += development_queen_early;
#ifdef DEBUGDV
  printf("developmentB.2 score=%d\n", score);
#endif
/*
 ************************************************************
 *                                                          *
 *   if the king hasn't moved at the beginning of the       *
 *   search, but it has moved somewhere in the current      *
 *   search path, make *sure* it's a castle move or else    *
 *   penalize the loss of castling privilege.               *
 *                                                          *
 *   if it *is* a castle move, penalize the king if it is   *
 *   castling to one side when the other side is much safer *
 *   but it will take longer to prepare to castle in that   *
 *   safer direction.                                       *
 *                                                          *
 *   the final test is to see if castling rights have been  *
 *   lost due to moving one rook.  if so, check to see if   *
 *   the remaining castle right would put the king in an    *
 *   unsafe position which is just as bad.                  *
 *                                                          *
 ************************************************************
 */
  if (BlackCastle(1) == 3) {
    possible = 0;
    real = 0;
    if (BlackCastle(ply) < 0) {
      if (File(BlackKingSQ) >= FILEE) {
        real = tree->pawn_score.black_defects_k;
        possible = tree->pawn_score.black_defects_q;
      } else {
        real = tree->pawn_score.black_defects_q;
        possible = tree->pawn_score.black_defects_k;
      }
    } else if (BlackCastle(ply) > 0 && BlackCastle(ply) != 3) {
      if (BlackCastle(ply) & 1) {
        real = tree->pawn_score.black_defects_k;
        possible = tree->pawn_score.black_defects_q;
      } else {
        real = tree->pawn_score.black_defects_q;
        possible = tree->pawn_score.black_defects_k;
      }
    }
    if (possible < real)
      score += temper_b[2 * (real - possible)];
  }
  if (BlackCastle(1) > 0) {
    if (BlackCastle(ply) == 0) {
      register int wq;

      wq = (WhiteQueens) ? 2 : 1;
      score +=
          (Flip(shared->root_wtm)) ? 2 * wq * development_losing_castle : wq *
          development_losing_castle;
    } else if (BlackCastle(ply) < 0) {
      int cmove = ((-BlackCastle(ply)) >> 2) + 1;

      score -= development_castle_bonus / cmove;
    }
    if (BlackCastle(ply) > 0)
      score += development_not_castled;
  }
#ifdef DEBUGDV
  printf("developmentB.3 score=%d\n", score);
#endif
  return (score);
}

/* last modified 08/07/05 */
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
  register int possible, real, score = 0;

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
  if (Occupied & ((WhitePawns & rank_mask[RANK2]) >> 8) & (file_mask[FILED] |
          file_mask[FILEE]))
    score -= blocked_center_pawn;
#ifdef DEBUGDV
  printf("developmentW.1 score=%d\n", score);
#endif
/*
 ************************************************************
 *                                                          *
 *   if all minor pieces aren't developed, then penalize    *
 *   the queen if it has moved.                             *
 *                                                          *
 ************************************************************
 */
  if (WhiteQueens && WhiteCastle(ply) > 0 && !(WhiteQueens & SetMask(D1)))
    score -= development_queen_early;
#ifdef DEBUGDV
  printf("developmentW.2 score=%d\n", score);
#endif
/*
 ************************************************************
 *                                                          *
 *   if the king hasn't moved at the beginning of the       *
 *   search, but it has moved somewhere in the current      *
 *   search path, make *sure* it's a castle move or else    *
 *   penalize the loss of castling privilege.               *
 *                                                          *
 *   if it *is* a castle move, penalize the king if it is   *
 *   castling to one side when the other side is much safer *
 *   but it will take longer to prepare to castle in that   *
 *   safer direction.                                       *
 *                                                          *
 *   the final test is to see if castling rights have been  *
 *   lost due to moving one rook.  if so, check to see if   *
 *   the remaining castle right would put the king in an    *
 *   unsafe position which is just as bad.                  *
 *                                                          *
 ************************************************************
 */
  if (WhiteCastle(1) == 3) {
    possible = 0;
    real = 0;
    if (WhiteCastle(ply) < 0) {
      if (File(WhiteKingSQ) >= FILEE) {
        real = tree->pawn_score.white_defects_k;
        possible = tree->pawn_score.white_defects_q;
      } else {
        real = tree->pawn_score.white_defects_q;
        possible = tree->pawn_score.white_defects_k;
      }
    } else if (WhiteCastle(ply) > 0 && WhiteCastle(ply) != 3) {
      if (WhiteCastle(ply) & 1) {
        real = tree->pawn_score.white_defects_k;
        possible = tree->pawn_score.white_defects_q;
      } else {
        real = tree->pawn_score.white_defects_q;
        possible = tree->pawn_score.white_defects_k;
      }
    }
    if (possible < real)
      score -= temper_w[2 * (real - possible)];
  }
  if (WhiteCastle(1) > 0) {
    register int bq;

    if (WhiteCastle(ply) == 0) {
      bq = (BlackQueens) ? 2 : 1;
      score -=
          (shared->root_wtm) ? 2 * bq * development_losing_castle : bq *
          development_losing_castle;
    } else if (WhiteCastle(ply) < 0) {
      int cmove = ((-WhiteCastle(ply)) >> 2) + 1;

      score += development_castle_bonus / cmove;
    }
    if (WhiteCastle(ply) > 0)
      score -= development_not_castled;
  }
#ifdef DEBUGDV
  printf("developmentW.3 score=%d\n", score);
#endif
  return (score);
}

/* last modified 08/07/05 */
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
  if (!mate_score) {
/*
 ************************************************************
 *                                                          *
 *   if white is winning, force the black king to the edge  *
 *   of the board.                                          *
 *                                                          *
 ************************************************************
 */
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

/* last modified 08/07/05 */
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
 *   check 1.  if one side is a whole piece ahead, this is good, as   *
 *   trading a piece for 3 pawns is generally bad, unless the pawns   *
 *   are so far advanced that the search can see them causing         *
 *   problems already.                                                *
 *                                                                    *
 *   check 2.  if one side has a rook more than the other side, then  *
 *   determine if the other side has two extra pieces.  this is a bad *
 *   idea, trading two pieces for (say) a rook + pawn.                *
 *                                                                    *
 *   check 3.  if one side has two rooks or a queen vs three pieces   *
 *   then the three pieces are often deadly, so that this is also a   *
 *   bad trade.                                                       *
 *                                                                    *
 **********************************************************************
 */
  if (TotalWhitePieces + TotalWhitePawns > 3 ||
      TotalBlackPieces + TotalBlackPawns > 3) {
    if (WhiteMinors != BlackMinors) {
      if (WhiteMajors == BlackMajors) {
        if (WhiteMinors > BlackMinors)
          score += bad_trade;
        else
          score -= bad_trade;
      } else if (BlackMajors - 1 == WhiteMajors) {
        if (WhiteMinors > BlackMinors + 1)
          score += bad_trade;
        else if (WhiteMinors > BlackMinors)
          score -= bad_trade >> 2;
      } else if (WhiteMajors - 1 == BlackMajors) {
        if (BlackMinors > WhiteMinors + 1)
          score -= bad_trade;
        else if (BlackMinors > WhiteMinors)
          score += bad_trade >> 2;
      } else if (abs(WhiteMajors - BlackMajors) == 2) {
        if (WhiteMinors > BlackMinors + 2)
          score += bad_trade;
        else if (BlackMinors > WhiteMinors + 2)
          score -= bad_trade;
      }
    } else if (WhiteMajors == BlackMajors) {
      if (WhiteQueens && !BlackQueens)
        score += bad_trade >> 1;
      else if (!WhiteQueens && BlackQueens)
        score -= bad_trade >> 1;
    } else {
      if (WhiteMajors > BlackMajors)
        score += bad_trade;
      else
        score -= bad_trade;
    }
  }
#ifdef DEBUGM
  printf("score[material]=                  %4d\n", Material);
  printf("score[bad trade]=                 %4d\n", score);
#endif
  return (score);
}

/* last modified 08/07/05 */
/*
 *******************************************************************************
 *                                                                             *
 *   EvaluateKingSafety() is used to evaluate king safety for both sides, based*
 *   on the pawns around the king and the material left on the board.          *
 *                                                                             *
 *******************************************************************************
 */
int EvaluateKingSafety(TREE * RESTRICT tree, int ply)
{
  int score = 0;

/*
 ************************************************************
 *                                                          *
 *   first, check for the "trojan horse" attack where the   *
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
 *   Now do normal scoring, if the king has castled, the    *
 *   pawns in front are important.  If not castled yet, the *
 *   pawns on the kingside should be preserved for this.    *
 *                                                          *
 ************************************************************
 */
  tree->w_safety = king_defects_w[WhiteKingSQ];
  if (WhiteCastle(ply) <= 0) {
    if (File(WhiteKingSQ) >= FILEE) {
      if (File(WhiteKingSQ) == FILEH)
        tree->w_kingsq &= 62;
      tree->w_safety += tree->pawn_score.white_defects_k;
      if (shared->root_wtm && shared->use_asymmetry) {
        if (File(BlackKingSQ) <= FILED)
          tree->w_safety += castle_opposite;
      }
      if (!(WhitePawns & SetMask(G2))) {
        if (SetMask(F3) & (BlackPawns | BlackBishops) && BlackQueens)
          tree->w_safety += king_safety_mate_g2g7;
      }
      if (File(WhiteKingSQ) == FILEE) {
        if (!(tree->all_pawns & file_mask[FILED]))
          tree->w_safety += king_safety_open_file >> 1;
        if (!(tree->all_pawns & file_mask[FILEE]))
          tree->w_safety += king_safety_open_file;
        if (!(tree->all_pawns & file_mask[FILEF]))
          tree->w_safety += king_safety_open_file >> 1;
      }
    } else if (File(WhiteKingSQ) <= FILED) {
      if (File(WhiteKingSQ) == FILEA)
        tree->w_kingsq |= 1;
      tree->w_safety += tree->pawn_score.white_defects_q;
      if (shared->root_wtm && shared->use_asymmetry) {
        if (File(BlackKingSQ) >= FILEE)
          tree->w_safety += castle_opposite;
      }
      if (!(WhitePawns & SetMask(B2))) {
        if (SetMask(C3) & (BlackPawns | BlackBishops) && BlackQueens)
          tree->w_safety += king_safety_mate_g2g7;
      }
      if (File(WhiteKingSQ) == FILED) {
        if (!(tree->all_pawns & file_mask[FILEC]))
          tree->w_safety += king_safety_open_file >> 1;
        if (!(tree->all_pawns & file_mask[FILED]))
          tree->w_safety += king_safety_open_file;
        if (!(tree->all_pawns & file_mask[FILEE]))
          tree->w_safety += king_safety_open_file >> 1;
      }
    }
  } else {
    if (WhiteCastle(ply) != 3) {
      if (WhiteCastle(ply) & 1)
        tree->w_safety += tree->pawn_score.white_defects_k >> 1;
      else if (WhiteCastle(ply) & 2)
        tree->w_safety += tree->pawn_score.white_defects_q >> 1;
    }
    if (!(tree->all_pawns & file_mask[FILED]))
      tree->w_safety += king_safety_open_file >> 1;
    if (!(tree->all_pawns & file_mask[FILEE]))
      tree->w_safety += king_safety_open_file;
    if (!(tree->all_pawns & file_mask[FILEF]))
      tree->w_safety += king_safety_open_file >> 1;
  }

  tree->b_safety = king_defects_b[BlackKingSQ];
  if (BlackCastle(ply) <= 0) {
    if (File(BlackKingSQ) >= FILEE) {
      if (File(BlackKingSQ) == FILEH)
        tree->b_kingsq &= 62;
      tree->b_safety += tree->pawn_score.black_defects_k;
      if (Flip(shared->root_wtm) && shared->use_asymmetry) {
        if (File(WhiteKingSQ) <= FILED)
          tree->b_safety += castle_opposite;
      }
      if (!(BlackPawns & SetMask(G7))) {
        if (SetMask(F6) & (WhitePawns | WhiteBishops) && WhiteQueens)
          tree->b_safety += king_safety_mate_g2g7;
      }
      if (File(BlackKingSQ) == FILEE) {
        if (!(tree->all_pawns & file_mask[FILED]))
          tree->b_safety += king_safety_open_file >> 1;
        if (!(tree->all_pawns & file_mask[FILEE]))
          tree->b_safety += king_safety_open_file;
        if (!(tree->all_pawns & file_mask[FILEF]))
          tree->b_safety += king_safety_open_file >> 1;
      }
    } else if (File(BlackKingSQ) <= FILED) {
      if (File(BlackKingSQ) == FILEA)
        tree->b_kingsq |= 1;
      tree->b_safety += tree->pawn_score.black_defects_q;
      if (Flip(shared->root_wtm) && shared->use_asymmetry) {
        if (File(WhiteKingSQ) >= FILEE)
          tree->b_safety += castle_opposite;
      }
      if (!(BlackPawns & SetMask(B7))) {
        if (SetMask(C6) & (WhitePawns | WhiteBishops) && WhiteQueens)
          tree->b_safety += king_safety_mate_g2g7;
      }
      if (File(BlackKingSQ) == FILED) {
        if (!(tree->all_pawns & file_mask[FILEC]))
          tree->b_safety += king_safety_open_file >> 1;
        if (!(tree->all_pawns & file_mask[FILED]))
          tree->b_safety += king_safety_open_file;
        if (!(tree->all_pawns & file_mask[FILEE]))
          tree->b_safety += king_safety_open_file >> 1;
      }
    }
  } else {
    if (BlackCastle(ply) != 3) {
      if (BlackCastle(ply) & 1)
        tree->b_safety += tree->pawn_score.black_defects_k >> 1;
      else if (BlackCastle(ply) & 2)
        tree->b_safety += tree->pawn_score.black_defects_q >> 1;
    }
    if (!(tree->all_pawns & file_mask[FILED]))
      tree->b_safety += king_safety_open_file >> 1;
    if (!(tree->all_pawns & file_mask[FILEE]))
      tree->b_safety += king_safety_open_file;
    if (!(tree->all_pawns & file_mask[FILEF]))
      tree->b_safety += king_safety_open_file >> 1;
  }
  if (!WhiteQueens)
    tree->b_safety >>= 1;
  if (!BlackQueens)
    tree->w_safety >>= 1;
  score -= temper_w[tree->w_safety] - temper_b[tree->b_safety];
  return (score);
}

/* last modified 08/07/05 */
/*
 *******************************************************************************
 *                                                                             *
 *   EvaluatePassedPawns() is used to evaluate passed pawns and the danger     *
 *   they produce.  the first bonus is for a passed pawn that has reached the  *
 *   6th rank and is supported by the king, making it very difficult to stop it*
 *   from queening.  the second case is two connected passed pawns on the 6th  *
 *   or 7th rank, with the opposing side having little material to stop them.  *
 *                                                                             *
 *******************************************************************************
 */
int EvaluatePassedPawns(TREE * RESTRICT tree)
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
      if (FileDistance(square, black_king_sq) == 1 &&
          Rank(black_king_sq) <= Rank(square))
        score -= supported_passer[RANK8 - Rank(square)];
      if (SetMask(square - 8) & Occupied) {
        score += blockading_passed_pawn_value[RANK8 - Rank(square)];
      }
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
          connected_passed_pawn_value[7 - Max(Rank(square1), Rank(square2))];
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
      if (FileDistance(square, white_king_sq) == 1 &&
          Rank(white_king_sq) >= Rank(square))
        score += supported_passer[Rank(square)];
      if (SetMask(square + 8) & Occupied) {
        score -= blockading_passed_pawn_value[Rank(square)];
      }
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
  return (score);
}

/* last modified 08/07/05 */
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
  if (WhitePawns && !BlackPawns && !TotalWhitePieces && !TotalBlackPieces)
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
    } while (0);
/*
 ************************************************************
 *                                                          *
 *   check to see if black has one pawn and neither side    *
 *   has any pieces.  if so, use the simple pawn evaluation *
 *   logic.                                                 *
 *                                                          *
 ************************************************************
 */
  if (BlackPawns && !WhitePawns && !TotalWhitePieces && !TotalBlackPieces)
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
    } while (0);
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
  if (!TotalWhitePieces && tree->pawn_score.passed_b) {
    passed = tree->pawn_score.passed_b;
    while ((file = FirstOne8Bit(passed)) != 8) {
      passed &= ~(128 >> file);
      square = FirstOne(BlackPawns & file_mask[file]);
      if ((wtm && !(black_pawn_race_wtm[square] & WhiteKing)) || (Flip(wtm) &&
              !(black_pawn_race_btm[square] & WhiteKing))) {
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
  if (!TotalBlackPieces && tree->pawn_score.passed_w) {
    passed = tree->pawn_score.passed_w;
    while ((file = FirstOne8Bit(passed)) != 8) {
      passed &= ~(128 >> file);
      square = LastOne(WhitePawns & file_mask[file]);
      if ((wtm && !(white_pawn_race_wtm[square] & BlackKing)) || (Flip(wtm) &&
              !(white_pawn_race_btm[square] & BlackKing))) {
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
            white_square = RANK8 + File(left);
            white_pawn = left;
          }
        }
        if (!(white_pawn_race_wtm[right] & SetMask(left))) {
          queen_distance = RANK8 - Rank(right);
          if (Rank(right) == RANK2)
            queen_distance--;
          if (queen_distance < white_queener) {
            white_queener = queen_distance;
            white_square = RANK8 + File(right);
            white_pawn = right;
          }
        }
      }
    }
  }
  if (realw && !realb) {
    black_queener = 8;
    black_square = 0;
  }
  if (realb && !realw) {
    white_queener = 8;
    white_square = 0;
  }
#ifdef DEBUGPP
  printf("white pawn on %d can promote at %d in %d moves.\n", white_pawn,
      white_square, white_queener);
#endif
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
  } while (0);
  return (0);
}

/* last modified 08/16/05 */
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
  register BITBOARD temp, left, right;
  register BITBOARD wp_moves, bp_moves, p_attacks;
  register int score = 0;
  register int pns, square, file, rank;
  register int w_isolated, b_isolated;
  register int w_isolated_of, b_isolated_of;
  register int w_unblocked, b_unblocked;
  register int wop, bop;
  register int defenders, attackers, weakness, blocked, sq;
  register int kside_open_files, qside_open_files;
  register int kside_half_open_files_b, kside_half_open_files_w;
  register int qside_half_open_files_b, qside_half_open_files_w;
  register int kside_w, kside_b, qside_w, qside_b;
  register int qmissb, kmissb, qmissw, kmissw;

#if defined(DEBUGP)
  int lastsc = 0;
#endif
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
#if !defined(FAST)
    tree->pawn_hits++;
#endif
    tree->pawn_score = *ptable;
    return (tree->pawn_score.p_score);
  }
  tree->pawn_score.key = PawnHashKey;

  tree->pawn_score.allw = 0;
  tree->pawn_score.white_defects_k = 0;
  tree->pawn_score.white_defects_q = 0;
  tree->pawn_score.candidates_w = 0;
  tree->pawn_score.passed_w = 0;

  tree->pawn_score.allb = 0;
  tree->pawn_score.black_defects_k = 0;
  tree->pawn_score.black_defects_q = 0;
  tree->pawn_score.candidates_b = 0;
  tree->pawn_score.passed_b = 0;

  tree->pawn_score.outside = 0;
  tree->pawn_score.protected = 0;
  tree->pawn_score.open_files = 255;
  w_isolated = 0;
  w_isolated_of = 0;
  b_isolated = 0;
  b_isolated_of = 0;
  w_unblocked = 0;
  b_unblocked = 0;
  kside_open_files = 0;
  qside_open_files = 0;
  kside_half_open_files_b = 0;
  kside_half_open_files_w = 0;
  qside_half_open_files_b = 0;
  qside_half_open_files_w = 0;
  qmissb = 0;
  kmissb = 0;
  qmissw = 0;
  kmissw = 0;
/*
 ************************************************************
 *                                                          *
 *   if there are 8 pawns of one color, penalize crafty     *
 *   for not opening the position.                          *
 *                                                          *
 ************************************************************
 */
  if (shared->use_asymmetry) {
    if (shared->root_wtm) {
      if (TotalWhitePawns == 8)
        score -= eight_pawns;
    } else {
      if (TotalBlackPawns == 8)
        score += eight_pawns;
    }
  }
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
    if (score != lastsc)
      printf("white pawn[static] file=%d,   score=%d\n", file, score);
    lastsc = score;
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
      if (!(plus8dir[square] & BlackPawns))
        w_isolated_of++;
    }
/*
 ************************************************************
 *                                                          *
 *   evaluate blocked pawns.  these are pawns that can not  *
 *   advance because they are blocked, or because they will *
 *   be instantly lost due to other enemy pawns preventing  *
 *   them from moving at all.                               *
 *                                                          *
 ************************************************************
 */
    else {
      blocked = 1;
      do {
        for (sq = square; sq < Min(square + 24, A8); sq += 8) {
          defenders = PopCnt(b_pawn_attacks[sq] & wp_moves);
          attackers = PopCnt(w_pawn_attacks[sq] & BlackPawns);
          if (attackers - defenders > 1)
            break;
          else if (attackers) {
            blocked = 0;
            break;
          }
          if (SetMask(sq + 8) & tree->all_pawns)
            break;
        }
        if (sq >= Min(square + 24, A8))
          blocked = 0;
      } while (0);
      if (!blocked)
        w_unblocked++;
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
        if (!weakness)
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
              if (!attackers)
                weakness = 0;
            }
            if (!weakness)
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
              if (!attackers)
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
      } while (0);
#ifdef DEBUGP
      if (score != lastsc)
        printf("white pawn[weak] file=%d,     score=%d\n", file, score);
      lastsc = score;
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
      if (score != lastsc)
        printf("white pawn[doubled] file=%d,  score=%d\n", file, score);
      lastsc = score;
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
      if (score != lastsc)
        printf("white pawn[duo] file=%d,      score=%d\n", file, score);
      lastsc = score;
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
      score += passed_pawn_value[Rank(square)];
      if (minus8dir[square] & WhitePawns)
        score -= passed_pawn_value[Rank(square)] >> 1;
      if (mask_pawn_protected_w[square] & WhitePawns)
        tree->pawn_score.protected |= 1;
      tree->pawn_score.passed_w |= 128 >> file;
#ifdef DEBUGP
      printf("white pawn[passed]            file=%d\n", file);
      if (score != lastsc)
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
                  !(plus8dir[square - 7] & BlackPawns) && (File(square) == FILEG
                      || !(plus8dir[square - 6] & BlackPawns))) ||
              (File(square) > FILEA && SetMask(square - 9) & WhitePawns &&
                  !(plus8dir[square - 9] & BlackPawns) && (File(square) == FILEB
                      || !(plus8dir[square - 10] & BlackPawns)))))
        score += hidden_passed_pawn_value[Rank(square)];
#ifdef DEBUGP
      if (score != lastsc)
        printf("white pawn[hidden] file=%d,   score=%d\n", file, score);
      lastsc = score;
#endif
    }
    Clear(square, pawns);
  }
/*
 ************************************************************
 *                                                          *
 *   now determine which squares are attacked by white      *
 *   pawns to evaluate space.                               *
 *                                                          *
 ************************************************************
 */
  p_attacks =
      ((WhitePawns & mask_left_edge) >> 7) | ((WhitePawns & mask_right_edge) >>
      9);
  for (rank = RANK3; rank <= RANK8; rank++) {
    score += pawn_space[rank] * PopCnt(p_attacks & rank_mask[rank]);
  }
#ifdef DEBUGP
      if (score != lastsc)
        printf("white pawn[space]             score=%d\n", score);
      lastsc = score;
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
    if (score != lastsc)
      printf("black pawn[static] file=%d,   score=%d\n", file, score);
    lastsc = score;
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
      if (!(minus8dir[square] & WhitePawns))
        b_isolated_of++;
    }
/*
 ************************************************************
 *                                                          *
 *   evaluate blocked pawns.  these are pawns that can not  *
 *   advance because they are blocked, or because they will *
 *   be instantly lost due to other enemy pawns preventing  *
 *   them from moving at all.                               *
 *                                                          *
 ************************************************************
 */
    else {
      blocked = 1;
      do {
        for (sq = square; sq > Max(square - 24, H1); sq -= 8) {
          attackers = PopCnt(b_pawn_attacks[sq] & WhitePawns);
          defenders = PopCnt(w_pawn_attacks[sq] & bp_moves);
          if (attackers - defenders > 1)
            break;
          else if (attackers) {
            blocked = 0;
            break;
          }
          if (SetMask(sq - 8) & tree->all_pawns)
            break;
        }
        if (sq <= Max(square - 24, H1))
          blocked = 0;
      } while (0);
      if (!blocked)
        b_unblocked++;
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
        if (!weakness)
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
              if (!attackers)
                weakness = 0;
            }
            if (!weakness)
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
              if (!attackers)
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
      } while (0);
#ifdef DEBUGP
      if (score != lastsc)
        printf("black pawn[weak] file=%d,     score=%d\n", file, score);
      lastsc = score;
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
      if (score != lastsc)
        printf("black pawn[doubled] file=%d,  score=%d\n", file, score);
      lastsc = score;
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
      if (score != lastsc)
        printf("black pawn[duo] file=%d,      score=%d\n", file, score);
      lastsc = score;
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
      score -= passed_pawn_value[(RANK8 - Rank(square))];
      if (plus8dir[square] & BlackPawns)
        score += passed_pawn_value[RANK8 - Rank(square)] >> 1;
      if (mask_pawn_protected_b[square] & BlackPawns)
        tree->pawn_score.protected |= 2;
      tree->pawn_score.passed_b |= 128 >> file;
#ifdef DEBUGP
      printf("black pawn[passed]            file=%d\n", file);
      if (score != lastsc)
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
      if (score != lastsc)
        printf("black pawn[hidden] file=%d,   score=%d\n", file, score);
      lastsc = score;
#endif
    }
    Clear(square, pawns);
  }
/*
 ************************************************************
 *                                                          *
 *   now determine which squares are attacked by black      *
 *   pawns to evaluate space.                               *
 *                                                          *
 ************************************************************
 */
  p_attacks =
      ((BlackPawns & mask_left_edge) << 9) | ((BlackPawns & mask_right_edge) <<
      7);
  for (rank = RANK1; rank < RANK7; rank++) {
    score -= pawn_space[7 - rank] * PopCnt(p_attacks & rank_mask[rank]);
  }
#ifdef DEBUGP
      if (score != lastsc)
        printf("black pawn[space]             score=%d\n", score);
      lastsc = score;
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
  if (score != lastsc)
    printf("pawn[isolated]          score=%d\n", score);
  lastsc = score;
#endif
/*
 ************************************************************
 *                                                          *
 *   now fold in the bonus for unblocked pawns.  the real   *
 *   issue is keeping "lever" possibilities for each side   *
 *   available, so there is some opportunity for breaking   *
 *   the position open, rather than letting it become       *
 *   totally blocked.                                       *
 *                                                          *
 ************************************************************
 */
  if (TotalWhitePawns > 4 && TotalBlackPawns > 4) {
    if (w_unblocked <= 2 && w_isolated < 2)
      score -= pawns_blocked * (3 - w_unblocked) * blocked_scale / 100;
    if (b_unblocked <= 2 && b_isolated < 2)
      score += pawns_blocked * (3 - b_unblocked) * blocked_scale / 100;
  }
#ifdef DEBUGP
  if (score != lastsc)
    printf("pawn[unblocked]         score=%d\n", score);
  lastsc = score;
#endif
/*
 ************************************************************
 *                                                          *
 *   now add in the pawn ram penalty.  the idea here is     *
 *   that rams hinder tactics by blocking the position,     *
 *   which sidesteps the computer's strong point.           *
 *                                                          *
 ************************************************************
 */
  if (shared->use_asymmetry && !shared->computer_opponent) {
    if (shared->root_wtm) {
      if (TotalWhitePawns > 5)
        score -= pawn_rams[PopCnt(WhitePawns & (BlackPawns << 8))];
    } else {
      if (TotalBlackPawns > 5)
        score += pawn_rams[PopCnt(WhitePawns & (BlackPawns << 8))];
    }
  }
#ifdef DEBUGP
  if (score != lastsc)
    printf("pawn[rams]              score=%d\n", score);
  lastsc = score;
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
  left = file_mask[FILEA];
  right = file_mask[FILEH];
  for (file = 0; file < 3; file++) {
    if (file < 2) {
      kside_w = 1;
      kside_b = 1;
    } else {
      kside_w =
          kside_open_files + kside_half_open_files_b + kside_half_open_files_w +
          kmissw;
      kside_b =
          kside_open_files + kside_half_open_files_b + kside_half_open_files_w +
          kmissb;
    }
    if (!(right & tree->all_pawns))
      kside_open_files++;
    else {
      if (kside_w) {
        if (!(right & WhitePawns)) {
          kside_half_open_files_w++;
        } else if (!(WhitePawns & SetMask(H2 - file))) {
          kmissw++;
          if (!(WhitePawns & SetMask(H3 - file)))
            kmissw++;
          if (file == 1)
            kmissw++;
        }
      }
      if (kside_b) {
        if (!(right & BlackPawns)) {
          kside_half_open_files_b++;
        } else if (!(BlackPawns & SetMask(H7 - file))) {
          kmissb++;
          if (!(BlackPawns & SetMask(H6 - file)))
            kmissb++;
          if (file == 1)
            kmissb++;
        }
      }
    }
    right = right << 1;

    if (file < 2) {
      qside_w = 1;
      qside_b = 1;
    } else {
      qside_w =
          qside_open_files + qside_half_open_files_b + qside_half_open_files_w +
          qmissw;
      qside_b =
          qside_open_files + qside_half_open_files_b + qside_half_open_files_w +
          qmissb;
    }
    if (!(left & tree->all_pawns))
      qside_open_files++;
    else {
      if (qside_w) {
        if (!(left & WhitePawns)) {
          qside_half_open_files_w++;
        } else if (!(WhitePawns & SetMask(A2 + file))) {
          qmissw++;
          if (!(WhitePawns & SetMask(A3 + file)))
            qmissw++;
          if (file == 1)
            qmissw++;
        }
      }
      if (qside_b) {
        if (!(left & BlackPawns)) {
          qside_half_open_files_b++;
        } else if (!(BlackPawns & SetMask(A7 + file))) {
          qmissb++;
          if (!(BlackPawns & SetMask(A6 + file)))
            qmissb++;
          if (file == 1)
            qmissb++;
        }
      }
    }
    left = left >> 1;
  }
/*
 ************************************************************
 *                                                          *
 *   now a special case, where one side has advanced two    *
 *   pawns (ie pawns at f2, g3, h3, where the two advances  *
 *   are not quite as dangerous as they seem.               *
 *                                                          *
 ************************************************************
 */
  if (kmissw) {
    if ((mask_F3G3 & WhitePawns) == mask_F3G3)
      kmissw--;
    else if ((mask_G3H3 & WhitePawns) == mask_G3H3)
      kmissw--;
  }
  if (qmissw) {
    if ((mask_A3B3 & WhitePawns) == mask_A3B3)
      qmissw--;
    else if ((mask_B3C3 & WhitePawns) == mask_B3C3)
      qmissw--;
  }
  if (kmissb) {
    if ((mask_F6G6 & BlackPawns) == mask_F6G6)
      kmissb--;
    else if ((mask_G6H6 & BlackPawns) == mask_G6H6)
      kmissb--;
  }
  if (qmissb) {
    if ((mask_A6B6 & BlackPawns) == mask_A6B6)
      qmissb--;
    else if ((mask_B6C6 & BlackPawns) == mask_B6C6)
      qmissb--;
  }
/*
 ************************************************************
 *                                                          *
 *   now we take the number of open/half-open files, plus   *
 *   the number of 'missing' pawns on the 2nd rank in front *
 *   of the king, and fold them into a 'defect count.'  we  *
 *   run these raw counts 'indirect' through a scaling      *
 *   procedure so that two open files are far worse than    *
 *   one, and so forth.                                     *
 *                                                          *
 ************************************************************
 */
  tree->pawn_score.white_defects_k =
      kmissw + openf[kside_open_files] + hopenf[kside_half_open_files_w] +
      (hopenf[kside_half_open_files_b] >> 1);
  tree->pawn_score.white_defects_q =
      qmissw + openf[qside_open_files] + hopenf[qside_half_open_files_w] +
      (hopenf[qside_half_open_files_b] >> 1);
  tree->pawn_score.black_defects_k =
      kmissb + openf[kside_open_files] +
      (hopenf[kside_half_open_files_w] >> 1) + hopenf[kside_half_open_files_b];
  tree->pawn_score.black_defects_q =
      qmissb + openf[qside_open_files] +
      (hopenf[qside_half_open_files_w] >> 1) + hopenf[qside_half_open_files_b];
#if defined(DEBUGK)
  printf("white: kmissing=%d  kopen=%d  khalf=%d\n", kmissw,
      openf[kside_open_files], hopenf[kside_half_open_files_w]);
  printf("white: qmissing=%d  qopen=%d  qhalf=%d\n", qmissw,
      openf[qside_open_files], hopenf[qside_half_open_files_w]);
  printf("black: kmissing=%d  kopen=%d  khalf=%d\n", kmissb,
      openf[kside_open_files], hopenf[kside_half_open_files_b]);
  printf("black: qmissing=%d  qopen=%d  qhalf=%d\n", qmissb,
      openf[qside_open_files], hopenf[qside_half_open_files_b]);
  printf("white, defects=%d(q)  %d(k)\n", tree->pawn_score.white_defects_q,
      tree->pawn_score.white_defects_k);
  printf("black, defects=%d(q)  %d(k)\n", tree->pawn_score.black_defects_q,
      tree->pawn_score.black_defects_k);
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
    if (!wop || !bop) {
      if (!(tree->pawn_score.protected & 2)) {
        if (wop > 1)
          tree->pawn_score.outside |= 2;
        else if (wop)
          tree->pawn_score.outside |= 1;
      }
      if (!(tree->pawn_score.protected & 1)) {
        if (bop > 1)
          tree->pawn_score.outside |= 8;
        else if (bop)
          tree->pawn_score.outside |= 4;
      }
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
  score = score * pawn_scale / 100;
  tree->pawn_score.p_score = score;
  *ptable = tree->pawn_score;
  return (score);
}

/* last modified 08/07/05 */
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
        BITBOARD moves = edge_moves[WhiteKingSQ];

        stalemate = 1;
        while (moves) {
          int square = FirstOne(moves);

          if (!Attacked(tree, square, 0)) {
            stalemate = 0;
            break;
          }
          moves ^= SetMask(square);
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
        BITBOARD moves = edge_moves[BlackKingSQ];

        stalemate = 1;
        while (moves) {
          int square = FirstOne(moves);

          if (!Attacked(tree, square, 1)) {
            stalemate = 0;
            break;
          }
          moves ^= SetMask(square);
        }
      }
    }
  }
  return (stalemate);
}

/* last modified 08/07/05 */
/*
 *******************************************************************************
 *                                                                             *
 *   EvaluateWinner() is used to determine if one side (or both) are in a      *
 *   position where winning is impossible.                                     *
 *                                                                             *
 *   return values:                                                            *
 *        0    ->     neither side can win, this is a dead drawn position.     *
 *        1    ->     white can win, black can not win.                        *
 *        2    ->     white can not win, black can win.                        *
 *        3    ->     both white and black can win.                            *
 *                                                                             *
 *******************************************************************************
 */
int EvaluateWinner(TREE * RESTRICT tree)
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
  if (WhiteMajors == BlackMajors) {
    if (TotalWhitePawns == 0 && WhiteMinors - BlackMinors == 1) {
      if ((mask_not_edge & BlackKing) || (WhiteMajors == 0))
        can_win &= 2;
    }
    if (TotalBlackPawns == 0 && BlackMinors - WhiteMinors == 1) {
      if ((mask_not_edge & WhiteKing) || (BlackMajors == 0))
        can_win &= 1;
    }
    if (can_win == 0)
      return (can_win);
  }
/*
 ************************************************************
 *                                                          *
 *   if one side is an exchange up, but has no pawns, then  *
 *   that side can not possibly win.                        *
 *                                                          *
 ************************************************************
 */
  if (WhiteMajors != BlackMajors) {
    if ((WhiteMajors - BlackMajors) == (BlackMinors - WhiteMinors)) {
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
  if (!TotalBlackPieces) {
    if (!TotalBlackPawns)
      can_win &= 1;
    if (TotalBlackPawns == 1 && TotalWhitePieces)
      can_win &= 1;
  }
  if (!TotalWhitePieces) {
    if (!TotalWhitePawns)
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

            bkd = Distance(BlackKingSQ, A8);
            if (bkd <= 1)
              can_win &= 2;
            else {
              wkd = Distance(WhiteKingSQ, A8);
              pd = Distance(LastOne(WhitePawns & file_mask[FILEA]), A8);
              if (bkd < (wkd - wtm) && bkd <= (pd - wtm))
                can_win &= 2;
            }
            continue;
          } else {
            int bkd, wkd, pd;

            bkd = Distance(BlackKingSQ, H8);
            if (bkd <= 1)
              can_win &= 2;
            else {
              wkd = Distance(WhiteKingSQ, H8);
              pd = Distance(LastOne(WhitePawns & file_mask[FILEH]), H8);
              if (bkd < (wkd - wtm) && bkd <= (pd - wtm))
                can_win &= 2;
            }
            continue;
          }
        }
      }
    } while (0);
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

            wkd = Distance(WhiteKingSQ, A1);
            if (wkd <= 1)
              can_win &= 1;
            else {
              bkd = Distance(BlackKingSQ, A1);
              pd = Distance(FirstOne(BlackPawns & file_mask[FILEA]), A1);
              if (wkd < (bkd + wtm) && wkd <= (pd + wtm))
                can_win &= 1;
            }
            continue;
          } else {
            int bkd, wkd, pd;

            wkd = Distance(WhiteKingSQ, H1);
            if (wkd <= 1)
              can_win &= 1;
            else {
              bkd = Distance(BlackKingSQ, H1);
              pd = Distance(FirstOne(BlackPawns & file_mask[FILEH]), H1);
              if (wkd < (bkd + wtm) && wkd <= (pd + wtm))
                can_win &= 1;
            }
            continue;
          }
        }
      }
    } while (0);
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

#if defined(DETECTDRAW)
/*
 *******************************************************************************
 *                                                                             *
 *   EvaluateDraws() detects completely blocked positions as draws (hopefully).*
 *                                                                             *
 *******************************************************************************
 */

#  define NODRAW       0
#  define DRAW         1

int EvaluateDraws(TREE * RESTRICT tree, int wtm)
{
  register int square, fdist, rdist, open = 0, rank = 0;
  int i, sq, blocked, defenders, attackers, kingpath = 0, rookpath = 0;
  int noblockw[2] = { 0, 0 }, noblockb[2] = {
  0, 0};
  BITBOARD temp;
  int wp, bp;

  for (i = FILEA; i <= FILEH; i++) {
    temp = file_mask[i];
    if (WhitePawns & temp) {
      square = LastOne(WhitePawns & temp);
      wp = square;
      blocked = 1;
      do {
        for (sq = square; sq < Min(square + 32, A8); sq += 8) {
          if (SetMask(sq + 8) & BlackPieces)
            break;
          defenders = PopCnt(b_pawn_attacks[sq] & WhitePawns);
          attackers = PopCnt(w_pawn_attacks[sq] & BlackPawns);
          if (attackers > defenders)
            break;
          else if (attackers) {
            blocked = 0;
            break;
          }
        }
        if (sq >= Min(square + 32, A8))
          blocked = 0;
      } while (0);
      if (blocked) {
        if (!(plus8dir[square] & BlackPieces)) {
          if (!TotalBlackPieces) {
            if (Rank(square) >= RANK4)
              blocked = 0;
          }
        }
      }
      if (!blocked) {
        if (noblockw[0])
          noblockw[1] = square;
        else
          noblockw[0] = square;
      }
    } else
      wp = 0;
    if (BlackPawns & temp) {
      square = FirstOne(BlackPawns & temp);
      bp = square;
      blocked = 1;
      do {
        for (sq = square; sq > Max(square - 32, H1); sq -= 8) {
          if (SetMask(sq - 8) & WhitePieces)
            break;
          attackers = PopCnt(b_pawn_attacks[sq] & WhitePawns);
          defenders = PopCnt(w_pawn_attacks[sq] & BlackPawns);
          if (attackers > defenders)
            break;
          else if (attackers) {
            blocked = 0;
            break;
          }
        }
        if (sq <= Max(square - 32, H1))
          blocked = 0;
      } while (0);
      if (blocked) {
        if (!(minus8dir[square] & WhitePieces)) {
          if (!TotalWhitePieces) {
            if (Rank(square) <= RANK5)
              blocked = 0;
          }
        }
      }
      if (!blocked) {
        if (noblockb[0])
          noblockb[1] = square;
        else
          noblockb[0] = square;
      }
    } else
      bp = 0;
    if (!kingpath) {
      if (wp && wp == bp - 8) {
        if (rank)
          rdist = RankDistance(wp, rank);
        else
          rdist = 0;
        rank = wp;
        if (open) {
          if (open == 1 && rdist > 1)
            kingpath = 1;
          else if (open == 2 && rdist)
            kingpath = 1;
        }
        open = 0;
      } else if (wp && SetMask(wp) & (BlackPieces << 8)) {
        if (rank)
          rdist = RankDistance(wp, rank);
        else
          rdist = 0;
        rank = wp;
        if (open) {
          if (open == 1 && rdist > 1)
            kingpath = 1;
          else if (open == 2 && rdist)
            kingpath = 1;
        }
        open = 0;
      } else if (bp && SetMask(bp) & (WhitePieces >> 8)) {
        if (rank)
          rdist = RankDistance(bp - 8, rank);
        else
          rdist = 0;
        rank = bp - 8;
        if (open) {
          if (open == 1 && rdist > 1)
            kingpath = 1;
          else if (open == 2 && rdist)
            kingpath = 1;
        }
        open = 0;
      } else
        open++;
      if (open > 2)
        kingpath = 1;
      else if (i == FILEH && open == 2)
        kingpath = 1;
    }
    if (kingpath) {
      if (WhiteMajors && BlackMajors)
        return (NODRAW);
      else if (WhiteMinors > 1 && BlackMinors > 1)
        return (NODRAW);
    }
    if (open)
      rookpath = 1;
  }
  if (!rookpath)
    return (DRAW);
  else if (!kingpath && !noblockw[0] && !noblockb[0])
    return (DRAW);
  if (noblockw[1]) {
    fdist = FileDistance(noblockw[1], noblockw[0]);
    if (WhiteMajors || WhiteMinors > 1)
      return (NODRAW);
    else if (!TotalBlackPieces) {
      if (fdist > 1)
        return (NODRAW);
      else {
        square = noblockw[1];
        if (Rank(square) >= RANK4) {
          rdist = 7 - Rank(square);
          fdist = FileDistance(square, BlackKingSQ);
          if (rdist < Max(fdist, 7 - Rank(BlackKingSQ)) - (1 - wtm) ||
              (Rank(WhiteKingSQ) >= Rank(square) && kingpath))
            return (NODRAW);
        }
      }
    } else if (!noblockb[0] && !kingpath && !BlackMajors && BlackMinors <= 1)
      return (DRAW);
  }
  if (noblockw[0]) {
    if (WhiteMajors || WhiteMinors > 1)
      return (NODRAW);
    else if (!TotalBlackPieces) {
      square = noblockw[0];
      if (Rank(square) >= RANK4) {
        rdist = 7 - Rank(square);
        fdist = FileDistance(square, BlackKingSQ);
        if (rdist < Max(fdist, 7 - Rank(BlackKingSQ)) - (1 - wtm) ||
            (Rank(WhiteKingSQ) >= Rank(square) && kingpath))
          return (NODRAW);
        else if (!noblockb[0] && !kingpath && !WhiteMajors && WhiteMinors <= 1)
          return (DRAW);
      }
    } else if (!noblockb[0] && !kingpath && !BlackMajors && BlackMinors <= 1)
      return (DRAW);
  }
  if (noblockb[1]) {
    fdist = FileDistance(noblockb[1], noblockb[0]);
    if (BlackMajors || BlackMinors > 1)
      return (NODRAW);
    else if (!TotalWhitePieces) {
      if (fdist > 1)
        return (NODRAW);
      else {
        square = noblockb[1];
        if (Rank(square) <= RANK5) {
          fdist = FileDistance(square, WhiteKingSQ);
          if (Rank(square) < Max(fdist, Rank(WhiteKingSQ)) - wtm ||
              (Rank(BlackKingSQ) <= Rank(square) && kingpath))
            return (NODRAW);
        }
      }
    } else if (!noblockw[0] && !kingpath && !WhiteMajors && WhiteMinors <= 1)
      return (DRAW);
  }
  if (noblockb[0]) {
    if (BlackMajors || BlackMinors > 1)
      return (NODRAW);
    else if (!TotalWhitePieces) {
      square = noblockb[0];
      if (Rank(square) <= RANK5) {
        fdist = FileDistance(square, WhiteKingSQ);
        if (Rank(square) < Max(fdist, Rank(WhiteKingSQ)) - wtm ||
            (Rank(BlackKingSQ) <= Rank(square) && kingpath))
          return (NODRAW);
        else if (!noblockw[0] && !kingpath && !BlackMajors && BlackMinors <= 1)
          return (DRAW);
      }
    } else if (!noblockw[0] && !kingpath && !WhiteMajors && WhiteMinors <= 1)
      return (DRAW);
  } else if (!(noblockb[0] || noblockw[0])) {
    if (!TotalWhitePieces && !TotalBlackPieces) {
      if (!kingpath)
        return (DRAW);
      else if (HasOpposition(wtm, WhiteKingSQ, BlackKingSQ) &&
          Rank(WhiteKingSQ) >= Rank(rank))
        return (NODRAW);
      else if (HasOpposition(Flip(wtm), BlackKingSQ, WhiteKingSQ) &&
          Rank(BlackKingSQ) <= Rank(rank))
        return (NODRAW);
      else
        return (DRAW);
    } else if (!TotalWhitePieces) {
      if (kingpath && BlackMinors | BlackMajors)
        return (NODRAW);
      else if (!kingpath && (BlackMinors > 1 || BlackMajors))
        return (NODRAW);
      else
        return (DRAW);
    } else if (!TotalBlackPieces) {
      if (kingpath && WhiteMinors | WhiteMajors)
        return (NODRAW);
      else if (!kingpath && (WhiteMinors > 1 || WhiteMajors))
        return (NODRAW);
      else
        return (DRAW);
    } else if (!kingpath &&
        WhiteMinors + PopCnt(WhiteRooks | WhiteQueens) ==
        BlackMinors + PopCnt(BlackRooks | BlackQueens))
      return (DRAW);
    else
      return (NODRAW);
  }
  return (NODRAW);
}
#endif
