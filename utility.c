#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include "chess.h"
#include "data.h"
#if !defined(AMIGA)
#  include <limits.h>
#endif
#if defined(SMP) && defined(NUMA)
#  include <numa.h>
#endif
#if defined(OS2)
#  include <sys/select.h>
#endif
#if defined(NT_i386)
#  include <windows.h>
#  include <winbase.h>
#  include <wincon.h>
#  include <io.h>
#  include <time.h>
#else
#  if !defined(MACOS)
#    include <sys/times.h>
#    include <sys/time.h>
#  endif
#endif
#if defined(UNIX)
#  include <unistd.h>
#  include <sys/types.h>
#  if !defined(LINUX) && !defined(ALPHA) && !defined(HP) && !defined(CRAY1) && !defined(FreeBSD) && !defined(NetBSD) && !defined(__EMX__)
#    if defined(AIX)
#      include <sys/termio.h>
#      include <sys/select.h>
#    else
#      if defined(NEXT)
#        include <bsd/termios.h>
#        include <sys/ioctl.h>
#      else
#        include <sys/filio.h>
#      endif
#    endif
#    if !defined(NEXT)
#      include <stropts.h>
#    endif
#    include <sys/conf.h>
#  else
#    include <sys/ioctl.h>
#  endif
#endif
#if defined(UNIX)
#  if !defined(CLK_TCK)
     static clock_t clk_tck = 0;
#  endif
#endif

#if defined(__EMX__)
#  define INCL_DOS
#  define INCL_KBD
#  include <os2.h>
#endif

#if defined(MACOS)
#  include <unistd.h>
#  include <unix.h>
#  include <Events.h>

   int CheckInput(void) {
      EventRecord theEvent;
   
      return OSEventAvail(keyDownMask | autoKeyMask, &theEvent);
   }
#endif

#if defined(AMIGA)
#  include <proto/dos.h>
#  define tv_sec tv_secs
#  define tv_usec tv_micro
#  include <exec/types.h>
#  define RAW 1
#  define CON 0
#  include <limits.h>

int _kbhit(void) {
  BPTR  inp;
  BOOLEAN  ret;

  inp=Input();
  if(!IsInteractive(inp)) return FALSE;
  Flush(inp);
  (void) SetMode(inp,RAW);
  ret=WaitForChar(inp,1);
  (void) SetMode(inp,CON);
  return ret;
}
#endif   /* if defined(AMIGA)  */

# if defined(NT_i386)
#  include <windows.h>
#  include <conio.h>
int CheckInput(void) {
  int i;
   static int init = 0, pipe;
   static HANDLE inh;
   DWORD dw;

  if (!xboard && !ics && !isatty(fileno(stdin))) return(0);
  if (batch_mode) return(0);
  if (strchr(cmd_buffer,'\n')) return(1);
  if (xboard) {
#if defined(FILE_CNT)
    if (stdin->_cnt > 0) return stdin->_cnt;
#endif
    if (!init) {
      init = 1;
      inh = GetStdHandle(STD_INPUT_HANDLE);
      pipe = !GetConsoleMode(inh, &dw);
      if (!pipe) {
        SetConsoleMode(inh, dw & ~(ENABLE_MOUSE_INPUT|ENABLE_WINDOW_INPUT));
        FlushConsoleInputBuffer(inh);
      }
    }
    if (pipe) {
      if (!PeekNamedPipe(inh, NULL, 0, NULL, &dw, NULL)) {
        return 1;
      }
      return dw;
    } else {
      GetNumberOfConsoleInputEvents(inh, &dw);
      return dw <= 1 ? 0 : dw;
    }
  }
  else {
    i=_kbhit();
  }
  return(i);
}
#endif

#if defined(UNIX)
#  ifdef __EMX__
int CheckInput(void) {
  static KBDKEYINFO keyinfo;
  int i;

  if (!xboard && !ics && !isatty(fileno(stdin))) return(0);
  if (strchr(cmd_buffer,'\n')) return(1);
  KbdPeek (&keyinfo, 0);
  if (keyinfo.fbStatus & KBDTRF_FINAL_CHAR_IN) i = 1;
  else i = 0;
  return(i);
}
#else
int CheckInput(void) {
  fd_set readfds;
  struct timeval tv;
  int data;

  if (!xboard && !ics && !isatty(fileno(stdin))) return(0);
  if (batch_mode) return(0);
  if (strchr(cmd_buffer,'\n')) return(1);
  FD_ZERO(&readfds);
  FD_SET(fileno(stdin), &readfds);
#if defined(DGT)
  if (DGT_active) FD_SET(from_dgt, &readfds);
#endif
  tv.tv_sec=0;
  tv.tv_usec=0;
  select(16, &readfds, 0, 0, &tv);
  data=FD_ISSET(fileno(stdin), &readfds);
#if defined(DGT)
  if (DGT_active) data|=FD_ISSET(from_dgt, &readfds);
#endif
  return(data);
}
#  endif
#endif

void ClearHashTableScores(int dopawnstoo) {
  int i;

  if (trans_ref) {
    for (i=0;i<hash_table_size;i++) {
      (trans_ref+i)->prefer.word2^=(trans_ref+i)->prefer.word1;
      (trans_ref+i)->prefer.word1=((trans_ref+i)->prefer.word1 &
                              mask_clear_entry) | (BITBOARD) 65536;
      (trans_ref+i)->prefer.word2^=(trans_ref+i)->prefer.word1;
      (trans_ref+i)->always[0].word2^=(trans_ref+i)->always[0].word1;
      (trans_ref+i)->always[0].word1=((trans_ref+i)->always[0].word1 &
                              mask_clear_entry) | (BITBOARD) 65536;
      (trans_ref+i)->always[0].word2^=(trans_ref+i)->always[0].word1;
      (trans_ref+i)->always[1].word2^=(trans_ref+i)->always[1].word1;
      (trans_ref+i)->always[1].word1=((trans_ref+i)->always[1].word1 &
                              mask_clear_entry) | (BITBOARD) 65536;
      (trans_ref+i)->always[1].word2^=(trans_ref+i)->always[1].word1;
    }
    if (dopawnstoo) {
      for (i=0;i<pawn_hash_table_size;i++) {
        (pawn_hash_table+i)->key=0;
        (pawn_hash_table+i)->p_score=0;
        (pawn_hash_table+i)->protected=0;
        (pawn_hash_table+i)->black_defects_k=0;
        (pawn_hash_table+i)->black_defects_q=0;
        (pawn_hash_table+i)->white_defects_k=0;
        (pawn_hash_table+i)->white_defects_q=0;
        (pawn_hash_table+i)->passed_w=0;
        (pawn_hash_table+i)->passed_w=0;
        (pawn_hash_table+i)->outside=0;
        (pawn_hash_table+i)->candidates_w=0;
        (pawn_hash_table+i)->candidates_b=0;
      }
    }
  }
  local[0]->pawn_score.key=0;
}

void DisplayBitBoard(BITBOARD board) {
  int i,j,x;
  for(i=7;i>=0;i--) {
    printf("  %2d ",i*8);
    x=board&255;
    board>>=8;
    for(j=128;j>0;j=j>>1)
      if(x & j) 
        printf("X ");
      else
        printf("- ");
    printf("\n");
  }
}

/*
********************************************************************************
*                                                                              *
*   DisplayChessBoard() is used to display the board since it is kept in       *
*   both the bit-board and array formats, here we use the array format which   *
*   is nearly ready for display as is.                                         *
*                                                                              *
********************************************************************************
*/
void DisplayChessBoard(FILE *display_file, POSITION pos) {
  int display_board[64];
  static const char display_string[16][4] =
    {"(Q)","(R)","(B)","   ","(K)","(N)","(P)","   ",
     " P "," N "," K ","   "," B "," R "," Q "," . "};
  int i,j;
/*
 ----------------------------------------------------------
|                                                          |
|   first, convert square values to indices to the proper  |
|   text string.                                           |
|                                                          |
 ----------------------------------------------------------
*/
  for(i=0;i<64;i++) {
    display_board[i]=pos.board[i]+7;
    if (pos.board[i] == 0) {
      if (((i/8)&1) == ((i%8)&1)) display_board[i]=15;
    }
  }
/*
 ----------------------------------------------------------
|                                                          |
|   now that that's done, simply display using 8 squares   |
|   per line.                                              |
|                                                          |
 ----------------------------------------------------------
*/
  fprintf(display_file,"\n       +---+---+---+---+---+---+---+---+\n");
  for(i=7;i>=0;i--) {
    fprintf(display_file,"   %2d  ",i+1);
    for(j=0;j<8;j++)
      fprintf(display_file,"|%s",display_string[display_board[i*8+j]]);
    fprintf(display_file,"|\n");
    fprintf(display_file,"       +---+---+---+---+---+---+---+---+\n");
  }
  fprintf(display_file,"         a   b   c   d   e   f   g   h\n\n");
}

char* DisplayEvaluation(int value, int wtm) {
  static char out[10];
  int tvalue;

  tvalue=(wtm) ? value : -value;
  if (abs(value) < MATE-300) 
    sprintf(out,"%7.2f",((float) tvalue)/100.0);
  else if (abs(value) > MATE) {
    if (tvalue < 0) sprintf(out," -infnty");
    else sprintf(out," +infnty");
  }
  else if (value==MATE-2 && wtm) sprintf(out,"   Mate");
  else if (value==MATE-2 && !wtm) sprintf(out,"  -Mate");
  else if (value==-(MATE-1) && wtm) sprintf(out,"  -Mate");
  else if (value==-(MATE-1) && !wtm) sprintf(out,"   Mate");
  else if (value>0 && wtm) sprintf(out,"  Mat%.2d",(MATE-value)/2);
  else if (value>0 && !wtm) sprintf(out," -Mat%.2d",(MATE-value)/2);
  else if (wtm) sprintf(out," -Mat%.2d",(MATE-abs(value))/2);
  else sprintf(out,"  Mat%.2d",(MATE-abs(value))/2);
  return(out);
}

char* DisplayEvaluationKibitz(int value, int wtm) {
  static char out[10];
  int tvalue;

  tvalue=(wtm) ? value : -value;
  if (abs(value) < MATE-300) 
    sprintf(out,"%+.2f",((float) tvalue)/100.0);
  else if (abs(value) > MATE) {
    if (tvalue < 0) sprintf(out,"-infnty");
    else sprintf(out,"+infnty");
  }
  else if (value==MATE-2 && wtm) sprintf(out,"Mate");
  else if (value==MATE-2 && !wtm) sprintf(out,"-Mate");
  else if (value==-(MATE-1) && wtm) sprintf(out,"-Mate");
  else if (value==-(MATE-1) && !wtm) sprintf(out,"Mate");
  else if (value>0 && wtm) sprintf(out,"Mat%.2d",(MATE-value)/2);
  else if (value>0 && !wtm) sprintf(out,"-Mat%.2d",(MATE-value)/2);
  else if (wtm) sprintf(out,"-Mat%.2d",(MATE-abs(value))/2);
  else sprintf(out,"Mat%.2d",(MATE-abs(value))/2);
  return(out);
}

void DisplayPieceBoards(signed char *white, signed char *black) {
  int i,j;
  printf("                 white                      ");
  printf("                 black\n");
  for (i=7;i>=0;i--) {
    for (j=i*8;j<i*8+8;j++) printf("%4d ",white[j]);
    printf("    ");
    for (j=i*8;j<i*8+8;j++) printf("%4d ",black[j]);
    printf("\n");
  }
}

