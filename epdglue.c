/*>>> epdglue.c: glue to connect Crafty to the EPD Kit routines */

/* Revised: 1995.12.03 */

/*
Copyright (C) 1995 by Steven J. Edwards (sje@mv.mv.com)
All rights reserved.  This code may be freely redistibuted and used by
both research and commerical applications.  No warranty exists.
*/

/*
The contents of this source file form the programmatic glue between
the host program Crafty and the EPD Kit.  Therefore, this file will
have to be changed if used with a different host program.  Also, the
contents of the prototype include file (epdglue.h) may also require
modification for a different host.

The contents of the other source files in the EPD Kit (epddefs.h,
epd.h, and epd.c) should not have to be changed for different hosts.
*/

/*
This file was originally prepared on an Apple Macintosh using the
Metrowerks CodeWarrior 6 ANSI C compiler.  Tabs are set at every
four columns.  Further testing and development was performed on a
generic PC running Linux 1.2.9 and using the gcc 2.6.3 compiler.
*/

/* system includes */

#include <stdio.h>
#include <string.h>
#include <time.h>

/* Crafty includes */

#include "types.h"
#include "data.h"
#include "function.h"

/* EPD Kit definitions (host program independent) */

#include "epddefs.h"

/* EPD Kit routine prototypes (host program independent) */

#include "epd.h"

/* prototypes for this file (host program dependent) */

#include "epdglue.h"

/* EPD glue command type */

typedef siT egcommT, *egcommptrT;
#define egcommL 10
#define egcomm_nil (-1)

#define egcomm_epdbfix 0 /* fix file for Bookup import */
#define egcomm_epdhelp 1 /* display EPD help */
#define egcomm_epdnoop 2 /* no operation */
#define egcomm_epdpfdn 3 /* process file: data normalization */
#define egcomm_epdpfdr 4 /* process file: data repair */
#define egcomm_epdpfga 5 /* process file: general analysis */
#define egcomm_epdpfop 6 /* process file: operation purge */
#define egcomm_epdscor 7 /* score EPD benchmark result file */
#define egcomm_epdshow 8 /* show EPD four fields for current position */
#define egcomm_epdtest 9 /* developer testing */

/* output text buffer */

#define tbufL 256
static char tbufv[tbufL];

/* EPD glue command strings */

static charptrT egcommstrv[egcommL];

/* EPD glue command string descriptions */

static charptrT eghelpstrv[egcommL];

/* EPD glue command parameter counts (includes command token) */

static siT egparmcountv[egcommL];

/*--> EGPrint: print a string to the output */
static
void
EGPrint(charptrT s)
{
/* this is an internal EPD glue routine */

/*
This routine is provided as an alternative to direct writing to the
standard output.  All EPD glue printing output goes through here.  The
idea is that the host program may have some special requirements for
printing output (like a window display), so a convenient single point
is provided to handle this.

Note that there is no corresponding routine for reading from the
standard input because the EPD glue does no interactive reading.
*/

/* for Crafty, the standard output is used */

printf("%s", s);

return;
}

/*--> EGPrintTB: print the contents of the text buffer */
static
void
EGPrintTB(void)
{
/* this is an internal EPD glue routine */

EGPrint(tbufv);

return;
}

/*--> EGLocateCommand: locate an EPD glue command from a token */
static
egcommT
EGLocateCommand(charptrT s)
{
egcommT egcomm, index;

/* this is an internal EPD glue routine */

/* set the default return value: no match */

egcomm = egcomm_nil;

/* scan the EPD glue command string vector */

index = 0;
while ((index < egcommL) && (egcomm == egcomm_nil))
	if (strcmp(s, egcommstrv[index]) == 0)
		egcomm = index;
	else
		index++;

return (egcomm);
}

/*--> EGMapFromHostColor: map a color from the host to the EPD style */
static
cT
EGMapFromHostColor(siT color)
{
cT c;

/* this is an internal glue routine */

/* map from Crafty's color representation */

if (color == 1)
	c = c_w;
else
	c = c_b;

return (c);
}

/*--> EGMapToHostColor: map a color to the host from the EPD style */
static
siT
EGMapToHostColor(cT c)
{
siT color;

/* this is an internal glue routine */

/* map to Crafty's color representation */

if (c == c_w)
	color = 1;
else
	color = 0;

return (color);
}

/*--> EGMapFromHostPiece: map a piece from the host to the EPD style */
static
pT
EGMapFromHostPiece(siT piece)
{
pT p;

/* this is an internal glue routine */

/* map from Crafty's piece representation */

switch (piece)
	{
	case pawn:
		p = p_p;
		break;
	case knight:
		p = p_n;
		break;
	case bishop:
		p = p_b;
		break;
	case rook:
		p = p_r;
		break;
	case queen:
		p = p_q;
		break;
	case king:
		p = p_k;
		break;
	};

return (p);
}

