#include <stdio.h>
#include <stdlib.h>
#include "chess.h"
#include "evaluate.h"
#include "data.h"

/* last modified 11/13/96 */
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

BITBOARD all_pawns;
int white_pof, white_defects_k, white_defects_q;
int black_pof, black_defects_k, black_defects_q;
int passed_w, passed_b, weak_w, weak_b, p_score;
unsigned int last_hash;

int Evaluate(int ply, int wtm, int alpha, int beta)
{
  register BITBOARD temp;
  register int square, file, score;
  register int w_safety, b_safety;
  register int bishops_opposite_color=0, drawn_ending=0;
  register int white_target, black_target;

/*
 ----------------------------------------------------------
|                                                          |
|   check for draws due to insufficient material and       |
|   adjust the score as necessary.                         |
|                                                          |
 ----------------------------------------------------------
*/
  if (TotalWhitePieces < 5 && TotalBlackPieces < 5) drawn_ending=EvaluateDraws();
  if (drawn_ending == 0) {
    if (((wtm) ? Material : -Material)-
        largest_positional_score >= beta) return(beta);
    if (((wtm) ? Material : -Material)+
        largest_positional_score <= alpha) return(alpha);
  }
  else if (drawn_ending > 0) return((root_wtm) ? DrawScore() : -DrawScore());

  evaluations++;
  score=Material;
#ifdef DEBUGEV
  printf("score[material]=                  %d\n",score);
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
  if ((TotalWhitePawns+TotalBlackPawns) == 0) {
    score+=EvaluateMate();
#ifdef DEBUGEV
    printf("score[mater]=                     %d\n",score);
#endif
    passed_w=0;
    passed_b=0;
    weak_w=0;
    weak_b=0;
    white_pof=0;
    black_pof=0;
  }
  else
    if (((unsigned int) PawnHashKey) == last_hash) {
#if !defined(FAST)
      pawn_hashes++;
#endif
      score+=p_score;
    }
    else score+=EvaluatePawns();
#ifdef DEBUGEV
  printf("score[pawns]=                     %d\n",score);
#endif
/*
**********************************************************************
*                                                                    *
*   if there are any passed pawns, first call EvaluatePassedPawns()  *
*   to evaluate them.  then, if one side has a passed pawn and the   *
*   other side has no pieces, call EvaluatePassedPawnRaces() to see  *
*   if the passed pawn can be stopped from promoting.  finally, we   *
*   use EvaluateOutsidePassedPawns() to see if one side has a passed *
*   pawn that represents a nearly won endgame advantage.             *
*                                                                    *
**********************************************************************
*/
  all_pawns=Or(BlackPawns,WhitePawns);
  if (passed_b || passed_w) {
    score+=EvaluatePassedPawns();
    if ((TotalWhitePieces==0 && passed_b) ||
        (TotalBlackPieces==0 && passed_w))
      score+=EvaluatePassedPawnRaces(wtm);
    score+=EvaluateOutsidePassedPawns();
#ifdef DEBUGEV
    printf("score[passed pawns]=              %d\n",score);
#endif
  }
/*
**********************************************************************
*                                                                    *
*   call EvaluateDevelopment() to evaluate development.  if the      *
*   flag "opening" is zero, skip the call.                           *
*                                                                    *
**********************************************************************
*/
  if (opening) score+=EvaluateDevelopment(ply);
#ifdef DEBUGEV
  printf("score[development]=               %d\n",score);
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
  white_target=BlackKingSQ;
  black_target=WhiteKingSQ;
  if (TotalWhitePieces > 9 && TotalBlackPieces > 9) {
    w_safety=king_defects_w[WhiteKingSQ];
    if (File(WhiteKingSQ) >= FILEE) {
      w_safety+=white_defects_k;
      if (!And(WhitePawns,set_mask[G2])) {
        if (And(WhitePawns,set_mask[G3]) && Distance(WhiteKingSQ,G2)==1 &&
            And(WhiteBishops,good_bishop_kw))
          w_safety-=KING_SAFETY_GOOD_BISHOP;
        if ((And(mask_F3H3,BlackPawns) || And(mask_F3H3,BlackBishops)) &&
            BlackQueens) w_safety+=KING_SAFETY_MATE_G2G7;
      }
    }
    else if (File(WhiteKingSQ) <= FILED) {
      w_safety+=white_defects_q;
      if (!And(WhitePawns,set_mask[B2])) {
        if (And(WhitePawns,set_mask[B3]) && Distance(WhiteKingSQ,B2)==1 &&
            And(WhiteBishops,good_bishop_qw))
          w_safety-=KING_SAFETY_GOOD_BISHOP;
        if ((And(mask_A3C3,BlackPawns) || And(mask_A3C3,BlackBishops)) &&
            BlackQueens) w_safety+=KING_SAFETY_MATE_G2G7;
      }
    }
    if ((File(WhiteKingSQ) == FILED) || (File(WhiteKingSQ) == FILEE)) {
      for (file=File(WhiteKingSQ)-1;file<File(WhiteKingSQ)+2;file++)
        if (!And(all_pawns,file_mask[file])) w_safety+=KING_SAFETY_OPEN_FILE;
    }

    b_safety=king_defects_b[BlackKingSQ];
    if (File(BlackKingSQ) >= FILEE) {
      b_safety+=black_defects_k;
      if (!And(BlackPawns,set_mask[G7])) {
        if (And(BlackPawns,set_mask[G6]) && Distance(BlackKingSQ,G7)==1 &&
            And(BlackBishops,good_bishop_kb))
          b_safety-=KING_SAFETY_GOOD_BISHOP;
        if ((And(mask_F6H6,WhitePawns) || And(mask_F6H6,WhiteBishops)) &&
            WhiteQueens) b_safety+=KING_SAFETY_MATE_G2G7;
      }
    }
    else if (File(BlackKingSQ) <= FILED) {
      b_safety+=black_defects_q;
      if (!And(BlackPawns,set_mask[B7])) {
        if (And(BlackPawns,set_mask[B6]) && Distance(BlackKingSQ,B7)==1 &&
            And(BlackBishops,good_bishop_qb))
          b_safety-=KING_SAFETY_GOOD_BISHOP;
        if ((And(mask_A6C6,WhitePawns) || And(mask_A6C6,WhiteBishops)) &&
            WhiteQueens) b_safety+=KING_SAFETY_MATE_G2G7;
      }
    }
    if ((File(BlackKingSQ) == FILED) || (File(BlackKingSQ) == FILEE)) {
      for (file=File(BlackKingSQ)-1;file<File(BlackKingSQ)+2;file++)
        if (!And(all_pawns,file_mask[file])) b_safety+=KING_SAFETY_OPEN_FILE;
    }
    if (w_safety > b_safety+10) {
      white_target=WhiteKingSQ;
      black_target=WhiteKingSQ;
    }
    else if (b_safety > w_safety+10) {
      white_target=BlackKingSQ;
      black_target=BlackKingSQ;
    }
    if (root_wtm)
      score-=king_safety_c[TotalBlackPieces]*w_safety-
             king_safety_o[TotalWhitePieces]*b_safety;
    else
      score-=king_safety_o[TotalBlackPieces]*w_safety-
             king_safety_c[TotalWhitePieces]*b_safety;
  }
  else {
    w_safety=0;
    b_safety=0;
    if ((TotalWhitePieces==bishop_v) && (TotalBlackPieces==bishop_v) &&
        BlackBishops && WhiteBishops) {
      if (square_color[FirstOne(BlackBishops)] != 
          square_color[FirstOne(WhiteBishops)])
        bishops_opposite_color=1;
    }
  }
#ifdef DEBUGEV
  printf("score[king safety]=               %d\n",score);
#endif
/*
**********************************************************************
*                                                                    *
*   if the score is roughly +/- two pawns (2000) outside the alpha/  *
*   beta window, then there's little chance that the remainder of    *
*   the evaluation code can bring it back into the window, since the *
*   "big" evaluator modules have had their chance.  this saves a lot *
*   of computation that is worthless.                                *
*                                                                    *
**********************************************************************
*/
  if (!drawn_ending) {
    if (((wtm) ? score : -score) < alpha-2000 || 
        ((wtm) ? score : -score) >  beta+2000) return((wtm) ? score : -score);
  }
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
  if (TotalBlackPieces < 10) score+=king_value_w[WhiteKingSQ];
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
    if (WhiteKingSQ > FILEE) {
      if (And(WhiteRooks,mask_kr_trapped_w[FILEH-WhiteKingSQ])) 
        score-=ROOK_TRAPPED;
    }
    else if (WhiteKingSQ < FILED) {
      if (And(WhiteRooks,mask_qr_trapped_w[WhiteKingSQ]))
        score-=ROOK_TRAPPED;
    }
  }
