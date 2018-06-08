#include  <signal.h>
#include  <stdio.h>
#include  <errno.h>
#include  <string.h>
#include  <sys/types.h>
#include  <sys/wait.h>
#include  <sys/time.h>
#include  <sys/socket.h>
#include  <sys/uio.h>
#include  <netinet/in.h>
#include  <netdb.h>
#include  <unistd.h>

#define login "scrappy\n"
#define password "robertm1\n"
#define msg_to_ics "cpw"
#define msg_to_crafty "2cpw"

/*
********************************************************************************
*                                                                              *
*   ics is the custom ICS interface for Crafty.  in simple terms, it manages   *
*   I/O from three streams:  (1) user [stdin/stdout], (2) crafty itself, and   *
*   (3) ICS.                                                                   *
*                                                                              *
*   this interface is primarily designed with the user/ICS interface as the    *
*   primary data channel, so that the user interacts with ICS just as though   *
*   the connection was a telnet connection.  Crafty is a passive chess engine  *
*   that the interface routes new positions/moves to, and takes moves/other    *
*   types of output and echoes it directly to ICS bypassing the human link     *
*   completely.                                                                *
*                                                                              *
********************************************************************************
*/
int to_crafty=-1, from_crafty=-1;
int to_operator, from_operator;
int ICS;
char buffer[512], icsrec[128];
char ICS_buffer[16384], crafty_buffer[512],  operator_buffer[512];
int  ICS_data=0,       crafty_data=0,       operator_data=0;
int crafty_pid, crafty_running=0;

FILE *rc;

int black_time_remaining, white_time_remaining;
int current_game=-1, last_color=-1;
char last_move[16]={" "}, last_level[32]={" "};
int crafty_playing_white;
void ProcessBoardPosition(void);

int main(int argc, char *argv[],  char *env[]) {
  fd_set readfds;
  int bytes;
  int i,j,k,numreads;
  extern int  ConnectToICS(int, char**);
  extern void CraftyExited(int);
  extern void CraftyInput(void);
  extern void ExecCrafty(void);
  extern void ICSInput(void);
  extern void KillCrafty(void);
  extern void OperatorInput(void);

/*
 ----------------------------------------------------------
|                                                          |
|  set up to catch a SIGCHLD so that when Crafty exits, we |
|  will catch it and reset things correctly.               |
|                                                          |
 ----------------------------------------------------------
*/
  signal(SIGCHLD,CraftyExited);
/*
 ----------------------------------------------------------
|                                                          |
|  the first step is to create the socket to communicate   |
|  with ICS.                                               |
|                                                          |
 ----------------------------------------------------------
*/
  to_operator=fileno(stdout);
  from_operator=fileno(stdin);
  ICS=ConnectToICS(argc,argv);
/*
 ----------------------------------------------------------
|                                                          |
|  now comes the main loop.  here, we use select() to wait |
|  for data on the three input channels, and when select() |
|  returns, indicating that a read() is needed, we read on |
|  all the descriptors that have data buffered, and echo   |
|  this data to the appropriate output channel.            |
|                                                          |
 ----------------------------------------------------------
*/
  while (1) {
    FD_ZERO (&readfds);
    FD_SET (fileno(stdin), &readfds);
    if (crafty_running) FD_SET (from_crafty, &readfds);
    FD_SET (ICS, &readfds);
    numreads=select (16, &readfds, 0, 0, 0);
/*
 --------------------------------------
|                                      |
|  get data from ICS.                  |
|                                      |
 --------------------------------------
*/
    if (FD_ISSET(ICS, &readfds)) {
      bytes=recv(ICS,buffer,512,0);
      if (bytes <= 0) break;
      memcpy(ICS_buffer+ICS_data,buffer,bytes);
      ICS_data+=bytes;
      ICSInput();
      if (--numreads == 0) continue;
    }
/*
 --------------------------------------
|                                      |
|  get data from Crafty.               |
|                                      |
 --------------------------------------
*/
    if (crafty_running && FD_ISSET(from_crafty, &readfds)) {
      bytes=read(from_crafty,buffer,512);
      if (bytes <= 0) KillCrafty();
      memcpy(crafty_buffer+crafty_data,buffer,bytes);
      crafty_data+=bytes;
      CraftyInput();
      if (--numreads == 0) continue;
    }
/*
 --------------------------------------
|                                      |
|  get data from the operator.         |
|                                      |
 --------------------------------------
*/
    if (FD_ISSET(fileno(stdin), &readfds)) {
      bytes=read(fileno(stdin),buffer,512);
      memcpy(operator_buffer+operator_data,buffer,bytes);
      operator_data+=bytes;
      OperatorInput();
      if (--numreads == 0) continue;
    }
  }
  
  KillCrafty();
  printf("normal exit\n");
}