/*--> EGMapToHostPiece: map a piece to the host from the EPD style */
static
siT
EGMapToHostPiece(pT p)
{
siT piece;

/* this is an internal glue routine */

/* map to Crafty's piece representation */

switch (p)
	{
	case p_p:
		piece = pawn;
		break;
	case p_n:
		piece = knight;
		break;
	case p_b:
		piece = bishop;
		break;
	case p_r:
		piece = rook;
		break;
	case p_q:
		piece = queen;
		break;
	case p_k:
		piece = king;
		break;
	};

return (piece);
}

/*--> EGMapFromHostCP: map a color piece from the host to the EPD style */
static
cpT
EGMapFromHostCP(siT hostcp)
{
cpT cp;

/* this is an internal glue routine */

/* map from Crafty's color-piece representation */

switch (hostcp)
	{
	case -queen:
		cp = cp_bq;
		break;
	case -rook:
		cp = cp_br;
		break;
	case -bishop:
		cp = cp_bb;
		break;
	case -king:
		cp = cp_bk;
		break;
	case -knight:
		cp = cp_bn;
		break;
	case -pawn:
		cp = cp_bp;
		break;
	case 0:
		cp = cp_v0;
		break;
	case pawn:
		cp = cp_wp;
		break;
	case knight:
		cp = cp_wn;
		break;
	case king:
		cp = cp_wk;
		break;
	case bishop:
		cp = cp_wb;
		break;
	case rook:
		cp = cp_wr;
		break;
	case queen:
		cp = cp_wq;
		break;
	};

return (cp);
}

/*--> EGMapToHostCP: map a color piece to the host from the EPD style */
static
siT
EGMapToHostCP(cpT cp)
{
siT hostcp;

/* this is an internal glue routine */

/* map to Crafty's color-piece representation */

switch (cp)
	{
	case cp_wp:
		hostcp = pawn;
		break;
	case cp_wn:
		hostcp = knight;
		break;
	case cp_wb:
		hostcp = bishop;
		break;
	case cp_wr:
		hostcp = rook;
		break;
	case cp_wq:
		hostcp = queen;
		break;
	case cp_wk:
		hostcp = king;
		break;
	case cp_bp:
		hostcp = -pawn;
		break;
	case cp_bn:
		hostcp = -knight;
		break;
	case cp_bb:
		hostcp = -bishop;
		break;
	case cp_br:
		hostcp = -rook;
		break;
	case cp_bq:
		hostcp = -queen;
		break;
	case cp_bk:
		hostcp = -king;
		break;
	case cp_v0:
		hostcp = 0;
		break;
	};

return (hostcp);
}

/*--> EGMapFromHostSq: map square index from host style */
static
sqT
EGMapFromHostSq(siT index)
{
sqT sq;

/* this is an internal glue routine */

/* Crafty's square index is the same as the EPD Kit square index */

sq = index;

return (sq);
}

/*--> EGMapToHostSq: map square index to host style */
static
siT
EGMapToHostSq(sqT sq)
{
siT index;

/* this is an internal glue routine */

/* Crafty's square index is the same as the EPD Kit square index */

index = sq;

return (index);
}

/*--> EGMapFromHostScore: map score from host style */
static
siT
EGMapFromHostScore(liT score)
{
siT ce;
liT distance;

/* this is an internal EPD glue routine */

/* check for a forced mate */

if (score > 64000)
	{
	/* convert forced mate score */
	
	distance = (MATE - score) / 2;
	ce = 32768 - (distance * 2);
	}
else
	if (score < -64000)
		{
		/* convert forced loss score */
		
		distance = (MATE + score) / 2;
		ce = -32767 + (distance * 2);
		}
	else
		{
		/* convert regular score */
		
		ce = score / 10;
		};

return (ce);
}

/*--> EGMapToHostScore: map score to host style */
static
liT
EGMapToHostScore(siT ce)
{
liT score;
liT distance;

/* this is an internal EPD glue routine */

/* check for a forced mate */

if (ce > 32000)
	{
	/* convert forced mate score */
	
	distance = ((32766 - ce) / 2) + 1;
	score = MATE - (distance * 2);
	}
else
	if (ce < -32000)
		{
		/* convert forced loss score */
		
		distance = (ce + 32767) / 2;
		score = -MATE + (distance * 2);
		}
	else
		{
		/* convert regular score */
		
		score = ((liT) ce) * 10;
		};

return (score);
}

