#include <stdio.h>
#include <string.h>
int main(int argc, char *argv[]) {
  FILE *file;
  char buffer[256], *data=(void*) -1;
  int twon=0, tlost=0, tdrawn=0;
  int color, arg = 0;

  while (++arg < argc) {
    file = fopen(argv[arg], "r");
    color = 1;
    while (1) {
      int fail=0;
      data = fgets(buffer, 256, file);
      if (!data) break;
      if (strstr(buffer, "+1")) fail++;
      else fail=0;
      if (fail > 1) printf("problem in file %s\n", argv[arg]);
    }
  }
}
