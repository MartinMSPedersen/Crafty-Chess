#   
#   tune the two gcc lines below to match your compiler.  you want to set up for
#   maximum optimization, but typically you will need to experiment to see which
#   options provide the fastest code.  this is optimized for gcc 2.6.3, which is
#   a fairly current compiler if you use the gcc lines below, or for the SUN C
#   compiler I use.
#   
#   set target to one of the following:
#
#     ALPHA    {DEC alpha running ultrix}
#     CRAY1    {any Cray-1 compatible architecture including XMP, YMP, C90, etc.}
#     HP       {HP workstation running HP_UX operating system (unix)}
#     LINUX    {80X86 architecture running LINUX (unix)}
#     PC       {PC running dos/windows, using DJGPP port of gcc to compile}
#     SUN      {Sun SparcStation running Solaris (SYSV/R4) Unix}
#     SUN_BSD  {Sun SparcStation running SunOX (BSD) Unix}
#   
#   set bookdir to point to the directory where you want to keep book.bin and
#   books.bin, otherwise "./" will make Crafty look in the current directory.
#   
#   set logdir to point to the directory where you want Crafty to put the
#   log.n and game.n files.  note that saying "crafty c" will continue the
#   last game from the point where it left off if these files are present.
#   
#   the next two defines are optimizations inside Crafty that you will have
#   test to see if either or both help.  on some machines, these will slow
#   things down by up to 10%, while on other machines these options will
#   result in improving search speed up to 20%.
#   
#   1.  opt = -DCOMPACT_ATTACKS
#   2.  opt = -DCOMPACT_ATTACKS -DUSE_SPLIT_SHIFTS
#   
target  =  SUN
bookdir =  .
logdir  =  .
opts =

objects  = main.o attacks.o book.o boolean.o data.o draw.o drawn.o edit.o     \
    enprise.o epd.o epdglue.o evaluate.o give.o history.o init.o input.o \
    interupt.o iterate.o lookup.o make.o movgen.o next.o nextc.o nexte.o \
    option.o output.o phase.o ponder.o preeval.o quiesce.o repeat.o \
    resign.o root.o search.o setboard.o store.o swap.o test.o time.o \
    utility.o valid.o validate.o 

includes = data.h function.h types.h
epdincludes = epd.h epddefs.h epdglue.h

eval_users = data.o evaluate.o init.o main.o make.o preeval.o resign.o validate.o


crafty:	$(objects) Makefile
#	gcc -fomit-frame-pointer -m486 -O2 -pipe -o crafty $(objects) -lm
	cc -xO4 -dalign -xcg89 -ocrafty $(objects) -lm

$(objects): $(includes)

$(eval_users): evaluate.h

epd.o epdglue.o option.o init.o : $(epdincludes)

%.o : %.c
#	gcc -D$(target) -DLOGDIR=\"$(logdir)\" -DBOOKDIR=\"$(bookdir)\" -fomit-frame-pointer -m486 -O2 -pipe $(opt) -c $*.c
	cc -D$(target) -DLOGDIR=\"$(logdir)\" -DBOOKDIR=\"$(bookdir)\" -fast -xO3 -dalign -xcg89 -c $*.c
