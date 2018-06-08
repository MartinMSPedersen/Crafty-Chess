#include <stdio.h>
#include <stdlib.h>
#include "types.h"
#include "function.h"
#include "data.h"
/*
********************************************************************************
*                                                                              *
*   ValidMove() is used to verify that a move is valid.  it is primarily used  *
*   to confirm that a move retrieved from the transposition/refutation table   *
*   is valid in the current position by checking the move against the current  *
*   chess board, castling status, en passant status, etc.                      *
*                                                                              *
********************************************************************************
*/
int ValidMove(int ply, int wtm, int move)
{
/*
 ----------------------------------------------------------
|                                                          |
|   make sure that the piece on <from> is the right color. |
|                                                          |
 ----------------------------------------------------------
*/
  if (wtm) {
    if (PieceOnSquare(ply,From(move)) != Piece(move)) return(0);
  }
  else {
    if (PieceOnSquare(ply,From(move)) != -Piece(move)) return(0);
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
            if ((!(WhiteCastle(ply)&2)) ||
                And(Or(WhitePieces(ply),BlackPieces(ply)),Shiftr(mask_3,1)) ||
                And(AttacksTo(2,ply),BlackPieces(ply)) ||
                And(AttacksTo(3,ply),BlackPieces(ply)) ||
                And(AttacksTo(4,ply),BlackPieces(ply))) return(0);
          }
          else if (To(move) == 6) {
            if ((!(WhiteCastle(ply)&1)) ||
                And(Or(WhitePieces(ply),BlackPieces(ply)),Shiftr(mask_2,5)) ||
                And(AttacksTo(4,ply),BlackPieces(ply)) ||
                And(AttacksTo(5,ply),BlackPieces(ply)) ||
                And(AttacksTo(6,ply),BlackPieces(ply))) return(0);
          }
        }
        else {
          if (To(move) == 58) {
            if ((!(BlackCastle(ply)&2)) ||
                And(Or(WhitePieces(ply),BlackPieces(ply)),Shiftr(mask_3,57)) ||
                And(AttacksTo(58,ply),WhitePieces(ply)) ||
                And(AttacksTo(59,ply),WhitePieces(ply)) ||
                And(AttacksTo(60,ply),WhitePieces(ply))) return(0);
          }
          if (To(move) == 62) {
            if ((!(BlackCastle(ply)&1)) ||
                And(Or(WhitePieces(ply),BlackPieces(ply)),Shiftr(mask_2,61)) ||
                And(AttacksTo(60,ply),WhitePieces(ply)) ||
                And(AttacksTo(61,ply),WhitePieces(ply)) ||
                And(AttacksTo(62,ply),WhitePieces(ply))) return(0);
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
          if ((Piece(move) == PieceOnSquare(ply,From(move))) &&
              (From(move) < To(move)) && !PieceOnSquare(ply,To(move))) return(1);
        }
        else {
          if ((Piece(move) == -PieceOnSquare(ply,From(move))) &&
              (From(move) > To(move)) && !PieceOnSquare(ply,To(move))) return(1);
        }
        return(0);
      }
      else if (abs(From(move)-To(move)) == 16) {
        if (wtm) {
          if ((Piece(move) == PieceOnSquare(ply,From(move))) &&
              !PieceOnSquare(ply,To(move)-8) && !PieceOnSquare(ply,To(move))) return(1);
        }
        else {
          if ((Piece(move) == -PieceOnSquare(ply,From(move))) &&
              !PieceOnSquare(ply,To(move)+8) && !PieceOnSquare(ply,To(move))) return(1);
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
        if ((PieceOnSquare(ply,From(move)) == pawn) &&
            (PieceOnSquare(ply,To(move)) == 0) &&
            (PieceOnSquare(ply,To(move)-8) == -pawn) &&
            And(EnPassantTarget(ply),set_mask[To(move)])) return(1);
      }
      else {
        if ((PieceOnSquare(ply,From(move)) == -pawn) &&
            (PieceOnSquare(ply,To(move)) == 0) &&
            (PieceOnSquare(ply,To(move)+8) == pawn) &&
            And(EnPassantTarget(ply),set_mask[To(move)])) return(1);
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
    if ((Piece(move) == PieceOnSquare(ply,From(move))) &&
        (Captured(move) == -PieceOnSquare(ply,To(move))) &&
        And(AttacksFrom(From(move),ply,wtm),set_mask[To(move)])) return(1);
  }
  else {
    if ((Piece(move) == -PieceOnSquare(ply,From(move))) &&
        (Captured(move) == PieceOnSquare(ply,To(move))) &&
        And(AttacksFrom(From(move),ply,wtm),set_mask[To(move)])) return(1);
  }
  return(0);
}