/* last modified 01/11/99 */
/*
********************************************************************************
*                                                                              *
*   DisplayPV() is used to display a PV during the search.  it will also note  *
*   when the PV was terminated by a hash table hit and will check the hash     *
*   entries to see if the PV can be extended by using moves from hits.         *
*                                                                              *
********************************************************************************
*/
void DisplayPV(TREE *tree, int level, int wtm, int time, int value, PATH *pv) {
  char buffer[512], *buffp, *bufftemp;
  int i, t_move_number, type, j, dummy;
  int nskip=0, twtm=wtm;
  root_print_ok=root_print_ok || tree->nodes_searched>noise_level ||
                abs(value)>MATE-300;
/*
 ----------------------------------------------------------
|                                                          |
|   initialize.                                            |
|                                                          |
 ----------------------------------------------------------
*/
#if defined(SMP)
  for (i=0;i<n_root_moves;i++)
    if (!(root_moves[i].status&128) && root_moves[i].status&64) nskip++;
#endif
  if (level==5) type=4; else type=2;
  t_move_number=move_number;
  if (display_options&64) sprintf(buffer," %d.",move_number);
  else buffer[0]=0;
  if ((display_options&64) && !wtm) sprintf(buffer+strlen(buffer)," ...");
  for (i=1;i<=(int) pv->pathl;i++) {
    if ((display_options&64) && i>1 && wtm)
      sprintf(buffer+strlen(buffer)," %d.",t_move_number);
    sprintf(buffer+strlen(buffer)," %s",OutputMove(tree,pv->path[i],i,wtm));
    MakeMove(tree,i,pv->path[i],wtm);
    wtm=Flip(wtm);
    if (wtm) t_move_number++;
  }
/*
 ----------------------------------------------------------
|                                                          |
|   if the pv was terminated prematurely by a trans/ref    |
|   hit, see if any more moves are in the trans/ref table  |
|   and if so, add 'em to the end of the PV so we will     |
|   have better move ordering next iteration.              |
|                                                          |
 ----------------------------------------------------------
*/
  if(pv->pathh == 1) {
    for (i=pv->pathl+1;i<MAXPLY;i++) {
      HashProbe(tree,i,0,wtm,&dummy,&dummy,&dummy);
      if (tree->hash_move[i] && LegalMove(tree,i,wtm,tree->hash_move[i])) {
        pv->path[i]=tree->hash_move[i];
        for (j=1;j<i;j++) 
          if (pv->path[i] == pv->path[j]) break;
        if (j < i) break;
        pv->pathl++;
        if ((display_options&64) && wtm)
          sprintf(buffer+strlen(buffer)," %d.",t_move_number);
        sprintf(buffer+strlen(buffer)," %s",OutputMove(tree,pv->path[i],i,wtm));
        MakeMove(tree,i,pv->path[i],wtm);
      }
      else break;
      wtm=Flip(wtm);
      if (wtm) t_move_number++;
    }
    sprintf(buffer+strlen(buffer)," <HT>");
  }
  else if(pv->pathh == 2) 
    sprintf(buffer+strlen(buffer)," <EGTB>");
  strcpy(kibitz_text,buffer);
  if (nskip>1 && max_threads>1) sprintf(buffer+strlen(buffer)," (s=%d)",nskip);
  if (root_print_ok) {
#if defined(SMP)
    Lock(lock_io);
#endif
    Print(type,"               ");
    if (level==6)
      Print(type,"%2i   %s%s   ",iteration_depth,
            DisplayTime(time),DisplayEvaluation(value,twtm));
    else
      Print(type,"%2i-> %s%s   ",iteration_depth,
            DisplayTime(time),DisplayEvaluation(value,twtm));
    buffp=buffer+1;
    do {
      if ((int) strlen(buffp) > 34) 
        bufftemp=strchr(buffp+34,' ');
      else 
        bufftemp=0;
      if (bufftemp) *bufftemp=0;
      Print(type,"%s\n",buffp);
      buffp=bufftemp+1;
      if (bufftemp) Print(type,"                                    ");
    } while(bufftemp);
    Kibitz(level,twtm,iteration_depth,end_time-start_time,value,
            tree->nodes_searched,0,tree->egtb_probes_successful,
            kibitz_text);
#if defined(SMP)
    Unlock(lock_io);
#endif
  }
  for (i=pv->pathl;i>0;i--) {
    wtm=Flip(wtm);
    UnmakeMove(tree,i,pv->path[i],wtm);
  }
}

char* DisplaySQ(unsigned int sq) {
  static char out[3];
  out[0]=File(From(sq))+'a';
  out[1]=Rank(From(sq))+'1';
  out[2]=0;
  return(out);
}

char* DisplayHHMM(unsigned int time) {
  static char out[10];

  time=time/6000;
  sprintf(out,"%3u:%02u", time/60, time%60);
  return(out);
}

char* DisplayTime(unsigned int time) {
  static char out[10];

  if (time < 6000) sprintf(out,"%6.2f",(float) time/100.0);
  else {
    time=time/100;
    sprintf(out,"%3u:%02u", time/60, time%60);
  }
  return(out);
}

char* DisplayTimeKibitz(unsigned int time) {
  static char out[10];

  if (time < 6000) sprintf(out,"%.2f",(float) time/100.0);
  else {
    time=time/100;
    sprintf(out,"%u:%02u", time/60, time%60);
  }
  return(out);
}

void DisplayTreeState(TREE *tree, int sply, int spos, int maxply) {
  int left, i, *mvp, parallel=0;
  char buf[1024];
  buf[0]=0;
  if (sply == 1) {
    left=0;
    for (i=0;i<n_root_moves;i++)
      if (!(root_moves[i].status&128)) left++;
    sprintf(buf,"%d:%d/%d  ",1,left,n_root_moves);
  }
  else {
    for (i=0;i<spos-6;i++) sprintf(buf+strlen(buf)," ");
    sprintf(buf+strlen(buf),"[p%2d] ",tree->thread_id);
  }
  for (i=Max(sply,2);i<=maxply;i++) {
    left=0;
    for (mvp=tree->last[i-1];mvp<tree->last[i];mvp++) 
      if (*mvp) left++;
    sprintf(buf+strlen(buf),"%d:%d/%d  ",i,left,
            (int) (tree->last[i]-tree->last[i-1]));
    if (!(i%8)) sprintf(buf+strlen(buf),"\n");
    if (tree->nprocs>1 && tree->ply==i) {
      parallel=strlen(buf);
      break;
    }
    if (sply > 1) break;
  }
  printf("%s\n",buf);
  if (sply == 1 && tree->nprocs) {
    for (i=0;i<max_threads;i++) if (tree->siblings[i])
        DisplayTreeState(tree->siblings[i], tree->ply+1,parallel, maxply);
  }
}

void Display64bitWord(BITBOARD word) {
  printf("%08x%08x\n",(int)(word>>32),(int)word);
}

void Display2BitBoards(BITBOARD board1, BITBOARD board2) {
  int i,j,x,y;
  for(i=7;i>=0;i--) {
    printf("  %2d ",i*8);
    x=board1&255;
    board1>>=8;
    for(j=128;j>0;j=j>>1)
      if(x & j) printf("X ");
      else printf("- ");
    printf("     %2d ",i*8);
    y=board2&255;
    board2>>=8;
    for(j=128;j>0;j=j>>1)
      if(y & j) printf("X ");
      else printf("- ");
    printf("\n");
  }
}

