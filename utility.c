#if defined(NT_i386) || defined(NT_AXP)
#  include <windows.h>
#  include <conio.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#if !defined(AMIGA)
#  include <limits.h>
#endif
#if !defined(NT_i386) && !defined(NT_AXP)
#  include <sys/times.h>
#  include <sys/time.h>
#endif
#include "types.h"
#include "function.h"
#include "data.h"
#if defined(UNIX)
#  include <unistd.h>
#  include <sys/types.h>
#  if !defined(LINUX) && !defined(ALPHA) && !defined(HP) && !defined(AIX) && !defined(CRAY1) && !defined(FreeBSD)
#    if defined(AIX)
#      include <sys/termio.h>
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
   struct tms t;
#  if !defined(CLK_TCK)
     static clock_t clk_tck = 0;
#  endif
#endif

#if defined(AMIGA)
#  include <proto/dos.h>
#  define tv_sec tv_secs
#  define tv_usec tv_micro
#  include <exec/types.h>
#  define RAW 1
#  define CON 0
#  include <limits.h>
#endif

#if defined(AMIGA)

typedef int BOOLEAN;

/* Returns FALSE right away if input stream is not a terminal
** TRUE if user pressed a key
** FALSE if user didn't press a key
**
** Note: Lightly tested
*/

BOOLEAN _kbhit(void)
{
  BPTR  inp;
  BOOLEAN  ret;

  inp=Input();
  if(!IsInteractive(inp))   /* Not a terminal? */
    return FALSE;       /*  Ok, then noone can press any keys...    */
  Flush(inp);
  (void) SetMode(inp,RAW);
  ret=WaitForChar(inp,1);
/*  ret=WaitForChar(inp,100000);*/  /* You may want to change this 100000. Its */
  (void) SetMode(inp,CON);          /* time the function will wait.  The time   */
  return ret;                       /* unit is microseconds. 1000000 is 1 sec   */
}                                   /* Current 100000 is 0.1 sec            */

/* Note that the character is LEFT in the input stream, which has the effect
** that subsequent calls will return TRUE right away
** c=FGetC(inp); to get the char code in c (c should be long)
** 
** Note also that A(miga), SHIFT, CTRL,ALT keys wont make the function
** return TRUE.
*/

#endif   /* if defined(AMIGA)  */

int CheckInput(void)
{
  int i;
#if defined(NT_i386) || defined(NT_AXP)
   static int init = 0, pipe;
   static HANDLE inh;
   DWORD dw;
#endif
#if defined(UNIX)
/*i=ioctl(0,I_NREAD,&arg);*/
  i=0;
  (void) ioctl((int)0,FIONREAD,&i);
  fflush(stdout);
#else
# if defined(NT_i386) || defined(NT_AXP)
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
# else
#  if defined(AMIGA)
    i=_kbhit();
#  else
    i=bioskey(1);
#  endif
# endif
#endif
  return(i);
}