/*--> EGMapFromHostMove: map move from host style to EPD style */
static
mT
EGMapFromHostMove(liT move)
{
mT m;
siT flag;
scmvT scmv;

/* this is an internal EPD glue routine */

/* the EPD current position must be properly set */

m.m_flag = 0;
m.m_frsq = EGMapFromHostSq(From(move));
m.m_tosq = EGMapFromHostSq(To(move));
m.m_frcp = EPDFetchCP(m.m_frsq);
m.m_tocp = EPDFetchCP(m.m_tosq);

/* determine special case move indication */

flag = 0;

if (!flag)
	if ((m.m_frcp == cp_wk) &&
		(m.m_frsq == sq_e1) && (m.m_tosq == sq_g1))
		{
		m.m_scmv = scmv_cks;
		flag = 1;
		};
		

if (!flag)
	if ((m.m_frcp == cp_bk) &&
		(m.m_frsq == sq_e8) && (m.m_tosq == sq_g8))
		{
		m.m_scmv = scmv_cks;
		flag = 1;
		};
		
if (!flag)
	if ((m.m_frcp == cp_wk) &&
		(m.m_frsq == sq_e1) && (m.m_tosq == sq_c1))
		{
		m.m_scmv = scmv_cqs;
		flag = 1;
		};

if (!flag)
	if ((m.m_frcp == cp_bk) &&
		(m.m_frsq == sq_e8) && (m.m_tosq == sq_c8))
		{
		m.m_scmv = scmv_cqs;
		flag = 1;
		};

if (!flag)
	if ((m.m_frcp == cp_wp) && (m.m_tosq == EPDFetchEPSQ()))
		{
		m.m_scmv = scmv_epc;
		flag = 1;
		};

if (!flag)
	if ((m.m_frcp == cp_bp) && (m.m_tosq == EPDFetchEPSQ()))
		{
		m.m_scmv = scmv_epc;
		flag = 1;
		};

if (!flag)
	if (Promote(move) != 0)
		{
		switch (Promote(move))
			{
			case knight:
				m.m_scmv = scmv_ppn;
				break;
			case bishop:
				m.m_scmv = scmv_ppb;
				break;
			case rook:
				m.m_scmv = scmv_ppr;
				break;
			case queen:
				m.m_scmv = scmv_ppq;
				break;
			};
		flag = 1;
		};

if (!flag)
	m.m_scmv = scmv_reg;

return (m);
}

/*--> EGMapToHostMove: map move to host style from EPD style */
static
siT
EGMapToHostMove(mT m)
{
liT move;

/* this is an internal EPD glue routine */

/* the EPD current position must be properly set */

move = 0;

move |= EGMapToHostSq(m.m_frsq);
move |= EGMapToHostSq(m.m_tosq) << 6;
move |= EGMapToHostPiece(EPDPieceFromCP(m.m_frcp)) << 12;

if (m.m_tocp != cp_v0)
	move |= EGMapToHostPiece(EPDPieceFromCP(m.m_tocp)) << 15;

switch (m.m_scmv)
	{
	case scmv_epc:
		move |= pawn << 15;
		break;
	case scmv_ppn:
		move |= knight << 18;
		break;
	case scmv_ppb:
		move |= bishop << 18;
		break;
	case scmv_ppr:
		move |= rook << 18;
		break;
	case scmv_ppq:
		move |= queen << 18;
		break;
	default:
		break;
	};

return (move);
}

/*--> EGGetHostPosition: copy from host position to EPD Kit position */
static
void
EGGetHostPosition(void)
{
sqT sq;
rbT rb;
cT actc;
castT cast;
sqT epsq;

/* this is an internal EPD glue routine */

/*
This routine is called from within the EPD glue to copy the host program's
current position into the EPD Kit.  Information about the previous EPD Kit
current position is lost.
*/

/* read from the host piece placement */

for (sq = sq_a1; sq <= sq_h8; sq++)
	rb.rbv[sq] =
		EGMapFromHostCP(position[0].board[EGMapToHostSq(sq)]);

/* read from the host piece active color */

actc = EGMapFromHostColor(wtm);

/* read from the host piece castling availability */

cast = 0;

switch (position[0].w_castle)
	{
	case 0:
		break;
	case 1:
		cast |= cf_wk;
		break;
	case 2:
		cast |= cf_wq;
		break;
	case 3:
		cast |= cf_wk | cf_wq;
		break;
	};

switch (position[0].b_castle)
	{
	case 0:
		break;
	case 1:
		cast |= cf_bk;
		break;
	case 2:
		cast |= cf_bq;
		break;
	case 3:
		cast |= cf_bk | cf_bq;
		break;
	};

/* read from the host piece en passant target square */

if (position[0].enpassant_target == 0)
	epsq = sq_nil;
else
	{
	sq = sq_a1;
	while ((epsq == sq_nil) && (sq <= sq_h8))
		if (position[0].enpassant_target == set_mask[EGMapToHostSq(sq)])
			epsq = sq;
		else
			sq++;
	};

/* set the EPD current position */

EPDSetCurrentPosition(&rb, actc, cast, epsq);

return;
}

