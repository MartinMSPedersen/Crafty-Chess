#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>


/********************************************************************
 * File           : html.c                                          *
 * Author         : George R. Barrett,  Ph.D. Candidate             *
 *                                      EECS Department             *
 *                                      Division of System Science  *
 *                                      University of Michigan      *
 * Date           : 6 Sep 97                                        *
 * Report bugs to : grbarret@eecs.umich.edu                         *
 *                                                                  *
 * Last Modified  : 25 Jan 99                                       *
 ********************************************************************
 * The functions in html.c are based on fen2html.c available at     *
 * http://www.eecs.umich.edu/~grbarret/chess                        *
 *                                                                  *
 * These functions provide HTML output capability to Crafty.        *
 *                                                                  *
 ********************************************************************/

int AnnotateHeaderHTML(char *title_text, FILE *annotate_out) {
  fprintf(annotate_out,"<HTML>\n");
  fprintf(annotate_out,"<HEAD><TITLE>%s</TITLE></HEAD>\n",title_text);
  fprintf(annotate_out,"<BODY BGColor=#ffffff>\n");
  return EXIT_SUCCESS;
}

int AnnotateFooterHTML(FILE *annotate_out) {
  fprintf(annotate_out,"</BODY>\n");
  fprintf(annotate_out,"</HTML>\n");
  return EXIT_SUCCESS;
}

int AnnotatePositionHTML(TREE *tree, char *text, int wtm, FILE *annotate_out) {
  char filename[32], html_piece;
  char xlate[15]={'q','r','b',0,'k','n','p','*','P','N','K',0,'B','R','Q'};
  int rank, file;
  
/*  Display the board in HTML using table of images.          */
  fprintf(annotate_out,"<CENTER>\n");
  fprintf(annotate_out,"<TABLE Border=1 CellSpacing=0 CellPadding=0>\n\n");

  for (rank=RANK8;rank>=RANK1; rank--) {
/*
    fprintf(annotate_out,"<TR>\n",rank+1);
*/
    fprintf(annotate_out,"<TR>\n");
    for (file=FILEA; file<=FILEH; file++) {
      (void)strcpy(filename,"bitmaps/");
      if ((rank+file) % 2) (void)strcat(filename,"w");
      else (void)strcat(filename,"b");
      if (PcOnSq((rank<<3)+file)) {
        html_piece=xlate[PcOnSq((rank<<3)+file)+7];
        switch(html_piece) {
        case 'p':
          (void)strcat(filename,"bp");
          break;
        case 'r':
          (void)strcat(filename,"br");
          break;
        case 'n':
          (void)strcat(filename,"bn");
          break;
        case 'b':
          (void)strcat(filename,"bb");
          break;
        case 'q':
          (void)strcat(filename,"bq");
          break;
        case 'k':
          (void)strcat(filename,"bk");
          break;
        case 'P':
          (void)strcat(filename,"wp");
          break;
        case 'R':
          (void)strcat(filename,"wr");
          break;
        case 'N':
          (void)strcat(filename,"wn");
          break;
        case 'B':
          (void)strcat(filename,"wb");
          break;
        case 'Q':
          (void)strcat(filename,"wq");
          break;
        case 'K':
          (void)strcat(filename,"wk");
          break;
        case '*':
        (void)strcat(filename,"sq");
          break;
        default:
          printf("Something's wrong\n");
          exit(0);
        }
      }
      (void)strcat(filename,".gif");
      fprintf(annotate_out,"<TD><IMG SRC=\"%s\"></TD>\n",filename);
    }
    fprintf(annotate_out,"</TR>\n\n");
  }
  fprintf(annotate_out,"</TABLE>\n");
  if (wtm)
    fprintf(annotate_out,"<H2>White to move.</H2>\n");
  else
    fprintf(annotate_out,"<H2>Black to move.</H2>\n");

  fprintf(annotate_out,"</CENTER></BR></BR></BR>\n");
  return EXIT_SUCCESS;
}
