#include <stdio.h>
#include <stdlib.h>
#include "types.h"
#include "function.h"
#include "data.h"
#include "evaluate.h"
/*
********************************************************************************
*                                                                              *
*   Make_Move() is responsible for updating the position database whenever a   *
*   piece is moved.  it performs the following operations:  (1) update the     *
*   board structure itself by moving the piece and removing any captured       *
*   piece.  (2) update the hash keys.  (3) update material counts.  (4) update *
*   castling status.  (5) update number of moves since last reversible move.   *
*                                                                              *
********************************************************************************
*/
static BITBOARD bit_move;
void Make_Move(int ply, int move, int wtm)
{
/*
 ----------------------------------------------------------
|                                                          |
|   first, clear the EnPassant_Target bit-mask.  moving a  |
|   pawn two ranks will set it later in Make_Move().       |
|                                                          |
 ----------------------------------------------------------
*/
  position[ply+1]=position[ply];
  EnPassant_Target(ply+1)=0;
/*
 ----------------------------------------------------------
|                                                          |
|   now do the piece-specific things by calling the        |
|   appropriate routine.                                   |
|                                                          |
 ----------------------------------------------------------
*/
  bit_move=Or(set_mask[From(move)],set_mask[To(move)]);
  switch (Piece(move)) {
  case pawn:
    Make_Move_Pawn(ply,From(move), To(move), Captured(move), 
                   Promote(move), wtm);
    if (Captured(move) == 1) {
      if (wtm) {
        if (!And(Black_Pawns(ply),set_mask[To(move)]))
          move&=~(7<<15);
      }
      else {
        if (!And(White_Pawns(ply),set_mask[To(move)]))
          move&=~(7<<15);
      }
    }
    position[ply+1].moves_since_cap_or_push=0;
    break;
  case knight:
    Make_Move_Knight(ply,From(move), To(move), wtm);
    break;
  case bishop:
    Make_Move_Bishop(ply,From(move), To(move), wtm);
    break;
  case rook:
    Make_Move_Rook(ply,From(move), To(move), wtm);
    break;
  case queen:
    Make_Move_Queen(ply,From(move), To(move), wtm);
    break;
  case king:
    Make_Move_King(ply,From(move), To(move), wtm);
    break;
  }
/*
********************************************************************************
*                                                                              *
*   now it is time to "gracefully" remove a piece from the game board since it *
*   is being captured.  this includes updating the board structure.            *
*                                                                              *
********************************************************************************
*/
  if(Captured(move)) {
    position[ply+1].moves_since_cap_or_push=0;
    if (Promote(move)) move=(move&(~(7<<12)))|(Promote(move)<<12);
    switch (Captured(move)) {
/*
 ----------------------------------------------------------
|                                                          |
|   remove a captured pawn.                                |
|                                                          |
 ----------------------------------------------------------
*/
    case pawn: 
      if (wtm) {
        Clear(To(move),Black_Pawns(ply+1));
        Clear(To(move),Black_Pieces(ply+1));
        Hash_Pb(To(move),Hash_Key(ply+1));
        Hash_Pb(To(move),Pawn_Hash_Key(ply+1));
        Material(ply+1)+=PAWN_VALUE;
        Total_Black_Pawns(ply+1)--;
      }
      else {
        Clear(To(move),White_Pawns(ply+1));
        Clear(To(move),White_Pieces(ply+1));
        Hash_Pw(To(move),Hash_Key(ply+1));
        Hash_Pw(To(move),Pawn_Hash_Key(ply+1));
        Material(ply+1)-=PAWN_VALUE;
        Total_White_Pawns(ply+1)--;
      }
    break;
/*
 ----------------------------------------------------------
|                                                          |
|   remove a captured knight.                              |
|                                                          |
 ----------------------------------------------------------
*/
    case knight: 
      if (wtm) {
        Clear(To(move),Black_Knights(ply+1));
        Clear(To(move),Black_Pieces(ply+1));
        Hash_Nb(To(move),Hash_Key(ply+1));
        Total_Black_Pieces(ply+1)-=3;
        Material(ply+1)+=KNIGHT_VALUE;
      }
      else {
        Clear(To(move),White_Knights(ply+1));
        Clear(To(move),White_Pieces(ply+1));
        Hash_Nw(To(move),Hash_Key(ply+1));
        Total_White_Pieces(ply+1)-=3;
        Material(ply+1)-=KNIGHT_VALUE;
      }
    break;
/*
 ----------------------------------------------------------
|                                                          |
|   remove a captured bishop.                              |
|                                                          |
 ----------------------------------------------------------
*/
    case bishop: 
      if ((Piece(move) == bishop) || (Piece(move) == queen))
        Set(To(move),Bishops_Queens(ply+1));
      else
        Clear(To(move),Bishops_Queens(ply+1));
      if (wtm) {
        Clear(To(move),Black_Bishops(ply+1));
        Clear(To(move),Black_Pieces(ply+1));
        Hash_Bb(To(move),Hash_Key(ply+1));
        Total_Black_Pieces(ply+1)-=3;
        Material(ply+1)+=BISHOP_VALUE;
      }
      else {
        Clear(To(move),White_Bishops(ply+1));
        Clear(To(move),White_Pieces(ply+1));
        Hash_Bw(To(move),Hash_Key(ply+1));
        Total_White_Pieces(ply+1)-=3;
        Material(ply+1)-=BISHOP_VALUE;
      }
    break;
/*
 ----------------------------------------------------------
|                                                          |
|   remove a captured rook.                                |
|                                                          |
 ----------------------------------------------------------
*/
    case rook: 
      if ((Piece(move) == rook) || (Piece(move) == queen))
        Set(To(move),Rooks_Queens(ply+1));
      else
        Clear(To(move),Rooks_Queens(ply+1));
      if (wtm) {
        Clear(To(move),Black_Rooks(ply+1));
        Clear(To(move),Black_Pieces(ply+1));
        Hash_Rb(To(move),Hash_Key(ply+1));
        if (To(move) == 56) Black_Castle(ply+1)&=1;
        if (To(move) == 63) Black_Castle(ply+1)&=2;
        Total_Black_Pieces(ply+1)-=5;
        Material(ply+1)+=ROOK_VALUE;
      }
      else {
        Clear(To(move),White_Rooks(ply+1));
        Clear(To(move),White_Pieces(ply+1));
        Hash_Rw(To(move),Hash_Key(ply+1));
        if (To(move) == 0) White_Castle(ply+1)&=1;
        if (To(move) == 7) White_Castle(ply+1)&=2;
        Total_White_Pieces(ply+1)-=5;
        Material(ply+1)-=ROOK_VALUE;
      }
    break;
/*
 ----------------------------------------------------------
|                                                          |
|   remove a captured queen.                               |
|                                                          |
 ----------------------------------------------------------
*/
    case queen: 
      if (Piece(move) == queen) {
        Set(To(move),Bishops_Queens(ply+1));
        Set(To(move),Rooks_Queens(ply+1));
      }
      else if (Piece(move) == rook) {
        Set(To(move),Rooks_Queens(ply+1));
        Clear(To(move),Bishops_Queens(ply+1));
      }
      else if (Piece(move) == bishop) {
        Set(To(move),Bishops_Queens(ply+1));
        Clear(To(move),Rooks_Queens(ply+1));
      }
      else {
        Clear(To(move),Bishops_Queens(ply+1));
        Clear(To(move),Rooks_Queens(ply+1));
      }
/*
      if ((Piece(move) == bishop) || (Piece(move) == queen))
        Set(To(move),Bishops_Queens(ply+1));
      else
        Clear(To(move),Bishops_Queens(ply+1));
      if ((Piece(move) == rook) || (Piece(move) == queen))
        Set(To(move),Rooks_Queens(ply+1));
      else
        Clear(To(move),Rooks_Queens(ply+1));
*/
      if (wtm) {
        Clear(To(move),Black_Queens(ply+1));
        Clear(To(move),Black_Pieces(ply+1));
        Hash_Qb(To(move),Hash_Key(ply+1));
        Total_Black_Pieces(ply+1)-=9;
        Material(ply+1)+=QUEEN_VALUE;
      }
      else {
        Clear(To(move),White_Queens(ply+1));
        Clear(To(move),White_Pieces(ply+1));
        Hash_Qw(To(move),Hash_Key(ply+1));
        Total_White_Pieces(ply+1)-=9;
        Material(ply+1)-=QUEEN_VALUE;
      }
      break;
/*
 ----------------------------------------------------------
|                                                          |
|   remove a captured king. [this is an error condition]   |
|                                                          |
 ----------------------------------------------------------
*/
    case king: 
      Print(1,"captured a king\n");
      Print(1,"piece=%d,from=%d,to=%d,captured=%d\n",
            Piece(move),From(move),
            To(move),Captured(move));
      Print(1,"ply=%d\n",ply);
      if (log_file) Display_Chess_Board(log_file,position[ply].board);
      exit(101);
    }
  }
  position[ply+1].moves_since_cap_or_push++;
  return;
}