/*--> EGPutHostPosition: copy from EPD Kit position to host position */
static
void
EGPutHostPosition(void)
{
sqT sq;
rbT rb;
cT actc;
castT cast;
sqT epsq;
siT index;

/* this is an internal EPD glue routine */

/*
This routine is called from within the EPD glue to copy the EPD Kit's current
position into the host program.  If the previous host program current position
is different from the new position, then information about the previous host
program current position is lost.  This means that the host program preserves
history information if and only if such preservation is appropriate.

Actually, the host position data is completely overwritten, so the above
comment is temporarily false, but will be true as developement proceeds.
*/

/* fetch the EPD current position data items */

rb = *EPDFetchBoard();
actc = EPDFetchACTC();
cast = EPDFetchCAST();
epsq = EPDFetchEPSQ();

/* copy the board into the host board */

for (sq = sq_a1; sq <= sq_h8; sq++)
	position[0].board[EGMapToHostSq(sq)] =
		EGMapToHostCP(rb.rbv[sq]);

/* copy the active color */

wtm = EGMapToHostColor(actc);

/* copy the castling availibility */

position[0].w_castle = 0;
if (cast & cf_wk)
	position[0].w_castle += 1;
if (cast & cf_wq)
	position[0].w_castle += 2;

position[0].b_castle = 0;
if (cast & cf_bk)
	position[0].b_castle += 1;
if (cast & cf_bq)
	position[0].b_castle += 2;

/* copy the en passant target square */

if (epsq == sq_nil)
	position[0].enpassant_target = 0;
else
	position[0].enpassant_target = set_mask[EGMapToHostSq(epsq)];

/* set secondary host data items */

SetChessBitBoards(&position[0]);

if (actc == c_w)
	repetition_head = 0;
else
	{
	repetition_head = 1;
	repetition_list[1] = 0;
	};

last_mate_score = 0;

last_move_in_book = -100;

/* clear the host history */

for (index = 0; index < 4096; index++)
	history_w[index] = history_b[index] = 0;

/* clear the host killer information */

for (index = 0; index < MAXPLY; index++)
	{
	killer_move[index][0] = killer_move[index][1] = 0;
	killer_move_count[index][0] = killer_move_count[index][1] = 0;
	};

/* clear miscellaneous host items */

ponder_completed = 0;
ponder_move = 0;
pv[0].path_iteration_depth = 0;
pv[0].path_length = 0;
over = 0;

return;
}

/*--> EGProcessBFIX: process the EG command epdbfix */
static
siT
EGProcessBFIX(void)
{
siT flag;
fptrT fptr0, fptr1;
eopptrT eopptr;
epdptrT epdptr, nptr;
charptrT s;
char ev[epdL];

/* this is an internal EPD glue routine */

/* set the default return value: success */

flag = 1;

/* clear the file pointers */

fptr0 = fptr1 = NULL;

/* parameter count check */

if (EPDTokenCount() != 3)
	{
	sprintf(tbufv, "This command takes two parameters\n");
	EGPrintTB();
	flag = 0;
	};

/* set up the input file */

if (flag)
	{
	fptr0 = fopen(EPDTokenFetch(1), "r");
	if (fptr0 == NULL)
		{
		sprintf(tbufv, "Can't open %s for reading\n",
			EPDTokenFetch(1));
		EGPrintTB();
		flag = 0;
		};
	};

/* set up the output file */

if (flag)
	{
	fptr1 = fopen(EPDTokenFetch(2), "w");
	if (fptr1 == NULL)
		{
		sprintf(tbufv, "Can't open %s for writing\n",
			EPDTokenFetch(2));
		EGPrintTB();
		flag = 0;
		};
	};

/* scan the file */

if (flag)
	while (flag && (fgets(ev, (epdL - 1), fptr0) != NULL))
		{
		/* decode the record into an EPD structure */
		
		epdptr = EPDDecode(ev);

		/* check record decode validity */
		
		if (epdptr == NULL)
			flag = 0;
		else
			{
			/* clone the input EPD structure base */
			
			nptr = EPDCloneEPDBase(epdptr);
			
			/* copy the ce operation */
			
			eopptr = EPDLocateEOP(epdptr, "ce");
			if (eopptr != NULL)
				EPDAppendEOP(nptr, EPDCloneEOP(eopptr));
	
			/* copy the pv operation */
			
			eopptr = EPDLocateEOP(epdptr, "pv");
			if (eopptr != NULL)
				EPDAppendEOP(nptr, EPDCloneEOP(eopptr));
	
			/* output the new EPD strucutre */
			
			s = EPDEncode(nptr);			
			fprintf(fptr1, "%s\n", s);
			EPDMemoryFree(s);
				
			/* deallocate both EPD structures */
			
			EPDReleaseEPD(epdptr);
			EPDReleaseEPD(nptr);
			};
		};

/* ensure file close */

if (fptr0 != NULL)
	fclose(fptr0);

if (fptr1 != NULL)
	fclose(fptr1);

return (flag);
}