void DisplayBitBoard(BITBOARD board)
{
  union doub {
    char i[8];
    BITBOARD d;
  };
  union doub x;
  int i,j;
#if defined(LITTLE_ENDIAN_ARCH) && defined(HAS_LONGLONG)
  int subs[8]={7,6,5,4,3,2,1,0};
#endif
#if defined(LITTLE_ENDIAN_ARCH) && !defined(HAS_LONGLONG)
  int subs[8]={3,2,1,0,7,6,5,4};
#endif

  x.d=board;
#if defined(LITTLE_ENDIAN_ARCH)
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
*   DisplayChessBoard() is used to display the board since it is kept in       *
*   both the bit-board and array formats, here we use the array format which   *
*   is nearly ready for display as is.                                         *
*                                                                              *
********************************************************************************
*/
void DisplayChessBoard(FILE *display_file, CHESS_POSITION pos)
{
  int display_board[64];
  char display_string[] =
    {"*Q\0*R\0*B\0  \0*K\0*N\0*P\0  \0P \0N \0K \0  \0B \0R \0Q \0"};
  int i,j;
/*
 ----------------------------------------------------------
|                                                          |
|   first, convert square values to indices to the proper  |
|   text string.                                           |
|                                                          |
 ----------------------------------------------------------
*/
  for(i=0;i<64;i++) display_board[i]=(pos.board[i]+7)*3;
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

char* DisplayEvaluation(int value)
{
  static char out[10];

  if (abs(value) < MATE-100) sprintf(out,"%8.3f",((float) value)/1000.0);
  else if (abs(value) > MATE) {
    if (value < 0) sprintf(out," -infnty");
    else sprintf(out," +infnty");
  }
  else if (value == MATE-2) sprintf(out,"    Mate");
  else if (value == -(MATE-1)) sprintf(out,"   -Mate");
  else if (value > 0) sprintf(out,"   Mat%.2d",(MATE-value)/2);
  else sprintf(out,"  -Mat%.2d",(MATE-abs(value))/2);
  return(out);
}

char* DisplayEvaluationWhisper(int value)
{
  static char out[10];

  if (abs(value) < MATE-100) sprintf(out,"%+.3f",((float) value)/1000.0);
  else if (abs(value) > MATE) {
    if (value < 0) sprintf(out,"-infnty");
    else sprintf(out,"+infnty");
  }
  else if (value == MATE-2) sprintf(out,"Mate");
  else if (value == -(MATE-1)) sprintf(out,"-Mate");
  else if (value > 0) sprintf(out,"Mat%.2d",(MATE-value)/2);
  else sprintf(out,"-Mat%.2d",(MATE-abs(value))/2);
  return(out);
}

void DisplayPieceBoards(int *white, int *black)
{
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

char* DisplayHHMM(unsigned int time)
{
  static char out[10];

  time=time/6000;
  sprintf(out,"%3u:%02u", time/60, time%60);
  return(out);
}

char* DisplayTime(unsigned int time)
{
  static char out[10];

  if (time < 6000) sprintf(out,"%6.2f",(float) time/100.0);
  else {
    time=time/100;
    sprintf(out,"%3u:%02u", time/60, time%60);
  }
  return(out);
}

char* DisplayTimeWhisper(unsigned int time)
{
  static char out[10];

  if (time < 6000) sprintf(out,"%6.2f",(float) time/100.0);
  else {
    time=time/100;
    sprintf(out,"%u:%02u", time/60, time%60);
  }
  return(out);
}

void Display64bitWord(BITBOARD word)
{
  union doub {
    unsigned int i[2];
    BITBOARD d;
  };
  union doub x;
  x.d=word;
#if defined(HAS_LONGLONG)
#if defined(LITTLE_ENDIAN_ARCH)
  printf("%x%x\n",x.i[1],x.i[0]);
#else
  printf("%llx\n",word);
#endif
#endif
#if !defined(HAS_LONGLONG) && !defined(LITTLE_ENDIAN_ARCH)
  printf("%x%x\n",x.i[0],x.i[1]);
#endif
}

void Display2BitBoards(BITBOARD board1, BITBOARD board2)
{
  union doub {
    char i[8];
    BITBOARD d;
  };
  union doub x,y;
  int i,j;
#if defined(LITTLE_ENDIAN_ARCH) && defined(HAS_LONGLONG)
  int subs[8]={7,6,5,4,3,2,1,0};
#endif
#if defined(LITTLE_ENDIAN_ARCH) && !defined(HAS_LONGLONG)
  int subs[8]={3,2,1,0,7,6,5,4};
#endif

  x.d=board1;
  y.d=board2;
  printf("          good                     bad\n");
#if defined(LITTLE_ENDIAN_ARCH)
  for(i=7;i>=0;i--) {
    printf("  %2d ",i*8);
    for(j=128;j>0;j=j>>1)
      if(x.i[subs[i]] & j) printf("X ");
      else printf("- ");
    printf("     %2d ",i*8);
    for(j=128;j>0;j=j>>1)
      if(y.i[subs[i]] & j) printf("X ");
      else printf("- ");
    printf("\n");
  }
#else
  for(i=7;i>=0;i--) {
    printf("  %2d ",i*8);
    for(j=128;j>0;j=j>>1)
      if(x.i[i] & j) printf("X ");
      else printf("- ");
    printf("     %2d ",i*8);
    for(j=128;j>0;j=j>>1)
      if(y.i[i] & j) printf("X ");
      else printf("- ");
    printf("\n");
  }
#endif
}

void DisplayChessMove(char *title, int move)
{
  Print(0,"%s  piece=%d, from=%d, to=%d, captured=%d, promote=%d\n",
         title,Piece(move),From(move), To(move),Captured(move),
         Promote(move));
}

unsigned int GetTime(TIME_TYPE type)
{
#if defined(UNIX) || defined(AMIGA)
  static struct tms t;
  static struct timeval timeval;
  static struct timezone timezone;
#endif

  switch (type) {
    case cpu:
#if defined(UNIX) || defined(AMIGA)
      (void) times(&t);
#  if defined(CLK_TCK)
      return((t.tms_utime+t.tms_stime)*100/CLK_TCK);
#  else
      if (!clk_tck) clk_tck = sysconf(_SC_CLK_TCK);
      return((t.tms_utime+t.tms_stime)*10/clk_tck);
#  endif
    case elapsed:
      gettimeofday(&timeval, &timezone);
      return(timeval.tv_sec*100+(timeval.tv_usec / 10000L));
    default:
      gettimeofday(&timeval, &timezone);
      return(timeval.tv_usec);
#endif
#if defined(NT_i386) || defined(NT_AXP)
    case elapsed:
      return( (unsigned int) GetTickCount()/10);
    default:
      return( (unsigned int) GetTickCount()/10);
#endif
#if defined(DOS)
    case elapsed:
      return(time(0)*100);
    default:
      return(time(0)*100);
#endif
  }
}

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
int HasOpposition(int on_move, int white_king, int black_king)
{
  register int file_distance, rank_distance;
  file_distance=FileDistance(white_king,black_king);
  rank_distance=RankDistance(white_king,black_king);
  if (rank_distance < 2) return(1);
  if (on_move) {
    if (rank_distance > 2) rank_distance--;
    else file_distance--;
  }
  if ((file_distance == 2) && (rank_distance == 2)) return(1);
  if ((file_distance == 0) && (rank_distance == 2)) return(1);
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
                          int checking_square)
{
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
      target=Xor(mask_plus1dir[king_square-1],mask_plus1dir[checking_square]);
      break;
    case +7:
      target=Xor(mask_plus7dir[king_square-7],mask_plus7dir[checking_square]);
      break;
    case +8:
      target=Xor(mask_plus8dir[king_square-8],mask_plus8dir[checking_square]);
      break;
    case +9:
      target=Xor(mask_plus9dir[king_square-9],mask_plus9dir[checking_square]);
      break;
    case -1:
      target=Xor(mask_minus1dir[king_square+1],mask_minus1dir[checking_square]);
      break;
    case -7:
      target=Xor(mask_minus7dir[king_square+7],mask_minus7dir[checking_square]);
      break;
    case -8:
      target=Xor(mask_minus8dir[king_square+8],mask_minus8dir[checking_square]);
      break;
    case -9:
      target=Xor(mask_minus9dir[king_square+9],mask_minus9dir[checking_square]);
      break;
    default:
      target=0;
      break;
  }
  return(target);
}
 
int KingPawnSquare(int pawn, int king, int queen, int ptm)
{
  register int pdist, kdist;
  pdist=abs((pawn>>3)-(queen>>3));
  kdist=(abs((king>>3)-(queen>>3)) > abs((king&7)-(queen&7))) ? 
    abs((king>>3)-(queen>>3)) : abs((king&7)-(queen&7));
  if (!ptm) pdist++;
  if (pdist < kdist) return(0);
  else return(1);
}

char* Normal(void)
{
  if (ansi)
#if defined(UNIX) || defined(AMIGA)
    return("\033[0m");
#else
    return("\033[1;44;37m");
#endif
  else return("");
}

int ParseTime(char* string)
{
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
      default: Print(0,"illegal character in time, please re-enter\n");
        break;
    }
    string++;
  }
  return(time*60+minutes);
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
int PinnedOnKing(int wtm, int square)
{
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
      if (And(AttacksRank(square),WhiteKing) != 0)
        return(And(And(AttacksRank(square),RooksQueens),
                   BlackPieces) != 0);
      else return(0);
    case 7: 
      if (And(AttacksDiaga1(square),WhiteKing) != 0)
        return(And(And(AttacksDiaga1(square),BishopsQueens),
                   BlackPieces) != 0);
      else return(0);
    case 8: 
      if (And(AttacksFile(square),WhiteKing) != 0)
        return(And(And(AttacksFile(square),RooksQueens),
                   BlackPieces) != 0);
      else return(0);
    case 9: 
      if (And(AttacksDiagh1(square),WhiteKing) != 0)
        return(And(And(AttacksDiagh1(square),BishopsQueens),
                   BlackPieces) != 0);
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
      if (And(AttacksRank(square),BlackKing) != 0)
        return(And(And(AttacksRank(square),RooksQueens),
                   WhitePieces) != 0);
      else return(0);
    case 7: 
      if (And(AttacksDiaga1(square),BlackKing) != 0)
        return(And(And(AttacksDiaga1(square),BishopsQueens),
                   WhitePieces) != 0);
      else return(0);
    case 8: 
      if (And(AttacksFile(square),BlackKing) != 0)
        return(And(And(AttacksFile(square),RooksQueens),
                   WhitePieces) != 0);
      else return(0);
    case 9: 
      if (And(AttacksDiagh1(square),BlackKing) != 0)
        return(And(And(AttacksDiagh1(square),BishopsQueens),
                   WhitePieces) != 0);
      else return(0);
    }
  }
  return(0);
}

