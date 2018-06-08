# To build crafty:
#
#	  You want to set up for maximum optimization, but typically you will
#	  need to experiment to see which options provide the fastest code.
#	  This is optimized for gcc, which is a fairly current compiler.
#   
#   The currently available targets:
#
#     aix          {IBM machines running AIX}
#     alpha        {DEC Alpha running OSF/1-Digital Unix}
#     darwin       {PPC on Mac OSX}
#     freebsd      {80X86 architecture running FreeBSD (Unix)}
#     hpux         {HP workstation running HP_UX operating system (Unix)}
#     linux        {80X86 architecture running LINUX (Unix) (gcc)}
#     linux-alpha  {Digital Alpha processor running LINUX (Unix)}
#     linux-amd64  {AMD X86-64 (opteron) running LINUX (Unix)}
#     linux-icc    {80X86 architecture running LINUX (Unix) (Intel compiler)}
#     netbsd       {80X86 architecture running netbsd(Unix)}
#     netbsd-sparc {sparc architecture running netbsd(Unix)}
#     next         {NextStep}
#     os2          {IBM OS/2 warp}
#     sgi          {SGI Workstation running Irix (SYSV/R4) Unix}
#     solaris      {Sun SparcStation running Solaris (SYSV/R4) Unix}
#     solaris-gcc  {Sun SparcStation running Solaris but using gcc}
#   
#   The next options are optimizations inside Crafty that you will have
#   test to see if they help.  on some machines, these will slow things
#   by up to 10%, while on other machines these options will result in
#   improving search speed up to 20%.  NOTE:  if you are running Linux
#   system, the default configurations below will use the hand-written
#   assembly modules.  Typical performance improvement is 5%, but this
#   only applies to X86 machines running Linux.
#   
#   -DSMP             enables SMP support (not clustering).  If you choose
#                     this option, you need to look closely at the following
#                     four options as well.
#   -DCPUS=n          defines the maximum number of CPUS Crafty will be able
#                     to use in a SMP system.  Note that this is the max you
#                     will be able to use.  You need to use the smpmt=n command
#                     to make crafty use more than the default 1 thread.
#   -DPOSIX           This enables POSIX threads support.  You must have either
#                     -DPOSIX or custom-written lock.h code to support your 
#                     system.
#   -DMUTEX           This uses posix threads MUTEX locks, which are not as
#                     efficient as spinlocks, but are very portable.  If this
#                     option is not chosen, then lock.h has to be modified to
#                     define inline assembly lock functions.
#
#   -DBOOKDIR         path to the directory containing the book binary files.
#                     The default for all such path values is "." if you don't
#                     specify a path with this macro definition.
#   -DDETECTDRAW      This enables experimental code that detects lots of the
#                     famous blocked-pawn positions as draws.  It is much slower
#                     and perhaps slightly risky.
#   -DDGT             This is a unix-only option to support the DGT board.
#   -DEPD             if you want full EPD support built in.
#   -DFAST            This option compiles out some of the statistics 
#                     gathering to slightly speed up the code.
#   -DFUTILITY        enables "futility pruning" a forward-pruning algorithm
#                     that seems to be relatively safe.
#   -DLOGDIR          path to the directory where Crafty puts the log.nnn and
#                     game.nnn files.
#   -DRCDIR           path to the directory where we look for the .craftyrc or
#                     crafty.rc (windows) file.
#   -DTBDIR           path to the directory where the endgame tablebase files
#                     are found.  default = "./TB"
#   -DTRACE           This enables the "trace" command so that the search tree
#                     can be dumped while running.
#   -DUSE_ASSEMBLY    Compiles with the Intel assembly code for FirstOne(),
#                     LastOne() and PopCnt().
#   -INLINE_AMD       Compiles with the Intel assembly code for FirstOne(),
#                     LastOne() and PopCnt() for the AMD opteron, only tested
#                     with the 64-bit opteron GCC compiler.
#   -INLINE_ASM       Compiles with the Intel assembly code for FirstOne(),
#                     LastOne() and PopCnt().  This is for gcc-style inlining
#                     and thoroughly breaks the Intel C/C++ compiler at the
#                     present (version 8.0).
#   

