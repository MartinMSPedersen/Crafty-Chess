#include <stdio.h>
#include <stdlib.h>
#include "types.h"
#include "function.h"
#include "data.h"
/*
********************************************************************************
*                                                                              *
*   Valid_Move() is used to verify that a move is valid.  it is primarily used *
*   to confirm that a move retrieved from the transposition/refutation table   *
*   is valid in the current position by checking the move against the current  *
*   chess board, castling status, en passant status, etc.                      *
*                                                                              *
********************************************************************************
*/
int Valid_Move(int ply, int wtm, int move)
{
/*
 ----------------------------------------------------------
|                                                          |
|   make sure that the piece on <from> is the right color. |
|                                                          |
 ----------------------------------------------------------
*/
  if (wtm) {
    if (Piece_On_Square(ply,From(move)) != Piece(move)) return(0);
  }
  else {
    if (Piece_On_Square(ply,From(move)) != -Piece(move)) return(0);
  }
      
  switch (Piece(move)) {
    case empty:
      return(0);
/*
 ----------------------------------------------------------
|                                                          |
|   king moves are validated here if the king is moving    |
|   two squares at ont time (castling moves).  otherwise   |
|   fall into the normal piece validation routine below.   |
|   for castling moves, we need to verify that the         |
|   castling status is correct to avoid "creating" a new   |
|   rook.                                                  |
|                                                          |
 ----------------------------------------------------------
*/
    case king:
      if (abs(From(move)-To(move)) == 2) {
        if (wtm) {
          if (To(move) == 2) {
            if ((!(White_Castle(ply)&2)) ||
                And(Or(White_Pieces(ply),
                       Black_Pieces(ply)),Shiftr(mask_3,1)) ||
                And(Attacks_To(2,ply),Black_Pieces(ply)) ||
                And(Attacks_To(3,ply),Black_Pieces(ply)) ||
                And(Attacks_To(4,ply),Black_Pieces(ply))) 
                   return(0);
          }
          else if (To(move) == 6) {
            if ((!(White_Castle(ply)&1)) ||
                And(Or(White_Pieces(ply),
                       Black_Pieces(ply)),Shiftr(mask_2,5)) ||
                And(Attacks_To(4,ply),Black_Pieces(ply)) ||
                And(Attacks_To(5,ply),Black_Pieces(ply)) ||
                And(Attacks_To(6,ply),Black_Pieces(ply))) 
                   return(0);
          }
        }
        else {
          if (To(move) == 58) {
            if ((!(Black_Castle(ply)&2)) ||
                And(Or(White_Pieces(ply),
                    Black_Pieces(ply)),Shiftr(mask_3,57)) ||
                And(Attacks_To(58,ply),White_Pieces(ply)) ||
                And(Attacks_To(59,ply),White_Pieces(ply)) ||
                And(Attacks_To(60,ply),White_Pieces(ply))) 
                   return(0);
          }
          if (To(move) == 62) {
            if ((!(Black_Castle(ply)&1)) ||
                And(Or(White_Pieces(ply),
                    Black_Pieces(ply)),Shiftr(mask_2,61)) ||
                And(Attacks_To(60,ply),White_Pieces(ply)) ||
                And(Attacks_To(61,ply),White_Pieces(ply)) ||
                And(Attacks_To(62,ply),White_Pieces(ply)))
                   return(0);
          }
        }
        return(1);
      }
      break;
/*
 ----------------------------------------------------------
|                                                          |
|   check for a normal pawn advance.                       |
|                                                          |
 ----------------------------------------------------------
*/
    case pawn:
      if (abs(From(move)-To(move)) == 8) {
        if (wtm) {
          if ((Piece(move) == Piece_On_Square(ply,From(move))) &&
              (From(move) < To(move)) &&
              !Piece_On_Square(ply,To(move))) return(1);
        }
        else {
          if ((Piece(move) == -Piece_On_Square(ply,From(move))) &&
              (From(move) > To(move)) &&
              !Piece_On_Square(ply,To(move))) return(1);
        }
        return(0);
      }
      else if (abs(From(move)-To(move)) == 16) {
        if (wtm) {
          if ((Piece(move) == Piece_On_Square(ply,From(move))) &&
              !Piece_On_Square(ply,To(move)-8) &&
              !Piece_On_Square(ply,To(move))) return(1);
        }
        else {
          if ((Piece(move) == -Piece_On_Square(ply,From(move))) &&
              !Piece_On_Square(ply,To(move)+8) &&
              !Piece_On_Square(ply,To(move))) return(1);
        }
        return(0);
      }
      if (!Captured(move)) return(0);
/*
 ----------------------------------------------------------
|                                                          |
|   check for an en passant capture which is somewhat      |
|   unusual in that the [to] square does not contain the   |
|   pawn being captured.  make sure that the pawn being    |
|   captured advanced two ranks the previous move.         |
|                                                          |
 ----------------------------------------------------------
*/
      if (wtm) {
        if ((Piece_On_Square(ply,From(move)) == 1) &&
            (Piece_On_Square(ply,To(move)) == 0) &&
            (Piece_On_Square(ply,To(move)-8) == -1) &&
            And(EnPassant_Target(ply),
                 set_mask[To(move)])) return(1);
      }
      else {
        if ((Piece_On_Square(ply,From(move)) == -1) &&
            (Piece_On_Square(ply,To(move)) == 0) &&
            (Piece_On_Square(ply,To(move)+8) == 1) &&
            And(EnPassant_Target(ply),
                 set_mask[To(move)])) return(1);
      }
/*
 ----------------------------------------------------------
|                                                          |
|   normal moves are all checked the same way.             |
|                                                          |
 ----------------------------------------------------------
*/
    case queen:
    case rook:
    case bishop:
    case knight:
      break;
  }
/*
 ----------------------------------------------------------
|                                                          |
|   all normal moves are validated in the same manner, by  |
|   checking the from and to squares and also the attack   |
|   status for completeness.                               |
|                                                          |
 ----------------------------------------------------------
*/
  if (wtm) {
    if ((Piece(move) == Piece_On_Square(ply,From(move))) &&
        (Captured(move) == -Piece_On_Square(ply,To(move))) &&
        And(Attacks_From(From(move),ply,wtm),
            set_mask[To(move)])) return(1);
  }
  else {
    if ((Piece(move) == -Piece_On_Square(ply,From(move))) &&
        (Captured(move) == Piece_On_Square(ply,To(move))) &&
        And(Attacks_From(From(move),ply,wtm),
            set_mask[To(move)])) return(1);
  }
  return(0);
}
