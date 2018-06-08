#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "types.h"
#include "evaluate.h"
#include "function.h"
#include "data.h"
#if defined(UNIX)
#  include <unistd.h>
#endif
#include "epdglue.h"
/*
********************************************************************************
*                                                                              *
*   Option() is used to handle user input necessary to control/customize the   *
*   program.  it performs all functions excepting chess move input which is    *
*   handled by main().                                                         *
*                                                                              *
********************************************************************************
*/
int Option(char *tinput)
{
  FILE *input_file, *output_file;
  int back_number, move, *mv;
  int i, j, mn, tmn, t1, t2, t3, t4, line1, line2, nmoves;
  int s1, s2, s3, s4, s5, s6, s7, s8, s9;
  int more, new_hash_size, error;
  char filename[64], title[64], input[100], text[100], onoff[10], colors[10];
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
	    };
	    (void) EGCommand(eg_commbufv);
	    return (1);
	  };
  };

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
  if ((equal=strchr(input,'='))) *strchr(input,'=')='\0';
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
  if ((slash=strchr(input,'/'))) *strchr(input,'/')='\0';
/*
 ----------------------------------------------------------
|                                                          |
|   "alarm" command turns audible move warning on/off.     |
|                                                          |
 ----------------------------------------------------------
*/
  if (OptionMatch("alarm",input)) {
    OptionGet(&equal,&slash,input,onoff,&more);
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
    if (thinking || pondering) return(2);
    ponder_completed=0;
    ponder_move=0;
    analyze_mode=1;
    printf("Analyze Mode: type \"exit\" to terminate.\n");
    do {
      do {
        pv[0].path_iteration_depth=0;
        pv[0].path_length=0;
        analyze_move_read=0;
        pondering=1;
        position[1]=position[0];
        (void) Iterate(wtm,think);
        pondering=0;
        if (wtm) printf("analyze.White(%d): ",move_number);
        else printf("analyze.Black(%d): ",move_number);
        if (!analyze_move_read) scanf_status=scanf("%s",tinput);
        else scanf_status=1;
        if (scanf_status <= 0) break;
        move=0;
        if (((tinput[0]>='a') && (tinput[0]<='z')) ||
            ((tinput[0]>='A') && (tinput[0]<='Z'))) {
          if (!strcmp(tinput,"exit")) break;
          move=InputMove(tinput,0,wtm,1);
          if (move) {
            fseek(history_file,((move_number-1)*2+1-wtm)*10,SEEK_SET);
            fprintf(history_file,"%10s",OutputMove(&move,0,wtm));
            if (analyze_move_read) printf("%s\n",OutputMove(&move,0,wtm));
            MakeMoveRoot(move,wtm);
          }
          else if (OptionMatch("back",tinput)) {
            nextc=getc(input_stream);
            if (nextc == ' ') scanf("%d",&back_number);
            else back_number=1;
            for (i=0;i<back_number;i++) {
              wtm=!wtm;
              if (!wtm) move_number--;
            }
            sprintf(tinput,"reset %d",move_number);
            Option(tinput);
          }
          else
            if (Option(tinput) != 1) printf("illegal move.\n");
        }
      } while (!move && (scanf_status>0));
      if (!strcmp(tinput,"exit")) break;
      if (scanf_status <= 0) break;
      wtm=!wtm;
      if (wtm) move_number++;
    } while (scanf_status > 0);
    analyze_mode=0;
    printf("analyze complete.\n");
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "annotate" command is used to search through the game  |
|   in the "history" file (often set by the "read" command |
|   which reads moves in, skipping non-move information    |
|   such as move numbers, times, etc.)                     |
|                                                          |
|   the format of the command is "annotate b|w|bw [moves]" |
|   where b/w/bw indicates whether to annotate only the    |
|   white side (w), the black side (b) or both (bw).       |
|                                                          |
|   [moves] is optional.  If omitted, annotate the         |
|   complete game;  if present, it can be a single number  |
|   which indicates the move to start annotation on, or    |
|   else a range [10-25] which says start annotation at    |
|   move 10 and stop after move 25.                        |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("annotate",input)) {
    OptionGet(&equal,&slash,input,colors,&more);
    line1=1;
    line2=move_number;
    if (more) {
      OptionGet(&equal,&slash,input,title,&more);
      if(strchr(title,'-')) sscanf(title,"%d-%d",&line1,&line2);
      else {
        sscanf(title,"%d",&line1);
        line2=move_number;
      }
    }
    if (!equal) {
      nextc=getc(input_stream);
      if (nextc == ' ') {
        fscanf(input_stream,"%s",title);
        if(strchr(title,'-')) sscanf(title,"%d-%d",&line1,&line2);
        else {
          sscanf(title,"%d",&line1);
          line2=move_number;
        }
      }
    }
    else {
      line1=1;
      line2=move_number;
    }
/*
   reset the game board and start reading through the
   history file.  if the current side-to-move matches
   the "color" then we search, otherwise we just make
   the move and continue until done.
*/
    ponder_completed=0;
    ponder_move=0;
    pv[0].path_iteration_depth=0;
    pv[0].path_length=0;
    InitializeChessBoard(&position[0]);
    wtm=1;
    move_number=1;
    do {
      if (feof(history_file)) break;
      if (move_number >= line1)
        if ((!wtm && strchr(colors,'b')) | (wtm && strchr(colors,'w'))) {
          pv[0].path_iteration_depth=0;
          thinking=1;
          position[1]=position[0];
          (void) Iterate(wtm,think);
          thinking=0;
        }
      fseek(history_file,((move_number-1)*2+1-wtm)*10,SEEK_SET);
      fscanf(history_file,"%s",text);
      move=InputMove(text,0,wtm,1);
      if (!move) {
        printf("illegal move (%s) from history, aborting\n",text);
        break;
      }
      if (wtm) printf("White(%d): %s\n",move_number,text);
      else printf("Black(%d): %s\n",move_number,text);
      MakeMoveRoot(move,wtm);
      wtm=!wtm;
      if (wtm) move_number++;
      if (move_number > line2) break;
    } while (1);
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "ansi" command turns video highlight on/off.           |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("ansi",input)) {
    OptionGet(&equal,&slash,input,onoff,&more);
    if (!strcmp(onoff,"on")) ansi=1;
    else if (!strcmp(onoff,"off")) ansi=0;
    else printf("usage:  ansi on|off\n");
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "black" command sets black to move (!wtm).             |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("black",input)) {
    ponder_completed=0;
    ponder_move=0;
    pv[0].path_iteration_depth=0;
    pv[0].path_length=0;
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
    printf("time remaining on clock is %s.\n",
           DisplayTime(tc_time_remaining*10));
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "display" command displays the chess             |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("display",input)) {
    nextc=getc(input_stream);
    if (nextc == ' ') {
      fscanf(input_stream,"%s",text);
      position[1]=position[0];
      PreEvaluate(wtm);
      if (OptionMatch("pawn",text)) 
        DisplayPieceBoards(pawn_value_w,pawn_value_b);
      if (OptionMatch("knight",text))
        DisplayPieceBoards(knight_value_w,knight_value_b);
      if (OptionMatch("bishop",text))
        DisplayPieceBoards(bishop_value_w,bishop_value_b);
      if (OptionMatch("queen",text))
        DisplayPieceBoards(queen_value_w,queen_value_b);
      if (OptionMatch("king",text))
        DisplayPieceBoards(king_value_w,king_value_b);
    }
    else DisplayChessBoard(stdout,position[0]);
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
    OptionGet(&equal,&slash,input,text,&more);
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
    Edit();
    ponder_completed=0;
    ponder_move=0;
    pv[0].path_iteration_depth=0;
    pv[0].path_length=0;
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "end" (or "quit") command terminates the program.      |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("end",input) || OptionMatch("quit",input)) {
    if (thinking || pondering) return(2);
    fclose(book_file);
    fclose(history_file);
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
  }
