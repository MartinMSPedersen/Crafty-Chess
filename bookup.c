#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include "types.h"
#include "function.h"
#include "data.h"
#if defined(UNIX)
#  include <unistd.h>
#endif
#define SORT_BLOCKSIZE 200000
#define MERGE_BLOCKSIZE 1000
/*
********************************************************************************
*                                                                              *
*   Book_Up() is used to create/add to the opening book file.  typing "book    *
*   create" will erase the old book file and start from scratch, typing "book  *
*   add" will simply add more moves to the existing file.                      *
*                                                                              *
*   the format of the input data is a left bracket ("[") followed by any title *
*   information desired, followed by a right bracket ("]") followed by a       *
*   sequence of moves.  the sequence of moves is assumed to start at ply=1,    *
*   with white-to-move (normal opening position) and can contain as many moves *
*   as desired (no limit on the depth of each variation.)  the file *must* be  *
*   terminated with a line that begins with "end", since handling the EOF      *
*   condition makes portable code difficult.                                   *
*                                                                              *
*   book moves can either be typed in by hand, directly into book_add(), by    *
*   using the "book create/add" command.  using the command "book add/create   *
*   filename" will cause book_add() to read its opening text moves from        *
*   filename rather than from the keyboard.                                    *
*                                                                              *
*   in addition to the normal text for a move (reduced or full algebraic is    *
*   accepted, ie, e4, ed, exd4, e3d4, etc. are all acceptable) some special    *
*   characters can be appended to a move.  the move must be immediately        *
*   followed by either a "/" or a "\" followed by one or more of the following *
*   mask characters with no intervening spaces.  if "/" preceeds the flags,    *
*   the flags are added (or'ed) to any already existing flags that might be    *
*   from other book variations that pass through this position.  if "\" is     *
*   used, these flags replace any existing flags, which is the easy way to     *
*   clear incorrect flags and/or replace them with new ones.                   *
*                                                                              *
*      ?? ->  never play this move.  since the same book is used for both      *
*             black and white, you can enter moves in that white might play,   *
*             but prevent the program from choosing them on its own.           *
*      ?  ->  avoid this move except for non-important games.  these openings  *
*             are historically those that the program doesn't play very well,  *
*             but which aren't outright losing.                                *
*      =  ->  drawish move, only play this move if drawish moves are allowed   *
*             by the operator.  this is used to encourage the program to play  *
*             drawish openings (Petrov's comes to mind) when the program needs *
*             to draw or is facing a formidable opponent (deep thought comes   *
*             to mind.)                                                        *
*      !  ->  always play this move, if there isn't a move with the !! flag    *
*             set also.  this is a strong move, but not as strong as a !!      *
*             moveing traps.                                                   *
*      !! ->  always play this move.  this can be used to make the program     *
*             favor particular lines, or to mark a strong move for certain     *
*             opening traps.                                                   *
*     0-f ->  16 user-defined flags.  the program will ignore these flags      *
*             unless the operator sets the "book mask" to contain them which   *
*             will "require" the program to play from the set of moves with    *
*             at least one of the flags in "book mask" set.                    *
*                                                                              *
********************************************************************************
*/
void Book_Up(char output_filename[])
{
  BOOK_POSITION *buffer;
  int move;
  BITBOARD temp_hash_key;
  FILE *book_input, *output_file;
  char flags[40], fname[64], text[30], nextc, which_mask[20];
  int i, mask_word, total_moves;
  int move_num, wtm, book_positions, rarely_played;
  int cluster, max_cluster, discarded, errors, data_read;
  int start_cpu_time, start_elapsed_time, following, ply, max_ply=256;
  int files, buffered=0;
  BOOK_POSITION current, next;
  int last, cluster_seek, next_cluster;
  int counter, *index, start;

/*
 ----------------------------------------------------------
|                                                          |
|   determine if we should read the book moves from a file |
|   or from the operator (which is normally used to add/   |
|   delete moves just before a game.)                      |
|                                                          |
 ----------------------------------------------------------
*/
  Initialize_Chess_Board(&position[1]);
  scanf("%s",text);
  book_input=input_stream;
  if (!strcmp(text,"add") || !strcmp(text,"create")) {
    nextc=getc(input_stream);
    if (nextc == ' ') {
      data_read=fscanf(input_stream,"%s",fname);
      if (!(book_input=fopen(fname,"r"))) {
        printf("file %s does not exist.\n",fname);
        return;
      }
      nextc=getc(input_stream);
      if (nextc == ' ') {
        data_read=fscanf(input_stream,"%d",&max_ply);
      }
    }
  }
/*
 ----------------------------------------------------------
|                                                          |
|   open the correct book file for writing/reading         |
|                                                          |
 ----------------------------------------------------------
*/
  if (!strcmp(text,"create")) {
    if (book_file) fclose(book_file);
    book_file=fopen(output_filename,"wb+");
    buffer=(BOOK_POSITION *) malloc(sizeof(BOOK_POSITION)*100);
    if (!buffer) {
      printf("not enough memory for buffer.\n");
      return;
    }
    fseek(book_file,0,SEEK_SET);
  }
  else if (!strcmp(text,"off")) {
    if (book_file) fclose(book_file);
    book_file=0;
    books_file=0;
    Print(0,"book file disabled.\n");
    return;
  }
  else if (!strcmp(text,"on")) {
    if (!book_file)
      book_file=fopen("book.bin","rb+");
      books_file=fopen("books.bin","rb+");
    Print(0,"book file enabled.\n");
    return;
  }
  else if (!strcmp(text,"played")) {
    scanf("%f",&book_min_percent_played);
    book_min_percent_played/=100.0;
    printf("moves must be played at least %6.2f to be used.\n",
           book_min_percent_played*100);
    return;
  }
  else if (!strcmp(text,"random")) {
    data_read=fscanf(input_stream,"%d",&book_random);
    switch (book_random) {
      case 0: 
        Print(0,"play best book lines.\n");
        break;
      case 1:
        Print(0,"play most popular book lines.\n");
        break;
      case 2:
        Print(0,"play most popular book lines, but vary more.\n");
        break;
      case 3:
        Print(0,"play random book lines.\n");
        break;
      default:
        Print(0,"valid options are 0-3.\n");
        break;
    }
    return;
  }
  else if (!strcmp(text,"mask")) {
    data_read=fscanf(input_stream,"%s",which_mask);
    data_read=fscanf(input_stream,"%s",flags);
    if (!strcmp(which_mask,"accept")) {
      book_accept_mask=Book_Mask(flags);
      book_reject_mask=book_reject_mask & ~book_accept_mask;
    }
    else if (!strcmp(which_mask,"reject")) {
      book_reject_mask=Book_Mask(flags);
      book_accept_mask=book_accept_mask & ~book_reject_mask;
    }
    else {
      printf("usage:  book mask accept|reject <chars>\n");
    }
    return;
  }
  else if (!strcmp(text,"lower")) {
    data_read=fscanf(input_stream,"%d",&book_lower_bound);
    return;
  }
  else {
    printf("usage:  book edit/create/off [filename]\n");
    return;
  }
/*
 ----------------------------------------------------------
|                                                          |
|   now, read in a series of moves (terminated by the "["  |
|   of the next title or by "end" for end of the file)     |
|   and make them.  after each Make_Move(), we can grab    |
|   the hash key, and use it to access the book data file  |
|   to add this position.  note that we have to check the  |
|   last character of a move for the special flags and     |
|   set the correct bit in the status for this position.   |
|   when we reach the end of a book line, we back up to    |
|   the starting position and start over.                  |
|                                                          |
 ----------------------------------------------------------
*/
  printf("parsing pgn move file (1000 moves/dot)\n");
  start_cpu_time=Get_Time(cpu);
  start_elapsed_time=Get_Time(elapsed);
  if (book_file) {
    total_moves=0;
    max_search_depth=0;
    discarded=0;
    errors=0;
    if (book_input == stdin) {
      printf("enter book text, first line must be [title information],\n");
      printf("type \"end\" to exit.\n");
      printf("title(1): ");
    }
    do 
      data_read=fscanf(book_input,"%s",text);
    while ((text[0] != '[') && strcmp(text,"end") && (data_read>0));
    do {
      if (book_input != stdin) 
        if (verbosity_level) Print(1,"%s ", text);
      while ((text[strlen(text)-1] != ']') && 
             strcmp(text,"end") && (data_read>0)) {
        data_read=fscanf(book_input,"%s",text);
        if ((book_input != stdin) && verbosity_level) Print(1,"%s ",text);
      }
      if ((book_input != stdin) && verbosity_level) Print(1,"\n");
      if (!strcmp(text,"end") || (data_read<=0)) break;
      wtm=1;
      move_num=1;
      position[2]=position[1];
      ply=0;
      following=1;
      while (data_read>0) {
        if (book_input == stdin)
          if (wtm)
            printf("White_Pieces(%d): ",move_num);
          else
            printf("Black_Pieces(%d): ",move_num);
        do
          data_read=fscanf(book_input,"%s",text);
        while ((text[0] >= '0') && (text[00] <= '9') && (data_read>0));
        mask_word=0;
        if (!strcmp(text,"end") || (data_read<=0)) {
          if (book_input != stdin) fclose(book_input);
          break;
        }
        else if (strchr(text,'/')) {
          strcpy(flags, strchr(text,'/')+1);
          *strchr(text,'/')='\0';
          mask_word=Book_Mask(flags);
        }
        else {
          for (i=0;i<(int)strlen(text);i++)
            if (strchr("?!",text[i])) {
              strcpy(flags,&text[i]);
              text[i]='\0';
              mask_word=Book_Mask(flags);
              break;
            }
        }
        if (text[0] == '[') break;
        if (!strchr(text,'$') && !strchr(text,'*')) {
          move=Input_Move(text,2,wtm,0);
          if (move) {
            ply++;
            max_search_depth=Max(max_search_depth,ply);
            total_moves++;
            if (!(total_moves % 1000)) {
              printf(".");
              if (!(total_moves % 60000)) printf(" (%d)\n",total_moves);
              fflush(stdout);
            }
            Make_Move(2,move,wtm);
            position[2]=position[3];
            if ((ply <= max_ply) || (following &&
                                     (Captured(move) ||
                                      Promote(move)))) {
              temp_hash_key=Xor(Hash_Key(2),
                enpassant_random[First_One(EnPassant_Target(2))]);
              temp_hash_key=Xor(temp_hash_key,
                          castle_random_w[(int) White_Castle(2)]);
              temp_hash_key=Xor(temp_hash_key,
                          castle_random_b[(int) Black_Castle(2)]);
              temp_hash_key=Xor(temp_hash_key,wtm_random[wtm]);
              buffer[buffered].position=temp_hash_key;
              buffer[buffered++].status=Shiftl((BITBOARD) mask_word,32);
              if (buffered > 99) {
                fwrite(buffer,sizeof(BOOK_POSITION),100,book_file);
                buffered=0;
              }
            }
            else {
              following=0;
              discarded++;
            }
          }
          else {
            errors++;
            Print(0,"ERROR!  move %d: %s is illegal\n",move_num,text);
            Display_Chess_Board(stdout,position[2].board);
            break;
          }
          wtm=!wtm;
          if (wtm) move_num++;
        } 
      }
    } while (strcmp(text,"end") && (data_read>0));
    if (buffered) {
      fwrite(buffer,sizeof(BOOK_POSITION),buffered,book_file);
    }
/*
 ----------------------------------------------------------
|                                                          |
|   done, now we have to sort this mess into ascending     |
|   order, move by move, to get moves that belong in the   |
|   same record adjacent to each other so we can "collapse |
|   and count" easily.                                     |
|                                                          |
 ----------------------------------------------------------
*/
    printf("\nsorting %d moves (%dK/dot) ",total_moves,SORT_BLOCKSIZE/1000);
    fflush(stdout);
    free(buffer);
    buffer=(BOOK_POSITION *) malloc(sizeof(BOOK_POSITION)*SORT_BLOCKSIZE);
    fseek(book_file,0,SEEK_SET);
    data_read=SORT_BLOCKSIZE;
    for (files=1;files < 10000; files++) {
      if (data_read < SORT_BLOCKSIZE) break;
      data_read=fread(buffer,sizeof(BOOK_POSITION),SORT_BLOCKSIZE,book_file);
      if (!data_read) break;
      qsort((char *) buffer,data_read,sizeof(BOOK_POSITION),Book_Up_Compare);
      sprintf(fname,"sort.%d",files);
      if(!(output_file=fopen(fname,"wb+"))) 
        printf("ERROR.  unable to open sort output file\n");
      fwrite(buffer,sizeof(BOOK_POSITION),data_read,output_file);
      fclose(output_file);
      if (files%10) printf(".");
      else printf("(%d)",files);
      fflush(stdout);
    }
    fclose(book_file);
    free(buffer);
    printf("<done>\n");
/*
 ----------------------------------------------------------
|                                                          |
|   now merge these "chunks" into book.bin, keeping all of |
|   the "flags" as well as counting the number of times    |
|   that each move was played.                             |
|                                                          |
 ----------------------------------------------------------
*/
    printf("merging sorted files (%d) (10K/dot)\n",files-1);
    start=strcmp(output_filename,"book.bin");
    counter=0;
    index=malloc(32768*sizeof(int));
    for (i=0;i<32768;i++) index[i]=-1;
    current=Book_Up_Next_Position(files);
    if (start)
      current.status=100;
    else
      current.status++;
    book_file=fopen(output_filename,"wb+");
    fseek(book_file,sizeof(int)*32768,SEEK_SET);
    last=current.position>>49;
    index[last]=ftell(book_file);
    book_positions=0;
    rarely_played=0;
    cluster=0;
    cluster_seek=sizeof(int)*32768;
    fseek(book_file,cluster_seek+sizeof(int),SEEK_SET);
    max_cluster=0;
    while (1) {
      next=Book_Up_Next_Position(files);
      counter++;
      if (counter%10000 == 0) {
        printf(".");
        if (counter%600000 == 0) printf("(%d)\n",counter);
        fflush(stdout);
      }
      if (current.position == next.position) {
        if (!start) current.status++;
        current.status=Or(current.status,next.status);
      }
      else {
        book_positions++;
        cluster++;
        max_cluster=Max(max_cluster,cluster);
        fwrite(&current,sizeof(BOOK_POSITION),1,book_file);
        if (next.position == 0) break;
        if (last != (next.position>>49)) {
          next_cluster=ftell(book_file);
          fseek(book_file,cluster_seek,SEEK_SET);
          fwrite(&cluster,sizeof(int),1,book_file);
          fseek(book_file,next_cluster+sizeof(int),SEEK_SET);
          cluster_seek=next_cluster;
          last=next.position>>49;
          index[last]=next_cluster;
          cluster=0;
        }
        if (And(current.status,mask_32) < book_lower_bound) rarely_played++;
        current=next;
        if (start)
          current.status=100;
        else
          current.status++;
      }
      if (next.position == 0) break;
    }
    fseek(book_file,0,SEEK_SET);
    fwrite(index,sizeof(int),32768,book_file);
/*
 ----------------------------------------------------------
|                                                          |
|   now clean up, remove the sort.n files, and print the   |
|   statistics for building the book.                      |
|                                                          |
 ----------------------------------------------------------
*/
    for (i=1;i<files;i++) {
      sprintf(fname,"sort.%d",i);
      remove(fname);
    }
    start_cpu_time=Get_Time(cpu)-start_cpu_time;
    start_elapsed_time=Get_Time(elapsed)-start_elapsed_time;
    Print(0,"\n\nparsed %d moves.\n",total_moves);
    Print(0,"found %d errors during parsing.\n",errors);
    Print(0,"discarded %d moves (maxply=%d).\n",discarded,max_ply);
    Print(0,"book contains %d unique positions.\n",book_positions);
    Print(0,"deepest book line was %d plies.\n",max_search_depth);
    Print(0,"longest cluster of moves was %d.\n",max_cluster);
    Print(0,"%d positions were played less than %d times.\n",
          rarely_played, book_lower_bound);
    Print(0,"time used:  %s cpu  ", Display_Time(start_cpu_time));
    Print(0,"  %s elapsed.\n", Display_Time(start_elapsed_time));
  }
}