#ifdef DEBUGEV
  printf("score[kings(white)]=              %d\n",score);
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
  if (TotalWhitePieces < 10) score-=king_value_b[BlackKingSQ];
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
    if (File(BlackKingSQ) > FILEE) {
      if (And(BlackRooks,mask_kr_trapped_b[FILEH-File(BlackKingSQ)]))
        score+=ROOK_TRAPPED;
    }
    else if (File(BlackKingSQ) < FILED) {
      if (And(BlackRooks,mask_qr_trapped_b[File(BlackKingSQ)]))
        score+=ROOK_TRAPPED;
    }
  }
#ifdef DEBUGEV
  printf("score[kings(black)]=              %d\n",score);
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
|   supported by a friendly pawn.  if the opponent has no  |
|   knight or bishop of the same color, the knight is      |
|   impossible to get rid of and is worth even more.       |
|                                                          |
 ----------------------------------------------------------
*/
  temp=WhiteKnights;
  while(temp) {
    square=FirstOne(temp);
    if (white_outpost[square] &&
        !And(mask_no_pawn_attacks_b[square],BlackPawns)) {
      score+=KNIGHT_OUTPOST*white_outpost[square]/2;
      if(And(b_pawn_attacks[square],WhitePawns))
        score+=KNIGHT_OUTPOST*white_outpost[square]/2;
      if (!BlackKnights && (!BlackBishops ||
                            ((And(set_mask[square],light_squares) &&
                              And(BlackBishops,dark_squares)) ||
                             (And(set_mask[square],dark_squares) &&
                              And(BlackBishops,light_squares)))))
        score+=KNIGHT_OUTPOST*white_outpost[square];
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
    score+=(4-Distance(square,white_target))*KNIGHT_KING_TROPISM;
/*
 ----------------------------------------------------------
|                                                          |
|   now check for a knight on a corner square.  if so,     |
|   check to see if either of the two possible flight      |
|   squares are "safe".  if not, the knight is trapped and |
|   potentially lost.                                      |
|                                                          |
 ----------------------------------------------------------
*/
    if (is_corner[square]) {
      if ((Swap(square,flight_sq[square][0],1) < 0) &&
          (Swap(square,flight_sq[square][1],1) < 0))
      score-=KNIGHT_TRAPPED;
    }
    Clear(square,temp);
  }
#ifdef DEBUGEV
  printf("score[knights(white)]=            %d\n",score);
#endif

/*
 ----------------------------------------------------------
|                                                          |
|   black knights.                                         |
|                                                          |
|   first, evaluate for "outposts" which is a knight that  |
|   can't be driven off by an enemy pawn, and which is     |
|   supported by a friendly pawn.  if the opponent has no  |
|   knight or bishop of the same color, the knight is      |
|   impossible to get rid of and is worth even more.       |
|                                                          |
 ----------------------------------------------------------
*/
  temp=BlackKnights;
  while(temp) {
    square=FirstOne(temp);
    if (black_outpost[square] &&
        !And(mask_no_pawn_attacks_w[square],WhitePawns)) {
      score-=KNIGHT_OUTPOST*black_outpost[square]/2;
      if (And(w_pawn_attacks[square],BlackPawns))
        score-=KNIGHT_OUTPOST*black_outpost[square]/2;
      if (!WhiteKnights && (!WhiteBishops ||
                            ((And(set_mask[square],light_squares) &&
                              And(WhiteBishops,dark_squares)) ||
                             (And(set_mask[square],dark_squares) &&
                              And(WhiteBishops,light_squares)))))
        score-=KNIGHT_OUTPOST*black_outpost[square];
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
    score-=(4-Distance(square,black_target))*KNIGHT_KING_TROPISM;
/*
 ----------------------------------------------------------
|                                                          |
|   now check for a knight on a corner square.  if so,     |
|   check to see if either of the two possible flight      |
|   squares are "safe".  if not, the knight is trapped and |
|   potentially lost.                                      |
|                                                          |
 ----------------------------------------------------------
*/
    if (is_corner[square]) {
      if ((Swap(square,flight_sq[square][0],0) < 0) &&
          (Swap(square,flight_sq[square][1],0) < 0))
      score+=KNIGHT_TRAPPED;
    }
    Clear(square,temp);
  }
#ifdef DEBUGEV
  printf("score[knights(black)]=            %d\n",score);
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
|   first, locate each bishop and score its mobility by    |
|   multiplying the number of squares it attacks by its    |
|   mobility scoring bonus.                                |
|                                                          |
 ----------------------------------------------------------
*/
  temp=WhiteBishops;
  while(temp) {
    square=FirstOne(temp);
    score+=BISHOP_MOBILITY*MobilityBishop(square);
/*
 ----------------------------------------------------------
|                                                          |
|   then fold in the "centralization" value from the       |
|   static piece/square table for bishops.                 |
|                                                          |
 ----------------------------------------------------------
*/
    score+=bishop_value_w[square];
/*
 ----------------------------------------------------------
|                                                          |
|   add in a bonus for occupying a rank or file close to   |
|   the king.                                              |
|                                                          |
 ----------------------------------------------------------
*/
    score+=(4-Distance(square,white_target))*BISHOP_KING_TROPISM;
/*
 ----------------------------------------------------------
|                                                          |
|   check to see if the bishop is trapped at a7 or h7 with |
|   a pawn at b6 or g6 that has trapped the bishop, and    |
|   that this trapping pawn is defended at least one time. |
|                                                          |
 ----------------------------------------------------------
*/
    if (square == A7) {
      if (And(mask_B6B7,BlackPawns) && 
          Attacked(41,0)) score-=BISHOP_TRAPPED;
    }
    else if (square == H7) {
      if (And(mask_G6G7,BlackPawns) && 
          Attacked(46,0)) score-=BISHOP_TRAPPED;
    }
    Clear(square,temp);
  }
/*
 ----------------------------------------------------------
|                                                          |
|   check to see if one side has a rook+bishop and the     |
|   other has a rook+knight.  if so, the rook+bishop is    |
|   much stronger.                                         |
|                                                          |
 ----------------------------------------------------------
*/
  if (TotalWhitePieces==8 && TotalBlackPieces == 7)
    score+=ROOK_BISHOP_VS_ROOK_KNIGHT;
  else if (TotalWhitePieces==7 && TotalBlackPieces == 8)
    score-=ROOK_BISHOP_VS_ROOK_KNIGHT;
#ifdef DEBUGEV
  printf("score[bishops(white)]=            %d\n",score);
#endif
/*
 ----------------------------------------------------------
|                                                          |
|   black bishops                                          |
|                                                          |
|   first, locate each bishop and score its mobility by    |
|   multiplying the number of squares it attacks by its    |
|   mobility scoring bonus.                                |
|                                                          |
 ----------------------------------------------------------
*/
  temp=BlackBishops;
  while(temp) {
    square=FirstOne(temp);
    score-=BISHOP_MOBILITY*MobilityBishop(square);
/*
 ----------------------------------------------------------
|                                                          |
|   then fold in the "centralization" value from the       |
|   static piece/square table for bishops.                 |
|                                                          |
 ----------------------------------------------------------
*/
    score-=bishop_value_b[square];
/*
 ----------------------------------------------------------
|                                                          |
|   add in a bonus for occupying a rank or file close to   |
|   the king.                                              |
|                                                          |
 ----------------------------------------------------------
*/
    score-=(4-Distance(square,black_target))*BISHOP_KING_TROPISM;
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
    if (square == A2) {
      if (And(mask_B2B3,WhitePawns) && 
          Attacked(17,1)) score+=BISHOP_TRAPPED;
    }
    else if (square == H2) {
      if (And(mask_G2G3,WhitePawns) && 
          Attacked(22,1)) score+=BISHOP_TRAPPED;
    }
    Clear(square,temp);
  }
#ifdef DEBUGEV
  printf("score[bishops(black)]=            %d\n",score);
#endif
/*
 ----------------------------------------------------------
|                                                          |
|   now, give either side a bonus for having two bishops.  |
|                                                          |
 ----------------------------------------------------------
*/
  if (And(WhiteBishops,WhiteBishops-1)) score+=BISHOP_PAIR;
  if (And(BlackBishops,BlackBishops-1)) score-=BISHOP_PAIR;
#ifdef DEBUGEV
  printf("score[bishop pair]=               %d\n",score);
#endif
/*
**********************************************************************
*                                                                    *
*   rook evaluation includes several simple cases, including open    *
*   files, 7th rank, connected, etc.                                 *
*                                                                    *
**********************************************************************
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
 ----------------------------------------------------------
*/
    if (!And(file_mask[file],all_pawns) ||
        (!And(file_mask[file],WhitePawns) && (weak_b & (128>>file)))) {
      score+=ROOK_OPEN_FILE;
      if (And(AttacksFile(square),WhiteRooks))
        score+=ROOK_CONNECTED_OPEN_FILE;
    }
/*
 ----------------------------------------------------------
|                                                          |
|   see if the rook is behind a passed pawn.  if it is,    |
|   it is counted as though the file is open.              |
|                                                          |
 ----------------------------------------------------------
*/
    if (128>>file & passed_w) {
      register int pawnsq=LastOne(And(WhitePawns,file_mask[file]));
      if (square<pawnsq && !And(BlackPieces,set_mask[pawnsq+8]))
        score+=ROOK_BEHIND_PASSED_PAWN;
      if (And(AttacksFile(square),WhiteRooks))
        score-=ROOK_BEHIND_PASSED_PAWN/2;
    }
    if (128>>file & passed_b) {
      register int pawnsq=FirstOne(And(BlackPawns,file_mask[file]));
      if (square > pawnsq) score+=ROOK_BEHIND_PASSED_PAWN;
    }
/*
 ----------------------------------------------------------
|                                                          |
|   if the opponent has weak pawns on open files, the rook |
|   is worth more since it can attack these pawns easily.  |
|                                                          |
 ----------------------------------------------------------
*/
    score+=black_pof*ROOK_ATTACK_WEAK_PAWNS;
/*
 ----------------------------------------------------------
|                                                          |
|   add in a bonus for occupying a rank or file close to   |
|   the king.                                              |
|                                                          |
 ----------------------------------------------------------
*/
    score+=(4-Distance(square,white_target))*ROOK_KING_TROPISM;
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
    if (Rank(square) == RANK7 && (And(BlackPawns,rank_mask[RANK7]) ||
                                  BlackKingSQ > H7)) {
      score+=ROOK_ON_7TH;
      if (And(b_pawn_attacks[square],WhitePawns)) score+=ROOK_ON_7TH_WITH_PAWN;
      if (TotalBlackPieces < 10) {
        if (BlackKingSQ > H7) score+=ROOK_ON_7TH;
        if (passed_w && !And(BlackPieces,mask_abs7_w)) score+=ROOK_ABSOLUTE_7TH;
      }
      if (And(AttacksRank(square),Or(WhiteRooks,WhiteQueens)))
        score+=ROOK_CONNECTED_7TH_RANK;
    }
    Clear(square,temp);
  }
#ifdef DEBUGEV
  printf("score[rooks(white)]=              %d\n",score);
#endif

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
 ----------------------------------------------------------
*/
    if (!And(file_mask[file],all_pawns) ||
        (!And(file_mask[file],BlackPawns) && (weak_w & (128>>file)))) {
      score-=ROOK_OPEN_FILE;
      if (And(AttacksFile(square),BlackRooks))
        score-=ROOK_CONNECTED_OPEN_FILE;
    }
/*
 ----------------------------------------------------------
|                                                          |
|   if not, see if the rook is behind a passed pawn.  if   |
|   it is, it is counted as though the file is open.       |
|                                                          |
 ----------------------------------------------------------
*/
    if (128>>file & passed_b) {
      register int pawnsq=FirstOne(And(BlackPawns,file_mask[file]));
      if (square>pawnsq && !And(WhitePieces,set_mask[pawnsq-8]))
        score-=ROOK_BEHIND_PASSED_PAWN;
      if (And(AttacksFile(square),BlackRooks))
        score+=ROOK_BEHIND_PASSED_PAWN/2;
    }
    if (128>>file & passed_w) {
      register int pawnsq=LastOne(And(WhitePawns,file_mask[file]));
      if (square < pawnsq) score-=ROOK_BEHIND_PASSED_PAWN;
    }
/*
 ----------------------------------------------------------
|                                                          |
|   if the opponent has weak pawns on open files, the rook |
|   is worth more since it can attack these pawns easily.  |
|                                                          |
 ----------------------------------------------------------
*/
    score-=white_pof*ROOK_ATTACK_WEAK_PAWNS;
/*
 ----------------------------------------------------------
|                                                          |
|   add in a bonus for occupying a rank or file close to   |
|   the king.                                              |
|                                                          |
 ----------------------------------------------------------
*/
    score-=(4-Distance(square,black_target))*ROOK_KING_TROPISM;
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
    if (Rank(square) == RANK2 && (And(WhitePawns,rank_mask[RANK2]) ||
                                  WhiteKingSQ < A2)) {
      score-=ROOK_ON_7TH;
      if (And(w_pawn_attacks[square],BlackPawns)) score-=ROOK_ON_7TH_WITH_PAWN;
      if (TotalWhitePieces < 10) {
        if (WhiteKingSQ < A2) score-=ROOK_ON_7TH;
        if (passed_b && !And(WhitePieces,mask_abs7_b)) score-=ROOK_ABSOLUTE_7TH;
      }
      if (And(AttacksRank(square),Or(BlackRooks,BlackQueens)))
        score-=ROOK_CONNECTED_7TH_RANK;
    }
    Clear(square,temp);
  }