/*
********************************************************************************
*                                                                              *
*   make bishop moves.                                                         *
*                                                                              *
********************************************************************************
*/
void Make_Move_Bishop(int ply, int from, int to, int wtm)
{
/*
 --------------------------------------------------------------------
|                                                                    |
|  first, update the occupied-square bitboards, of which there are   |
|  several.                                                          |
|                                                                    |
 --------------------------------------------------------------------
*/
  Clear_Set(bit_move,Bishops_Queens(ply+1));
  Clear_rl90(from,Occupied_RL90(ply+1));
  Clear_rl45(from,Occupied_RL45(ply+1));
  Clear_rr45(from,Occupied_RR45(ply+1));
  Set_rl90(to,Occupied_RL90(ply+1));
  Set_rl45(to,Occupied_RL45(ply+1));
  Set_rr45(to,Occupied_RR45(ply+1));
  if (wtm) {
    Clear_Set(bit_move,White_Bishops(ply+1));
    Clear_Set(bit_move,White_Pieces(ply+1));
    Hash_Bw(from,Hash_Key(ply+1));
    Hash_Bw(to,Hash_Key(ply+1));
    Piece_On_Square(ply+1,from)=0;
    Piece_On_Square(ply+1,to)=bishop;
  }
  else {
    Clear_Set(bit_move,Black_Bishops(ply+1));
    Clear_Set(bit_move,Black_Pieces(ply+1));
    Hash_Bb(from,Hash_Key(ply+1));
    Hash_Bb(to,Hash_Key(ply+1));
    Piece_On_Square(ply+1,from)=0;
    Piece_On_Square(ply+1,to)=-bishop;
  }
}

