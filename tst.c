#define BITBOARD "long long"
#include "inlinex86.h"
main () {
  long long v;
  v=1;
  printf("FirstOne=%d\n",FirstOne(v));
  printf("LastOne=%d\n",LastOne(v));
  v=v<<63;
  printf("FirstOne=%d\n",FirstOne(v));
  printf("LastOne=%d\n",LastOne(v));
  v=v<<63;
  printf("FirstOne=%d\n",FirstOne(v));
  printf("LastOne=%d\n",LastOne(v));
  v=3;
  printf("FirstOne=%d\n",FirstOne(v));
  printf("LastOne=%d\n",LastOne(v));
}
