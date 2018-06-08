#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "types.h"
#include "function.h"
#include "data.h"
/*
********************************************************************************
*                                                                              *
*   Input_Move() is responsible for converting a move from a text string to    *
*   the internal move format.  it allows the so-called "reduced algebraic      *
*   move format" which makes the origin square optional unless required for    *
*   clarity.  it also accepts as little as required to remove ambiguity from   *
*   the move, by using Generate_Moves() to produce a set of legal moves        *
*   that the text can be applied against to eliminate those moves not          *
*   intended.  hopefully, only one move will remain after the elimination      *
*   and legality checks.                                                       *
*                                                                              *
********************************************************************************
*/
int Input_Move(char *text, int ply, int wtm, int silent)
{
  int moves[200], *mv, *mvp, *goodmove;
  BITBOARD target;
  int piece, capture, promote, give_check;
  int ffile, frank, tfile, trank;
  int current, i, nleft, ambig;
  char *goodchar;
  char movetext[10];
  char pieces[15]={' ',' ','P','p','N','n','B','b',
                   'R','r','Q','q','K','k','\0'};
/*
   check for fully-qualified input (f1e1) and handle if needed.
*/
  if (!silent && strlen(text) == 4) {
    if ((text[0] >= 'a') && (text[0] <= 'h') &&
        (text[1] >= '1') && (text[1] <= '8') &&
        (text[2] >= 'a') && (text[2] <= 'h') &&
        (text[3] >= '1') && (text[3] <= '8'))
    return(Input_Move_ICS(text,ply,wtm,silent));
  }
/*
   initialize move structure in case an error is found
*/
  position[MAXPLY]=position[ply];
  strcpy(movetext,text);
  moves[0]=0;
  piece=0;
  capture=0;
  promote=0;
  give_check=0;
  frank=-1;
  ffile=-1;
  trank=-1;
  tfile=-1;
  ambig=0;
  goodchar=strchr(movetext,'#');
  if (goodchar) *goodchar='\0';
/*
   first, figure out what each character means.  the first thing to
   do is eliminate castling moves
*/
  if (!strcmp(movetext,"o-o") || !strcmp(movetext,"o-o+") ||
      !strcmp(movetext,"O-O") || !strcmp(movetext,"O-O+") ||
      !strcmp(movetext,"0-0") || !strcmp(movetext,"0-0+")) {
    piece=6;
    if(wtm) {
      ffile=4;
      frank=0;
      tfile=6;
      trank=0;
    }
    else {
      ffile=4;
      frank=7;
      tfile=6;
      trank=7;
    }
  }
  else 
    if (!strcmp(movetext,"o-o-o") || !strcmp(movetext,"o-o-o+") ||
		!strcmp(movetext,"O-O-O") || !strcmp(movetext,"O-O-O+") ||
        !strcmp(movetext,"0-0-0") || !strcmp(movetext,"0-0-0+")) {
      piece=6;
      if(wtm) {
        ffile=4;
        frank=0;
        tfile=2;
        trank=0;
      }
      else {
        ffile=4;
        frank=7;
        tfile=2;
        trank=7;
      }
    }
  else {
/*
   ok, it's not a castling.  check for the first two characters of "bb" which
   indicates that the first "b" really means "B" since pawn advances don't
   require a source file.  
*/
    if ((movetext[0] == 'b') && (movetext[1] == 'b')) movetext[0]='B';
/*
   now, start by picking off the check indicator (+) if one is present.
*/
    if (strchr(movetext,'+')) {
      *strchr(movetext,'+')='\0';
      give_check=1;
    }
/*
   now, continue by picking off the promotion piece if one is present.  this 
   is indicated by something like =q on the end of the move string.
*/
    if (strchr(movetext,'=')) {
      goodchar=strchr(movetext,'=');
      goodchar++;
      promote=(strchr(pieces,*goodchar)-pieces) >> 1;
      *strchr(movetext,'=')='\0';
    }
/*
   the next thing to do is extract the last rank/file designators since
   the destination is required.  note that we can have either or both.
*/
    current=strlen(movetext)-1;
    trank=movetext[current]-'1';
    if ((trank >= 0) && (trank <= 7)) 
      movetext[current]='\0';
    else 
      trank=-1;
    current=strlen(movetext)-1;
    tfile=movetext[current]-'a';
    if ((tfile >= 0) || (tfile <= 7)) 
      movetext[current]='\0';
    else
      tfile=-1;
    if (strlen(movetext)) {
/*
   now check the first character to see if it's a piece indicator
   (PpNnBbRrQqKk).  if so, strip it off, unless it's a "b".  since
   this is sometimes unclear as to what it means, we'll try it as both
   a file and a piece unless there are only two characters in the move.
   we simply won't allow b4 to mean a bishop move, it *has* to be a
   pawn.
*/
      if ((movetext[0] != 'b') || ((int) strlen(text) > 2)) {
        if (strchr("  PpNnBbRrQqKk",*movetext)) {
          piece=(strchr(pieces,movetext[0])-pieces) >> 1;
          ambig=(movetext[0] == 'b');
          if (!ambig) 
            for(i=0;i<(int) strlen(movetext);i++) 
              movetext[i]=movetext[i+1];
        }
      }
/*
   now that we have the destination and the moving piece (if any)
   the next step is to see if the last character is now an "x"
   indicating a capture   if so, set the capture flag, remove the
   trailing "x" and continue.
*/
      if ((strlen(movetext)) && (movetext[strlen(movetext)-1] == 'x')) {
        capture=1;
        movetext[strlen(movetext)-1]='\0';
      }
      else
        capture=0;
/*
   now, all that can be left is a rank designator, a file designator
   or both.  if the last character a number, then the first (if present)
   has to be a letter.
*/
      if (strlen(movetext)) {
        ffile=movetext[0]-'a';
        if ((ffile < 0) || (ffile > 7)) {
          ffile=-1;
          frank=movetext[0]-'1';
          if ((frank < 0) || (frank > 7)) piece=-1;
        }
        else {
          if (strlen(movetext) == 2) {
            frank=movetext[1]-'1';
            if ((frank < 0) || (frank > 7)) piece=-1;
          }
        }
      }
    }
  }
  if (!piece) piece=1;
  if (wtm)
    target=Compl(White_Pieces(MAXPLY));
  else
    target=Compl(Black_Pieces(MAXPLY));
  mvp=Generate_Moves(MAXPLY, 1, wtm, target, 1, moves);
  for (mv=&moves[0];mv<mvp;mv++) {
    if (!ambig) {
      if (piece && (Piece(*mv) != piece)) *mv=0;
      if ((ffile >= 0) && ((From(*mv) & 7) != ffile)) *mv=0;
    }
    else {
      if ((Piece(*mv) != bishop) &&
          (Piece(*mv) != pawn)) *mv=0;
      if ((Piece(*mv) != bishop) &&
          ((From(*mv) & 7) != ffile)) *mv=0;
    }
    if (capture && (!Captured(*mv))) *mv=0;
    if (promote && (Promote(*mv) != promote)) *mv=0;
    if ((frank >= 0)  && ((From(*mv) / 8) != frank)) *mv=0;
    if ((tfile >= 0)  && ((To(*mv) & 7) != tfile)) *mv=0;
    if ((trank >= 0)  && ((To(*mv) / 8) != trank)) *mv=0;
    if (*mv) {
      Make_Move(MAXPLY, *mv, wtm);
      if(Check(MAXPLY+1,wtm)) *mv=0;
      if (give_check && Check(MAXPLY+1,wtm)) *mv=0;
    }
  }
  nleft=0;
  for (mv=&moves[0];mv<mvp;mv++) {
    if (*mv) {
      nleft++;
      goodmove=mv;
    }
  }
  if (nleft == 1)
    return(*goodmove);
  if (ambig) {
    if (nleft > 1) {
      for (mv=&moves[0];mv<mvp;mv++) 
        if (Piece(*mv) != pawn) *mv=0;
      nleft=0;
      for (mv=&moves[0];mv<mvp;mv++) {
        if (*mv) {
          nleft++;
          goodmove=mv;
        }
      }
      if (nleft == 1)
        return(*goodmove);
    }
  }
  if (!silent) {
    if (!nleft)
      Print(0,"illegal move.\n");
    else if (piece < 0)
      Print(0,"unrecognizable move.\n");
    else
      Print(0,"move is ambiguous.\n");
  }
  moves[0]=0;
  return(moves[0]);
}
/*
********************************************************************************
*                                                                              *
*   Input_Move_ICS() is responsible for converting a move from the ics format  *
*   [from][to][promote] to the program's internal format.                      *
*                                                                              *
********************************************************************************
*/
int Input_Move_ICS(char *text, int ply, int wtm, int silent)
{
  int moves[200], *mv, *mvp, *goodmove;
  BITBOARD target;
  int piece, promote;
  int ffile, frank, tfile, trank;
  int nleft;
  char movetext[10];
  char pieces[15]={' ',' ','P','p','N','n','B','b',
                   'R','r','Q','q','K','k','\0'};
/*
   initialize move structure in case an error is found
*/
  position[MAXPLY]=position[ply];
  strcpy(movetext,text);
  moves[0]=0;
  promote=0;
/*
   first, figure out what each character means.  the first thing to
   do is eliminate castling moves
*/
  if (!strcmp(movetext,"o-o") || !strcmp(movetext,"O-O")) {
    piece=6;
    if(wtm) {
      ffile=4;
      frank=0;
      tfile=6;
      trank=0;
    }
    else {
      ffile=4;
      frank=7;
      tfile=6;
      trank=7;
    }
  }
  else 
    if (!strcmp(movetext,"o-o-o") || !strcmp(movetext,"O-O-O")) {
      piece=6;
      if(wtm) {
        ffile=4;
        frank=0;
        tfile=2;
        trank=0;
      }
      else {
        ffile=4;
        frank=7;
        tfile=2;
        trank=7;
      }
    }
  else {
/*
   the next thing to do is extract the last rank/file designators since
   the destination is required.  note that we can have either or both.
*/
    ffile=movetext[0]-'a';
    frank=movetext[1]-'1';
    tfile=movetext[2]-'a';
    trank=movetext[3]-'1';
/*
   now, continue by picking off the promotion piece if one is present.  this 
   is indicated by something like q on the end of the move string.
*/
    if ((movetext[4] != '\0') && (movetext[4] != ' ')) {
      promote=(strchr(pieces,movetext[4])-pieces) >> 1;
    }
  }
  if (wtm)
    target=Compl(White_Pieces(MAXPLY));
  else
    target=Compl(Black_Pieces(MAXPLY));
  mvp=Generate_Moves(MAXPLY, 1, wtm, target, 1, moves);
  for (mv=&moves[0];mv<mvp;mv++) {
    if (promote && (Promote(*mv) != promote)) *mv=0;
    if ((frank >= 0)  && ((From(*mv) / 8) != frank)) *mv=0;
    if ((ffile >= 0)  && ((From(*mv) & 7) != ffile)) *mv=0;
    if ((trank >= 0)  && ((To(*mv) / 8) != trank)) *mv=0;
    if ((tfile >= 0)  && ((To(*mv) & 7) != tfile)) *mv=0;
    if (*mv) {
      Make_Move(MAXPLY, *mv, wtm);
      if(Check(MAXPLY+1,wtm)) *mv=0;
    }
  }
  nleft=0;
  for (mv=&moves[0];mv<mvp;mv++) {
    if (*mv) {
      nleft++;
      goodmove=mv;
    }
  }
  if (nleft == 1)
    return(*goodmove);
  if (!silent) {
    if (!nleft)
      Print(0,"illegal move.\n");
    else if (piece < 0)
      Print(0,"unrecognizable move.\n");
    else
      Print(0,"move is ambiguous.\n");
  }
  moves[0]=0;
  return(moves[0]);
}