/*
********************************************************************************
*                                                                              *
*   Book_Mask() is used to convert the flags for a book move into a 32 bit     *
*   mask that is either kept in the file, or is set by the operator to select  *
*   which opening(s) the program is allowed to play.                           *
*                                                                              *
********************************************************************************
*/
int Book_Mask(char *flags)
{
  int f, i, mask;
  mask=0;
  for (i=0;i<(int) strlen(flags);i++) {
    if (flags[i] == '*')
      mask=-1;
    else if (flags[i] == '?') {
      if (flags[i+1] == '?') {
        mask=mask | 1;
        i++;
      }
      else
        mask=mask | 2;
    }
    else if (flags[i] == '=') {
      mask=mask | 4;
    }
    else if (flags[i] == '!') {
      if (flags[i+1] == '!') {
        mask=mask | 16;
        i++;
      }
      else
        mask=mask | 8;
    }
    else {
      f=flags[i]-'0';
      if ((f >= 0) && (f <= 9))
        mask=mask | (1 << (f+8));
      else {
        f=flags[i]-'a';
        if ((f >= 0) && (f <= 5))
          mask=mask | (1 << (f+18));
        else {
          f=flags[i]-'A';
          if ((f >= 0) && (f <= 5))
            mask=mask | (1 << (f+18));
          else
            printf("unknown book mask character -%c- ignored.\n",flags[i]);
        }
      }
    }
  }
  return(mask);
}

