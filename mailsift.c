#include <stdio.h>
#include <string.h>
int main(int argc, char *argv[]) {
  FILE *input, *output;
  char buffer[512], *bytes;
  int arg, skip=0, where=0, deleted;
  
  for (arg=1; arg< argc;arg++) {
    deleted=0;
    input=fopen(argv[arg],"r");
    output=fopen("mail.temp","w+");
    bytes=(char *) 1;
    while (bytes) {
      bytes=fgets(buffer,512,input);
      if (!bytes) break;
/*
    look for raw envelope header "From ".  This marks the beginning
    of a message.  Remember where we are in the output file in case
    we want to back up and dump all of this message and start over.
*/
      if (strncmp(buffer,"From ",5) == 0) {
        skip=0;
        where=ftell(output);
      }
/*
    look for a line with "majordom" in it.  
    reset the output file to point to the start of this message, and
    we then just skip the rest of the message up to the next From
    line.
*/
      if (strstr(buffer,"majordom") || strstr(buffer,"Majordom")) {
        skip=1;
        fseek(output,where,SEEK_SET);
        deleted++;
      }
      if (!skip) fputs(buffer,output);
    }
/*
    done.  Close the input/output files, then rename mail.temp to the
    original filename.
*/
    fclose(input);
    fclose(output);
    printf("user %s  deleted %d messages\n",argv[arg],deleted);
  }
}
