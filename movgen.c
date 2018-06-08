#include <stdio.h>
#include <stdlib.h>
#include "types.h"
#include "function.h"
#include "data.h"
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
int* GenerateCheckEvasions(int ply, int wtm, BITBOARD target,
                                    int checkers, int check_direction,
                                    int king_square, int *move)
{
  register BITBOARD targetc, targetp, piecebd, moves;
  register BITBOARD padvances1, padvances2, pcapturesl, pcapturesr;
  register BITBOARD padvances1_all, empty;
  register int from, to, temp;
  if (wtm) {
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
 ----------------------------------------------------------
*/
    if (checkers == 1)
      target=Or(target,And(AttacksTo(king_square,ply),BlackPieces(ply)));
/*
 ----------------------------------------------------------
|                                                          |
|   first produce king moves by cycling through the        |
|   *_king board to locate a [from] square and then        |
|   cycling through attacks_to[] to locate to squares      |
|   that the king on [from] attacks.                       |
|                                                          |
 ----------------------------------------------------------
*/
    from=king_square;
    moves=And(king_attacks[from],Compl(WhitePieces(ply)));
    temp=from+(king<<12);
    while (moves) {
      to=FirstOne(moves);
      if (!Attacked(to,ply,0) && (directions[from][to] != check_direction))
        *move++=temp|(to<<6)|((-PieceOnSquare(ply,to))<<15);
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
      piecebd=WhiteKnights(ply);
      while (piecebd) {
        from=FirstOne(piecebd);
        moves=And(knight_attacks[from],target);
        temp=from+(knight<<12);
        while (moves) {
          to=FirstOne(moves);
          *move++=temp|(to<<6)|((-PieceOnSquare(ply,to))<<15);
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
      piecebd=WhiteBishops(ply);
      while (piecebd) {
        from=FirstOne(piecebd);
        moves=And(AttacksBishop(from),target);
        temp=from+(bishop<<12);
        while (moves) {
          to=FirstOne(moves);
          *move++=temp|(to<<6)|((-PieceOnSquare(ply,to))<<15);
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
      piecebd=WhiteRooks(ply);
      while (piecebd) {
        from=FirstOne(piecebd);
        moves=And(AttacksRook(from),target);
        temp=from+(rook<<12);
        while (moves) {
          to=FirstOne(moves);
          *move++=temp|(to<<6)|((-PieceOnSquare(ply,to))<<15);
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
      piecebd=WhiteQueens(ply);
      while (piecebd) {
        from=FirstOne(piecebd);
        moves=And(AttacksQueen(from),target);
        temp=from+(queen<<12);
        while (moves) {
          to=FirstOne(moves);
          *move++=temp|(to<<6)|((-PieceOnSquare(ply,to))<<15);
          Clear(to,moves);
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
      empty=Compl(Or(WhitePieces(ply),BlackPieces(ply)));
      targetp=And(target,empty);
      padvances1=And(Shiftr(WhitePawns(ply),8),targetp);
      padvances1_all=And(Shiftr(WhitePawns(ply),8),empty);
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
        to=FirstOne(padvances2);
        *move++=(to-16)|(to<<6)|(pawn<<12);
        Clear(to,padvances2);
      }
      while (padvances1) {
        to=FirstOne(padvances1);
        if (to < 56) *move++=(to-8)|(to<<6)|(pawn<<12);
        else {
          *move++=(to-8)|(to<<6)|(pawn<<12)|(queen<<18);
          *move++=(to-8)|(to<<6)|(pawn<<12)|(rook<<18);
          *move++=(to-8)|(to<<6)|(pawn<<12)|(bishop<<18);
          *move++=(to-8)|(to<<6)|(pawn<<12)|(knight<<18);
        }
        Clear(to,padvances1);
      }

      targetc=Or(BlackPieces(ply),EnPassantTarget(ply));
      targetc=And(targetc,target);
      if (checkers == 1) {
        if (And(And(BlackPawns(ply),target),Shiftl(EnPassantTarget(ply),8)))
          targetc=Or(targetc,EnPassantTarget(ply));
      }
      pcapturesl=And(Shiftr(And(WhitePawns(ply),mask_left_edge),7),targetc);
      pcapturesr=And(Shiftr(And(WhitePawns(ply),mask_right_edge),9),targetc);
      while (pcapturesl) {
        to=FirstOne(pcapturesl);
        if (to < 56) {
          if(PieceOnSquare(ply,to)) 
            *move++=(to-7)|(to<<6)|(pawn<<12)|((-PieceOnSquare(ply,to))<<15);
          else
            *move++=(to-7)|(to<<6)|(pawn<<12)|(pawn<<15);
        }
        else {
          *move++=(to-7)|(to<<6)|(pawn<<12)|((-PieceOnSquare(ply,to))<<15)|(queen<<18);
          *move++=(to-7)|(to<<6)|(pawn<<12)|((-PieceOnSquare(ply,to))<<15)|(rook<<18);
          *move++=(to-7)|(to<<6)|(pawn<<12)|((-PieceOnSquare(ply,to))<<15)|(bishop<<18);
          *move++=(to-7)|(to<<6)|(pawn<<12)|((-PieceOnSquare(ply,to))<<15)|(knight<<18);
        }
        Clear(to,pcapturesl);
      }
      while (pcapturesr) {
        to=FirstOne(pcapturesr);
        if (to < 56) {
          if(PieceOnSquare(ply,to)) 
            *move++=(to-9)|(to<<6)|(pawn<<12)|((-PieceOnSquare(ply,to))<<15);
          else
            *move++=(to-9)|(to<<6)|(pawn<<12)|(pawn<<15);
        }
        else {
          *move++=(to-9)|(to<<6)|(pawn<<12)|((-PieceOnSquare(ply,to))<<15)|(queen<<18);
          *move++=(to-9)|(to<<6)|(pawn<<12)|((-PieceOnSquare(ply,to))<<15)|(rook<<18);
          *move++=(to-9)|(to<<6)|(pawn<<12)|((-PieceOnSquare(ply,to))<<15)|(bishop<<18);
          *move++=(to-9)|(to<<6)|(pawn<<12)|((-PieceOnSquare(ply,to))<<15)|(knight<<18);
        }
        Clear(to,pcapturesr);
      }
    }
  }
  else {
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
 ----------------------------------------------------------
*/
    if (checkers == 1)
      target=Or(target,And(AttacksTo(king_square,ply),WhitePieces(ply)));
/*
 ----------------------------------------------------------
|                                                          |
|   first produce king moves by cycling through the        |
|   *_king board to locate a [from] square and then        |
|   cycling through attacks_to[] to locate to squares      |
|   that the king on [from] attacks.                       |
|                                                          |
 ----------------------------------------------------------
*/
    from=king_square;
    moves=And(king_attacks[from],Compl(BlackPieces(ply)));
    temp=from+(king<<12);
    while (moves) {
      to=FirstOne(moves);
      if (!Attacked(to,ply,1) && (directions[from][to] != check_direction))
        *move++=temp|(to<<6)|(PieceOnSquare(ply,to)<<15);
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
      piecebd=BlackKnights(ply);
      while (piecebd) {
        from=FirstOne(piecebd);
        moves=And(knight_attacks[from],target);
        temp=from+(knight<<12);
        while (moves) {
          to=FirstOne(moves);
          *move++=temp|(to<<6)|(PieceOnSquare(ply,to)<<15);
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
      piecebd=BlackBishops(ply);
      while (piecebd) {
        from=FirstOne(piecebd);
        moves=And(AttacksBishop(from),target);
        temp=from+(bishop<<12);
        while (moves) {
          to=FirstOne(moves);
          *move++=temp|(to<<6)|(PieceOnSquare(ply,to)<<15);
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
      piecebd=BlackRooks(ply);
      while (piecebd) {
        from=FirstOne(piecebd);
        moves=And(AttacksRook(from),target);
        temp=from+(rook<<12);
        while (moves) {
          to=FirstOne(moves);
          *move++=temp|(to<<6)|(PieceOnSquare(ply,to)<<15);
          Clear(to,moves);
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
      piecebd=BlackQueens(ply);
      while (piecebd) {
        from=FirstOne(piecebd);
        moves=And(AttacksQueen(from),target);
        temp=from+(queen<<12);
        while (moves) {
          to=FirstOne(moves);
          *move++=temp|(to<<6)|(PieceOnSquare(ply,to)<<15);
          Clear(to,moves);
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
      empty=Compl(Or(WhitePieces(ply),BlackPieces(ply)));
      targetp=And(target,empty);
      padvances1=And(Shiftl(BlackPawns(ply),8),targetp);
      padvances1_all=And(Shiftl(BlackPawns(ply),8),empty);
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
        *move++=(to+16)|(to<<6)|(pawn<<12);
        Clear(to,padvances2);
      }
      while (padvances1) {
        to=FirstOne(padvances1);
        if (to > 7) *move++=(to+8)|(to<<6)|(pawn<<12);
        else {
          *move++=(to+8)|(to<<6)|(pawn<<12)|(queen<<18);
          *move++=(to+8)|(to<<6)|(pawn<<12)|(rook<<18);
          *move++=(to+8)|(to<<6)|(pawn<<12)|(bishop<<18);
          *move++=(to+8)|(to<<6)|(pawn<<12)|(knight<<18);
        }
        Clear(to,padvances1);
      }

      targetc=Or(WhitePieces(ply),EnPassantTarget(ply));
      targetc=And(targetc,target);
      if (checkers == 1) {
        if (And(And(WhitePawns(ply),target),Shiftr(EnPassantTarget(ply),8)))
          targetc=Or(targetc,EnPassantTarget(ply));
      }
      pcapturesl=And(Shiftl(And(BlackPawns(ply),mask_left_edge),9),targetc);
      pcapturesr=And(Shiftl(And(BlackPawns(ply),mask_right_edge),7),targetc);
      while (pcapturesl) {
        to=FirstOne(pcapturesl);
        if (to > 7) {
          if(PieceOnSquare(ply,to)) 
            *move++=(to+9)|(to<<6)|(pawn<<12)|(PieceOnSquare(ply,to)<<15);
          else
            *move++=(to+9)|(to<<6)|(pawn<<12)|(pawn<<15);
        }
        else {
          *move++=(to+9)|(to<<6)|(pawn<<12)|(PieceOnSquare(ply,to)<<15)|(queen<<18);
          *move++=(to+9)|(to<<6)|(pawn<<12)|(PieceOnSquare(ply,to)<<15)|(rook<<18);
          *move++=(to+9)|(to<<6)|(pawn<<12)|(PieceOnSquare(ply,to)<<15)|(bishop<<18);
          *move++=(to+9)|(to<<6)|(pawn<<12)|(PieceOnSquare(ply,to)<<15)|(knight<<18);
        }
        Clear(to,pcapturesl);
      }
      while (pcapturesr) {
        to=FirstOne(pcapturesr);
        if (to > 7) {
          if(PieceOnSquare(ply,to)) 
            *move++=(to+7)|(to<<6)|(pawn<<12)|(PieceOnSquare(ply,to)<<15);
          else
            *move++=(to+7)|(to<<6)|(pawn<<12)|(pawn<<15);
        }
        else {
          *move++=(to+7)|(to<<6)|(pawn<<12)|(PieceOnSquare(ply,to)<<15)|(queen<<18);
          *move++=(to+7)|(to<<6)|(pawn<<12)|(PieceOnSquare(ply,to)<<15)|(rook<<18);
          *move++=(to+7)|(to<<6)|(pawn<<12)|(PieceOnSquare(ply,to)<<15)|(bishop<<18);
          *move++=(to+7)|(to<<6)|(pawn<<12)|(PieceOnSquare(ply,to)<<15)|(knight<<18);
        }
        Clear(to,pcapturesr);
      }
    }
  }
  return(move);
}

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
*   shifting the pawn bit-board 8 bits "forward" and anding this with the      *
*   complement of the occupied squares bit-  double advances are         *
*   produced by anding the pawn bit-board with a mask containing 1's on the    *
*   second rank, shifting this 16 bits "forward" and then anding this with     *
*   the complement of the occupied squares bit-board as before.  if [to]       *
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
    if (WhiteCastle(ply)) {
      if ((WhiteCastle(ply) & 1) && And(set_mask[6],target) &&
          !And(Or(WhitePieces(ply),BlackPieces(ply)),Shiftr(mask_2,5)) &&
          !Attacked(4,ply,0) && !Attacked(5,ply,0) && !Attacked(6,ply,0)) {
        *move++=12676;
      }
      if ((WhiteCastle(ply) & 2) && And(set_mask[2],target) &&
          !And(Or(WhitePieces(ply),BlackPieces(ply)),Shiftr(mask_3,1)) &&
          !Attacked(2,ply,0) && !Attacked(3,ply,0) && !Attacked(4,ply,0)) {
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
    piecebd=WhiteKnights(ply);
    while (piecebd) {
      from=FirstOne(piecebd);
      moves=And(knight_attacks[from],target);
      temp=from+(knight<<12);
      while (moves) {
        to=FirstOne(moves);
        *move++=temp|(to<<6)|((-PieceOnSquare(ply,to))<<15);
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
    piecebd=WhiteBishops(ply);
    while (piecebd) {
      from=FirstOne(piecebd);
      moves=And(AttacksBishop(from),target);
      temp=from+(bishop<<12);
      while (moves) {
        to=FirstOne(moves);
        *move++=temp|(to<<6)|((-PieceOnSquare(ply,to))<<15);
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
    piecebd=WhiteRooks(ply);
    while (piecebd) {
      from=FirstOne(piecebd);
      moves=And(AttacksRook(from),target);
      temp=from+(rook<<12);
      while (moves) {
        to=FirstOne(moves);
        *move++=temp|(to<<6)|((-PieceOnSquare(ply,to))<<15);
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
    piecebd=WhiteQueens(ply);
    while (piecebd) {
      from=FirstOne(piecebd);
      moves=And(AttacksQueen(from),target);
      temp=from+(queen<<12);
      while (moves) {
        to=FirstOne(moves);
        *move++=temp|(to<<6)|((-PieceOnSquare(ply,to))<<15);
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
    from=WhiteKingSQ(ply);
    moves=And(king_attacks[from],target);
    temp=from+(king<<12);
    while (moves) {
      to=FirstOne(moves);
      *move++=temp|(to<<6)|((-PieceOnSquare(ply,to))<<15);
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
    temp_target=Compl(Or(WhitePieces(ply),BlackPieces(ply)));
    targets=And(temp_target,target);
    if(generate_captures) targets=Or(targets,And(mask_120,temp_target));
    padvances1=And(Shiftr(WhitePawns(ply),8),targets);
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
      to=FirstOne(padvances2);
      *move++=(to-16)|(to<<6)|(pawn<<12);
      Clear(to,padvances2);
    }
    while (padvances1) {
      to=FirstOne(padvances1);
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
      targetc=And(BlackPieces(ply),target);
      targetc=Or(targetc,EnPassantTarget(ply));
      pcapturesl=And(Shiftr(And(WhitePawns(ply),mask_left_edge),7),targetc);
      pcapturesr=And(Shiftr(And(WhitePawns(ply),mask_right_edge),9),targetc);
      while (pcapturesl) {
        to=FirstOne(pcapturesl);
        if (to < 56) {
          if(PieceOnSquare(ply,to)) 
            *move++=(to-7)|(to<<6)|(pawn<<12)|((-PieceOnSquare(ply,to))<<15);
          else
            *move++=(to-7)|(to<<6)|(pawn<<12)|(pawn<<15);
        }
        else {
          *move++=(to-7)|(to<<6)|(pawn<<12)|((-PieceOnSquare(ply,to))<<15)|(queen<<18);
          if (depth > 0) {
            *move++=(to-7)|(to<<6)|(pawn<<12)|((-PieceOnSquare(ply,to))<<15)|(rook<<18);
            *move++=(to-7)|(to<<6)|(pawn<<12)|((-PieceOnSquare(ply,to))<<15)|(bishop<<18);
            *move++=(to-7)|(to<<6)|(pawn<<12)|((-PieceOnSquare(ply,to))<<15)|(knight<<18);
          }
        }
        Clear(to,pcapturesl);
      }
      while (pcapturesr) {
        to=FirstOne(pcapturesr);
        if (to < 56) {
          if(PieceOnSquare(ply,to)) 
            *move++=(to-9)|(to<<6)|(pawn<<12)|(-PieceOnSquare(ply,to)<<15);
          else
            *move++=(to-9)|(to<<6)|(pawn<<12)|(pawn<<15);
        }
        else {
          *move++=(to-9)|(to<<6)|(pawn<<12)|((-PieceOnSquare(ply,to))<<15)|(queen<<18);
          if (depth > 0) {
            *move++=(to-9)|(to<<6)|(pawn<<12)|((-PieceOnSquare(ply,to))<<15)|(rook<<18);
            *move++=(to-9)|(to<<6)|(pawn<<12)|((-PieceOnSquare(ply,to))<<15)|(bishop<<18);
            *move++=(to-9)|(to<<6)|(pawn<<12)|((-PieceOnSquare(ply,to))<<15)|(knight<<18);
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
    if (BlackCastle(ply)) {
      if ((BlackCastle(ply) & 1)  && And(set_mask[62],target) &&
          !And(Or(WhitePieces(ply),BlackPieces(ply)),Shiftr(mask_2,61)) &&
          !Attacked(60,ply,1) && !Attacked(61,ply,1) && !Attacked(62,ply,1)) {
        *move++=16316;
      }
      if ((BlackCastle(ply) & 2) && And(set_mask[58],target) &&
          !And(Or(WhitePieces(ply),BlackPieces(ply)),Shiftr(mask_3,57)) &&
          !Attacked(58,ply,1) && !Attacked(59,ply,1) && !Attacked(60,ply,1)) {
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
    piecebd=BlackKnights(ply);
    while (piecebd) {
      from=FirstOne(piecebd);
      moves=And(knight_attacks[from],target);
      temp=from+(knight<<12);
      while (moves) {
        to=FirstOne(moves);
        *move++=temp|(to<<6)|(PieceOnSquare(ply,to)<<15);
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
    piecebd=BlackBishops(ply);
    while (piecebd) {
      from=FirstOne(piecebd);
      moves=And(AttacksBishop(from),target);
      temp=from+(bishop<<12);
      while (moves) {
        to=FirstOne(moves);
        *move++=temp|(to<<6)|(PieceOnSquare(ply,to)<<15);
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
    piecebd=BlackRooks(ply);
    while (piecebd) {
      from=FirstOne(piecebd);
      moves=And(AttacksRook(from),target);
      temp=from+(rook<<12);
      while (moves) {
        to=FirstOne(moves);
        *move++=temp|(to<<6)|(PieceOnSquare(ply,to)<<15);
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
    piecebd=BlackQueens(ply);
    while (piecebd) {
      from=FirstOne(piecebd);
      moves=And(AttacksQueen(from),target);
      temp=from+(queen<<12);
      while (moves) {
        to=FirstOne(moves);
        *move++=temp|(to<<6)|(PieceOnSquare(ply,to)<<15);
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
    from=BlackKingSQ(ply);
    moves=And(king_attacks[from],target);
    temp=from+(king<<12);
    while (moves) {
      to=FirstOne(moves);
      *move++=temp|(to<<6)|(PieceOnSquare(ply,to)<<15);
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
    temp_target=Compl(Or(WhitePieces(ply),BlackPieces(ply)));
    targets=And(temp_target,target);
    if(generate_captures) targets=Or(targets,And(mask_8,temp_target));
    padvances1=And(Shiftl(BlackPawns(ply),8),targets);
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
      targetc=And(WhitePieces(ply),target);
      targetc=Or(targetc,EnPassantTarget(ply));
      pcapturesl=And(Shiftl(And(BlackPawns(ply),mask_left_edge),9),targetc);
      pcapturesr=And(Shiftl(And(BlackPawns(ply),mask_right_edge),7),targetc);
      while (pcapturesl) {
        to=FirstOne(pcapturesl);
        if (to > 7) {
          if(PieceOnSquare(ply,to)) 
            *move++=(to+9)|(to<<6)|(pawn<<12)|(PieceOnSquare(ply,to)<<15);
          else
            *move++=(to+9)|(to<<6)|(pawn<<12)|(pawn<<15);
        }
        else {
          *move++=(to+9)|(to<<6)|(pawn<<12)|
            (PieceOnSquare(ply,to)<<15)|(queen<<18);
          if (depth > 0) {
            *move++=(to+9)|(to<<6)|(pawn<<12)|(PieceOnSquare(ply,to)<<15)|(rook<<18);
            *move++=(to+9)|(to<<6)|(pawn<<12)|(PieceOnSquare(ply,to)<<15)|(bishop<<18);
            *move++=(to+9)|(to<<6)|(pawn<<12)|(PieceOnSquare(ply,to)<<15)|(knight<<18);
          }
        }
        Clear(to,pcapturesl);
      }
      while (pcapturesr) {
        to=FirstOne(pcapturesr);
        if (to > 7) {
          if(PieceOnSquare(ply,to)) 
            *move++=(to+7)|(to<<6)|(pawn<<12)|(PieceOnSquare(ply,to)<<15);
          else
            *move++=(to+7)|(to<<6)|(pawn<<12)|(pawn<<15);
        }
        else {
          *move++=(to+7)|(to<<6)|(pawn<<12)|(PieceOnSquare(ply,to)<<15)|(queen<<18);
          if (depth > 0) {
            *move++=(to+7)|(to<<6)|(pawn<<12)|(PieceOnSquare(ply,to)<<15)|(rook<<18);
            *move++=(to+7)|(to<<6)|(pawn<<12)|(PieceOnSquare(ply,to)<<15)|(bishop<<18);
            *move++=(to+7)|(to<<6)|(pawn<<12)|(PieceOnSquare(ply,to)<<15)|(knight<<18);
          }
        }
        Clear(to,pcapturesr);
      }
    }
  }
  return(move);
}
