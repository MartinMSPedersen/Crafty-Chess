# To build crafty:
#
#	  You want to set up for maximum optimization, but typically you will
#	  need to experiment to see which options provide the fastest code.
#	  This is optimized for pgcc, which is a fairly current compiler.
#   
#   The currently available targets:
#
#     AIX        {IBM machines running AIX}
#     ALPHA      {DEC Alpha running OSF/1-Digital Unix}
#     ALPHALINUX {DEC Alpha running Linux}
#     CRAY1      {any Cray-1 compatible architecture including XMP, YMP, 
#                 C90, etc.}
#     HP         {HP workstation running HP_UX operating system (Unix)}
#     LINUX      {80X86 architecture running LINUX (Unix)}
#     NT_i386    {80X86 architecture running Windows 95 or NT}
#     NT_AXP     {DEC Alpha running Windows NT}
#     DOS        {PC running dos/windows, using DJGPP port of gcc to compile}
#     NEXT       {NextStep}
#     OS/2       {IBM OS/2 warp}
#     SGI        {SGI Workstation running Irix (SYSV/R4) Unix}
#     SUN        {Sun SparcStation running Solaris (SYSV/R4) Unix}
#     SUN_GCC    {Sun SparcStation running Solaris but using gcc
#     FreeBSD    {80X86 architecture running FreeBSD (Unix)}
#     NetBSD     {multi-architecture running NetBSD (Unix)}
#     Cygwin     {80X86 running Cygwin under Win32 (Unix)}
#   
#   The next options are optimizations inside Crafty that you will have
#   test to see if they help.  on some machines, these will slow things
#   by up to 10%, while on other machines these options will result in
#   improving search speed up to 20%.  NOTE:  if you are running Linux
#   or have a SUN Sparc-20 machine, the default configurations below
#   will use the hand-written assembly modules.  Typical performance
#   improvement is 33%, but this only applies to X86 machines and the
#   Sun Sparc-20.
#   
#   1.  opt = -DCOMPACT_ATTACKS
#   2.  opt = -DCOMPACT_ATTACKS -DUSE_ATTACK_FUNCTIONS
#   
#   Finally, if you have a Symmetric MultiProcessor machine, you should
#   add -DSMP to the opt definition for your make configuration, and then
#   add -DCPUS=N where N is the number of processors (max) you will use.
#   
#   if you want 6 man EGTB support, you will need to add -DEGTB6 to the
#   options above.

default:
	$(MAKE) -j4  linux-icc-elf
help:
	@echo "You must specify the system which you want to compile for:"
	@echo ""
	@echo "make aix              IBM AIX"
	@echo "make alpha            DEC Alpha with OSF/1-Digital UNIX"
	@echo "make alpha-host       Alpha DECstation optimized for host"
	@echo "make alpha-host-nocix Alpha DECstation optimezed for host, no CIX insn"
	@echo "make cygwin           Cygwin under Win32"
	@echo "make dos              DOS on i386 with DJGPP"
	@echo "make hpux             HP/UX 9/10, /7xx"
	@echo "make linux            Linux optimized for i386"
	@echo "make linux-elf        Linux optimized for i386, ELF format"
	@echo "make linux-i686       Linux optimized for i686"
	@echo "make linux-i686-elf   Linux optimized for i686, ELF format"
	@echo "make linux-alpha      Linux optimized for alpha"
	@echo "make freebsd          FreeBSD"
	@echo "make freebsd-pgcc     FreeBSD using Pentium GNU cc"
	@echo "make netbsd           NetBSD"
	@echo "make netbsd-i386      NetBSD optimized for i386"
	@echo "make netbsd-i386-elf  NetBSD optimized for i386, ELF format"
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
		CC=cc CXX='$$(CC)' \
		CFLAGS='$(CFLAGS} -O2' \
		CXFLAGS='$(CFLAGS}' \
		LDFLAGS='$(LDFLAGS)' \
		opt='$(opt) -DCOMPACT_ATTACKS -DUSE_ATTACK_FUNCTIONS' \
		crafty-make

#Note: "-arch host" assumes you will run the binary on exactly the
# same kind of ALPHA you compiled it on.  Omit it if you want to run
# the same binary on several kinds of Alpha.  If you are on an early
# EV6 that does not have the CIX instruction set extension, a compiler
# bug (?) causes these instructions to be generated anyway.  If this
# happens you'll see a message about "instr emulated" after starting
# crafty; to fix it, change "-arch host" to "-arch ev56 -tune host"
# and recompile.

