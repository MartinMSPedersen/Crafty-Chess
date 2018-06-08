#include <stdio.h>
#include <stdlib.h>
#include "chess.h"
#include "evaluate.h"
#include "data.h"

/* last modified 02/26/01 */
/*
********************************************************************************
*                                                                              *
*   Evaluate() is used to evaluate the chess board.  broadly, it addresses     *
*   four (4) distinct areas:  (1) material score which is simply a summing of  *
*   piece types multiplied by piece values;  (2) pawn scoring which considers  *
*   placement of pawns and also evaluates passed pawns, particularly in end-   *
*   game situations;  (3) piece scoring which evaluates the placement of each  *
*   piece as well as things like piece mobility;  (4) king safety which        *
*   considers the pawn shelter around the king along with material present to  *
*   facilitate an attack.                                                      *
*                                                                              *
********************************************************************************
*/

int Evaluate(TREE *tree, int ply, int wtm, int alpha, int beta) {
  register BITBOARD temp;
  register int square, file, score, tscore, w_tropism=0, b_tropism=0;
  register int w_spread, b_spread, trop, drawn_ending=0;
#if defined(DEBUGEV)
  int lastsc;
#endif
/*
**********************************************************************
*                                                                    *
*   check for draws due to insufficient material and adjust the      *
*   score as necessary.                                              *
*                                                                    *
**********************************************************************
*/
  if (TotalWhitePieces<9 && TotalBlackPieces<9)
    drawn_ending=EvaluateDraws(tree);
  if (drawn_ending > 0) return(DrawScore(wtm));
  score=EvaluateMaterial(tree);
#ifdef DEBUGEV
  printf("score[material]=                  %4d\n",score);
  lastsc=score;
#endif
/*
**********************************************************************
*                                                                    *
*   determine if this is position should be evaluated to force mate  *
*   (neither side has pawns) or if it should be evaluated normally.  *
*   call EvaluatePawns() to evaluate the current pawn position.      *
*   this routine modifies the "passed" pawn bit-vector which         *
*   indicates whether a pawn on each file is passed or not.          *
*                                                                    *
**********************************************************************
*/
  tree->all_pawns=BlackPawns|WhitePawns;
  if ((TotalWhitePawns+TotalBlackPawns) == 0) do {
    int ms=EvaluateMate(tree);
    if (ms == 99999) break;
    score+=ms;
#ifdef DEBUGEV
    printf("score[mater]=                     %4d (%+d)\n",score,score-lastsc);
#endif
    if (score>0 && drawn_ending==-1) return(DrawScore(wtm));
    if (score<0 && drawn_ending==-2) return(DrawScore(wtm));
    return((wtm) ? score : -score);
  } while(0);
#if !defined(FAST)
  tree->pawn_probes++;
#endif
  if (PawnHashKey == tree->pawn_score.key) {
#if !defined(FAST)
    tree->pawn_hits++;
#endif
    score+=tree->pawn_score.p_score;
  }
  else score+=EvaluatePawns(tree);
#ifdef DEBUGEV
  if (score != lastsc)
    printf("score[pawns]=                     %4d (%+d)\n",score,score-lastsc);
  lastsc=score;
#endif
/*
**********************************************************************
*                                                                    *
*   if there are any passed pawns, first call EvaluatePassedPawns()  *
*   to evaluate them.  then, if one side has a passed pawn and the   *
*   other side has no pieces, call EvaluatePassedPawnRaces() to see  *
*   if the passed pawn can be stopped from promoting.  finally, we   *
*   use tree->pawn_score.outside to see if one side has an outside   *
*   passed pawn that represents a nearly won endgame advantage.      *
*                                                                    *
**********************************************************************
*/
  if (tree->pawn_score.passed_b || tree->pawn_score.passed_w) {
    int pscore=EvaluatePassedPawns(tree);
    int wmult=0, bmult=0;
    if (tree->pawn_score.outside&12) {
      wmult=outside_passed[(int) TotalBlackPieces];
      if (!BlackQueens && !BlackRooks && !BlackBishops) wmult*=2;
    }
    if (tree->pawn_score.outside&192) {
      bmult=outside_passed[(int) TotalWhitePieces];
      if (!WhiteQueens && !WhiteRooks && !WhiteBishops) bmult*=2;
    }
    if (tree->pawn_score.outside&8)
      pscore+=2*wmult;
    else if (tree->pawn_score.outside&4)
      pscore+=wmult;
    if (tree->pawn_score.outside&128)
      pscore-=2*bmult;
    else if (tree->pawn_score.outside&64)
      pscore-=bmult;
    if ((TotalWhitePieces==0 && tree->pawn_score.passed_b) ||
        (TotalBlackPieces==0 && tree->pawn_score.passed_w))
      pscore+=EvaluatePassedPawnRaces(tree,wtm);
    score+=pscore*passed_scale/100;
  }
  if (!(tree->pawn_score.outside&12)) {
    if (tree->pawn_score.outside&2)
      score+=2*majority[(int) TotalBlackPieces];
    else if (tree->pawn_score.outside&1)
      score+=majority[(int) TotalBlackPieces];
  }
  if (!(tree->pawn_score.outside&192)) {
     if (tree->pawn_score.outside&32)
       score-=2*majority[(int) TotalWhitePieces];
     else if (tree->pawn_score.outside&16)
       score-=majority[(int) TotalWhitePieces];
  }
  if (TotalWhitePieces+TotalBlackPieces == 0) {
    int pscore=0;
    int wfile=File(WhiteKingSQ);
    int bfile=File(BlackKingSQ);
    if (wfile+wtm > bfile+1) {
      if (wfile < first_ones_8bit[tree->pawn_score.allb])
        pscore=WON_KP_ENDING;
    }
    else if (wfile-wtm < bfile-1) {
      if (wfile > last_ones_8bit[tree->pawn_score.allb])
        pscore=WON_KP_ENDING;
    }
    if (bfile > wfile+1-wtm) {
      if (bfile < first_ones_8bit[tree->pawn_score.allw])
        pscore=-WON_KP_ENDING;
    }
    else if (bfile < wfile-1+wtm) {
      if (bfile > last_ones_8bit[tree->pawn_score.allw])
        pscore=-WON_KP_ENDING;
    }
    score+=pscore*passed_scale/100;
  }
  else {
    if (!TotalBlackPieces) {
      w_spread=file_spread[tree->pawn_score.passed_w|
                           tree->pawn_score.candidates_w];
      if (w_spread>1) score+=(w_spread-1)*(outside_passed[0]>>1);
    }
    if (!TotalWhitePieces) {
      b_spread=file_spread[tree->pawn_score.passed_b|
                           tree->pawn_score.candidates_b];
      if (b_spread>1) score-=(b_spread-1)*(outside_passed[0]>>1);
    }
  }
#ifdef DEBUGEV
  if (score != lastsc) {
    printf("score[passed pawns]=              %4d (%+d)\n",score,score-lastsc);
    printf("score[passed pawns] (flags) =     %4x\n",tree->pawn_score.outside);
    lastsc=score;
  }
#endif
/*
 ----------------------------------------------------------
|                                                          |
|   check to see if the bishop is trapped at a2 or h2 with |
|   a pawn at b2 or g2 that can advance one square and     |
|   trap the bishop, or a pawn at b3 or g3 that has        |
|   trapped the bishop already.  also make sure that this  |
|   pawn is defended to close the trap.                    |
|                                                          |
 ----------------------------------------------------------
*/
  if (WhiteBishops) {
    if (WhiteBishops&mask_A7H7) {
      if (WhiteBishops&SetMask(A7) && SetMask(B6)&BlackPawns)
        score-=BISHOP_TRAPPED;
      else if (WhiteBishops&SetMask(H7) && SetMask(G6)&BlackPawns)
        score-=BISHOP_TRAPPED;
    }
  }
  if (BlackBishops) {
    if (BlackBishops&mask_A2H2) {
      if (BlackBishops&SetMask(A2) && SetMask(B3)&WhitePawns)
          score+=BISHOP_TRAPPED;
      else if (BlackBishops&SetMask(H2) && SetMask(G3)&WhitePawns)
        score+=BISHOP_TRAPPED;
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
  if (WhiteCastle(1)) score+=EvaluateDevelopmentW(tree,ply);
  if (BlackCastle(1)) score+=EvaluateDevelopmentB(tree,ply);
#ifdef DEBUGEV
  if (score != lastsc)
    printf("score[development]=               %4d (%+d)\n",score,score-lastsc);
  lastsc=score;
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
  tree->endgame=(TotalWhitePieces<=EG_MAT || TotalBlackPieces<=EG_MAT);
  tree->w_kingsq=WhiteKingSQ;
  tree->b_kingsq=BlackKingSQ;
  tree->w_safety=0;
  tree->b_safety=0;
  if (!tree->endgame) score+=EvaluateKingSafety(tree,ply);
#ifdef DEBUGEV
  if (score != lastsc)
    printf("score[king safety]=               %4d (%+d)\n",score,score-lastsc);
  lastsc=score;
#endif
/*
**********************************************************************
*                                                                    *
*  check to see if we can take a lazy exit and avoid the time-       *
*  consuming part of the evaluation code.                            *
*                                                                    *
**********************************************************************
*/
  if (drawn_ending == 0) {
    register const int tscore=(wtm)?score:-score;
    if (tscore-largest_positional_score >= beta) return(beta);
    if (tscore+largest_positional_score <= alpha) return(alpha);
  }
  tscore=score;
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
 ----------------------------------------------------------
|                                                          |
|   white king.                                            |
|                                                          |
|   first, check for a weak back rank.                     |
|                                                          |
 ----------------------------------------------------------
*/
  if (tree->endgame) {
    if ((tree->all_pawns & mask_efgh) && (tree->all_pawns & mask_abcd))
      score+=kval_wn[WhiteKingSQ];
    else if (tree->all_pawns & mask_efgh)
      score+=kval_wk[WhiteKingSQ];
    else if (tree->all_pawns & mask_abcd)
      score+=kval_wq[WhiteKingSQ];
  }
  if (WhiteKingSQ < A2) {
    if (!(king_attacks[WhiteKingSQ]&rank_mask[RANK2] &
             ~WhitePawns)) score-=KING_BACK_RANK;
/*
 ----------------------------------------------------------
|                                                          |
|   check to see if the king has been forced to move and   |
|   has trapped a rook at a1/a2/b1/g1/h1/h2, if so, then   |
|   penalize the trapped rook to help extricate it.        |
|                                                          |
 ----------------------------------------------------------
*/
    if (WhiteKingSQ > E1) {
      if (WhiteRooks&mask_kr_trapped_w[FILEH-WhiteKingSQ])
        score-=ROOK_TRAPPED;
    }
    else if (WhiteKingSQ < D1) {
      if (WhiteRooks&mask_qr_trapped_w[WhiteKingSQ])
        score-=ROOK_TRAPPED;
    }
  }
#ifdef DEBUGEV
  if (score != lastsc)
    printf("score[kings(white)]=              %4d (%+d)\n",score,score-lastsc);
  lastsc=score;
#endif

/*
 ----------------------------------------------------------
|                                                          |
|   black king.                                            |
|                                                          |
|   first, check for a weak back rank.                     |
|                                                          |
 ----------------------------------------------------------
*/
  if (tree->endgame) {
    if ((tree->all_pawns & mask_efgh) && (tree->all_pawns & mask_abcd))
      score-=kval_bn[BlackKingSQ];
    else if (tree->all_pawns & mask_efgh)
      score-=kval_bk[BlackKingSQ];
    else if (tree->all_pawns & mask_abcd)
      score-=kval_bq[BlackKingSQ];
  }
  if (BlackKingSQ > H7) {
    if (!(king_attacks[BlackKingSQ]&rank_mask[RANK7]&
             ~BlackPawns)) score+=KING_BACK_RANK;
/*
 ----------------------------------------------------------
|                                                          |
|   check to see if the king has been forced to move and   |
|   has trapped a rook at a1/a2/b1/g1/h1/h2, if so, then   |
|   penalize the trapped rook to help extricate it.        |
|                                                          |
 ----------------------------------------------------------
*/
    if (BlackKingSQ > E8) {
      if (BlackRooks&mask_kr_trapped_b[FILEH-File(BlackKingSQ)])
        score+=ROOK_TRAPPED;
    }
    else if (BlackKingSQ < D8) {
      if (BlackRooks&mask_qr_trapped_b[File(BlackKingSQ)])
        score+=ROOK_TRAPPED;
    }
  }
#ifdef DEBUGEV
  if (score != lastsc)
    printf("score[kings(black)]=              %4d (%+d)\n",score,score-lastsc);
  lastsc=score;
#endif

/*
**********************************************************************
*                                                                    *
*   knight evaluation includes centralization and "outposts".        *
*                                                                    *
**********************************************************************
*/
/*
 ----------------------------------------------------------
|                                                          |
|   white knights.                                         |
|                                                          |
|   first, evaluate for "outposts" which is a knight that  |
|   can't be driven off by an enemy pawn, and which is     |
|   supported by a friendly pawn.  an outpost is much      |
|   stronger if it is supported by two pawns (capturing it |
|   results in a protected passed pawn) or if the opponent |
|   has no minor piece that can attack it.                 |
|                                                          |
 ----------------------------------------------------------
*/
  temp=WhiteKnights;
  while(temp) {
    square=FirstOne(temp);
    if (white_outpost[square] &&
        !(mask_no_pawn_attacks_b[square]&BlackPawns)) {
      int defenders, safe=0;
      score+=white_outpost[square];
      if (b_pawn_attacks[square]&WhitePawns) {
        defenders=PopCnt(b_pawn_attacks[square]&WhitePawns);
        if (defenders && !BlackKnights) {
          if (light_squares&SetMask(square)) {
            if (!(light_squares&BlackBishops)) safe=1;
          }
          else {
            if (!(dark_squares&BlackBishops)) safe=1;
          }
        }
        if (safe || defenders == 2) score+=white_outpost[square];
        else if (defenders == 1) score+=white_outpost[square]>>1;
      }
      if ((square==D6 || square==E6) &&
          SetMask(square+8)&BlackPawns) score+=BLOCKED_CENTER_PAWN;
      if ((square==D3 || square==E3) &&
          SetMask(square-8)&WhitePawns) score-=BLOCKED_CENTER_PAWN;
    }
/*
 ----------------------------------------------------------
|                                                          |
|   now fold in centralization score from the piece/square |
|   table "nval_*".                                |
|                                                          |
 ----------------------------------------------------------
*/
    score+=nval_w[square];
/*
 ----------------------------------------------------------
|                                                          |
|   adjust the white tropism count for this piece.         |
|                                                          |
 ----------------------------------------------------------
*/
    w_tropism+=king_tropism_n[Distance(square,tree->b_kingsq)];
    Clear(square,temp);
  }
#ifdef DEBUGEV
  if (score != lastsc)
    printf("score[knights(white)]=            %4d (%+d)\n",score,score-lastsc);
  lastsc=score;
#endif
/*
 ----------------------------------------------------------
|                                                          |
|   black knights.                                         |
|                                                          |
|   first, evaluate for "outposts" which is a knight that  |
|   can't be driven off by an enemy pawn, and which is     |
|   supported by a friendly pawn.  an outpost is much      |
|   stronger if it is supported by two pawns (capturing it |
|   results in a protected passed pawn) or if the opponent |
|   has no minor piece that can attack it.                 |
|                                                          |
 ----------------------------------------------------------
*/
  temp=BlackKnights;
  while(temp) {
    square=FirstOne(temp);
    if (black_outpost[square] &&
        !(mask_no_pawn_attacks_w[square]&WhitePawns)) {
      int defenders, safe=0;
      score-=black_outpost[square];
      if (w_pawn_attacks[square]&BlackPawns) {
        defenders=PopCnt(w_pawn_attacks[square]&BlackPawns);
        if (defenders && !WhiteKnights) {
          if (light_squares&SetMask(square)) {
            if (!(light_squares&WhiteBishops)) safe=1;
          }
          else {
            if (!(dark_squares&WhiteBishops)) safe=1;
          }
        }
        if (safe || defenders == 2) score-=black_outpost[square];
        else if (defenders == 1) score-=black_outpost[square]>>1;
      }
      if ((square==D3 || square==E3) &&
          SetMask(square-8)&WhitePawns) score-=BLOCKED_CENTER_PAWN;
      if ((square==D6 || square==E6) &&
          SetMask(square+8)&BlackPawns) score+=BLOCKED_CENTER_PAWN;
    }
/*
 ----------------------------------------------------------
|                                                          |
|   now fold in centralization score from the piece/square |
|   table "nval_*".                                |
|                                                          |
 ----------------------------------------------------------
*/
    score-=nval_b[square];
/*
 ----------------------------------------------------------
|                                                          |
|   adjust the black tropism count for this piece.         |
|                                                          |
 ----------------------------------------------------------
*/
    b_tropism+=king_tropism_n[Distance(square,tree->w_kingsq)];
    Clear(square,temp);
  }
#ifdef DEBUGEV
  if (score != lastsc)
    printf("score[knights(black)]=            %4d (%+d)\n",score,score-lastsc);
  lastsc=score;
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
 ----------------------------------------------------------
|                                                          |
|   white bishops                                          |
|                                                          |
|   first, locate each bishop and add in its static score  |
|   from the bishop piece/square table.                    |
|                                                          |
 ----------------------------------------------------------
*/
  temp=WhiteBishops;
  while(temp) {
    square=FirstOne(temp);
    score+=bval_w[square];
/*
 ----------------------------------------------------------
|                                                          |
|   then fold in the mobility score.                       |
|                                                          |
 ----------------------------------------------------------
*/
    score+=BISHOP_MOBILITY*MobilityBishop(square);
/*
 ----------------------------------------------------------
|                                                          |
|   now add in a bonus for a bishop blocking a center pawn |
|   at D6/E6 as that is very cramping.                     |
|                                                          |
 ----------------------------------------------------------
*/
    if (white_outpost[square] &&
        !(mask_no_pawn_attacks_b[square]&BlackPawns)) {
      if ((square==D6 || square==E6) &&
          SetMask(square+8)&BlackPawns) score+=BLOCKED_CENTER_PAWN;
      if ((square==D3 || square==E3) &&
          SetMask(square-8)&WhitePawns) score-=BLOCKED_CENTER_PAWN;
    }
/*
 ----------------------------------------------------------
|                                                          |
|   adjust the white tropism count for this piece.         |
|                                                          |
 ----------------------------------------------------------
*/
    w_tropism+=king_tropism_b[Distance(square,tree->b_kingsq)];
    Clear(square,temp);
  }
#ifdef DEBUGEV
  if (score != lastsc)
    printf("score[bishops(white)]=            %4d (%+d)\n",score,score-lastsc);
  lastsc=score;
#endif
/*
 ----------------------------------------------------------
|                                                          |
|   black bishops                                          |
|                                                          |
|   first, locate each bishop and add in its static score  |
|   from the bishop piece/square table.                    |
|                                                          |
 ----------------------------------------------------------
*/
  temp=BlackBishops;
  while(temp) {
    square=FirstOne(temp);
    score-=bval_b[square];
/*
 ----------------------------------------------------------
|                                                          |
|   then fold in the mobility score.                       |
|                                                          |
 ----------------------------------------------------------
*/
    score-=BISHOP_MOBILITY*MobilityBishop(square);
/*
 ----------------------------------------------------------
|                                                          |
|   now add in a bonus for a bishop blocking a center pawn |
|   at D3/E3 as that is very cramping.                     |
|                                                          |
 ----------------------------------------------------------
*/
    if (black_outpost[square] &&
        !(mask_no_pawn_attacks_b[square]&WhitePawns)) {
      if ((square==D3 || square==E3) &&
          SetMask(square-8)&WhitePawns) score-=BLOCKED_CENTER_PAWN;
      if ((square==D6 || square==E6) &&
          SetMask(square+8)&BlackPawns) score+=BLOCKED_CENTER_PAWN;
    }
/*
 ----------------------------------------------------------
|                                                          |
|   adjust the black tropism count for this piece.         |
|                                                          |
 ----------------------------------------------------------
*/
    b_tropism+=king_tropism_b[Distance(square,tree->w_kingsq)];
    Clear(square,temp);
  }
#ifdef DEBUGEV
  if (score != lastsc)
    printf("score[bishops(black)]=            %4d (%+d)\n",score,score-lastsc);
  lastsc=score;
#endif
/*
 ----------------------------------------------------------
|                                                          |
|   now, give either side a bonus for having two bishops.  |
|   if in the endgame phase, penalize pawns on the same    |
|   color as the bishop, if one side has only one bishop.  |
|                                                          |
|   also add in a bonus for a bishop, if there are pawns   |
|   on both sides of the board in an endgame, because a    |
|   bishop is much more valuable there.                    |
|                                                          |
|   add in a bonus for filling holes when the king has     |
|   castled and played b3/g3/b6/g6.                        |
|                                                          |
 ----------------------------------------------------------
*/
  if (WhiteBishops) {
    if (WhiteBishops&(WhiteBishops-1)) {
      score+=BISHOP_PAIR;
    }
    else {
      if (tree->endgame) {
        if (WhiteBishops&light_squares)
          score-=PopCnt(WhitePawns&light_squares)*BISHOP_PLUS_PAWNS_ON_COLOR;
        else
          score-=PopCnt(WhitePawns&dark_squares)*BISHOP_PLUS_PAWNS_ON_COLOR;
      }
      if (tree->all_pawns&mask_fgh && tree->all_pawns&mask_abc &&
          TotalWhitePieces<7 && !BlackBishops)
        score+=BISHOP_OVER_KNIGHT_ENDGAME;
    }
    if (!tree->endgame) {
      if (File(tree->w_kingsq) > FILEE) {
        if (!(WhitePawns&SetMask(G2)) && (WhitePawns&SetMask(G3)) &&
            Distance(tree->w_kingsq,G2)==1 && WhiteBishops&good_bishop_kw)
          score+=BISHOP_KING_SAFETY;
      }
      else if (File(tree->w_kingsq) < FILED) {
        if (!(WhitePawns&SetMask(B2)) && (WhitePawns&SetMask(B3)) &&
            Distance(tree->w_kingsq,B2)==1 && WhiteBishops&good_bishop_qw)
          score+=BISHOP_KING_SAFETY;
      }
    }
  }
  if (BlackBishops) {
    if (BlackBishops&(BlackBishops-1)) {
      score-=BISHOP_PAIR;
    }
    else {
      if (tree->endgame) {
        if (BlackBishops&light_squares)
          score+=PopCnt(BlackPawns&light_squares)*BISHOP_PLUS_PAWNS_ON_COLOR;
        else
          score+=PopCnt(BlackPawns&dark_squares)*BISHOP_PLUS_PAWNS_ON_COLOR;
      }
      if (tree->all_pawns&mask_fgh && tree->all_pawns&mask_abc &&
          TotalBlackPieces<7 && !WhiteBishops)
        score-=BISHOP_OVER_KNIGHT_ENDGAME;
    }
    if (!tree->endgame) {
      if (File(tree->b_kingsq) > FILEE) {
        if (!(BlackPawns&SetMask(G7)) && (BlackPawns& SetMask(G6)) &&
            Distance(tree->b_kingsq,G7)==1 && BlackBishops&good_bishop_kb)
          score-=BISHOP_KING_SAFETY;
      }
      else if (File(tree->b_kingsq) < FILED) {
        if (!(BlackPawns&SetMask(B7)) && (BlackPawns&SetMask(B6)) &&
            Distance(tree->b_kingsq,B7)==1 && BlackBishops&good_bishop_qb)
          score-=BISHOP_KING_SAFETY;
      }
    }
  }
#ifdef DEBUGEV
  if (score != lastsc)
    printf("score[bishop pair]=               %4d (%+d)\n",score,score-lastsc);
  lastsc=score;
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
 ----------------------------------------------------------
|                                                          |
|   white rooks                                            |
|                                                          |
 ----------------------------------------------------------
*/
  temp=WhiteRooks;
  while(temp) {
    square=FirstOne(temp);
    file=File(square);
    score+=rval_w[square];
/*
 ----------------------------------------------------------
|                                                          |
|   determine if the rook is on an open file.  if it is,   |
|   determine if this rook attacks another friendly rook,  |
|   making it difficult to drive the rooks off the file.   |
|                                                          |
 ----------------------------------------------------------
*/
    trop=7;
    if (!(file_mask[file]&tree->all_pawns)) {
      score+=ROOK_OPEN_FILE;
      trop=FileDistance(square,tree->b_kingsq);
    }
    else if (!(file_mask[file]&WhitePawns)) {
      score+=ROOK_HALF_OPEN_FILE;
      trop=FileDistance(square,tree->b_kingsq);
    }
    else if (!(plus8dir[square]&WhitePawns)) {
      trop=FileDistance(square,tree->b_kingsq);
    }
/*
 ----------------------------------------------------------
|                                                          |
|   see if the rook can move horizontally.  if not, it is  |
|   somewhat precarious in how it is situated.             |
|                                                          |
 ----------------------------------------------------------
*/
    if (!MobilityRank(square)) score-=ROOK_LIMITED;
/*
 ----------------------------------------------------------
|                                                          |
|   see if the rook is behind a passed pawn.  if it is,    |
|   it is given a bonus.                                   |
|                                                          |
 ----------------------------------------------------------
*/
    if (128>>file&tree->pawn_score.passed_w) {
      register const int pawnsq=LastOne(WhitePawns&file_mask[file]);
      if (pawnsq > square) {
        register const int rbp=(AttacksFile(square)&SetMask(pawnsq)) != 0;
        if (rbp && !(BlackPieces&SetMask(pawnsq+8)))
          score+=ROOK_BEHIND_PASSED_PAWN;
      }
    }
    if (128>>file&tree->pawn_score.passed_b) {
      register const int pawnsq=FirstOne(BlackPawns&file_mask[file]);
      if (pawnsq < square) {
        register const int rbp=(AttacksFile(square)&SetMask(pawnsq)) != 0;
        if (rbp) score+=ROOK_BEHIND_PASSED_PAWN;
      }
    }
/*
 ----------------------------------------------------------
|                                                          |
|   finally check to see if any rooks are on the 7th rank, |
|   with the opponent having pawns on that rank and the    |
|   opponent's king being hemmed in on the 7th/8th rank.   |
|   if so, and another rook or queen is also on the 7th,   |
|   then this is a *strong* positional advantage.          |
|                                                          |
 ----------------------------------------------------------
*/
    if (Rank(square)==RANK7 && (BlackKingSQ>H7 ||
                                BlackPawns&rank_mask[RANK7])) {
      score+=ROOK_ON_7TH;
      if (tree->pawn_score.passed_w && BlackKingSQ>H7 &&
          !(BlackPieces&mask_abs7_w)) score+=ROOK_ABSOLUTE_7TH;
      if (AttacksRank(square)&(WhiteRooks|WhiteQueens))
        score+=ROOK_CONNECTED_7TH_RANK;
    }
/*
 ----------------------------------------------------------
|                                                          |
|   adjust the white tropism count for this piece.         |
|                                                          |
 ----------------------------------------------------------
*/
    w_tropism+=Max(king_tropism_at_r[trop],
                   king_tropism_r[Distance(square,tree->b_kingsq)]);
    Clear(square,temp);
  }
#ifdef DEBUGEV
  if (score != lastsc)
    printf("score[rooks(white)]=              %4d (%+d)\n",score,score-lastsc);
  lastsc=score;
#endif
/*
 ----------------------------------------------------------
|                                                          |
|   black rooks                                            |
|                                                          |
 ----------------------------------------------------------
*/
  temp=BlackRooks;
  while(temp) {
    square=FirstOne(temp);
    file=File(square);
    score-=rval_b[square];
/*
 ----------------------------------------------------------
|                                                          |
|   determine if the rook is on an open file.  if it is,   |
|   determine if this rook attacks another friendly rook,  |
|   making it difficult to drive the rooks off the file.   |
|                                                          |
 ----------------------------------------------------------
*/
    trop=7;
    if (!(file_mask[file]&tree->all_pawns)) {
      score-=ROOK_OPEN_FILE;
      trop=FileDistance(square,tree->w_kingsq);
    }
    else if (!(file_mask[file]&BlackPawns)) {
      score-=ROOK_HALF_OPEN_FILE;
      trop=FileDistance(square,tree->w_kingsq);
    }
    else if (!(minus8dir[square]&BlackPawns)) {
      trop=FileDistance(square,tree->w_kingsq);
    }
/*
 ----------------------------------------------------------
|                                                          |
|   see if the rook can move horizontally.  if not, it is  |
|   somewhat precarious in how it is situated.             |
|                                                          |
 ----------------------------------------------------------
*/
    if (!MobilityRank(square)) score+=ROOK_LIMITED;
/*
 ----------------------------------------------------------
|                                                          |
|   see if the rook is behind a passed pawn.  if it is,    |
|   it is given a bonus.                                   |
|                                                          |
 ----------------------------------------------------------
*/
    if (128>>file&tree->pawn_score.passed_b) {
      register const int pawnsq=FirstOne(BlackPawns&file_mask[file]);
      if (pawnsq < square) {
        register const int rbp=(AttacksFile(square)&SetMask(pawnsq)) != 0;
        if (rbp && !(WhitePieces&SetMask(pawnsq-8)))
          score-=ROOK_BEHIND_PASSED_PAWN;
      }
    }
    if (128>>file&tree->pawn_score.passed_w) {
      register const int pawnsq=LastOne(WhitePawns&file_mask[file]);
      if (pawnsq > square) {
        register const int rbp=(AttacksFile(square)&SetMask(pawnsq)) != 0;
        if (rbp) score-=ROOK_BEHIND_PASSED_PAWN;
      }
    }
/*
 ----------------------------------------------------------
|                                                          |
|   finally check to see if any rooks are on the 7th rank, |
|   with the opponent having pawns on that rank and the    |
|   opponent's king being hemmed in on the 7th/8th rank.   |
|   if so, and another rook or queen is also on the 7th,   |
|   then this is a *strong* positional advantage.          |
|                                                          |
 ----------------------------------------------------------
*/
    if (Rank(square)==RANK2 && (WhiteKingSQ<A2 ||
                                WhitePawns&rank_mask[RANK2])) {
      score-=ROOK_ON_7TH;
      if (tree->pawn_score.passed_b && WhiteKingSQ<A2 &&
          !(WhitePieces&mask_abs7_b)) score-=ROOK_ABSOLUTE_7TH;
      if (AttacksRank(square)&(BlackRooks|BlackQueens))
        score-=ROOK_CONNECTED_7TH_RANK;
    }
/*
 ----------------------------------------------------------
|                                                          |
|   adjust the black tropism count for this piece.         |
|                                                          |
 ----------------------------------------------------------
*/
    b_tropism+=Max(king_tropism_at_r[trop],
                   king_tropism_r[Distance(square,tree->w_kingsq)]);
    Clear(square,temp);
  }
#ifdef DEBUGEV
  if (score != lastsc)
    printf("score[rooks(black)]=              %4d (%+d)\n",score,score-lastsc);
  lastsc=score;
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
 ----------------------------------------------------------
|                                                          |
|   white queens                                           |
|                                                          |
|   first locate each queen and obtain it's centralization |
|   score from the static piece/square table for queens.   |
|                                                          |
|   then, if the opposing side's king safety is much worse |
|   than the king safety for this side, add in a bonus to  |
|   keep the queen around.                                 |
|                                                          |
 ----------------------------------------------------------
*/
  temp=WhiteQueens;
  while(temp) {
    square=FirstOne(temp);
    score+=qval_w[square];
/*
 ----------------------------------------------------------
|                                                          |
|   check to see if the queen is in a strong position on   |
|   the 7th rank supported by a rook on the 7th.  if so,   |
|   the positional advantage is almost overwhelming.       |
|                                                          |
 ----------------------------------------------------------
*/
    if (Rank(square)==RANK7 && (BlackPawns&rank_mask[RANK7] ||
         (BlackKingSQ > H7))) {
      if (AttacksRank(square)&WhiteRooks) score+=QUEEN_ROOK_ON_7TH_RANK;
    }
/*
 ----------------------------------------------------------
|                                                          |
|   adjust the white tropism count for this piece.         |
|                                                          |
 ----------------------------------------------------------
*/
    trop=7;
    if (!(WhitePawns&plus8dir[square]))
      trop=FileDistance(square,tree->b_kingsq);
    w_tropism+=Max(king_tropism_q[Distance(square,tree->b_kingsq)],
                   king_tropism_at_q[trop]);
/*
 ----------------------------------------------------------
|                                                          |
|   add in a bonus if the opponent's king position is not  |
|   safe, since the queen is a strong attacking piece.     |
|                                                          |
 ----------------------------------------------------------
*/
    if (tree->b_safety > tree->w_safety+4) score+=QUEEN_IS_STRONG;
/*
 ----------------------------------------------------------
|                                                          |
|   add in a penalty for being on the wrong side of the    |
|   board, which can lead to attacks by the opponent.      |
|                                                          |
 ----------------------------------------------------------
*/
    if (TotalWhitePawns > 4) {
      int offside=(File(square)<FILEC && File(tree->b_kingsq)>FILEE) ||
                  (File(square)>FILEF && File(tree->b_kingsq)<FILED);
      if (offside) {
        w_tropism-=QUEEN_OFFSIDE_TROPISM;
        w_tropism=Max(w_tropism,-8);
      }
    }
    Clear(square,temp);
  }
#ifdef DEBUGEV
  if (score != lastsc)
    printf("score[queens(white)]=             %4d (%+d)\n",score,score-lastsc);
  lastsc=score;
#endif
/*
 ----------------------------------------------------------
|                                                          |
|   black queens                                           |
|                                                          |
|   first locate each queen and obtain it's centralization |
|   score from the static piece/square table for queens.   |
|                                                          |
|   then, if the opposing side's king safety is much worse |
|   than the king safety for this side, add in a bonus to  |
|   keep the queen around.                                 |
|                                                          |
 ----------------------------------------------------------
*/
  temp=BlackQueens;
  while(temp) {
    square=FirstOne(temp);
    score-=qval_b[square];
/*
 ----------------------------------------------------------
|                                                          |
|   check to see if the queen is in a strong position on   |
|   the 7th rank supported by a rook on the 7th.  if so,   |
|   the positional advantage is almost overwhelming.       |
|                                                          |
 ----------------------------------------------------------
*/
    if (Rank(square)==RANK2 && (WhitePawns&rank_mask[RANK2] ||
         (WhiteKingSQ < A2))) {
      if (AttacksRank(square)&BlackRooks) score-=QUEEN_ROOK_ON_7TH_RANK;
    }
/*
 ----------------------------------------------------------
|                                                          |
|   adjust the black tropism count for this piece.         |
|                                                          |
 ----------------------------------------------------------
*/
    trop=7;
    if (!(BlackPawns&minus8dir[square]))
      trop=FileDistance(square,tree->w_kingsq);
    b_tropism+=Max(king_tropism_q[Distance(square,tree->w_kingsq)],
                   king_tropism_at_q[trop]);
/*
 ----------------------------------------------------------
|                                                          |
|   add in a bonus if the opponent's king position is not  |
|   safe, since the queen is a strong attacking piece.     |
|                                                          |
 ----------------------------------------------------------
*/
    if (tree->w_safety > tree->b_safety+4) score-=QUEEN_IS_STRONG;
/*
 ----------------------------------------------------------
|                                                          |
|   add in a penalty for being on the wrong side of the    |
|   board, which can lead to attacks by the opponent.      |
|                                                          |
 ----------------------------------------------------------
*/
    if (TotalBlackPawns > 4) {
      int offside=(File(square)<FILEC && File(tree->w_kingsq)>FILEE) ||
                  (File(square)>FILEF && File(tree->w_kingsq)<FILED);
      if (offside) {
        b_tropism-=QUEEN_OFFSIDE_TROPISM;
        b_tropism=Max(b_tropism,-8);
      }
     }
    Clear(square,temp);
  }
#ifdef DEBUGEV
  if (score != lastsc)
    printf("score[queens(black)]=             %4d (%+d)\n",score,score-lastsc);
  lastsc=score;
#endif
/*
 ----------------------------------------------------------
|                                                          |
|   now fold in the king tropism score, which take into    |
|   account _all_ pieces for each side that are close to   |
|   the opponent's king. the calculation uses three values |
|   for each side.  the 'total tropism' number, multiplied |
|   by the opponent's king safety, but this is first run   |
|   through an indirect table probe to get a multiplier    |
|   value.  the ttemper[] array provides a number between  |
|   16 and N, where 16 says use all the tropism numbers,   |
|   32 says to double the tropism scores (ie this array is |
|   specified in units of 1/16th).                         |
|                                                          |
 ----------------------------------------------------------
*/
  score+=((tropism[w_tropism+8]*ttemper[tree->b_safety])>>4)-
         ((tropism[b_tropism+8]*ttemper[tree->w_safety])>>4);
#ifdef DEBUGEV
  if (score != lastsc) {
    printf("score[king tropism]=              %4d (%+d)\n",score,score-lastsc);
    printf("w_safety=%d  b_safety=%d\n",tree->w_safety,tree->b_safety);
    printf("w_tropism=%d  b_tropism=%d\n",w_tropism,b_tropism);
    printf("wtrop.score=%d\n",(tropism[w_tropism+8]*ttemper[tree->b_safety])>>4);
    printf("btrop.score=%d\n",(tropism[b_tropism+8]*ttemper[tree->w_safety])>>4);
  }
  lastsc=score;
#endif
  if (abs(score-tscore) > largest_positional_score)
    largest_positional_score=abs(score-tscore);
/*
 ----------------------------------------------------------
|                                                          |
|   if the ending has only bishops of opposite colors, the |
|   score is pulled closer to a draw.  if the score says   |
|   one side is winning, but that side doesn't have enough |
|   material to win, the score is set to DRAW.             |
|                                                          |
 ----------------------------------------------------------
*/
  if (TotalWhitePieces<=11 && TotalBlackPieces<=11) {
    if (WhiteBishops && BlackBishops &&
        (!(WhiteBishops&(WhiteBishops-1)) &&
         !(BlackBishops&(BlackBishops-1)))) {
      if (square_color[FirstOne(BlackBishops)] !=
          square_color[FirstOne(WhiteBishops)]) {
        if (abs(Material) > 2*PAWN_VALUE) {
          if (TotalWhitePieces==3 && TotalBlackPieces==3)
            score=score>>1;
          else if (TotalWhitePieces==TotalBlackPieces)
            score=((score-Material)>>1)+Material;
        }
        else {
          if (TotalWhitePieces==3 && TotalBlackPieces==3)
            score=score>>2;
        }
      }
    }
  }
  if (drawn_ending < 0) {
    if (drawn_ending == -1 && score > 0) score=DrawScore(1);
    else if (drawn_ending == -2 && score < 0) score=DrawScore(1);
  }
#ifdef DEBUGEV
  if (score != lastsc)
    printf("score[draws]=                     %4d (%+d)\n",score,score-lastsc);
  lastsc=score;
#endif
  return((wtm) ? score : -score);
}

/* last modified 10/20/99 */
/*
********************************************************************************
*                                                                              *
*   EvaluateDevelopmentB() is used to encourage the program to develop its     *
*   pieces before moving its queen.  standard developmental principles are     *
*   applied.  they include:  (1) don't move the queen until minor pieces are   *
*   developed;  (2) advance the center pawns as soon as possible;  (3) don't   *
*   move the king unless its a castling move.                                  *
*                                                                              *
********************************************************************************
*/
int EvaluateDevelopmentB(TREE *tree, int ply) {
  register int possible, real, score=0;
/*
 ----------------------------------------------------------
|                                                          |
|   first, some "thematic" things, which includes don't    |
|   block the c-pawn in queen-pawn openings, then also     |
|   check to see if center pawns are blocked.              |
|                                                          |
 ----------------------------------------------------------
*/
  if (!SetMask(E4)&WhitePawns && SetMask(D4)&WhitePawns) {
    if (SetMask(C7)&BlackPawns &&
        SetMask(C6)&(BlackKnights|BlackBishops))
    score+=DEVELOPMENT_THEMATIC;
  }
  if (Occupied&((BlackPawns&rank_mask[RANK7])<<8)&
      (file_mask[FILED]|file_mask[FILEE]))
    score+=BLOCKED_CENTER_PAWN;
#ifdef DEBUGDV
  printf("developmentB.1 score=%d\n", score);
#endif
/*
 ----------------------------------------------------------
|                                                          |
|   if all minor pieces aren't developed, then penalize    |
|   the queen if it has moved.                             |
|                                                          |
 ----------------------------------------------------------
*/
  if (BlackCastle(ply)>0 &&
      !(BlackQueens&SetMask(D8))) score+=DEVELOPMENT_QUEEN_EARLY;
#ifdef DEBUGDV
  printf("developmentB.2 score=%d\n",score);
#endif
/*
 ----------------------------------------------------------
|                                                          |
|   if the king hasn't moved at the beginning of the       |
|   search, but it has moved somewhere in the current      |
|   search path, make *sure* it's a castle move or else    |
|   penalize the loss of castling privilege.               |
|                                                          |
|   if it *is* a castle move, penalize the king if it is   |
|   castling to one side when the other side is much safer |
|   but will it will take longer to prepare to castle in   |
|   that safer direction.                                  |
|                                                          |
|   the final test is to see if castling rights have been  |
|   lost due to moving one rook.  if so, check to see if   |
|   the remaining castle right would put the king in an    |
|   unsafe position which is just as bad.                  |
|                                                          |
 ----------------------------------------------------------
*/
  if (BlackCastle(1) == 3) {
    possible=0;
    real=0;
    if (BlackCastle(ply) < 0) {
      if (File(BlackKingSQ) >= FILEE) {
        real=tree->pawn_score.black_defects_k;
        possible=tree->pawn_score.black_defects_q;
      }
      else {
        real=tree->pawn_score.black_defects_q;
        possible=tree->pawn_score.black_defects_k;
      }
    }
    else if (BlackCastle(1)==3 && BlackCastle(ply)>0 && BlackCastle(ply)!=3) {
      if (BlackCastle(ply)&1) {
        real=tree->pawn_score.black_defects_k;
        possible=tree->pawn_score.black_defects_q;
      }
      else {
        real=tree->pawn_score.black_defects_q;
        possible=tree->pawn_score.black_defects_k;
      }
    }
    if (possible < real) score+=temper_b[2*(real-possible)];
  }
  if (BlackCastle(1) > 0) {
    if (BlackCastle(ply) == 0) {
      register int wq;
      wq=(WhiteQueens) ? 2 : 1;
      score+=(!root_wtm) ? 2*wq*DEVELOPMENT_LOSING_CASTLE :
                             wq*DEVELOPMENT_LOSING_CASTLE;
    }
    if (BlackCastle(ply) > 0) score+=DEVELOPMENT_NOT_CASTLED;
  }
#ifdef DEBUGDV
  printf("developmentB.3 score=%d\n",score);
#endif
  return(score);
}

/* last modified 10/20/99 */
/*
********************************************************************************
*                                                                              *
*   EvaluateDevelopment() is used to encourage the program to develop its      *
*   pieces before moving its queen.  standard developmental principles are     *
*   applied.  they include:  (1) don't move the queen until minor pieces are   *
*   developed;  (2) advance the center pawns as soon as possible;  (3) don't   *
*   move the king unless its a castling move.                                  *
*                                                                              *
********************************************************************************
*/
int EvaluateDevelopmentW(TREE *tree, int ply) {
  register int possible, real, score=0;
/*
 ----------------------------------------------------------
|                                                          |
|   first, some "thematic" things, which includes don't    |
|   block the c-pawn in queen-pawn openings, then also     |
|   check to see if center pawns are blocked.              |
|                                                          |
 ----------------------------------------------------------
*/
  if (!(SetMask(E4)&WhitePawns) && SetMask(D4)&WhitePawns) {
    if (SetMask(C2)&WhitePawns &&
        SetMask(C3)&(WhiteKnights|WhiteBishops))
    score-=DEVELOPMENT_THEMATIC;
  }
  if (Occupied&((WhitePawns&rank_mask[RANK2])>>8) &
                       (file_mask[FILED]|file_mask[FILEE]))
    score-=BLOCKED_CENTER_PAWN;
#ifdef DEBUGDV
  printf("developmentW.1 score=%d\n", score);
#endif
/*
 ----------------------------------------------------------
|                                                          |
|   if all minor pieces aren't developed, then penalize    |
|   the queen if it has moved.                             |
|                                                          |
 ----------------------------------------------------------
*/
  if (WhiteCastle(ply)>0 &&
      !(WhiteQueens&SetMask(D1))) score-=DEVELOPMENT_QUEEN_EARLY;
#ifdef DEBUGDV
  printf("developmentW.2 score=%d\n",score);
#endif
/*
 ----------------------------------------------------------
|                                                          |
|   if the king hasn't moved at the beginning of the       |
|   search, but it has moved somewhere in the current      |
|   search path, make *sure* it's a castle move or else    |
|   penalize the loss of castling privilege.               |
|                                                          |
|   if it *is* a castle move, penalize the king if it is   |
|   castling to one side when the other side is much safer |
|   but will it will take longer to prepare to castle in   |
|   that safer direction.                                  |
|                                                          |
|   the final test is to see if castling rights have been  |
|   lost due to moving one rook.  if so, check to see if   |
|   the remaining castle right would put the king in an    |
|   unsafe position which is just as bad.                  |
|                                                          |
 ----------------------------------------------------------
*/
  if (WhiteCastle(1) == 3) {
    possible=0;
    real=0;
    if (WhiteCastle(ply) < 0) {
      if (File(WhiteKingSQ) >= FILEE) {
        real=tree->pawn_score.white_defects_k;
        possible=tree->pawn_score.white_defects_q;
      }
      else {
        real=tree->pawn_score.white_defects_q;
        possible=tree->pawn_score.white_defects_k;
      }
    }
    else if (WhiteCastle(ply)>0 && WhiteCastle(ply)!=3) {
      if (WhiteCastle(ply)&1) {
        real=tree->pawn_score.white_defects_k;
        possible=tree->pawn_score.white_defects_q;
      }
      else {
        real=tree->pawn_score.white_defects_q;
        possible=tree->pawn_score.white_defects_k;
      }
    }
    if (possible < real) score-=temper_w[2*(real-possible)];
  }
  if (WhiteCastle(1) > 0) {
    register int bq;
    if (WhiteCastle(ply) == 0) {
      bq=(BlackQueens) ? 2 : 1;
      score-=(root_wtm) ? 2*bq*DEVELOPMENT_LOSING_CASTLE :
                            bq*DEVELOPMENT_LOSING_CASTLE;
    }
    if (WhiteCastle(ply) > 0) score-=DEVELOPMENT_NOT_CASTLED;
  }
#ifdef DEBUGDV
  printf("developmentW.3 score=%d\n",score);
#endif
  return(score);
}

/* last modified 03/12/01 */
/*
********************************************************************************
*                                                                              *
*   EvaluateDraws() is used to determine if one side (or both) are in a        *
*   position where winning is impossible.                                      *
*                                                                              *
********************************************************************************
*/
int EvaluateDraws(TREE *tree) {
/*
 ----------------------------------------------------------
|                                                          |
|   if lots of material is left, it's not a draw.          |
|                                                          |
 ----------------------------------------------------------
*/
  if (TotalWhitePieces > 8 || TotalBlackPieces >8) return(0);
  if (TotalWhitePieces==6 && WhiteBishops && TotalWhitePawns) return(0);
  if (TotalBlackPieces==6 && BlackBishops && TotalBlackPawns) return(0);
  if (TotalWhitePieces==0 && TotalBlackPieces==0) {
    if ((WhitePawns|BlackPawns)&not_rook_pawns) return(0);
  }
/*
 ----------------------------------------------------------
|                                                          |
|   if white has a bishop and pawn(s) then the pawn had    |
|   better not be only rook pawns, or else the bishop had  |
|   better be the right color, otherwise its a DRAW if the |
|   black king can block the pawn.                         |
|                                                          |
 ----------------------------------------------------------
*/
  if (TotalBlackPieces==0 && TotalWhitePawns && TotalWhitePieces<=3 &&
      !(WhitePawns&not_rook_pawns)) {
    if (WhiteBishops) {
      if (WhiteBishops&dark_squares) {
        if (file_mask[FILEH]&WhitePawns) return(0);
      }
      else if (file_mask[FILEA]&WhitePawns) return(0);
    }
    else if (TotalWhitePieces==0) {
      if (file_mask[FILEA]&WhitePawns &&
          file_mask[FILEH]&WhitePawns) return(0);
    }
    else return(0);

    if (!(WhitePawns&file_mask[FILEA]) ||
        !(WhitePawns&file_mask[FILEH])) {
      if (WhitePawns&file_mask[FILEA]) {
        int bkd, wkd, pd;
        bkd=Distance(BlackKingSQ,A8);
        wkd=Distance(WhiteKingSQ,A8);
        if (bkd <= 1) wkd++;
        pd=Distance(LastOne(WhitePawns&file_mask[FILEA]),A8);
        if (bkd<(wkd-wtm) && bkd<=(pd-wtm)) return(1);
        return(0);
      }
      else {
        int bkd, wkd, pd;
        bkd=Distance(BlackKingSQ,H8);
        wkd=Distance(WhiteKingSQ,H8);
        if (bkd <= 1) wkd++;
        pd=Distance(LastOne(WhitePawns&file_mask[FILEH]),H8);
        if (bkd<(wkd-wtm) && bkd<=(pd-wtm)) return(1);
        return(0);
      }
    }
  }
/*
 ----------------------------------------------------------
|                                                          |
|   if black has a bishop and pawn(s) then the pawn had    |
|   better not be only rook pawns, or else the bishop had  |
|   better be the right color, otherwise its a DRAW if the |
|   white king can block the pawn.                         |
|                                                          |
 ----------------------------------------------------------
*/
  if (TotalWhitePieces==0 && TotalBlackPawns && TotalBlackPieces<=3 &&
      !(BlackPawns&not_rook_pawns) && !(WhitePawns&not_rook_pawns)) {
    if (BlackBishops) {
      if (BlackBishops&dark_squares) {
        if (file_mask[FILEA]&BlackPawns) return(0);
      }
      else if (file_mask[FILEH]&BlackPawns) return(0);
    }
    else if (TotalBlackPieces==0) {
      if (file_mask[FILEA]&BlackPawns &&
          file_mask[FILEH]&BlackPawns) return(0);
    }
    else return(0);

    if (!(BlackPawns&file_mask[FILEA]) ||
        !(BlackPawns&file_mask[FILEH])) {
      if (BlackPawns&file_mask[FILEA]) {
        int bkd, wkd, pd;
        bkd=Distance(BlackKingSQ,A1);
        wkd=Distance(WhiteKingSQ,A1);
        if (wkd <= 1) bkd++;
        pd=Distance(FirstOne(BlackPawns&file_mask[FILEA]),A1);
        if (wkd<(bkd+wtm) && wkd<=(pd+wtm)) return(1);
        return(0);
      }
      else {
        int bkd, wkd, pd;
        bkd=Distance(BlackKingSQ,H1);
        wkd=Distance(WhiteKingSQ,H1);
        if (wkd <= 1) bkd++;
        pd=Distance(FirstOne(BlackPawns&file_mask[FILEH]),H1);
        if (wkd<(bkd+wtm) && wkd<=(pd+wtm)) return(1);
        return(0);
      }
    }
  }
/*
 ----------------------------------------------------------
|                                                          |
|   if both sides have pawns, the game is not a draw for   |
|   lack of material.  also, if one side has at least a    |
|   B+N, then it's not a drawn position.                   |
|                                                          |
|   if one side has a rook, while the other side has a     |
|   minor + pawns, then the rook can't possibly win.       |
|                                                          |
 ----------------------------------------------------------
*/
  if (TotalWhitePawns && TotalBlackPawns) return(0);
  if (TotalWhitePawns==0 && TotalWhitePieces==5 &&
      TotalBlackPieces==3) return(-1);
  if (TotalWhitePawns==0 && TotalWhitePieces==8 &&
      TotalBlackPieces==5) return(-1);
  if (TotalBlackPawns==0 && TotalBlackPieces==5 &&
      TotalWhitePieces==3) return(-2);
  if (TotalBlackPawns==0 && TotalBlackPieces==8 &&
      TotalWhitePieces==5) return(-2);
  if (TotalWhitePawns==0 && TotalWhitePieces<=6 &&
      TotalWhitePieces-TotalBlackPieces==3) return(-1);
  if (TotalBlackPawns==0 && TotalBlackPieces<=6 &&
      TotalBlackPieces-TotalWhitePieces==3) return(-2);
  if (TotalWhitePawns==0 && TotalWhitePieces<4) return(-1);
  else if (TotalBlackPawns==0 && TotalBlackPieces<4) return(-2);
  return(0);
}

/* last modified 12/17/99 */
/*
********************************************************************************
*                                                                              *
*   EvaluateMate() is used to evaluate positions where neither side has pawns  *
*   and one side has enough material to force checkmate.  it simply trys to    *
*   force the losing king to the edge of the board, and then to the corner     *
*   where mates are easier to find.                                            *
*                                                                              *
********************************************************************************
*/
int EvaluateMate(TREE *tree) {
  register int mate_score=DrawScore(1);
/*
 ----------------------------------------------------------
|                                                          |
|   if one side has a bishop+knight and the other side has |
|   no pieces or pawns, then use the special bishop_knight |
|   scoring board for the losing king to force it to the   |
|   right corner for mate.                                 |
|                                                          |
 ----------------------------------------------------------
*/
  if ((TotalBlackPieces==0) && (TotalWhitePieces==6) &&
      (!WhitePawns) && (!BlackPawns) && WhiteBishops && WhiteKnights) {
    if (dark_squares&WhiteBishops)
      mate_score=b_n_mate_dark_squares[BlackKingSQ];
    else
      mate_score=b_n_mate_light_squares[BlackKingSQ];
  }
  if ((TotalBlackPieces==6) && (TotalWhitePieces==0) &&
      (!WhitePawns) && (!BlackPawns) && BlackBishops && BlackKnights) {
    if (dark_squares&BlackBishops)
      mate_score=-b_n_mate_dark_squares[WhiteKingSQ];
    else
      mate_score=-b_n_mate_light_squares[WhiteKingSQ];
  }
  if (!mate_score) {
/*
 ----------------------------------------------------------
|                                                          |
|   if white is winning, force the black king to the edge  |
|   of the board.                                          |
|                                                          |
 ----------------------------------------------------------
*/
    if (Material >0) {
      mate_score=mate[BlackKingSQ];
      mate_score-=(Distance(WhiteKingSQ,BlackKingSQ)-3)*KING_KING_TROPISM;
    }
/*
 ----------------------------------------------------------
|                                                          |
|   if black is winning, force the white king to the edge  |
|   of the board.                                          |
|                                                          |
 ----------------------------------------------------------
*/
    else if (Material < 0) {
      mate_score=-mate[WhiteKingSQ];
      mate_score+=(Distance(WhiteKingSQ,BlackKingSQ)-3)*KING_KING_TROPISM;
    }
  }
  return(mate_score);
}

/* last modified 11/27/00 */
/*
********************************************************************************
*                                                                              *
*   EvaluateMaterial() is used to evaluate material on the board.  it really   *
*   accomplishes detecting cases where one side has made a 'bad trade' as the  *
*   comments below show.                                                       *
*                                                                              *
********************************************************************************
*/
int EvaluateMaterial(TREE *tree) {
  register int score;
/*
**********************************************************************
*                                                                    *
*   we start with the raw Material balance for the current position. *
*                                                                    *
**********************************************************************
*/
  score=Material;
/*
**********************************************************************
*                                                                    *
*   check 1.  if one side is a whole piece ahead, this is good, as a *
*   piece for 3 pawns is generally bad, unless the pawns are so far  *
*   advanced that the search can see them causing problems already.  *
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
  do {
    if (TotalWhitePieces<4 && TotalBlackPieces<4) break;
    if (WhiteMinors != BlackMinors) {
      if (WhiteMajors == BlackMajors) {
        if (WhiteMinors > BlackMinors)
          score+=BAD_TRADE;
        else
          score-=BAD_TRADE;
      }
      else if (abs(WhiteMajors-BlackMajors) == 1) {
        if (WhiteMinors > BlackMinors+1) score+=BAD_TRADE;
        else if (BlackMinors > WhiteMinors+1) score-=BAD_TRADE;
      }
      else if (abs(WhiteMajors-BlackMajors) == 2) {
        if (WhiteMinors > BlackMinors+2) score+=BAD_TRADE;
        else if (BlackMinors > WhiteMinors+2) score-=BAD_TRADE;
      }
      break;
    }
    else if (WhiteMajors == BlackMajors) {
      if (WhiteQueens && !BlackQueens) score+=BAD_TRADE>>1;
      else if (!WhiteQueens && BlackQueens) score-=BAD_TRADE>>1;
    }
    else {
      if (WhiteMajors>BlackMajors && WhiteMinors==BlackMinors) score+=BAD_TRADE;
      else if (BlackMajors>WhiteMajors && WhiteMinors==BlackMinors) score-=BAD_TRADE;
    }
  } while(0);
#ifdef DEBUGM
  printf("score[material]=                  %4d\n",Material);
  printf("score[bad trade]=                 %4d\n",score);
#endif
  return (score);
}

/* last modified 12/08/99 */
/*
********************************************************************************
*                                                                              *
*   EvaluateKingSafety() is used to evaluate king safety for both sides, based *
*   on the pawns around the king and the material left on the board.           *
*                                                                              *
********************************************************************************
*/
int EvaluateKingSafety(TREE *tree, int ply) {
  int score=0;
/*
 ----------------------------------------------------------
|                                                          |
|   first, check for the "trojan horse" attack where the   |
|   opponent offers a piece to open the h-file with a very |
|   difficult to refute attack.                            |
|                                                          |
 ----------------------------------------------------------
*/
  if (trojan_check) {
    if (root_wtm && File(WhiteKingSQ) >= FILEE) {
      if (!(tree->all_pawns&file_mask[FILEH])) {
        if (BlackRooks && BlackQueens) score-=KING_SAFETY_MATE_THREAT;
      }
    }
    if (!root_wtm && File(BlackKingSQ) >= FILEE) {
      if (!(tree->all_pawns&file_mask[FILEH])) {
        if (WhiteRooks && WhiteQueens) score+=KING_SAFETY_MATE_THREAT;
      }
    }
  }
/*
 ----------------------------------------------------------
|                                                          |
|   Now do normal scoring, if the king has castled, the    |
|   pawns in front are important.  If not castled yet, the |
|   pawns on the kingside should be preserved for this.    |
|                                                          |
 ----------------------------------------------------------
*/
  tree->w_safety=king_defects_w[WhiteKingSQ];
  if (WhiteCastle(ply) <= 0) {
    if (File(WhiteKingSQ) >= FILEE) {
      if (File(WhiteKingSQ) == FILEH) tree->w_kingsq&=62;
      tree->w_safety+=tree->pawn_score.white_defects_k;
      if (!(WhitePawns&SetMask(G2))) {
        if (SetMask(F3)&(BlackPawns|BlackBishops) && BlackQueens)
          tree->w_safety+=KING_SAFETY_MATE_G2G7;
      }
      if (File(WhiteKingSQ)==FILEE) {
        if (!(tree->all_pawns&file_mask[FILED]))
          tree->w_safety+=KING_SAFETY_OPEN_FILE>>1;
        if (!(tree->all_pawns&file_mask[FILEE]))
          tree->w_safety+=KING_SAFETY_OPEN_FILE;
        if (!(tree->all_pawns&file_mask[FILEF]))
          tree->w_safety+=KING_SAFETY_OPEN_FILE>>1;
      }
    }
    else if (File(WhiteKingSQ) <= FILED) {
      if (File(WhiteKingSQ) == FILEA) tree->w_kingsq|=1;
      tree->w_safety+=tree->pawn_score.white_defects_q;
      if (!(WhitePawns&SetMask(B2))) {
        if (SetMask(C3)&(BlackPawns|BlackBishops) && BlackQueens)
          tree->w_safety+=KING_SAFETY_MATE_G2G7;
      }
      if (File(WhiteKingSQ)==FILED) {
        if (!(tree->all_pawns&file_mask[FILEC]))
          tree->w_safety+=KING_SAFETY_OPEN_FILE>>1;
        if (!(tree->all_pawns&file_mask[FILED]))
          tree->w_safety+=KING_SAFETY_OPEN_FILE;
        if (!(tree->all_pawns&file_mask[FILEE]))
          tree->w_safety+=KING_SAFETY_OPEN_FILE>>1;
      }
    }
  }
  else {
    if (WhiteCastle(ply) != 3) {
      if (WhiteCastle(ply)&1)
        tree->w_safety+=tree->pawn_score.white_defects_k>>1;
      else if (WhiteCastle(ply)&2)
        tree->w_safety+=tree->pawn_score.white_defects_q>>1;
    }
    if (!(tree->all_pawns&file_mask[FILED]))
      tree->w_safety+=KING_SAFETY_OPEN_FILE>>1;
    if (!(tree->all_pawns&file_mask[FILEE]))
      tree->w_safety+=KING_SAFETY_OPEN_FILE;
    if (!(tree->all_pawns&file_mask[FILEF]))
      tree->w_safety+=KING_SAFETY_OPEN_FILE>>1;
  }

  tree->b_safety=king_defects_b[BlackKingSQ];
  if (BlackCastle(ply) <= 0) {
    if (File(BlackKingSQ) >= FILEE) {
      if (File(BlackKingSQ) == FILEH) tree->b_kingsq&=62;
      tree->b_safety+=tree->pawn_score.black_defects_k;
      if (!(BlackPawns&SetMask(G7))) {
        if (SetMask(F6)&(WhitePawns|WhiteBishops) && WhiteQueens)
          tree->b_safety+=KING_SAFETY_MATE_G2G7;
      }
      if (File(BlackKingSQ)==FILEE) {
        if (!(tree->all_pawns&file_mask[FILED]))
          tree->b_safety+=KING_SAFETY_OPEN_FILE>>1;
        if (!(tree->all_pawns&file_mask[FILEE]))
          tree->b_safety+=KING_SAFETY_OPEN_FILE;
        if (!(tree->all_pawns&file_mask[FILEF]))
          tree->b_safety+=KING_SAFETY_OPEN_FILE>>1;
      }
    }
    else if (File(BlackKingSQ) <= FILED) {
      if (File(BlackKingSQ) == FILEA) tree->b_kingsq|=1;
      tree->b_safety+=tree->pawn_score.black_defects_q;
      if (!(BlackPawns&SetMask(B7))) {
        if (SetMask(C6)&(WhitePawns|WhiteBishops) && WhiteQueens)
          tree->b_safety+=KING_SAFETY_MATE_G2G7;
      }
      if (File(BlackKingSQ)==FILED) {
        if (!(tree->all_pawns&file_mask[FILEC]))
          tree->b_safety+=KING_SAFETY_OPEN_FILE>>1;
        if (!(tree->all_pawns&file_mask[FILED]))
          tree->b_safety+=KING_SAFETY_OPEN_FILE;
        if (!(tree->all_pawns&file_mask[FILEE]))
          tree->b_safety+=KING_SAFETY_OPEN_FILE>>1;
      }
    }
  }
  else {
    if (BlackCastle(ply) != 3) {
      if (BlackCastle(ply)&1)
        tree->b_safety+=tree->pawn_score.black_defects_k>>1;
      else if (BlackCastle(ply)&2)
        tree->b_safety+=tree->pawn_score.black_defects_q>>1;
    }
    if (!(tree->all_pawns&file_mask[FILED]))
      tree->b_safety+=KING_SAFETY_OPEN_FILE>>1;
    if (!(tree->all_pawns&file_mask[FILEE]))
      tree->b_safety+=KING_SAFETY_OPEN_FILE;
    if (!(tree->all_pawns&file_mask[FILEF]))
      tree->b_safety+=KING_SAFETY_OPEN_FILE>>1;
  }
  if (!WhiteQueens) tree->b_safety>>=1;
  if (!BlackQueens) tree->w_safety>>=1;
  score-=temper_w[tree->w_safety]-temper_b[tree->b_safety];
  return(score);
}

/* last modified 03/11/98 */
/*
********************************************************************************
*                                                                              *
*   EvaluatePassedPawns() is used to evaluate passed pawns and the danger      *
*   they produce.  the first bonus is for a passed pawn that has reached the   *
*   6th rank and is supported by the king, making it very difficult to stop it *
*   from queening.  the second case is two connected passed pawns on the 6th   *
*   or 7th rank, with the opposing side having little material to stop them.   *
*                                                                              *
********************************************************************************
*/
int EvaluatePassedPawns(TREE *tree) {
  register int file, square, score=0;
  register int white_king_sq, black_king_sq;
  register int pawns;

/*
 ----------------------------------------------------------
|                                                          |
|   check to see if black has any passed pawns.  if so,    |
|   and the king supports the pawn, then the pawn is even  |
|   more valuable.  at the same time, check to see if the  |
|   is blockaded by an enemy piece.  if so then the pawn   |
|   is less valuable since it can't advance easily.  as    |
|   material is removed, passed pawns also become more     |
|   valuable.                                              |
|                                                          |
 ----------------------------------------------------------
*/
  if (tree->pawn_score.passed_b) {
    black_king_sq=BlackKingSQ;
    pawns=tree->pawn_score.passed_b;
    while (pawns) {
      file=first_ones_8bit[pawns];
      pawns&=~(128>>file);
      square=FirstOne(BlackPawns&file_mask[file]);
      if (FileDistance(square,black_king_sq)==1 &&
          Rank(black_king_sq)<=Rank(square))
        score-=supported_passer[RANK8-Rank(square)];
      if (SetMask(square-8)&Occupied) {
        score+=blockading_passed_pawn_value[RANK8-Rank(square)];
      }
    }
#ifdef DEBUGPP
  printf("score.1 after black passers = %d\n", score);
#endif
/*
 ----------------------------------------------------------
|                                                          |
|   check to see if black has any connected passed pawns.  |
|   if so, and they have both reached the 6th/7th rank,    |
|   then they are very dangerous.                          |
|                                                          |
 ----------------------------------------------------------
*/
    pawns=tree->pawn_score.passed_b;
    while ((file=connected_passed[pawns])) {
      register int square1,square2;
      pawns&=~(128>>file);
      square1=FirstOne(BlackPawns&file_mask[file-1]);
      square2=FirstOne(BlackPawns&file_mask[file]);
      score-=connected_passed_pawn_value[7-Max(Rank(square1),Rank(square2))];
      if (Rank(square1) > RANK3) continue;
      if (Rank(square2) > RANK3) continue;
      if (TotalWhitePieces < queen_v &&
          !(SetMask(square1-8)&WhitePieces) &&
          !(SetMask(square2-8)&WhitePieces)) {
        score-=2*PAWN_CONNECTED_PASSED_6TH;
        if (wtm) {
          if (!(WhiteKing&black_pawn_race_wtm[square1]) &&
              !(WhiteKing&black_pawn_race_wtm[square2]))
            score-=2*PAWN_CONNECTED_PASSED_6TH;
        }
        else {
          if (!(WhiteKing&black_pawn_race_btm[square1]) ||
              !(WhiteKing&black_pawn_race_btm[square2]))
            score-=2*PAWN_CONNECTED_PASSED_6TH;
        }
      }
    }
  }
#ifdef DEBUGPP
  printf("score.2 after black passers = %d\n", score);
#endif
/*
 ----------------------------------------------------------
|                                                          |
|   check to see if white has any passed pawns.  if so,    |
|   and the king supports the pawn, then the pawn is even  |
|   more valuable.  at the same time, check to see if the  |
|   is blockaded by an enemy piece.  if so then the pawn   |
|   is less valuable since it can't advance easily.  as    |
|   material is removed, passed pawns also become more     |
|   valuable.                                              |
|                                                          |
 ----------------------------------------------------------
*/
  if (tree->pawn_score.passed_w) {
    white_king_sq=WhiteKingSQ;
    pawns=tree->pawn_score.passed_w;
    while (pawns) {
      file=first_ones_8bit[pawns];
      pawns&=~(128>>file);
      square=LastOne(WhitePawns&file_mask[file]);
      if (FileDistance(square,white_king_sq)==1 &&
          Rank(white_king_sq)>=Rank(square))
        score+=supported_passer[Rank(square)];
      if (SetMask(square+8)&Occupied) {
        score-=blockading_passed_pawn_value[Rank(square)];
      }
    }
#ifdef DEBUGPP
  printf("score.1 after white passers = %d\n", score);
#endif
/*
 ----------------------------------------------------------
|                                                          |
|   check to see if white has any connected passed pawns.  |
|   if so, and they have both reached the 6th/7th rank,    |
|   then they are very dangerous.                          |
|                                                          |
 ----------------------------------------------------------
*/
    pawns=tree->pawn_score.passed_w;
    while ((file=connected_passed[pawns])) {
      register int square1,square2;
      pawns&=~(128>>file);
      square1=LastOne(WhitePawns&file_mask[file-1]);
      square2=LastOne(WhitePawns&file_mask[file]);
      score+=connected_passed_pawn_value[Min(Rank(square1),Rank(square2))];
      if (Rank(square1) < RANK6) continue;
      if (Rank(square2) < RANK6) continue;
      if (TotalBlackPieces < queen_v &&
          !(SetMask(square1+8)&BlackPieces) &&
          !(SetMask(square2+8)&BlackPieces)) {
        score+=2*PAWN_CONNECTED_PASSED_6TH;
        if (wtm) {
          if (!(BlackKing&white_pawn_race_wtm[square1]) &&
              !(BlackKing&white_pawn_race_wtm[square2]))
            score+=2*PAWN_CONNECTED_PASSED_6TH;
        }
        else {
          if (!(BlackKing&white_pawn_race_btm[square1]) ||
              !(BlackKing&white_pawn_race_btm[square2]))
            score+=2*PAWN_CONNECTED_PASSED_6TH;
        }
      }
    }
  }
#ifdef DEBUGPP
  printf("score.2 after white passers = %d\n", score);
#endif
  if (TotalBlackPawns==1 && TotalWhitePawns==0 &&
      TotalBlackPieces==5 && TotalWhitePieces==5) {
    square=FirstOne(BlackPawns);
    if (FileDistance(WhiteKingSQ,square)<=1 &&
        Rank(WhiteKingSQ)<Rank(square)) return(0);
    if (Rank(BlackKingSQ)>Rank(square) ||
        FileDistance(BlackKingSQ,square) > 1) return(0);
  }
  if (TotalWhitePawns==1 && TotalBlackPawns==0 &&
      TotalWhitePieces==5 && TotalBlackPieces==5) {
    square=FirstOne(WhitePawns);
    if (FileDistance(BlackKingSQ,square)<=1 &&
        Rank(BlackKingSQ)>Rank(square)) return(0);
    if (Rank(WhiteKingSQ)<Rank(square) ||
        FileDistance(WhiteKingSQ,square) > 1) return(0);
  }
  return(score);
}

/* last modified 03/11/98 */
/*
********************************************************************************
*                                                                              *
*   EvaluatePassedPawnRaces() is used to evalaute passed pawns when one        *
*   side has passed pawns and the other side (or neither) has pieces.  in      *
*   such a case, the critical question is can the defending king stop the pawn *
*   from queening or is it too far away?  if only one side has pawns that can  *
*   "run" then the situation is simple.  when both sides have pawns that can   *
*   "run" it becomes more complex as it then becomes necessary to see if       *
*   pawn queens with check, or if either pawn queens and simultaneously        *
*   attacks the opposing side's queening square.  for the special case of a    *
*   single pawn, the simple evaluation rules are used:  king two squares in    *
*   front of the pawn=win, one square in front with opposition=win,  king on   *
*   6th pawn close by is a win.  rook pawns are handled separately and are     *
*   harder to queen.                                                           *
*                                                                              *
********************************************************************************
*/
int EvaluatePassedPawnRaces(TREE *tree, int wtm) {
  register int file, square;
  register int white_queener=8, white_square=0;
  register int black_queener=8, black_square=0;
  register int white_pawn=0, black_pawn=0, queen_distance;
  register int pawnsq;
  register BITBOARD tempw, tempb;
  register int passed;

/*
 ----------------------------------------------------------
|                                                          |
|   check to see if white has one pawn and neither side    |
|   has any pieces.  if so, use the simple pawn evaluation |
|   logic.                                                 |
|                                                          |
 ----------------------------------------------------------
*/
  if (WhitePawns && !BlackPawns &&
      !TotalWhitePieces && !TotalBlackPieces) do {
    pawnsq=LastOne(WhitePawns);
/*
 ------------------------------------------------
|                                                |
|   king must be in front of the pawn or we      |
|   go no further.                               |
|                                                |
 ------------------------------------------------
*/
    if (Rank(WhiteKingSQ) <= Rank(pawnsq)) break;
/*
 ------------------------------------------------
|                                                |
|   first a special case.  if this is a rook     |
|   pawn, then the king must be on the adjacent  |
|   file, and be closer to the queening square   |
|   than the opposing king.                      |
|                                                |
 ------------------------------------------------
*/
    if (File(pawnsq) == FILEA) {
      if ((File(WhiteKingSQ) == FILEB) &&
          (Distance(WhiteKingSQ,A8) < Distance(BlackKingSQ,A8)))
        return(QUEEN_VALUE-BISHOP_VALUE);
      break;
    }
    else if (File(pawnsq) == FILEH) {
      if ((File(WhiteKingSQ) == FILEG) &&
          (Distance(WhiteKingSQ,H8) < Distance(BlackKingSQ,H8)))
        return(QUEEN_VALUE-BISHOP_VALUE);
      break;
    }
/*
 ------------------------------------------------
|                                                |
|   if king is two squares in front of the pawn  |
|   then it's a win immediately.  if the king is |
|   on the 6th rank and closer to the pawn than  |
|   the opposing king, it's also a win.          |
|                                                |
 ------------------------------------------------
*/
    if (Distance(WhiteKingSQ,pawnsq) < Distance(BlackKingSQ,pawnsq)) {
      if (Rank(WhiteKingSQ) > Rank(pawnsq)+1)
        return(QUEEN_VALUE-BISHOP_VALUE);
      if (Rank(WhiteKingSQ) == RANK6)
        return(QUEEN_VALUE-BISHOP_VALUE);
    }
/*
 ------------------------------------------------
|                                                |
|   last chance:  if the king is one square in   |
|   front of the pawn and has the opposition,    |
|   then it's still a win.                       |
|                                                |
 ------------------------------------------------
*/
    if ((Rank(WhiteKingSQ) == Rank(pawnsq)+1) &&
        HasOpposition(wtm,WhiteKingSQ,BlackKingSQ))
      return(QUEEN_VALUE-BISHOP_VALUE);
  } while(0);
/*
 ----------------------------------------------------------
|                                                          |
|   check to see if black has one pawn and neither side    |
|   has any pieces.  if so, use the simple pawn evaluation |
|   logic.                                                 |
|                                                          |
 ----------------------------------------------------------
*/
  if (BlackPawns && !WhitePawns &&
      !TotalWhitePieces && !TotalBlackPieces) do {
    pawnsq=FirstOne(BlackPawns);
/*
 ------------------------------------------------
|                                                |
|   king must be in front of the pawn or we      |
|   go no further.                               |
|                                                |
 ------------------------------------------------
*/
    if (Rank(BlackKingSQ) >= Rank(pawnsq)) break;
/*
 ------------------------------------------------
|                                                |
|   first a special case.  if this is a rook     |
|   pawn, then the king must be on the adjacent  |
|   file, and be closer to the queening square   |
|   than the opposing king.                      |
|                                                |
 ------------------------------------------------
*/
    if (File(pawnsq) == FILEA) {
      if ((File(BlackKingSQ) == FILEB) &&
          (Distance(BlackKingSQ,A1) < Distance(WhiteKingSQ,A1)))
        return(-(QUEEN_VALUE-BISHOP_VALUE));
      break;
    }
    else if (File(pawnsq) == FILEH) {
      if ((File(BlackKingSQ) == FILEG) &&
          (Distance(BlackKingSQ,H1) < Distance(WhiteKingSQ,H1)))
        return(-(QUEEN_VALUE-BISHOP_VALUE));
      break;
    }
/*
 ------------------------------------------------
|                                                |
|   if king is two squares in front of the pawn  |
|   then it's a win immediately.  if the king is |
|   on the 6th rank and closer to the pawn than  |
|   the opposing king, it's also a win.          |
|                                                |
 ------------------------------------------------
*/
    if (Distance(BlackKingSQ,pawnsq) < Distance(WhiteKingSQ,pawnsq)) {
      if (Rank(BlackKingSQ) < Rank(pawnsq)-1)
        return(-(QUEEN_VALUE-BISHOP_VALUE));
      if (Rank(BlackKingSQ) == RANK3)
        return(-(QUEEN_VALUE-BISHOP_VALUE));
    }
/*
 ------------------------------------------------
|                                                |
|   last chance:  if the king is one square in   |
|   front of the pawn and has the opposition,    |
|   then it's still a win.                       |
|                                                |
 ------------------------------------------------
*/
    if ((Rank(BlackKingSQ) == Rank(pawnsq)-1) &&
        HasOpposition(ChangeSide(wtm),BlackKingSQ,WhiteKingSQ))
      return(-(QUEEN_VALUE-BISHOP_VALUE));
  } while(0);
/*
 ----------------------------------------------------------
|                                                          |
|   check to see if white is out of pieces and black has   |
|   passed pawns.  if so, see if any of these passed pawns |
|   can outrun the defending king and promote.             |
|                                                          |
 ----------------------------------------------------------
*/
  if (!TotalWhitePieces && tree->pawn_score.passed_b) {
    passed=tree->pawn_score.passed_b;
    while ((file=first_ones_8bit[passed]) != 8) {
      passed&=~(128>>file);
      square=FirstOne(BlackPawns&file_mask[file]);
      if ((wtm && !(black_pawn_race_wtm[square]&WhiteKing)) ||
          (ChangeSide(wtm) && !(black_pawn_race_btm[square]&WhiteKing))) {
        queen_distance=Rank(square);
        if (BlackKing&minus8dir[square]) {
          if (file==FILEA || file==FILEH) queen_distance=99;
          queen_distance++;
        }
        if (Rank(square) == RANK7) queen_distance--;
        if (queen_distance < black_queener) {
          black_queener=queen_distance;
          black_square=file;
          black_pawn=square;
        }
      }
    }
  }
#ifdef DEBUGPP
  printf("black pawn on %d can promote at %d in %d moves.\n",
         black_pawn,black_square,black_queener);
#endif
/*
 ----------------------------------------------------------
|                                                          |
|   check to see if black is out of pieces and white has   |
|   passed pawns.  if so, see if any of these passed pawns |
|   can outrun the defending king and promote.             |
|                                                          |
 ----------------------------------------------------------
*/
  if (!TotalBlackPieces && tree->pawn_score.passed_w) {
    passed=tree->pawn_score.passed_w;
    while ((file=first_ones_8bit[passed]) != 8) {
      passed&=~(128>>file);
      square=LastOne(WhitePawns&file_mask[file]);
      if ((wtm && !(white_pawn_race_wtm[square]&BlackKing)) ||
          (ChangeSide(wtm) && !(white_pawn_race_btm[square]&BlackKing))) {
        queen_distance=RANK8-Rank(square);
        if (WhiteKing&plus8dir[square]) {
          if (file==FILEA || file==FILEH) queen_distance=99;
          queen_distance++;
        }
        if (Rank(square) == RANK2) queen_distance--;
        if (queen_distance < white_queener) {
          white_queener=queen_distance;
          white_square=file+A8;
          white_pawn=square;
        }
      }
    }
  }
#ifdef DEBUGPP
  printf("white pawn on %d can promote at %d in %d moves.\n",
         white_pawn,white_square,white_queener);
#endif

  do {
    if ((white_queener==8) && (black_queener==8)) break;
/*
 ----------------------------------------------------------
|                                                          |
|   now that we know which pawns can outrun the kings for  |
|   each side, we need to do the following:  if one side   |
|   queens before the other (two moves or more) then that  |
|   side wins.                                             |
|                                                          |
 ----------------------------------------------------------
*/
    if ((white_queener < 8) && (black_queener == 8))
      return(QUEEN_VALUE-BISHOP_VALUE+(5-white_queener)*PAWN_VALUE/10);
    else if ((black_queener < 8) && (white_queener == 8))
      return(-(QUEEN_VALUE-BISHOP_VALUE+(5-black_queener)*PAWN_VALUE/10));
    if (ChangeSide(wtm)) black_queener--;
    if (white_queener < black_queener)
      return(QUEEN_VALUE-BISHOP_VALUE+(5-white_queener)*PAWN_VALUE/10);
    else if (black_queener < white_queener-1)
      return(-(QUEEN_VALUE-BISHOP_VALUE+(5-black_queener)*PAWN_VALUE/10));
    if ((white_queener==8) || (black_queener==8)) break;
/*
 ----------------------------------------------------------
|                                                          |
|   if the white pawn queens one move before black, then   |
|   if the new queen checks the black king, or the new     |
|   queen attacks the queening square of black, white wins |
|   unless the black king is protecting the black queening |
|   square in which case it's a draw.                      |
|                                                          |
 ----------------------------------------------------------
*/
    if (white_queener == black_queener) {
      tempw=WhitePieces;
      Clear(white_pawn,WhitePieces);
      WhitePieces=WhitePieces|SetMask(white_square);
      tempb=BlackPieces;
      Clear(black_pawn,BlackPieces);
      BlackPieces=BlackPieces|SetMask(black_square);
      if (Attack(BlackKingSQ,white_square)) {
        WhitePieces=tempw;
        BlackPieces=tempb;
        return(QUEEN_VALUE-BISHOP_VALUE+(5-white_queener)*PAWN_VALUE/10);
      }
      if (Attack(black_square,white_square) &&
          !(king_attacks[black_square]&BlackKing)) {
        WhitePieces=tempw;
        BlackPieces=tempb;
        return(QUEEN_VALUE-BISHOP_VALUE+(5-white_queener)*PAWN_VALUE/10);
      }
      WhitePieces=tempw;
      BlackPieces=tempb;
    }
/*
 ----------------------------------------------------------
|                                                          |
|   if the black pawn queens one move before white, then   |
|   if the new queen checks the white king, or the new     |
|   queen attacks the queening square of white, black wins |
|   unless the white king is protecting the white queening |
|   square in which case it's a draw.                      |
|                                                          |
 ----------------------------------------------------------
*/
    if (black_queener == white_queener-1) {
      tempw=WhitePieces;
      Clear(white_pawn,WhitePieces);
      WhitePieces=WhitePieces|SetMask(white_square);
      tempb=BlackPieces;
      Clear(black_pawn,BlackPieces);
      BlackPieces=BlackPieces|SetMask(black_square);
      if (Attack(WhiteKingSQ,black_square)) {
        WhitePieces=tempw;
        BlackPieces=tempb;
        return(-(QUEEN_VALUE-BISHOP_VALUE+(5-black_queener)*PAWN_VALUE/10));
      }
      if (Attack(white_square,black_square) &&
          !(king_attacks[white_square]&WhiteKing)) {
        WhitePieces=tempw;
        BlackPieces=tempb;
        return(-(QUEEN_VALUE-BISHOP_VALUE+(5-black_queener)*PAWN_VALUE/10));
      }
      WhitePieces=tempw;
      BlackPieces=tempb;
    }
  } while(0);
  return(0);
}

/* last modified 12/15/00 */
/*
********************************************************************************
*                                                                              *
*   EvaluatePawns() is used to evaluate pawns.  broadly, it addresses three    *
*   distinct actions:  (1) basic pawn scoring is hashed, the first thing done  *
*   is to see if the score is in the pawn hash table;  (2) passed pawn scoring *
*   for positions with pieces left;  (3) passed pawns where one side has a     *
*   passer and the other side has no pieces, so that the pawn can potentially  *
*   outrun the opposing king and promote.                                      *
*                                                                              *
********************************************************************************
*/
int EvaluatePawns(TREE *tree) {
  register PAWN_HASH_ENTRY *ptable;
  register BITBOARD pawns;
  register BITBOARD temp, left, right;
  register BITBOARD wp_moves, bp_moves;
  register int score=0;
  register int pns, square, file;
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
  int lastsc=0;
#endif
/*
 ----------------------------------------------------------
|                                                          |
|   first check to see if this position has been handled   |
|   before.  if so, we can skip the work saved in the pawn |
|   hash table.                                            |
|                                                          |
 ----------------------------------------------------------
*/
  ptable=pawn_hash_table+(PawnHashKey&pawn_hash_mask);
  if (ptable->key == PawnHashKey) {
#if !defined(FAST)
    tree->pawn_hits++;
#endif
    tree->pawn_score=*ptable;
    return(tree->pawn_score.p_score);
  }
  tree->pawn_score.key=PawnHashKey;
  tree->pawn_score.passed_w=0;
  tree->pawn_score.passed_b=0;
  tree->pawn_score.protected=0;
  tree->pawn_score.white_defects_k=0;
  tree->pawn_score.white_defects_q=0;
  tree->pawn_score.black_defects_k=0;
  tree->pawn_score.black_defects_q=0;
  tree->pawn_score.outside=0;
  tree->pawn_score.candidates_w=0;
  tree->pawn_score.candidates_b=0;
  tree->pawn_score.allw=0;
  tree->pawn_score.allb=0;
  w_isolated=0;
  w_isolated_of=0;
  b_isolated=0;
  b_isolated_of=0;
  w_unblocked=0;
  b_unblocked=0;
  kside_open_files=0;
  qside_open_files=0;
  kside_half_open_files_b=0;
  kside_half_open_files_w=0;
  qside_half_open_files_b=0;
  qside_half_open_files_w=0;
  qmissb=0;
  kmissb=0;
  qmissw=0;
  kmissw=0;
/*
 ----------------------------------------------------------
|                                                          |
|   if there are 8 pawns of one color, penalize crafty     |
|   for not opening the position.                          |
|                                                          |
 ----------------------------------------------------------
*/
  if (root_wtm) {
    if (TotalWhitePawns == 8) score-=EIGHT_PAWNS;
  }
  else {
    if (TotalBlackPawns == 8) score+=EIGHT_PAWNS;
  }
/*
 ----------------------------------------------------------
|                                                          |
|   first, determine which squares pawns can reach.        |
|                                                          |
 ----------------------------------------------------------
*/
  pawns=WhitePawns;
  wp_moves=0;
  while (pawns) {
    square=FirstOne(pawns);
    tree->pawn_score.allw|=128>>File(square);
    for (sq=square;sq<A7;sq+=8) {
      wp_moves|=SetMask(sq);
      if (SetMask(sq+8)&tree->all_pawns) break;
      defenders=PopCnt(b_pawn_attacks[sq+8]&WhitePawns);
      attackers=PopCnt(w_pawn_attacks[sq+8]&BlackPawns);
      if (attackers-defenders > 0) break;
    }
    Clear(square,pawns);
  }
  pawns=BlackPawns;
  bp_moves=0;
  while (pawns) {
    square=FirstOne(pawns);
    tree->pawn_score.allb|=128>>File(square);
    for (sq=square;sq>H2;sq-=8) {
      bp_moves|=SetMask(sq);
      if (SetMask(sq-8)&tree->all_pawns) break;
      attackers=PopCnt(b_pawn_attacks[sq-8]&WhitePawns);
      defenders=PopCnt(w_pawn_attacks[sq-8]&BlackPawns);
      if (attackers-defenders > 0) break;
    }
    Clear(square,pawns);
  }
/*
 ----------------------------------------------------------
|                                                          |
|   white pawns.                                           |
|                                                          |
 ----------------------------------------------------------
*/
  pawns=WhitePawns;
  while (pawns) {
    square=LastOne(pawns);
    file=File(square);
/*
 ----------------------------------------------------------
|                                                          |
|   evaluate pawn advances.  center pawns are encouraged   |
|   to advance, while wing pawns are pretty much neutral.  |
|                                                          |
 ----------------------------------------------------------
*/
    score+=pval_w[square];
#ifdef DEBUGP
    if (score != lastsc)
      printf("white pawn[static] file=%d,   score=%d\n", file,score);
    lastsc=score;
#endif
/*
 ----------------------------------------------------------
|                                                          |
|   evaluate weak pawns.  weak pawns are evaluated by the  |
|   following rules:  (1) if a pawn is defended by a pawn, |
|   it isn't weak;  (2) if a pawn is undefended by a pawn  |
|   and advances one (or two if it hasn't moved yet) ranks |
|   and is defended fewer times than it is attacked, it is |
|   weak.  note that the penalty is greater if the pawn is |
|   on an open file.  note that an isolated pawn is just   |
|   another case of a weak pawn, since it can never be     |
|   defended by a pawn.                                    |
|                                                          |
|   note that an isolated pawn on an open file is counted  |
|   twice as it is much easier to attack and win such a    |
|   pawn.                                                  |
|                                                          |
 ----------------------------------------------------------
*/
    if (!(mask_pawn_isolated[square]&WhitePawns)) {
      w_isolated++;
      if (!(plus8dir[square]&BlackPawns)) w_isolated_of++;
    }
    else do {
/*
 ----------------------------------------------------------
|                                                          |
|  test the pawn to see it if forms a "duo" which is two   |
|  pawns side-by-side.                                     |
|                                                          |
 ----------------------------------------------------------
*/
      if (mask_pawn_duo[square]&WhitePawns)
        score+=PAWN_DUO;
#ifdef DEBUGP
      if (score != lastsc)
        printf("white pawn[duo] file=%d,      score=%d\n", file,score);
      lastsc=score;
#endif
/*
 ----------------------------------------------------------
|                                                          |
|  test the pawn where it is, and as it advances.  note    |
|  whether it can end up with more pawn defenders than     |
|  pawn attackers as it advances.                          |
|                                                          |
 ----------------------------------------------------------
*/
      weakness=0;
      if (plus8dir[square]&BlackPawns) break;
      defenders=PopCnt(b_pawn_attacks[square]&WhitePawns);
      attackers=PopCnt(w_pawn_attacks[square]&BlackPawns);
      if (defenders > attackers) break;
      defenders=PopCnt(b_pawn_attacks[square+8]&WhitePawns);
      attackers=PopCnt(w_pawn_attacks[square+8]&BlackPawns);
      if (attackers && attackers >= defenders) weakness=attackers-defenders+1;
      else if (attackers || defenders) break;
      if (!weakness) break;
/*
 ----------------------------------------------------------
|                                                          |
|  if the pawn can be defended by a pawn, and that pawn    |
|  can safely advance, then this pawn is not weak.         |
|                                                          |
 ----------------------------------------------------------
*/
      if ((temp=mask_no_pawn_attacks_w[square]&WhitePawns)) {
        if (file > FILEA) {
          const BITBOARD temp1=temp&file_mask[file-1];
          attackers=1;
          if (temp1) {
            const int defend_sq=LastOne(temp1);
            for (sq=defend_sq;sq<(Rank(square)<<3);sq+=8) {
              attackers=1;
              if (sq!=defend_sq && tree->all_pawns&SetMask(sq)) break;
              attackers=PopCnt(w_pawn_attacks[sq]&BlackPawns);
              if (attackers) break;
            }
            if (!attackers) weakness=0;
          }
          if (!weakness) break;
        }
        if (file < FILEH) {
          const BITBOARD temp1=temp&file_mask[file+1];
          if (temp1) {
            const int defend_sq=LastOne(temp1);
            for (sq=defend_sq;sq<(Rank(square)<<3);sq+=8) {
              attackers=1;
              if (sq != defend_sq && tree->all_pawns&SetMask(sq)) break;
              attackers=PopCnt(w_pawn_attacks[sq]&BlackPawns);
              if (attackers) break;
            }
            if (!attackers) weakness=0;
          }
        }
      }

      if (weakness > 0) {
        if (weakness == 3) score-=PAWN_WEAK_P2;
        else if (weakness) score-=PAWN_WEAK_P1;
      }
    } while(0);
#ifdef DEBUGP
    if (score != lastsc)
      printf("white pawn[weak] file=%d,     score=%d\n", file,score);
    lastsc=score;
#endif
/*
 ----------------------------------------------------------
|                                                          |
|   evaluate doubled pawns.  if there are other pawns on   |
|   this file, penalize this pawn.                         |
|                                                          |
 ----------------------------------------------------------
*/
    if ((pns=PopCnt(file_mask[file]&WhitePawns)) > 1) {
      score-=doubled_pawn_value[pns];
    }
#ifdef DEBUGP
    if (score != lastsc)
      printf("white pawn[doubled] file=%d,  score=%d\n", file,score);
    lastsc=score;
#endif
/*
 ----------------------------------------------------------
|                                                          |
|   evaluate passed pawns.                                 |
|                                                          |
 ----------------------------------------------------------
*/
    if (!(mask_pawn_passed_w[square]&BlackPawns)) {
      score+=passed_pawn_value[Rank(square)];
      if (minus8dir[square]&WhitePawns)
        score-=passed_pawn_value[Rank(square)]>>1;
      if (mask_pawn_protected_w[square]&WhitePawns)
        tree->pawn_score.protected|=1;
      tree->pawn_score.passed_w|=128>>file;
#ifdef DEBUGP
      printf("white pawn[passed]            file=%d\n", file);
      if (score != lastsc)
        printf("white pawn[passed]            score=%d\n", score);
#endif
    }
/*
 ----------------------------------------------------------
|                                                          |
|   now determine if this pawn is a candidate passer,      |
|   since we now know it isn't passed.  a candidate is a   |
|   pawn on a file with no enemy pawns in front of it, and |
|   if it advances until it contacts an enemy pawn, and it |
|   is defended as many times as it is attacked when it    |
|   reaches that pawn, then all that is left is to see if  |
|   it is passed when the attacker(s) get removed.         |
|                                                          |
 ----------------------------------------------------------
*/
    else {
      if (!(file_mask[File(square)]&BlackPawns) &&
          mask_pawn_isolated[square]&WhitePawns &&
          !(w_pawn_attacks[square]&BlackPawns)) {
        attackers=1;
        defenders=0;
        for (sq=square;sq<A7;sq+=8) {
          if (SetMask(sq+8)&tree->all_pawns) break;
          defenders=PopCnt(b_pawn_attacks[sq]&wp_moves);
          attackers=PopCnt(w_pawn_attacks[sq]&BlackPawns);
          if (attackers) break;
        }
        if (attackers <= defenders) {
          if (!(mask_pawn_passed_w[sq+8]&BlackPawns)) {
            tree->pawn_score.candidates_w|=128>>file;
          }
        }
      }
      if (!(tree->pawn_score.candidates_w&128)) {
        if (file <= FILED) {
          if (WhitePawns&file_mask[file+1] && WhitePawns&file_mask[file+2]) {
            if (!(BlackPawns&file_mask[file]) &&
                !(BlackPawns&file_mask[file+2]) &&
                !(BlackPawns&file_mask[file+3]) &&
                PopCnt(BlackPawns&file_mask[file+1])<=2)
              tree->pawn_score.candidates_w|=128>>file;
          }
        }
        else {
          if (WhitePawns&file_mask[file-1] && WhitePawns&file_mask[file-2]) {
            if (!(BlackPawns&file_mask[file]) &&
                !(BlackPawns&file_mask[file-2]) &&
                !(BlackPawns&file_mask[file-3]) &&
                PopCnt(BlackPawns&file_mask[file-1])<=2)
              tree->pawn_score.candidates_w|=128>>file;
          }
        }
      }
#ifdef DEBUGP
      if (tree->pawn_score.candidates_w&(128>>file))
        printf("white pawn[candidate]       square=%d\n", square);
#endif
    }
/*
 ----------------------------------------------------------
|                                                          |
|   evaluate "hidden" passed pawns.  simple case is a pawn |
|   chain (white) at b5, a6, with a black pawn at a7.      |
|   it appears the b-pawn is backward, with a ram at a6/a7 |
|   but this is misleading, because the pawn at a6 is      |
|   really passed when white plays b6.                     |
|                                                          |
 ----------------------------------------------------------
*/
    if (Rank(square) > RANK5 && SetMask(square+8)&BlackPawns &&
        !(mask_pawn_passed_w[square+8]&BlackPawns) &&
        ((File(square) < FILEH && SetMask(square-7)&WhitePawns &&
          !(plus8dir[square-7]&BlackPawns) &&
          (File(square) == FILEG || !(plus8dir[square-6]&BlackPawns))) ||
         (File(square) > FILEA && SetMask(square-9)&WhitePawns &&
          !(plus8dir[square-9]&BlackPawns) &&
          (File(square) == FILEB || !(plus8dir[square-10]&BlackPawns))))) {
      score+=hidden_passed_pawn_value[Rank(square)];
    }
#ifdef DEBUGP
    if (score != lastsc)
      printf("white pawn[hidden] file=%d,   score=%d\n", file,score);
    lastsc=score;
#endif
/*
 ----------------------------------------------------------
|                                                          |
|   evaluate blocked pawns.  these are pawns that can not  |
|   advance because they are blocked, or because they will |
|   be instantly lost due to other enemy pawns preventing  |
|   them from moving at all.                               |
|                                                          |
 ----------------------------------------------------------
*/
    blocked=1;
    do {
      for (sq=square;sq<Min(square+24,A8);sq+=8) {
        defenders=PopCnt(b_pawn_attacks[sq]&wp_moves);
        attackers=PopCnt(w_pawn_attacks[sq]&BlackPawns);
        if (attackers-defenders > 1) break;
        else if (attackers) {
          blocked=0;
          break;
        }
        if (SetMask(sq+8)&tree->all_pawns) break;
      }
      if (sq >= Min(square+24,A8)) blocked=0;
    } while(0);
    if (!blocked) w_unblocked++;
    Clear(square,pawns);
  }
/*
 ----------------------------------------------------------
|                                                          |
|   black pawns.                                           |
|                                                          |
 ----------------------------------------------------------
*/
  pawns=BlackPawns;
  while(pawns) {
    square=FirstOne(pawns);
    file=File(square);
/*
 ----------------------------------------------------------
|                                                          |
|   evaluate pawn advances.  center pawns are encouraged   |
|   to advance, while wing pawns are pretty much neutral.  |
|                                                          |
 ----------------------------------------------------------
*/
    score-=pval_b[square];
#ifdef DEBUGP
    if (score != lastsc)
      printf("black pawn[static] file=%d,   score=%d\n", file,score);
    lastsc=score;
#endif
/*
 ----------------------------------------------------------
|                                                          |
|   evaluate weak pawns.  weak pawns are evaluated by the  |
|   following rules:  (1) if a pawn is defended by a pawn, |
|   it isn't weak;  (2) if a pawn is undefended by a pawn  |
|   and advances one (or two if it hasn't moved yet) ranks |
|   and is defended fewer times than it is attacked, it is |
|   weak.  note that the penalty is greater if the pawn is |
|   on an open file.  note that an isolated pawn is just   |
|   another case of a weak pawn, since it can never be     |
|   defended by a pawn.                                    |
|                                                          |
 ----------------------------------------------------------
*/
    if (!(mask_pawn_isolated[square]&BlackPawns)) {
      b_isolated++;
      if (!(minus8dir[square]&WhitePawns)) b_isolated_of++;
    }
    else do {
/*
 ----------------------------------------------------------
|                                                          |
|  test the pawn to see it if forms a "duo" which is two   |
|  pawns side-by-side.                                     |
|                                                          |
 ----------------------------------------------------------
*/
      if (mask_pawn_duo[square]&BlackPawns)
        score-=PAWN_DUO;
#ifdef DEBUGP
      if (score != lastsc)
        printf("black pawn[duo] file=%d,      score=%d\n", file,score);
      lastsc=score;
#endif
/*
 ----------------------------------------------------------
|                                                          |
|  test the pawn where it is, and as it advances.  note    |
|  whether it can end up with more pawn defenders than     |
|  pawn attackers as it advances.                          |
|                                                          |
 ----------------------------------------------------------
*/
      weakness=0;
      if (minus8dir[square]&WhitePawns) break;
      attackers=PopCnt(b_pawn_attacks[square]&WhitePawns);
      defenders=PopCnt(w_pawn_attacks[square]&BlackPawns);
      if (defenders > attackers) break;
      attackers=PopCnt(b_pawn_attacks[square-8]&WhitePawns);
      defenders=PopCnt(w_pawn_attacks[square-8]&BlackPawns);
      if (attackers && attackers >= defenders) weakness=attackers-defenders+1;
      else if (attackers || defenders) break;
      if (!weakness) break;
/*
 ----------------------------------------------------------
|                                                          |
|  if the pawn can be defended by a pawn, and that pawn    |
|  can safely advance, then this pawn is not weak.         |
|                                                          |
 ----------------------------------------------------------
*/
      if ((temp=mask_no_pawn_attacks_b[square]&BlackPawns)) {
        if (file > FILEA) {
          const BITBOARD temp1=temp&file_mask[file-1];
          attackers=1;
          if (temp1) {
            const int defend_sq=FirstOne(temp1);
            for (sq=defend_sq;sq>=((Rank(square)+1)<<3);sq-=8) {
              attackers=1;
              if (sq!=defend_sq && tree->all_pawns&SetMask(sq)) break;
              attackers=PopCnt(b_pawn_attacks[sq]&WhitePawns);
              if (attackers) break;
            }
            if (!attackers) weakness=0;
          }
          if (!weakness) break;
        }
        if (file < FILEH) {
          const BITBOARD temp1=temp&file_mask[file+1];
          if (temp1) {
            const int defend_sq=FirstOne(temp1);
            for (sq=defend_sq;sq>=((Rank(square)+1)<<3);sq-=8) {
              attackers=1;
              if (sq!=defend_sq && tree->all_pawns&SetMask(sq)) break;
              attackers=PopCnt(b_pawn_attacks[sq]&WhitePawns);
              if (attackers) break;
            }
            if (!attackers) weakness=0;
          }
        }
      }

      if (weakness > 0) {
        if (weakness == 3) score+=PAWN_WEAK_P2;
        else if (weakness) score+=PAWN_WEAK_P1;
        else score+=PAWN_WEAK_P2;
      }
    } while(0);
#ifdef DEBUGP
    if (score != lastsc)
      printf("black pawn[weak] file=%d,     score=%d\n", file,score);
    lastsc=score;
#endif
/*
 ----------------------------------------------------------
|                                                          |
|   evaluate doubled pawns.  if there are other pawns on   |
|   this file, penalize this pawn.                         |
|                                                          |
 ----------------------------------------------------------
*/
    if ((pns=PopCnt(file_mask[file]&BlackPawns)) > 1) {
      score+=doubled_pawn_value[pns];
    }
#ifdef DEBUGP
    if (score != lastsc)
      printf("black pawn[doubled] file=%d,  score=%d\n", file,score);
    lastsc=score;
#endif
/*
 ----------------------------------------------------------
|                                                          |
|   evaluate passed pawns.                                 |
|                                                          |
 ----------------------------------------------------------
*/
    if (!(mask_pawn_passed_b[square]&WhitePawns)) {
      score-=passed_pawn_value[(RANK8-Rank(square))];
      if (plus8dir[square]&BlackPawns)
        score+=passed_pawn_value[RANK8-Rank(square)]>>1;
      if (mask_pawn_protected_b[square]&BlackPawns)
        tree->pawn_score.protected|=2;
      tree->pawn_score.passed_b|=128>>file;
#ifdef DEBUGP
      printf("black pawn[passed]            file=%d\n", file);
      if (score != lastsc)
        printf("black pawn[passed]            score=%d\n", score);
#endif
    }
/*
 ----------------------------------------------------------
|                                                          |
|   now determine if this pawn is a candidate passer,      |
|   since we now know it isn't passed.  a candidate is a   |
|   pawn on a file with no enemy pawns in front of it, and |
|   if it advances until it contacts an enemy pawn, and it |
|   is defended as many times as it is attacked when it    |
|   reaches that pawn, then all that is left is to see if  |
|   it is passed when the attacker(s) get removed.         |
|                                                          |
 ----------------------------------------------------------
*/
    else {
      if (!(file_mask[File(square)]&WhitePawns) &&
          mask_pawn_isolated[square]&BlackPawns &&
          !(b_pawn_attacks[square]&WhitePawns)) {
        attackers=1;
        defenders=0;
        for (sq=square;sq>H2;sq-=8) {
          if (SetMask(sq-8)&tree->all_pawns) break;
          attackers=PopCnt(b_pawn_attacks[sq]&WhitePawns);
          defenders=PopCnt(w_pawn_attacks[sq]&bp_moves);
          if (attackers) break;
        }
        if (attackers <= defenders) {
          if (!(mask_pawn_passed_b[sq-8]&WhitePawns)) {
            tree->pawn_score.candidates_b|=128>>file;
          }
        }
      }
      if (!(tree->pawn_score.candidates_b&128)) {
        if (file <= FILED) {
          if (BlackPawns&file_mask[file+1] && BlackPawns&file_mask[file+2]) {
            if (!(WhitePawns&file_mask[file]) &&
                !(WhitePawns&file_mask[file+2]) &&
                !(WhitePawns&file_mask[file+3]) &&
                PopCnt(WhitePawns&file_mask[file+1])<=2)
              tree->pawn_score.candidates_b|=128>>file;
          }
        }
        else {
          if (BlackPawns&file_mask[file-1] && BlackPawns&file_mask[file-2]) {
            if (!(WhitePawns&file_mask[file]) &&
                !(WhitePawns&file_mask[file-2]) &&
                  !(WhitePawns&file_mask[file-3]) &&
                PopCnt(WhitePawns&file_mask[file-1])<=2)
              tree->pawn_score.candidates_b|=128>>file;
          }
        }
      }
#ifdef DEBUGP
      if (tree->pawn_score.candidates_b&(128>>file))
        printf("black pawn[candidate]       square=%d\n", square);
#endif
    }
/*
 ----------------------------------------------------------
|                                                          |
|   evaluate "hidden" passed pawns.  simple case is a pawn |
|   chain (white) at b5, a6, with a black pawn at a7.      |
|   it appears the b-pawn is backward, with a ram at a6/a7 |
|   but this is misleading, because the pawn at a6 is      |
|   really passed when white plays b6.                     |
|                                                          |
 ----------------------------------------------------------
*/
    if (Rank(square) < RANK4 && SetMask(square-8)&WhitePawns &&
        !(mask_pawn_passed_b[square-8]&WhitePawns) &&
        ((File(square) < FILEH && SetMask(square+9)&BlackPawns &&
          !(minus8dir[square+9]&WhitePawns) &&
          (File(square) == FILEG || !(minus8dir[square+10]&WhitePawns))) ||
         (File(square) > FILEA && SetMask(square+7)&BlackPawns &&
          !(minus8dir[square+7]&WhitePawns) &&
          (File(square) == FILEB || !(minus8dir[square+6]&WhitePawns))))) {
      score-=hidden_passed_pawn_value[(RANK8-Rank(square))];
    }
#ifdef DEBUGP
    if (score != lastsc)
      printf("black pawn[hidden] file=%d,   score=%d\n", file,score);
    lastsc=score;
#endif
/*
 ----------------------------------------------------------
|                                                          |
|   evaluate blocked pawns.  these are pawns that can not  |
|   advance because they are blocked, or because they will |
|   be instantly lost due to other enemy pawns preventing  |
|   them from moving at all.                               |
|                                                          |
 ----------------------------------------------------------
*/
    blocked=1;
    do {
      for (sq=square;sq>Max(square-24,H1);sq-=8) {
        attackers=PopCnt(b_pawn_attacks[sq]&WhitePawns);
        defenders=PopCnt(w_pawn_attacks[sq]&bp_moves);
        if (attackers-defenders > 1) break;
        else if (attackers) {
          blocked=0;
          break;
        }
        if (SetMask(sq-8)&tree->all_pawns) break;
      }
      if (sq <= Max(square-24,H1)) blocked=0;
    } while(0);
    if (!blocked) b_unblocked++;
    Clear(square,pawns);
  }
/*
 ----------------------------------------------------------
|                                                          |
|   now fold in the penalty for isolated pawns, which is   |
|   non-linear to penalize more isolani more severely.     |
|   note that the penalty penalizes the side with the      |
|   most isolated pawns, in an exponential rate.           |
|                                                          |
 ----------------------------------------------------------
*/
  score-=isolated_pawn_value[w_isolated];
  score-=isolated_pawn_of_value[w_isolated_of];
  score+=isolated_pawn_value[b_isolated];
  score+=isolated_pawn_of_value[b_isolated_of];
#ifdef DEBUGP
  if (score != lastsc)
    printf("pawn[isolated]          score=%d\n", score);
  lastsc=score;
#endif
/*
 ----------------------------------------------------------
|                                                          |
|   now fold in the bonus for unblocked pawns.  the real   |
|   issue is keeping "lever" possibilities for each side   |
|   available, so there is some opportunity for breaking   |
|   the position open, rather than letting it become       |
|   totally blocked.                                       |
|                                                          |
 ----------------------------------------------------------
*/
  if (TotalWhitePawns>4 && TotalBlackPawns>4) {
    if (w_unblocked <= 2)
      score-=PAWNS_BLOCKED*(3-w_unblocked)*blocked_scale/100;
    if (b_unblocked <= 2)
      score+=PAWNS_BLOCKED*(3-b_unblocked)*blocked_scale/100;
  }
#ifdef DEBUGP
  if (score != lastsc)
    printf("pawn[unblocked]         score=%d\n", score);
  lastsc=score;
#endif
/*
 ----------------------------------------------------------
|                                                          |
|   now fold in the penalty for "rams" which are pawns     |
|   that are blocked by a pawn on the same file.           |
|                                                          |
 ----------------------------------------------------------
*/
  if (root_wtm) {
    if (TotalWhitePawns > 5)
      score-=pawn_rams[PopCnt(WhitePawns&(BlackPawns<<8))];
  }
  else {
    if (TotalBlackPawns > 5)
      score+=pawn_rams[PopCnt(WhitePawns&(BlackPawns<<8))];
  }
#ifdef DEBUGP
  if (score != lastsc)
    printf("pawn[rams]              score=%d\n",score);
  lastsc=score;
#endif
/*
 ----------------------------------------------------------
|                                                          |
|   now evaluate king safety.                              |
|                                                          |
|   the first step is to step across the board and note    |
|   which files are open/half open.  since this is common  |
|   to both kings, this is only done once since it is the  |
|   same for both players.                                 |
|                                                          |
|   at the same time we note if any of the three pawns in  |
|   front of the king have moved, and count those as well. |
|                                                          |
 ----------------------------------------------------------
*/
  left=file_mask[FILEA];
  right=file_mask[FILEH];
  for (file=0;file<3;file++) {
    if (file < 2) {
      kside_w=1;
      kside_b=1;
    }
    else {
      kside_w=kside_open_files+kside_half_open_files_b+kside_half_open_files_w+kmissw;
      kside_b=kside_open_files+kside_half_open_files_b+kside_half_open_files_w+kmissb;
    }
    if (!(right&tree->all_pawns))
      kside_open_files++;
    else {
      if (kside_w) {
        if (!(right&WhitePawns)) {
          kside_half_open_files_w++;
        }
        else if (!(WhitePawns&SetMask(H2-file))) {
          kmissw++;
          if (!(WhitePawns&SetMask(H3-file))) kmissw++;
          if (file == 1) kmissw++;
        }
      }
      if (kside_b) {
        if (!(right&BlackPawns)) {
          kside_half_open_files_b++;
        }
        else if (!(BlackPawns&SetMask(H7-file))) {
          kmissb++;
          if (!(BlackPawns&SetMask(H6-file))) kmissb++;
          if (file == 1) kmissb++;
        }
      }
    }
    right=right<<1;

    if (file < 2) {
      qside_w=1;
      qside_b=1;
    }
    else {
      qside_w=qside_open_files+qside_half_open_files_b+qside_half_open_files_w+qmissw;
      qside_b=qside_open_files+qside_half_open_files_b+qside_half_open_files_w+qmissb;
    }
    if (!(left&tree->all_pawns))
      qside_open_files++;
    else {
      if (qside_w) {
        if (!(left&WhitePawns)) {
          qside_half_open_files_w++;
        }
        else if (!(WhitePawns&SetMask(A2+file))) {
          qmissw++;
          if (!(WhitePawns&SetMask(A3+file))) qmissw++;
          if (file == 1) qmissw++;
        }
      }
      if (qside_b) {
        if (!(left&BlackPawns)) {
          qside_half_open_files_b++;
        }
        else if (!(BlackPawns&SetMask(A7+file))) {
          qmissb++;
          if (!(BlackPawns&SetMask(A6+file))) qmissb++;
          if (file == 1) qmissb++;
        }
      }
    }
    left=left>>1;
  }
/*
 ----------------------------------------------------------
|                                                          |
|   now a special case, where one side has advanced two    |
|   pawns (ie pawns at f2, g3, h3, where the two advances  |
|   are not quite as dangerous as they seem.               |
|                                                          |
 ----------------------------------------------------------
*/
  if (kmissw) {
    if ((mask_F3G3&WhitePawns) == mask_F3G3) kmissw--;
    else if ((mask_G3H3&WhitePawns) == mask_G3H3) kmissw--;
  }
  if (qmissw) {
    if ((mask_A3B3&WhitePawns) == mask_A3B3) qmissw--;
    else if ((mask_B3C3&WhitePawns) == mask_B3C3) qmissw--;
  }
  if (kmissb) {
    if ((mask_F6G6&BlackPawns) == mask_F6G6) kmissb--;
    else if ((mask_G6H6&BlackPawns) == mask_G6H6) kmissb--;
  }
  if (qmissb) {
    if ((mask_A6B6&BlackPawns) == mask_A6B6) qmissb--;
    else if ((mask_B6C6&BlackPawns) == mask_B6C6) qmissb--;
  }
/*
 ----------------------------------------------------------
|                                                          |
|   now we take the number of open/half-open files, plus   |
|   the number of 'missing' pawns on the 2nd rank in front |
|   of the king, and fold them into a 'defect count.'  we  |
|   run these raw counts 'indirect' through a scaling      |
|   procedure so that two open files are far worse than    |
|   one, and so forth.                                     |
|                                                          |
 ----------------------------------------------------------
*/
  tree->pawn_score.white_defects_k=missing[kmissw]+
                                   openf[kside_open_files]+
                                   hopenf[kside_half_open_files_w]+
                                   (hopenf[kside_half_open_files_b]>>1);
  tree->pawn_score.white_defects_q=missing[qmissw]+
                                   openf[qside_open_files]+
                                   hopenf[qside_half_open_files_w]+
                                   (hopenf[qside_half_open_files_b]>>1);
  tree->pawn_score.black_defects_k=missing[kmissb]+
                                   openf[kside_open_files]+
                                   (hopenf[kside_half_open_files_w]>>1)+
                                   hopenf[kside_half_open_files_b];
  tree->pawn_score.black_defects_q=missing[qmissb]+
                                   openf[qside_open_files]+
                                   (hopenf[qside_half_open_files_w]>>1)+
                                   hopenf[qside_half_open_files_b];
#if defined(DEBUGK)
  printf("white: kmissing=%d  kopen=%d  khalf=%d\n",
         missing[kmissw],openf[kside_open_files], hopenf[kside_half_open_files_w]);
  printf("white: qmissing=%d  qopen=%d  qhalf=%d\n",
         missing[qmissw],openf[qside_open_files], hopenf[qside_half_open_files_w]);
  printf("black: kmissing=%d  kopen=%d  khalf=%d\n",
         missing[kmissb],openf[kside_open_files], hopenf[kside_half_open_files_b]);
  printf("black: qmissing=%d  qopen=%d  qhalf=%d\n",
         missing[qmissb],openf[qside_open_files], hopenf[qside_half_open_files_b]);
  printf("white, defects=%d(q)  %d(k)\n",
         tree->pawn_score.white_defects_q,tree->pawn_score.white_defects_k);
  printf("black, defects=%d(q)  %d(k)\n",
         tree->pawn_score.black_defects_q,tree->pawn_score.black_defects_k);
#endif
/*
 ----------------------------------------------------------
|                                                          |
|   now look for the "stonewall" formation in the white    |
|   pawns and penalize the kingside safety if this type of |
|   pawn position is found.  this also catched closed type |
|   positions also.                                        |
|                                                          |
 ----------------------------------------------------------
*/
  if (root_wtm) {
    if (PopCnt(BlackPawns&stonewall_black)==2 &&
        WhitePawns&e2_e3)
      tree->pawn_score.white_defects_k+=KING_SAFETY_STONEWALL;
  }
  else {
    if (PopCnt(WhitePawns&stonewall_white) == 2 &&
        BlackPawns&e7_e6)
      tree->pawn_score.black_defects_k+=KING_SAFETY_STONEWALL;
  }
#if defined(DEBUGK)
  printf("white.SW, defects=%d(q)  %d(k)\n",
         tree->pawn_score.white_defects_q,tree->pawn_score.white_defects_k);
  printf("black.SW, defects=%d(q)  %d(k)\n",
         tree->pawn_score.black_defects_q,tree->pawn_score.black_defects_k);
#endif
/*
 ----------------------------------------------------------
|                                                          |
|  evaluate outside passed pawns by analyzing the passed   |
|  pawns for both sides. we use a pre-computed table that  |
|  can determine if one side has a passed pawn that is to  |
|  the left of all other pawns, or to the right of all     |
|  other pawns.  if both seem to have an outside passed    |
|  pawn, this means one side has one on one side of the    |
|  board while the other side has a passed pawn on the     |
|  opposite side of the board.  this case is treated as    |
|  neither having an outside passer.                       |
|                                                          |
|  an outside passer is defined as a pawn closer to one    |
|  edge of the board than _all_ other pawns on the board.  |
|  and that pawn must have at least one file between it    |
|  and any other pawns on the board.  this last limitation |
|  means we have to do a little work here to remove our    |
|  own pawn from the test if (say) we have a passed pawn   |
|  on the a-file, and one of our pawns on the b-file.      |
|                                                          |
|  we repeat for candidate passed pawns as well.           |
|                                                          |
|  tree->pawn_score.outside is a bitmap with 4 bits:       |
|                                                          |
|        xxxx xxx1   (1) -> white has outside candidate    |
|        xxxx xx1x   (2) -> white has 2 outside candidates |
|        xxxx x1xx   (4) -> white has outside passer       |
|        xxxx 1xxx   (8) -> white has 2 outside passers    |
|        xxx1 xxxx  (16) -> black has outside candidate    |
|        xx1x xxxx  (32) -> black has 2 outside candidates |
|        x1xx xxxx  (64) -> black has outside passer       |
|        1xxx xxxx (128) -> black has 2 outside passers    |
|                                                          |
 ----------------------------------------------------------
*/
  wop=is_outside[tree->pawn_score.passed_w][tree->pawn_score.allb];
  bop=is_outside[tree->pawn_score.passed_b][tree->pawn_score.allw];
  if (!wop || !bop) {
    if (wop > 1) tree->pawn_score.outside|=8;
    else if (wop && TotalWhitePawns>1) tree->pawn_score.outside|=4;
    if (bop > 1) tree->pawn_score.outside|=128;
    else if (bop && TotalBlackPawns>1) tree->pawn_score.outside|=64;
  }

  wop=is_outside_c[tree->pawn_score.candidates_w][tree->pawn_score.allb];
  bop=is_outside_c[tree->pawn_score.candidates_b][tree->pawn_score.allw];
  if (!wop || !bop) {
    if (wop > 1) tree->pawn_score.outside|=2;
    else if (wop) tree->pawn_score.outside|=1;
    if (bop > 1) tree->pawn_score.outside|=32;
    else if (bop) tree->pawn_score.outside|=16;
  }

  if (!(tree->pawn_score.outside & 0314)) {
    if (tree->pawn_score.candidates_w && !tree->pawn_score.candidates_b)
      tree->pawn_score.outside|=1;
    if (tree->pawn_score.candidates_b && !tree->pawn_score.candidates_w)
      tree->pawn_score.outside|=16;
  }
  if (!(tree->pawn_score.outside&144)) {
    if (tree->pawn_score.protected&1 && !(tree->pawn_score.protected&2) &&
        pop_cnt_8bit[(int)(tree->pawn_score.passed_b)]<2)
      tree->pawn_score.outside|=4;
    if (tree->pawn_score.protected&2 && !(tree->pawn_score.protected&1) &&
        pop_cnt_8bit[(int)(tree->pawn_score.passed_w)]<2)
      tree->pawn_score.outside|=64;
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
 ----------------------------------------------------------
|                                                          |
|   store the results in the pawn hash table for reuse at  |
|   a later time as needed.                                |
|                                                          |
 ----------------------------------------------------------
*/
  score=score*pawn_scale/100;
  tree->pawn_score.p_score=score;
  *ptable=tree->pawn_score;
  return(score);
}
