#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "types.h"
#include "function.h"
#include "data.h"
/*
********************************************************************************
*                                                                              *
*   Set_Board() is used to set up the board in any position desired.  it uses  *
*   a forsythe-like string of characters to describe the board position.       *
*                                                                              *
*   the standard piece codes p,n,b,r,q,k are used to denote the type of piece  *
*   on a square, upper/lower case are used to indicate the side (program/opp.) *
*   of the piece.                                                              *
*                                                                              *
*   the pieces are entered with the rank on the program's side of the board    *
*   entered first, and the rank on the opponent's side entered last.  to enter *
*   empty squares, use a number between 1 and 8 to indicate how many adjacent  *
*   squares are empty.  use a / to terminate each rank after all of the pieces *
*   for that rank have been entered.                                           *
*                                                                              *
*   the following input will setup the board position that given below:        *
*                                                                              *
*         K2R/PPP////q/5ppp/7k/ b                                              *
*                                                                              *
*   this assumes that k represents a white king and -q represents a black      *
*   queen.                                                                     *
*                                                                              *
*                          k  *  *  r  *  *  *  *                              *
*                          p  p  p  *  *  *  *  *                              *
*                          *  *  *  *  *  *  *  *                              *
*                          *  *  *  *  *  *  *  *                              *
*                          *  *  *  *  *  *  *  *                              *
*                         -q  *  *  *  *  *  *  *                              *
*                          *  *  *  *  * -p -p -p                              *
*                          *  *  *  *  *  *  * -k                              *
*                                                                              *
*   the field after the final "/" should be either b or w to indicate which    *
*   side is "on move."  after this side-to-move field any of the following     *
*   characters can appear to indicate the following:  KQ: white can castle     *
*   kingside/queenside/both;  kq: same for black;  a1-h8: indicates the        *
*   square occupied by a pawn that can be captured en passant.                 *
*                                                                              *
********************************************************************************
*/
void Set_Board(void)
{
  int i, match, num, pos, square, tboard[64];
  int bcastle, ep, twtm, wcastle;
  char input[80];
  char bdinfo[22] = {'k','q','r','b','n','p','@','P','N','B','R','Q','K',
                     '1','2','3','4','5','6','7','8','/'};
  char status[13]={'K','Q','k','q','a','b','c','d','e','f','g','h',' '};
  int whichsq, firstsq[8]={56,48,40,32,24,16,8,0};

  fgets(input,80,input_stream);
  if (input_stream != stdin) 
    Print(0,"%s\n",input);
  else
    if (log_file) fprintf(log_file,"%s\n",input);
  input[strlen(input)-1]='\0';
  for (i=0;i<64;i++) tboard[i]=0;
  for (pos=0;(pos<(int) strlen(input)) && (input[pos]==' ');pos++);
/*
 ----------------------------------------------------------
|                                                          |
|   scan the input string searching for pieces, numbers    |
|   [empty squares], slashes [end-of-rank] and a blank     |
|   [end of board, start of castle status].                |
|                                                          |
 ----------------------------------------------------------
*/
  whichsq=0;
  square=firstsq[whichsq];;
  num=0;
  for (;pos<(int) strlen(input);pos++) {
    for (match=0;match<21 && input[pos]!=bdinfo[match];match++);
/*
   " " -> completed this part.
*/
    if (match > 21) {
      strcpy(status,&bdinfo[pos]);
      break;
    }
/*
   "/" -> end of this rank.
*/
    else if (match == 21) {
      num=0;
      square=firstsq[++whichsq];
      if (whichsq > 7) break;
    }
/*
   "1-8" -> empty squares.
*/
    else if (match >= 13) {
      num+=match-12;
      square+=match-12;
      if (num > 8) {
        printf("more than 8 squares on one rank\n");
        return;
      }
      continue;
    }
/*
   piece codes.
*/
    else {
      num++;
      if (num > 8) {
        printf("more than 8 squares on one rank\n");
        return;
      }
      tboard[square++]=match-6;;
    }
  }
/*
 ----------------------------------------------------------
|                                                          |
|   now extract (a) side to move [w/b], (b) castle status  |
|   [KkQq for white/black king-side ok, white/black queen- |
|   side ok], (c) enpassant target square.                 |
|                                                          |
 ----------------------------------------------------------
*/
  twtm=0;
  ep=-1;
  wcastle=0;
  bcastle=0;
  for (pos++;(pos<(int) strlen(input)) && (input[pos]==' ');pos++);
  if (input[pos]=='w') twtm=1;
  else if (input[pos]=='b') twtm=0;
  else printf("side to move is bad\n");
  for (pos++;(pos<(int) strlen(input)) && (input[pos]==' ');pos++);
  for (;pos<(int) strlen(input);pos++) {
    for (match=0;match<13 && input[pos]!=status[match];match++);
    if (!match) wcastle+=1;
    else if (match == 1) wcastle+=2;
    else if (match == 2) bcastle+=1;
    else if (match == 3) bcastle+=2;
    else if ((match > 3) && (match < 12) &&
             (input[pos+1] > '0') && (input[pos+1] < '9')) {
      ep=(input[pos+1]-'1')*8+match-4;
      pos++;
    }
    else if (match == 12) continue;
    else printf("position ok, color/castle/enpassant is bad.\n");
  }
  for (i=0;i<64;i++) Piece_On_Square(0,i)=tboard[i];
  White_Castle(0)=wcastle;
  Black_Castle(0)=bcastle;
  if (ep >= 0) EnPassant_Target(0)=set_mask[ep];
  wtm=twtm;
  Set_Chess_Bit_Boards(&position[0]);
  if (log_file) Display_Chess_Board(log_file,position[0].board);
  if (wtm)
    repetition_head=0;
  else {
    repetition_head=1;
    repetition_list[1]=0;
  }
  for (i=0;i<4096;i++) {
    history_w[i]=0;
    history_b[i]=0;
  }
  for (i=0;i<MAXPLY;i++) {
    killer_move[i][0]=0;
    killer_move[i][1]=0;
    killer_move_count[i][0]=0;
    killer_move_count[i][1]=0;
  }
  last_move_in_book=-100;
}