/*--> EGProcessHELP: process the EG command epdhelp */
static
siT
EGProcessHELP(void)
{
siT flag;
egcommT egcomm;

/* this is an internal EPD glue routine */

/* set the default return value: success */

flag = 1;

/* parameter count check */

if (EPDTokenCount() != 1)
	{
	sprintf(tbufv, "This command takes no parameters\n");
	EGPrintTB();
	flag = 0;
	};

/* process the epdhelp command */

if (flag)
	{
	sprintf(tbufv, "Available EPD glue command list\n");
	EGPrintTB();
	sprintf(tbufv, "-------------------------------\n");
	EGPrintTB();

	for (egcomm = 0; egcomm < egcommL; egcomm++)
		{
		sprintf(tbufv, "%s: %s\n",
			egcommstrv[egcomm], eghelpstrv[egcomm]);
		EGPrintTB();
		};
	};

return (flag);
}

/*--> EGProcessNOOP: process the EG command epdnoop */
static
siT
EGProcessNOOP(void)
{
siT flag;

/* this is an internal EPD glue routine */

/* set the default return value: success */

flag = 1;

/* process the epdnoop command */

return (flag);
}

/*--> EGProcessPFDN: process the EG command epdpfdn */
static
siT
EGProcessPFDN(void)
{
siT flag;

/* this is an internal EPD glue routine */

/* set the default return value: success */

flag = 1;

/* parameter count check */

if (EPDTokenCount() != 3)
	{
	sprintf(tbufv, "This command takes two parameters\n");
	EGPrintTB();
	flag = 0;
	};

/* process the epdpfdn command */

if (flag)
	flag = EPDNormalizeFile(EPDTokenFetch(1), EPDTokenFetch(2));

return (flag);
}

/*--> EGProcessPFDR: process the EG command epdpfdr */
static
siT
EGProcessPFDR(void)
{
siT flag;

/* this is an internal EPD glue routine */

/* set the default return value: success */

flag = 1;

/* parameter count check */

if (EPDTokenCount() != 3)
	{
	sprintf(tbufv, "This command takes two parameters\n");
	EGPrintTB();
	flag = 0;
	};

/* process the epdpfdr command */

if (flag)
	flag = EPDRepairFile(EPDTokenFetch(1), EPDTokenFetch(2));

return (flag);
}

/*--> EGProcessPFGA: process the EG command epdpfga */
static
siT
EGProcessPFGA(void)
{
siT flag;
fptrT fptr0, fptr1;
time_t start_time;
liT result;
siT index;
liT host_acn, host_acs;
siT host_ce;
charptrT s;
liT move;
mT m;
sanT san;
eovptrT eovptr;
eopptrT eopptr;
epdptrT epdptr;
char ev[epdL];

/* this is an internal EPD glue routine */

/* set the default return value: success */

flag = 1;

/* clear the file pointers */

fptr0 = fptr1 = NULL;

/* parameter count check */

if (EPDTokenCount() != 3)
	{
	sprintf(tbufv, "This command takes two parameters\n");
	EGPrintTB();
	flag = 0;
	};

/* set up the input file */

if (flag)
	{
	fptr0 = fopen(EPDTokenFetch(1), "r");
	if (fptr0 == NULL)
		{
		sprintf(tbufv, "Can't open %s for reading\n",
			EPDTokenFetch(1));
		EGPrintTB();
		flag = 0;
		};
	};

/* set up the output file */

if (flag)
	{
	fptr1 = fopen(EPDTokenFetch(2), "w");
	if (fptr1 == NULL)
		{
		sprintf(tbufv, "Can't open %s for writing\n",
			EPDTokenFetch(2));
		EGPrintTB();
		flag = 0;
		};
	};

/* scan the file */

if (flag)
	while (flag && (fgets(ev, (epdL - 1), fptr0) != NULL))
		{
		/* decode the record into an EPD structure */
		
		epdptr = EPDDecode(ev);

		/* check record decode validity */
		
		if (epdptr == NULL)
			flag = 0;
		else
			{
			/* set up the position in the EPD Kit */
			
			EPDRealize(epdptr);

			/* set up the host current position */
			
			EGPutHostPosition();
			
			/* set host search parameters */

			position[1] = position[0];
			do_ponder = 0;

			/* get the starting time */
			
			start_time = time(NULL);

			/* run host search */

			result = Iterate(EGMapToHostColor(EPDFetchACTC()),think);

			/* extract analysis count: nodes */

			host_acn = nodes_searched;
			if (host_acn == 0)
				host_acn = 1;

			/* insert analysis count: nodes */

			EPDDropIfLocEOP(epdptr, "acn");
			eopptr = EPDCreateEOP("acn");
			eovptr = EPDCreateEOVInt(host_acn);
			EPDAppendEOV(eopptr, eovptr);
			EPDAppendEOP(epdptr, eopptr);
			
			/* extract analysis count: seconds */

			host_acs = time(NULL) - start_time;
			if (host_acs == 0)
				host_acs = 1;

			/* insert analysis count: seconds */

			EPDDropIfLocEOP(epdptr, "acs");
			eopptr = EPDCreateEOP("acs");
			eovptr = EPDCreateEOVInt(host_acs);
			EPDAppendEOV(eopptr, eovptr);
			EPDAppendEOP(epdptr, eopptr);
			
			/* extract centipawn evaluation */

			host_ce = EGMapFromHostScore(result);

			/* insert centipawn evaluation */

			EPDDropIfLocEOP(epdptr, "ce");
			eopptr = EPDCreateEOP("ce");
			eovptr = EPDCreateEOVInt(host_ce);
			EPDAppendEOV(eopptr, eovptr);
			EPDAppendEOP(epdptr, eopptr);
			
			/* delete predicted move */

			EPDDropIfLocEOP(epdptr, "pm");
			
			/* extract/insert predicted variation */

			EPDDropIfLocEOP(epdptr, "pv");
			eopptr = EPDCreateEOP("pv");

			for (index = 1; index <= pv[0].path_length; index++)
				{
				/* generate moves for the current position */
				
				EPDGenMoves();
				
				/* fetch the predicted move at this ply */
				
				move = pv[0].path[index];
				
				/* map the host move to EPD style */
				
				m = EGMapFromHostMove(move);
				
				/* set the flag bits in the EPD move */
				
				EPDSetMoveFlags(&m);
				
				/* construct the SAN for the move */
				
				EPDSANEncode(&m, san);
				
				/* create and append the SAN move */
				
				eovptr = EPDCreateEOVSym(san);
				EPDAppendEOV(eopptr, eovptr);
				
				/* execute the move to update the EPD position */
				
				EPDExecuteUpdate(&m);
				};

			/* retract predicted variation moves */
			
			EPDRetractAll();

			/* append the pv operation to the EPD structure */

			EPDAppendEOP(epdptr, eopptr);
			
			/* encode the EPD into a string, write, and release */
			
			s = EPDEncode(epdptr);			
			fprintf(fptr1, "%s\n", s);
			EPDMemoryFree(s);
			
			/* deallocate the EPD structure */
			
			EPDReleaseEPD(epdptr);
			};
		};

/* ensure file close */

if (fptr0 != NULL)
	fclose(fptr0);

if (fptr1 != NULL)
	fclose(fptr1);

return (flag);
}

