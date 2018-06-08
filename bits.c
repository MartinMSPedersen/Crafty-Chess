///////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <time.h>

///////////////////////////////////////////////////////////////////////////////

typedef unsigned long long Bitboard;

///////////////////////////////////////////////////////////////////////////////

// Each bitscan will run max*64 times

const int max = 10000000;

////////////////////////////////////////////////////////////////////////////////

const Bitboard mask[64] = {
0x0000000000000001ULL, 0x0000000000000002ULL,
0x0000000000000004ULL, 0x0000000000000008ULL,
0x0000000000000010ULL, 0x0000000000000020ULL,
0x0000000000000040ULL, 0x0000000000000080ULL,
0x0000000000000100ULL, 0x0000000000000200ULL,
0x0000000000000400ULL, 0x0000000000000800ULL,
0x0000000000001000ULL, 0x0000000000002000ULL,
0x0000000000004000ULL, 0x0000000000008000ULL,
0x0000000000010000ULL, 0x0000000000020000ULL,
0x0000000000040000ULL, 0x0000000000080000ULL,
0x0000000000100000ULL, 0x0000000000200000ULL,
0x0000000000400000ULL, 0x0000000000800000ULL,
0x0000000001000000ULL, 0x0000000002000000ULL,
0x0000000004000000ULL, 0x0000000008000000ULL,
0x0000000010000000ULL, 0x0000000020000000ULL,
0x0000000040000000ULL, 0x0000000080000000ULL,
0x0000000100000000ULL, 0x0000000200000000ULL,
0x0000000400000000ULL, 0x0000000800000000ULL,
0x0000001000000000ULL, 0x0000002000000000ULL,
0x0000004000000000ULL, 0x0000008000000000ULL,
0x0000010000000000ULL, 0x0000020000000000ULL,
0x0000040000000000ULL, 0x0000080000000000ULL,
0x0000100000000000ULL, 0x0000200000000000ULL,
0x0000400000000000ULL, 0x0000800000000000ULL,
0x0001000000000000ULL, 0x0002000000000000ULL,
0x0004000000000000ULL, 0x0008000000000000ULL,
0x0010000000000000ULL, 0x0020000000000000ULL,
0x0040000000000000ULL, 0x0080000000000000ULL,
0x0100000000000000ULL, 0x0200000000000000ULL,
0x0400000000000000ULL, 0x0800000000000000ULL,
0x1000000000000000ULL, 0x2000000000000000ULL,
0x4000000000000000ULL, 0x8000000000000000ULL,
};


///////////////////////////////////////////////////////////////////////////////

int Index (int x, int y) {
	return (y << 3) + x;
}

///////////////////////////////////////////////////////////////////////////////

const Bitboard magic = 0x03f08c5392f756cdULL;

const int table[64] = {
	 0,  1, 12,  2, 13, 22, 17,  3,
	14, 33, 23, 36, 18, 58, 28,  4,
	62, 15, 34, 26, 24, 48, 50, 37,
	19, 55, 59, 52, 29, 44, 39,  5,
	63, 11, 21, 16, 32, 35, 57, 27,
	61, 25, 47, 49, 54, 51, 43, 38,
	10, 20, 31, 56, 60, 46, 53, 42,
	 9, 30, 45, 41,  8, 40,  7,  6,
};

int MagicBitscan (const Bitboard b) {
	const Bitboard lsb = b & -b;
	//bb ^= lsb;
	return table[(lsb * magic) >> 58];
}

void TestMagicBitscan () {
	clock_t start, stop;
	int count, i;
	unsigned long total = 0;

	start = clock();
	for (count = 0; count < max; count++) {
		for (i = 0; i < 64; i++) {
			total += MagicBitscan(mask[i]);
		}
	}
	stop = clock();
	printf("%15s %10u %.3f\n", "magic bitscan", total, (float)(stop - start) / CLOCKS_PER_SEC);
}

///////////////////////////////////////////////////////////////////////////////

int table16[65536];

void InitTable16 () {
	Bitboard i;
	for (i = 0; i < 65536; i++)
		table16[i] = MagicBitscan(i);
}

int table8[256];

void InitTable8 () {
	Bitboard i;
	for (i = 0; i < 256; i++)
		table8[i] = MagicBitscan(i);
}

////////////////////////////////////////////////////////////////////////////////

