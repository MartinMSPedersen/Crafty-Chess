#include  <signal.h>
#include  <stdio.h>
#include  <stdlib.h>
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

int parg[2];
char plog[2][32];
char result[128];
int over;
int tomove;
int moveno;
int mtime, lasttime;
int trusted = -1;
int first[2];
int to[2][2], from[2][2], pid[2];
char name[2][32];
int ngames = -1, stime = -1, inc = -1;
char *program[2], *host[2], args[2];
char position[256], *PGNname = NULL;
char buffer[512], cmd_buffer[2][4096];
int black_time_remaining, white_time_remaining;
fd_set readfds;
char PGNmoves[65536];
struct timeval maxtime;

/*
  ----------------------------------------------------------
 |                                                          |
 |  AddToPGN() adds each move to the PGN game history so it |
 |  can be saved for later analysis.                        |
 |                                                          |
  ----------------------------------------------------------
 */
void AddToPGN(char *move, int moveno, int wtm)
{
  char buf[128];

  if (wtm) {
    sprintf(buf, "%d. ", moveno);
    strcat(PGNmoves, buf);
  }
  strcat(PGNmoves, move);
  strcat(PGNmoves, " ");
}

/*
  ----------------------------------------------------------
 |                                                          |
 |  SavePGN() dumps the PGN to the match save file for      |
 |  later analysis.                                         |
 |                                                          |
  ----------------------------------------------------------
 */
void SavePGN(int round, char *white_name, char *black_name, char *result)
{
  FILE *savepgn;
  char *moves, *t;

  savepgn = fopen(PGNname, "a");
  fprintf(savepgn, "[Event \"Computer chess game\"]\n");
  fprintf(savepgn, "[Site \"olympus.cis.uab.edu\"]\n");
  fprintf(savepgn, "[Date \"2007.02.08\"]\n");
  fprintf(savepgn, "[Round \"%d\"]\n", round);
  fprintf(savepgn, "[White \"%s\"]\n", white_name);
  fprintf(savepgn, "[Black \"%s\"]\n", black_name);
  fprintf(savepgn, "[Result \"%s\"]\n", result);
  fprintf(savepgn, "[TimeControl \"%d+%d\"]\n", stime, inc);
  fprintf(savepgn, "[FEN \"%s\"]\n\n", position);
  strcat(PGNmoves, result);
  moves = PGNmoves;
  while (strlen(moves) > 60) {
    t = strpbrk(moves + 60, " ");
    if (!t)
      break;
    *t = 0;
    fprintf(savepgn, "%s\n", moves);
    moves = t + 1;
  }
  if (strlen(moves))
    fprintf(savepgn, "%s\n", moves);
  fprintf(savepgn, "\n");
  fflush(savepgn);
  fclose(savepgn);
}

/*
  ----------------------------------------------------------
 |                                                          |
 |  KillOpponent terminates crafty when we exit, or after a |
 |  game ends, so that it won't be left in a state where it |
 |  burns a lot of excessive cpu cycles pondering a move    |
 |  that will never be made.                                |
 |                                                          |
  ----------------------------------------------------------
 */
void KillOpponent(int which)
{
  if (pid[which]) {
    write(to[which][1], "quit\n", 6);
#if defined(DEBUG)
    printf("1.sending[%d]->quit\n", which);
    fflush(stdout);
#endif
    sleep(1);
    kill(pid[which], SIGKILL);
    pid[which] = 0;
  }
}

/*
  ----------------------------------------------------------
 |                                                          |
 |  Read() grabs the next line (terminated by a N/L and     |
 |  copies it to the input buffer.  it returns 0 if there   |
 |  is no complete input line.                              |
 |                                                          |
  ----------------------------------------------------------
 */
int Read(int which, char *buffer)
{
  char *eol, *ret, readdata;

  *buffer = 0;
  eol = strchr(cmd_buffer[which], '\n');
  if (!eol)
    return(0);
  *eol = 0;
  ret = strchr(cmd_buffer[which], '\r');
  if (ret)
    *ret = ' ';
  strcpy(buffer, cmd_buffer[which]);
  memmove(cmd_buffer[which], eol + 1, strlen(eol + 1) + 1);
  return (1);
}

unsigned int ReadClock(void)
{
  struct timeval timeval;
  struct timezone timezone;

  gettimeofday(&timeval, &timezone);
  return (timeval.tv_sec * 100 + (timeval.tv_usec / 10000));
}