/*
********************************************************************************
*                                                                              *
*   make king moves.  the only special case is castling, which is indicated    *
*   by from=4, to=6 for o-o as an example.  the king is moving from e1-g1      *
*   which is normally illegal.  in this case, the correct rook is also moved.  *
*                                                                              *
*   note that moving the king in any direction resets the x_castle [x=w or b]  *
*   flag indicating that castling is no int possible in *this* position.    *
*                                                                              *
********************************************************************************
*/
void Make_Move_King(int ply, int from, int to, int wtm)
{
/*
 --------------------------------------------------------------------
|                                                                    |
|  first, update the occupied-square bitboards, of which there are   |
|  several.                                                          |
|                                                                    |
 --------------------------------------------------------------------
*/
  Clear_rl90(from,Occupied_RL90(ply+1));
  Clear_rl45(from,Occupied_RL45(ply+1));
  Clear_rr45(from,Occupied_RR45(ply+1));
  Set_rl90(to,Occupied_RL90(ply+1));
  Set_rl45(to,Occupied_RL45(ply+1));
  Set_rr45(to,Occupied_RR45(ply+1));
  if (wtm) {
    Clear_Set(bit_move,White_King(ply+1));
    Clear_Set(bit_move,White_Pieces(ply+1));
    Hash_Kw(from,Hash_Key(ply+1));
    Hash_Kw(to,Hash_Key(ply+1));
    Piece_On_Square(ply+1,from)=0;
    Piece_On_Square(ply+1,to)=king;
    White_Castle(ply+1)=0;
    White_King_SQ(ply+1)=to;
    if (abs(to-from) == 2)
      if (to == 6) {
        bit_move=Or(set_mask[5],set_mask[7]);
        Make_Move_Rook(ply,7,5,wtm);
      }
      else {
        bit_move=Or(set_mask[0],set_mask[3]);
        Make_Move_Rook(ply,0,3,wtm);
      }
  }
  else {
    Clear_Set(bit_move,Black_King(ply+1));
    Clear_Set(bit_move,Black_Pieces(ply+1));
    Hash_Kb(from,Hash_Key(ply+1));
    Hash_Kb(to,Hash_Key(ply+1));
    Piece_On_Square(ply+1,from)=0;
    Piece_On_Square(ply+1,to)=-king;
    Black_King_SQ(ply+1)=to;
    Black_Castle(ply+1)=0;
    if (abs(to-from) == 2)
      if (to == 62) {
        bit_move=Or(set_mask[61],set_mask[63]);
        Make_Move_Rook(ply,63,61,wtm);
      }
      else {
        bit_move=Or(set_mask[56],set_mask[59]);
        Make_Move_Rook(ply,56,59,wtm);
      }
  }
}