default:
	$(MAKE) -j linux-icc
help:
	@echo "You must specify the system which you want to compile for:"
	@echo ""
	@echo "make aix              IBM AIX"
	@echo "make alpha            DEC Alpha with OSF/1-Digital UNIX"
	@echo "make alpha-host       Alpha DECstation optimized for host"
	@echo "make alpha-host-nocix Alpha DECstation optimezed for host, no CIX insn"
	@echo "make darwin           Darwin on OSX"
	@echo "make hpux             HP/UX 9/10, /7xx"
	@echo "make linux            Linux optimized for i386, ELF format"
	@echo "make linux-alpha      Linux optimized for alpha"
	@echo "make linux-AMD64      Linux optimized for AMD opteron
	@echo "make freebsd          FreeBSD"
	@echo "make netbsd           NetBSD"
	@echo "make netbsd-sparc     NetBSD optimized for sparc"
	@echo "make next             NeXTSTEP"
	@echo "make os2              IBM OS/2 Warp"
	@echo "make sgi              SGI running IRIX"
	@echo "make solaris          Solaris 2.x"
	@echo "make solaris-gcc      Solaris 2.x using GNU cc"
	@echo ""
	@echo "make generic          Try this one if your system isn't listed above;"
	@echo "                      it assumes you have installed GNU cc"
	@echo ""

aix:
	$(MAKE) target=AIX \
		CC=cc CXX=$(CC) \
		CFLAGS='$(CFLAGS) -O2' \
		CXFLAGS=$(CFLAGS) \
		opt='$(opt) -DFUTILITY' \
		crafty-make

alpha:
	$(MAKE) target=ALPHA \
		CC=cc CXX=cxx \
		CFLAGS='$(CFLAGS) -std -fast -O4 -pthread -newc' \
		CXFLAGS=$(CFLAGS) \
		LDFLAGS='$(LDFLAGS) $(CFLAGS)' \
		LIBS='-lpthread -lexc' \
		opt='$(opt) -DFUTILITY -DSMP -DCPUS=8 -DFAST -DPOSIX' \
		crafty-make

alpha-host:
	$(MAKE) target=ALPHA \
		CC=cc CXX=cxx \
		CFLAGS='$(CFLAGS) -std -fast -O4 -pthread -newc -arch host' \
		CXFLAGS=$(CFLAGS) \
		LDFLAGS='$(LDFLAGS) $(CFLAGS)' \
		LIBS='-lpthread -lexc' \
		opt='$(opt) -DFUTILITY -DSMP -DCPUS=8 -DFAST -DPOSIX' \
		crafty-make

alpha-host-nocix:
	$(MAKE) target=ALPHA \
		CC=cc CXX=cxx \
		CFLAGS='$(CFLAGS) -std -fast -O4 -pthread -newc' \
		CXFLAGS='$(CFLAGS) -arch ev56 -tune host' \
		LDFLAGS='$(LDFLAGS) $(CFLAGS)' \
		LIBS='-lpthread -lexc' \
		opt='$(opt) -DFUTILITY -DSMP -DCPUS=8 -DFAST -DPOSIX' \
		crafty-make

darwin:
	$(MAKE) target=FreeBSD \
		CC=gcc CXX=g++ \
		CFLAGS='$(CFLAGS) -Wall -pipe -O3' \
		CXFLAGS=$(CFLAGS) \
		LDFLAGS=$(LDFLAGS) \
		LIBS='-lstdc++' \
		opt='$(opt) -DFUTILITY -DFAST' \
		crafty-make
		
freebsd:
	$(MAKE) target=FreeBSD \
		CC=gcc CXX='$(CC)' \
		CFLAGS='$(CFLAGS) -fomit-frame-pointer -m486 -O3 -Wall' \
		CXFLAGS=$(CFLAGS) \
		LDFLAGS=$(LDFLAGS) \
		opt='$(opt) -DFUTILITY -DUSE_ASSEMBLY -DFAST' \
		asm=X86.o \
		crafty-make