/*--> EGProcessPFOP: process the EG command epdpfop */
static
siT
EGProcessPFOP(void)
{
siT flag;

/* this is an internal EPD glue routine */

/* set the default return value: success */

flag = 1;

/* parameter count check */

if (EPDTokenCount() != 4)
	{
	sprintf(tbufv, "This command takes three parameters\n");
	EGPrintTB();
	flag = 0;
	};

/* process the epdpfop command */

if (flag)
	flag = EPDPurgeOpFile(
		EPDTokenFetch(1), EPDTokenFetch(2), EPDTokenFetch(3));

return (flag);
}

/*--> EGProcessSCOR: process the EG command epdscor */
static
siT
EGProcessSCOR(void)
{
siT flag;
char fn[tL];
bmsT bms;

/* this is an internal EPD glue routine */

/* set the default return value: success */

flag = 1;

/* parameter count check */

if (EPDTokenCount() != 2)
	{
	sprintf(tbufv, "This command takes one parameter\n");
	EGPrintTB();
	flag = 0;
	};

/* process the epdscor command */

if (flag)
	{
	flag = EPDScoreFile(EPDTokenFetch(1), &bms);
	if (flag)
		{
		sprintf(tbufv, "total: %ld\n", bms.bms_total);
		EGPrintTB();
		if (bms.bms_total != 0)
			{
			if (bms.bms_acnflag)
				{
				sprintf(tbufv, "total acn: %ld   mean total acn: %ld\n",
					bms.bms_total_acn, (bms.bms_total_acn / bms.bms_total));
				EGPrintTB();
				};
	
			if (bms.bms_acsflag)
				{
				sprintf(tbufv, "total acs: %ld   mean total acs: %ld\n",
					bms.bms_total_acs, (bms.bms_total_acs / bms.bms_total));
				EGPrintTB();
				};
	
			if (bms.bms_acnflag && bms.bms_acsflag)
				if (bms.bms_total_acs != 0)
					{
					sprintf(tbufv, "total mean node frequency: %ld Hz\n",
						(bms.bms_total_acn / bms.bms_total_acs));
					EGPrintTB();
					};
	
			sprintf(tbufv, "solve: %ld\n", bms.bms_solve);
			EGPrintTB();
			if (bms.bms_solve != 0)
				{
				if (bms.bms_acnflag)
					{
					sprintf(tbufv, "solve acn: %ld   mean solve acn: %ld\n",
						bms.bms_solve_acn, (bms.bms_solve_acn / bms.bms_solve));
					EGPrintTB();
					};
		
				if (bms.bms_acsflag)
					{
					sprintf(tbufv, "solve acs: %ld   mean solve acs: %ld\n",
						bms.bms_solve_acs, (bms.bms_solve_acs / bms.bms_solve));
					EGPrintTB();
					};
	
				if (bms.bms_acnflag && bms.bms_acsflag)
					if (bms.bms_solve_acs != 0)
						{
						sprintf(tbufv, "solve mean node frequency: %ld Hz\n",
							(bms.bms_solve_acn / bms.bms_solve_acs));
						EGPrintTB();
						};
				};
	
			sprintf(tbufv, "unsol: %ld\n", bms.bms_unsol);
			EGPrintTB();
			if (bms.bms_unsol != 0)
				{
				if (bms.bms_acnflag)
					{
					sprintf(tbufv, "unsol acn: %ld   mean unsol acn: %ld\n",
						bms.bms_unsol_acn, (bms.bms_unsol_acn / bms.bms_unsol));
					EGPrintTB();
					};
		
				if (bms.bms_acsflag)
					{
					sprintf(tbufv, "unsol acs: %ld   mean unsol acs: %ld\n",
						bms.bms_unsol_acs, (bms.bms_unsol_acs / bms.bms_unsol));
					EGPrintTB();
					};
	
				if (bms.bms_acnflag && bms.bms_acsflag)
					if (bms.bms_unsol_acs != 0)
						{
						sprintf(tbufv, "unsol mean node frequency: %ld Hz\n",
							(bms.bms_unsol_acn / bms.bms_unsol_acs));
						EGPrintTB();
						};
				};
			};
		};
	};

return (flag);
}