/*
********************************************************************************
*                                                                              *
*   make knight moves.                                                         *
*                                                                              *
********************************************************************************
*/
void Make_Move_Knight(int ply, int from, int to, int wtm)
{
/*
 --------------------------------------------------------------------
|                                                                    |
|  first, update the occupied-square bitboards, of which there are   |
|  several.                                                          |
|                                                                    |
 --------------------------------------------------------------------
*/
  Clear_rl90(from,Occupied_RL90(ply+1));
  Clear_rl45(from,Occupied_RL45(ply+1));
  Clear_rr45(from,Occupied_RR45(ply+1));
  Set_rl90(to,Occupied_RL90(ply+1));
  Set_rl45(to,Occupied_RL45(ply+1));
  Set_rr45(to,Occupied_RR45(ply+1));
  if (wtm) {
    Clear_Set(bit_move,White_Knights(ply+1));
    Clear_Set(bit_move,White_Pieces(ply+1));
    Hash_Nw(from,Hash_Key(ply+1));
    Hash_Nw(to,Hash_Key(ply+1));
    Piece_On_Square(ply+1,from)=0;
    Piece_On_Square(ply+1,to)=knight;
  }
  else {
    Clear_Set(bit_move,Black_Knights(ply+1));
    Clear_Set(bit_move,Black_Pieces(ply+1));
    Hash_Nb(from,Hash_Key(ply+1));
    Hash_Nb(to,Hash_Key(ply+1));
    Piece_On_Square(ply+1,from)=0;
    Piece_On_Square(ply+1,to)=-knight;
  }
}

