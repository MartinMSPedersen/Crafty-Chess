#include <stdio.h>
#include <stdlib.h>
#include "chess.h"
#include "data.h"

/* modified 08/22/96 */
/*
********************************************************************************
*                                                                              *
*   GenerateCaptures() is used to generate capture and pawn promotion moves    *
*   from the current position.                                                 *
*                                                                              *
*   the destination square set is the set of squares occupied by opponent      *
*   pieces, plus the set of squares on the 8th rank that pawns can advance to  *
*   and promote.                                                               *
*                                                                              *
********************************************************************************
*/
int* GenerateCaptures(int ply, int wtm, int *move)
{
  register BITBOARD target, piecebd, moves;
  register BITBOARD  promotions, pcapturesl, pcapturesr;
  register int from, to, temp;

  if (wtm) {
/*
 ----------------------------------------------------------
|                                                          |
|   now, produce knight moves by cycling through the       |
|   *_knight board to locate a [from] square and then      |
|   cycling through knight_attacks[] to locate to squares  |
|   that a knight on [from] attacks.                       |
|                                                          |
 ----------------------------------------------------------
*/
    piecebd=WhiteKnights;
    while (piecebd) {
      from=LastOne(piecebd);
      moves=And(knight_attacks[from],BlackPieces);
      temp=from+(knight<<12);
      while (moves) {
        to=LastOne(moves);
        *move++=temp|(to<<6)|((-PieceOnSquare(to))<<15);
        Clear(to,moves);
      }
      Clear(from,piecebd);
    }
/*
 ----------------------------------------------------------
|                                                          |
|   now, produce bishop moves by cycling through the       |
|   *_bishop board to locate a [from] square and then      |
|   generate the AttacksFrom() bitmap which supplies the   |
|   list of valid <to> squares.                            |
|                                                          |
 ----------------------------------------------------------
*/
    piecebd=WhiteBishops;
    while (piecebd) {
      from=LastOne(piecebd);
      moves=And(AttacksBishop(from),BlackPieces);
      temp=from+(bishop<<12);
      while (moves) {
        to=LastOne(moves);
        *move++=temp|(to<<6)|((-PieceOnSquare(to))<<15);
        Clear(to,moves);
      }
      Clear(from,piecebd);
    }
/*
 ----------------------------------------------------------
|                                                          |
|   now, produce rook moves by cycling through the         |
|   *_rook board to locate a [from] square and then        |
|   generate the AttacksFrom() bitmap which supplies the   |
|   list of valid <to> squares.                            |
|                                                          |
 ----------------------------------------------------------
*/
    piecebd=WhiteRooks;
    while (piecebd) {
      from=LastOne(piecebd);
      moves=And(AttacksRook(from),BlackPieces);
      temp=from+(rook<<12);
      while (moves) {
        to=LastOne(moves);
        *move++=temp|(to<<6)|((-PieceOnSquare(to))<<15);
        Clear(to,moves);
      }
      Clear(from,piecebd);
    }
/*
 ----------------------------------------------------------
|                                                          |
|   now, produce queen moves by cycling through the        |
|   *_queen board to locate a [from] square and then       |
|   generate the AttacksFrom() bitmap which supplies the   |
|   list of valid <to> squares.                            |
|                                                          |
 ----------------------------------------------------------
*/
    piecebd=WhiteQueens;
    while (piecebd) {
      from=LastOne(piecebd);
      moves=And(AttacksQueen(from),BlackPieces);
      temp=from+(queen<<12);
      while (moves) {
        to=LastOne(moves);
        *move++=temp|(to<<6)|((-PieceOnSquare(to))<<15);
        Clear(to,moves);
      }
      Clear(from,piecebd);
    }
/*
 ----------------------------------------------------------
|                                                          |
|   now, produce king moves by cycling through the         |
|   *_king board to locate a [from] square and then        |
|   cycling through king_attacks[] to locate to squares    |
|   that a king on [from] attacks.                         |
|                                                          |
 ----------------------------------------------------------
*/
    from=WhiteKingSQ;
    moves=And(king_attacks[from],BlackPieces);
    temp=from+(king<<12);
    while (moves) {
      to=LastOne(moves);
      *move++=temp|(to<<6)|((-PieceOnSquare(to))<<15);
      Clear(to,moves);
    }
/*
 ----------------------------------------------------------
|                                                          |
|   now, produce pawn moves.  this is done differently due |
|   to inconsistencies in the way a pawn moves when it     |
|   captures as opposed to normal non-capturing moves.     |
|   another exception is capturing enpassant.  the first   |
|   step is to generate all possible pawn promotions.  we  |
|   do this by masking  all pawns but those on the 7th     |
|   rank and then advancing them ahead if the square in    |
|   front is empty.                                        |
|                                                          |
 ----------------------------------------------------------
*/
    promotions=And(Shiftr(And(WhitePawns,rank_mask[RANK7]),8),
                   Compl(Occupied));
/*
 ----------------------------------------------------------
|                                                          |
|   now that we got 'em, we simply enumerate the to        |
|   squares as before, but in three steps since we have    |
|   three sets of potential moves.                         |
|                                                          |
 ----------------------------------------------------------
*/
    while (promotions) {
      to=LastOne(promotions);
      if (to < 56) *move++=(to-8)|(to<<6)|(pawn<<12);
      else *move++=(to-8)|(to<<6)|(pawn<<12)|(queen<<18);
      Clear(to,promotions);
    }

    target=Or(BlackPieces,EnPassantTarget(ply));
    pcapturesl=And(Shiftr(And(WhitePawns,mask_left_edge),7),target);
    pcapturesr=And(Shiftr(And(WhitePawns,mask_right_edge),9),target);
    while (pcapturesl) {
      to=LastOne(pcapturesl);
      if (to < 56) {
        if(PieceOnSquare(to)) 
          *move++=(to-7)|(to<<6)|(pawn<<12)|((-PieceOnSquare(to))<<15);
        else
          *move++=(to-7)|(to<<6)|(pawn<<12)|(pawn<<15);
      }
      else 
        *move++=(to-7)|(to<<6)|(pawn<<12)|((-PieceOnSquare(to))<<15)|(queen<<18);
      Clear(to,pcapturesl);
    }
    while (pcapturesr) {
      to=LastOne(pcapturesr);
      if (to < 56) {
        if(PieceOnSquare(to)) 
          *move++=(to-9)|(to<<6)|(pawn<<12)|(-PieceOnSquare(to)<<15);
        else
          *move++=(to-9)|(to<<6)|(pawn<<12)|(pawn<<15);
      }
      else 
        *move++=(to-9)|(to<<6)|(pawn<<12)|((-PieceOnSquare(to))<<15)|(queen<<18);
      Clear(to,pcapturesr);
    }
  }
/*
 ----------------------------------------------------------
|                                                          |
|   now, produce knight moves by cycling through the       |
|   *_knight board to locate a [from] square and then      |
|   cycling through knight_attacks[] to locate to squares  |
|   that a knight on [from] attacks.                       |
|                                                          |
 ----------------------------------------------------------
*/
  else {
    piecebd=BlackKnights;
    while (piecebd) {
      from=FirstOne(piecebd);
      moves=And(knight_attacks[from],WhitePieces);
      temp=from+(knight<<12);
      while (moves) {
        to=FirstOne(moves);
        *move++=temp|(to<<6)|(PieceOnSquare(to)<<15);
        Clear(to,moves);
      }
      Clear(from,piecebd);
    }
/*
 ----------------------------------------------------------
|                                                          |
|   now, produce bishop moves by cycling through the       |
|   *_bishop board to locate a [from] square and then      |
|   generate the AttacksFrom() bitmap which supplies the   |
|   list of valid <to> squares.                            |
|                                                          |
 ----------------------------------------------------------
*/
    piecebd=BlackBishops;
    while (piecebd) {
      from=FirstOne(piecebd);
      moves=And(AttacksBishop(from),WhitePieces);
      temp=from+(bishop<<12);
      while (moves) {
        to=FirstOne(moves);
        *move++=temp|(to<<6)|(PieceOnSquare(to)<<15);
        Clear(to,moves);
      }
      Clear(from,piecebd);
    }
/*
 ----------------------------------------------------------
|                                                          |
|   now, produce rook moves by cycling through the         |
|   *_rook board to locate a [from] square and then        |
|   generate the AttacksFrom() bitmap which supplies the   |
|   list of valid <to> squares.                            |
|                                                          |
 ----------------------------------------------------------
*/
    piecebd=BlackRooks;
    while (piecebd) {
      from=FirstOne(piecebd);
      moves=And(AttacksRook(from),WhitePieces);
      temp=from+(rook<<12);
      while (moves) {
        to=FirstOne(moves);
        *move++=temp|(to<<6)|(PieceOnSquare(to)<<15);
        Clear(to,moves);
      }
      Clear(from,piecebd);
    }
/*
 ----------------------------------------------------------
|                                                          |
|   now, produce queen moves by cycling through the        |
|   *_queen board to locate a [from] square and then       |
|   generate the AttacksFrom() bitmap which supplies the   |
|   list of valid <to> squares.                            |
|                                                          |
 ----------------------------------------------------------
*/
    piecebd=BlackQueens;
    while (piecebd) {
      from=FirstOne(piecebd);
      moves=And(AttacksQueen(from),WhitePieces);
      temp=from+(queen<<12);
      while (moves) {
        to=FirstOne(moves);
        *move++=temp|(to<<6)|(PieceOnSquare(to)<<15);
        Clear(to,moves);
      }
      Clear(from,piecebd);
    }
/*
 ----------------------------------------------------------
|                                                          |
|   now, produce king moves by cycling through the         |
|   *_king board to locate a [from] square and then        |
|   cycling through king_attacks[] to locate to squares    |
|   that a king on [from] attacks.                         |
|                                                          |
 ----------------------------------------------------------
*/
    from=BlackKingSQ;
    moves=And(king_attacks[from],WhitePieces);
    temp=from+(king<<12);
    while (moves) {
      to=FirstOne(moves);
      *move++=temp|(to<<6)|(PieceOnSquare(to)<<15);
      Clear(to,moves);
    }
/*
 ----------------------------------------------------------
|                                                          |
|   now, produce pawn moves.  this is done differently due |
|   to inconsistencies in the way a pawn moves when it     |
|   captures as opposed to normal non-capturing moves.     |
|   another exception is capturing enpassant.  the first   |
|   step is to generate all possible pawn promotions.  we  |
|   do this by masking  all pawns but those on the 7th     |
|   rank and then advancing them ahead if the square in    |
|   front is empty.                                        |
|                                                          |
 ----------------------------------------------------------
*/
    promotions=And(Shiftl(And(BlackPawns,rank_mask[RANK2]),8),
                   Compl(Occupied));
/*
 ----------------------------------------------------------
|                                                          |
|   now that we got 'em, we simply enumerate the to        |
|   squares as before, but in three steps since we have    |
|   three sets of potential moves.                         |
|                                                          |
 ----------------------------------------------------------
*/
    while (promotions) {
      to=FirstOne(promotions);
      if (to > 7) *move++=(to+8)|(to<<6)|(pawn<<12);
      else *move++=(to+8)|(to<<6)|(pawn<<12)|(queen<<18);
      Clear(to,promotions);
    }

    target=Or(WhitePieces,EnPassantTarget(ply));
    pcapturesl=And(Shiftl(And(BlackPawns,mask_left_edge),9),target);
    pcapturesr=And(Shiftl(And(BlackPawns,mask_right_edge),7),target);
    while (pcapturesl) {
      to=FirstOne(pcapturesl);
      if (to > 7) {
        if(PieceOnSquare(to)) 
          *move++=(to+9)|(to<<6)|(pawn<<12)|(PieceOnSquare(to)<<15);
        else
          *move++=(to+9)|(to<<6)|(pawn<<12)|(pawn<<15);
      }
      else
        *move++=(to+9)|(to<<6)|(pawn<<12)|(PieceOnSquare(to)<<15)|(queen<<18);
      Clear(to,pcapturesl);
    }
    while (pcapturesr) {
      to=FirstOne(pcapturesr);
      if (to > 7) {
        if(PieceOnSquare(to)) 
          *move++=(to+7)|(to<<6)|(pawn<<12)|(PieceOnSquare(to)<<15);
        else
          *move++=(to+7)|(to<<6)|(pawn<<12)|(pawn<<15);
      }
      else
        *move++=(to+7)|(to<<6)|(pawn<<12)|(PieceOnSquare(to)<<15)|(queen<<18);
      Clear(to,pcapturesr);
    }
  }
  return(move);
}