alpha:
	$(MAKE) target=ALPHA \
		CC=cc CXX=cxx \
		CFLAGS='$(CFLAGS) -std -fast -O4 -pthread -newc' \
		CXFLAGS='$(CFLAGS}' \
		LDFLAGS='$(LDFLAGS) $(CFLAGS)' \
		LIBS='-lpthread -lexc' \
		opt='$(opt) -DSMP -DCPUS=8 -DFAST -DPOSIX' \
		crafty-make

alpha-host:
	$(MAKE) target=ALPHA \
		CC=cc CXX=cxx \
		CFLAGS='$(CFLAGS) -std -fast -O4 -pthread -newc -arch host' \
		CXFLAGS='$(CFLAGS}' \
		LDFLAGS='$(LDFLAGS) $(CFLAGS)' \
		LIBS='-lpthread -lexc' \
		opt='$(opt) -DSMP -DCPUS=8 -DFAST -DPOSIX' \
		crafty-make

alpha-host-nocix:
	$(MAKE) target=ALPHA \
		CC=cc CXX=cxx \
		CFLAGS='$(CFLAGS) -std -fast -O4 -pthread -newc \
		CXFLAGS='$(CFLAGS}' \
			-arch ev56 -tune host' \
		LDFLAGS='$(LDFLAGS) $(CFLAGS)' \
		LIBS='-lpthread -lexc' \
		opt='$(opt) -DSMP -DCPUS=8 -DFAST -DPOSIX' \
		crafty-make

cygwin:
	$(MAKE) target=LINUX \
		CC=gcc CXX='$$(CC)' \
		CFLAGS='$(CFLAGS) -pipe -D_REENTRANT -mpentium -O2 -Wall' \
		CXFLAGS='$(CFLAGS}' \
		LDFLAGS='$(LDFLAGS)' \
		opt='$(opt) -DCOMPACT_ATTACKS -DUSE_ATTACK_FUNCTIONS \
		     -DUSE_ASSEMBLY_A -DUSE_ASSEMBLY_B -DFAST' \
		asm=X86-aout.o \
		crafty-make

dos:
	$(MAKE) target=DOS \
		CC=gcc CXX='$$(CC)' \
		CFLAGS='$(CFLAGS) -fomit-frame-pointer -m486 -O3' \
		CXFLAGS='$(CFLAGS}' \
		LDFLAGS='$(LDFLAGS)' \
		opt='$(opt) -DCOMPACT_ATTACKS -DUSE_ATTACK_FUNCTIONS \
		     -DUSE_ASSEMBLY_A -DUSE_ASSEMBLY_B' \
		asm='X86.o' \
		crafty-make

freebsd:
	$(MAKE) target=FreeBSD \
		CC=gcc CXX='$$(CC)' \
		CFLAGS='$(CFLAGS) -fomit-frame-pointer -m486 -O3 -Wall' \
		CXFLAGS='$(CFLAGS}' \
		LDFLAGS='$(LDFLAGS)' \
		opt='$(opt) -DCOMPACT_ATTACKS -DUSE_ATTACK_FUNCTIONS \
		     -DUSE_ASSEMBLY_A -DUSE_ASSEMBLY_B -DFAST' \
		asm=X86-elf.o \
		crafty-make

freebsd-pgcc:
	$(MAKE) target=FreeBSD \
		CC=gcc CXX='$$(CC)' \
		CFLAGS='$(CFLAGS) -pipe -D_REENTRANT -mpentium -O -Wall' \
		CXFLAGS='$(CFLAGS}' \
		LDFLAGS='$(LDFLAGS)' \
		opt='$(opt) -DCOMPACT_ATTACKS -DUSE_ATTACK_FUNCTIONS \
		     -DUSE_ASSEMBLY_A -DUSE_ASSEMBLY_B -DFAST' \
		asm=X86-elf.o \
		crafty-make

hpux:
	$(MAKE) target=HP \
		CC=cc CXX='$$(CC)' \
		CFLAGS='$(CFLAGS) +ESlit -Ae +w1' \
		CXFLAGS='$(CFLAGS}' \
		LDFLAGS='$(LDFLAGS) +O3 +Onolimit $(CFLAGS)' \
		crafty-make

