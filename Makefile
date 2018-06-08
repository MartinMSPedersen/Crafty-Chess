
# To build crafty:
#
#         Uncomment the sections relevant to your architecture.
#	  You may need to tune the two gcc lines below to match your compiler.
#	  You want to set up for maximum optimization, but typically you will
#	  need to experiment to see which options provide the fastest code.
#	  This is optimized for pgcc, which is a fairly current compiler.
#   
#   The currently available targets:
#
#     AIX      {IBM machines running AIX}
#     ALPHA    {DEC Alpha running OSF/1-Digital Unix}
#     CRAY1    {any Cray-1 compatible architecture including XMP, YMP, 
#               C90, etc.}
#     HP       {HP workstation running HP_UX operating system (unix)}
#     LINUX    {80X86 architecture running LINUX (unix)}
#     NT_i386  {80X86 architecture running Windows 95 or NT}
#     NT_AXP   {DEC Alpha running Windows NT}
#     DOS      {PC running dos/windows, using DJGPP port of gcc to compile}
#     NEXT     {NextStep}
#     OS/2     {IBM OS/2 warp}
#     SGI      {SGI Workstation running Irix (SYSV/R4) Unix}
#     SUN      {Sun SparcStation running Solaris (SYSV/R4) Unix}
#     SUN_BSD  {Sun SparcStation running SunOS (BSD) Unix}
#     FreeBSD  {80X86 architecture running FreeBSD (unix)}
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
#   
#   
# AIX
#target  = AIX
#CC      = cc
#CFLAGS  = -O2
#CPP	 = $(CC)
#LDFLAGS =
#opt     = -DCOMPACT_ATTACKS -DUSE_ATTACK_FUNCTIONS
#opt     = -DCOMPACT_ATTACKS

# ALPHA
#target  = ALPHA
#CC      = cc
#Note: "-arch host" assumes you will run the binary on exactly the
# same kind of ALPHA you compiled it on.  Omit it if you want to run
# the same binary on several kinds of Alpha.  If you are on an early
# EV6 that does not have the CIX instruction set extension, a compiler
# bug (?) causes these instructions to be generated anyway.  If this
# happens you'll see a message about "instr emulated" after starting
# crafty; to fix it, change "-arch host" to "-arch ev56 -tune host"
# and recompile.
#CFLAGS  = -std -fast -O4 -pthread -newc -arch host
#CPP	 = cxx
#LDFLAGS = $(CFLAGS)
#LIBS    = -lpthread -lexc
#opt     = -DSMP -DCPUS=8 -DFAST -DPOSIX


# DOS
# target  = DOS
# CC      = gcc
# CFLAGS  = -fomit-frame-pointer -m486 -O3
# CPP	  = $(CC)
# LDFLAGS =
# opt     = -DCOMPACT_ATTACKS -DUSE_ATTACK_FUNCTIONS \
#           -DUSE_ASSEMBLY_A -DUSE_ASSEMBLY_B
# asm     = X86.o

# FreeBSD (gcc 2.6.3)
#target  = FreeBSD
#CC      = gcc
#CFLAGS  = -fomit-frame-pointer -m486 -O3 -Wall
#CPP	 = $(CC)
#LDFLAGS = 
#opt     = -DCOMPACT_ATTACKS -DUSE_ATTACK_FUNCTIONS \
#          -DUSE_ASSEMBLY_A -DUSE_ASSEMBLY_B -DFAST

# FreeBSD (pgcc)
#target  = FreeBSD
#CC      = gcc
#CFLAGS  = -pipe -D_REENTRANT -mpentium -O -Wall
#CPP	 = $(CC)
#LDFLAGS = 
#opt     = -DCOMPACT_ATTACKS -DUSE_ATTACK_FUNCTIONS \
#          -DUSE_ASSEMBLY_A -DUSE_ASSEMBLY_B -DFAST