/* modified 01/11/96 */
/*
********************************************************************************
*                                                                              *
*   GenerateCheckEvasions() is used to generate moves when the king is in      *
*   check.                                                                     *
*                                                                              *
*   three types of check-evasion moves are generated:                          *
*                                                                              *
*   (1) generate king moves to squares that are not attacked by the opponent's *
*   pieces.  this includes capture and non-capture moves.                      *
*                                                                              *
*   (2) generate interpositions along the rank/file that the checking attack   *
*   is coming along (assuming (a) only one piece is checking the king, and     *
*   (b) the checking piece is a sliding piece [bishop, rook, queen]).          *
*                                                                              *
*   (3) generate capture moves, but only to the square(s) that are giving      *
*   check.  captures are a special case.  if there is one checking piece, then *
*   capturing it by any piece is tried.  if there are two pieces checking the  *
*   king, then the only legal capture to try is for the king to capture one of *
*   the checking pieces that is on an un-attacked square.                      *
*                                                                              *
********************************************************************************
*/
int* GenerateCheckEvasions(int ply, int wtm, int *move)
{
  register BITBOARD target, targetc, targetp, piecebd, moves;
  register BITBOARD padvances1, padvances2, pcapturesl, pcapturesr;
  register BITBOARD padvances1_all, empty, checksqs;
  register int from, to, temp;
  register int king_square, checkers, checking_square;
  register int check_direction1=0, check_direction2=0;

/*
 ----------------------------------------------------------
|                                                          |
|   first, determine how many pieces are attacking the     |
|   king and where they are, so we can figure out how to   |
|   legally get out of check.                              |
|                                                          |
 ----------------------------------------------------------
*/
  if (wtm) {
    king_square=WhiteKingSQ;
    checksqs=And(AttacksTo(king_square),BlackPieces);
    checkers=PopCnt(checksqs);
    if (checkers == 1) {
      checking_square=FirstOne(And(AttacksTo(king_square),
                                    BlackPieces));
      if (PieceOnSquare(checking_square) != -pawn)
        check_direction1=directions[checking_square][king_square];
      target=InterposeSquares(check_direction1,king_square,checking_square);
      target=Or(target,And(AttacksTo(king_square),BlackPieces));
      target=Or(target,BlackKing);
    }
    else {
      target=BlackKing;
      checking_square=FirstOne(checksqs);
      if (PieceOnSquare(checking_square) != -pawn)
        check_direction1=directions[checking_square][king_square];
      Clear(checking_square,checksqs);
      checking_square=FirstOne(checksqs);
      if (PieceOnSquare(checking_square) != -pawn)
        check_direction2=directions[checking_square][king_square];
    }
/*
 ----------------------------------------------------------
|                                                          |
|   the next step is to produce the set of valid           |
|   destination squares.  for king moves, this is simply   |
|   the set of squares that are not attacked by enemy      |
|   pieces (if there are any such squares.)                |
|                                                          |
|   then, if the checking piece is not a knight, we need   |
|   to know the checking direction so that we can either   |
|   move the king "off" of that direction, or else "block" |
|   that direction.                                        |
|                                                          |
|   first produce king moves by cycling through the        |
|   *_king board to locate a [from] square and then        |
|   cycling through attacks_to[] to locate to squares      |
|   that the king on [from] attacks.                       |
|                                                          |
 ----------------------------------------------------------
*/
    from=king_square;
    moves=And(king_attacks[from],Compl(WhitePieces));
    temp=from+(king<<12);
    while (moves) {
      to=LastOne(moves);
      if (!Attacked(to,0) && (directions[from][to] != check_direction1) &&
                                 (directions[from][to] != check_direction2))
        *move++=temp|(to<<6)|((-PieceOnSquare(to))<<15);
      Clear(to,moves);
    }
/*
 ----------------------------------------------------------
|                                                          |
|   now, produce knight moves by cycling through the       |
|   *_knight board to locate a [from] square and then      |
|   cycling through knight_attacks[] to locate to squares  |
|   that a knight on [from] attacks.                       |
|                                                          |
 ----------------------------------------------------------
*/
    if (checkers == 1) {
      piecebd=WhiteKnights;
      while (piecebd) {
        from=LastOne(piecebd);
        if (!PinnedOnKing(wtm,from)) {
          moves=And(knight_attacks[from],target);
          temp=from+(knight<<12);
          while (moves) {
            to=LastOne(moves);
            *move++=temp|(to<<6)|((-PieceOnSquare(to))<<15);
            Clear(to,moves);
          }
        }
        Clear(from,piecebd);
      }
/*
 ----------------------------------------------------------
|                                                          |
|   now, produce bishop moves by cycling through the       |
|   *_bishop board to locate a [from] square and then      |
|   generate the AttacksFrom() bitmap which supplies the   |
|   list of valid <to> squares.                            |
|                                                          |
 ----------------------------------------------------------
*/
      piecebd=WhiteBishops;
      while (piecebd) {
        from=LastOne(piecebd);
        if (!PinnedOnKing(wtm,from)) {
          moves=And(AttacksBishop(from),target);
          temp=from+(bishop<<12);
          while (moves) {
            to=LastOne(moves);
            *move++=temp|(to<<6)|((-PieceOnSquare(to))<<15);
            Clear(to,moves);
          }
        }
        Clear(from,piecebd);
      }
/*
 ----------------------------------------------------------
|                                                          |
|   now, produce rook moves by cycling through the         |
|   *_rook board to locate a [from] square and then        |
|   generate the AttacksFrom() bitmap which supplies the   |
|   list of valid <to> squares.                            |
|                                                          |
 ----------------------------------------------------------
*/
      piecebd=WhiteRooks;
      while (piecebd) {
        from=LastOne(piecebd);
        if (!PinnedOnKing(wtm,from)) {
          moves=And(AttacksRook(from),target);
          temp=from+(rook<<12);
          while (moves) {
            to=LastOne(moves);
            *move++=temp|(to<<6)|((-PieceOnSquare(to))<<15);
            Clear(to,moves);
          }
        }
        Clear(from,piecebd);
      }
/*
 ----------------------------------------------------------
|                                                          |
|   now, produce queen moves by cycling through the        |
|   *_queen board to locate a [from] square and then       |
|   generate the AttacksFrom() bitmap which supplies the   |
|   list of valid <to> squares.                            |
|                                                          |
 ----------------------------------------------------------
*/
      piecebd=WhiteQueens;
      while (piecebd) {
        from=LastOne(piecebd);
        if (!PinnedOnKing(wtm,from)) {
          moves=And(AttacksQueen(from),target);
          temp=from+(queen<<12);
          while (moves) {
            to=LastOne(moves);
            *move++=temp|(to<<6)|((-PieceOnSquare(to))<<15);
            Clear(to,moves);
          }
        }
        Clear(from,piecebd);
      }
/*
 ----------------------------------------------------------
|                                                          |
|   now, produce pawn moves.  this is done differently due |
|   to inconsistencies in the way a pawn moves when it     |
|   captures as opposed to normal non-capturing moves.     |
|   another exception is capturing enpassant.  the first   |
|   step is to generate all possible pawn moves.  we do    |
|   this in 2 operations:  (1) shift the pawns forward one |
|   rank then and with empty squares;  (2) shift the pawns |
|   forward two ranks and then and with empty squares.     |
|                                                          |
 ----------------------------------------------------------
*/
      empty=Compl(Occupied);
      targetp=And(target,empty);
      padvances1=And(Shiftr(WhitePawns,8),targetp);
      padvances1_all=And(Shiftr(WhitePawns,8),empty);
      padvances2=And(Shiftr(And(padvances1_all,Shiftr(mask_8,16)),8),targetp);
/*
 ----------------------------------------------------------
|                                                          |
|   now that we got 'em, we simply enumerate the to        |
|   squares as before, but in four steps since we have     |
|   four sets of potential moves.                          |
|                                                          |
 ----------------------------------------------------------
*/
      while (padvances2) {
        to=LastOne(padvances2);
        if (!PinnedOnKing(wtm,to-16)) *move++=(to-16)|(to<<6)|(pawn<<12);
        Clear(to,padvances2);
      }
      while (padvances1) {
        to=LastOne(padvances1);
        if (!PinnedOnKing(wtm,to-8)) {
          if (to < 56) *move++=(to-8)|(to<<6)|(pawn<<12);
          else {
            *move++=(to-8)|(to<<6)|(pawn<<12)|(queen<<18);
            *move++=(to-8)|(to<<6)|(pawn<<12)|(rook<<18);
            *move++=(to-8)|(to<<6)|(pawn<<12)|(bishop<<18);
            *move++=(to-8)|(to<<6)|(pawn<<12)|(knight<<18);
          }
        }
        Clear(to,padvances1);
      }

      targetc=Or(BlackPieces,EnPassantTarget(ply));
      targetc=And(targetc,target);
      if (And(And(BlackPawns,target),Shiftl(EnPassantTarget(ply),8)))
        targetc=Or(targetc,EnPassantTarget(ply));
      pcapturesl=And(Shiftr(And(WhitePawns,mask_left_edge),7),targetc);
      pcapturesr=And(Shiftr(And(WhitePawns,mask_right_edge),9),targetc);
      while (pcapturesl) {
        to=LastOne(pcapturesl);
        if (!PinnedOnKing(wtm,to-7)) {
          if (to < 56) {
            if(PieceOnSquare(to)) 
              *move++=(to-7)|(to<<6)|(pawn<<12)|((-PieceOnSquare(to))<<15);
            else
              *move++=(to-7)|(to<<6)|(pawn<<12)|(pawn<<15);
          }
          else {
            *move++=(to-7)|(to<<6)|(pawn<<12)|((-PieceOnSquare(to))<<15)|(queen<<18);
            *move++=(to-7)|(to<<6)|(pawn<<12)|((-PieceOnSquare(to))<<15)|(rook<<18);
            *move++=(to-7)|(to<<6)|(pawn<<12)|((-PieceOnSquare(to))<<15)|(bishop<<18);
            *move++=(to-7)|(to<<6)|(pawn<<12)|((-PieceOnSquare(to))<<15)|(knight<<18);
          }
        }
        Clear(to,pcapturesl);
      }
      while (pcapturesr) {
        to=LastOne(pcapturesr);
        if (!PinnedOnKing(wtm,to-9)) {
          if (to < 56) {
            if(PieceOnSquare(to)) 
              *move++=(to-9)|(to<<6)|(pawn<<12)|((-PieceOnSquare(to))<<15);
            else
              *move++=(to-9)|(to<<6)|(pawn<<12)|(pawn<<15);
          }
          else {
            *move++=(to-9)|(to<<6)|(pawn<<12)|((-PieceOnSquare(to))<<15)|(queen<<18);
            *move++=(to-9)|(to<<6)|(pawn<<12)|((-PieceOnSquare(to))<<15)|(rook<<18);
            *move++=(to-9)|(to<<6)|(pawn<<12)|((-PieceOnSquare(to))<<15)|(bishop<<18);
            *move++=(to-9)|(to<<6)|(pawn<<12)|((-PieceOnSquare(to))<<15)|(knight<<18);
          }
        }
        Clear(to,pcapturesr);
      }
    }
  }
/*
 ----------------------------------------------------------
|                                                          |
|   first, determine how many pieces are attacking the     |
|   king and where they are, so we can figure out how to   |
|   legally get out of check.                              |
|                                                          |
 ----------------------------------------------------------
*/
  else {
    king_square=BlackKingSQ;
    checksqs=And(AttacksTo(king_square),WhitePieces);
    checkers=PopCnt(checksqs);
    if (checkers == 1) {
      checking_square=FirstOne(And(AttacksTo(king_square),
                                    WhitePieces));
      if (PieceOnSquare(checking_square) != pawn)
        check_direction1=directions[checking_square][king_square];
      target=InterposeSquares(check_direction1,king_square,checking_square);
      target=Or(target,And(AttacksTo(king_square),WhitePieces));
      target=Or(target,WhiteKing);
    }
    else {
      target=WhiteKing;
      checking_square=FirstOne(checksqs);
      if (PieceOnSquare(checking_square) != pawn)
        check_direction1=directions[checking_square][king_square];
      Clear(checking_square,checksqs);
      checking_square=FirstOne(checksqs);
      if (PieceOnSquare(checking_square) != pawn)
        check_direction2=directions[checking_square][king_square];
    }
/*
 ----------------------------------------------------------
|                                                          |
|   the first step is to produce the set of valid          |
|   destination squares.  for king moves, this is simply   |
|   the set of squares that are not attacked by enemy      |
|   pieces (if there are any such squares.)                |
|                                                          |
|   then, if the checking piece is not a knight, we need   |
|   to know the checking direction so that we can either   |
|   move the king "off" of that direction, or else "block" |
|   that direction.                                        |
|                                                          |
|   first produce king moves by cycling through the        |
|   *_king board to locate a [from] square and then        |
|   cycling through attacks_to[] to locate to squares      |
|   that the king on [from] attacks.                       |
|                                                          |
 ----------------------------------------------------------
*/
    from=king_square;
    moves=And(king_attacks[from],Compl(BlackPieces));
    temp=from+(king<<12);
    while (moves) {
      to=FirstOne(moves);
      if (!Attacked(to,1) && (directions[from][to] != check_direction1) &&
                                 (directions[from][to] != check_direction2))
        *move++=temp|(to<<6)|(PieceOnSquare(to)<<15);
      Clear(to,moves);
    }
/*
 ----------------------------------------------------------
|                                                          |
|   now, produce knight moves by cycling through the       |
|   *_knight board to locate a [from] square and then      |
|   cycling through knight_attacks[] to locate to squares  |
|   that a knight on [from] attacks.                       |
|                                                          |
 ----------------------------------------------------------
*/
    if (checkers == 1) {
      piecebd=BlackKnights;
      while (piecebd) {
        from=FirstOne(piecebd);
        if (!PinnedOnKing(wtm,from)) {
          moves=And(knight_attacks[from],target);
          temp=from+(knight<<12);
          while (moves) {
            to=FirstOne(moves);
            *move++=temp|(to<<6)|(PieceOnSquare(to)<<15);
            Clear(to,moves);
          }
        }
        Clear(from,piecebd);
      }
/*
 ----------------------------------------------------------
|                                                          |
|   now, produce bishop moves by cycling through the       |
|   *_bishop board to locate a [from] square and then      |
|   generate the AttacksFrom() bitmap which supplies the   |
|   list of valid <to> squares.                            |
|                                                          |
 ----------------------------------------------------------
*/
      piecebd=BlackBishops;
      while (piecebd) {
        from=FirstOne(piecebd);
        if (!PinnedOnKing(wtm,from)) {
          moves=And(AttacksBishop(from),target);
          temp=from+(bishop<<12);
          while (moves) {
            to=FirstOne(moves);
            *move++=temp|(to<<6)|(PieceOnSquare(to)<<15);
            Clear(to,moves);
          }
        }
        Clear(from,piecebd);
      }
/*
 ----------------------------------------------------------
|                                                          |
|   now, produce rook moves by cycling through the         |
|   *_rook board to locate a [from] square and then        |
|   generate the AttacksFrom() bitmap which supplies the   |
|   list of valid <to> squares.                            |
|                                                          |
 ----------------------------------------------------------
*/
      piecebd=BlackRooks;
      while (piecebd) {
        from=FirstOne(piecebd);
        if (!PinnedOnKing(wtm,from)) {
          moves=And(AttacksRook(from),target);
          temp=from+(rook<<12);
          while (moves) {
            to=FirstOne(moves);
            *move++=temp|(to<<6)|(PieceOnSquare(to)<<15);
            Clear(to,moves);
          }
        }
        Clear(from,piecebd);
      }
/*
 ----------------------------------------------------------
|                                                          |
|   now, produce queen moves by cycling through the        |
|   *_queen board to locate a [from] square and then       |
|                                                          |
 ----------------------------------------------------------
*/
      piecebd=BlackQueens;
      while (piecebd) {
        from=FirstOne(piecebd);
        if (!PinnedOnKing(wtm,from)) {
          moves=And(AttacksQueen(from),target);
          temp=from+(queen<<12);
          while (moves) {
            to=FirstOne(moves);
            *move++=temp|(to<<6)|(PieceOnSquare(to)<<15);
            Clear(to,moves);
          }
        }
        Clear(from,piecebd);
      }
/*
 ----------------------------------------------------------
|                                                          |
|   now, produce pawn moves.  this is done differently due |
|   to inconsistencies in the way a pawn moves when it     |
|   captures as opposed to normal non-capturing moves.     |
|   another exception is capturing enpassant.  the first   |
|   step is to generate all possible pawn moves.  we do    |
|   this in 4 operations:  (1) shift the pawns forward one |
|   rank then and with empty squares;  (2) shift the pawns |
|   forward two ranks and then and with empty squares;     |
|   (3) remove the a-pawn(s) then shift the pawns          |
|   diagonally left then and with enemy occupied squares;  |
|   (4) remove the h-pawn(s) then shift the pawns          |
|   diagonally right then and with enemy occupied squares; |
|   note that enemy occupied squares includes the special  |
|   case of the enpassant target square also.              |
|                                                          |
 ----------------------------------------------------------
*/
      empty=Compl(Occupied);
      targetp=And(target,empty);
      padvances1=And(Shiftl(BlackPawns,8),targetp);
      padvances1_all=And(Shiftl(BlackPawns,8),empty);
      padvances2=And(Shiftl(And(padvances1_all,Shiftl(mask_120,16)),8),targetp);
/*
 ----------------------------------------------------------
|                                                          |
|   now that we got 'em, we simply enumerate the to        |
|   squares as before, but in four steps since we have     |
|   four sets of potential moves.                          |
|                                                          |
 ----------------------------------------------------------
*/
      while (padvances2) {
        to=FirstOne(padvances2);
        if (!PinnedOnKing(wtm,to+16)) *move++=(to+16)|(to<<6)|(pawn<<12);
        Clear(to,padvances2);
      }
      while (padvances1) {
        to=FirstOne(padvances1);
        if (!PinnedOnKing(wtm,to+8)) {
          if (to > 7) *move++=(to+8)|(to<<6)|(pawn<<12);
          else {
            *move++=(to+8)|(to<<6)|(pawn<<12)|(queen<<18);
            *move++=(to+8)|(to<<6)|(pawn<<12)|(rook<<18);
            *move++=(to+8)|(to<<6)|(pawn<<12)|(bishop<<18);
            *move++=(to+8)|(to<<6)|(pawn<<12)|(knight<<18);
          }
        }
        Clear(to,padvances1);
      }

      targetc=Or(WhitePieces,EnPassantTarget(ply));
      targetc=And(targetc,target);
      if (And(And(WhitePawns,target),Shiftr(EnPassantTarget(ply),8)))
        targetc=Or(targetc,EnPassantTarget(ply));
      pcapturesl=And(Shiftl(And(BlackPawns,mask_left_edge),9),targetc);
      pcapturesr=And(Shiftl(And(BlackPawns,mask_right_edge),7),targetc);
      while (pcapturesl) {
        to=FirstOne(pcapturesl);
        if (!PinnedOnKing(wtm,to+9)) {
          if (to > 7) {
            if(PieceOnSquare(to)) 
              *move++=(to+9)|(to<<6)|(pawn<<12)|(PieceOnSquare(to)<<15);
            else
              *move++=(to+9)|(to<<6)|(pawn<<12)|(pawn<<15);
          }
          else {
            *move++=(to+9)|(to<<6)|(pawn<<12)|(PieceOnSquare(to)<<15)|(queen<<18);
            *move++=(to+9)|(to<<6)|(pawn<<12)|(PieceOnSquare(to)<<15)|(rook<<18);
            *move++=(to+9)|(to<<6)|(pawn<<12)|(PieceOnSquare(to)<<15)|(bishop<<18);
            *move++=(to+9)|(to<<6)|(pawn<<12)|(PieceOnSquare(to)<<15)|(knight<<18);
          }
        }
        Clear(to,pcapturesl);
      }
      while (pcapturesr) {
        to=FirstOne(pcapturesr);
        if (!PinnedOnKing(wtm,to+7)) {
          if (to > 7) {
            if(PieceOnSquare(to)) 
              *move++=(to+7)|(to<<6)|(pawn<<12)|(PieceOnSquare(to)<<15);
            else
              *move++=(to+7)|(to<<6)|(pawn<<12)|(pawn<<15);
          }
          else {
            *move++=(to+7)|(to<<6)|(pawn<<12)|(PieceOnSquare(to)<<15)|(queen<<18);
            *move++=(to+7)|(to<<6)|(pawn<<12)|(PieceOnSquare(to)<<15)|(rook<<18);
            *move++=(to+7)|(to<<6)|(pawn<<12)|(PieceOnSquare(to)<<15)|(bishop<<18);
            *move++=(to+7)|(to<<6)|(pawn<<12)|(PieceOnSquare(to)<<15)|(knight<<18);
          }
        }
        Clear(to,pcapturesr);
      }
    }
  }
  return(move);
}

