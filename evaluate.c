#include <stdio.h>
#include <stdlib.h>
#include "chess.h"
#include "evaluate.h"
#include "data.h"

/* last modified 10/22/98 */
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
  register int drawn_ending=0;
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
  tree->all_pawns=Or(BlackPawns,WhitePawns);
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
    if (And(WhiteBishops,mask_A7H7)) {
      if (And(WhiteBishops,SetMask(A7)) && And(mask_B6B7,BlackPawns)) {
        if (And(SetMask(B6),BlackPawns) || Swap(tree,B7,B6,0)>=0)
          score-=BISHOP_TRAPPED;
      }
      else if (And(WhiteBishops,SetMask(H7)) && And(mask_G6G7,BlackPawns)) {
        if (And(SetMask(G6),BlackPawns) || Swap(tree,G7,G6,0)>=0)
          score-=BISHOP_TRAPPED;
      }
    }
  }
  if (BlackBishops) {
    if (And(BlackBishops,mask_A2H2)) {
      if (And(BlackBishops,SetMask(A2)) && And(mask_B2B3,WhitePawns)) {
        if (And(SetMask(B3),WhitePawns) || Swap(tree,B2,B3,1)>=0)
          score+=BISHOP_TRAPPED;
      }
      else if (And(BlackBishops,SetMask(H2)) && And(mask_G2G3,WhitePawns)) {
        if (And(SetMask(G3),WhitePawns) || Swap(tree,G2,G3,1)>=0)
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
/*
 ----------------------------------------------------------
|                                                          |
|   white king.                                            |
|                                                          |
|   first, check for a weak back rank.                     |
|                                                          |
 ----------------------------------------------------------
*/
  if (TotalBlackPieces < EG_MAT) score+=king_value_w[WhiteKingSQ];
  if (WhiteKingSQ < A2) {
    if (!And(And(king_attacks[WhiteKingSQ],rank_mask[RANK2]),
             Compl(WhitePawns))) score-=KING_BACK_RANK;
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
      if (And(WhiteRooks,mask_kr_trapped_w[FILEH-WhiteKingSQ])) 
        score-=ROOK_TRAPPED;
    }
    else if (WhiteKingSQ < D1) {
      if (And(WhiteRooks,mask_qr_trapped_w[WhiteKingSQ]))
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
  if (TotalWhitePieces < EG_MAT) score-=king_value_b[BlackKingSQ];
  if (BlackKingSQ > H7) {
    if (!And(And(king_attacks[BlackKingSQ],rank_mask[RANK7]),
             Compl(BlackPawns))) score+=KING_BACK_RANK;
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
      if (And(BlackRooks,mask_kr_trapped_b[FILEH-File(BlackKingSQ)]))
        score+=ROOK_TRAPPED;
    }
    else if (BlackKingSQ < D8) {
      if (And(BlackRooks,mask_qr_trapped_b[File(BlackKingSQ)]))
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
        !And(mask_no_pawn_attacks_b[square],BlackPawns)) {
      score+=KNIGHT_OUTPOST*white_outpost[square];
      if(And(b_pawn_attacks[square],WhitePawns)) score+=KNIGHT_OUTPOST;
    }
/*
 ----------------------------------------------------------
|                                                          |
|   now fold in centralization score from the piece/square |
|   table "knight_value_*" and king tropism.               |
|                                                          |
 ----------------------------------------------------------
*/
    score+=knight_value_w[square];
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
        !And(mask_no_pawn_attacks_w[square],WhitePawns)) {
      score-=KNIGHT_OUTPOST*black_outpost[square];
      if (And(w_pawn_attacks[square],BlackPawns)) score-=KNIGHT_OUTPOST;
    }
/*
 ----------------------------------------------------------
|                                                          |
|   now fold in centralization score from the piece/square |
|   table "knight_value_*" and king tropism.               |
|                                                          |
 ----------------------------------------------------------
*/
    score-=knight_value_b[square];
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
    score+=bishop_value_w[square];
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
        !And(mask_no_pawn_attacks_b[square],BlackPawns)) {
      if(And(b_pawn_attacks[square],WhitePawns)) 
        score+=BISHOP_OUTPOST*white_outpost[square];
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
    score-=bishop_value_b[square];
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
        !And(mask_no_pawn_attacks_w[square],WhitePawns)) {
      if(And(w_pawn_attacks[square],BlackPawns)) 
        score-=BISHOP_OUTPOST*black_outpost[square];
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
    if (And(WhiteBishops,WhiteBishops-1))
      score+=BISHOP_PAIR;
    else {
      if (TotalBlackPieces <= EG_MAT) {
        if (And(WhiteBishops,light_squares))
          score-=PopCnt(And(WhitePawns,light_squares))*BISHOP_PLUS_PAWNS_ON_COLOR;
        else
          score-=PopCnt(And(WhitePawns,dark_squares))*BISHOP_PLUS_PAWNS_ON_COLOR;
      }
      if (And(tree->all_pawns,mask_fgh) && And(tree->all_pawns,mask_abc) &&
          TotalWhitePieces==3 && TotalBlackPieces==2) 
          score+=BISHOP_OVER_KNIGHT_ENDGAME;
    }
  }
  if (BlackBishops) {
    if (And(BlackBishops,BlackBishops-1))
      score-=BISHOP_PAIR;
    else {
      if (TotalWhitePieces <= EG_MAT) {
        if (And(BlackBishops,light_squares))
          score+=PopCnt(And(BlackPawns,light_squares))*BISHOP_PLUS_PAWNS_ON_COLOR;
        else
          score+=PopCnt(And(BlackPawns,dark_squares))*BISHOP_PLUS_PAWNS_ON_COLOR;
      }
      if (And(tree->all_pawns,mask_fgh) && And(tree->all_pawns,mask_abc) &&
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
    score+=rook_value_w[square];
/*
 ----------------------------------------------------------
|                                                          |
|   determine if the rook is on an open file.  if it is,   |
|   determine if this rook attacks another friendly rook,  |
|   making it difficult to drive the rooks off the file.   |
|   if the file is not open, see if it's only closed by a  |
|   enemy pawn that is weak.  if so, a rook here is still  |
|   strong.                                                |
|                                                          |
|   add in a bonus for occupying a rank or file close to   |
|   the king, if there are no friendly pawns on the same   |
|   file.                                                  |
|                                                          |
 ----------------------------------------------------------
*/
    if (!And(file_mask[file],tree->all_pawns)) {
      if (FileDistance(square,tree->b_kingsq) <=1)
        score+=ScaleDown(2*tree->b_safety,TotalWhitePieces);
      score+=ROOK_OPEN_FILE;
    }
    else if (!And(plus8dir[square],WhitePawns)) score+=ROOK_HALF_OPEN_FILE;
/*
 ----------------------------------------------------------
|                                                          |
|   see if the rook is behind a passed pawn.  if it is,    |
|   it is counted as though the file is open.              |
|                                                          |
 ----------------------------------------------------------
*/
    if (128>>file & tree->pawn_score.passed_w) {
      register const int pawnsq=LastOne(And(WhitePawns,file_mask[file]));
      if (square<pawnsq && !And(BlackPieces,SetMask(pawnsq+8)))
        score+=ROOK_BEHIND_PASSED_PAWN;
      if (And(AttacksFile(square),WhiteRooks)) score-=ROOK_BEHIND_PASSED_PAWN>>1;
    }
    if (128>>file & tree->pawn_score.passed_b) {
      register const int pawnsq=FirstOne(And(BlackPawns,file_mask[file]));
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
                                And(BlackPawns,rank_mask[RANK7]))) {
      score+=ScaleUp(ROOK_ON_7TH,TotalBlackPieces);
      if (tree->pawn_score.passed_w && BlackKingSQ>H7 &&
          !And(BlackPieces,mask_abs7_w)) score+=ROOK_ABSOLUTE_7TH;
      if (And(AttacksRank(square),Or(WhiteRooks,WhiteQueens)))
        score+=ROOK_CONNECTED_7TH_RANK;
    }
/*
 ----------------------------------------------------------
|                                                          |
|   adjust the king tropism as well.                       |
|                                                          |
 ----------------------------------------------------------
*/
    w_tropism+=king_tropism_r[Distance(square,tree->b_kingsq)];
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
    score-=rook_value_b[square];
/*
 ----------------------------------------------------------
|                                                          |
|   determine if the rook is on an open file.  if it is,   |
|   determine if this rook attacks another friendly rook,  |
|   making it difficult to drive the rooks off the file.   |
|   if the file is not open, see if it's only closed by a  |
|   enemy pawn that is weak.  if so, a rook here is still  |
|   strong.                                                |
|                                                          |
|   add in a bonus for occupying a rank or file close to   |
|   the king, if there are no friendly pawns on the same   |
|   file.                                                  |
|                                                          |
 ----------------------------------------------------------
*/
    if (!And(file_mask[file],tree->all_pawns)) {
      if (FileDistance(square,tree->w_kingsq) <= 1)
        score-=ScaleDown(2*tree->w_safety,TotalBlackPieces);
      score-=ROOK_OPEN_FILE;
    }
    else if (!And(minus8dir[square],BlackPawns)) score-=ROOK_HALF_OPEN_FILE;
/*
 ----------------------------------------------------------
|                                                          |
|   if not, see if the rook is behind a passed pawn.  if   |
|   it is, it is counted as though the file is open.       |
|                                                          |
 ----------------------------------------------------------
*/
    if (128>>file & tree->pawn_score.passed_b) {
      register const int pawnsq=FirstOne(And(BlackPawns,file_mask[file]));
      if (square>pawnsq && !And(WhitePieces,SetMask(pawnsq-8)))
        score-=ROOK_BEHIND_PASSED_PAWN;
      if (And(AttacksFile(square),BlackRooks)) score+=ROOK_BEHIND_PASSED_PAWN>>1;
    }
    if (128>>file & tree->pawn_score.passed_w) {
      register const int pawnsq=LastOne(And(WhitePawns,file_mask[file]));
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
                                And(WhitePawns,rank_mask[RANK2]))) {
      score-=ScaleUp(ROOK_ON_7TH,TotalWhitePieces);
      if (tree->pawn_score.passed_b && WhiteKingSQ<A2 &&
          !And(WhitePieces,mask_abs7_b)) score-=ROOK_ABSOLUTE_7TH;
      if (And(AttacksRank(square),Or(BlackRooks,BlackQueens)))
        score-=ROOK_CONNECTED_7TH_RANK;
    }
/*
 ----------------------------------------------------------
|                                                          |
|   adjust the king tropism as well.                       |
|                                                          |
 ----------------------------------------------------------
*/
    b_tropism+=king_tropism_r[Distance(square,tree->w_kingsq)];
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
    score+=queen_value_w[square];
/*
 ----------------------------------------------------------
|                                                          |
|   check to see if the queen is in a strong positiono on  |
|   the 7th rank supported by a rook on the 7th.  if so,   |
|   the positional advantage is almost overwhelming.       |
|                                                          |
 ----------------------------------------------------------
*/
    if ((Rank(square) == RANK7) && (And(BlackPawns,rank_mask[RANK7]) ||
         (BlackKingSQ > H7))) {
      if (And(AttacksRank(square),WhiteRooks)) score+=QUEEN_ROOK_ON_7TH_RANK;
    }
/*
 ----------------------------------------------------------
|                                                          |
|   add in a bonus for occupying a rank or file close to   |
|   the king.                                              |
|                                                          |
 ----------------------------------------------------------
*/
    w_tropism+=king_tropism_q[Distance(square,tree->b_kingsq)];
/*
 ----------------------------------------------------------
|                                                          |
|   add in a penalty for being on the wrong side of the    |
|   board, which can lead to attacks by the opponent.      |
|                                                          |
 ----------------------------------------------------------
*/
    b_tropism+=king_tropism_file_q[FileDistance(square,tree->b_kingsq)];
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
    score-=queen_value_b[square];
/*
 ----------------------------------------------------------
|                                                          |
|   check to see if the queen is in a strong positiono on  |
|   the 7th rank supported by a rook on the 7th.  if so,   |
|   the positional advantage is almost overwhelming.       |
|                                                          |
 ----------------------------------------------------------
*/
    if ((Rank(square) == RANK2) && (And(WhitePawns,rank_mask[RANK2]) ||
         (WhiteKingSQ < A2))) {
      if (And(AttacksRank(square),BlackRooks)) score-=QUEEN_ROOK_ON_7TH_RANK;
    }
/*
 ----------------------------------------------------------
|                                                          |
|   add in a bonus for occupying a rank or file close to   |
|   the king.                                              |
|                                                          |
 ----------------------------------------------------------
*/
    b_tropism+=king_tropism_q[Distance(square,tree->w_kingsq)];
/*
 ----------------------------------------------------------
|                                                          |
|   add in a penalty for being on the wrong side of the    |
|   board, which can lead to attacks by the opponent.      |
|                                                          |
 ----------------------------------------------------------
*/
    w_tropism+=king_tropism_file_q[FileDistance(square,tree->w_kingsq)];
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
|   the opponent's king.                                   |
|                                                          |
 ----------------------------------------------------------
*/
  score+=tropism[w_tropism]-tropism[b_tropism];
#ifdef DEBUGEV
  if (score != lastsc)
    printf("score[king tropism]=              %4d (%+d)\n",score,score-lastsc);
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

/* last modified 02/11/99 */
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
int EvaluateDevelopment(TREE *tree, int ply) {
  register int possible, real, score=0;
  BITBOARD unmoved_pieces;

/*
 ----------------------------------------------------------
|                                                          |
|   first, some "thematic" things, which includes don't    |
|   block the c-pawn in queen-pawn openings, then also     |
|   check to see if center pawns are blocked.              |
|                                                          |
 ----------------------------------------------------------
*/
  if (root_wtm) {
    if (!And(SetMask(E4),WhitePawns) && And(SetMask(D4),WhitePawns)) {
      if (And(SetMask(C2),WhitePawns) &&
          And(SetMask(C3),Or(WhiteKnights,WhiteBishops)))
      score-=DEVELOPMENT_THEMATIC;
    }
    if (And(Occupied,And(Shiftr(And(WhitePawns,rank_mask[RANK2]),8),
                              Or(file_mask[FILED],file_mask[FILEE]))))
      score-=DEVELOPMENT_BLOCKED_PAWN;
  }
  else {
    if (!And(SetMask(E4),WhitePawns) && And(SetMask(D4),WhitePawns)) {
      if (And(SetMask(C7),BlackPawns) &&
          And(SetMask(C6),Or(BlackKnights,BlackBishops)))
      score+=DEVELOPMENT_THEMATIC;
    }
    if (And(Occupied,And(Shiftl(And(BlackPawns,rank_mask[RANK7]),8),
                              Or(file_mask[FILED],file_mask[FILEE]))))
      score+=DEVELOPMENT_BLOCKED_PAWN;
  }
#ifdef DEBUGDV
  printf("development.1 score=%d\n", score);
#endif
/*
 ----------------------------------------------------------
|                                                          |
|   if all minor pieces aren't developed, then penalize    |
|   the queen if it has moved.                             |
|                                                          |
 ----------------------------------------------------------
*/
  if ((unmoved_pieces=And(Or(WhiteKnights,WhiteBishops),white_minor_pieces))) {
    const int unmoved=PopCnt(unmoved_pieces);
    score-=unmoved*DEVELOPMENT_UNMOVED;
    if ((unmoved>1 || WhiteCastle(ply)>0) &&
        !And(WhiteQueens,SetMask(D1))) score-=DEVELOPMENT_QUEEN_EARLY;
  }
  if ((unmoved_pieces=And(Or(BlackKnights,BlackBishops),black_minor_pieces))) {
    const int unmoved=PopCnt(unmoved_pieces);
    score+=unmoved*DEVELOPMENT_UNMOVED;
    if ((unmoved>1 || BlackCastle(ply)>0) &&
        !And(BlackQueens,SetMask(D8))) score+=DEVELOPMENT_QUEEN_EARLY;
  }
#ifdef DEBUGDV
  printf("development.2 score=%d\n",score);
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
    if (possible < real) score-=KingSafety(temper[real-possible],TotalBlackPieces+
                                           ((BlackQueens)?QUEEN_KING_SAFETY:0));
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
    if (possible < real) score+=KingSafety(temper[real-possible],TotalWhitePieces+
                                           ((WhiteQueens)?QUEEN_KING_SAFETY:0));
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
  printf("development.3 score=%d\n",score);
#endif
  return(score);
}

int EvaluateDraws(TREE *tree) {
  register int square;
/*
 ----------------------------------------------------------
|                                                          |
|   if lots of material is left, it's not a draw.          |
|                                                          |
 ----------------------------------------------------------
*/
  if (TotalWhitePieces >= 5 || TotalBlackPieces >=5) return(0);
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
  if (TotalBlackPieces==0 && TotalWhitePawns &&
      !And(WhitePawns,not_rook_pawns)) {
    if (TotalWhitePieces==3) {
      if (And(WhiteBishops,dark_squares)) {
        if (And(file_mask[FILEH],WhitePawns)) return(0);
      }
      else if (And(file_mask[FILEA],WhitePawns)) return(0);
    }
    else if (TotalWhitePieces==0) {
      if (And(file_mask[FILEA],WhitePawns) &&
          And(file_mask[FILEH],WhitePawns)) return(0);
    }
    else return(0);

    if (!And(WhitePawns,file_mask[FILEA]) ||
        !And(WhitePawns,file_mask[FILEH])) {
      square=LastOne(WhitePawns);
      if (Rank(BlackKingSQ) >= Rank(square))
        if (FileDistance(BlackKingSQ,square)<=1) return(1);
      return(0);
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
  if (TotalWhitePieces==0 && TotalBlackPawns &&
      !And(BlackPawns,not_rook_pawns)) {
    if (TotalBlackPieces==3) {
      if (And(BlackBishops,dark_squares)) {
        if (And(file_mask[FILEA],BlackPawns)) return(0);
      }
      else if (And(file_mask[FILEH],BlackPawns)) return(0);
    }
    else if (TotalBlackPieces==0) {
      if (And(file_mask[FILEA],BlackPawns) &&
          And(file_mask[FILEH],BlackPawns)) return(0);
    }
    else return(0);

    if (!And(BlackPawns,file_mask[FILEA]) ||
        !And(BlackPawns,file_mask[FILEH])) {
      square=FirstOne(BlackPawns);
      if (Rank(WhiteKingSQ) <= Rank(square))
        if (FileDistance(WhiteKingSQ,square)<=1) return(1);
      return(0);
    }
  }
/*
 ----------------------------------------------------------
|                                                          |
|   if both sides have pawns, the game is not a draw for   |
|   lack of material.  also, if one side has at least a    |
|   B+N, then it's not a drawn position.                   |
|                                                          |
 ----------------------------------------------------------
*/
  if (!TotalWhitePawns && !TotalBlackPawns &&
      TotalWhitePieces < 5 && TotalBlackPieces < 5) return(1);
  if (TotalWhitePawns == 0 && TotalWhitePieces < 4) return(-1);
  else if (TotalBlackPawns == 0 && TotalBlackPieces < 4) return(-2);
  return(0);
}

/* last modified 05/27/98 */
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
  register int mate_score=0;
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
  if ((TotalBlackPieces==0) && (TotalWhitePieces==5) &&
      (!WhitePawns) && (!BlackPawns) && WhiteBishops) {
    if (And(dark_squares,WhiteBishops))
      mate_score=b_n_mate_dark_squares[BlackKingSQ];
    else
      mate_score=b_n_mate_light_squares[BlackKingSQ];
  }
  if ((TotalBlackPieces==5) && (TotalWhitePieces==0) &&
      (!WhitePawns) && (!BlackPawns) && BlackBishops) {
    if (And(dark_squares,BlackBishops))
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

/* last modified 02/10/99 */
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
      if (!And(tree->all_pawns,file_mask[FILEH])) {
        if (BlackRooks && BlackQueens) score-=KING_SAFETY_MATE_THREAT;
      }
    }
    if (!root_wtm && File(BlackKingSQ) >= FILEE) {
      if (!And(tree->all_pawns,file_mask[FILEH])) {
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
  tree->w_kingsq=WhiteKingSQ;
  tree->b_kingsq=BlackKingSQ;
  if (TotalWhitePieces>EG_MAT && TotalBlackPieces>EG_MAT) {
    tree->w_safety=king_defects_w[WhiteKingSQ];
    if (WhiteCastle(ply) <= 0) {
      if (File(WhiteKingSQ) >= FILEE) {
        if (File(WhiteKingSQ) == FILEH)
          tree->w_kingsq=(Rank(WhiteKingSQ)<<3)+FILEG;
        tree->w_safety+=tree->pawn_score.white_defects_k;
        if (!And(WhitePawns,SetMask(G2))) {
          if (And(WhitePawns,SetMask(G3)) && Distance(WhiteKingSQ,G2)==1 &&
              And(WhiteBishops,good_bishop_kw))
            tree->w_safety-=KING_SAFETY_GOOD_BISHOP;
          if ((And(SetMask(H3),BlackPawns) || And(mask_F3H3,BlackBishops)) &&
              BlackQueens) tree->w_safety+=KING_SAFETY_MATE_G2G7;
        }
        if (File(WhiteKingSQ)==FILEE) {
          if (!And(tree->all_pawns,file_mask[FILED]))
            tree->w_safety+=KING_SAFETY_1OPEN_FILE>>1;
          if (!And(tree->all_pawns,file_mask[FILEE]))
            tree->w_safety+=KING_SAFETY_1OPEN_FILE;
          if (!And(tree->all_pawns,file_mask[FILEF]))
            tree->w_safety+=KING_SAFETY_1OPEN_FILE>>1;
        }
      }
      else if (File(WhiteKingSQ) <= FILED) {
        if (File(WhiteKingSQ) == FILEA)
          tree->w_kingsq=(Rank(WhiteKingSQ)<<3)+FILEB;
        tree->w_safety+=tree->pawn_score.white_defects_q;
        if (!And(WhitePawns,SetMask(B2))) {
          if (And(WhitePawns,SetMask(B3)) && Distance(WhiteKingSQ,B2)==1 &&
              And(WhiteBishops,good_bishop_qw))
            tree->w_safety-=KING_SAFETY_GOOD_BISHOP;
          if ((And(SetMask(A3),BlackPawns) || And(mask_A3C3,BlackBishops)) &&
              BlackQueens) tree->w_safety+=KING_SAFETY_MATE_G2G7;
        }
      }
      if (File(WhiteKingSQ)==FILED) {
        if (!And(tree->all_pawns,file_mask[FILEC]))
          tree->w_safety+=KING_SAFETY_1OPEN_FILE>>1;
        if (!And(tree->all_pawns,file_mask[FILED]))
          tree->w_safety+=KING_SAFETY_1OPEN_FILE;
        if (!And(tree->all_pawns,file_mask[FILEE]))
          tree->w_safety+=KING_SAFETY_1OPEN_FILE>>1;
      }
    }
    else {
      if (WhiteCastle(ply) != 3) {
        if (WhiteCastle(ply) & 1)
          tree->w_safety+=tree->pawn_score.white_defects_k;
        else if (WhiteCastle(ply) & 2)
          tree->w_safety+=tree->pawn_score.white_defects_q;
      }
      if (!And(tree->all_pawns,file_mask[FILED]))
        tree->w_safety+=KING_SAFETY_1OPEN_FILE>>1;
      if (!And(tree->all_pawns,file_mask[FILEE]))
        tree->w_safety+=KING_SAFETY_1OPEN_FILE;
      if (!And(tree->all_pawns,file_mask[FILEF]))
        tree->w_safety+=KING_SAFETY_1OPEN_FILE>>1;
    }

    tree->b_safety=king_defects_b[BlackKingSQ];
    if (BlackCastle(ply) <= 0) {
      if (File(BlackKingSQ) >= FILEE) {
        if (File(BlackKingSQ) == FILEH)
          tree->b_kingsq=(Rank(BlackKingSQ)<<3)+FILEG;
        tree->b_safety+=tree->pawn_score.black_defects_k;
        if (!And(BlackPawns,SetMask(G7))) {
          if (And(BlackPawns,SetMask(G6)) && Distance(BlackKingSQ,G7)==1 &&
              And(BlackBishops,good_bishop_kb))
            tree->b_safety-=KING_SAFETY_GOOD_BISHOP;
          if ((And(SetMask(H6),WhitePawns) || And(mask_F6H6,WhiteBishops)) &&
              WhiteQueens) tree->b_safety+=KING_SAFETY_MATE_G2G7;
        }
        if (File(BlackKingSQ)==FILEE) {
          if (!And(tree->all_pawns,file_mask[FILED]))
            tree->b_safety+=KING_SAFETY_1OPEN_FILE>>1;
          if (!And(tree->all_pawns,file_mask[FILEE]))
            tree->b_safety+=KING_SAFETY_1OPEN_FILE;
          if (!And(tree->all_pawns,file_mask[FILEF]))
            tree->b_safety+=KING_SAFETY_1OPEN_FILE>>1;
        }
      }
      else if (File(BlackKingSQ) <= FILED) {
        if (File(BlackKingSQ) == FILEA)
          tree->b_kingsq=(Rank(BlackKingSQ)<<3)+FILEB;
        tree->b_safety+=tree->pawn_score.black_defects_q;
        if (File(BlackKingSQ) == FILEA)
          tree->b_kingsq=(Rank(BlackKingSQ)<<3)+FILEB;
        if (!And(BlackPawns,SetMask(B7))) {
          if (And(BlackPawns,SetMask(B6)) && Distance(BlackKingSQ,B7)==1 &&
              And(BlackBishops,good_bishop_qb))
            tree->b_safety-=KING_SAFETY_GOOD_BISHOP;
          if ((And(SetMask(A6),WhitePawns) || And(mask_A6C6,WhiteBishops)) &&
              WhiteQueens) tree->b_safety+=KING_SAFETY_MATE_G2G7;
        }
        if (File(BlackKingSQ)==FILED) {
          if (!And(tree->all_pawns,file_mask[FILEC]))
            tree->b_safety+=KING_SAFETY_1OPEN_FILE>>1;
          if (!And(tree->all_pawns,file_mask[FILED]))
            tree->b_safety+=KING_SAFETY_1OPEN_FILE;
          if (!And(tree->all_pawns,file_mask[FILEE]))
            tree->b_safety+=KING_SAFETY_1OPEN_FILE>>1;
        }
      }
    }
    else {
      if (BlackCastle(ply) != 3) {
        if (BlackCastle(ply) & 1)
          tree->b_safety+=tree->pawn_score.black_defects_k;
        else if (BlackCastle(ply) & 2)
          tree->b_safety+=tree->pawn_score.black_defects_q;
      }
      if (!And(tree->all_pawns,file_mask[FILED]))
        tree->b_safety+=KING_SAFETY_1OPEN_FILE>>1;
      if (!And(tree->all_pawns,file_mask[FILEE]))
        tree->b_safety+=KING_SAFETY_1OPEN_FILE;
      if (!And(tree->all_pawns,file_mask[FILEF]))
        tree->b_safety+=KING_SAFETY_1OPEN_FILE>>1;
    }
    tree->w_safety=temper[Max(tree->w_safety,0)];
    tree->b_safety=temper[Max(tree->b_safety,0)];
  }
  else {
    tree->w_safety=0;
    tree->b_safety=0;
  }
  score-=KingSafety(tree->w_safety,TotalBlackPieces+
                    ((BlackQueens)?QUEEN_KING_SAFETY:0));
  score+=KingSafety(tree->b_safety,TotalWhitePieces+
                    ((WhiteQueens)?QUEEN_KING_SAFETY:0));
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
      square=FirstOne(And(BlackPawns,file_mask[file]));
      if (TotalWhitePieces < 20)
        score-=reduced_material_passer[TotalWhitePieces]*(RANK8-Rank(square));
      if (FileDistance(square,black_king_sq)==1 &&
          Rank(black_king_sq)<=Rank(square))
        score-=supported_passer[RANK8-Rank(square)];
      if (And(SetMask(square-8),WhitePieces))
        score+=passed_pawn_value[RANK8-Rank(square)]>>1;
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
      square1=FirstOne(And(BlackPawns,file_mask[file-1]));
      square2=FirstOne(And(BlackPawns,file_mask[file]));
      score-=connected_passed_pawn_value[7-Min(Rank(square1),Rank(square2))];
      if (Rank(square1) > RANK3) continue;
      if (Rank(square2) > RANK3) continue;
      score-=PAWN_CONNECTED_PASSED_6TH;
      if (TotalWhitePieces < queen_v &&
          !And(SetMask(square1-8),WhitePieces) &&
          !And(SetMask(square2-8),WhitePieces)) {
        score-=2*PAWN_CONNECTED_PASSED_6TH;
        if ((TotalWhitePieces <= rook_v) && 
            (!And(WhiteKing,black_pawn_race_btm[square1]) ||
             !And(WhiteKing,black_pawn_race_btm[square2])))
          score-=6*PAWN_CONNECTED_PASSED_6TH;
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
      square=LastOne(And(WhitePawns,file_mask[file]));
      if (TotalBlackPieces < 20)
        score+=reduced_material_passer[TotalBlackPieces]*Rank(square);
      if (FileDistance(square,white_king_sq)==1 &&
          Rank(white_king_sq)>=Rank(square)) 
        score+=supported_passer[Rank(square)];
      if (And(SetMask(square+8),BlackPieces))
        score-=passed_pawn_value[Rank(square)]>>1;
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
      score+=PAWN_CONNECTED_PASSED;
      pawns&=~(128>>file);
      square1=LastOne(And(WhitePawns,file_mask[file-1]));
      square2=LastOne(And(WhitePawns,file_mask[file]));
      score+=connected_passed_pawn_value[Max(Rank(square1),Rank(square2))];
      if (Rank(square1) < RANK6) continue;
      if (Rank(square2) < RANK6) continue;
      score+=PAWN_CONNECTED_PASSED_6TH;
      if (TotalBlackPieces < queen_v &&
          !And(SetMask(square1+8),BlackPieces) &&
          !And(SetMask(square2+8),BlackPieces)) {
        score+=2*PAWN_CONNECTED_PASSED_6TH;
        if ((TotalBlackPieces <= rook_v) && 
            (!And(BlackKing,white_pawn_race_wtm[square1]) ||
             !And(BlackKing,white_pawn_race_wtm[square2])))
          score+=6*PAWN_CONNECTED_PASSED_6TH;
      }
    }
  }
#ifdef DEBUGPP
  printf("score.2 after white passers = %d\n", score);
#endif
  if (TotalBlackPawns==1 && TotalWhitePawns==0 &&
      TotalBlackPieces==5 && BlackRooks &&
      TotalWhitePieces==5 && WhiteRooks) {
    square=FirstOne(BlackPawns);
    if (FileDistance(WhiteKingSQ,square)<=1 &&
        Rank(WhiteKingSQ)<Rank(square)) return(0);
    if (Rank(BlackKingSQ)>Rank(square) ||
        FileDistance(BlackKingSQ,square) > 1) return(0);
  }
  if (TotalWhitePawns==1 && TotalBlackPawns==0 &&
      TotalWhitePieces==5 && WhiteRooks &&
      TotalBlackPieces==5 && BlackRooks) {
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
          (Distance(WhiteKingSQ,56) < Distance(BlackKingSQ,56)))
        return(QUEEN_VALUE-BISHOP_VALUE);
      break;
    }
    else if (File(pawnsq) == FILEH) {
      if ((File(WhiteKingSQ) == FILEG) &&
          (Distance(WhiteKingSQ,63) < Distance(BlackKingSQ,63)))
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
          (Distance(BlackKingSQ,0) < Distance(WhiteKingSQ,0)))
        return(-(QUEEN_VALUE-BISHOP_VALUE));
      break;
    }
    else if (File(pawnsq) == FILEH) {
      if ((File(WhiteKingSQ) == FILEG) &&
          (Distance(BlackKingSQ,8) < Distance(WhiteKingSQ,8)))
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
      square=FirstOne(And(BlackPawns,file_mask[file]));
      if ((wtm && !And(black_pawn_race_wtm[square],WhiteKing)) ||
          (ChangeSide(wtm) && !And(black_pawn_race_btm[square],WhiteKing))) {
        queen_distance=Rank(square);
        if (And(BlackKing,minus8dir[square])) {
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
      square=LastOne(And(WhitePawns,file_mask[file]));
      if ((wtm && !And(white_pawn_race_wtm[square],BlackKing)) ||
          (ChangeSide(wtm) && !And(white_pawn_race_btm[square],BlackKing))) {
        queen_distance=RANK8-Rank(square);
        if (And(WhiteKing,plus8dir[square])) {
          if (file==FILEA || file==FILEH) queen_distance=99;
          queen_distance++;
        }
        if (Rank(square) == RANK2) queen_distance--;
        if (queen_distance < white_queener) {
          white_queener=queen_distance;
          white_square=file+56;
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
      WhitePieces=Or(WhitePieces,SetMask(white_square));
      tempb=BlackPieces;
      Clear(black_pawn,BlackPieces);
      BlackPieces=Or(BlackPieces,SetMask(black_square));
      if (Attack(BlackKingSQ,white_square,ply)) {
        WhitePieces=tempw;
        BlackPieces=tempb;
        return(QUEEN_VALUE-BISHOP_VALUE+(5-white_queener)*PAWN_VALUE/10);
      }
      if (Attack(black_square,white_square,ply) &&
          !And(king_attacks[black_square],BlackKing)) {
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
      WhitePieces=Or(WhitePieces,SetMask(white_square));
      tempb=BlackPieces;
      Clear(black_pawn,BlackPieces);
      BlackPieces=Or(BlackPieces,SetMask(black_square));
      if (Attack(WhiteKingSQ,black_square,ply)) {
        WhitePieces=tempw;
        BlackPieces=tempb;
        return(-(QUEEN_VALUE-BISHOP_VALUE+(5-black_queener)*PAWN_VALUE/10));
      }
      if (Attack(white_square,black_square,ply) &&
          !And(king_attacks[white_square],WhiteKing)) {
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

/* last modified 02/09/99 */
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
  register int pns, square, file;
  register int w_isolated=0, b_isolated=0;
  register int w_unblocked=0, b_unblocked=0;
  register int w_file_l, w_file_r, b_file_l, b_file_r;
  register int defenders, attackers, weakness, blocked, sq;
  register int kside_open_files=0, kside_half_open_w=0, kside_half_open_b=0;
  register int qside_open_files=0, qside_half_open_w=0, qside_half_open_b=0;
  register int qmissb=0, kmissb=0, qmissw=0, kmissw=0;
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
  Lock(lock_pawn_hash);
  if (ptable->key == PawnHashKey) {
#if !defined(FAST)
    tree->pawn_hits++;
#endif
    tree->pawn_score=*ptable;
    UnLock(lock_pawn_hash);
    return(tree->pawn_score.p_score);
  }
  UnLock(lock_pawn_hash);
  tree->pawn_score.key=PawnHashKey;
  tree->pawn_score.p_score=0;
  tree->pawn_score.passed_w=0;
  tree->pawn_score.passed_b=0;
  tree->pawn_score.white_protected=0;
  tree->pawn_score.black_protected=0;
  tree->pawn_score.white_defects_k=0;
  tree->pawn_score.white_defects_q=0;
  tree->pawn_score.black_defects_k=0;
  tree->pawn_score.black_defects_q=0;
  tree->pawn_score.outside=0;
/*
 ----------------------------------------------------------
|                                                          |
|   if there are 8 pawns of one color, penalize crafty     |
|   for not opening the position.                          |
|                                                          |
 ----------------------------------------------------------
*/
  if (root_wtm) {
    if (TotalWhitePawns == 8) tree->pawn_score.p_score-=EIGHT_PAWNS;
  }
  else {
    if (TotalBlackPawns == 8) tree->pawn_score.p_score+=EIGHT_PAWNS;
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
    for (sq=square;sq<A7;sq+=8) {
      wp_moves|=SetMask(sq);
      if (And(SetMask(sq+8),tree->all_pawns)) break;
      defenders=PopCnt(And(b_pawn_attacks[sq+8],WhitePawns));
      attackers=PopCnt(And(w_pawn_attacks[sq+8],BlackPawns));
      if (attackers-defenders == 2) break;
    }
    Clear(square,pawns);
  }
  pawns=BlackPawns;
  bp_moves=0;
  while (pawns) {
    square=FirstOne(pawns);
    for (sq=square;sq>H2;sq-=8) {
      bp_moves|=SetMask(sq);
      if (And(SetMask(sq-8),tree->all_pawns)) break;
      attackers=PopCnt(And(b_pawn_attacks[sq-8],WhitePawns));
      defenders=PopCnt(And(w_pawn_attacks[sq-8],BlackPawns));
      if (attackers-defenders == 2) break;
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
    tree->pawn_score.p_score+=pawn_value_w[square];
#ifdef DEBUGP
    if (tree->pawn_score.p_score != lastsc)
      printf("white pawn[static] file=%d,   score=%d\n",
             file,tree->pawn_score.p_score);
    lastsc=tree->pawn_score.p_score;
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
    if (!And(mask_pawn_isolated[square],WhitePawns)) {
      w_isolated++;
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
      if (And(mask_pawn_duo[square],WhitePawns))
        tree->pawn_score.p_score+=PAWN_DUO;
#ifdef DEBUGP
      if (tree->pawn_score.p_score != lastsc)
        printf("white pawn[duo] file=%d,      score=%d\n",
               file,tree->pawn_score.p_score);
      lastsc=tree->pawn_score.p_score;
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
      if (And(plus8dir[square],BlackPawns)) break;
      defenders=PopCnt(And(b_pawn_attacks[square],WhitePawns));
      attackers=PopCnt(And(w_pawn_attacks[square],BlackPawns));
      if (defenders > attackers) break;
      defenders=PopCnt(And(b_pawn_attacks[square+8],WhitePawns));
      attackers=PopCnt(And(w_pawn_attacks[square+8],BlackPawns));
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
      if ((temp=And(mask_no_pawn_attacks_w[square],WhitePawns))) {
        if (file > FILEA) {
          const BITBOARD temp1=And(temp,file_mask[file-1]);
          attackers=1;
          if (temp1) {
            const int defend_sq=LastOne(temp1);
            for (sq=defend_sq;sq<(Rank(square)<<3);sq+=8) {
              attackers=1;
              if (sq!=defend_sq && And(tree->all_pawns,SetMask(sq))) break;
              attackers=PopCnt(And(w_pawn_attacks[sq],BlackPawns));
              if (attackers) break;
            }
            if (!attackers) weakness=0;
          }
          if (!weakness) break;
        }
        if (file < FILEH) {
          const BITBOARD temp1=And(temp,file_mask[file+1]);
          if (temp1) {
            const int defend_sq=LastOne(temp1);
            for (sq=defend_sq;sq<(Rank(square)<<3);sq+=8) {
              attackers=1;
              if (sq != defend_sq && And(tree->all_pawns,SetMask(sq))) break;
              attackers=PopCnt(And(w_pawn_attacks[sq],BlackPawns));
              if (attackers) break;
            }
            if (!attackers) weakness=0;
          }
        }
      }

      if (weakness > 0) {
        if (weakness == 3) tree->pawn_score.p_score-=PAWN_WEAK_P2;
        else if (weakness) tree->pawn_score.p_score-=PAWN_WEAK_P1;
      }
    } while(0);
#ifdef DEBUGP
    if (tree->pawn_score.p_score != lastsc)
      printf("white pawn[weak] file=%d,     score=%d\n",
             file,tree->pawn_score.p_score);
    lastsc=tree->pawn_score.p_score;
#endif
/*
 ----------------------------------------------------------
|                                                          |
|   evaluate doubled pawns.  if there are other pawns on   |
|   this file, penalize this pawn.                         |
|                                                          |
 ----------------------------------------------------------
*/
    if ((pns=PopCnt(And(file_mask[file],WhitePawns))) > 1) {
      tree->pawn_score.p_score-=doubled_pawn_value[pns];
    }
#ifdef DEBUGP
    if (tree->pawn_score.p_score != lastsc)
      printf("white pawn[doubled] file=%d,     score=%d\n",
             file,tree->pawn_score.p_score);
    lastsc=tree->pawn_score.p_score;
#endif
/*
 ----------------------------------------------------------
|                                                          |
|   evaluate passed pawns.                                 |
|                                                          |
 ----------------------------------------------------------
*/
    if (!And(mask_pawn_passed_w[square],BlackPawns)) {
      tree->pawn_score.p_score+=passed_pawn_value[Rank(square)];
      if (And(minus8dir[square],WhitePawns))
        tree->pawn_score.p_score-=passed_pawn_value[Rank(square)]>>1;
      if (And(mask_pawn_protected_w[square],WhitePawns))
        tree->pawn_score.white_protected=1;
      tree->pawn_score.passed_w|=128>>file;
    }
#ifdef DEBUGP
    if (tree->pawn_score.p_score != lastsc)
      printf("white pawn[passed] file=%d,   score=%d\n",
             file,tree->pawn_score.p_score);
    lastsc=tree->pawn_score.p_score;
#endif
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
    if (Rank(square) > RANK5 && And(SetMask(square+8),BlackPawns) &&
        !And(mask_pawn_passed_w[square+8],BlackPawns) &&
        ((File(square) < FILEH && And(SetMask(square-7),WhitePawns) &&
          !And(plus8dir[square-7],BlackPawns) &&
          (File(square) == FILEG || !And(plus8dir[square-6],BlackPawns))) ||
         (File(square) > FILEA && And(SetMask(square-9),WhitePawns) &&
          !And(plus8dir[square-9],BlackPawns) &&
          (File(square) == FILEB || !And(plus8dir[square-10],BlackPawns))))) {
      tree->pawn_score.p_score+=passed_pawn_value[Rank(square)];
    }
#ifdef DEBUGP
    if (tree->pawn_score.p_score != lastsc)
      printf("white pawn[hidden] file=%d,   score=%d\n",
             file,tree->pawn_score.p_score);
    lastsc=tree->pawn_score.p_score;
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
        defenders=PopCnt(And(b_pawn_attacks[sq],wp_moves));
        attackers=PopCnt(And(w_pawn_attacks[sq],BlackPawns));
        if (attackers > defenders) break;
        else if (attackers) {
          blocked=0;
          break;
        }
        if (And(SetMask(sq+8),tree->all_pawns)) break;
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
    tree->pawn_score.p_score-=pawn_value_b[square];
#ifdef DEBUGP
    if (tree->pawn_score.p_score != lastsc)
      printf("black pawn[static] file=%d,   score=%d\n",
             file,tree->pawn_score.p_score);
    lastsc=tree->pawn_score.p_score;
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
    if (!And(mask_pawn_isolated[square],BlackPawns)) {
      b_isolated++;
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
      if (And(mask_pawn_duo[square],BlackPawns))
        tree->pawn_score.p_score-=PAWN_DUO;
#ifdef DEBUGP
      if (tree->pawn_score.p_score != lastsc)
        printf("black pawn[duo] file=%d,      score=%d\n",
               file,tree->pawn_score.p_score);
      lastsc=tree->pawn_score.p_score;
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
      if (And(minus8dir[square],WhitePawns)) break;
      attackers=PopCnt(And(b_pawn_attacks[square],WhitePawns));
      defenders=PopCnt(And(w_pawn_attacks[square],BlackPawns));
      if (defenders > attackers) break;
      attackers=PopCnt(And(b_pawn_attacks[square-8],WhitePawns));
      defenders=PopCnt(And(w_pawn_attacks[square-8],BlackPawns));
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
      if ((temp=And(mask_no_pawn_attacks_b[square],BlackPawns))) {
        if (file > FILEA) {
          const BITBOARD temp1=And(temp,file_mask[file-1]);
          attackers=1;
          if (temp1) {
            const int defend_sq=FirstOne(temp1);
            for (sq=defend_sq;sq>=((Rank(square)+1)<<3);sq-=8) {
              attackers=1;
              if (sq!=defend_sq && And(tree->all_pawns,SetMask(sq))) break;
              attackers=PopCnt(And(b_pawn_attacks[sq],WhitePawns));
              if (attackers) break;
            }
            if (!attackers) weakness=0;
          }
          if (!weakness) break;
        }
        if (file < FILEH) {
          const BITBOARD temp1=And(temp,file_mask[file+1]);
          if (temp1) {
            const int defend_sq=FirstOne(temp1);
            for (sq=defend_sq;sq>=((Rank(square)+1)<<3);sq-=8) {
              attackers=1;
              if (sq!=defend_sq && And(tree->all_pawns,SetMask(sq))) break;
              attackers=PopCnt(And(b_pawn_attacks[sq],WhitePawns));
              if (attackers) break;
            }
            if (!attackers) weakness=0;
          }
        }
      }

      if (weakness > 0) {
        if (weakness == 3) tree->pawn_score.p_score+=PAWN_WEAK_P2;
        else if (weakness) tree->pawn_score.p_score+=PAWN_WEAK_P1;
        else tree->pawn_score.p_score+=PAWN_WEAK_P2;
      }
    } while(0);
#ifdef DEBUGP
    if (tree->pawn_score.p_score != lastsc)
      printf("black pawn[weak] file=%d,     score=%d\n",
             file,tree->pawn_score.p_score);
    lastsc=tree->pawn_score.p_score;
#endif
/*
 ----------------------------------------------------------
|                                                          |
|   evaluate doubled pawns.  if there are other pawns on   |
|   this file, penalize this pawn.                         |
|                                                          |
 ----------------------------------------------------------
*/
    if ((pns=PopCnt(And(file_mask[file],BlackPawns))) > 1) {
      tree->pawn_score.p_score+=doubled_pawn_value[pns];
    }
#ifdef DEBUGP
    if (tree->pawn_score.p_score != lastsc)
      printf("black pawn[doubled] file=%d,     score=%d\n",
             file,tree->pawn_score.p_score);
    lastsc=tree->pawn_score.p_score;
#endif
/*
 ----------------------------------------------------------
|                                                          |
|   evaluate passed pawns.                                 |
|                                                          |
 ----------------------------------------------------------
*/
    if (!And(mask_pawn_passed_b[square],WhitePawns)) {
      tree->pawn_score.p_score-=passed_pawn_value[(RANK8-Rank(square))];
      if (And(plus8dir[square],BlackPawns))
        tree->pawn_score.p_score+=passed_pawn_value[Rank(square)]>>1;
      if (And(mask_pawn_protected_b[square],BlackPawns))
        tree->pawn_score.black_protected=1;
      tree->pawn_score.passed_b|=128>>file;
    }
#ifdef DEBUGP
    if (tree->pawn_score.p_score != lastsc)
      printf("black pawn[passed] file=%d,   score=%d\n",
             file,tree->pawn_score.p_score);
    lastsc=tree->pawn_score.p_score;
#endif
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
    if (Rank(square) < RANK4 && And(SetMask(square-8),WhitePawns) &&
        !And(mask_pawn_passed_b[square-8],WhitePawns) &&
        ((File(square) < FILEH && And(SetMask(square+9),BlackPawns) &&
          !And(minus8dir[square+9],WhitePawns) &&
          (File(square) == FILEG || !And(minus8dir[square+10],WhitePawns))) ||
         (File(square) > FILEA && And(SetMask(square+7),BlackPawns) &&
          !And(minus8dir[square+7],WhitePawns) &&
          (File(square) == FILEB || !And(minus8dir[square+6],WhitePawns))))) {
      tree->pawn_score.p_score-=passed_pawn_value[(RANK8-Rank(square))];
    }
#ifdef DEBUGP
    if (tree->pawn_score.p_score != lastsc)
      printf("black pawn[hidden] file=%d,   score=%d\n",
             file,tree->pawn_score.p_score);
    lastsc=tree->pawn_score.p_score;
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
        attackers=PopCnt(And(b_pawn_attacks[sq],WhitePawns));
        defenders=PopCnt(And(w_pawn_attacks[sq],bp_moves));
        if (attackers > defenders) break;
        else if (attackers) {
          blocked=0;
          break;
        }
        if (And(SetMask(sq-8),tree->all_pawns)) break;
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
  tree->pawn_score.p_score-=isolated_pawn_value[w_isolated];
  tree->pawn_score.p_score+=isolated_pawn_value[b_isolated];
#ifdef DEBUGP
  if (tree->pawn_score.p_score != lastsc)
    printf("pawn[isolated]          score=%d\n",tree->pawn_score.p_score);
  lastsc=tree->pawn_score.p_score;
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
  if (TotalWhitePawns>5 && TotalBlackPawns>5) {
    if (w_unblocked == 0) tree->pawn_score.p_score+=PAWNS_BLOCKED;
    if (b_unblocked == 0) tree->pawn_score.p_score-=PAWNS_BLOCKED;
  }
#ifdef DEBUGP
  if (tree->pawn_score.p_score != lastsc)
    printf("pawn[unblocked]         score=%d\n",tree->pawn_score.p_score);
  lastsc=tree->pawn_score.p_score;
#endif
/*
 ----------------------------------------------------------
|                                                          |
|   now fold in the penalty for "rams" which are pawns     |
|   that are blocked by a pawn on the same file.           |
|                                                          |
 ----------------------------------------------------------
*/
  if (root_wtm)
    tree->pawn_score.p_score-=
      PAWN_RAM*PopCnt(And(WhitePawns,Shiftl(BlackPawns,8)));
  else
    tree->pawn_score.p_score+=
      PAWN_RAM*PopCnt(And(WhitePawns,Shiftl(BlackPawns,8)));
#ifdef DEBUGP
  if (tree->pawn_score.p_score != lastsc)
    printf("pawn[rams]              score=%d\n",tree->pawn_score.p_score);
  lastsc=tree->pawn_score.p_score;
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
    if (!(WhitePawns & set_mask[A2+file])) {
      qmissw++;
      if(!(WhitePawns & set_mask[A3+file])) qmissw++;
    }
    if (!(WhitePawns & set_mask[H2-file])) {
      kmissw++;
      if(!(WhitePawns & set_mask[H3-file])) kmissw++;
    }
    if (!(BlackPawns & set_mask[A7+file])) {
      qmissb++;
      if(!(BlackPawns & set_mask[A6+file])) qmissb++;
    }
    if (!(BlackPawns & set_mask[H7-file])) {
      kmissb++;
      if(!(BlackPawns & set_mask[H6-file])) kmissb++;
    }
    if (!(left & tree->all_pawns)) qside_open_files++;
    else {
      if (!(left & WhitePawns)) qside_half_open_w++;
      else if (!(left & BlackPawns)) qside_half_open_b++;
    }
    if (!(right & tree->all_pawns)) kside_open_files++;
    else {
      if (!(right & WhitePawns)) kside_half_open_w++;
      else if (!(right & BlackPawns)) kside_half_open_b++;
    }
    left=left>>1;
    right=right<<1;
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
                                   hopenf[kside_half_open_w]+
                                   hopenfo[kside_half_open_b];
  tree->pawn_score.white_defects_q=missing[qmissw]+
                                   openf[qside_open_files]+
                                   hopenf[qside_half_open_w]+
                                   hopenfo[qside_half_open_b];
  tree->pawn_score.black_defects_k=missing[kmissb]+
                                   openf[kside_open_files]+
                                   hopenf[kside_half_open_b]+
                                   hopenfo[kside_half_open_w];
  tree->pawn_score.black_defects_q=missing[qmissb]+
                                   openf[qside_open_files]+
                                   hopenf[qside_half_open_b]+
                                   hopenfo[qside_half_open_w];
#if defined(DEBUGK)
  printf("white: kmissing=%d  kopen=%d  khalf=%d  khalfo=%d\n",
         kmissw,kside_open_files,kside_half_open_w, kside_half_open_b);
  printf("white: qmissing=%d  qopen=%d  qhalf=%d  qhalfo=%d\n",
         qmissw,qside_open_files,qside_half_open_w, qside_half_open_b);
  printf("black: kmissing=%d  kopen=%d  khalf=%d  khalfo=%d\n",
         kmissb,kside_open_files,kside_half_open_b, kside_half_open_w);
  printf("black: qmissing=%d  qopen=%d  qhalf=%d  qhalfo=%d\n",
         qmissb,qside_open_files,qside_half_open_b, qside_half_open_w);
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
    if (PopCnt(And(BlackPawns,stonewall_black)) == 3)
      tree->pawn_score.white_defects_k+=KING_SAFETY_STONEWALL;
    if (PopCnt(And(BlackPawns,closed_black)) == 3)
      tree->pawn_score.white_defects_k+=KING_SAFETY_CLOSED;
  }
  else {
    if (PopCnt(And(WhitePawns,stonewall_white)) == 3)
      tree->pawn_score.black_defects_k+=KING_SAFETY_STONEWALL;
    if (PopCnt(And(WhitePawns,closed_white)) == 3)
      tree->pawn_score.black_defects_k+=KING_SAFETY_CLOSED;
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
|  pawns for both sides. this returns a bonus if one side  |
|  has a passed pawn on the opposite side of the board     |
|  from the rest of the pawns.  the side with the most     |
|  remote (outside) pawn gets a bonus that dynamically     |
|  varies proportionally to the number of pieces on the    |
|  board so that it encourages the side with the outside   |
|  passer to trade down to a simple king and pawn ending   |
|  which the outside passer wins.                          |
|                                                          |
 ----------------------------------------------------------
*/
  tree->pawn_score.outside=0;
  w_file_l=first_ones_8bit[tree->pawn_score.passed_w];
  if (w_file_l == 8) w_file_l=9;
  b_file_l=first_ones_8bit[tree->pawn_score.passed_b];
  if (b_file_l == 8) b_file_l=9;
  w_file_r=last_ones_8bit[tree->pawn_score.passed_w];
  if (w_file_r == 8) w_file_r=-9;
  b_file_r=last_ones_8bit[tree->pawn_score.passed_b];
  if (b_file_r == 8) b_file_r=-9;
/*
 ------------------------------------------------
|                                                |
|  if one side has a passed pawn that is closer  |
|  to the side of the board than any other pawn, |
|  then this pawn is "outside" and valuable.     |
|                                                |
 ------------------------------------------------
*/
  if (w_file_l != 9) {
    if ((w_file_l < b_file_l-1) || (w_file_r > b_file_r+1)) do {
      if (w_file_l < 4) {
        if(And(tree->all_pawns,right_side_mask[w_file_l]) &&
           !And(BlackPawns,left_side_empty_mask[w_file_l])) {
          if (!tree->pawn_score.black_protected ||
              tree->pawn_score.white_protected) tree->pawn_score.outside|=1;
          break;
        }
      }
      if (w_file_r > 3) {
        if (And(tree->all_pawns,left_side_mask[w_file_r]) &&
            !And(BlackPawns,right_side_empty_mask[w_file_r])) {
          if (!tree->pawn_score.black_protected ||
              tree->pawn_score.white_protected) tree->pawn_score.outside|=1;
        }
      }
    } while(0);
  }
  if (b_file_l != 9) {
    if ((b_file_l < w_file_l-1) || (b_file_r > w_file_r+1)) do {
      if (b_file_l < 4) {
        if(And(tree->all_pawns,right_side_mask[b_file_l]) &&
           !And(WhitePawns,left_side_empty_mask[b_file_l])) {
          if (!tree->pawn_score.white_protected ||
              tree->pawn_score.black_protected) tree->pawn_score.outside|=2;
          break;
        }
      }
      if (b_file_r > 3) {
        if (And(tree->all_pawns,left_side_mask[b_file_r]) &&
            !And(WhitePawns,right_side_empty_mask[b_file_r])) {
          if (!tree->pawn_score.white_protected ||
              tree->pawn_score.black_protected) tree->pawn_score.outside|=2;
        }
      }
    } while(0);
  }
  if (!tree->pawn_score.outside) {
    if (tree->pawn_score.white_protected && !tree->pawn_score.black_protected)
      tree->pawn_score.outside|=1;
    if (tree->pawn_score.black_protected && !tree->pawn_score.white_protected)
      tree->pawn_score.outside|=2;
  }

/*
 ----------------------------------------------------------
|                                                          |
|   store the results in the pawn hash table for reuse at  |
|   a later time as needed.                                |
|                                                          |
 ----------------------------------------------------------
*/
  Lock(lock_pawn_hash);
  *ptable=tree->pawn_score;
  UnLock(lock_pawn_hash);
  return(tree->pawn_score.p_score);
}
