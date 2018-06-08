#include <stdio.h>
#include "chess.h"
#include "data.h"

/* last modified 11/03/98 */
/*
*******************************************************************************
*                                                                             *
*  EGTBProbe() is the interface to the new tablebase code by Eugene Nalimov.  *
*  this is called from Search() when 5 or fewer pieces are left on the board. *
*                                                                             *
*******************************************************************************
*/

#define  XX  127
#define  C_PIECES  3  /* Maximum # of pieces of one color OTB */

typedef unsigned int INDEX;
typedef unsigned int square;

/* Those declarations necessary because Crafty is C, not C++ program */

#if defined (_MSC_VER)
#define  TB_FASTCALL  __fastcall
#else
#define  TB_FASTCALL
#endif

  typedef  int  color;
  #define  x_colorWhite  0
  #define  x_colorBlack  1
  #define  x_colorNeutral  2
  #define COLOR_DECLARED

  typedef  int  piece;
  #define  x_pieceNone    0
  #define  x_piecePawn    1
  #define  x_pieceKnight  2
  #define  x_pieceBishop  3
  #define  x_pieceRook    4
  #define  x_pieceQueen   5
  #define  x_pieceKing    6
  #define PIECES_DECLARED
typedef  signed char tb_t;

#define pageL       256
#define tbbe_ssL    ((pageL-4)/2)
#define bev_broken  (tbbe_ssL+1)    /* illegal or busted */
#define bev_mi1     tbbe_ssL        /* mate in 1 move */
#define bev_mimin   1               /* mate in 126 moves */
#define bev_draw    0               /* draw */
#define bev_limax   (-1)            /* mated in 125 moves */
#define bev_li0     (-tbbe_ssL)     /* mated in 0 moves */
#define bev_unknown (-tbbe_ssL-2)   /* unknown */

typedef INDEX (TB_FASTCALL * PfnCalcIndex)
    (square*, square*, square, int fInverse);

extern int IDescFindFromCounters (int*);
extern int FRegisteredFun (int, color);
extern PfnCalcIndex PfnIndCalcFun (int, color);
extern tb_t TB_FASTCALL TbtProbeTable (int, color, INDEX);

#define PfnIndCalc PfnIndCalcFun
#define FRegistered FRegisteredFun

int EGTBProbe (TREE *tree, int ply, int wtm, int *score) {
  int       rgiCounters[10], iTb, fInvert;
  color     side;
  square    rgsqWhite[C_PIECES*5+1], rgsqBlack[C_PIECES*5+1];
  square    *psqW, *psqB, sqEnP;
  INDEX     ind;
  tb_t      tbValue;
/*
 ----------------------------------------------------------
|                                                          |
|   initialize counters and piece arrays so the probe code |
|   can compute the modified Godel number.                 |
|                                                          |
 ----------------------------------------------------------
*/
  VInitSqCtr(rgiCounters, rgsqWhite, 0, WhitePawns);
  VInitSqCtr(rgiCounters, rgsqWhite, 1, WhiteKnights);
  VInitSqCtr(rgiCounters, rgsqWhite, 2, WhiteBishops);
  VInitSqCtr(rgiCounters, rgsqWhite, 3, WhiteRooks);
  VInitSqCtr(rgiCounters, rgsqWhite, 4, WhiteQueens);
  VInitSqCtr(rgiCounters+5, rgsqBlack, 0, BlackPawns);
  VInitSqCtr(rgiCounters+5, rgsqBlack, 1, BlackKnights);
  VInitSqCtr(rgiCounters+5, rgsqBlack, 2, BlackBishops);
  VInitSqCtr(rgiCounters+5, rgsqBlack, 3, BlackRooks);
  VInitSqCtr(rgiCounters+5, rgsqBlack, 4, BlackQueens);
/*
 ----------------------------------------------------------
|                                                          |
|   quick early exit.  is the tablebase for the current    |
|   set of pieces registered?                              |
|                                                          |
 ----------------------------------------------------------
*/
  iTb=IDescFindFromCounters (rgiCounters);
  if (!iTb) return(0);
/*
 ----------------------------------------------------------
|                                                          |
|   yes, finish setting up to probe the tablebase.  if     |
|   black is the "winning" side (more pieces) then we need |
|   to "invert" the pieces in the lists.                   |
|                                                          |
 ----------------------------------------------------------
*/
  rgsqWhite[C_PIECES*5]=WhiteKingSQ;
  rgsqBlack[C_PIECES*5]=BlackKingSQ;
  if (iTb > 0) {
    side=wtm ? x_colorWhite : x_colorBlack;
    fInvert=0;
    psqW=rgsqWhite;
    psqB=rgsqBlack;
  }
  else {
    side=wtm ? x_colorBlack : x_colorWhite;
    fInvert=1;
    psqW=rgsqBlack;
    psqB=rgsqWhite;
    iTb=-iTb;
  }
/*
 ----------------------------------------------------------
|                                                          |
|   now check to see if this particular tablebase for this |
|   color to move is registered.                           |
|                                                          |
 ----------------------------------------------------------
*/
  if (!FRegistered (iTb, side)) return(0);
  sqEnP = EnPassant(ply) ? EnPassant(ply) : XX;
  ind=PfnIndCalc (iTb, side) (psqW, psqB, sqEnP, fInvert);
#if defined(SMP)
  Lock(lock_io);
#endif
  tbValue=TbtProbeTable (iTb, side, ind);
#if defined(SMP)
  UnLock(lock_io);
#endif
  if (bev_broken == tbValue) return(0);
/*
 ----------------------------------------------------------
|                                                          |
|   now convert to correct MATE range to match the value   |
|   Crafty uses.                                           |
|                                                          |
 ----------------------------------------------------------
*/
  if (tbValue > 0)
    *score=MATE+2*(-bev_mi1+tbValue-1);
  else if (tbValue < 0)
    *score=-MATE+2*(bev_mi1+tbValue);
  else
    *score=0;
  return(1);
}
