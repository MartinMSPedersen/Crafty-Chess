#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(int argc, char *argv[]) {
  FILE *output;
  int bytes, written;
  char buffer=1;
  
  output=fopen(argv[1], "w+");
  bytes=atoi(argv[2]);
  printf("creating %s (%d 1K blocks)\n", argv[1], bytes);
  bytes=bytes*1024;
  for (written=0; written<bytes; written++) {
    fwrite(&buffer, 1, 1, output);
  }
  fclose(output);
}
