#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include "chess.h"
#include "data.h"
#if defined(UNIX) || defined(AMIGA)
#  include <unistd.h>
#endif
#include "epdglue.h"

/* last modified 01/14/99 */
/*
********************************************************************************
*                                                                              *
*   Option() is used to handle user input necessary to control/customize the   *
*   program.  it performs all functions excepting chess move input which is    *
*   handled by main().                                                         *
*                                                                              *
********************************************************************************
*/
int Option(TREE *tree) {
  static int egtbsetup=0;
/*
 ----------------------------------------------------------
|                                                          |
|   EPD implementation interface code.  EPD commands can   |
|   not be handled if the program is actually searching in |
|   a real game, and if Crafty is "pondering" this has to  |
|   be stopped.                                            |
|                                                          |
 ----------------------------------------------------------
*/
  nargs=ReadParse(buffer,args," 	;=/");
  if (!nargs) return(0);
  if (args[0][0] == '#') return(1);
  if (initialized) {
    if (EGCommandCheck(buffer)) {
      if (thinking || pondering) return (2);
      else {
        (void) EGCommand(buffer);
        return (1);
      }
    }
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "!" character is a 'shell escape' that passes the rest |
|   of the command to a shell for execution.               |
|                                                          |
 ----------------------------------------------------------
*/
  if (strchr(buffer,'!')) {
    system(strchr(buffer,'!')+1);
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "." ignores "." if it happens to get to this point, if |
|   xboard is running.                                     |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch(".",*args)) {
    if (xboard) {
      printf("stat01: 0 0 0 0 0\n");
      fflush(stdout);
      return(1);
    }
    else return(0);
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "alarm" command turns audible move warning on/off.     |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("alarm",*args)) {
    RestoreGame();
    if (!strcmp(args[1],"on")) audible_alarm=0x07;
    else if (!strcmp(args[1],"off")) audible_alarm=0x00;
    else printf("usage:  alarm on|off\n");
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "analyze" puts crafty in analyze mode, where it reads  |
|   moves in and between moves, computes as though it is   |
|   trying to find the best move to make.  when another    |
|   move is entered, it switches sides and continues.  it  |
|   will never make a move on its own, rather, it will     |
|   continue to analyze until an "exit" command is given.  |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("analyze",*args)) {
    if (thinking || pondering) return(2);
    Analyze();
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "annotate" command is used to read a series of moves   |
|   and analyze the resulting game, producing comments as  |
|   requested by the user.                                 |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("annotateh",*args)) {
    if (thinking || pondering) return(2);
    Annotate();
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "ansi" command turns video highlight on/off.           |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("ansi",*args)) {
    if (nargs < 2) printf("usage:  ansi on|off\n");
    if (!strcmp(args[1],"on")) ansi=1;
    else if (!strcmp(args[1],"off")) ansi=0;
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "auto232" enables auto232 mode.  note that there is    |
|   another alias "DR".                                    |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("auto232",*args) || OptionMatch("DR",*args)) {
    if (auto_file) {
      fclose(auto_file);
      auto_file=0;
      auto232=0;
      printf("auto232 disabled\n");
    }
    else {
      auto_file=fopen("PRN", "w");
      auto232=1;
      printf("auto232 enabled\n");
      book_selection_width=3;
      mode=tournament_mode;
    }
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "batch" command disables asynchronous I/O so that a    |
|   stream of commands can be put into a file and they are |
|   not executed instantly.                                |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("batch",*args)) {
    if (!strcmp(args[1],"on")) batch_mode=1;
    else if (!strcmp(args[1],"off")) batch_mode=0;
    else printf("usage:  batch on|off\n");
  }
/*
 ----------------------------------------------------------
|                                                          |
|  "beep" command is ignored. [xboard compatibility]       |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("beep",*args)) {
    return(xboard);
  }
/*
 ----------------------------------------------------------
|                                                          |
|  "bench" runs internal performance benchmark             |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("bench",*args)) {
    Bench();
  }
/*
 ----------------------------------------------------------
|                                                          |
|  "bk"  book command from xboard sends the suggested book |
|  moves.                                                  |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("bk",*args)) {
    printf("\t%s\n\n",book_hint);
    fflush(stdout);
    return(xboard);
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "black" command sets black to move (ChangeSide(wtm)).             |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("black",*args)) {
    if (strlen(*args) == 1) return(1);
    if (thinking || pondering) return (2);
    ponder_move=0;
    last_pv.pathd=0;
    last_pv.pathl=0;
    if (wtm) Pass();
    force=0;
  }
/*
 ----------------------------------------------------------
|                                                          |
|  "bogus" command is ignored. [xboard compatibility]      |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("bogus",*args)) {
    return(xboard);
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "bookw" command updates the book selection weights.    |
|                                                          |
 ----------------------------------------------------------
*/
  else if (!strcmp("bookw",*args)) {
    if (nargs > 1) {
      if (!strcmp("freq",args[1]))
        book_weight_freq=atof(args[2]);
      else if (!strcmp("eval",args[1]))
        book_weight_eval=atof(args[2]);
      else if (!strcmp("learn",args[1]))
        book_weight_learn=atof(args[2]);
    }
    else {
      Print(128,"frequency (freq)..............%4.2f\n",book_weight_freq);
      Print(128,"static evaluation (eval)......%4.2f\n",book_weight_eval);
      Print(128,"learning (learn)..............%4.2f\n",book_weight_learn);
    }
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "book" command updates/creates the opening book file.  |
|                                                          |
 ----------------------------------------------------------
*/
  else if (!strcmp("book",*args)) {
    nargs=ReadParse(buffer,args," 	;");
    BookUp(tree,"book.bin", nargs-1,args+1);
  }
  else if (!strcmp("books",*args)) {
    nargs=ReadParse(buffer,args," 	;");
    BookUp(tree,"books.bin", nargs-1,args+1);
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "channel" command behaves just like the whisper        |
|   command, but sends the output to "channel n" instead.  |
|   there is an optional second parameter that will be     |
|   added to the channel tell to indicate what the tell is |
|   connected to, such as when multiple GM games are going |
|   on, so that the comment can be directed to a game.     |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("channel",*args)) {
    int tchannel;

    nargs=ReadParse(buffer,args," 	;");
    if (nargs < 2) {
      printf("usage:  channel <n> [title]\n");
      return(1);
    }
    tchannel=atoi(args[1]);
    if (tchannel) channel=tchannel;
    if (nargs > 1) {
      char *from=args[2];
      char *to=channel_title;
      while (*from) {
        if (*from != '*') *to++=*from;
        from++;
      }
      *to=0;
    }
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "cache" is used to set the EGTB cache size.  as always |
|   bigger is better.  the default is 1mb.  sizes can be   |
|   specified in bytes, Kbytes or Mbytes as with the hash  |
|   commands.                                              |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("cache",*args)) {
    EGTB_cache_size=atoi(args[1]);
    if (strchr(args[1],'K') || strchr(args[1],'k')) EGTB_cache_size*=1<<10;
    if (strchr(args[1],'M') || strchr(args[1],'m')) EGTB_cache_size*=1<<20;
    if (EGTB_cache) free(EGTB_cache);
    EGTB_cache=malloc(EGTB_cache_size);
    if (!EGTB_cache) {
      Print(2095,"ERROR:  unable to malloc specified cache size, using default\n");
      EGTB_cache=malloc(EGTB_CACHE_DEFAULT);
    }
    if (EGTB_cache_size < 1<<20)
      Print(4095,"EGTB cache memory = %dK bytes.\n", EGTB_cache_size/(1<<10));
    else
      Print(4095,"EGTB cache memory = %dM bytes.\n", EGTB_cache_size/(1<<20));
    FTbSetCacheSize(EGTB_cache,EGTB_cache_size);
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "clock" command displays chess clock.                  |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("clock",*args)) {
    if (nargs > 1)
      tc_time_remaining=ParseTime(args[1])*6000;
      if (tc_time_remaining <= tc_operator_time) {
        Print(4095,"ERROR:  remaining time less than operator time\n");
        Print(4095,"ERROR:  resetting operator time to 0:00.\n");
        Print(4095,"ERROR:  use \"operator n\" command to correct after time control\n");
        tc_operator_time=0;
      }

    if (nargs > 2)
      tc_time_remaining_opponent=ParseTime(args[2])*6000;
    Print(4095,"time remaining %s (Crafty)",
          DisplayHHMM(tc_time_remaining));
    Print(4095,"  %s (opponent).\n",
          DisplayHHMM(tc_time_remaining_opponent));
    Print(4095,"%d moves to next time control (Crafty)\n",
          tc_moves_remaining);
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "computer" lets crafty know it is playing a computer.  |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("computer",*args)) {
    Print(128,"playing a computer!\n");
    computer_opponent=1;
    book_selection_width=2;
    usage_level=0;
    book_weight_freq=1.0;
    book_weight_eval=.1;
    book_weight_learn=.2;
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "dgt" command activates the DGT board interface.       |
|                                                          |
 ----------------------------------------------------------
*/
#if defined(DGT)
  else if (!strcmp("dgt",*args)) {
    nargs=ReadParse(buffer,args," 	;");
    if (to_dgt == 0) DGTInit(nargs,args);
    else {
      write(to_dgt,args[1],strlen(args[1]));
    }
  }
#endif
/*
 ----------------------------------------------------------
|                                                          |
|   "display" command displays the chess board.            |
|                                                          |
|   "display" command sets specific display options which  |
|   control how "chatty" the program is.  in the variable  |
|   display_options, the following bits are set/cleared    |
|   based on the option chosen.                            |
|                                                          |
|     1 -> display time for moves.                         |
|     2 -> display variation when it changes.              |
|     4 -> display variation at end of iteration.          |
|     8 -> display basic search statistics.                |
|    16 -> display extended search statistics.             |
|    32 -> display root moves as they are searched.        |
|    64 -> display move numbers in the PV output.          |
|   128 -> display general informational messages.         |
|   256 -> display ply-1 move node counts after each       |
|          iteration.                                      |
|   512 -> display ply-1 moves and positional evaluations  |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("display",*args)) {
    if (nargs > 1) do {
      if (OptionMatch("time",args[1])) {
        display_options|=1;
        Print(128,"display time for moves played in game.\n");
      }
      else if (OptionMatch("notime",args[1])) {
        display_options&=4095-1;
        Print(128,"don't display time for moves played in game.\n");
      }
      else if (OptionMatch("changes",args[1])) {
        display_options|=2;
        Print(128,"display PV each time it changes.\n");
      }
      else if (OptionMatch("nochanges",args[1])) {
        display_options&=4095-2;
        Print(128,"don't display PV each time it changes.\n");
      }
      else if (OptionMatch("variation",args[1])) {
        display_options|=4;
        Print(128,"display PV at end of each iteration.\n");
      }
      else if (OptionMatch("novariation",args[1])) {
        display_options&=4095-4;
        Print(128,"don't display PV at end of each iteration.\n");
      }
      else if (OptionMatch("stats",args[1])) {
        display_options|=8;
        Print(128,"display statistics at end of each search.\n");
      }
      else if (OptionMatch("nostats",args[1])) {
        display_options&=4095-8;
        Print(128,"don't.display statistics at end of each search.\n");
      }
      else if (OptionMatch("extstats",args[1])) {
        display_options|=16;
        Print(128,"display extended statistics at end of each search.\n");
      }
      else if (OptionMatch("noextstats",args[1])) {
        display_options&=4095-16;
        Print(128,"don't display extended statistics at end of each search.\n");
      }
      else if (OptionMatch("movenum",args[1])) {
        display_options|=64;
        Print(128,"display move numbers in variations.\n");
      }
      else if (OptionMatch("nomovenum",args[1])) {
        display_options&=4095-64;
        Print(128,"don't display move numbers in variations.\n");
      }
      else if (OptionMatch("moves",args[1])) {
        display_options|=32;
        Print(128,"display ply-1 moves as they are searched.\n");
      }
      else if (OptionMatch("nomoves",args[1])) {
        display_options&=4095-32;
        Print(128,"don't display ply-1 moves as they are searched.\n");
      }
      else if (OptionMatch("general",args[1])) {
        display_options|=128;
        Print(128,"display informational messages.\n");
      }
      else if (OptionMatch("nogeneral",args[1])) {
        display_options&=4095-128;
        Print(128,"don't display informational messages.\n");
      }
      else if (OptionMatch("nodes",args[1])) {
        display_options|=256;
        Print(128,"display ply-1 node counts after each iteration.\n");
      }
      else if (OptionMatch("nonodes",args[1])) {
        display_options&=4095-256;
        Print(128,"don't display ply-1 node counts after each iteration.\n");
      }
      else if (OptionMatch("ply1",args[1])) {
        display_options|=512;
        Print(128,"display ply-1 moves/evaluations.\n");
      }
      else if (OptionMatch("noply1",args[1])) {
        display_options&=4095-512;
        Print(128,"don't display ply-1 moves/evaluations.\n");
      }
      else if (OptionMatch("*",args[1])) {
        if (display_options&1)
          printf("display time for moves\n");
        if (display_options&2)
          printf("display variation when it changes.\n");
        if (display_options&4)
          printf("display variation at end of iteration.\n");
        if (display_options&8)
          printf("display basic search stats.\n");
        if (display_options&16)
          printf("display extended search stats.\n");
        if (display_options&32)
          printf("display ply-1 moves as they are searched.\n");
        if (display_options&64)
          printf("display move numbers in variations.\n");
        if (display_options&128)
          printf("display general messages.\n");
        if (display_options&256)
          printf("display ply-1 node counts every iteration.\n");
        if (display_options&512)
          printf("display ply-1 moves and evaluations.\n");
      }
      else break;
      return(1);
    } while(0);
    if (nargs > 1) {
      if (thinking || pondering) return (2);
      tree->position[1]=tree->position[0];
      PreEvaluate(tree,wtm);
      if (OptionMatch("pawn",args[1]))
        DisplayPieceBoards(pval_w,pval_b);
      if (OptionMatch("knight",args[1]))
        DisplayPieceBoards(nval_w,nval_b);
      if (OptionMatch("bishop",args[1]))
        DisplayPieceBoards(bval_w,bval_b);
      if (OptionMatch("rook",args[1]))
        DisplayPieceBoards(rval_w,rval_b);
      if (OptionMatch("queen",args[1]))
        DisplayPieceBoards(qval_w,qval_b);
      if (OptionMatch("king",args[1]))
        DisplayPieceBoards(kval_w,kval_b);
    }
    else DisplayChessBoard(stdout,display);
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "delay" command sets a specific delay (in ms) for      |
|   auto232 synchronization.                               |
|                                                          |
 ----------------------------------------------------------
*/
  else if (!strcmp("delay",*args)) {
    if (nargs < 2) {
      printf("usage:  delay <n>\n");
      return(1);
    }
    auto232_delay=atoi(args[1]);
    Print(4095,"auto232 delay value set to %d ms.\n",auto232_delay);
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "depth" command sets a specific search depth to        |
|   control the tree search depth. [xboard compatibility]. |
|                                                          |
 ----------------------------------------------------------
*/
  else if (!strcmp("depth",*args)) {
    if (nargs < 2) {
      printf("usage:  depth <n>\n");
      return(1);
    }
    search_depth=atoi(args[1]);
    Print(4095,"search depth set to %d.\n",search_depth);
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "draw" is used to offer Crafty a draw.                 |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("draw",*args)) {
    if (nargs == 1) {
      int drawsc=DrawScore(1);
      if (move_number<40 || !accept_draws) drawsc=-300;
      if (last_search_value<=drawsc && (tc_increment!=0 ||
          tc_time_remaining_opponent>=1000)) {
        if (xboard) Print(4095,"tellics draw\n");
        else Print(4095,"Draw accepted.\n");
        Print(4095,"1/2-1/2 {Draw agreed}\n");
        strcpy(pgn_result,"1/2-1/2");
      }
      else {
        if (xboard) {
          Print(4095,"tellics decline\n");
          Print(4095,"Decline\n");
        }
        else Print(4095,"Draw declined.\n");
      }
    }
    else {
      if (!strcmp(args[1],"accept")) {
        accept_draws=1;
        Print(128,"accept draw offers\n");
      }
      else if (!strcmp(args[1],"decline")) {
        accept_draws=0;
        Print(128,"decline draw offers\n");
      }
      else Print(128,"usage: draw accept|decline\n");
    }
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "drawscore" sets the default draw score (which is      |
|    forced to zero when the endgame is reached.)          |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("drawscore",*args)) {
    if (nargs > 2) {
      printf("usage:  drawscore <n>\n");
      return(1);
    }
    if (nargs == 2) draw_score=atoi(args[1]);
    printf("draw score set to %7.2f pawns.\n",
           ((float) draw_score) / 100.0);
  }
/*
 ----------------------------------------------------------
|                                                          |
|  "easy" command disables thinking on opponent's time.    |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("easy",*args)) {
    ponder=0;
    Print(4095,"pondering disabled.\n");
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "echo" command displays messages from command file.    |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("echo",*args) || OptionMatch("title",*args)) {
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "edit" command modifies the board position.            |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("edit",*args) && strcmp(*args,"ed")) {
    if (thinking || pondering) return (2);
    Edit();
    move_number=1; /* discard history */
    if (!wtm) {
      wtm=1;
      Pass();
    }
    ponder_move=0;
    last_pv.pathd=0;
    last_pv.pathl=0;
    strcpy(buffer,"savepos *");
    (void) Option(tree);
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "egtb" command enables/disables tablebases and sets    |
|   the number of pieces available for probing.            |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("egtb",*args)) {
    if (!egtbsetup) {
      Print(128,"EGTB access enabled\n");
      Print(128,"using tbpath=%s\n",tb_path);
      EGTBlimit=IInitializeTb(tb_path);
      Print(128,"%d piece tablebase files found\n",EGTBlimit);
      if (0 != cbEGTBCompBytes)
        Print(128,"%dkb of RAM used for TB indices and decompression tables\n",
              (cbEGTBCompBytes+1023)/1024);
      if (EGTBlimit) {
        if (!EGTB_cache) EGTB_cache=malloc(EGTB_cache_size);
        if (!EGTB_cache) {
          Print(4095,"ERROR  EGTB cache malloc failed\n");
          EGTB_cache=malloc(EGTB_CACHE_DEFAULT);
        }
        else FTbSetCacheSize(EGTB_cache,EGTB_cache_size);
        egtbsetup=1;
      }
    }
    else {
      if (nargs==2) EGTBlimit=Min(atoi(args[1]),5);
    }
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "end" (or "quit") command terminates the program.      |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("end",*args) || OptionMatch("quit",*args)) {
    if (moves_out_of_book)
      LearnBook(tree,crafty_is_white,last_search_value,0,0,1);
    Print(4095,"execution complete.\n");
    fflush(stdout);
    if (book_file) fclose(book_file);
    if (books_file) fclose(books_file);
    if (book_lrn_file) fclose(book_lrn_file);
    if (position_file) fclose(position_file);
    if (position_lrn_file) fclose(position_lrn_file);
    if (history_file) fclose(history_file);
    if (log_file) fclose(log_file);
    EGTerm();
#if defined(DGT)
    if (DGT_active) write(to_dgt,"exit\n",5);
#endif
    exit(0);
  }
/*
 ----------------------------------------------------------
|                                                          |
|  "eot" command is a no-operation that is used to keep    |
|  Crafty and the ICS interface in sync.                   |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("eot",*args)) {
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "evaluation" command is used to adjust the eval terms  |
|   to modify the way Crafty behaves.                      |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("evaluation",*args)) {
    int i;
    if (nargs < 3) {
      printf("see 'help evaluation' for details on using this command\n");
      return(1);
    }
    if (OptionMatch("asymmetry",args[1])) {
      king_safety_asymmetry=atoi(args[2]);
    }
    else if (OptionMatch("bscale",args[1])) {
      blocked_scale=atoi(args[2]);
    }
    else if (OptionMatch("kscale",args[1])) {
      king_safety_scale=atoi(args[2]);
    }
    else if (OptionMatch("ppscale",args[1])) {
      passed_scale=atoi(args[2]);
    }
    else if (OptionMatch("pscale",args[1])) {
      pawn_scale=atoi(args[2]);
    }
    else if (OptionMatch("tropism",args[1])) {
      king_safety_tropism=atoi(args[2]);
    }
    else printf("unknown option %s\n",args[1]);
    PreEvaluate(tree,!wtm);
    if (OptionMatch("kscale",args[1]) || OptionMatch("asymmetry",args[1])) {
      Print(128,"modified king-safety values:\n");
      Print(128,"white: ");
      for (i=0;i<16;i++)
        Print(128,"%3d ", temper_w[i]);
      Print(128,"\n       ");
      for (i=16;i<32;i++)
        Print(128,"%3d ", temper_w[i]);
      Print(128,"\n       ");
      for (i=33;i<48;i++)
        Print(128,"%3d ", temper_w[i]);
      Print(128,"\n       ");
      for (i=49;i<64;i++)
        Print(128,"%3d ", temper_w[i]);
      Print(128,"\n\nblack: ");
      for (i=0;i<16;i++)
        Print(128,"%3d ", temper_b[i]);
      Print(128,"\n       ");
      for (i=16;i<32;i++)
        Print(128,"%3d ", temper_b[i]);
      Print(128,"\n       ");
      for (i=32;i<48;i++)
        Print(128,"%3d ", temper_b[i]);
      Print(128,"\n");
      for (i=48;i<64;i++)
        Print(128,"%3d ", temper_b[i]);
      Print(128,"\n");
    }
    else if (OptionMatch("tropism",args[1])) {
      Print(128,"modified king-tropism values:\n");
      for (i=0;i<16;i++)
        Print(128,"%3d ", tropism[i]);
      Print(128,"\n");
      for (i=16;i<32;i++)
        Print(128,"%3d ", tropism[i]);
      Print(128,"\n");
    }
    else if (OptionMatch("bscale",args[1])) {
      Print(128,"modified blocked_pawn values:\n");
      for (i=0;i<9;i++)
        Print(128,"%3d ", pawn_rams[i]);
      Print(128,"\n");
    }
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "evtest" command runs a test suite of problems and     |
|   prints evaluations only.                               |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("evtest",*args)) {
    if (thinking || pondering) return(2);
    if (nargs < 2) {
      printf("usage:  evtest <filename> [exitcnt]\n");
      return(1);
    }
    EVTest(args[1]);
    ponder_move=0;
    last_pv.pathd=0;
    last_pv.pathl=0;
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "exit" command resets input device to STDIN.           |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("exit",*args)) {
    if (analyze_mode) return(0);
    if (input_stream != stdin) fclose(input_stream);
    input_stream=stdin;
    ReadClear();
    Print(4095,"\n");
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "extension" command allows setting the various search  |
|   extension depths to any reasonable value between 0 and |
|   1.0                                                    |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("extensions",*args)) {
    if (nargs < 3) {
      printf("usage:  ext/name=n\n");
      return(1);
    }
    if (OptionMatch("incheck",args[1])) {
      const float ext=atof(args[2]);
      incheck_depth=60.0*ext;
      if (incheck_depth < 0) incheck_depth=0;
      if (incheck_depth > 60) incheck_depth=60;
    }
    if (OptionMatch("onerep",args[1])) {
      const float ext=atof(args[2]);
      onerep_depth=60.0*ext;
      if (onerep_depth < 0) onerep_depth=0;
      if (onerep_depth > 60) onerep_depth=60;
    }
    if (OptionMatch("pushpp",args[1])) {
      const float ext=atof(args[2]);
      pushpp_depth=60.0*ext;
      if (pushpp_depth < 0) pushpp_depth=0;
      if (pushpp_depth > 60) pushpp_depth=60;
    }
    if (OptionMatch("recapture",args[1])) {
      const float ext=atof(args[2]);
      recap_depth=60.0*ext;
      if (recap_depth < 0) recap_depth=0;
      if (recap_depth > 60) recap_depth=60;
    }
    if (OptionMatch("singular",args[1])) {
      const float ext=atof(args[2]);
      singular_depth=60.0*ext;
      if (singular_depth < 0) singular_depth=0;
      if (singular_depth > 60) singular_depth=60;
    }
    if (OptionMatch("threat",args[1])) {
      const float ext=atof(args[2]);
      threat_depth=60.0*ext;
      if (threat_depth < 0) threat_depth=0;
      if (threat_depth > 60) threat_depth=60;
    }
    Print(1,"one-reply extension..................%4.2f\n",(float) onerep_depth/60.0);
    Print(1," in-check extension..................%4.2f\n",(float) incheck_depth/60.0);
    Print(1,"recapture extension..................%4.2f\n",(float) recap_depth/60.0);
    Print(1,"   pushpp extension..................%4.2f\n",(float) pushpp_depth/60.0);
    Print(1,"mate thrt extension..................%4.2f\n",(float) threat_depth/60.0);
    Print(1," singular extension..................%4.2f\n",(float) singular_depth/60.0);
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "flag" command controls whether Crafty will call the   |
|   flag in xboard/winboard games (to end the game.)       |
|                                                          |
 ----------------------------------------------------------
*/
  else if (!strcmp("flag",*args)) {
    if (nargs < 2) {
      printf("usage:  flag on|off\n");
      return(1);
    }
    if (!strcmp(args[1],"on")) call_flag=1;
    else if (!strcmp(args[1],"off")) call_flag=0;
    if (call_flag) Print(4095,"end game on time forfeits\n");
    else Print(4095,"ignore time forfeits\n");
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "flip" command flips the board, interchanging each     |
|   rank with the corresponding rank on the other half of  |
|   the board, and also reverses the color of all pieces.  |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("flip",*args)) {
    int file, rank, piece;
    if (thinking || pondering) return(2);
    for (rank=0;rank<4;rank++) {
      for (file=0;file<8;file++) {
        piece=-PieceOnSquare((rank<<3)+file);
        PieceOnSquare((rank<<3)+file)=-PieceOnSquare(((7-rank)<<3)+file);
        PieceOnSquare(((7-rank)<<3)+file)=piece;
      }
    }
    SetChessBitBoards(&tree->position[0]);
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "flop" command flops the board, interchanging each     |
|   file with the corresponding file on the other half of  |
|   the board.                                             |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("flop",*args)) {
    int file, rank, piece;
    if (thinking || pondering) return(2);
    for (rank=0;rank<8;rank++) {
      for (file=0;file<4;file++) {
        piece=PieceOnSquare((rank<<3)+file);
        PieceOnSquare((rank<<3)+file)=PieceOnSquare((rank<<3)+7-file);
        PieceOnSquare((rank<<3)+7-file)=piece;
      }
    }
    SetChessBitBoards(&tree->position[0]);
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "force" command forces the program to make a specific  |
|   move instead of its last chosen move.                  |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("force",*args)) {
    int move, movenum, save_move_number;
    char text[16];

    if (xboard) {
      force=1;
      return(3);
    }
    if (thinking || pondering) return (2);
    if (nargs < 2) {
      printf("usage:  force <move>\n");
      return(1);
    }
    ponder_move=0;
    last_pv.pathd=0;
    last_pv.pathl=0;
    save_move_number=move_number;
    movenum=move_number;
    if (wtm) movenum--;
    strcpy(text,args[1]);
    sprintf(buffer,"reset %d",movenum);
    wtm=ChangeSide(wtm);
    (void) Option(tree);
    move=InputMove(tree,text,0,wtm,0,0);
    if (move) {
      if (input_stream != stdin) printf("%s\n",OutputMove(tree,move,0,wtm));
      fseek(history_file,((movenum-1)*2+1-wtm)*10,SEEK_SET);
      fprintf(history_file,"%9s\n",OutputMove(tree,move,0,wtm));
      MakeMoveRoot(tree,move,wtm);
      last_pv.pathd=0;
      last_pv.pathl=0;
    }
    else if (input_stream == stdin) printf("illegal move.\n");
    wtm=ChangeSide(wtm);
    move_number=save_move_number;
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "go" command does nothing, except force main() to      |
|   start a search.  ("move" is an alias for go).          |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("go",*args) || OptionMatch("move",*args)) {
    char temp[64];
    if (thinking) return(2);
    if (wtm) {
      if (strncmp(pgn_white,"Crafty",6)) {
        strcpy(temp,pgn_white);
        strcpy(pgn_white,pgn_black);
        strcpy(pgn_black,temp);
      }
    }
    else{
      if (strncmp(pgn_black,"Crafty",6)) {
        strcpy(temp,pgn_white);
        strcpy(pgn_white,pgn_black);
        strcpy(pgn_black,temp);
      }
    }
    return(-1);
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "history" command displays game history (moves).       |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("history",*args)) {
    int i;
    char buffer[128];

    printf("    white       black\n");
    for (i=0;i<(move_number-1)*2-wtm+1;i++) {
      fseek(history_file,i*10,SEEK_SET);
      fscanf(history_file,"%s",buffer);
      if (!(i%2)) printf("%3d",i/2+1);
      printf("  %-10s",buffer);
      if (i%2 == 1) printf("\n");
    }
    if (ChangeSide(wtm))printf("  ...\n");
  }
/*
 ----------------------------------------------------------
|                                                          |
|  "hard" command enables thinking on opponent's time.     |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("hard",*args)) {
    ponder=1;
    Print(4095,"pondering enabled.\n");
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "hash" command controls the transposition table size.  |
|   the size can be entered in one of three ways:          |
|                                                          |
|      hash=nnn  where nnn is in bytes.                    |
|      hash=nnnK where nnn is in K bytes.                  |
|      hash=nnnM where nnn is in M bytes.                  |
|                                                          |
|   the only restriction is that the hash table is com-    |
|   puted as follows:  one entry is 16 bytes long.  there  |
|   are 4 tables, two for black, two for white, with one   |
|   of each being twice the size of the other for the same |
|   side.  this means that one entry in one of the small   |
|   tables corresponds to two in the other, so one entry   |
|   really translates to six entries.  Therefore, the size |
|   that is entered is divided by 6*16, and then rounded   |
|   down to the nearest power of two which is a restric-   |
|   tion on the size of a single table.                    |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("hash",*args)) {
    int new_hash_size;

    if (thinking || pondering) return(2);
    if (nargs > 1) {
      new_hash_size=atoi(args[1]);
      if (strchr(args[1],'K') || strchr(args[1],'k')) new_hash_size*=1<<10;
      if (strchr(args[1],'M') || strchr(args[1],'m')) new_hash_size*=1<<20;
      if (new_hash_size < 48*1024) {
        printf("ERROR.  Minimum hash table size is 48K bytes.\n");
        return(1);
      }
      if (new_hash_size > 0) {
        if (hash_table_size) {
          free(trans_ref_a_orig);
          free(trans_ref_b_orig);
        }
        new_hash_size/=16*3;
        for (log_hash=0;log_hash<(int) (8*sizeof(int));log_hash++)
          if ((1<<(log_hash+1)) > new_hash_size) break;
        if (log_hash) {
          hash_table_size=1<<log_hash;
          trans_ref_a_orig=(HASH_ENTRY *) malloc(16*hash_table_size+15);
          trans_ref_b_orig=(HASH_ENTRY *) malloc(16*2*hash_table_size+15);
          trans_ref_a=(HASH_ENTRY*) (((unsigned long) trans_ref_a_orig+15)&~15);
          trans_ref_b=(HASH_ENTRY*) (((unsigned long) trans_ref_b_orig+15)&~15);
          if (!trans_ref_a || !trans_ref_b) {
            printf("malloc() failed, not enough memory.\n");
            free(trans_ref_a_orig);
            free(trans_ref_b_orig);
            hash_table_size=0;
            log_hash=0;
            trans_ref_a=0;
            trans_ref_b=0;
          }
          hash_maska=(1<<log_hash)-1;
          hash_maskb=(1<<(log_hash+1))-1;
          ClearHashTableScores();
        }
        else {
          trans_ref_a=0;
          trans_ref_b=0;
          hash_table_size=0;
          log_hash=0;
        }
      }
      else Print(4095,"ERROR:  hash table size must be > 0\n");
    }
    if (hash_table_size*3*sizeof(HASH_ENTRY) < 1<<20)
      Print(4095,"hash table memory = %dK bytes.\n",
            hash_table_size*3*sizeof(HASH_ENTRY)/(1<<10));
    else {
      if (hash_table_size*3*sizeof(HASH_ENTRY)%(1<<20))
        Print(4095,"hash table memory = %.1fM bytes.\n",
              (float) hash_table_size*3*sizeof(HASH_ENTRY)/(1<<20));
      else
        Print(4095,"hash table memory = %dM bytes.\n",
              hash_table_size*3*sizeof(HASH_ENTRY)/(1<<20));
    }
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "hashp" command controls the pawn hash table size.     |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("hashp",*args)) {
    int i, new_hash_size;

    if (thinking || pondering) return(2);
    if (nargs > 1) {
      new_hash_size=atoi(args[1]);
      if (strchr(args[1],'K') || strchr(args[1],'k')) new_hash_size*=1<<10;
      if (strchr(args[1],'M') || strchr(args[1],'m')) new_hash_size*=1<<20;
      if (new_hash_size < 16*1024) {
        printf("ERROR.  Minimum pawn hash table size is 16K bytes.\n");
        return(1);
      }
      if (pawn_hash_table) {
        free(pawn_hash_table_orig);
        pawn_hash_table_size=0;
        log_pawn_hash=0;
        pawn_hash_table=0;
      }
      new_hash_size/=sizeof(PAWN_HASH_ENTRY);
      for (log_pawn_hash=0;
           log_pawn_hash<(int) (8*sizeof(int));
           log_pawn_hash++)
        if ((1<<(log_pawn_hash+1)) > new_hash_size) break;
      pawn_hash_table_size=1<<log_pawn_hash;
      pawn_hash_table_orig=(PAWN_HASH_ENTRY *) malloc(sizeof(PAWN_HASH_ENTRY)*pawn_hash_table_size+15);
      pawn_hash_table=(PAWN_HASH_ENTRY*) (((unsigned long) pawn_hash_table_orig+15)&~15);
      if (!pawn_hash_table) {
        printf("malloc() failed, not enough memory.\n");
        free(pawn_hash_table_orig);
        pawn_hash_table_size=0;
        log_pawn_hash=0;
        pawn_hash_table=0;
      }
      pawn_hash_mask=(1<<log_pawn_hash)-1;
      for (i=0;i<pawn_hash_table_size;i++) {
        (pawn_hash_table+i)->key=0;
        (pawn_hash_table+i)->p_score=0;
        (pawn_hash_table+i)->black_protected=0;
        (pawn_hash_table+i)->white_protected=0;
        (pawn_hash_table+i)->black_defects_k=0;
        (pawn_hash_table+i)->black_defects_q=0;
        (pawn_hash_table+i)->white_defects_k=0;
        (pawn_hash_table+i)->white_defects_q=0;
        (pawn_hash_table+i)->passed_w=0;
        (pawn_hash_table+i)->passed_w=0;
        (pawn_hash_table+i)->outside=0;
      }
    }
    if (pawn_hash_table_size*sizeof(PAWN_HASH_ENTRY) < 1<<20)
      Print(4095,"pawn hash table memory = %dK bytes.\n",
            pawn_hash_table_size*sizeof(PAWN_HASH_ENTRY)/(1<<10));
    else {
      if (pawn_hash_table_size*sizeof(PAWN_HASH_ENTRY)%(1<<20))
        Print(4095,"pawn hash table memory = %.1fM bytes.\n",
              (float) pawn_hash_table_size*sizeof(PAWN_HASH_ENTRY)/(1<<20));
      else
        Print(4095,"pawn hash table memory = %dM bytes.\n",
              pawn_hash_table_size*sizeof(PAWN_HASH_ENTRY)/(1<<20));
    }
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "help" command lists commands/options.                 |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("help",*args)) {
    if (nargs > 1) {
      if (!strcmp("analyze",args[1])) {
        printf("analyze\n");
        printf("the analyze command puts Crafty into a mode where it will\n");
        printf("search forever in the current position.  when a move is\n");
        printf("entered, crafty will make that move, switch sides, and\n");
        printf("again compute, printing analysis as it searches.  you can\n");
        printf("back up a move by entering \"back\" or you can back up\n");
        printf("several moves by entering \"back <n>\".  note that <n> is\n");
        printf("the number of moves, counting each player's move as one.\n");
      }
      else if (!strcmp("annotate",args[1])) {
        printf("annotate[h] filename b|w|bw moves margin time [n]\n");
        printf("where filename is the input file with game moves, while the\n");
        printf("output will be written to filename.can.  the input file is\n");
        printf("PGN-compatible with one addition, the ability to request that\n");
        printf("alternative moves also be analyzed at any point.  to do this\n");
        printf("at the point where you have alternative moves, simply include\n");
        printf("them in braces {move1, move2}, and Crafty will then search\n");
        printf("them also. b/w/bw indicates whether to annotate only the white\n");
        printf("side (w), the black side (b) or both (bw).  moves indicates\n");
        printf("which moves to annotate.  a single value says start at the\n");
        printf("indicated move and go through the entire game.  a range (20-30)\n");
        printf("annoates the given range only. margin is the difference between\n");
        printf("the search value for the move played in the game, and the best move\n");
        printf("crafty found, before a comment is generated (pawn=1.0).  time is\n");
        printf("the time limit per move in seconds.  if the optional \"n\" is\n");
        printf("appended, this produces N best moves/scores/PV's, rather than\n");
        printf("just the very best move.  it won't display any move that is worse\n");
        printf("than the actual game move played, but you can use -N to force\n");
        printf("Crafty to produce N PV's regardless of how bad they get.\n");
        printf("using 'annotateh' produces an HTML file with bitmapped\n");
        printf("board displays where analysis was displayed.\n");
      }
      else if (!strcmp("book",args[1])) {
        printf("you can use the following commands to customize how the\n");
        printf("program uses the opening book(book.bin and books.bin).\n");
        printf("typically, book.bin contains a large opening database made\n");
        printf("from GM games.  books.bin is a short, customized book that\n");
        printf("contains selected lines that are well-suited to Crafty's\n");
        printf("style of play.  the <flags> can further refine how this\n");
        printf("small book file is used to encourage/avoid specific lines.\n");
        printf("book[s] create [<filename>] [maxply] [mp] [wpc]...creates a\n");
        printf("   new book by first removing the old book.bin.  it then\n");
        printf("   will parse <filename> and add the moves to book.bin (if\n");
        printf("   the book create command was used) or to books.bin (if the\n");
        printf("   books create command was used.)  <maxply> truncates book\n");
        printf("   lines after that many plies (typically 60).  <mp> will \n");
        printf("   exclude *any* move not appearing <mp> times in the input.\n");
        printf("   <wpc> is the winning percentage.  50 means exclude any\n");
        printf("   book move that doesn't have at least 50%% as many wins as\n");
        printf("   losses.\n");
        printf("book mask accept <chars>...............sets the accept mask to\n");
        printf("   the flag characters in <chars> (see flags below.)  any flags\n");
        printf("   set in this mask will include either (a) moves with the \n");
        printf("   flag set, or (b) moves with no flags set.\n");
        printf("more...");
        fflush(stdout);
        (void) Read(1,buffer);
        printf("book mask reject <chars>...............sets the reject mask to\n");
        printf("   the flag characters in <chars> (see flags below.)  any flags\n");
        printf("   set in this mask will reject any moves with the flag\n");
        printf("   set (in the opening book.)\n");
        printf("book off...............................turns the book completely off.\n");
        printf("book random 0|1........................disables/enables randomness.\n");
        printf("bookw weight <v>.......................sets weight for book ordering.\n");
        printf("   (weights are freq (frequency), eval (evaluation)\n");
        printf("   and learn (learned scores).\n");
        printf("book width n...........................specifies how many moves from\n");
        printf("   the sorted set of book moves are to be considered.  1 produces\n");
        printf("   the best move from the set, but provides little randomness.\n");
        printf("   99 includes all moves in the book move set.\n");
        printf("more...");
        fflush(stdout);
        (void) Read(1,buffer);
        printf("flags are one (or more) members of the following set of\n");
        printf("characters:  {?? ? = ! !! 0 1 2 3 4 5 6 7 8 9 A B C D E F}\n");
        printf("normally, ?? means never play, ? means rarely play,\n");
        printf("= means drawish opening, ! means good move, !! means always\n");
        printf("play, and 0-F are user flags that a user can add to any\n");
        printf("move in the book, and by setting the right mask (above) can\n");
        printf("force the program to either always play the move or never\n");
        printf("play the move.  the special character * means all flags\n");
        printf("and is probably dangerous to use.\n");
        printf("flags are added to a move by entering the move, a / or \\\n");
        printf("followed by the flags.  / means add the flags to the move\n");
        printf("preserving other flags already there while \\ means replace\n");
        printf("any flags with those following the \\.\n");
        printf("the format of the book text (raw data) is as follows:\n");
        printf("[title information] (required)\n");
        printf("e4 e5 ... (a sequence of moves)\n");
        printf("[title information for next line] (required)\n");
        printf("e4 e6 ...\n");
        printf("end (optional)\n");
        printf("\n");
      }
      else if (!strcmp("display",args[1])) {
        printf("display changes   -> display variation when it changes.\n");
        printf("display extstats  -> display search extension statistics.\n");
        printf("display general   -> display general info messages.\n");
        printf("display hashstats -> display search hashing statistics.\n");
        printf("display movenum   -> display move numbers in PV.\n");
        printf("display moves     -> display moves as they are searched.\n");
        printf("display nodes     -> display nodes for each move searched.\n");
        printf("display ply1      -> display ply-1 move list/sorting info.\n");
        printf("display stats     -> display basic search statistics.\n");
        printf("display time      -> display time for moves.\n");
        printf("display variation -> display variation at end of iteration.\n");
      }
      else if (OptionMatch("edit",args[1])) {
        printf("edit is used to set up or modify a board position.  it \n");
        printf("recognizes 4 \"commands\" that it uses to alter/set up the\n");
        printf("board position (with appropriate aliases to interface with\n");
        printf("x\n");
        printf("\n");
        printf("# command causes edit to clear the chess board\n");
        printf("c command causes edit to toggle piece color.\n");
        printf("white command causes edit to place white pieces on the\n");
        printf("board; black command causes edit to place black pieces on\n");
        printf("the \n");
        printf("end (\".\" for xboard) causes edit to exit.\n");
        printf("\n");
        printf("three strings of the form [piece][file][rank] will\n");
        printf("place a [piece] on square [file][rank].  the color is set\n");
        printf("by the previous white/black command.  ex:  Ke8 puts a king\n");
        printf("on square e8\n");
        printf("\n");
      }
      else if (OptionMatch("evaluation",args[1])) {
        printf("evaluation option <value>.  this command can be\n");
        printf("used to adjust evaluation.\n");
        printf("\n");
        printf("asymmetry adjusts the asymmetry in king safety. A value of\n");
        printf("zero means 'no asymmetry at all'.  A value of +50 adjusts \n");
        printf("king safety scores so that the opponent king safety scores\n");
        printf("are increased by 50%% which will tend to make Crafty play\n");
        printf("much more aggressively. a value of -50 will adjust the\n");
        printf("opponent's king safety down by 50%% which will tend to\n");
        printf("(make Crafty play much more defensively/passively.\n");
        printf("\n");
        printf("bscale adjusts the scale for blocked pawns.  The default\n");
        printf("is 100.  Making this larger will make Crafty try to avoid\n");
        printf("blocked pawn positions and keep the position open.\n");
        printf("\n");
        printf("kscale is used to increase/decrease the overall king-safety\n");
        printf("scores.  +50 will increase the values of all the king-\n");
        printf("safety scoring terms, and might tend to make crafty try\n");
        printf("some wild (and often unsound) king-side sacrifices to\n");
        printf("expose the opponent's king.  scaling king safety down (-50)\n");
        printf("makes king-safety less important and allows the other \n");
        printf("positional scores to influence the game more, but probably\n");
        printf("will make Crafty easier to attack\n");
        printf("\n");
        printf("pscale adjusts the scale for pawns.  The default\n");
        printf("is 100.  Making this larger will increase the weight of\n");
        printf("pawn structure.\n");
        printf("\n");
        printf("ppscale adjusts the scale for passed pawns.  The default\n");
        printf("is 100.  Making this larger will increase the weight of\n");
        printf("some passed pawn scores like outside passed pawns.\n");
        printf("\n");
        printf("tropism is used to increase/decrease the overall king\n");
        printf("tropism scores.  this attracts pieces toward the enemy\n");
        printf("king.  100 is again the default (same as scale) while\n");
        printf("larger numbers will exaggerate aggressiveness at the\n");
        printf("expense of positional quality.\n");
        printf("\n");
      }
      else if (OptionMatch("list",args[1])) {
        printf("list is used to update the GM/IM/computer lists, which are\n");
        printf("used internally to control how crafty uses the opening book.\n");
        printf("Syntax:  list GM|IM|B|C|S [+|-name] ...\n");
        printf("   GM/IM/C selects the appropriate list.  if no name is given,\n");
        printf("the list is displayed.  if a name is given, it must be preceeded\n");
        printf("by a + (add to list) or -(remove from list).  note that this\n");
        printf("list is not saved in a file, so that anything added or removed\n");
        printf("will be lost when Crafty is re-started.  To solve this, these\n");
        printf("commands can be added to the .craftyrc file.\n");
      }
      else if (OptionMatch("pgn",args[1])) {
        printf("the pgn command is used to set the various PGN headers\n");
        printf("which are printed at the top of a PGN file produced by the\n");
        printf("savegame command.\n");
        printf("options are Event, Site, Round, White, WhiteElo, Black,\n");
        printf("BlackElo, and Result.  each is followed by the appropriate\n");
        printf("value.  most of these should likely be placed in the\n");
        printf("crafty.rc/.craftyrc file since they will be constant for a\n");
        printf("complete event.\n");
      }
      else if (OptionMatch("setboard",args[1])) {
        printf("sb is used to set up the board in any position desired.  it\n");
        printf("uses a forsythe-like string of characters to describe the\n");
        printf("board position.\n");
        printf("the standard piece codes p,n,b,r,q,k are used to denote the\n");
        printf("piece on a square, upper/lower case is used to indicate the\n");
        printf("color (WHITE/black) of the piece.\n");
        printf("\n");
        printf("the pieces are entered with the ranks on the black side of\n");
        printf("the board entered first, and the ranks on the white side\n");
        printf("entered last (ie rank 8 through rank 1).  empty squares, \n");
        printf("a number between 1 and 8 to indicate how many adjacent\n");
        printf("squares are empty.  use a / to terminate each rank after\n");
        printf("all pieces for that rank have been entered.\n");
        printf("\n");
        printf("more...");
        fflush(stdout);
        (void) Read(1,buffer);
        printf("the following input will setup the board position that\n");
        printf("is given below:\n");
        printf("\n");
        printf("                  K2R/PPP////q/5ppp/7k/ b\n");
        printf("\n");
        printf("this assumes that k represents a white king and -q\n");
        printf("represents a black queen.\n");
        printf("\n");
        printf("                   k  *  *  r  *  *  *  *\n");
        printf("                   p  p  p  *  *  *  *  *\n");
        printf("                   *  *  *  *  *  *  *  *\n");
        printf("                   *  *  *  *  *  *  *  *\n");
        printf("                   *  *  *  *  *  *  *  *\n");
        printf("                  -q  *  *  *  *  *  *  *\n");
        printf("                   *  *  *  *  * -p -p -p\n");
        printf("                   *  *  *  *  *  *  * -k\n");
        printf("\n");
        printf("the character after the final / should be either b or w to\n");
        printf("indicate the side to move.  after this side-to-move field\n");
        printf("any of the following characters can appear to indicate the\n");
        printf("following:  KQ: white can castle kingside/queenside/both;\n");
        printf("kq: same for black;  a1-h8: indicates the square occupied\n");
        printf("by a pawn that can be captured en passant.\n");
      }
      else if (!strcmp("test",args[1])) {
        printf("test <filename> [N]\n");
        printf("test is used to run a suite of \"crafty format\"\n");
        printf("test positions in a batch run.  <filename> is the\n");
        printf("name of the file in crafty test format.  [N] is an\n");
        printf("optional paremeter that is used to shorten the test\n");
        printf("time.  If crafty likes the solution move for [N]\n");
        printf("consecutive iterations, it will stop searching that\n");
        printf("position and consider it correct.  This makes a Win At\n");
        printf("Chess 60 second run take under 1/2 hour, for example.\n");
        printf("the \"crafty format\" requires three lines per position.\n");
        printf("The first line must be a \"title\" line and is used to\n");
        printf("identify each position.  The second line is a \"setboard\"\n");
        printf("command to set the position.  The third line is a line that\n");
        printf("begins with \"solution\", and then is followed by one or\n");
        printf("more solution moves.  If a position is correct only if a\n");
        printf("particular move or moves is *not* played, enter the move\n");
        printf("followed by a \"?\?, as in Nf3?, which means that this\n");
        printf("position will be counted as correct only if Nf3 is not\n");
        printf("played.\n");
      }
      else if (!strcmp("time",args[1])) {
        printf("time controls whether the program uses cpu time or\n");
        printf("wall-clock time for timing.  for tournament play,\n");
        printf("it is safer to use wall-clock timing, for testing it\n");
        printf("may be more consistent to use cpu timing if the\n");
        printf("machine is used for other things concurrently with the\n");
        printf("tests being run.\n");
        printf("\n");
        printf("time is also used to set the basic search timing\n");
        printf("controls.  the general form of the command is as\n");
        printf("follows:\n");
        printf("\n");
        printf("      time nmoves/ntime/[nmoves/ntime]/[increment]\n");
        printf("\n");
        printf("nmoves/ntime represents a traditional first time\n");
        printf("control when nmoves is an integer representing the\n");
        printf("number of moves and ntime is the total time allowed\n");
        printf("for these moves.  the [optional] nmoves/ntime is a\n");
        printf("traditional secondary time control.  increment is a\n");
        printf("feature related to ics play and emulates the fischer\n");
        printf("clock where <increment> is added to the time left\n");
        printf("after each move is made.\n");
        printf("\n");
        printf("as an alternative, nmoves can be \"sd\" which represents\n");
        printf("a sudden death time control of the remainder of the\n");
        printf("game played in ntime.  the optional secondary time\n");
        printf("control can be a sudden-death time control, as in the\n");
        printf("following example:\n");
        printf("\n");
        printf("        time 60/30/sd/30\n");
        printf("\n");
        printf("this sets 60 moves in 30 minutes, then game in 30\n");
        printf("additional minutes.  an increment can be added if\n");
        printf("desired.\n");
      }
      else printf("no help available for that command\n");
    }
    else {
      printf("!command..................passes command to a shell.\n");
      printf("alarm on|off..............turns audible alarm on/off.\n");
      printf("analyze...................analyze a game in progress\n");
      printf("annotate..................annotate game [help].\n");
      printf("ansi......................toggles reverse video highlighting.\n");
      printf("bench.....................runs performance benchmark.\n");
      printf("book......................controls book [help].\n");
      printf("black.....................sets black to move.\n");
      printf("cache=n...................sets tablebase cache size.\n");
      printf("clock.....................displays chess clock.\n");
      printf("display...................displays chess board\n");
      printf("display <n>...............sets display options [help]\n");
      printf("draw <n>..................sets default draw score.\n");
      printf("echo......................echos output to display.\n");
      printf("edit......................edit board position. [help]\n");
      printf("epdhelp...................info about EPD facility.\n");
      printf("egtb......................enables endgame database probes\n");
      printf("end.......................terminates program.\n");
      printf("exit......................restores STDIN to key\n");
      printf("evaluation................adjust evaluation terms. [help]\n");
      printf("force <move>..............forces specific move.\n");
      printf("help [command]............displays help.\n");
      printf("hash n....................sets transposition table size\n");
      printf("                          (n bytes, nK bytes or nM bytes)\n");
      printf("hashp n...................sets pawn hash table size\n");
      printf("history...................display game moves.\n");
      printf("import <filename>.........imports learning data (.lrn files).\n");
      printf("info......................displays program settings.\n");
      printf("more...");
      fflush(stdout);
      (void) Read(1,buffer);
      printf("input <filename> [title]..sets STDIN to <filename>.\n");
      printf("                          (and positions to [title] record.)\n");
      printf("kibitz <n>................sets kibitz mode <n> on ICS.\n");
      printf("level <m> <t> <i>.........sets ICS time controls.\n");
      printf("learn <n>.................enables/disables learning.\n");
      printf("list                      update/display GM/IM/computer lists.\n");
      printf("load <file> <title>       load a position from problem file.\n");
      printf("log on|off................turn logging on/off.\n");
      printf("mode normal|tournament....toggles tournament mode.\n");
      printf("move......................initiates search (same as go).\n");
# if defined(SMP)
      printf("mt n......................sets max parallel threads to use.\n");
      printf("mtmin n...................don't thread last n plies.\n");
      printf("mtmax n...................keep threads within n plies.\n");
# endif
      printf("name......................sets opponent's name.\n");
      printf("new.......................initialize and start new game.\n");
      printf("noise n...................no status until n nodes searched.\n");
      printf("operator <minutes>........allocates operator time.\n");
      printf("perf......................times the move generator/make_move.\n");
      printf("perft.....................tests the move generator/make_move.\n");
      printf("pgn option value..........set PGN header information. [help]\n");
      printf("ponder on|off.............toggle pondering off/on.\n");
      printf("ponder move...............ponder \"move\" as predicted move.\n");
      printf("read <filename>...........read moves in [from <filename>].\n");
      printf("reada <filename>..........read moves in [from <filename>].\n");
      printf("                          (appends to current game history.)\n");
      printf("reset n...................reset game to move n.\n");
      printf("resign <m> <n>............set resign threshold to <m> pawns.\n");
      printf("                          <n> = # of moves before resigning.\n");
      printf("more...");
      fflush(stdout);
      (void) Read(1,buffer);
      printf("savegame <filename>.......saves game in PGN format.\n");
      printf("savepos <filename>........saves position in FEN string.\n");
      printf("score.....................print evaluation of position.\n");
      printf("sd n......................sets absolute search depth.\n");
      printf("search <move>.............search specified move only.\n");
      printf("setboard <FEN>............sets board position to FEN position. [help]\n");
      printf("settc.....................set time controls.\n");
      printf("show book.................toggle book statistics.\n");
      printf("sn n......................sets absolute search node limit.\n");
      printf("st n......................sets absolute search time.\n");
      printf("swindle on|off............enables/disables swindle mode.\n");
      printf("test <file> [N]...........test a suite of problems. [help]\n");
      printf("tags......................list PGN header tags.\n");
      printf("time......................time controls. [help]\n");
      printf("trace n...................display search tree below depth n.\n");
      printf("usage <percentage>........adjusts crafty's time usage up or down.\n");
      printf("whisper n.................sets ICS whisper mode n.\n");
      printf("wild n....................sets ICS wild position (7 for now)\n");
      printf("white.....................sets white to move.\n");
      printf("xboard....................sets xboard compatibility mode.\n");
    }
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "hint" displays the expected move based on the last    |
|   search done. [xboard compatibility]                    |
|                                                          |
 ----------------------------------------------------------
*/
  else if (!strcmp("hint",*args)) {
    if (strlen(hint)) {
      printf("Hint: %s\n",hint);
      fflush(stdout);
    }
  }
/*
 ----------------------------------------------------------
|                                                          |
|  "ics" command is normally invoked from main() via the   |
|  ics command-line option.  it sets proper defaults for   |
|  defaults for the custom crafty/ics interface program.   |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("ics",*args)) {
    ics=1;
    display_options&=4095-32;
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "import" <filename> command is used to import a .lrn   |
|   file (either book.lrn or position.lrn) and store the   |
|   results in crafty's binary files (book.bin or          |
|   position.bin) just as though it had learned those      |
|   results by itself.                                     |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("import",*args)) {
    if (thinking || pondering) return(2);
    nargs=ReadParse(buffer,args," 	;=");
    if (nargs < 2) printf("usage:  import <filename> [clear]\n");
    else LearnImport(tree,nargs-1,args+1);
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "input" command directs the program to read input from |
|   a file until eof is reached or an "exit" command is    |
|   encountered while reading the file.                    |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("input",*args)) {
    if (thinking || pondering) return(2);
    nargs=ReadParse(buffer,args," 	=");
    if (nargs < 2) {
      printf("usage:  input <filename>\n");
      return(1);
    }
    if (!(input_stream=fopen(args[1],"r"))) {
      printf("file does not exist.\n");
      input_stream=stdin;
    }
  }
/*
 ----------------------------------------------------------
|                                                          |
|  "info" command gives some information about Crafty.     |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("info",*args)) {
    Print(4095,"Crafty version %s\n",version);
    if (hash_table_size*6*sizeof(HASH_ENTRY) < 1<<20)
      Print(4095,"hash table memory = %dK bytes.\n",
            hash_table_size*3*sizeof(HASH_ENTRY)/(1<<10));
    else {
      if (hash_table_size*6*sizeof(HASH_ENTRY)%(1<<20))
        Print(4095,"hash table memory = %.1fM bytes.\n",
              (float) hash_table_size*3*sizeof(HASH_ENTRY)/(1<<20));
      else
        Print(4095,"hash table memory = %dM bytes.\n",
              hash_table_size*3*sizeof(HASH_ENTRY)/(1<<20));
    }
    if (pawn_hash_table_size*sizeof(PAWN_HASH_ENTRY) < 1<<20)
      Print(4095,"pawn hash table memory = %dK bytes.\n",
            pawn_hash_table_size*sizeof(PAWN_HASH_ENTRY)/(1<<10));
    else {
      if (pawn_hash_table_size*6*sizeof(PAWN_HASH_ENTRY)%(1<<20))
        Print(4095,"pawn hash table memory = %.1fM bytes.\n",
              (float) pawn_hash_table_size*sizeof(PAWN_HASH_ENTRY)/(1<<20));
      else
        Print(4095,"pawn hash table memory = %dM bytes.\n",
              pawn_hash_table_size*sizeof(PAWN_HASH_ENTRY)/(1<<20));
    }
    if (!tc_sudden_death) {
      Print(4095,"%d moves/%d minutes %d seconds primary time control\n",
            tc_moves, tc_time/6000, (tc_time/100)%60);
      Print(4095,"%d moves/%d minutes %d seconds secondary time control\n",
            tc_secondary_moves, tc_secondary_time/6000,
            (tc_secondary_time/100)%60);
      if (tc_increment) Print(4095,"increment %d seconds.\n",tc_increment/100);
    }
    else if (tc_sudden_death == 1) {
      Print(4095," game/%d minutes primary time control\n", tc_time/100);
      if (tc_increment) Print(4095,"increment %d seconds.\n",(tc_increment/100)%60);
    }
    else if (tc_sudden_death == 2) {
      Print(4095,"%d moves/%d minutes primary time control\n",
            tc_moves, tc_time/6000);
      Print(4095,"game/%d minutes secondary time control\n",
            tc_secondary_time/6000);
      if (tc_increment) Print(4095,"increment %d seconds.\n",tc_increment/100);
    }
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "kibitz" command sets kibitz mode for ICS.  =1 will    |
|   kibitz mate announcements, =2 will kibitz scores and   |
|   other info, =3 will kibitz scores and PV, =4 adds the  |
|   list of book moves, =5 displays the PV after each      |
|   iteration completes, and =6 displays the PV each time  |
|   it changes in an iteration.                            |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("kibitz",*args)) {
    if (nargs < 2) {
      printf("usage:  kibitz <level>\n");
      return(1);
    }
    kibitz=atoi(args[1]);
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "learn" command enables/disables the learning          |
|   algorithms used in crafty.  these are controlled by    |
|   a single variable with multiple boolean switches in    |
|   it as defined below:                                   |
|                                                          |
|   000 -> no learning enabled.                            |
|   001 -> learn which book moves are good and bad.        |
|   010 -> learn middlegame positions.                     |
|   100 -> learn from game "results" (win/lose).           |
|                                                          |
|   these are entered as an integer, which is formed by    |
|   adding the integer values 1,2,4 to enable the various  |
|   forms of learning.                                     |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("learn",*args)) {
    if (nargs == 2) learning=atoi(args[1]);
    if (learning&book_learning)
      Print(4095,"book learning enabled\n");
    else
      Print(4095,"book learning disabled\n");
    if (learning&result_learning)
      Print(4095,"result learning enabled\n");
    else
      Print(4095,"result learning disabled\n");
    if (learning&position_learning)
      Print(4095,"position learning enabled\n");
    else
      Print(4095,"position learning disabled\n");
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "level" command sets time controls [ics/xboard         |
|   compatibility.]                                        |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("level",*args)) {
    if (nargs < 4) {
      printf("usage:  level <nmoves> <stime> <inc>\n");
      return(1);
    }
    tc_moves=atoi(args[1]);
    tc_time=atoi(args[2])*100;
    tc_increment=atoi(args[3])*100;
    tc_time_remaining=tc_time;
    tc_time_remaining_opponent=tc_time;
    if (!tc_moves) {
      tc_sudden_death=1;
      tc_moves=1000;
      tc_moves_remaining=1000;
    }
    else tc_sudden_death=0;
    if (tc_moves) {
      tc_secondary_moves=tc_moves;
      tc_secondary_time=tc_time;
      tc_moves_remaining=tc_moves;
    }
    if (!tc_sudden_death) {
      Print(4095,"%d moves/%d minutes primary time control\n",
            tc_moves, tc_time/100);
      Print(4095,"%d moves/%d minutes secondary time control\n",
            tc_secondary_moves, tc_secondary_time/100);
      if (tc_increment) Print(4095,"increment %d seconds.\n",tc_increment/100);
    }
    else if (tc_sudden_death == 1) {
      Print(4095," game/%d minutes primary time control\n",tc_time/100);
      if (tc_increment) Print(4095,"increment %d seconds.\n",tc_increment/100);
    }
    tc_time*=60;
    tc_time_remaining=tc_time;
    tc_secondary_time*=60;
    if (tc_time > 30000 || tc_increment > 300) whisper=0;
    if (tc_time <= 6000 && tc_increment <= 100) whisper=0;
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "list" command allows the operator to add or remove    |
|   names from the GM_list, IM_list , computer_list,       |
|   auto_kibitz_list or special_list.                      |
|   The  syntax is "list <list> <option> <name>.           |
|   <list> is one of GM, IM, C, AK or S.                   |
|   The final parameter is a name to add  or remove.       |
|   if the name is in the list, it is removed,             |
|   otherwise it is added.  if no name is given, the list  |
|   is displayed.                                          |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("list",*args)) {
    int i, j;
    char listname[8];
    char **targs;

    targs=args;
    strcpy(listname,args[1]);
    nargs-=2;
    targs+=2;
    if (nargs) {
      while (nargs) {
        if (!strcmp(listname,"GM")) {
          if (targs[0][0] == '-') {
            for (i=0;i<number_of_GMs;i++)
              if (!strcmp(GM_list[i],targs[0]+1)) {
                for (j=i;j<number_of_GMs;j++)
                  strcpy(GM_list[j],GM_list[j+1]);
                number_of_GMs--;
                i=0;
                Print(4095,"%s removed from GM list.\n",targs[0]+1);
                break;
              }
          }
          else if (targs[0][0] == '+') {
            for (i=0;i<number_of_GMs;i++)
              if (!strcmp(GM_list[i],targs[0]+1)) {
                Print(4095, "Warning: %s is already in GM list.\n",targs[0]+1);
                break;
              }
            if (number_of_GMs >= 512)
              Print(4095,"ERROR!  GM list is full at 512 entries\n");
            else if (i==number_of_GMs) {
              strcpy(GM_list[number_of_GMs++],targs[0]+1);
              Print(4095,"%s added to GM list.\n",targs[0]+1);
            }
          }
          else if (!strcmp(targs[0],"clear")) {
            number_of_GMs=0;
          }
          else printf("error, name must be preceeded by +/- flag.\n");
        }
        if (!strcmp(listname,"B")) {
          if (targs[0][0] == '-') {
            for (i=0;i<number_of_blockers;i++)
              if (!strcmp(blocker_list[i],targs[0]+1)) {
                for (j=i;j<number_of_blockers;j++)
                  strcpy(blocker_list[j],blocker_list[j+1]);
                number_of_blockers--;
                i=0;
                Print(4095,"%s removed from blocker list.\n",targs[0]+1);
                break;
              }
          }
          else if (targs[0][0] == '+') {
            for (i=0;i<number_of_blockers;i++)
              if (!strcmp(blocker_list[i],targs[0]+1)) {
                Print(4095, "Warning: %s is already in B list.\n",targs[0]+1);
                break;
              }
            if (number_of_blockers >= 512)
              Print(4095,"ERROR!  blocker list is full at 512 entries\n");
            else if (i==number_of_blockers) {
              strcpy(blocker_list[number_of_blockers++],targs[0]+1);
              Print(4095,"%s added to blocker list.\n",targs[0]+1);
            }
            }
          else if (!strcmp(targs[0],"clear")) {
            number_of_blockers=0;
          }
          else Print(4095,"error, name must be preceeded by +/- flag.\n");
        }
        if (!strcmp(listname,"S")) {
          if (targs[0][0] == '-') {
            for (i=0;i<number_of_specials;i++)
              if (!strcmp(special_list[i],targs[0]+1)) {
                for (j=i;j<number_of_specials;j++)
                  strcpy(special_list[j],special_list[j+1]);
                number_of_specials--;
                i=0;
                Print(4095,"%s removed from special list.\n",targs[0]+1);
                break;
              }
          }
          else if (targs[0][0] == '+') {
            for (i=0;i<number_of_specials;i++)
              if (!strcmp(special_list[i],targs[0]+1)) {
                Print(4095, "Warning: %s is already in S list.\n",targs[0]+1);
                break;
              }
            if (number_of_specials >= 512)
              Print(4095,"ERROR!  special list is full at 512 entries\n");
            else if (i==number_of_specials) {
              strcpy(special_list[number_of_specials++],targs[0]+1);
              Print(4095,"%s added to special list.\n",targs[0]+1);
            }
          }
          else if (!strcmp(targs[0],"clear")) {
            number_of_specials=0;
          }
          else Print(4095,"error, name must be preceeded by +/- flag.\n");
        }
        if (!strcmp(listname,"IM")) {
          if (targs[0][0] == '-') {
            for (i=0;i<number_of_IMs;i++)
              if (!strcmp(IM_list[i],targs[0]+1)) {
                for (j=i;j<number_of_IMs;j++)
                  strcpy(IM_list[j],IM_list[j+1]);
                number_of_IMs--;
                i=0;
                Print(4095,"%s removed from IM list.\n",targs[0]+1);
                break;
              }
          }
          else if (targs[0][0] == '+') {
            for (i=0;i<number_of_IMs;i++)
              if (!strcmp(IM_list[i],targs[0]+1)) {
                Print(4095, "Warning: %s is already in IM list.\n",targs[0]+1);
                break;
              }
            if (number_of_IMs >= 512)
              Print(4095,"ERROR!  IM list is full at 512 entries\n");
            else if (i==number_of_IMs) {
              strcpy(IM_list[number_of_IMs++],targs[0]+1);
              Print(4095,"%s added to IM list.\n",targs[0]+1);
            }
          }
          else if (!strcmp(targs[0],"clear")) {
            number_of_IMs=0;
          }
          else Print(4095,"error, name must be preceeded by +/- flag.\n");
        }
        if (!strcmp(listname,"C")) {
          if (targs[0][0] == '-') {
            for (i=0;i<number_of_computers;i++)
              if (!strcmp(computer_list[i],targs[0]+1)) {
                for (j=i;j<number_of_computers;j++)
                  strcpy(computer_list[j],computer_list[j+1]);
                number_of_computers--;
                i=0;
                Print(4095,"%s removed from computer list.\n",targs[0]+1);
                break;
              }
          }
          else if (targs[0][0] == '+') {
            for (i=0;i<number_of_computers;i++)
              if (!strcmp(computer_list[i],targs[0]+1)) {
                Print(4095, "Warning: %s is already in C list.\n",targs[0]+1);
                break;
              }
            if (number_of_computers >= 512)
              Print(4095,"ERROR!  C list is full at 512 entries\n");
            else if (i==number_of_computers) {
              strcpy(computer_list[number_of_computers++],targs[0]+1);
              Print(4095,"%s added to computer list.\n",targs[0]+1);
            }
          }
          else if (!strcmp(targs[0],"clear")) {
            number_of_computers=0;
          }
          else Print(4095,"error, name must be preceeded by +/- flag.\n");
        }
        if (!strcmp(listname,"AK")) {
          if (targs[0][0] == '-') {
            for (i=0;i<number_auto_kibitzers;i++)
              if (!strcmp(auto_kibitz_list[i],targs[0]+1)) {
                for (j=i;j<number_auto_kibitzers;j++)
                  strcpy(auto_kibitz_list[j],auto_kibitz_list[j+1]);
                number_auto_kibitzers--;
                i=0;
                Print(4095,"%s removed from auto kibitz list.\n",targs[0]+1);
                break;
              }
          }
          else if (targs[0][0] == '+') {
            for (i=0;i<number_auto_kibitzers;i++)
              if (!strcmp(auto_kibitz_list[i],targs[0]+1)) {
                Print(4095, "Warning: %s is already in AK list.\n",targs[0]+1);
                break;
              }
            if (number_auto_kibitzers >= 64)
              Print(4095,"ERROR!  AK list is full at 64 entries\n");
            else if (i==number_auto_kibitzers) {
              strcpy(auto_kibitz_list[number_auto_kibitzers++],targs[0]+1);
              Print(4095,"%s added to auto kibitz list.\n",targs[0]+1);
            }
          }
          else if (!strcmp(targs[0],"clear")) {
            number_auto_kibitzers=0;
          }
          else Print(4095,"error, name must be preceeded by +/- flag.\n");
        }
        nargs--;
        targs++;
      }
    }
    else if (!strcmp(listname,"GM")) {
      Print(4095,"GM List:\n");
      for (i=0;i<number_of_GMs;i++)
        Print(4095,"%s\n",GM_list[i]);
    }
    else if (!strcmp(listname,"B")) {
      Print(4095,"blocker List:\n");
      for (i=0;i<number_of_blockers;i++)
        Print(4095,"%s\n",blocker_list[i]);
    }
    else if (!strcmp(listname,"S")) {
      Print(4095,"special List:\n");
      for (i=0;i<number_of_specials;i++)
        Print(4095,"%s\n",special_list[i]);
    }
    else if (!strcmp(listname,"IM")) {
      Print(4095,"IM List:\n");
      for (i=0;i<number_of_IMs;i++)
        Print(4095,"%s\n",IM_list[i]);
    }
    else if (!strcmp(listname,"C")) {
      Print(4095, "computer list:\n");
      for (i=0;i<number_of_computers;i++)
        Print(4095,"%s\n",computer_list[i]);
    }
    else if (!strcmp(listname,"AK")) {
      Print(4095, "auto kibitz list:\n");
      for (i=0;i<number_auto_kibitzers;i++)
        Print(4095,"%s\n",auto_kibitz_list[i]);
    }
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "load" command directs the program to read input from  |
|   a file until a "setboard" command is found  this       |
|   command is then executed, setting up the position for  |
|   a search.                                              |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("load",*args)) {
    char title[64];
    char *readstat;
    FILE *prob_file;

    if (thinking || pondering) return(2);
    nargs=ReadParse(buffer,args," 	=");
    if (nargs < 3) {
      printf("usage:  input <filename> title\n");
      return(1);
    }
    if (!(prob_file=fopen(args[1],"r"))) {
      printf("file does not exist.\n");
      return(1);
    }
    strcpy(title,args[2]);
    while (!feof(prob_file)) {
      readstat=fgets(buffer,128,prob_file);
      if (readstat) {
        char *delim;
        delim=strchr(buffer,'\n');
        if (delim) *delim=0;
        delim=strchr(buffer,'\r');
        if (delim) *delim=' ';
      }
      if (readstat == NULL) break;
      nargs=ReadParse(buffer,args," 	;\n");
      if (!strcmp(args[0],"title") && strstr(buffer,title)) break;
    }
    while (!feof(prob_file)) {
      readstat=fgets(buffer,128,prob_file);
      if (readstat) {
        char *delim;
        delim=strchr(buffer,'\n');
        if (delim) *delim=0;
        delim=strchr(buffer,'\r');
        if (delim) *delim=' ';
      }
      if (readstat == NULL) break;
      nargs=ReadParse(buffer,args," 	;\n");
      if (!strcmp(args[0],"setboard")) {
        Option(tree);
        break;
      }
    }
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "log" command turns log on/off, and also lets you view |
|   the end of the log or copy it to disk as needed.  To   |
|   view the end, simply type "log <n>" where n is the #   |
|   of lines you'd like to see (the last <n> lines).  you  |
|   can add a filename to the end and the output will go   |
|   to this file instead.                                  |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("log",*args)) {
    FILE *output_file;
    char filename[64], buffer[128];

    if (nargs < 2) {
      printf("usage:  log on|off|n [filename]\n");
      return(1);
    }
    if (!strcmp(args[1],"on")) {
      for (log_id=1;log_id <300;log_id++) {
#if defined(MACOS)
        sprintf(log_filename,":%s:log.%03d",log_path,log_id);
        sprintf(history_filename,":%s:game.%03d",log_path,log_id);
#else
        sprintf(log_filename,"%s/log.%03d",log_path,log_id);
        sprintf(history_filename,"%s/game.%03d",log_path,log_id);
#endif
        log_file=fopen(log_filename,"r");
        if (!log_file) break;
        fclose(log_file);
      }
      log_file=fopen(log_filename,"w");
      history_file=fopen(history_filename,"w+");
    }
    else if (!strcmp(args[1],"off")) {
      if (log_file) fclose(log_file);
      log_file=0;
#if defined(MACOS)
      sprintf(filename,":%s:log.%03d",log_path,log_id);
#else
      sprintf(filename,"%s/log.%03d",log_path,log_id);
#endif
      remove(filename);
    }
    else {
      int nrecs, trecs, lrecs;
      char *eof;
      FILE *log;

      nrecs=atoi(args[1]);
      output_file=stdout;
      if (nargs > 2) output_file=fopen(args[2],"w");
      log=fopen(log_filename,"r");
      for (trecs=1; trecs<99999999; trecs++) {
        eof=fgets(buffer,128,log);
        if (eof) {
          char *delim;
          delim=strchr(buffer,'\n');
          if (delim) *delim=0;
          delim=strchr(buffer,'\r');
          if (delim) *delim=' ';
        }
        else break;
      }
      fseek(log,0,SEEK_SET);
      for (lrecs=1; lrecs<trecs-nrecs; lrecs++) {
        eof=fgets(buffer,128,log);
        if (eof) {
          char *delim;
          delim=strchr(buffer,'\n');
          if (delim) *delim=0;
          delim=strchr(buffer,'\r');
          if (delim) *delim=' ';
        }
        else break;
      }
      for (;lrecs<trecs; lrecs++) {
        eof=fgets(buffer,128,log);
        if (eof) {
          char *delim;
          delim=strchr(buffer,'\n');
          if (delim) *delim=0;
          delim=strchr(buffer,'\r');
          if (delim) *delim=' ';
        }
        else break;
        fprintf(output_file,"%s\n",buffer);
      }
      if (output_file != stdout) fclose(output_file);
    }
  }
#if defined(SMP)
/*
 ----------------------------------------------------------
|                                                          |
|   "smp" command is used to tune the various SMP search   |
|   parameters.                                            |
|                                                          |
|   "smpmin" command is used to set the minimum depth of   |
|   a tree before a thread can be started.  this is used   |
|   to prevent the thread creation overhead from becoming  |
|   larger than the time actually needed to search the     |
|   tree.                                                  |
|                                                          |
|   "smpmt" command is used to set the maximum number of   |
|   parallel threads to use, assuming that Crafty was      |
|   compiled with -DSMP.  this value can not be set        |
|   larger than the compiled-in -DCPUS=n value.            |
|                                                          |
|   "smproot" command is used to enable (1) or disable (0) |
|   splitting the tree at the root (ply=1).  splitting at  |
|   the root is more efficient, but might slow finding the |
|   move in some test positions.                           |
|                                                          |
|   "smpgroup" command is used to control how many threads |
|   may work together at any point in the tree.  the       |
|   usual default is 8, but this might be reduced on a     |
|   machine with a large number of processors.  it should  |
|   be tested, of course.                                  |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("smpgroup",*args)) {
    if (nargs < 2) {
      printf("usage:  smpgroup <threads>\n");
      return(1);
    }
    max_thread_group=atoi(args[1]);
    Print(4095,"maximum thread group size set to %d\n",max_thread_group);
  }
  else if (OptionMatch("smpmin",*args)) {
    if (nargs < 2) {
      printf("usage:  smpmin <plies>\n");
      return(1);
    }
    min_thread_depth=INCPLY*atoi(args[1]);
    Print(4095,"minimum thread depth set to %d\n",min_thread_depth);
  }
  else if (OptionMatch("smpmt",*args) || OptionMatch("mt",*args)) {
    int proc;
    if (nargs < 2) {
      printf("usage:  smpmt=<threads>\n");
      return(1);
    }
    if (thinking || pondering) return(3);
    max_threads=atoi(args[1]);
    if (max_threads > CPUS) {
      Print(4095,"ERROR - Crafty was compiled with CPUS=%d.",CPUS);
      Print(4095,"  mt can not exceed this value.\n");
      max_threads=CPUS;
    }
    if (max_threads)
      Print(4095,"max threads set to %d\n",max_threads);
    else
      Print(4095,"parallel threads disabled.\n");
    for (proc=0;proc<CPUS;proc++)
      if (proc >= max_threads) thread[proc]=(TREE*)-1;
  }
  else if (OptionMatch("smproot",*args)) {
    if (nargs < 2) {
      printf("usage:  smproot 0|1\n");
      return(1);
    }
    split_at_root=atoi(args[1]);
    if (split_at_root)
      Print(4095,"SMP search split at ply >= 1\n");
    else
      Print(4095,"SMP search split at ply > 1\n");
  }
#endif
/*
 ----------------------------------------------------------
|                                                          |
|   "mn" command is used to set the move number to a       |
|   specific value...                                      |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("mn",*args)) {
    if (nargs < 2) {
      printf("usage:  mn <move_number>\n");
      return(1);
    }
    move_number=atoi(args[1]);
    Print(4095,"move number set to %d\n",move_number);
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "mode" command sets tournament mode or normal mode.    |
|   tournament mode is used when crafty is in a "real"     |
|   tournament.  it forces draw_score to 0, and makes      |
|   crafty display the chess clock after each move.        |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("mode",*args)) {
    if (nargs > 1) {
      if (!strcmp(args[1],"tournament")) {
        mode=tournament_mode;
        printf("use 'settc' command if a game is restarted after crafty\n");
        printf("has been terminated for any reason.\n");
      }
      else if (!strcmp(args[1],"normal")) {
        mode=normal_mode;
      }
      else {
        printf("usage:  mode normal|tournament\n");
        mode=normal_mode;
      }
    }
    if (mode == tournament_mode) printf("tournament mode.\n");
    else if (mode == normal_mode) printf("normal mode.\n");
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "name" command saves opponents name and writes it into |
|   logfile along with the date/time.  it also scans the   |
|   list of known computers and adjusts its opening book   |
|   to play less "risky" if it matches.  if the opponent   |
|   is in the GM list, it tunes the resignation controls   |
|   to resign earlier.  ditto for IM, although the resign  |
|   threshold is not tightened so much.                    |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("name",*args)) {
    int i;
    char *next;

    if (nargs < 2) {
      printf("usage:  name <name>\n");
      return(1);
    }
    if (wtm) {
      strcpy(pgn_white,args[1]);
      sprintf(pgn_black,"Crafty %s",version);
    }
    else {
      strcpy(pgn_black,args[1]);
      sprintf(pgn_white,"Crafty %s",version);
    }
    Print(4095,"Crafty %s vs %s\n",version,args[1]);
    next=args[1];
    while (*next) {
      *next=tolower(*next);
      next++;
    }
    if (mode != tournament_mode) {
      for (i=0;i<number_of_blockers;i++)
        if (!strcmp(blocker_list[i],args[1])) {
          blocked_scale*=1.5;
          break;
        }
      for (i=0;i<number_auto_kibitzers;i++)
        if (!strcmp(auto_kibitz_list[i],args[1])) {
          kibitz=4;
          break;
        }
      for (i=0;i<number_of_computers;i++)
        if (!strcmp(computer_list[i],args[1])) {
          Print(128,"playing a computer!\n");
          computer_opponent=1;
          book_selection_width=2;
          usage_level=0;
          book_weight_freq=1.0;
          book_weight_eval=.1;
          book_weight_learn=.2;
          break;
        }
      for (i=0;i<number_of_GMs;i++)
        if (!strcmp(GM_list[i],args[1])) {
          Print(128,"playing a GM!\n");
          book_selection_width=3;
          resign=Min(6,resign);
          resign_count=4;;
          draw_count=8;
          accept_draws=1;
          kibitz=0;
          break;
        }
      for (i=0;i<number_of_specials;i++)
        if (!strcmp(special_list[i],args[1])) {
          Print(128,"playing a special player!\n");
          book_selection_width=4;
          resign=Min(9,resign);
          resign_count=5;
          draw_count=8;
          kibitz=0;
          trojan_check=1;
          break;
        }
      for (i=0;i<number_of_IMs;i++)
        if (!strcmp(IM_list[i],args[1])) {
          Print(128,"playing an IM!\n");
          book_selection_width=4;
          resign=Min(9,resign);
          resign_count=5;
          draw_count=8;
          accept_draws=1;
          kibitz=0;
          break;
        }
    }
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "new" command initializes for a new game.  note that   |
|   "AN" is an alias for this command, for auto232         |
|   compatibility.                                         |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("new",*args) || OptionMatch("AN",*args)) {
    new_game=1;
    if (thinking || pondering) return(3);
    NewGame(0);
    return(3);
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "noise" command sets a minimum limit on nodes searched |
|   such that until this number of nodes has been searched |
|   no program output will occur.  this is used to prevent |
|   simple endgames from swamping the display device since |
|   30+ ply searches are possible, which can produce 100's |
|   of lines of output.                                    |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("noise",*args)) {
    if (nargs < 2) {
      printf("usage:  noise <n>\n");
      return(1);
    }
    noise_level=atoi(args[1]);
    Print(4095,"noise level set to %d.\n",noise_level);
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "operator" command sets the operator time.  this time  |
|   is the time per move that the operator needs.  it is   |
|   multiplied by the number of moves left to time control |
|   to reserve operator time.                              |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("operator",*args)) {
    if (nargs < 2) {
      printf("usage:  operator <seconds>\n");
      return(1);
    }
    tc_operator_time=ParseTime(args[1])*100;
    Print(4095,"reserving %d seconds per move for operator overhead.\n",
          tc_operator_time/100);
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "otime" command sets the opponent's time remaining.    |
|   this is used to determine if the opponent is in time   |
|   trouble, and is factored into the draw score if he is. |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("otime",*args)) {
    if (nargs < 2) {
      printf("usage:  otime <time(unit=.01 secs))>\n");
      return(1);
    }
    tc_time_remaining_opponent=atoi(args[1]);
    if (log_file && time_limit>99)
      fprintf(log_file,"time remaining: %s (opponent).\n",
              DisplayTime(tc_time_remaining_opponent));
    if (call_flag && xboard && tc_time_remaining_opponent <= 1) {
      if (crafty_is_white) Print(4095,"1-0 {Black ran out of time}\n");
      else Print(4095,"0-1 {White ran out of time}\n");
    }
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "output" command sets long or short algebraic output.  |
|   long is Ng1f3, while short is simply Nf3.              |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("output",*args)) {
    if (nargs < 2) {
      printf("usage:  output long|short\n");
      return(1);
    }
    if (!strcmp(args[1],"long")) output_format=1;
    else if (!strcmp(args[1],"short")) output_format=0;
    else printf("usage:  output long|short\n");
    if (output_format == 1)
      Print(4095,"output moves in long algebraic format\n");
    else if (output_format == 0)
      Print(4095,"output moves in short algebraic format\n");
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "bookpath", "logpath" and "tbpath" commands set the    |
|   default paths to find or create these files.           |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("logpath",*args) ||
           OptionMatch("bookpath",*args) ||
           OptionMatch("tbpath",*args)) {
    if (OptionMatch("logpath",*args) ||
        OptionMatch("bookpath",*args)) {
      if (log_file)
        Print(4095,"ERROR -- this must be used on command line only\n");
    }
    nargs=ReadParse(buffer,args," 	=");
    if (nargs < 2) {
      printf("usage:  bookpath|logpath|tbpath <path>\n");
      return(1);
    }
    if (!strchr(args[1],'(')) {
      if (strstr(args[0],"bookpath")) strcpy(book_path,args[1]);
      else if (strstr(args[0],"logpath")) strcpy(log_path,args[1]);
      else if (strstr(args[0],"tbpath")) {
        strcpy(tb_path,args[1]);
        EGTBlimit=IInitializeTb(tb_path);
        Print(128,"%d piece tablebase files found\n",EGTBlimit);
        if (0 != cbEGTBCompBytes)
          Print(128,"%dkb of RAM used for TB indices and decompression tables\n",
                (cbEGTBCompBytes+1023)/1024);
        if (EGTBlimit) {
          if (!EGTB_cache) EGTB_cache=malloc(EGTB_cache_size);
          if (!EGTB_cache) {
            Print(4095,"ERROR  EGTB cache malloc failed\n");
            EGTB_cache=malloc(EGTB_CACHE_DEFAULT);
          }
          else FTbSetCacheSize(EGTB_cache,EGTB_cache_size);
          egtbsetup=1;
        }
      }
    }
    else {
      if (strchr(args[1],')')) {
        *strchr(args[1],')')=0;
        if (strstr(args[0],"bookpath")) strcpy(book_path,args[1]+1);
        else if (strstr(args[0],"logpath")) strcpy(log_path,args[1]+1);
        else if (strstr(args[0],"tbpath")) {
          strcpy(tb_path,args[1]+1);
          EGTBlimit=IInitializeTb(tb_path);
          Print(128,"%d piece tablebase files found\n",EGTBlimit);
          if (0 != cbEGTBCompBytes)
            Print(128,"%dkb of RAM used for TB indices and decompression tables\n",
                  (cbEGTBCompBytes+1023)/1024);
          if (EGTBlimit) {
            if (!EGTB_cache) EGTB_cache=malloc(EGTB_cache_size);
            if (!EGTB_cache) {
              Print(4095,"ERROR  EGTB cache malloc failed\n");
              EGTB_cache=malloc(EGTB_CACHE_DEFAULT);
            }
            else FTbSetCacheSize(EGTB_cache,EGTB_cache_size);
            egtbsetup=1;
          }
        }
      }
      else Print(4095,"ERROR multiple paths must be enclosed in ( and )\n");
    }
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "perf" command turns times move generator/make_move.   |
|                                                          |
 ----------------------------------------------------------
*/
#define PERF_CYCLES 100000
  else if (OptionMatch("perf",*args)) {
    int i, *mv, clock_before, clock_after;
    float time_used;

    if (thinking || pondering) return(2);
    clock_before = clock();
    while (clock() == clock_before);
    clock_before = clock();
    for (i=0;i<PERF_CYCLES;i++) {
      tree->last[1]=GenerateCaptures(tree, 0, wtm, tree->last[0]);
      tree->last[1]=GenerateNonCaptures(tree, 0, wtm, tree->last[1]);
    }
    clock_after=clock();
    time_used=((float) clock_after-(float) clock_before) /
              (float) CLOCKS_PER_SEC;
    printf("generated %d moves, time=%.2f seconds\n",
           (tree->last[1]-tree->last[0])*PERF_CYCLES,time_used);
    printf("generated %d moves per second\n",(int) (((float) (PERF_CYCLES*
           (tree->last[1]-tree->last[0])))/time_used));
    clock_before=clock();
    while (clock() == clock_before);
    clock_before = clock();
    for (i=0;i<PERF_CYCLES;i++) {
      tree->last[1]=GenerateCaptures(tree, 0, wtm, tree->last[0]);
      tree->last[1]=GenerateNonCaptures(tree, 0, wtm, tree->last[1]);
      for (mv=tree->last[0];mv<tree->last[1];mv++) {
        MakeMove(tree,0,*mv,wtm);
        UnMakeMove(tree,0,*mv,wtm);
      }
    }
    clock_after=clock();
    time_used=((float) clock_after-(float) clock_before) /
              (float) CLOCKS_PER_SEC;
    printf("generated/made/unmade %d moves, time=%.2f seconds\n",
      (tree->last[1]-tree->last[0])*PERF_CYCLES, time_used);
    printf("generated/made/unmade %d moves per second\n",(int) (((float) (PERF_CYCLES*
           (tree->last[1]-tree->last[0])))/time_used));
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "perft" command turns tests move generator/make_move.  |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("perft",*args)) {
    int i;

    if (thinking || pondering) return(2);
    if (nargs < 2) {
      printf("usage:  perftest <depth>\n");
      return(1);
    }
    tree->position[1]=tree->position[0];
    tree->last[0]=tree->move_list;
    i=atoi(args[1]);
    total_moves=0;
    OptionPerft(tree,1,i,wtm);
    printf("total moves=%d\n",total_moves);
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "pgn" command sets the various PGN header files.       |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("pgn",*args)) {
    int i;

    if (nargs < 3) {
      printf("usage:  pgn <tag> <value>\n");
      return(1);
    }
    if (!strcmp(args[1],"Event")) {
      pgn_event[0]=0;
      for (i=2;i<nargs;i++) {
        strcpy(pgn_event+strlen(pgn_event),args[i]);
        strcpy(pgn_event+strlen(pgn_event)," ");
      }
    }
    else if (!strcmp(args[1],"Site")) {
      pgn_site[0]=0;
      for (i=2;i<nargs;i++) {
        strcpy(pgn_site+strlen(pgn_site),args[i]);
        strcpy(pgn_site+strlen(pgn_site)," ");
      }
    }
    else if (!strcmp(args[1],"Round")) {
      pgn_round[0]=0;
      strcpy(pgn_round,args[2]);
    }
    else if (!strcmp(args[1],"White")) {
      pgn_white[0]=0;
      for (i=2;i<nargs;i++) {
        strcpy(pgn_white+strlen(pgn_white),args[i]);
        strcpy(pgn_white+strlen(pgn_white)," ");
      }
    }
    else if (!strcmp(args[1],"WhiteElo")) {
      pgn_white_elo[0]=0;
      strcpy(pgn_white_elo,args[2]);
    }
    else if (!strcmp(args[1],"Black")) {
      pgn_black[0]=0;
      for (i=2;i<nargs;i++) {
        strcpy(pgn_black+strlen(pgn_black),args[i]);
        strcpy(pgn_black+strlen(pgn_black)," ");
      }
    }
    else if (!strcmp(args[1],"BlackElo")) {
      pgn_black_elo[0]=0;
      strcpy(pgn_black_elo,args[2]);
    }
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "ponder" command toggles pondering off/on or sets a    |
|   move to ponder.                                        |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("ponder",*args)) {
    if (thinking || pondering) return(2);
    if (nargs < 2) {
      printf("usage:  ponder off|on|<move>\n");
      return(1);
    }
    if (!strcmp(args[1],"on")) {
      ponder=1;
      Print(4095,"pondering enabled.\n");
    }
    else if (!strcmp(args[1],"off")) {
      ponder=0;
      Print(4095,"pondering disabled.\n");
    }
    else {
      ponder_move=InputMove(tree,args[1],0,wtm,0,0);
      last_pv.pathd=0;
      last_pv.pathl=0;
    }
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "post/nopost" command sets/resets "show thinking" mode |
|   for xboard compatibility.                              |
|                                                          |
 ----------------------------------------------------------
*/
  else if (!strcmp("post",*args)) {
    post=1;
  }
  else if (!strcmp("nopost",*args)) {
    post=0;
  }
/*
 ----------------------------------------------------------
|                                                          |
|  "random" command is ignored. [xboard compatibility]     |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("random",*args)) {
    return(xboard);
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "rating" is used by xboard to set crafty's rating and  |
|   the opponent's rating, which is used by the learning   |
|   functions.                                             |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("rating",*args)) {
    if (nargs < 3) {
      printf("usage:  rating <crafty> <opponent>\n");
      return(1);
    }
    crafty_rating=atoi(args[1]);
    opponent_rating=atoi(args[2]);
    if (crafty_rating-opponent_rating < 0) draw_score=+20;
    else if (crafty_rating-opponent_rating < 200) draw_score=0;
    else if (crafty_rating-opponent_rating < 400) draw_score=-20;
    else if (crafty_rating-opponent_rating < 600) draw_score=-30;
    else draw_score=-50;
    if (log_file) {
      fprintf(log_file,"Crafty's rating: %d.\n",crafty_rating);
      fprintf(log_file,"opponent's rating: %d.\n",opponent_rating);
    }
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "remove" command backs up the game one whole move,     |
|   leaving the opponent still on move.  it's intended for |
|   xboard compatibility, but works in any mode.           |
|                                                          |
 ----------------------------------------------------------
*/
  else if (!strcmp("remove",*args)) {
    if (thinking || pondering) return(2);
    move_number--;
    sprintf(buffer,"reset %d",move_number);
    (void) Option(tree);
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "reset" restores (backs up) a game to a prior position |
|   with the same side on move.  reset 17 would reset the  |
|   position to what it was at move 17.                    |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("reset",*args)) {
    int i, move, nmoves;

    if (thinking || pondering) return(2);
    if (nargs < 2) {
      printf("usage:  reset <movenumber>\n");
      return(1);
    }
    ponder_move=0;
    last_mate_score=0;
    last_pv.pathd=0;
    last_pv.pathl=0;
    if (thinking || pondering) return(2);
    over=0;
    move_number=atoi(args[1]);
    if (!move_number) {
      move_number=1;
      return(1);
    }
    nmoves=(move_number-1)*2+1-wtm;
    root_wtm=ChangeSide(wtm);
    InitializeChessBoard(&tree->position[0]);
    wtm=1;
    move_number=1;
    for (i=0;i<nmoves;i++) {
      fseek(history_file,i*10,SEEK_SET);
      fscanf(history_file,"%s",buffer);
/*
    If the move is "pass", that means that the side on move passed.
    This includes the case where the game started from a black-to-move
    position; then white's first move is recorded as a pass.
*/
      if(strcmp(buffer,"pass")==0) {
	wtm=ChangeSide(wtm);
	if (wtm) move_number++;
	continue;
      }
      move=InputMove(tree,buffer,0,wtm,0,0);
      if (move) {
        MakeMoveRoot(tree,move,wtm);
      }
      else {
        printf("ERROR!  move %s is illegal\n",buffer);
        break;
      }
      wtm=ChangeSide(wtm);
      if (wtm) move_number++;
      Phase();
    }
    moves_out_of_book=0;
    tc_moves_remaining=tc_moves-move_number+1;
    while (tc_moves_remaining <= 0) tc_moves_remaining+=tc_secondary_moves;
    printf("NOTICE: %d moves to next time control\n",tc_moves_remaining);
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "read" reads game moves in and makes them.  this can   |
|   be used in two ways:  (1) type "read" and then start   |
|   entering moves;  type "exit" when done;  (2) type      |
|   "read <filename>" to read moves in from <filename>.    |
|   note that read will attempt to skip over "non-move"    |
|   text and try to extract moves if it can.               |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("read",*args) || OptionMatch("reada",*args)) {
    int append, move, readstat;
    FILE *read_input=0;

    if (thinking || pondering) return(2);
    nargs=ReadParse(buffer,args," 	=");
    if (!strcmp("reada",*args))
      append=1;
    else
      append=0;
    ponder_move=0;
    last_pv.pathd=0;
    last_pv.pathl=0;
    if (nargs > 1) {
      if (!(read_input=fopen(args[1],"r"))) {
        printf("file %s does not exist.\n",args[1]);
        return(1);
      }
    }
    else {
      printf("type \"exit\" to terminate.\n");
      read_input=stdin;
    }
    if (!append) {
      InitializeChessBoard(&tree->position[0]);
      wtm=1;
      move_number=1;
    }
/*
   step 1:  read in the PGN tags.
*/
    readstat=ReadPGN(0,0);
    do {
      if (read_input == stdin) {
        if (wtm)
          printf("read.White(%d): ",move_number);
        else
          printf("read.Black(%d): ",move_number);
        fflush(stdout);
      }
      readstat=ReadPGN(read_input,0);
    } while (readstat==1);
    if (readstat < 0) return(1);
/*
   step 2:  read in the moves.
*/
    do {
      move=0;
      move=ReadNextMove(tree,buffer,0,wtm);
      if (move) {
        if (read_input != stdin) {
          printf("%s ",OutputMove(tree,move,0,wtm));
          if (!(move_number % 8) && ChangeSide(wtm)) printf("\n");
        }
        fseek(history_file,((move_number-1)*2+1-wtm)*10,SEEK_SET);
        fprintf(history_file,"%9s\n",OutputMove(tree,move,0,wtm));
        MakeMoveRoot(tree,move,wtm);
#if defined(DEBUG)
        ValidatePosition(tree,1,move,"Option()");
#endif
      }
      else if (!read_input) printf("illegal move.\n");
      if (move) {
        wtm=ChangeSide(wtm);
        Phase();
        if (wtm) move_number++;
      }
      if (read_input == stdin) {
        if (wtm)
          printf("read.White(%d): ",move_number);
        else
          printf("read.Black(%d): ",move_number);
        fflush(stdout);
      }
      readstat=ReadPGN(read_input,0);
      if (readstat < 0) break;
      if (!strcmp(buffer,"exit")) break;
    } while (1);
    moves_out_of_book=0;
    root_wtm=!wtm;
    if (read_input != stdin) {
      printf("\n");
      fclose(read_input);
    }
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "resign" command sets the resignation threshold to     |
|   the number of pawns the program must be behind before  |
|   resigning (0 -> disable resignations).  resign with no |
|   arguments will mark the pgn result as lost by the      |
|   opponent.                                              |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("resign",*args)) {
    if (nargs < 2) {
      if (crafty_is_white) {
        Print(4095,"result 1-0\n");
        strcpy(pgn_result,"1-0");
      }
      else {
        Print(4095,"result 0-1\n");
        strcpy(pgn_result,"0-1");
      }
      return(1);
    }
    resign=atoi(args[1]);
    if (resign)
      Print(128,"threshold set to %d pawns.\n",resign);
    else
      Print(128,"disabled resignations.\n");
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "result" command comes from xboard/winboard and gives  |
|   the result of the current game.  if learning routines  |
|   have not yet been activated, this will do it.          |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("result",*args)) {
    if (nargs > 1) {
      if (!strcmp(args[1],"1-0")) {
        strcpy(pgn_result,"1-0");
        if (!crafty_is_white) LearnResult(tree,crafty_is_white);
      }
      else if (!strcmp(args[1],"0-1")) {
        strcpy(pgn_result,"0-1");
        if (crafty_is_white) LearnResult(tree,crafty_is_white);
      }
      else if (!strcmp(args[1],"1/2-1/2")) {
        strcpy(pgn_result,"1/2-1/2");
      }
      return(1);
    }
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "savegame" command saves the game in a file in PGN     |
|   format.  command has an optional filename.  note that  |
|   SR is an auto232alias that behaves slightly            |
|   differently.                                           |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("savegame",*args) ||
           OptionMatch("SR",*args)) {
    struct tm *timestruct;
    int i, secs, more, swtm;
    FILE *output_file;
    char input[128], text[128], *next;

    output_file=stdout;
    secs=time(0);
    timestruct=localtime((time_t*) &secs);
    if (nargs > 1) {
      if (!(output_file=fopen(args[1],"w"))) {
        printf("unable to open %s for write.\n",args[1]);
        return(1);
      }
    }
    fprintf(output_file,"[Event \"%s\"]\n",pgn_event);
    fprintf(output_file,"[Site \"%s\"]\n",pgn_site);
    fprintf(output_file,"[Date \"%4d.%02d.%02d\"]\n",timestruct->tm_year+1900,
            timestruct->tm_mon+1,timestruct->tm_mday);
    fprintf(output_file,"[Round \"%s\"]\n",pgn_round);
    fprintf(output_file,"[White \"%s\"]\n",pgn_white);
    fprintf(output_file,"[WhiteElo \"%s\"]\n",pgn_white_elo);
    fprintf(output_file,"[Black \"%s\"]\n",pgn_black);
    fprintf(output_file,"[BlackElo \"%s\"]\n",pgn_black_elo);
    fprintf(output_file,"[Result \"%s\"]\n",pgn_result);
    /* Handle setup positions and initial pass by white */
    swtm=1;
    if (move_number>1 || !wtm) {
      fseek(history_file,0,SEEK_SET);
      if (fscanf(history_file,"%s",input)==1 && strcmp(input,"pass")==0) swtm=0;
    }
    if (initial_position[0])
      fprintf(output_file,"[FEN \"%s\"]\n[SetUp \"1\"]\n",initial_position);
    else if (!swtm) {
      fprintf(output_file,
        "[FEN \"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR b KQkq - 0 1\"\n"
	"[SetUp \"1\"]\n");
    }
    fprintf(output_file,"\n");
    next=text;
    if (!swtm) {
      strcpy(next,"1... ");
      next=text+strlen(text);
    }
/* Output the moves */
    more=0;
    for (i=(swtm?0:1);i<(move_number-1)*2-wtm+1;i++) {
      fseek(history_file,i*10,SEEK_SET);
      fscanf(history_file,"%s",input);
      if (!(i%2)) {
        sprintf(next,"%d. ",i/2+1);
        next=text+strlen(text);
      }
      sprintf(next,"%s ",input);
      next=text+strlen(text);
      more=1;
      if (next-text >= 60) {
        fprintf(output_file,"%s\n",text);
        more=0;
        next=text;
      }
    }
    if (more)
      fprintf(output_file,"%s",text);
    fprintf(output_file,"%s\n",pgn_result);
    if (output_file != stdout) fclose(output_file);
    printf("PGN save complete.\n");
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "savepos" command saves the current position in a FEN  |
|   (Forsythe notation) string that can be later used to   |
|   recreate this exact position.                          |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("savepos",*args)) {
    int rank, file, nempty;
    FILE *output_file;
    output_file=stdout;
    if (nargs > 1) {
      if (!strcmp(args[1],"*")) {
        output_file=0;
        strcpy(initial_position,"");
      }
      else if (!(output_file=fopen(args[1],"w"))) {
        printf("unable to open %s for write.\n",args[1]);
        return(1);
      }
    }
    if (output_file) fprintf(output_file,"setboard ");
    for (rank=RANK8;rank>=RANK1;rank--) {
      nempty=0;
      for (file=FILEA;file<=FILEH;file++) {
        if (PieceOnSquare((rank<<3)+file)) {
          if (nempty) {
            if (output_file)
              fprintf(output_file,"%c",empty[nempty]);
            else
              sprintf(initial_position+strlen(initial_position),"%c",
                      empty[nempty]);
            nempty=0;
          }
          if (output_file)
            fprintf(output_file,"%c",xlate[PieceOnSquare((rank<<3)+file)+7]);
            else
              sprintf(initial_position+strlen(initial_position),"%c",
                      xlate[PieceOnSquare((rank<<3)+file)+7]);
        }
        else nempty++;
      }
      if (output_file)
        fprintf(output_file,"/");
      else
        sprintf(initial_position+strlen(initial_position),"%c",'/');
    }
    if (output_file)
      fprintf(output_file," %c ",(wtm)?'w':'b');
    else
      sprintf(initial_position+strlen(initial_position)," %c ", (wtm)?'w':'b');
    if (WhiteCastle(0) & 1) {
      if (output_file)
        fprintf(output_file,"K");
      else
        sprintf(initial_position+strlen(initial_position),"%c",'K');
    }
    if (WhiteCastle(0) & 2) {
      if (output_file)
        fprintf(output_file,"Q");
      else
        sprintf(initial_position+strlen(initial_position),"%c",'Q');
    }
    if (BlackCastle(0) & 1) {
      if (output_file)
        fprintf(output_file,"k");
      else
        sprintf(initial_position+strlen(initial_position),"%c",'k');
    }
    if (BlackCastle(0) & 2) {
      if (output_file)
        fprintf(output_file,"q");
      else
        sprintf(initial_position+strlen(initial_position),"%c",'q');
    }
    if (EnPassant(0)) {
      if (output_file)
        fprintf(output_file," %c%c",File(EnPassant(0))+'a',
                              Rank(EnPassant(0))+'1');
      else
        sprintf(initial_position+strlen(initial_position), "%c%c",
                File(EnPassant(0))+'a',
                Rank(EnPassant(0))+'1');
    }
    if (output_file) fprintf(output_file,"\n");
    if (output_file && output_file != stdout) {
      fprintf(output_file,"exit\n");
      fclose(output_file);
    }
    if (output_file) printf("FEN save complete.\n");
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "search" command sets a specific move for the search   |
|   to analyze, ignoring all others completely.            |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("search",*args)) {
    if (thinking || pondering) return(2);
    if (nargs < 2) {
      printf("usage:  search <move>\n");
      return(1);
    }
    search_move=InputMove(tree,args[1],0,wtm,0,0);
    if (!search_move) search_move=InputMove(tree,args[1],0,ChangeSide(wtm),0,0);
    if (!search_move) printf("illegal move.\n");
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "settc" command is used to reset the time controls     |
|   after a complete restart.                              |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("settc",*args)) {
    if (thinking || pondering) return(2);
    if (nargs < 4) {
      printf("usage:  settc <moves> <ctime> <otime>\n");
      return(1);
    }
    tc_moves_remaining=atoi(args[1]);
    tc_time_remaining=ParseTime(args[2])*6000;
    tc_time_remaining_opponent=ParseTime(args[3])*6000;
    Print(4095,"time remaining: %s (crafty).\n",
            DisplayTime(tc_time_remaining));
    Print(4095,"time remaining: %s (opponent).\n",
            DisplayTime(tc_time_remaining_opponent));
    Print(4095,"%d moves to next time control (Crafty)\n",
          tc_moves_remaining);
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "setboard" command sets the board to a specific        |
|   position for analysis by the program.                  |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("setboard",*args)) {
    if (thinking || pondering) return(2);
    nargs=ReadParse(buffer,args," 	;=");
    SetBoard(nargs-1,args+1,0);
    move_number=1;
    if (!wtm) {
      wtm=1;
      Pass();
    }
    ponder_move=0;
    last_pv.pathd=0;
    last_pv.pathl=0;
    over=0;
    strcpy(buffer,"savepos *");
    (void) Option(tree);
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "score" command displays static evaluation of the      |
|   current board position.                                |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("score",*args)) {
    int s1, s2=0, s3=0, s4=0, s5=0, s6=0, s7;

    if (thinking || pondering) return(2);
    root_wtm=ChangeSide(wtm);
    tree->position[1]=tree->position[0];
    PreEvaluate(tree,wtm);
    s7=Evaluate(tree,1,1,-99999,99999);
    s1=Material;
    if (opening) s2=EvaluateDevelopment(tree,1);
    if (TotalWhitePawns+TotalBlackPawns) {
      s3=EvaluatePawns(tree);
      s4=EvaluatePassedPawns(tree);
      s5=EvaluatePassedPawnRaces(tree,wtm);
    }
    s6=EvaluateKingSafety(tree,0);
    Print(128,"note: scores are for the white side\n");
    Print(128,"material evaluation.................%s\n",
      DisplayEvaluation(s1));
    Print(128,"development.........................%s\n",
      DisplayEvaluation(s2));
    Print(128,"pawn evaluation.....................%s\n",
      DisplayEvaluation(s3));
    Print(128,"passed pawn evaluation..............%s\n",
      DisplayEvaluation(s4));
    Print(128,"passed pawn race evaluation.........%s\n",
      DisplayEvaluation(s5));
    Print(128,"king safety evaluation..............%s\n",
      DisplayEvaluation(s6));
    Print(128,"interactive piece evaluation........%s\n",
      DisplayEvaluation(s7-s1-s2-s3-s4-s5-s6));
    Print(128,"total evaluation....................%s\n",
      DisplayEvaluation(s7));
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "sd" command sets a specific search depth to control   |
|   the tree search depth.                                 |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("sd",*args)) {
    if (nargs < 2) {
      printf("usage:  sd <depth>\n");
      return(1);
    }
    search_depth=atoi(args[1]);
    Print(4095,"search depth set to %d.\n",search_depth);
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "show" command enables/disables various display        |
|   such as "show extensions" which adds a character to    |
|   each pv move showing if/why it was extended.           |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("show",*args)) {
    if (nargs < 2) {
      printf("usage:  show book\n");
      return(1);
    }
    if (OptionMatch("book",args[1])) {
      show_book=!show_book;
      if (show_book) Print(4095,"show book statistics\n");
      else Print(4095,"don't show book statistics\n");
    }
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "sn" command sets a specific number of nodes to search |
|   before stopping.                                       |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("sn",*args)) {
    if (nargs < 2) {
      printf("usage:  sn <nodes>\n");
      return(1);
    }
    search_nodes=atoi(args[1]);
    Print(4095,"search nodes set to %d.\n",search_nodes);
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "SP" command does nothing, except force main() to      |
|   start a search.  [auto232 compatibility]               |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("SP",*args)) {
    if (thinking) return(2);
    return(-1);
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "st" command sets a specific search time to control    |
|   the tree search time.                                  |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("st",*args)) {
    int fract;
    if (nargs < 2) {
      printf("usage:  st <time>\n");
      return(1);
    }
    search_time_limit=atoi(args[1])*100;
    if (strchr(args[1],'.')) {
      fract=atoi(strchr(args[1],'.')+1);
      if (fract<10 && *(strchr(args[1],'.')+1) != '0') fract*=10;
      search_time_limit+=fract;
    }
    Print(4095,"search time set to %.2f.\n",(float)search_time_limit/100.0);
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "surplus" command sets a specific time surplus target  |
|   for normal tournament games.                           |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("surplus",*args)) {
    if (nargs == 2) tc_safety_margin=atoi(args[1])*6000;
    Print(4095,"time surplus set to %s.\n",DisplayTime(tc_safety_margin));
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "swindle" command turns swindle mode off/on.           |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("swindle",*args)) {
    if (!strcmp(args[1],"on")) swindle_mode=1;
    else if (!strcmp(args[1],"off")) swindle_mode=0;
    else printf("usage:  swindle on|off\n");
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "tags" command lists the current PGN header tags.      |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("tags",*args)) {
    struct tm *timestruct;
    int secs;
    secs=time(0);
    timestruct=localtime((time_t*) &secs);
    printf("[Event \"%s\"]\n",pgn_event);
    printf("[Site \"%s\"]\n",pgn_site);
    printf("[Date \"%4d.%02d.%02d\"]\n",timestruct->tm_year+1900,
            timestruct->tm_mon+1,timestruct->tm_mday);
    printf("[Round \"%s\"]\n",pgn_round);
    printf("[White \"%s\"]\n",pgn_white);
    printf("[WhiteElo \"%s\"]\n",pgn_white_elo);
    printf("[Black \"%s\"]\n",pgn_black);
    printf("[BlackElo \"%s\"]\n",pgn_black_elo);
    printf("[Result \"%s\"]\n",pgn_result);
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "test" command runs a test suite of problems and       |
|   prints results.                                        |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("test",*args)) {
    nargs=ReadParse(buffer,args,"	 ;=");
    if (thinking || pondering) return(2);
    if (nargs < 2) {
      printf("usage:  test <filename> [exitcnt]\n");
      return(1);
    }
    if (nargs > 2) early_exit=atoi(args[2]);
    Test(args[1]);
    ponder_move=0;
    last_pv.pathd=0;
    last_pv.pathl=0;
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "time" controls whether the program uses cpu time or   |
|   wall-clock time for timing.  for tournament play,      |
|   it is safer to use wall-clock timing, for testing it   |
|   may be more consistent to use cpu timing if the        |
|   machine is used for other things concurrently with the |
|   tests being run.                                       |
|                                                          |
|   "time" is also used to set the basic search timing     |
|   controls.  the general form of the command is as       |
|   follows:                                               |
|                                                          |
|     time nmoves/ntime/[nmoves/ntime]/[increment]         |
|                                                          |
|   nmoves/ntime represents a traditional first time       |
|   control when nmoves is an integer representing the     |
|   number of moves and ntime is the total time allowed    |
|   for these moves.  the [optional] nmoves/ntime is a     |
|   traditional secondary time control.  increment is a    |
|   feature related to ics play and emulates the fischer   |
|   clock where "increment" is added to the time left      |
|   after each move is made.                               |
|                                                          |
|   as an alternative, nmoves can be "sd" which represents |
|   a "sudden death" time control of the remainder of the  |
|   game played in ntime.  the optional secondary time     |
|   control can be a sudden-death time control, as in the  |
|   following example:                                     |
|                                                          |
|     time 60/30/sd/30                                     |
|                                                          |
|   this sets 60 moves in 30 minutes, then game in 30      |
|   additional minutes.  an increment can be added if      |
|   desired.                                               |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("time",*args)) {
    if (ics || xboard) {
      tc_time_remaining=atoi(args[1]);
      if (log_file && time_limit>99)
        fprintf(log_file,"time remaining: %s (crafty).\n",
                DisplayTime(tc_time_remaining));
    }
    else {
      if (thinking || pondering) return(2);
      if (nargs == 2) {
        if (!strcmp("cpu",args[1])) {
          time_type=cpu;
          Print(4095,"using cpu time\n");
        }
        else if (!strcmp("elapsed",args[1])) {
          time_type=elapsed;
          Print(4095,"using elapsed time\n");
        }
        else printf("usage:  time cpu|elapsed|<controls>\n");
      }
      else {
        tc_moves=60;
        tc_time=180000;
        tc_moves_remaining=60;
        tc_time_remaining=180000;
        tc_time_remaining_opponent=180000;
        tc_secondary_moves=60;
        tc_secondary_time=180000;
        tc_increment=0;
        tc_sudden_death=0;
/*
  first let's pick off the basic time control (moves/minutes)
*/
        if (nargs > 1)
          if (!strcmp(args[1],"sd")) {
            tc_sudden_death=1;
            tc_moves=1000;
          }
        if (nargs > 2) {
          tc_moves=atoi(args[1]);
          tc_time=atoi(args[2])*100;
        }
/*
  now let's pick off the secondary time control (moves/minutes)
*/
        tc_secondary_time=tc_time;
        tc_secondary_moves=tc_moves;
        if (nargs > 3)
          if (!strcmp(args[3],"sd")) {
            tc_sudden_death=2;
            tc_secondary_moves=1000;
          }
        if (nargs > 4) {
          tc_secondary_moves=atoi(args[3]);
          tc_secondary_time=atoi(args[4])*100;
        }
        if (nargs > 5)
          tc_increment=atoi(args[5])*100;
        tc_time_remaining=tc_time;
        tc_time_remaining_opponent=tc_time;
        tc_moves_remaining=tc_moves;
        if (!tc_sudden_death) {
          Print(4095,"%d moves/%d minutes primary time control\n",
                tc_moves, tc_time/100);
          Print(4095,"%d moves/%d minutes secondary time control\n",
                tc_secondary_moves, tc_secondary_time/100);
          if (tc_increment) Print(4095,"increment %d seconds.\n",tc_increment/100);
        }
        else if (tc_sudden_death == 1) {
          Print(4095," game/%d minutes primary time control\n",
                tc_time/100);
          if (tc_increment) Print(4095,"increment %d seconds.\n",tc_increment/100);
        }
        else if (tc_sudden_death == 2) {
          Print(4095,"%d moves/%d minutes primary time control\n",
                tc_moves, tc_time/100);
          Print(4095,"game/%d minutes secondary time control\n",
                tc_secondary_time/100);
          if (tc_increment) Print(4095,"increment %d seconds.\n",tc_increment/100);
        }
        tc_time*=60;
        tc_time_remaining*=60;
        tc_time_remaining_opponent*=60;
        tc_secondary_time*=60;
        tc_safety_margin=tc_time/6;
      }
    }
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "timeleft" command comes from the custom ICS interface |
|   and indicates how much time is left for white and for  |
|   black.                                                 |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("timeleft",*args)) {
    if (nargs < 3) {
      printf("usage:  timeleft <wtime> <btime>\n");
      return(1);
    }
    if (crafty_is_white) {
      tc_time_remaining=atoi(args[1]);
      tc_time_remaining_opponent=atoi(args[2]);
    }
    else {
      tc_time_remaining_opponent=atoi(args[1]);
      tc_time_remaining=atoi(args[2]);
    }
    if (log_file) {
      fprintf(log_file,"time remaining: %s (crafty).\n",
                       DisplayTime(tc_time_remaining));
      fprintf(log_file,"time remaining: %s (opponent).\n",
                       DisplayTime(tc_time_remaining_opponent));
    }
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "trace" command sets the search trace level which will |
|   dump the tree as it is searched.                       |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("trace",*args)) {
#if !defined(TRACE)
    printf("Sorry, but I can't display traces unless compiled with -DTRACE\n");
#endif
    if (nargs < 2) {
      printf("usage:  trace <depth>\n");
      return(1);
    }
    trace_level=atoi(args[1]);
    printf("trace=%d\n",trace_level);
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "tt" command is used to test time control logic.       |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("tt",*args)) {
    int time_used;
    do {
      TimeSet(think);
      printf("time used? ");
      fflush(stdout);
      fgets(buffer,128,stdin);
      if (strlen(buffer)) time_used=atoi(buffer);
      else time_used=time_limit;
      TimeAdjust(time_used,opponent);
      TimeAdjust(time_used,crafty);
      sprintf(buffer,"clock");
      Option(tree);
      move_number++;
    } while (time_used >= 0);
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "undo" command backs up 1/2 move, which leaves the     |
|   opposite side on move. [xboard compatibility]          |
|                                                          |
 ----------------------------------------------------------
*/
  else if (!strcmp("undo",*args)) {
    if (thinking || pondering) return(2);
    wtm=ChangeSide(wtm);
    if (ChangeSide(wtm)) move_number--;
    sprintf(buffer,"reset %d",move_number);
    (void) Option(tree);
  }
/*
 ---------------------------------------------------------------
|                                                               |
|   "usage" command controls the time usage multiple factors    |
|  used in the game  - percntage increase or decrease in time   |
|  used up front.  Enter a number between 1 to 100 for the      |
|  % decrease to increase - although other time limitations     !
| controls may kick in.  negatives work as well, may be used    |
| in crafty.rc                                                  |
|                                                               |
 ---------------------------------------------------------------
*/
 else if (OptionMatch("usage",*args)) {
    if (nargs < 2) {
      printf("usage:  usage <percentage>\n");
      return(1);
    }
    usage_level=atoi(args[1]);
    if (usage_level > 50) usage_level=50;
    else if (usage_level < -50) usage_level=-50;
    Print(4095,"time usage up front set to %d percent increase/(-)decrease.\n",
          usage_level);
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "whisper" command sets whisper mode for ICS.  =1 will  |
|   whisper mate announcements, =2 will whisper scores and |
|   other info, =3 will whisper scores and PV, =4 adds the |
|   list of book moves, =5 displays the PV after each      |
|   iteration completes, and =6 displays the PV each time  |
|   it changes in an iteration.                            |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("whisper",*args)) {
    if (nargs < 2) {
      printf("usage:  whisper <level>\n");
      return(1);
    }
    whisper=atoi(args[1]);
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "wild" command sets up an ICS wild position (only 7 at |
|   present, but any can be added easily, except for those |
|   that Crafty simply can't play (two kings, invisible    |
|   pieces, etc.)                                          |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("wild",*args)) {
    int i;

    if (nargs < 2) {
      printf("usage:  wild <value>\n");
      return(1);
    }
    i=atoi(args[1]);
    switch (i) {
    case 7:
      strcpy(buffer,"setboard 4k/5ppp/////PPP/3K/ w");
      (void) Option(tree);
      break;
    default:
      printf("sorry, only wild7 implemented at present\n");
      break;
    }
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "white" command sets white to move (wtm).              |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("white",*args)) {
    if (thinking || pondering) return(2);
    ponder_move=0;
    last_pv.pathd=0;
    last_pv.pathl=0;
    if (!wtm) Pass();
    force=0;
  }
/*
 ----------------------------------------------------------
|                                                          |
|  "xboard" command is normally invoked from main() via    |
|  the xboard command-line option.  it sets proper         |
|  defaults for ics/Xboard interface requirements.         |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("xboard",*args) || OptionMatch("winboard",*args)) {
    if (!xboard) {
      signal(SIGINT,SIG_IGN);
      xboard=1;
      display_options&=4095-1-2-4-8-16-32-128;
      ansi=0;
      printf("\n");
      printf("kibitz Hello from Crafty v%s! (%d cpus)\n",version,CPUS);
      printf("tellics set 1 Crafty v%s (%d cpus)\n",version,CPUS);
      fflush(stdout);
    }
  }
/*
 ----------------------------------------------------------
|                                                          |
|  "?" command does nothing, but since this is the "move   |
|  now" keystroke, if crafty is not searching, this will   |
|  simply "wave it off" rather than produce an error.      |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("?",*args)) {
  }
/*
 ----------------------------------------------------------
|                                                          |
|   unknown command, it must be a move.                    |
|                                                          |
 ----------------------------------------------------------
*/
  else
    return(0);
/*
 ----------------------------------------------------------
|                                                          |
|   command executed, return for another.                  |
|                                                          |
 ----------------------------------------------------------
*/
    return(1);
}

/*
********************************************************************************
*                                                                              *
*   OptionMatch() is used to recognize user commands.  it requires that the    *
*   command (text input which is the *2nd parameter* conform to the following  *
*   simple rules:                                                              *
*                                                                              *
*     1.  the input must match the command, starting at the left-most          *
*         character.                                                           *
*     2.  if the command starts with a sequence of characters that could       *
*         be interpreted as a chess move as well (re for reset and/or rook     *
*         to the e-file) then the input must match enough of the command       *
*         to get past the ambiguity (res would be minimum we will accept       *
*         for the reset command.)                                              *
*                                                                              *
********************************************************************************
*/
int OptionMatch(char *command, char *input) {
/*
 ----------------------------------------------------------
|                                                          |
|   check for the obvious exact match first.               |
|                                                          |
 ----------------------------------------------------------
*/
  if (!strcmp(command,input)) return(1);
/*
 ----------------------------------------------------------
|                                                          |
|   now use strstr() to see if "input" is in "command."    |
|   the first requirement is that input matches command    |
|   starting at the very left-most character;              |
|                                                          |
 ----------------------------------------------------------
*/
  if (strstr(command,input) == command) return(1);
  return(0);
}

void OptionPerft(TREE *tree, int ply,int depth,int wtm) {
  int i, *mv;

  tree->last[ply]=GenerateCaptures(tree, ply, wtm, tree->last[ply-1]);
  tree->last[ply]=GenerateNonCaptures(tree, ply, wtm, tree->last[ply]);
  for (mv=tree->last[ply-1];mv<tree->last[ply];mv++) {
    MakeMove(tree,ply,*mv,wtm);
    if (!Check(wtm)) {
      if (ply <= trace_level) {
        for (i=1;i<ply;i++) printf("  ");
        printf("%s\n", OutputMove(tree,*mv,ply,wtm));
      }
      if (depth-1) OptionPerft(tree,ply+1,depth-1,ChangeSide(wtm));
      else total_moves++;
    }
    UnMakeMove(tree,ply,*mv,wtm);
  }
}