/* last modified 12/27/99 */
/*
********************************************************************************
*                                                                              *
*   EGTBPV() is used to display the full PV (path) for a mate/mated in N EGTB  *
*   position.                                                                  *
*                                                                              *
********************************************************************************
*/
void EGTBPV(TREE *tree, int wtm) {
  int moves[1024], current[256];
  BITBOARD hk[1024], phk[1024];
  char buffer[16384], *next;
  BITBOARD pos[1024];
  int value;
  register int ply, i, j, nmoves, *last, t_move_number;
  register int best=0, bestmv=0, optimal_mv=0;
  register int legal;
/*
 ----------------------------------------------------------
|                                                          |
|   first, see if this is a known EGTB position.  if not,  |
|   we can bug out right now.                              |
|                                                          |
 ----------------------------------------------------------
*/
  if (!EGTB_setup) return;
  tree->position[1]=tree->position[0];
  if (WhiteCastle(1)+BlackCastle(1)) return;
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
    legal=0;
    for (i=0;i<nmoves;i++) {
      MakeMove(tree,1,current[i],wtm);
      if (!Check(wtm)) {
        legal++;
        if(TotalPieces==2 || EGTBProbe(tree, 2, Flip(wtm), &value)) {
          if (TotalPieces > 2) value=-value;
          else value=DrawScore(wtm);
          if (value > best) {
            best=value;
            bestmv=current[i];
            optimal_mv=1;
          }
          else if (value == best) optimal_mv=0;
        }
      }
      UnmakeMove(tree,1,current[i],wtm);
    }
    if (best > -MATE-1) {
      moves[ply]=bestmv;
      if ((display_options&64) && ply>1 && wtm)
        sprintf(buffer+strlen(buffer)," %d.",t_move_number);
      sprintf(buffer+strlen(buffer)," %s",OutputMove(tree,bestmv,1,wtm));
      if (!strchr(buffer,'#') && legal>1 &&
          optimal_mv) sprintf(buffer+strlen(buffer),"!");
      hk[ply]=HashKey;
      phk[ply]=PawnHashKey;
      MakeMove(tree,1,bestmv,wtm);
      tree->position[1]=tree->position[2];
      wtm=Flip(wtm);
      for (j=2-(ply&1);j<ply;j+=2)
        if (pos[ply] == pos[j]) break;
      if (j < ply) break;
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
    wtm=Flip(wtm);
      tree->save_hash_key[1]=hk[ply];
      tree->save_pawn_hash_key[1]=phk[ply];
    UnmakeMove(tree,1,moves[ply],wtm);
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

void DisplayChessMove(char *title, int move) {
  Print(4095,"%s  piece=%d, from=%d, to=%d, captured=%d, promote=%d\n",
         title,Piece(move),From(move), To(move),Captured(move),
         Promote(move));
}

/* last modified 02/17/98 */
/*
********************************************************************************
*                                                                              *
*   FormatPV() is used to display a PV during the search.  it will also note   *
*   when the PV was terminated by a hash table hit.                            *
*                                                                              *
********************************************************************************
*/
char *FormatPV(TREE *tree, int wtm, PATH pv) {
  static char buffer[512];
  int i, t_move_number;
/*
 ----------------------------------------------------------
|                                                          |
|   initialize.                                            |
|                                                          |
 ----------------------------------------------------------
*/
  t_move_number=move_number;
  if (display_options&64) sprintf(buffer," %d.",move_number);
  else buffer[0]=0;
  if ((display_options&64) && !wtm) sprintf(buffer+strlen(buffer)," ...");
  for (i=1;i<=(int) pv.pathl;i++) {
    if ((display_options&64) && i>1 && wtm)
      sprintf(buffer+strlen(buffer)," %d.",t_move_number);
    sprintf(buffer+strlen(buffer)," %s",OutputMove(tree,pv.path[i],i,wtm));
    MakeMove(tree,i,pv.path[i],wtm);
    wtm=Flip(wtm);
    if (wtm) t_move_number++;
  }
  for (i=pv.pathl;i>0;i--) {
    wtm=Flip(wtm);
    UnmakeMove(tree,i,pv.path[i],wtm);
  }
  return (buffer);
}

unsigned int ReadClock(TIME_TYPE type) {
#if defined(UNIX) || defined(AMIGA)
  struct tms t;
  struct timeval timeval;
  struct timezone timezone;
  BITBOARD cputime=0;
#endif
#if defined(MACOS)
  return(clock() * 100 / CLOCKS_PER_SEC);
#endif
#if defined(NT_i386)
  HANDLE hThread;
  FILETIME ftCreate, ftExit, ftKernel, ftUser;
  unsigned int cputime;
  BITBOARD tUser64;
#endif

  switch (type) {
#if defined(UNIX) || defined(AMIGA)
    case cpu:
      (void) times(&t);
      cputime=t.tms_utime+t.tms_stime+t.tms_cutime+t.tms_cstime;
#  if defined(CLK_TCK)
      cputime=cputime*100/CLK_TCK;
      return((unsigned int) cputime);
#  else
      if (!clk_tck) clk_tck = sysconf(_SC_CLK_TCK);
      cputime=cputime*100/clk_tck;
      return((unsigned int) cputime);
#  endif
    case elapsed:
      gettimeofday(&timeval, &timezone);
      return(timeval.tv_sec*100+(timeval.tv_usec/10000));
    default:
      gettimeofday(&timeval, &timezone);
      return(timeval.tv_sec*100+(timeval.tv_usec/10000));
#endif
#if defined(NT_i386)
    case cpu:
      hThread = GetCurrentThread();
      if (GetThreadTimes(hThread, &ftCreate, &ftExit, &ftKernel,
&ftUser)) {
        tUser64 = *(BITBOARD *)&ftUser;
        cputime=(unsigned int)(tUser64/100000);
      }
      return(cputime);
    case elapsed:
      return( (unsigned int) GetTickCount()/10);
    default:
      return( (unsigned int) GetTickCount()/10);
#endif
  }
}

#if defined(SMP)
/*
********************************************************************************
*                                                                              *
*   FindBlockID() converts a thread block pointer into an ID that is easier to *
*   understand when debugging.                                                 *
*                                                                              *
********************************************************************************
*/
int FindBlockID(TREE *block) {
  int i;
  for (i=0;i<MAX_BLOCKS+1;i++)
    if (block == local[i]) return(i);
  return(-1);
}
#endif

/*
********************************************************************************
*                                                                              *
*   HasOpposition() is used to determine if one king stands in "opposition"    *
*   to the other.  if the kings are opposed on the same file or else are       *
*   opposed on the same diagonal, then the side not-to-move has the opposition *
*   and the side-to-move must give way.                                        *
*                                                                              *
********************************************************************************
*/
int HasOpposition(int on_move, int white_king, int black_king) {
  register int file_distance, rank_distance;
  file_distance=FileDistance(white_king,black_king);
  rank_distance=RankDistance(white_king,black_king);
  if (rank_distance < 2) return(1);
  if (on_move) {
    if (rank_distance&1) {
      rank_distance--;
      if (file_distance&1) file_distance--;
    }
    else if (file_distance&1) {
      file_distance--;
      if (rank_distance&1) rank_distance--;
    }
  }
  if (!(file_distance&1) && !(rank_distance&1)) return(1);
  if (!(file_distance&1) && !(rank_distance&1)) return(1);
  return(0);
}

/*
********************************************************************************
*                                                                              *
*   InterposeSquares() is used to compute the set of squares that block an     *
*   attack on the king by a sliding piece, by interposing any piece between    *
*   the attacking piece and the king on the same ray.                          *
*                                                                              *
********************************************************************************
*/
BITBOARD InterposeSquares(int check_direction, int king_square, 
                          int checking_square) {
  register BITBOARD target;
/*
 ----------------------------------------------------------
|                                                          |
|   if this is a check from a single sliding piece, then   |
|   we can interpose along the checking rank/file/diagonal |
|   and block the check.  otherwise, interposing is not a  |
|   possibility.                                           |
|                                                          |
 ----------------------------------------------------------
*/
  switch (check_direction) {
    case +1:
      target=plus1dir[king_square-1] ^ plus1dir[checking_square];
      break;
    case +7:
      target=plus7dir[king_square-7] ^ plus7dir[checking_square];
      break;
    case +8:
      target=plus8dir[king_square-8] ^ plus8dir[checking_square];
      break;
    case +9:
      target=plus9dir[king_square-9] ^ plus9dir[checking_square];
      break;
    case -1:
      target=minus1dir[king_square+1] ^ minus1dir[checking_square];
      break;
    case -7:
      target=minus7dir[king_square+7] ^ minus7dir[checking_square];
      break;
    case -8:
      target=minus8dir[king_square+8] ^ minus8dir[checking_square];
      break;
    case -9:
      target=minus9dir[king_square+9] ^ minus9dir[checking_square];
      break;
    default:
      target=0;
      break;
  }
  return(target);
}
 
int KingPawnSquare(int pawn, int king, int queen, int ptm) {
  register int pdist, kdist;
  pdist=abs(Rank(pawn)-Rank(queen));
  kdist=(abs(Rank(king)-Rank(queen)) > abs(File(king)-File(queen))) ? 
    abs(Rank(king)-Rank(queen)) : abs(File(king)-File(queen));
  if (!ptm) pdist++;
  if (pdist < kdist) return(0);
  else return(1);
}

/* last modified 03/13/01 */
/*
********************************************************************************
*                                                                              *
*   NewGame() is used to initialize the chess position and timing controls to  *
*   the setup needed to start a new game.                                      *
*                                                                              *
********************************************************************************
*/
void NewGame(int save) {
  char filename[64];
  static int save_book_selection_width=5;
  static int save_kibitz=0, save_channel=0;
  static int save_resign=0, save_resign_count=0, save_draw_count=0;
  static int save_learning=0;
  static int save_accept_draws=0;
  TREE * const tree=local[0];

  new_game=0;
  if (save) {
    save_book_selection_width=book_selection_width;
    save_kibitz=kibitz;
    save_channel=channel;
    save_resign=resign;
    save_resign_count=resign_count;
    save_draw_count=draw_count;
    save_learning=learning;
    save_accept_draws=accept_draws;
  }
  else {
    if (learning&book_learning && moves_out_of_book) {
      int val=(crafty_is_white)?last_search_value:-last_search_value;
      LearnBook(tree,crafty_is_white,val,0,0,1);
    }
    if (ics) printf("*whisper Hello from Crafty v%s !\n",version);
    if (xboard) {
      printf("tellicsnoalias set 1 Crafty v%s (%d cpus)\n",
             version,Max(1,max_threads));
    }
    over=0;
    moves_out_of_book=0;
    ponder_move=0;
    last_search_value=0;
    last_pv.pathd=0;
    last_pv.pathl=0;
    strcpy(initial_position,"");
    InitializeChessBoard(&tree->position[0]);
    InitializeHashTables();
    force=0;
    trojan_check=0;
    computer_opponent=0;
    books_file=normal_bs_file;
    draw_score[0]=0;
    draw_score[1]=0;
    wtm=1;
    move_number=1;
    tc_time_remaining=tc_time;
    tc_time_remaining_opponent=tc_time;
    tc_moves_remaining=tc_moves;
    if (move_actually_played) {
      if (log_file) fclose(log_file);
      if (history_file) fclose(history_file);
      if (log_file) {
        if (log_id < 299) log_id++;
#if defined(MACOS)
        sprintf(filename,":%s:log.%03d",log_path,log_id);
#else
        sprintf(filename,"%s/log.%03d",log_path,log_id);
#endif
        log_file=fopen(filename,"w+");
      }
#if defined(MACOS)
      sprintf(filename,":%s:game.%03d",log_path,log_id);
#else
      sprintf(filename,"%s/game.%03d",log_path,log_id);
#endif
      history_file=fopen(filename,"w+");
    }
    move_actually_played=0;
    book_selection_width=save_book_selection_width;
    kibitz=save_kibitz;
    channel=save_channel;
    resign=save_resign;
    resign_count=save_resign_count;
    resign_counter=0;
    draw_count=save_draw_count;
    accept_draws=save_accept_draws;
    draw_counter=0;
    usage_level=0;
    learning=save_learning;
    lazy_eval_cutoff=200;
    largest_positional_score=300;
    predicted=0;
    kibitz_depth=0;
    tree->nodes_searched=0;
    tree->fail_high=0;
    tree->fail_high_first=0;
    cpu_percent=0;
    kibitz_text[0]=0;
  }
}

char* Normal(void) {
#if defined(NT_i386)
  HANDLE  std_console;
  std_console = GetStdHandle(STD_OUTPUT_HANDLE);
#endif

  if (ansi) {
#if defined(UNIX) || defined(AMIGA)
    return("\033[0m");
#elif defined(NT_i386)
    SetConsoleTextAttribute(std_console, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
    return("");
#else
    return("\033[1;44;37m");
#endif
  }
  return("");
}

int ParseTime(char* string) {
  int time=0;
  int minutes=0;
  while (*string) {
    switch (*string) {
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
        minutes=minutes*10+(*string)-'0';
        break;
      case ':':
        time=time*60+minutes;
        minutes=0;
        break;
      default: Print(4095,"illegal character in time, please re-enter\n");
        break;
    }
    string++;
  }
  return(time*60+minutes);
}

void Pass(void) {
  char buffer[128];
  const int halfmoves_done=2*(move_number-1)+(1-wtm);
  int prev_pass=0;
  /* Was previous move a pass? */
  if (halfmoves_done>0) {
    fseek(history_file,(halfmoves_done-1)*10,SEEK_SET);
    if (fscanf(history_file,"%s",buffer)==0 || strcmp(buffer,"pass")==0) {
      prev_pass=1;
    }
  }
  if (prev_pass) {
    if (wtm) move_number--;
  } else {
    fseek(history_file,halfmoves_done*10,SEEK_SET);
    fprintf(history_file,"%9s\n","pass");
    if (!wtm) move_number++;
  }
  wtm=Flip(wtm);
}


/*
********************************************************************************
*                                                                              *
*   PinnedOnKing() is used to determine if the piece on <square> is pinned     *
*   against the king, so that it's illegal to move it.  this is used to screen *
*   potential moves by GenerateCheckEvasions() so that illegal moves are not   *
*   produced.                                                                  *
*                                                                              *
********************************************************************************
*/
int PinnedOnKing(TREE * RESTRICT tree, int wtm, int square) {
  register int ray;
  if (wtm) {
/*
 ----------------------------------------------------------
|                                                          |
|   first, determine if the piece being moved is on the    |
|   same diagonal, rank or file as the king.               |
|                                                          |
 ----------------------------------------------------------
*/
    ray=directions[square][WhiteKingSQ];
    if (!ray) return(0);
/*
 ----------------------------------------------------------
|                                                          |
|   if they are on the same ray, then determine if the     |
|   king blocks a bishop attack in one direction from this |
|   square and a bishop or queen blocks a bishop attack    |
|   on the same diagonal in the opposite direction.        |
|                                                          |
 ----------------------------------------------------------
*/
    switch (abs(ray)) {
    case 1: 
      if (AttacksRank(square) & WhiteKing)
        return((AttacksRank(square) & RooksQueens & BlackPieces) != 0);
      else return(0);
    case 7: 
      if (AttacksDiaga1(square) & WhiteKing)
        return((AttacksDiaga1(square) & BishopsQueens & BlackPieces) != 0);
      else return(0);
    case 8: 
      if (AttacksFile(square) & WhiteKing)
        return((AttacksFile(square) & RooksQueens & BlackPieces) != 0);
      else return(0);
    case 9: 
      if (AttacksDiagh1(square) & WhiteKing)
        return((AttacksDiagh1(square) & BishopsQueens & BlackPieces) != 0);
      else return(0);
    }
  }
  else {
/*
 ----------------------------------------------------------
|                                                          |
|   first, determine if the piece being moved is on the    |
|   same diagonal, rank or file as the king.               |
|                                                          |
 ----------------------------------------------------------
*/
    ray=directions[BlackKingSQ][square];
    if (!ray) return(0);
/*
 ----------------------------------------------------------
|                                                          |
|   if they are on the same ray, then determine if the     |
|   king blocks a bishop attack in one direction from this |
|   square and a bishop or queen blocks a bishop attack    |
|   on the same diagonal in the opposite direction.        |
|                                                          |
 ----------------------------------------------------------
*/
    switch (abs(ray)) {
    case 1: 
      if (AttacksRank(square) & BlackKing)
        return((AttacksRank(square) & RooksQueens & WhitePieces) != 0);
      else return(0);
    case 7: 
      if (AttacksDiaga1(square) & BlackKing)
        return((AttacksDiaga1(square) & BishopsQueens & WhitePieces) != 0);
      else return(0);
    case 8: 
      if (AttacksFile(square) & BlackKing)
        return((AttacksFile(square) & RooksQueens & WhitePieces) != 0);
      else return(0);
    case 9: 
      if (AttacksDiagh1(square) & BlackKing)
        return((AttacksDiagh1(square) & BishopsQueens & WhitePieces) != 0);
      else return(0);
    }
  }
  return(0);
}

void Print(int vb, char *fmt, ...) {
  va_list ap;
  va_start(ap,fmt);
  if (vb&display_options) vprintf(fmt, ap);
  fflush(stdout);
  if (time_limit>99 || tc_time_remaining>6000 || vb==4095) {
    va_start(ap,fmt);
    if (log_file) vfprintf(log_file, fmt, ap);
    if (log_file) fflush(log_file);
  }
  va_end(ap);
}

char *PrintKM(int val, int realK) {
  static char buf[16];
  if (realK) {
    if (val>=1<<20 && !(val&((1<<20)-1)))
      sprintf(buf,"%4dM", val/(1<<20));
    else if (val >= 1<<10)
      sprintf(buf,"%4dK", val/(1<<10));
    else
      sprintf(buf,"%5d", val);
    return (buf);
  }
  else {
    if (val>=1000000 && !(val%1000000))
      sprintf(buf,"%4dM", val/1000000);
    else if (val >= 1000)
      sprintf(buf,"%4dK", val/1000);
    else
      sprintf(buf,"%5d", val);
    return (buf);
  }
}

/*

A 32 bit random number generator. An implementation in C of the algorithm given by
Knuth, the art of computer programming, vol. 2, pp. 26-27. We use e=32, so 
we have to evaluate y(n) = y(n - 24) + y(n - 55) mod 2^32, which is implicitly
done by unsigned arithmetic.

*/

unsigned int Random32(void) {
  /*
  random numbers from Mathematica 2.0.
  SeedRandom = 1;
  Table[Random[Integer, {0, 2^32 - 1}]
  */
  static const unsigned long x[55] = {
    1410651636UL, 3012776752UL, 3497475623UL, 2892145026UL, 1571949714UL,
    3253082284UL, 3489895018UL, 387949491UL, 2597396737UL, 1981903553UL,
    3160251843UL, 129444464UL, 1851443344UL, 4156445905UL, 224604922UL,
    1455067070UL, 3953493484UL, 1460937157UL, 2528362617UL, 317430674UL, 
    3229354360UL, 117491133UL, 832845075UL, 1961600170UL, 1321557429UL,
    747750121UL, 545747446UL, 810476036UL, 503334515UL, 4088144633UL,
    2824216555UL, 3738252341UL, 3493754131UL, 3672533954UL, 29494241UL,
    1180928407UL, 4213624418UL, 33062851UL, 3221315737UL, 1145213552UL,
    2957984897UL, 4078668503UL, 2262661702UL, 65478801UL, 2527208841UL,
    1960622036UL, 315685891UL, 1196037864UL, 804614524UL, 1421733266UL,
    2017105031UL, 3882325900UL, 810735053UL, 384606609UL, 2393861397UL };
  static int init = 1;
  static unsigned long y[55];
  static int j, k;
  unsigned long ul;
  
  if (init)
  {
    int i;
    
    init = 0;
    for (i = 0; i < 55; i++) y[i] = x[i];
    j = 24 - 1;
    k = 55 - 1;
  }
  
  ul = (y[k] += y[j]);
  if (--j < 0) j = 55 - 1;
  if (--k < 0) k = 55 - 1;
  return((unsigned int)ul);
}

BITBOARD Random64(void) {
  BITBOARD result;
  unsigned int r1, r2;

  r1=Random32();
  r2=Random32();
  result=r1 | (BITBOARD) r2<<32;
  return (result);
}

/* last modified 05/06/97 */
/*
********************************************************************************
*                                                                              *
*   Read() copies data from the command_buffer into a local buffer, and then   *
*   uses ReadParse to break this command up into tokens for processing.        *
*                                                                              *
********************************************************************************
*/
int Read(int wait, char *buffer) {
  char *eol, *ret, readdata;

  *buffer=0;
#if defined(DGT)
  if (DGT_active && DGTCheckInput()) DGTRead();
#endif
/*
   case 1:  we have a complete command line, with terminating
   N/L character in the buffer.  we can simply extract it from
   the I/O buffer, parse it and return.
*/
  if (strchr(cmd_buffer,'\n'));
/*
   case 2:  the buffer does not contain a complete line.  If we
   were asked to not wait for a complete command, then we first
   see if I/O is possible, and if so, read in what is available.
   If that includes a N/L, then we are ready to parse and return.
   If not, we return indicating no input available just yet.
*/
  else if (!wait) {
    if (CheckInput()) {
      readdata=ReadInput();
      if (!strchr(cmd_buffer,'\n')) return(0);
      if (!readdata) return(-1);
    }
    else return(0);
  }
/*
   case 3:  the buffer does not contain a complete line, but we
   were asked to wait until a complete command is entered.  So we
   hang by doing a ReadInput() and continue doing so until we get
   a N/L character in the buffer.  Then we parse and return.
*/
  else while (!strchr(cmd_buffer,'\n')) {
#if defined(DGT)
    if (DGT_active) {
      fd_set readfds;
      struct timeval tv;
    
      FD_ZERO (&readfds);
      FD_SET (from_dgt, &readfds);
      FD_SET (fileno(stdin), &readfds);
      tv.tv_sec=999999;
      tv.tv_usec=0;
      (void) select(32, &readfds, 0, 0, &tv);
      if (FD_ISSET(from_dgt, &readfds)) DGTRead();
      if (FD_ISSET(fileno(stdin), &readfds)) readdata=ReadInput();
    }
    else {
#endif
      readdata=ReadInput();
      if (!readdata) return(-1);
#if defined(DGT)
    }
#endif
  }

  eol=strchr(cmd_buffer,'\n');
  *eol=0;
  ret=strchr(cmd_buffer,'\r');
  if (ret) *ret=' ';
  strcpy(buffer,cmd_buffer);
  memmove(cmd_buffer,eol+1,strlen(eol+1)+1);
  return(1);
}

/* last modified 04/23/97 */
/*
********************************************************************************
*                                                                              *
*   ReadClear() clears the input buffer when input_stream is being switched to *
*   a file, since we have info buffered up from a different input stream.      *
*                                                                              *
********************************************************************************
*/
void ReadClear() {
  cmd_buffer[0]=0;
}

/* last modified 05/06/97 */
/*
********************************************************************************
*                                                                              *
*   ReadParse() takes one complete command-line, and breaks it up into tokens. *
*   common delimiters are used, such as " ", ",", "/" and ";", any of which    *
*   delimit fields.                                                            *
*                                                                              *
********************************************************************************
*/
int ReadParse(char *buffer, char *args[], char *delims) {
  char *next, tbuffer[512];
  int nargs;

  strcpy(tbuffer,buffer);
  for (nargs=0;nargs<16;nargs++) *(args[nargs])=0;
  next=strtok(tbuffer,delims);
  if (!next) return(0);
  strcpy(args[0],next);
  for (nargs=1;nargs<32;nargs++) {
    next=strtok(0,delims);
    if (!next) break;
    strcpy(args[nargs],next);
  }
  return(nargs);
}

/* last modified 04/23/97 */
/*
********************************************************************************
*                                                                              *
*   ReadInput() reads data from the input_stream, and buffers this into the    *
*   command_buffer for later processing.                                       *
*                                                                              *
********************************************************************************
*/
int ReadInput(void) {
  char buffer[512], *end;
  int bytes;

#if defined(MACOS)
  gets((char *) &buffer);
  bytes=strlen(buffer);
  end=cmd_buffer+strlen(cmd_buffer);
  memcpy(end,buffer,bytes);
  *(end+bytes)='\n';
  *(end+bytes+1)=0;
#else
  do
    bytes=read(fileno(input_stream),buffer,512);
  while (bytes<0 && errno==EINTR);
  if (bytes == 0) {
    if (input_stream != stdin) fclose(input_stream);
    input_stream=stdin;
    return(0);
  }
  else if (bytes < 0) {
    Print(4095,"ERROR!  input I/O stream is unreadable, exiting.\n");
    exit(1);
  }
  end=cmd_buffer+strlen(cmd_buffer);
  memcpy(end,buffer,bytes);
  *(end+bytes)=0;
#endif
  return(1);
}

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
int ReadChessMove(TREE *tree, FILE *input, int wtm, int one_move) {

  static char text[128];
  char *tmove;
  int move=0, status;

  while (move == 0) {
    status=fscanf(input,"%s",text);
    if (status <= 0) return(-1);
    if (strcmp(text,"0-0") && strcmp(text,"0-0-0"))
      tmove=text+strspn(text,"0123456789.");
    else
      tmove=text;
    if (((tmove[0]>='a' && tmove[0]<='z') ||
         (tmove[0]>='A' && tmove[0]<='Z')) ||
        !strcmp(tmove,"0-0") || !strcmp(tmove,"0-0-0")) {
      if (!strcmp(tmove,"exit")) return(-1);
      move=InputMove(tree,tmove,0,wtm,1,0);
    }
    if (one_move) break;
  }
  return(move);
}

/* last modified 05/13/97 */
/*
********************************************************************************
*                                                                              *
*   ReadNextMove() is used to take a text chess move from a file, and see if   *
*   if is legal, skipping a sometimes embedded move number (1.e4 for example)  *
*   to make PGN import easier.                                                 *
*                                                                              *
********************************************************************************
*/
int ReadNextMove(TREE *tree, char *text, int ply, int wtm) {

  char *tmove;
  int move=0;

  if (strcmp(text,"0-0") && strcmp(text,"0-0-0"))
    tmove=text+strspn(text,"0123456789./-");
  else
    tmove=text;
  if (((tmove[0]>='a' && tmove[0]<='z') ||
       (tmove[0]>='A' && tmove[0]<='Z')) ||
      !strcmp(tmove,"0-0") || !strcmp(tmove,"0-0-0")) {
    if (!strcmp(tmove,"exit")) return(-1);
    move=InputMove(tree,tmove,ply,wtm,1,0);
  }
  return(move);
}

/* last modified 06/15/98 */
/*
********************************************************************************
*                                                                              *
*   this routine reads a move from a PGN file to build an opening book or for  *
*   annotating.  It returns a 1 if a header is read, it returns a 0 if a move  *
*   is read, and returns a -1 on end of file.  It counts lines and this        *
*   counter can be accessed by calling this function with a non-zero second    *
*   formal parameter.                                                          *
*                                                                              *
********************************************************************************
*/
int ReadPGN(FILE *input, int option) {
  static int data=0, lines_read=0;
  static char input_buffer[512];
  char temp[512], *eof, analysis_move[64];
  int braces=0, parens=0, brackets=0, analysis=0, last_good_line;
  
/*
 ----------------------------------------------------------
|                                                          |
|  if the line counter is being requested, return it with  |
|  no other changes being made.  if "purge" is true, clear |
|  the current input buffer.                               |
|                                                          |
 ----------------------------------------------------------
*/
  pgn_suggested_percent=0;
  if (!input) {
    lines_read=0;
    data=0;
    return(0);
  }
  if (option==-1) data=0;
  if (option==-2) return(lines_read);
/*
 ----------------------------------------------------------
|                                                          |
|  if we don't have any data in the buffer, the first step |
|  is to read the next line.                               |
|                                                          |
 ----------------------------------------------------------
*/
  while (1) {
    if (!data) {
      eof=fgets(input_buffer,512,input);
      if (!eof) return(-1);
      if (strchr(input_buffer,'\n')) *strchr(input_buffer,'\n')=0;
      if (strchr(input_buffer,'\r')) *strchr(input_buffer,'\r')=' ';
      lines_read++;
      buffer[0]=0;
      sscanf(input_buffer,"%s",buffer);
      if (buffer[0] == '[') do {
        char *bracket1, *bracket2, value[128];
        strcpy(buffer,input_buffer);
        bracket1=strchr(input_buffer,'\"');
        if (bracket1 == 0) return(1);
        bracket2=strchr(bracket1+1,'\"');
        if (bracket2 == 0) return(1);
        *bracket1=0;
        *bracket2=0;
        strcpy(value,bracket1+1);
        if (strstr(input_buffer,"Event")) strcpy(pgn_event,value);
        else if (strstr(input_buffer,"Site")) strcpy(pgn_site,value);
        else if (strstr(input_buffer,"Round")) strcpy(pgn_round,value);
        else if (strstr(input_buffer,"Date")) strcpy(pgn_date,value);
        else if (strstr(input_buffer,"WhiteElo")) strcpy(pgn_white_elo,value);
        else if (strstr(input_buffer,"White")) strcpy(pgn_white,value);
        else if (strstr(input_buffer,"BlackElo")) strcpy(pgn_black_elo,value);
        else if (strstr(input_buffer,"Black")) strcpy(pgn_black,value);
        else if (strstr(input_buffer,"Result")) strcpy(pgn_result,value);
        else if (strstr(input_buffer,"FEN")) {
          sprintf(buffer,"setboard %s",value);
          (void) Option(local[0]);
          continue;
        }
        return(1);
      } while(0);
      data=1;
    }
/*
 ----------------------------------------------------------
|                                                          |
|  if we already have data in the buffer, it is just a     |
|  matter of extracting the next move and returning it to  |
|  the caller.  if the buffer is empty, another line has   |
|  to be read in.                                          |
|                                                          |
 ----------------------------------------------------------
*/
    else {
      buffer[0]=0;
      sscanf(input_buffer,"%s",buffer);
      if (strlen(buffer) == 0) {
        data=0;
        continue;
      }
      else {
        char *skip;
        strcpy(temp,input_buffer);
        skip=strstr(input_buffer,buffer)+strlen(buffer);
        if (skip) strcpy(input_buffer,skip);
      }
/*
 ----------------------------------------------------------
|                                                          |
|  this skips over nested { or ( characters and finds the  |
|  'mate', before returning any more moves.  it also stops |
|  if a PGN header is encountered, probably due to an      |
|  incorrectly bracketed analysis variation.               |
|                                                          |
 ----------------------------------------------------------
*/
      last_good_line=lines_read;
      analysis_move[0]=0;
      if (strchr(buffer,'{') || strchr(buffer,'(')) while (1) {
        char *skip, *ch;
        analysis=1;
        while ((ch=strpbrk(buffer,"(){}[]"))) {
          if (*ch == '(') {
            *strchr(buffer,'(')=' ';
            if (!braces) parens++;
          }
          if (*ch == ')') {
            *strchr(buffer,')')=' ';
            if (!braces) parens--;
          }
          if (*ch == '{') {
            *strchr(buffer,'{')=' ';
            braces++;
          }
          if (*ch == '}') {
            *strchr(buffer,'}')=' ';
            braces--;
          }
          if (*ch == '[') {
            *strchr(buffer,'[')=' ';
            if (!braces) brackets++;
          }
          if (*ch == ']') {
            *strchr(buffer,']')=' ';
            if (!braces) brackets--;
          }
        }
        if (analysis && analysis_move[0]==0) {
          if (strspn(buffer," ") != strlen(buffer)) {
            char *tmove=analysis_move;
            sscanf(buffer,"%64s",analysis_move);
            strcpy(buffer,analysis_move);
            if (strcmp(buffer,"0-0") && strcmp(buffer,"0-0-0"))
              tmove=buffer+strspn(buffer,"0123456789.");
            else
              tmove=buffer;
            if ((tmove[0]>='a' && tmove[0]<='z') ||
                (tmove[0]>='A' && tmove[0]<='Z') ||
                !strcmp(tmove,"0-0") || !strcmp(tmove,"0-0-0"))
              strcpy(analysis_move,buffer);
            else
              analysis_move[0]=0;
          }
        }
        if (parens==0 && braces==0 && brackets==0) break;
        buffer[0]=0;
        sscanf(input_buffer,"%s",buffer);
        if (strlen(buffer) == 0) {
          eof=fgets(input_buffer,512,input);
          if (!eof) {
            parens=0;
            braces=0;
            brackets=0;
            return(-1);
          }
          if (strchr(input_buffer,'\n')) *strchr(input_buffer,'\n')=0;
          if (strchr(input_buffer,'\r')) *strchr(input_buffer,'\r')=' ';
          lines_read++;
          if (lines_read-last_good_line >= 100) {
            parens=0;
            braces=0;
            brackets=0;
            Print(4095,"ERROR.  comment spans over 100 lines, starting at line %d\n",
                  last_good_line);
            break;
          }
        }
        strcpy(temp,input_buffer);
        skip=strstr(input_buffer,buffer)+strlen(buffer);
        strcpy(input_buffer,skip);
      }
      else {
        int skip;
        if ((skip=strspn(buffer,"0123456789."))) {
          char temp[512];
          strcpy(temp,buffer+skip);
          strcpy(buffer,temp);
        }
        if (isalpha(buffer[0]) || strchr(buffer,'-')) {
          char *first, *last, *percent;
          first=input_buffer+strspn(input_buffer," ");
          if (first==0 || *first != '{') return(0);
          last=strchr(input_buffer,'}');
          if (last == 0) return(0);
          percent=strstr(first,"play");
          if (percent == 0) return(0);
          pgn_suggested_percent=atoi(percent+4+strspn(percent+4," "));
          return(0);
        }
      }
      if (analysis_move[0] && option==1) {
        strcpy(buffer,analysis_move);
        return(2);
      }
    }
  }
}

/* last modified 06/10/98 */
/*
********************************************************************************
*                                                                              *
*   RestoreGame() resets the position to the beginning of the game, and then   *
*   reads in the game.nnn history file to set the position up so that the game *
*   position matches the position at the end of the history file.              *
*                                                                              *
********************************************************************************
*/
void RestoreGame(void) {
  int i, move;
  char cmd[16];
  wtm=1;
  InitializeChessBoard(&local[0]->position[0]);
  for (i=0;i<500;i++) {
    fseek(history_file,i*10,SEEK_SET);
    strcpy(cmd,"");
    fscanf(history_file,"%s",cmd);
    if (strcmp(cmd,"pass")) {
      move=InputMove(local[0],cmd,0,wtm,1,0);
      if (move) MakeMoveRoot(local[0],move,wtm);
      else break;
    }
    wtm=Flip(wtm);
  } 
  Phase();
}
char* Reverse(void) {
#if defined(NT_i386)
  HANDLE  std_console;
  std_console = GetStdHandle(STD_OUTPUT_HANDLE);
#endif

  if (ansi) {
#if defined(UNIX) || defined(AMIGA)
    return("\033[7m");
#elif defined(NT_i386)
    SetConsoleTextAttribute(std_console, BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE | BACKGROUND_INTENSITY);
    return("");
#else
    return("\033[7;47;33m");
#endif
  }
  return("");
}

#if defined(COMPACT_ATTACKS)

#if !defined(USE_ASSEMBLY_A)

BITBOARD AttacksDiaga1Func (DIAG_INFO *diag, POSITION *boardp) {
  return AttacksDiaga1Int(diag,boardp);
}

BITBOARD AttacksDiagh1Func(DIAG_INFO *diag, POSITION *boardp) {
  return AttacksDiagh1Int(diag,boardp);
}

BITBOARD AttacksFileFunc(int square, POSITION *boardp) {
  return AttacksFileInt(square,boardp);
}

BITBOARD AttacksRankFunc(int square, POSITION *boardp) {
  BITBOARD tmp = (boardp)->w_occupied | (boardp)->b_occupied;

  unsigned char tmp2 = 
    at.rank_attack_bitboards[File(square)]
      [at.which_attack[File(square)]
        [(tmp>>((Rank(~(square))<<3)+1)) & 0x3f]];

  return ((BITBOARD)tmp2<<(Rank(~(square))<<3));
}

BITBOARD AttacksBishopFunc(DIAG_INFO *diag, POSITION *boardp) {
  return (AttacksDiaga1Int(diag,boardp) | AttacksDiagh1Int(diag,boardp));
}

BITBOARD AttacksRookFunc(int square, POSITION *boardp) {
  BITBOARD tmp = (boardp)->w_occupied | (boardp)->b_occupied;

  unsigned char tmp2 = 
    at.rank_attack_bitboards[File(square)][at.which_attack[File(square)]
        [(tmp>>((Rank(~(square))<<3)+1)) & 0x3f]];

  return (((BITBOARD) tmp2 << (Rank(~(square))<<3)) |
          AttacksFileInt(square,boardp));
}

unsigned MobilityDiaga1Func(DIAG_INFO *diag, POSITION *boardp) {
  return MobilityDiaga1Int(diag,boardp);
}

unsigned MobilityDiagh1Func(DIAG_INFO *diag, POSITION *boardp) {
  return MobilityDiagh1Int(diag,boardp);
}

unsigned MobilityFileFunc(int square, POSITION *boardp) {
  return MobilityFileInt (square,boardp);
}

unsigned MobilityRankFunc(int square, POSITION *boardp) {
  return MobilityRankInt (square,boardp);
}

#endif

const unsigned char bishop_shift_rl45[64] = {
          59, 57, 54, 50, 45, 39, 32,  0,
          57, 54, 50, 45, 39, 32,  0,  8,
          54, 50, 45, 39, 32,  0,  8, 15,
          50, 45, 39, 32,  0,  8, 15, 21,
          45, 39, 32,  0,  8, 15, 21, 60,
          39, 32,  0,  8, 15, 21, 60, 26,
          32,  0,  8, 15, 21, 60, 26, 29,
           0,  8, 15, 21, 60, 26, 29, 31 };

const unsigned char bishop_shift_rr45[64] = {
            0,  8, 15, 21, 60, 26, 29, 31,
           32,  0,  8, 15, 21, 60, 26, 29,
           39, 32,  0,  8, 15, 21, 60, 26,
           45, 39, 32,  0,  8, 15, 21, 60,
           50, 45, 39, 32,  0,  8, 15, 21,
           54, 50, 45, 39, 32,  0,  8, 15,
           57, 54, 50, 45, 39, 32,  0,  8,
           59, 57, 54, 50, 45, 39, 32,  0 };


const unsigned char init_l45[64] = {
              4,  5,  7, 10, 14, 19, 25, 56, 
              6,  8, 11, 15, 20, 26, 57, 49,
              9, 12, 16, 21, 27, 58, 50, 43,
             13, 17, 22, 28, 59, 51, 44, 38,
             18, 23, 29, 60, 52, 45, 39,  0,
             24, 30, 61, 53, 46, 40,  1, 35,
             31, 62, 54, 47, 41,  2, 36, 33,
             63, 55, 48, 42,  3, 37, 34, 32 };
        
const unsigned char init_r45[64] = {
             56, 49, 43, 38,  0, 35, 33, 32,
             25, 57, 50, 44, 39,  1, 36, 34,
             19, 26, 58, 51, 45, 40,  2, 37,
             14, 20, 27, 59, 52, 46, 41,  3,
             10, 15, 21, 28, 60, 53, 47, 42,
              7, 11, 16, 22, 29, 61, 54, 48,
              5,  8, 12, 17, 23, 30, 62, 55,
              4,  6,  9, 13, 18, 24, 31, 63 };

const unsigned char init_l90[64] = {
        0,  8, 16, 24, 32, 40, 48, 56,
        1,  9, 17, 25, 33, 41, 49, 57,
        2, 10, 18, 26, 34, 42, 50, 58,
        3, 11, 19, 27, 35, 43, 51, 59,
        4, 12, 20, 28, 36, 44, 52, 60,
        5, 13, 21, 29, 37, 45, 53, 61,
        6, 14, 22, 30, 38, 46, 54, 62,
        7, 15, 23, 31, 39, 47, 55, 63 };

/* How many attacks are there on a length n gfile from square m */
/*                             N  M */
const unsigned char n_attacks[9][8] =
{
  {  0 },
  {  1 },
  {  1,  1 },
  {  2,  1,  2 },
  {  3,  2,  2,  3 },
  {  4,  3,  4,  3,  4 },
  {  5,  4,  6,  6,  4,  5 },
  {  6,  5,  8,  9,  8,  5,  6 },
  {  7,  6, 10, 12, 12, 10,  6,  7}
};

/* How many attacks are there from all squares on a gfile of length n. */
const unsigned char n_length_attacks[9] =
{ 0, 1, 2, 5, 10, 18, 30, 47, 70 };

#define NDIAG           (7 + 1 + 7)
#define NGFILES (NDIAG + NDIAG + 1 + 1)

static struct gfile_info {
  BITBOARD *bitboard;
  unsigned char length;
  unsigned char *map;
  unsigned char *mobility;
  unsigned char inc;
} gfiles[NGFILES];

/* For each diagonal, a map from index on that diagonal to bit index */
/* in the normal bitboard representation. */
static unsigned char diag_map [NDIAG] [8];

/* For each anti diagonal, a map from index to bitboard map. */
static unsigned char anti_diag_map [NDIAG] [8];

/* For the representative file, a map from index to bitboard map. */
static unsigned char file_map [8];

/* For the representative rank, a map from index to bitboard map. */
static unsigned char rank_map [8];

#define MASK(bits)  (~((~0) << (bits)))

#define SQ(r,f) (((r)<<3) | (f))

#define DIAG_LENGTH(sq) (8 - (Rank(sq) > File(sq) ?  \
            Rank(sq) - File(sq) :  \
            File(sq) - Rank(sq)))

#define ANTI_LENGTH(sq) (8 - (Rank(sq) > File(~sq) ?  \
            Rank(sq) - File(~sq) :  \
            File(~sq) - Rank(sq)))

