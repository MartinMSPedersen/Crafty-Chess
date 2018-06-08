#include <stdio.h>
#include <string.h>
main(int argc, char* argv[]) {
  char rec[512], *badc, *eof;
  int lines=0, removed=0;
  FILE *in, *out;
  in=fopen(argv[1],"r");
  out=fopen(argv[2],"w");
  while (1) {
    eof=fgets(rec,128,in);
    if (eof == 0) break;
    lines++;
    if ((badc=strchr(rec,13))) {
      removed++;
      printf("fixing line %d=%s",lines,rec);
      strcpy(badc,badc+1);
    }
    fprintf(out,"%s",rec);
  }
  printf("%d lines read\n",lines);
  printf("%d lines corrected\n",removed);
  fclose(in);
  fclose(out);
}
