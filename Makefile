# To build crafty:
#
#  You want to set up for maximum optimization, but typically you will
#  need to experiment to see which options provide the fastest code.  The
#  following option descriptions explain each option.  To use one or more
#  of these options, to the "opt =" line that follows the explanations and
#  add the options you want, being careful to stay inside the single quote
#  marks.
#   
#   -DAFFINITY     Include code to set processor/thread affinity on Unix
#                  systems.  Note this will not work on Apple OS X as it does
#                  not support any sort of processor affinity mechanism.
#   -DBOOKDIR      Path to the directory containing the book binary files.
#                  The default for all such path values is "." if you don't
#                  specify a path with this macro definition.
#   -DCPUS=n       Defines the maximum number of CPUS Crafty will be able
#                  to use in a SMP system.  Note that this is the max you
#                  will be able to use.  You need to use the smpmt=n command
#                  to make crafty use more than the default 1 process.
#   -DDEBUG        This is used for testing changes.  Enabling this option
#                  greatly slows Crafty down, but every time a move is made,
#                  the corresponding position is checked to make sure all of
#                  the bitboards are set correctly.
#   -DEPD          If you want full EPD support built in.
#   -DINLINEASM    Compiles with the Intel assembly code for FirstOne(),
#                  LastOne() and PopCnt().  This is for gcc-style inlining
#                  and now works with the Intel C/C++ compiler as well.  It
#                  also has a MSVC compatible version included.
#   -DLOGDIR       Path to the directory where Crafty puts the log.nnn and
#                  game.nnn files.
#   -DNODES        This enables the sn=x command.  Crafty will search until
#                  exactly X nodes have been searched, then the search 
#                  terminates as if time ran out.
#   -DNOEGTB       Eliminates the egtb code for compilers that can't deal
#                  with the large egtb.cpp code/templates.
#   -DNUMA         Says this system is NUMA, which is mainly used for Linux
#                  or Windows systems, and references libnuma, needed for the
#                  NUMA calls (crafty doesn't use many of these, it does the
#                  memory setup stuff itself)
#   -DPOPCNT       Says this system is a newer Intel/AMD processor with the
#                  built-in hardware popcnt instruction.
#   -DPOSITIONS    Causes Crafty to emit FEN strings, one per book line, as
#                  it creates a book.  I use this to create positions to use
#                  for cluster testing.
#   -DRCDIR        Path to the directory where we look for the .craftyrc or
#                  crafty.rc (windows) file.
#   -DSKILL        Enables the "skill" command which allows you to arbitrarily
#                  lower Crafty's playing skill so it does not seem so
#                  invincible to non-GM-level players.
#   -DTBDIR        Path to the directory where the endgame tablebase files
#                  are found.  default = "./TB"
#   -DTEST         Displays evaluation table after each move (in the logfile)
#   -DTRACE        This enables the "trace" command so that the search tree
#                  can be dumped while running.
#   -DUNIX         This identifies the target O/S as being Unix-based, if this
#                  option is omitted, windows is assumed.

default:
	$(MAKE) -j unix-gcc
help:
	@echo "You must specify the system which you want to compile for:"
	@echo ""
	@echo "make unix-gcc         Unix w/gcc compiler"
	@echo "make unix-icc         Unix w/icc compiler"
	@echo "make profile          profile-guided-optimizations"
	@echo "                      (edit Makefile to make the profile"
	@echo "                      option use the right compiler)"
	@echo ""

quick:
	$(MAKE) target=UNIX \
		CC=gcc CXX=g++ \
		opt='-DTEST -DTRACE -DINLINEASM -DCPUS=8' \
		CFLAGS='-Wall -O3 -pipe -pthread' \
		CXFLAGS='-Wall -pipe -O3 -pthread' \
		LDFLAGS='$(LDFLAGS) -pthread -lstdc++' \
		crafty-make

unix-gcc:
	$(MAKE) target=UNIX \
		CC=gcc CXX=g++ \
		opt='-DTEST -DINLINEASM -DCPUS=8' \
		CFLAGS='-Wall -pipe -O3 -fprofile-use -fprofile-correction -pthread' \
		CXFLAGS='-Wall -pipe -O3 -fprofile-use -fprofile-correction -pthread' \
		LDFLAGS='$(LDFLAGS) -fprofile-use -pthread -lstdc++' \
		crafty-make