static void InitializeMaps(BITBOARD *temp_rank_attack_bitboards) {
  int file, rank;
  int diag;
  int gfile;
  BITBOARD *b;
  unsigned char *m;
  unsigned char *mobility_for_length[9];
  static const unsigned char diag_base [NDIAG] =
    { SQ(7,0), SQ(6,0), SQ(5,0), SQ(4,0), SQ(3,0), SQ(2,0), SQ(1,0), SQ(0,0),
      SQ(0,1), SQ(0,2), SQ(0,3), SQ(0,4), SQ(0,5), SQ(0,6), SQ(0,7) };
  static const unsigned char anti_base[NDIAG] =
    { SQ(0,0), SQ(0,1), SQ(0,2), SQ(0,3), SQ(0,4), SQ(0,5), SQ(0,6), SQ(0,7),
      SQ(1,7), SQ(2,7), SQ(3,7), SQ(4,7), SQ(5,7), SQ(6,7), SQ(7,7) };

  {
    int i;
    unsigned char *m = at.short_mobility;
    for (i = 1; i < 8; i++) {
      mobility_for_length[i] = m;
      m += n_length_attacks[i];
    }
    mobility_for_length[8] = &at.length8_mobility[0][0];
  }

  gfile = 0;

  b = diag_attack_bitboards;
  for (diag = 0; diag < NDIAG; diag++, gfile++) {
    int sq = diag_base[diag];
    const int len = DIAG_LENGTH(sq);
    const int excess = 8 - len;
    int i;

    gfiles[gfile].length = len;
    gfiles[gfile].map = diag_map[diag];
    gfiles[gfile].bitboard = b;
    gfiles[gfile].mobility = (diag < 7 ? mobility_for_length[len] : 0);
    gfiles[gfile].inc = 0;
    m = mobility_for_length[len];
    for (i = 0; i < len; i++, sq += 9) {
      diag_map[diag][i] = sq;
      diag_info[sq].d_shift = bishop_shift_rr45[sq] + 1 - excess;
      diag_info[sq].d_mask = MASK(Max(len-2,0)) << excess;
      diag_info[sq].d_which_attack = &at.which_attack[i][0];
      diag_info[sq].d_attacks = b;
      b += n_attacks[len][i];
      diag_info[sq].d_mobility = m;
      m += (len == 8 ? MAX_ATTACKS_FROM_SQUARE : n_attacks[len][i]);
    }
  }
      
  b = anti_diag_attack_bitboards;
  for (diag = 0; diag < NDIAG; diag++, gfile++) {
    int sq = anti_base[diag];
    int len = ANTI_LENGTH(sq);
    int excess = 8 - len;
    int i;

    gfiles[gfile].length = len;
    gfiles[gfile].map = anti_diag_map[diag];
    gfiles[gfile].bitboard = b;
    gfiles[gfile].mobility = 0;
    gfiles[gfile].inc = 0;
    m = mobility_for_length[len];
    for (i = 0; i < len; i++, sq += 7) {
      anti_diag_map[diag][i] = sq;
      diag_info[sq].ad_shift = bishop_shift_rl45[sq] + 1 - excess;
      diag_info[sq].ad_mask = MASK(Max(len-2,0)) << excess;
      diag_info[sq].ad_which_attack = &at.which_attack[i][0];
      diag_info[sq].ad_attacks = b;
      b += n_attacks[len][i];
      diag_info[sq].ad_mobility = m;
      m += (len == 8 ? MAX_ATTACKS_FROM_SQUARE : n_attacks[len][i]);
    }
  }    

  gfiles[gfile].length = 8;
  gfiles[gfile].map = file_map;
  gfiles[gfile].bitboard = &at.file_attack_bitboards[0][0];
  gfiles[gfile].mobility = &at.length8_mobility[0][0];
  gfiles[gfile].inc = MAX_ATTACKS_FROM_SQUARE;
  for (rank = 0; rank < 8; rank++) file_map[rank] = (rank << 3) | 7;

  gfile++;
  
  gfiles[gfile].length = 8;
  gfiles[gfile].map = rank_map;
  gfiles[gfile].bitboard = temp_rank_attack_bitboards;
  gfiles[gfile].mobility = 0;
  gfiles[gfile].inc = MAX_ATTACKS_FROM_SQUARE;
  for (file = 0; file < 8; file++) rank_map[file] = file | (7 << 3);
}

