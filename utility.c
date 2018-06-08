#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <limits.h>
#include <sys/times.h>
#include <time.h>
#include "types.h"
#include "function.h"
#include "data.h"
#if defined(UNIX)
  #include <unistd.h>
  #include <sys/types.h>
  #if !defined(LINUX) && !defined(ALPHA)
    #include <sys/filio.h>
    #include <stropts.h>
    #include <sys/conf.h>
  #else
    #include <sys/ioctl.h>
  #endif
#else
  #include <bios.h>
#endif
#if defined(BSD)&&(!defined(ALPHA))
  #define CLK_TCK _SC_CLK_TCK
#endif

int Check_Input(void)
{
  int i;
#if defined(UNIX)
/*i=ioctl(0,I_NREAD,&arg);*/
  i=0;
  (void) ioctl((int)0,FIONREAD,&i);
#else
  i=bioskey(1);
#endif
  return(i);
}

void Display_Bit_Board(BITBOARD board)
{
  union doub {
    char i[8];
    BITBOARD d;
  };
  union doub x;
  int i,j;
#if defined(LITTLE_ENDIAN) && defined(HAS_LONGLONG)
  int subs[8]={7,6,5,4,3,2,1,0};
#endif
#if defined(LITTLE_ENDIAN) && !defined(HAS_LONGLONG)
  int subs[8]={3,2,1,0,7,6,5,4};
#endif

  x.d=board;
#if defined(LITTLE_ENDIAN)
  for(i=7;i>=0;i--) {
    printf("  %2d ",i*8);
    for(j=128;j>0;j=j>>1)
      if(x.i[subs[i]] & j) 
        printf("X ");
      else
        printf("- ");
    printf("\n");
  }
#else
  for(i=7;i>=0;i--) {
    printf("  %2d ",i*8);
    for(j=128;j>0;j=j>>1)
      if(x.i[i] & j) 
        printf("X ");
      else
        printf("- ");
    printf("\n");
  }
#endif
}

/*
********************************************************************************
*                                                                              *
*   display() is used to display the chess board.  since the board is kept in  *
*   both the bit-board and array formats, here we use the array format which   *
*   is nearly ready for display as is.                                         *
*                                                                              *
********************************************************************************
*/
void Display_Chess_Board(FILE *display_file, CHESS_BOARD board)
{
  int display_board[64];
  char display_string[] =
    {"*K\0*Q\0*R\0*B\0*N\0*P\0  \0P \0N \0B \0R \0Q \0K \0"};
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
    display_board[i]=(board.board[i]+6)*3;
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
      fprintf(display_file,"| %s",&display_string[display_board[i*8+j]]);
    fprintf(display_file,"|\n");
    fprintf(display_file,"       +---+---+---+---+---+---+---+---+\n");
  }
  fprintf(display_file,"         a   b   c   d   e   f   g   h\n\n");
}

char* Display_Evaluation(int value)
{
  static char out[10];

  if (abs(value) < MATE-100) {
    sprintf(out,"%8.3f",((float) value)/1000.0);
  }
  else if (abs(value) > MATE) {
    if (value < 0)
      sprintf(out," -infnty");
    else
      sprintf(out," +infnty");
  }
  else if (value == MATE-2)
    sprintf(out,"    Mate");
  else if (value == -(MATE-1))
    sprintf(out,"   -Mate");
  else if (value > 0) 
    sprintf(out,"   Mat%.2d",(MATE-value)/2);
  else 
    sprintf(out,"  -Mat%.2d",(MATE-abs(value))/2);
  return(out);
}

void Display_Piece_Boards(int *white, int *black)
{
  int i,j;
  printf("                 white                      ");
  printf("                 black\n");
  for (i=7;i>=0;i--) {
    for (j=i*8;j<i*8+8;j++)
      printf("%4d ",white[j]);
    printf("    ");
    for (j=i*8;j<i*8+8;j++)
      printf("%4d ",black[j]);
    printf("\n");
  }
}

char* Display_Time(unsigned int time)
{
  static char out[10];

  if (time < 600) 
    sprintf(out,"%5.1fs",(float) time/10.0);
  else {
    time=time/10;
    sprintf(out,"%3u:%02u", time/60, time%60);
  }
  return(out);
}

void Display_64bit_Word(BITBOARD word)
{
  union doub {
    unsigned int i[2];
    BITBOARD d;
  };
  union doub x;
  x.d=word;
#if defined(HAS_LONGLONG)
#if defined(LITTLE_ENDIAN)
  printf("%x%x\n",x.i[1],x.i[0]);
#else
  printf("%lx\n",word);
#endif
#endif
#if !defined(HAS_LONGLONG) && !defined(LITTLE_ENDIAN)
  printf("%x%x\n",x.i[0],x.i[1]);
#endif
}

