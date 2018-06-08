#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "types.h"
#include "function.h"
#include "data.h"
#if defined(UNIX) || defined(AMIGA)
#  include <unistd.h>
#endif
#include "epdglue.h"

/* last modified 09/27/96 */
/*
********************************************************************************
*                                                                              *
*   Option() is used to handle user input necessary to control/customize the   *
*   program.  it performs all functions excepting chess move input which is    *
*   handled by main().                                                         *
*                                                                              *
********************************************************************************
*/
int weak_w, weak_b;
int Option(char *tinput)
{
  FILE *input_file, *output_file;
  int back_number, move, *mv;
  int i, j, mn, tmn, nmoves;
  int s1, s2, s3, s4, s5, s6, s7;
  int more, new_hash_size, error;
  char filename[64], title[64], input[100], text[100], onoff[10];
  char log_filename[64], history_filename[64];
  char nextc, *next, *next1;
  char *equal, *slash;
  int scanf_status, append;
  int clock_before, clock_after;
  BITBOARD target;
  float time_used;
  int eg_parmcount, eg_parmindex;
  char eg_commbufv[256], eg_parmbufv[256];

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
  if (EGCommandCheck(tinput)) {
    if (thinking || pondering) return (2);
    else {
      strcpy(eg_commbufv, tinput);
      eg_parmcount = EGCommandParmCount(tinput);
      for (eg_parmindex = 1; eg_parmindex < eg_parmcount; eg_parmindex++) {
        fscanf(input_stream, "%s", eg_parmbufv);
        strcat(eg_commbufv, " ");
        strcat(eg_commbufv, eg_parmbufv);
      }
      (void) EGCommand(eg_commbufv);
      return (1);
    }
  }
/*
 ----------------------------------------------------------
|                                                          |
|   first, see if the command has a "=" imbedded in it.    |
|   if so, replace it with a '\0' to make the command      |
|   comparisons work.  set the flag "equal" so that each   |
|   command will know that it's parameter is already in    |
|   the input string (following the \0).                   |
|                                                          |
 ----------------------------------------------------------
*/
  strcpy(input,tinput);
  if ((equal=strchr(input,'='))) *equal=0;
/*
 ----------------------------------------------------------
|                                                          |
|   now, see if the command has a "/" imbedded in it.      |
|   if so, replace it with a '\0' to make the command      |
|   comparisons work.  set the flag "slash" so that each   |
|   command will know that it's parameter is already in    |
|   the input string (following the \0).                   |
|                                                          |
 ----------------------------------------------------------
*/
  if ((slash=strchr(input,'/'))) *slash=0;
/*
 ----------------------------------------------------------
|                                                          |
|   "alarm" command turns audible move warning on/off.     |
|                                                          |
 ----------------------------------------------------------
*/
  if (OptionMatch("alarm",input)) {
    OptionGet(&equal,&slash,onoff,&more);
    if (!strcmp(onoff,"on")) audible_alarm=0x07;
    else if (!strcmp(onoff,"off")) audible_alarm=0x00;
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
  else if (OptionMatch("analyze",input)) {
    int temp_draw_score_is_zero;
    if (thinking || pondering) return(2);
    temp_draw_score_is_zero=draw_score_is_zero;
    draw_score_is_zero=1;
    ponder_completed=0;
    ponder_move=0;
    analyze_mode=1;
    if (!xboard && (verbosity_level < 5)) verbosity_level=5;
    printf("Analyze Mode: type \"exit\" to terminate.\n");
    do {
      do {
        last_pv.path_iteration_depth=0;
        last_pv.path_length=0;
        analyze_move_read=0;
        pondering=1;
        position[1]=position[0];
        (void) Iterate(wtm,think);
        pondering=0;
        if (!xboard) {
          if (wtm) printf("analyze.White(%d): ",move_number);
          else printf("analyze.Black(%d): ",move_number);
        }
        if (!analyze_move_read) do {
          scanf_status=scanf("%s",tinput);
          if (strstr(tinput,"timeleft") && !xboard) {
            if (wtm) printf("analyze.White(%d): ",move_number);
            else printf("analyze.Black(%d): ",move_number);
          }
        } while (strstr(tinput,"timeleft"));
        else scanf_status=1;
        if (scanf_status <= 0) break;
        move=0;
        if (!strcmp(tinput,"exit")) break;
        if (xboard) move=InputMoveICS(tinput,0,wtm,0,0);
        else move=InputMove(tinput,0,wtm,0,0);
        if (move) {
          fseek(history_file,((move_number-1)*2+1-wtm)*10,SEEK_SET);
          fprintf(history_file,"%10s",OutputMove(&move,0,wtm));
          if (wtm) Print(1,"White(%d): ",move_number);
            else Print(1,"Black(%d): ",move_number);
          Print(1,"%s\n",OutputMove(&move,0,wtm));
          MakeMoveRoot(move,wtm);
        }
        else if (OptionMatch("back",tinput)) {
          nextc=getc(input_stream);
          if (nextc == ' ') scanf("%d",&back_number);
          else back_number=1;
          for (i=0;i<back_number;i++) {
            wtm=ChangeSide(wtm);
            if (ChangeSide(wtm)) move_number--;
          }
          sprintf(tinput,"reset=%d",move_number);
          Option(tinput);
        }
        else {
          pondering=0;
          if (Option(tinput) == 0) printf("illegal move.\n");
          pondering=1;
        }
      } while (!move && (scanf_status>0));
      if (!strcmp(tinput,"exit")) break;
      if (scanf_status <= 0) break;
      wtm=ChangeSide(wtm);
      if (wtm) move_number++;
    } while (scanf_status > 0);
    analyze_mode=0;
    printf("analyze complete.\n");
    draw_score_is_zero=temp_draw_score_is_zero;
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
  else if (OptionMatch("annotate",input)) {
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
  else if (OptionMatch("ansi",input)) {
    OptionGet(&equal,&slash,onoff,&more);
    if (!strcmp(onoff,"on")) ansi=1;
    else if (!strcmp(onoff,"off")) ansi=0;
    else printf("usage:  ansi on|off\n");
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "autoplay" enables autoplay mode.  note that there is  |
|   another alias "DR" that may or may not be necessary.   |
|   if not needed, it will be "canned".                    |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("autoplay",input) || OptionMatch("DR",input)) {
    if (auto_file) {
      fclose(auto_file);
      auto_file = NULL;
      autoplay = 0;
      printf("autoplay disabled\n");
    }
    else {
      auto_file=fopen("PRN", "w");
      autoplay=1;
      printf("autoplay enabled\n");
    }
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "black" command sets black to move (ChangeSide(wtm)).             |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("black",input)) {
    if (thinking || pondering) return (2);
    ponder_completed=0;
    ponder_move=0;
    last_pv.path_iteration_depth=0;
    last_pv.path_length=0;
    wtm=0;
    force=0;
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "book" command updates/creates the opening book file.  |
|                                                          |
 ----------------------------------------------------------
*/
  else if (!strcmp("book",input)) BookUp("book.bin");
  else if (!strcmp("books",input)) BookUp("books.bin");
/*
 ----------------------------------------------------------
|                                                          |
|   "clock" command displays chess clock.                  |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("clock",input)) {
    nextc=getc(input_stream);
    if (nextc == ' ') {
      fscanf(input_stream,"%s",text);
      tc_time_remaining=ParseTime(text)*6000;
      fscanf(input_stream,"%s",text);
      tc_time_remaining_opponent=ParseTime(text)*6000;
    }
    Print(0,"time remaining %s (Crafty)",
          DisplayHHMM(tc_time_remaining));
    Print(0,"  %s (opponent).\n",
          DisplayHHMM(tc_time_remaining_opponent));
    Print(0,"%d moves to next time control (Crafty)\n",
          tc_moves_remaining);
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "display" command displays the chess board.            |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("display",input)) {
    nextc=getc(input_stream);
    if (nextc == ' ') {
      if (thinking || pondering) return (2);
      fscanf(input_stream,"%s",text);
      position[1]=position[0];
      PreEvaluate(wtm);
      if (OptionMatch("pawn",text)) {
        DisplayPieceBoards(pawn_value_w,pawn_value_b);
        s7=Evaluate(1,1,-99999,99999);
        printf(" -----------------weak-----------------");
        printf("      -----------------weak-----------------\n");
        for (i=128;i;i=i>>1) printf("%4d ",(i&weak_w)>0);
        printf("    ");
        for (i=128;i;i=i>>1) printf("%4d ",(i&weak_b)>0);
        printf("\n");
      }
      if (OptionMatch("knight",text))
        DisplayPieceBoards(knight_value_w,knight_value_b);
      if (OptionMatch("bishop",text))
        DisplayPieceBoards(bishop_value_w,bishop_value_b);
      if (OptionMatch("rook",text))
        DisplayPieceBoards(rook_value_w,rook_value_b);
      if (OptionMatch("queen",text))
        DisplayPieceBoards(queen_value_w,queen_value_b);
      if (OptionMatch("king",text))
        DisplayPieceBoards(king_value_w,king_value_b);
    }
    else DisplayChessBoard(stdout,display);
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "depth" command sets a specific search depth to        |
|   control the tree search depth. [xboard compatibility]. |
|                                                          |
 ----------------------------------------------------------
*/
  else if (!strcmp("depth",input)) {
    OptionGet(&equal,&slash,text,&more);
    search_depth=atoi(text);
    Print(0,"search depth set to %d.\n",search_depth);
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "draw" sets the default draw score (which is forced to |
|     zero when the endgame is reached.)                   |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("draw",input)) {
    OptionGet(&equal,&slash,text,&more);
    default_draw_score=atoi(text);
    printf("draw score set to %7.3f pawns.\n",
           ((float) default_draw_score) / 1000.0);
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "echo" command displays messages from command file.    |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("echo",input)) {
    fgets(text,80,input_stream);
    if (input_stream != stdin) Print(0,"%s\n",text);
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "edit" command modifies the board position.            |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("edit",input) && strcmp(input,"ed")) {
    if (thinking || pondering) return (2);
    Edit();
    ponder_completed=0;
    ponder_move=0;
    last_pv.path_iteration_depth=0;
    last_pv.path_length=0;
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "end" (or "quit") command terminates the program.      |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("end",input) || OptionMatch("quit",input)) {
    if (!xboard && (thinking || pondering)) return (2);
    if (book_file) fclose(book_file);
    if (book_file) fclose(history_file);
    EGTerm();
    Print(0,"execution complete.\n");
    fflush(stdout);
    exit(0);
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "exit" command resets input device to STDIN.           |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("exit",input)) {
    if (analyze_mode) return(0);
    if (input_stream != stdin) fclose(input_stream);
    input_stream=stdin;
    Print(0,"\n");
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "force" command forces the program to make a specific  |
|   move instead of its last chosen move.                  |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("force",input)) {
    if (thinking || pondering) return (2);
    ponder_completed=0;
    ponder_move=0;
    last_pv.path_iteration_depth=0;
    last_pv.path_length=0;
    if (xboard) force=1;
    else {
      tmn=move_number;
      if (thinking || pondering) return(2);
      mn=move_number;
      if (wtm) mn--;
      sprintf(tinput,"reset=%d",mn);
      wtm=ChangeSide(wtm);
      i=Option(tinput);
      nextc=getc(input_stream);
      if (nextc != ' ')
        if ((input_stream == stdin) && !xboard)
          if (wtm) printf("force.White(%d): ",mn);
          else printf("force.Black(%d): ",mn);
      OptionGet(&equal,&slash,text,&more);
      move=0;
      move=InputMove(text,0,wtm,0,0);
      if (move) {
        if (input_stream != stdin) printf("%s\n",OutputMove(&move,0,wtm));
        fseek(history_file,((mn-1)*2+1-wtm)*10,SEEK_SET);
        fprintf(history_file,"%10s",OutputMove(&move,0,wtm));
        MakeMoveRoot(move,wtm);
        last_pv.path_iteration_depth=0;
        last_pv.path_length=0;
      }
      else if (input_stream == stdin) printf("illegal move.\n");
      wtm=ChangeSide(wtm);
      move_number=tmn;
    }
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "history" command displays game history (moves).       |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("history",input)) {
    printf("    white       black\n");
    for (i=0;i<(move_number-1)*2-wtm+1;i++) {
      fseek(history_file,i*10,SEEK_SET);
      fscanf(history_file,"%s",text);
      if (!(i%2)) printf("%3d",i/2+1);
      printf("  %-10s",text);
      if (i%2 == 1) printf("\n");
    }
    if (ChangeSide(wtm))printf("  ...\n");
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
  else if (OptionMatch("hash",input)) {
    if (thinking || pondering) return(2);
    OptionGet(&equal,&slash,text,&more);
    new_hash_size=atoi(text);
    if (strchr(text,'K') || strchr(text,'k')) new_hash_size*=1<<10;
    if (strchr(text,'M') || strchr(text,'m')) new_hash_size*=1<<20;
    if (new_hash_size < 65536) {
      printf("ERROR.  Minimum hash table size is 64K bytes.\n");
      return(1);
    }
    if (new_hash_size != 0) {
      if (hash_table_size) {
        free(trans_ref_wa);
        free(trans_ref_wb);
        free(trans_ref_ba);
        free(trans_ref_bb);
      }
      new_hash_size/=16*6;
      for (log_hash_table_size=0;log_hash_table_size<8*sizeof(int);log_hash_table_size++)
        if ((1<<(log_hash_table_size+1)) > new_hash_size) break;
      if (log_hash_table_size) {
        hash_table_size=1<<log_hash_table_size;
        trans_ref_wa=malloc(16*(hash_table_size));
        trans_ref_wb=malloc(16*2*(hash_table_size));
        trans_ref_ba=malloc(16*(hash_table_size));
        trans_ref_bb=malloc(16*2*(hash_table_size));
        if (!trans_ref_wa || !trans_ref_wb || !trans_ref_ba || !trans_ref_bb) {
          printf("malloc() failed, not enough memory.\n");
          free(trans_ref_wa);
          free(trans_ref_wb);
          free(trans_ref_ba);
          free(trans_ref_bb);
          hash_table_size=0;
          log_hash_table_size=0;
          trans_ref_wa=0;
          trans_ref_wb=0;
          trans_ref_ba=0;
          trans_ref_bb=0;
        }
        hash_maska=(1<<log_hash_table_size)-1;
        hash_maskb=(1<<(log_hash_table_size+1))-1;
        if (hash_table_size*96 < 1<<20)
          Print(0,"hash table memory = %dK bytes.\n",hash_table_size*96/(1<<10));
        else {
          if (hash_table_size*96%(1<<20))
            Print(0,"hash table memory = %.1fM bytes.\n",(float) hash_table_size*96/(1<<20));
          else
            Print(0,"hash table memory = %dM bytes.\n",hash_table_size*96/(1<<20));
        }
        for (i=0;i<hash_table_size;i++) {
          (trans_ref_wa+i)->word1=0;
          (trans_ref_wa+i)->word2=0;
          (trans_ref_ba+i)->word1=0;
          (trans_ref_ba+i)->word2=0;
        }
        for (i=0;i<2*hash_table_size;i++) {
          (trans_ref_wb+i)->word1=0;
          (trans_ref_wb+i)->word2=0;
          (trans_ref_bb+i)->word1=0;
          (trans_ref_bb+i)->word2=0;
        }
      }
      else {
        trans_ref_wa=0;
        trans_ref_wb=0;
        trans_ref_ba=0;
        trans_ref_bb=0;
        hash_table_size=0;
        log_hash_table_size=0;
      }
    }
    else Print(0,"ERROR:  hash table size must be > 0\n");
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "hashp" command controls the pawn hash table size.     |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("hashp",input)) {
    if (thinking || pondering) return(2);
    OptionGet(&equal,&slash,text,&more);
    new_hash_size=atoi(text);
    if (strchr(text,'K') || strchr(text,'k')) new_hash_size*=1<<10;
    if (strchr(text,'M') || strchr(text,'m')) new_hash_size*=1<<20;
    if (new_hash_size != 0) {
      if (pawn_hash_table) {
        free(pawn_hash_table);
        pawn_hash_table_size=0;
        log_pawn_hash_table_size=0;
        pawn_hash_table=0;
      }
      new_hash_size/=16;
      for (log_pawn_hash_table_size=0;log_pawn_hash_table_size<8*sizeof(int);log_pawn_hash_table_size++)
        if ((1<<(log_pawn_hash_table_size+1)) > new_hash_size) break;
      pawn_hash_table_size=1<<log_pawn_hash_table_size;
      pawn_hash_table=malloc(16*pawn_hash_table_size);
      if (!pawn_hash_table) {
        printf("malloc() failed, not enough memory.\n");
        free(pawn_hash_table);
        pawn_hash_table_size=0;
        log_pawn_hash_table_size=0;
        pawn_hash_table=0;
      }
      pawn_hash_mask=((unsigned int) 037777777777)>>(32-log_pawn_hash_table_size);
      if (pawn_hash_table_size*16 < 1<<20)
        Print(0,"hash table memory = %dK bytes.\n",pawn_hash_table_size*16/(1<<10));
      else
        Print(0,"hash table memory = %dM bytes.\n",pawn_hash_table_size*16/(1<<20));
      for (i=0;i<pawn_hash_table_size;i++) {
        (pawn_hash_table+i)->word1=0;
        (pawn_hash_table+i)->word2=0;
      }
    }
    else Print(0,"ERROR:  pawn hash table size must be > 0\n");
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "help" command lists commands/options.                 |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("help",input)) {
    nextc=getc(input_stream);
    if (nextc == ' ') {
      fscanf(input_stream,"%s",title);
      i=getchar();
      if (!strcmp("analyze",title)) {
        printf("analyze\n");
        printf("the analyze command puts Crafty into a mode where it will\n");
        printf("search forever in the current position.  when a move is\n");
        printf("entered, crafty will make that move, switch sides, and\n");
        printf("again compute, printing analysis as it searches.  you can\n");
        printf("back up a move by entering \"back\" or you can back up\n");
        printf("several moves by entering \"back <n>\".  note that <n> is\n");
        printf("the number of moves, counting each player's move as one.\n");
      }
      else if (!strcmp("annotate",title)) {
        printf("annotate filename b|w|bw moves margin time\n");
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
        printf("the time limit per move in seconds.\n");
      }
      else if (!strcmp("book",title)) {
        printf("you can use the following commands to customize how the\n");
        printf("program uses the opening book(book.bin and books.bin).\n");
        printf("typically, book.bin contains a large opening database made\n");
        printf("from GM games.  books.bin is a short, customized book that\n");
        printf("contains selected lines that are well-suited to Crafty's\n");
        printf("style of play.  the <flags> can further refine how this\n");
        printf("small book file is used to encourage/avoid specific lines.\n");
        printf("book random n..............controls how random the program\n");
        printf("   chooses its opening moves.  <0> will play the least\n");
        printf("   randomly and follow book lines that are well-suited to\n");
        printf("   Crafty's style of play.  this mode also uses an alpha/beta\n");
        printf("   search to select from the set of book moves.  <1> chooses\n");
        printf("   from the book moves with the best winning percentage.\n");
        printf("   <2> chooses from the book moves that were played the\n");
        printf("   most frequently in the GM database. <3> chooses from the\n");
        printf("   set of book moves that produce the best static evaluation.\n");
        printf("   <4> chooses completely randomly from the known book moves.\n");
        printf("book width n...............specifies how many moves from the\n");
        printf("   sorted set of book moves are to be considered.  1 produces\n");
        printf("   the best move from the set, but provides little randomness.\n");
        printf("   99 includes all moves in the book move set.\n");
        printf("book mask accept <chars>...sets the accept mask to the\n");
        printf("   flag characters in <chars> (see flags below.)  any flags\n");
        printf("   set in this mask will include either (a) moves with the \n");
        printf("   flag set, or (b) moves with no flags set.\n");
        printf("book mask reject <chars>...sets the reject mask to the\n");
        printf("   flag characters in <chars> (see flags below.)  any flags\n");
        printf("   set in this mask will reject any moves with the flag\n");
        printf("   set (in the opening book.)\n");
        printf("book off...................turns the book completely off.\n");
        printf("more...");
        i=getchar();
        printf("book[s] create [<filename>] [maxply]...creates a new opening\n");
        printf("   book by first removing the old book.bin.  it then will\n");
        printf("   parse <filename> and add the moves to either book.bin (if\n");
        printf("   the book create command was used) or to books.bin (if the\n");
        printf("   books create command was used.)  <maxply> truncates book\n");
        printf("   lines after that many plies (typically 60).\n");
        printf("flags are one (or more) members of the following set of\n");
        printf("characters:  {?? ? = ! !! 0 1 2 3 4 5 6 7 8 9 A B C D E F}\n");
        printf("normally, ?? means never play, ? means rarely play,\n");
        printf("= means drawish opening, ! means good move, !! means always\n");
        printf("play, and 0-F are user flags that a user can add to any\n");
        printf("move in the book, and by setting the right mask (above) can\n");
        printf("force the program to either always play the move or never\n");
        printf("play the move.  the special character * means all flags\n");
        printf("and is probably dangerous to use.\n");
        printf("more...");
        i=getchar();
        printf("flags are added to a move by entering the move, a / or \\\n");
        printf("followed by the flags.  / means add the flags to the move\n");
        printf("preserving other flags already there while \\ means replace\n");
        printf("any flags with those following the \\.\n");
        printf("the format of the book text (raw data) is as follows:\n");
        printf("[title information] (required)\n");
        printf("e4 e5 ... (a sequence of moves)\n");
        printf("[title information for next line] (required)\n");
        printf("e4 e6 ...\n");
        printf("end (required)\n");
        printf("\n");
      }
      else if (OptionMatch("edit",title)) {
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
      else if (OptionMatch("list",title)) {
        printf("list is used to update the GM/IM/computer lists, which are\n");
        printf("used internally to control how crafty uses the opening book.\n");
        printf("Syntax:  list GM|IM|C [+|-name] ...\n");
        printf("   GM/IM/C selects the appropriate list.  if no name is given,\n");
        printf("the list is displayed.  if a name is given, it must be preceeded\n");
        printf("by a + (add to list) or -(remove from list).  note that this\n");
        printf("list is not saved in a file, so that anything added or removed\n");
        printf("will be lost when Crafty is re-started.  To solve this, these\n");
        printf("commands can be added to the .craftyrc file.\n");
      }
      else if (OptionMatch("setboard",title)) {
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
        i=getchar();
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
      else if (!strcmp("time",title)) {
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
      else if (!strcmp("verbose",title)) {
        printf("verbose  0 -> no informational output except moves.\n");
        printf("verbose  1 -> display time for moves.\n");
        printf("verbose  2 -> display variation when it changes.\n");
        printf("verbose  3 -> display variation at end of iteration.\n");
        printf("verbose  4 -> display basic search statistics.\n");
        printf("verbose  5 -> display search extension statistics.\n");
        printf("verbose  6 -> display search hashing statistics.\n");
        printf("verbose  9 -> display root moves as they are searched.\n");
      }
      else printf("no help available for that command\n");
    }
    else {
      printf("help command gives a detailed command syntax\n");
      printf("alarm on|off..............turns audible alarm on/off.\n");
      printf("analyze...................analyze a game in progress\n");
      printf("annotate..................annotate game [help].\n");
      printf("book......................controls book [help].\n");
      printf("black.....................sets black to move.\n");
      printf("clock.....................displays chess clock.\n");
      printf("d.........................displays chess \n");
      printf("echo......................echos output to display.\n");
      printf("edit......................edit board position. [help]\n");
      printf("epdhelp...................info about EPD facility.\n");
      printf("exit......................restores STDIN to key\n");
      printf("end.......................terminates program.\n");
      printf("history...................display game moves.\n");
      printf("hash n....................sets transposition table size\n");
      printf("                          (n bytes, nK bytes or nM bytes)\n");
      printf("hashp n...................sets pawn hash table size\n");
      printf("input <filename> [title]..sets STDIN to <filename>.\n");
      printf("                          (and positions to [title] record.)\n");
      printf("list                      update/display GM/IM/computer lists.\n");
      printf("log on|off................turn logging on/off.\n");
      printf("move......................initiates search (same as go).\n");
      printf("new.......................initialize and start new game.\n");
      printf("noise n...................no status until n nodes searched.\n");
      printf("more...");
      i=getchar();
      printf("ponder on|off.............toggle pondering off/on.\n");
      printf("ponder move...............ponder \"move\" as predicted move.\n");
      printf("read <filename>...........read moves in [from <filename>].\n");
      printf("reada <filename>..........read moves in [from <filename>].\n");
      printf("                          (appends to current game history.)\n");
      printf("reset n...................reset game to move n.\n");
      printf("resign <m> <n>............set resign threshold to <m> pawns.\n");
      printf("                          <n> = # of moves before resigning.\n");
      printf("score.....................printf evaluation of position.\n");
      printf("sd n......................sets absolute search depth.\n");
      printf("st n......................sets absolute search time.\n");
      printf("set.................sets board position. [help]\n");
      printf("test......................test a suite of problems. [help]\n");
      printf("time......................time controls. [help]\n");
      printf("trace n...................display search tree below depth n.\n");
      printf("verbose n.................set verbosity level. [help]\n");
      printf("white.....................sets white to move.\n");
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
  else if (!strcmp("hint",input)) {
    printf("Hint: %s\n",hint);
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
  else if (OptionMatch("ics",input)) {
    ics=1;
    verbosity_level=0;
    show_book=0;
    resign=0;
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "input" command directs the program to read input from |
|   a file until eof is reached or an "exit" command is    |
|   encountered while reading the file.  if there is a     |
|   third paramater (which follows the filename), then it  |
|   is used to position the input file by reading through  |
|   the input file, scanning for records that start with   |
|   "title" and matching the remaining text with the third |
|   parameter.                                             |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("input",input)) {
    if (thinking || pondering) return(2);
    title[0]='\0';
    OptionGet(&equal,&slash,filename,&more);
    if (!equal) {
      nextc=getc(input_stream);
      if (nextc == ' ')
        fscanf(input_stream,"%s",title);
    }
    if (!(input_stream=fopen(filename,"r"))) {
      printf("file does not exist.\n");
      input_stream=stdin;
    }
    if ((signed int) strlen(title) > 0) {
      while (!feof(input_stream)) {
        fscanf(input_stream,"%s",text);
        if (!strcmp(text,"title")) {
          fscanf(input_stream,"%s",text);
          if (strstr(text,title)) {
            printf("title %s",text);
            break;
          }
        }
      } 
    }
  }
/*
 ----------------------------------------------------------
|                                                          |
|  "info" command gives some information about Crafty.     |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("info",input)) {
    Print(0,"Crafty version %s\n",version);
    Print(0,"hash table memory = %d 64-bit words.\n",
      12*hash_table_size);
    Print(0,"pawn hash table memory = %d 64-bit words.\n",
      (int) (2*pawn_hash_table_size));
    if (!tc_sudden_death) {
      Print(0,"%d moves/%d minutes %d seconds primary time control\n",
            tc_moves, tc_time/6000, (tc_time/100)%60);
      Print(0,"%d moves/%d minutes %d seconds secondary time control\n",
            tc_secondary_moves, tc_secondary_time/6000,
            (tc_secondary_time/100)%60);
      if (tc_increment) Print(0,"increment %d seconds.\n",tc_increment/100);
    }
    else if (tc_sudden_death == 1) {
      Print(0," game/%d minutes primary time control\n", tc_time/100);
      if (tc_increment) Print(0,"increment %d seconds.\n",(tc_increment/100)%60);
    }
    else if (tc_sudden_death == 2) {
      Print(0,"%d moves/%d minutes primary time control\n",
            tc_moves, tc_time/6000);
      Print(0,"game/%d minutes secondary time control\n",
            tc_secondary_time/6000);
      if (tc_increment) Print(0,"increment %d seconds.\n",tc_increment/100);
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
  else if (OptionMatch("kibitz",input)) {
    OptionGet(&equal,&slash,text,&more);
    kibitz=atoi(text);
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "level" command sets time controls [ics/xboard mode    |
|   only.]                                                 |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("level",input)) {
    OptionGet(&equal,&slash,text,&more);
    tc_moves=atoi(OptionNext(1,text));
    tc_time=atoi(OptionNext(0,text))*100;
    tc_increment=atoi(OptionNext(0,text))*100;
    if (tc_time > 500 || tc_increment > 300) whisper=0;
    if (!tc_moves) {
      tc_sudden_death=1;
      tc_moves=1000;
      tc_moves_remaining=1000;
    }
    else tc_sudden_death=0;
    if (tc_moves) {
      tc_secondary_moves=tc_moves;
      tc_secondary_time=tc_time;
    }
    if (!tc_sudden_death) {
      Print(0,"%d moves/%d minutes primary time control\n",
            tc_moves, tc_time/100);
      Print(0,"%d moves/%d minutes secondary time control\n",
            tc_secondary_moves, tc_secondary_time/100);
      if (tc_increment) Print(0,"increment %d seconds.\n",tc_increment/100);
    }
    else if (tc_sudden_death == 1) {
      Print(0," game/%d minutes primary time control\n",tc_time/100);
      if (tc_increment) Print(0,"increment %d seconds.\n",tc_increment/100);
    }
    tc_time*=60;
    tc_time_remaining=tc_time;
    tc_secondary_time*=60;
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "list" command allows the operator to add or remove    |
|   names from the GM_list, IM_list , computer_list or     |
|   auto_kibitz_list.                                      |
|   The  syntax is "list <list> <option> <name>.           |
|   <list> is one of GM, IM,  C or AK.                     |
|   The final parameter is a name to add  or remove.       |
|   if the name is in the list, it is removed,             |
|   otherwise it is added.  if no name is given, the list  |
|   is displayed.                                          |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("list",input)) {
    char listname[10], name[20]={""};
    fscanf(input_stream,"%s",listname);
    nextc=getc(input_stream);
    while (nextc == ' ') {
      fscanf(input_stream,"%s",name);
      if (!strcmp(listname,"GM")) {
        if (name[0] == '-') {
          for (i=0;i<number_of_GMs;i++)
            if (!strcmp(GM_list[i],name+1)) {
              for (j=i;j<number_of_GMs;j++)
                strcpy(GM_list[j],GM_list[j+1]);
              number_of_GMs--;
              i=0;
              Print(0,"%s removed from GM list.\n",name+1);
              break;
            }
        }
        else if (name[0] == '+') {
          for (i=0;i<number_of_GMs;i++)
            if (!strcmp(GM_list[i],name+1)) {
              Print(0, "Warning: %s is already in GM list.\n",name+1);
              break;
            }
          if (i==number_of_GMs) {
            strcpy(GM_list[number_of_GMs++],name+1);
            Print(0,"%s added to GM list.\n",name+1);
          }
        }
        else printf("error, name must be preceeded by +/- flag.\n");
      }
      if (!strcmp(listname,"IM")) {
        if (name[0] == '-') {
          for (i=0;i<number_of_IMs;i++)
            if (!strcmp(IM_list[i],name+1)) {
              for (j=i;j<number_of_IMs;j++)
                strcpy(IM_list[j],IM_list[j+1]);
              number_of_IMs--;
              i=0;
              Print(0,"%s removed from IM list.\n",name+1);
 
              break;
            }
        }
        else if (name[0] == '+') {
          for (i=0;i<number_of_IMs;i++)
            if (!strcmp(IM_list[i],name+1)) {
              Print(0, "Warning: %s is already in IM list.\n",name+1);
              break;
            }
          if (i==number_of_IMs) {
            strcpy(IM_list[number_of_IMs++],name+1);
            Print(0,"%s added to IM list.\n",name+1);
          }
        }         
        else Print(0,"error, name must be preceeded by +/- flag.\n");
      }
      if (!strcmp(listname,"C")) {
        if (name[0] == '-') {
          for (i=0;i<number_of_computers;i++)
            if (!strcmp(computer_list[i],name+1)) {
              for (j=i;j<number_of_computers;j++)
                strcpy(computer_list[j],computer_list[j+1]);
              number_of_computers--;
              i=0;
              Print(0,"%s removed from computer list.\n",name+1);
              break;
            }
        }
        else if (name[0] == '+') {
          for (i=0;i<number_of_computers;i++)
            if (!strcmp(computer_list[i],name+1)) {
              Print(0, "Warning: %s is already in computer list.\n",name+1);
              break;
            }
          if (i==number_of_computers) {
            strcpy(computer_list[number_of_computers++],name+1);
            Print(0,"%s added to computer list.\n",name+1);
          }
        }
        else Print(0,"error, name must be preceeded by +/- flag.\n");
      }
      
      if (!strcmp(listname,"AK")) {
        if (name[0] == '-') {
          for (i=0;i<number_auto_kibitzers;i++)
            if (!strcmp(auto_kibitz_list[i],name+1)) {
              for (j=i;j<number_auto_kibitzers;j++)
                strcpy(auto_kibitz_list[j],auto_kibitz_list[j+1]);
              number_auto_kibitzers--;
              i=0;
              Print(0,"%s removed from auto kibitz list.\n",name+1);
              break;
            }
        }
        else if (name[0] == '+') {
          for (i=0;i<number_auto_kibitzers;i++)
            if (!strcmp(auto_kibitz_list[i],name+1)) {
              Print(0, "Warning: %s is already in auto kibitz list.\n",name+1);
              break;
            }
          if (i==number_auto_kibitzers) {
            strcpy(auto_kibitz_list[number_auto_kibitzers++],name+1);
            Print(0,"%s added to auto kibitz list.\n",name+1);
          }
        }
        else Print(0,"error, name must be preceeded by +/- flag.\n");
      }
      nextc=getc(input_stream);
    }
    if (name[0] == '\0') { /* No name was specified */
      if (!strcmp(listname,"GM")) {
        Print(0,"GM List:\n");
        for (i=0;i<number_of_GMs;i++)
          Print(0,"%s\n",GM_list[i]);
      }
      else if (!strcmp(listname,"IM")) {
        Print(0,"IM List:\n");
        for (i=0;i<number_of_IMs;i++)
          Print(0,"%s\n",IM_list[i]);
      }
      else if (!strcmp(listname,"C")) {
        Print(0, "computer list:\n");
        for (i=0;i<number_of_computers;i++)
          Print(0,"%s\n",computer_list[i]);
      }
      else if (!strcmp(listname,"AK")) {
        Print(0, "auto kibitz list:\n");
        for (i=0;i<number_auto_kibitzers;i++)
          Print(0,"%s\n",auto_kibitz_list[i]);
      }
    }
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "log" command turns log on/off.                        |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("log",input)) {
    OptionGet(&equal,&slash,text,&more);
    if (!strcmp(text,"on")) {
      for (log_id=1;log_id <300;log_id++) {
        sprintf(log_filename,"%s/log.%03d",LOGDIR,log_id);
        sprintf(history_filename,"%s/game.%03d",LOGDIR,log_id);
        log_file=fopen(log_filename,"r");
        if (!log_file) break;
        fclose(log_file);
      }
      log_file=fopen(log_filename,"w");
      history_file=fopen(history_filename,"w+");
    }
    else if (!strcmp(text,"off")) {
      fclose(log_file);
      log_file=0;
      sprintf(filename,"%s/log.%03d",LOGDIR,log_id);
      remove(filename);
    }
    else Print(0,"usage:  log on|off\n");
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
  else if (OptionMatch("mode",input)) {
    if (equal) {
      OptionGet(&equal,&slash,text,&more);
      if (!strcmp(text,"tournament")) {
        mode=tournament_mode;
        draw_score_is_zero=1;
        book_random=3;
        printf("use 'settc' command if a game is restarted after crafty\n");
        printf("has been terminated for any reason.\n");
      }
      else if (!strcmp(text,"normal"))
        mode=normal_mode;
      else {
        printf("usage: mode normal|tournament\n");
        mode=normal_mode;
      }
    }
    else {
      nextc=getc(input_stream);
      if (nextc == ' ') {
        fscanf(input_stream,"%s",text);
        if (!strcmp(text,"tournament")) {
          mode=tournament_mode;
          draw_score_is_zero=1;
        }
        else if (!strcmp(text,"normal"))
          mode=normal_mode;
        else {
          printf("usage: mode normal|tournament\n");
          mode=normal_mode;
        }
      }
    }
    if (mode == tournament_mode)
      printf("tournament mode.\n");
    else if (mode == normal_mode)
      printf("normal mode.\n");
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
  else if (OptionMatch("name",input)) {
    fscanf(input_stream,"%s",opponents_name);
    Print(0,"Crafty %s vs %s\n",version,opponents_name);
    next=opponents_name;
    while (*next) {
      *next=tolower(*next);
      next++;
    }
    if (mode != tournament_mode) {
      for (i=0;i<number_auto_kibitzers;i++)
/* 
    decrease  the new aggresive use of time against automated opponents.
    the multiplier is now 9 for non automated opponents - up from 8, 
    automated stays @ 8.
    this is here ( as opposed to in the computer section below) to
    counterattack the aggressive use of time by manually operated
    computer opponents.
*/
        if (!strcmp(auto_kibitz_list[i],opponents_name)) {
          kibitz=4;
          inc_time_multiplier=7.5;
          zero_inc_factor=6.8;
          auto_kibitzing=1;
          break;
        }
      for (i=0;i<number_of_computers;i++)
        if (!strcmp(computer_list[i],opponents_name)) {
          draw_score_is_zero=1;
          book_random=2;
          book_selection_width=2;
          Print(1,"playing a computer!\n");
          break;
        }
      for (i=0;i<number_of_GMs;i++)
        if (!strcmp(GM_list[i],opponents_name)) {
          Print(1,"playing a GM!\n");
          book_random=1;
          book_selection_width=3;
          resign=5;
          resign_count=6;
          draw_count=6;
          kibitz=0;
          break;
        }
      for (i=0;i<number_of_IMs;i++)
        if (!strcmp(IM_list[i],opponents_name)) {
          Print(1,"playing an IM!\n");
          book_random=1;
          book_selection_width=4;
          resign=6;
          resign_count=8;
          draw_count=8;
          kibitz=0;
          break;
        }
      }
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "new" command initializes for a new game.  note that   |
|   "AN" is an alias for this command, for autoplay 232    |
|   compatibility.                                         |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("new",input) || OptionMatch("AN",input)) {
    if (thinking || pondering) return(2);
    ponder_completed=0;
    ponder_move=0;
    last_pv.path_iteration_depth=0;
    last_pv.path_length=0;
    InitializeChessBoard(&position[0]);
    InitializeHashTables();
    wtm=1;
    move_number=1;
    tc_time_remaining=tc_time;
    tc_moves_remaining=tc_moves;
    if (log_file) fclose(log_file);
    if (history_file) fclose(history_file);
    log_id++;
    if (log_file) {
      sprintf(filename,"%s/log.%03d",LOGDIR,log_id);
      log_file=fopen(filename,"w+");
    }
    sprintf(filename,"%s/game.%03d",LOGDIR,log_id);
    history_file=fopen(filename,"w+");
  }
/*
 ----------------------------------------------------------
|                                                          |
|  "noop" command is a no-operation that is used to keep   |
|  Crafty and the ICS interface in sync.                   |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("noop",input)) {
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
  else if (OptionMatch("noise",input)) {
    OptionGet(&equal,&slash,text,&more);
    noise_level=atoi(text);
    Print(0,"noise level set to %d.\n",noise_level);
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "operator" command sets the operator time.  this time  |
|   is subtracted from the time remaining, so that the     |
|   operator is allocated some time for normal operation   |
|   of crafty.                                             |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("operator",input)) {
    OptionGet(&equal,&slash,text,&more);
    tc_operator_time=ParseTime(text)*6000;
    Print(0,"reserving %s for operator's overhead\n",
          DisplayHHMM(tc_operator_time));
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "otime" command sets the opponent's time remaining.    |
|   currently, it is unused, but is here for compatibility |
|   with the ics/Xboard interface which supplies this      |
|   after each move.                                       |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("otime",input)) {
    OptionGet(&equal,&slash,text,&more);
    tc_time_remaining_opponent=atoi(text);
    if (log_file) fprintf(log_file,"time remaining: %s (opponent).\n",
                          DisplayTime(tc_time_remaining_opponent));
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "perf" command turns times move generator/make_move.   |
|                                                          |
 ----------------------------------------------------------
*/
#define PERF_CYCLES 100000
  else if (OptionMatch("perf",input)) {
    if (thinking || pondering) return(2);
    if (wtm)
      target=Compl(WhitePieces);
    else
      target=Compl(BlackPieces);
    clock_before = clock();
    while (clock() == clock_before);
    clock_before = clock();
    for (i=0;i<PERF_CYCLES;i++)
      last[1]=GenerateMoves(0, 1, wtm, target, 1, last[0]);
    clock_after=clock();
    time_used=((float) clock_after-(float) clock_before) / 
              (float) CLOCKS_PER_SEC;
    printf("generated %d moves, time=%d microseconds\n",
           (last[1]-last[0])*PERF_CYCLES, (clock_after-clock_before));
    printf("generated %d moves per second\n",(int) (((float) (PERF_CYCLES*
           (last[1]-last[0])))/time_used));
    clock_before=clock();
    while (clock() == clock_before);
    clock_before = clock();
    for (i=0;i<PERF_CYCLES;i++) {
      last[1]=GenerateMoves(0, 1, wtm, target, 1, last[0]);
      for (mv=last[0];mv<last[1];mv++) {
        MakeMove(0,*mv,wtm);
        UnMakeMove(0,*mv,wtm);
      }
    }
    clock_after=clock();
    time_used=((float) clock_after-(float) clock_before) / 
              (float) CLOCKS_PER_SEC;
    printf("generated/made/unmade %d moves, time=%d microseconds\n",
      (last[1]-last[0])*PERF_CYCLES,
    (clock_after-clock_before));
    printf("generated/made/unmade %d moves per second\n",(int) (((float) (PERF_CYCLES*
           (last[1]-last[0])))/time_used));
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "perft" command turns tests move generator/make_move.  |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("perft",input)) {
    if (thinking || pondering) return(2);
    position[1]=position[0];
    last[0]=move_list;
    OptionGet(&equal,&slash,text,&more);
    j=atoi(text);
    total_moves=0;
    OptionPerft(1,j,wtm);
    printf("total moves=%d\n",total_moves);
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "ponder" command toggles pondering off/on or sets a    |
|   move to ponder.                                        |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("ponder",input)) {
    if (thinking || pondering) return(2);
    OptionGet(&equal,&slash,text,&more);
    if (!strcmp(text,"on")) {
      do_ponder=1;
      Print(0,"pondering enabled.\n");
    }
    else if (!strcmp(text,"off")) {
      do_ponder=0;
      Print(0,"pondering disabled.\n");
    }
    else {
      ponder_move=InputMove(text,0,wtm,0,0);
      ponder_completed=0;
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
  else if (!strcmp("post",input)) {
    post=1;
  }
  else if (!strcmp("nopost",input)) {
    post=0;
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "savegame" command saves the game in a file in PGN     |
|   format.  command has an optional filename.  note that  |
|   SR is an autoplay 232 alias that behaves slightly      |
|   differently.                                           |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("savegame",input) ||
           OptionMatch("SR",input)) {
    output_file=stdout;
    if (OptionMatch("SR",input)) nextc=' ';
    else nextc=getc(input_stream);
  
    if (nextc == ' ') {
      fscanf(input_stream,"%s",filename);
      if (!(output_file=fopen(filename,"w"))) {
        printf("unable to open %s for write.\n",filename);
        return(1);
      }
    }
    if (wtm)
      fprintf(output_file,"[ %s vs Crafty ]\n",
        *opponents_name ? opponents_name : "human");
    else
      fprintf(output_file,"[ Crafty vs %s ]\n",
        *opponents_name ? opponents_name : "human");
    next=text;
    for (i=0;i<(move_number-1)*2-wtm+1;i++) {
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
      fprintf(output_file,"%s\n",text);
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
  else if (OptionMatch("savepos",input)) {
    char xlate[15]={'q','r','b',0,'k','n','p',0,'P','N','K',0,'B','R','Q'};
    char empty[9]={' ','1','2','3','4','5','6','7','8'};
    int rank, file, nempty;
    output_file=stdout;
    nextc=getc(input_stream);
  
    if (nextc == ' ') {
      fscanf(input_stream,"%s",filename);
      if (!(output_file=fopen(filename,"w"))) {
        printf("unable to open %s for write.\n",filename);
        return(1);
      }
    }
    fprintf(output_file,"setboard ");
    for (rank=RANK8;rank>=RANK1;rank--) {
      nempty=0;
      for (file=FILEA;file<=FILEH;file++) {
        if (PieceOnSquare((rank<<3)+file)) {
          if (nempty) {
            fprintf(output_file,"%c",empty[nempty]);
            nempty=0;
          }
          fprintf(output_file,"%c",xlate[PieceOnSquare((rank<<3)+file)+7]);
        }
        else nempty++;
      }
      fprintf(output_file,"/");
    }
    fprintf(output_file," %c ",(wtm)?'w':'b');
    if (WhiteCastle(0) & 1) fprintf(output_file,"K");
    if (WhiteCastle(0) & 2) fprintf(output_file,"Q");
    if (BlackCastle(0) & 1) fprintf(output_file,"k");
    if (BlackCastle(0) & 2) fprintf(output_file,"q");
    if (EnPassant(0)) fprintf(output_file,"%c%c",File(EnPassant(0))+'a',
                              Rank(EnPassant(0))+((wtm)?-1:+1)+'1');
    fprintf(output_file,"\n");
 
    if (output_file != stdout) {
      fprintf(output_file,"exit\n");
      fclose(output_file);
    }
    printf("FEN save complete.\n");
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
  else if (!strcmp("remove",input)) {
    if (thinking || pondering) return(2);
    move_number--;
    sprintf(tinput,"reset=%d",move_number);
    Option(tinput);
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
  else if (OptionMatch("reset",input)) {
    if (thinking || pondering) return(2);
    ponder_completed=0;
    ponder_move=0;
    last_mate_score=0;
    last_pv.path_iteration_depth=0;
    last_pv.path_length=0;
    if (thinking || pondering) return(2);
    over=0;
    OptionGet(&equal,&slash,text,&more);
    move_number=atoi(text);
    if (!move_number) {
      move_number=1;
      return(1);
    }
    nmoves=(move_number-1)*2+1-wtm;
    root_wtm=ChangeSide(wtm);
    wtm=1;
    move_number=1;
    InitializeChessBoard(&position[0]);
    for (i=0;i<nmoves;i++) {
      fseek(history_file,i*10,SEEK_SET);
      fscanf(history_file,"%s",text);
      move=InputMove(text,0,wtm,0,0);
      if (move) {
        MakeMoveRoot(move,wtm);
      }
      else {
        printf("ERROR!  move %s is illegal\n",text);
        break;
      }
      wtm=ChangeSide(wtm);
      if (wtm) move_number++;
      Phase();
    } 
    last_move_in_book=move_number;
    tc_moves_remaining=tc_moves-move_number+1;
    while (tc_moves_remaining < 0) tc_moves_remaining+=tc_secondary_moves;
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
  else if (OptionMatch("read",input) ||
           OptionMatch("reada",input)) {
    if (thinking || pondering) return(2);
    if (!strcmp("reada",input))
      append=1;
    else
      append=0;
    ponder_completed=0;
    ponder_move=0;
    last_pv.path_iteration_depth=0;
    last_pv.path_length=0;
    input_file=input_stream;
    if (equal) {
      OptionGet(&equal,&slash,filename,&more);
      if (!(input_file=fopen(filename,"r"))) {
        printf("file %s does not exist.\n",filename);
        return(1);
      }
    }
    else {
      nextc=getc(input_stream);
      if (nextc == ' ') {
        fscanf(input_stream,"%s",filename);
        if (!(input_file=fopen(filename,"r"))) {
          printf("file %s does not exist.\n",filename);
          return(1);
        }
      }
    }
    if (input_file == stdin)
      printf("type \"exit\" to terminate.\n");
    if (thinking || pondering) return(2);
    if (!append) {
      InitializeChessBoard(&position[0]);
      wtm=1;
      move_number=1;
    }
    do {
      do {
        if (input_file == stdin)
          if (wtm)
            printf("read.White(%d): ",move_number);
          else
            printf("read.Black(%d): ",move_number);
        scanf_status=fscanf(input_file,"%s",text);
        if (scanf_status <= 0) break;
        move=0;
        if (((text[0]>='a') && (text[0]<='z')) ||
            ((text[0]>='A') && (text[0]<='Z'))) {
          if (!strcmp(text,"exit")) {
            if (input_file != stdin)
              fclose(input_file);
            break;
          }
          move=InputMove(text,0,wtm,1,0);
          if (move) {
            if (input_file != stdin) {
              printf("%s ",OutputMove(&move,0,wtm));
              if (!(move_number % 8) && ChangeSide(wtm))
                printf("\n");
            }
            fseek(history_file,((move_number-1)*2+1-wtm)*10,SEEK_SET);
            fprintf(history_file,"%10s",OutputMove(&move,0,wtm));
            MakeMoveRoot(move,wtm);
#if defined(DEBUG)
            ValidatePosition(1,move,"Option()");
#endif
          }
          else {
            if (input_file == stdin)
              printf("illegal move.\n");
          }
        }
      } while (!move && (scanf_status>0));
      if (!strcmp(text,"exit")) break;
      if (scanf_status <= 0) break;
      wtm=ChangeSide(wtm);
      Phase();
      if (wtm) move_number++;
    } while (scanf_status > 0);
    last_move_in_book=move_number;
    if (input_file != stdin) {
      printf("\n");
      fclose(input_file);
    }
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "resign" command sets the resignation threshold to     |
|   the number of pawns the program must be behind before  |
|   resigning (0 -> disable resignations).                 |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("resign",input)) {
    OptionGet(&equal,&slash,text,&more);
    resign=atoi(text);
    if (!ics & !xboard) {
      if (resign)
        Print(0,"resignation threshold set to %d pawns.\n",resign);
      else
        Print(0,"resignations disabled.\n");
    }
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "search" command sets a specific move for the search   |
|   to analyze, ignoring all others completely.            |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("search",input)) {
    if (thinking || pondering) return(2);
    OptionGet(&equal,&slash,text,&more);
    search_move=InputMove(text,0,wtm,0,0);
    if (!search_move) search_move=InputMove(text,0,ChangeSide(wtm),0,0);
    if (!search_move) printf("illegal move\n");
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "settc" command is used to reset the time controls     |
|   after a complete restart.                              |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("settc",input)) {
    if (thinking || pondering) return(2);
    printf("moves to time control? ");
    scanf("%d",&tc_moves_remaining);
    printf("How much time left on Crafty's clock? ");
    scanf("%s",text);
    tc_time_remaining=ParseTime(text)*6000;
    printf("How much time left on opponent's clock? ");
    fscanf(input_stream,"%s",text);
    tc_time_remaining_opponent=ParseTime(text)*6000;
    fprintf(log_file,"time remaining: %s (crafty).\n",
                     DisplayTime(tc_time_remaining));
    fprintf(log_file,"time remaining: %s (opponent).\n",
                     DisplayTime(tc_time_remaining_opponent));
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "setboard" command sets the board to a specific        |
|   position for analysis by the program.                  |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("setboard",input)) {
    if (thinking || pondering) return(2);
    SetBoard();
    ponder_completed=0;
    ponder_move=0;
    last_pv.path_iteration_depth=0;
    last_pv.path_length=0;
    over=0;
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "score" command displays static evaluation of the      |
|   current board position.                                |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("score",input)) {
    if (thinking || pondering) return(2);
    root_wtm=ChangeSide(wtm);
    position[1]=position[0];
    PreEvaluate(wtm);
    s7=Evaluate(1,1,-99999,99999);
    s1=Material;
    s2=EvaluateDevelopment(1);
    s3=EvaluatePawns();
    s4=EvaluatePassedPawns();
    s5=EvaluatePassedPawnRaces(wtm);
    s6=EvaluateOutsidePassedPawns();
    Print(1,"note: scores are for the white side\n");
    Print(1,"material evaluation.................%s\n",
      DisplayEvaluation(s1));
    Print(1,"development.........................%s\n",
      DisplayEvaluation(s2));
    Print(1,"pawn evaluation.....................%s\n",
      DisplayEvaluation(s3));
    Print(1,"passed pawn evaluation..............%s\n",
      DisplayEvaluation(s4));
    Print(1,"passed pawn race evaluation.........%s\n",
      DisplayEvaluation(s5));
    Print(1,"outside passed pawn evaluation......%s\n",
      DisplayEvaluation(s6));
    Print(1,"piece evaluation....................%s\n",
      DisplayEvaluation(s7-s1-s2-s3-s4-s5-s6));
    Print(1,"total evaluation....................%s\n",
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
  else if (OptionMatch("sd",input)) {
    OptionGet(&equal,&slash,text,&more);
    search_depth=atoi(text);
    Print(0,"search depth set to %d.\n",search_depth);
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
  else if (OptionMatch("show",input)) {
    OptionGet(&equal,&slash,text,&more);
    if (OptionMatch("book",text)) {
      show_book=!show_book;
      if (show_book) Print(0,"show book statistics\n");
      else Print(0,"don't show book statistics\n");
    }
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "st" command sets a specific search time to control    |
|   the tree search time.                                  |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("st",input)) {
    OptionGet(&equal,&slash,text,&more);
    search_time_limit=atoi(text)*100;
    Print(0,"search time set to %d.\n",search_time_limit/100);
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "test" command runs a test suite of problems and       |
|   prints results.                                        |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("test",input)) {
    if (thinking || pondering) return(2);
    OptionGet(&equal,&slash,filename,&more);
    if (!(input_stream=fopen(filename,"r"))) {
      printf("file does not exist.\n");
      input_stream=stdin;
    }
    else {
      Test();
      ponder_completed=0;
      ponder_move=0;
      last_pv.path_iteration_depth=0;
      last_pv.path_length=0;
    }
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
  else if (OptionMatch("time",input)) {
    if (ics || xboard) {
      OptionGet(&equal,&slash,text,&more);
      tc_time_remaining=atoi(text);
      if (log_file) fprintf(log_file,"time remaining: %s (crafty).\n",
                            DisplayTime(tc_time_remaining));
    }
    else {
      if (thinking || pondering) return(2);
      OptionGet(&equal,&slash,text,&more);
      if (!strcmp("cpu",text)) {
        time_type=cpu;
        Print(0,"using cpu time\n");
      }
      else if (!strcmp("elapsed",text)) {
        time_type=elapsed;
        Print(0,"using elapsed time\n");
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
        error=0;
        do {
          next=strchr(text,'/');
          if (!next) {
            error=1;
            break;
          }
          *next='\0';
          if (!strcmp(text,"sd")) {
            tc_sudden_death=1;
            tc_moves=1000;
          }
          else 
            tc_moves=atoi(text);
          next++;
          tc_time=TtoI(next)*100;
  
          next1=strchr(next,'/');
          if (!next1) {
            tc_secondary_time=tc_time;
            tc_secondary_moves=tc_moves;
            break;
          }
          else {
            next1++;
            next=strchr(next1,'/');
            *next='\0';
            if (!strcmp(next1,"sd")) {
              tc_sudden_death=2;
              tc_secondary_moves=1000;
            }
            else tc_secondary_moves=atoi(next1);
            next++;
            tc_secondary_time=TtoI(next)*100;
  
            next1=strchr(next,'/');
            if (!next1) break;
            next1++;
            tc_increment=atoi(next1)*100;
          }
        } while (0);
        tc_time_remaining=tc_time;
        tc_time_remaining_opponent=tc_time;
        tc_moves_remaining=tc_moves;
        if (!tc_sudden_death) {
          Print(0,"%d moves/%d minutes primary time control\n",
                tc_moves, tc_time/100);
          Print(0,"%d moves/%d minutes secondary time control\n",
                tc_secondary_moves, tc_secondary_time/100);
          if (tc_increment) Print(0,"increment %d seconds.\n",tc_increment/100);
        }
        else if (tc_sudden_death == 1) {
          Print(0," game/%d minutes primary time control\n",
                tc_time);
          if (tc_increment) Print(0,"increment %d seconds.\n",tc_increment);
        }
        else if (tc_sudden_death == 2) {
          Print(0,"%d moves/%d minutes primary time control\n",
                tc_moves, tc_time);
          Print(0,"game/%d minutes secondary time control\n",
                tc_secondary_time);
          if (tc_increment) Print(0,"increment %d seconds.\n",tc_increment);
        }
        tc_time*=60;
        tc_time_remaining*=60;
        tc_time_remaining_opponent*=60;
        tc_secondary_time*=60;
        if (error)
          printf("usage:  time nmoves/ntime [nmoves/ntime] increment\n");
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
  else if (OptionMatch("timeleft",input)) {
    OptionGet(&equal,&slash,text,&more);
    if (wtm) {
      tc_time_remaining=atoi(OptionNext(1,text));
      tc_time_remaining_opponent=atoi(OptionNext(0,text));
    }
    else {
      tc_time_remaining_opponent=atoi(OptionNext(1,text));
      tc_time_remaining=atoi(OptionNext(0,text));
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
  else if (OptionMatch("trace",input)) {
    fscanf(input_stream,"%d",&trace_level);
#if defined(FAST)
    printf("Sorry, but I can't display traces when compiled with -DFAST\n");
#else
    printf("trace=%d\n",trace_level);
#endif
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "undo" command backs up 1/2 move, which leaves the     |
|   opposite side on move. [xboard compatibility]          |
|                                                          |
 ----------------------------------------------------------
*/
  else if (!strcmp("undo",input)) {
    if (thinking || pondering) return(2);
    wtm=ChangeSide(wtm);
    if (ChangeSide(wtm)) move_number--;
    sprintf(tinput,"reset=%d",move_number);
    Option(tinput);
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
 else if (OptionMatch("usage",input)) {
    OptionGet(&equal,&slash,text,&more);
    usage_level=atof(text);
    Print(0,"time usage up front set to %5.1f percent increase/(-)decrease.\n",
          usage_level);
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "verbose" command sets specific verbosity level which  |
|   controls how "chatty" the program is.  "0" is the      |
|   least "talkative" and anything >9 is maximum.          |
|                                                          |
|     0 -> no informational output except moves.           |
|     1 -> display time for moves.                         |
|     2 -> display variation when it changes.              |
|     3 -> display variation at end of iteration.          |
|     4 -> display basic search statistics.                |
|     5 -> display extended search statistics.             |
|     6 -> display root moves as they are searched.        |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("verbose",input)) {
    OptionGet(&equal,&slash,text,&more);
    verbosity_level=atoi(text);
    Print(0,"verbosity set to %d.\n",verbosity_level);
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
  else if (OptionMatch("whisper",input)) {
    OptionGet(&equal,&slash,text,&more);
    whisper=atoi(text);
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "white" command sets white to move (wtm).              |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("white",input)) {
    if (thinking || pondering) return(2);
    ponder_completed=0;
    ponder_move=0;
    last_pv.path_iteration_depth=0;
    last_pv.path_length=0;
    wtm=1;
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
  else if (OptionMatch("xboard",input)) {
    printf("Chess\n");
    xboard=1;
    verbosity_level=0;
    show_book=0;
    resign=0;
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
*   OptionGet() returns the next value from the input string.                  *
*                                                                              *
********************************************************************************
*/
void OptionGet(char **equal,char **slash,char *parameter,int *more)
{
  *more=0;
  if (*slash) {
    strcpy(parameter,*slash+1);
    *slash=0;
  }
  else if (*equal) {
    strcpy(parameter,*equal+1);
    *equal=0;
  }
  else fscanf(input_stream,"%s",parameter);
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
int OptionMatch(char *command, char *input)
{
  char bad[]={"abcdefghnbrqk"};
  int i,j;
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
|   now use strstr() to see if "input" in in "command"     |
|   the first requirement is that input matches command    |
|   starting at the very left-most character;              |
|                                                          |
 ----------------------------------------------------------
*/
  if (strstr(command,input) == command) {
    for (i=0;i<(int) strlen(input);i++) {
      for (j=0;j<(int) strlen(bad);j++) if (bad[j] == input[i]) break;
      if ((j=strlen(bad))) return(1);
    }
  }
  return(0);
}

/*
********************************************************************************
*                                                                              *
*   OptionNext() is used to return the next "token" from the input string and  *
*   if none is available, read another string from the input stream and        *
*   continue.                                                                  *
*                                                                              *
*                                                                              *
********************************************************************************
*/
char* OptionNext(int first, char *input)
{
  char delims[]={" ;/"};
  char *next;

  if (first) return(strtok(input,delims));
  else next=strtok(0,delims);
  if (!next) {
    fscanf(input_stream,"%s",input);
    next=strtok(input,delims);
  }
  return(next);
}

void OptionPerft(int ply,int depth,int wtm)
{
  BITBOARD target;
  int i, *mv;

  if (wtm) target=Compl(WhitePieces);
  else target=Compl(BlackPieces);
  last[ply]=GenerateMoves(ply, 99, wtm, target, 1, last[ply-1]);
  for (mv=last[ply-1];mv<last[ply];mv++) {
    MakeMove(ply,*mv,wtm);
    if (!Check(wtm)) {
      if (ply <= trace_level) {
        for (i=1;i<ply;i++) printf("  ");
        printf("%s\n", OutputMove(mv,ply,wtm));
      }
      total_moves++;
      if (depth-1) OptionPerft(ply+1,depth-1,ChangeSide(wtm));
    }
    UnMakeMove(ply,*mv,wtm);
  }
}
