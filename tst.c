#include "inlinex86.h"
main() {
  unsigned long long v = (unsigned long long) 0x0f00ff00ff0000fe;

  printf("v=%llx\n", v);
  printf("first=%d\n", FirstOne(v));
  printf("last=%d\n", LastOne(v));
}