freebsd-pgcc:
	$(MAKE) target=FreeBSD \
		CC=gcc CXX='$(CC)' \
		CFLAGS='$(CFLAGS) -pipe -D_REENTRANT -mpentium -O -Wall' \
		CXFLAGS=$(CFLAGS) \
		LDFLAGS=$(LDFLAGS) \
		opt='$(opt) -DFUTILITY -DUSE_ASSEMBLY -DFAST' \
		asm=X86.o \
		crafty-make

hpux:
	$(MAKE) target=HP \
		CC=cc CXX='$(CC)' \
		CFLAGS='$(CFLAGS) +ESlit -Ae +w1' \
		CXFLAGS=$(CFLAGS) \
		LDFLAGS='$(LDFLAGS) +O3 +Onolimit $(CFLAGS)' \
		crafty-make

linux:
	$(MAKE) target=LINUX \
		CC=gcc CXX=g++ \
		CFLAGS='$(CFLAGS) -Wall -pipe -D_REENTRANT -march=i686 -O3 \
			-fbranch-probabilities -fforce-mem -fomit-frame-pointer\
			-fno-gcse -mpreferred-stack-boundary=2' \
		CXFLAGS=$(CFLAGS) \
		LDFLAGS='$(LDFLAGS) -lpthread -lstdc++' \
		opt='$(opt) -DFUTILITY -DUSE_ASSEMBLY -DFAST \
		     -DPOSIX -DSMP -DCPUS=4' \
		asm=X86.o \
		crafty-make

linux-amd64:
	$(MAKE) target=LINUX \
		CC=gcc CXX=g++ \
		CFLAGS='$(CFLAGS) -Wall -pipe -D_REENTRANT \
		-fomit-frame-pointer -O3' \
		CXFLAGS=$(CFLAGS) \
		LDFLAGS='$(LDFLAGS) -lpthread -lstdc++' \
		opt='$(opt) -DPOSIX -DFAST -DSMP -DCPUS=4 \
		-DFUTILITY -DUSE_ASSEMBLY -DINLINE_AMD' \
		crafty-make

linux-profile:
	$(MAKE) target=LINUX \
		CC=gcc CXX=g++ \
		CFLAGS='$(CFLAGS) -Wall -pipe -D_REENTRANT -march=i686 -O3 \
			-fprofile-arcs -fforce-mem \
			-fno-gcse -mpreferred-stack-boundary=2' \
		CXFLAGS=$(CFLAGS) \
		LDFLAGS='$(LDFLAGS) -fprofile-arcs -lstdc++ -lpthread' \
		opt='$(opt) -DFUTILITY -DUSE_ASSEMBLY -DFAST \
		     -DPOSIX -DSMP -DCPUS=4' \
		asm= \
		crafty-make

linux-icc-profile:
	$(MAKE) target=LINUX \
		CC=icc CXX=icc \
		CFLAGS='$(CFLAGS) -D_REENTRANT -O2 \
                        -prof_genx -prof_dir ./profdir \
                        -Ob2 -fno-alias' \
		CXFLAGS='$(CFLAGS) -D_REENTRANT -O2 \
                        -prof_genx -prof_dir ./profdir' \
		LDFLAGS='$(LDFLAGS) -lpthread' \
		opt='$(opt) -DFUTILITY -DFAST \
                         -DPOSIX -DSMP -DCPUS=4' \
		crafty-make

linux-icc:
	$(MAKE) target=LINUX \
		CC=icc CXX=icc \
		CFLAGS='$(CFLAGS) -D_REENTRANT -O2 \
                        -prof_use -prof_dir ./profdir \
                        -Ob2 -fno-alias' \
		CXFLAGS='$(CFLAGS) -D_REENTRANT -O2 \
                        -prof_use -prof_dir ./profdir' \
		LDFLAGS='$(LDFLAGS) -lpthread' \
		opt='$(opt) -DFUTILITY -DFAST \
                         -DPOSIX -DSMP -DCPUS=4' \
		crafty-make

linux-alpha:
	$(MAKE) target=ALPHA \
		CC=gcc CXX=g++ \
		CFLAGS='$(CFLAGS) -O4 -ffast-math -funroll-loops' \
		CXFLAGS=$(CFLAGS) \
		LDFLAGS='$(LDFLAGS) $(CFLAGS)' \
		LIBS='-lpthread' \
		opt='$(opt) -DFUTILITY -DSMP -DCPUS=8 -DFAST -DPOSIX \
                         -DNOBUILTINS' \
		crafty-make