/*
********************************************************************************
*                                                                              *
*   make pawn moves.  there are two special cases:  (a) enpassant captures     *
*   where the captured pawn is not on the "to" square and must be removed in   *
*   a different way, and (2) pawn promotions (where the "Promote" variable  *
*   is non-zero) requires updating the appropriate bit boards since we are     *
*   creating a new piece.                                                      *
*                                                                              *
********************************************************************************
*/
void Make_Move_Pawn(int ply, int from, int to, int Captured,
                    int Promote, int wtm)
{
/*
 --------------------------------------------------------------------
|                                                                    |
|  now, update the occupied-square bitboards, of which there are     |
|  several.                                                          |
|                                                                    |
 --------------------------------------------------------------------
*/
  Clear_rl90(from,Occupied_RL90(ply+1));
  Clear_rl45(from,Occupied_RL45(ply+1));
  Clear_rr45(from,Occupied_RR45(ply+1));
  Set_rl90(to,Occupied_RL90(ply+1));
  Set_rl45(to,Occupied_RL45(ply+1));
  Set_rr45(to,Occupied_RR45(ply+1));
  if (wtm) {
    Clear_Set(bit_move,White_Pawns(ply+1));
    Clear_Set(bit_move,White_Pieces(ply+1));
    Hash_Pw(from,Hash_Key(ply+1));
    Hash_Pw(from,Pawn_Hash_Key(ply+1));
    Hash_Pw(to,Hash_Key(ply+1));
    Hash_Pw(to,Pawn_Hash_Key(ply+1));
    Piece_On_Square(ply+1,from)=0;
    Piece_On_Square(ply+1,to)=pawn;
    if (Captured == 1) {
      if(!And(Black_Pawns(ply+1),set_mask[to])) {
        Clear_rl90(to-8,Occupied_RL90(ply+1));
        Clear_rl45(to-8,Occupied_RL45(ply+1));
        Clear_rr45(to-8,Occupied_RR45(ply+1));
        Clear(to-8,Black_Pawns(ply+1));
        Clear(to-8,Black_Pieces(ply+1));
        Hash_Pb(to-8,Hash_Key(ply+1));
        Hash_Pb(to-8,Pawn_Hash_Key(ply+1));
        Piece_On_Square(ply+1,to-8)=0;
        Material(ply+1)+=PAWN_VALUE;
        Total_Black_Pawns(ply+1)--;
      }
    }
/*
 --------------------------------------------------------------------
|                                                                    |
|  if this is a pawn promotion, remove the pawn from the pawn board. |
|  then update the correct piece board to reflect the piece just     |
|  created.                                                          |
|                                                                    |
 --------------------------------------------------------------------
*/
    if (Promote) {
      Total_White_Pawns(ply+1)--;
      Material(ply+1)-=PAWN_VALUE;
      Clear(to,White_Pawns(ply+1));
      Hash_Pw(to,Hash_Key(ply+1));
      Hash_Pw(to,Pawn_Hash_Key(ply+1));
      switch (Promote) {
      case knight:
        Set(to,White_Knights(ply+1));
        Hash_Nw(to,Hash_Key(ply+1));
        Piece_On_Square(ply+1,to)=knight;
        Total_White_Pieces(ply+1)+=3;
        Material(ply+1)+=KNIGHT_VALUE;
        break;
      case bishop:
        Set(to,White_Bishops(ply+1));
        Set(to,Bishops_Queens(ply+1));
        Hash_Bw(to,Hash_Key(ply+1));
        Piece_On_Square(ply+1,to)=bishop;
        Total_White_Pieces(ply+1)+=3;
        Material(ply+1)+=BISHOP_VALUE;
        break;
      case rook:
        Set(to,White_Rooks(ply+1));
        Set(to,Rooks_Queens(ply+1));
        Hash_Rw(to,Hash_Key(ply+1));
        Piece_On_Square(ply+1,to)=rook;
        Total_White_Pieces(ply+1)+=5;
        Material(ply+1)+=ROOK_VALUE;
        break;
      case queen:
        Set(to,White_Queens(ply+1));
        Set(to,Bishops_Queens(ply+1));
        Set(to,Rooks_Queens(ply+1));
        Hash_Qw(to,Hash_Key(ply+1));
        Piece_On_Square(ply+1,to)=queen;
        Total_White_Pieces(ply+1)+=9;
        Material(ply+1)+=QUEEN_VALUE;
        break;
      }
    }
    else 
      if ((to-from == 16) && And(mask_enpassant_test[to],Black_Pawns(ply+1)))
        EnPassant_Target(ply+1)=set_mask[to-8];
  }
  else {
    Clear_Set(bit_move,Black_Pawns(ply+1));
    Clear_Set(bit_move,Black_Pieces(ply+1));
    Hash_Pb(from,Hash_Key(ply+1));
    Hash_Pb(from,Pawn_Hash_Key(ply+1));
    Hash_Pb(to,Hash_Key(ply+1));
    Hash_Pb(to,Pawn_Hash_Key(ply+1));
    Piece_On_Square(ply+1,from)=0;
    Piece_On_Square(ply+1,to)=-pawn;
    if (Captured == 1) {
      if(!And(White_Pawns(ply+1),set_mask[to])) {
        Clear_rl90(to+8,Occupied_RL90(ply+1));
        Clear_rl45(to+8,Occupied_RL45(ply+1));
        Clear_rr45(to+8,Occupied_RR45(ply+1));
        Clear(to+8,White_Pawns(ply+1));
        Clear(to+8,White_Pieces(ply+1));
        Hash_Pw(to+8,Hash_Key(ply+1));
        Hash_Pw(to+8,Pawn_Hash_Key(ply+1));
        Piece_On_Square(ply+1,to+8)=0;
        Material(ply+1)-=PAWN_VALUE;
        Total_White_Pawns(ply+1)--;
      }
    }
/*
 --------------------------------------------------------------------
|                                                                    |
|  if this is a pawn promotion, remove the pawn from the pawn board. |
|  then update the correct piece board to reflect the piece just     |
|  created.                                                          |
|                                                                    |
 --------------------------------------------------------------------
*/
    if (Promote) {
      Total_Black_Pawns(ply+1)--;
      Material(ply+1)+=PAWN_VALUE;
      Clear(to,Black_Pawns(ply+1));
      Hash_Pb(to,Hash_Key(ply+1));
      Hash_Pb(to,Pawn_Hash_Key(ply+1));
      switch (Promote) {
      case knight:
        Set(to,Black_Knights(ply+1));
        Hash_Nb(to,Hash_Key(ply+1));
        Piece_On_Square(ply+1,to)=-knight;
        Total_Black_Pieces(ply+1)+=3;
        Material(ply+1)-=KNIGHT_VALUE;
        break;
      case bishop:
        Set(to,Black_Bishops(ply+1));
        Set(to,Bishops_Queens(ply+1));
        Hash_Bb(to,Hash_Key(ply+1));
        Piece_On_Square(ply+1,to)=-bishop;
        Total_Black_Pieces(ply+1)+=3;
        Material(ply+1)-=BISHOP_VALUE;
        break;
      case rook:
        Set(to,Black_Rooks(ply+1));
        Set(to,Rooks_Queens(ply+1));
        Hash_Rb(to,Hash_Key(ply+1));
        Piece_On_Square(ply+1,to)=-rook;
        Total_Black_Pieces(ply+1)+=5;
        Material(ply+1)-=ROOK_VALUE;
        break;
      case queen:
        Set(to,Black_Queens(ply+1));
        Set(to,Bishops_Queens(ply+1));
        Set(to,Rooks_Queens(ply+1));
        Hash_Qb(to,Hash_Key(ply+1));
        Piece_On_Square(ply+1,to)=-queen;
        Total_Black_Pieces(ply+1)+=9;
        Material(ply+1)-=QUEEN_VALUE;
        break;
      }
    }
    else 
      if ((from-to == 16) && And(mask_enpassant_test[to],White_Pawns(ply+1)))
        EnPassant_Target(ply+1)=set_mask[to+8];
  }
}

