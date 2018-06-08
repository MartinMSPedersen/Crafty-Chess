#include <stdio.h>
#include <stdlib.h>
#include "chess.h"
#include "evaluate.h"
#include "data.h"
/* last modified 05/20/99 */
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

int EvaluateDB(TREE *tree, int ply, int wtm, int alpha, int beta) {
  register BITBOARD temp;
  register int square, file, score, tscore, w_tropism=8, b_tropism=8;
  register int trop, drawn_ending=0;
#if defined(DEBUGEV)
  int lastsc=Material;
#endif
/*
**********************************************************************
*                                                                    *
*   check for draws due to insufficient material and adjust the      *
*   score as necessary.                                              *
*                                                                    *
**********************************************************************
*/
  if (TotalWhitePieces<5 && TotalBlackPieces<5)
    drawn_ending=EvaluateDraws(tree);
  if (drawn_ending > 0) return(DrawScore(root_wtm==wtm));
  score=Material;
#ifdef DEBUGEV
  printf("score[material]=                  %4d\n",score);
#endif
/*
**********************************************************************
*                                                                    *
*   check for bad trades, which includes a queen for 3 minor pieces  *
*   and a rook for two minor pieces.                                 *
*                                                                    *
**********************************************************************
*/
  if (WhiteMinors != BlackMinors) {
    if (WhiteMajors == BlackMajors) {
      if (WhiteMinors > BlackMinors) score+=BAD_TRADE;
      else  score-=BAD_TRADE;
    }
    else if (abs(WhiteMajors-BlackMajors) == 1) {
      if (WhiteMinors > BlackMinors+1) score+=BAD_TRADE;
      else if (BlackMinors > WhiteMinors+1) score-=BAD_TRADE;
    }
    else if (abs(WhiteMajors-BlackMajors) == 2) {
      if (WhiteMinors > BlackMinors+2) score+=BAD_TRADE;
      else if (BlackMinors > WhiteMinors+2) score-=BAD_TRADE;
    }
  }
#ifdef DEBUGEV
  if (score != lastsc)
    printf("score[bad trade]=                 %4d (%+d)\n",score,score-lastsc);
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
  tree->all_pawns=BlackPawns | WhitePawns;
  if ((TotalWhitePawns+TotalBlackPawns) == 0) {
    score+=EvaluateMate(tree);
#ifdef DEBUGEV
    printf("score[mater]=                     %4d (%+d)\n",score,score-lastsc);
#endif
    return((wtm) ? score : -score);
  }
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
    score+=EvaluatePassedPawns(tree);
    if (tree->pawn_score.outside&1)
      score+=outside_passed[(int) TotalBlackPieces];
    if (tree->pawn_score.outside&2)
      score-=outside_passed[(int) TotalWhitePieces];
    if ((TotalWhitePieces==0 && tree->pawn_score.passed_b) ||
        (TotalBlackPieces==0 && tree->pawn_score.passed_w))
      score+=EvaluatePassedPawnRaces(tree,wtm);
#ifdef DEBUGEV
    if (score != lastsc)
      printf("score[passed pawns]=              %4d (%+d)\n",score,score-lastsc);
    lastsc=score;
#endif
  }
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
    if (WhiteBishops & mask_A7H7) {
      if (WhiteBishops & SetMask(A7) && mask_B6B7 & BlackPawns) {
        if (SetMask(B6) & BlackPawns || Swap(tree,B7,B6,0)>=0)
          score-=BISHOP_TRAPPED;
      }
      else if (WhiteBishops & SetMask(H7) && mask_G6G7 & BlackPawns) {
        if (SetMask(G6) & BlackPawns || Swap(tree,G7,G6,0)>=0)
          score-=BISHOP_TRAPPED;
      }
    }
  }
  if (BlackBishops) {
    if (BlackBishops & mask_A2H2) {
      if (BlackBishops & SetMask(A2) && mask_B2B3 & WhitePawns) {
        if (SetMask(B3) & WhitePawns || Swap(tree,B2,B3,1)>=0)
          score+=BISHOP_TRAPPED;
      }
      else if (BlackBishops & SetMask(H2) && mask_G2G3 & WhitePawns) {
        if (SetMask(G3) & WhitePawns || Swap(tree,G2,G3,1)>=0)
          score+=BISHOP_TRAPPED;
      }
    }
  }