/*
  ----------------------------------------------------------
 |                                                          |
 |  PlayGame() is the main workhorse of this program.  It   |
 |  creates two processes to run the two chess engines,     |
 |  initializes the engines, and then ships moves back and  |
 |  forth until the game is over.  It also collects the     |
 |  moves to produce the PGN output that is saved for each  |
 |  game played.                                            |
 |                                                          |
  ----------------------------------------------------------
 */
int PlayGame(int who_goes_first)
{
  int i, j, bytes;
  char *argv[10];
  int elapsed, lasttime, mtime;
  int onmove, onmove_color;
  int pcolor[2];

/*
  ----------------------------------------------------------
 |                                                          |
 |  figure out who is playing white/black so we can handle  |
 |  the resign stuff properly.                              |
 |                                                          |
  ----------------------------------------------------------
*/
  onmove = who_goes_first;
  onmove_color = tomove;
  if (who_goes_first == 0) {
    pcolor[0] = onmove_color;
    pcolor[1] = 1 - onmove_color;
  }
  else {
    pcolor[1] = onmove_color;
    pcolor[0] = 1 - onmove_color;
  }
  
/*
  ----------------------------------------------------------
 |                                                          |
 |  first, we need to create a new process for each engine. |
 |  this process will either exec() the engine directly if  |
 |  there are no "host" options provided, or else it will   |
 |  exec() "ssh" to execute the actual engine on a remote   |
 |  machine (in a cluster environment usually).             |
 |                                                          |
  ----------------------------------------------------------
 */
  for (i = 0; i < 2; i++) {
    pipe(to[i]);
    pipe(from[i]);
  }
  strcpy(buffer, "                                            ");
  first[0] = 1;
  first[1] = 1;
  for (i = 0; i < 2; i++) {
    char *exec, *args[10], *env[2];

    if ((pid[i] = fork()) == 0) {
      int arg = 0;
      dup2(to[i][0], 0);
      dup2(from[i][1], 1);
      dup2(from[i][1], 2);
      if (strcmp(host[i], "localhost")) {
        argv[arg++] = "/usr/bin/ssh";
        argv[arg++] = host[i];
        if (strstr(program[i],".ini")) {
          argv[arg++] = "/shared/home/hyatt/polyglot";
          argv[arg++] = program[i];
        }
          argv[arg++] = program[i];
        if (strstr(program[i], "crafty"))
          argv[arg++] = plog[i];
        argv[arg++] = NULL;
      } else {
        if (strstr(program[i], ".ini"))
          argv[arg++] = "/shared/home/hyatt/polyglot";
        argv[arg++] = program[i];
        if (strstr(program[i], "crafty"))
          argv[arg++] = plog[i];
        argv[arg++] = NULL;
      }
#if defined(DEBUG)
      printf("===========================================\n");
      for (j=0; j<arg; j++)
        printf("argv[%d] = \"%s\"\n", j, argv[j]);
      printf("===========================================\n");
      fflush(stdout);
#endif
      env[0] = "LD_LIBRARY_PATH=/opt/intel/cce/9.1.042/lib";
      env[1] = 0;
      execve(argv[0], argv, env);
      printf("ERROR: unable to execute %s for program[%d]\n", argv[0], i);
      exit(1);
    }
#if defined(DEBUG)
      printf("new child (%d) PID = %d\n", i, pid[i]);
      fflush(stdout);
#endif
  }
/*
  ----------------------------------------------------------
 |                                                          |
 |  The engines have been started.  Now we send them the    |
 |  usual xboard protocol startup commands to prepare to    |
 |  actually play the game.                                 |
 |                                                          |
  ----------------------------------------------------------
 */
  char commands[6][32] =
      { "xboard\n", "new\n", "random\n", "easy\n", "computer\n",
    "protover 2\n"
  };
  for (i = 0; i < 2; i++) {
    for (j = 0; j < 6; j++) {
      write(to[i][1], commands[j], strlen(commands[j]));
#if defined(DEBUG)
      printf("2.sending[%d]->%s", i, commands[j]);
      fflush(stdout);
#endif
    }
  }
  for (i = 0; i < 2; i++) {
    char command[128];

    strcpy(command, "level 0");
    sprintf(command + strlen(command), " %d", stime);
    sprintf(command + strlen(command), " %d\n", inc);
    write(to[i][1], command, strlen(command));
#if defined(DEBUG)
    printf("3.sending[%d]->%s", i, command);
    fflush(stdout);
#endif
  }
  for (i = 0; i < 2; i++) {
    write(to[i][1], "force\n", 6);
#if defined(DEBUG)
    printf("4.sending[%d]->force\n", i);
    fflush(stdout);
#endif
    write(to[i][1], "setboard ", 9);
    write(to[i][1], position, strlen(position));
    write(to[i][1], "\n", 1);
#if defined(DEBUG)
    printf("5.sending[%d]->setboard ", i);
    printf("6.sending[%d]->%s\n", i, position);
    fflush(stdout);
#endif
  }
  white_time_remaining = 6000 * stime;
  black_time_remaining = 6000 * stime;
  for (i = 0; i < 2; i++) {
    char command[32];

    sprintf(command, "time %d\n", white_time_remaining);
    write(to[i][1], command, strlen(command));
#if defined(DEBUG)
    printf("7.sending[%d]->%s", i, command);
    fflush(stdout);
#endif
    sprintf(command, "otim %d\n", black_time_remaining);
    write(to[i][1], command, strlen(command));
#if defined(DEBUG)
    printf("8.sending[%d]->%s", i, command);
    fflush(stdout);
#endif
  }
  for (i=0; i<2; i++)
    if (strstr(program[i], "crafty"))
      trusted = i;
#if defined(DEBUG)
  printf("trusted process = %d\n", trusted);
  printf("player 0 has color %d\n", pcolor[0]);
  printf("player 1 has color %d\n", pcolor[1]);
  fflush(stdout);
#endif
/*
  ----------------------------------------------------------
 |                                                          |
 |  The engines have been started.  Now we wait until we    |
 |  get the expected "done=1" before we start the actual    |
 |  game.                                                   |
 |                                                          |
  ----------------------------------------------------------
 */
  bytes = 1;
  while (bytes > 0) {
    char *end;
    bytes = read(from[0][0], buffer, 512);
    end = cmd_buffer[0] + strlen(cmd_buffer[0]);
    memcpy(end, buffer, bytes);
    *(end + bytes) = 0;
    while (Read(0, buffer)) {
#if defined(DEBUG)
      printf("1.received[%d]->%s\n", 0, buffer);
      fflush(stdout);
#endif
      if (strstr(buffer, "myname")) {
        char *myname = strstr(buffer, "myname") + 7;
        myname = myname + strspn(myname, " ");
        myname = strtok(myname, "\"");
        strcpy(name[0], myname);
      }
      if (strstr(buffer, "done=1"))
        break;
    }
    if (strstr(buffer, "done=1"))
      break;
  }
  bytes = 1;
  while (bytes > 0) {
    char *end;
    bytes = read(from[1][0], buffer, 512);
    end = cmd_buffer[1] + strlen(cmd_buffer[1]);
    memcpy(end, buffer, bytes);
    *(end + bytes) = 0;
    while (Read(1, buffer)) {
#if defined(DEBUG)
      printf("2,received[%d]->%s\n", 1, buffer);
      fflush(stdout);
#endif
      if (strstr(buffer, "myname")) {
        char *myname = strstr(buffer, "myname") + 7;
        myname = myname + strspn(myname, " ");
        myname = strtok(myname, "\"");
        strcpy(name[1], myname);
      }
      if (strstr(buffer, "done=1"))
        break;
    }
    if (strstr(buffer, "done=1"))
      break;
  }
  lasttime = ReadClock();
  if (who_goes_first) {
    write(to[1][1], "go\n", 3);
#if defined(DEBUG)
    printf("9.sending[1]->go\n");
    fflush(stdout);
#endif
  } else {
    write(to[0][1], "go\n", 3);
#if defined(DEBUG)
    printf("10.sending[0]->go\n");
    fflush(stdout);
#endif
  }
  onmove = who_goes_first;
  onmove_color = tomove;
  moveno = 1;
  strcpy(PGNmoves, "");
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
  over = 0;
  while (!over) {
    int numreads;

    if (pid[0] == 0 && pid[1] == 0)
      break;
    FD_ZERO(&readfds);
    if (pid[0])
      FD_SET(from[0][0], &readfds);
    if (pid[1])
      FD_SET(from[1][0], &readfds);
    if (onmove_color == 1) {
      maxtime.tv_sec = white_time_remaining / 100;
      maxtime.tv_usec = (white_time_remaining % 100) * 10000;
    } else {
      maxtime.tv_sec = black_time_remaining / 100;
      maxtime.tv_usec = (black_time_remaining % 100) * 10000;
    }
    if (maxtime.tv_sec < 0)
      maxtime.tv_sec = 1;
    if (maxtime.tv_usec < 0)
      maxtime.tv_usec = 1;
    numreads = select(16, &readfds, 0, 0, &maxtime);
    if (numreads < 0) {
      if (errno == EINTR)
        continue;
      else
        break;
    } else if (numreads == 0) {
      if (onmove_color == 1)
        strcpy(result, "0-1 {White's flag fell}");
      else
        strcpy(result, "1-0 {Black's flag fell}");
      over = 1;
    }
/*
  --------------------------------------
 |                                      |
 |  get data from a player.             |
 |                                      |
  --------------------------------------
*/
    for (i = 0; i < 2; i++) {
      if (FD_ISSET(from[i][0], &readfds)) {
        char *end;
        int bytes = read(from[i][0], buffer, 512);
        if (bytes <= 0) {
          KillOpponent(i);
          continue;
        }
        end = cmd_buffer[i] + strlen(cmd_buffer[i]);
        memcpy(end, buffer, bytes);
        *(end + bytes) = 0;
        while (Read(i, buffer)) {
#if defined(DEBUG)
          printf("3.received[%d]->%s\n", i, buffer);
          fflush(stdout);
#endif
          if (strstr(buffer, "myname")) {
            char *myname = strstr(buffer, "myname") + 7;

            myname = myname + strspn(myname, " ");
            myname = strtok(myname, "\"");
            strcpy(name[i], myname);
          }
          else if (strstr(buffer, "offer draw")) {
            write(to[onmove ^ 1][1], "draw\n", 6);
#if defined(DEBUG)
            printf("11.sending[%d]->draw\n", i ^ 1);
            fflush(stdout);
#endif
          }
          else if (strstr(buffer, "tellics resign")) {
            write(to[onmove ^ 1][1], "resign\n", 8);
#if defined(DEBUG)
            printf("12.sending[%d]->resign\n", i ^ 1);
            fflush(stdout);
#endif
            if (onmove_color == 0)
              strcpy(result, "1-0 {Black resigns}");
            else
              strcpy(result, "0-1 {White resigns}");
            over = 1;
            for (i = 0; i < 2; i++)
              KillOpponent(i);
          }
          else if (strstr(buffer, "1-0 {") || strstr(buffer, "0-1 {") ||
              strstr(buffer, "1/2-1/2 {")) {
            if (trusted == i) {
              strcpy(result, buffer);
              over = 1;
            }
            else if (strstr(buffer, "resigns")) {
              if (pcolor[i] == 0)
                strcpy(result, "1-0 {Black resigns}");
              else
                strcpy(result, "0-1 {White resigns}");
              over = 1;
            }
          }
          else if (strstr(buffer, "move"))
            do {
              char *move;
  
              if (strstr(buffer, "llegal"))
                break;
              move =
                strstr(buffer, "move") + strspn(strstr(buffer, "move") + 4,
                  " ") + 4;
#if defined(DEBUG)
              printf("found the move \"%s\"\n", move);
              fflush(stdout);
#endif
              mtime = ReadClock();
              elapsed = mtime - lasttime;
              lasttime = mtime;
              if (onmove_color == 1)
                white_time_remaining -= elapsed - inc * 100;
              else
                black_time_remaining -= elapsed - inc * 100;
              if (white_time_remaining < 0) {
                strcpy(result, "0-1 {White's flag fell}");
                for (i = 0; i < 2; i++)
                  KillOpponent(i);
              }
              if (black_time_remaining < 0) {
                strcpy(result, "1-0 {Black's flag fell}");
                for (i = 0; i < 2; i++)
                  KillOpponent(i);
              }
              AddToPGN(move, moveno, onmove_color);
              onmove_color ^= 1;
              onmove ^= 1;
              if (onmove_color)
                moveno++;
              if (onmove_color) {
                char command[32];
  
                sprintf(command, "time %d\n", white_time_remaining);
                write(to[onmove][1], command, strlen(command));
#if defined(DEBUG)
                printf("13.sending[%d]->%s", onmove, command);
                fflush(stdout);
#endif
                sprintf(command, "otim %d\n", black_time_remaining);
                write(to[onmove][1], command, strlen(command));
#if defined(DEBUG)
                printf("14.sending[%d]->%s", onmove, command);
                fflush(stdout);
#endif
              } else {
                char command[32];

                sprintf(command, "time %d\n", black_time_remaining);
                write(to[onmove][1], command, strlen(command));
#if defined(DEBUG)
                printf("15.sending[%d]->%s", onmove, command);
                fflush(stdout);
#endif
                sprintf(command, "otim %d\n", white_time_remaining);
                write(to[onmove][1], command, strlen(command));
#if defined(DEBUG)
                printf("16.sending[%d]->%s", onmove, command);
                fflush(stdout);
#endif
              }
              write(to[onmove][1], move, strlen(move));
              write(to[onmove][1], "\n", 1);
#if defined(DEBUG)
              printf("17.sending[%d]->%s\n", onmove, move);
              fflush(stdout);
#endif
              if (first[onmove]) {
                write(to[onmove][1], "go\n", 3);
#if defined(DEBUG)
                printf("18.sending[%d]->go\n", onmove);
                fflush(stdout);
#endif
              }
              first[0] = 0;
              first[1] = 0;
              fflush(stdout);
            }
          while (0);
        }
      }
    }
  }
  for (i = 0; i < 2; i++) {
    KillOpponent(i);
    close(to[i][0]);
    close(to[i][1]);
    close(from[i][0]);
    close(from[i][1]);
  }
}