/*--> EGProcessSHOW: process the EG command epdshow */
static
siT
EGProcessSHOW(void)
{
siT flag;
charptrT s;

/* this is an internal EPD glue routine */

/* set the default return value: success */

flag = 1;

/* parameter count check */

if (EPDTokenCount() != 1)
	{
	sprintf(tbufv, "This command takes no parameters\n");
	EGPrintTB();
	flag = 0;
	};

/* process the epdshow command */

if (flag)
	{
	/* load the host pointion into the EPD Kit */
	
	EGGetHostPosition();
	
	/* get the EPD string for the current position */
	
	s = EPDGenBasicCurrent();
	
	/* print and release */
	
	sprintf(tbufv, "%s\n", s);
	EGPrintTB();
	EPDMemoryFree(s);
	};

return (flag);
}

/*--> EGProcessTEST: process the EG command epdtest */
static
siT
EGProcessTEST(void)
{
siT flag;

/* this is an internal EPD glue routine */

/* set the default return value: success */

flag = 1;

/* process the epdtest command */

if (flag)
	{
	sprintf(tbufv, "This command is not yet implemented\n");
	EGPrintTB();
	flag = 0;
	};

return (flag);
}

/*--> EGCommandParmCount: return parameter count for a command */
nonstatic
int
EGCommandParmCount(char *s)
{
siT count;
egcommT egcomm;

/* this is called by Option() in option.c */

/*
This routine is required for interfacing with Crafty to support EPD glue
command line construction.  Because Crafty only distributes command input
one token at a time, it is necessary to have input processor code in
Option() assemble an EPD glue command line complete with parameter tokens.
To assist with this task, the code in Option has to have a way of knowing
how many parameters are associated with a given EPD glue command.

Note that the Crafty is unable to uniformly process commands that have a
variable number of parameters.  This is why every EPD glue command
parameter count in this integration is fixed for the corresponding command.

Other implementations may send an entire command text line.  This is a
simpler approach and also easily supports having a variable number of
parameters for EPD glue commands.  For these other implementations, the
driver code in the host program need only call the EGCommand() routine
with the whole command input string as the single EGCommand() parameter.
*/

/* get command index */

egcomm = EGLocateCommand(s);

if (egcomm == egcomm_nil)
	count = 0;
else
	count = egparmcountv[egcomm];

return (count);
}

/*--> EGCommandCheck: check if a string starts with an EPD command token */
nonstatic
int
EGCommandCheck(char *s)
{
siT flag;
charptrT tokenptr;
egcommT egcomm;

/* this routine is called from Option() in option.c */

/*
This routine is used to quickly check if an input command line starts with
one of the available EPD glue command tokens.  The idea is that EPD action
can be quickly selected (or deselected) for any command input text string.

Because Crafty distributes command input one token at a time, the calls to
this routine in this implementation will only have a single token for input.
Other implementations that use an entire input command line will use the
full power of this routine which includes token scanning and recording
which is handled by the EPD Kit routines in the epd.c file.
*/

/* set default return value: no match */

flag = 0;

/* tokenize the input string */

EPDTokenize(s);

/* was there at least one token? */

if (EPDTokenCount() > 0)
	{
	/* get a pointer to the first token (origin zero) */
	
	tokenptr = EPDTokenFetch(0);
	
	/* get the glue command indicator */

	egcomm = EGLocateCommand(tokenptr);
	
	/* was there a command match? */
	
	if (egcomm != egcomm_nil)
		flag = 1;
	};

return (flag);
}