/*
********************************************************************************
*                                                                              *
*   make queen moves                                                           *
*                                                                              *
********************************************************************************
*/
void Make_Move_Queen(int ply, int from, int to, int wtm)
{
/*
 --------------------------------------------------------------------
|                                                                    |
|  first, update the occupied-square bitboards, of which there are   |
|  several.                                                          |
|                                                                    |
 --------------------------------------------------------------------
*/
  Clear_Set(bit_move,Bishops_Queens(ply+1));
  Clear_Set(bit_move,Rooks_Queens(ply+1));
  Clear_rl90(from,Occupied_RL90(ply+1));
  Clear_rl45(from,Occupied_RL45(ply+1));
  Clear_rr45(from,Occupied_RR45(ply+1));
  Set_rl90(to,Occupied_RL90(ply+1));
  Set_rl45(to,Occupied_RL45(ply+1));
  Set_rr45(to,Occupied_RR45(ply+1));
  if (wtm) {
    Clear_Set(bit_move,White_Queens(ply+1));
    Clear_Set(bit_move,White_Pieces(ply+1));
    Hash_Qw(from,Hash_Key(ply+1));
    Hash_Qw(to,Hash_Key(ply+1));
    Piece_On_Square(ply+1,from)=0;
    Piece_On_Square(ply+1,to)=queen;
  }
  else {
    Clear_Set(bit_move,Black_Queens(ply+1));
    Clear_Set(bit_move,Black_Pieces(ply+1));
    Hash_Qb(from,Hash_Key(ply+1));
    Hash_Qb(to,Hash_Key(ply+1));
    Piece_On_Square(ply+1,from)=0;
    Piece_On_Square(ply+1,to)=-queen;
  }
}

