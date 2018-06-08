#    configuration information:  at the bottom of this Makefile, you will
#      need to alter the "cc" lines to reflect the optimal settings for your
#      specific C compiler.  this makefile is configured for a sun sparcstation
#      running Solaris 2.4.  For maximum speed, you should cat all source into
#      one large .c file and then compile this with maximum optimization including
#      inlining.  Play with this file, and optimization settings to get the very
#      fastest executable.  This Makefile is used for development, so that I don't
#      have to re-compile everything after changing one module.

objects = main.o attacks.o book.o bookup.o boolean.o data.o drawn.o draw.o edit.o  \
    enprise.o evaluate.o give.o history.o init.o input.o interupt.o iterate.o      \
    lookup.o make.o movgen.o next.o nextc.o nexte.o option.o output.o phase.o      \
    ponder.o preeval.o quiesce.o repeat.o resign.o root.o search.o setboard.o      \
    store.o swap.o test.o time.o utility.o valid.o validate.o

includes = data.h function.h types.h

eval_users = data.o evaluate.o exchange.o init.o main.o make.o preeval.o resign.o  \
   validate.o

crafty:	$(objects) $(includes) makefile
	@echo 'cc -ocrafty $(objects)'
	@cc -xO4 -dalign -xcg89 -ocrafty $(objects) -lm

$(objects): $(includes)

$(eval_users): evaluate.h

.c.o:
	cc -xO4 -dalign -xcg89 -c $*.c
