#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "chess.h"
#include "data.h"

/* last modified 11/14/96 */
/*
********************************************************************************
*                                                                              *
*  "annotate" command is used to search through the game in the "history" file *
*  (often set by the "read" command which reads moves in, skipping non-move    *
*  information such as move numbers, times, etc.)                              *
*                                                                              *
*  the normal output of this command is a file, in PGN format, that contains   *
*  the moves of the game, along with analysis when Crafty does not think that  *
*  move was the best choice.  the definition of "best choice" is somewhat      *
*  vague, because if the move played is "close" to the best move available,    *
*  Crafty will not comment on the move.  "close" is defined by the <margin>    *
*  option explained below.  this basic type of annotation works by first       *
*  using the normal tree search algorithm to find the best move.  if this      *
*  move was the move played, no output is produced.  if a different move is    *
*  considered best, then the actual move played is searched to the same depth  *
*  and if the best move and actual move scores are within <margin> of each     *
*  other, no comment is produced, otherwise crafty inserts the evaluation for  *
*  the move played, followed by the eval and PV for the best continuation it   *
*  found.                                                                      *
*                                                                              *
*  the extended annotation takes a little longer, because crafty searches all  *
*  moves *except* the move played, and then searches the move played, to the   *
*  same depth.  This is used to produce more detailed statistics about the     *
*  game, including such things as how often the player found the *only* move   *
*  to win something or to avoid losing something.  this sort of information    *
*  will eventually be used to attempt to compute a rating for a player based   *
*  on analyzing a game (or games) played by him/her.                           *
*                                                                              *
*  the format of the command is as follows:                                    *
*                                                                              *
*      annotate filename b|w|bw moves margin time [x]                          *
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
*  x [optional] indicates that annotate should use the "extended annotation"   *
*  algorithm, which takes longer to execute, but which provides more info on   *
*  how well the player(x) did compared to Crafty's search algorithm.           *
*                                                                              *
********************************************************************************
*/
void Annotate() {

  FILE *annotate_in, *annotate_out;
  char command[80], text[128], colors[32], next;
  int annotate_margin, annotate_score=0, player_score=0;
  int annotate_search_time_limit, only;
  int twtm, path_len, analysis_printed=0, extended=0;
  int wtm, move_number, line1, line2, move, suggested, i;
  int blunders[10], only_move[10], matched=0, book_moves=0;
  int temp_draw_score_is_zero, last_white_eval=0, last_black_eval=0;
  CHESS_PATH temp;

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
  sprintf(command,"read=%s",text);
  Option(command);
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
  annotate_search_time_limit=atoi(text)*100;
  next=getc(input_stream);
  if (next == ' ') {
    fscanf(input_stream,"%s",text);
    extended=!strcmp(text,"x"); 
  }
  for (i=0;i<10;i++) {
    blunders[i]=0;
    only_move[i]=0;
  }
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
  temp_draw_score_is_zero=draw_score_is_zero;
  draw_score_is_zero=1;
  ponder_completed=0;
  ponder_move=0;
  last_pv.path_iteration_depth=0;
  last_pv.path_length=0;
  InitializeChessBoard(&position[0]);
  wtm=1;
  move_number=1;


  fprintf(annotate_out,"[Event \"%s\"]\n",pgn_event);
  fprintf(annotate_out,"[Site \"%s\"]\n",pgn_site);
  fprintf(annotate_out,"[Date \"%s\"]\n",pgn_date);
  fprintf(annotate_out,"[Round \"%s\"]\n",pgn_round);
  fprintf(annotate_out,"[White \"%s\"]\n",pgn_white);
  fprintf(annotate_out,"[WhiteElo \"%s\"]\n",pgn_white_elo);
  fprintf(annotate_out,"[Black \"%s\"]\n",pgn_black);
  fprintf(annotate_out,"[BlackElo \"%s\"]\n",pgn_black_elo);
  fprintf(annotate_out,"[Result \"%s\"]\n",pgn_result);

  fprintf(annotate_out,"[Annotator \"Crafty v%s\"]\n",version);
  if (!strcmp(colors,"bw") || !strcmp(colors,"wb"))
    fprintf(annotate_out,"{annotating both black and white moves.}\n");
  else if (strchr(colors,'b'))
    fprintf(annotate_out,"{annotating only black moves.}\n");
  else if (strchr(colors,'w'))
    fprintf(annotate_out,"{annotating only white moves.}\n");
  fprintf(annotate_out,"{using a scoring margin of %s pawns.}\n",
          DisplayEvaluationWhisper(annotate_margin));
  fprintf(annotate_out,"{search time limit is %s}\n\n",
          DisplayTimeWhisper(annotate_search_time_limit));
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
    if (move_number >= line1) {
      only=0;
      if ((!wtm && strchr(colors,'b')) | ( wtm && strchr(colors,'w'))) {
        last_pv.path_iteration_depth=0;
        last_pv.path_length=0;
        thinking=1;
/*
 ----------------------------------------------------------
|                                                          |
|   now search the position to see if the move played is   |
|   the best move possible.  if not, then search just the  |
|   move played to get a score for it as well, so we can   |
|   determine if annotated output is appropriate.          |
|                                                          |
 ----------------------------------------------------------
*/
        if (!extended) {
          search_time_limit=annotate_search_time_limit;
          Print(0,"\n              Searching all legal moves.");
          Print(0,"----------------------------------\n");
          position[1]=position[0];
          annotate_score=Iterate(wtm,think);
          temp=pv[0];
          player_score=annotate_score;
          if (temp.path[1] != move) {
            Print(0,"\n              Searching only the move played in game.");
            Print(0,"--------------------\n");
            position[1]=position[0];
            search_move=move;
            search_time_limit=99999999;
            search_depth=temp.path_iteration_depth;
            player_score=Iterate(wtm,think);
            search_depth=0;
            search_time_limit=annotate_search_time_limit;
            search_move=0;
          }
        }
/*
 ----------------------------------------------------------
|                                                          |
|   now search the position, by searching every move but   |
|   the move played, and then searching the move played,   |
|   determine how good that move actually is.              |
|                                                          |
 ----------------------------------------------------------
*/
        else {
          search_time_limit=annotate_search_time_limit;
          Print(0,"\n              Searching all legal moves (except move played).");
          Print(0,"------------\n");
          position[1]=position[0];
          search_move=-move;
          annotate_score=Iterate(wtm,think);
          temp=pv[0];
          Print(0,"\n              Searching only the move played in game.");
          Print(0,"--------------------\n");
          search_move=move;
          position[1]=position[0];
          search_time_limit=99999999;
          search_depth=temp.path_iteration_depth;
          player_score=Iterate(wtm,think);
          search_depth=0;
          search_time_limit=annotate_search_time_limit;
          search_move=0;
/*
 --------------------------------------------
|                                            |
|  determine if the move played was a        |
|  blunder.                                  |
|                                            |
 --------------------------------------------
*/
          if (annotate_score-player_score > 800) {
            int blunder_index=(annotate_score-player_score+800)/1000;
            blunder_index=Min(blunder_index,9);
            blunders[blunder_index]++;
          }
/*
 --------------------------------------------
|                                            |
|  determine if the move played was the only |
|  reasonable move.                          |
|                                            |
 --------------------------------------------
*/
          if (player_score-annotate_score > 800) {
            int only_index=(player_score-annotate_score+800)/1000;
            only_index=Min(only_index,9);
            only_move[only_index]++;
            only=1;
          }
/*
 --------------------------------------------
|                                            |
|  determine if the move played was a book   |
|  move.                                     |
|                                            |
 --------------------------------------------
*/
          if (pv[0].path_iteration_depth <= 1) {
            book_moves++;
          }
/*
 --------------------------------------------
|                                            |
|  determine if the move played matched the  |
|  move Crafty likes best.                   |
|                                            |
 --------------------------------------------
*/
          if (player_score >= annotate_score) {
            matched++;
          }
        }
        thinking=0;
        twtm = wtm;
        path_len = temp.path_length;
        if (temp.path_iteration_depth > 1 && path_len >= 1 && 
            player_score+annotate_margin <= annotate_score &&
            (move != temp.path[1] || annotate_margin < 0.0)) {
          if (wtm) {
            analysis_printed=1;
            fprintf(annotate_out,"\n");
          }
          fprintf(annotate_out,"{%s", DisplayEvaluationWhisper(player_score));
          fprintf(annotate_out," (%d:%s", temp.path_iteration_depth,
                  DisplayEvaluationWhisper(annotate_score)); 
          for (i=1;i<=path_len;i++) {
            fprintf(annotate_out," %s",OutputMove(&temp.path[i],i,twtm)); 
            MakeMove(i,temp.path[i],twtm);
            twtm=ChangeSide(twtm);
          }
          for (i=path_len;i>0;i--) {
            twtm=ChangeSide(twtm);
            UnMakeMove(i,temp.path[i],twtm);
          }
          fprintf(annotate_out,")}\n");
        }
        if (only) {
          if (wtm || analysis_printed) fprintf(annotate_out,"\n");
          analysis_printed=1;
          if (wtm) {
            if (annotate_score > last_white_eval)
              fprintf(annotate_out,"{only move to win at least %d pawn(s)}\n",
                      Min((player_score-annotate_score+800)/1000,9));
            else
              fprintf(annotate_out,"{only move to avoid losing at least %d pawn(s)}\n",
                      Min((player_score-annotate_score+800)/1000,9));
          }
          else {
            if (annotate_score > last_black_eval)
              fprintf(annotate_out,"{only move to win at least %d pawn(s)}\n",
                      Min((player_score-annotate_score+800)/1000,9));
            else
              fprintf(annotate_out,"{only move to avoid losing at least %d pawn(s)}\n",
                      Min((player_score-annotate_score+800)/1000,9));
          }
        }
        if (wtm) last_white_eval=player_score;
        else last_black_eval=player_score;
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
        fprintf(annotate_out,"  {suggested %d:%s",
                pv[0].path_iteration_depth,
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
  if (extended) {
    fprintf(annotate_out,"\n\n--------------------Summary-------------------\n\n");
    fprintf(annotate_out,"book moves.................................%3d\n",book_moves);
    fprintf(annotate_out,"best move played...........................%3d\n",matched);
    for (i=0;i<10;i++)
      if (blunders[i])
        fprintf(annotate_out,"blundered at least %d pawns.................%3d\n",i,blunders[i]);
    for (i=0;i<10;i++)
      if (only_move[i])
        fprintf(annotate_out,"only move to win at least %d pawns..........%3d\n",i,only_move[i]);
  }
  if (annotate_out != NULL) fclose(annotate_out);
  search_time_limit=0;
  draw_score_is_zero=temp_draw_score_is_zero;
}
