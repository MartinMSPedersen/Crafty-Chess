#include <stdio.h>
#include <string.h>
int main() {
  char color[16], buffer[256], *data=(void*) -1, *pcolor;
  int nwon, nlost, ndrawn;
  int twon=0, tlost=0, tdrawn=0;
  int presult, position=0, blown=0;
  while (1) {
    position++;
    nwon=0;
    nlost=0;
    ndrawn=0;
    data = fgets(buffer, 256, stdin);
    if (!data) break;
    if (strstr(buffer, "1-0")) {
       nwon++;
       twon++;
    }
    else if (strstr(buffer, "0-1")) {
      nlost++;
      tlost++;
    }
    else {
      ndrawn++;
      tdrawn++;
    }
    data = fgets(buffer, 256, stdin);
    if (!data) break;
    if (strstr(buffer, "0-1")) {
      nwon++;
      twon++;
    }
    else if (strstr(buffer, "1-0")) {
      nlost++;
      tlost++;
    }
    else {
      ndrawn++;
      tdrawn++;
    }
    if (nwon >= nlost) continue;
      blown++;
      printf("[%2d] blown position %2d     New   won  %d  drew  %d  lost  %d\n", blown, position, nwon, ndrawn, nlost);
  }
  printf("                 total               %2d       %2d       %2d\n", twon, tdrawn, tlost);
}
