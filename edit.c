#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "types.h"
#include "function.h"
#include "data.h"

/* last modified 07/12/96 */
/*
********************************************************************************
*                                                                              *
*   Edit() is used to edit (alter) the current board position.  it clears the  *
*   board and then allows the operator to enter a position using one of four   *
*                                                                              *
*   white sets the color of pieces added to white.  this color will stay in    *
*   effect until specifically changed.                                         *
*                                                                              *
*   black sets the color of pieces added to black.  this color will stay in    *
*   effect until specifically changed.                                         *
*                                                                              *
*   # clears the chessboard completely.                                        *
*                                                                              *
*   c changes (toggles) the color of pieces being placed on the          *
*                                                                              *
*   end (or . for ICS/Xboard) terminates Edit().                               *
*                                                                              *
*   pieces are placed on the board by three character "commands" of the form   *
*   [piece][square] where piece is a member of the normal set of pieces        *
*   {P,N,B,R,Q,K} and [square] is algebraic square notation (a1-h8).  ex: Ke8  *
*   puts a king (of the current "color") on square e8.                         *
*                                                                              *
********************************************************************************
*/
void Edit(void)
{
  char command[80];
  int i, tfile, trank, square, piece;
  char pieces[]={'x','X','P','p','N','n','K','k','*','*',
                   'B','b','R','r','Q','q','\0'};
/*
 ----------------------------------------------------------
|                                                          |
|   next, process the commands to set the board[n] form    |
|   of the chess position.                                 |
|                                                          |
 ----------------------------------------------------------
*/
  while (1) {
    if ((input_stream == stdin) && !xboard)
      if (wtm)
        printf("edit(white): ");
      else
        printf("edit(black): ");
    fscanf(input_stream,"%s",command);
    if (xboard) Print(1,"edit.command:%s\n",command);
    if (!strcmp(command,"white")) {
      wtm=1;
    }
    else if (!strcmp(command,"black")) {
      wtm=0;
    }
    if (!strcmp(command,"#")) {
      for (i=0;i<64;i++)
        PieceOnSquare(i)=0;
    }
    else if (!strcmp(command,"c")) {
      wtm=ChangeSide(wtm);
    }
    else if (!strcmp(command,"end") || (!strcmp(command,"."))) {
      break;
    }
    else if (!strcmp(command,"d")) {
      DisplayChessBoard(stdout,search);
    }
    else {
      if (strchr(pieces,command[0])) {
        piece=(strchr(pieces,command[0])-pieces) >> 1;
        tfile=command[1]-'a';
        trank=command[2]-'1';
        square=(trank<<3)+tfile;
        if ((square < 0) || (square > 63))
          printf("unrecognized square %s\n",command);
        if (wtm)
          PieceOnSquare(square)=piece;
        else
          PieceOnSquare(square)=-piece;
      }
      else {
        printf("unrecognized piece %s\n",command);
      }
    }
  }
/*
 ----------------------------------------------------------
|                                                          |
|   now, if a king is on its original square, check the    |
|   rooks to see if they are and set the castle status     |
|   accordingly.                                           |
|                                                          |
 ----------------------------------------------------------
*/
  WhiteCastle(0)=0;
  BlackCastle(0)=0;
  if (PieceOnSquare(4) == king) {
    if (PieceOnSquare(0) == rook)
      WhiteCastle(0)=WhiteCastle(0)|2;
    if (PieceOnSquare(7) == rook)
      WhiteCastle(0)=WhiteCastle(0)|1;
  }
  if (PieceOnSquare(60) == -king) {
    if (PieceOnSquare(56) == -rook)
      BlackCastle(0)=BlackCastle(0)|2;
    if (PieceOnSquare(63) == -rook)
      BlackCastle(0)=BlackCastle(0)|1;
  }
/*
 ----------------------------------------------------------
|                                                          |
|   basic board is now set.  now it's time to set the bit  |
|   board representation correctly.                        |
|                                                          |
 ----------------------------------------------------------
*/
  SetChessBitBoards(&position[0]);
  if (log_file) DisplayChessBoard(log_file,search);
  wtm=1;
  move_number=1;
  repetition_head_b=repetition_list_b;
  repetition_head_w=repetition_list_w;
  position[0].rule_50_moves=0;
  last_move_in_book=move_number;
}
