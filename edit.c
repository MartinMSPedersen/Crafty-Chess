#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "types.h"
#include "function.h"
#include "data.h"
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
*   c changes (toggles) the color of pieces being placed on the board.         *
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
  char pieces[15]={'x','X','P','p','N','n','B','b',
                   'R','r','Q','q','K','k','\0'};
/*
 ----------------------------------------------------------
|                                                          |
|   next, process the commands to set the board[n] form    |
|   of the chess position.                                 |
|                                                          |
 ----------------------------------------------------------
*/
  while (1) {
    if (input_stream == stdin)
      printf("edit: ");
    fscanf(input_stream,"%s",command);
    if (ics) Print(1,"edit.command:%s\n",command);
    if (!strcmp(command,"white")) {
      wtm=1;
    }
    else if (!strcmp(command,"black")) {
      wtm=0;
    }
    if (!strcmp(command,"#")) {
      for (i=0;i<64;i++)
        Piece_On_Square(0,i)=0;
    }
    else if (!strcmp(command,"c")) {
      wtm=!wtm;
    }
    else if (!strcmp(command,"end") || (!strcmp(command,"."))) {
      break;
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
          Piece_On_Square(0,square)=piece;
        else
          Piece_On_Square(0,square)=-piece;
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
  White_Castle(0)=0;
  Black_Castle(0)=0;
  if (Piece_On_Square(0,4) == 6) {
    if (Piece_On_Square(0,0) == 4)
      White_Castle(0)=White_Castle(0)|2;
    if (Piece_On_Square(0,7) == 4)
      White_Castle(0)=White_Castle(0)|1;
  }
  if (Piece_On_Square(0,60) == -6) {
    if (Piece_On_Square(0,56) == -4)
      Black_Castle(0)=Black_Castle(0)|2;
    if (Piece_On_Square(0,63) == -4)
      Black_Castle(0)=Black_Castle(0)|1;
  }
/*
 ----------------------------------------------------------
|                                                          |
|   basic board is now set.  now it's time to set the bit  |
|   board representation correctly.                        |
|                                                          |
 ----------------------------------------------------------
*/
  Set_Chess_Bit_Boards(&position[0]);
  if (log_file)
    Display_Chess_Board(log_file,position[0].board);
  wtm=1;
  move_number=1;
  if (wtm)
    repetition_head=0;
  else {
    repetition_head=1;
    repetition_list[1]=0;
  }
  last_move_in_book=-100;
}
