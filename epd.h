#if !defined(EPD_INCLUDED)
#  define EPD_INCLUDED
/*>>> epd.h: subprogram prototypes for epd.c */

/* Revised: 1995.12.03 */

/*
Copyright (C) 1995 by Steven J. Edwards (sje@mv.mv.com)
All rights reserved.  This code may be freely redistibuted and used by
both research and commerical applications.  No warranty exists.
*/

/*
Everything in this source file is independent of the host program.
Requests for changes and additions should be communicated to the author
via the e-mail address given above.
*/

/*
This file was originally prepared on an Apple Macintosh using the
Metrowerks CodeWarrior 6 ANSI C compiler.  Tabs are set at every
four columns.  Further testing and development was performed on a
generic PC running Linux 1.2.9 and using the gcc 2.6.3 compiler.
*/

void EPDFatal(charptrT s);
void EPDSwitchFault(charptrT s);
voidptrT EPDMemoryGrab(liT n);
void EPDMemoryFree(voidptrT ptr);
charptrT EPDStringGrab(charptrT s);
charptrT EPDStringAppendChar(charptrT s, char c);
charptrT EPDStringAppendStr(charptrT s0, charptrT s1);
void EPDTokenize(charptrT s);
siT EPDTokenCount(void);
charptrT EPDTokenFetch(siT n);
siT EPDCICharEqual(char ch0, char ch1);
pT EPDPieceFromCP(cpT cp);
siT EPDCheckPiece(char ch);
pT EPDEvaluatePiece(char ch);
siT EPDCheckColor(char ch);
cT EPDEvaluateColor(char ch);
siT EPDCheckRank(char ch);
rankT EPDEvaluateRank(char ch);
siT EPDCheckFile(char ch);
fileT EPDEvaluateFile(char ch);
eovptrT EPDNewEOV(void);
void EPDReleaseEOV(eovptrT eovptr);
void EPDAppendEOV(eopptrT eopptr, eovptrT eovptr);
eovptrT EPDCreateEOVStr(charptrT str);
eovptrT EPDCreateEOVSym(charptrT sym);
eovptrT EPDCreateEOVInt(liT lval);
eovptrT EPDLocateEOV(eopptrT eopptr, charptrT strval);
siT EPDCountEOV(eopptrT eopptr);
void EPDReplaceEOVStr(eovptrT eovptr, charptrT str);
eopptrT EPDNewEOP(void);
void EPDReleaseEOP(eopptrT eopptr);
void EPDAppendEOP(epdptrT epdptr, eopptrT eopptr);
eopptrT EPDCreateEOP(charptrT opsym);
eopptrT EPDLocateEOP(epdptrT epdptr, charptrT opsym);
siT EPDCountEOP(epdptrT epdptr);
void EPDDropIfLocEOP(epdptrT epdptr, charptrT opsym);
epdptrT EPDNewEPD(void);
void EPDReleaseOperations(epdptrT epdptr);
void EPDReleaseEPD(epdptrT epdptr);
epdptrT EPDCloneEPDBase(epdptrT epdptr);
eovptrT EPDCloneEOV(eovptrT eovptr);
eopptrT EPDCloneEOP(eopptrT eopptr);
epdptrT EPDCloneEPD(epdptrT epdptr);
epdptrT EPDSet(rbptrT rbptr, cT actc, castT cast, sqT epsq);
void EPDSetCurrentPosition(rbptrT rbptr, cT actc, castT cast, sqT epsq);
cT EPDFetchACTC(void);
castT EPDFetchCAST(void);
sqT EPDFetchEPSQ(void);
rbptrT EPDFetchBoard(void);
cpT EPDFetchCP(sqT sq);
charptrT EPDGenBasic(rbptrT rbptr, cT actc, castT cast, sqT epsq);
charptrT EPDGenBasicCurrent(void);
epdptrT EPDDecode(charptrT s);
charptrT EPDEncode(epdptrT epdptr);
void EPDRealize(epdptrT epdptr);
void EPDInitArray(void);
void EPDSANEncode(mptrT mptr, sanT san);
mptrT EPDSANDecodeAux(sanT san, siT strict);
void EPDExecuteUpdate(mptrT mptr);
void EPDRetractUpdate(void);
void EPDRetractAll(void);
void EPDGenMoves(void);
void EPDSetMoveFlags(mptrT mptr);
siT EPDPurgeOpFile(charptrT opsym, charptrT fn0, charptrT fn1);
void EPDRepairEPD(epdptrT epdptr);
siT EPDRepairFile(charptrT fn0, charptrT fn1);
siT EPDNormalizeFile(charptrT fn0, charptrT fn1);
siT EPDScoreFile(charptrT fn, bmsptrT bmsptr);
liT EPDEnumerate(siT depth);
void EPDInit(void);
void EPDTerm(void);

/*<<< epd.h: EOF */
#endif