#ifdef DEBUGEV
  printf("score[rooks(black)]=              %d\n",score);
#endif
/*
**********************************************************************
*                                                                    *
*   queen evaluation only includes centralization, plus a bonus if   *
*   the opponent's king-side is shredded, making the queen a more    *
*   dangerous piece.                                                 *
*                                                                    *
**********************************************************************
*/
/*
 ----------------------------------------------------------
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
    if (w_safety+ACCEPTABLE_FAULTS < b_safety) score+=QUEEN_IS_STRONG;
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
      if (And(AttacksRank(square),WhiteRooks))
        score+=QUEEN_ROOK_ON_7TH_RANK;
    }
/*
 ----------------------------------------------------------
|                                                          |
|   add in a bonus for occupying a rank or file close to   |
|   the king.                                              |
|                                                          |
 ----------------------------------------------------------
*/
    score+=(12-RankDistance(square,white_target)
             -2*FileDistance(square,white_target))*QUEEN_KING_TROPISM;
    Clear(square,temp);
  }
/*
 ----------------------------------------------------------
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
    if (b_safety+ACCEPTABLE_FAULTS < w_safety) score-=QUEEN_IS_STRONG;
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
      if (And(AttacksRank(square),BlackRooks))
        score-=QUEEN_ROOK_ON_7TH_RANK;
    }
/*
 ----------------------------------------------------------
|                                                          |
|   add in a bonus for occupying a rank or file close to   |
|   the king.                                              |
|                                                          |
 ----------------------------------------------------------
*/
    score-=(12-RankDistance(square,black_target)
             -2*FileDistance(square,black_target))*QUEEN_KING_TROPISM;
    Clear(square,temp);
  }
#ifdef DEBUGEV
  printf("score[queens]=                    %d\n",score);
#endif
  if (abs(score-Material) > largest_positional_score)
    largest_positional_score=abs(score-Material);
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
  if (bishops_opposite_color) score=score>>1;
  if (drawn_ending < 0) {
    if (drawn_ending == -1 && score > 0) score=DrawScore();
    else if (drawn_ending == -2 && score < 0) score=DrawScore();
  }
#ifdef DEBUGEV
  printf("score[draws]=                     %d\n",score);
#endif
  return((wtm) ? score : -score);
}