# HP
#target  = HP
#CC      = cc
#OPT     = +O3 +Onolimit
#CFLAGS  = +ESlit -Ae +w1
#CPP	 = $(CC)
#LDFLAGS = $(OPT) $(CFLAGS)
#opt     = 
 
# LINUX (gcc 2.95)
# Note: You have to uncomment exactly ONE of the `asm' lines below.
target  = LINUX
CC      = gcc
CPP	= g++
CFLAGS  = -Wall -pipe -D_REENTRANT -march=i686 -O -fforce-mem \
          -fomit-frame-pointer -mpreferred-stack-boundary=2
LDFLAGS = -lpthread
opt     = -DCOMPACT_ATTACKS -DUSE_ATTACK_FUNCTIONS \
          -DUSE_ASSEMBLY_A -DUSE_ASSEMBLY_B -DFAST -DSMP -DCPUS=4 -DDGT

# Uncomment the FIRST `asm' line for a.out systems.
# Uncomment the SECOND `asm' line for ELF systems.
#
#asm     = X86-aout.o
asm     = X86-elf.o
 
# NEXT
#target  = NEXT
#CC      = /bin/cc
#CFLAGS  = -O2
#CPP	 = $(CC)
#LDFLAGS = $(CFLAGS)
#opt     = -DCOMPACT_ATTACKS

# OS2 (emx09c)
#  target  = OS2
#  CC      = gcc
#  CFLAGS  = -fomit-frame-pointer -m486 -O3 -Wall
#  CPP     = $(CC)
#  LDFLAGS = -Zexe -Zcrtdll -s
#  opt = -DCOMPACT_ATTACKS -DUSE_ATTACK_FUNCTIONS \
#        -DUSE_ASSEMBLY_A -DUSE_ASSEMBLY_B -DFAST -DOS2
#  asm     = X86.o

# SGI
#target  = SGI
#AS      = /bin/as
#CC      = cc
#AFLAGS  = -P
#CFLAGS  = -g -32 -mips2 -cckr
#CPP	 = $(CC)
#LDFLAGS = 
#opt     = -DCOMPACT_ATTACKS -DUSE_ATTACK_FUNCTIONS
#opt     = 

# SUN
#target  = SUN
#AS      = /usr/ccs/bin/as
#CC      = cc
#AFLAGS  = -P
#CPP	 = $(CC)
#CFLAGS  = -fast -xO5 -xunroll=20
#LDFLAGS = -lpthread
#opt     = -DCOMPACT_ATTACKS -DUSE_ATTACK_FUNCTIONS \
#          -DUSE_ASSEMBLY_A -DSMP -DCPUS=4 -DMUTEX -DPOSIX
#asm     = Sparc.o

# Do not change anything below this line!

opts = $(opt) -D$(target)

objects = searchr.o search.o thread.o searchmp.o repeat.o next.o nexte.o      \
       nextr.o history.o quiesce.o evaluate.o movgen.o make.o unmake.o hash.o \
       attacks.o swap.o boolean.o utility.o valid.o probe.o book.o data.o     \
       drawn.o edit.o enprise.o epd.o epdglue.o init.o input.o interupt.o     \
       iterate.o main.o option.o output.o phase.o ponder.o preeval.o resign.o \
       root.o learn.o setboard.o test.o time.o validate.o annotate.o          \
       analyze.o evtest.o bench.o egtb.o dgt.o $(asm)

includes = data.h chess.h

epdincludes = epd.h epddefs.h epdglue.h

eval_users = data.o evaluate.o preeval.o 

crafty:	$(objects) 
	$(CC) $(LDFLAGS) -o crafty $(objects) -lm  $(LIBS)
	@rm -f X86-elf.S
	@rm -f X86-aout.S

dgt:    dgtdrv.o
	@cc -O -o dgt dgtdrv.c

egtb.o: egtb.cpp
	$(CPP) -c $(CFLAGS) $(opts) egtb.cpp
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
