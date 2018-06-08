#include <stdio.h>
#include <stdlib.h>
#include "chess.h"
#include "data.h"

/* last modified 05/30/96 */
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
    if (PieceOnSquare(From(move)) != Piece(move)) return(0);
  }
  else {
    if (PieceOnSquare(From(move)) != -Piece(move)) return(0);
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
                And(Occupied,Shiftr(mask_3,1)) ||
                And(AttacksTo(2),BlackPieces) ||
                And(AttacksTo(3),BlackPieces) ||
                And(AttacksTo(4),BlackPieces)) return(0);
          }
          else if (To(move) == 6) {
            if ((!(WhiteCastle(ply)&1)) ||
                And(Occupied,Shiftr(mask_2,5)) ||
                And(AttacksTo(4),BlackPieces) ||
                And(AttacksTo(5),BlackPieces) ||
                And(AttacksTo(6),BlackPieces)) return(0);
          }
        }
        else {
          if (To(move) == 58) {
            if ((!(BlackCastle(ply)&2)) ||
                And(Occupied,Shiftr(mask_3,57)) ||
                And(AttacksTo(58),WhitePieces) ||
                And(AttacksTo(59),WhitePieces) ||
                And(AttacksTo(60),WhitePieces)) return(0);
          }
          if (To(move) == 62) {
            if ((!(BlackCastle(ply)&1)) ||
                And(Occupied,Shiftr(mask_2,61)) ||
                And(AttacksTo(60),WhitePieces) ||
                And(AttacksTo(61),WhitePieces) ||
                And(AttacksTo(62),WhitePieces)) return(0);
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
          if ((Piece(move) == PieceOnSquare(From(move))) &&
              (From(move) < To(move)) && !PieceOnSquare(To(move))) return(1);
        }
        else {
          if ((Piece(move) == -PieceOnSquare(From(move))) &&
              (From(move) > To(move)) && !PieceOnSquare(To(move))) return(1);
        }
        return(0);
      }
      else if (abs(From(move)-To(move)) == 16) {
        if (wtm) {
          if ((Piece(move) == PieceOnSquare(From(move))) &&
              !PieceOnSquare(To(move)-8) && !PieceOnSquare(To(move))) return(1);
        }
        else {
          if ((Piece(move) == -PieceOnSquare(From(move))) &&
              !PieceOnSquare(To(move)+8) && !PieceOnSquare(To(move))) return(1);
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
        if ((PieceOnSquare(From(move)) == pawn) &&
            (PieceOnSquare(To(move)) == 0) &&
            (PieceOnSquare(To(move)-8) == -pawn) &&
            And(EnPassantTarget(ply),set_mask[To(move)])) return(1);
      }
      else {
        if ((PieceOnSquare(From(move)) == -pawn) &&
            (PieceOnSquare(To(move)) == 0) &&
            (PieceOnSquare(To(move)+8) == pawn) &&
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
    if ((Piece(move) == PieceOnSquare(From(move))) &&
        (Captured(move) == -PieceOnSquare(To(move))) &&
        And(AttacksFrom(From(move),wtm),set_mask[To(move)])) return(1);
  }
  else {
    if ((Piece(move) == -PieceOnSquare(From(move))) &&
        (Captured(move) == PieceOnSquare(To(move))) &&
        And(AttacksFrom(From(move),wtm),set_mask[To(move)])) return(1);
  }
  return(0);
}
