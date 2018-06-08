
// http://supertech.csail.mit.edu/papers/debruijn.pdf
const BitBoard magic = 0x03f79d71b4cb0a89;

const unsigned int magictable[64] =
{
    0,  1, 48,  2, 57, 49, 28,  3,
   61, 58, 50, 42, 38, 29, 17,  4,
   62, 55, 59, 36, 53, 51, 43, 22,
   45, 39, 33, 30, 24, 18, 12,  5,
   63, 47, 56, 27, 60, 41, 37, 16,
   54, 35, 52, 21, 44, 32, 23, 11,
   46, 26, 40, 15, 34, 20, 31, 10,
   25, 14, 19,  9, 13,  8,  7,  6,
};

int bitScanForward (BitBoard bb) {
  assert (bb != 0);
  return magictable[((bb&-bb)*magic) >> 58];
}

// Matt Taylor's folded bitscan
int bitScanForward(BitBoard bb)
{
  assert (bb != 0);
  bb ^= (bb - 1);
  unsigned int foldedLSB = ((int)bb) ^ ((int)(bb>>32));
  return lookUp[(foldedLSB * 0x78291ACF) >> (32-6)]; // range is 0..63
}


// return index 0..63 of MSB
//          -1023 if passing zero
int bitScanReverse(BitBoard bb)
{
   union {
      double d;
      struct {
         unsigned int mantissal : 32;     
         unsigned int mantissah : 20;     
         unsigned int exponent : 11;     
         unsigned int sign : 1;     
      };
   } ud;
   ud.d = (double)(__int64)(bb & ~(bb >> 32));
   int idx = (ud.exponent - 1023) | (63*ud.sign);
   return idx;
}


