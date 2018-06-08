#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "chess.h"
#include "data.h"

/* last modified 09/21/99 */
/*
********************************************************************************
*                                                                              *
*   EGTBPV() is used to display the full PV (path) for a mate/mated in N EGTB  *
*   position.  if the second token is a !, then we show which moves are the    *
*   only optimal moves by adding a ! to them.                                  *
*                                                                              *
********************************************************************************
*/
void EGTBPV(TREE *tree, int wtm) {
  int moves[1024], current[256];
  char buffer[1024], *next;
  BITBOARD pos[1024];
  int value;
  register int ply, i, j, nmoves, *last, t_move_number;
  register int best=0, bestmv=0, optimal_mv=0;
  register int bang=0;
  if (!strcmp(args[1],"!")) bang=1;
/*
 ----------------------------------------------------------
|                                                          |
|   first, see if this is a known EGTB position.  if not,  |
|   we can bug out right now.                              |
|                                                          |
 ----------------------------------------------------------
*/
  if (!EGTB_setup) return;
  if(!EGTBProbe(tree, 1, wtm, &value)) return;
  t_move_number=move_number;
  if (display_options&64) sprintf(buffer,"%d.",move_number);
  else buffer[0]=0;
  if ((display_options&64) && !wtm) sprintf(buffer+strlen(buffer)," ...");
/*
 ----------------------------------------------------------
|                                                          |
|   the rest is simple, but messy.  generate all moves,    |
|   then find the move with the best egtb score and make   |
|   it (note that if there is only one that is optimal, it |
|   is flagged as such).  we then repeat this over and     |
|   over until we reach the end, or until we repeat a move |
|   and can call it a repetition.                          |
|                                                          |
 ----------------------------------------------------------
*/
  for (ply=1;ply<1024;ply++) {
    pos[ply]=HashKey;
    last=GenerateCaptures(tree, 1, wtm, current);
    last=GenerateNonCaptures(tree, 1, wtm, last);
    nmoves=last-current;
    best=-MATE-1;
    for (i=0;i<nmoves;i++) {
      MakeMove(tree,1,current[i],wtm);
      if (!Check(wtm)) {
        if(EGTBProbe(tree, 2, ChangeSide(wtm), &value)) {
          value=-value;
          if (value > best) {
            best=value;
            bestmv=current[i];
            optimal_mv=1;
          }
          else if (value == best) optimal_mv=0;
        }
      }
      UnMakeMove(tree,1,current[i],wtm);
    }
    if (best > -MATE-1) {
      moves[ply]=bestmv;
      if ((display_options&64) && ply>1 && wtm)
        sprintf(buffer+strlen(buffer)," %d.",t_move_number);
      sprintf(buffer+strlen(buffer)," %s",OutputMove(tree,bestmv,1,wtm));
      if (bang && optimal_mv) sprintf(buffer+strlen(buffer),"!");
      for (j=1;j<ply;j++) 
        if (pos[ply] == pos[j]) break;
      if (j < ply) break;
      MakeMove(tree,1,bestmv,wtm);
      tree->position[1]=tree->position[2];
      wtm=ChangeSide(wtm);
      if (wtm) t_move_number++;
      if (strchr(buffer,'#')) break;
    }
    else {
      ply--;
      break;
    }
  }
  nmoves=ply;
  for (;ply>0;ply--) {
    wtm=ChangeSide(wtm);
    UnMakeMove(tree,1,moves[ply],wtm);
    tree->position[2]=tree->position[1];
  }
  next=buffer;
  while (nmoves) {
    if (strlen(next) > 72) {
      int i;
      for (i=0;i<16;i++) 
        if (*(next+64+i) == ' ') break;
      *(next+64+i)=0;
      printf("%s\n",next);
      next+=64+i+1;
    }
    else {
      printf("%s\n",next);
      break;
    }
  }
}
