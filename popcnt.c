#define BITBOARD long long
int PopCnt(register BITBOARD a) {
  register int c=0;

  while(a) {
    c++;
    a &= a - 1;
  }
  return(c);
}