static void InitializeBrev (unsigned char brev[]) {
  unsigned value;

  for (value = 0; value < 64; value++) {
    unsigned br = 0;
    int i;
    for (i = 0; i < 6; i++) br |= (((value >> i) & 1) << (5 - i));
      brev[value] = br;
  }
}

#define MakeAttack(lower,upper) ((lower) | ((upper) << 3))

void ComputeAttacksAndMobility () {
  BITBOARD temp_rank_attack_bitboards[8][MAX_ATTACKS_FROM_SQUARE];
  int attacks_seen[MAX_ATTACKS_FROM_SQUARE];
  int attacker;
  unsigned g;
  unsigned char brev[64];

  InitializeMaps(&temp_rank_attack_bitboards[0][0]);
  InitializeBrev(brev);

  for (attacker = 0; attacker < 8; attacker++) {
    unsigned attacks_found = 0;
    unsigned gf;
    memset ((char *)attacks_seen, 0, sizeof attacks_seen);
    for (g = 0; g < 64; g++) {
      int a, p, found;
      unsigned lower, upper, attack;
      const unsigned gfile_value = g << 1;
  
      lower = 0;
      for (p = attacker-1; p >= 0; p--) {
        lower++;
        if (gfile_value & (1 << p)) break;
      }
      upper = 0;
      for (p = attacker+1; p < 8; p++) {
        upper++;
        if (gfile_value & (1 << p)) break;
      }

      attack = MakeAttack(lower,upper);
      found = 0;
      for (a=0; a<(int)attacks_found; a++)
        if ((int)attack == attacks_seen[a]) {
          found = 1;
          break;
        }
      if (!found) {
        int gf;
        attacks_seen[attacks_found] = attack;
        for (gf = 0; gf < NGFILES; gf++) {
          const unsigned max_attacks = n_attacks[gfiles[gf].length][attacker];
          if (attacks_found < max_attacks) {
            BITBOARD b = 0;
            int i, p;
            for (p = attacker-1, i = 0; i < (int) lower; i++, p--)
              Set(gfiles[gf].map[p], b);
            for (p=attacker+1,i=0;(i<(int) upper) && (p<(int) gfiles[gf].length);
                 i++, p++)
              Set(gfiles[gf].map[p], b);
            if (gfiles[gf].mobility)
            gfiles[gf].mobility[attacks_found] =
              lower + Min((int) upper, (int) gfiles[gf].length - attacker - 1);
            gfiles[gf].bitboard[attacks_found] = b;
          }
        }
        attacks_found++;
      }
      at.which_attack[attacker][brev[g]] = a;
    }
    for (gf = 0; gf < NGFILES; gf++) {
      const unsigned len = gfiles[gf].length;
      const unsigned this_inc =
        (gfiles[gf].inc ? gfiles[gf].inc : n_attacks[len][attacker]);
      if ((unsigned) attacker < len) {
        if (gfiles[gf].mobility) gfiles[gf].mobility += this_inc;
        gfiles[gf].bitboard += this_inc;
      }
    }
  }
  {
    int i, a;
    for (i = 0; i < 8; i++)
      for (a = 0; a < MAX_ATTACKS_FROM_SQUARE; a++)
        at.rank_attack_bitboards[i][a]=
          temp_rank_attack_bitboards[i][a] & 0xff;
  }
}
#endif