linux:
	$(MAKE) target=LINUX \
		CC=gcc CXX=g++ \
		CFLAGS='$(CFLAGS) -Wall -pipe -D_REENTRANT -O3 \
		CXFLAGS='$(CFLAGS}' \
			-fforce-mem -fomit-frame-pointer' \
		LDFLAGS='$(LDFLAGS) -lpthread' \
		opt='$(opt) -DCOMPACT_ATTACKS -DUSE_ATTACK_FUNCTIONS \
		     -DUSE_ASSEMBLY_A -DUSE_ASSEMBLY_B -DFAST' \
		asm=X86-aout.o \
		crafty-make

linux-elf:
	$(MAKE) target=LINUX \
		CC=gcc CXX=g++ \
		CFLAGS='$(CFLAGS) -Wall -pipe -D_REENTRANT -O3 \
			-fforce-mem -fomit-frame-pointer' \
		CXFLAGS='$(CFLAGS}' \
		LDFLAGS='$(LDFLAGS) -lpthread' \
		opt='$(opt) -DCOMPACT_ATTACKS -DUSE_ATTACK_FUNCTIONS \
		     -DUSE_ASSEMBLY_A -DUSE_ASSEMBLY_B -DFAST' \
		asm=X86-elf.o \
		crafty-make

linux-i686:
	$(MAKE) target=LINUX \
		CC=gcc CXX=g++ \
		CFLAGS='$(CFLAGS) -Wall -pipe -D_REENTRANT -march=i686 -O \
			-fforce-mem -fomit-frame-pointer \
			-mpreferred-stack-boundary=2' \
		CXFLAGS='$(CFLAGS}' \
		LDFLAGS='$(LDFLAGS) -lpthread' \
		opt='$(opt) -DCOMPACT_ATTACKS -DUSE_ATTACK_FUNCTIONS \
		     -DUSE_ASSEMBLY_A -DUSE_ASSEMBLY_B -DFAST \
		     -DSMP -DCPUS=4 -DDGT' \
		asm=X86-aout.o \
		crafty-make

linux-i686-elf:
	$(MAKE) target=LINUX \
		CC=gcc CXX=g++ \
		CFLAGS='$(CFLAGS) -Wall -pipe -D_REENTRANT -march=i686 -O \
			-fforce-mem -fomit-frame-pointer \
			-fno-gcse -mpreferred-stack-boundary=2' \
		CXFLAGS='$(CFLAGS}' \
		LDFLAGS='$(LDFLAGS)' \
		opt='$(opt) -DCOMPACT_ATTACKS -DUSE_ATTACK_FUNCTIONS \
		     -DUSE_ASSEMBLY_A -DUSE_ASSEMBLY_B -DFAST \
		     -DSMP -DCPUS=4 -DCLONE -DDGT' \
		asm=X86-elf.o \
		crafty-make

linux-icc-elf-profile:
	$(MAKE) target=LINUX \
		CC=icc CXX=gcc \
		CFLAGS='$(CFLAGS) -D_REENTRANT -O2 \
			-prof_gen -prof_dir ./profdir -fno-alias -tpp6' \
		CXFLAGS='$(CXFLAGS) -Wall -pipe -D_REENTRANT -march=i686 -O \
			-fforce-mem -fomit-frame-pointer \
			-fno-gcse -mpreferred-stack-boundary=2' \
		LDFLAGS='$(LDFLAGS)' \
		opt='$(opt) -DCOMPACT_ATTACKS -DUSE_ATTACK_FUNCTIONS \
		     -DUSE_ASSEMBLY_A -DUSE_ASSEMBLY_B -DFAST \
		     -DCLONE -DDGT' \
		asm=X86-elf.o \
		crafty-make

linux-icc-elf:
	$(MAKE) target=LINUX \
		CC=icc CXX=gcc \
		CFLAGS='$(CFLAGS) -D_REENTRANT -O2 \
			-prof_use -prof_dir ./profdir -fno-alias -tpp6' \
		CXFLAGS='$(CXFLAGS) -Wall -pipe -D_REENTRANT -march=i686 -O \
			-fforce-mem -fomit-frame-pointer \
			-fno-gcse -mpreferred-stack-boundary=2' \
		LDFLAGS='$(LDFLAGS)' \
		opt='$(opt) -DCOMPACT_ATTACKS -DUSE_ATTACK_FUNCTIONS \
		     -DUSE_ASSEMBLY_A -DUSE_ASSEMBLY_B -DFAST \
		     -DCLONE -DDGT' \
		asm=X86-elf.o \
		crafty-make

