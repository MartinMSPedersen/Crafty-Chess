#if !defined(NOEGTB)
#  include "chess.h"
#  include "data.h"
/* last modified 02/23/14 */
/*
 *******************************************************************************
 *                                                                             *
 *  EGTBProbe() is the interface to the new tablebase code by Eugene Nalimov.  *
 *  This is called from Search() after a capture, when the number of pieces    *
 *  remaining on the board is less than or equal to the max number of pieces   *
 *  we have in the EGTB files that are available for use.                      *
 *                                                                             *
 *******************************************************************************
 */
#  define                                                              \
VInitSqCtr(rgCtr, rgSquares, piece, bitboard) {                        \
  int  cPieces=0;                                                      \
  uint64_t bbTemp=(bitboard);                                          \
  while (bbTemp) {                                                     \
    const squaret sq=MSB(bbTemp);                                      \
    (rgSquares)[(piece)*C_PIECES+cPieces]=sq;                          \
    cPieces++;                                                         \
    Clear(sq, bbTemp);                                                 \
  }                                                                    \
  (rgCtr)[(piece)]=cPieces;                                            \
}
#  define  T_INDEX64
#  define  C_PIECES  3 /* Maximum # of pieces of one color OTB */
#  if defined (T_INDEX64) && defined (_MSC_VER)
typedef uint64_t INDEX;
#  elif defined (T_INDEX64)
typedef uint64_t INDEX;
#  else
typedef unsigned long INDEX;
#  endif
typedef unsigned int squaret;

/* Those declarations necessary because Crafty is C, not C++ program */
#  if defined (_MSC_VER)
#    define  TB_FASTCALL  __fastcall
#  else
#    define  TB_FASTCALL
#  endif
typedef int pcolor;

#  define  x_colorWhite         0
#  define  x_colorBlack         1
#  define  x_colorNeutral       2
#  define COLOR_DECLARED
typedef int piece;

#  define  x_pieceNone          0
#  define  x_piecePawn          1
#  define  x_pieceKnight        2
#  define  x_pieceBishop        3
#  define  x_pieceRook          4
#  define  x_pieceQueen         5
#  define  x_pieceKing          6
#  define PIECES_DECLARED
typedef signed char tb_t;

#  define pageL       65536
#  define tbbe_ssL    ((pageL-4)/2)
#  define bev_broken  (tbbe_ssL+1) /* illegal or busted */
#  define bev_mi1     tbbe_ssL /* mate in 1 move */
#  define bev_mimin   1 /* mate in max moves */
#  define bev_draw    0 /* draw */
#  define bev_limax   (-1) /* mated in max moves */
#  define bev_li0     (-tbbe_ssL)
    /* mated in 0 moves */
typedef INDEX(TB_FASTCALL * PfnCalcIndex)
 (squaret *, squaret *, squaret, int fInverse);
extern int IDescFindFromCounters(int *);
extern int FRegisteredFun(int, pcolor);
extern PfnCalcIndex PfnIndCalcFun(int, pcolor);
extern int TB_FASTCALL L_TbtProbeTable(int, pcolor, INDEX);

#  define PfnIndCalc PfnIndCalcFun
#  define FRegistered FRegisteredFun
int EGTBProbe(TREE * RESTRICT tree, int ply, int wtm, int *score) {
  int rgiCounters[10], iTb, fInvert;
  pcolor side;
  squaret rgsqWhite[C_PIECES * 5 + 1], rgsqBlack[C_PIECES * 5 + 1];
  squaret *psqW, *psqB, sqEnP;
  INDEX ind;
  int tbValue;

/*
 ************************************************************
 *                                                          *
 *  Initialize counters and piece arrays so the probe code  *
 *  can compute the modified Godel number.                  *
 *                                                          *
 ************************************************************
 */
  VInitSqCtr(rgiCounters, rgsqWhite, 0, Pawns(white));
  VInitSqCtr(rgiCounters, rgsqWhite, 1, Knights(white));
  VInitSqCtr(rgiCounters, rgsqWhite, 2, Bishops(white));
  VInitSqCtr(rgiCounters, rgsqWhite, 3, Rooks(white));
  VInitSqCtr(rgiCounters, rgsqWhite, 4, Queens(white));
  VInitSqCtr(rgiCounters + 5, rgsqBlack, 0, Pawns(black));
  VInitSqCtr(rgiCounters + 5, rgsqBlack, 1, Knights(black));
  VInitSqCtr(rgiCounters + 5, rgsqBlack, 2, Bishops(black));
  VInitSqCtr(rgiCounters + 5, rgsqBlack, 3, Rooks(black));
  VInitSqCtr(rgiCounters + 5, rgsqBlack, 4, Queens(black));
/*
 ************************************************************
 *                                                          *
 *  Quick early exit.  Is the tablebase for the current set *
 *  of pieces registered?                                   *
 *                                                          *
 ************************************************************
 */
  iTb = IDescFindFromCounters(rgiCounters);
  if (!iTb)
    return 0;
/*
 ************************************************************
 *                                                          *
 *  Yes, finish setting up to probe the tablebase.  If      *
 *  black is the "winning" side (more pieces) then we need  *
 *  to "invert" the pieces in the lists.                    *
 *                                                          *
 ************************************************************
 */
  rgsqWhite[C_PIECES * 5] = KingSQ(white);
  rgsqBlack[C_PIECES * 5] = KingSQ(black);
  if (iTb > 0) {
    side = wtm ? x_colorWhite : x_colorBlack;
    fInvert = 0;
    psqW = rgsqWhite;
    psqB = rgsqBlack;
  } else {
    side = wtm ? x_colorBlack : x_colorWhite;
    fInvert = 1;
    psqW = rgsqBlack;
    psqB = rgsqWhite;
    iTb = -iTb;
  }
/*
 ************************************************************
 *                                                          *
 *  Now check to see if this particular tablebase for this  *
 *  color to move is registered.                            *
 *                                                          *
 ************************************************************
 */
  if (!FRegistered(iTb, side))
    return 0;
  sqEnP = EnPassant(ply) ? EnPassant(ply) : 127;
  ind = PfnIndCalc(iTb, side) (psqW, psqB, sqEnP, fInvert);
  tbValue = L_TbtProbeTable(iTb, side, ind);
  if (bev_broken == tbValue)
    return 0;
/*
 ************************************************************
 *                                                          *
 *  Convert to correct MATE range to match the value        *
 *  Crafty uses.                                            *
 *                                                          *
 ************************************************************
 */
  if (tbValue > 0)
    *score = MATE + 2 * (-bev_mi1 + tbValue - 1);
  else if (tbValue < 0)
    *score = -MATE + 2 * (bev_mi1 + tbValue);
  else
    *score = 0;
  return 1;
}
#endif