netbsd:
	$(MAKE) target=NetBSD \
		CC=gcc CXX=g++ \
		CFLAGS='$(CFLAGS) -D_REENTRANT -O3 -Wall \
			-fomit-frame-pointer -funroll-all-loops \
			-finline-functions -ffast-math' \
		CXFLAGS=$(CFLAGS) \
		LDFLAGS=$(LDFLAGS) \
		opt='$(opt) -DFUTILITY -DFAST' \
		crafty-make

netbsd-i386:
	$(MAKE) target=NetBSD \
		CC=gcc CXX=g++ \
		CFLAGS='$(CFLAGS) -D_REENTRANT -O3 -Wall -m486 \
			-fomit-frame-pointer -funroll-all-loops \
			-finline-functions -ffast-math' \
		CXFLAGS=$(CFLAGS) \
		LDFLAGS=$(LDFLAGS) \
		opt='$(opt) -DFUTILITY -DFAST -DUSE_ASSEMBLY' \
		asm=X86.o \
		crafty-make

netbsd-sparc:
	$(MAKE) target=NetBSD \
		CC=gcc CXX=g++ \
		CFLAGS='$(CFLAGS) -D_REENTRANT -O3 -Wall \
			-fomit-frame-pointer -funroll-all-loops \
			-finline-functions -ffast-math' \
		CXFLAGS=$(CFLAGS) \
		LDFLAGS=$(LDFLAGS) \
		opt='$(opt) -DFUTILITY -DFAST' \
		asm=Sparc.o \
		crafty-make

next:
	$(MAKE) target=NEXT \
		CC=/bin/cc CXX='$(CC)' \
		CFLAGS='$(CFLAGS) -O2' \
		CXFLAGS=$(CFLAGS) \
		LDFLAGS='$(LDFLAGS) $(CFLAGS)'
		opt='$(opt) -DFUTILITY' \
		crafty-make

os2:
	$(MAKE) target=OS2 \
		CC=gcc CXX='$(CC)' \
		CFLAGS='$(CFLAGS) -fomit-frame-pointer -m486 -O3 -Wall' \
		CXFLAGS=$(CFLAGS) \
		LDFLAGS='$(LDFLAGS) -Zexe -Zcrtdll -s' \
		opt='$(opt) -DFUTILITY -DUSE_ASSEMBLY -DFAST' \
		asm=X86.o \
		crafty-make

sgi:
	$(MAKE) target=SGI \
		AS=/bin/as CC=cc CXX='$(CC)' \
		AFLAGS='-P' \
		CFLAGS='$(CFLAGS) -32 -mips2 -cckr' \
		CXFLAGS=$(CFLAGS) \
		LDFLAGS=$(LDFLAGS) \
		opt='$(opt) -DFUTILITY' \
		crafty-make

solaris:
	$(MAKE) target=SUN \
		AS=/usr/ccs/bin/as CC=cc CXX='$(CC)' \
		AFLAGS='-P' \
		CFLAGS='$(CFLAGS) -fast -xO5 -xunroll=20' \
		CXFLAGS=$(CFLAGS) \
		LDFLAGS='$(LDFLAGS) -lpthread' \
		opt='$(opt) -DFUTILITY -DSMP -DCPUS=4 -DMUTEX -DPOSIX' \
		crafty-make

solaris-gcc:
	$(MAKE) target=SUN \
		AS=/usr/ccs/bin/as CC=gcc CXX=g++ \
		AFLAGS='-P' \
		CFLAGS='$(CFLAGS) -Wall -pipe -D_REENTRANT -O2 \
			-fforce-mem -fomit-frame-pointer' \
		CXFLAGS=$(CFLAGS) \
		LDFLAGS='$(LDFLAGS) -lstdc++' \
		opt='$(opt) -DFUTILITY -DFAST' \
		crafty-make

generic:
	$(MAKE) CC=gcc CXX=g++ \
		CXFLAGS=$(CFLAGS) \
		LDFLAGS=$(LDFLAGS) \
		crafty-make

