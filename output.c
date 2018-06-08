#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "types.h"
#include "function.h"
#include "data.h"

/* last modified 06/03/96 */
/*
********************************************************************************
*                                                                              *
*   OutputMove() is responsible for converting the internal move format to a   *
*   string that can be displayed.  first, it simply converts the from/to       *
*   squares to fully-qualified algebraic (which includes o-o and o-o-o for     *
*   castling moves).  next, we try several "shortcut" forms and call           *
*   input_move(silent=1) to let it silently check the move for uniqueness.     *
*   as soon as we get a non-ambiguous move, we return that text string.        *
*                                                                              *
********************************************************************************
*/
char* OutputMove(int *move, int ply, int wtm)
{
  static char text_move[10];
  int *mvp;
  char new_text[10];
  char *text;
  char piece_names[8] = { ' ','P','N','K',' ','B','R','Q'};

  text=text_move;
/*
   check for null_move first
*/
  if (*move == 0) {
    strcpy(text,"null");
    return(text);
  }
/*
   check for castling first
*/
  if ((Piece(*move) == king) &&
      (abs(From(*move)-To(*move)) == 2)) {
    if (wtm) {
      if (To(*move) == 2) strcpy(text,"o-o-o");
      else strcpy(text,"o-o");
    }
    else {
      if (To(*move) == 58) strcpy(text,"o-o-o");
      else strcpy(text,"o-o");
    }
    return(text);
  }
/*
   not a castling move.  convert to fully-qualified algebraic form
   first.
*/
  text=new_text;
  if ((int) Piece(*move) > pawn) *text++=piece_names[Piece(*move)];
  *text++=(From(*move) & 7)+'a';
  *text++=(From(*move) / 8)+'1';
  if (Captured(*move)) *text++='x';
  *text++=(To(*move) & 7)+'a';
  *text++=(To(*move) / 8)+'1';
  if (Promote(*move)) {
    *text++='=';
    *text++=piece_names[Promote(*move)];
  }
/*
   determine if it's a checking move, or if it's mate.  if so, 
   append "+" or "#" as appropriate.
*/
  position[MAXPLY]=position[ply];
  MakeMove(MAXPLY, *move, wtm);
  if (Check(ChangeSide(wtm))) {
    mvp=GenerateCheckEvasions(1, ChangeSide(wtm), move_list+9000);
    if (mvp == (move_list+9000))
      *text++='#';
    else
      *text++='+';
  }
  UnMakeMove(MAXPLY, *move, wtm);
  *text='\0';
/*
   now, try some short forms.  if the moving piece is a pawn, and
   it is not capturing anything, simply try the destination square
   first: (e2e4->e4)
*/
  if (Piece(*move) == pawn) {
    if (!Captured(*move)) {
      strcpy(text_move,new_text+2);
      if (OutputGood(text_move,ply,wtm)) return (text_move);
    }
/*
   if the move is a pawn capturing a piece, try the short-form for pawn
   capture moves: (e4xf5->exf5)
*/
    text_move[0]=new_text[0];
    strcpy(text_move+1,new_text+2);
    if (OutputGood(text_move,ply,wtm)) return (text_move);
/*
   punt, return the fully-qualified move.
*/
    strcpy(text_move,new_text);
    return (text_move);
  }
/*
   if the move is a piece move, but not a capture, then try the short-
   form for non-capturing moves:  (Ng1f3->Nf3)
*/
  if (!Captured(*move)) {
    text_move[0]=new_text[0];
    strcpy(text_move+1,new_text+3);
    if (OutputGood(text_move,ply,wtm)) return (text_move);
/*
   now try to qualify with the origin file only: (Ng1f3->Ngf3)
*/
    text_move[0]=new_text[0];
    text_move[1]=new_text[1];
    strcpy(text_move+2,new_text+3);
    if (OutputGood(text_move,ply,wtm)) return (text_move);
/*
   now try to qualify with the origin rank only: (Ng1f3->N1f3)
*/
    text_move[0]=new_text[0];
    strcpy(text_move+1,new_text+2);
    if (OutputGood(text_move,ply,wtm)) return (text_move);
/*
   punt, return the fully-qualified move.
*/
    strcpy(text_move,new_text);
    return (text_move);
  }
  else {
/*
   if the move is a piece move, and a capture, then try the short-
   form for capturing moves:  (Ng1xf3->Nxf3)
*/
    text_move[0]=new_text[0];
    strcpy(text_move+1,new_text+3);
    if (OutputGood(text_move,ply,wtm)) return (text_move);
/*
   next, try adding in the origin file: (Ng1xf3->Ngxf3)
*/
    text_move[0]=new_text[0];
    text_move[1]=new_text[1];
    strcpy(text_move+2,new_text+3);
    if (OutputGood(text_move,ply,wtm)) return (text_move);
/*
   next, try adding in the origin rank: (Ng1xf3->N1xf3)
*/
    text_move[0]=new_text[0];
    strcpy(text_move+1,new_text+2);
    if (OutputGood(text_move,ply,wtm)) return (text_move);
/*
   punt, return the fully-qualified move.
*/
    strcpy(text_move,new_text);
    return (text_move);
  }
}

/* last modified 06/03/96 */
/*
********************************************************************************
*                                                                              *
*   OutputMoveICS() is responsible for producing the rather primitive move     *
*   format that the ics interface wants, namely [from][to][promote].           *
*                                                                              *
********************************************************************************
*/
char* OutputMoveICS(int *move)
{
  static char text_move[10];
  char *text;
  char piece_names[8] = { ' ','P','N','K',' ','B','R','Q'};

/*
   convert to fully-qualified algebraic form first.
*/
  text=text_move;
  *text++=(From(*move) & 7)+'a';
  *text++=(From(*move) / 8)+'1';
  *text++=(To(*move) & 7)+'a';
  *text++=(To(*move) / 8)+'1';
  if (Promote(*move))
    *text++=piece_names[Promote(*move)];
  *text='\0';
  return (text_move);
}

int OutputGood(char* text, int ply, int wtm)
{
  int new_move;
  new_move=InputMove(text,ply,wtm,1,0);
  return (Piece(new_move));
}