int TableBitscan16 (Bitboard b) {
	static const Bitboard TwoFullRanks = 65535;
	static const Bitboard FPMask1 = (Bitboard) TwoFullRanks << 16;
    static const Bitboard FPMask2 = (Bitboard) TwoFullRanks << 32;

	if (b & TwoFullRanks)
		return table16[b & TwoFullRanks];
	if (b & FPMask1)
		return table16[(b >> 16) & TwoFullRanks] + 16;
	if (b & FPMask2)
		return table16[(b >> 32) & TwoFullRanks] + 32;
	return table16[b >> 48] + 48;
}

void TestTableBitscan16 () {
	clock_t start, stop;
	int count, i;
	unsigned long total = 0;

	start = clock();
	for (count = 0; count < max; count++) {
		for (i = 0; i < 64; i++) {
			total += TableBitscan16(mask[i]);
		}
	}
	stop = clock();
	printf("%15s %10u %.3f\n", "table 16", total, (float)(stop - start) / CLOCKS_PER_SEC);
}
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////

const int lsz64_tbl[64] =
{
  0,31, 4,33,60,15,12,34,
 61,25,51,10,56,20,22,35,
 62,30, 3,54,52,24,42,19,
 57,29, 2,44,47,28, 1,36,
 63,32,59, 5, 6,50,55, 7,
 16,53,13,41, 8,43,46,17,
 26,58,49,14,11,40, 9,45,
 21,48,39,23,18,38,37,27,
};

int GerdBitScan (Bitboard bb)
{
	const Bitboard lsb = (bb & -(long long)bb) - 1;
	//bb ^= lsb--;
	//lsb--;
	const unsigned int foldedLSB = ((int) lsb) ^ ((int)(lsb>>32));
	return lsz64_tbl[foldedLSB * 0x78291ACF >> 26];
}

void TestGerdBitscan () {
	clock_t start, stop;
	int count, i;
	unsigned long total = 0;

	start = clock();
	for (count = 0; count < max; count++) {
		for (i = 0; i < 64; i++) {
			total += GerdBitScan(mask[i]);
		}
	}
	stop = clock();
	printf("%15s %10u %.3f\n", "gerd", total, (float)(stop - start) / CLOCKS_PER_SEC);
}

///////////////////////////////////////////////////////////////////////////////

int EugeneBitscan (Bitboard arg) {
	int result = 0;

	if (arg > 0xFFFFFFFF) {
	arg >>= 32;
	result = 32;
	}

	if (arg > 0xFFFF) {
	arg >>= 16;
	result += 16;
	}

	if (arg > 0xFF) {
	arg >>= 8;
	result += 8;
	}

	return result + table8[arg];
}

void TestEugeneBitscan () {
	clock_t start, stop;
	int count, i;
	unsigned long total = 0;

	start = clock();
	for (count = 0; count < max; count++) {
		for (i = 0; i < 64; i++) {
			total += EugeneBitscan(mask[i]);
		}
	}
	stop = clock();
	printf("%15s %10u %.3f\n", "eugene", total, (float)(stop - start) / CLOCKS_PER_SEC);
}

///////////////////////////////////////////////////////////////////////////////

int EugeneBitscan2 (Bitboard arg) {
	int result = 0;

	if (arg > 0xFFFFFFFF) {
	arg >>= 32;
	result = 32;
	}

	if (arg > 0xFFFF) {
	arg >>= 16;
	result += 16;
	}

	return result + table16[arg];
}

void TestEugeneBitscan2 () {
	clock_t start, stop;
	int count, i;
	unsigned long total = 0;

	start = clock();
	for (count = 0; count < max; count++) {
		for (i = 0; i < 64; i++) {
			total += EugeneBitscan2(mask[i]);
		}
	}
	stop = clock();
	printf("%15s %10u %.3f\n", "eugene2", total, (float)(stop - start) / CLOCKS_PER_SEC);
}

////////////////////////////////////////////////////////////////////////////////

int main () {

	InitTable16();
	InitTable8();

	TestMagicBitscan();
	TestMagicBitscan();
	TestMagicBitscan();
	TestMagicBitscan();
	printf("  ------------------------------\n");

	TestTableBitscan16();
	TestTableBitscan16();
	TestTableBitscan16();
	TestTableBitscan16();
	printf("  ------------------------------\n");

	TestGerdBitscan();
	TestGerdBitscan();
	TestGerdBitscan();
	TestGerdBitscan();
	printf("  ------------------------------\n");

	TestEugeneBitscan();
	TestEugeneBitscan();
	TestEugeneBitscan();
	TestEugeneBitscan();
	printf("  ------------------------------\n");

	TestEugeneBitscan2();
	TestEugeneBitscan2();
	TestEugeneBitscan2();
	TestEugeneBitscan2();

	return 0;
}