profile:
	@rm -rf profdir
	@rm -rf position.bin
	@mkdir profdir
	@touch *.c *.cpp *.h
	$(MAKE) linux-icc-profile
	@echo "#!/bin/csh" > runprof
	@echo "./crafty <<EOF" >>runprof
	@echo "st=10" >>runprof
	@echo "ponder=off" >>runprof
	@echo "display nomoves" >>runprof
	@echo "setboard rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq" >>runprof
	@echo "move" >>runprof
	@echo "book off" >>runprof
	@echo "setboard rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq" >>runprof
	@echo "move" >>runprof
	@echo "setboard 1k1r4/pp1b1R2/3q2pp/4p3/2B5/4Q3/PPP2B2/2K5 b" >>runprof
	@echo "move" >>runprof
	@echo "setboard 3r1k2/4npp1/1ppr3p/p6P/P2PPPP1/1NR5/5K2/2R5 w" >>runprof
	@echo "move" >>runprof
	@echo "setboard 2q1rr1k/3bbnnp/p2p1pp1/2pPp3/PpP1P1P1/1P2BNNP/2BQ1PRK/7R b" >>runprof
	@echo "move" >>runprof
	@echo "setboard rnbqkb1r/p3pppp/1p6/2ppP3/3N4/2P5/PPP1QPPP/R1B1KB1R w KQkq" >>runprof
	@echo "move" >>runprof
	@echo "setboard r1b2rk1/2q1b1pp/p2ppn2/1p6/3QP3/1BN1B3/PPP3PP/R4RK1 w" >>runprof
	@echo "move" >>runprof
	@echo "setboard 2r3k1/pppR1pp1/4p3/4P1P1/5P2/1P4K1/P1P5/8 w" >>runprof
	@echo "move" >>runprof
	@echo "setboard 1nk1r1r1/pp2n1pp/4p3/q2pPp1N/b1pP1P2/B1P2R2/2P1B1PP/R2Q2K1 w" >>runprof
	@echo "move" >>runprof
	@echo "setboard 4b3/p3kp2/6p1/3pP2p/2pP1P2/4K1P1/P3N2P/8 w" >>runprof
	@echo "move" >>runprof
	@echo "setboard 2kr1bnr/pbpq4/2n1pp2/3p3p/3P1P1B/2N2N1Q/PPP3PP/2KR1B1R w" >>runprof
	@echo "move" >>runprof
	@echo "setboard 3rr1k1/pp3pp1/1qn2np1/8/3p4/PP1R1P2/2P1NQPP/R1B3K1 b" >>runprof
	@echo "move" >>runprof
	@echo "setboard 2r1nrk1/p2q1ppp/bp1p4/n1pPp3/P1P1P3/2PBB1N1/4QPPP/R4RK1 w" >>runprof
	@echo "move" >>runprof
	@echo "setboard r3r1k1/ppqb1ppp/8/4p1NQ/8/2P5/PP3PPP/R3R1K1 b" >>runprof
	@echo "move" >>runprof
	@echo "setboard r2q1rk1/4bppp/p2p4/2pP4/3pP3/3Q4/PP1B1PPP/R3R1K1 w" >>runprof
	@echo "move" >>runprof
	@echo "setboard rnb2r1k/pp2p2p/2pp2p1/q2P1p2/8/1Pb2NP1/PB2PPBP/R2Q1RK1 w" >>runprof
	@echo "move" >>runprof
	@echo "setboard 2r3k1/1p2q1pp/2b1pr2/p1pp4/6Q1/1P1PP1R1/P1PN2PP/5RK1 w" >>runprof
	@echo "move" >>runprof
	@echo "setboard r1bqkb1r/4npp1/p1p4p/1p1pP1B1/8/1B6/PPPN1PPP/R2Q1RK1 w kq" >>runprof
	@echo "move" >>runprof
	@echo "setboard r2q1rk1/1ppnbppp/p2p1nb1/3Pp3/2P1P1P1/2N2N1P/PPB1QP2/R1B2RK1 b" >>runprof
	@echo "move" >>runprof
	@echo "setboard r1bq1rk1/pp2ppbp/2np2p1/2n5/P3PP2/N1P2N2/1PB3PP/R1B1QRK1 b" >>runprof
	@echo "move" >>runprof
	@echo "setboard 3rr3/2pq2pk/p2p1pnp/8/2QBPP2/1P6/P5PP/4RRK1 b" >>runprof
	@echo "move" >>runprof
	@echo "setboard r4k2/pb2bp1r/1p1qp2p/3pNp2/3P1P2/2N3P1/PPP1Q2P/2KRR3 w" >>runprof
	@echo "move" >>runprof
	@echo "setboard 3rn2k/ppb2rpp/2ppqp2/5N2/2P1P3/1P5Q/PB3PPP/3RR1K1 w" >>runprof
	@echo "move" >>runprof
	@echo "setboard 2r2rk1/1bqnbpp1/1p1ppn1p/pP6/N1P1P3/P2B1N1P/1B2QPP1/R2R2K1 b" >>runprof
	@echo "move" >>runprof
	@echo "setboard r1bqk2r/pp2bppp/2p5/3pP3/P2Q1P2/2N1B3/1PP3PP/R4RK1 b kq" >>runprof
	@echo "move" >>runprof
	@echo "setboard r2qnrnk/p2b2b1/1p1p2pp/2pPpp2/1PP1P3/PRNBB3/3QNPPP/5RK1 w" >>runprof
	@echo "move" >>runprof
	@echo "setboard /k/3p/p2P1p/P2P1P///K/ w" >>runprof
	@echo "move" >>runprof
	@echo "setboard /k/rnn////5RBB/K/ w" >>runprof
	@echo "move" >>runprof
	@echo "mt=0" >>runprof
	@echo "quit" >>runprof
	@echo "EOF" >>runprof
	@chmod +x runprof
	@./runprof
	@echo "#!/bin/csh" > runprof
	@echo "./crafty <<EOF" >>runprof
	@echo "st=10" >>runprof
	@echo "ponder=off" >>runprof
	@echo "mt=4" >>runprof
	@echo "setboard 2r2rk1/1bqnbpp1/1p1ppn1p/pP6/N1P1P3/P2B1N1P/1B2QPP1/R2R2K1 b" >>runprof
	@echo "move" >>runprof
	@echo "mt=0" >>runprof
	@echo "quit" >>runprof
	@echo "EOF" >>runprof
	@chmod +x runprof
	@./runprof
	@rm runprof
	@touch *.c *.cpp *.h
	$(MAKE) linux-icc