/*
 ----------------------------------------------------------
|                                                          |
|  bzero() fills an area of memory with zeroes.            |
|                                                          |
 ----------------------------------------------------------
*/
/*
void bzero(void *b, int length) {
	register char *p;

	for (p = b; length--;)
		*p++ = '\0';
}
*/

/*
 ----------------------------------------------------------
|                                                          |
|  ConnectToICS creates a socket and connects to the host  |
|  and port given on the command line.  it returns this    |
|  socket to the caller.                                   |
|                                                          |
 ----------------------------------------------------------
*/
int ConnectToICS(int argc, char *argv[]) {
/*
  char ICSHost[]={"chess.net"};
  char ICSHost[]={"chessclub.com"};
  char ICSHost[]={"fics.onenet.net"};
*/
  char ICSHost[]={"chessclub.com"};
  short port;
  int  ICS_socket;
  struct sockaddr ICS_address;
  struct hostent *hp, *gethostbyname();
  /*void bzero(void *, int);*/

  if ((ICS_socket=socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    printf("cannot create socket.\n");
    exit(1);
  }
  bzero (&ICS_address,sizeof(ICS_address));
  ICS_address.sa_family = AF_INET;
  if (argc > 1)
    hp=gethostbyname(argv[1]);
  else
    hp=gethostbyname(ICSHost);
  memcpy((unsigned char *) &ICS_address.sa_data[2],
         (unsigned char *) hp->h_addr, 
         hp->h_length);
  if (argc > 2) 
    port=htons(atoi(argv[2]));
  else
    port=htons(5000);
  memcpy((unsigned char *) &ICS_address.sa_data[0],
         (unsigned char *) &port,
         2);
  if (connect(ICS_socket, &ICS_address, sizeof(ICS_address)) < 0) {
    printf ("cannot connect to host : %s at Port : %d\n", argv[1], port);
    exit(1);
  }
  return(ICS_socket);
}
/*
 ----------------------------------------------------------
|                                                          |
|  ExecCrafty sets up the pipes and starts Crafty by using |
|  fork() to spawn a new process, then execve() to execute |
|  crafty instead of the cloned interface code created by  |
|  the fork() call.                                        |
|                                                          |
 ----------------------------------------------------------
*/
void ExecCrafty() {
  char crafty[]={"crafty"};
  char *argv[3], *env[2];
  
  int tc[2],fc[2];

  if (!crafty_running) {
    printf("Executing Crafty\n");
    argv[0]=crafty;
    argv[1]="ics";
    argv[2]=0;
    env[0]=crafty;
    env[1]=0;
    signal(SIGPIPE, SIG_IGN);
    pipe(tc);
    pipe(fc);
    to_crafty=tc[1];
    from_crafty=fc[0];
/*
 ------------------------------------------------
|                                                |
|  child process has to force stdin, stdout and  |
|  stderr to use the two pipes we just created.  |
|                                                |
 ------------------------------------------------
*/
    if ((crafty_pid = fork()) == 0) {
      dup2(tc[0], 0);
      dup2(fc[1], 1);
      close(tc[0]);
      close(tc[1]);
      close(fc[0]);
      close(fc[1]);
      dup2(1, fileno(stderr));
      execve("crafty",argv,env);
      printf("ERROR: unable to execute Crafty!\n");
      exit(1);
    }
/*
 ------------------------------------------------
|                                                |
|  parent will then feed crafty * lines from the |
|  .icsinit file to set things up correctly.     |
|                                                |
 ------------------------------------------------
*/
    crafty_running=1;
    close(tc[0]);
    close(fc[1]);
    rc=fopen(".icsinit","r");
    if (rc) {
      fgets(icsrec,128,rc);
      while (!feof(rc)) {
        if (icsrec[0] == '*')
          write(to_crafty,icsrec+1,strlen(icsrec)-1);
        fgets(icsrec,128,rc);
      }
      fclose(rc);
    }
  }
}

/*
 ----------------------------------------------------------
|                                                          |
|  CraftyExited is a signal handler that gets control when |
|  Crafty exits for whatever reason.  it simply does a     |
|  waitpid() to clear the zombie process then resets the   |
|  flags so it will be restarted when needed.              |
|                                                          |
 ----------------------------------------------------------
*/
void CraftyExited(int sig) {
  int status;
  if (to_crafty >= 0) close(to_crafty);
  if (from_crafty >= 0) close(from_crafty);
  to_crafty=-1;
  from_crafty=-1;
  crafty_running=0;
  current_game=-1;
  last_color=-1;
  strcpy(last_move," ");
  waitpid(crafty_pid,&status,0);
  printf("crafty terminated\n");
}

/*
 ----------------------------------------------------------
|                                                          |
|  CraftyInput() handles messages from Crafty.  if the     |
|  message starts with a "*", it is removed and the mess-  |
|  age is sent to ICC.  if not, the message is sent to the |
|  operator only.                                          |
|                                                          |
 ----------------------------------------------------------
*/
void CraftyInput(void) {
  char buffer[512];
  char last_buffer[512];
  int i,len;

  crafty_buffer[crafty_data]=0;
  while (strchr(crafty_buffer,'\n')) {
    *strchr(crafty_buffer,'\n')=0;
    memcpy(buffer,crafty_buffer,strlen(crafty_buffer)+1);
    memmove(crafty_buffer,crafty_buffer+strlen(crafty_buffer)+1,
           crafty_data-strlen(buffer));

    len=strlen(buffer);
    buffer[len]='\n';
    if (strcmp(buffer+1,last_buffer+1)) {
      if (buffer[0] == '*') {
        send(ICS,buffer+1,len,0);
        strcpy(last_buffer+1,buffer+1);
      }
      else if (strncmp(buffer,"tellics ",8) == 0) {
        send(ICS,buffer+8,len-8,0);
      }
      else {
        buffer[len+1]=0;
        printf("%s",buffer);
      }
    }
    crafty_data-=len+1;
  }
}

/*
 ----------------------------------------------------------
|                                                          |
|  ICSInput handles data coming in from ICS.  it either    |
|  directs it to the appropriate output channel, or else   |
|  parses it and takes action as appropriate.              |
|                                                          |
 ----------------------------------------------------------
*/
void ICSInput(void) {
  char buffer[512], orig[512];
  char *ch, *tch, *token;
  int buflen;

  ICS_buffer[ICS_data]=0;
  while (ch=strchr(ICS_buffer,'\n')) {
    *ch=0;
    buflen=strlen(ICS_buffer)+1;
    memcpy(buffer,ICS_buffer,buflen);
    memmove(ICS_buffer,ICS_buffer+buflen,ICS_data-buflen+1);
    ICS_data-=buflen;
    strcpy(orig,buffer);
    if (strstr(buffer,"+refresh+")) {
      token=strtok(strstr(buffer,"refresh")," ");
      token=strtok(0," ");
      if (token) {
        send(ICS,"refresh ",8,0);
        send(ICS,token,strlen(token),0);
        send(ICS,"\n",1,0);
      }
    }
    token=strtok(buffer," 	");
    if (token == 0);
    else if (strcmp(token,"<12>") == 0 || strcmp(token,"\r<12>") == 0) {
      ProcessBoardPosition();
    }
    else if (tch=strstr(orig,msg_to_ics)) {
      send(ICS,tch+strlen(msg_to_ics),strlen(tch)-strlen(msg_to_ics),0);
      send(ICS,"\n",1,0);
    }
    else if (tch=strstr(orig,msg_to_crafty)) {
      ExecCrafty();
      write(to_crafty,tch+strlen(msg_to_crafty),strlen(tch)-strlen(msg_to_crafty));
      write(to_crafty,"\n",1);
      write(to_crafty,"eot\n",4);
    }
    else {
      printf("%s\n",orig);
      if (strstr(orig,"taking back ")) current_game=-1;
    }
  }
/*
 -------------------------------------------
|                                           |
|  if we have unused data, it could be the  |
|  first part of an incomplete command, so  |
|  we wait for the rest.  or, it could be a |
|  login: or password: prompt.  if so, we   |
|  fire the correct string right back.  if  |
|  we get a "password" prompt, we send the  |
|  password and then follow that by any     |
|  lines in .icsinit that don't start with  |
|  a '*' character.                         |
|                                           |
 -------------------------------------------
*/
  if (ICS_data) 
    if (strstr(ICS_buffer,"login") ||
        strstr(ICS_buffer,"password") ||
        strstr(ICS_buffer,"aics")) {
      ICS_buffer[ICS_data]=0;
      printf("%s",ICS_buffer);
      fflush(stdout);
      if (strstr(ICS_buffer,"login"))
        send(ICS,login,strlen(login),0);
      else if (strstr(ICS_buffer,"password")) {
        send(ICS,password,strlen(password),0);
        rc=fopen(".icsinit","r");
        if (rc) {
          fgets(icsrec,128,rc);
          while (!feof(rc)) {
            if (icsrec[0] != '*') 
              send(ICS,icsrec,strlen(icsrec),0);
            fgets(icsrec,128,rc);
          }
          fclose(rc);
        }
      }
      ICS_data=0;
    }
}

/*
 ----------------------------------------------------------
|                                                          |
|  KillCrafty terminates crafty when we exit, or after a   |
|  game ends, so that it won't be left in a state where it |
|  burns a lot of excessive cpu cycles pondering a move    |
|  that will never be made.                                |
|                                                          |
 ----------------------------------------------------------
*/
void KillCrafty() {
  if (to_crafty >= 0) close(to_crafty);
  if (from_crafty >= 0) close(from_crafty);
  to_crafty=-1;
  from_crafty=-1;
  crafty_running=0;
  current_game=-1;
  last_color=-1;
  strcpy(last_move," ");
  kill(crafty_pid,SIGKILL);
}

/*
 ----------------------------------------------------------
|                                                          |
|  OperatorInput() handles messages from the operator.  if |
|  the message starts with a "*", it is sent only to       |
|  Crafty.  otherwise it only goes to ICC. ** is a special |
|  flag used after a takeback so that the board will be    |
|  reset using setboard, to clear the position.            |
|                                                          |
 ----------------------------------------------------------
*/
void OperatorInput(void) {
  char buffer[512];
  int i,buflen;

  operator_buffer[operator_data]=0;
  while (strchr(operator_buffer,'\n')) {
    *strchr(operator_buffer,'\n')=0;
    buflen=strlen(operator_buffer)+1;
    memcpy(buffer,operator_buffer,buflen);
    memmove(operator_buffer,operator_buffer+buflen,operator_data-buflen);

    buffer[buflen-1]='\n';
    if (buffer[0] == '*')
      if (buffer[1] == '*') current_game=-1;
      else {
        ExecCrafty();
        write(to_crafty,buffer+1,buflen-1);
        write(to_crafty,"eot\n",4);
      }
    else
      send(ICS,buffer,buflen,0);
    if (strcmp(buffer,"refresh") == 0) {
      current_game=-1;
      strcpy(last_move," ");
    }
    operator_data-=buflen;
  }
}

/*
 ----------------------------------------------------------
|                                                          |
|  ProcessBoardPosition() handles the style 12 board       |
|  description sent by ICC/FICS.  if this board goes with  |
|  the current game, we just shoot the move to crafty and  |
|  we are done.  if not, we compose a setboard command to  |
|  set to this position and then ship it over.  if it's    |
|  our turn, we also send a "go" command.                  |
|                                                          |
|  we get here when the first "token" on a line is <12>    |
|                                                          |
 ----------------------------------------------------------
*/
void ProcessBoardPosition(void) {
  char *token, *next, setboard[96], level[32], move[16], sq, *wt, *bt;
  char *wtoken, *btoken, chanid[64], movenum[16];
  int eptarget, i, j, color, empty, this_game, relation;
  int time, increment;

/*
 -------------------------------------------
|                                           |
|  first process the 64 squares of the      |
|  board and construct a setboard command   |
|  to be used if needed.                    |
|                                           |
 -------------------------------------------
*/
  ExecCrafty();
  next=setboard;
  for (i=0;i<8;i++) {
    token=strtok(0," ");
    empty=0;
    for (j=0;j<8;j++) {
      sq=*(token+j);
      switch (sq) {
      case '-':
        empty++;
        break;
      default:
        if (empty) {
          *next++='0'+empty;
          empty=0;
        }
        *next++=sq;
        break;
      }
    }
    *next++='/';
  }
/*
 -------------------------------------------
|                                           |
|  now pick up the color, castling and then |
|  the enpassant target (if any).           |
|                                           |
 -------------------------------------------
*/
  *next++=' ';
  token=strtok(0," ");
  *next++=tolower(*token);
  color=(*token == 'W') ? 1 : 0;


  token=strtok(0," ");
  if ((*token>'0') && (*token<'9'))
    eptarget=atoi(token);
  else
    eptarget=0;

  *next++=' ';
  token=strtok(0," ");
  if (*token == '1') *next++='K';
  token=strtok(0," ");
  if (*token == '1') *next++='Q';
  token=strtok(0," ");
  if (*token == '1') *next++='k';
  token=strtok(0," ");
  if (*token == '1') *next++='q';

  if (eptarget) {
    *next++=' ';
    *next++='a'+eptarget;
    *next++='3'+3*color;
  }
  *next=0;
/*
 -------------------------------------------
|                                           |
|  skip the 50 move count for now and pick  |
|  up the current game so we can tell if we |
|  need to send a setboard command.         |
|                                           |
 -------------------------------------------
*/

  token=strtok(0," "); /* skip 50move count */

  token=strtok(0," ");
  this_game=atoi(token);

/*
 -------------------------------------------
|                                           |
|  skip several fields and pick up the time |
|  remaining for each side and save it.     |
|                                           |
 -------------------------------------------
*/
  wtoken=strtok(0," "); /* skip white's name */
  btoken=strtok(0," "); /* skip black's name */
  sprintf(chanid,"channel * %s/%s\n",wtoken,btoken);
  token=strtok(0," "); 

  relation=atoi(token);

  token=strtok(0," "); /* process time */
  time=atoi(token);
  token=strtok(0," "); /* process increment */
  increment=atoi(token);
  sprintf(level,"level=0;%d;%d\n",time,increment);
/*
  if (strcmp(level,last_level)) write(to_crafty,level,strlen(level));
*/
  strcpy(last_level,level);

  token=strtok(0," "); /* skip white strength */
  token=strtok(0," "); /* skip black strength */

  token=strtok(0," "); 
  white_time_remaining=atoi(token);
  wt=token;
  token=strtok(0," "); 
  black_time_remaining=atoi(token);
  bt=token;
/*
 -------------------------------------------
|                                           |
|  skip several fields and pick up the move |
|  made.  we'll send this to crafty if the  |
|  same game is being played.               |
|                                           |
 -------------------------------------------
*/

  token=strtok(0," "); /* skip move number */
  strcpy(movenum,token);
  token=strtok(0," "); /* skip verbose move format */
  token=strtok(0," "); /* skip last move time */

  token=strtok(0," "); /* skip verbose move format */
  strcpy(move,token);

/*
 -------------------------------------------
|                                           |
|  done.  if this is the same game as when  |
|  we reached here the last time, then we   |
|  simply ship the move to crafty.  other-  |
|  wise, we set the position and "go"       |
|                                           |
 -------------------------------------------
*/
  if (this_game != current_game) {
    write(to_crafty,chanid,strlen(chanid));
    write(to_crafty,"timeleft ",9);
    write(to_crafty,wt,strlen(wt));
    write(to_crafty,";",1);
    write(to_crafty,bt,strlen(bt));
    write(to_crafty,"\n",1);
    write(to_crafty,"setboard ",9);
    write(to_crafty,setboard,strlen(setboard));
    write(to_crafty,"\n",1);
    write(to_crafty,"mn ",3);
    write(to_crafty,movenum,strlen(movenum));
    write(to_crafty,"\n",1);
    current_game=this_game;
    if (relation == 1) {
      write(to_crafty,"go\n",3);
      write(to_crafty,"eot\n",4);
    }
  }
  else {
    if ((relation == 1) || (relation == 0) || (relation == -2)) {
      if (strcmp(move,last_move) || (color != last_color)) {
        write(to_crafty,"timeleft ",9);
        write(to_crafty,wt,strlen(wt));
        write(to_crafty,";",1);
        write(to_crafty,bt,strlen(bt));
        write(to_crafty,"\n",1);

        write(to_crafty,move,strlen(move));
        write(to_crafty,"\n",1);

        strcpy(last_move,move);
        last_color=color;
      }
    }
  }
}
