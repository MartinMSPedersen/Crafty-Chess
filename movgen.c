#include <stdio.h>
#include <stdlib.h>
#include "types.h"
#include "function.h"
#include "data.h"
/*
********************************************************************************
*                                                                              *
*   Generate_Check_Evasions() is used to generate moves when the king is in    *
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
int* Generate_Check_Evasions(int ply, int wtm, BITBOARD target,
                                    int checkers, int check_direction,
                                    int king_square, int *move)
{
  register BITBOARD targetc, targetp, piecebd, moves;
  register BITBOARD padvances1, padvances2, pcapturesl, pcapturesr;
  register BITBOARD padvances1_all, empty;
  register int from, to, temp, promote;
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
      target=Or(target,And(Attacks_To(king_square,ply),
                           Black_Pieces(ply)));
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
    moves=And(king_attacks[from],Compl(White_Pieces(ply)));
    temp=from+(king<<12);
    while (moves) {
      to=First_One(moves);
      if (!Attacked(to,ply,0) && (directions[from][to] != check_direction))
        *move++=temp|(to<<6)|((-Piece_On_Square(ply,to))<<15);
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
      piecebd=White_Knights(ply);
      while (piecebd) {
        from=First_One(piecebd);
        moves=And(knight_attacks[from],target);
        temp=from+(knight<<12);
        while (moves) {
          to=First_One(moves);
          *move++=temp|(to<<6)|((-Piece_On_Square(ply,to))<<15);
          Clear(to,moves);
        }
        Clear(from,piecebd);
      }
/*
 ----------------------------------------------------------
|                                                          |
|   now, produce bishop moves by cycling through the       |
|   *_bishop board to locate a [from] square and then      |
|   generate the Attacks_From() bitmap which supplies the  |
|   list of valid <to> squares.                            |
|                                                          |
 ----------------------------------------------------------
*/
      piecebd=White_Bishops(ply);
      while (piecebd) {
        from=First_One(piecebd);
        moves=And(Attacks_Bishop(from),target);
        temp=from+(bishop<<12);
        while (moves) {
          to=First_One(moves);
          *move++=temp|(to<<6)|((-Piece_On_Square(ply,to))<<15);
          Clear(to,moves);
        }
        Clear(from,piecebd);
      }
/*
 ----------------------------------------------------------
|                                                          |
|   now, produce rook moves by cycling through the         |
|   *_rook board to locate a [from] square and then        |
|   generate the Attacks_From() bitmap which supplies the  |
|   list of valid <to> squares.                            |
|                                                          |
 ----------------------------------------------------------
*/
      piecebd=White_Rooks(ply);
      while (piecebd) {
        from=First_One(piecebd);
        moves=And(Attacks_Rook(from),target);
        temp=from+(rook<<12);
        while (moves) {
          to=First_One(moves);
          *move++=temp|(to<<6)|((-Piece_On_Square(ply,to))<<15);
          Clear(to,moves);
        }
        Clear(from,piecebd);
      }
