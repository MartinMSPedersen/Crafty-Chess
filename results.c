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
      do {
        data = fgets(buffer, 256, file);
        if (!data) break;
      } while (!strstr(buffer, "Result"));
      if (!data) break;
      if (strstr(buffer, "1-0")) {
        if (color == 1)
          twon++;
        else
          tlost++;
      }
      else if (strstr(buffer, "0-1")) {
        if (color == 1)
          tlost++;
        else
          twon++;
      }
      else
        tdrawn++;
      color ^= 1;
    }
  }
  printf("%2d       %2d       %2d\n", twon, tdrawn, tlost);
}
