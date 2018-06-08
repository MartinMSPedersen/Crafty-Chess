#include <stdio.h>
#include <stdlib.h>
#include "types.h"
#include "function.h"
#include "data.h"

/* last modified 10/11/96 */
/*
********************************************************************************
*                                                                              *
*   ReadChessMove() is used to read a move from an input file.  the main issue *
*   is to skip over "trash" like move numbers, times, comments, and so forth,  *
*   and find the next actual move.                                             *
*                                                                              *
********************************************************************************
*/
int ReadChessMove(FILE *input, int wtm) {

  static char text[128];
  int move=0, status;

  status=fscanf(input_file,"%s",text);
  if (status <= 0) return(-1);
  if (((text[0]>='a') && (text[0]<='z')) ||
      ((text[0]>='A') && (text[0]<='Z'))) {
    if (!strcmp(text,"exit")) return(-1);
    return(InputMove(text,0,wtm,1,0));
  }
}