/* last modified 05/16/96 */
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
int EvaluateDevelopment(int ply)
{
  register int possible, real, w_score, b_score;

  w_score=0;
  b_score=0;
/*
 ----------------------------------------------------------
|                                                          |
|   first, some "thematic" things, which includes don't    |
|   block the c-pawn in queen-pawn openings.               |
|                                                          |
 ----------------------------------------------------------
*/
  if (!And(set_mask[E4],WhitePawns)) {
    if (And(set_mask[D4],WhitePawns)) {
      if (And(set_mask[C2],WhitePawns) &&
          And(set_mask[C3],Or(WhiteKnights,WhiteBishops)))
      w_score-=DEVELOPMENT_THEMATIC;
      if (And(set_mask[C7],BlackPawns) &&
          And(set_mask[C6],Or(BlackKnights,BlackBishops)))
      b_score-=DEVELOPMENT_THEMATIC;
    }
  }
#ifdef DEBUGDV
  printf("development.1 w_score=%d  b_score=%d\n",w_score, b_score);
#endif
/*
 ----------------------------------------------------------
|                                                          |
|   if all minor pieces aren't developed, then penalize    |
|   the score to get 'em out.                              |
|                                                          |
 ----------------------------------------------------------
*/
    w_score-=DEVELOPMENT_UNMOVED_PIECES*
      PopCnt(And(Or(WhiteKnights,WhiteBishops),white_minor_pieces));
    b_score-=DEVELOPMENT_UNMOVED_PIECES*
      PopCnt(And(Or(BlackKnights,BlackBishops),black_minor_pieces));
#ifdef DEBUGDV
  printf("development.2 w_score=%d  b_score=%d\n",w_score, b_score);
#endif
/*
 ----------------------------------------------------------
|                                                          |
|   if all minor pieces aren't developed, then penalize    |
|   the queen if it has moved.                             |
|                                                          |
 ----------------------------------------------------------
*/
  if ((PopCnt(And(Or(WhiteKnights,WhiteBishops),
       white_minor_pieces)) > 1) && !And(WhiteQueens,set_mask[D1]))
    w_score-=DEVELOPMENT_QUEEN_EARLY;
  if ((PopCnt(And(Or(BlackKnights,BlackBishops),
       black_minor_pieces)) > 1) && !And(BlackQueens,set_mask[D8]))
    b_score-=DEVELOPMENT_QUEEN_EARLY;
#ifdef DEBUGDV
  printf("development.3 w_score=%d  b_score=%d\n",w_score, b_score);
#endif
/*
 ----------------------------------------------------------
|                                                          |
|   check to see if center pawns are blocked.              |
|                                                          |
 ----------------------------------------------------------
*/
  if (And(Occupied,And(Shiftr(And(WhitePawns,rank_mask[RANK2]),8),
                            Or(file_mask[FILED],file_mask[FILEE]))))
    w_score-=DEVELOPMENT_BLOCKED_PAWN;
  if (And(Occupied,And(Shiftl(And(BlackPawns,rank_mask[RANK7]),8),
                            Or(file_mask[FILED],file_mask[FILEE]))))
    b_score-=DEVELOPMENT_BLOCKED_PAWN;
#ifdef DEBUGDV
  printf("development.4 w_score=%d  b_score=%d\n",w_score, b_score);
#endif
/*
 ----------------------------------------------------------
|                                                          |
|   check to see if center pawns have moved.               |
|                                                          |
 ----------------------------------------------------------
*/
  w_score-=PopCnt(And(WhitePawns,
                      white_center_pawns))*DEVELOPMENT_UNMOVED_PAWNS;
  b_score-=PopCnt(And(BlackPawns,
                      black_center_pawns))*DEVELOPMENT_UNMOVED_PAWNS;
#ifdef DEBUGDV
  printf("development.5 w_score=%d  b_score=%d\n",w_score, b_score);
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
  if (WhiteCastle(1) > 0) {
    possible=0;
    real=0;
    if (WhiteCastle(ply) == 0)
        w_score-=(root_wtm) ? 2*DEVELOPMENT_LOSING_CASTLE :
                              DEVELOPMENT_LOSING_CASTLE;
    if (WhiteCastle(ply) < 0) {
      if (File(WhiteKingSQ) > FILEE) {
        real=-(15+(TotalBlackPieces>>1))*white_defects_k;
        possible=-(15+(TotalBlackPieces>>1))*white_defects_q;
      }
      else if (File(WhiteKingSQ) < FILED) {
        real=-(15+(TotalBlackPieces>>1))*white_defects_q;
        possible=-(15+(TotalBlackPieces>>1))*white_defects_k;
      }
    }
    if (WhiteCastle(1)==3 && WhiteCastle(ply)>0 && WhiteCastle(ply)!=3) {
      if (WhiteCastle(ply)&1) {
        real=-(15+(TotalBlackPieces>>1))*white_defects_k;
        possible=-(15+(TotalBlackPieces>>1))*white_defects_q;
      }
      else if (WhiteCastle(ply)&2) {
        real=-(15+(TotalBlackPieces>>1))*white_defects_q;
        possible=-(15+(TotalBlackPieces>>1))*white_defects_k;
      }
    }
    if (possible > real) 
      w_score-=(root_wtm) ? 3*(possible-real) : 2*(possible-real);
    if (WhiteCastle(ply) > 0) w_score-=DEVELOPMENT_NOT_CASTLED;
  }
  if (BlackCastle(1) > 0) {
    possible=0;
    real=0;
    if (BlackCastle(ply) == 0)
      b_score-=(!root_wtm) ? 2*DEVELOPMENT_LOSING_CASTLE :
                             DEVELOPMENT_LOSING_CASTLE;
    if (BlackCastle(ply) < 0) {
      if (File(BlackKingSQ) > FILEE) {
        real=-(15+(TotalWhitePieces>>1))*black_defects_k;
        possible=-(15+(TotalWhitePieces>>1))*black_defects_q;
      }
      else {
        real=-(15+(TotalWhitePieces>>1))*black_defects_q;
        possible=-(15+(TotalWhitePieces>>1))*black_defects_k;
      }
    }
    if (BlackCastle(1)==3 && BlackCastle(ply)>0 && BlackCastle(ply)!=3) {
      if (BlackCastle(ply)&1) {
        real=-(15+(TotalWhitePieces>>1))*black_defects_k;
        possible=-(15+(TotalWhitePieces>>1))*black_defects_q;
      }
      else if (BlackCastle(ply)&2) {
        real=-(15+(TotalWhitePieces>>1))*black_defects_q;
        possible=-(15+(TotalWhitePieces>>1))*black_defects_k;
      }
    }
    if (possible > real) 
      b_score-=(!root_wtm) ? 3*(possible-real) : 2*(possible-real);
    if (BlackCastle(ply) > 0) b_score-=DEVELOPMENT_NOT_CASTLED;
  }
#ifdef DEBUGDV
  printf("development.6 w_score=%d  b_score=%d\n",w_score, b_score);
#endif
/*
 ----------------------------------------------------------
|                                                          |
|  done.                                                   |
|                                                          |
 ----------------------------------------------------------
*/

  return(w_score-b_score);
}

/* last modified 09/10/96 */
/*
********************************************************************************
*                                                                              *
*   EvaluateDraws() is used to catch positions where one side is ahead in      *
*   material, but has insufficient material to force mate.                     *
*                                                                              *
*   the rules are simple:  (1) if both sides have pawns, it is not yet a draw; *
*   (2) if one side has no pawns, then the score cannot be in favor of that    *
*   side, it is taken to DRAW;  (3) if neither side has pawns, and neither     *
*   side has at least a bishop and knight, then the score is taken to DRAW.    *
*                                                                              *
********************************************************************************
*/
int EvaluateDraws()
{
  register int square;
  register BITBOARD pawns;
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
|   better be the right color, otherwise its a DRAW.  of   |
|   course we have to check for an unstoppable pawn to be  |
|   safe.                                                  |
|                                                          |
 ----------------------------------------------------------
*/
  if ((TotalWhitePieces == 3) && (TotalBlackPieces == 0) &&
      TotalWhitePawns && !And(WhitePawns,not_rook_pawns)) {
    if (And(WhiteBishops,dark_squares)) {
      if (And(file_mask[FILEH],WhitePawns)) return(0);
    }
    else if (And(file_mask[FILEA],WhitePawns)) return(0);

    pawns=WhitePawns;
    while (pawns) {
      square=FirstOne(pawns);
      if (( wtm && !And(white_pawn_race_wtm[square],BlackKing)) ||
          (!wtm && !And(white_pawn_race_btm[square],BlackKing))) return(0);
      Clear(square,pawns);
    }
    return(1);
  }
/*
 ----------------------------------------------------------
|                                                          |
|   if black has a bishop and pawn(s) then the pawn had    |
|   better not be only rook pawns, or else the bishop had  |
|   better be the right color, otherwise its a DRAW.       |
|                                                          |
 ----------------------------------------------------------
*/
  if ((TotalBlackPieces == 3) && (TotalWhitePieces == 0) &&
      TotalBlackPawns && !And(BlackPawns,not_rook_pawns)) {
    if (And(BlackBishops,dark_squares)) {
      if (And(file_mask[FILEA],BlackPawns)) return(0);
    }
    else if (And(file_mask[FILEH],BlackPawns)) return(0);

    pawns=BlackPawns;
    while (pawns) {
      square=FirstOne(pawns);
      if (( wtm && !And(black_pawn_race_wtm[square],WhiteKing)) ||
          (!wtm && !And(black_pawn_race_btm[square],WhiteKing))) return(0);
      Clear(square,pawns);
    }
    return(1);
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
  if (TotalWhitePawns == 0 && TotalWhitePieces < 5) return(-1);
  else if (TotalBlackPawns == 0 && TotalBlackPieces < 5) return(-2);
  return(0);
}

/* last modified 05/16/96 */
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
int EvaluateMate(void)
{
  register int mate_score;
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
  mate_score=0;
  if ((TotalBlackPieces==0) && (TotalWhitePieces==6) &&
      (!WhitePawns) && (!BlackPawns) && (PopCnt(WhiteBishops)==1)) {
    if (And(dark_squares,WhiteBishops))
      mate_score=b_n_mate_dark_squares[BlackKingSQ];
    else
      mate_score=b_n_mate_light_squares[BlackKingSQ];
  }
  if ((TotalBlackPieces==6) && (TotalWhitePieces==0) &&
      (!WhitePawns) && (!BlackPawns) && (PopCnt(BlackBishops)==1)) {
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
|   of the                                           |
|                                                          |
 ----------------------------------------------------------
*/
    if (TotalWhitePieces >= TotalBlackPieces+3) {
      mate_score=mate[BlackKingSQ];
    }
/*
 ----------------------------------------------------------
|                                                          |
|   if black is winning, force the white king to the edge  |
|   of the                                           |
|                                                          |
 ----------------------------------------------------------
*/
    if (TotalBlackPieces >= TotalWhitePieces+3) {
      mate_score=-mate[WhiteKingSQ];
    }
  }
  return(mate_score);
}

/* last modified 05/16/96 */
/*
********************************************************************************
*                                                                              *
*   EvaluateOutsidePassedPawns() is used to evaluate outside passed pawns.     *
*   it analyzes the passed pawns for both sides, and returns a bonus if one    *
*   side has a passed pawn on the opposite side of the board from the rest of  *
*   the pawns.  the side with the most remote (outside) pawn gets a bonus that *
*   dynamically varies proportionally to the number of pieces on the board so  *
*   that it encourages the side with the outside passer to trade down to a     *
*   simple king and pawn ending which the outside passer wins.                 *
*                                                                              *
********************************************************************************
*/
int EvaluateOutsidePassedPawns(void)
{
  register int score, w_file_l, w_file_r, b_file_l, b_file_r;

  score=0;
  w_file_l=first_ones_8bit[passed_w];
  if (w_file_l == 8) w_file_l=9;
  b_file_l=first_ones_8bit[passed_b];
  if (b_file_l == 8) b_file_l=9;
  w_file_r=last_ones_8bit[passed_w];
  if (w_file_r == 8) w_file_r=-9;
  b_file_r=last_ones_8bit[passed_b];
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
    if ((w_file_l < (b_file_l-1)) || (w_file_r > (b_file_r+1))) do {
      if (w_file_l < 4) {
        if(And(all_pawns,right_side_mask[w_file_l]) &&
           !And(BlackPawns,left_side_empty_mask[w_file_l])) {
          score+=outside_passed[(int) TotalBlackPieces];
          break;
        }
      }
      if (w_file_r > 3) {
        if (And(all_pawns,left_side_mask[w_file_r]) &&
            !And(BlackPawns,right_side_empty_mask[w_file_r]))
          score+=outside_passed[(int) TotalBlackPieces];
      }
    } while(0);
  }
  if (b_file_l != 9) {
    if ((b_file_l < (w_file_l-1)) || (b_file_r > (w_file_r+1))) do {
      if (b_file_l < 4) {
        if(And(all_pawns,right_side_mask[b_file_l]) &&
           !And(WhitePawns,left_side_empty_mask[b_file_l])) {
          score-=outside_passed[(int) TotalWhitePieces];
          break;
        }
      }
      if (b_file_r > 3) {
        if (And(all_pawns,left_side_mask[b_file_r]) &&
            !And(WhitePawns,right_side_empty_mask[b_file_r]))
          score-=outside_passed[(int) TotalWhitePieces];
      }
    } while(0);
  }
  return(score);
}