# You may wish to add additional targets called linux-alpha-<your_cpu>
# to produce optimized code for your CPU.  Just copy the linux-alpha target
# and add -mcpu=<your_cpu> to the CFLAGS, where the type of your CPU is
# ev4,ev45... or 21064,21064A...  Or, just type
#
# 	make linux-alpha CFLAGS='-mcpu=<your_cpu>'
#
# If you have the Compaq C Compiler for AlphaLinux you can link the
# machine/builtins.h from ccc's private include file to /usr/include/alpha,
# link the directory alpha to machine and remove -DNOBUILTINS from the
# opt-line.
#
# THIS TARGET IS EXPERIMENTAL !!!

linux-alpha:
	$(MAKE) target=ALPHA \
		CC=gcc CXX=g++ \
		CFLAGS='$(CFLAGS) -O4 -ffast-math -funroll-loops' \
		CXFLAGS='$(CFLAGS}' \
		LDFLAGS='$(LDFLAGS) $(CFLAGS)' \
		LIBS='-lpthread' \
		opt='$(opt) -DSMP -DCPUS=8 -DFAST -DPOSIX -DNOBUILTINS' \
		crafty-make

netbsd:
	$(MAKE) target=NetBSD \
		CC=gcc CXX=g++ \
		CFLAGS='$(CFLAGS) -D_REENTRANT -O3 -Wall \
			-fomit-frame-pointer -funroll-all-loops \
			-finline-functions -ffast-math' \
		CXFLAGS='$(CFLAGS}' \
		LDFLAGS='$(LDFLAGS)' \
		opt='$(opt) -DCOMPACT_ATTACKS -DUSE_ATTACK_FUNCTIONS -DFAST' \
		crafty-make

netbsd-i386:
	$(MAKE) target=NetBSD \
		CC=gcc CXX=g++ \
		CFLAGS='$(CFLAGS) -D_REENTRANT -O3 -Wall -m486 \
			-fomit-frame-pointer -funroll-all-loops \
			-finline-functions -ffast-math' \
		CXFLAGS='$(CFLAGS}' \
		LDFLAGS='$(LDFLAGS)' \
		opt='$(opt) -DCOMPACT_ATTACKS -DUSE_ATTACK_FUNCTIONS -DFAST \
		     -DUSE_ASSEMBLY_A -DUSE_ASSEMBLY_B' \
		asm=X86-aout.o \
		crafty-make

netbsd-i386-elf:
	$(MAKE) target=NetBSD \
		CC=gcc CXX=g++ \
		CFLAGS='$(CFLAGS) -D_REENTRANT -O3 -Wall -m486 \
			-fomit-frame-pointer -funroll-all-loops \
			-finline-functions -ffast-math' \
		CXFLAGS='$(CFLAGS}' \
		LDFLAGS='$(LDFLAGS)' \
		opt='$(opt) -DCOMPACT_ATTACKS -DUSE_ATTACK_FUNCTIONS -DFAST \
		     -DUSE_ASSEMBLY_A -DUSE_ASSEMBLY_B' \
		asm=X86-elf.o \
		crafty-make

netbsd-sparc:
	$(MAKE) target=NetBSD \
		CC=gcc CXX=g++ \
		CFLAGS='$(CFLAGS) -D_REENTRANT -O3 -Wall \
			-fomit-frame-pointer -funroll-all-loops \
			-finline-functions -ffast-math' \
		CXFLAGS='$(CFLAGS}' \
		LDFLAGS='$(LDFLAGS)' \
		opt='$(opt) -DCOMPACT_ATTACKS -DUSE_ATTACK_FUNCTIONS -DFAST \
		     -DUSE_ASSEMBLY_A' \
		asm=Sparc.o \
		crafty-make

next:
	$(MAKE) target=NEXT \
		CC=/bin/cc CXX='$$(CC)' \
		CFLAGS='$(CFLAGS) -O2' \
		CXFLAGS='$(CFLAGS}' \
		LDFLAGS='$(LDFLAGS) $(CFLAGS)'
		opt='$(opt) -DCOMPACT_ATTACKS' \
		crafty-make

os2:
	$(MAKE) target=OS2 \
		CC=gcc CXX='$$(CC)' \
		CFLAGS='$(CFLAGS) -fomit-frame-pointer -m486 -O3 -Wall' \
		CXFLAGS='$(CFLAGS}' \
		LDFLAGS='$(LDFLAGS) -Zexe -Zcrtdll -s' \
		opt='$(opt) -DCOMPACT_ATTACKS -DUSE_ATTACK_FUNCTIONS \
		     -DUSE_ASSEMBLY_A -DUSE_ASSEMBLY_B -DFAST' \
		asm=X86.o \
		crafty-make