/*
 ----------------------------------------------------------
|                                                          |
|  ProgramExited is a signal handler that gets control     |
|  when either program exits, so that we can clean up and  |
|  not have zombie processes lying around.                 |
|                                                          |
 ----------------------------------------------------------
*/
void ProgramExited(int sig)
{
  int status, i = 1, j;

  while (i > 0) {
    i = waitpid(-1, &status, WNOHANG);
#if defined(DEBUG)
    if (i > 0)
      printf("pid %d exited\n", i);
    fflush(stdout);
#endif
  }
}

/*
 *******************************************************************************
 *                                                                             *
 *   match is the custom match program that plays program A vs program B.  The *
 *   command line looks like this:                                             *
 *                                                                             *
 *     match <PGNname> <ngames> <fen> <mins> <inc> <program1> <host1> <args>   *
 *           <program2> <host2> <args>                                         *
 *                                                                             *
 *******************************************************************************
*/

int main(int argc, char *argv[])
{
  FILE *testfile;
  char *filename, *tm;
  char p1[128], p2[128];
  int bytes, position_number;
  int game, who_goes_first = 0;
  int i, numreads;

/*
  ----------------------------------------------------------
 |                                                          |
 |  set up to catch a SIGCHLD so that when a program  exits |
 |  we will will catch it and reset things correctly so we  |
 |  don't end up up to our ears in zombies.                 |
 |                                                          |
  ----------------------------------------------------------
*/
  signal(SIGCHLD, ProgramExited);
  signal(SIGPIPE, SIG_IGN);
/*
  ----------------------------------------------------------
 |                                                          |
 |  the first step is to parse the command line arguments   |
 |  to get ready to start the match.                        |
 |                                                          |
  ----------------------------------------------------------
*/
  i = 1;
  PGNname = argv[1];
  filename = argv[2];
  position_number = atoi(argv[3]);
  ngames = atoi(argv[4]);
  stime = atoi(argv[5]);
  inc = atoi(argv[6]);
  strcpy(p1, "/shared/home/hyatt/");
  strcat(p1, argv[7]);
  program[0] = p1;
  host[0] = argv[8];
  parg[0] = atoi(argv[9]);
  strcpy(p2, "/shared/home/hyatt/");
  strcat(p2, argv[10]);
  program[1] = p2;
  host[1] = argv[11];
  parg[1] = atoi(argv[12]);
#if defined(DEBUG)
  printf("first program=%s  first host=%s\n", program[0], host[0]);
  printf("second program=%s  second host=%s\n", program[1], host[1]);
  printf("games=%d  initial time=%d  inc=%d\n", ngames, stime, inc);
  printf("match PGNname=%s\n", PGNname);
#endif
  testfile = fopen(filename, "r");
  for (i = 1; i <= position_number; i++)
    fgets(position, 256, testfile);
#if defined(DEBUG)
  printf("filename=%s\n", filename);
  printf("position=\"%s\"\n", position);
#endif
  tm = strchr(position, ' ') + 1;
  if (*tm == 'w')
    tomove = 1;
  else
    tomove = 0;
/*
  ----------------------------------------------------------
 |                                                          |
 |  now repeatedly call PlayGame, alternating the player    |
 |  that gets white each game.                              |
 |                                                          |
  ----------------------------------------------------------
 */
  if (ngames < 1)
    ngames = 1;
  for (game = 1; game <= ngames; game++) {
    who_goes_first ^= 1;
    sprintf(plog[0], "log=%d", parg[0] + game - 1);
    sprintf(plog[1], "log=%d", parg[1] + game - 1);
    PlayGame(who_goes_first);
    if (tomove == who_goes_first)
      SavePGN(game, name[1], name[0], result);
    else
      SavePGN(game, name[0], name[1], result);
  }
#if defined(DEBUG)
  printf("normal exit\n");
#endif
}