/*
 ----------------------------------------------------------
|                                                          |
|   here we handle a group of commands that are used to    |
|   control various search extensions.                     |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("extension",input)) {
    OptionGet(&equal,&slash,input,text,&more);
    if (OptionMatch("check",text)) {
      OptionGet(&equal,&slash,input,text,&more);
      check_extensions=atoi(text);
      if (check_extensions) Print(0,"check extensions enabled.\n");
      else Print(0,"check extensions disabled.\n");
    }
    else if (OptionMatch("qcheck",text)) {
      OptionGet(&equal,&slash,input,text,&more);
      quiescence_checks=atoi(text);
      if (quiescence_checks) Print(0,"quiescence checks = %d.\n",
                                   quiescence_checks);
      else Print(0,"quiescence check extensions disabled.\n");
    }
    else if (OptionMatch("recapture",text)) {
      OptionGet(&equal,&slash,input,text,&more);
      recapture_extensions=atoi(text);
      if (recapture_extensions) Print(0,"recapture extensions enabled.\n");
      else Print(0,"recapture extensions disabled.\n");
    }
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
    ponder_completed=0;
    ponder_move=0;
    pv[0].path_iteration_depth=0;
    pv[0].path_length=0;
    if (ics) force=1;
    else {
      tmn=move_number;
      if (thinking || pondering) return(2);
      mn=move_number;
      if (wtm) mn--;
      sprintf(tinput,"reset=%d",mn);
      wtm=!wtm;
      i=Option(tinput);
      nextc=getc(input_stream);
      if (nextc != ' ')
        if ((input_stream == stdin) && !ics)
          if (wtm) printf("force.White(%d): ",mn);
          else printf("force.Black(%d): ",mn);
      OptionGet(&equal,&slash,input,text,&more);
      move=0;
      move=InputMove(text,0,wtm,1);
      if (move) {
        if (input_stream != stdin) printf("%s\n",OutputMove(&move,0,wtm));
        fseek(history_file,((mn-1)*2+1-wtm)*10,SEEK_SET);
        fprintf(history_file,"%10s",OutputMove(&move,0,wtm));
        MakeMoveRoot(move,wtm);
        pv[0].path_iteration_depth=0;
        pv[0].path_length=0;
      }
      else if (input_stream == stdin) printf("illegal move.\n");
      wtm=!wtm;
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
    if (!wtm)printf("  ...\n");
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "hash" command controls the transposition table size.  |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("hash",input)) {
    if (thinking || pondering) return(2);
    OptionGet(&equal,&slash,input,text,&more);
    new_hash_size=atoi(text);
    if (hash_table_size) {
      free(trans_ref_w);
      free(trans_ref_b);
    }
    log_hash_table_size=new_hash_size;
    if (log_hash_table_size) {
      hash_table_size=1<<log_hash_table_size;
      trans_ref_w=malloc(16*(hash_table_size+4096));
      trans_ref_b=malloc(16*(hash_table_size+4096));
      if (!trans_ref_w || !trans_ref_b) {
       printf("malloc() failed, not enough memory.\n");
        free(trans_ref_w);
        free(trans_ref_b);
        hash_table_size=0;
        log_hash_table_size=0;
        trans_ref_w=0;
        trans_ref_b=0;
      }
      hash_mask=Mask(128-log_hash_table_size);
      Print(0,"hash table memory = %d 64-bit words.\n",
            (hash_table_size+4096)*4);
      for (i=0;i<hash_table_size;i++) {
        (trans_ref_w+i)->word1=0;
        (trans_ref_w+i)->word2=0;
        (trans_ref_b+i)->word1=0;
        (trans_ref_b+i)->word2=0;
      }
    }
    else {
      trans_ref_w=0;
      trans_ref_b=0;
      hash_table_size=0;
      log_hash_table_size=0;
    }
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "hashk" command controls the king hash table size.     |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("hashk",input)) {
    if (thinking || pondering) return(2);
    OptionGet(&equal,&slash,input,text,&more);
    new_hash_size=atoi(text);
    if (new_hash_size) {
      log_king_hash_table_size=new_hash_size;
      king_hash_table_size=1<<new_hash_size;
      king_hash_table=malloc(8*king_hash_table_size);
      if (!king_hash_table) {
       printf("malloc() failed, not enough memory.\n");
        free(king_hash_table);
        king_hash_table_size=0;
        log_king_hash_table_size=0;
        king_hash_table=0;
      }
      king_hash_mask=Mask(128-log_king_hash_table_size);
      Print(0,"king hash table memory = %d 64-bit words.\n",
        king_hash_table_size);
      for (i=0;i<king_hash_table_size;i++) *(king_hash_table+i)=0;
    }
    else {
      free(king_hash_table);
      king_hash_table_size=0;
      log_king_hash_table_size=0;
      king_hash_table=0;
    }
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
    OptionGet(&equal,&slash,input,text,&more);
    new_hash_size=atoi(text);
    if (new_hash_size) {
      log_pawn_hash_table_size=new_hash_size;
      pawn_hash_table_size=1<<new_hash_size;
      pawn_hash_table=malloc(8*pawn_hash_table_size);
      pawn_hash_table_x=malloc(4*pawn_hash_table_size);
      if (!pawn_hash_table || !pawn_hash_table_x) {
        printf("malloc() failed, not enough memory.\n");
        free(pawn_hash_table);
        free(pawn_hash_table_x);
        pawn_hash_table_size=0;
        log_pawn_hash_table_size=0;
        pawn_hash_table=0;
        pawn_hash_table_x=0;
      }
      pawn_hash_mask=Mask(128-log_pawn_hash_table_size);
      Print(0,"pawn hash table memory = %d 64-bit words.\n",
        (int) (pawn_hash_table_size*1.5));
      for (i=0;i<pawn_hash_table_size;i++) *(pawn_hash_table+i)=0;
      for (i=0;i<pawn_hash_table_size/2;i++) *(pawn_hash_table_x+i)=0;
    }
    else {
      free(pawn_hash_table);
      free(pawn_hash_table_x);
      pawn_hash_table_size=0;
      log_pawn_hash_table_size=0;
      pawn_hash_table=0;
      pawn_hash_table_x=0;
    }
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
        printf("annotate b|w|bw [moves]\n");
        printf("where b/w/bw indicates whether to annotate only the white\n");
        printf("side (w), the black side (b) or both (bw).  [moves] is\n");
        printf("optional.  If [moves] is omitted, annotate the complete\n");
        printf("game;  if present, it can be a single number which\n");
        printf("indicates the move to start annotation on, or else a\n");
        printf("range [10-25] which says start annotation at move 10 and\n");
        printf("stop after move 25.\n");
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
        printf("   search to select from the set of book moves.  <1> plays\n");
        printf("   from the set of most popular moves, but culls rarely\n");
        printf("   played moves.  <2> simply emulates the frequence of moves\n");
        printf("   played in the opening database.  <3> plays even more\n");
        printf("   randomly by compressing the frequency distribution using\n");
        printf("   the sqrt() function.  <4> simply chooses moves completely\n");
        printf("   at random from the set of book moves.\n");
        printf("book mask accept <chars>...sets the accept mask to the\n");
        printf("   flag characters in <chars> (see flags below.)  any flags\n");
        printf("   set in this mask will include either (a) moves with the \n");
        printf("   flag set, or (b) moves with no flags set.\n");
        printf("book mask reject <chars>...sets the reject mask to the\n");
        printf("   flag characters in <chars> (see flags below.)  any flags\n");
        printf("   set in this mask will reject any moves with the flag\n");
        printf("   set (in the opening book.)\n");
        printf("book played................requirest than an opening move\n");
        printf("   be played at least this percent of the time to be     \n");
        printf("   considered.\n");
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
      else if (OptionMatch("command",title)) {
        printf("[command]  commands are free-form.  for example, disable\n");
        printf(" check extensions, you can type:\n");
        printf("\n");
        printf("   extension check 0\n");
        printf("\n");
        printf(" an alternative format for commands that expect values (as\n");
        printf(" the keyword check above does) is to use the following\n");
        printf(" syntax:\n");
        printf("\n");
        printf("   extension check=0\n");
        printf("\n");
        printf(" since commands can also be entered by using the usual UNIX\n");
        printf(" command-line interface (sh, csh, ksh, etc.) this long\n");
        printf(" command can be entered as:\n");
        printf("\n");
        printf("   extension/check=0\n");
        printf("\n");
        printf(" so that it will be treated as one command (as it is\n");
        printf(" intended to be.)                                          \n");
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
      else if (OptionMatch("extension",title)) {
        printf("extension is used to enable/disable/control specific search\n");
        printf("extensions.\n");
        printf("check =     0/1 disables/enables inclusion of checking\n");
        printf("                moves in the selective portion of the\n");
        printf("                search.\n");
        printf("incheck =   0/1 disables/enables the out-of-check extension\n");
        printf("                that extends the depth one ply when in\n");
        printf("                check.\n");
        printf("pawn =      0/1 disables/enables the passed pawn pushes in\n");
        printf("                the selective portion of the search.\n");
        printf("recapture = 0/1 disables/enables the recapture extension\n");
        printf("                that extends the depth one ply when\n");
        printf("                recapturing.\n");
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
        printf("verbose  7 -> display null-move search statistics.\n");
        printf("verbose  9 -> display root moves as they are searched.\n");
        printf("verbose 11 -> display intra-iteration node counts.\n");
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
      printf("exit......................restores STDIN to key\n");
      printf("extension.................enables/disables extensions [help].\n");
      printf("end.......................terminates program.\n");
      printf("history...................display game moves.\n");
      printf("hash......................sets transposition table size (2^n)\n");
      printf("                          (one table for white, one for black\n");
      printf("hashk.....................sets king hash table size (2^n)\n");
      printf("hashp.....................sets pawn hash table size (2^n)\n");
      printf("input <filename> [title]..sets STDIN to <filename>.\n");
      printf("                          (and positions to [title] record.)\n");
      printf("log on|off|<n>............turn logging on/off (at log.n).\n");
      printf("move......................initiates search (same as go).\n");
      printf("new.......................initialize and start new game.\n");
      printf("noise n...................no status until n nodes searched.\n");
      printf("null n....................null move reduces depth by n.\n");
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
|  "ics" command is normally invoked from main() via the   |
|  ics command-line option.  it sets proper defaults for   |
|  ics/Xboard interface requirements.                      |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("ics",input)) {
    printf("Chess\n");
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
    OptionGet(&equal,&slash,input,filename,&more);
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
      (hash_table_size+4096)*4);
    Print(0,"king hash table memory = %d 64-bit words.\n",
      king_hash_table_size);
    Print(0,"pawn hash table memory = %d 64-bit words.\n",
      (int) (pawn_hash_table_size*1.5));
    if (!tc_sudden_death) {
      Print(0,"%d moves/%d minutes primary time control\n",
            tc_moves, tc_time);
      Print(0,"%d moves/%d minutes secondary time control\n",
            tc_secondary_moves, tc_secondary_time);
      if (tc_increment) Print(0,"increment %d seconds.\n",tc_increment);
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
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "kibitz" command sets kibitz mode for ICS.  =1 will    |
|   kibitz mate announcements, =2 will kibitz scores and   |
|   other info like whisper=1, and =3 will kibitz scores   |
|   and PV's just like whisper=2.  =4 adds the list of     |
|   book moves as in whisper=3.                            |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("kibitz",input)) {
    OptionGet(&equal,&slash,input,text,&more);
    kibitz=atoi(text);
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "level" command sets time controls [ics mode only.]    |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("level",input)) {
    fscanf(input_stream,"%d",&tc_moves);
    fscanf(input_stream,"%d",&tc_time);
    fscanf(input_stream,"%d",&tc_increment);
    if (!tc_moves) {
      tc_sudden_death=1;
      tc_moves=1000;
      tc_moves_remaining=1000;
    }
    if (tc_moves) {
      tc_secondary_moves=tc_moves;
      tc_secondary_time=tc_time;
    }
    if (!tc_sudden_death) {
      Print(0,"%d moves/%d minutes primary time control\n",
            tc_moves, tc_time);
      Print(0,"%d moves/%d minutes secondary time control\n",
            tc_secondary_moves, tc_secondary_time);
      if (tc_increment) Print(0,"increment %d seconds.\n",tc_increment);
    }
    else if (tc_sudden_death == 1) {
      Print(0," game/%d minutes primary time control\n",
            tc_time);
      if (tc_increment) Print(0,"increment %d seconds.\n",tc_increment);
    }
    tc_time*=60;
    tc_time_remaining=tc_time;
    tc_secondary_time*=60;
    if (!tc_increment)
      tc_simple_average_time=(tc_time-
                              tc_operator_time*tc_moves)/
                             tc_moves;
    else
      tc_simple_average_time=tc_increment-tc_operator_time;
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "log" command turns log on/off.                        |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("log",input)) {
    OptionGet(&equal,&slash,input,text,&more);
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
    }
    else {
      log_id=atoi(text);
      fclose(log_file);
      sprintf(log_filename,"%s/log.%03d",LOGDIR,log_id);
      log_file=fopen(log_filename,"w");
    }
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
    scanf("%s",opponents_name);
    Print(0,"Crafty %s vs %s\n",version,opponents_name);
    next=opponents_name;
    while (*next) {
      *next=tolower(*next);
      next++;
    }
    for (i=0;i<number_of_computers;i++)
      if (!strcmp(computer_list[i],opponents_name)) {
        book_random=2;
        book_absolute_lower_bound=2;
        kibitz=4;
        Print(1,"playing a computer!\n");
        break;
      }
    for (i=0;i<number_of_GMs;i++)
      if (!strcmp(GM_list[i],opponents_name)) {
        Print(1,"playing a GM!\n");
        book_random=2;
        resign=5;
        resign_count=6;
        draw_count=6;
        kibitz=0;
        break;
      }
    for (i=0;i<number_of_IMs;i++)
      if (!strcmp(IM_list[i],opponents_name)) {
        Print(1,"playing a IM!\n");
        book_random=3;
        resign=6;
        resign_count=8;
        draw_count=8;
        kibitz=0;
        break;
      }
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "new" command initializes for a new game.              |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("new",input)) {
    ponder_completed=0;
    ponder_move=0;
    pv[0].path_iteration_depth=0;
    pv[0].path_length=0;
    InitializeChessBoard(&position[0]);
    InitializeHashTables();
    wtm=1;
    move_number=1;
    if (log_file) fclose(log_file);
    log_id++;
    strcpy(filename,"log.");
    sprintf(text,"%d",log_id);
    strcpy(filename+4,text);
    log_file=fopen(filename,"w");
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "null" command controls the null-move search.          |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("null",input)) {
    OptionGet(&equal,&slash,input,text,&more);
    null_depth=atoi(text);
    Print(0,"null move reduces search by %d ply(s).\n",null_depth);
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
    OptionGet(&equal,&slash,input,text,&more);
    noise_level=atoi(text);
    Print(0,"noise level set to %d.\n",noise_level);
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
    OptionGet(&equal,&slash,input,text,&more);
    tc_time_remaining_opponent=atoi(text)/100;
    if (log_file) fprintf(log_file,"time remaining: %s (opponent).\n",
                          DisplayTime(tc_time_remaining_opponent*10));
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
    if (wtm)
      target=Compl(WhitePieces(0));
    else
      target=Compl(BlackPieces(0));
    first[1]=move_list;
    clock_before = clock();
    for (i=0;i<PERF_CYCLES;i++)
      last[1]=GenerateMoves(0, 1, wtm, target, 1, first[1]);
    clock_after=clock();
    time_used=((float) clock_after-(float) clock_before) / 
              (float) CLOCKS_PER_SEC;
    printf("generated %ld moves, time=%d microseconds\n",
           (last[1]-first[1])*PERF_CYCLES, (clock_after-clock_before));
    printf("generated %d moves per second\n",(int) (((float) (PERF_CYCLES*
           (last[1]-first[1])))/time_used));
    clock_before=clock();
    for (i=0;i<PERF_CYCLES;i++) {
      last[1]=GenerateMoves(0, 1, wtm, target, 1, first[1]);
      for (mv=first[1];mv<last[1];mv++)
        MakeMove(0,*mv,wtm);
    }
    clock_after=clock();
    time_used=((float) clock_after-(float) clock_before) / 
              (float) CLOCKS_PER_SEC;
    printf("generated/made %ld moves, time=%d microseconds\n",
      (last[1]-first[1])*PERF_CYCLES,
    (clock_after-clock_before));
    printf("generated/made %d moves per second\n",(int) (((float) (PERF_CYCLES*
           (last[1]-first[1])))/time_used));
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "perft" command turns tests move generator/make_move.  |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("perft",input)) {
    position[1]=position[0];
    last[0]=move_list;
    OptionGet(&equal,&slash,input,text,&more);
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
    if (pondering) return(2);
    OptionGet(&equal,&slash,input,text,&more);
    if (!strcmp(text,"on")) {
      do_ponder=1;
      Print(0,"pondering enabled.\n");
    }
    else if (!strcmp(text,"off")) {
      do_ponder=0;
      Print(0,"pondering disabled.\n");
    }
    else {
      ponder_move=InputMove(text,0,wtm,0);
    }
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "savegame" command saves the game in a file in PGN     |
|   format.  command has an optional filename.             |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("savegame",input)) {
    output_file=stdout;
    nextc=getc(input_stream);
    if (nextc == ' ') {
      fscanf(input_stream,"%s",filename);
      if (!(output_file=fopen(filename,"w"))) {
        printf("unable to open %s for write.\n",filename);
        return(1);
      }
    }
    if (wtm)
      fprintf(output_file,"[ human vs Crafty ]\n");
    else
      fprintf(output_file,"[ Crafty vs human ]\n");
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
    ponder_completed=0;
    ponder_move=0;
    last_mate_score=0;
    pv[0].path_iteration_depth=0;
    pv[0].path_length=0;
    if (thinking || pondering) return(2);
    over=0;
    OptionGet(&equal,&slash,input,text,&more);
    move_number=atoi(text);
    if (!move_number) {
      move_number=1;
      return(1);
    }
    nmoves=(move_number-1)*2+1-wtm;
    wtm=1;
    move_number=1;
    InitializeChessBoard(&position[0]);
    for (i=0;i<nmoves;i++) {
      fseek(history_file,i*10,SEEK_SET);
      fscanf(history_file,"%s",text);
      move=InputMove(text,0,wtm,0);
      if (move) {
        MakeMoveRoot(move,wtm);
      }
      else {
        printf("ERROR!  move %s is illegal\n",text);
        break;
      }
      wtm=!wtm;
      if (wtm) move_number++;
    } 
    Phase();
    last_move_in_book=move_number;
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
    if (!strcmp("reada",input))
      append=1;
    else
      append=0;
    ponder_completed=0;
    ponder_move=0;
    pv[0].path_iteration_depth=0;
    pv[0].path_length=0;
    input_file=input_stream;
    if (equal) {
      OptionGet(&equal,&slash,input,filename,&more);
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
          move=InputMove(text,0,wtm,1);
          if (move) {
            if (input_file != stdin) {
              printf("%s ",OutputMove(&move,0,wtm));
              if (!(move_number % 8) && !wtm)
                printf("\n");
            }
            fseek(history_file,((move_number-1)*2+1-wtm)*10,SEEK_SET);
            fprintf(history_file,"%10s",OutputMove(&move,0,wtm));
            MakeMoveRoot(move,wtm);
#if defined(DEBUG)
            ValidatePosition(1,move);
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
      wtm=!wtm;
      if (wtm) move_number++;
    } while (scanf_status > 0);
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
    OptionGet(&equal,&slash,input,text,&more);
    resign=atoi(text);
    if (!ics) {
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
    if (pondering) return(2);
    OptionGet(&equal,&slash,input,text,&more);
    search_move=InputMove(text,0,wtm,0);
    if (!search_move) search_move=InputMove(text,0,!wtm,0);
    if (!search_move) printf("illegal move\n");
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
    pv[0].path_iteration_depth=0;
    pv[0].path_length=0;
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
    static_eval[1]=0;
    root_wtm=!wtm;
    position[1]=position[0];
    PreEvaluate(wtm);
    s1=Material(0);
    s2=EvaluateDevelopment(1);
    s3=EvaluatePawns(1,&t1,&t2,&t3,&t4);
    s4=EvaluatePassedPawns(1,t1,t2);
    s5=EvaluatePassedPawnRacess(1,wtm,t1,t2);
    s6=EvaluateOutsidePassedPawns(1,t1,t2);
    if((TotalWhitePieces(0) > 16) &&
       (TotalBlackPieces(0) > 16)) {
      s7=-(15+(TotalBlackPieces(0)>>1))*
         EvaluateKingSafetyW(1,WhiteKingSQ(0));
      s8= (15+(TotalBlackPieces(0)>>1))*
         EvaluateKingSafetyB(1,BlackKingSQ(0));
    }
    else {
      s7=0;
      s8=0;
    }
    s9=Evaluate(1,1,-99999,99999);
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
    Print(1,"white king safety...................%s\n",
      DisplayEvaluation(s7));
    Print(1,"black king safety...................%s\n",
      DisplayEvaluation(s8));
    Print(1,"piece evaluation....................%s\n",
      DisplayEvaluation(s9-s1-s2-s3-s4-s5-s6-s7-s8));
    Print(1,"total evaluation....................%s\n",
      DisplayEvaluation(s9));
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
    OptionGet(&equal,&slash,input,text,&more);
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
    OptionGet(&equal,&slash,input,text,&more);
    if (OptionMatch("book",text)) {
      show_book=!show_book;
      if (show_book) Print(0,"show book statistics\n");
      else Print(0,"don't show book statistics\n");
    }
    if (OptionMatch("extensions",text)) {
#if defined(FAST)
      printf("Sorry, but I can't show extensions when compiled with -DFAST\n");
#else
      show_extensions=!show_extensions;
      if (show_extensions) Print(0,"show search extensions\n");
      else Print(0,"don't show search extensions\n");
#endif
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
    OptionGet(&equal,&slash,input,text,&more);
    search_time_limit=atoi(text);
    Print(0,"search time set to %d.\n",search_time_limit);
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
    OptionGet(&equal,&slash,input,filename,&more);
    if (!(input_stream=fopen(filename,"r"))) {
      printf("file does not exist.\n");
      input_stream=stdin;
    }
    else {
      Test();
      ponder_completed=0;
      ponder_move=0;
      pv[0].path_iteration_depth=0;
      pv[0].path_length=0;
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
|     time 60/30 sd/30                                     |
|                                                          |
|   this sets 60 moves in 30 minutes, then game in 30      |
|   additional minutes.  an increment can be added if      |
|   desired.                                               |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("time",input)) {
    if (ics) {
      OptionGet(&equal,&slash,input,text,&more);
      tc_time_remaining=atoi(text);
      tc_time_remaining/=100;
      if (log_file) fprintf(log_file,"time remaining: %s (crafty).\n",
                            DisplayTime(10*tc_time_remaining));
    }
    else {
      if (thinking || pondering) return(2);
      OptionGet(&equal,&slash,input,text,&more);
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
        tc_time=1800;
        tc_moves_remaining=60;
        tc_time_remaining=1800;
        tc_secondary_moves=60;
        tc_secondary_time=1800;
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
          tc_time=TtoI(next);
  
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
            tc_secondary_time=TtoI(next);
  
            next1=strchr(next,'/');
            if (!next1) break;
            next1++;
            tc_increment=atoi(next1);
          }
        } while (0);
        tc_time_remaining=tc_time;
        tc_moves_remaining=tc_moves;
        if (!tc_sudden_death) {
          Print(0,"%d moves/%d minutes primary time control\n",
                tc_moves, tc_time);
          Print(0,"%d moves/%d minutes secondary time control\n",
                tc_secondary_moves, tc_secondary_time);
          if (tc_increment) Print(0,"increment %d seconds.\n",tc_increment);
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
        tc_secondary_time*=60;
        if (!tc_increment)
          tc_simple_average_time=(tc_time-
                                  tc_operator_time*tc_moves)/
                                 tc_moves;
        else tc_simple_average_time=tc_increment-tc_operator_time;
        if (error)
          printf("usage:  time nmoves/ntime [nmoves/ntime] increment\n");
      }
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
    OptionGet(&equal,&slash,input,text,&more);
    verbosity_level=atoi(text);
    Print(0,"verbosity set to %d.\n",verbosity_level);
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "whisper" command sets whisper mode for ICS.  =1 will  |
|   whisper scores, etc., but no PV.  =2 whispers PV as    |
|   well. =3 adds the list of known book moves crafty      |
|   chose from.                                            |
|                                                          |
 ----------------------------------------------------------
*/
  else if (OptionMatch("whisper",input)) {
    OptionGet(&equal,&slash,input,text,&more);
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
    ponder_completed=0;
    ponder_move=0;
    pv[0].path_iteration_depth=0;
    pv[0].path_length=0;
    wtm=1;
    force=0;
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
void OptionGet(char **equal,char **slash,char *input,char *parameter,int *more)
{
  char *ic;

  *more=0;
  if (*slash) {
    strcpy(parameter,*slash+1);
    if ((ic=strchr(parameter,';'))) {
      *ic='\0';
      strcpy(input,ic+1);
      *more=1;
    }
    *slash=0;
  }
  else if (*equal) {
    strcpy(parameter,*equal+1);
    if ((ic=strchr(parameter,';'))) {
      *ic='\0';
      strcpy(input,ic+1);
      *more=1;
    }
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
void OptionPerft(int ply,int depth,int wtm)
{
  BITBOARD target;
  int i, *mv;

  if (wtm) target=Compl(WhitePieces(ply));
  else target=Compl(BlackPieces(ply));
  first[ply]=last[ply-1];
  last[ply]=GenerateMoves(ply, 99, wtm, target, 1, first[ply]);
  for (mv=first[ply];mv<last[ply];mv++) {
    MakeMove(ply,*mv,wtm);
    if (!Check(ply+1,wtm)) {
      if (ply <= trace_level) {
        for (i=1;i<ply;i++) printf("  ");
        printf("%s\n", OutputMove(mv,ply,wtm));
      }
      total_moves++;
      if (depth-1) OptionPerft(ply+1,depth-1,!wtm);
    }
  }
}
