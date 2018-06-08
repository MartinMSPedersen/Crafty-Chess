#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
char status[65536], command[65536][512], submit[512];
int early_stop = 0;

void SignalInterrupt(int sigtype) {
  signal(SIGINT,SignalInterrupt);
  printf("caught a signal <%d>\n",sigtype);
  if (sigtype == SIGHUP) {
    system("./abortall");
    system("./cleanup");
    exit(EXIT_FAILURE);
  }
  else
    early_stop = 1;
}

int main(int argc, char *argv[])
{
  FILE *input, *output;
  int i, num_commands = 0, next_command = 0, min_cpus, max_cpus, max_jobs, busy, others;
  char *bytes;
  time_t secs;

/*
  initialize
*/
  signal(SIGHUP,SignalInterrupt);
  output = fopen("autosub.log", "w+");
  min_cpus = atoi(argv[1]);
  max_cpus = atoi(argv[2]);
/*
  now, let's read in the commands we need to execute.
*/

  system("ls run/run.* > commands");
  input = fopen("commands", "r");
  while (1) {
    bytes = fgets(command[num_commands], 512, input);
    if (!bytes)
      break;
    *strchr(command[num_commands++], '\n') = 0;
  }
  fclose(input);
/*
  now we check every 15 seconds to see if anything has terminated,
  and if so, we qsub the next command.
*/
  for (i=0; i<num_commands; i++) {
    status[i] = 0;
    fseek(output, i*72, SEEK_SET);
    fwrite("                                                                       \n", 1, 72, output);
  }
  next_command = -1;
  while (1) {
    char buff[128];
    time(&secs);
    system("qstat | grep hyatt | wc -l > active");
    system("qstat | grep -v hyatt | awk '{print $9}' >> active");
    input = fopen("active", "r");
    fgets(buff, 128, input);
    busy = atoi(buff);
    others = 0;
    while (!feof(input)) {
      int t;
      fgets(buff, 128, input);
      t = atoi(buff);
      others += t;
    }
    fclose(input);
    if (others >= 2) others -= 2;
    max_jobs = max_cpus - others;
    max_jobs = (max_jobs > min_cpus) ? max_jobs : min_cpus;
    if (next_command >= num_commands && busy <= 0) break;
    while (busy++ < max_jobs) {
      if (++next_command < num_commands) {
        sprintf(submit, "qsub %s", command[next_command]);
        system(submit);
        fseek(output, next_command*72, SEEK_SET);
        fwrite(command[next_command], 1, strlen(command[next_command]), output);
        fseek(output, next_command*72 + 20, SEEK_SET);
        fwrite(asctime(localtime(&secs))+4, 1, 20, output);
        fflush(output);
        status[next_command] = 1;
      }
    }
/*
    for (i=0; i<=next_command; i++) {
      int state;
      char cmd[256];

      if (status[i] == 1) {
        sprintf(cmd, "qstat -j run.%04d 2> /dev/null | wc -l > running", i+1);
        system(cmd);
        input = fopen("running", "r");
        fscanf(input,"%d", &state);
        fclose(input);
        if (state <= 2) {
          status[i] = 2;
          fseek(output, i*72 + 44, SEEK_SET);
          fwrite(asctime(localtime(&secs))+4, 1, 20, output);
          fflush(output);
        }
      }
    }
    for (i=0; i<num_commands; i++)
      if (status[i] != 2) break;
    if (i >= num_commands) break;
*/
    sleep(10);
  }
  if (!early_stop)
    exit(EXIT_SUCCESS);
  else
    exit(EXIT_FAILURE);
}