/* modified 11/12/96 */
/*
********************************************************************************
*                                                                              *
*   GenerateMoves() is used to generate moves to a set of squares from the     *
*   current position.  this set of squares is passed through "target" and is   *
*   most often either (a) the complement of the current side's piece locations *
*   (which generates all legal moves) or (b) the opposing side's piece         *
*   locations (which generates captures).                                      *
*                                                                              *
*   once the valid destination squares are known, we have to locate a friendly *
*   piece to get a attacks_to[] entry.  we then produce the moves for this     *
*   piece by using the source square as [from] and enumerating each square it  *
*   attacks into [to].                                                         *
*                                                                              *
*   pawns are handled differently.  regular pawn moves are produced by         *
*   shifting the pawn bitmap 8 bits "forward" and anding this with the         *
*   complement of the occupied squares bitmap  double advances are then        *
*   produced by anding the pawn bitmap with a mask containing 1's on the       *
*   second rank, shifting this 16 bits "forward" and then anding this with     *
*   the complement of the occupied squares bitmap as before.  if [to]          *
*   reaches the 8th rank, we produce a set of four moves, promoting the pawn   *
*   to knight, bishop, rook and queen.                                         *
*                                                                              *
*   the paramater "generate_captures" is somewhat confusing, but is used to    *
*   clear up two special cases caused by pawns.  (1) en passant captures are   *
*   tricky since the pawn is capturing on a square that is not really occupied *
*   and including the EnPassant_Target in "target" would erroneously cause     *
*   GenerateMoves() to produce piece moves to this "wrong" square.  (2)        *
*   promotions are another special case.  since a promotion can be thought of  *
*   as a special case of captures (since the move does gain material..) the    *
*   quiescence search needs these moves included.  generate_captures forces    *
*   their inclusion.                                                           *
*                                                                              *
********************************************************************************
*/
int* GenerateMoves(int ply, int depth, int wtm, BITBOARD target, 
                              int generate_captures, int *move)
{
  register BITBOARD targets, targetc, temp_target , piecebd, moves;
  register BITBOARD  padvances1, padvances2, pcapturesl, pcapturesr;
  register int from, to, temp;
  if (wtm) {
/*
 ----------------------------------------------------------
|                                                          |
|   first, produce castling moves if it is legal.          |
|                                                          |
 ----------------------------------------------------------
*/
    if (WhiteCastle(ply) > 0) {
      if ((WhiteCastle(ply)&1) && And(set_mask[G1],target) &&
          !And(Occupied,Shiftr(mask_2,5)) &&
          !Attacked(E1,0) && !Attacked(F1,0) && !Attacked(G1,0)) {
        *move++=12676;
      }
      if ((WhiteCastle(ply)&2) && And(set_mask[C1],target) &&
          !And(Occupied,Shiftr(mask_3,1)) &&
          !Attacked(C1,0) && !Attacked(D1,0) && !Attacked(E1,0)) {
        *move++=12420;
      }
    }
/*
 ----------------------------------------------------------
|                                                          |
|   now, produce knight moves by cycling through the       |
|   *_knight board to locate a [from] square and then      |
|   cycling through knight_attacks[] to locate to squares  |
|   that a knight on [from] attacks.                       |
|                                                          |
 ----------------------------------------------------------
*/
    piecebd=WhiteKnights;
    while (piecebd) {
      from=LastOne(piecebd);
      moves=And(knight_attacks[from],target);
      temp=from+(knight<<12);
      while (moves) {
        to=LastOne(moves);
        *move++=temp|(to<<6)|((-PieceOnSquare(to))<<15);
        Clear(to,moves);
      }
      Clear(from,piecebd);
    }
/*
 ----------------------------------------------------------
|                                                          |
|   now, produce bishop moves by cycling through the       |
|   *_bishop board to locate a [from] square and then      |
|   generate the AttacksFrom() bitmap which supplies the   |
|   list of valid <to> squares.                            |
|                                                          |
 ----------------------------------------------------------
*/
    piecebd=WhiteBishops;
    while (piecebd) {
      from=LastOne(piecebd);
      moves=And(AttacksBishop(from),target);
      temp=from+(bishop<<12);
      while (moves) {
        to=LastOne(moves);
        *move++=temp|(to<<6)|((-PieceOnSquare(to))<<15);
        Clear(to,moves);
      }
      Clear(from,piecebd);
    }
/*
 ----------------------------------------------------------
|                                                          |
|   now, produce rook moves by cycling through the         |
|   *_rook board to locate a [from] square and then        |
|   generate the AttacksFrom() bitmap which supplies the   |
|   list of valid <to> squares.                            |
|                                                          |
 ----------------------------------------------------------
*/
    piecebd=WhiteRooks;
    while (piecebd) {
      from=LastOne(piecebd);
      moves=And(AttacksRook(from),target);
      temp=from+(rook<<12);
      while (moves) {
        to=LastOne(moves);
        *move++=temp|(to<<6)|((-PieceOnSquare(to))<<15);
        Clear(to,moves);
      }
      Clear(from,piecebd);
    }
/*
 ----------------------------------------------------------
|                                                          |
|   now, produce queen moves by cycling through the        |
|   *_queen board to locate a [from] square and then       |
|   generate the AttacksFrom() bitmap which supplies the   |
|   list of valid <to> squares.                            |
|                                                          |
 ----------------------------------------------------------
*/
    piecebd=WhiteQueens;
    while (piecebd) {
      from=LastOne(piecebd);
      moves=And(AttacksQueen(from),target);
      temp=from+(queen<<12);
      while (moves) {
        to=LastOne(moves);
        *move++=temp|(to<<6)|((-PieceOnSquare(to))<<15);
        Clear(to,moves);
      }
      Clear(from,piecebd);
    }
/*
 ----------------------------------------------------------
|                                                          |
|   now, produce king moves by cycling through the         |
|   *_king board to locate a [from] square and then        |
|   cycling through king_attacks[] to locate to squares    |
|   that a king on [from] attacks.                         |
|                                                          |
 ----------------------------------------------------------
*/
    from=WhiteKingSQ;
    moves=And(king_attacks[from],target);
    temp=from+(king<<12);
    while (moves) {
      to=LastOne(moves);
      *move++=temp|(to<<6)|((-PieceOnSquare(to))<<15);
      Clear(to,moves);
    }
/*
 ----------------------------------------------------------
|                                                          |
|   now, produce pawn moves.  this is done differently due |
|   to inconsistencies in the way a pawn moves when it     |
|   captures as opposed to normal non-capturing moves.     |
|   another exception is capturing enpassant.  the first   |
|   step is to generate all possible pawn moves.  we do    |
|   this in 2 operations:  (1) shift the pawns forward one |
|   rank then and with empty squares;  (2) shift the pawns |
|   forward two ranks and then and with empty squares.     |
|                                                          |
 ----------------------------------------------------------
*/
    temp_target=Compl(Occupied);
    targets=And(temp_target,target);
    if(generate_captures) targets=Or(targets,And(rank_mask[RANK8],temp_target));
    else targets=And(targets,mask_not_rank8);
    padvances1=And(Shiftr(WhitePawns,8),targets);
    padvances2=And(Shiftr(And(padvances1,mask_advance_2_w),8),targets);
/*
 ----------------------------------------------------------
|                                                          |
|   now that we got 'em, we simply enumerate the to        |
|   squares as before, but in four steps since we have     |
|   four sets of potential moves.                          |
|                                                          |
 ----------------------------------------------------------
*/
    while (padvances2) {
      to=LastOne(padvances2);
      *move++=(to-16)|(to<<6)|(pawn<<12);
      Clear(to,padvances2);
    }
    while (padvances1) {
      to=LastOne(padvances1);
      if (to < 56) *move++=(to-8)|(to<<6)|(pawn<<12);
      else {
        *move++=(to-8)|(to<<6)|(pawn<<12)|(queen<<18);
        if (depth > 0) {
          *move++=(to-8)|(to<<6)|(pawn<<12)|(rook<<18);
          *move++=(to-8)|(to<<6)|(pawn<<12)|(bishop<<18);
          *move++=(to-8)|(to<<6)|(pawn<<12)|(knight<<18);
        }
      }
      Clear(to,padvances1);
    }
    if (generate_captures) {
      targetc=And(BlackPieces,target);
      targetc=Or(targetc,EnPassantTarget(ply));
      pcapturesl=And(Shiftr(And(WhitePawns,mask_left_edge),7),targetc);
      pcapturesr=And(Shiftr(And(WhitePawns,mask_right_edge),9),targetc);
      while (pcapturesl) {
        to=LastOne(pcapturesl);
        if (to < 56) {
          if(PieceOnSquare(to)) 
            *move++=(to-7)|(to<<6)|(pawn<<12)|((-PieceOnSquare(to))<<15);
          else
            *move++=(to-7)|(to<<6)|(pawn<<12)|(pawn<<15);
        }
        else {
          *move++=(to-7)|(to<<6)|(pawn<<12)|((-PieceOnSquare(to))<<15)|(queen<<18);
          if (depth > 0) {
            *move++=(to-7)|(to<<6)|(pawn<<12)|((-PieceOnSquare(to))<<15)|(rook<<18);
            *move++=(to-7)|(to<<6)|(pawn<<12)|((-PieceOnSquare(to))<<15)|(bishop<<18);
            *move++=(to-7)|(to<<6)|(pawn<<12)|((-PieceOnSquare(to))<<15)|(knight<<18);
          }
        }
        Clear(to,pcapturesl);
      }
      while (pcapturesr) {
        to=LastOne(pcapturesr);
        if (to < 56) {
          if(PieceOnSquare(to)) 
            *move++=(to-9)|(to<<6)|(pawn<<12)|(-PieceOnSquare(to)<<15);
          else
            *move++=(to-9)|(to<<6)|(pawn<<12)|(pawn<<15);
        }
        else {
          *move++=(to-9)|(to<<6)|(pawn<<12)|((-PieceOnSquare(to))<<15)|(queen<<18);
          if (depth > 0) {
            *move++=(to-9)|(to<<6)|(pawn<<12)|((-PieceOnSquare(to))<<15)|(rook<<18);
            *move++=(to-9)|(to<<6)|(pawn<<12)|((-PieceOnSquare(to))<<15)|(bishop<<18);
            *move++=(to-9)|(to<<6)|(pawn<<12)|((-PieceOnSquare(to))<<15)|(knight<<18);
          }
        }
        Clear(to,pcapturesr);
      }
    }
  }
/*
 ----------------------------------------------------------
|                                                          |
|   first, produce castling moves if it is legal.          |
|                                                          |
 ----------------------------------------------------------
*/
  else {
    if (BlackCastle(ply) > 0) {
      if ((BlackCastle(ply)&1)  && And(set_mask[G8],target) &&
          !And(Occupied,Shiftr(mask_2,61)) &&
          !Attacked(E8,1) && !Attacked(F8,1) && !Attacked(G8,1)) {
        *move++=16316;
      }
      if ((BlackCastle(ply)&2) && And(set_mask[C8],target) &&
          !And(Occupied,Shiftr(mask_3,57)) &&
          !Attacked(C8,1) && !Attacked(D8,1) && !Attacked(E8,1)) {
        *move++=16060;
      }
    }
/*
 ----------------------------------------------------------
|                                                          |
|   now, produce knight moves by cycling through the       |
|   *_knight board to locate a [from] square and then      |
|   cycling through knight_attacks[] to locate to squares  |
|   that a knight on [from] attacks.                       |
|                                                          |
 ----------------------------------------------------------
*/
    piecebd=BlackKnights;
    while (piecebd) {
      from=FirstOne(piecebd);
      moves=And(knight_attacks[from],target);
      temp=from+(knight<<12);
      while (moves) {
        to=FirstOne(moves);
        *move++=temp|(to<<6)|(PieceOnSquare(to)<<15);
        Clear(to,moves);
      }
      Clear(from,piecebd);
    }
/*
 ----------------------------------------------------------
|                                                          |
|   now, produce bishop moves by cycling through the       |
|   *_bishop board to locate a [from] square and then      |
|   generate the AttacksFrom() bitmap which supplies the   |
|   list of valid <to> squares.                            |
|                                                          |
 ----------------------------------------------------------
*/
    piecebd=BlackBishops;
    while (piecebd) {
      from=FirstOne(piecebd);
      moves=And(AttacksBishop(from),target);
      temp=from+(bishop<<12);
      while (moves) {
        to=FirstOne(moves);
        *move++=temp|(to<<6)|(PieceOnSquare(to)<<15);
        Clear(to,moves);
      }
      Clear(from,piecebd);
    }
/*
 ----------------------------------------------------------
|                                                          |
|   now, produce rook moves by cycling through the         |
|   *_rook board to locate a [from] square and then        |
|   generate the AttacksFrom() bitmap which supplies the   |
|   list of valid <to> squares.                            |
|                                                          |
 ----------------------------------------------------------
*/
    piecebd=BlackRooks;
    while (piecebd) {
      from=FirstOne(piecebd);
      moves=And(AttacksRook(from),target);
      temp=from+(rook<<12);
      while (moves) {
        to=FirstOne(moves);
        *move++=temp|(to<<6)|(PieceOnSquare(to)<<15);
        Clear(to,moves);
      }
      Clear(from,piecebd);
    }
/*
 ----------------------------------------------------------
|                                                          |
|   now, produce queen moves by cycling through the        |
|   *_queen board to locate a [from] square and then       |
|   generate the AttacksFrom() bitmap which supplies the   |
|   list of valid <to> squares.                            |
|                                                          |
 ----------------------------------------------------------
*/
    piecebd=BlackQueens;
    while (piecebd) {
      from=FirstOne(piecebd);
      moves=And(AttacksQueen(from),target);
      temp=from+(queen<<12);
      while (moves) {
        to=FirstOne(moves);
        *move++=temp|(to<<6)|(PieceOnSquare(to)<<15);
        Clear(to,moves);
      }
      Clear(from,piecebd);
    }
/*
 ----------------------------------------------------------
|                                                          |
|   now, produce king moves by cycling through the         |
|   *_king board to locate a [from] square and then        |
|   cycling through king_attacks[] to locate to squares    |
|   that a king on [from] attacks.                         |
|                                                          |
 ----------------------------------------------------------
*/
    from=BlackKingSQ;
    moves=And(king_attacks[from],target);
    temp=from+(king<<12);
    while (moves) {
      to=FirstOne(moves);
      *move++=temp|(to<<6)|(PieceOnSquare(to)<<15);
      Clear(to,moves);
    }
/*
 ----------------------------------------------------------
|                                                          |
|   now, produce pawn moves.  this is done differently due |
|   to inconsistencies in the way a pawn moves when it     |
|   captures as opposed to normal non-capturing moves.     |
|   another exception is capturing enpassant.  the first   |
|   step is to generate all possible pawn moves.  we do    |
|   this in 4 operations:  (1) shift the pawns forward one |
|   rank then and with empty squares;  (2) shift the pawns |
|   forward two ranks and then and with empty squares;     |
|   (3) remove the a-pawn(s) then shift the pawns          |
|   diagonally left then and with enemy occupied squares;  |
|   (4) remove the h-pawn(s) then shift the pawns          |
|   diagonally right then and with enemy occupied squares; |
|   note that enemy occupied squares includes the special  |
|   case of the enpassant target square also.              |
|                                                          |
 ----------------------------------------------------------
*/
    temp_target=Compl(Occupied);
    targets=And(temp_target,target);
    if(generate_captures) targets=Or(targets,And(rank_mask[RANK1],temp_target));
    else targets=And(targets,mask_not_rank1);
    padvances1=And(Shiftl(BlackPawns,8),targets);
    padvances2=And(Shiftl(And(padvances1,mask_advance_2_b),8),targets);
/*
 ----------------------------------------------------------
|                                                          |
|   now that we got 'em, we simply enumerate the to        |
|   squares as before, but in four steps since we have     |
|   four sets of potential moves.                          |
|                                                          |
 ----------------------------------------------------------
*/
    while (padvances2) {
      to=FirstOne(padvances2);
      *move++=(to+16)|(to<<6)|(pawn<<12);
      Clear(to,padvances2);
    }
    while (padvances1) {
      to=FirstOne(padvances1);
      if (to > 7) *move++=(to+8)|(to<<6)|(pawn<<12);
      else {
        *move++=(to+8)|(to<<6)|(pawn<<12)|(queen<<18);
        if (depth > 0) {
          *move++=(to+8)|(to<<6)|(pawn<<12)|(rook<<18);
          *move++=(to+8)|(to<<6)|(pawn<<12)|(bishop<<18);
          *move++=(to+8)|(to<<6)|(pawn<<12)|(knight<<18);
        }
      }
      Clear(to,padvances1);
    }
    if (generate_captures) {
      targetc=And(WhitePieces,target);
      targetc=Or(targetc,EnPassantTarget(ply));
      pcapturesl=And(Shiftl(And(BlackPawns,mask_left_edge),9),targetc);
      pcapturesr=And(Shiftl(And(BlackPawns,mask_right_edge),7),targetc);
      while (pcapturesl) {
        to=FirstOne(pcapturesl);
        if (to > 7) {
          if(PieceOnSquare(to)) 
            *move++=(to+9)|(to<<6)|(pawn<<12)|(PieceOnSquare(to)<<15);
          else
            *move++=(to+9)|(to<<6)|(pawn<<12)|(pawn<<15);
        }
        else {
          *move++=(to+9)|(to<<6)|(pawn<<12)|
            (PieceOnSquare(to)<<15)|(queen<<18);
          if (depth > 0) {
            *move++=(to+9)|(to<<6)|(pawn<<12)|(PieceOnSquare(to)<<15)|(rook<<18);
            *move++=(to+9)|(to<<6)|(pawn<<12)|(PieceOnSquare(to)<<15)|(bishop<<18);
            *move++=(to+9)|(to<<6)|(pawn<<12)|(PieceOnSquare(to)<<15)|(knight<<18);
          }
        }
        Clear(to,pcapturesl);
      }
      while (pcapturesr) {
        to=FirstOne(pcapturesr);
        if (to > 7) {
          if(PieceOnSquare(to)) 
            *move++=(to+7)|(to<<6)|(pawn<<12)|(PieceOnSquare(to)<<15);
          else
            *move++=(to+7)|(to<<6)|(pawn<<12)|(pawn<<15);
        }
        else {
          *move++=(to+7)|(to<<6)|(pawn<<12)|(PieceOnSquare(to)<<15)|(queen<<18);
          if (depth > 0) {
            *move++=(to+7)|(to<<6)|(pawn<<12)|(PieceOnSquare(to)<<15)|(rook<<18);
            *move++=(to+7)|(to<<6)|(pawn<<12)|(PieceOnSquare(to)<<15)|(bishop<<18);
            *move++=(to+7)|(to<<6)|(pawn<<12)|(PieceOnSquare(to)<<15)|(knight<<18);
          }
        }
        Clear(to,pcapturesr);
      }
    }
  }
  return(move);
}
