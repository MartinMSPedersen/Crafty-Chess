#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
int main(int argc, char *argv[])
{
  FILE *input, *output;
  char node[256][32], command[65536][512];
  int pids[256], status, busy;
  int i, procid, num_hosts = 0, num_commands = 0, next_command = 0, which;
  char buffer[512], *bytes, temp[512], execute[512], *hostid, *begin;

/*
  initialize
*/
  for (i = 0; i < 256; i++)
    pids[i] = -1;
  output = fopen("cluster.log", "w+");
/*
  first, let's read in the list of hosts we are supposed to use.
*/
  input = fopen("hosts", "r");
  while (1) {
    bytes = fgets(node[num_hosts], 512, input);
    if (!bytes)
      break;
    *strchr(node[num_hosts++], '\n') = 0;
  }
  fclose(input);
/*
  now, let's read in the commands we need to execute.
*/

  input = fopen("commands", "r");
  while (1) {
    bytes = fgets(command[num_commands], 512, input);
    if (!bytes)
      break;
    *strchr(command[num_commands++], '\n') = 0;
  }
  fprintf(output, "found %d hosts,  %d commands to execute\n", num_hosts,
      num_commands);
/*
  fclose(input);
  for (i=0;i<num_hosts;i++)
    printf("host[%d]=%s\n", i, hostname[i]);
  for (i=0;i<num_commands;i++)
    printf("command[%d]=%s\n", i, command[i]);
*/

/*
  now we cycle thru the list of nodes and fork a process for
  each idle node.  Once all nodes are busy, we simply wait until
  one terminates, and then start another command on that node.
  we repeat this until all commands have been completed.
*/
  while (1) {
    fflush(output);
    if (next_command < num_commands) {
      for (which = 0; which < num_hosts; which++) {
        if (pids[which] < 0) {
          if ((procid = fork()) < 0) {
            fprintf(output, "Error:  Failed to create child.\n");
          } else if (procid == 0) {
/*
  OK, we are the child.  Parse the command and replace every "%%"
  with the name of the node we want this command to use.
*/
            strcpy(temp, command[next_command]);
            begin = temp;
            strcpy(execute, "");
            while (1) {
              hostid = strstr(begin, "%%");
              if (!hostid)
                break;
              *hostid = 0;
              strcpy(execute + strlen(execute), begin);
              strcpy(execute + strlen(execute), node[which]);
              begin = hostid + 2;
            }
            strcpy(execute + strlen(execute), begin);
            system(execute);
            exit(0);
          }
          pids[which] = procid;
          fprintf(output, "starting command %d on host %s\n", next_command,
              node[which]);
          fflush(output);
          if (++next_command >= num_commands)
            break;
        }
      }
    }
    procid = wait(&status);
    for (which = 0; which < num_hosts; which++)
      if (pids[which] == procid) {
        pids[which] = -1;
        break;
      }
    busy = 0;
    for (i = 0; i < num_hosts; i++)
      if (pids[i] != -1) 
        busy++;
    if (busy == 0)
      break;
    fprintf(output, "node=%s  pid=%d terminated, %d remain.\n", node[which], procid, busy);
  }
  fprintf(output, "all commands executed, terminating...\n");
  exit(0);
}