/*
**********************************************************************
*                                                                    *
*   call EvaluateDevelopment() to evaluate development.  if the      *
*   flag "opening" is zero, skip the call.                           *
*                                                                    *
**********************************************************************
*/
  if (opening) score+=EvaluateDevelopment(tree,ply);
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
  score+=EvaluateKingSafety(tree,ply);
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
    if (tscore-largest_positional_score>= beta) return(beta);
    if (tscore+largest_positional_score<= alpha) return(alpha);
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
  tree->endgame=(TotalWhitePieces<EG_MAT) | (TotalBlackPieces<EG_MAT);
/*
 ----------------------------------------------------------
|                                                          |
|   white king.                                            |
|                                                          |
|   first, check for a weak back rank.                     |
|                                                          |
 ----------------------------------------------------------
*/
  if (tree->endgame) score+=kval_w[WhiteKingSQ];
  if (WhiteKingSQ < A2) {
    if (!(king_attacks[WhiteKingSQ] & rank_mask[RANK2] &
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
      if (WhiteRooks & mask_kr_trapped_w[FILEH-WhiteKingSQ]) 
        score-=ROOK_TRAPPED;
    }
    else if (WhiteKingSQ < D1) {
      if (WhiteRooks & mask_qr_trapped_w[WhiteKingSQ])
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
  if (tree->endgame) score-=kval_b[BlackKingSQ];
  if (BlackKingSQ > H7) {
    if (!(king_attacks[BlackKingSQ] & rank_mask[RANK7] & 
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
      if (BlackRooks & mask_kr_trapped_b[FILEH-File(BlackKingSQ)])
        score+=ROOK_TRAPPED;
    }
    else if (BlackKingSQ < D8) {
      if (BlackRooks & mask_qr_trapped_b[File(BlackKingSQ)])
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
|   supported by a friendly pawn.                          |
|                                                          |
 ----------------------------------------------------------
*/
  temp=WhiteKnights;
  while(temp) {
    square=FirstOne(temp);
    if (white_outpost[square] &&
        !(mask_no_pawn_attacks_b[square] & BlackPawns)) {
      score+=white_outpost[square];
      if(b_pawn_attacks[square] & WhitePawns)
        score+=white_outpost[square]>>1;
      if (white_outpost[square]==MAXO &&
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
|   supported by a friendly pawn.                          |
|                                                          |
 ----------------------------------------------------------
*/
  temp=BlackKnights;
  while(temp) {
    square=FirstOne(temp);
    if (black_outpost[square] &&
        !(mask_no_pawn_attacks_w[square] & WhitePawns)) {
      score-=black_outpost[square];
      if (w_pawn_attacks[square] & BlackPawns) 
        score-=black_outpost[square]>>1;
      if (black_outpost[square]==MAXO &&
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
|   now add in a bonus for a bishop outpost, which is      |
|   similar to a knight outpost although less valuable.    |
|                                                          |
 ----------------------------------------------------------
*/
    if (white_outpost[square] &&
        !(mask_no_pawn_attacks_b[square] & BlackPawns)) {
      if(b_pawn_attacks[square] & WhitePawns) 
        score+=white_outpost[square]>>1;
      if (white_outpost[square]==MAXO &&
          SetMask(square+8)&BlackPawns) score+=BLOCKED_CENTER_PAWN;
    }
/*
 ----------------------------------------------------------
|                                                          |
|   add in a bonus for occupying a rank or file close to   |
|   the king.                                              |
|                                                          |
 ----------------------------------------------------------
*/
    w_tropism+=king_tropism_b[Distance(square,tree->b_kingsq)];
/*
 ----------------------------------------------------------
|                                                          |
|   add in a bonus for filling holes when the king has     |
|   castled and played b3/g3/b6/g6.                        |
|                                                          |
 ----------------------------------------------------------
*/
    if (!tree->endgame) {
      if (File(tree->w_kingsq) > FILEE) {
        if (!(WhitePawns & SetMask(G2)) && (WhitePawns & SetMask(G3)) &&
            Distance(tree->w_kingsq,G2)==1 && WhiteBishops & good_bishop_kw)
              score+=BISHOP_KING_SAFETY;
      }
      else if (File(tree->w_kingsq) < FILED) {
        if (!(WhitePawns & SetMask(B2)) && (WhitePawns & SetMask(B3)) &&
            Distance(tree->w_kingsq,B2)==1 && WhiteBishops & good_bishop_qw)
              score+=BISHOP_KING_SAFETY;
      }
    }
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
|   now add in a bonus for a bishop outpost, which is      |
|   similar to a knight outpost although less valuable.    |
|                                                          |
 ----------------------------------------------------------
*/
    if (black_outpost[square] &&
        !(mask_no_pawn_attacks_w[square] & WhitePawns)) {
      if(w_pawn_attacks[square] & BlackPawns) 
        score-=black_outpost[square]>>1;
      if (black_outpost[square]==MAXO &&
          SetMask(square-8)&WhitePawns) score-=BLOCKED_CENTER_PAWN;
    }
/*
 ----------------------------------------------------------
|                                                          |
|   add in a bonus for occupying a rank or file close to   |
|   the king.                                              |
|                                                          |
 ----------------------------------------------------------
*/
    b_tropism+=king_tropism_b[Distance(square,tree->w_kingsq)];
/*
 ----------------------------------------------------------
|                                                          |
|   add in a bonus for filling holes when the king has     |
|   castled and played b3/g3/b6/g6.                        |
|                                                          |
 ----------------------------------------------------------
*/
    if (!tree->endgame) {
      if (File(tree->b_kingsq) > FILEE) {
        if (!(BlackPawns & SetMask(G7)) && (BlackPawns & SetMask(G6)) &&
            Distance(tree->b_kingsq,G7)==1 && BlackBishops & good_bishop_kb)
              score-=BISHOP_KING_SAFETY;
      }
      else if (File(tree->b_kingsq) < FILED) {
        if (!(BlackPawns & SetMask(B7)) && (BlackPawns & SetMask(B6)) &&
            Distance(tree->b_kingsq,B7)==1 && BlackBishops & good_bishop_qb)
              score-=BISHOP_KING_SAFETY;
      }
    }
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
 ----------------------------------------------------------
*/
  if (WhiteBishops) {
    if (WhiteBishops & (WhiteBishops-1)) {
      score+=BISHOP_PAIR;
    }
    else {
      if (tree->endgame) {
        if (WhiteBishops & light_squares)
          score-=PopCnt(WhitePawns & light_squares)*BISHOP_PLUS_PAWNS_ON_COLOR;
        else
          score-=PopCnt(WhitePawns & dark_squares)*BISHOP_PLUS_PAWNS_ON_COLOR;
      }
      if (tree->all_pawns & mask_fgh && tree->all_pawns & mask_abc &&
          TotalWhitePieces==3 && TotalBlackPieces==2) 
          score+=BISHOP_OVER_KNIGHT_ENDGAME;
    }
  }
  if (BlackBishops) {
    if (BlackBishops & (BlackBishops-1)) {
      score-=BISHOP_PAIR;
    }
    else {
      if (tree->endgame) {
        if (BlackBishops & light_squares)
          score+=PopCnt(BlackPawns & light_squares)*BISHOP_PLUS_PAWNS_ON_COLOR;
        else
          score+=PopCnt(BlackPawns & dark_squares)*BISHOP_PLUS_PAWNS_ON_COLOR;
      }
      if (tree->all_pawns & mask_fgh && tree->all_pawns & mask_abc &&
          TotalBlackPieces==3 && TotalWhitePieces==2) 
        score-=BISHOP_OVER_KNIGHT_ENDGAME;
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
    if (!(file_mask[file] & tree->all_pawns)) {
      score+=ROOK_OPEN_FILE;
      trop=FileDistance(square,tree->b_kingsq);
    }
    else if (!(plus8dir[square] & WhitePawns)) score+=ROOK_HALF_OPEN_FILE;
/*
 ----------------------------------------------------------
|                                                          |
|   see if the rook is behind a passed pawn.  if it is,    |
|   it is counted as though the file is open.              |
|                                                          |
 ----------------------------------------------------------
*/
    if (128>>file & tree->pawn_score.passed_w) {
      register const int pawnsq=LastOne(WhitePawns & file_mask[file]);
      if (square<pawnsq && !(BlackPieces & SetMask(pawnsq+8)))
        score+=ROOK_BEHIND_PASSED_PAWN;
      if (AttacksFile(square) & WhiteRooks) score-=ROOK_BEHIND_PASSED_PAWN>>1;
    }
    if (128>>file & tree->pawn_score.passed_b) {
      register const int pawnsq=FirstOne(BlackPawns & file_mask[file]);
      if (square > pawnsq) score+=ROOK_BEHIND_PASSED_PAWN;
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
                                BlackPawns & rank_mask[RANK7])) {
      score+=ScaleUp(ROOK_ON_7TH,TotalBlackPieces);
      if (tree->pawn_score.passed_w && BlackKingSQ>H7 &&
          !(BlackPieces & mask_abs7_w)) score+=ROOK_ABSOLUTE_7TH;
      if (AttacksRank(square) & (WhiteRooks | WhiteQueens))
        score+=ROOK_CONNECTED_7TH_RANK;
    }
/*
 ----------------------------------------------------------
|                                                          |
|   adjust the king tropism as well.                       |
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
    if (!(file_mask[file] & tree->all_pawns)) {
      score-=ROOK_OPEN_FILE;
      trop=FileDistance(square,tree->w_kingsq);
    }
    else if (!(minus8dir[square] & BlackPawns)) score-=ROOK_HALF_OPEN_FILE;
/*
 ----------------------------------------------------------
|                                                          |
|   if not, see if the rook is behind a passed pawn.  if   |
|   it is, it is counted as though the file is open.       |
|                                                          |
 ----------------------------------------------------------
*/
    if (128>>file & tree->pawn_score.passed_b) {
      register const int pawnsq=FirstOne(BlackPawns & file_mask[file]);
      if (square>pawnsq && !(WhitePieces & SetMask(pawnsq-8)))
        score-=ROOK_BEHIND_PASSED_PAWN;
      if (AttacksFile(square) & BlackRooks) score+=ROOK_BEHIND_PASSED_PAWN>>1;
    }
    if (128>>file & tree->pawn_score.passed_w) {
      register const int pawnsq=LastOne(WhitePawns & file_mask[file]);
      if (square < pawnsq) score-=ROOK_BEHIND_PASSED_PAWN;
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
                                WhitePawns & rank_mask[RANK2])) {
      score-=ScaleUp(ROOK_ON_7TH,TotalWhitePieces);
      if (tree->pawn_score.passed_b && WhiteKingSQ<A2 &&
          !(WhitePieces & mask_abs7_b)) score-=ROOK_ABSOLUTE_7TH;
      if (AttacksRank(square) & (BlackRooks | BlackQueens))
        score-=ROOK_CONNECTED_7TH_RANK;
    }
/*
 ----------------------------------------------------------
|                                                          |
|   adjust the king tropism as well.                       |
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
|   check to see if the queen is in a strong positiono on  |
|   the 7th rank supported by a rook on the 7th.  if so,   |
|   the positional advantage is almost overwhelming.       |
|                                                          |
 ----------------------------------------------------------
*/
    if (Rank(square)==RANK7 && (BlackPawns & rank_mask[RANK7] ||
         (BlackKingSQ > H7))) {
      if (AttacksRank(square) & WhiteRooks) score+=QUEEN_ROOK_ON_7TH_RANK;
    }
/*
 ----------------------------------------------------------
|                                                          |
|   if white has a queen and black doesn't, then the white |
|   queen is stronger if white has at least one more piece |
|   on the board (ie Q vs RR is not so good for Q unless   |
|   there is another piece with the Q).                    |
|                                                          |
 ----------------------------------------------------------
*/
    if (!BlackQueens) {
      if (TotalWhitePieces>9 &&
          WhiteMajors==BlackMajors) score+=QUEEN_VS_2_ROOKS;
    }
/*
 ----------------------------------------------------------
|                                                          |
|   adjust the white tropism count for this piece.         |
|                                                          |
 ----------------------------------------------------------
*/
    trop=7;
    if (!(WhitePawns & plus8dir[square]))
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
    if (TotalWhitePawns > 4)
      w_tropism-=king_tropism_file_q[FileDistance(square,tree->b_kingsq)];
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
|   check to see if the queen is in a strong positiono on  |
|   the 7th rank supported by a rook on the 7th.  if so,   |
|   the positional advantage is almost overwhelming.       |
|                                                          |
 ----------------------------------------------------------
*/
    if (Rank(square)==RANK2 && (WhitePawns & rank_mask[RANK2] ||
         (WhiteKingSQ < A2))) {
      if (AttacksRank(square) & BlackRooks) score-=QUEEN_ROOK_ON_7TH_RANK;
    }
/*
 ----------------------------------------------------------
|                                                          |
|   if black has a queen and white doesn't, then the black |
|   queen is stronger if black has at least one more piece |
|   on the board (ie Q vs RR is not so good for Q unless   |
|   there is another piece with the Q).                    |
|                                                          |
 ----------------------------------------------------------
*/
    if (!WhiteQueens) {
      if (TotalBlackPieces>9 &&
          WhiteMajors==BlackMajors) score-=QUEEN_VS_2_ROOKS;
    }
/*
 ----------------------------------------------------------
|                                                          |
|   adjust the white tropism count for this piece.         |
|                                                          |
 ----------------------------------------------------------
*/
    trop=7;
    if (!(BlackPawns & minus8dir[square]))
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
    if (TotalBlackPawns > 4)
      b_tropism-=king_tropism_file_q[FileDistance(square,tree->w_kingsq)];
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
  score+=((tropism[w_tropism]*ttemper[tree->b_safety])>>4)-
         ((tropism[b_tropism]*ttemper[tree->w_safety])>>4);
#ifdef DEBUGEV
  if (score != lastsc) {
    printf("score[king tropism]=              %4d (%+d)\n",score,score-lastsc);
    printf("w_safety=%d  b_safety=%d\n",tree->w_safety,tree->b_safety);
    printf("w_tropism=%d  b_tropism=%d\n",w_tropism,b_tropism);
    printf("wtrop.score=%d\n",(tropism[w_tropism]*ttemper[tree->b_safety])>>4);
    printf("btrop.score=%d\n",(tropism[b_tropism]*ttemper[tree->w_safety])>>4);
  }
  lastsc=score;
#endif
  if (abs(score-tscore) > largest_positional_score) {
    largest_positional_score=abs(score-tscore);
    DisplayChessBoard(stdout,tree->pos);
    printf("lps=%d\n",largest_positional_score);
    printf("score=%d\n", Evaluate(tree,ply,wtm,alpha,beta));
  }
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
        if (TotalWhitePieces==3 && TotalBlackPieces==3) score=score>>1;
        else if (TotalWhitePieces == TotalBlackPieces)
          score=((score-Material)>>1)+Material;
      }
    }
  }
  if (drawn_ending < 0) {
    if (drawn_ending == -1 && score > 0) score=DrawScore(root_wtm==wtm);
    else if (drawn_ending == -2 && score < 0) score=DrawScore(root_wtm==wtm);
  }
#ifdef DEBUGEV
  if (score != lastsc)
    printf("score[draws]=                     %4d (%+d)\n",score,score-lastsc);
  lastsc=score;
#endif
  return((wtm) ? score : -score);
}
