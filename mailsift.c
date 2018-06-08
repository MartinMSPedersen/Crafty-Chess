#include <stdio.h>
#include <string.h>
int main(int argc, char *argv[]) {
  FILE *input, *output;
  char buffer[512], *bytes;
  int arg, messno = 0;
  char filename[256];
  
  output=0;
  for (arg=1; arg< argc;arg++) {
    input=fopen(argv[arg],"r");
    bytes=(char *) 1;
    while (bytes) {
      bytes=fgets(buffer,512,input);
      if (!bytes) break;
/*
    look for raw envelope header "From ".  This marks the beginning
    of a message.  Remember where we are in the output file in case
    we want to back up and dump all of this message and start over.
*/
      if (strncmp(buffer,"From - ",7) == 0) {
        printf("saving message %d\n", ++messno);
        if (output)
          fclose(output);
        sprintf(filename, "janie/mail.%04d", messno);
        output=fopen(filename,"w+");
        continue;
      }
      fputs(buffer,output);
    }
    printf("saving message %d\n", ++messno);
/*
    done.  Close the input/output files, then rename mail.temp to the
    original filename.
*/
    fclose(input);
    fclose(output);
  }
}
