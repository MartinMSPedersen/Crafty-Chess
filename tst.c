#include <stdio.h>
void main() {
  int i;
  char c;
  for (i=0;i<256;i++) {
    c = i;
    printf("%03d=%x\n", i, c);
  }
}
