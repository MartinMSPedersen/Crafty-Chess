#include <stdio.h>
#include <stdlib.h>
#include "chess.h"
#include "data.h"

/* last modified 02/27/97 */
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
int ValidMove(TREE *tree, int ply, int wtm, int move) {
/*
 ----------------------------------------------------------
|                                                          |
|   make sure that the piece on <from> is the right color. |
|                                                          |
 ----------------------------------------------------------
*/
  if (wtm) {
    if (PcOnSq(From(move)) != Piece(move)) return(0);
  }
  else {
    if (PcOnSq(From(move)) != -Piece(move)) return(0);
  }
      
  switch (Piece(move)) {
    case none:
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
          if (WhiteCastle(ply) > 0) {
            if (To(move) == 2) {
              if ((!(WhiteCastle(ply) & 2)) ||
                  (Occupied & mask_3>>1) ||
                  (AttacksTo(tree,2) & BlackPieces) ||
                  (AttacksTo(tree,3) & BlackPieces) ||
                  (AttacksTo(tree,4) & BlackPieces)) return(0);
            }
            else if (To(move) == 6) {
              if ((!(WhiteCastle(ply) & 1)) ||
                  (Occupied & mask_2>>5) ||
                  (AttacksTo(tree,4) & BlackPieces) ||
                  (AttacksTo(tree,5) & BlackPieces) ||
                  (AttacksTo(tree,6) & BlackPieces)) return(0);
            }
          }
          else return(0);
        }
        else {
          if (BlackCastle(ply) > 0) {
            if (To(move) == 58) {
              if ((!(BlackCastle(ply) & 2)) ||
                  (Occupied & mask_3>>57) ||
                  (AttacksTo(tree,58) & WhitePieces) ||
                  (AttacksTo(tree,59) & WhitePieces) ||
                  (AttacksTo(tree,60) & WhitePieces)) return(0);
            }
            if (To(move) == 62) {
              if ((!(BlackCastle(ply) & 1)) ||
                  (Occupied & mask_2>>61) ||
                  (AttacksTo(tree,60) & WhitePieces) ||
                  (AttacksTo(tree,61) & WhitePieces) ||
                  (AttacksTo(tree,62) & WhitePieces)) return(0);
            }
          }
          else return(0);
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
          if ((Piece(move) == PcOnSq(From(move))) &&
              (From(move) < To(move)) && !PcOnSq(To(move))) return(1);
        }
        else {
          if ((Piece(move) == -PcOnSq(From(move))) &&
              (From(move) > To(move)) && !PcOnSq(To(move))) return(1);
        }
        return(0);
      }
      else if (abs(From(move)-To(move)) == 16) {
        if (wtm) {
          if ((Piece(move) == PcOnSq(From(move))) &&
              !PcOnSq(To(move)-8) && !PcOnSq(To(move))) return(1);
        }
        else {
          if ((Piece(move) == -PcOnSq(From(move))) &&
              !PcOnSq(To(move)+8) && !PcOnSq(To(move))) return(1);
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
        if ((PcOnSq(From(move)) == pawn) &&
            (PcOnSq(To(move)) == 0) &&
            (PcOnSq(To(move)-8) == -pawn) &&
            (EnPassantTarget(ply) &SetMask(To(move)))) return(1);
      }
      else {
        if ((PcOnSq(From(move)) == -pawn) &&
            (PcOnSq(To(move)) == 0) &&
            (PcOnSq(To(move)+8) == pawn) &&
            (EnPassantTarget(ply) &SetMask(To(move)))) return(1);
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
    if (Piece(move) == PcOnSq(From(move)) &&
        Captured(move) == -PcOnSq(To(move)) &&
        Captured(move) != king &&
        (AttacksFrom(tree,From(move),wtm) & SetMask(To(move)))) return(1);
  }
  else {
    if (Piece(move) == -PcOnSq(From(move)) &&
        Captured(move) == PcOnSq(To(move)) &&
        Captured(move) != king &&
        (AttacksFrom(tree,From(move),wtm) & SetMask(To(move)))) return(1);
  }
  return(0);
}