/*
 ----------------------------------------------------------
|                                                          |
|   now, produce queen moves by cycling through the        |
|   *_queen board to locate a [from] square and then       |
|   generate the Attacks_From() bitmap which supplies the  |
|   list of valid <to> squares.                            |
|                                                          |
 ----------------------------------------------------------
*/
      piecebd=White_Queens(ply);
      while (piecebd) {
        from=First_One(piecebd);
        moves=And(Attacks_Queen(from),target);
        temp=from+(queen<<12);
        while (moves) {
          to=First_One(moves);
          *move++=temp|(to<<6)|((-Piece_On_Square(ply,to))<<15);
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
      empty=Compl(Or(White_Pieces(ply),
                     Black_Pieces(ply)));
      targetp=And(target,empty);
      padvances1=And(Shiftr(White_Pawns(ply),8),targetp);
      padvances1_all=And(Shiftr(White_Pawns(ply),8),empty);
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
        to=First_One(padvances2);
        *move++=(to-16)|(to<<6)|(pawn<<12);
        Clear(to,padvances2);
      }
      while (padvances1) {
        to=First_One(padvances1);
        if (to < 56)
          *move++=(to-8)|(to<<6)|(pawn<<12);
        else
          for (promote=queen;promote>pawn;promote--)
            *move++=(to-8)|(to<<6)|(pawn<<12)|(promote<<18);
        Clear(to,padvances1);
      }

      targetc=Or(Black_Pieces(ply),
                 EnPassant_Target(ply));
      targetc=And(targetc,target);
      if (checkers == 1) {
        if (And(And(Black_Pawns(ply),target),
                 Shiftl(EnPassant_Target(ply),8)))
          targetc=Or(targetc,EnPassant_Target(ply));
      }
      pcapturesl=And(Shiftr(And(White_Pawns(ply),
                                mask_left_edge),7),targetc);
      pcapturesr=And(Shiftr(And(White_Pawns(ply),
                                mask_right_edge),9),targetc);
      while (pcapturesl) {
        to=First_One(pcapturesl);
        if (to < 56) {
          if(Piece_On_Square(ply,to)) 
            *move++=(to-7)|(to<<6)|(pawn<<12)|
              ((-Piece_On_Square(ply,to))<<15);
          else
            *move++=(to-7)|(to<<6)|(pawn<<12)|(pawn<<15);
        }
        else
          for (promote=queen;promote>pawn;promote--)
            *move++=(to-7)|(to<<6)|(pawn<<12)|
              ((-Piece_On_Square(ply,to))<<15)|(promote<<18);
        Clear(to,pcapturesl);
      }
      while (pcapturesr) {
        to=First_One(pcapturesr);
        if (to < 56) {
          if(Piece_On_Square(ply,to)) 
            *move++=(to-9)|(to<<6)|(pawn<<12)|
              ((-Piece_On_Square(ply,to))<<15);
          else
            *move++=(to-9)|(to<<6)|(pawn<<12)|(pawn<<15);
        }
        else
          for (promote=queen;promote>pawn;promote--)
            *move++=(to-9)|(to<<6)|(pawn<<12)|
              ((-Piece_On_Square(ply,to))<<15)|(promote<<18);
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
      target=Or(target,And(Attacks_To(king_square,ply),
                           White_Pieces(ply)));
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
    moves=And(king_attacks[from],Compl(Black_Pieces(ply)));
    temp=from+(king<<12);
    while (moves) {
      to=First_One(moves);
      if (!Attacked(to,ply,1) && (directions[from][to] != check_direction))
        *move++=temp|(to<<6)|(Piece_On_Square(ply,to)<<15);
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
      piecebd=Black_Knights(ply);
      while (piecebd) {
        from=First_One(piecebd);
        moves=And(knight_attacks[from],target);
        temp=from+(knight<<12);
        while (moves) {
          to=First_One(moves);
          *move++=temp|(to<<6)|(Piece_On_Square(ply,to)<<15);
          Clear(to,moves);
        }
        Clear(from,piecebd);
      }
/*
 ----------------------------------------------------------
|                                                          |
|   now, produce bishop moves by cycling through the       |
|   *_bishop board to locate a [from] square and then      |
|   generate the Attacks_From() bitmap which supplies the  |
|   list of valid <to> squares.                            |
|                                                          |
 ----------------------------------------------------------
*/
      piecebd=Black_Bishops(ply);
      while (piecebd) {
        from=First_One(piecebd);
        moves=And(Attacks_Bishop(from),target);
        temp=from+(bishop<<12);
        while (moves) {
          to=First_One(moves);
          *move++=temp|(to<<6)|(Piece_On_Square(ply,to)<<15);
          Clear(to,moves);
        }
        Clear(from,piecebd);
      }
/*
 ----------------------------------------------------------
|                                                          |
|   now, produce rook moves by cycling through the         |
|   *_rook board to locate a [from] square and then        |
|   generate the Attacks_From() bitmap which supplies the  |
|   list of valid <to> squares.                            |
|                                                          |
 ----------------------------------------------------------
*/
      piecebd=Black_Rooks(ply);
      while (piecebd) {
        from=First_One(piecebd);
        moves=And(Attacks_Rook(from),target);
        temp=from+(rook<<12);
        while (moves) {
          to=First_One(moves);
          *move++=temp|(to<<6)|(Piece_On_Square(ply,to)<<15);
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
      piecebd=Black_Queens(ply);
      while (piecebd) {
        from=First_One(piecebd);
        moves=And(Attacks_Queen(from),target);
        temp=from+(queen<<12);
        while (moves) {
          to=First_One(moves);
          *move++=temp|(to<<6)|(Piece_On_Square(ply,to)<<15);
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
      empty=Compl(Or(White_Pieces(ply),
                     Black_Pieces(ply)));
      targetp=And(target,empty);
      padvances1=And(Shiftl(Black_Pawns(ply),8),targetp);
      padvances1_all=And(Shiftl(Black_Pawns(ply),8),empty);
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
        to=First_One(padvances2);
        *move++=(to+16)|(to<<6)|(pawn<<12);
        Clear(to,padvances2);
      }
      while (padvances1) {
        to=First_One(padvances1);
        if (to > 7)
          *move++=(to+8)|(to<<6)|(pawn<<12);
        else
          for (promote=queen;promote>pawn;promote--)
            *move++=(to+8)|(to<<6)|(pawn<<12)|(promote<<18);
        Clear(to,padvances1);
      }

      targetc=Or(White_Pieces(ply),
                  EnPassant_Target(ply));
      targetc=And(targetc,target);
      if (checkers == 1) {
        if (And(And(White_Pawns(ply),target),
                 Shiftr(EnPassant_Target(ply),8)))
          targetc=Or(targetc,EnPassant_Target(ply));
      }
      pcapturesl=And(Shiftl(And(Black_Pawns(ply),
                                mask_left_edge),9),targetc);
      pcapturesr=And(Shiftl(And(Black_Pawns(ply),
                                mask_right_edge),7),targetc);
      while (pcapturesl) {
        to=First_One(pcapturesl);
        if (to > 7) {
          if(Piece_On_Square(ply,to)) 
            *move++=(to+9)|(to<<6)|(pawn<<12)|
              (Piece_On_Square(ply,to)<<15);
          else
            *move++=(to+9)|(to<<6)|(pawn<<12)|(pawn<<15);
        }
        else
          for (promote=queen;promote>pawn;promote--)
            *move++=(to+9)|(to<<6)|(pawn<<12)|
              (Piece_On_Square(ply,to)<<15)|(promote<<18);
        Clear(to,pcapturesl);
      }
      while (pcapturesr) {
        to=First_One(pcapturesr);
        if (to > 7) {
          if(Piece_On_Square(ply,to)) 
            *move++=(to+7)|(to<<6)|(pawn<<12)|
              (Piece_On_Square(ply,to)<<15);
          else
            *move++=(to+7)|(to<<6)|(pawn<<12)|(pawn<<15);
        }
        else
          for (promote=queen;promote>pawn;promote--)
            *move++=(to+7)|(to<<6)|(pawn<<12)|
              (Piece_On_Square(ply,to)<<15)|(promote<<18);
        Clear(to,pcapturesr);
      }
    }
  }
  return(move);
}

/*
********************************************************************************
*                                                                              *
*   Generate_Moves() is used to generate moves to a set of squares from the    *
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
*   complement of the occupied squares bit-board.  double advances are         *
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
*   Generate_Moves() to produce piece moves to this "wrong" square.  (2)       *
*   promotions are another special case.  since a promotion can be thought of  *
*   as a special case of captures (since the move does gain material..) the    *
*   quiescence search needs these moves included.  generate_captures forces    *
*   their inclusion.                                                           *
*                                                                              *
********************************************************************************
*/
int* Generate_Moves(int ply, int depth, int wtm, BITBOARD target, 
                              int generate_captures, int *move)
{
  register BITBOARD targets, targetc, temp_target , piecebd, moves;
  register BITBOARD  padvances1, padvances2, pcapturesl, pcapturesr;
  register int from, to, promote, temp;
  if (wtm) {
/*
 ----------------------------------------------------------
|                                                          |
|   first, produce castling moves if it is legal.          |
|                                                          |
 ----------------------------------------------------------
*/
    if (White_Castle(ply)) {
      if ((White_Castle(ply) & 1) && And(set_mask[6],target) &&
          !And(Or(White_Pieces(ply),
                  Black_Pieces(ply)),Shiftr(mask_2,5)) &&
          !Attacked(4,ply,0) && !Attacked(5,ply,0) && !Attacked(6,ply,0)) {
        *move++=24964;
      }
      if ((White_Castle(ply) & 2) && And(set_mask[2],target) &&
          !And(Or(White_Pieces(ply),
                  Black_Pieces(ply)),Shiftr(mask_3,1)) &&
          !Attacked(2,ply,0) && !Attacked(3,ply,0) && !Attacked(4,ply,0)) {
        *move++=24708;
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
    piecebd=White_Knights(ply);
    while (piecebd) {
      from=First_One(piecebd);
      moves=And(knight_attacks[from],target);
      temp=from+(knight<<12);
      while (moves) {
        to=First_One(moves);
        *move++=temp|(to<<6)|((-Piece_On_Square(ply,to))<<15);
        Clear(to,moves);
      }
      Clear(from,piecebd);
    }
/*
 ----------------------------------------------------------
|                                                          |
|   now, produce bishop moves by cycling through the       |
|   *_bishop board to locate a [from] square and then      |
|   generate the Attacks_From() bitmap which supplies the  |
|   list of valid <to> squares.                            |
|                                                          |
 ----------------------------------------------------------
*/
    piecebd=White_Bishops(ply);
    while (piecebd) {
      from=First_One(piecebd);
      moves=And(Attacks_Bishop(from),target);
      temp=from+(bishop<<12);
      while (moves) {
        to=First_One(moves);
        *move++=temp|(to<<6)|((-Piece_On_Square(ply,to))<<15);
        Clear(to,moves);
      }
      Clear(from,piecebd);
    }
/*
 ----------------------------------------------------------
|                                                          |
|   now, produce rook moves by cycling through the         |
|   *_rook board to locate a [from] square and then        |
|   generate the Attacks_From() bitmap which supplies the  |
|   list of valid <to> squares.                            |
|                                                          |
 ----------------------------------------------------------
*/
    piecebd=White_Rooks(ply);
    while (piecebd) {
      from=First_One(piecebd);
      moves=And(Attacks_Rook(from),target);
      temp=from+(rook<<12);
      while (moves) {
        to=First_One(moves);
        *move++=temp|(to<<6)|((-Piece_On_Square(ply,to))<<15);
        Clear(to,moves);
      }
      Clear(from,piecebd);
    }
/*
 ----------------------------------------------------------
|                                                          |
|   now, produce queen moves by cycling through the        |
|   *_queen board to locate a [from] square and then       |
|   generate the Attacks_From() bitmap which supplies the  |
|   list of valid <to> squares.                            |
|                                                          |
 ----------------------------------------------------------
*/
    piecebd=White_Queens(ply);
    while (piecebd) {
      from=First_One(piecebd);
      moves=And(Attacks_Queen(from),target);
      temp=from+(queen<<12);
      while (moves) {
        to=First_One(moves);
        *move++=temp|(to<<6)|((-Piece_On_Square(ply,to))<<15);
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
    from=White_King_SQ(ply);
    moves=And(king_attacks[from],target);
    temp=from+(king<<12);
    while (moves) {
      to=First_One(moves);
      *move++=temp|(to<<6)|((-Piece_On_Square(ply,to))<<15);
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
    temp_target=Compl(Or(White_Pieces(ply),
                         Black_Pieces(ply)));
    targets=And(temp_target,target);
    if(generate_captures) targets=Or(targets,And(mask_120,temp_target));
    padvances1=And(Shiftr(White_Pawns(ply),8),targets);
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
      to=First_One(padvances2);
      *move++=(to-16)|(to<<6)|(pawn<<12);
      Clear(to,padvances2);
    }
    while (padvances1) {
      to=First_One(padvances1);
      if (to < 56)
        *move++=(to-8)|(to<<6)|(pawn<<12);
      else {
        *move++=(to-8)|(to<<6)|(pawn<<12)|(queen<<18);
        if (depth > 0) 
          for (promote=rook;promote>pawn;promote--)
            *move++=(to-8)|(to<<6)|(pawn<<12)|(promote<<18);
      }
      Clear(to,padvances1);
    }
    if (generate_captures) {
      targetc=And(Black_Pieces(ply),target);
      targetc=Or(targetc,EnPassant_Target(ply));
      pcapturesl=And(Shiftr(And(White_Pawns(ply),
                                mask_left_edge),7),targetc);
      pcapturesr=And(Shiftr(And(White_Pawns(ply),
                                mask_right_edge),9),targetc);
      while (pcapturesl) {
        to=First_One(pcapturesl);
        if (to < 56) {
          if(Piece_On_Square(ply,to)) 
            *move++=(to-7)|(to<<6)|(pawn<<12)|
              ((-Piece_On_Square(ply,to))<<15);
          else
            *move++=(to-7)|(to<<6)|(pawn<<12)|(pawn<<15);
        }
        else {
          *move++=(to-7)|(to<<6)|(pawn<<12)|
            ((-Piece_On_Square(ply,to))<<15)|(queen<<18);
          if (depth > 0) 
            for (promote=rook;promote>pawn;promote--)
              *move++=(to-7)|(to<<6)|(pawn<<12)|
                ((-Piece_On_Square(ply,to))<<15)|(promote<<18);
        }
        Clear(to,pcapturesl);
      }
      while (pcapturesr) {
        to=First_One(pcapturesr);
        if (to < 56) {
          if(Piece_On_Square(ply,to)) 
            *move++=(to-9)|(to<<6)|(pawn<<12)|
              (-Piece_On_Square(ply,to)<<15);
          else
            *move++=(to-9)|(to<<6)|(pawn<<12)|(pawn<<15);
        }
        else {
          *move++=(to-9)|(to<<6)|(pawn<<12)|
            ((-Piece_On_Square(ply,to))<<15)|(queen<<18);
          if (depth > 0) 
            for (promote=rook;promote>pawn;promote--)
              *move++=(to-9)|(to<<6)|(pawn<<12)|
                ((-Piece_On_Square(ply,to))<<15)|(promote<<18);
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
    if (Black_Castle(ply)) {
      if ((Black_Castle(ply) & 1)  && And(set_mask[62],target) &&
          !And(Or(White_Pieces(ply),
                  Black_Pieces(ply)),Shiftr(mask_2,61)) &&
          !Attacked(60,ply,1) && !Attacked(61,ply,1) && !Attacked(62,ply,1)) {
        *move++=28604;
      }
      if ((Black_Castle(ply) & 2) && And(set_mask[58],target) &&
          !And(Or(White_Pieces(ply),
                  Black_Pieces(ply)),Shiftr(mask_3,57)) &&
          !Attacked(58,ply,1) && !Attacked(59,ply,1) && !Attacked(60,ply,1)) {
        *move++=28348;
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
    piecebd=Black_Knights(ply);
    while (piecebd) {
      from=First_One(piecebd);
      moves=And(knight_attacks[from],target);
      temp=from+(knight<<12);
      while (moves) {
        to=First_One(moves);
        *move++=temp|(to<<6)|(Piece_On_Square(ply,to)<<15);
        Clear(to,moves);
      }
      Clear(from,piecebd);
    }
/*
 ----------------------------------------------------------
|                                                          |
|   now, produce bishop moves by cycling through the       |
|   *_bishop board to locate a [from] square and then      |
|   generate the Attacks_From() bitmap which supplies the  |
|   list of valid <to> squares.                            |
|                                                          |
 ----------------------------------------------------------
*/
    piecebd=Black_Bishops(ply);
    while (piecebd) {
      from=First_One(piecebd);
      moves=And(Attacks_Bishop(from),target);
      temp=from+(bishop<<12);
      while (moves) {
        to=First_One(moves);
        *move++=temp|(to<<6)|(Piece_On_Square(ply,to)<<15);
        Clear(to,moves);
      }
      Clear(from,piecebd);
    }
/*
 ----------------------------------------------------------
|                                                          |
|   now, produce rook moves by cycling through the         |
|   *_rook board to locate a [from] square and then        |
|   generate the Attacks_From() bitmap which supplies the  |
|   list of valid <to> squares.                            |
|                                                          |
 ----------------------------------------------------------
*/
    piecebd=Black_Rooks(ply);
    while (piecebd) {
      from=First_One(piecebd);
      moves=And(Attacks_Rook(from),target);
      temp=from+(rook<<12);
      while (moves) {
        to=First_One(moves);
        *move++=temp|(to<<6)|(Piece_On_Square(ply,to)<<15);
        Clear(to,moves);
      }
      Clear(from,piecebd);
    }
/*
 ----------------------------------------------------------
|                                                          |
|   now, produce queen moves by cycling through the        |
|   *_queen board to locate a [from] square and then       |
|   generate the Attacks_From() bitmap which supplies the  |
|   list of valid <to> squares.                            |
|                                                          |
 ----------------------------------------------------------
*/
    piecebd=Black_Queens(ply);
    while (piecebd) {
      from=First_One(piecebd);
      moves=And(Attacks_Queen(from),target);
      temp=from+(queen<<12);
      while (moves) {
        to=First_One(moves);
        *move++=temp|(to<<6)|(Piece_On_Square(ply,to)<<15);
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
    from=Black_King_SQ(ply);
    moves=And(king_attacks[from],target);
    temp=from+(king<<12);
    while (moves) {
      to=First_One(moves);
      *move++=temp|(to<<6)|(Piece_On_Square(ply,to)<<15);
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
    temp_target=Compl(Or(White_Pieces(ply),
                         Black_Pieces(ply)));
    targets=And(temp_target,target);
    if(generate_captures) targets=Or(targets,And(mask_8,temp_target));
    padvances1=And(Shiftl(Black_Pawns(ply),8),targets);
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
      to=First_One(padvances2);
      *move++=(to+16)|(to<<6)|(pawn<<12);
      Clear(to,padvances2);
    }
    while (padvances1) {
      to=First_One(padvances1);
      if (to > 7)
        *move++=(to+8)|(to<<6)|(pawn<<12);
      else {
        *move++=(to+8)|(to<<6)|(pawn<<12)|(queen<<18);
        if (depth > 0) 
          for (promote=rook;promote>pawn;promote--)
            *move++=(to+8)|(to<<6)|(pawn<<12)|(promote<<18);
      }
      Clear(to,padvances1);
    }
    if (generate_captures) {
      targetc=And(White_Pieces(ply),target);
      targetc=Or(targetc,EnPassant_Target(ply));
      pcapturesl=And(Shiftl(And(Black_Pawns(ply),
                                mask_left_edge),9),targetc);
      pcapturesr=And(Shiftl(And(Black_Pawns(ply),
                                mask_right_edge),7),targetc);
      while (pcapturesl) {
        to=First_One(pcapturesl);
        if (to > 7) {
          if(Piece_On_Square(ply,to)) 
            *move++=(to+9)|(to<<6)|(pawn<<12)|
              (Piece_On_Square(ply,to)<<15);
          else
            *move++=(to+9)|(to<<6)|(pawn<<12)|(pawn<<15);
        }
        else {
          *move++=(to+9)|(to<<6)|(pawn<<12)|
            (Piece_On_Square(ply,to)<<15)|(queen<<18);
          if (depth > 0) 
            for (promote=rook;promote>pawn;promote--)
              *move++=(to+9)|(to<<6)|(pawn<<12)|
                (Piece_On_Square(ply,to)<<15)|(promote<<18);
        }
        Clear(to,pcapturesl);
      }
      while (pcapturesr) {
        to=First_One(pcapturesr);
        if (to > 7) {
          if(Piece_On_Square(ply,to)) 
            *move++=(to+7)|(to<<6)|(pawn<<12)|
              (Piece_On_Square(ply,to)<<15);
          else
            *move++=(to+7)|(to<<6)|(pawn<<12)|(pawn<<15);
        }
        else {
          *move++=(to+7)|(to<<6)|(pawn<<12)|
            (Piece_On_Square(ply,to)<<15)|(queen<<18);
          if (depth > 0) 
            for (promote=rook;promote>pawn;promote--)
              *move++=(to+7)|(to<<6)|(pawn<<12)|
                (Piece_On_Square(ply,to)<<15)|(promote<<18);
        }
        Clear(to,pcapturesr);
      }
    }
  }
  return(move);
}