void Print(int vb, char *fmt, ...)
{
  va_list ap;
  va_start(ap,fmt);
  if (vb <= verbosity_level) vprintf(fmt, ap);
  if (log_file) vfprintf(log_file, fmt, ap);
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
    for (i = 0; i < 55; i++) y[i] = x[i];
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

  while (move == 0) {
    status=fscanf(input,"%s",text);
    if (status <= 0) return(-1);
    if (((text[0]>='a') && (text[0]<='z')) ||
          ((text[0]>='A') && (text[0]<='Z'))) {
      if (!strcmp(text,"exit")) return(-1);
      move=InputMove(text+strspn(text,"0123456789."),0,wtm,1,0);
    }
  }
  return(move);
}

char* Reverse(void)
{
  if (ansi)
#if defined(UNIX) || defined(AMIGA)
    return("\033[7m");
#else
    return("\033[7;47;33m");
#endif
  else
    return("");
}


int TtoI(char *text)
{
  int t;
  char *n;

  t=0;
  if (!strchr(text,':')) t=atoi(text);
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

#if defined(COMPACT_ATTACKS)

#if !defined(USE_ASSEMBLY_A)

BITBOARD AttacksDiaga1Func (DIAG_INFO *diag, CHESS_POSITION *boardp)
{
  return AttacksDiaga1Int(diag,boardp);
}

BITBOARD AttacksDiagh1Func(DIAG_INFO *diag, CHESS_POSITION *boardp)
{
  return AttacksDiagh1Int(diag,boardp);
}

BITBOARD AttacksFileFunc(int square, CHESS_POSITION *boardp)
{
  return AttacksFileInt(square,boardp);
}

BITBOARD AttacksRankFunc(int square, CHESS_POSITION *boardp)
{
  BITBOARD tmp = Or((boardp)->w_occupied, (boardp)->b_occupied);

  unsigned char tmp2 = 
    at.rank_attack_bitboards[File(square)]
      [at.which_attack[File(square)]
        [And(SplitShiftr(tmp,(Rank(~(square))<<3)+1),0x3f) ] ];

  return SplitShiftl (tmp2, Rank(~(square))<<3);
}

BITBOARD AttacksBishopFunc(DIAG_INFO *diag, CHESS_POSITION *boardp)
{
  return Or(AttacksDiaga1Int(diag,boardp),
      AttacksDiagh1Int(diag,boardp));
}

BITBOARD AttacksRookFunc(int square, CHESS_POSITION *boardp)
{
  BITBOARD tmp = Or((boardp)->w_occupied, (boardp)->b_occupied);

  unsigned char tmp2 = 
    at.rank_attack_bitboards[File(square)][at.which_attack[File(square)]
        [And(SplitShiftr(tmp,(Rank(~(square))<<3)+1),0x3f)]];

  return Or(SplitShiftl (tmp2, Rank(~(square))<<3),
      AttacksFileInt(square,boardp));
}

unsigned MobilityDiaga1Func(DIAG_INFO *diag, CHESS_POSITION *boardp)
{
  return MobilityDiaga1Int(diag,boardp);
}

unsigned MobilityDiagh1Func(DIAG_INFO *diag, CHESS_POSITION *boardp)
{
  return MobilityDiagh1Int(diag,boardp);
}

unsigned MobilityFileFunc(int square, CHESS_POSITION *boardp)
{
  return MobilityFileInt (square,boardp);
}

unsigned MobilityRankFunc(int square, CHESS_POSITION *boardp)
{
  return MobilityRankInt (square,boardp);
}

#endif

unsigned char bishop_shift_rl45[64] = {
          59, 57, 54, 50, 45, 39, 32,  0,
          57, 54, 50, 45, 39, 32,  0,  8,
          54, 50, 45, 39, 32,  0,  8, 15,
          50, 45, 39, 32,  0,  8, 15, 21,
          45, 39, 32,  0,  8, 15, 21, 60,
          39, 32,  0,  8, 15, 21, 60, 26,
          32,  0,  8, 15, 21, 60, 26, 29,
           0,  8, 15, 21, 60, 26, 29, 31 };

unsigned char bishop_shift_rr45[64] = {
            0,  8, 15, 21, 60, 26, 29, 31,
           32,  0,  8, 15, 21, 60, 26, 29,
           39, 32,  0,  8, 15, 21, 60, 26,
           45, 39, 32,  0,  8, 15, 21, 60,
           50, 45, 39, 32,  0,  8, 15, 21,
           54, 50, 45, 39, 32,  0,  8, 15,
           57, 54, 50, 45, 39, 32,  0,  8,
           59, 57, 54, 50, 45, 39, 32,  0 };


unsigned char init_l45[64] = {
              4,  5,  7, 10, 14, 19, 25, 56, 
              6,  8, 11, 15, 20, 26, 57, 49,
              9, 12, 16, 21, 27, 58, 50, 43,
             13, 17, 22, 28, 59, 51, 44, 38,
             18, 23, 29, 60, 52, 45, 39,  0,
             24, 30, 61, 53, 46, 40,  1, 35,
             31, 62, 54, 47, 41,  2, 36, 33,
             63, 55, 48, 42,  3, 37, 34, 32 };
        
unsigned char init_r45[64] = {
             56, 49, 43, 38,  0, 35, 33, 32,
             25, 57, 50, 44, 39,  1, 36, 34,
             19, 26, 58, 51, 45, 40,  2, 37,
             14, 20, 27, 59, 52, 46, 41,  3,
             10, 15, 21, 28, 60, 53, 47, 42,
              7, 11, 16, 22, 29, 61, 54, 48,
              5,  8, 12, 17, 23, 30, 62, 55,
              4,  6,  9, 13, 18, 24, 31, 63 };

unsigned char init_l90[64] = {
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
static unsigned char n_attacks[9][8] =
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
static unsigned char n_length_attacks[9] =
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

static void InitializeMaps(BITBOARD *temp_rank_attack_bitboards)
{
  int file, rank;
  int diag;
  int gfile;
  BITBOARD *b;
  unsigned char *m;
  unsigned char *mobility_for_length[9];
  unsigned char diag_base [NDIAG] =
    { SQ(7,0), SQ(6,0), SQ(5,0), SQ(4,0), SQ(3,0), SQ(2,0), SQ(1,0), SQ(0,0),
      SQ(0,1), SQ(0,2), SQ(0,3), SQ(0,4), SQ(0,5), SQ(0,6), SQ(0,7) };
  unsigned char anti_base[NDIAG] =
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
    int len = DIAG_LENGTH(sq);
    int excess = 8 - len;
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

static void InitializeBrev (unsigned char brev[])
{
  unsigned value;

  for (value = 0; value < 64; value++) {
    unsigned br = 0;
    int i;
    for (i = 0; i < 6; i++) br |= (((value >> i) & 1) << (5 - i));
      brev[value] = br;
  }
}

#define MakeAttack(lower,upper) ((lower) | ((upper) << 3))

void ComputeAttacksAndMobility ()
{
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
      unsigned gfile_value = g << 1;
  
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
      for (a = 0; a < attacks_found; a++)
        if (attack == attacks_seen[a]) {
          found = 1;
          break;
        }
      if (!found) {
        int gf;
        attacks_seen[attacks_found] = attack;
        for (gf = 0; gf < NGFILES; gf++) {
          unsigned max_attacks = n_attacks[gfiles[gf].length][attacker];
          if (attacks_found < max_attacks) {
            BITBOARD b = 0;
            int i, p;
            for (p = attacker-1, i = 0; i < lower; i++, p--)
              Set(gfiles[gf].map[p], b);
            for (p=attacker+1,i=0;(i<upper) && (p<(unsigned) gfiles[gf].length);
                 i++, p++)
              Set(gfiles[gf].map[p], b);
            if (gfiles[gf].mobility)
            gfiles[gf].mobility[attacks_found] =
              lower + Min(upper, gfiles[gf].length - attacker - 1);
            gfiles[gf].bitboard[attacks_found] = b;
          }
        }
        attacks_found++;
      }
      at.which_attack[attacker][brev[g]] = a;
    }
    for (gf = 0; gf < NGFILES; gf++) {
      unsigned len = gfiles[gf].length;
      unsigned this_inc =
        (gfiles[gf].inc ? gfiles[gf].inc : n_attacks[len][attacker]);
      if (attacker < len) {
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

/*
********************************************************************************
*                                                                              *
*   Whisper() is used to whisper/kibitz information to a chess server.  it has *
*   to handle the xboard whisper/kibitz interface as well as the custom ics    *
*   interface for Crafty.  there are two main issues:  (a) presenting only the *
*   information specified by the current value of whisper or kibitz variables; *
*   (a) if using the custom ICS interface, preceeding the commands with a "*"  *
*   so the interface will direct them to the server rather than the operator.  *
*                                                                              *
********************************************************************************
*/
void Whisper(int level,int depth,int time,int value,unsigned int nodes,
             int cpu,char* pv)
{
  if (!puzzling) {
    switch (level) {
    case 1:
      if (kibitz && (value > 0)) {
        if (ics) printf("*");
        printf("kibitz mate in %d moves.\n\n",value);
      }
      else if (whisper && (value > 0)) {
        if (ics) printf("*");
        printf("whisper mate in %d moves.\n\n",value);
      }
      if (kibitz && (value < 0)) {
        if (ics) printf("*");
        printf("whisper mated in %d moves.\n\n",-value);
      }
      break;
    case 2:
      if (kibitz >= 2) {
        if (ics) printf("*");
        printf("kibitz depth %d; score %s; nodes %u; nps %d; cpu %d%%\n",
               depth,DisplayEvaluationWhisper(value),
               nodes,(time) ? 100*nodes/time : nodes,cpu);
      }
      else if (whisper >= 2) {
        if (ics) printf("*");
        printf("whisper depth %d; score %s; nodes %u; nps %d; cpu %d%%\n",
               depth,DisplayEvaluationWhisper(value),
               nodes,(time) ? 100*nodes/time : nodes,cpu);
      }
    case 3:
      if ((kibitz >= 3) && nodes) {
        if (ics) printf("*");
        printf("kibitz pv:%s\n",pv);
      }
      else if ((whisper >= 3) && nodes) {
        if (ics) printf("*");
        printf("whisper pv:%s\n",pv);
      }
      break;
    case 4:
      if (kibitz >= 4) {
        if (ics) printf("*");
        printf("kibitz %s\n",pv);
      }
      else if (whisper >= 4) {
        if (ics) printf("*");
        printf("whisper %s\n",pv);
      }
      break;
    case 5:
      if (kibitz >= 5) {
        if (ics) printf("*");
        printf("kibitz d%d-> %s %s %s\n",depth, DisplayTimeWhisper(time),
                                       DisplayEvaluationWhisper(value),pv);
      }
      else if (whisper >= 5) {
        if (ics) printf("*");
        printf("whisper d%d-> %s %s %s\n",depth, DisplayTimeWhisper(time),
                                       DisplayEvaluationWhisper(value),pv);
      }
      break;
    case 6:
      if (kibitz >= 6) {
        if (ics) printf("*");
        if (cpu == 0)
          printf("kibitz d%d+ %s %s %s\n",depth, DisplayTimeWhisper(time),
                                           DisplayEvaluationWhisper(value),pv);
        else
          printf("kibitz d%d+ %s >(%s) %s <re-searching>\n",depth,
                 DisplayTimeWhisper(time),DisplayEvaluationWhisper(value),pv);
      }
      else if (whisper >= 6) {
        if (ics) printf("*");
        if (cpu == 0)
          printf("whisper d%d+ %s %s %s\n",depth, DisplayTimeWhisper(time),
                                            DisplayEvaluationWhisper(value),pv);
        else
          printf("whisper d%d+ %s >(%s) %s <re-searching>\n",depth,
                 DisplayTimeWhisper(time),DisplayEvaluationWhisper(value),pv);
      }
      break;
    }
    if (post) {
      if (strstr(pv,"book"))
        printf("	%2d  %5d %7d %6u %s\n",depth,value/10,time,nodes,pv+10);
      else
        printf("	%2d  %5d %7d %6u %s\n",depth,value/10,time,nodes,pv);
    }
    fflush(stdout);
  }
}
