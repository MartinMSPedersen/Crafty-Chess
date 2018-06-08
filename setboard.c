#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "chess.h"
#include "data.h"

/* last modified 05/13/97 */
/*
********************************************************************************
*                                                                              *
*   SetBoard() is used to set up the board in any position desired.  it uses   *
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
void SetBoard(int nargs, char *args[], int special)
{
  int twtm, i, match, num, pos, square, tboard[64];
  int bcastle, ep, wcastle;
  char input[80];
  static const char bdinfo[] = {'q','r','b','*','k','n','p','*','P','N',
                                'K','*','B','R', 'Q','*','1','2','3','4',
                                '5','6','7','8','/'};
  static const char status[13]={'K','Q','k','q','a','b','c','d','e','f','g',
                                'h',' '};
  int whichsq;
  static const int firstsq[8]={56,48,40,32,24,16,8,0};
  TREE * const tree=local[0];

  if (special)
    strcpy(input,initial_position);
  else
    strcpy(input,args[0]);
  for (i=0;i<64;i++) tboard[i]=0;
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
  square=firstsq[whichsq];
  num=0;
  for (pos=0;pos<(int) strlen(args[0]);pos++) {
    for (match=0;match<25 && args[0][pos]!=bdinfo[match];match++);
    if (match > 24) break;
/*
   "/" -> end of this rank.
*/
    else if (match == 24) {
      num=0;
      square=firstsq[++whichsq];
      if (whichsq > 7) break;
    }
/*
   "1-8" -> empty squares.
*/
    else if (match >= 16) {
      num+=match-15;
      square+=match-15;
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
      if (++num > 8) {
        printf("more than 8 squares on one rank\n");
        return;
      }
      tboard[square++]=match-7;
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
  ep=0;
  wcastle=0;
  bcastle=0;
/*
 ----------------------------------------------------------
|                                                          |
|   side to move.                                          |
|                                                          |
 ----------------------------------------------------------
*/
  if (args[1][0] == 'w') twtm=1;
  else if (args[1][0] == 'b') twtm=0;
  else printf("side to move is bad\n");
/*
 ----------------------------------------------------------
|                                                          |
|   castling/enpassant status.                             |
|                                                          |
 ----------------------------------------------------------
*/
  if (nargs>2 && strlen(args[2])) {
    if (args[2][0]>='a' && args[2][0]<='h' &&
        args[2][1]>'0' && args[2][1]<'9') {
      ep=(args[2][1]-'1')*8+args[2][0]-'a';
      pos++;
    }
    else for (pos=0;pos<(int) strlen(args[2]);pos++) {
      for (match=0;(match<13) && (args[2][pos]!=status[match]);match++);
      if (match == 0) wcastle+=1;
      else if (match == 1) wcastle+=2;
      else if (match == 2) bcastle+=1;
      else if (match == 3) bcastle+=2;
      else if (args[2][0]!='-') printf("castling status is bad.\n");
    }
  }
  if (nargs>3 && strlen(args[3])) {
    if (args[3][0]>='a' && args[3][0]<='h' &&
        args[3][1]>'0' && args[3][1]<'9') {
      ep=(args[3][1]-'1')*8+args[3][0]-'a';
      pos++;
    }
    else if (args[3][0]!='-') printf("enpassant status is bad.\n");
  }
  for (i=0;i<64;i++) PieceOnSquare(i)=tboard[i];
  WhiteCastle(0)=wcastle;
  BlackCastle(0)=bcastle;
  EnPassant(0)=ep;
  Rule50Moves(0)=0;
/*
 ----------------------------------------------------------
|                                                          |
|   now check the castling status and enpassant status to  |
|   make sure that the board is in a state that matches    |
|   these.                                                 |
|                                                          |
 ----------------------------------------------------------
*/
  if (((WhiteCastle(0) & 2) && (PieceOnSquare(A1) != rook)) ||
      ((WhiteCastle(0) & 1) && (PieceOnSquare(H1) != rook)) ||
      ((BlackCastle(0) & 2) && (PieceOnSquare(A8) != -rook)) ||
      ((BlackCastle(0) & 1) && (PieceOnSquare(H8) != -rook))) {
    printf("ERROR-- castling status does not match board position\n");
    InitializeChessBoard(&tree->position[0]);
  }
  if ((twtm && EnPassant(0) && (PieceOnSquare(EnPassant(0)+8) != -pawn) &&
       (PieceOnSquare(EnPassant(0)-7) != pawn) &&
       (PieceOnSquare(EnPassant(0)-9) != pawn)) ||
      (ChangeSide(twtm) && EnPassant(0) && (PieceOnSquare(EnPassant(0)-8) != pawn) &&
       (PieceOnSquare(EnPassant(0)+7) != -pawn) &&
       (PieceOnSquare(EnPassant(0)+9) != -pawn))) {
    EnPassant(0)=0;
  }
  SetChessBitBoards(&tree->position[0]);
  if (log_file) DisplayChessBoard(log_file,tree->pos);
  tree->rephead_b=tree->replist_b;
  tree->rephead_w=tree->replist_w;
  if (twtm) *tree->rephead_w++=HashKey;
  else *tree->rephead_b++=HashKey;
  tree->position[0].rule_50_moves=0;
  if (!special) {
    last_mate_score=0;
    for (i=0;i<4096;i++) {
      history_w[i]=0;
      history_b[i]=0;
    }
    for (i=0;i<MAXPLY;i++) {
      tree->killer_move1[i]=0;
      tree->killer_move2[i]=0;
    }
    last_pv.pathd=0;
    last_pv.pathl=0;
    tree->pv[0].pathd=0;
    tree->pv[0].pathl=0;
    moves_out_of_book=0;
    largest_positional_score=100;
    opening=0;
    middle_game=1;
    end_game=0;
    wtm=twtm;
  }
  if (Check(!wtm)) {
    Print(4095,"ERROR side not on move is in check!\n");
    InitializeChessBoard(&tree->position[0]);
  }
}