/*
********************************************************************************
*                                                                              *
*   Book_Up_Next_Position() is the heart of the "merge" operation that is done *
*   after the chunks of the parsed/hashed move file are sorted.  this code     *
*   opens the sort.n files, and returns the least (lexically) position key to  *
*   counted/merged into the main book database.                                *
*                                                                              *
********************************************************************************
*/
BOOK_POSITION Book_Up_Next_Position(int files)
{
  static int init=0;
  char fname[20];
  static FILE *input_file[100];
  static BOOK_POSITION *buffer[100];
  static int data_read[100], next[100];
  int i, used;
  BOOK_POSITION least;
  if (!init) {
    init=1;
    for (i=1;i<files;i++) {
      sprintf(fname,"sort.%d",i);
      if (!(input_file[i]=fopen(fname,"rb"))) {
        printf("unable to open sort.%d file, may be too many files open.\n",i);
      }
      buffer[i]=malloc(sizeof(BOOK_POSITION)*MERGE_BLOCKSIZE);
      if (!buffer[i]) {
        printf("out of memory.  aborting. \n");
        exit(1);
      }
      fseek(input_file[i],0,SEEK_SET);
      data_read[i]=fread(buffer[i],sizeof(BOOK_POSITION),MERGE_BLOCKSIZE,input_file[i]);
      next[i]=0;
    }
  }
  least.position=0;
  for (i=1;i<files;i++)
    if (data_read[i]) {
      least=buffer[i][next[i]];
      used=i;
      break;
    }
  if (least.position == 0) {
    for (i=1;i<files;i++) fclose(input_file[i]);
    return(least);
  }
  for (i++;i<files;i++) {
    if (data_read[i]) {
      if (least.position > buffer[i][next[i]].position) {
        least=buffer[i][next[i]];
        used=i;
      }
    }
  }
  if (--data_read[used] == 0) {
    data_read[used]=fread(buffer[used],sizeof(BOOK_POSITION),MERGE_BLOCKSIZE,input_file[used]);
    next[used]=0;
  }
  else
    next[used]++;
  return(least);
}

int Book_Up_Compare(const void *pos1, const void *pos2)
{
  if (((BOOK_POSITION *)pos1)->position < ((BOOK_POSITION *)pos2)->position) return(-1);
  else if (((BOOK_POSITION *)pos1)->position > ((BOOK_POSITION *)pos2)->position) return(1);
  return(0);
}