sgi:
	$(MAKE) target=SGI \
		AS=/bin/as CC=cc CXX='$$(CC)' \
		AFLAGS='-P' \
		CFLAGS='$(CFLAGS) -g -32 -mips2 -cckr' \
		CXFLAGS='$(CFLAGS}' \
		LDFLAGS='$(LDFLAGS)' \
		opt='$(opt) -DCOMPACT_ATTACKS -DUSE_ATTACK_FUNCTIONS' \
		crafty-make

solaris:
	$(MAKE) target=SUN \
		AS=/usr/ccs/bin/as CC=cc CXX='$$(CC)' \
		AFLAGS='-P' \
		CFLAGS='$(CFLAGS) -fast -xO5 -xunroll=20' \
		CXFLAGS='$(CFLAGS}' \
		LDFLAGS='$(LDFLAGS) -lpthread' \
		opt='$(opt) -DCOMPACT_ATTACKS -DUSE_ATTACK_FUNCTIONS \
		     -DUSE_ASSEMBLY_A -DSMP -DCPUS=4 -DMUTEX -DPOSIX' \
		asm=Sparc.o \
		crafty-make

solaris-gcc:
	$(MAKE) target=SUN \
		AS=/usr/ccs/bin/as CC=gcc CXX=g++ \
		AFLAGS='-P' \
		CFLAGS='$(CFLAGS) -Wall -pipe -D_REENTRANT -O2 \
			-fforce-mem -fomit-frame-pointer' \
		CXFLAGS='$(CFLAGS}' \
		LDFLAGS='$(LDFLAGS)' \
		opt='$(opt) -DCOMPACT_ATTACKS -DUSE_ATTACK_FUNCTIONS \
		     -DUSE_ASSEMBLY_A' \
		asm=Sparc.o \
		crafty-make

generic:
	$(MAKE) CC=gcc CXX=g++ \
		CFLAGS='$(CFLAGS)' \
		CXFLAGS='$(CFLAGS}' \
		LDFLAGS='$(LDFLAGS)' \
		crafty-make

profile:
	touch *.c
	rm -rf profdir
	mkdir profdir
	$(MAKE) linux-icc-elf-profile
	./runprof
	touch *.c
	$(MAKE)

# Do not change anything below this line!

opts = $(opt) -D$(target)

objects = searchr.o search.o thread.o searchmp.o repeat.o next.o nexte.o      \
       nextr.o history.o quiesce.o evaluate.o movgen.o make.o unmake.o hash.o \
       attacks.o swap.o boolean.o utility.o valid.o probe.o book.o data.o     \
       drawn.o edit.o epd.o epdglue.o init.o input.o interupt.o iterate.o     \
       main.o option.o output.o phase.o ponder.o preeval.o resign.o root.o    \
       learn.o setboard.o test.o time.o validate.o annotate.o enprise.o       \
       analyze.o evtest.o bench.o egtb.o dgt.o $(asm)

includes = data.h chess.h

epdincludes = epd.h epddefs.h epdglue.h

eval_users = data.o evaluate.o preeval.o 

crafty-make:
	@$(MAKE) \
		AS='$(AS)' CC='$(CC)' CXX='$(CXX)' \
		AFLAGS='$(AFLAGS)' CFLAGS='$(CFLAGS)' LDFLAGS='$(LDFLAGS)' \
		opt='$(opt)' asm='$(asm)' \
		crafty

crafty:	$(objects) 
	$(CC) $(LDFLAGS) -o crafty $(objects) -lm  $(LIBS)
	@rm -f X86-elf.S
	@rm -f X86-aout.S

dgt:    dgtdrv.o
	@cc -O -o dgt dgtdrv.c

egtb.o: egtb.cpp
	$(CXX) -c $(CXFLAGS) $(opts) egtb.cpp
clean:
	-rm -f *.o crafty X86-elf.X X86-aout.S

$(objects): $(includes)

$(eval_users): evaluate.h

epd.o epdglue.o option.o init.o : $(epdincludes)

.c.o:
	$(CC) $(CFLAGS) $(opts) -c $*.c

.s.o:
	$(AS) $(AFLAGS) -o $*.o $*.s

X86-aout.o:
	sed -e 's/ALIGN/4/' X86.s > X86-aout.S
	$(CC) -c X86-aout.S
	@rm X86-aout.S

X86-elf.o:
	sed -e '/ _/s// /' -e '/^_/s///' -e 's/ALIGN/16/' X86.s > X86-elf.S
	$(CC) -c X86-elf.S
	@rm X86-elf.S