unix-gcc-profile:
	$(MAKE) target=UNIX \
		CC=gcc CXX=g++ \
		opt='-DTEST -DINLINEASM -DCPUS=8' \
		CFLAGS='-Wall -pipe -O3 -fprofile-arcs -pthread' \
		CXFLAGS='-Wall -pipe -O3 -fprofile-arcs -pthread' \
		LDFLAGS='$(LDFLAGS) -fprofile-arcs -pthread -lstdc++ ' \
		crafty-make

unix-icc:
	$(MAKE) target=UNIX \
		CC=icc CXX=icc \
		opt='-DTEST -DINLINEASM -DCPUS=8' \
		CFLAGS='-Wall -w -O2 -prof_use -prof_dir ./prof -fno-alias -pthread' \
		CXFLAGS='-Wall -w -O2 -prof_use -prof_dir ./prof -pthread' \
		LDFLAGS='$(LDFLAGS) -pthread -lstdc++' \
		crafty-make

unix-icc-profile:
	$(MAKE) target=UNIX \
		CC=icc CXX=icc \
		opt='-DTEST -DINLINEASM -DCPUS=8' \
		CFLAGS='-Wall -w -O2 -prof_genx -prof_dir ./prof -fno-alias -pthread' \
		CXFLAGS='-Wall -w -O2 -prof_genx -prof_dir ./prof -pthread' \
		LDFLAGS='$(LDFLAGS) -pthread -lstdc++ ' \
		crafty-make

profile:
	@rm -rf prof
	@rm -rf *.gcda
	@mkdir prof
	@touch *.c *.cpp *.h
	$(MAKE) unix-gcc-profile
	@echo "#!/bin/csh" > runprof
	@echo "./crafty <<EOF" >>runprof
	@echo "st=10" >>runprof
	@echo "mt=0" >>runprof
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
	@echo "egtb" >>runprof
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
	@echo "mt=2" >>runprof
	@echo "setboard 2r2rk1/1bqnbpp1/1p1ppn1p/pP6/N1P1P3/P2B1N1P/1B2QPP1/R2R2K1 b" >>runprof
	@echo "move" >>runprof
	@echo "mt=2" >>runprof
	@echo "quit" >>runprof
	@echo "EOF" >>runprof
	@chmod +x runprof
	@./runprof
	@rm runprof
	@touch *.c *.cpp *.h
	$(MAKE) unix-gcc

#
#  one of the two following definitions for "objects" should be used.  The
#  default is to compile everything separately.  However, if you use the 
#  definition that refers to crafty.o, that will compile using the file crafty.c
#  which #includes every source file into one large glob.  This gives the
#  compiler max opportunity to inline functions as appropriate.  You should try
#  compiling both ways to see which way produces the fastest code.
#

#objects = search.o thread.o repeat.o next.o killer.o quiesce.o evaluate.o     \
       movgen.o make.o unmake.o hash.o  attacks.o swap.o boolean.o  utility.o  \
       probe.o book.o data.o drawn.o edit.o epd.o epdglue.o init.o input.o     \
       interrupt.o iterate.o main.o option.o output.o ponder.o resign.o root.o \
       learn.o setboard.o test.o time.o validate.o annotate.o analyze.o        \
       evtest.o bench.o
objects = crafty.o

# Do not change anything below this line!

opts = $(opt) -D$(target)

includes = data.h chess.h

crafty-make:
	@$(MAKE) -j opt='$(opt)' CXFLAGS='$(CXFLAGS)' CFLAGS='$(CFLAGS)' crafty

crafty.o: *.c *.h

crafty:	$(objects) egtb.o
	$(CC) $(LDFLAGS) -o crafty $(objects) egtb.o -lm  $(LIBS)

egtb.o: egtb.cpp
	$(CXX) -c $(CXFLAGS) $(opts) egtb.cpp
clean:
	-rm -f *.o crafty

$(objects): $(includes)

.c.o:
	$(CC) $(CFLAGS) $(opts) -c $*.c

.s.o:
	$(AS) $(AFLAGS) -o $*.o $*.s
