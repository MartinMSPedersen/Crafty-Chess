# To build crafty:
#
# Step 1: Uncomment the sections relevant to your architecture.
#	  You may need to tune the two gcc lines below to match your compiler.
#	  You want to set up for maximum optimization, but typically you will
#	  need to experiment to see which options provide the fastest code.
#	  This is optimized for gcc 2.6.3, which is a fairly current compiler,
#	  and for the SUN C compiler I use.
#   
#   The currently available targets:
#
#     ALPHA    {DEC Alpha running OSF/1-Digital Unix}
#     CRAY1    {any Cray-1 compatible architecture including XMP, YMP, 
#               C90, etc.}
#     HP       {HP workstation running HP_UX operating system (unix)}
#     LINUX    {80X86 architecture running LINUX (unix)}
#     NT_i386  {80X86 architecture running Windows 95 or NT}
#     NT_AXP   {DEC Alpha running Windows NT}
#     DOS      {PC running dos/windows, using DJGPP port of gcc to compile}
#     NEXT     {NextStep}
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
#   2.  opt = -DCOMPACT_ATTACKS -DUSE_SPLIT_SHIFTS
#   3.  opt = -DCOMPACT_ATTACKS -DUSE_SPLIT_SHIFTS -DUSE_ATTACK_FUNCTIONS
#   
#   If you want support for Steven Edward's endgame tablebases, then add
#   -DTABLEBASES to opt below.  you will also need to obtain the endgame
#   tablebase files from chess.onenet.net, pub/chess/TB. 
#   

# ALPHA
#target  = ALPHA
#CC      = gcc
#CFLAGS  = -O4
#LDFLAGS =
#opt     = -DCOMPACT_ATTACKS -DTABLEBASES

# DOS
# target  = DOS
# CC      = gcc
# CFLAGS  = -fomit-frame-pointer -m486 -O3
# LDFLAGS =
# opt     = -DCOMPACT_ATTACKS -DUSE_SPLIT_SHIFTS -DUSE_ATTACK_FUNCTIONS \
#   -DTABLEBASES -DUSE_ASSEMBLY_A -DUSE_ASSEMBLY_B
# asm     = X86.o

# FreeBSD (gcc 2.6.3)
#target  = FreeBSD
#CC      = gcc
#CFLAGS  = -fomit-frame-pointer -m486 -O3 -Wall
#LDFLAGS = 
#opt     = -DCOMPACT_ATTACKS -DUSE_SPLIT_SHIFTS -DUSE_ATTACK_FUNCTIONS \
#          -DTABLEBASES -DUSE_ASSEMBLY_A -DUSE_ASSEMBLY_B -DFAST

# HP
#target  = HP
#CC      = cc
#CFLAGS  = +O3 +Onolimit -Ae +w1
#LDFLAGS = $(CFLAGS)
#opt     = -DTABLEBASES

# LINUX
# Note: You have to uncomment exactly ONE of the `asm' lines below.
target  = LINUX
CC      = gcc
CFLAGS  = -fomit-frame-pointer -mpentium -O3 -Wall -fswap-for-agi -frisc-const -fschedule-stack-reg-insns
LDFLAGS = 
opt     = -DCOMPACT_ATTACKS -DUSE_SPLIT_SHIFTS -DUSE_ATTACK_FUNCTIONS \
          -DTABLEBASES -DUSE_ASSEMBLY_A -DUSE_ASSEMBLY_B -DFAST

# Uncomment the FIRST `asm' line for a.out systems.
# Uncomment the SECOND `asm' line for ELF systems.
#
asm     = X86-aout.o
#asm     = X86-elf.o

# NEXT
#target  = NEXT
#CC      = /bin/cc
#CFLAGS  = -O2
#LDFLAGS = $(CFLAGS)
#opt     = -DCOMPACT_ATTACKS

# SUN
#target  = SUN
#AS      = /usr/ccs/bin/as
#CC      = cc
#AFLAGS  = -P
#CFLAGS  = -fast -xO3 -dalign -xcg89
#LDFLAGS = 
#opt     = -DCOMPACT_ATTACKS -DUSE_SPLIT_SHIFTS -DUSE_ATTACK_FUNCTIONS \
#          -DTABLEBASES -DUSE_ASSEMBLY_A
#asm     = Sparc.o

# Step 2:
#
#   Set bookdir to point to the directory where you want to keep book.bin and
#   books.bin, use . to make Crafty look in the current directory.
#   
#   Set logdir to point to the directory where you want Crafty to put the
#   log.n and game.n files.  Note that saying "crafty c" will continue the
#   last game from the point where it left off if these files are present.
#
#   Set tbdir to point to the directory where the tablebases are kept.
#   Alternatively you can create a symbolic link TB in the directory you
#   execute crafty in, that points to the directory where the tablebase files
#   are kept. You will then need to delete the '-DTBDIR=\"$(tbdir)\"' part 
#   of the opts line below the 'Do not change anything below this line!'
#   comment.

bookdir  = .
logdir   = .
tbdir    = ./TB

# Do not change anything below this line!

opts = $(opt) -D$(target) -DLOGDIR=\"$(logdir)\" \
       -DBOOKDIR=\"$(bookdir)\" -DTBDIR=\"$(tbdir)\"

objects = main.o annotate.o attacks.o book.o boolean.o data.o draw.o drawn.o \
          edit.o enprise.o epd.o epdglue.o evaluate.o history.o init.o input.o \
          interupt.o iterate.o lookup.o make.o movgen.o next.o nexte.o \
          option.o output.o phase.o ponder.o preeval.o quiesce.o repeat.o \
          resign.o root.o search.o searchr.o setboard.o store.o swap.o test.o \
          time.o unmake.o utility.o valid.o validate.o $(asm)

includes = data.h function.h types.h

epdincludes = epd.h epddefs.h epdglue.h

eval_users = data.o evaluate.o preeval.o 

crafty:	$(objects) Makefile
	$(CC) $(LDFLAGS) -o crafty $(objects) -lm
	@rm -f X86-elf.S
	@rm -f X86-aout.S

$(objects): $(includes)

$(eval_users): evaluate.h

epd.o epdglue.o option.o init.o : $(epdincludes)

.c.o:
	$(CC) $(CFLAGS) $(opts) -c $*.c

.s.o:
	$(AS) $(AFLAGS) -o $*.o $*.s

X86-aout.o:
	cp X86.s X86-aout.S
	$(CC) -c X86-aout.S

X86-elf.o:
	sed -e '/ _/s// /' -e '/^_/s///' X86.s > X86-elf.S
	$(CC) -c X86-elf.S