void Display_2_Bit_Boards(BITBOARD board1, BITBOARD board2)
{
  union doub {
    char i[8];
    BITBOARD d;
  };
  union doub x,y;
  int i,j;
#if defined(LITTLE_ENDIAN) && defined(HAS_LONGLONG)
  int subs[8]={7,6,5,4,3,2,1,0};
#endif
#if defined(LITTLE_ENDIAN) && !defined(HAS_LONGLONG)
  int subs[8]={3,2,1,0,7,6,5,4};
#endif

  x.d=board1;
  y.d=board2;
  printf("          good                     bad\n");
#if defined(LITTLE_ENDIAN)
  for(i=7;i>=0;i--) {
    printf("  %2d ",i*8);
    for(j=128;j>0;j=j>>1)
      if(x.i[subs[i]] & j) 
        printf("X ");
      else
        printf("- ");
    printf("     %2d ",i*8);
    for(j=128;j>0;j=j>>1)
      if(y.i[subs[i]] & j) 
        printf("X ");
      else
        printf("- ");
    printf("\n");
  }
#else
  for(i=7;i>=0;i--) {
    printf("  %2d ",i*8);
    for(j=128;j>0;j=j>>1)
      if(x.i[i] & j) 
        printf("X ");
      else
        printf("- ");
    printf("     %2d ",i*8);
    for(j=128;j>0;j=j>>1)
      if(y.i[i] & j) 
        printf("X ");
      else
        printf("- ");
    printf("\n");
  }
#endif
}

void Display_Chess_Move(char *title, int move)
{
  printf("%s  piece=%d, from=%d, to=%d, captured=%d, promote=%d\n",
         title,Piece(move),From(move), To(move),Captured(move),
         Promote(move));
}

unsigned int Get_Time(TIME_TYPE type)
{
#if defined(UNIX)
	struct tms t;
#endif
  switch (type) {
    case cpu:
#if defined(UNIX)
      (void) times(&t);
	    return((t.tms_utime+t.tms_stime)*10/CLK_TCK);
#else
      return((clock()*10)/CLOCKS_PER_SEC);
#endif
    case elapsed:
      return(time(0)*10);
    default:
      return(0);
  }
}
 
/*
********************************************************************************
*                                                                              *
*   Has_Opposition() is used to determine if one king stands in "opposition"   *
*   to the other.  if the kings are opposed on the same file or else are       *
*   opposed on the same diagonal, then the side not-to-move has the opposition *
*   and the side-to-move must give way.                                        *
*                                                                              *
********************************************************************************
*/
int Has_Opposition(int on_move, int white_king, int black_king)
{
  int file_distance, rank_distance;
  file_distance=File_Distance(white_king,black_king);
  rank_distance=Rank_Distance(white_king,black_king);
  if (rank_distance < 2) return(1);
  if (on_move) {
    if (rank_distance > 2)
      rank_distance--;
    else
      file_distance--;
  }
  if ((file_distance == 2) && (rank_distance == 2)) return(1);
  if ((file_distance == 0) && (rank_distance == 2)) return(1);
  return(0);
}
 
int King_Pawn_Square(int pawn, int king, int queen, int ptm)
{
  int pdist, kdist;
  pdist=abs((pawn>>3)-(queen>>3));
  kdist=(abs((king>>3)-(queen>>3)) > abs((king&7)-(queen&7))) ? 
    abs((king>>3)-(queen>>3)) : abs((king&7)-(queen&7));
  if (!ptm) pdist++;
  if (pdist < kdist)
    return(0);
  else
    return(1);
}

char* Normal(void)
{
  if (ansi)
#if defined(UNIX)
    return("\x1B[0m");
#else
    return("\x1B[1;44;37m");
#endif
  else
    return("");
}

void Print(int vb, char *fmt, ...)
{
  va_list ap;
  va_start(ap,fmt);
  if (vb <= verbosity_level)
    vprintf(fmt, ap);
  if (log_file)
    vfprintf(log_file, fmt, ap);
  va_end(ap);
  fflush(stdout);
  if (log_file) fflush(log_file);
}

/*

A 32 bit random number generator. An implementation in C of the algorithm given by
Knuth, the art of computer programming, vol. 2, pp. 26-27. We use e=32, so 
we have to evaluate y(n) = y(n - 24) + y(n - 55) mod 2^32, which is implicitly
done by unsigned arithmetic.

*/

unsigned int Random32(void)
{
	/*
	random numbers from Mathematica 2.0.
	SeedRandom = 1;
	Table[Random[Integer, {0, 2^32 - 1}]
	*/
	static unsigned long x[55] = {
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
		for (i = 0; i < 55; i++)
			y[i] = x[i];
		j = 24 - 1;
		k = 55 - 1;
	}
	
	ul = (y[k] += y[j]);
	if (--j < 0) j = 55 - 1;
	if (--k < 0) k = 55 - 1;
	return((unsigned int)ul);
}

BITBOARD Random64(void)
{
  BITBOARD result;
  unsigned int r1, r2;

  r1=Random32();
  r2=Random32();
  result=Or(r1,Shiftl((BITBOARD) r2,32));
  return (result);
}

char* Reverse(void)
{
  if (ansi)
#if defined(UNIX)
    return("\x1B[7m");
#else
    return("\x1B[7;47;33m");
#endif
  else
    return("");
}


int TtoI(char *text)
{
  int t;
  char *n;

  t=0;
  if (!strchr(text,':'))
    t=atoi(text);
  else {
    n=text-1;
    do {
      n++;
      t=t*60+atoi(n);
      n=strchr(n,':');
    } while (n);
  }
  return(t);
}
