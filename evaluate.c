#include <stdio.h>
#include <stdlib.h>
#include "types.h"
#include "function.h"
#include "evaluate.h"
#include "data.h"
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
int Evaluate(int ply, int wtm, int alpha, int beta)
{
  register BITBOARD temp;
  register int square, file, score;
  int passed_w, passed_b;
  register int w_safety, b_safety;
  register int tempo_score;
  register int white_target, white_attack, black_target, black_attack;

/*
**********************************************************************
*                                                                    *
*   determine if this is position needs evaluation, or do we already *
*   have an evaluation from the transposition table.                 *
*                                                                    *
**********************************************************************
*/
  tempo_score=0;
  if (opening) tempo_score=Evaluate_Tempo(ply);
#if !defined(DEBUG_HASH)
  if (positional_evaluation[ply]) {
    evaluations_hashed++;
    if (positional_evaluation[ply] > 0) {
      if (wtm) return( tempo_score+positional_evaluation[ply]-1);
      else return(-tempo_score-positional_evaluation[ply]+1);
    }
    if (wtm) return( tempo_score+positional_evaluation[ply]);
    else return(-tempo_score-positional_evaluation[ply]);
  }
#endif
  evaluations++;
  score=Material(ply);
#ifdef DEBUGEV
  printf("score[material]=                  %d\n",score);
#endif
/*
**********************************************************************
*                                                                    *
*   call Evaluate_Development() to evaluate development.  if the     *
*   flag "opening" is zero, skip the call.                           *
*                                                                    *
**********************************************************************
*/
  if (opening) score+=Evaluate_Development(ply);
#ifdef DEBUGEV
  printf("score[development]=               %d\n",score);
#endif
/*
**********************************************************************
*                                                                    *
*   determine if this is position should be evaluated to force mate  *
*   (neither side has pawns) or if it should be evaluated normally.  *
*   call Evaluate_Pawns() to evaluate the current pawn position.     *
*   this routine modifies the "passed" pawn bit-vector which         *
*   indicates whether a pawn on each file is passed or not.          *
*                                                                    *
**********************************************************************
*/
  if (!White_Pawns(ply) && !Black_Pawns(ply)) {
    score+=Evaluate_Mate(ply);
#ifdef DEBUGEV
    printf("score[mater]=                     %d\n",score);
#endif
    passed_w=0;
    passed_b=0;
  }
  else
    score+=Evaluate_Pawns(ply,&passed_b,&passed_w);
#ifdef DEBUGEV
  printf("score[pawns]=                     %d\n",score);
#endif
/*
**********************************************************************
*                                                                    *
*   call Evaluate_Trades() to determine if trades are good or bad,   *
*   and then check the current path to see what was traded.          *
*                                                                    *
**********************************************************************
*/
  score+=Evaluate_Trades(ply);
#ifdef DEBUGEV
  printf("score[trades]=                    %d\n",score);
#endif
/*
**********************************************************************
*                                                                    *
*   call Evaluate_Passed_Pawns() to evaluate the current position to *
*   discover if one side has a passed pawn that is supported by the  *
*   king which makes it very dangerous.                              *
*                                                                    *
**********************************************************************
*/
  if (passed_b || passed_w)
    score+=Evaluate_Passed_Pawns(ply,passed_b,passed_w);
/*
**********************************************************************
*                                                                    *
*   call Evaluate_Passed_Pawn_Races() to evaluate the current        *
*   position if (and only if) one side has passed pawns and the      *
*   other side has *no* pieces other than the king.  this procedure  *
*   uses the "square of the king" rule to determine if any of the    *
*   pawns can queen by outrunning the defending king.                *
*                                                                    *
**********************************************************************
*/
  if (((!Total_White_Pieces(ply)) && passed_b) ||
      ((!Total_Black_Pieces(ply)) && passed_w))
    score+=Evaluate_Passed_Pawn_Races(ply,wtm,passed_b,passed_w);
#ifdef DEBUGEV
  printf("score[passer races]=              %d\n",score);
#endif
/*
 ----------------------------------------------------------
|                                                          |
|   now evaluate the passed pawns (if any) found by the    |
|   pawn evaluation.  this code determines if either side  |
|   has any passed pawns, and if so, is it "remote" or     |
|   "outside" the opponent's passed pawn.  if so, it is    |
|   a winning advantage since it will serve to decoy the   |
|   king away from the remaining pawns.                    |
|                                                          |
 ----------------------------------------------------------
*/
  if (passed_b || passed_w)
    score+=Evaluate_Outside_Passed_Pawns(ply,passed_b,passed_w);
#ifdef DEBUGEV
  printf("score[outside passers]=           %d\n",score);
#endif
/*
**********************************************************************
*                                                                    *
*   call Evaluate_King_Safety_*() to evaluate the current king       *
*   safety for each side.  we will keep w_safety and b_safety for    *
*   use later on to evaluate pieces and how they are attacking the   *
*   king (if it's exposed).                                          *
*                                                                    *
**********************************************************************
*/
  if ((Total_White_Pieces(ply) > 16) && (Total_Black_Pieces(ply) > 16)) {
    w_safety=Evaluate_King_Safety_W(ply,White_King_SQ(ply));
    b_safety=Evaluate_King_Safety_B(ply,Black_King_SQ(ply));
    if (root_wtm)
      score+=2*KING_SAFETY*w_safety-KING_SAFETY*b_safety;
    else
      score+=KING_SAFETY*w_safety-2*KING_SAFETY*b_safety;
    white_attack=black_attack=KING_TROPISM;
    if (root_wtm && (w_safety > ACCEPTABLE_FAULTS)) {
      white_target=White_King_SQ(ply)+16;
      black_target=White_King_SQ(ply);
    }
    else if (!root_wtm && (b_safety > ACCEPTABLE_FAULTS)) {
      white_target=Black_King_SQ(ply);
      black_target=Black_King_SQ(ply)-16;
    }
    else {
      white_target=Black_King_SQ(ply);
      black_target=White_King_SQ(ply);
    }
  }
  else {
    w_safety=0;
    white_target=Black_King_SQ(ply);
    white_attack=KING_TROPISM;
    b_safety=0;
    black_target=White_King_SQ(ply);
    black_attack=KING_TROPISM;
  }
#ifdef DEBUGEV
  printf("score[king safety]=               %d\n",score);
#endif
  if (Total_White_Pawns(ply) && Total_Black_Pawns(ply)) {
    if (wtm) {
      if((score < alpha-2000) || (score > beta+2000)) return(score);
    }
    else {
      if((-score < alpha-2000) || (-score > beta+2000)) return(-score);
    }
  }
  open_files=0;
  temp=Or(White_Pawns(ply),Black_Pawns(ply));
  for (file=0;file<8;file++)
    if (!And(file_mask[file],temp)) open_files=Or(open_files,file_mask[file]);
/*
**********************************************************************
*                                                                    *
*   now evaluate the kings.  currently, this only includes detecting *
*   a weak back rank.  if there are no rooks/queen on the back rank, *
*   then the king needs a safe flight square (luft) to avoid back    *
*   mate problems.                                                   *
*                                                                    *
**********************************************************************
*/
  if (end_game) score+=king_value_w[White_King_SQ(ply)];
  if (White_King_SQ(ply) < 8)
    if (!And(And(king_attacks[White_King_SQ(ply)],rank_mask[1]),
             Compl(White_Pawns(ply))) && !And(And(Rooks_Queens(ply),
                 White_Pieces(ply)), rank_mask[0]))
        score-=KING_BACK_RANK;
  if (end_game) score-=king_value_b[Black_King_SQ(ply)];
  if (Black_King_SQ(ply) > 55)
    if (!And(And(king_attacks[Black_King_SQ(ply)],rank_mask[6]),
             Compl(Black_Pawns(ply))) && !And(And(Rooks_Queens(ply),
                 Black_Pieces(ply)), rank_mask[7]))
        score+=KING_BACK_RANK;
/*
**********************************************************************
*                                                                    *
*   knight evaluation include centralization and "outposts".         *
*                                                                    *
**********************************************************************
*/
/*
 ----------------------------------------------------------
|                                                          |
|   first, evaluate for "outposts" which is a knight that  |
|   can't be driven off by an enemy pawn, and which is     |
|   supported by a friendly pawn.                          |
|                                                          |
 ----------------------------------------------------------
*/
  temp=White_Knights(ply);
  while(temp) {
    square=First_One(temp);
    if (white_outpost[square] &&
        !And(mask_no_pawn_attacks_b[square],Black_Pawns(ply)) &&
        And(b_pawn_attacks[square],White_Pawns(ply)))
      score+=KNIGHT_OUTPOST*white_outpost[square];
/*
 ----------------------------------------------------------
|                                                          |
|   now fold in centralization score from the piece/square |
|   table "knight_value_*" and king tropism.               |
|                                                          |
 ----------------------------------------------------------
*/
    score+=knight_value_w[square];
    score+=(4-Distance(square,white_target))*white_attack;
    Clear(square,temp);
  }
#ifdef DEBUGEV
  printf("score[knights(white)]=            %d\n",score);
#endif

/*
 ----------------------------------------------------------
|                                                          |
|   first, evaluate for "outposts" which is a knight that  |
|   can't be driven off by an enemy pawn, and which is     |
|   supported by a friendly pawn.                          |
|                                                          |
 ----------------------------------------------------------
*/
  temp=Black_Knights(ply);
  while(temp) {
    square=First_One(temp);
    if (black_outpost[square] &&
        !And(mask_no_pawn_attacks_w[square],White_Pawns(ply)) &&
        And(w_pawn_attacks[square],Black_Pawns(ply)))
      score-=KNIGHT_OUTPOST*black_outpost[square];
/*
 ----------------------------------------------------------
|                                                          |
|   now fold in centralization score from the piece/square |
|   table "knight_value_*" and king tropism.               |
|                                                          |
 ----------------------------------------------------------
*/
    score-=knight_value_b[square];
    score-=(4-Distance(square,black_target))*black_attack;
    Clear(square,temp);
  }
#ifdef DEBUGEV
  printf("score[knights (black)]=           %d\n",score);
#endif
/*
**********************************************************************
*                                                                    *
*   bishop evaluation include mobility and centralization as well as *
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
|   first, locate each bishop and score its mobility by    |
|   multiplying the number of squares it attacks by its    |
|   mobility scoring bonus.                                |
|                                                          |
 ----------------------------------------------------------
*/
  temp=White_Bishops(ply);
  while(temp) {
    square=First_One(temp);
    score+=BISHOP_MOBILITY*(Mobility_Bishop(square)-7);
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
|   check to see if the bishop is fianchettoed in front of |
|   the king and is therefore a valuable defender.         |
|                                                          |
 ----------------------------------------------------------
*/
    if ((Total_Black_Pieces(ply) > 16) && ((square == 9) || (square == 14)))
      if (And(king_attacks[square],White_King(ply)) && Black_Queens(ply)) 
      score+=KING_SAFETY_GOOD_BISHOP;
/*
 ----------------------------------------------------------
|                                                          |
|   check to see if the bishop is trapped at a7 or h7 with |
|   a pawn at b6 or g6 that has trapped the bishop, and    |
|   that this trapping pawn is defended at least one time. |
|                                                          |
 ----------------------------------------------------------
*/
    if (square == 48) {
      if (And(set_mask[49],Black_Pawns(ply)) && Attacked(41,ply,0))
        score-=BISHOP_TRAPPED;
    }
    else if (square == 55) {
      if (And(set_mask[46],Black_Pawns(ply)) && Attacked(46,ply,0))
        score-=BISHOP_TRAPPED;
    }
    Clear(square,temp);
  }
#ifdef DEBUGEV
  printf("score[bishops (white)]=           %d\n",score);
#endif
/*
 ----------------------------------------------------------
|                                                          |
|   first, locate each bishop and score its mobility by    |
|   multiplying the number of squares it attacks by its    |
|   mobility scoring bonus.                                |
 ----------------------------------------------------------
*/
  temp=Black_Bishops(ply);
  while(temp) {
    square=First_One(temp);
    score-=BISHOP_MOBILITY*(Mobility_Bishop(square)-7);
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
|   check to see if the bishop is fianchettoed in front of |
|   the king and is therefore a valuable defender.         |
|                                                          |
 ----------------------------------------------------------
*/
    if ((Total_White_Pieces(ply) > 16) && ((square == 49) || (square == 54)))
      if (And(king_attacks[square],Black_King(ply)) && White_Queens(ply)) 
      score-=KING_SAFETY_GOOD_BISHOP;
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
    if (square == 8) {
      if (And(set_mask[17],White_Pawns(ply)) && Attacked(17,ply,1))
        score+=BISHOP_TRAPPED;
    }
    else if (square == 15) {
      if (And(set_mask[22],White_Pawns(ply)) && Attacked(22,ply,1))
        score+=BISHOP_TRAPPED;
    }
    Clear(square,temp);
  }
#ifdef DEBUGEV
  printf("score[bishops (black)]=           %d\n",score);
#endif
/*
 ----------------------------------------------------------
|                                                          |
|   now, give either side a bonus for having two bishops.  |
|                                                          |
 ----------------------------------------------------------
*/
  if (And(White_Bishops(ply),White_Bishops(ply)-1)) score+=BISHOP_PAIR;
  if (And(Black_Bishops(ply),Black_Bishops(ply)-1)) score-=BISHOP_PAIR;
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
  temp=White_Rooks(ply);
  while(temp) {
    square=First_One(temp);
    file=square&7;
/*
 ----------------------------------------------------------
|                                                          |
|   first, count mobility.                                 |
|                                                          |
 ----------------------------------------------------------
*/
    score+=ROOK_MOBILITY*(Mobility_Rook(square)-7);
/*
 ----------------------------------------------------------
|                                                          |
|   then, see if the rook is on an open file.  if it is,   |
|   determine if this rook attacks another friendly rook,  |
|   making it difficult to drive the rooks off the file.   |
|   if the file is not open, see if it's half open and     |
|   give a bonus if so.                                    |
|                                                          |
 ----------------------------------------------------------
*/
    if (And(file_mask[file],open_files)) {
      score+=ROOK_OPEN_FILE;
      if (And(Attacks_File(square),White_Rooks(ply)))
        score+=ROOK_CONNECTED_OPEN_FILE;
    }
    else
      if (!And(file_mask[file],White_Pawns(ply))) score+=ROOK_HALF_OPEN_FILE;
/*
 ----------------------------------------------------------
|                                                          |
|   if not, check to see if the rook is "attacking" any    |
|   open files.  penalize if not, so that the rook won't   |
|   move into bizarre positions, and, instead, will be     |
|   able to move to the open file quickly.                 |
|                                                          |
 ----------------------------------------------------------
*/
    else
      if (open_files && !And(Attacks_Rank(square),open_files))
        score-=ROOK_POORLY_PLACED;
/*
 ----------------------------------------------------------
|                                                          |
|   see if the rook is behind a passed pawn.  if it is,    |
|   it is counted as though the file is open.              |
|                                                          |
 ----------------------------------------------------------
*/
    if (((128 >> file) & passed_w) && (square < Last_One(And(White_Pawns(ply),
                               file_mask[file]))))
      score+=ROOK_BEHIND_PASSED_PAWN;
    if (((128 >> file) & passed_b) && (square > First_One(And(Black_Pawns(ply),
                                file_mask[file]))))
      score+=ROOK_BEHIND_PASSED_PAWN;
/*
 ----------------------------------------------------------
|                                                          |
|   add in a bonus for occupying a rank or file close to   |
|   the king.                                              |
|                                                          |
 ----------------------------------------------------------
*/
    score+=(4-Distance(square,white_target))*white_attack;
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
    if (((square>>3) == 6) && (And(Black_Pawns(ply),rank_mask[6]) ||
        (Black_King_SQ(ply) >55))) {
      score+=ROOK_ON_7TH;
      if (And(Attacks_Rank(square),Or(White_Rooks(ply),White_Queens(ply))))
        score+=ROOK_CONNECTED_7TH_RANK;
    }
    Clear(square,temp);
  }
#ifdef DEBUGEV
  printf("score[rooks (white)]=             %d\n",score);
#endif
  temp=Black_Rooks(ply);
  while(temp) {
    square=First_One(temp);
    file=square&7;
/*
 ----------------------------------------------------------
|                                                          |
|   first, count mobility.                                 |
|                                                          |
 ----------------------------------------------------------
*/
    score-=ROOK_MOBILITY*(Mobility_Rook(square)-7);
/*
 ----------------------------------------------------------
|                                                          |
|   next, see if the rook is on an open file.  if it is,   |
|   determine if this rook attacks another friendly rook,  |
|   making it difficult to drive the rooks off the file.   |
|   if the file is not open, see if it's half open and     |
|   give a bonus if so.                                    |
|                                                          |
 ----------------------------------------------------------
*/
    if (And(file_mask[file],open_files)) {
      score-=ROOK_OPEN_FILE;
      if (And(Attacks_File(square),Black_Rooks(ply)))
        score-=ROOK_CONNECTED_OPEN_FILE;
    }
    else
      if (!And(file_mask[file],Black_Pawns(ply))) score-=ROOK_HALF_OPEN_FILE;
/*
 ----------------------------------------------------------
|                                                          |
|   if not, check to see if the rook is "attacking" any    |
|   open files.  penalize if not, so that the rook won't   |
|   move into bizarre positions, and, instead, will be     |
|   able to move to the open file quickly.                 |
|                                                          |
 ----------------------------------------------------------
*/
    else
      if (open_files && !And(Attacks_Rank(square),open_files))
        score+=ROOK_POORLY_PLACED;
/*
 ----------------------------------------------------------
|                                                          |
|   if not, see if the rook is behind a passed pawn.  if   |
|   it is, it is counted as though the file is open.       |
|                                                          |
 ----------------------------------------------------------
*/
    if (((128 >> file) & passed_b) && (square > First_One(And(Black_Pawns(ply),
                                file_mask[file]))))
      score-=ROOK_BEHIND_PASSED_PAWN;
    if (((128 >> file) & passed_w) && (square < Last_One(And(White_Pawns(ply),
                               file_mask[file]))))
      score-=ROOK_BEHIND_PASSED_PAWN;
/*
 ----------------------------------------------------------
|                                                          |
|   add in a bonus for occupying a rank or file close to   |
|   the king.                                              |
|                                                          |
 ----------------------------------------------------------
*/
    score-=(4-Distance(square,black_target))*black_attack;
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
    if (((square>>3) == 1) && (And(White_Pawns(ply),rank_mask[1]) ||
        (White_King_SQ(ply) <8))) {
      score-=ROOK_ON_7TH;
      if (And(Attacks_Rank(square),Or(Black_Rooks(ply),Black_Queens(ply))))
        score-=ROOK_CONNECTED_7TH_RANK;
    }
    Clear(square,temp);
  }
#ifdef DEBUGEV
  printf("score[rooks (black)]=             %d\n",score);
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
  temp=White_Queens(ply);
  while(temp) {
    square=First_One(temp);
    score+=queen_value_w[square];
    if (w_safety+ACCEPTABLE_FAULTS < b_safety) score+=QUEEN_IS_STRONG;
/*
 ----------------------------------------------------------
|                                                          |
|   first, count mobility.                                 |
|                                                          |
 ----------------------------------------------------------
*/
    score+=QUEEN_MOBILITY*(Mobility_Queen(square)-14);
/*
 ----------------------------------------------------------
|                                                          |
|   check to see if the queen is in a strong positiono on  |
|   the 7th rank supported by a rook on the 7th.  if so,   |
|   the positional advantage is almost overwhelming.       |
|                                                          |
 ----------------------------------------------------------
*/
    if (((square>>3) == 6) && (And(Black_Pawns(ply),rank_mask[6]) ||
         (Black_King_SQ(ply) > 55))) {
      if (And(Attacks_Rank(square),White_Rooks(ply)))
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
    score+=(4-Distance(square,white_target))*white_attack;
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
  temp=Black_Queens(ply);
  while(temp) {
    square=First_One(temp);
    score-=queen_value_b[square];
    if (b_safety+ACCEPTABLE_FAULTS < w_safety) score-=QUEEN_IS_STRONG;
/*
 ----------------------------------------------------------
|                                                          |
|   first, count mobility.                                 |
|                                                          |
 ----------------------------------------------------------
*/
    score-=QUEEN_MOBILITY*(Mobility_Queen(square)-14);
/*
 ----------------------------------------------------------
|                                                          |
|   check to see if the queen is in a strong positiono on  |
|   the 7th rank supported by a rook on the 7th.  if so,   |
|   the positional advantage is almost overwhelming.       |
|                                                          |
 ----------------------------------------------------------
*/
    if (((square>>3) == 1) && (And(White_Pawns(ply),rank_mask[1]) ||
         (White_King_SQ(ply) < 8))) {
      if (And(Attacks_Rank(square),Black_Rooks(ply)))
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
    score-=(4-Distance(square,black_target))*black_attack;
    Clear(square,temp);
  }
#ifdef DEBUGEV
  printf("score[queens]=                    %d\n",score);
#endif
  if (abs(score-Material(ply)) > largest_positional_score)
    largest_positional_score=abs(score-Material(ply));
/*
 ----------------------------------------------------------
|                                                          |
|   check for draws due to insufficient material and       |
|   adjust the score as necessary.                         |
|                                                          |
 ----------------------------------------------------------
*/
  score=Evaluate_Draws(ply,score);
  if (score < 0) positional_evaluation[ply]=score;
  else positional_evaluation[ply]=score+1;
#if defined(DEBUG_HASH)
  {
    int temp_eval;
    if (positional_evaluation[ply]) {
      temp_eval=positional_evaluation[ply];
      if (temp_eval > 0) temp_eval--;
      if (temp_eval != score) 
        printf("hash error (evaluate)!! ply=%d  hash=%d  compute=%d\n",
               ply,temp_eval,score);
    }
  }
#endif
  if (wtm) return( tempo_score+score);
  else return(-tempo_score-score);
}

/*
********************************************************************************
*                                                                              *
*   Evaluate_Development() is used to encourage the program to develop its     *
*   pieces before moving its queen.  standard developmental principles are     *
*   applied.  they include:  (1) don't move the queen until minor pieces are   *
*   developed;  (2) advance the center pawns as soon as possible;  (3) don't   *
*   move the king unless its a castling move.                                  *
*                                                                              *
********************************************************************************
*/
int Evaluate_Development(int ply)
{
  register int i, possible, real, w_score, b_score;
  register int gave_up_castle_b, gave_up_castle_w;
  register int square, moves;
  register BITBOARD temp;

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
  if (And(set_mask[27],White_Pawns(ply))) {
    if (root_wtm) {
      if (And(set_mask[10],White_Pawns(ply)) &&
          And(set_mask[18],Or(White_Knights(ply),
                              White_Bishops(ply))))
      w_score+=DEVELOPMENT_THEMATIC;
    }
    else {
      if (And(set_mask[50],Black_Pawns(ply)) &&
          And(set_mask[42],Or(Black_Knights(ply),
                              Black_Bishops(ply))))
      b_score-=DEVELOPMENT_THEMATIC;
    }
  }
/*
 ----------------------------------------------------------
|                                                          |
|   if all minor pieces aren't developed, then penalize    |
|   the queen if it has moved.                             |
|                                                          |
 ----------------------------------------------------------
*/
  if (root_wtm) {
    if ((Popcnt(And(Or(White_Knights(ply),White_Bishops(ply)),
         white_minor_pieces)) > 1) && !And(White_Queens(ply),set_mask[3]))
      w_score+=DEVELOPMENT_QUEEN_EARLY;
  }
  else {
    if ((Popcnt(And(Or(Black_Knights(ply),Black_Bishops(ply)),
         black_minor_pieces)) > 1) && !And(Black_Queens(ply),set_mask[59]))
      b_score-=DEVELOPMENT_QUEEN_EARLY;
  }
#ifdef DEBUGDV
  printf("development.1 w_score=%d  b_score=%d\n",w_score, b_score);
#endif
/*
 ----------------------------------------------------------
|                                                          |
|   check to see if center pawns are blocked.              |
|                                                          |
 ----------------------------------------------------------
*/
  if (root_wtm) {
    if (And(Or(Or(White_Knights(ply),White_Bishops(ply)),White_Queens(ply)),
            And(Shiftr(And(White_Pawns(ply),rank_mask[1]),8),
                Or(file_mask[3],file_mask[4]))))
      w_score+=DEVELOPMENT_BLOCKED_PAWN;
  }
  else {
    if (And(Or(Or(Black_Knights(ply),Black_Bishops(ply)),Black_Queens(ply)),
            And(Shiftl(And(Black_Pawns(ply),rank_mask[6]),8),
                Or(file_mask[3],file_mask[4]))))
      b_score-=DEVELOPMENT_BLOCKED_PAWN;
  }
#ifdef DEBUGDV
  printf("development.2 w_score=%d  b_score=%d\n",w_score, b_score);
#endif
/*
 ----------------------------------------------------------
|                                                          |
|   check to see if center pawns have moved.               |
|                                                          |
 ----------------------------------------------------------
*/
  if (root_wtm)
    w_score+=Popcnt(And(White_Pawns(ply),
                        white_center_pawns))*DEVELOPMENT_UNMOVED_PAWNS;
  else
    b_score-=Popcnt(And(Black_Pawns(ply),
                        black_center_pawns))*DEVELOPMENT_UNMOVED_PAWNS;
#ifdef DEBUGDV
  printf("development.3 w_score=%d  b_score=%d\n",w_score, b_score);
#endif
/*
 ----------------------------------------------------------
|                                                          |
|   check to see if pieces (bishops at present) are        |
|   cramped (usually by a knight.)                         |
|                                                          |
 ----------------------------------------------------------
*/
  if (root_wtm) {
    temp=White_Bishops(ply);
    while(temp) {
      square=First_One(temp);
      moves=Popcntl(And(Attacks_Bishop(square),Compl(White_Pieces(ply))));
      if (!moves) w_score+=DEVELOPMENT_CRAMPED;
      Clear(square,temp);
    }
  }
  else {
    temp=Black_Bishops(ply);
    while(temp) {
      square=First_One(temp);
      moves=Popcntl(And(Attacks_Bishop(square),Compl(Black_Pieces(ply))));
      if (!moves) b_score-=DEVELOPMENT_CRAMPED;
      Clear(square,temp);
    }
  }
#ifdef DEBUGDV
  printf("development.4 w_score=%d  b_score=%d\n",w_score, b_score);
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
 ----------------------------------------------------------
*/
  gave_up_castle_w=0;
  if (White_Castle(1)&1) {
    if (!(White_Castle(ply)&1)) {
      for (i=3-root_wtm;i<=ply;i+=2) {
        if (!(White_Castle(i)&1)) {
          if ((Piece(current_move[i-1]) != 6) ||
              (abs(From(current_move[i-1])-To(current_move[i-1])) != 2)) {
            w_score+=DEVELOPMENT_LOSING_CASTLE;
            gave_up_castle_w=1;
          }
          else if (White_Castle(i-1) == 3) {
            real=KING_SAFETY*Evaluate_King_Safety_W(ply,To(current_move[i-1]));
            if (To(current_move[i-1]) == 6)
              possible=KING_SAFETY*Evaluate_King_Safety_W(ply,2);
            else
              possible=KING_SAFETY*Evaluate_King_Safety_W(ply,6);
            if (possible > real+50) {
              w_score-=3*(possible-real);
              gave_up_castle_w=1;
            }
          }
          break;
        }
      }
    }
  }
  gave_up_castle_b=0;
  if (Black_Castle(1)&1) {
    if (!(Black_Castle(ply)&1)) {
      for (i=2+root_wtm;i<=ply;i+=2) {
        if (!(Black_Castle(i)&1)) {
          if ((Piece(current_move[i-1]) != 6) ||
              (abs(From(current_move[i-1])-To(current_move[i-1])) != 2)) {
            b_score-=DEVELOPMENT_LOSING_CASTLE;
            gave_up_castle_b=1;
          }
          else if (Black_Castle(i-1) == 3) {
            real=KING_SAFETY*Evaluate_King_Safety_B(ply,To(current_move[i-1]));
            if (To(current_move[i-1]) == 62)
              possible=KING_SAFETY*Evaluate_King_Safety_B(ply,58);
            else
              possible=KING_SAFETY*Evaluate_King_Safety_B(ply,62);
            if (possible > real+50) {
              b_score+=3*(possible-real);
              gave_up_castle_b=1;
            }
          }
          break;
        }
      }
    }
  }
#ifdef DEBUGDV
  printf("development.5 w_score=%d  b_score=%d\n",w_score, b_score);
#endif
/*
 ----------------------------------------------------------
|                                                          |
|   if the king hasn't castled, penalize the score for     |
|   two reasons:  (1) to encourage castling to get out of  |
|   the center of the board; (2) Evaluate_Development()    |
|   will return a non-zero score which will continue to    |
|   monitor developmental status until castling occurs.    |
|                                                          |
 ----------------------------------------------------------
*/
  if (root_wtm) {
    if (White_Castle(ply) || gave_up_castle_w) w_score+=DEVELOPMENT_NOT_CASTLED;
  }
  else {
    if (Black_Castle(ply) || gave_up_castle_b) b_score-=DEVELOPMENT_NOT_CASTLED;
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

  return(w_score+b_score);
}

/*
********************************************************************************
*                                                                              *
*   Evaluate_Draws() is used to catch positions where one side is ahead in     *
*   material, but has insufficient material to force mate.                     *
*                                                                              *
*   the rules are simple:  (1) if both sides have pawns, it is not yet a draw; *
*   (2) if one side has no pawns, then the score cannot be in favor of that    *
*   side, it is taken to DRAW;  (3) if neither side has pawns, and neither     *
*   side has at least a bishop and knight, then the score is taken to DRAW.    *
*                                                                              *
********************************************************************************
*/
int Evaluate_Draws(int ply, int score)
{
/*
 ----------------------------------------------------------
|                                                          |
|   if both sides have pawns, the game is not a draw for   |
|   lack of material.                                      |
|                                                          |
 ----------------------------------------------------------
*/
  if (White_Pawns(ply) && Black_Pawns(ply)) return(score);
#ifdef DEBUGD
  printf("both sides do not have pawns.\n");
#endif
/*
 ----------------------------------------------------------
|                                                          |
|   if neither side has pawns, then one side must have     |
|   at least a bishop and knight (or rook, or queen...)    |
|   to be able to force a win.                             |
|                                                          |
 ----------------------------------------------------------
*/
  if ((!White_Pawns(ply)) && (!Black_Pawns(ply))) {
    if ((Total_White_Pieces(ply)!=5) || (Total_Black_Pieces(ply)!=5)) {
      if ((Total_White_Pieces(ply)==5) || (Total_White_Pieces(ply)>6) ||
          ((Total_White_Pieces(ply)==6) && (White_Bishops(ply)))) return(score);
      if ((Total_Black_Pieces(ply)==5) || (Total_Black_Pieces(ply)>6) ||
          ((Total_Black_Pieces(ply)==6) && (Black_Bishops(ply)))) return(score);
    }
    if (root_wtm) return(Draw_Score());
    else return(-Draw_Score());
  }
#ifdef DEBUGD
  printf("one side has pawns\n");
#endif
/*
 ----------------------------------------------------------
|                                                          |
|   if white has no pawns, and white has insufficient      |
|   material to force mate, and the score indicates that   |
|   white is winning, return DRAW.                         |
|                                                          |
 ----------------------------------------------------------
*/
  if (!White_Pawns(ply)) {
    if ((Total_White_Pieces(ply)==5) || (Total_White_Pieces(ply)>6) ||
        ((Total_White_Pieces(ply)==6) && (White_Bishops(ply)))) return(score);
#ifdef DEBUGD
  printf("white can not win.\n");
#endif
    if (score > 0) return(score-Material(ply));
    else return(score);
  }
/*
 ----------------------------------------------------------
|                                                          |
|   if black has no pawns, and black has insufficient      |
|   material to force mate, and the score indicates that   |
|   black is winning, return DRAW.                         |
|                                                          |
 ----------------------------------------------------------
*/
  if (!Black_Pawns(ply)) {
    if ((Total_Black_Pieces(ply)==5) || (Total_Black_Pieces(ply)>6) ||
        ((Total_Black_Pieces(ply)==6) && (Black_Bishops(ply)))) return(score);
#ifdef DEBUGD
  printf("black can not win.\n");
#endif
    if (score < 0) return(score-Material(ply));
    else return(score);
  }
  return(score);
}

/*
********************************************************************************
*                                                                              *
*   Evaluate_King_Safety_B() is used to evaluate king safety for the black     *
*   pieces.  it is primarily interested in the pawn structure around the king. *
*   it checks for missing pawns, open files, pawns too far advanced, weak      *
*   squares around the king and so forth.  this score is hashed to save time,  *
*   in a manner similar to the Evaluate_Pawns() routine.  the safety score is  *
*   made available in a global place so that evaluate() can use this number to *
*   rate how important it is to attack the black king.                         *
*                                                                              *
********************************************************************************
*/
int Evaluate_King_Safety_B(int ply, int king_square)
{
  register BITBOARD *ktable, king_hash_key;
  register int file, rank, i, faults, rpfile, npfile, bpfile;
#if defined(DEBUG_HASH)
  int t_faults;
  int hashed=0;
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
  king_hash_key=Xor(Pawn_Hash_Key(ply),b_king_random[king_square]);
  ktable=king_hash_table+And(king_hash_key,king_hash_mask);
  if (king_hash_table && !Xor(And(*ktable,mask_72),Shiftr(king_hash_key,8))) {
    king_hashes++;
    faults=Shiftr(*ktable,56);
#ifdef DEBUGK
    printf("black.h, faults=%d\n",faults);
#endif
#if !defined(DEBUG_HASH)
    return(faults);
#else
    hashed=1;
    t_faults=faults;
#endif
  }
/*
 ----------------------------------------------------------
|                                                          |
|   determine which files are bearing on the king's        |
|   position.  we evaluate safety from three perspectives: |
|   king on left side of board, king in the center of      |
|   board, king on right side of board.                    |
|                                                          |
 ----------------------------------------------------------
*/
  faults=0;
  if ((king_square&7) < 3) {
    rpfile=0;
    npfile=1;
    bpfile=2;
  }
  else if ((king_square&7) > 4) {
    rpfile=7;
    npfile=6;
    bpfile=5;
  }
/*
 ----------------------------------------------------------
|                                                          |
|   if the king is in the center of the board, then it     |
|   earns a penalty.  then we check the files around the   |
|   king and penalize additional amounts if they are open. |
|                                                          |
 ----------------------------------------------------------
*/
  else {
    faults+=KING_SAFETY_IN_CENTER;
    for (i=(king_square&7)-1;i<(king_square&7)+2;i++) 
      if (!And(Or(White_Pawns(ply),Black_Pawns(ply)),file_mask[i]))
        faults+=KING_SAFETY_OPEN_FILE;
#ifdef DEBUGK
      printf("black.1, faults=%d\n",faults);
#endif
  }
/*
 ----------------------------------------------------------
|                                                          |
|   now evaluate the files.  the basic idea is this:  if   |
|   a pawn is on its original square, no fault is given.   |
|   if a pawn has advanced 1 rank, faults are added in     |
|   with one exception:  the knight pawn can advance one   |
|   square with a bishop fianchettoed behind it without    |
|   incurring a significant penalty.  if the pawn has      |
|   advanced two files more faults are added to the total. |
|   otherwise if the pawn is completely missing, even more |
|   faults are added in, and in that case, if the opponent |
|   has no pawn on this file it is even worse.  this is    |
|   repeated for each file.                                |
|                                                          |
 ----------------------------------------------------------
*/
/*
 ------------------------------------------------
|                                                |
|   rook pawn file.                              |
|                                                |
 ------------------------------------------------
*/
  if (!faults) {
    if (And(Black_Pawns(ply),set_mask[rpfile+48]));
    else if (And(Black_Pawns(ply),set_mask[rpfile+40]))
      faults+=KING_SAFETY_RP_ADV1;
    else if (And(Black_Pawns(ply),set_mask[rpfile+32]))
      faults+=KING_SAFETY_RP_ADV2;
    else if (!And(Black_Pawns(ply),file_mask[rpfile])) {
      faults+=KING_SAFETY_RP_MISSING;
      if (!And(White_Pawns(ply),file_mask[rpfile]))
        faults+=KING_SAFETY_RP_FILE_OPEN;
    }
    else
      faults+=KING_SAFETY_RP_TOO_FAR;
#ifdef DEBUGK
    printf("black.2, faults=%d\n",faults);
#endif
/*
 ------------------------------------------------
|                                                |
|   knight pawn file.                            |
|                                                |
 ------------------------------------------------
*/
    if (And(Black_Pawns(ply),set_mask[npfile+48]));
    else if (And(Black_Pawns(ply),set_mask[npfile+40])) 
      faults+=KING_SAFETY_NP_ADV1;
    else if (And(Black_Pawns(ply),set_mask[npfile+32]))
      faults+=KING_SAFETY_NP_ADV2;
    else if (!And(Black_Pawns(ply),file_mask[npfile])) {
      faults+=KING_SAFETY_NP_MISSING;
      if (!And(White_Pawns(ply),file_mask[npfile]))
        faults+=KING_SAFETY_NP_FILE_OPEN;
    }
    else
      faults+=KING_SAFETY_NP_TOO_FAR;
#ifdef DEBUGK
    printf("black.3, faults=%d\n",faults);
#endif
/*
 ------------------------------------------------
|                                                |
|   bishop pawn file.  note that this file is    |
|   not evaluated if the rp/np has not been      |
|   moved, since it's then hard to attack down   |
|   the bp file.                                 |
|                                                |
 ------------------------------------------------
*/
    if (faults > 1) {
      if (And(Black_Pawns(ply),set_mask[bpfile+48]));
      else if (And(Black_Pawns(ply),set_mask[bpfile+40]))
        faults+=KING_SAFETY_BP_ADV1;
      else if (And(Black_Pawns(ply),set_mask[bpfile+32]))
        faults+=KING_SAFETY_BP_ADV2;
      else if (!And(Black_Pawns(ply),file_mask[bpfile]))
        faults+=KING_SAFETY_BP_MISSING;
      else
        faults+=KING_SAFETY_BP_TOO_FAR;
#ifdef DEBUGK
      printf("black.4, faults=%d\n",faults);
#endif
    }
/*
 ------------------------------------------------
|                                                |
|   check for an enemy pawn on the b/g file that |
|   has been pushed to b3/g3 which produces some |
|   really dangerous back-rank problems.         |
|                                                |
 ------------------------------------------------
*/
    if (And(set_mask[npfile+16],Black_Pawns(ply)))
      faults+=KING_SAFETY_PAWN_ATTACK;
#ifdef DEBUGK
    printf("black.6, faults=%d\n",faults);
#endif
/*
 ------------------------------------------------
|                                                |
|   penalize the king for advancing out of the   |
|   corner (ie, moving up the board or over      |
|   toward the center.  also penalize the king   |
|   for sitting in the corner squares (a1,h1,a8, |
|   h8) where it's easier to get mated.          |
|                                                |
 ------------------------------------------------
*/
    file=king_square&7;
    if ((file>1) && (file<6)) faults+=KING_SAFETY_NOT_CORNER;
    if ((file>2) && (file<5)) faults+=KING_SAFETY_NOT_CORNER;
    rank=king_square>>3;
    if (rank < 7) faults+=KING_SAFETY_NOT_CORNER;
    if (rank < 6) faults+=KING_SAFETY_NOT_CORNER;
    if (rank < 5) faults+=KING_SAFETY_NOT_CORNER;
    if (And(mask_corner_squares,Black_King(ply)))
      faults+=KING_SAFETY_IN_CORNER;
#ifdef DEBUGK
    printf("black.7, faults=%d\n",faults);
#endif
  }
/*
 ----------------------------------------------------------
|                                                          |
|   store faults in the hash table so that it won't have   |
|   to be computed again.                                  |
|                                                          |
 ----------------------------------------------------------
*/
#if defined(DEBUG_HASH)
  if (hashed && (faults != t_faults))
    printf("hash error (king_b)!!  hash=%d  real=%d  entry=%d\n",
            t_faults, faults, (int) And(king_hash_key,king_hash_mask));
#endif
  if (king_hash_table) 
    *ktable=Or(Shiftl((BITBOARD) faults,56),Shiftr(king_hash_key,8));
  return(faults);
}

/*
********************************************************************************
*                                                                              *
*   Evaluate_King_Safety_W() is used to evaluate king safety for the white     *
*   pieces.  it is primarily interested in the pawn structure around the king. *
*   it checks for missing pawns, open files, pawns too far advanced, weak      *
*   squares around the king and so forth.  this score is hashed to save time,  *
*   in a manner similar to the Evaluate_Pawns() routine.  the safety score is  *
*   made available in a global place so that evaluate() can use this number to *
*   rate how important it is to attack the white king.                         *
*                                                                              *
********************************************************************************
*/
int Evaluate_King_Safety_W(int ply, int king_square)
{
  register BITBOARD *ktable, king_hash_key;
  register int file, rank, i, faults, rpfile, npfile, bpfile;
#if defined(DEBUG_HASH)
  int t_faults;
  int hashed=0;
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
  king_hash_key=Xor(Pawn_Hash_Key(ply),w_king_random[king_square]);
  ktable=king_hash_table+And(king_hash_key,king_hash_mask);
  if (king_hash_table && !Xor(And(*ktable,mask_72),Shiftr(king_hash_key,8))) {
    king_hashes++;
    faults=Shiftr(*ktable,56);
#ifdef DEBUGK
    printf("white.h, faults=%d\n",faults);
#endif
#if !defined(DEBUG_HASH)
    return(faults);
#else
    hashed=1;
    t_faults=faults;
#endif
  }
/*
 ----------------------------------------------------------
|                                                          |
|   determine which files are bearing on the king's        |
|   position.  we evaluate safety from three perspectives: |
|   king on left side of board, king in the center of      |
|   board, king on right side of board.                    |
|                                                          |
 ----------------------------------------------------------
*/
  faults=0;
  if ((king_square&7) < 3) {
    rpfile=0;
    npfile=1;
    bpfile=2;
  }
  else if ((king_square&7) > 4) {
    rpfile=7;
    npfile=6;
    bpfile=5;
  }
/*
 ----------------------------------------------------------
|                                                          |
|   if the king is in the center of the board, then it     |
|   earns a penalty.  then we check the files around the   |
|   king and penalize additional amounts if they are open. |
|                                                          |
 ----------------------------------------------------------
*/
  else {
    faults+=KING_SAFETY_IN_CENTER;
    for (i=(king_square&7)-1;i<(king_square&7)+2;i++) 
      if (!And(Or(White_Pawns(ply),Black_Pawns(ply)),file_mask[i]))
        faults+=KING_SAFETY_OPEN_FILE;
#ifdef DEBUGK
    printf("white.1, faults=%d\n",faults);
#endif
  }
/*
 ----------------------------------------------------------
|                                                          |
|   now evaluate the files.  the basic idea is this:  if   |
|   a pawn is on its original square, no fault is given.   |
|   if a pawn has advanced 1 rank, faults are added in     |
|   with one exception:  the knight pawn can advance one   |
|   square with a bishop fianchettoed behind it without    |
|   incurring a significant penalty.  if the pawn has      |
|   advanced two files more faults are added to the total. |
|   otherwise if the pawn is completely missing, even more |
|   faults are added in, and in that case, if the opponent |
|   has no pawn on this file it is even worse.  this is    |
|   repeated for each file.                                |
|                                                          |
 ----------------------------------------------------------
*/
/*
 ------------------------------------------------
|                                                |
|   rook pawn file.                              |
|                                                |
 ------------------------------------------------
*/
  if (!faults) {
    if (And(White_Pawns(ply),set_mask[rpfile+8]));
    else if (And(White_Pawns(ply),set_mask[rpfile+16]))
      faults+=KING_SAFETY_RP_ADV1;
    else if (And(White_Pawns(ply),set_mask[rpfile+24]))
      faults+=KING_SAFETY_RP_ADV2;
    else if (!And(White_Pawns(ply),file_mask[rpfile])) {
      faults+=KING_SAFETY_RP_MISSING;
      if (!And(Black_Pawns(ply),file_mask[rpfile]))
        faults+=KING_SAFETY_RP_FILE_OPEN;
    }
    else
      faults+=KING_SAFETY_RP_TOO_FAR;
#ifdef DEBUGK
    printf("white.2, faults=%d\n",faults);
#endif
/*
 ------------------------------------------------
|                                                |
|   knight pawn file.                            |
|                                                |
 ------------------------------------------------
*/
    if (And(White_Pawns(ply),set_mask[npfile+8]));
    else if (And(White_Pawns(ply),set_mask[npfile+16])) 
      faults+=KING_SAFETY_NP_ADV1;
    else if (And(White_Pawns(ply),set_mask[npfile+24]))
      faults+=KING_SAFETY_NP_ADV2;
    else if (!And(White_Pawns(ply),file_mask[npfile])) {
      faults+=KING_SAFETY_NP_MISSING;
      if (!And(Black_Pawns(ply),file_mask[npfile]))
        faults+=KING_SAFETY_NP_FILE_OPEN;
    }
    else
      faults+=KING_SAFETY_NP_TOO_FAR;
#ifdef DEBUGK
    printf("white.3, faults=%d\n",faults);
#endif
/*
 ------------------------------------------------
|                                                |
|   bishop pawn file.  note that this file is    |
|   not evaluated if the rp/np has not been      |
|   moved, since it's then hard to attack down   |
|   the bp file.                                 |
|                                                |
 ------------------------------------------------
*/
    if (faults > 1) {
      if (And(White_Pawns(ply),set_mask[bpfile+8]));
      else if (And(White_Pawns(ply),set_mask[bpfile+16]))
        faults+=KING_SAFETY_BP_ADV1;
      else if (And(White_Pawns(ply),set_mask[bpfile+24]))
        faults+=KING_SAFETY_BP_ADV2;
      else if (!And(White_Pawns(ply),file_mask[bpfile]))
        faults+=KING_SAFETY_BP_MISSING;
      else
        faults+=KING_SAFETY_BP_TOO_FAR;
#ifdef DEBUGK
      printf("white.4, faults=%d\n",faults);
#endif
    }
/*
 ------------------------------------------------
|                                                |
|   check for an enemy pawn on the b/g file that |
|   has been pushed to b3/g3 which produces some |
|   really dangerous back-rank problems.         |
|                                                |
 ------------------------------------------------
*/
    if (And(set_mask[npfile+40],White_Pawns(ply)))
      faults+=KING_SAFETY_PAWN_ATTACK;
#ifdef DEBUGK
    printf("white.6, faults=%d\n",faults);
#endif
/*
 ------------------------------------------------
|                                                |
|   penalize the king for advancing out of the   |
|   corner (ie, moving up the board or over      |
|   toward the center.  also penalize the king   |
|   for sitting in the corner squares (a1,h1,a8, |
|   h8) where it's easier to get mated.          |
|                                                |
 ------------------------------------------------
*/
    file=king_square&7;
    if ((file>1) && (file<6)) faults+=KING_SAFETY_NOT_CORNER;
    if ((file>2) && (file<5)) faults+=KING_SAFETY_NOT_CORNER;
    rank=king_square>>3;
    if (rank > 0) faults+=KING_SAFETY_NOT_CORNER;
    if (rank > 1) faults+=KING_SAFETY_NOT_CORNER;
    if (rank > 2) faults+=KING_SAFETY_NOT_CORNER;
    if (And(mask_corner_squares,White_King(ply)))
      faults+=KING_SAFETY_IN_CORNER;
#ifdef DEBUGK
    printf("white.7, faults=%d\n",faults);
#endif
  }
/*
 ----------------------------------------------------------
|                                                          |
|   store faults in the hash table so that it won't have   |
|   to be computed again.                                  |
|                                                          |
 ----------------------------------------------------------
*/
#if defined(DEBUG_HASH)
  if (hashed && (faults != t_faults))
    printf("hash error (king_w)!!  hash=%d  real=%d  entry=%d\n",
            t_faults, faults, (int) And(king_hash_key,king_hash_mask));
#endif
  if (king_hash_table) 
    *ktable=Or(Shiftl((BITBOARD) faults,56),Shiftr(king_hash_key,8));
  return(faults);
}

/*
********************************************************************************
*                                                                              *
*   Evaluate_Mate() is used to evaluate positions where neither side has pawns *
*   and one side has enough material to force checkmate.  it simply trys to    *
*   force the losing king to the edge of the board, and then to the corner     *
*   where mates are easier to find.                                            *
*                                                                              *
********************************************************************************
*/
int Evaluate_Mate(int ply)
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
  if ((Total_Black_Pieces(1)==0) && (Total_White_Pieces(1)==6) &&
      (!White_Pawns(1)) && (!Black_Pawns(1)) && (Popcnt(White_Bishops(1))==1)) {
    if (And(dark_squares,White_Bishops(1)))
      mate_score=b_n_mate_dark_squares[Black_King_SQ(ply)];
    else
      mate_score=b_n_mate_light_squares[Black_King_SQ(ply)];
  }
  if ((Total_Black_Pieces(1)==6) && (Total_White_Pieces(1)==0) &&
      (!White_Pawns(1)) && (!Black_Pawns(1)) && (Popcnt(Black_Bishops(1))==1)) {
    if (And(dark_squares,Black_Bishops(1)))
      mate_score=-b_n_mate_dark_squares[White_King_SQ(ply)];
    else
      mate_score=-b_n_mate_light_squares[White_King_SQ(ply)];
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
    if (Total_White_Pieces(ply) > Total_Black_Pieces(ply)+3) {
      mate_score=mate[Black_King_SQ(ply)];
    }
/*
 ----------------------------------------------------------
|                                                          |
|   if black is winning, force the white king to the edge  |
|   of the board.                                          |
|                                                          |
 ----------------------------------------------------------
*/
    if (Total_Black_Pieces(ply) > Total_White_Pieces(ply)+3) {
      mate_score=-mate[White_King_SQ(ply)];
    }
  }
  return(mate_score);
}

/*
********************************************************************************
*                                                                              *
*   Evaluate_Outside_Passed_Pawns() is used to evaluate outside passed pawns.  *
*   it analyzes the passed pawns for both sides, and returns a bonus if one    *
*   side has a passed pawn on the opposite side of the board from the rest of  *
*   the pawns.  the side with the most remote (outside) pawn gets a bonus that *
*   dynamically varies proportionally to the number of pieces on the board so  *
*   that it encourages the side with the outside passer to trade down to a     *
*   simple king and pawn ending which the outside passer wins.                 *
*                                                                              *
********************************************************************************
*/
int Evaluate_Outside_Passed_Pawns(int ply, int passed_b, int passed_w)
{
  register int score, w_file_l, w_file_r, b_file_l, b_file_r;
  register BITBOARD all_pawns;

  score=0;
  all_pawns=Or(Black_Pawns(ply),White_Pawns(ply));
  w_file_l=first_ones_8bit[passed_w];
  if (w_file_l == 8) w_file_l=-1;
  b_file_l=first_ones_8bit[passed_b];
  if (b_file_l == 8) b_file_l=-1;
  w_file_r=last_ones_8bit[passed_w];
  b_file_r=last_ones_8bit[passed_b];
/*
 ------------------------------------------------
|                                                |
|  if one side has a passed pawn that is closer  |
|  to the side of the board than any other pawn, |
|  then this pawn is "outside" and valuable.     |
|                                                |
 ------------------------------------------------
*/
  if (w_file_l != -1) {
    if ((w_file_l < (b_file_l-1)) || (w_file_r > (b_file_r+1))) {
      if (w_file_l < 4) {
        if(And(all_pawns,right_side_mask[w_file_l]) &&
           !And(Black_Pawns(ply),left_side_empty_mask[w_file_l]))
          score+=outside_passed[(int) Total_Black_Pieces(ply)];
      }
      if (w_file_r > 3) {
        if (And(all_pawns,left_side_mask[w_file_r]) &&
            !And(Black_Pawns(ply),right_side_empty_mask[w_file_r]))
          score+=outside_passed[(int) Total_Black_Pieces(ply)];
      }
    }
  }
  if (b_file_l != -1) {
    if ((b_file_l < (w_file_l-1)) || (b_file_r > (w_file_r+1))) {
      if (b_file_l < 4) {
        if(And(all_pawns,right_side_mask[b_file_l]) &&
           !And(White_Pawns(ply),left_side_empty_mask[b_file_l]))
          score-=outside_passed[(int) Total_White_Pieces(ply)];
      }
      if (b_file_r > 3) {
        if (And(all_pawns,left_side_mask[b_file_r]) &&
            !And(White_Pawns(ply),right_side_empty_mask[b_file_r]))
          score-=outside_passed[(int) Total_White_Pieces(ply)];
      }
    }
  }
  return(score);
}

/*
********************************************************************************
*                                                                              *
*   Evaluate_Passed_Pawns() is used to evaluate passed pawns and the danger    *
*   they produce.  currently, this code simply gives a bonus for an advanced   *
*   passed pawn that is supported by the king being in front of it to make it  *
*   very difficult to stop it from queening.                                   *
*                                                                              *
********************************************************************************
*/
int Evaluate_Passed_Pawns(int ply, int passed_b,int passed_w)
{
  register int file, square, score=0;
  register int white_king, black_king;

/*
 ----------------------------------------------------------
|                                                          |
|   check to see if black has any passed pawns.  if so,    |
|   and the king supports the pawn, then the pawn is even  |
|   more valuable.                                         |
|                                                          |
 ----------------------------------------------------------
*/
  if (passed_b) {
    black_king=Black_King_SQ(ply);
    for (file=0;file<8;file++) {
      if (passed_b & (128 >> file)) {
        square=First_One(And(Black_Pawns(ply),file_mask[file]));
        if (Distance(square,black_king) < 2)
          score-=supported_passer[7-(square>>3)];
      }
    }
  }
#ifdef DEBUGPP
  printf("score after black passers = %d\n", score);
#endif
/*
 ----------------------------------------------------------
|                                                          |
|   check to see if white has any passed pawns.  if so,    |
|   and the king supports the pawn, then the pawn is even  |
|   more valuable.                                         |
|                                                          |
 ----------------------------------------------------------
*/
  if (passed_w) {
    white_king=White_King_SQ(ply);
    for (file=0;file<8;file++) {
      if (passed_w & (128 >> file)) {
        square=Last_One(And(White_Pawns(ply),file_mask[file]));
        if (Distance(square,white_king) < 2) 
          score+=supported_passer[square>>3];
      }
    }
  }
#ifdef DEBUGPP
  printf("score after white passers = %d\n", score);
#endif
  return(score);
}

/*
********************************************************************************
*                                                                              *
*   Evaluate_Passed_Pawn_Races() is used to evalaute passed pawns when one     *
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
int Evaluate_Passed_Pawn_Races(int ply,int wtm,int passed_b,int passed_w)
{
  register int file, square;
  register int  white_queener, white_square, black_queener, black_square;
  register int white_pawn, black_pawn, queen_distance;
  register int  white_protected, black_protected;
  register int pawn;
  register BITBOARD tempw, tempb;

  white_queener=8;
  white_protected=0;
  black_queener=8;
  black_protected=0;
/*
 ----------------------------------------------------------
|                                                          |
|   check to see if white has one pawn and neither side    |
|   has any pieces.  if so, use the simple pawn evaluation |
|   logic.                                                 |
|                                                          |
 ----------------------------------------------------------
*/
  if (White_Pawns(ply) && (!Black_Pawns(ply)) &&
      (!Total_White_Pieces(ply)) && (!Total_Black_Pieces(ply))) do {
    pawn=First_One(White_Pawns(ply));
/*
 ------------------------------------------------
|                                                |
|   king must be in front of the pawn or we      |
|   go no further.                               |
|                                                |
 ------------------------------------------------
*/
    if ((White_King_SQ(ply)>>3) <= (pawn>>3)) break;
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
    if ((pawn&7) == 0) {
      if (((White_King_SQ(ply)&7) == 1) &&
          (Distance(White_King_SQ(ply),56) < Distance(Black_King_SQ(ply),56)))
        return(QUEEN_VALUE-2*PAWN_VALUE);
      break;
    }
    else if ((pawn&7) == 7) {
      if (((White_King_SQ(ply)&7) == 6) &&
          (Distance(White_King_SQ(ply),63) < Distance(Black_King_SQ(ply),63)))
        return(QUEEN_VALUE-2*PAWN_VALUE);
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
    if (Distance(White_King_SQ(ply),pawn)<=Distance(Black_King_SQ(ply),pawn)) {
      if ((White_King_SQ(ply)>>3) > ((pawn>>3)+1))
        return(QUEEN_VALUE-2*PAWN_VALUE);
      if ((White_King_SQ(ply)>>3) == 5)
        return(QUEEN_VALUE-2*PAWN_VALUE);
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
    if (((White_King_SQ(ply)>>3) == ((pawn>>3)+1)) &&
        Has_Opposition(wtm,White_King_SQ(ply),Black_King_SQ(ply)))
      return(QUEEN_VALUE-2*PAWN_VALUE);
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
  if (Black_Pawns(ply) && (!White_Pawns(ply)) &&
      (!Total_White_Pieces(ply)) && (!Total_Black_Pieces(ply))) do {
    pawn=First_One(Black_Pawns(ply));
/*
 ------------------------------------------------
|                                                |
|   king must be in front of the pawn or we      |
|   go no further.                               |
|                                                |
 ------------------------------------------------
*/
    if ((Black_King_SQ(ply)>>3) >= (pawn>>3)) break;
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
    if ((pawn&7) == 0) {
      if (((Black_King_SQ(ply)&7) == 1) &&
          (Distance(Black_King_SQ(ply),0) < Distance(White_King_SQ(ply),0)))
        return(-(QUEEN_VALUE-2*PAWN_VALUE));
      break;
    }
    else if ((pawn&7) == 7) {
      if (((White_King_SQ(ply)&7) == 6) &&
          (Distance(Black_King_SQ(ply),8) < Distance(White_King_SQ(ply),8)))
        return(-(QUEEN_VALUE-2*PAWN_VALUE));
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
    if (Distance(Black_King_SQ(ply),pawn)<=Distance(White_King_SQ(ply),pawn)) {
      if ((Black_King_SQ(ply)>>3) < ((pawn>>3)-1))
        return(-(QUEEN_VALUE-2*PAWN_VALUE));
      if ((Black_King_SQ(ply)>>3) == 2)
        return(-(QUEEN_VALUE-2*PAWN_VALUE));
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
    if (((Black_King_SQ(ply)>>3) == ((pawn>>3)-1)) &&
        Has_Opposition(!wtm,Black_King_SQ(ply),White_King_SQ(ply)))
      return(-(QUEEN_VALUE-2*PAWN_VALUE));
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
  if (!Total_White_Pieces(ply) && passed_b) {
    for (file=0;file<8;file++) {
      if (passed_b & (128 >> file)) {
        square=First_One(And(Black_Pawns(ply),file_mask[file]));
        if (And(w_pawn_attacks[square],Black_Pawns(ply))) black_protected=1;
        if ((wtm && !And(black_pawn_race_wtm[square],White_King(ply))) ||
            (!wtm && !And(black_pawn_race_btm[square],White_King(ply)))) {
          queen_distance=square>>3;
          if (And(Black_King(ply),mask_minus8dir[square])) queen_distance++;
          if ((square>>3) == 6) queen_distance--;
          if (queen_distance < black_queener) {
            black_queener=queen_distance;
            black_square=square&7;
            black_pawn=square;
          }
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
  if (!Total_Black_Pieces(ply) && passed_w) {
    for (file=0;file<8;file++) {
      if (passed_w & (128 >> file)) {
        square=Last_One(And(White_Pawns(ply),file_mask[file]));
        if (And(b_pawn_attacks[square],White_Pawns(ply))) white_protected=1;
        if ((wtm && !And(white_pawn_race_wtm[square],Black_King(ply))) ||
            (!wtm && !And(white_pawn_race_btm[square],Black_King(ply)))) {
          queen_distance=7-(square>>3);
          if (And(White_King(ply),mask_plus8dir[square])) queen_distance++;
          if ((square>>3) == 1) queen_distance--;
          if (queen_distance < white_queener) {
            white_queener=queen_distance;
            white_square=(square&7)+56;
            white_pawn=square;
          }
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
      return(QUEEN_VALUE-2*PAWN_VALUE+white_queener*PAWN_VALUE/10);
    else if ((black_queener < 8) && (white_queener == 8))
      return(-(QUEEN_VALUE-2*PAWN_VALUE+black_queener*PAWN_VALUE/10));
    if (!wtm) black_queener--;
    if (white_queener < black_queener)
      return(QUEEN_VALUE-2*PAWN_VALUE+white_queener*PAWN_VALUE/10);
    else if (black_queener < white_queener-1)
      return(-(QUEEN_VALUE-2*PAWN_VALUE+black_queener*PAWN_VALUE/10));
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
      tempw=White_Pieces(ply);
      Clear(white_pawn,White_Pieces(ply));
      White_Pieces(ply)=Or(White_Pieces(ply),set_mask[white_square]);
      tempb=Black_Pieces(ply);
      Clear(black_pawn,Black_Pieces(ply));
      Black_Pieces(ply)=Or(Black_Pieces(ply),set_mask[black_square]);
      if (Attack(Black_King_SQ(ply),white_square,ply)) {
        White_Pieces(ply)=tempw;
        Black_Pieces(ply)=tempb;
        return(QUEEN_VALUE-2*PAWN_VALUE+white_queener*PAWN_VALUE/10);
      }
      if (Attack(black_square,white_square,ply) &&
          !And(king_attacks[black_square],Black_King(ply))) {
        White_Pieces(ply)=tempw;
        Black_Pieces(ply)=tempb;
        return(QUEEN_VALUE-2*PAWN_VALUE+white_queener*PAWN_VALUE/10);
      }
      White_Pieces(ply)=tempw;
      Black_Pieces(ply)=tempb;
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
      tempw=White_Pieces(ply);
      Clear(white_pawn,White_Pieces(ply));
      White_Pieces(ply)=Or(White_Pieces(ply),set_mask[white_square]);
      tempb=Black_Pieces(ply);
      Clear(black_pawn,Black_Pieces(ply));
      Black_Pieces(ply)=Or(Black_Pieces(ply),set_mask[black_square]);
      if (Attack(White_King_SQ(ply),black_square,ply)) {
        White_Pieces(ply)=tempw;
        Black_Pieces(ply)=tempb;
        return(-(QUEEN_VALUE-2*PAWN_VALUE+black_queener*PAWN_VALUE/10));
      }
      if (Attack(white_square,black_square,ply) &&
          !And(king_attacks[white_square],White_King(ply))) {
        White_Pieces(ply)=tempw;
        Black_Pieces(ply)=tempb;
        return(-(QUEEN_VALUE-2*PAWN_VALUE+black_queener*PAWN_VALUE/10));
      }
      White_Pieces(ply)=tempw;
      Black_Pieces(ply)=tempb;
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

/*
********************************************************************************
*                                                                              *
*   Evaluate_Pawns() is used to evaluate pawns.  broadly, it addresses three   *
*   distinct actions:  (1) basic pawn scoring is hashed, the first thing done  *
*   is to see if the score is in the pawn hash table;  (2) passed pawn scoring *
*   for positions with pieces left;  (3) passed pawns where one side has a     *
*   passer and the other side has no pieces, so that the pawn can potentially  *
*   outrun the opposing king and promote.                                      *
*                                                                              *
********************************************************************************
*/
int Evaluate_Pawns(int ply, int *passed_b, int *passed_w)
{
  register BITBOARD *ptable, *ptable_x, temp_key;
  register int square, attackers, defenders, rams, file, score, pshift;
  register int half_entry;
  register int w_weak, b_weak;
#if defined(DEBUG_HASH)
  int t_weak_w=0,t_weak_b=0,t_passed_w=0,t_passed_b=0,t_score=0;
  int hashed=0;
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
  score=0;
  *passed_w=0;
  *passed_b=0;
  temp_key=Pawn_Hash_Key(ply);
  if (White_Rooks(ply)) temp_key=Xor(temp_key,w_rooks_random);
  if (Black_Rooks(ply)) temp_key=Xor(temp_key,b_rooks_random);
  ptable=pawn_hash_table+And(temp_key,pawn_hash_mask);
  ptable_x=pawn_hash_table_x+Shiftr(And(temp_key,pawn_hash_mask),1);
  pshift=And(temp_key,mask_127) << 5;
  if (pawn_hash_table && !Xor(And(*ptable,mask_80),Shiftr(temp_key,16))) {
    pawn_hashes++;
    score+=Shiftr(*ptable,48)-32768;
    half_entry=And(Shiftr(*ptable_x,pshift),mask_96);
#if !defined(DEBUG_HASH)
    *passed_b=(half_entry >> 16) & 255;
    *passed_w=(half_entry >> 24) & 255;
    return(score);
#else
    t_passed_b=(half_entry >> 16) & 255;
    t_passed_w=(half_entry >> 24) & 255;
    t_score=score;
    score=0;
    hashed=1;
#endif
  }
  for (file=0;file<8;file++) {
    w_weak=0;
    b_weak=0;
/*
 ----------------------------------------------------------
|                                                          |
|   evaluate pawn advances.  center pawns are encouraged   |
|   to advance, while wing pawns are pretty much neutral.  |
|                                                          |
 ----------------------------------------------------------
*/
    if (And(White_Pawns(ply),file_mask[file])) {
      square=Last_One(And(White_Pawns(ply),file_mask[file]));
      score+=pawn_value_w[square];
    }
    if (And(Black_Pawns(ply),file_mask[file])) {
      square=First_One(And(Black_Pawns(ply),file_mask[file]));
      score-=pawn_value_b[square];
    }
/*
 ----------------------------------------------------------
|                                                          |
|   evaluate isolated (weak) pawns.                        |
|                                                          |
 ----------------------------------------------------------
*/
    if (And(White_Pawns(ply),file_mask[file])) {
      square=Last_One(And(White_Pawns(ply),file_mask[file]));
      if (!And(mask_pawn_isolated[square],White_Pawns(ply))) {
        w_weak=1;
        score-=isolated[Popcnt(And(White_Pawns(ply),file_mask[file]))];
        if (!And(mask_plus8dir[square],Black_Pawns(ply)))
          if (Black_Rooks(ply)) score-=PAWN_ISOLATED_WEAK;
      }
    }
    if (And(Black_Pawns(ply),file_mask[file])) {
      square=First_One(And(Black_Pawns(ply),file_mask[file]));
      if (!And(mask_pawn_isolated[square],Black_Pawns(ply))) {
        b_weak=1;
        score+=isolated[Popcnt(And(Black_Pawns(ply),file_mask[file]))];
        if (!And(mask_minus8dir[square],White_Pawns(ply)))
          if (White_Rooks(ply)) score+=PAWN_ISOLATED_WEAK;
      }
    }
#ifdef DEBUGP
    printf("pawn[isolated] file=%d, score=%d\n",file,score);
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
|   on an open file.                                       |
|                                                          |
 ----------------------------------------------------------
*/
    if (!w_weak) {
      square=Last_One(And(White_Pawns(ply),file_mask[file]));
      if (square < 64) {
        if (!And(mask_pawn_backward_w[square],White_Pawns(ply))) {
          defenders=Popcnt(And(b_pawn_attacks[square+8],White_Pawns(ply)));
          attackers=Popcnt(And(w_pawn_attacks[square+8],Black_Pawns(ply)));
          if (attackers > defenders) {
            if (!And(mask_plus8dir[square],Black_Pawns(ply))) {
              if (Black_Rooks(ply))
                if (attackers-defenders == 1) score-=PAWN_VERY_WEAK_P1;
                else if (attackers-defenders == 2) score-=PAWN_VERY_WEAK_P2;
            }
            else {
              if (attackers-defenders == 1) score-=PAWN_WEAK_P1;
              else if (attackers-defenders == 2) score-=PAWN_WEAK_P2;
            }
          }
          else if (!defenders && (square < 16)) {
            defenders=Popcnt(And(b_pawn_attacks[square+16],White_Pawns(ply)));
            attackers=Popcnt(And(w_pawn_attacks[square+16],Black_Pawns(ply)));
            if (attackers > defenders) {
              if (!And(mask_plus8dir[square],Black_Pawns(ply))) {
                if (Black_Rooks(ply))
                  if (attackers-defenders == 1) score-=PAWN_VERY_WEAK_P1;
                  else if (attackers-defenders == 2) score-=PAWN_VERY_WEAK_P2;
              }
              else {
                if (attackers-defenders == 1) score-=PAWN_WEAK_P1;
                else if (attackers-defenders == 2) score-=PAWN_WEAK_P2;
              }
            }
          }
        }
      }
    }
    if (!b_weak) {
      square=First_One(And(Black_Pawns(ply),file_mask[file]));
      if (square < 64) {
        if (!And(mask_pawn_backward_b[square],Black_Pawns(ply))) {
          defenders=Popcnt(And(w_pawn_attacks[square-8],Black_Pawns(ply)));
          attackers=Popcnt(And(b_pawn_attacks[square-8],White_Pawns(ply)));
          if (attackers > defenders) {
            if (!And(mask_minus8dir[square],White_Pawns(ply))) {
              if (White_Rooks(ply))
                if (attackers-defenders == 1) score+=PAWN_VERY_WEAK_P1;
                else if (attackers-defenders == 2) score+=PAWN_VERY_WEAK_P2;
            }
            else {
              if (attackers-defenders == 1) score+=PAWN_WEAK_P1;
              else if (attackers-defenders == 2) score+=PAWN_WEAK_P2;
            }
          }
          else if (!defenders && (square > 47)) {
            defenders=Popcnt(And(w_pawn_attacks[square-16],Black_Pawns(ply)));
            attackers=Popcnt(And(b_pawn_attacks[square-16],White_Pawns(ply)));
            if (attackers > defenders) {
              if (!And(mask_minus8dir[square],White_Pawns(ply))) {
              if (attackers-defenders == 1)
                if (White_Rooks(ply))
                  if (attackers-defenders == 1) score+=PAWN_VERY_WEAK_P1;
                  else if (attackers-defenders == 2) score+=PAWN_VERY_WEAK_P2;
              }
              else {
                if (attackers-defenders == 1) score+=PAWN_WEAK_P1;
                else if (attackers-defenders == 2) score+=PAWN_WEAK_P2;
              }
            }
          }
        }
      }
    }
#ifdef DEBUGP
  printf("pawn[weak] file=%d,     score=%d\n",file,score);
#endif
  
/*
 ----------------------------------------------------------
|                                                          |
|   evaluate passed pawns.                                 |
|                                                          |
 ----------------------------------------------------------
*/
    if (And(White_Pawns(ply),file_mask[file])) {
      square=Last_One(And(White_Pawns(ply),file_mask[file]));
      if (!And(mask_pawn_passed_w[square],Black_Pawns(ply))) {
        if (And(mask_pawn_isolated[square],White_Pawns(ply))) {
          score+=PAWN_PASSED*(square>>3);
          if (And(mask_pawn_connected[square],White_Pawns(ply)))
            score+=connected_passer[square>>3];
        }
        else
          score+=PAWN_PASSED*(square>>3);
        *passed_w=*passed_w | (128 >> file);
      }
    }
    if (And(Black_Pawns(ply),file_mask[file])) {
      square=First_One(And(Black_Pawns(ply),file_mask[file]));
      if (!And(mask_pawn_passed_b[square],White_Pawns(ply))) {
        if (And(mask_pawn_isolated[square],Black_Pawns(ply))) {
          score-=PAWN_PASSED*(7-(square>>3));
          if (And(mask_pawn_connected[square],Black_Pawns(ply)))
            score-=connected_passer[7-(square>>3)];
        }
        else
          score-=PAWN_PASSED*(7-(square>>3));
        *passed_b=*passed_b | (128 >> file);
      }
    }
#ifdef DEBUGP
  printf("pawn[passed] file=%d,   score=%d\n",file,score);
#endif
  }
/*
 ----------------------------------------------------------
|                                                          |
|   next count the number of pawn "rams" on the board.     |
|   these are simply two pawns of opposite color on the    |
|   same file, contacting each other face-to-face.  such   |
|   positions are more blocked than tactical, which does   |
|   not take advantage of the program's tactical skills.   |
|                                                          |
 ----------------------------------------------------------
*/
/*
  if (!end_game) {
*/
    rams=Popcnt(And(Shiftr(White_Pawns(ply),8),Black_Pawns(ply)));
    if (root_wtm) score-=pawn_ram[rams];
    else score+=pawn_ram[rams];
#ifdef DEBUGP
    printf("pawn[rams] file=%d,     score=%d\n",file,score);
#endif
/*
  }
*/
  if (pawn_hash_table) {
    *ptable=Or(Shiftl((BITBOARD) score+32768,48),Shiftr(temp_key,16));
    half_entry=(*passed_b<<16)+(*passed_w<<24);
    if (pshift)
      *ptable_x=Or(And(*ptable_x,mask_96),Shiftl((BITBOARD) half_entry,32));
    else
      *ptable_x=Or(And(*ptable_x,mask_32),And(half_entry,mask_96));
  }
#if defined(DEBUG_HASH)
  if (hashed && ((t_passed_b != *passed_b) || (t_passed_w != *passed_w) ||
      (t_score != score))) {
    printf("hash error (pawn)!!  entry=%d\n", And(temp_key,pawn_hash_mask));
    printf("hash:  passed_b=%2x  passed_w=%2x", t_passed_b, t_passed_w);
    printf("  score=%d\n",t_score);
    printf("real:  passed_b=%2x  passed_w=%2x", *passed_b, *passed_w);
    printf("  score=%d\n",score);
    Display_Chess_Board(stdout,position[ply].board);
    exit(1);
  }
#endif
  return(score);
}

/*
********************************************************************************
*                                                                              *
*   Evaluate_Tempo() is used to discourage the program from moving pieces more *
*   than once until all minor pieces have been developed.  this tends to get   *
*   pieces off the back rank in a hurry since other piece moves produce a      *
*   penalty as long as pieces are still standing on the back rank.             *
*                                                                              *
********************************************************************************
*/
int Evaluate_Tempo(int ply)
{
  register int i, score;
/*
 ----------------------------------------------------------
|                                                          |
|   if the king hasn't castled, penalize the score for     |
|   two reasons:  (1) to encourage castling to get out of  |
|   the center of the board; (2) Evaluate_Development()    |
|   will return a non-zero score which will continue to    |
|   monitor developmental status until castling occurs.    |
|   if the king hasn't castled, penalize every move that   |
|   doesn't develop a piece (except for captures.)  this   |
|   gives Crafty a sense of "urgency" to get pieces out    |
|   into action, rather than probing around with queen (or |
|   other pieces) moves that waste tempi.                  |
|                                                          |
 ----------------------------------------------------------
*/
  score=0;
  if (root_wtm) {
    for (i=1;i<ply;i+=2) {
      if (((Piece(current_move[i]) > 1) && (From(current_move[i]) > 7) &&
           (!Captured(current_move[i]) ||
            (To(current_move[i]) == To(current_move[i-1])))))
        score+=DEVELOPMENT_WASTED_TEMPO;
      else if ((Piece(current_move[i]) == 1) && (From(current_move[i]) > 15) &&
               (!Captured(current_move[i])))
        score+=DEVELOPMENT_WASTED_TEMPO;
      else if ((Piece(current_move[i]) == 5) && (!Captured(current_move[i]) ||
                (To(current_move[i]) == To(current_move[i-1]))))
        score+=DEVELOPMENT_WASTED_TEMPO;
    }
  }
  else {
    for (i=1;i<ply;i+=2) {
      if (((Piece(current_move[i]) > 1) && (From(current_move[i]) < 56) &&
           (!Captured(current_move[i]) ||
            (To(current_move[i]) == To(current_move[i-1])))))
        score-=DEVELOPMENT_WASTED_TEMPO;
      else if ((Piece(current_move[i]) == 1) && (From(current_move[i]) < 48) &&
               (!Captured(current_move[i])))
        score-=DEVELOPMENT_WASTED_TEMPO;
      else if ((Piece(current_move[i]) == 5) && (!Captured(current_move[i]) ||
                (To(current_move[i]) == To(current_move[i-1]))))
        score-=DEVELOPMENT_WASTED_TEMPO;
    }
  }
  return(score);
}
 

/*
********************************************************************************
*                                                                              *
*   Evaluate_Trades() is used to encourage the program to trade pieces but not *
*   pawns when it's ahead in material, and to trade pawns but not pieces when  *
*   it's behind in material.  Currently the threshold to trigger this routine  *
*   is one side is at least two (2) pawns ahead (in material, not counting any *
*   positional score).                                                         *
*                                                                              *
********************************************************************************
*/
int Evaluate_Trades(int ply)
{
  register int trade_bonus=0, ahead=0, behind=0;

/*
 ----------------------------------------------------------
|                                                          |
|   first, determine if the program is ahead or behind by  |
|   more than one pawn.                                    |
|                                                          |
 ----------------------------------------------------------
*/
  if (root_wtm) {
    if (Material(ply) > PAWN_VALUE) ahead=1;
    else if (Material(ply) < -PAWN_VALUE) behind=1;
  }
  else {
    if (Material(ply) > PAWN_VALUE) behind=1;
    else if (Material(ply) < -PAWN_VALUE) ahead=1;
  }
/*
 ----------------------------------------------------------
|                                                          |
|   if the program is ahead in material, then every piece  |
|   trade should receive a bonus.                          |
|                                                          |
 ----------------------------------------------------------
*/
  if (ahead)
    if (root_wtm) {
      trade_bonus+=(Total_Black_Pieces(0)-Total_Black_Pieces(ply))*TRADE_PIECES;
    }
    else {
      trade_bonus+=(Total_White_Pieces(0)-Total_White_Pieces(ply))*TRADE_PIECES;
    }
/*
 ----------------------------------------------------------
|                                                          |
|   if the program is behind in material, then every piece |
|   trade should receive a penalty and every pawn trade    |
|   should receive a bonus.                                |
|                                                          |
 ----------------------------------------------------------
*/
  else if (behind) 
    if (root_wtm) {
      trade_bonus-=(Total_White_Pieces(0)-Total_White_Pieces(ply))*TRADE_PIECES;
      trade_bonus+=(Total_Black_Pawns(0)-Total_Black_Pawns(ply))*TRADE_PAWNS;
    }
    else {
      trade_bonus-=(Total_Black_Pieces(0)-Total_Black_Pieces(ply))*TRADE_PIECES;
      trade_bonus+=(Total_White_Pawns(0)-Total_White_Pawns(ply))*TRADE_PAWNS;
    }
  return(trade_bonus);
}