#if defined(SMP)

/*
********************************************************************************
*                                                                              *
*   CopyFromSMP() is used to copy data from a child thread to a parent thread. *
*   this only copies the appropriate parts of the TREE structure to avoid      *
*   burning memory bandwidth by copying everything.                            *
*                                                                              *
********************************************************************************
*/
void CopyFromSMP(TREE *p, TREE *c) {
  int i;
  if (c->nodes_searched && !c->stop && c->search_value > p->search_value) {
    p->pv[p->ply]=c->pv[p->ply];
    p->search_value=c->search_value;
    for (i=1;i<MAXPLY;i++) p->killers[i]=c->killers[i];
    for (i=0;i<4096;i++) {
      p->history_w[i]=c->history_w[i];
      p->history_b[i]=c->history_b[i];
    }
  }
  p->nodes_searched+=c->nodes_searched;
  p->fail_high+=c->fail_high;
  p->fail_high_first+=c->fail_high_first;
  p->evaluations+=c->evaluations;
  p->transposition_probes+=c->transposition_probes;
  p->transposition_hits+=c->transposition_hits;
  p->transposition_good_hits+=c->transposition_good_hits;
  p->transposition_uppers+=c->transposition_uppers;
  p->transposition_lowers+=c->transposition_lowers;
  p->transposition_exacts+=c->transposition_exacts;
  p->pawn_probes+=c->pawn_probes;
  p->pawn_hits+=c->pawn_hits;
  p->egtb_probes+=c->egtb_probes;
  p->egtb_probes_successful+=c->egtb_probes_successful;
  p->check_extensions_done+=c->check_extensions_done;
  p->recapture_extensions_done+=c->recapture_extensions_done;
  p->passed_pawn_extensions_done+=c->passed_pawn_extensions_done;
  p->one_reply_extensions_done+=c->one_reply_extensions_done;
  p->mate_extensions_done+=c->mate_extensions_done;
  strcpy(c->root_move_text,p->root_move_text);
  c->used=0;
}

