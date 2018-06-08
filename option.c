#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include "types.h"
#include "evaluate.h"
#include "function.h"
#include "data.h"
#if defined(UNIX)
  #include <unistd.h>
#endif
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
  int i, mn, tmn, t1, t2, line1, line2, nmoves;
  int s1, s2, s3, s4, s5, s6, s7, s8, s9;
  int more, new_hash_size, error;
  char filename[64], title[64], input[100], text[100], onoff[10], colors[10];
  char nextc, *next, *next1;
  char *equal, *slash;
  int scanf_status, append;
  int clock_before, clock_after;
  BITBOARD target;
  float time_used;

  strcpy(input,tinput);
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
  if (Option_Match("alarm",input)) {
    Option_Get(&equal,&slash,input,onoff,&more);
    if (!strcmp(onoff,"on"))
      strcpy(audible_alarm,"\a");
    else if (!strcmp(onoff,"off"))
      strcpy(audible_alarm,"");
    else 
     printf("usage:  alarm on|off\n");
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
  else if (Option_Match("analyze",input)) {
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
        (void) Iterate(wtm);
        pondering=0;
        if (wtm)
          printf("analyze.White(%d): ",move_number);
        else
          printf("analyze.Black(%d): ",move_number);
        if (!analyze_move_read)
           scanf_status=scanf("%s",tinput);
        else
           scanf_status=1;
        if (scanf_status <= 0) break;
        move=0;
        if (((tinput[0]>='a') && (tinput[0]<='z')) ||
            ((tinput[0]>='A') && (tinput[0]<='Z'))) {
          if (!strcmp(tinput,"exit")) break;
          move=Input_Move(tinput,0,wtm,1);
          if (move) {
            fseek(history_file,((move_number-1)*2+1-wtm)*10,SEEK_SET);
            fprintf(history_file,"%10s",Output_Move(&move,0,wtm));
            if (analyze_move_read) printf("%s\n",Output_Move(&move,0,wtm));
            Make_Move_Root(move,wtm);
          }
          else if (Option_Match("back",tinput)) {
            nextc=getc(input_stream);
            if (nextc == ' ') {
              scanf("%d",&back_number);
            }
            else back_number=1;
            for (i=0;i<back_number;i++) {
              wtm=!wtm;
              if (!wtm) move_number--;
            }
            sprintf(tinput,"reset %d",move_number);
            Option(tinput);
          }
          else
            if (Option(tinput) != 1)
              printf("illegal move.\n");
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
  else if (Option_Match("annotate",input)) {
    Option_Get(&equal,&slash,input,colors,&more);
    if (more) {
      Option_Get(&equal,&slash,input,title,&more);
      if(strchr(title,'-'))
        sscanf(title,"%d-%d",&line1,&line2);
      else {
        sscanf(title,"%d",&line1);
        line2=move_number;
      }
    }
    if (!equal) {
      nextc=getc(input_stream);
      if (nextc == ' ') {
        fscanf(input_stream,"%s",title);
        if(strchr(title,'-'))
          sscanf(title,"%d-%d",&line1,&line2);
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
    Initialize_Chess_Board(&position[0]);
    wtm=1;
    move_number=1;
    do {
      if (feof(history_file)) break;
      if (move_number >= line1)
        if ((!wtm && strchr(colors,'b')) | (wtm && strchr(colors,'w'))) {
          pv[0].path_iteration_depth=0;
          thinking=1;
          position[1]=position[0];
          (void) Iterate(wtm);
          thinking=0;
        }
      fseek(history_file,((move_number-1)*2+1-wtm)*10,SEEK_SET);
      fscanf(history_file,"%s",text);
      move=Input_Move(text,0,wtm,1);
      if (!move) {
        printf("illegal move (%s) from history, aborting\n",text);
        break;
      }
      if (wtm)
        printf("White(%d): %s\n",move_number,text);
      else
        printf("Black(%d): %s\n",move_number,text);
      Make_Move_Root(move,wtm);
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
  else if (Option_Match("ansi",input)) {
    Option_Get(&equal,&slash,input,onoff,&more);
    if (!strcmp(onoff,"on"))
      ansi=1;
    else if (!strcmp(onoff,"off"))
      ansi=0;
    else 
     printf("usage:  ansi on|off\n");
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "black" command sets black to move (!wtm).             |
|                                                          |
 ----------------------------------------------------------
*/
  else if (Option_Match("black",input)) {
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
  else if (!strcmp("book",input)) {
    Book_Up("book.bin");
  }
  else if (!strcmp("books",input)) {
    Book_Up("books.bin");
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "clock" command displays chess clock.                  |
|                                                          |
 ----------------------------------------------------------
*/
  else if (Option_Match("clock",input)) {
    printf("time remaining on clock is %s.\n",
           Display_Time(tc_time_remaining*10));
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "display" command displays the chess board.            |
|                                                          |
 ----------------------------------------------------------
*/
  else if (Option_Match("display",input)) {
    nextc=getc(input_stream);
    if (nextc == ' ') {
      fscanf(input_stream,"%s",text);
      position[1]=position[0];
      Pre_Evaluate(wtm,0);
      if (Option_Match("pawn",text)) 
        Display_Piece_Boards(pawn_value_w,pawn_value_b);
      if (Option_Match("knight",text))
        Display_Piece_Boards(knight_value_w,knight_value_b);
      if (Option_Match("bishop",text))
        Display_Piece_Boards(bishop_value_w,bishop_value_b);
      if (Option_Match("queen",text))
        Display_Piece_Boards(queen_value_w,queen_value_b);
      if (Option_Match("king",text))
        Display_Piece_Boards(king_value_w,king_value_b);
    }
    else
      Display_Chess_Board(stdout,position[0].board);
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "draw" sets the default draw score (which is forced to |
|     zero when the endgame is reached.)                   |
|                                                          |
 ----------------------------------------------------------
*/
  else if (Option_Match("draw",input)) {
    Option_Get(&equal,&slash,input,text,&more);
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
  else if (Option_Match("echo",input)) {
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
  else if (Option_Match("edit",input) &&
           strcmp(input,"ed")) {
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
  else if (Option_Match("end",input) || Option_Match("quit",input)) {
    if (thinking || pondering) return(2);
    fclose(book_file);
    fclose(history_file);
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
  else if (Option_Match("exit",input)) {
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
  else if (Option_Match("extension",input)) {
    Option_Get(&equal,&slash,input,text,&more);
    if (Option_Match("check",text)) {
      Option_Get(&equal,&slash,input,text,&more);
      check_extensions=atoi(text);
      if (check_extensions)
        Print(0,"check extensions enabled.\n");
      else
        Print(0,"check extensions disabled.\n");
    }
    else if (Option_Match("qcheck",text)) {
      Option_Get(&equal,&slash,input,text,&more);
      quiescence_checks=atoi(text);
      if (quiescence_checks)
        Print(0,"quiescence check extensions = %d.\n",quiescence_checks);
      else
        Print(0,"quiescence check extensions disabled.\n");
    }
    else if (Option_Match("recapture",text)) {
      Option_Get(&equal,&slash,input,text,&more);
      recapture_extensions=atoi(text);
      if (recapture_extensions)
        Print(0,"recapture extensions enabled.\n");
      else
        Print(0,"recapture extensions disabled.\n");
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
  else if (Option_Match("force",input)) {
    ponder_completed=0;
    ponder_move=0;
    pv[0].path_iteration_depth=0;
    pv[0].path_length=0;
    if (ics)
      force=1;
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
          if (wtm)
            printf("force.White(%d): ",mn);
          else
            printf("force.Black(%d): ",mn);
      Option_Get(&equal,&slash,input,text,&more);
      move=0;
      move=Input_Move(text,0,wtm,1);
      if (move) {
        if (input_stream != stdin)
          printf("%s\n",Output_Move(&move,0,wtm));
        fseek(history_file,((mn-1)*2+1-wtm)*10,SEEK_SET);
        fprintf(history_file,"%10s",Output_Move(&move,0,wtm));
        Make_Move_Root(move,wtm);
        pv[0].path_iteration_depth=0;
        pv[0].path_length=0;
      }
      else {
        if (input_stream == stdin)
          printf("illegal move.\n");
      }
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
  else if (Option_Match("history",input)) {
   printf("    white       black\n");
    for (i=0;i<(move_number-1)*2-wtm+1;i++) {
      fseek(history_file,i*10,SEEK_SET);
      fscanf(history_file,"%s",text);
      if (!(i%2))
        printf("%3d",i/2+1);
      printf("  %-10s",text);
      if (i%2 == 1)
        printf("\n");
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
  else if (Option_Match("hash",input)) {
    if (thinking || pondering) return(2);
    Option_Get(&equal,&slash,input,text,&more);
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
  else if (Option_Match("hashk",input)) {
    if (thinking || pondering) return(2);
    Option_Get(&equal,&slash,input,text,&more);
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
      for (i=0;i<king_hash_table_size;i++) {
        *(king_hash_table+i)=0;
      }
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
  else if (Option_Match("hashp",input)) {
    if (thinking || pondering) return(2);
    Option_Get(&equal,&slash,input,text,&more);
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
      for (i=0;i<pawn_hash_table_size;i++) {
        *(pawn_hash_table+i)=0;
      }
      for (i=0;i<pawn_hash_table_size/2;i++) {
        *(pawn_hash_table_x+i)=0;
      }
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
  else if (Option_Match("help",input)) {
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
        printf("   Crafty's style of play.  <1> emulates the frequency\n");
        printf("   opening lines are played by top GM players.  <2> increases\n");
        printf("   randomness somewhat, and <3> is completely random from the\n");
        printf("   big book only.\n");
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
      else if (Option_Match("command",title)) {
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
      else if (Option_Match("edit",title)) {
        printf("edit is used to set up or modify a board position.  it \n");
        printf("recognizes 4 \"commands\" that it uses to alter/set up the\n");
        printf("board position (with appropriate aliases to interface with\n");
        printf("xboard.\n");
        printf("\n");
        printf("# command causes edit to clear the chess board\n");
        printf("c command causes edit to toggle piece color.\n");
        printf("white command causes edit to place white pieces on the\n");
        printf("board; black command causes edit to place black pieces on\n");
        printf("the board.\n");
        printf("end (\".\" for xboard) causes edit to exit.\n");
        printf("\n");
        printf("three strings of the form [piece][file][rank] will\n");
        printf("place a [piece] on square [file][rank].  the color is set\n");
        printf("by the previous white/black command.  ex:  Ke8 puts a king\n");
        printf("on square e8\n");
        printf("\n");
      }
      else if (Option_Match("extension",title)) {
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
      else if (Option_Match("setboard",title)) {
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
      printf("d.........................displays chess board.\n");
      printf("echo......................echos output to display.\n");
      printf("edit......................edit board position. [help]\n");
      printf("exit......................restores STDIN to keyboard.\n");
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
      printf("setboard..................sets board position. [help]\n");
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
  else if (Option_Match("ics",input)) {
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
  else if (Option_Match("input",input)) {
    if (thinking || pondering) return(2);
    title[0]='\0';
    Option_Get(&equal,&slash,input,filename,&more);
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
  else if (Option_Match("info",input)) {
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
|   and PV's just like whisper=2.                          |
|                                                          |
 ----------------------------------------------------------
*/
  else if (Option_Match("kibitz",input)) {
    Option_Get(&equal,&slash,input,text,&more);
    kibitz=atoi(text);
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "level" command sets time controls [ics mode only.]    |
|                                                          |
 ----------------------------------------------------------
*/
  else if (Option_Match("level",input)) {
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
  else if (Option_Match("log",input)) {
    Option_Get(&equal,&slash,input,text,&more);
    if (!strcmp(text,"on"))
      log_file=fopen("log","w");
    else if (!strcmp(text,"off")) {
      fclose(log_file);
      log_file=0;
    }
    else {
      log_id=atoi(text);
      fclose(log_file);
      sprintf(filename,"log.%d",log_id);
      log_file=fopen(filename,"w");
    }
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "mvv_lva" command controls the capture search move     |
|   ordering.  non-zero searches all captures in MVV/LVA   |
|   (Most Valuable Victim / Least Valuable Aggressor)      |
|   order, zero uses normal SEE (Static Exchange Evaluate) |
|   ordering which also culls losing captures in q-search. |
|                                                          |
 ----------------------------------------------------------
*/
  else if (Option_Match("mvv_lva",input)) {
    Option_Get(&equal,&slash,input,text,&more);
    mvv_lva_ordering=atoi(text);
    if (mvv_lva_ordering)
      printf("MVV/LVA move ordering enabled.\n");
    else
      printf("SEE move ordering enabled.\n");
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "name" command saves opponents name and writes it into |
|   logfile along with the date/time.                      |
|                                                          |
 ----------------------------------------------------------
*/
  else if (Option_Match("name",input)) {
    char computer_names[] = {"nowx zarkovx ferret kerrigan geniuspentium fitter\
                              fritzpentium wchessx"};
    scanf("%s",opponents_name);
    Print(0,"Crafty %s vs %s\n",version,opponents_name);
    next=opponents_name;
    while (*next) {
      *next=tolower(*next);
      next++;
    }
    if (strstr(computer_names,opponents_name)) {
      book_random=0;
      kibitz=3;
      Print(1,"playing a computer!\n");
    }
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "new" command initializes for a new game.              |
|                                                          |
 ----------------------------------------------------------
*/
  else if (Option_Match("new",input)) {
    ponder_completed=0;
    ponder_move=0;
    pv[0].path_iteration_depth=0;
    pv[0].path_length=0;
    Initialize_Chess_Board(&position[0]);
    Initialize_Hash_Tables();
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
  else if (Option_Match("null",input)) {
    Option_Get(&equal,&slash,input,text,&more);
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
  else if (Option_Match("noise",input)) {
    Option_Get(&equal,&slash,input,text,&more);
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
  else if (Option_Match("otime",input)) {
    Option_Get(&equal,&slash,input,text,&more);
    tc_time_remaining_opponent=atoi(text)/100;
    if (log_file) fprintf(log_file,"time remaining: %s (opponent).\n",
                          Display_Time(tc_time_remaining_opponent*10));
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "perf" command turns times move generator/make_move.   |
|                                                          |
 ----------------------------------------------------------
*/
#define PERF_CYCLES 100000
  else if (Option_Match("perf",input)) {
    if (wtm)
      target=Compl(White_Pieces(0));
    else
      target=Compl(Black_Pieces(0));
    first[1]=move_list;
    clock_before = clock();
    for (i=0;i<PERF_CYCLES;i++)
      last[1]=Generate_Moves(0, 1, wtm, target, 1, first[1]);
    clock_after=clock();
    time_used=((float) clock_after-(float) clock_before) / 
              (float) CLOCKS_PER_SEC;
    printf("generated %ld moves, time=%d microseconds\n",
           (last[1]-first[1])*PERF_CYCLES, (clock_after-clock_before));
    printf("generated %d moves per second\n",(int) (((float) (PERF_CYCLES*
           (last[1]-first[1])))/time_used));
    clock_before=clock();
    for (i=0;i<PERF_CYCLES;i++) {
      last[1]=Generate_Moves(0, 1, wtm, target, 1, first[1]);
      for (mv=first[1];mv<last[1];mv++)
        Make_Move(0,*mv,wtm);
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
|   "ponder" command toggles pondering off/on or sets a    |
|   move to ponder.                                        |
|                                                          |
 ----------------------------------------------------------
*/
  else if (Option_Match("ponder",input)) {
    if (pondering) return(2);
    Option_Get(&equal,&slash,input,text,&more);
    if (!strcmp(text,"on")) {
      do_ponder=1;
      Print(0,"pondering enabled.\n");
    }
    else if (!strcmp(text,"off")) {
      do_ponder=0;
      Print(0,"pondering disabled.\n");
    }
    else {
      ponder_move=Input_Move(text,0,wtm,0);
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
  else if (Option_Match("savegame",input)) {
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
  else if (Option_Match("reset",input)) {
    ponder_completed=0;
    ponder_move=0;
    pv[0].path_iteration_depth=0;
    pv[0].path_length=0;
    if (thinking || pondering) return(2);
    over=0;
    Option_Get(&equal,&slash,input,text,&more);
    move_number=atoi(text);
    if (!move_number) {
      move_number=1;
      return(1);
    }
    nmoves=(move_number-1)*2+1-wtm;
    wtm=1;
    move_number=1;
    Initialize_Chess_Board(&position[0]);
    for (i=0;i<nmoves;i++) {
      fseek(history_file,i*10,SEEK_SET);
      fscanf(history_file,"%s",text);
      move=Input_Move(text,0,wtm,0);
      if (move) {
        Make_Move_Root(move,wtm);
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
  else if (Option_Match("read",input) ||
           Option_Match("reada",input)) {
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
      Option_Get(&equal,&slash,input,filename,&more);
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
      Initialize_Chess_Board(&position[0]);
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
          move=Input_Move(text,0,wtm,1);
          if (move) {
            if (input_file != stdin) {
              printf("%s ",Output_Move(&move,0,wtm));
              if (!(move_number % 8) && !wtm)
                printf("\n");
            }
            fseek(history_file,((move_number-1)*2+1-wtm)*10,SEEK_SET);
            fprintf(history_file,"%10s",Output_Move(&move,0,wtm));
            Make_Move_Root(move,wtm);
#if defined(DEBUG)
            Validate_Position(1,move);
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
  else if (Option_Match("resign",input)) {
    Option_Get(&equal,&slash,input,text,&more);
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
  else if (Option_Match("search",input)) {
    if (pondering) return(2);
    Option_Get(&equal,&slash,input,text,&more);
    search_move=Input_Move(text,0,wtm,0);
    if (!search_move) search_move=Input_Move(text,0,!wtm,0);
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
  else if (Option_Match("setboard",input)) {
    if (thinking || pondering) return(2);
    Set_Board();
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
  else if (Option_Match("score",input)) {
    positional_evaluation[1]=0;
    root_wtm=!wtm;
    position[1]=position[0];
    Pre_Evaluate(wtm,0);
    s1=Material(0);
    s2=Evaluate_Development(1);
    s3=Evaluate_Pawns(1,&t1,&t2);
    s4=Evaluate_Passed_Pawns(1,t1,t2);
    s5=Evaluate_Passed_Pawn_Races(1,wtm,t1,t2);
    s6=Evaluate_Outside_Passed_Pawns(1,t1,t2);
    if((Total_White_Pieces(0) > 16) &&
       (Total_Black_Pieces(0) > 16)) {
      s7=KING_SAFETY*Evaluate_King_Safety_W(1,White_King_SQ(0));
      s8=-KING_SAFETY*Evaluate_King_Safety_B(1,Black_King_SQ(0));
    }
    else {
      s7=0;
      s8=0;
    }
    if (root_wtm)
      s7=s7*2;
    else
      s8=s8*2;
    s9=Evaluate(1,1,-99999,99999);
    printf("note: scores are for the white side\n");
    printf("material evaluation.................%s\n",
      Display_Evaluation(s1));
    printf("development.........................%s\n",
      Display_Evaluation(s2));
    printf("pawn evaluation.....................%s\n",
      Display_Evaluation(s3));
    printf("passed pawn evaluation..............%s\n",
      Display_Evaluation(s4));
    printf("passed pawn race evaluation.........%s\n",
      Display_Evaluation(s5));
    printf("outside passed pawn evaluation......%s\n",
      Display_Evaluation(s6));
    printf("white king safety...................%s\n",
      Display_Evaluation(s7));
    printf("black king safety...................%s\n",
      Display_Evaluation(s8));
    printf("piece evaluation....................%s\n",
      Display_Evaluation(s9-s1-s2-s3-s4-s5-s6-s7-s8));
    printf("total evaluation....................%s\n",
      Display_Evaluation(s9));
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "sd" command sets a specific search depth to control   |
|   the tree search depth.                                 |
|                                                          |
 ----------------------------------------------------------
*/
  else if (Option_Match("sd",input)) {
    Option_Get(&equal,&slash,input,text,&more);
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
  else if (Option_Match("show",input)) {
    Option_Get(&equal,&slash,input,text,&more);
    if (Option_Match("book",text)) {
      show_book=!show_book;
      if (show_book)
        Print(0,"show book statistics\n");
      else
        Print(0,"don't show book statistics\n");
    }
    if (Option_Match("extensions",text)) {
#if defined(FAST)
      printf("Sorry, but I can't show extensions when compiled with -DFAST\n");
#else
      show_extensions=!show_extensions;
      if (show_extensions)
        Print(0,"show search extensions\n");
      else
        Print(0,"don't show search extensions\n");
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
  else if (Option_Match("st",input)) {
    Option_Get(&equal,&slash,input,text,&more);
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
  else if (Option_Match("test",input)) {
    Option_Get(&equal,&slash,input,filename,&more);
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
  else if (Option_Match("time",input)) {
    if (ics) {
      Option_Get(&equal,&slash,input,text,&more);
      tc_time_remaining=atoi(text);
      tc_time_remaining/=100;
      if (log_file) fprintf(log_file,"time remaining: %s (crafty).\n",
                            Display_Time(10*tc_time_remaining));
    }
    else {
      if (thinking || pondering) return(2);
      Option_Get(&equal,&slash,input,text,&more);
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
            else
              tc_secondary_moves=atoi(next1);
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
        else
          tc_simple_average_time=tc_increment-tc_operator_time;
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
  else if (Option_Match("trace",input)) {
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
  else if (Option_Match("verbose",input)) {
    Option_Get(&equal,&slash,input,text,&more);
    verbosity_level=atoi(text);
    Print(0,"verbosity set to %d.\n",verbosity_level);
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "whisper" command sets whisper mode for ICS.  =1 will  |
|   whisper scores, etc., but no PV.  =2 whispers PV as    |
|   well.                                                  |
|                                                          |
 ----------------------------------------------------------
*/
  else if (Option_Match("whisper",input)) {
    Option_Get(&equal,&slash,input,text,&more);
    whisper=atoi(text);
  }
/*
 ----------------------------------------------------------
|                                                          |
|   "white" command sets white to move (wtm).              |
|                                                          |
 ----------------------------------------------------------
*/
  else if (Option_Match("white",input)) {
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
*   Option_Get() returns the next value from the input string.                 *
*                                                                              *
********************************************************************************
*/
void Option_Get(char **equal,char **slash,char *input,char *parameter,int *more)
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
  else
    fscanf(input_stream,"%s",parameter);
}

/*
********************************************************************************
*                                                                              *
*   Option_Match() is used to recognize user commands.  it requires that the   *
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
int Option_Match(char *command, char *input)
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
      for (j=0;j<(int) strlen(bad);j++)
        if (bad[j] == input[i]) break;
      if ((j=strlen(bad))) return(1);
    }
  }
  return(0);
}