/*--> EGCommand: process an EPD command string */
nonstatic
int
EGCommand(char *s)
{
siT flag;
egcommT egcomm;

/* this routine is called from Option() in option.c */

/*
This routine actviates EPD glue command processing.  it is called with a
string that represents an entire EPD glue command input, including any
parameters following the command.

Because Crafty distributes command input one token at a time, it is
necessary for the host calling code to assemble a command line from the
input token stream.  See the code in Option() for details.
*/

/* set the default return value: success */

flag = 1;

/* check the command string (this also tokenizes it) */

if (EGCommandCheck(s))
	egcomm = EGLocateCommand(EPDTokenFetch(0));
else
	egcomm = egcomm_nil;

/* was a valid EPD glue command located? */

if (egcomm == egcomm_nil)
	{
	sprintf(tbufv, "EG fault: can't locate valid EG command\n");
	EGPrintTB();
	flag = 0;
	}
else
	{
	/* a valid command token was found; perform command dispatch */
	
	switch (egcomm)
		{
		case egcomm_epdbfix:
			flag = EGProcessBFIX();
			break;

		case egcomm_epdhelp:
			flag = EGProcessHELP();
			break;

		case egcomm_epdnoop:
			flag = EGProcessNOOP();
			break;

		case egcomm_epdpfdn:
			flag = EGProcessPFDN();
			break;

		case egcomm_epdpfdr:
			flag = EGProcessPFDR();
			break;

		case egcomm_epdpfga:
			flag = EGProcessPFGA();
			break;

		case egcomm_epdpfop:
			flag = EGProcessPFOP();
			break;

		case egcomm_epdscor:
			flag = EGProcessSCOR();
			break;

		case egcomm_epdshow:
			flag = EGProcessSHOW();
			break;

		case egcomm_epdtest:
			flag = EGProcessTEST();
			break;
		};
	
	/* check result */

	if (!flag)
		{
		sprintf(tbufv, "EG fault: a problem occurred during %s processing\n",
			EPDTokenFetch(0));
		EGPrintTB();
		};
	};

return (flag);
}

/*--> EGInit: one time EPD glue initialization */
nonstatic
void
EGInit(void)
{
/* this is called by Initialize() in init.c */

/* call the EPD one time set up code */

EPDInit();

/* initialize the EPD glue command strings vector */

egcommstrv[egcomm_epdbfix] = "epdbfix";
egcommstrv[egcomm_epdhelp] = "epdhelp";
egcommstrv[egcomm_epdnoop] = "epdnoop";
egcommstrv[egcomm_epdpfdn] = "epdpfdn";
egcommstrv[egcomm_epdpfdr] = "epdpfdr";
egcommstrv[egcomm_epdpfga] = "epdpfga";
egcommstrv[egcomm_epdpfop] = "epdpfop";
egcommstrv[egcomm_epdscor] = "epdscor";
egcommstrv[egcomm_epdshow] = "epdshow";
egcommstrv[egcomm_epdtest] = "epdtest";

/* initialize the EPD glue command string descriptions vector */

eghelpstrv[egcomm_epdbfix] = "Fix <file1> data for Bookup input <file2>";
eghelpstrv[egcomm_epdhelp] = "Display EPD glue command descriptions";
eghelpstrv[egcomm_epdnoop] = "No operation";
eghelpstrv[egcomm_epdpfdn] = "Normalize EPD data from <file1> to <file2>";
eghelpstrv[egcomm_epdpfdr] = "Repair EPD data from <file1> to <file2>";
eghelpstrv[egcomm_epdpfga] = "Analyze EPD data from <file1> to <file2>";
eghelpstrv[egcomm_epdpfop] = "Purge EPD <opcode> from <file1> to <file2>";
eghelpstrv[egcomm_epdscor] = "Score benchmark EPD results from <file>";
eghelpstrv[egcomm_epdshow] = "Show EPD four fields for the current position";
eghelpstrv[egcomm_epdtest] = "EPD glue developer testing";

/* initialize the EPD glue command parameter counts vector */

egparmcountv[egcomm_epdbfix] = 3;
egparmcountv[egcomm_epdhelp] = 1;
egparmcountv[egcomm_epdnoop] = 1;
egparmcountv[egcomm_epdpfdn] = 3;
egparmcountv[egcomm_epdpfdr] = 3;
egparmcountv[egcomm_epdpfga] = 3;
egparmcountv[egcomm_epdpfop] = 4;
egparmcountv[egcomm_epdscor] = 2;
egparmcountv[egcomm_epdshow] = 1;
egparmcountv[egcomm_epdtest] = 1;

return;
}

/*--> EGTerm: one time EPD glue termination */
nonstatic
void
EGTerm(void)
{
/* this is called by Option() in option.c */

/* call the EPD one time close down code */

EPDTerm();

return;
}

/*<<< epdglue.c: EOF */