/*
********************************************************************************
*                                                                              *
*   CopyToSMP() is used to copy data from a parent thread to a particular      *
*   child thread.  this only copies the appropriate parts of the TREE          *
*   structure to avoid burning memory bandwidth by copying everything.         *
*                                                                              *
********************************************************************************
*/
TREE* CopyToSMP(TREE *p, int thread) {
  int i, j, max;
  TREE *c;
  int first=thread*(MAX_BLOCKS/CPUS)+1;
  int last=first+16;
  int maxb=16*max_threads+1;
  for (i=first;i<last && local[i]->used;i++);
  if (i >= last) {
    for (i=1;i<maxb && local[i]->used;i++);
    if (i >= last) {
      Print(128, "ERROR.  no SMP block can be allocated\n");
      return(0);
    }
  }
  max=0;
  for (j=1;j<maxb;j++)
    if (local[j]->used) max++;
  max_split_blocks=Max(max_split_blocks,max);
  c=local[i];
  c->used=1;
  c->stop=0;
  for (i=0;i<max_threads;i++) c->siblings[i]=0;
  c->pos=p->pos;
  c->pv[p->ply-1]=p->pv[p->ply-1];
  c->pv[p->ply]=p->pv[p->ply];
  c->next_status[p->ply]=p->next_status[p->ply];
  c->save_hash_key[p->ply]=p->save_hash_key[p->ply];
  c->save_pawn_hash_key[p->ply]=p->save_pawn_hash_key[p->ply];
  c->rep_game=p->rep_game;
  for (i=0;i<256;i++) 
    c->rep_list[i]=p->rep_list[i];
  c->last[p->ply]=c->move_list;
  c->hash_move[p->ply]=p->hash_move[p->ply];
  for (i=1;i<=p->ply+1;i++) {
    c->position[i]=p->position[i];
    c->current_move[i]=p->current_move[i];
    c->in_check[i]=p->in_check[i];
    c->phase[i]=p->phase[i];
  }
  for (i=1;i<MAXPLY;i++) c->killers[i]=p->killers[i];
  for (i=0;i<4096;i++) {
    c->history_w[i]=p->history_w[i];
    c->history_b[i]=p->history_b[i];
  }
  c->nodes_searched=0;
  c->fail_high=0;
  c->fail_high_first=0;
  c->evaluations=0;
  c->transposition_probes=0;
  c->transposition_hits=0;
  c->transposition_good_hits=0;
  c->transposition_uppers=0;
  c->transposition_lowers=0;
  c->transposition_exacts=0;
  c->pawn_probes=0;
  c->pawn_hits=0;
  c->egtb_probes=0;
  c->egtb_probes_successful=0;
  c->check_extensions_done=0;
  c->mate_extensions_done=0;
  c->recapture_extensions_done=0;
  c->passed_pawn_extensions_done=0;
  c->one_reply_extensions_done=0;
  c->alpha=p->alpha;
  c->beta=p->beta;
  c->value=p->value;
  c->wtm=p->wtm;
  c->ply=p->ply;
  c->depth=p->depth;
  c->mate_threat=p->mate_threat;
  c->search_value=0;
  c->next_time_check=Min(nodes_between_time_checks/Max(max_threads,1),p->next_time_check);
  strcpy(c->root_move_text,p->root_move_text);
  strcpy(c->remaining_moves_text,p->remaining_moves_text);
  return(c);
}

#endif

/*
********************************************************************************
*                                                                              *
*   Kibitz() is used to whisper/kibitz information to a chess server.  it has  *
*   to handle the xboard whisper/kibitz interface as well as the custom ics    *
*   interface for Crafty.  there are two main issues:  (a) presenting only the *
*   information specified by the current value of whisper or kibitz variables; *
*   (a) if using the custom ICS interface, preceeding the commands with a "*"  *
*   so the interface will direct them to the server rather than the operator.  *
*                                                                              *
********************************************************************************
*/
void Kibitz(int level, int wtm, int depth, int time, int value,
             BITBOARD nodes, int cpu, int tb_hits, char *pv) {

  int nps;
  char snps[32];
  nps=(int) ((time)?100*nodes/(BITBOARD)time:nodes);
  if (nps < 1000000)
    sprintf(snps,"%dK",nps/1000);
  else
    sprintf(snps,"%.2fM",(float) nps/1000000.0);
  if (!puzzling) {
    char prefix[128];

    if (strlen(channel_title) && channel)
      sprintf(prefix,"tell %d (%s) ",channel, channel_title);
    else if (channel) sprintf(prefix,"tell %d",channel);
    else if (!(kibitz&16)) sprintf(prefix,"kibitz");
    else sprintf(prefix,"whisper");
    switch (level) {
    case 1:
      if ((kibitz&15) >= 2) {
        if (value > 0) {
          if (ics) printf("*");
          printf("%s mate in %d moves.\n\n",prefix,value);
        }
        if (value < 0) {
          if (ics) printf("*");
          printf("%s mated in %d moves.\n\n",prefix,-value);
        }
      }
      break;
    case 2:
      if ((kibitz&15) >= 2) {
        if (ics) printf("*");
        printf("%s ply=%d; eval=%s; nps=%s; time=%s; cpu=%d%%; egtb=%d\n",
               prefix,depth,DisplayEvaluationKibitz(value,wtm),snps,
               DisplayTimeKibitz(time),cpu,tb_hits);
      }
    case 3:
      if ((kibitz&15)>=3 && (nodes>5000 || level==2)) {
        if (ics) printf("*");
        printf("%s %s\n",prefix,pv);
      }
      break;
    case 4:
      if ((kibitz&15) >= 4) {
        if (ics) printf("*");
        printf("%s %s\n",prefix,pv);
      }
      break;
    case 5:
      if ((kibitz&15)>=5 && nodes>5000) {
        if (ics) printf("*");
        printf("%s d%d-> %s/s %s %s %s ",prefix,depth,snps,
               DisplayTimeKibitz(time),
               DisplayEvaluationKibitz(value,wtm),pv);
        if (tb_hits) printf("egtb=%d",tb_hits);
        printf("\n");
      }
      break;
    case 6:
      if ((kibitz&15) >= 6 && nodes>5000) {
        if (ics) printf("*");
        if (cpu == 0)
          printf("%s d%d+ %s/s %s %s %s\n",prefix,depth,snps,
                 DisplayTimeKibitz(time),
                 DisplayEvaluationKibitz(value,wtm),pv);
        else
          printf("%s d%d+ %s/s %s >(%s) %s <re-searching>\n",prefix,depth,snps,
                 DisplayTimeKibitz(time),DisplayEvaluationKibitz(value,wtm),pv);
      }
      break;
    }
    value=(wtm)?value:-value;
    if (post && level>1) {
      if (strstr(pv,"book"))
        printf("	%2d  %5d %7d " BMF6 " %s\n",depth,value,time,nodes,pv+10);
      else
        printf("	%2d  %5d %7d " BMF6 " %s\n",depth,value,time,nodes,pv);
    }
    fflush(stdout);
  }
}

/* last modified 07/07/98 */
/*
********************************************************************************
*                                                                              *
*   LegalMove() tests a move to confirm it is absolutely legal.  it should not *
*   be used inside the search, but can be used to check a 21-bit (compressed)  *
*   move to be sure it is safe to make it on the permanent game board.         *
*                                                                              *
********************************************************************************
*/
int LegalMove(TREE * RESTRICT tree, int ply, int wtm, int move) {
  int moves[220], *mv, *mvp;
/*
   generate moves, then eliminate any that are illegal.               
*/
  if (move == 0) return(0);
  tree->position[MAXPLY]=tree->position[ply];
  mvp=GenerateCaptures(tree,MAXPLY, wtm, moves);
  mvp=GenerateNonCaptures(tree,MAXPLY, wtm, mvp);
  for (mv=&moves[0];mv<mvp;mv++) {
    MakeMove(tree,MAXPLY, *mv, wtm);
    if (!Check(wtm) && move==*mv) {
      UnmakeMove(tree,MAXPLY, *mv, wtm);
      return(1);
    }
    UnmakeMove(tree,MAXPLY, *mv, wtm);
  }
  return(0);
}

/* last modified 07/07/98 */
/*
********************************************************************************
*                                                                              *
*   StrCnt() counts the number of times a character occurs in a string.        *
*                                                                              *
********************************************************************************
*/
int StrCnt(char *string, char testchar) {
  int count=0, i;
  for (i=0;i<strlen(string);i++)  if (string[i] == testchar) count++;
  return(count);
}


/*
********************************************************************************
*                                                                              *
*   Windows NUMA support                                                       *
*                                                                              *
********************************************************************************
*/

#if (defined(_WIN32) || defined(_WIN64)) && defined(SMP)

lock_t ThreadsLock;

static BOOL  (WINAPI *pGetNumaHighestNodeNumber) (PULONG);
static BOOL  (WINAPI *pGetNumaNodeProcessorMask) (UCHAR, PULONGLONG);
static DWORD (WINAPI *pSetThreadIdealProcessor)  (HANDLE, DWORD);

static volatile BOOL fThreadsInitialized = FALSE;
static BOOL fSystemIsNUMA = FALSE;
static ULONGLONG ullProcessorMask[256];
static ULONG ulNumaNodes;
static ULONG ulNumaNode = 0;

// Get NUMA-related information from Windows

static void WinNumaInit (void) {
  DWORD_PTR dwMask;
  HMODULE hModule;
  ULONG ulCPU, ulNode;
  ULONGLONG ullMask;
  DWORD dwCPU;

  if (!fThreadsInitialized) {
    Lock(ThreadsLock);
    if (!fThreadsInitialized) {
      printf ("\nInitializing multiple threads.\n");
      fThreadsInitialized = TRUE;
      hModule = GetModuleHandle("kernel32");
      pGetNumaHighestNodeNumber = (void*) GetProcAddress(hModule, "GetNumaHighestNodeNumber");
      pGetNumaNodeProcessorMask = (void*) GetProcAddress(hModule, "GetNumaNodeProcessorMask");
      pSetThreadIdealProcessor  = (void*) GetProcAddress(hModule, "SetThreadIdealProcessor");
      if (pGetNumaHighestNodeNumber && pGetNumaNodeProcessorMask &&
          pGetNumaHighestNodeNumber(&ulNumaNodes) && (ulNumaNodes > 0)) {
        fSystemIsNUMA = TRUE;
        if (ulNumaNodes > 255) ulNumaNodes = 255;
        printf ("System is NUMA. %d nodes reported by Windows\n", ulNumaNodes+1);
        for (ulNode = 0; ulNode <= ulNumaNodes; ulNode ++) {
          pGetNumaNodeProcessorMask((UCHAR) ulNode, &ullProcessorMask[ulNode]);
          printf ("Node %d CPUs: ", ulNode);
          ullMask = ullProcessorMask[ulNode];
          if (0 == ullMask) fSystemIsNUMA = FALSE;
          else {
            ulCPU = 0;
            do {
              if (ullMask & 1) printf ("%d ", ulCPU);
              ulCPU ++;
              ullMask >>= 1;
            } while (ullMask);
          }
          printf ("\n");
        }
        // Thread 0 was already started on some CPU. To simplify things further,
        // exchange ullProcessorMask[0] and ullProcessorMask[node for that CPU],
        // so ullProcessorMask[0] would always be node for thread 0
        dwCPU = pSetThreadIdealProcessor(GetCurrentThread(), MAXIMUM_PROCESSORS);
        printf ("Current ideal CPU is %u\n", dwCPU);
        pSetThreadIdealProcessor(GetCurrentThread(), dwCPU);
        if ((((DWORD) -1) != dwCPU) &&
            (MAXIMUM_PROCESSORS != dwCPU) &&
            !(ullProcessorMask[0] & (1ui64 << dwCPU))) {
          for (ulNode = 1; ulNode <= ulNumaNodes; ulNode ++) {
            if (ullProcessorMask[ulNode] & (1ui64 << dwCPU)) {
              printf ("Exchanging nodes 0 and %d\n", ulNode);
              ullMask = ullProcessorMask[ulNode];
              ullProcessorMask[ulNode] = ullProcessorMask[0];
              ullProcessorMask[0] = ullMask;
              break;
            }
          }
        }
      }
      else printf ("System is SMP, not NUMA.\n");
    }
    Unlock(ThreadsLock);
  }
}

// Start thread. For NUMA system set it affinity.