/* last modified 05/16/96 */
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
int EvaluatePassedPawns(void)
{
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
|   is less valuable since it can't advance easily.        |
|                                                          |
 ----------------------------------------------------------
*/
  if (passed_b) {
    black_king_sq=BlackKingSQ;
    pawns=passed_b;
    while (pawns) {
      file=first_ones_8bit[pawns];
      pawns&=~(128>>file);
      square=FirstOne(And(BlackPawns,file_mask[file]));
      if (Distance(square,black_king_sq) < 2)
        score-=supported_passer[7-Rank(square)];
      if (And(set_mask[square-8],WhitePieces))
        score+=(PAWN_PASSED*(RANK8-Rank(square)))>>1;
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
    pawns=passed_b;
    while ((file=connected_passed[pawns])) {
      register int square1,square2;
      pawns&=~(128>>file);
      square1=FirstOne(And(BlackPawns,file_mask[file-1]));
      if (Rank(square1) > RANK3) continue;
      square2=FirstOne(And(BlackPawns,file_mask[file]));
      if (Rank(square2) > RANK3) continue;
      score-=PAWN_CONNECTED_PASSED_SIXTH;
      if (TotalWhitePieces < queen_v) {
        score-=PAWN_CONNECTED_PASSED_SIXTH;
        if ((TotalWhitePieces <= rook_v) && 
            (!And(WhiteKing,black_pawn_race_btm[square1]) ||
             !And(WhiteKing,black_pawn_race_btm[square2])))
          score-=3*PAWN_CONNECTED_PASSED_SIXTH;
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
|   is less valuable since it can't advance easily.        |
|                                                          |
 ----------------------------------------------------------
*/
  if (passed_w) {
    white_king_sq=WhiteKingSQ;
    pawns=passed_w;
    while (pawns) {
      file=first_ones_8bit[pawns];
      pawns&=~(128>>file);
      square=LastOne(And(WhitePawns,file_mask[file]));
      if (Distance(square,white_king_sq) < 2) 
        score+=supported_passer[Rank(square)];
      if (And(set_mask[square+8],BlackPieces))
        score-=(PAWN_PASSED*Rank(square))>>1;
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
    pawns=passed_w;
    while ((file=connected_passed[pawns])) {
      register int square1,square2;
      pawns&=~(128>>file);
      square1=LastOne(And(WhitePawns,file_mask[file-1]));
      if (Rank(square1) < RANK6) continue;
      square2=LastOne(And(WhitePawns,file_mask[file]));
      if (Rank(square2) < RANK6) continue;
      score+=PAWN_CONNECTED_PASSED_SIXTH;
      if (TotalBlackPieces < queen_v) {
        score+=PAWN_CONNECTED_PASSED_SIXTH;
        if ((TotalBlackPieces <= rook_v) && 
            (!And(BlackKing,white_pawn_race_wtm[square1]) ||
             !And(BlackKing,white_pawn_race_wtm[square2])))
          score+=3*PAWN_CONNECTED_PASSED_SIXTH;
      }
    }
  }
#ifdef DEBUGPP
  printf("score.2 after white passers = %d\n", score);
#endif
  return(score);
}

/* last modified 05/16/96 */
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
int EvaluatePassedPawnRaces(int wtm)
{
  register int file, square;
  register int white_queener=8, white_square=0;
  register int black_queener=8, black_square=0;
  register int white_pawn=0, black_pawn=0, queen_distance;
  register int white_protected=0, black_protected=0;
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
    if (Distance(WhiteKingSQ,pawnsq)<=Distance(BlackKingSQ,pawnsq)) {
      if (Rank(WhiteKingSQ) > (Rank(pawnsq)+1))
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
    if ((Rank(WhiteKingSQ) == (Rank(pawnsq)>+1)) &&
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
    if (Distance(BlackKingSQ,pawnsq)<=Distance(WhiteKingSQ,pawnsq)) {
      if (Rank(BlackKingSQ) < (Rank(pawnsq)-1))
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
    if ((Rank(BlackKingSQ) == (Rank(pawnsq)-1)) &&
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
  if (!TotalWhitePieces && passed_b) {
    passed=passed_b;
    while ((file=first_ones_8bit[passed]) != 8) {
      passed&=~(128>>file);
      square=FirstOne(And(BlackPawns,file_mask[file]));
      if (And(w_pawn_attacks[square],BlackPawns)) black_protected=1;
      if ((wtm && !And(black_pawn_race_wtm[square],WhiteKing)) ||
          (ChangeSide(wtm) && !And(black_pawn_race_btm[square],WhiteKing))) {
        queen_distance=Rank(square);
        if (And(BlackKing,mask_minus8dir[square])) queen_distance++;
        if (Rank(square) == RANK7) queen_distance--;
        if (queen_distance < black_queener) {
          black_queener=queen_distance;
          black_square=File(square);
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
  if (!TotalBlackPieces && passed_w) {
    passed=passed_w;
    while ((file=first_ones_8bit[passed]) != 8) {
      passed&=~(128>>file);
      square=LastOne(And(WhitePawns,file_mask[file]));
      if (And(b_pawn_attacks[square],WhitePawns)) white_protected=1;
      if ((wtm && !And(white_pawn_race_wtm[square],BlackKing)) ||
          (ChangeSide(wtm) && !And(white_pawn_race_btm[square],BlackKing))) {
        queen_distance=RANK8-Rank(square);
        if (And(WhiteKing,mask_plus8dir[square])) queen_distance++;
        if (Rank(square) == RANK2) queen_distance--;
        if (queen_distance < white_queener) {
          white_queener=queen_distance;
          white_square=File(square)+56;
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
      return(QUEEN_VALUE-BISHOP_VALUE+white_queener*PAWN_VALUE/10);
    else if ((black_queener < 8) && (white_queener == 8))
      return(-(QUEEN_VALUE-BISHOP_VALUE+black_queener*PAWN_VALUE/10));
    if (ChangeSide(wtm)) black_queener--;
    if (white_queener < black_queener)
      return(QUEEN_VALUE-BISHOP_VALUE+white_queener*PAWN_VALUE/10);
    else if (black_queener < white_queener-1)
      return(-(QUEEN_VALUE-BISHOP_VALUE+black_queener*PAWN_VALUE/10));
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
      WhitePieces=Or(WhitePieces,set_mask[white_square]);
      tempb=BlackPieces;
      Clear(black_pawn,BlackPieces);
      BlackPieces=Or(BlackPieces,set_mask[black_square]);
      if (Attack(BlackKingSQ,white_square,ply)) {
        WhitePieces=tempw;
        BlackPieces=tempb;
        return(QUEEN_VALUE-BISHOP_VALUE+white_queener*PAWN_VALUE/10);
      }
      if (Attack(black_square,white_square,ply) &&
          !And(king_attacks[black_square],BlackKing)) {
        WhitePieces=tempw;
        BlackPieces=tempb;
        return(QUEEN_VALUE-BISHOP_VALUE+white_queener*PAWN_VALUE/10);
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
    if (black_queener == (white_queener-1)) {
      tempw=WhitePieces;
      Clear(white_pawn,WhitePieces);
      WhitePieces=Or(WhitePieces,set_mask[white_square]);
      tempb=BlackPieces;
      Clear(black_pawn,BlackPieces);
      BlackPieces=Or(BlackPieces,set_mask[black_square]);
      if (Attack(WhiteKingSQ,black_square,ply)) {
        WhitePieces=tempw;
        BlackPieces=tempb;
        return(-(QUEEN_VALUE-BISHOP_VALUE+black_queener*PAWN_VALUE/10));
      }
      if (Attack(white_square,black_square,ply) &&
          !And(king_attacks[white_square],WhiteKing)) {
        WhitePieces=tempw;
        BlackPieces=tempb;
        return(-(QUEEN_VALUE-BISHOP_VALUE+black_queener*PAWN_VALUE/10));
      }
      WhitePieces=tempw;
      BlackPieces=tempb;
    }
  } while(0);
/*
 ----------------------------------------------------------
|                                                          |
|   if one side has a protected passed pawn, and the other |
|   side has no pieces, then the protected passer is a     |
|   *real* advantage, unless both sides have one.          |
|                                                          |
 ----------------------------------------------------------
*/
  if (white_protected && !black_protected) 
    return(PAWN_PROTECTED_PASSER_WINS);
  else if (!white_protected && black_protected) 
    return(-PAWN_PROTECTED_PASSER_WINS);
  else
    return(0);
}

/* last modified 11/12/96 */
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
*   pawn hashtable entry format:  two 64bit words.                             *
*                                                                              *
*     bits   SL  description (word1)                                           *
*      15    49  pawn score +16384 to make sure it's positive                  *
*       3    46  weak pawns (white) that are on open files                     *
*       3    43  weak pawns (black) that are on open files                     *
*      43     0  hash key (leftmost bits, right bits are address)              *
*                                                                              *
*     bits   SL  description (word2)                                           *
*       8    56  white king safety defects (queen-side)                        *
*       8    48  white king safety defects (king-side)                         *
*       8    40  black king safety defects (queen-side)                        *
*       8    32  black king safety defects (king-side)                         *
*       8    24  passed white pawns (8bit mask 0x80=a file, 0x1=h file)        *
*       8    16  passed black pawns (8bit mask 0x80=a file, 0x1=h file)        *
*       8     8  weak white pawns (8bit mask 0x80=a file, 0x1=h file)          *
*       8     0  weak black pawns (8bit mask 0x80=a file, 0x1=h file)          *
*                                                                              *
********************************************************************************
*/
int EvaluatePawns()
{
  register BITBOARD temp_key, pawns, rams;
  register HASH_ENTRY *ptable;
  register int square, attackers, defenders, file, score;
  register int w_weak, w_isolated, b_weak, b_isolated, temp;
/*
 ----------------------------------------------------------
|                                                          |
|   first check to see if this position has been handled   |
|   before.  if so, we can skip the work saved in the pawn |
|   hash table.                                            |
|                                                          |
 ----------------------------------------------------------
*/
  temp_key=PawnHashKey;
  last_hash=temp_key;
  ptable=pawn_hash_table+(temp_key&pawn_hash_mask);
  temp_key=temp_key>>21;
  if (!Xor(And(ptable->word1,mask_85),temp_key)) {
#if !defined(FAST)
    pawn_hashes++;
#endif
    p_score=Shiftr(ptable->word1,49)-16384;
    black_pof=((unsigned int)(ptable->word1>>43))&7;
    white_pof=((unsigned int)(ptable->word1>>46))&7;
    weak_b=((unsigned int)ptable->word2)&255;
    weak_w=(((unsigned int)ptable->word2)>>8)&255;
    passed_b=(((unsigned int)ptable->word2)>>16)&255;
    passed_w=(((unsigned int)ptable->word2)>>24)&255;
    temp=(unsigned int) Shiftr(ptable->word2,32);
    black_defects_k=temp&255;
    black_defects_q=(temp>>8)&255;
    white_defects_k=(temp>>16)&255;
    white_defects_q=(temp>>24)&255;
    
    return(p_score);
  }
  score=0;
  passed_w=0;
  passed_b=0;
  weak_w=0;
  weak_b=0;
  white_pof=0;
  black_pof=0;
/*
 ----------------------------------------------------------
|                                                          |
|   white pawns.                                           |
|                                                          |
 ----------------------------------------------------------
*/
  w_isolated=0;
  pawns=WhitePawns;
  while (pawns) {
    w_weak=0;
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
    score+=pawn_value_w[square];
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
      w_weak=128;
    }
    else {
      if (!And(mask_pawn_backward_w[square],WhitePawns)) {
        defenders=PopCnt(And(b_pawn_attacks[square+8],WhitePawns));
        attackers=PopCnt(And(w_pawn_attacks[square+8],BlackPawns));
        w_weak=128;
        if (attackers) {
          if (attackers-defenders == 0) score-=PAWN_WEAK_P0;
          else if (attackers-defenders == 1) score-=PAWN_WEAK_P1;
          else if (attackers-defenders == 2) score-=PAWN_WEAK_P2;
          else w_weak=0;
        }
        else w_weak=0;
      }
    }
    if (w_weak) {
      if (!And(mask_plus8dir[square],BlackPawns)) white_pof++;
      weak_w|=w_weak>>file;
    }
#ifdef DEBUGP
  printf("white pawn[weak] file=%d,     score=%d\n",file,score);
#endif
/*
 ----------------------------------------------------------
|                                                          |
|   evaluate doubled pawns.  if there are other pawns on   |
|   this file, penalize thispawn.                          |
|                                                          |
 ----------------------------------------------------------
*/
    if (PopCnt(And(file_mask[file],WhitePawns)) > 1) {
      score-=PAWN_DOUBLED;
#ifdef DEBUGP
      printf("white pawn[doubled] file=%d,     score=%d\n",file,score);
#endif
    }
/*
 ----------------------------------------------------------
|                                                          |
|   evaluate loose pawns.  these are pawns that have       |
|   advanced far enough that neighboring pawns might not   |
|   able to help if they are attacked.                     |
|                                                          |
 ----------------------------------------------------------
*/
    if (And(mask_pawn_isolated[square],WhitePawns) &&
        !And(mask_pawn_connected[square],WhitePawns)) {
      score-=PAWN_LOOSE;
#ifdef DEBUGP
      printf("white pawn[loose] file=%d,       score=%d\n",file,score);
#endif
    }
/*
 ----------------------------------------------------------
|                                                          |
|   evaluate passed pawns.                                 |
|                                                          |
 ----------------------------------------------------------
*/
    if (!And(mask_pawn_passed_w[square],BlackPawns)) {
      score+=PAWN_PASSED*(Rank(square)-1);
      if (Rank(square) == RANK7) score+=PAWN_PASSED_ON_7TH;
      if (And(mask_pawn_isolated[square],WhitePawns)) {
        if (And(mask_pawn_connected[square],WhitePawns))
          score+=connected_passer[Rank(square)];
      }
      passed_w|=128>>file;
#ifdef DEBUGP
  printf("white pawn[passed] file=%d,   score=%d\n",file,score);
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
    if (Rank(square) > RANK3 && And(set_mask[square+8],BlackPawns) &&
        !And(mask_pawn_passed_w[square+8],BlackPawns) &&
        ((File(square) < FILEH && And(set_mask[square-7],WhitePawns) &&
          !And(mask_plus8dir[square+1],BlackPawns) &&
          !And(mask_plus8dir[square+2],BlackPawns)) ||
         (File(square) > FILEA && And(set_mask[square-9],WhitePawns) &&
          !And(mask_plus8dir[square-1],BlackPawns) &&
          !And(mask_plus8dir[square-2],BlackPawns)))) {
      score+=PAWN_HIDDEN_PASSED;
#ifdef DEBUGP
      printf("white pawn[hidden] file=%d,   score=%d\n",file,score);
#endif
    }
    Clear(square,pawns);
  }
/*
 ----------------------------------------------------------
|                                                          |
|   black pawns.                                           |
|                                                          |
 ----------------------------------------------------------
*/
  b_isolated=0;
  pawns=BlackPawns;
  while(pawns) {
    b_weak=0;
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
    score-=pawn_value_b[square];
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
      b_weak=128;
    }
    else {
      if (!And(mask_pawn_backward_b[square],BlackPawns)) {
        defenders=PopCnt(And(w_pawn_attacks[square-8],BlackPawns));
        attackers=PopCnt(And(b_pawn_attacks[square-8],WhitePawns));
        b_weak=128;
        if (attackers) {
          if (attackers-defenders == 0) score+=PAWN_WEAK_P0;
          else if (attackers-defenders == 1) score+=PAWN_WEAK_P1;
          else if (attackers-defenders == 2) score+=PAWN_WEAK_P2;
          else b_weak=0;
        }
        else b_weak=0;
      }
    }
    if (b_weak) {
      if (!And(mask_minus8dir[square],WhitePawns)) black_pof++;
      weak_b|=b_weak>>file;
    }
#ifdef DEBUGP
    printf("black pawn[weak] file=%d,     score=%d\n",file,score);
#endif
/*
 ----------------------------------------------------------
|                                                          |
|   evaluate doubled pawns.  if there are other pawns on   |
|   this file, penalize thispawn.                          |
|                                                          |
 ----------------------------------------------------------
*/
    if (PopCnt(And(file_mask[file],BlackPawns)) > 1) {
      score+=PAWN_DOUBLED;
#ifdef DEBUGP
      printf("black pawn[doubled] file=%d,     score=%d\n",file,score);
#endif
    }
/*
 ----------------------------------------------------------
|                                                          |
|   evaluate loose pawns.  these are pawns that have       |
|   advanced far enough that neighboring pawns might not   |
|   able to help if they are attacked.                     |
|                                                          |
 ----------------------------------------------------------
*/
    if (And(mask_pawn_isolated[square],BlackPawns) &&
        !And(mask_pawn_connected[square],BlackPawns)) {
      score+=PAWN_LOOSE;
#ifdef DEBUGP
      printf("black pawn[loose] file=%d,       score=%d\n",file,score);
#endif
    }
/*
 ----------------------------------------------------------
|                                                          |
|   evaluate passed pawns.                                 |
|                                                          |
 ----------------------------------------------------------
*/
    if (!And(mask_pawn_passed_b[square],WhitePawns)) {
      score-=PAWN_PASSED*(RANK7-Rank(square));
      if (Rank(square) == RANK2) score-=PAWN_PASSED_ON_7TH;
      if (And(mask_pawn_isolated[square],BlackPawns)) {
        if (And(mask_pawn_connected[square],BlackPawns))
          score-=connected_passer[RANK8-Rank(square)];
      }
      passed_b|=128>>file;
#ifdef DEBUGP
  printf("black pawn[passed] file=%d,   score=%d\n",file,score);
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
    if (Rank(square) < RANK4 && And(set_mask[square-8],WhitePawns) &&
        !And(mask_pawn_passed_b[square-8],WhitePawns) &&
        ((File(square) < FILEH && And(set_mask[square+9],BlackPawns) &&
          !And(mask_minus8dir[square+1],WhitePawns) &&
          !And(mask_minus8dir[square+2],WhitePawns)) ||
         (File(square) > FILEA && And(set_mask[square+7],BlackPawns) &&
          !And(mask_minus8dir[square-1],WhitePawns) &&
          !And(mask_minus8dir[square-2],WhitePawns)))) {
      score-=PAWN_HIDDEN_PASSED;
#ifdef DEBUGP
      printf("black pawn[hidden] file=%d,   score=%d\n",file,score);
#endif
    }
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
  if (w_isolated > b_isolated)
    score-=pawns_isolated[w_isolated-b_isolated];
  else if (b_isolated > w_isolated)
    score+=pawns_isolated[b_isolated-w_isolated];
#ifdef DEBUGP
  printf("pawn[isolated]          score=%d\n",score);
#endif

/*
 ----------------------------------------------------------
|                                                          |
|   next count the number of pawn "rams" on the board.     |
|   these are simply two pawns of opposite color on the    |
|   same file, contacting each other face-to-face.  we     |
|   handle three distinct types of rams here.  (1) rams    |
|   where the two pawns are on the 4th/5th rank are just   |
|   blocked.  (2) pawns more advanced than that are "good" |
|   rams, (3) pawns less advanced are cramping and "bad".  |
|                                                          |
 ----------------------------------------------------------
*/
  rams=And(Shiftr(WhitePawns,8),BlackPawns);
  if (rams) {
    if (root_wtm) {
      score-=pawn_rams[PopCnt(rams)]+
             cramping_pawn_rams[PopCnt(And(rams,mask_white_half))]+
             bad_pawn_rams[PopCnt(And(rams,rank_mask[RANK3]))];
    }
    else {
      score+=pawn_rams[PopCnt(rams)]+
             cramping_pawn_rams[PopCnt(And(rams,mask_black_half))]+
             bad_pawn_rams[PopCnt(And(rams,rank_mask[RANK7]))];
    }
#ifdef DEBUGP
  printf("pawn[rams]              score=%d\n",score);
#endif
  }
/*
 ----------------------------------------------------------
|                                                          |
|   now evaluate king safety.  the basic idea is this:  if |
|   a pawn is on its original square, no defect is given.  |
|   if a pawn has advanced 1 rank, defects are added in.   |
|   if the pawn has advanced two files more defects are    |
|   added to the total. otherwise if the pawn is com-      |
|   pletely missing, even more defects are added in, and   |
|   in that case, if the opponent has no pawn on this file |
|   it is even worse.  this is repeated for each file.     |
|                                                          |
 ----------------------------------------------------------
*/
/*
 ------------------------------------------------
|                                                |
|   rook pawn file.  (black)                     |
|                                                |
 ------------------------------------------------
*/
  black_defects_k=0;
  black_defects_q=0;
  if (And(BlackPawns,set_mask[A7]));
  else if (And(BlackPawns,set_mask[A6]))
    black_defects_q+=KING_SAFETY_RP_ADV1;
  else if (And(BlackPawns,set_mask[A5]))
    black_defects_q+=KING_SAFETY_RP_ADV2;
  else if (!And(BlackPawns,file_mask[FILEA]))
    black_defects_q+=KING_SAFETY_RP_MISSING;
  else
    black_defects_q+=KING_SAFETY_RP_TOO_FAR;
  if (!And(WhitePawns,file_mask[FILEA]))
    black_defects_q+=KING_SAFETY_RP_FILE_OPEN;

  if (And(BlackPawns,set_mask[H7]));
  else if (And(BlackPawns,set_mask[H6]))
    black_defects_k+=KING_SAFETY_RP_ADV1;
  else if (And(BlackPawns,set_mask[H5]))
    black_defects_k+=KING_SAFETY_RP_ADV2;
  else if (!And(BlackPawns,file_mask[FILEH]))
    black_defects_k+=KING_SAFETY_RP_MISSING;
  else
    black_defects_k+=KING_SAFETY_RP_TOO_FAR;
  if (!And(WhitePawns,file_mask[FILEH]))
    black_defects_k+=KING_SAFETY_RP_FILE_OPEN;
#ifdef DEBUGK
  printf("black.1, defects=%d(q)  %d(k)\n",black_defects_q,black_defects_k);
#endif
/*
 ------------------------------------------------
|                                                |
|   knight pawn file.  (black)                   |
|                                                |
 ------------------------------------------------
*/
  if (And(set_mask[B7],BlackPawns));
  else if (And(BlackPawns,set_mask[B6])) 
    black_defects_q+=KING_SAFETY_NP_ADV1;
  else if (And(BlackPawns,set_mask[B5]))
    black_defects_q+=KING_SAFETY_NP_ADV2;
  else if (!And(BlackPawns,file_mask[FILEB]))
    black_defects_q+=KING_SAFETY_NP_MISSING;
  else
    black_defects_q+=KING_SAFETY_NP_TOO_FAR;
  if (!And(WhitePawns,file_mask[FILEB]))
    black_defects_q+=KING_SAFETY_NP_FILE_OPEN;

  if (And(set_mask[G7],BlackPawns));
  else if (And(BlackPawns,set_mask[G6])) 
    black_defects_k+=KING_SAFETY_NP_ADV1;
  else if (And(BlackPawns,set_mask[G5]))
    black_defects_k+=KING_SAFETY_NP_ADV2;
  else if (!And(BlackPawns,file_mask[FILEG]))
    black_defects_k+=KING_SAFETY_NP_MISSING;
  else
    black_defects_k+=KING_SAFETY_NP_TOO_FAR;
  if (!And(WhitePawns,file_mask[FILEG]))
    black_defects_k+=KING_SAFETY_NP_FILE_OPEN;
#ifdef DEBUGK
  printf("black.2, defects=%d(q)  %d(k)\n",black_defects_q,black_defects_k);
#endif
/*
 ------------------------------------------------
|                                                |
|   bishop pawn file.  (black)                   |
|                                                |
 ------------------------------------------------
*/
  if (And(BlackPawns,set_mask[C7]));
  else if (And(BlackPawns,set_mask[C6]))
    black_defects_q+=KING_SAFETY_BP_ADV1;
  else if (And(BlackPawns,set_mask[C5]))
    black_defects_q+=KING_SAFETY_BP_ADV2;
  else if (!And(BlackPawns,file_mask[FILEC]))
    black_defects_q+=KING_SAFETY_BP_MISSING;
  else
    black_defects_q+=KING_SAFETY_BP_TOO_FAR;
  if (!And(WhitePawns,file_mask[FILEC]))
    black_defects_q+=KING_SAFETY_BP_FILE_OPEN;

  if (And(BlackPawns,set_mask[F7]));
  else if (And(BlackPawns,set_mask[F6]))
    black_defects_k+=KING_SAFETY_BP_ADV1;
  else if (And(BlackPawns,set_mask[F5]))
    black_defects_k+=KING_SAFETY_BP_ADV2;
  else if (!And(BlackPawns,file_mask[FILEF]))
    black_defects_k+=KING_SAFETY_BP_MISSING;
  else
    black_defects_k+=KING_SAFETY_BP_TOO_FAR;
  if (!And(WhitePawns,file_mask[FILEF]))
    black_defects_k+=KING_SAFETY_BP_FILE_OPEN;
#ifdef DEBUGK
    printf("black.3, defects=%d(q)  %d(k)\n",black_defects_q,black_defects_k);
#endif
/*
 ------------------------------------------------
|                                                |
|   check for an enemy pawn on files close to    |
|   the king that are on the 3rd/4th rank, which |
|   produces some serious mate threats.          |
|                                                |
 ------------------------------------------------
*/
  black_defects_q+=PopCnt(And(mask_bq_3rd,WhitePawns))*
            KING_SAFETY_PAWN_ATTACK_CLOSE;
  black_defects_q+=PopCnt(And(mask_bq_4th,WhitePawns))*
            KING_SAFETY_PAWN_ATTACK;

  black_defects_k+=PopCnt(And(mask_bk_3rd,WhitePawns))*
            KING_SAFETY_PAWN_ATTACK_CLOSE;
  black_defects_k+=PopCnt(And(mask_bk_4th,WhitePawns))*
            KING_SAFETY_PAWN_ATTACK;
#ifdef DEBUGK
  printf("black.4, defects=%d(q)  %d(k)\n",black_defects_q,black_defects_k);
#endif

/*
 ------------------------------------------------
|                                                |
|   rook pawn file.  (white)                     |
|                                                |
 ------------------------------------------------
*/
  white_defects_k=0;
  white_defects_q=0;
  if (And(WhitePawns,set_mask[A2]));
  else if (And(WhitePawns,set_mask[A3]))
    white_defects_q+=KING_SAFETY_RP_ADV1;
  else if (And(WhitePawns,set_mask[A4]))
    white_defects_q+=KING_SAFETY_RP_ADV2;
  else if (!And(WhitePawns,file_mask[FILEA]))
    white_defects_q+=KING_SAFETY_RP_MISSING;
  else
    white_defects_q+=KING_SAFETY_RP_TOO_FAR;
  if (!And(BlackPawns,file_mask[FILEA]))
    white_defects_q+=KING_SAFETY_RP_FILE_OPEN;

  if (And(WhitePawns,set_mask[H2]));
  else if (And(WhitePawns,set_mask[H3]))
    white_defects_k+=KING_SAFETY_RP_ADV1;
  else if (And(WhitePawns,set_mask[H4]))
    white_defects_k+=KING_SAFETY_RP_ADV2;
  else if (!And(WhitePawns,file_mask[FILEH]))
    white_defects_k+=KING_SAFETY_RP_MISSING;
  else
    white_defects_k+=KING_SAFETY_RP_TOO_FAR;
  if (!And(BlackPawns,file_mask[FILEH]))
    white_defects_k+=KING_SAFETY_RP_FILE_OPEN;
#ifdef DEBUGK
  printf("white.1, defects=%d(q)  %d(k)\n",white_defects_q,white_defects_k);
#endif
/*
 ------------------------------------------------
|                                                |
|   knight pawn file.  (white)                   |
|                                                |
 ------------------------------------------------
*/
  if (And(set_mask[B2],WhitePawns));
  else if (And(WhitePawns,set_mask[B3])) 
    white_defects_q+=KING_SAFETY_NP_ADV1;
  else if (And(WhitePawns,set_mask[B4]))
    white_defects_q+=KING_SAFETY_NP_ADV2;
  else if (!And(WhitePawns,file_mask[FILEB]))
    white_defects_q+=KING_SAFETY_NP_MISSING;
  else
    white_defects_q+=KING_SAFETY_NP_TOO_FAR;
  if (!And(BlackPawns,file_mask[FILEB]))
    white_defects_q+=KING_SAFETY_NP_FILE_OPEN;

  if (And(set_mask[G2],WhitePawns));
  else if (And(WhitePawns,set_mask[G3])) 
    white_defects_k+=KING_SAFETY_NP_ADV1;
  else if (And(WhitePawns,set_mask[G4]))
    white_defects_k+=KING_SAFETY_NP_ADV2;
  else if (!And(WhitePawns,file_mask[FILEG]))
    white_defects_k+=KING_SAFETY_NP_MISSING;
  else
    white_defects_k+=KING_SAFETY_NP_TOO_FAR;
  if (!And(BlackPawns,file_mask[FILEG]))
    white_defects_k+=KING_SAFETY_NP_FILE_OPEN;
#ifdef DEBUGK
  printf("white.2, defects=%d(q)  %d(k)\n",white_defects_q,white_defects_k);
#endif
/*
 ------------------------------------------------
|                                                |
|   bishop pawn file.  (white)                   |
|                                                |
 ------------------------------------------------
*/
  if (And(WhitePawns,set_mask[C2]));
  else if (And(WhitePawns,set_mask[C3]))
    white_defects_q+=KING_SAFETY_BP_ADV1;
  else if (And(WhitePawns,set_mask[C4]))
    white_defects_q+=KING_SAFETY_BP_ADV2;
  else if (!And(WhitePawns,file_mask[FILEC]))
    white_defects_q+=KING_SAFETY_BP_MISSING;
  else
    white_defects_q+=KING_SAFETY_BP_TOO_FAR;
  if (!And(BlackPawns,file_mask[FILEC]))
    white_defects_q+=KING_SAFETY_BP_FILE_OPEN;

  if (And(WhitePawns,set_mask[F2]));
  else if (And(WhitePawns,set_mask[F3]))
    white_defects_k+=KING_SAFETY_BP_ADV1;
  else if (And(WhitePawns,set_mask[F4]))
    white_defects_k+=KING_SAFETY_BP_ADV2;
  else if (!And(WhitePawns,file_mask[FILEF]))
    white_defects_k+=KING_SAFETY_BP_MISSING;
  else
    white_defects_k+=KING_SAFETY_BP_TOO_FAR;
  if (!And(BlackPawns,file_mask[FILEF]))
    white_defects_k+=KING_SAFETY_BP_FILE_OPEN;
#ifdef DEBUGK
    printf("white.3, defects=%d(q)  %d(k)\n",white_defects_q,white_defects_k);
#endif
/*
 ------------------------------------------------
|                                                |
|   check for an enemy pawn on files close to    |
|   the king that are on the 3rd/4th rank, which |
|   produces some serious mate threats.          |
|                                                |
 ------------------------------------------------
*/
  white_defects_q+=PopCnt(And(mask_wq_3rd,BlackPawns))*
            KING_SAFETY_PAWN_ATTACK_CLOSE;
  white_defects_q+=PopCnt(And(mask_wq_4th,BlackPawns))*
            KING_SAFETY_PAWN_ATTACK;

  white_defects_k+=PopCnt(And(mask_wk_3rd,BlackPawns))*
            KING_SAFETY_PAWN_ATTACK_CLOSE;
  white_defects_k+=PopCnt(And(mask_wk_4th,BlackPawns))*
            KING_SAFETY_PAWN_ATTACK;
#ifdef DEBUGK
  printf("white.4, defects=%d(q)  %d(k)\n",white_defects_q,white_defects_k);
#endif

/*
 ----------------------------------------------------------
|                                                          |
|   store the results in the pawn hash table for reuse at  |
|   a later time as needed.                                |
|                                                          |
 ----------------------------------------------------------
*/
  ptable->word1=Or(Shiftl((BITBOARD) (((score+16384)<<6)+
                   (white_pof<<3)+black_pof),43),temp_key);
  ptable->word2= Or((BITBOARD) ((unsigned int) ((passed_w<<24)+(passed_b<<16)+
                                     (weak_w<<8)+weak_b)),
                    Shiftl((BITBOARD) ((unsigned int) ((white_defects_q<<24)+
                                     (white_defects_k<<16)+
                                     (black_defects_q<<8)+black_defects_k)),32));
  p_score=score;
  return(score);
}