/*
********************************************************************************
*                                                                              *
*   make rook moves.  the only special case handling required is to determine  *
*   if x_castle is non-zero [x=w or b based on side to move].  if it is non-   *
*   zero, the value must be corrected if either rook is moving from its        *
*   original square, so that castling with that rook becomes impossible.       *
*                                                                              *
********************************************************************************
*/
void Make_Move_Rook(int ply, int from, int to, int wtm)
{
/*
 --------------------------------------------------------------------
|                                                                    |
|  first, update the occupied-square bitboards, of which there are   |
|  several.                                                          |
|                                                                    |
 --------------------------------------------------------------------
*/
  Clear_Set(bit_move,Rooks_Queens(ply+1));
  Clear_rl90(from,Occupied_RL90(ply+1));
  Clear_rl45(from,Occupied_RL45(ply+1));
  Clear_rr45(from,Occupied_RR45(ply+1));
  Set_rl90(to,Occupied_RL90(ply+1));
  Set_rl45(to,Occupied_RL45(ply+1));
  Set_rr45(to,Occupied_RR45(ply+1));
  if (wtm) {
    Clear_Set(bit_move,White_Rooks(ply+1));
    Clear_Set(bit_move,White_Pieces(ply+1));
    Hash_Rw(from,Hash_Key(ply+1));
    Hash_Rw(to,Hash_Key(ply+1));
    Piece_On_Square(ply+1,from)=0;
    Piece_On_Square(ply+1,to)=rook;
    if (White_Castle(ply+1))
      if (from == 0) White_Castle(ply+1)&=1;
      else if (from == 7) White_Castle(ply+1)&=2;
  }
  else {
    Clear_Set(bit_move,Black_Rooks(ply+1));
    Clear_Set(bit_move,Black_Pieces(ply+1));
    Hash_Rb(from,Hash_Key(ply+1));
    Hash_Rb(to,Hash_Key(ply+1));
    Piece_On_Square(ply+1,from)=0;
    Piece_On_Square(ply+1,to)=-rook;
    if (Black_Castle(ply+1))
      if (from == 56) Black_Castle(ply+1)&=1;
      else if (from == 63) Black_Castle(ply+1)&=2;
  }
}

/*
********************************************************************************
*                                                                              *
*   Make_Move_Root() is used to make a move at the root of the game tree,      *
*   before any searching is done.  it uses Make_Move() to execute the move,    *
*   but then copies the resulting position back to position[0], the actual     *
*   board position.  it handles the special-case of the draw-by-repetition     *
*   rule by maintaining a list of previous positions, which is reset each time *
*   a non-reversible (pawn move or capture move) is made.                      *
*                                                                              *
********************************************************************************
*/
void Make_Move_Root(int move, int wtm)
{
/*
 ----------------------------------------------------------
|                                                          |
|   first, make the move and replace position[0] with the  |
|   new position.                                          |
|                                                          |
 ----------------------------------------------------------
*/
  Make_Move(0,move,wtm);
/*
 ----------------------------------------------------------
|                                                          |
|   now, if this is a non-reversible move, reset the       |
|   repetition list pointer to start the count over.       |
|                                                          |
 ----------------------------------------------------------
*/
  if ((Piece(move) == 1) || (Captured(move)) ||
      (White_Castle(0) != White_Castle(1)) ||
      (Black_Castle(0) != Black_Castle(1))) {
    if (wtm)
      repetition_head=0;
    else {
      repetition_head=1;
      repetition_list[1]=0;
    }
    position[1].moves_since_cap_or_push=0;
  }
  position[0]=position[1];
  repetition_list[++repetition_head]=Hash_Key(0);
}
