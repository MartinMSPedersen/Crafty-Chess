#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "chess.h"
#include "data.h"

/* last modified 02/17/98 */
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
*  found.  you can enter suggested moves for Crafty to analyze at any point    *
*  by simply entering a move as an analysis-type comment using (move) or       *
*  {move}.  Crafty will search that move in addition to the move actually      *
*  played and the move it thinks is best.                                      *
*                                                                              *
*  the format of the command is as follows:                                    *
*                                                                              *
*      annotate filename b|w|bw moves margin time [n]                          *
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
*  [n] is optional and tells Crafty to produce the PV/score for the "n" best   *
*  moves.  Crafty stops when the best move reaches the move played in the game *
*  or after displaying n moves, whichever comes first.  if you use -n, then it *
*  will display n moves regardless of where the game move ranks.               *
*                                                                              *
********************************************************************************
*/
void Annotate() {

  FILE *annotate_in, *annotate_out;
  char text[128], tbuffer[512], colors[32];
  int annotate_margin, annotate_score[100], player_score, best_moves;
  int annotate_search_time_limit, search_player;
  int twtm, path_len, analysis_printed=0, *mv;
  int wtm, move_num, line1, line2, move, suggested, i;
  int searches_done, read_status;
  PATH temp[100], player_pv;
  int temp_search_depth;
  TREE *tree=local[0];
/*
 ----------------------------------------------------------
|                                                          |
|   first, quiz the user for the options needed to         |
|   successfully annotate a game.                          |
|                                                          |
 ----------------------------------------------------------
*/
  strcpy(tbuffer,buffer);
  nargs=ReadParse(tbuffer,args," 	;");
  if (nargs < 6) {
    printf("usage: annotate <file> <color> <moves> <margin> <time> [nmoves]\n");
    return;
  }
  annotate_in = fopen(args[1],"r");
  if (annotate_in == NULL) {
    Print(4095,"unable to open %s for input\n", args[1]);
    return;
  }
  nargs=ReadParse(tbuffer,args," 	;");
  strcpy(text,args[1]);
  strcpy(text+strlen(text),".can");
  annotate_out = fopen(text,"w");
  if (annotate_out == NULL) {
    Print(4095,"unable to open %s for output\n", text);
    return;
  }
  draw_score_normal=1;
  strcpy(colors,args[2]);
  line1=1;
  line2=999;
  if(strchr(args[3],'-')) sscanf(args[3],"%d-%d",&line1,&line2);
  else {
    sscanf(args[3],"%d",&line1);
    line2=999;
  }
  annotate_margin=atof(args[4])*PAWN_VALUE;
  annotate_search_time_limit=atoi(args[5])*100;
  if (nargs > 6) 
    best_moves=atoi(args[6]);
  else
    best_moves=1;
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
  annotate_mode=1;
  ponder=0;
  temp_search_depth=search_depth;
 
  read_status=ReadPGN(0,0);
  read_status=ReadPGN(annotate_in,0);
  while (read_status != -1) {
    ponder_move=0;
    last_pv.path_iteration_depth=0;
    last_pv.path_length=0;
    InitializeChessBoard(&tree->position[0]);
    tree->position[1]=tree->position[0];
    wtm=1;
    move_number=1;
/*
 ----------------------------------------------------------
|                                                          |
|   now grab the PGN tag values so they can be copied to   |
|   the .can file for reference.                           |
|                                                          |
 ----------------------------------------------------------
*/
    do
      read_status=ReadPGN(annotate_in,0);
    while(read_status==1);
    if (read_status == -1) break;
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
      move=ReadNextMove(tree,buffer,0,wtm);
      if (move <= 0) break;
      strcpy(text,OutputMove(tree,move,0,wtm));
      fseek(history_file,((move_number-1)*2+1-wtm)*10,SEEK_SET);
      fprintf(history_file,"%9s\n",text);
      if (wtm) Print(4095,"White(%d): %s\n",move_number,text);
      else Print(4095,"Black(%d): %s\n",move_number,text);
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
        if ((!wtm && strchr(colors,'b')) || ( wtm && strchr(colors,'w'))) {
          last_pv.path_iteration_depth=0;
          last_pv.path_length=0;
          thinking=1;
          RootMoveList(wtm);
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
          search_time_limit=annotate_search_time_limit;
          search_depth=temp_search_depth;
          player_score=-999999;
          search_player=1;
          for (searches_done=0;searches_done<abs(best_moves);searches_done++) {
            if (searches_done > 0) {
              search_time_limit=3*annotate_search_time_limit;
              search_depth=temp[0].path_iteration_depth;
            }
            Print(4095,"\n              Searching all legal moves.");
            Print(4095,"----------------------------------\n");
            tree->position[1]=tree->position[0];
            InitializeHashTables();
            annotate_score[searches_done]=Iterate(wtm,annotate,1);
            if (tree->pv[0].path[1] == move) {
              player_score=annotate_score[searches_done];
              player_pv=tree->pv[0];
              search_player=0;
            }
            temp[searches_done]=tree->pv[0];
            for (mv=tree->last[0];mv<tree->last[1];mv++) {
              if (*mv == tree->pv[0].path[1]) {
                for (;mv<tree->last[1]-1;mv++) *mv=*(mv+1);
                tree->last[1]--;
                break;
              }
            }
            if (tree->last[1] < tree->last[0] ||
                (player_score+annotate_margin>annotate_score[searches_done] &&
                 best_moves>0)) break;
          }
          if (search_player) {
            Print(4095,"\n              Searching only the move played in game.");
            Print(4095,"--------------------\n");
            tree->position[1]=tree->position[0];
            search_move=move;
            search_time_limit=3*annotate_search_time_limit;
            search_depth=temp[0].path_iteration_depth;
            if (search_depth==temp_search_depth)
              search_time_limit=annotate_search_time_limit;
            InitializeHashTables();
            player_score=Iterate(wtm,annotate,0);
            player_pv=tree->pv[0];
            search_depth=temp_search_depth;
            search_time_limit=annotate_search_time_limit;
            search_move=0;
          }
/*
 ----------------------------------------------------------
|                                                          |
|   output the score/pv for the move played unless it      |
|   matches what Crafty would have played.  if it doesn't  |
|   then output the pv for what Crafty thinks is best.     |
|                                                          |
 ----------------------------------------------------------
*/
          thinking=0;
          if (player_pv.path_iteration_depth>1 && player_pv.path_length>=1 && 
              player_score+annotate_margin<annotate_score[0] &&
              (temp[0].path[1]!=player_pv.path[1] || annotate_margin<0.0 ||
              best_moves!=1)) {
            if (wtm) {
              analysis_printed=1;
              fprintf(annotate_out,"\n");
            }
            fprintf(annotate_out,"                ({%d:%s}",
                    player_pv.path_iteration_depth,
                    DisplayEvaluationWhisper(player_score)); 
            path_len=player_pv.path_length;
            fprintf(annotate_out," %s", FormatPV(tree,wtm, player_pv));
            fprintf(annotate_out,")\n");
            for (move_num=0;move_num<searches_done;move_num++) {
              if (move != temp[move_num].path[1]) {
                fprintf(annotate_out,"                ({%d:%s}",
                        temp[move_num].path_iteration_depth,
                        DisplayEvaluationWhisper(annotate_score[move_num])); 
                path_len=temp[move_num].path_length;
                fprintf(annotate_out," %s", FormatPV(tree,wtm, temp[move_num]));
                fprintf(annotate_out,")\n");
              }
            }
          }
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
      read_status=ReadPGN(annotate_in,1);
      while (read_status == 2) {
        suggested=InputMove(tree,buffer,0,wtm,1,0);
        if (suggested > 0) {
          thinking=1;
          Print(4095,"\n              Searching only the move suggested.");
          Print(4095,"--------------------\n");
          tree->position[1]=tree->position[0];
          search_move=suggested;
          search_time_limit=3*annotate_search_time_limit;
          search_depth=temp[0].path_iteration_depth;
          InitializeHashTables();
          annotate_score[0]=Iterate(wtm,annotate,0);
          search_depth=temp_search_depth;
          search_time_limit=annotate_search_time_limit;
          search_move=0;
          thinking=0;
          twtm = wtm;
          path_len = tree->pv[0].path_length;
          if (tree->pv[0].path_iteration_depth > 1 && path_len >= 1) {
            if (wtm && !analysis_printed) {
              analysis_printed=1;
              fprintf(annotate_out,"\n");
            }
            fprintf(annotate_out,"                ({suggested %d:%s}",
                    tree->pv[0].path_iteration_depth,
                    DisplayEvaluationWhisper(annotate_score[0])); 
            for (i=1;i<=path_len;i++) {
              fprintf(annotate_out," %s",OutputMove(tree,tree->pv[0].path[i],i,twtm)); 
              MakeMove(tree,i,tree->pv[0].path[i],twtm);
              twtm=ChangeSide(twtm);
              }
            for (i=path_len;i>0;i--) {
              twtm=ChangeSide(twtm);
              UnMakeMove(tree,i,tree->pv[0].path[i],twtm);
            }
            fprintf(annotate_out,")\n");
          }
        }
        read_status=ReadPGN(annotate_in,1);
        if (read_status != 2) break;
      }
      MakeMoveRoot(tree,move,wtm);
      wtm=ChangeSide(wtm);
      if (wtm) move_number++;
      if (move_number > line2) break;
      if (read_status != 0) break;
    } while (1);
    fprintf(annotate_out,"\n");
    if (read_status == -1) break;
    do
      read_status=ReadPGN(annotate_in,0);
    while(read_status==0);
    if (read_status == -1) break;
  }
  fprintf(annotate_out,"\n");
  if (annotate_out) fclose(annotate_out);
  if (annotate_in) fclose(annotate_in);
  search_time_limit=0;
  annotate_mode=0;
}