pthread_t NumaStartThread(void * func, void * args) {
  HANDLE hThread;
  ULONGLONG ullMask;

  WinNumaInit();
  if (fSystemIsNUMA) {
    ulNumaNode ++;
    if (ulNumaNode > ulNumaNodes) ulNumaNode = 0;
    ullMask = ullProcessorMask[ulNumaNode];
    printf ("Starting thread on node %d CPU mask %I64d\n", ulNumaNode, ullMask);
    SetThreadAffinityMask(GetCurrentThread(), (DWORD_PTR) ullMask);
    hThread = (HANDLE) _beginthreadex(0,0,func,args,CREATE_SUSPENDED,0);
    SetThreadAffinityMask(hThread, (DWORD_PTR) ullMask);
    ResumeThread(hThread);
    SetThreadAffinityMask(GetCurrentThread(), ullProcessorMask[0]);
  }
  else hThread = (HANDLE) _beginthreadex(0,0,func,args,0,0);
  return hThread;
}

// Allocate memory for thread #N

void * WinMalloc(size_t cbBytes, int iThread) {
  HANDLE hThread;
  DWORD_PTR dwAffinityMask;
  void *pBytes;
  ULONG ulNode;

  WinNumaInit();
  if (fSystemIsNUMA) {
    ulNode = iThread % (ulNumaNodes+1);
    hThread = GetCurrentThread();
    dwAffinityMask = SetThreadAffinityMask(hThread, ullProcessorMask[ulNode]);
    pBytes = VirtualAlloc(NULL, cbBytes, MEM_COMMIT, PAGE_READWRITE);
    memset (pBytes, 0, cbBytes);
    SetThreadAffinityMask(hThread, dwAffinityMask);
    return pBytes;
  }
  else return malloc(cbBytes);
}

// Allocate interleaved memory

void * WinMallocInterleaved(size_t cbBytes, int cThreads) {
  char *pBase;
  char *pEnd;
  char * pch;
  HANDLE hThread;
  DWORD_PTR dwAffinityMask;
  ULONG ulNode;
  SYSTEM_INFO sSysInfo;
  size_t dwStep;
  int iThread;
  DWORD dwPageSize;   // the page size on this computer
  LPVOID lpvResult;

  WinNumaInit();
  if (fSystemIsNUMA && (cThreads > 1)) {
    GetSystemInfo(&sSysInfo);     // populate the system information structure
    dwPageSize = sSysInfo.dwPageSize;

    // Reserve pages in the process's virtual address space.
    pBase = (char*) VirtualAlloc(NULL, cbBytes, MEM_RESERVE, PAGE_NOACCESS);
    if (pBase == NULL) {
      printf ("VirtualAlloc() reserve failed\n");
      exit(0);
    }

    // Now walk through memory, committing each page
    hThread = GetCurrentThread();
    dwStep = dwPageSize * cThreads;
    pEnd = pBase + cbBytes;
    for (iThread = 0; iThread < cThreads; iThread++) {
      ulNode = iThread % (ulNumaNodes+1);
      dwAffinityMask = SetThreadAffinityMask(hThread, ullProcessorMask[ulNode]);
      for (pch = pBase + iThread * dwPageSize;
           pch < pEnd;
           pch += dwStep) {
        lpvResult = VirtualAlloc(pch,             // next page to commit
                                 dwPageSize,      // page size, in bytes
                                 MEM_COMMIT,      // allocate a committed page
                                 PAGE_READWRITE); // read/write access
        if (lpvResult == NULL)
          ExitProcess(GetLastError());
        memset (lpvResult, 0, dwPageSize);
      }
      SetThreadAffinityMask(hThread, dwAffinityMask);
    }
  }
  else {
    pBase = VirtualAlloc(NULL, cbBytes, MEM_COMMIT, PAGE_READWRITE);
    if (pBase == NULL)
      ExitProcess(GetLastError());
    memset (pBase, 0, cbBytes);
  }
  return (void *) pBase;
}

// Free interleaved memory

void WinFreeInterleaved(void *pMemory, size_t cBytes) {
  VirtualFree(pMemory, 0, MEM_RELEASE);
}

#endif

/*
********************************************************************************
*                                                                              *
*   Linux NUMA support                                                         *
*                                                                              *
********************************************************************************
*/

#if defined(LINUX) && defined(NUMA)

/*
********************************************************************************
*                                                                              *
*   First, discover if we are on a NUMA box.  If not, the normal SMP stuff is  *
*   primed and ready to go.  If we are on a NUMA machine, we need to know (a)  *
*   how many processors (nodes in the case of AMD/Intel) we have on the        *
*   machine and (b) how many processors (threads) the user intends to run.     *
*   It becomes important for the "smpmt=n" command to be either on the         *
*   command-line or in the crafty.rc/.craftyrc files, otherwise we might not   *
*   get things initialized optimally in a NUMA environment.                    *
*                                                                              *
********************************************************************************
*/

void NumaInit (void) {
  int numa_machine, maxNumaNodes;

  if (!fThreadsInitialized) {
    Lock(ThreadsLock);
    if (!fThreadsInitialized) {
      printf ("\nInitializing multiple threads.\n");
      fThreadsInitialized = TRUE;
      numa_machine=numa_available();
      if (numa_machine >= 0) {
        maxNumaNodes=numa_max_node();
        printf ("System is NUMA. %d nodes reported by Linux\n", maxNumaNodes+1);
      }
      else {
        Print(4095,"system is not NUMA, skipping NUMA initialization\n");
	return;
      }

      pGetNumaHighestNodeNumber = (void*) GetProcAddress(hModule, "GetNumaHighestNodeNumber");
      pGetNumaNodeProcessorMask = (void*) GetProcAddress(hModule, "GetNumaNodeProcessorMask");
      pSetThreadIdealProcessor  = (void*) GetProcAddress(hModule, "SetThreadIdealProcessor");
      if (pGetNumaHighestNodeNumber && pGetNumaNodeProcessorMask &&
          pGetNumaHighestNodeNumber(&ulNumaNodes) && (ulNumaNodes > 0)) {
        fSystemIsNUMA = TRUE;
        if (ulNumaNodes > 255) ulNumaNodes = 255;
        printf ("System is NUMA. %d nodes reported by Windows\n", ulNumaNodes+1);
        for (ulNode = 0; ulNode <= ulNumaNodes; ulNode ++) {
          pGetNumaNodeProcessorMask((UCHAR) ulNode, &ullProcessorMask[ulNode]);
          printf ("Node %d CPUs: ", ulNode);
          ullMask = ullProcessorMask[ulNode];
          if (0 == ullMask) fSystemIsNUMA = FALSE;
          else {
            ulCPU = 0;
            do {
              if (ullMask & 1) printf ("%d ", ulCPU);
              ulCPU ++;
              ullMask >>= 1;
            } while (ullMask);
          }
          printf ("\n");
        }
        // Thread 0 was already started on some CPU. To simplify things further,
        // exchange ullProcessorMask[0] and ullProcessorMask[node for that CPU],
        // so ullProcessorMask[0] would always be node for thread 0
        dwCPU = pSetThreadIdealProcessor(GetCurrentThread(), MAXIMUM_PROCESSORS);
        printf ("Current ideal CPU is %u\n", dwCPU);
        pSetThreadIdealProcessor(GetCurrentThread(), dwCPU);
        if ((((DWORD) -1) != dwCPU) &&
            (MAXIMUM_PROCESSORS != dwCPU) &&
            !(ullProcessorMask[0] & (1ui64 << dwCPU))) {
          for (ulNode = 1; ulNode <= ulNumaNodes; ulNode ++) {
            if (ullProcessorMask[ulNode] & (1ui64 << dwCPU)) {
              printf ("Exchanging nodes 0 and %d\n", ulNode);
              ullMask = ullProcessorMask[ulNode];
              ullProcessorMask[ulNode] = ullProcessorMask[0];
              ullProcessorMask[0] = ullMask;
              break;
            }
          }
        }
      }
      else printf ("System is SMP, not NUMA.\n");
    }
    Unlock(ThreadsLock);
  }
}

// Start thread. For NUMA system set it affinity.

pthread_t NumaStartThread(void * func, void * args) {
  HANDLE hThread;
  ULONGLONG ullMask;

  WinNumaInit();
  if (fSystemIsNUMA) {
    ulNumaNode ++;
    if (ulNumaNode > ulNumaNodes) ulNumaNode = 0;
    ullMask = ullProcessorMask[ulNumaNode];
    printf ("Starting thread on node %d CPU mask %I64d\n", ulNumaNode, ullMask);
    SetThreadAffinityMask(GetCurrentThread(), (DWORD_PTR) ullMask);
    hThread = (HANDLE) _beginthreadex(0,0,func,args,CREATE_SUSPENDED,0);
    SetThreadAffinityMask(hThread, (DWORD_PTR) ullMask);
    ResumeThread(hThread);
    SetThreadAffinityMask(GetCurrentThread(), ullProcessorMask[0]);
  }
  else hThread = (HANDLE) _beginthreadex(0,0,func,args,0,0);
  return hThread;
}

// Allocate memory for thread #N

void * WinMalloc(size_t cbBytes, int iThread) {
  HANDLE hThread;
  DWORD_PTR dwAffinityMask;
  void *pBytes;
  ULONG ulNode;

  WinNumaInit();
  if (fSystemIsNUMA) {
    ulNode = iThread % (ulNumaNodes+1);
    hThread = GetCurrentThread();
    dwAffinityMask = SetThreadAffinityMask(hThread, ullProcessorMask[ulNode]);
    pBytes = VirtualAlloc(NULL, cbBytes, MEM_COMMIT, PAGE_READWRITE);
    memset (pBytes, 0, cbBytes);
    SetThreadAffinityMask(hThread, dwAffinityMask);
    return pBytes;
  }
  else return malloc(cbBytes);
}

// Allocate interleaved memory

void * WinMallocInterleaved(size_t cbBytes, int cThreads) {
  char *pBase;
  char *pEnd;
  char * pch;
  HANDLE hThread;
  DWORD_PTR dwAffinityMask;
  ULONG ulNode;
  SYSTEM_INFO sSysInfo;
  size_t dwStep;
  int iThread;
  DWORD dwPageSize;   // the page size on this computer
  LPVOID lpvResult;

  WinNumaInit();
  if (fSystemIsNUMA && (cThreads > 1)) {
    GetSystemInfo(&sSysInfo);     // populate the system information structure
    dwPageSize = sSysInfo.dwPageSize;

    // Reserve pages in the process's virtual address space.
    pBase = (char*) VirtualAlloc(NULL, cbBytes, MEM_RESERVE, PAGE_NOACCESS);
    if (pBase == NULL) {
      printf ("VirtualAlloc() reserve failed\n");
      exit(0);
    }

    // Now walk through memory, committing each page
    hThread = GetCurrentThread();
    dwStep = dwPageSize * cThreads;
    pEnd = pBase + cbBytes;
    for (iThread = 0; iThread < cThreads; iThread++) {
      ulNode = iThread % (ulNumaNodes+1);
      dwAffinityMask = SetThreadAffinityMask(hThread, ullProcessorMask[ulNode]);
      for (pch = pBase + iThread * dwPageSize;
           pch < pEnd;
           pch += dwStep) {
        lpvResult = VirtualAlloc(pch,             // next page to commit
                                 dwPageSize,      // page size, in bytes
                                 MEM_COMMIT,      // allocate a committed page
                                 PAGE_READWRITE); // read/write access
        if (lpvResult == NULL)
          ExitProcess(GetLastError());
        memset (lpvResult, 0, dwPageSize);
      }
      SetThreadAffinityMask(hThread, dwAffinityMask);
    }
  }
  else {
    pBase = VirtualAlloc(NULL, cbBytes, MEM_COMMIT, PAGE_READWRITE);
    if (pBase == NULL)
      ExitProcess(GetLastError());
    memset (pBase, 0, cbBytes);
  }
  return (void *) pBase;
}

// Free interleaved memory

void WinFreeInterleaved(void *pMemory, size_t cBytes) {
  VirtualFree(pMemory,                      // base address of block
              cBytes,                       // bytes of committed pages
              MEM_DECOMMIT|MEM_RELEASE);    // decommit the pages
}

#endif