#
#  one of the two following definitions for "objects" should be used.  The
#  default is to compile everything separately.  However, if you use the 
#  definition that refers to crafty.o, that will compile using the file crafty.c
#  which #includes every source file into one large glob.  This gives the
#  compiler max opportunity to inline functions as appropriate.  You should try
#  compiling both ways to see which way produces the fastest code.
#

objects = searchr.o search.o thread.o searchmp.o repeat.o next.o nexte.o      \
       nextr.o history.o quiesce.o evaluate.o movgen.o make.o unmake.o hash.o  \
       attacks.o swap.o boolean.o utility.o valid.o book.o data.o      \
       drawn.o edit.o epd.o epdglue.o init.o input.o interupt.o iterate.o      \
       main.o option.o output.o phase.o ponder.o preeval.o resign.o root.o     \
       learn.o setboard.o test.o time.o validate.o annotate.o analyze.o        \
       evtest.o bench.o dgt.o $(asm)
#objects = crafty.o $(asm)

# Do not change anything below this line!

opts = $(opt) -D$(target)

includes = data.h chess.h

crafty-make:
	@$(MAKE) opt='$(opt)' asm='$(asm)' crafty

crafty:	$(objects) 
	$(CC) $(LDFLAGS) -o crafty $(objects) -lm  $(LIBS)

dgt:    dgtdrv.o
	@cc -O -o dgt dgtdrv.c

egtb.o: egtb.cpp
	$(CXX) -c $(CXFLAGS) $(opts) egtb.cpp
clean:
	-rm -f *.o crafty

$(objects): $(includes)

.c.o:
	$(CC) $(CFLAGS) $(opts) -c $*.c

.s.o:
	$(AS) $(AFLAGS) -o $*.o $*.s
