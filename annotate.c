#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "types.h"
#include "function.h"
#include "data.h"

/* last modified 10/12/96 */
/*
********************************************************************************
*                                                                              *
*  "annotate" command is used to search through the game in the "history" file *
*  (often set by the "read" command which reads moves in, skipping non-move    *
*  information such as move numbers, times, etc.)                              *
*                                                                              *
*  the format of the command is as follows:                                    *
*                                                                              *
*      annotate filename b|w|bw moves margin time                              *
*                                                                              *
*  filename is the input file where Crafty will obtain the moves to annotate,  *
*  and output will be written to file "filename.ann".                          *
*                                                                              *
*  where b/w/bw indicates whether to annotate only the white side (w), the     *
*  black side (b) or both (bw).                                                *
*                                                                              *
*  moves indicates the move or moves to annotate.  it can be a single move,    *
*  which indicates the starting move number to annotate, or it can be a range, *
*  which indicates a range of move (1-999 gets the whole game.)                *
*                                                                              *
*  margin is the difference between Crafty's evaluation for the move actually  *
*  played and for the move Crafty thinks is best, before crafty will generate  *
*  a comment in the annotation file.  1.0 is a pawn, and will only generate    *
*  comments if the move played is 1.000 (1 pawn) worse than the best move      *
*  found by doing a complete search.                                           *
*                                                                              *
*  time is time per move to search, in seconds.                                *
*                                                                              *
********************************************************************************
*/
void Annotate() {

  FILE *annotate_in, *annotate_out;
  char text[128], colors[32], next;
  int annotate_margin, annotate_score=0, player_score=0;
  int twtm, path_len, analysis_printed=0;
  int wtm, move_number, line1, line2, move, suggested, i;

/*
 ----------------------------------------------------------
|                                                          |
|   first, quiz the user for the options needed to         |
|   successfully annotate a game.                          |
|                                                          |
 ----------------------------------------------------------
*/
  Print(0,"\nannotate filename: ");
  fscanf(input_stream,"%s",text);
  annotate_in = fopen(text,"r");
  if (annotate_in == NULL) {
    Print(0,"unable to open %s for input\n", text);
    return;
  }
  strcpy(text+strlen(text),".can");
  annotate_out = fopen(text,"w");
  if (annotate_out == NULL) {
    Print(0,"unable to open %s for output\n", text);
    return;
  }
  Print(0,"\ncolor(s): ");
  fscanf(input_stream,"%s",colors);
  line1=1;
  line2=999;
  Print(0,"\nstarting move number or range: ");
  fscanf(input_stream,"%s",text);
  if(strchr(text,'-')) sscanf(text,"%d-%d",&line1,&line2);
  else {
    sscanf(text,"%d",&line1);
    line2=999;
  }
  Print(0,"\nannotation margin: ");
  fscanf(input_stream,"%s",text);
  annotate_margin=atof(text)*PAWN_VALUE;
  Print(0,"\ntime per move: ");
  fscanf(input_stream,"%s",text);
  search_time_limit=atoi(text)*100;
/*
 ----------------------------------------------------------
|                                                          |
|   reset the game to "square 0" to start the annotation   |
|   procedure.  then we read moves from the input file,    |
|   make them on the game board, and annotate if the move  |
|   is for the correct side.  if we haven't yet reached    |
|   the starting move to annotate, we skip the Search()    |
|   stuff and read another move.                           |
|                                                          |
 ----------------------------------------------------------
*/
  ponder_completed=0;
  ponder_move=0;
  last_pv.path_iteration_depth=0;
  last_pv.path_length=0;
  InitializeChessBoard(&position[0]);
  wtm=1;
  move_number=1;

  if (!strcmp(colors,"bw") || !strcmp(colors,"wb"))
    fprintf(annotate_out,"annotating both black and white moves\n");
  else if (strchr(colors,'b'))
    fprintf(annotate_out,"annotating only black moves\n");
  else if (strchr(colors,'w'))
    fprintf(annotate_out,"annotating only white moves\n");
  fprintf(annotate_out,"using a scoring margin of %s pawns.\n",
          DisplayEvaluationWhisper(annotate_margin));
  fprintf(annotate_out,"search time limit is %s\n\n",
          DisplayTimeWhisper(search_time_limit));
  do {
    fflush(annotate_out);
    move=ReadChessMove(annotate_in,wtm);
    if (move < 0) break;
    strcpy(text,OutputMove(&move,0,wtm));
    fseek(history_file,((move_number-1)*2+1-wtm)*10,SEEK_SET);
    fprintf(history_file,"%10s ",text);
    if (wtm) Print(0,"White(%d): %s\n",move_number,text);
    else Print(0,"Black(%d): %s\n",move_number,text);
    if (analysis_printed)
      fprintf(annotate_out,"%3d.%s%6s\n",move_number,(wtm?"":"   ..."),text);
    else {
      if (wtm)
        fprintf(annotate_out,"%3d.%6s",move_number,text);
      else
        fprintf(annotate_out,"%6s\n",text);
    }
    analysis_printed=0;
/*
 ----------------------------------------------------------
|                                                          |
|   now search the actual move played to obtain the score  |
|   from Crafty's tree search algorithm, then search all   |
|   legal moves in this position to obtain the score for   |
|   what Crafty thinks is best here, and if crafty's move  |
|   is better than the actual move by "margin" then we     |
|   output the analysis and continue.                      |
|                                                          |
 ----------------------------------------------------------
*/
    if (move_number >= line1)
      if ((!wtm && strchr(colors,'b')) | ( wtm && strchr(colors,'w'))) {
        last_pv.path_iteration_depth=0;
        last_pv.path_length=0;
        thinking=1;
        Print(0,"\n              Searching only the move played in game.");
        Print(0,"--------------------\n");
        position[1]=position[0];
        search_move=move;
        player_score = Iterate(wtm,think);
        Print(0,"\n              Searching all legal moves.");
        Print(0,"----------------------------------\n");
        search_move=0;
        position[1]=position[0];
        annotate_score = Iterate(wtm,think);
        thinking=0;
        twtm = wtm;
        path_len = pv[0].path_length;
        if (pv[0].path_iteration_depth > 1 && path_len >= 1 && 
            player_score+annotate_margin <= annotate_score &&
            (move != pv[0].path[1] || annotate_margin < 0.0)) {
          if (wtm) {
            analysis_printed=1;
            fprintf(annotate_out,"\n");
          }
          fprintf(annotate_out,"{%s", DisplayEvaluationWhisper(player_score));
          fprintf(annotate_out," (%d:%s", pv[0].path_iteration_depth,
                  DisplayEvaluationWhisper(annotate_score)); 
          for (i=1;i<=path_len;i++) {
            fprintf(annotate_out," %s",OutputMove(&pv[0].path[i],i,twtm)); 
            MakeMove(i,pv[0].path[i],twtm);
            twtm=ChangeSide(twtm);
          }
          for (i=path_len;i>0;i--) {
            twtm=ChangeSide(twtm);
            UnMakeMove(i,pv[0].path[i],twtm);
          }
          fprintf(annotate_out,")}\n");
        }
      }
/*
 ----------------------------------------------------------
|                                                          |
|   before going on to the next move, see if the user has  |
|   included a set of other moves that require a search.   |
|   if so, search them one at a time and produce the ana-  |
|   lysis for each one.                                    |
|                                                          |
 ----------------------------------------------------------
*/
    do {
      next=getc(annotate_in);
    } while (next==' ' || next=='\n');
    ungetc(next,annotate_in);
    if (next == EOF) break;
    if (next == '{') do {
      do {
        next=getc(annotate_in);
      } while (next==' ');
      ungetc(next,annotate_in);
      if (next == EOF || next == '}') break;
      suggested=ReadChessMove(annotate_in,wtm);
      if (suggested < 0) break;
      thinking=1;
      Print(0,"\n              Searching only the move suggested.");
      Print(0,"--------------------\n");
      position[1]=position[0];
      search_move=suggested;
      annotate_score = Iterate(wtm,think);
      search_move=0;
      thinking=0;
      twtm = wtm;
      path_len = pv[0].path_length;
      if (pv[0].path_iteration_depth > 1 && path_len >= 1) {
        if (wtm && !analysis_printed) {
          analysis_printed=1;
          fprintf(annotate_out,"\n");
        }
        fprintf(annotate_out,"  {suggested %d:%s", pv[0].path_iteration_depth,
                DisplayEvaluationWhisper(annotate_score)); 
        for (i=1;i<=path_len;i++) {
          fprintf(annotate_out," %s",OutputMove(&pv[0].path[i],i,twtm)); 
          MakeMove(i,pv[0].path[i],twtm);
          twtm=ChangeSide(twtm);
        }
        for (i=path_len;i>0;i--) {
          twtm=ChangeSide(twtm);
          UnMakeMove(i,pv[0].path[i],twtm);
        }
        fprintf(annotate_out,"}\n");
      }
    } while(1);
    MakeMoveRoot(move,wtm);
    wtm=ChangeSide(wtm);
    if (wtm) move_number++;
    if (move_number > line2) break;
  } while (1);
  if (annotate_out != NULL) fclose(annotate_out);
  search_time_limit=0;
}
