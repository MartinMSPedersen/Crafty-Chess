#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#if defined(_WIN32)
#include <windows.h>
#endif

#define NEW
#define T41_INCLUDE
#define T42_INCLUDE
#define T33_INCLUDE
#define T_INDEX64
#define  XX  127

#if defined (T_INDEX64) && defined (_MSC_VER)
typedef unsigned __int64 INDEX;
#elif defined (T_INDEX64)
typedef unsigned long long INDEX;
#else
typedef unsigned long INDEX;
#endif

typedef unsigned int square;

typedef int color;

#define  x_colorWhite    0
#define  x_colorBlack    1
#define  x_colorNeutral  2

typedef int piece;

#define  x_pieceNone     0
#define  x_piecePawn     1
#define  x_pieceKnight   2
#define  x_pieceBishop   3
#define  x_pieceRook     4
#define  x_pieceQueen    5
#define  x_pieceKing     6

/*
   Macro that fetches positions of pieces
 */

#define  C_PIECES  3    /* Maximum # of pieces of one color OTB */

#define  SqFindKing(psq)       (psq[C_PIECES*(x_pieceKing-1)])
#define  SqFindOne(psq, pi)    (psq[C_PIECES*(pi-1)])
#define  SqFindFirst(psq, pi)  (psq[C_PIECES*(pi-1)])
#define  SqFindSecond(psq, pi) (psq[C_PIECES*(pi-1)+1])
#define  SqFindThird(psq, pi)  (psq[C_PIECES*(pi-1)+2])

#include "lock.h"

/*
   All defined, now include probing code
 */

/* -------------------------------------------------------------------- */
/*                                                                      */
/*              Probe chess endgame database ("tablebase")              */
/*                                                                      */
/*               Copyright (c) 1998--2001 Eugene Nalimov                */
/*                                                                      */
/* The code listed below should not be used in any product (software or */
/* hardware,  commercial or not,  and so on) without written permission */
/* from the author.                                                     */
/*                                                                      */
/* -------------------------------------------------------------------- */

#if defined (_WIN32) || defined(_WIN64)
#  include <windows.h>
#endif

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#if !defined (DEBUG) && !defined(NDEBUG)
#  define   NDEBUG
#endif
#include <assert.h>

static	lock_t	lockLRU;

// Declarations

typedef unsigned    char BYTE;
typedef unsigned    long ULONG;
typedef signed      char tb_t;

#if !defined (COLOR_DECLARED)
typedef int color;
#  define   x_colorWhite    0
#  define   x_colorBlack    1
#  define   x_colorNeutral  2
#  define COLOR_DECLARED
#endif

#if !defined (PIECES_DECLARED)
typedef int piece;
#  define   x_pieceNone     0
#  define   x_piecePawn     1
#  define   x_pieceKnight   2
#  define   x_pieceBishop   3
#  define   x_pieceRook     4
#  define   x_pieceQueen    5
#  define   x_pieceKing     6
#  define PIECES_DECLARED
#endif

#if !defined (SqFind2)
#  define   SqFind2(psq,pi1,sq1,pi2,sq2)    sq1=SqFindFirst(psq,pi1);sq2=SqFindFirst(psq,pi2);
#endif

// Machine and compiler-specific declarations

#if defined (_MSC_VER)

#  undef    TB_CDECL
#  define   TB_CDECL    __cdecl
#  define   TB_FASTCALL __fastcall
#  if _MSC_VER >= 1200
#    define INLINE      __forceinline
#  endif

#else

#  define   TB_CDECL
#  define   TB_FASTCALL

#endif

#if !defined (INLINE)
#  define   INLINE  inline
#endif

// Printf formats

#if defined (T_INDEX64) && defined (_MSC_VER)
#   define HEX_INDEX_FORMAT "%016I64X"
#   define DEC_INDEX_FORMAT "%I64u"
#elif defined (T_INDEX64)
#   define HEX_INDEX_FORMAT "%016llX"
#   define DEC_INDEX_FORMAT "%llu"
#else
#   define HEX_INDEX_FORMAT "%08X"
#   define DEC_INDEX_FORMAT "%lu"
#endif

// Directory delimiter

#if defined (_WIN32) || defined(_WIN64)
#   define DELIMITER   "\\"
#elif defined (__MWERKS__)
#   define DELIMITER   ":"
#else
#   define DELIMITER   "/"
#endif


// Some constants from SJE program

#define pageL 256

/* tablebase byte entry semispan length */

#define tbbe_ssL ((pageL - 4) / 2)

/* tablebase signed byte entry values */

#define bev_broken  (tbbe_ssL + 1)  /* illegal or busted */

#define bev_mi1     tbbe_ssL        /* mate in 1 move */
#define bev_mimin   1               /* mate in 126 moves */

#define bev_draw    0               /* draw */

#define bev_limax   (-1)            /* mated in 125 moves */
#define bev_li0     (-tbbe_ssL)     /* mated in 0 moves */

#define bev_limaxx  (-tbbe_ssL - 1) /* mated in 126 moves */
#define bev_miminx  (-tbbe_ssL - 2) /* mate in 127 moves */

// Some constants for 16-bit tables

#define L_pageL 65536

/* tablebase short entry semispan length */

#define L_tbbe_ssL ((L_pageL - 4) / 2)

/* tablebase signed short entry values */

#define L_bev_broken  (L_tbbe_ssL + 1)      /* illegal or busted */

#define L_bev_mi1     L_tbbe_ssL            /* mate in 1 move */
#define L_bev_mimin   1                     /* mate in 32766 moves */

#define L_bev_draw    0                     /* draw */

#define L_bev_limax   (-1)                  /* mated in 32765 moves */
#define L_bev_li0     (-L_tbbe_ssL)         /* mated in 0 moves */

#define L_bev_limaxx    (-L_tbbe_ssL - 1)   /* mated in 32766 moves */
#define L_bev_miminx    (-L_tbbe_ssL - 2)   /* mate in 32767 moves */

// Convertion from 8-bit to 16-bit score
// UNDONE: Maybe implement via lookup table?

#define S_to_L(tbt)\
    (\
     (0 == tbt) ? 0:\
     (tbt > 0) ? (bev_broken != tbt ? tbt + 32640 : L_bev_broken):\
     (tbt >= bev_li0) ? tbt - 32640:\
     (bev_limaxx == tbt) ? -32640:\
     /*bev_miminx == tbt*/  32640\
    )

// Constants

#define i8  ((unsigned) 8)
#define i14 ((unsigned) 14)
#define i42 ((unsigned) 42)
#define i43 ((unsigned) 43)
#define i44 ((unsigned) 44)
#define i45 ((unsigned) 45)
#define i46 ((unsigned) 46)
#define i47 ((unsigned) 47)
#define i48 ((unsigned) 48)
#define i57 ((unsigned) 57)
#define i58 ((unsigned) 58)
#define i59 ((unsigned) 59)
#define i60 ((unsigned) 60)
#define i61 ((unsigned) 61)
#define i62 ((unsigned) 62)
#define i63 ((unsigned) 63)
#define i64 ((unsigned) 64)

#define x_row_1 0
#define x_row_2 1
#define x_row_3 2
#define x_row_4 3
#define x_row_5 4
#define x_row_6 5
#define x_row_7 6
#define x_row_8 7

#define x_column_a 0
#define x_column_b 1
#define x_column_c 2
#define x_column_d 3
#define x_column_e 4
#define x_column_f 5
#define x_column_g 6
#define x_column_h 7

/* reflection macros */

#define reflect_x(sq) ((sq) ^ 0x38)
#define reflect_y(sq) ((sq) ^ 0x07)
#define reflect_xy(sq) rgsqReflectXY[sq]

static const square rgsqReflectXY [] =
{
  0,  8, 16, 24, 32, 40, 48, 56,
  1,  9, 17, 25, 33, 41, 49, 57,
  2, 10, 18, 26, 34, 42, 50, 58,
  3, 11, 19, 27, 35, 43, 51, 59,
  4, 12, 20, 28, 36, 44, 52, 60,
  5, 13, 21, 29, 37, 45, 53, 61,
  6, 14, 22, 30, 38, 46, 54, 62,
  7, 15, 23, 31, 39, 47, 55, 63,
};

static const square rgsqReflectMaskY [] =
{
 0, 0, 0, 0, 7, 7, 7, 7,
 0, 0, 0, 0, 7, 7, 7, 7,
 0, 0, 0, 0, 7, 7, 7, 7,
 0, 0, 0, 0, 7, 7, 7, 7,
 0, 0, 0, 0, 7, 7, 7, 7,
 0, 0, 0, 0, 7, 7, 7, 7,
 0, 0, 0, 0, 7, 7, 7, 7,
 0, 0, 0, 0, 7, 7, 7, 7,
};

static const square rgsqReflectMaskYandX [] =
{
 0, 0, 0, 0, 7, 7, 7, 7,
 0, 0, 0, 0, 7, 7, 7, 7,
 0, 0, 0, 0, 7, 7, 7, 7,
 0, 0, 0, 0, 7, 7, 7, 7,
 0x38, 0x38, 0x38, 0x38, 0x38+7, 0x38+7, 0x38+7, 0x38+7,
 0x38, 0x38, 0x38, 0x38, 0x38+7, 0x38+7, 0x38+7, 0x38+7,
 0x38, 0x38, 0x38, 0x38, 0x38+7, 0x38+7, 0x38+7, 0x38+7,
 0x38, 0x38, 0x38, 0x38, 0x38+7, 0x38+7, 0x38+7, 0x38+7,
};

static const square rgsqReflectInvertMask[] = { 0, 0x38 };

/* useful macros */

#define TbRow(sq)               ((sq) >> 3)
#define TbColumn(sq)            ((sq) & 7)

#if defined (NEW)
#  define   PchExt(side) ((x_colorWhite == side) ? ".nbw" : ".nbb")
#else
#  define   PchExt(side) ((x_colorWhite == side) ? ".tbw" : ".tbb")
#endif

// Verbose levels

static bool     fPrint = false;     // Print some technical statistics
static bool     fVerbose = false;   // Print additional information

// Malloc that checks for out-of-memory condition

static size_t   cbAllocated;
static int      cOpenFilesAttempts;
static int      cOpenFilesSuceed;

static void* PvMalloc
    (
    size_t cb
    )
    {
    void    *pv;

    pv = malloc (cb);
    if (NULL == pv)
        {
        printf ("*** Cannot allocate %d bytes of memory\n", cb);
        exit (1);
        }
    cbAllocated += cb;
    return pv;
    }

#if defined (NEW)   // New index schema ----------------------------------------

// 'Invalid' value have to be large, so index
// of invalid position will be very large, too.

#define INF 4000

// Enumeration: valid positions with 2 kings on board; white king restricted to
// a1-d1-d4 triangle; also, if it's at a1-d4 half-diagonal, then black king
// must be in a1-h1-h8 triangle

static const short rgsTriKings [64 * 64] =
{
 INF, INF,   0,   1,   2,   3,   4,   5, INF, INF,   6,   7,   8,   9,  10,  11,
 INF, INF,  12,  13,  14,  15,  16,  17, INF, INF, INF,  18,  19,  20,  21,  22,
 INF, INF, INF, INF,  23,  24,  25,  26, INF, INF, INF, INF, INF,  27,  28,  29,
 INF, INF, INF, INF, INF, INF,  30,  31, INF, INF, INF, INF, INF, INF, INF,  32,
 INF, INF, INF,  33,  34,  35,  36,  37, INF, INF, INF,  38,  39,  40,  41,  42,
  43,  44,  45,  46,  47,  48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,
  59,  60,  61,  62,  63,  64,  65,  66,  67,  68,  69,  70,  71,  72,  73,  74,
  75,  76,  77,  78,  79,  80,  81,  82,  83,  84,  85,  86,  87,  88,  89,  90,
  91, INF, INF, INF,  92,  93,  94,  95,  96, INF, INF, INF,  97,  98,  99, 100,
 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116,
 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132,
 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148,
 149, 150, INF, INF, INF, 151, 152, 153, 154, 155, INF, INF, INF, 156, 157, 158,
 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174,
 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190,
 191, 192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, 207, 208, 209, 210, 211, INF, INF, INF, 212, 213, 214, 215, 216,
 INF, INF, INF, 217, 218, 219, 220, 221, INF, INF, INF, 222, 223, 224, 225, 226,
 INF, INF, INF, INF, 227, 228, 229, 230, INF, INF, INF, INF, INF, 231, 232, 233,
 INF, INF, INF, INF, INF, INF, 234, 235, INF, INF, INF, INF, INF, INF, INF, 236,
 237, INF, INF, INF, 238, 239, 240, 241, 242, INF, INF, INF, 243, 244, 245, 246,
 247, INF, INF, INF, 248, 249, 250, 251, 252, 253, 254, 255, 256, 257, 258, 259,
 260, 261, 262, 263, 264, 265, 266, 267, 268, 269, 270, 271, 272, 273, 274, 275,
 276, 277, 278, 279, 280, 281, 282, 283, 284, 285, 286, 287, 288, 289, 290, 291,
 292, 293, INF, INF, INF, 294, 295, 296, 297, 298, INF, INF, INF, 299, 300, 301,
 302, 303, INF, INF, INF, 304, 305, 306, 307, 308, 309, 310, 311, 312, 313, 314,
 315, 316, 317, 318, 319, 320, 321, 322, 323, 324, 325, 326, 327, 328, 329, 330,
 331, 332, 333, 334, 335, 336, 337, 338, 339, 340, 341, 342, 343, 344, 345, 346,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 347, 348, 349, 350, 351, 352, 353, 354, INF, INF, INF, INF, 355, 356, 357, 358,
 INF, INF, INF, INF, 359, 360, 361, 362, INF, INF, INF, INF, 363, 364, 365, 366,
 INF, INF, INF, INF, 367, 368, 369, 370, INF, INF, INF, INF, INF, 371, 372, 373,
 INF, INF, INF, INF, INF, INF, 374, 375, INF, INF, INF, INF, INF, INF, INF, 376,
 377, 378, 379, 380, 381, 382, 383, 384, 385, 386, INF, INF, INF, 387, 388, 389,
 390, 391, INF, INF, INF, 392, 393, 394, 395, 396, INF, INF, INF, 397, 398, 399,
 400, 401, 402, 403, 404, 405, 406, 407, 408, 409, 410, 411, 412, 413, 414, 415,
 416, 417, 418, 419, 420, 421, 422, 423, 424, 425, 426, 427, 428, 429, 430, 431,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 432, 433, 434, 435, 436, 437, 438, 439, INF, 440, 441, 442, 443, 444, 445, 446,
 INF, INF, INF, INF, INF, 447, 448, 449, INF, INF, INF, INF, INF, 450, 451, 452,
 INF, INF, INF, INF, INF, 453, 454, 455, INF, INF, INF, INF, INF, 456, 457, 458,
 INF, INF, INF, INF, INF, INF, 459, 460, INF, INF, INF, INF, INF, INF, INF, 461,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
};

// Enumeration: all valid positions with 2 kings on board when white king
// restricted to left half of the board

static const short rgsHalfKings [64 * 64] =
{
 INF, INF,   0,   1,   2,   3,   4,   5, INF, INF,   6,   7,   8,   9,  10,  11,
  12,  13,  14,  15,  16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,
  28,  29,  30,  31,  32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,
  44,  45,  46,  47,  48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,
 INF, INF, INF,  60,  61,  62,  63,  64, INF, INF, INF,  65,  66,  67,  68,  69,
  70,  71,  72,  73,  74,  75,  76,  77,  78,  79,  80,  81,  82,  83,  84,  85,
  86,  87,  88,  89,  90,  91,  92,  93,  94,  95,  96,  97,  98,  99, 100, 101,
 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117,
 118, INF, INF, INF, 119, 120, 121, 122, 123, INF, INF, INF, 124, 125, 126, 127,
 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143,
 144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159,
 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175,
 176, 177, INF, INF, INF, 178, 179, 180, 181, 182, INF, INF, INF, 183, 184, 185,
 186, 187, 188, 189, 190, 191, 192, 193, 194, 195, 196, 197, 198, 199, 200, 201,
 202, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 214, 215, 216, 217,
 218, 219, 220, 221, 222, 223, 224, 225, 226, 227, 228, 229, 230, 231, 232, 233,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, 234, 235, 236, 237, 238, 239, INF, INF, 240, 241, 242, 243, 244, 245,
 INF, INF, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255, 256, 257, 258, 259,
 260, 261, 262, 263, 264, 265, 266, 267, 268, 269, 270, 271, 272, 273, 274, 275,
 276, 277, 278, 279, 280, 281, 282, 283, 284, 285, 286, 287, 288, 289, 290, 291,
 INF, INF, INF, 292, 293, 294, 295, 296, INF, INF, INF, 297, 298, 299, 300, 301,
 INF, INF, INF, 302, 303, 304, 305, 306, 307, 308, 309, 310, 311, 312, 313, 314,
 315, 316, 317, 318, 319, 320, 321, 322, 323, 324, 325, 326, 327, 328, 329, 330,
 331, 332, 333, 334, 335, 336, 337, 338, 339, 340, 341, 342, 343, 344, 345, 346,
 347, INF, INF, INF, 348, 349, 350, 351, 352, INF, INF, INF, 353, 354, 355, 356,
 357, INF, INF, INF, 358, 359, 360, 361, 362, 363, 364, 365, 366, 367, 368, 369,
 370, 371, 372, 373, 374, 375, 376, 377, 378, 379, 380, 381, 382, 383, 384, 385,
 386, 387, 388, 389, 390, 391, 392, 393, 394, 395, 396, 397, 398, 399, 400, 401,
 402, 403, INF, INF, INF, 404, 405, 406, 407, 408, INF, INF, INF, 409, 410, 411,
 412, 413, INF, INF, INF, 414, 415, 416, 417, 418, 419, 420, 421, 422, 423, 424,
 425, 426, 427, 428, 429, 430, 431, 432, 433, 434, 435, 436, 437, 438, 439, 440,
 441, 442, 443, 444, 445, 446, 447, 448, 449, 450, 451, 452, 453, 454, 455, 456,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 457, 458, 459, 460, 461, 462, 463, 464, INF, INF, 465, 466, 467, 468, 469, 470,
 INF, INF, 471, 472, 473, 474, 475, 476, INF, INF, 477, 478, 479, 480, 481, 482,
 483, 484, 485, 486, 487, 488, 489, 490, 491, 492, 493, 494, 495, 496, 497, 498,
 499, 500, 501, 502, 503, 504, 505, 506, 507, 508, 509, 510, 511, 512, 513, 514,
 515, 516, 517, 518, 519, 520, 521, 522, INF, INF, INF, 523, 524, 525, 526, 527,
 INF, INF, INF, 528, 529, 530, 531, 532, INF, INF, INF, 533, 534, 535, 536, 537,
 538, 539, 540, 541, 542, 543, 544, 545, 546, 547, 548, 549, 550, 551, 552, 553,
 554, 555, 556, 557, 558, 559, 560, 561, 562, 563, 564, 565, 566, 567, 568, 569,
 570, 571, 572, 573, 574, 575, 576, 577, 578, INF, INF, INF, 579, 580, 581, 582,
 583, INF, INF, INF, 584, 585, 586, 587, 588, INF, INF, INF, 589, 590, 591, 592,
 593, 594, 595, 596, 597, 598, 599, 600, 601, 602, 603, 604, 605, 606, 607, 608,
 609, 610, 611, 612, 613, 614, 615, 616, 617, 618, 619, 620, 621, 622, 623, 624,
 625, 626, 627, 628, 629, 630, 631, 632, 633, 634, INF, INF, INF, 635, 636, 637,
 638, 639, INF, INF, INF, 640, 641, 642, 643, 644, INF, INF, INF, 645, 646, 647,
 648, 649, 650, 651, 652, 653, 654, 655, 656, 657, 658, 659, 660, 661, 662, 663,
 664, 665, 666, 667, 668, 669, 670, 671, 672, 673, 674, 675, 676, 677, 678, 679,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 680, 681, 682, 683, 684, 685, 686, 687, 688, 689, 690, 691, 692, 693, 694, 695,
 INF, INF, 696, 697, 698, 699, 700, 701, INF, INF, 702, 703, 704, 705, 706, 707,
 INF, INF, 708, 709, 710, 711, 712, 713, 714, 715, 716, 717, 718, 719, 720, 721,
 722, 723, 724, 725, 726, 727, 728, 729, 730, 731, 732, 733, 734, 735, 736, 737,
 738, 739, 740, 741, 742, 743, 744, 745, 746, 747, 748, 749, 750, 751, 752, 753,
 INF, INF, INF, 754, 755, 756, 757, 758, INF, INF, INF, 759, 760, 761, 762, 763,
 INF, INF, INF, 764, 765, 766, 767, 768, 769, 770, 771, 772, 773, 774, 775, 776,
 777, 778, 779, 780, 781, 782, 783, 784, 785, 786, 787, 788, 789, 790, 791, 792,
 793, 794, 795, 796, 797, 798, 799, 800, 801, 802, 803, 804, 805, 806, 807, 808,
 809, INF, INF, INF, 810, 811, 812, 813, 814, INF, INF, INF, 815, 816, 817, 818,
 819, INF, INF, INF, 820, 821, 822, 823, 824, 825, 826, 827, 828, 829, 830, 831,
 832, 833, 834, 835, 836, 837, 838, 839, 840, 841, 842, 843, 844, 845, 846, 847,
 848, 849, 850, 851, 852, 853, 854, 855, 856, 857, 858, 859, 860, 861, 862, 863,
 864, 865, INF, INF, INF, 866, 867, 868, 869, 870, INF, INF, INF, 871, 872, 873,
 874, 875, INF, INF, INF, 876, 877, 878, 879, 880, 881, 882, 883, 884, 885, 886,
 887, 888, 889, 890, 891, 892, 893, 894, 895, 896, 897, 898, 899, 900, 901, 902,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 903, 904, 905, 906, 907, 908, 909, 910, 911, 912, 913, 914, 915, 916, 917, 918,
 919, 920, 921, 922, 923, 924, 925, 926, INF, INF, 927, 928, 929, 930, 931, 932,
 INF, INF, 933, 934, 935, 936, 937, 938, INF, INF, 939, 940, 941, 942, 943, 944,
 945, 946, 947, 948, 949, 950, 951, 952, 953, 954, 955, 956, 957, 958, 959, 960,
 961, 962, 963, 964, 965, 966, 967, 968, 969, 970, 971, 972, 973, 974, 975, 976,
 977, 978, 979, 980, 981, 982, 983, 984, INF, INF, INF, 985, 986, 987, 988, 989,
 INF, INF, INF, 990, 991, 992, 993, 994, INF, INF, INF, 995, 996, 997, 998, 999,
1000,1001,1002,1003,1004,1005,1006,1007,1008,1009,1010,1011,1012,1013,1014,1015,
1016,1017,1018,1019,1020,1021,1022,1023,1024,1025,1026,1027,1028,1029,1030,1031,
1032,1033,1034,1035,1036,1037,1038,1039,1040, INF, INF, INF,1041,1042,1043,1044,
1045, INF, INF, INF,1046,1047,1048,1049,1050, INF, INF, INF,1051,1052,1053,1054,
1055,1056,1057,1058,1059,1060,1061,1062,1063,1064,1065,1066,1067,1068,1069,1070,
1071,1072,1073,1074,1075,1076,1077,1078,1079,1080,1081,1082,1083,1084,1085,1086,
1087,1088,1089,1090,1091,1092,1093,1094,1095,1096, INF, INF, INF,1097,1098,1099,
1100,1101, INF, INF, INF,1102,1103,1104,1105,1106, INF, INF, INF,1107,1108,1109,
1110,1111,1112,1113,1114,1115,1116,1117,1118,1119,1120,1121,1122,1123,1124,1125,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
1126,1127,1128,1129,1130,1131,1132,1133,1134,1135,1136,1137,1138,1139,1140,1141,
1142,1143,1144,1145,1146,1147,1148,1149,1150,1151,1152,1153,1154,1155,1156,1157,
 INF, INF,1158,1159,1160,1161,1162,1163, INF, INF,1164,1165,1166,1167,1168,1169,
 INF, INF,1170,1171,1172,1173,1174,1175,1176,1177,1178,1179,1180,1181,1182,1183,
1184,1185,1186,1187,1188,1189,1190,1191,1192,1193,1194,1195,1196,1197,1198,1199,
1200,1201,1202,1203,1204,1205,1206,1207,1208,1209,1210,1211,1212,1213,1214,1215,
 INF, INF, INF,1216,1217,1218,1219,1220, INF, INF, INF,1221,1222,1223,1224,1225,
 INF, INF, INF,1226,1227,1228,1229,1230,1231,1232,1233,1234,1235,1236,1237,1238,
1239,1240,1241,1242,1243,1244,1245,1246,1247,1248,1249,1250,1251,1252,1253,1254,
1255,1256,1257,1258,1259,1260,1261,1262,1263,1264,1265,1266,1267,1268,1269,1270,
1271, INF, INF, INF,1272,1273,1274,1275,1276, INF, INF, INF,1277,1278,1279,1280,
1281, INF, INF, INF,1282,1283,1284,1285,1286,1287,1288,1289,1290,1291,1292,1293,
1294,1295,1296,1297,1298,1299,1300,1301,1302,1303,1304,1305,1306,1307,1308,1309,
1310,1311,1312,1313,1314,1315,1316,1317,1318,1319,1320,1321,1322,1323,1324,1325,
1326,1327, INF, INF, INF,1328,1329,1330,1331,1332, INF, INF, INF,1333,1334,1335,
1336,1337, INF, INF, INF,1338,1339,1340,1341,1342,1343,1344,1345,1346,1347,1348,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
1349,1350,1351,1352,1353,1354,1355,1356,1357,1358,1359,1360,1361,1362,1363,1364,
1365,1366,1367,1368,1369,1370,1371,1372,1373,1374,1375,1376,1377,1378,1379,1380,
1381,1382,1383,1384,1385,1386,1387,1388, INF, INF,1389,1390,1391,1392,1393,1394,
 INF, INF,1395,1396,1397,1398,1399,1400, INF, INF,1401,1402,1403,1404,1405,1406,
1407,1408,1409,1410,1411,1412,1413,1414,1415,1416,1417,1418,1419,1420,1421,1422,
1423,1424,1425,1426,1427,1428,1429,1430,1431,1432,1433,1434,1435,1436,1437,1438,
1439,1440,1441,1442,1443,1444,1445,1446, INF, INF, INF,1447,1448,1449,1450,1451,
 INF, INF, INF,1452,1453,1454,1455,1456, INF, INF, INF,1457,1458,1459,1460,1461,
1462,1463,1464,1465,1466,1467,1468,1469,1470,1471,1472,1473,1474,1475,1476,1477,
1478,1479,1480,1481,1482,1483,1484,1485,1486,1487,1488,1489,1490,1491,1492,1493,
1494,1495,1496,1497,1498,1499,1500,1501,1502, INF, INF, INF,1503,1504,1505,1506,
1507, INF, INF, INF,1508,1509,1510,1511,1512, INF, INF, INF,1513,1514,1515,1516,
1517,1518,1519,1520,1521,1522,1523,1524,1525,1526,1527,1528,1529,1530,1531,1532,
1533,1534,1535,1536,1537,1538,1539,1540,1541,1542,1543,1544,1545,1546,1547,1548,
1549,1550,1551,1552,1553,1554,1555,1556,1557,1558, INF, INF, INF,1559,1560,1561,
1562,1563, INF, INF, INF,1564,1565,1566,1567,1568, INF, INF, INF,1569,1570,1571,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
1572,1573,1574,1575,1576,1577,1578,1579,1580,1581,1582,1583,1584,1585,1586,1587,
1588,1589,1590,1591,1592,1593,1594,1595,1596,1597,1598,1599,1600,1601,1602,1603,
1604,1605,1606,1607,1608,1609,1610,1611,1612,1613,1614,1615,1616,1617,1618,1619,
 INF, INF,1620,1621,1622,1623,1624,1625, INF, INF,1626,1627,1628,1629,1630,1631,
1632,1633,1634,1635,1636,1637,1638,1639,1640,1641,1642,1643,1644,1645,1646,1647,
1648,1649,1650,1651,1652,1653,1654,1655,1656,1657,1658,1659,1660,1661,1662,1663,
1664,1665,1666,1667,1668,1669,1670,1671,1672,1673,1674,1675,1676,1677,1678,1679,
 INF, INF, INF,1680,1681,1682,1683,1684, INF, INF, INF,1685,1686,1687,1688,1689,
1690,1691,1692,1693,1694,1695,1696,1697,1698,1699,1700,1701,1702,1703,1704,1705,
1706,1707,1708,1709,1710,1711,1712,1713,1714,1715,1716,1717,1718,1719,1720,1721,
1722,1723,1724,1725,1726,1727,1728,1729,1730,1731,1732,1733,1734,1735,1736,1737,
1738, INF, INF, INF,1739,1740,1741,1742,1743, INF, INF, INF,1744,1745,1746,1747,
1748,1749,1750,1751,1752,1753,1754,1755,1756,1757,1758,1759,1760,1761,1762,1763,
1764,1765,1766,1767,1768,1769,1770,1771,1772,1773,1774,1775,1776,1777,1778,1779,
1780,1781,1782,1783,1784,1785,1786,1787,1788,1789,1790,1791,1792,1793,1794,1795,
1796,1797, INF, INF, INF,1798,1799,1800,1801,1802, INF, INF, INF,1803,1804,1805,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
};

// Useful macro and enumeration tables

#define IndTriKings(sqk1,sqk2)  ((unsigned) rgsTriKings[sqk1*64+sqk2])
#define IndHalfKings(sqk1,sqk2) ((unsigned) rgsHalfKings[sqk1*64+sqk2])

static const bool rgfTriangle[64] =
{
     true,  true,  true,  true,  false, false, false, false,
     false, true,  true,  true,  false, false, false, false,
     false, false, true,  true,  false, false, false, false,
     false, false, false, true,  false, false, false, false,
     false, false, false, false, false, false, false, false,
     false, false, false, false, false, false, false, false,
     false, false, false, false, false, false, false, false,
     false, false, false, false, false, false, false, false,
};

static const bool rgfNotDiagonal[64] =
{
     false, true,  true,  true,  true,  true,  true,  true,
     true,  false, true,  true,  true,  true,  true,  true,
     true,  true,  false, true,  true,  true,  true,  true,
     true,  true,  true,  false, true,  true,  true,  true,
     true,  true,  true,  true,  false, true,  true,  true,
     true,  true,  true,  true,  true,  false, true,  true,
     true,  true,  true,  true,  true,  true,  false, true,
     true,  true,  true,  true,  true,  true,  true,  false,
};

static const bool rgfInLargeTriangle[64] =
{
     true,  true,  true,  true,  true,  true,  true,  true,
     false, true,  true,  true,  true,  true,  true,  true,
     false, false, true,  true,  true,  true,  true,  true,
     false, false, false, true,  true,  true,  true,  true,
     false, false, false, false, true,  true,  true,  true,
     false, false, false, false, false, true,  true,  true,
     false, false, false, false, false, false, true,  true,
     false, false, false, false, false, false, false, true,
};

#define FInTriangle(sqwk,sqbk)  (rgfTriangle[sqwk] & (rgfNotDiagonal[sqwk]|rgfInLargeTriangle[sqbk]))

// Sort pieces

#define SORT(sq1,sq2)   if (sq1>sq2) { square sqTmp; sqTmp=sq1; sq1=sq2; sq2=sqTmp; }

// Exclude occupied squares

#define EXCLUDE1(sq,sq1)                         (sq-(sq>sq1))
#define EXCLUDE2(sq,sq1,sq2)                     (sq-((sq>sq1)+(sq>sq2)))
#define EXCLUDE3(sq,sq1,sq2,sq3)                 (sq-((sq>sq1)+(sq>sq2)+(sq>sq3)))
#define EXCLUDE4(sq,sq1,sq2,sq3,sq4)             (sq-((sq>sq1)+(sq>sq2)+(sq>sq3)+(sq>sq4)))
#define EXCLUDE5(sq,sq1,sq2,sq3,sq4,sq5)         (sq-((sq>sq1)+(sq>sq2)+(sq>sq3)+(sq>sq4)+(sq>sq5)))
#define EXCLUDE6(sq,sq1,sq2,sq3,sq4,sq5,sq6)     (sq-((sq>sq1)+(sq>sq2)+(sq>sq3)+(sq>sq4)+(sq>sq5)+(sq>sq6)))
#define EXCLUDE7(sq,sq1,sq2,sq3,sq4,sq5,sq6,sq7) (sq-((sq>sq1)+(sq>sq2)+(sq>sq3)+(sq>sq4)+(sq>sq5)+(sq>sq6)+(sq>sq7)))

#if !defined(SWAP)
#define SWAP(sq1, sq2)  {square sq_tmp = sq2; sq2 = sq1; sq1 = sq_tmp;}
#endif

// Calculate index - a lot of functions...

// Enumeration tables

static BYTE     *rgprgsqPiece[6];   // Enumeration for each piece (0 - black pawn)
                                    // For each position of the King, all legal squares
                                    // of the opposite piece enumerated
static BYTE     rgcLegal[6][64];    // # of enumerated positions for each piece and each
                                    // location of enemy king

// Enumerations - indexed by [piece] and [kings enumeration].
// In each table for each [piece] and [king enumeration] we store # of preceeding positions.

static ULONG    *rgprgulSinglePawnless[6];
static ULONG    *rgprgulPairPawnless[6][6];
#if defined (T41_INCLUDE) || defined (T42_INCLUDE)
  static ULONG  *rgprgulTriplePawnless[6][6][6];
#endif
static ULONG    *rgprgulSinglePawnPresent[6];
static ULONG    *rgprgulPairPawnPresent[6][6];
#if defined (T41_INCLUDE) || defined (T42_INCLUDE)
  static ULONG  *rgprgulTriplePawnPresent[6][6][6];
#endif

// Total # of enumerated positions

static ULONG    rgcSinglePawnPresent[6];
static ULONG    rgcSinglePawnless[6];
static ULONG    rgcPairPawnPresent[6][6];
static ULONG    rgcPairPawnless[6][6];
#if defined (T41_INCLUDE) || defined (T42_INCLUDE)
  static ULONG  rgcTriplePawnPresent[6][6][6];
  static ULONG  rgcTriplePawnless[6][6][6];
#endif

// Infinities. Have to be larger than any legal enumeration yet small enough
// so there will be no overflow when combining them with remaining pieces.

#define INF_SINGLE  (110000)
#define INF_PAIR    (6500000)
#define INF_TRIPLE  (500000000)

// Initialize squares and counters table for one piece.
// Piece can be x_pieceNone - that means 'pawn of the wrong color', e.g. KPK BTM.

static void VInitSquaresTable
    (
    piece   pi,
    BYTE    *prgsqPiece,
    BYTE    *prgcLegal
    )
    {
    int sqLo, sqHi;

    memset (prgsqPiece, -1, 64*64);
    sqLo = 0;
    sqHi = 64;
    if (pi <= x_piecePawn)
        {
        sqLo = 8;
        sqHi = 56;
        }
    for (int sqKing = 0; sqKing < 64; sqKing ++)
        {
        int iPiece;

        iPiece = 0;
        for (int sq = sqLo; sq < sqHi; sq ++)
            {
            if (sq == sqKing)
                continue;
            switch (pi)
                {
            case x_piecePawn:
                if (
                    0 != TbColumn (sq) && sqKing == sq+7 ||
                    7 != TbColumn (sq) && sqKing == sq+9
                   )
                   continue;
                break;
            case x_pieceKnight:
                if (
                    TbRow (sq) >= 2 && TbColumn (sq) >= 1 && sqKing == sq-17 ||
                    TbRow (sq) >= 2 && TbColumn (sq) <= 6 && sqKing == sq-15 ||
                    TbRow (sq) >= 1 && TbColumn (sq) >= 2 && sqKing == sq-10 ||
                    TbRow (sq) >= 1 && TbColumn (sq) <= 5 && sqKing == sq-6 ||
                    TbRow (sq) <= 6 && TbColumn (sq) >= 2 && sqKing == sq+6 ||
                    TbRow (sq) <= 6 && TbColumn (sq) <= 5 && sqKing == sq+10 ||
                    TbRow (sq) <= 5 && TbColumn (sq) >= 1 && sqKing == sq+15 ||
                    TbRow (sq) <= 5 && TbColumn (sq) <= 6 && sqKing == sq+17
                   )
                    continue;
                break;
            case x_pieceBishop:
                if (
                    0 != TbRow (sq) && 0 != TbColumn (sq) && sqKing == sq-9 ||
                    0 != TbRow (sq) && 7 != TbColumn (sq) && sqKing == sq-7 ||
                    7 != TbRow (sq) && 0 != TbColumn (sq) && sqKing == sq+7 ||
                    7 != TbRow (sq) && 7 != TbColumn (sq) && sqKing == sq+9
                   )
                    continue;
                break;
            case x_pieceRook:
                if (
                    0 != TbColumn (sq) && sqKing == sq-1 ||
                    7 != TbColumn (sq) && sqKing == sq+1 ||
                    0 != TbRow (sq) && sqKing == sq-8 ||
                    7 != TbRow (sq) && sqKing == sq+8
                   )
                    continue;
                break;
            case x_pieceQueen:
                if (
                    0 != TbColumn (sq) && sqKing == sq-1 ||
                    7 != TbColumn (sq) && sqKing == sq+1 ||
                    0 != TbRow (sq) && sqKing == sq-8 ||
                    7 != TbRow (sq) && sqKing == sq+8 ||
                    0 != TbRow (sq) && 0 != TbColumn (sq) && sqKing == sq-9 ||
                    0 != TbRow (sq) && 7 != TbColumn (sq) && sqKing == sq-7 ||
                    7 != TbRow (sq) && 0 != TbColumn (sq) && sqKing == sq+7 ||
                    7 != TbRow (sq) && 7 != TbColumn (sq) && sqKing == sq+9
                   )
                    continue;
                break;
                }
            prgsqPiece[sqKing*64+sq] = (BYTE) iPiece;
            iPiece ++;
            }
        prgcLegal[sqKing] = (BYTE) iPiece;
        }
    }

// Initialize enumeration table for single piece

static void VInitSingle
    (
    ULONG       *prgIndex,
    const short *prgsKings,
    const BYTE  *prgcLegal,
    const BYTE  *prgsqPiece,
    ULONG       *pcEnumeration
    )
    {
    ULONG iIndex;

    iIndex = 0;
    for (int sqKing1 = 0; sqKing1 < 64; sqKing1 ++)
        for (int sqKing2 = 0; sqKing2 < 64; sqKing2 ++)
            {
            if (INF != prgsKings[sqKing1*64+sqKing2])
                {
                prgIndex[prgsKings[sqKing1*64+sqKing2]] = iIndex;
                iIndex += prgcLegal[sqKing2] - ((BYTE) -1 != prgsqPiece[sqKing2*64+sqKing1]);
                }
            }
    *pcEnumeration = iIndex;
    }

// Initialize enumeration table for pair of pieces

static void VInitPair
    (
    ULONG       *prgIndex,
    const short *prgsKings,
    const BYTE  *prgcLegal1,
    const BYTE  *prgsqPiece1,
    const BYTE  *prgcLegal2,
    const BYTE  *prgsqPiece2,
    ULONG       *pcEnumeration
    )
    {
    ULONG iIndex;
    ULONG cPositions1, cPositions2;

    iIndex = 0;
    for (int sqKing1 = 0; sqKing1 < 64; sqKing1 ++)
        for (int sqKing2 = 0; sqKing2 < 64; sqKing2 ++)
            {
            if (INF != prgsKings[sqKing1*64+sqKing2])
                {
                prgIndex[prgsKings[sqKing1*64+sqKing2]] = iIndex;
                cPositions1 = prgcLegal1[sqKing2] - ((BYTE) -1 != prgsqPiece1[sqKing2*64+sqKing1]);
                if (prgcLegal1 == prgcLegal2)
                    iIndex += cPositions1*(cPositions1-1)/2;
                else
                    {
                    cPositions2 = prgcLegal2[sqKing2] - ((BYTE) -1 != prgsqPiece2[sqKing2*64+sqKing1]);
                    iIndex += cPositions1*cPositions2;
                    }
                }
            }
    *pcEnumeration = iIndex;
    }

#if defined (T41_INCLUDE) || defined (T42_INCLUDE)

// Initialize enumeration table for triple piece

static void VInitTriple
    (
    ULONG       *prgIndex,
    const short *prgsKings,
    const BYTE  *prgcLegal1,
    const BYTE  *prgsqPiece1,
    const BYTE  *prgcLegal2,
    const BYTE  *prgsqPiece2,
    const BYTE  *prgcLegal3,
    const BYTE  *prgsqPiece3,
    ULONG       *pcEnumeration
    )
    {
    ULONG iIndex;
    ULONG cPositions1, cPositions2, cPositions3;

    iIndex = 0;
    for (int sqKing1 = 0; sqKing1 < 64; sqKing1 ++)
        for (int sqKing2 = 0; sqKing2 < 64; sqKing2 ++)
            {
            if (INF != prgsKings[sqKing1*64+sqKing2])
                {
                prgIndex[prgsKings[sqKing1*64+sqKing2]] = iIndex;
                cPositions1 = prgcLegal1[sqKing2] - ((BYTE) -1 != prgsqPiece1[sqKing2*64+sqKing1]);
                if (prgcLegal1 == prgcLegal2 && prgcLegal2 == prgcLegal3)
                    iIndex += cPositions1*(cPositions1-1)*(cPositions1-2)/6;
                else if (prgcLegal1 == prgcLegal2)
                    {
                    cPositions3 = prgcLegal3[sqKing2] - ((BYTE) -1 != prgsqPiece3[sqKing2*64+sqKing1]);
                    iIndex += cPositions1*(cPositions1-1)/2*cPositions3;
                    }
                else if (prgcLegal2 == prgcLegal3)
                    {
                    cPositions2 = prgcLegal2[sqKing2] - ((BYTE) -1 != prgsqPiece2[sqKing2*64+sqKing1]);
                    iIndex += cPositions1*cPositions2*(cPositions2-1)/2;
                    }
                else
                    {
                    cPositions2 = prgcLegal2[sqKing2] - ((BYTE) -1 != prgsqPiece2[sqKing2*64+sqKing1]);
                    cPositions3 = prgcLegal3[sqKing2] - ((BYTE) -1 != prgsqPiece3[sqKing2*64+sqKing1]);
                    iIndex += cPositions1*cPositions2*cPositions3;
                    }
                }
            }
    *pcEnumeration = iIndex;
    }

#endif

// Initialize all Enumeration tables

static bool fEnumerationInitted = false;

static void VInitEnumerations (void)
    {
    piece   pi1;
    piece   pi2;
#if defined (T41_INCLUDE) || defined (T42_INCLUDE)
    piece   pi3;
#endif

    if (fEnumerationInitted)
        return;
    fEnumerationInitted = true;
    // Initialize square tables
    for (pi1 = x_pieceNone; pi1 <= x_pieceQueen; pi1 = (piece) (pi1 + 1))
        {
        rgprgsqPiece[pi1] = (BYTE *) PvMalloc (64*64);
        VInitSquaresTable (pi1, rgprgsqPiece[pi1], rgcLegal[pi1]);
        }

    for (pi1 = x_pieceNone; pi1 <= x_pieceQueen; pi1 = (piece) (pi1 + 1))
        {
        // Initialize enumeration tables for single piece
        rgprgulSinglePawnPresent[pi1] = (ULONG *) PvMalloc (1806*sizeof (ULONG));
        VInitSingle (rgprgulSinglePawnPresent[pi1], rgsHalfKings,
                     rgcLegal[pi1], rgprgsqPiece[pi1], &rgcSinglePawnPresent[pi1]);
        if (pi1 > x_piecePawn)
            {
            rgprgulSinglePawnless[pi1] = (ULONG *) PvMalloc (462*sizeof (ULONG));
            VInitSingle (rgprgulSinglePawnless[pi1], rgsTriKings,
                         rgcLegal[pi1], rgprgsqPiece[pi1], &rgcSinglePawnless[pi1]);
            }
        // Initialize enumeration tables for pair of pieces
        for (pi2 = (x_pieceNone == pi1 ? x_pieceNone : x_piecePawn); pi2 <= pi1; pi2 = (piece) (pi2 + 1))
            {
            rgprgulPairPawnPresent[pi1][pi2] = (ULONG *) PvMalloc (1806*sizeof (ULONG));
            VInitPair (rgprgulPairPawnPresent[pi1][pi2], rgsHalfKings,
                       rgcLegal[pi1], rgprgsqPiece[pi1], rgcLegal[pi2], rgprgsqPiece[pi2],
                       &rgcPairPawnPresent[pi1][pi2]);
            if (pi1 > x_piecePawn && pi2 > x_piecePawn)
                {
                rgprgulPairPawnless[pi1][pi2] = (ULONG *) PvMalloc (462*sizeof (ULONG));
                VInitPair (rgprgulPairPawnless[pi1][pi2], rgsTriKings,
                           rgcLegal[pi1], rgprgsqPiece[pi1], rgcLegal[pi2], rgprgsqPiece[pi2],
                           &rgcPairPawnless[pi1][pi2]);
                }
#if defined (T41_INCLUDE) || defined (T42_INCLUDE)
            // Initialize enumeration tables for three pieces
            for (pi3 = (x_pieceNone == pi1 ? x_pieceNone : x_piecePawn); pi3 <= pi2; pi3 = (piece) (pi3 + 1))
                {
                if (pi1 <= x_piecePawn || pi2 <= x_piecePawn || pi3 <= x_piecePawn)
                    {
                    rgprgulTriplePawnPresent[pi1][pi2][pi3] = (ULONG *) PvMalloc (1806*sizeof (ULONG));
                    VInitTriple (rgprgulTriplePawnPresent[pi1][pi2][pi3], rgsHalfKings,
                                 rgcLegal[pi1], rgprgsqPiece[pi1],
                                 rgcLegal[pi2], rgprgsqPiece[pi2],
                                 rgcLegal[pi3], rgprgsqPiece[pi3],
                                 &rgcTriplePawnPresent[pi1][pi2][pi3]);
                    }
                else
                    {
                    rgprgulTriplePawnless[pi1][pi2][pi3] = (ULONG *) PvMalloc (462*sizeof (ULONG));
                    VInitTriple (rgprgulTriplePawnless[pi1][pi2][pi3], rgsTriKings,
                        rgcLegal[pi1], rgprgsqPiece[pi1],
                        rgcLegal[pi2], rgprgsqPiece[pi2],
                        rgcLegal[pi3], rgprgsqPiece[pi3],
                        &rgcTriplePawnless[pi1][pi2][pi3]);
#if defined (T42_INCLUDE)
                    rgprgulTriplePawnPresent[pi1][pi2][pi3] = (ULONG *) PvMalloc (1806*sizeof (ULONG));
                    VInitTriple (rgprgulTriplePawnPresent[pi1][pi2][pi3], rgsHalfKings,
                                 rgcLegal[pi1], rgprgsqPiece[pi1],
                                 rgcLegal[pi2], rgprgsqPiece[pi2],
                                 rgcLegal[pi3], rgprgsqPiece[pi3],
                                 &rgcTriplePawnPresent[pi1][pi2][pi3]);
#endif
                    }
                }
#endif
            }
        }

    // All done!
    if (fPrint)
        {
        for (pi1 = x_pieceNone; pi1 <= x_pieceQueen; pi1 = (piece) (pi1 + 1))
            printf ("%c - %d enumerated positions\n", "pPNBRQ"[pi1], rgcSinglePawnPresent[pi1]);
        for (pi1 = x_pieceNone; pi1 <= x_pieceQueen; pi1 = (piece) (pi1 + 1))
            {
            if (0 != rgcSinglePawnless[pi1])
                printf ("pawnless %c - %d enumerated positions\n", "pPNBRQ"[pi1], rgcSinglePawnless[pi1]);
            }
        for (pi1 = x_pieceNone; pi1 <= x_pieceQueen; pi1 = (piece) (pi1 + 1))
            for (pi2 = x_pieceNone; pi2 <= pi1; pi2 = (piece) (pi2 + 1))
                {
                if (0 != rgcPairPawnPresent[pi1][pi2])
                    printf ("%c%c - %d enumerated positions\n", "pPNBRQ"[pi1], "pPNBRQ"[pi2],
                            rgcPairPawnPresent[pi1][pi2]);
                }
        for (pi1 = x_pieceNone; pi1 <= x_pieceQueen; pi1 = (piece) (pi1 + 1))
            for (pi2 = x_pieceNone; pi2 <= pi1; pi2 = (piece) (pi2 + 1))
                {
                if (0 != rgcPairPawnless[pi1][pi2])
                    printf ("pawnless %c%c - %d enumerated positions\n", "pPNBRQ"[pi1], "pPNBRQ"[pi2],
                            rgcPairPawnless[pi1][pi2]);
                }
#if defined (T41_INCLUDE) || defined (T42_INCLUDE)
        for (pi1 = x_pieceNone; pi1 <= x_pieceQueen; pi1 = (piece) (pi1 + 1))
            for (pi2 = x_pieceNone; pi2 <= pi1; pi2 = (piece) (pi2 + 1))
                for (pi3 = x_pieceNone; pi3 <= pi2; pi3 = (piece) (pi3 + 1))
                    {
                    if (0 != rgcTriplePawnPresent[pi1][pi2][pi3])
                        printf ("%c%c%c - %d enumerated positions\n",
                                "pPNBRQ"[pi1], "pPNBRQ"[pi2], "pPNBRQ"[pi3],
                                rgcTriplePawnPresent[pi1][pi2][pi3]);
                    }
        for (pi1 = x_pieceNone; pi1 <= x_pieceQueen; pi1 = (piece) (pi1 + 1))
            for (pi2 = x_pieceNone; pi2 <= pi1; pi2 = (piece) (pi2 + 1))
                for (pi3 = x_pieceNone; pi3 <= pi2; pi3 = (piece) (pi3 + 1))
                    {
                    if (0 != rgcTriplePawnless[pi1][pi2][pi3])
                        printf ("pawnless %c%c%c - %d enumerated positions\n",
                                "pPNBRQ"[pi1], "pPNBRQ"[pi2], "pPNBRQ"[pi3],
                                rgcTriplePawnless[pi1][pi2][pi3]);
                    }
#endif
        printf ("\nAllocated %dk\n\n", (cbAllocated + 1023)/1024);
        }
    }

// Return enumeration of 2 kings and single piece

template <int piw1, bool fPawns, bool fInvert> class TEnumerate1
    {
public:
    static INLINE unsigned TB_FASTCALL Index
        (
        square  sqwk,
        square  sqw1,
        square  sqbk
        )
        {
        unsigned ind;
        ULONG    ulKings;

        // For black pawn invert the board
        if (piw1 <= x_piecePawn && fInvert)
            {
            sqwk = reflect_x(sqwk);
            sqw1 = reflect_x(sqw1);
            sqbk = reflect_x(sqbk);
            }
        
        // Get enumerated square
        ind = rgprgsqPiece[piw1][sqbk*64+sqw1];
#if defined (ILLEGAL_POSSIBLE)
        if ((BYTE) -1 == ind)
            return INF_SINGLE;
#endif
        // Get enumerated position of both kings
        if (fPawns)
            ulKings = rgsHalfKings[sqwk*64+sqbk];   // 0..1805
        else
            ulKings = rgsTriKings[sqwk*64+sqbk];    // 0..461
#if defined (ILLEGAL_POSSIBLE)
        if (INF == ulKings)
            return INF_SINGLE;
#endif
        // Can we remove one extra square?
        if ((piw1>x_pieceKnight) || ((BYTE)-1 != rgprgsqPiece[piw1][sqbk*64+sqwk]))
            ind -= (sqw1 > sqwk);
        // Add enumerated square to the # of the preceeding positions
        return ind + (fPawns ? rgprgulSinglePawnPresent[piw1][ulKings] : rgprgulSinglePawnless[piw1][ulKings]);
        }
    };

// Return enumeration of 2 kings and 2 pieces

template <int piw1, int piw2, bool fPawns, bool fInvert> class TEnumerate2
    {
public:
    static INLINE unsigned TB_FASTCALL Index
        (
        square  sqwk,
        square  sqw1,
        square  sqw2,
        square  sqbk
        )
        {
        unsigned ind1, ind2, cInd2;
        ULONG    ulKings;

        // For black pawn invert the board
        if (piw2 <= x_piecePawn && fInvert)
            {
            sqwk = reflect_x(sqwk);
            sqw1 = reflect_x(sqw1);
            sqw2 = reflect_x(sqw2);
            sqbk = reflect_x(sqbk);
            }
        
        // Get enumerated squares for both pieces
        if (piw1 == piw2)
            SORT (sqw1, sqw2);
        ind1 = rgprgsqPiece[piw1][sqbk*64+sqw1];
        ind2 = rgprgsqPiece[piw2][sqbk*64+sqw2];
#if defined (ILLEGAL_POSSIBLE)
        if ((BYTE) -1 == ind1 || (BYTE) -1 == ind2)
            return INF_PAIR;
#endif
        // Get enumerated position of both kings
        if (fPawns)
            ulKings = rgsHalfKings[sqwk*64+sqbk];   // 0..1805
        else
            ulKings = rgsTriKings[sqwk*64+sqbk];    // 0..461
#if defined (ILLEGAL_POSSIBLE)
        if (INF == ulKings)
            return INF_PAIR;
#endif
        if (piw1 == piw2)
            {
            // Can we remove one extra square?
            if ((piw1>x_pieceKnight) || ((BYTE)-1 != rgprgsqPiece[piw1][sqbk*64+sqwk]))
                {
                ind1 -= (sqw1 > sqwk);
                ind2 -= (sqw2 > sqwk);
                }
            // Add enumerated squares to the # of the preceeding positions
            return  ind2*(ind2-1)/2 + ind1 +
                    (fPawns ? rgprgulPairPawnPresent[piw1][piw2][ulKings] : rgprgulPairPawnless[piw1][piw2][ulKings]);
            }
        else
            {
            // Can we remove WK square from 1st piece Enumeration?
            if ((piw1>x_pieceKnight) || ((BYTE)-1 != rgprgsqPiece[piw1][sqbk*64+sqwk]))
                ind1 -= (sqw1 > sqwk);
            // Get # of enumerated positions of 2nd piece
            cInd2 = rgcLegal[piw2][sqbk];
            // Can we remove WK square from 2nd piece Enumeration?
            if ((piw2>x_pieceKnight) || ((BYTE)-1 != rgprgsqPiece[piw2][sqbk*64+sqwk]))
                {
                cInd2 --;
                ind2 -= (sqw2 > sqwk);
                }
            // Add enumerated square to the # of the preceeding positions
            return cInd2*ind1 + ind2 + (fPawns ? rgprgulPairPawnPresent[piw1][piw2][ulKings] : rgprgulPairPawnless[piw1][piw2][ulKings]);
            }
        }
    };

#if defined (T41_INCLUDE) || defined (T42_INCLUDE)

// Return enumeration of 2 kings and 3 pieces

template <int piw1, int piw2, int piw3, bool fPawns, bool fInvert> class TEnumerate3
    {
public:
    static INLINE unsigned TB_FASTCALL Index
        (
        square  sqwk,
        square  sqw1,
        square  sqw2,
        square  sqw3,
        square  sqbk
        )
        {
        unsigned ind1, ind2, ind3, cInd1, cInd2, cInd3;
        ULONG    ulKings;

        // For black pawn invert the board
        if (piw3 <= x_piecePawn && fInvert)
            {
            sqwk = reflect_x(sqwk);
            sqw1 = reflect_x(sqw1);
            sqw2 = reflect_x(sqw2);
            sqw3 = reflect_x(sqw3);
            sqbk = reflect_x(sqbk);
            }
        
        // Get enumerated squares for all pieces
        if (piw1 == piw2 && piw1 == piw3)
            {
            SORT (sqw1, sqw2);
            SORT (sqw2, sqw3);
            SORT (sqw1, sqw2);
            }
        else if (piw1 == piw2)
            {
            SORT (sqw1, sqw2);
            }
        else if (piw2 == piw3)
            {
            SORT (sqw2, sqw3);
            }
        ind1 = rgprgsqPiece[piw1][sqbk*64+sqw1];
        ind2 = rgprgsqPiece[piw2][sqbk*64+sqw2];
        ind3 = rgprgsqPiece[piw3][sqbk*64+sqw3];
#if defined (ILLEGAL_POSSIBLE)
        if ((BYTE) -1 == ind1 || (BYTE) -1 == ind2 || (BYTE) -1 == ind3)
            return INF_TRIPLE;
#endif
        // Get enumerated position of both kings
        if (fPawns)
            ulKings = rgsHalfKings[sqwk*64+sqbk];   // 0..1805
        else
            ulKings = rgsTriKings[sqwk*64+sqbk];    // 0..461
#if defined (ILLEGAL_POSSIBLE)
        if (INF == ulKings)
            return INF_TRIPLE;
#endif
        if (piw1 == piw2 && piw2 == piw3)
            {
            // Can we remove one extra square?
            if ((piw1>x_pieceKnight) || ((BYTE)-1 != rgprgsqPiece[piw1][sqbk*64+sqwk]))
                {
                ind1 -= (sqw1 > sqwk);
                ind2 -= (sqw2 > sqwk);
                ind3 -= (sqw3 > sqwk);
                }
            // Add enumerated squares to the # of the preceeding positions
            return  ind3*(ind3-1)*(ind3-2)/6 + ind2*(ind2-1)/2 + ind1 +
                    (fPawns ? rgprgulTriplePawnPresent[piw1][piw2][piw3][ulKings] :
                              rgprgulTriplePawnless[piw1][piw2][piw3][ulKings]);
            }
        else if (piw1 == piw2)
            {
            // Can we remove one extra square?
            if ((piw1>x_pieceKnight) || ((BYTE)-1 != rgprgsqPiece[piw1][sqbk*64+sqwk]))
                {
                ind1 -= (sqw1 > sqwk);
                ind2 -= (sqw2 > sqwk);
                }
            // Get # of enumerated positions of 3rd piece
            cInd3 = rgcLegal[piw3][sqbk];
            // Can we remove WK square from 3rd piece Enumeration?
            if ((piw3>x_pieceKnight) || ((BYTE)-1 != rgprgsqPiece[piw3][sqbk*64+sqwk]))
                {
                cInd3 --;
                ind3 -= (sqw3 > sqwk);
                }
            // Add enumerated squares to the # of the preceeding positions
            return  (ind2*(ind2-1)/2 + ind1)*cInd3 + ind3 +
                    (fPawns ? rgprgulTriplePawnPresent[piw1][piw2][piw3][ulKings] :
                              rgprgulTriplePawnless[piw1][piw2][piw3][ulKings]);
            }
        else if (piw2 == piw3)
            {
            // Can we remove one extra square?
            if ((piw2>x_pieceKnight) || ((BYTE)-1 != rgprgsqPiece[piw2][sqbk*64+sqwk]))
                {
                ind2 -= (sqw2 > sqwk);
                ind3 -= (sqw3 > sqwk);
                }
            // Get # of enumerated positions of 1st piece
            cInd1 = rgcLegal[piw1][sqbk];
            // Can we remove WK square from 3rd piece Enumeration?
            if ((piw1>x_pieceKnight) || ((BYTE)-1 != rgprgsqPiece[piw1][sqbk*64+sqwk]))
                {
                cInd1 --;
                ind1 -= (sqw1 > sqwk);
                }
            // Add enumerated squares to the # of the preceeding positions
            return  (ind3*(ind3-1)/2 + ind2)*cInd1 + ind1 +
                    (fPawns ? rgprgulTriplePawnPresent[piw1][piw2][piw3][ulKings] :
                              rgprgulTriplePawnless[piw1][piw2][piw3][ulKings]);
            }
        else
            {
            // Can we remove WK square from 1st piece Enumeration?
            if ((piw1>x_pieceKnight) || ((BYTE)-1 != rgprgsqPiece[piw1][sqbk*64+sqwk]))
                ind1 -= (sqw1 > sqwk);
            // Get # of enumerated positions of 2nd piece
            cInd2 = rgcLegal[piw2][sqbk];
            // Can we remove WK square from 2nd piece Enumeration?
            if ((piw2>x_pieceKnight) || ((BYTE)-1 != rgprgsqPiece[piw2][sqbk*64+sqwk]))
                {
                cInd2 --;
                ind2 -= (sqw2 > sqwk);
                }
            // Get # of enumerated positions of 3rd piece
            cInd3 = rgcLegal[piw3][sqbk];
            // Can we remove WK square from 3rd piece Enumeration?
            if ((piw3>x_pieceKnight) || ((BYTE)-1 != rgprgsqPiece[piw3][sqbk*64+sqwk]))
                {
                cInd3 --;
                ind3 -= (sqw3 > sqwk);
                }
            // Add enumerated square to the # of the preceeding positions
            return  cInd3*(cInd2*ind1 + ind2) + ind3 +
                    (fPawns ? rgprgulTriplePawnPresent[piw1][piw2][piw3][ulKings] :
                              rgprgulTriplePawnless[piw1][piw2][piw3][ulKings]);
            }
        }
    };

#endif

// Enumerate en passant captures

static INLINE unsigned TB_FASTCALL IndEnPassant11W
    (
    square  sqw,
    square  sqb,
    square  sqEnP
    )
    {
    assert (sqb+8 == sqEnP);
    if (sqw+7 == sqEnP)
        // Capture to the left
        return  (sqw&7)-1;
    else
        {
        // Capture to the right
        assert (sqw+9 == sqEnP);
        return  (sqw&7)+7;
        }
    }

static INLINE unsigned TB_FASTCALL IndEnPassant11B
    (
    square  sqw,
    square  sqb,
    square  sqEnP
    )
    {
    assert (sqw-8 == sqEnP);
    if (sqb-9 == sqEnP)
        // Capture to the left
        return (sqb&7)-1;
    else
        {
        // Capture to the right
        assert (sqb-7 == sqEnP);
        return (sqb&7)+7;
        }
    }

static INLINE unsigned TB_FASTCALL IndEnPassant21W
    (
    square  sqw1,
    square  sqw2,
    square  sqb,
    square  sqEnP
    )
    {
    assert (sqb+8 == sqEnP);
    SORT (sqw1, sqw2);
    if (sqw1+7 == sqEnP && 0 != TbColumn(sqw1))
        // Capture to the left
        return (sqw1&7)-1+(EXCLUDE3(sqw2,sqb,sqEnP,sqEnP+8)-i8-1)*i14;
    else if (sqw1+9 == sqEnP && 7 != TbColumn(sqw1))
        // Capture to the right
        return (sqw1&7)+7+(EXCLUDE3(sqw2,sqb,sqEnP,sqEnP+8)-i8-1)*i14;
    else if (sqw2+7 == sqEnP && 0 != TbColumn(sqw2))
        // Capture to the left
        return (sqw2&7)-1+(EXCLUDE3(sqw1,sqb,sqEnP,sqEnP+8)-i8)*i14;
    else
        {
        // Capture to the right
        assert (sqw2+9 == sqEnP && 7 != TbColumn(sqw2));
        return (sqw2&7)+7+(EXCLUDE3(sqw1,sqb,sqEnP,sqEnP+8)-i8)*i14;
        }
    }

static INLINE unsigned TB_FASTCALL IndEnPassant21B
    (
    square  sqw1,
    square  sqw2,
    square  sqb,
    square  sqEnP
    )
    {
    assert (sqw1 < sqw2);   // Must be already sorted
    if (sqb-9 == sqEnP && 0 != TbColumn(sqb))
        // Capture to the left
        if (sqw1-8 == sqEnP)
            return (sqb&7)-1+(EXCLUDE3(sqw2,sqb,sqEnP,sqEnP-8)-i8-1)*i14;
        else
            {
            assert (sqw2-8 == sqEnP);
            return (sqb&7)-1+(EXCLUDE3(sqw1,sqb,sqEnP,sqEnP-8)-i8)*i14;
            }
    else
        {
        // Capture to the right
        assert (sqb-7 == sqEnP && 7 != TbColumn(sqb));
        if (sqw1-8 == sqEnP)
            return (sqb&7)+7+(EXCLUDE3(sqw2,sqb,sqEnP,sqEnP-8)-i8-1)*i14;
        else
            {
            assert (sqw2-8 == sqEnP);
            return (sqb&7)+7+(EXCLUDE3(sqw1,sqb,sqEnP,sqEnP-8)-i8)*i14;
            }
        }
    }

static INLINE unsigned TB_FASTCALL IndEnPassant12W
    (
    square  sqw,
    square  sqb1,
    square  sqb2,
    square  sqEnP
    )
{
    if(sqb2+8 == sqEnP)
        SWAP(sqb1, sqb2);

    assert(sqb1+8 == sqEnP);

    if (sqw+7 == sqEnP && 0 != TbColumn(sqw))
        // Capture to the left
        return TbColumn(sqw)-1+(EXCLUDE4(sqb2,sqb1,sqw,sqEnP,sqEnP+8)-i8)*i14;

    assert(sqw+9 == sqEnP && 7 != TbColumn(sqw));

    // Capture to the right
    return TbColumn(sqw)+7 + (EXCLUDE4(sqb2,sqb1,sqw,sqEnP,sqEnP+8)-i8)*i14;
}

static INLINE unsigned TB_FASTCALL IndEnPassant12B
    (
    square  sqw,
    square  sqb1,
    square  sqb2,
    square  sqEnP
    )
{
    assert(sqw-8 == sqEnP);

    SORT(sqb1, sqb2);

    if(sqb1-9 == sqEnP && 0 != TbColumn(sqb1))
        // Capture to the left
        return TbColumn(sqb1) - 1 + (sqb2-i8-4)*i14;
    else if(sqb1-7 == sqEnP && 7 != TbColumn(sqb1))
        // Capture to the right
        return TbColumn(sqb1) + 7 + (sqb2-i8-4)*i14;
    else if(sqb2-9 == sqEnP && 0 != TbColumn(sqb2))
        // Capture to the left
        return TbColumn(sqb2) - 1 + (EXCLUDE2(sqb1,sqEnP,sqEnP-8)-i8)*i14;

    // Capture to the right
    assert(sqb2-7 == sqEnP && 7 != TbColumn(sqb2));
    return TbColumn(sqb2) + 7 + (EXCLUDE2(sqb1,sqEnP,sqEnP-8)-i8)*i14;
}

static INLINE unsigned TB_FASTCALL IndEnPassant22W
    (
    square  sqw1,
    square  sqw2,
    square  sqb1,
    square  sqb2,
    square  sqEnP
    )
{
    square sqEmptyEnP = sqEnP + 8;

    if(sqb2+8== sqEnP)
       SWAP(sqb1, sqb2);

    assert(sqb1+8 == sqEnP);

    SORT(sqw1, sqw2);

    if (sqw1+7 == sqEnP && 0 != TbColumn(sqw1))
            // Capture to the left
        return TbColumn(sqw1)-1+
           (EXCLUDE2(sqw2,sqEnP,sqEmptyEnP)-i8-2)*i14+
           (EXCLUDE5(sqb2,sqb1,sqw1,sqw2,sqEnP,sqEmptyEnP)-i8)*(i14*i44);
    else if (sqw1+9 == sqEnP && 7 != TbColumn(sqw1))
            // Capture to the right
        return TbColumn(sqw1)+7+
           (EXCLUDE2(sqw2,sqEnP,sqEmptyEnP)-i8-2)*i14+
           (EXCLUDE5(sqb2,sqb1,sqw1,sqw2,sqEnP,sqEmptyEnP)-i8)*(i14*i44);
    else if (sqw2+7 == sqEnP && 0 != TbColumn(sqw2))
            // Capture to the left
        return TbColumn(sqw2)-1+
           (sqw1-i8)*i14+
           (EXCLUDE5(sqb2,sqb1,sqw1,sqw2,sqEnP,sqEmptyEnP)-i8)*(i14*i44);

    // Capture to the right
    assert(sqw2+9 == sqEnP && 7 != TbColumn(sqw2));
    return TbColumn(sqw2)+7+
           (sqw1-i8)*i14+
       (EXCLUDE5(sqb2,sqb1,sqw1,sqw2,sqEnP,sqEmptyEnP)-i8)*(i14*i44);
}

static INLINE unsigned TB_FASTCALL IndEnPassant22B
    (
    square  sqw1,
    square  sqw2,
    square  sqb1,
    square  sqb2,
    square  sqEnP
    )
{
    square sqEmptyEnP = sqEnP - 8;

    if(sqw2-8 == sqEnP)
       SWAP(sqw1, sqw2);

    assert(sqw1-8 == sqEnP);

    SORT(sqb1, sqb2);

    if (sqb1-9 == sqEnP && 0 != TbColumn(sqb1))
        // Capture to the left
        return TbColumn(sqb1)-1+
           (sqb2-i8-4)*i14+
           (EXCLUDE5(sqw2,sqw1,sqb1,sqb2,sqEnP,sqEmptyEnP)-i8)*(i14*i44);
    else if (sqb1-7 == sqEnP && 7 != TbColumn(sqb1))
        // Capture to the right
        return TbColumn(sqb1)+7+
           (sqb2-i8-4)*i14+
           (EXCLUDE5(sqw2,sqw1,sqb1,sqb2,sqEnP,sqEmptyEnP)-i8)*(i14*i44);
    else if (sqb2-9 == sqEnP && 0 != TbColumn(sqb2))
        // Capture to the left
        return TbColumn(sqb2)-1+
           (EXCLUDE2(sqb1,sqEnP,sqEmptyEnP)-i8)*i14+
           (EXCLUDE5(sqw2,sqw1,sqb1,sqb2,sqEnP,sqEmptyEnP)-i8)*(i14*i44);

    // Capture to the right
    assert(sqb2-7 == sqEnP && 7 != TbColumn(sqb2));
    return TbColumn(sqb2)+7+
           (EXCLUDE2(sqb1,sqEnP,sqEmptyEnP)-i8)*i14+
       (EXCLUDE5(sqw2,sqw1,sqb1,sqb2,sqEnP,sqEmptyEnP)-i8)*(i14*i44);
}

static INLINE unsigned TB_FASTCALL IndEnPassant31W
    (
    square  sqw1,
    square  sqw2,
    square  sqw3,
    square  sqb,
    square  sqEnP
    )
{
    square sqEmptyEnP = sqEnP + 8;

    assert(sqb + 8 == sqEnP);

    SORT (sqw1, sqw2);
    SORT (sqw2, sqw3);
    SORT (sqw1, sqw2);

    if (sqw1+7 == sqEnP && 0 != TbColumn(sqw1)) {
        // Capture to the left
        sqw3 = EXCLUDE2(sqw3,sqEnP,sqEmptyEnP)-i8-2;
        return TbColumn(sqw1)-1+
               (EXCLUDE2(sqw2,sqEnP,sqEmptyEnP)-i8-2)*i14+
               (sqw3*(sqw3-1)/2)*i14;
        }
    else if (sqw1+9 == sqEnP && 7 != TbColumn(sqw1)) {
        // Capture to the right
        sqw3 = EXCLUDE2(sqw3,sqEnP,sqEmptyEnP)-i8-2;
        return TbColumn(sqw1)+7+
               (EXCLUDE2(sqw2,sqEnP,sqEmptyEnP)-i8-2)*i14+
               (sqw3*(sqw3-1)/2)*i14;
    }
    else if (sqw2+7 == sqEnP && 0 != TbColumn(sqw2)) {
        // Capture to the left
        sqw3 = EXCLUDE2(sqw3,sqEnP,sqEmptyEnP)-i8-2;
        return TbColumn(sqw2)-1+
               (sqw1-i8)*i14+
               (sqw3*(sqw3-1)/2)*i14;
    }
    else if (sqw2+9 == sqEnP && 7 != TbColumn(sqw2)) {
        // Capture to the right
        sqw3 = EXCLUDE2(sqw3,sqEnP,sqEmptyEnP)-i8-2;
        return TbColumn(sqw2)+7+
               (sqw1-i8)*i14+
               (sqw3*(sqw3-1)/2)*i14;
    }
    else if (sqw3+7 == sqEnP && 0 != TbColumn(sqw3)) {
        // Capture to the left
        sqw2 = sqw2-i8;
        return TbColumn(sqw3)-1+
               (sqw1-i8)*i14+
               (sqw2*(sqw2-1)/2)*i14;
    }
    else {
        // Capture to the right
        sqw2 = sqw2-i8;
        return TbColumn(sqw3)+7+
               (sqw1-i8)*i14+
               (sqw2*(sqw2-1)/2)*i14;
    }
}

static INLINE unsigned TB_FASTCALL IndEnPassant31B
    (
    square  sqw1,
    square  sqw2,
    square  sqw3,
    square  sqb,
    square  sqEnP
    )
{
    square sqEmptyEnP = sqEnP - 8;

    if(sqw2 - 8 == sqEnP)
        SWAP(sqw1, sqw2);
    if(sqw3 - 8 == sqEnP)
        SWAP(sqw1, sqw3);

    assert(sqw1 - 8 == sqEnP);

    SORT(sqw2, sqw3);

    if(sqb - 9 == sqEnP && 0 != TbColumn(sqb)) {
        sqw3 = EXCLUDE4(sqw3,sqw1,sqb,sqEnP,sqEmptyEnP)-i8;
        return TbColumn(sqb)-1+
        (EXCLUDE4(sqw2,sqw1,sqb,sqEnP,sqEmptyEnP)-i8)*i14+
        (sqw3*(sqw3-1)/2)*i14;
    }
    else {
        assert(sqb - 7 == sqEnP && 7 != TbColumn(sqb));
        sqw3 = EXCLUDE4(sqw3,sqw1,sqb,sqEnP,sqEmptyEnP)-i8;
        return TbColumn(sqb)+7+
            (EXCLUDE4(sqw2,sqw1,sqb,sqEnP,sqEmptyEnP)-i8)*i14+
        (sqw3*(sqw3-1)/2)*i14;
    }
}

// Index calculation functions for different endgame classes

template <int piw1> class T21
    {
public:
    static INDEX TB_FASTCALL IndCalcW
        (
        square  *psqW,
        square  *psqB,
        square  sqEnP,
        int     fInvert
        )
        {
        square sqwk, sqw1, sqbk, sqMask;

        sqwk = SqFindKing (psqW);
        sqw1 = SqFindOne  (psqW, piw1);
        sqbk = SqFindKing (psqB);

        if (x_piecePawn == piw1)
            sqMask = rgsqReflectMaskY [sqwk] ^ rgsqReflectInvertMask [fInvert];
        else
            sqMask = rgsqReflectMaskYandX [sqwk];
        sqwk ^= sqMask;
        sqbk ^= sqMask;
        sqw1 ^= sqMask;

        if (x_piecePawn != piw1)
            {
            // No pawn
            if (! FInTriangle (sqwk, sqbk))
                {
                sqwk = reflect_xy(sqwk);
                sqbk = reflect_xy(sqbk);
                sqw1 = reflect_xy(sqw1);
                };
            }
        return TEnumerate1<piw1,x_piecePawn==piw1 ? true : false,false>::Index(sqwk,sqw1,sqbk);
        }

    static INDEX TB_FASTCALL IndCalcB
        (
        square  *psqW,
        square  *psqB,
        square  sqEnP,
        int     fInvert
        )
        {
        square sqwk, sqw1, sqbk, sqMask;

        sqwk = SqFindKing (psqW);
        sqw1 = SqFindOne  (psqW, piw1);
        sqbk = SqFindKing (psqB);

        if (x_piecePawn == piw1)
            sqMask = rgsqReflectMaskY [sqbk] ^ rgsqReflectInvertMask [fInvert];
        else
            sqMask = rgsqReflectMaskYandX [sqbk];
        sqwk ^= sqMask;
        sqbk ^= sqMask;
        sqw1 ^= sqMask;

        if (x_piecePawn == piw1)
            return TEnumerate1<x_pieceNone,true,true>::Index(sqbk,sqw1,sqwk);
        else
            {
            // No pawn
            if (! FInTriangle (sqbk, sqwk))
                {
                sqwk = reflect_xy(sqwk);
                sqbk = reflect_xy(sqbk);
                sqw1 = reflect_xy(sqw1);
                };
            return IndTriKings(sqbk,sqwk)*i62 + EXCLUDE2(sqw1,sqwk,sqbk);
            }
        }
    };

template <int piw1, int pib1> class T22
    {
public:
    static INDEX TB_FASTCALL IndCalcW
        (
        square  *psqW,
        square  *psqB,
        square  sqEnP,
        int     fInvert
        )
        {
        square sqwk, sqw1, sqbk, sqb1, sqMask;

        sqwk = SqFindKing (psqW);
        sqw1 = SqFindOne  (psqW, piw1);
        sqbk = SqFindKing (psqB);
        sqb1 = SqFindOne  (psqB, pib1);

        if (x_piecePawn == pib1)
            sqMask = rgsqReflectMaskY [sqwk] ^ rgsqReflectInvertMask [fInvert];
        else
            sqMask = rgsqReflectMaskYandX [sqwk];
        sqwk ^= sqMask;
        sqbk ^= sqMask;
        sqw1 ^= sqMask;
        sqb1 ^= sqMask;

        if (x_piecePawn == pib1)
            {
            // There are pawns on the board
            if (x_piecePawn == piw1)
                {
                // One white and one black pawn
                if (XX == sqEnP)
                    return  TEnumerate1<x_piecePawn,true,false>::Index(sqwk,sqw1,sqbk)*i47 + 
                            EXCLUDE1(sqb1,sqw1)-i8; // 47
                else
                    return  rgcSinglePawnPresent[x_piecePawn]*i47 +
                            IndHalfKings(sqwk,sqbk)*i14 +
                            IndEnPassant11W (sqw1, sqb1, sqEnP ^ sqMask);
                }
            else
                // Only black pawn
                return  TEnumerate1<piw1,true,false>::Index(sqwk,sqw1,sqbk)*i48 + sqb1-i8;
            }
        else
            {
            // No pawns at all
            if (!FInTriangle (sqwk, sqbk))
                {
                sqwk = reflect_xy(sqwk);
                sqbk = reflect_xy(sqbk);
                sqw1 = reflect_xy(sqw1);
                sqb1 = reflect_xy(sqb1);
                };
            return  TEnumerate1<piw1,false,false>::Index(sqwk,sqw1,sqbk)*i61 +
                    EXCLUDE3(sqb1,sqwk,sqbk,sqw1);  // 61
            }
        }

    static INDEX TB_FASTCALL IndCalcB
        (
        square  *psqW,
        square  *psqB,
        square  sqEnP,
        int     fInvert
        )
        {
        square sqwk, sqw1, sqbk, sqb1, sqMask;

        sqwk = SqFindKing (psqW);
        sqw1 = SqFindOne  (psqW, piw1);
        sqbk = SqFindKing (psqB);
        sqb1 = SqFindOne  (psqB, pib1);

        if (x_piecePawn == pib1)
            sqMask = rgsqReflectMaskY [sqbk] ^ rgsqReflectInvertMask [fInvert];
        else
            sqMask = rgsqReflectMaskYandX [sqbk];
        sqwk ^= sqMask;
        sqbk ^= sqMask;
        sqw1 ^= sqMask;
        sqb1 ^= sqMask;

        if (x_piecePawn == pib1)
            {
            // There are pawns on the board
            if (x_piecePawn == piw1)
                {
                // One white and one black pawn
                if (XX == sqEnP)
                    return  TEnumerate1<x_piecePawn,true,true>::Index(sqbk,sqb1,sqwk)*i47 + 
                            EXCLUDE1(sqw1,sqb1)-i8; // 47
                else
                    return  rgcSinglePawnPresent[x_piecePawn]*i47 +
                            IndHalfKings(sqbk,sqwk)*i14 +
                            IndEnPassant11B (sqw1, sqb1, sqEnP ^ sqMask);
                }
            }
        else
            {
            // No pawns at all
            if (!FInTriangle (sqbk, sqwk))
                {
                sqwk = reflect_xy(sqwk);
                sqbk = reflect_xy(sqbk);
                sqw1 = reflect_xy(sqw1);
                sqb1 = reflect_xy(sqb1);
                };
            }
        return  (x_piecePawn == pib1 ?  TEnumerate1<pib1,true,true>::Index(sqbk,sqb1,sqwk) :
                                        TEnumerate1<pib1,false,false>::Index(sqbk,sqb1,sqwk))*i61 +
                EXCLUDE3(sqw1,sqwk,sqbk,sqb1);  // 61
        }
    };

template <int piw1, int piw2> class T31
    {
public:
    static INDEX TB_FASTCALL IndCalcW
        (
        square  *psqW,
        square  *psqB,
        square  sqEnP,
        int     fInvert
        )
        {
        square sqwk, sqw1, sqw2, sqbk, sqMask;

        sqwk = SqFindKing (psqW);
        if (piw1 == piw2)
            {
            sqw1 = SqFindFirst  (psqW, piw1);
            sqw2 = SqFindSecond (psqW, piw2);
            }
        else
            {
            SqFind2 (psqW, piw1, sqw1, piw2, sqw2);
            }
        sqbk = SqFindKing (psqB);

        if (x_piecePawn == piw2)
            sqMask = rgsqReflectMaskY [sqwk] ^ rgsqReflectInvertMask [fInvert];
        else
            sqMask = rgsqReflectMaskYandX [sqwk];
        sqwk ^= sqMask;
        sqbk ^= sqMask;
        sqw1 ^= sqMask;
        sqw2 ^= sqMask;

        if (x_piecePawn != piw2)
            {
            // There are no pawns on the board
            if (!FInTriangle (sqwk, sqbk))
                {
                sqwk = reflect_xy(sqwk);
                sqw1 = reflect_xy(sqw1);
                sqw2 = reflect_xy(sqw2);
                sqbk = reflect_xy(sqbk);
                };
            }
        return TEnumerate2<piw1, piw2, x_piecePawn==piw2 ? true : false, false>::Index(sqwk, sqw1, sqw2, sqbk);
        }

    static INDEX TB_FASTCALL IndCalcB
        (
        square  *psqW,
        square  *psqB,
        square  sqEnP,
        int     fInvert
        )
        {
        square sqwk, sqw1, sqw2, sqbk, sqMask;

        sqwk = SqFindKing (psqW);
        if (piw1 == piw2)
            {
            sqw1 = SqFindFirst (psqW, piw1);
            sqw2 = SqFindSecond (psqW, piw2);
            }
        else
            {
            SqFind2 (psqW, piw1, sqw1, piw2, sqw2);
            }
        sqbk = SqFindKing (psqB);

        if (x_piecePawn == piw2)
            sqMask = rgsqReflectMaskY [sqbk] ^ rgsqReflectInvertMask [fInvert];
        else
            sqMask = rgsqReflectMaskYandX [sqbk];
        sqwk ^= sqMask;
        sqbk ^= sqMask;
        sqw1 ^= sqMask;
        sqw2 ^= sqMask;

        if (x_piecePawn == piw2)
            {
            // There are pawns on the board
            if (x_piecePawn == piw1)
                // Two white pawns
                return  TEnumerate2<x_pieceNone,x_pieceNone,true,true>::Index(sqbk,sqw1,sqw2,sqwk);
            else
                // Only one white pawn
                return  TEnumerate1<x_pieceNone,true,true>::Index(sqbk,sqw2,sqwk)*i61 +
                        EXCLUDE3(sqw1,sqwk,sqbk,sqw2);  // 61
            }
        else
            {
            // No pawns
            if (!FInTriangle (sqbk, sqwk))
                {
                sqwk = reflect_xy(sqwk);
                sqw1 = reflect_xy(sqw1);
                sqw2 = reflect_xy(sqw2);
                sqbk = reflect_xy(sqbk);
                };
            if (piw1 == piw2)
                {
                SORT (sqw1, sqw2);
                sqw2 = EXCLUDE2(sqw2,sqwk,sqbk);                    // 62
                return  IndTriKings(sqbk,sqwk)*(i62*i61/2) +
                        sqw2*(sqw2-1)/2+EXCLUDE2(sqw1,sqwk,sqbk);   // 62*61/2
                }
            else
                return  IndTriKings(sqbk,sqwk)*(i62*i61) +
                        EXCLUDE2(sqw1,sqwk,sqbk)*i61 +  // 62
                        EXCLUDE3(sqw2,sqwk,sqbk,sqw1);  // 61
            }
        }
    };

template <int piw1, int piw2, int pib1> class T32
    {
public:
    static INDEX TB_FASTCALL IndCalcW
        (
        square  *psqW,
        square  *psqB,
        square  sqEnP,
        int     fInvert
        )
        {
        square sqwk, sqw1, sqw2, sqbk, sqb1, sqMask;

        sqwk = SqFindKing (psqW);
        if (piw1 == piw2)
            {
            sqw1 = SqFindFirst  (psqW, piw1);
            sqw2 = SqFindSecond (psqW, piw2);
            }
        else
            {
            SqFind2 (psqW, piw1, sqw1, piw2, sqw2);
            }
        sqbk = SqFindKing (psqB);
        sqb1 = SqFindOne  (psqB, pib1);

        if (x_piecePawn == piw2 || x_piecePawn == pib1)
            sqMask = rgsqReflectMaskY [sqwk] ^ rgsqReflectInvertMask [fInvert];
        else
            sqMask = rgsqReflectMaskYandX [sqwk];
        sqwk ^= sqMask;
        sqbk ^= sqMask;
        sqw1 ^= sqMask;
        sqw2 ^= sqMask;
        sqb1 ^= sqMask;

        if (x_piecePawn == piw2 || x_piecePawn == pib1)
            {
            // There are pawns on the board
            if (x_piecePawn == pib1)
                {
                // Black pawn
                if (x_piecePawn == piw1 && x_piecePawn == piw2)
                    {
                    // All 3 pieces are pawns
                    if (XX == sqEnP)
                        return  TEnumerate2<x_piecePawn,x_piecePawn,true,false>::
                                    Index(sqwk,sqw1,sqw2,sqbk)*i46+
                                EXCLUDE2(sqb1,sqw1,sqw2)-i8;    // 46
                    else
                        // En passant capture
                        return  rgcPairPawnPresent[x_piecePawn][x_piecePawn]*i46 +
                                IndHalfKings(sqwk,sqbk)*(i14*i44) +
                                IndEnPassant21W (sqw1, sqw2, sqb1, sqEnP ^ sqMask);
                    }
                else if (x_piecePawn == piw2)
                    {
                    // One white pawn, one black pawn
                    if (XX == sqEnP)
                        return  TEnumerate2<piw1,x_piecePawn,true,false>::
                                    Index(sqwk,sqw1,sqw2,sqbk)*i47 +
                                EXCLUDE1(sqb1,sqw2)-i8; // 47
                    else
                        // En passant capture
                        return  rgcPairPawnPresent[piw1][x_piecePawn]*i47 +
                                TEnumerate1<piw1,true,false>::Index(sqwk,sqw1,sqbk)*i14 +
                                IndEnPassant11W (sqw2, sqb1, sqEnP ^ sqMask);
                    }
                else
                    // Only black pawn
                    return  TEnumerate2<piw1,piw2,true,false>::
                                Index(sqwk,sqw1,sqw2,sqbk)*i48 +
                            sqb1-i8;    // 48
                }
            }
        else
            {
            // No pawns
            if (!FInTriangle (sqwk, sqbk))
                {
                sqwk = reflect_xy(sqwk);
                sqw1 = reflect_xy(sqw1);
                sqw2 = reflect_xy(sqw2);
                sqbk = reflect_xy(sqbk);
                sqb1 = reflect_xy(sqb1);
                };
            }
        return  TEnumerate2<piw1,piw2,(x_piecePawn==piw2||x_piecePawn==pib1) ? true : false,false>::
                    Index(sqwk,sqw1,sqw2,sqbk)*i60 +
                EXCLUDE4(sqb1,sqwk,sqbk,sqw1,sqw2); // 60
        }

    static INDEX TB_FASTCALL IndCalcB
        (
        square  *psqW,
        square  *psqB,
        square  sqEnP,
        int     fInvert
        )
        {
        square sqwk, sqw1, sqw2, sqbk, sqb1, sqMask;

        sqwk = SqFindKing (psqW);
        if (piw1 == piw2)
            {
            sqw1 = SqFindFirst  (psqW, piw1);
            sqw2 = SqFindSecond (psqW, piw2);
            }
        else
            {
            SqFind2 (psqW, piw1, sqw1, piw2, sqw2);
            }
        sqbk = SqFindKing (psqB);
        sqb1 = SqFindOne  (psqB, pib1);

        if (x_piecePawn == piw2 || x_piecePawn == pib1)
            sqMask = rgsqReflectMaskY [sqbk] ^ rgsqReflectInvertMask [fInvert];
        else
            sqMask = rgsqReflectMaskYandX [sqbk];
        sqwk ^= sqMask;
        sqbk ^= sqMask;
        sqw1 ^= sqMask;
        sqw2 ^= sqMask;
        sqb1 ^= sqMask;

        if (x_piecePawn == piw2 || x_piecePawn == pib1)
            {
            // There are pawns on the board
            if (x_piecePawn == pib1)
                {
                // Black pawn
                if (x_piecePawn == piw1 && x_piecePawn == piw2)
                    {
                    // All 3 pieces are pawns
                    SORT (sqw1, sqw2);
                    if (XX == sqEnP)
                        {
                        sqw2 = EXCLUDE1(sqw2,sqb1)-i8;                  // 47
                        return  TEnumerate1<x_piecePawn,true,true>::
                                    Index(sqbk,sqb1,sqwk)*(i47*i46/2) +
                                sqw2*(sqw2-1)/2+EXCLUDE1(sqw1,sqb1)-i8; // 47*46/2
                        }
                    else
                        // En passant capture
                        return  rgcSinglePawnPresent[x_piecePawn]*(i47*i46/2) +
                                IndHalfKings(sqbk,sqwk)*(i44*i14) +
                                IndEnPassant21B (sqw1, sqw2, sqb1, sqEnP ^ sqMask);
                    }
                else if (x_piecePawn == piw2)
                    {
                    // One white pawn, one black pawn
                    if (XX == sqEnP)
                        return  TEnumerate1<x_piecePawn,true,true>::
                                    Index(sqbk,sqb1,sqwk)*(i60*i47) +
                                EXCLUDE4(sqw1,sqwk,sqbk,sqw2,sqb1)*i47 +    // 60
                                EXCLUDE1(sqw2,sqb1)-i8;                     // 47
                    else
                        {
                        // En passant capture
                        sqEnP ^= sqMask;
                        return  rgcSinglePawnPresent[x_piecePawn]*(i60*i47) +
                                IndHalfKings(sqbk,sqwk)*(i58*i14) +
                                EXCLUDE6(sqw1,sqwk,sqbk,sqw2,sqb1,sqEnP,sqEnP-8)*i14 +  // 58
                                IndEnPassant11B (sqw2, sqb1, sqEnP);
                        }
                    }
                else
                    {
                    // Only black pawn
                    if (piw1 == piw2)
                        {
                        // 2 identical white pieces
                        SORT (sqw1, sqw2);
                        sqw2 = EXCLUDE3(sqw2,sqwk,sqbk,sqb1);                       // 61
                        return  TEnumerate1<x_piecePawn,true,true>::
                                    Index(sqbk,sqb1,sqwk)*(i61*i60/2) +
                                sqw2*(sqw2-1)/2 + EXCLUDE3(sqw1,sqwk,sqbk,sqb1);    // 61*60/2
                        }
                    return  TEnumerate1<x_piecePawn,true,true>::
                                Index(sqbk,sqb1,sqwk)*(i61*i60) +
                            EXCLUDE3(sqw1,sqwk,sqbk,sqb1)*i60 + // 61
                            EXCLUDE4(sqw2,sqwk,sqbk,sqw1,sqb1); // 60
                    }
                }
            else
                {
                // No black pawn
                if (x_piecePawn == piw1)
                    {
                    // Only 2 white pawns
                    SORT (sqw1, sqw2);
                    sqw2 -= i8;
                    return  TEnumerate1<pib1,true,true>::
                                Index(sqbk,sqb1,sqwk)*(i48*47/2) +
                            sqw2*(sqw2-1)/2+sqw1-i8;            // 48*47/2
                    }
                else
                    // Only one white pawn
                    return  TEnumerate1<pib1,true,true>::
                                Index(sqbk,sqb1,sqwk)*(i60*i48) +
                            EXCLUDE4(sqw1,sqwk,sqbk,sqw2,sqb1)*i48 +    // 60
                            sqw2-i8;                                    // 48
                }
            }
        else
            {
            // No pawns
            if (!FInTriangle (sqbk, sqwk))
                {
                sqwk = reflect_xy(sqwk);
                sqw1 = reflect_xy(sqw1);
                sqw2 = reflect_xy(sqw2);
                sqbk = reflect_xy(sqbk);
                sqb1 = reflect_xy(sqb1);
                };
            if (piw1 == piw2)
                {
                // 2 identical white pieces
                SORT (sqw1, sqw2);
                sqw2 = EXCLUDE3(sqw2,sqwk,sqbk,sqb1);                   // 61
                return  TEnumerate1<pib1,false,false>::
                            Index(sqbk,sqb1,sqwk)*(i61*i60/2) +
                        sqw2*(sqw2-1)/2+EXCLUDE3(sqw1,sqwk,sqbk,sqb1);  // 61*60/2
                }
            else
                return  TEnumerate1<pib1,false,false>::
                            Index(sqbk,sqb1,sqwk)*(i61*i60) +
                        EXCLUDE3(sqw1,sqwk,sqbk,sqb1)*i60 + // 61
                        EXCLUDE4(sqw2,sqwk,sqbk,sqw1,sqb1); // 60
            }
        }
    };

#if defined (T41_INCLUDE)

template <int piw1, int piw2, int piw3> class T41
    {
public:
    static INDEX TB_FASTCALL IndCalcW
        (
        square  *psqW,
        square  *psqB,
        square  sqEnP,
        int     fInvert
        )
        {
        square sqwk, sqw1, sqw2, sqw3, sqbk, sqMask;

        sqwk = SqFindKing (psqW);
        sqw1 = SqFindFirst (psqW, piw1);
        if (piw1 == piw2 && piw2 == piw3)
            {
            sqw2 = SqFindSecond (psqW, piw2);
            sqw3 = SqFindThird (psqW, piw3);
            }
        else if (piw1 == piw2)
            {
            sqw2 = SqFindSecond (psqW, piw2);
            sqw3 = SqFindFirst (psqW, piw3);
            }
        else if (piw2 == piw3)
            {
            sqw2 = SqFindFirst (psqW, piw2);
            sqw3 = SqFindSecond (psqW, piw3);
            }
        else
            {
            sqw2 = SqFindFirst (psqW, piw2);
            sqw3 = SqFindFirst (psqW, piw3);
            }
        sqbk = SqFindKing (psqB);

        if (x_piecePawn == piw3)
            sqMask = rgsqReflectMaskY [sqwk] ^ rgsqReflectInvertMask [fInvert];
        else
            sqMask = rgsqReflectMaskYandX [sqwk];
        sqwk ^= sqMask;
        sqbk ^= sqMask;
        sqw1 ^= sqMask;
        sqw2 ^= sqMask;
        sqw3 ^= sqMask;

        if (x_piecePawn != piw3)
            {
            // No pawns
            if (!FInTriangle (sqwk, sqbk))
                {
                sqwk = reflect_xy(sqwk);
                sqw1 = reflect_xy(sqw1);
                sqw2 = reflect_xy(sqw2);
                sqw3 = reflect_xy(sqw3);
                sqbk = reflect_xy(sqbk);
                };
            }
        return  TEnumerate3<piw1,piw2,piw3,x_piecePawn == piw3,false>::Index(sqwk,sqw1,sqw2,sqw3,sqbk);
        }

    static INDEX TB_FASTCALL IndCalcB
        (
        square  *psqW,
        square  *psqB,
        square  sqEnP,
        int     fInvert
        )
        {
        square sqwk, sqw1, sqw2, sqw3, sqbk, sqMask;

        sqwk = SqFindKing (psqW);
        sqw1 = SqFindFirst (psqW, piw1);
        if (piw1 == piw2 && piw2 == piw3)
            {
            sqw2 = SqFindSecond (psqW, piw2);
            sqw3 = SqFindThird (psqW, piw3);
            }
        else if (piw1 == piw2)
            {
            sqw2 = SqFindSecond (psqW, piw2);
            sqw3 = SqFindFirst (psqW, piw3);
            }
        else if (piw2 == piw3)
            {
            sqw2 = SqFindFirst (psqW, piw2);
            sqw3 = SqFindSecond (psqW, piw3);
            }
        else
            {
            sqw2 = SqFindFirst (psqW, piw2);
            sqw3 = SqFindFirst (psqW, piw3);
            }
        sqbk = SqFindKing (psqB);

        if (x_piecePawn == piw3)
            sqMask = rgsqReflectMaskY [sqbk] ^ rgsqReflectInvertMask [fInvert];
        else
            sqMask = rgsqReflectMaskYandX [sqbk];
        sqwk ^= sqMask;
        sqbk ^= sqMask;
        sqw1 ^= sqMask;
        sqw2 ^= sqMask;
        sqw3 ^= sqMask;

        if (x_piecePawn == piw3)
            {
            // There are pawns on the board
            if (x_piecePawn == piw1)
                // 3 white pawns
                return  TEnumerate3<x_pieceNone,x_pieceNone,x_pieceNone,true,true>::
                            Index(sqbk,sqw1,sqw2,sqw3,sqwk);
            else if (x_piecePawn == piw2)
                // 2 white pawns
                return  TEnumerate2<x_pieceNone,x_pieceNone,true,true>::
                            Index(sqbk,sqw2,sqw3,sqwk)*i60 +
                        EXCLUDE4(sqw1,sqwk,sqbk,sqw2,sqw3); // 60
            else if (piw1 == piw2)
                {
                // 1 pawn, 2 pieces equal
                SORT (sqw1, sqw2);
                sqw2 = EXCLUDE3(sqw2,sqwk,sqbk,sqw3);                   // 61
                return  TEnumerate1<x_pieceNone,true,true>::
                            Index(sqbk,sqw3,sqwk)*(i61*i60/2) +
                        sqw2*(sqw2-1)/2+EXCLUDE3(sqw1,sqwk,sqbk,sqw3);  // 61*60/2
                }
            else
                // Only one white pawn
                return  TEnumerate1<x_pieceNone,true,true>::Index(sqbk,sqw3,sqwk)*i61*i60 +
                        EXCLUDE3(sqw1,sqwk,sqbk,sqw3)*i60 + // 61
                        EXCLUDE4(sqw2,sqwk,sqbk,sqw1,sqw3); // 60
            }
        else
            {
            // No pawns
            if (!FInTriangle (sqbk, sqwk))
                {
                sqwk = reflect_xy(sqwk);
                sqw1 = reflect_xy(sqw1);
                sqw2 = reflect_xy(sqw2);
                sqw3 = reflect_xy(sqw3);
                sqbk = reflect_xy(sqbk);
                };
            if (piw1 == piw2 && piw2 == piw3)
                {
                // All 3 pieces equal
                SORT (sqw1, sqw2);
                SORT (sqw2, sqw3);
                SORT (sqw1, sqw2);
                sqw3 = EXCLUDE2(sqw3,sqwk,sqbk);    // 62
                sqw2 = EXCLUDE2(sqw2,sqwk,sqbk);
                return  IndTriKings(sqbk,sqwk)*(i62*i61*i60/6) +
                        sqw3*(sqw3-1)*(sqw3-2)/6+
                        sqw2*(sqw2-1)/2+
                        EXCLUDE2(sqw1,sqwk,sqbk);   // 62*61*60/6
                }
            else if (piw1 == piw2)
                {
                // 2 major pieces equal
                SORT (sqw1, sqw2);
                sqw2 = EXCLUDE3(sqw2,sqwk,sqbk,sqw3);                           // 61
                return  IndTriKings(sqbk,sqwk)*(i61*i60/2*i62) +
                        (sqw2*(sqw2-1)/2+EXCLUDE3(sqw1,sqwk,sqbk,sqw3))*i62 +   // 61*60/2
                        EXCLUDE2(sqw3,sqwk,sqbk);                               // 62
                }
            else if (piw2 == piw3)
                {
                // 2 minor pieces equal
                SORT (sqw2, sqw3);
                sqw3 = EXCLUDE3(sqw3,sqwk,sqbk,sqw1);                   // 61
                return  IndTriKings(sqbk,sqwk)*(i62*i61*i60/2) +
                        EXCLUDE2(sqw1,sqwk,sqbk)*(i61*i60/2) +          // 62
                        sqw3*(sqw3-1)/2+EXCLUDE3(sqw2,sqwk,sqbk,sqw1);  // 61*60/2
                }
            else
                return  IndTriKings(sqbk,sqwk)*(i62*i61*i60) +
                        EXCLUDE2(sqw1,sqwk,sqbk)*(i61*i60) +    // 62
                        EXCLUDE3(sqw2,sqwk,sqbk,sqw1)*i60 +     // 61
                        EXCLUDE4(sqw3,sqwk,sqbk,sqw1,sqw2);     // 60
            }
        }
    };

#endif  // T41

#if defined (ILLEGAL_POSSIBLE)

#define CHECK_INF_SINGLE(ind)\
        if (INF_SINGLE == ind)\
            return (INDEX) -1;\

#define CHECK_INF_PAIR(ind)\
        if (INF_PAIR == ind)\
            return (INDEX) -1;\

#define CHECK_INF_TRIPLE(ind)\
        if (INF_TRIPLE == ind)\
            return (INDEX) -1;\

#else

#define CHECK_INF_SINGLE(ind)
#define CHECK_INF_PAIR(ind)
#define CHECK_INF_TRIPLE(ind)

#endif

#if defined (T33_INCLUDE)

template <int piw1, int piw2, int pib1, int pib2> class T33
{
public:
    static INDEX TB_FASTCALL IndCalcW
        (
        square  *psqW,
        square  *psqB,
        square  sqEnP,
        int     fInvert
        )
    {
        square   sqwk, sqw1, sqw2, sqbk, sqb1, sqb2, sqMask;
        unsigned uInd;

        sqwk = SqFindKing (psqW);
        if (piw1 == piw2) {
            sqw1 = SqFindFirst  (psqW, piw1);
            sqw2 = SqFindSecond (psqW, piw2);
            }
        else {
            SqFind2 (psqW, piw1, sqw1, piw2, sqw2);
            }
        sqbk = SqFindKing (psqB);
        if (pib1 == pib2) {
            sqb1 = SqFindFirst  (psqB, pib1);
            sqb2 = SqFindSecond (psqB, pib2);
            }
        else {
            SqFind2 (psqB, pib1, sqb1, pib2, sqb2);
            }

        if(x_piecePawn == piw2 || x_piecePawn == pib2)
            sqMask = rgsqReflectMaskY[sqwk] ^ rgsqReflectInvertMask[fInvert];
        else
            sqMask = rgsqReflectMaskYandX[sqwk];

        sqwk ^= sqMask;
        sqbk ^= sqMask;
        sqw1 ^= sqMask;
        sqw2 ^= sqMask;
        sqb1 ^= sqMask;
        sqb2 ^= sqMask;

        if(x_piecePawn == piw2 || x_piecePawn == pib2) {
            // There are pawns on the board
            if(XX == sqEnP) {
                // No En Passant possible
                uInd = TEnumerate2<piw1,piw2,true,false>::Index(sqwk,sqw1,sqw2,sqbk);
                CHECK_INF_PAIR(uInd);
                if(x_piecePawn == piw1) {
                    // all 4 pieces are pawns
                    SORT(sqb1, sqb2);
                    sqb2 = EXCLUDE2(sqb2,sqw1,sqw2) - i8;
                    return uInd*(INDEX)(i46*i45/2) +
                           sqb2*(sqb2-1)/2 + EXCLUDE2(sqb1,sqw1,sqw2) - i8;
                }
                else if(x_piecePawn == piw2) {
                    if(x_piecePawn == pib1) {
                        // 1 white, 2 black pawns
                        SORT(sqb1, sqb2);
                        sqb2 = EXCLUDE1(sqb2,sqw2) - i8;
                        return uInd*(INDEX)(i47*i46/2) +
                               sqb2*(sqb2-1)/2 + EXCLUDE1(sqb1,sqw2) - i8;
                    }
                    else if(x_piecePawn == pib2) {
                        // 1 white, 1 black pawn
                        return uInd*(INDEX)(i59*i47) +
                               (EXCLUDE1(sqb2,sqw2)-i8)*i59 +
                               EXCLUDE5(sqb1,sqb2,sqwk,sqw1,sqw2,sqbk);
                    }
                    else if(pib2 == pib1) {
                        // only 1 white pawn, two identical black pieces
                        SORT(sqb1, sqb2);
                        sqb2 = EXCLUDE4(sqb2,sqwk,sqw1,sqw2,sqbk);
                        return uInd*(INDEX)(i60*i59/2) +
                               sqb2*(sqb2-1)/2 +
                               EXCLUDE4(sqb1,sqwk,sqw1,sqw2,sqbk);
                    }
                    else {
                        // only 1 white pawn, two different black pieces
                        return uInd*(INDEX)(i60*i59) +
                               EXCLUDE4(sqb2,sqwk,sqw1,sqw2,sqbk)*i59 +
                               EXCLUDE5(sqb1,sqb2,sqwk,sqw1,sqw2,sqbk);
                    }
                }
                else if(x_piecePawn == pib1) {
                    // no white pawns, two black pawns
                    SORT(sqb1, sqb2);
                    sqb2 -= i8;
                    return uInd*(INDEX)(i48*i47/2) +
                           sqb2*(sqb2-1)/2 + sqb1 - i8;
                }
                else {
                    // no white pawns, 1 black pawn
                    return uInd*(INDEX)(i48*i59) +
                           (sqb2 - i8)*i59 +
                           EXCLUDE5(sqb1,sqb2,sqwk,sqw1,sqw2,sqbk);
                }
            }
            else {
                // En Passant possible
                sqEnP ^= sqMask;
                if(x_piecePawn == piw1) {
                    // all 4 pieces are pawns
                    uInd = rgcPairPawnPresent[x_piecePawn][x_piecePawn];
                    return uInd*(INDEX)(i46*i45/2) +
                           IndHalfKings(sqwk,sqbk)*(i14*i44*i43) +
                           IndEnPassant22W(sqw1, sqw2, sqb1, sqb2, sqEnP);
                }
                else {
                    assert(x_piecePawn == piw2);

                    uInd = TEnumerate1<piw1, true, false>::
                           Index(sqwk,sqw1,sqbk);
                    CHECK_INF_SINGLE(uInd);

                    if(x_piecePawn == pib1) {
                        // 1 white pawn, 2 black pawns
                        return rgcPairPawnPresent[piw1][x_piecePawn]*((INDEX)(i47*i46/2)) +
                               (uInd*(i14*i44) +
                                IndEnPassant12W(sqw2, sqb1, sqb2, sqEnP));
                    }
                    else {
                        // 1 white pawn, 1 black pawn
                        assert(x_piecePawn == pib2);
                        return rgcPairPawnPresent[piw1][x_piecePawn]*((INDEX)(i47*i59))+
                               (uInd*(i14*i57) +
                                 EXCLUDE7(sqb1,sqb2,sqwk,sqw1,sqw2,sqbk,sqEnP,sqEnP+8)*i14+
                                 IndEnPassant11W(sqw2,sqb2,sqEnP));
                    }
                }
            }
        }
        else {
            // no pawns at all
            if (!FInTriangle (sqwk, sqbk)) {
                sqwk = reflect_xy(sqwk);
                sqw1 = reflect_xy(sqw1);
                sqw2 = reflect_xy(sqw2);
                sqbk = reflect_xy(sqbk);
                sqb1 = reflect_xy(sqb1);
                sqb2 = reflect_xy(sqb2);
                }

            uInd = TEnumerate2<piw1,piw2,false,false>::Index(sqwk,sqw1,sqw2,sqbk);
            CHECK_INF_PAIR(uInd);
            if (pib1 == pib2) {
                SORT (sqb1, sqb2);
                sqb2 = EXCLUDE4(sqb2,sqwk,sqbk,sqw1,sqw2);
                return  uInd*(i60*i59/2) +
                        sqb2*(sqb2-1)/2+
                        EXCLUDE4(sqb1,sqwk,sqbk,sqw1,sqw2);
                }
            else {
                // Divide by 2 to avoid overflow on the 32-bit systems, later
                // add to itself to produce the correct result. Only the final
                // addition have to be done using 64-bit arithmetic.
                uInd *= (i60*i59/2);
                return  ((INDEX) uInd) +
                          (INDEX) (uInd +
                            EXCLUDE4(sqb1,sqwk,sqbk,sqw1,sqw2)*i59 +
                            EXCLUDE5(sqb2,sqwk,sqbk,sqw1,sqw2,sqb1));
                }
            }
    }

    static INDEX TB_FASTCALL IndCalcB
        (
        square  *psqW,
        square  *psqB,
        square  sqEnP,
        int     fInvert
        )
    {
        square   sqwk, sqw1, sqw2, sqbk, sqb1, sqb2, sqMask;
        unsigned uInd;

        sqwk = SqFindKing (psqW);
        if (piw1 == piw2) {
            sqw1 = SqFindFirst  (psqW, piw1);
            sqw2 = SqFindSecond (psqW, piw2);
            }
        else {
            SqFind2 (psqW, piw1, sqw1, piw2, sqw2);
            }
        sqbk = SqFindKing (psqB);
        if (pib1 == pib2) {
            sqb1 = SqFindFirst  (psqB, pib1);
            sqb2 = SqFindSecond (psqB, pib2);
            }
        else {
            SqFind2 (psqB, pib1, sqb1, pib2, sqb2);
            }

        if(piw2 == x_piecePawn || pib2 == x_piecePawn)
            sqMask = rgsqReflectMaskY[sqbk] ^ rgsqReflectInvertMask[fInvert];
        else
            sqMask = rgsqReflectMaskYandX[sqbk];

        sqwk ^= sqMask;
        sqw1 ^= sqMask;
        sqw2 ^= sqMask;
        sqbk ^= sqMask;
        sqb1 ^= sqMask;
        sqb2 ^= sqMask;

        if(x_piecePawn == piw2 || x_piecePawn == pib2) {
            // There are pawns on the board
            if(XX == sqEnP) {
                // No En Passant possible
                uInd = TEnumerate2<pib1,pib2,true,true>::Index(sqbk,sqb1,sqb2,sqwk);
                CHECK_INF_PAIR(uInd);
                if(x_piecePawn == piw1) {
                    // all 4 pieces are pawns
                    SORT(sqw1, sqw2);
                    sqw2 = EXCLUDE2(sqw2,sqb1,sqb2) - i8;
                    return uInd*(INDEX)(i46*i45/2) +
                           sqw2*(sqw2-1)/2 + EXCLUDE2(sqw1,sqb1,sqw2) - i8;
                }
                else if(x_piecePawn == piw2) {
                    if(x_piecePawn == pib1) {
                        // 1 white, 2 black pawns
                        return uInd*(INDEX)(i46*i59) +
                               (EXCLUDE2(sqw2,sqb1,sqb2)-i8)*i59 +
                               EXCLUDE5(sqw1,sqw2,sqb1,sqb2,sqwk,sqbk);
                    }
                    else if(x_piecePawn == pib2) {
                        // 1 white, 1 black pawn
                        return uInd*(INDEX)(i47*i59) +
                               (EXCLUDE1(sqw2,sqb2)-i8)*i59 +
                               EXCLUDE5(sqw1,sqw2,sqb1,sqb2,sqwk,sqbk);
                    }
                    else {
                        // only 1 white pawn
                        return uInd*(INDEX)(i48*i59) +
                               (sqw2 - i8)*i59 +
                               EXCLUDE5(sqw1,sqw2,sqb1,sqb2,sqwk,sqbk);
                    }
                }
                else if(piw1 == piw2) {
                    // no white pawns, two identical white pieces
                    SORT(sqw1, sqw2);
                    sqw2 = EXCLUDE4(sqw2,sqb1,sqb2,sqwk,sqbk);
                    return uInd*(INDEX)(i60*i59/2) +
                           sqw2*(sqw2-1)/2 +
                           EXCLUDE4(sqw1,sqb1,sqb2,sqwk,sqbk);
                }
                else {
                    // no white pawns, two different white pieces
                    return uInd*(INDEX)(i60*i59) +
                           EXCLUDE4(sqw2,sqb1,sqb2,sqwk,sqbk)*i59 +
                           EXCLUDE5(sqw1,sqw2,sqb1,sqb2,sqwk,sqbk);
                }
            }
            else {
                // En Passant possible
                sqEnP ^= sqMask;
                if(x_piecePawn == piw1) {
                    // all 4 pieces are pawns
                    uInd = rgcPairPawnPresent[x_piecePawn][x_piecePawn];
                    return uInd*(INDEX)(i46*i45/2) +
                           IndHalfKings(sqbk,sqwk)*(i14*i44*i43) +
                           IndEnPassant22B(sqw1, sqw2, sqb1, sqb2, sqEnP);
                }
                else if(x_piecePawn == pib1) {
                    // 1 white, 2 black pawns
                    assert(x_piecePawn == piw2);
                    uInd = rgcPairPawnPresent[x_piecePawn][x_piecePawn];
                    return uInd*(INDEX)(i46*i59) +
                           IndHalfKings(sqbk,sqwk)*(i14*i44*i57) +
                           EXCLUDE7(sqw1,sqw2,sqb1,sqb2,sqbk,sqwk,sqEnP,sqEnP-8)*(i44*i14)+
                           IndEnPassant12B(sqw2, sqb1, sqb2, sqEnP);
                }
                else {
                    // 1 white, 1 black pawn
                    assert(x_piecePawn == piw2 && x_piecePawn == pib2);
                    uInd = TEnumerate1<pib1, true, true>::
                           Index(sqbk,sqb1,sqwk);
                    CHECK_INF_SINGLE(uInd);
                    return rgcPairPawnPresent[pib1][x_piecePawn]*((INDEX)(i47*i59)) +
                           (uInd*(i14*i57) +
                            EXCLUDE7(sqw1,sqw2,sqb1,sqb2,sqwk,sqbk,sqEnP,sqEnP-8)*i14 +
                            IndEnPassant11B(sqw2,sqb2,sqEnP));
                }
            }
        }
        else {
            // no pawns at all
            if (!FInTriangle (sqbk, sqwk)) {
            sqwk = reflect_xy(sqwk);
            sqw1 = reflect_xy(sqw1);
            sqw2 = reflect_xy(sqw2);
            sqbk = reflect_xy(sqbk);
            sqb1 = reflect_xy(sqb1);
            sqb2 = reflect_xy(sqb2);
            }
            uInd = TEnumerate2<pib1,pib2,false,false>::Index(sqbk,sqb1,sqb2,sqwk);
            CHECK_INF_PAIR(uInd);
            if (piw1 == piw2) {
            SORT (sqw1, sqw2);
                sqw2 = EXCLUDE4(sqw2,sqbk,sqwk,sqb1,sqb2);
                return  uInd*(i60*i59/2) +
                    sqw2*(sqw2-1)/2+
                        EXCLUDE4(sqw1,sqbk,sqwk,sqb1,sqb2);
            }
            else {
            // Divide by 2 to avoid overflow on the 32-bit systems, later
            // add to itself to produce the correct result. Only the final
                // addition have to be done using 64-bit arithmetic.
                uInd *= (i60*i59/2);
                return  ((INDEX) uInd) +
                         (INDEX) (uInd +
                          EXCLUDE4(sqw1,sqbk,sqwk,sqb1,sqb2)*i59 +
                           EXCLUDE5(sqw2,sqbk,sqwk,sqb1,sqb2,sqw1));
            }
        }
    }

    // IndCalcBF should replace IndCalcB for symmetric endgames

    static INDEX TB_FASTCALL IndCalcBF
        (
        square  *psqW,
        square  *psqB,
        square  sqEnP,
        int fInvert
        )
    {
            return IndCalcW(psqB, psqW, sqEnP, !fInvert);
    }
};

#endif  // T33

#if defined (T42_INCLUDE)

// TODO: Add code for the TBs with pawns

template <int piw1, int piw2, int piw3, int pib1> class T42
    {
public:
    static INDEX TB_FASTCALL IndCalcW
        (
        square  *psqW,
        square  *psqB,
        square  sqEnP,
        int     fInvert
        )
        {
        unsigned uInd;
        square sqwk, sqw1, sqw2, sqw3, sqbk, sqb1, sqMask;

        sqwk = SqFindKing (psqW);
        sqw1 = SqFindFirst (psqW, piw1);
        if (piw1 == piw2 && piw2 == piw3)
            {
            sqw2 = SqFindSecond (psqW, piw2);
            sqw3 = SqFindThird (psqW, piw3);
            }
        else if (piw1 == piw2)
            {
            sqw2 = SqFindSecond (psqW, piw2);
            sqw3 = SqFindFirst (psqW, piw3);
            }
        else if (piw2 == piw3)
            {
            sqw2 = SqFindFirst (psqW, piw2);
            sqw3 = SqFindSecond (psqW, piw3);
            }
        else
            {
            sqw2 = SqFindFirst (psqW, piw2);
            sqw3 = SqFindFirst (psqW, piw3);
            }
        sqbk = SqFindKing (psqB);
        sqb1 = SqFindOne  (psqB, pib1);

        if (x_piecePawn == piw3 || x_piecePawn == pib1)
            sqMask = rgsqReflectMaskY [sqwk] ^ rgsqReflectInvertMask [fInvert];
        else
            sqMask = rgsqReflectMaskYandX [sqwk];
        sqwk ^= sqMask;
        sqbk ^= sqMask;
        sqw1 ^= sqMask;
        sqw2 ^= sqMask;
        sqw3 ^= sqMask;
        sqb1 ^= sqMask;

        if (x_piecePawn == piw3 || x_piecePawn == pib1)
            {
            // There are pawns on the board
            if (x_piecePawn == pib1)
                {
                // Black pawn
                if (x_piecePawn == piw1)
                    {
                    // All 4 pieces are pawns
                    if (XX == sqEnP)
                        {
                        uInd = TEnumerate3<x_piecePawn,x_piecePawn,x_piecePawn,true,false>::
                                Index(sqwk,sqw1,sqw2,sqw3,sqbk);
                        CHECK_INF_TRIPLE(uInd);
                        return uInd*i45 + EXCLUDE3(sqb1,sqw1,sqw2,sqw3)-i8; // 45
                        }
                    else
                        {
                        // En passant capture
                        uInd = rgcTriplePawnPresent[x_piecePawn][x_piecePawn][x_piecePawn];
                        return  uInd*i45 +
                                IndHalfKings(sqwk,sqbk)*(i14*i44*i43/2) +
                                IndEnPassant31W (sqw1, sqw2, sqw3, sqb1, sqEnP ^ sqMask);
                        }
                    }
                else if (x_piecePawn == piw2)
                    {
                    // Two white pawns, one black pawn
                    if (XX == sqEnP)
                        {
                        uInd = TEnumerate3<piw1,x_piecePawn,x_piecePawn,true,false>::
                                Index(sqwk,sqw1,sqw2,sqw3,sqbk);
                        CHECK_INF_TRIPLE(uInd);
                        return uInd*(INDEX)i46 + (EXCLUDE2(sqb1,sqw2,sqw3)-i8);   // 46
                        }
                    else
                        {
                        // En passant capture
                        uInd = TEnumerate1<piw1,true,false>::Index(sqwk,sqw1,sqbk);
                        CHECK_INF_SINGLE(uInd);
                        return  rgcTriplePawnPresent[piw1][x_piecePawn][x_piecePawn]*(INDEX)i46 +
                                (uInd*(i14*i44) + IndEnPassant21W (sqw2, sqw3, sqb1, sqEnP ^ sqMask));
                        }
                    }
                else if (x_piecePawn == piw3)
                    {
                    // One white pawn, one black pawn
                    if (XX == sqEnP)
                        {
                        uInd = TEnumerate3<piw1,piw2,x_piecePawn,true,false>::
                                Index(sqwk,sqw1,sqw2,sqw3,sqbk);
                        CHECK_INF_TRIPLE(uInd);
                        return  uInd*((INDEX)i47) + (EXCLUDE1(sqb1,sqw3)-i8);    // 47
                        }
                    else
                        {
                        // En passant capture
                        uInd = TEnumerate2<piw1,piw2,true,false>::Index(sqwk,sqw1,sqw2,sqbk);
                        CHECK_INF_PAIR(uInd);
                        return  rgcTriplePawnPresent[piw1][piw2][x_piecePawn]*(INDEX)i47 +
                                (uInd*i14 + IndEnPassant11W (sqw3, sqb1, sqEnP ^ sqMask));
                        }
                    }
                else
                    {
                    // Only black pawn
                    uInd = TEnumerate3<piw1,piw2,piw3,true,false>::
                            Index(sqwk,sqw1,sqw2,sqw3,sqbk);
                    CHECK_INF_TRIPLE(uInd);
                    return  (uInd*3)*(INDEX)16 + (sqb1-i8); // 48
                    }
                }
            else
                {
                // No black pawn
                uInd = TEnumerate3<piw1,piw2,piw3,true,false>::
                    Index(sqwk,sqw1,sqw2,sqw3,sqbk);
                CHECK_INF_TRIPLE(uInd);
                return uInd*(INDEX)i59 + EXCLUDE5(sqb1,sqwk,sqbk,sqw1,sqw2,sqw3);    // 59
                }
            }
        else
            {
            // No pawns
            if (!FInTriangle (sqwk, sqbk))
                {
                sqwk = reflect_xy(sqwk);
                sqw1 = reflect_xy(sqw1);
                sqw2 = reflect_xy(sqw2);
                sqw3 = reflect_xy(sqw3);
                sqbk = reflect_xy(sqbk);
                sqb1 = reflect_xy(sqb1);
                };
            uInd = TEnumerate3<piw1,piw2,piw3,false,false>::Index(sqwk,sqw1,sqw2,sqw3,sqbk);
            CHECK_INF_TRIPLE(uInd);
            return uInd*(INDEX)i59 + EXCLUDE5(sqb1,sqwk,sqbk,sqw1,sqw2,sqw3);   // 59
            }
        }

    static INDEX TB_FASTCALL IndCalcB
        (
        square  *psqW,
        square  *psqB,
        square  sqEnP,
        int     fInvert
        )
        {
        unsigned uInd;
        square sqwk, sqw1, sqw2, sqw3, sqbk, sqb1, sqMask, sqTemp;

        sqwk = SqFindKing (psqW);
        sqw1 = SqFindFirst (psqW, piw1);
        if (piw1 == piw2 && piw2 == piw3)
            {
            sqw2 = SqFindSecond (psqW, piw2);
            sqw3 = SqFindThird (psqW, piw3);
            }
        else if (piw1 == piw2)
            {
            sqw2 = SqFindSecond (psqW, piw2);
            sqw3 = SqFindFirst (psqW, piw3);
            }
        else if (piw2 == piw3)
            {
            sqw2 = SqFindFirst (psqW, piw2);
            sqw3 = SqFindSecond (psqW, piw3);
            }
        else
            {
            sqw2 = SqFindFirst (psqW, piw2);
            sqw3 = SqFindFirst (psqW, piw3);
            }
        sqbk = SqFindKing (psqB);
        sqb1 = SqFindOne  (psqB, pib1);

        if (x_piecePawn == piw3 || x_piecePawn == pib1)
            sqMask = rgsqReflectMaskY [sqbk] ^ rgsqReflectInvertMask [fInvert];
        else
            sqMask = rgsqReflectMaskYandX [sqbk];
        sqwk ^= sqMask;
        sqbk ^= sqMask;
        sqw1 ^= sqMask;
        sqw2 ^= sqMask;
        sqw3 ^= sqMask;
        sqb1 ^= sqMask;

        if (x_piecePawn == piw3 || x_piecePawn == pib1)
            {
            // There are pawns on the board
            if (x_piecePawn == pib1)
                {
                // Black pawn
                if (x_piecePawn == piw1)
                    {
                    // All 4 pieces are pawns
                    SORT (sqw1, sqw2);
                    SORT (sqw2, sqw3);
                    SORT (sqw1, sqw2);
                    if (XX == sqEnP)
                        {
                        sqw3 = EXCLUDE1(sqw3,sqb1)-i8;                  // 47
                        sqw2 = EXCLUDE1(sqw2,sqb1)-i8;                  // 47
                        uInd = TEnumerate1<x_piecePawn,true,true>::Index(sqbk,sqb1,sqwk);
                        CHECK_INF_SINGLE(uInd);
                        return  uInd*(i47*i46*i45/6) +
                                sqw3*(sqw3-1)*(sqw3-2)/6 +
                                sqw2*(sqw2-1)/2 +
                                EXCLUDE1(sqw1,sqb1)-i8; // 47*46*45/6
                        }
                    else
                        // En passant capture
                        return  rgcSinglePawnPresent[x_piecePawn]*(i47*i46*i45/6) +
                                IndHalfKings(sqbk,sqwk)*(i44*i43/2*i14) +
                                IndEnPassant31B (sqw1, sqw2, sqw3, sqb1, sqEnP ^ sqMask);
                    }
                else if (x_piecePawn == piw2)
                    {
                    // Two white pawns, one black pawn
                    SORT (sqw2, sqw3);
                    if (XX == sqEnP)
                        {
                        sqTemp = EXCLUDE1(sqw3,sqb1)-i8;                    // 47
                        uInd = TEnumerate1<x_piecePawn,true,true>::
                                    Index(sqbk,sqb1,sqwk);
                        CHECK_INF_SINGLE(uInd);
                        return (uInd*(i59*i47)*(INDEX)(i46/2)) +
                                (EXCLUDE5(sqw1,sqwk,sqbk,sqw2,sqw3,sqb1)*(i47*i46/2) +  // 59
                                 sqTemp*(sqTemp-1)/2 +
                                 EXCLUDE1(sqw2,sqb1)-i8);   // 47*46/2
                        }
                    else
                        {
                        // En passant capture
                        sqEnP ^= sqMask;
                        uInd = rgcSinglePawnPresent[x_piecePawn];
                        return (uInd*(i59*i47))*(INDEX)(i46/2) +
                                (IndHalfKings(sqbk,sqwk)*(i57*i44*i14) +
                                 EXCLUDE7(sqw1,sqwk,sqbk,sqw2,sqw3,sqb1,sqEnP,sqEnP-8)*(i44*i14) +  // 57
                                 IndEnPassant21B (sqw2, sqw3, sqb1, sqEnP));
                        }
                    }
                else if (x_piecePawn == piw3)
                    {
                    // One white pawn, one black pawn
                    if (piw1 == piw2)
                        {
                        // Two identical white pieces
                        SORT (sqw1, sqw2);
                        if (XX == sqEnP)
                            {
                            sqw2 = EXCLUDE4(sqw2,sqwk,sqbk,sqw3,sqb1);
                            sqw1 = EXCLUDE4(sqw1,sqwk,sqbk,sqw3,sqb1);
                            uInd = TEnumerate1<x_piecePawn,true,true>::Index(sqbk,sqb1,sqwk);
                            CHECK_INF_SINGLE(uInd);
                            return  (uInd*(i59*i47))*((INDEX)i60/2) +
                                    ((sqw2*(sqw2-1)/2+sqw1)*i47 +   // 60*59/2
                                     EXCLUDE1(sqw3,sqb1)-i8);           // 47
                            }
                        else
                            {
                            // En passant capture
                            sqEnP ^= sqMask;
                            sqw2 = EXCLUDE6(sqw2,sqwk,sqbk,sqw3,sqb1,sqEnP,sqEnP-8);
                            sqw1 = EXCLUDE6(sqw1,sqwk,sqbk,sqw3,sqb1,sqEnP,sqEnP-8);
                            uInd = rgcSinglePawnPresent[x_piecePawn];
                            return  (uInd*(i59*i47))*((INDEX)i60/2) +
                                    (IndHalfKings(sqbk,sqwk)*(i58*i57/2*i14) +
                                     (sqw2*(sqw2-1)/2+sqw1)*i14 + // 58*57/2
                                     IndEnPassant11B (sqw3, sqb1, sqEnP));
                            }
                        }
                    else
                        {
                        // Two different white pieces
                        if (XX == sqEnP)
                            {
                            uInd = TEnumerate1<x_piecePawn,true,true>::Index(sqbk,sqb1,sqwk);
                            CHECK_INF_SINGLE(uInd);
                            return  (uInd*(i59*i47))*((INDEX)i60) +
                                    (EXCLUDE4(sqw1,sqwk,sqbk,sqw3,sqb1)*i59*i47 +   // 60
                                     EXCLUDE5(sqw2,sqwk,sqbk,sqw1,sqw3,sqb1)*i47 +  // 59
                                     EXCLUDE1(sqw3,sqb1)-i8);                       // 47
                            }
                        else
                            {
                            // En passant capture
                            sqEnP ^= sqMask;
                            uInd = rgcSinglePawnPresent[x_piecePawn];
                            return  (uInd*(i59*i47))*((INDEX)i60) +
                                    (IndHalfKings(sqbk,sqwk)*(i58*i57*i14) +
                                     EXCLUDE6(sqw1,sqwk,sqbk,sqw3,sqb1,sqEnP,sqEnP-8)*(i57*i14) +   // 58
                                     EXCLUDE7(sqw2,sqwk,sqbk,sqw1,sqw3,sqb1,sqEnP,sqEnP-8)*i14 +        // 57
                                     IndEnPassant11B (sqw3, sqb1, sqEnP));
                            }
                        }
                    }
                else
                    {
                    // Only black pawn
                    uInd = TEnumerate1<x_piecePawn,true,true>::Index(sqbk,sqb1,sqwk);
                    CHECK_INF_SINGLE(uInd);
                    if (piw1 == piw3)
                        {
                        // 3 identical white pieces
                        SORT (sqw1, sqw2);
                        SORT (sqw2, sqw3);
                        SORT (sqw1, sqw2);
                        sqw1 = EXCLUDE3(sqw1,sqwk,sqbk,sqb1);                       // 61
                        sqw2 = EXCLUDE3(sqw2,sqwk,sqbk,sqb1);                       // 61
                        sqw3 = EXCLUDE3(sqw3,sqwk,sqbk,sqb1);                       // 61
                        return  uInd*(i61*i60*i59/6) +
                                sqw3*(sqw3-1)*(sqw3-2)/6 + sqw2*(sqw2-1)/2 + sqw1;  // 61*60*59/6
                        }
                    else if (piw1 == piw2)
                        {
                        // 2 identical major white pieces
                        SORT (sqw1, sqw2);
                        sqw3 = EXCLUDE5(sqw3,sqwk,sqbk,sqw1,sqw2,sqb1);
                        sqw1 = EXCLUDE3(sqw1,sqwk,sqbk,sqb1);               // 61
                        sqw2 = EXCLUDE3(sqw2,sqwk,sqbk,sqb1);               // 61
                        return  (uInd*(i61*i59))*(INDEX)(i60/2) +
                                ((sqw2*(sqw2-1)/2 + sqw1)*i59 +             // 61*60/2
                                 sqw3);                                     // 59
                        }
                    else if (piw2 == piw3)
                        {
                        // 2 identical minor white pieces
                        SORT (sqw2, sqw3);
                        sqw2 = EXCLUDE4(sqw2,sqwk,sqbk,sqw1,sqb1);              // 60
                        sqw3 = EXCLUDE4(sqw3,sqwk,sqbk,sqw1,sqb1);              // 60
                        return  (uInd*(i61*i59))*(INDEX)(i60/2) +
                                (EXCLUDE3(sqw1,sqwk,sqbk,sqb1)*(i60*i59/2) +    // 61
                                 sqw3*(sqw3-1)/2 + sqw2);                       // 60*59/2
                        }
                    else
                        {
                        // All 3 white pieces are different
                        return  (uInd*(i61*i59))*(INDEX)i60 +
                                (EXCLUDE3(sqw1,sqwk,sqbk,sqb1)*(i60*i59) +  // 61
                                 EXCLUDE4(sqw2,sqwk,sqbk,sqw1,sqb1)*i59 +   // 60
                                 EXCLUDE5(sqw3,sqwk,sqbk,sqw1,sqw2,sqb1));  // 59
                        }
                    }
                }
            else
                {
                // No black pawn
                uInd = TEnumerate1<pib1,true,true>::Index(sqbk,sqb1,sqwk);
                CHECK_INF_SINGLE(uInd);
                if (x_piecePawn == piw1)
                    {
                    // Only 3 white pawns
                    SORT (sqw1, sqw2);
                    SORT (sqw2, sqw3);
                    SORT (sqw1, sqw2);
                    sqw3 -= i8;
                    sqw2 -= i8;
                    return  uInd*(i48*47*i46/6) +
                            sqw3*(sqw3-1)*(sqw3-2)/6+sqw2*(sqw2-1)/2+sqw1-i8;           // 48*47*46/6
                    }
                else if (x_piecePawn == piw2)
                    {
                    // 2 white pawns, one non-pawn
                    SORT (sqw2, sqw3);
                    sqTemp = sqw3 - i8;
                    return  (uInd*(i59*47))*(INDEX)(i48/2) +
                            (EXCLUDE5(sqw1,sqwk,sqbk,sqb1,sqw2,sqw3)*(i48*i47/2) +
                             sqTemp*(sqTemp-1)/2+sqw2-i8);          // 48*47/2
                    }
                else if (piw1 == piw2)
                    {
                    // One white pawn, 2 identical white pieces
                    SORT (sqw1, sqw2);
                    sqw1 = EXCLUDE4(sqw1,sqwk,sqbk,sqb1,sqw3);
                    sqw2 = EXCLUDE4(sqw2,sqwk,sqbk,sqb1,sqw3);
                    return  (uInd*(i60*i59/2))*(INDEX)i48 +
                            ((sqw2*(sqw2-1)/2+sqw1)*i48 +   // 60*59/2
                             sqw3-i8);                      // 48
                    }
                else
                    {
                    // One white pawn, 2 different white pieces
                    return  (uInd*(i60*i59))*(INDEX)i48 +
                            (EXCLUDE4(sqw1,sqwk,sqbk,sqb1,sqw3)*(i59*i48) + // 60
                             EXCLUDE5(sqw2,sqwk,sqbk,sqb1,sqw1,sqw3)*i48 +  // 59
                             sqw3-i8);                                      // 48
                    }
                }
            }
        else
            {
            // No pawns
            if (!FInTriangle (sqbk, sqwk))
                {
                sqwk = reflect_xy(sqwk);
                sqw1 = reflect_xy(sqw1);
                sqw2 = reflect_xy(sqw2);
                sqw3 = reflect_xy(sqw3);
                sqbk = reflect_xy(sqbk);
                sqb1 = reflect_xy(sqb1);
                };
            uInd = TEnumerate1<pib1,false,false>::Index(sqbk,sqb1,sqwk);
            CHECK_INF_SINGLE(uInd);
            if (piw1 == piw2 && piw2 == piw3)
                {
                // All 3 pieces equal
                SORT (sqw1, sqw2);
                SORT (sqw2, sqw3);
                SORT (sqw1, sqw2);
                sqw3 = EXCLUDE3(sqw3,sqwk,sqbk,sqb1);   // 61
                sqw2 = EXCLUDE3(sqw2,sqwk,sqbk,sqb1);
                return  uInd*(i61*i60*i59/6) +
                        sqw3*(sqw3-1)*(sqw3-2)/6+
                        sqw2*(sqw2-1)/2+
                        EXCLUDE3(sqw1,sqwk,sqbk,sqb1);  // 61*60*59/6
                }
            else if (piw1 == piw2)
                {
                // 2 major pieces equal
                SORT (sqw1, sqw2);
                sqw2 = EXCLUDE4(sqw2,sqwk,sqbk,sqw3,sqb1);                          // 60
                return uInd*(i60*i59/2*i61) +
                        (sqw2*(sqw2-1)/2+EXCLUDE4(sqw1,sqwk,sqbk,sqw3,sqb1))*i61 +  // 60*59/2
                        EXCLUDE3(sqw3,sqwk,sqbk,sqb1);                              // 61
                }
            else if (piw2 == piw3)
                {
                // 2 minor pieces equal
                SORT (sqw2, sqw3);
                sqw3 = EXCLUDE4(sqw3,sqwk,sqbk,sqw1,sqb1);                  // 60
                return uInd*(i61*i60*i59/2) +
                        EXCLUDE3(sqw1,sqwk,sqbk,sqb1)*(i60*i59/2) +         // 62
                        sqw3*(sqw3-1)/2+EXCLUDE4(sqw2,sqwk,sqbk,sqw1,sqb1); // 60*59/2
                }
            else
                {
                uInd *= i61*i60*i59/2;
                return (INDEX) uInd + 
                       (INDEX) (uInd +
                                EXCLUDE3(sqw1,sqwk,sqbk,sqb1)*(i60*i59) +   // 61
                                EXCLUDE4(sqw2,sqwk,sqbk,sqw1,sqb1)*i59 +    // 60
                                EXCLUDE5(sqw3,sqwk,sqbk,sqw1,sqw2,sqb1));   // 59
                }
            }
        }
    };

#endif  // T42

#else   // Old SJE schema ------------------------------------------------------

/* scanning pattern: triangle encoding */

static const INDEX sptriv[] = 
    {
     0,  1,  2,  3, -1, -1, -1, -1,
    -1,  4,  5,  6, -1, -1, -1, -1,
    -1, -1,  7,  8, -1, -1, -1, -1,
    -1, -1, -1,  9, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    };

/* scanning pattern: queenside flank encoding */

static const INDEX spqsfv[] = 
    {
     0,  1,  2,  3, -1, -1, -1, -1,
     4,  5,  6,  7, -1, -1, -1, -1,
     8,  9, 10, 11, -1, -1, -1, -1,
    12, 13, 14, 15, -1, -1, -1, -1,
    16, 17, 18, 19, -1, -1, -1, -1,
    20, 21, 22, 23, -1, -1, -1, -1,
    24, 25, 26, 27, -1, -1, -1, -1,
    28, 29, 30, 31, -1, -1, -1, -1,
    };

/*--> CalcIndex3A: calculate index, mode 3A */
INLINE INDEX CalcIndex3A
    (
    square sq0,
    square sq1,
    square sq2
    )
    {
    INDEX index;

    if (TbRow(sq2) > x_row_4)
        {
        sq0 = reflect_x(sq0);
        sq1 = reflect_x(sq1);
        sq2 = reflect_x(sq2);
        };

    if (TbColumn(sq2) > x_column_d)
        {
        sq0 = reflect_y(sq0);
        sq1 = reflect_y(sq1);
        sq2 = reflect_y(sq2);
        };

    if (TbRow(sq2) > TbColumn(sq2))
        {
        sq0 = reflect_xy(sq0);
        sq1 = reflect_xy(sq1);
        sq2 = reflect_xy(sq2);
        };

    index =
        sq0 +
        sq1 * i64 +
        sptriv [sq2] * i64 * i64;

    return (index);
    }

/*--> CalcIndex3B: calculate index, mode 3B */
INLINE INDEX CalcIndex3B
    (
    square sq0,
    square sq1,
    square sq2
    )
    {
    INDEX index;

    if (TbColumn(sq1) > x_column_d)
        {
        sq0 = reflect_y(sq0);
        sq1 = reflect_y(sq1);
        sq2 = reflect_y(sq2);
        };

    index =
        sq0 +
        spqsfv [sq1] * i64 +
        sq2 * (i64 / 2) * i64;

    return (index);
    }

/*--> CalcIndex4A: calculate index, mode 4A */
INLINE INDEX CalcIndex4A
    (
    square sq0,
    square sq1,
    square sq2,
    square sq3
    )
    {
    INDEX index;

    if (TbRow(sq3) > x_row_4)
        {
        sq0 = reflect_x(sq0);
        sq1 = reflect_x(sq1);
        sq2 = reflect_x(sq2);
        sq3 = reflect_x(sq3);
        };

    if (TbColumn(sq3) > x_column_d)
        {
        sq0 = reflect_y(sq0);
        sq1 = reflect_y(sq1);
        sq2 = reflect_y(sq2);
        sq3 = reflect_y(sq3);
        };

    if (TbRow(sq3) > TbColumn(sq3))
        {
        sq0 = reflect_xy(sq0);
        sq1 = reflect_xy(sq1);
        sq2 = reflect_xy(sq2);
        sq3 = reflect_xy(sq3);
        };

    index =
        sq0 +
        sq1 * i64 +
        sq2 * i64 * i64 +
        sptriv [sq3] * i64 * i64 * i64;

    return (index);
    }

/*--> CalcIndex4B: calculate index, mode 4B */
INLINE INDEX CalcIndex4B
    (
    square sq0,
    square sq1,
    square sq2,
    square sq3
    )
    {
    INDEX index;

    if (TbColumn(sq3) > x_column_d)
        {
        sq0 = reflect_y(sq0);
        sq1 = reflect_y(sq1);
        sq2 = reflect_y(sq2);
        sq3 = reflect_y(sq3);
        };

    index =
        sq0 +
        sq1 * i64 +
        sq2 * i64 * i64 +
        spqsfv [sq3] * i64 * i64 * i64;

    return (index);
    }

/*--> CalcIndex4C: calculate index, mode 4C */
INLINE INDEX CalcIndex4C
    (
    square sq0,
    square sq1,
    square sq2,
    square sq3
    )
    {
    INDEX index;

    if (TbColumn(sq2) > x_column_d)
        {
        sq0 = reflect_y(sq0);
        sq1 = reflect_y(sq1);
        sq2 = reflect_y(sq2);
        sq3 = reflect_y(sq3);
        };

    index =
        sq0 +
        sq1 * i64 +
        spqsfv [sq2] * i64 * i64 +
        sq3 * (i64 / 2) * i64 * i64;

    return (index);
    }

/*--> CalcIndex5A: calculate index, mode 5A */
INLINE INDEX CalcIndex5A
    (
    square sq0,
    square sq1,
    square sq2,
    square sq3,
    square sq4
    )
    {
    INDEX index;

    if (TbRow(sq4) > x_row_4)
        {
        sq0 = reflect_x(sq0);
        sq1 = reflect_x(sq1);
        sq2 = reflect_x(sq2);
        sq3 = reflect_x(sq3);
        sq4 = reflect_x(sq4);
        };

    if (TbColumn(sq4) > x_column_d)
        {
        sq0 = reflect_y(sq0);
        sq1 = reflect_y(sq1);
        sq2 = reflect_y(sq2);
        sq3 = reflect_y(sq3);
        sq4 = reflect_y(sq4);
        };

    if (TbRow(sq4) > TbColumn(sq4))
        {
        sq0 = reflect_xy(sq0);
        sq1 = reflect_xy(sq1);
        sq2 = reflect_xy(sq2);
        sq3 = reflect_xy(sq3);
        sq4 = reflect_xy(sq4);
        };

    index =
        sq0 +
        sq1 * i64 +
        sq2 * i64 * i64 +
        sq3 * i64 * i64 * i64 +
        sptriv [sq4] * i64 * i64 * i64 * i64;

    return (index);
    }

/*--> CalcIndex5B: calculate index, mode 5B */
INLINE INDEX CalcIndex5B
    (
    square sq0,
    square sq1,
    square sq2,
    square sq3,
    square sq4
    )
    {
    INDEX index;

    if (TbColumn(sq4) > x_column_d)
        {
        sq0 = reflect_y(sq0);
        sq1 = reflect_y(sq1);
        sq2 = reflect_y(sq2);
        sq3 = reflect_y(sq3);
        sq4 = reflect_y(sq4);
        };

    index =
        sq0 +
        sq1 * i64 +
        sq2 * i64 * i64 +
        sq3 * i64 * i64 * i64 +
        spqsfv [sq4] * i64 * i64 * i64 * i64;

    return (index);
    }

/*--> CalcIndex5C: calculate index, mode 5C */
INLINE INDEX CalcIndex5C
    (
    square sq0,
    square sq1,
    square sq2,
    square sq3,
    square sq4
    )
    {
    INDEX index;

    if (TbColumn(sq3) > x_column_d)
        {
        sq0 = reflect_y(sq0);
        sq1 = reflect_y(sq1);
        sq2 = reflect_y(sq2);
        sq3 = reflect_y(sq3);
        sq4 = reflect_y(sq4);
        };

    index =
        sq0 +
        sq1 * i64 +
        sq2 * i64 * i64 +
        spqsfv [sq3] * i64 * i64 * i64 +
        sq4 * (i64 / 2) * i64 * i64 * i64;

    return (index);
    }

/*--> CalcIndex5D: calculate index, mode 5D */
INLINE INDEX CalcIndex5D
    (
    square sq0,
    square sq1,
    square sq2,
    square sq3,
    square sq4
    )
    {
    INDEX index;

    if (TbColumn(sq2) > x_column_d)
        {
        sq0 = reflect_y(sq0);
        sq1 = reflect_y(sq1);
        sq2 = reflect_y(sq2);
        sq3 = reflect_y(sq3);
        sq4 = reflect_y(sq4);
        };

    index =
        sq0 +
        sq1 * i64 +
        spqsfv [sq2] * i64 * i64 +
        sq3 * (i64 / 2) * i64 * i64 +
        sq4 * (i64 / 2) * i64 * i64 * i64;

    return (index);
    }

// Calculate index - a lot of functions...

#define IndCalcW    IndCalc
#define IndCalcB    IndCalc

template <int pi> class T21
    {
public:
    static INDEX TB_FASTCALL IndCalc
        (
        square  *psqW,
        square  *psqB,
        square  sqEnP,
        int     fInvert
        )
        {
        square sq0, sq1, sq2;

        sq0 = SqFindKing (psqW);
        sq1 = SqFindOne  (psqW, pi);
        sq2 = SqFindKing (psqB);

        if (x_piecePawn == pi)
            {
            if (fInvert)
                {
                sq0 = reflect_x (sq0);
                sq1 = reflect_x (sq1);
                sq2 = reflect_x (sq2);
                }
            return CalcIndex3B (sq0, sq1, sq2);
            }
        else
            return CalcIndex3A (sq0, sq1, sq2);
        }
    };

template <int pi1, int pi2> class T22
    {
public:
    static INDEX TB_FASTCALL IndCalc
        (
        square  *psqW,
        square  *psqB,
        square  sqEnP,
        int     fInvert
        )
        {
        square sq0, sq1, sq2, sq3;

        sq0 = SqFindKing (psqW);
        sq1 = SqFindOne  (psqW, pi1);
        sq2 = SqFindKing (psqB);
        sq3 = SqFindOne  (psqB, pi2);

        if (x_piecePawn == pi1 || x_piecePawn == pi2)
            {
            if (fInvert)
                {
                sq0 = reflect_x (sq0);
                sq1 = reflect_x (sq1);
                sq2 = reflect_x (sq2);
                sq3 = reflect_x (sq3);
                }
            return CalcIndex4B (sq0, sq1, sq2, sq3);
            }
        else
            return CalcIndex4A (sq0, sq1, sq2, sq3);
        }
    };

template <int pi1, int pi2> class T31
    {
public:
    static INDEX TB_FASTCALL IndCalc
        (
        square  *psqW,
        square  *psqB,
        square  sqEnP,
        int     fInvert
        )
        {
        square sq0, sq1, sq2, sq3;

        sq0 = SqFindKing (psqW);
        sq1 = SqFindFirst (psqW, pi1);
        if (pi1 == pi2)
            sq2 = SqFindSecond (psqW, pi2);
        else
            sq2 = SqFindFirst (psqW, pi2);
        sq3 = SqFindKing (psqB);

        if (x_piecePawn == pi1 || x_piecePawn == pi2)
            {
            if (fInvert)
                {
                sq0 = reflect_x (sq0);
                sq1 = reflect_x (sq1);
                sq2 = reflect_x (sq2);
                sq3 = reflect_x (sq3);
                }
            return CalcIndex4C (sq0, sq1, sq2, sq3);
            }
        else
            return CalcIndex4A (sq0, sq1, sq2, sq3);
        }
    };

template <int pi1, int pi2, int pi3> class T32
    {
public:
    static INDEX TB_FASTCALL IndCalc
        (
        square  *psqW,
        square  *psqB,
        square  sqEnP,
        int     fInvert
        )
        {
        square sq0, sq1, sq2, sq3, sq4;

        sq0 = SqFindKing (psqW);
        sq1 = SqFindFirst (psqW, pi1);
        if (pi1 == pi2)
            sq2 = SqFindSecond (psqW, pi2);
        else
            sq2 = SqFindFirst (psqW, pi2);
        sq3 = SqFindKing (psqB);
        sq4 = SqFindOne  (psqB, pi3);

        if (x_piecePawn == pi1 || x_piecePawn == pi2 || x_piecePawn == pi3)
            {
            if (fInvert)
                {
                sq0 = reflect_x (sq0);
                sq1 = reflect_x (sq1);
                sq2 = reflect_x (sq2);
                sq3 = reflect_x (sq3);
                sq4 = reflect_x (sq4);
                }
            if (x_piecePawn == pi3)
                return CalcIndex5B (sq0, sq1, sq2, sq3, sq4);
            else
                return CalcIndex5D (sq0, sq1, sq2, sq3, sq4);
            }
        else
            return CalcIndex5A (sq0, sq1, sq2, sq3, sq4);
        }
    };

#if defined (T41_INCLUDE)

template <int pi1, int pi2, int pi3> class T41
    {
public:
    static INDEX TB_FASTCALL IndCalc
        (
        square  *psqW,
        square  *psqB,
        square  sqEnP,
        int     fInvert
        )
        {
        square sq0, sq1, sq2, sq3, sq4;

        sq0 = SqFindKing (psqW);
        sq1 = SqFindFirst (psqW, pi1);
        sq2 = SqFindFirst (psqW, pi2);
        sq3 = SqFindFirst (psqW, pi3);
        sq4 = SqFindKing (psqB);

        if (x_piecePawn == pi1 || x_piecePawn == pi2 || x_piecePawn == pi3)
            {
            // There are pawns on the board
            if (fInvert)
                {
                sq0 = reflect_x (sq0);
                sq1 = reflect_x (sq1);
                sq2 = reflect_x (sq2);
                sq3 = reflect_x (sq3);
                sq4 = reflect_x (sq4);
                }
            return CalcIndex5C (sq0, sq1, sq2, sq3, sq4);
            }
        else    // No pawns
            return CalcIndex5A (sq0, sq1, sq2, sq3, sq4);
        }
    };

#endif

#endif  //----------------------------------------------------------------------

// All tablebases enumerated

#define tbid_kk      0
#define tbid_kpk     1
#define tbid_knk     2
#define tbid_kbk     3
#define tbid_krk     4
#define tbid_kqk     5
#define tbid_kpkp    6
#define tbid_knkp    7
#define tbid_knkn    8
#define tbid_kbkp    9
#define tbid_kbkn   10
#define tbid_kbkb   11
#define tbid_krkp   12
#define tbid_krkn   13
#define tbid_krkb   14
#define tbid_krkr   15
#define tbid_kqkp   16
#define tbid_kqkn   17
#define tbid_kqkb   18
#define tbid_kqkr   19
#define tbid_kqkq   20
#define tbid_kppk   21
#define tbid_knpk   22
#define tbid_knnk   23
#define tbid_kbpk   24
#define tbid_kbnk   25
#define tbid_kbbk   26
#define tbid_krpk   27
#define tbid_krnk   28
#define tbid_krbk   29
#define tbid_krrk   30
#define tbid_kqpk   31
#define tbid_kqnk   32
#define tbid_kqbk   33
#define tbid_kqrk   34
#define tbid_kqqk   35
#define tbid_kppkp  36
#define tbid_kppkn  37
#define tbid_kppkb  38
#define tbid_kppkr  39
#define tbid_kppkq  40
#define tbid_knpkp  41
#define tbid_knpkn  42
#define tbid_knpkb  43
#define tbid_knpkr  44
#define tbid_knpkq  45
#define tbid_knnkp  46
#define tbid_knnkn  47
#define tbid_knnkb  48
#define tbid_knnkr  49
#define tbid_knnkq  50
#define tbid_kbpkp  51
#define tbid_kbpkn  52
#define tbid_kbpkb  53
#define tbid_kbpkr  54
#define tbid_kbpkq  55
#define tbid_kbnkp  56
#define tbid_kbnkn  57
#define tbid_kbnkb  58
#define tbid_kbnkr  59
#define tbid_kbnkq  60
#define tbid_kbbkp  61
#define tbid_kbbkn  62
#define tbid_kbbkb  63
#define tbid_kbbkr  64
#define tbid_kbbkq  65
#define tbid_krpkp  66
#define tbid_krpkn  67
#define tbid_krpkb  68
#define tbid_krpkr  69
#define tbid_krpkq  70
#define tbid_krnkp  71
#define tbid_krnkn  72
#define tbid_krnkb  73
#define tbid_krnkr  74
#define tbid_krnkq  75
#define tbid_krbkp  76
#define tbid_krbkn  77
#define tbid_krbkb  78
#define tbid_krbkr  79
#define tbid_krbkq  80
#define tbid_krrkp  81
#define tbid_krrkn  82
#define tbid_krrkb  83
#define tbid_krrkr  84
#define tbid_krrkq  85
#define tbid_kqpkp  86
#define tbid_kqpkn  87
#define tbid_kqpkb  88
#define tbid_kqpkr  89
#define tbid_kqpkq  90
#define tbid_kqnkp  91
#define tbid_kqnkn  92
#define tbid_kqnkb  93
#define tbid_kqnkr  94
#define tbid_kqnkq  95
#define tbid_kqbkp  96
#define tbid_kqbkn  97
#define tbid_kqbkb  98
#define tbid_kqbkr  99
#define tbid_kqbkq 100
#define tbid_kqrkp 101
#define tbid_kqrkn 102
#define tbid_kqrkb 103
#define tbid_kqrkr 104
#define tbid_kqrkq 105
#define tbid_kqqkp 106
#define tbid_kqqkn 107
#define tbid_kqqkb 108
#define tbid_kqqkr 109
#define tbid_kqqkq 110

#if defined (T41_INCLUDE)
#  define tbid_kpppk    111
#  define tbid_knppk    112
#  define tbid_knnpk    113
#  define tbid_knnnk    114
#  define tbid_kbppk    115
#  define tbid_kbnpk    116
#  define tbid_kbnnk    117
#  define tbid_kbbpk    118
#  define tbid_kbbnk    119
#  define tbid_kbbbk    120
#  define tbid_krppk    121
#  define tbid_krnpk    122
#  define tbid_krnnk    123
#  define tbid_krbpk    124
#  define tbid_krbnk    125
#  define tbid_krbbk    126
#  define tbid_krrpk    127
#  define tbid_krrnk    128
#  define tbid_krrbk    129
#  define tbid_krrrk    130
#  define tbid_kqppk    131
#  define tbid_kqnpk    132
#  define tbid_kqnnk    133
#  define tbid_kqbpk    134
#  define tbid_kqbnk    135
#  define tbid_kqbbk    136
#  define tbid_kqrpk    137
#  define tbid_kqrnk    138
#  define tbid_kqrbk    139
#  define tbid_kqrrk    140
#  define tbid_kqqpk    141
#  define tbid_kqqnk    142
#  define tbid_kqqbk    143
#  define tbid_kqqrk    144
#  define tbid_kqqqk    145
#endif

#if defined (T33_INCLUDE)
#  if defined (T41_INCLUDE)
#    define BASE_33 145
#  else
#    define BASE_33 110
#  endif
#  define tbid_knnknn   (BASE_33 + 1)
#  define tbid_kbnknn   (BASE_33 + 2)
#  define tbid_kbbknn   (BASE_33 + 3)
#  define tbid_kbbkbn   (BASE_33 + 4)
#  define tbid_kbbkbb   (BASE_33 + 5)
#  define tbid_krnknn   (BASE_33 + 6)
#  define tbid_krnkbb   (BASE_33 + 7)
#  define tbid_krbknn   (BASE_33 + 8)
#  define tbid_krbkbb   (BASE_33 + 9)
#  define tbid_krrknn   (BASE_33 + 10)
#  define tbid_krrkbn   (BASE_33 + 11)
#  define tbid_krrkbb   (BASE_33 + 12)
#  define tbid_krrkrn   (BASE_33 + 13)
#  define tbid_krrkrb   (BASE_33 + 14)
#  define tbid_krrkrr   (BASE_33 + 15)
#  define tbid_kqnknn   (BASE_33 + 16)
#  define tbid_kqnkbb   (BASE_33 + 17)
#  define tbid_kqnkrr   (BASE_33 + 18)
#  define tbid_kqbknn   (BASE_33 + 19)
#  define tbid_kqbkbb   (BASE_33 + 20)
#  define tbid_kqbkrr   (BASE_33 + 21)
#  define tbid_kqrknn   (BASE_33 + 22)
#  define tbid_kqrkbb   (BASE_33 + 23)
#  define tbid_kqrkrr   (BASE_33 + 24)
#  define tbid_kqqknn   (BASE_33 + 25)
#  define tbid_kqqkbn   (BASE_33 + 26)
#  define tbid_kqqkbb   (BASE_33 + 27)
#  define tbid_kqqkrn   (BASE_33 + 28)
#  define tbid_kqqkrb   (BASE_33 + 29)
#  define tbid_kqqkrr   (BASE_33 + 30)
#  define tbid_kqqkqn   (BASE_33 + 31)
#  define tbid_kqqkqb   (BASE_33 + 32)
#  define tbid_kqqkqr   (BASE_33 + 33)
#  define tbid_kqqkqq   (BASE_33 + 34)
#  if defined (T_INDEX64)
#    define tbid_kbnkbn (BASE_33 + 35)
#    define tbid_krnkrn (BASE_33 + 36)
#    define tbid_krbkrb (BASE_33 + 37)
#    define tbid_kqnkqn (BASE_33 + 38)
#    define tbid_kqbkqb (BASE_33 + 39)
#    define tbid_kqrkqr (BASE_33 + 40)
#    define tbid_krnkbn (BASE_33 + 41)
#    define tbid_krbkbn (BASE_33 + 42)
#    define tbid_krbkrn (BASE_33 + 43)
#    define tbid_kqnkbn (BASE_33 + 44)
#    define tbid_kqnkrn (BASE_33 + 45)
#    define tbid_kqnkrb (BASE_33 + 46)
#    define tbid_kqbkbn (BASE_33 + 47)
#    define tbid_kqbkrn (BASE_33 + 48)
#    define tbid_kqbkrb (BASE_33 + 49)
#    define tbid_kqbkqn (BASE_33 + 50)
#    define tbid_kqrkbn (BASE_33 + 51)
#    define tbid_kqrkrn (BASE_33 + 52)
#    define tbid_kqrkrb (BASE_33 + 53)
#    define tbid_kqrkqn (BASE_33 + 54)
#    define tbid_kqrkqb (BASE_33 + 55)
#    define tbid_kppkpp (BASE_33 + 56)
#    define tbid_knpkpp (BASE_33 + 57)
#    define tbid_knpknp (BASE_33 + 58)
#    define tbid_knnkpp (BASE_33 + 59)
#    define tbid_knnknp (BASE_33 + 60)
#    define tbid_kbpkpp (BASE_33 + 61)
#    define tbid_kbpknp (BASE_33 + 62)
#    define tbid_kbpknn (BASE_33 + 63)
#    define tbid_kbpkbp (BASE_33 + 64)
#    define tbid_kbnkpp (BASE_33 + 65)
#    define tbid_kbnknp (BASE_33 + 66)
#    define tbid_kbnkbp (BASE_33 + 67)
#    define tbid_kbbkpp (BASE_33 + 68)
#    define tbid_kbbknp (BASE_33 + 69)
#    define tbid_kbbkbp (BASE_33 + 70)
#    define tbid_krpkpp (BASE_33 + 71)
#    define tbid_krpknp (BASE_33 + 72)
#    define tbid_krpknn (BASE_33 + 73)
#    define tbid_krpkbp (BASE_33 + 74)
#    define tbid_krpkbn (BASE_33 + 75)
#    define tbid_krpkbb (BASE_33 + 76)
#    define tbid_krpkrp (BASE_33 + 77)
#    define tbid_krnkpp (BASE_33 + 78)
#    define tbid_krnknp (BASE_33 + 79)
#    define tbid_krnkbp (BASE_33 + 80)
#    define tbid_krnkrp (BASE_33 + 81)
#    define tbid_krbkpp (BASE_33 + 82)
#    define tbid_krbknp (BASE_33 + 83)
#    define tbid_krbkbp (BASE_33 + 84)
#    define tbid_krbkrp (BASE_33 + 85)
#    define tbid_krrkpp (BASE_33 + 86)
#    define tbid_krrknp (BASE_33 + 87)
#    define tbid_krrkbp (BASE_33 + 88)
#    define tbid_krrkrp (BASE_33 + 89)
#    define tbid_kqpkpp (BASE_33 + 90)
#    define tbid_kqpknp (BASE_33 + 91)
#    define tbid_kqpknn (BASE_33 + 92)
#    define tbid_kqpkbp (BASE_33 + 93)
#    define tbid_kqpkbn (BASE_33 + 94)
#    define tbid_kqpkbb (BASE_33 + 95)
#    define tbid_kqpkrp (BASE_33 + 96)
#    define tbid_kqpkrn (BASE_33 + 97)
#    define tbid_kqpkrb (BASE_33 + 98)
#    define tbid_kqpkrr (BASE_33 + 99)
#    define tbid_kqpkqp (BASE_33 + 100)
#    define tbid_kqnkpp (BASE_33 + 101)
#    define tbid_kqnknp (BASE_33 + 102)
#    define tbid_kqnkbp (BASE_33 + 103)
#    define tbid_kqnkrp (BASE_33 + 104)
#    define tbid_kqnkqp (BASE_33 + 105)
#    define tbid_kqbkpp (BASE_33 + 106)
#    define tbid_kqbknp (BASE_33 + 107)
#    define tbid_kqbkbp (BASE_33 + 108)
#    define tbid_kqbkrp (BASE_33 + 109)
#    define tbid_kqbkqp (BASE_33 + 110)
#    define tbid_kqrkpp (BASE_33 + 111)
#    define tbid_kqrknp (BASE_33 + 112)
#    define tbid_kqrkbp (BASE_33 + 113)
#    define tbid_kqrkrp (BASE_33 + 114)
#    define tbid_kqrkqp (BASE_33 + 115)
#    define tbid_kqqkpp (BASE_33 + 116)
#    define tbid_kqqknp (BASE_33 + 117)
#    define tbid_kqqkbp (BASE_33 + 118)
#    define tbid_kqqkrp (BASE_33 + 119)
#    define tbid_kqqkqp (BASE_33 + 120)
#    define C33 (tbid_kqqkqp - BASE_33)
#  else
#    define C33 (tbid_kqqkqq - BASE_33)
#  endif
#else
#  define C33   0
#endif

#if defined (T41_INCLUDE)
#  define BASE_42   (145 + C33)
#else
#  define BASE_42   (110 + C33)
#endif

#if defined (T42_INCLUDE)
#  define tbid_knnnkn   (BASE_42 + 1)
#  define tbid_kbnnkn   (BASE_42 + 2)
#  define tbid_kbbnkn   (BASE_42 + 3)
#  define tbid_kbbbkn   (BASE_42 + 4)
#  define tbid_krnnkn   (BASE_42 + 5)
#  define tbid_krbbkn   (BASE_42 + 6)
#  define tbid_krrnkn   (BASE_42 + 7)
#  define tbid_krrbkn   (BASE_42 + 8)
#  define tbid_krrrkn   (BASE_42 + 9)
#  define tbid_kqnnkn   (BASE_42 + 10)
#  define tbid_kqbbkn   (BASE_42 + 11)
#  define tbid_kqrrkn   (BASE_42 + 12)
#  define tbid_kqqnkn   (BASE_42 + 13)
#  define tbid_kqqbkn   (BASE_42 + 14)
#  define tbid_kqqrkn   (BASE_42 + 15)
#  define tbid_kqqqkn   (BASE_42 + 16)
#  define tbid_knnnkb   (BASE_42 + 17)
#  define tbid_kbnnkb   (BASE_42 + 18)
#  define tbid_kbbnkb   (BASE_42 + 19)
#  define tbid_kbbbkb   (BASE_42 + 20)
#  define tbid_krnnkb   (BASE_42 + 21)
#  define tbid_krbbkb   (BASE_42 + 22)
#  define tbid_krrnkb   (BASE_42 + 23)
#  define tbid_krrbkb   (BASE_42 + 24)
#  define tbid_krrrkb   (BASE_42 + 25)
#  define tbid_kqnnkb   (BASE_42 + 26)
#  define tbid_kqbbkb   (BASE_42 + 27)
#  define tbid_kqrrkb   (BASE_42 + 28)
#  define tbid_kqqnkb   (BASE_42 + 29)
#  define tbid_kqqbkb   (BASE_42 + 30)
#  define tbid_kqqrkb   (BASE_42 + 31)
#  define tbid_kqqqkb   (BASE_42 + 32)
#  define tbid_knnnkr   (BASE_42 + 33)
#  define tbid_kbnnkr   (BASE_42 + 34)
#  define tbid_kbbnkr   (BASE_42 + 35)
#  define tbid_kbbbkr   (BASE_42 + 36)
#  define tbid_krnnkr   (BASE_42 + 37)
#  define tbid_krbbkr   (BASE_42 + 38)
#  define tbid_krrnkr   (BASE_42 + 39)
#  define tbid_krrbkr   (BASE_42 + 40)
#  define tbid_krrrkr   (BASE_42 + 41)
#  define tbid_kqnnkr   (BASE_42 + 42)
#  define tbid_kqbbkr   (BASE_42 + 43)
#  define tbid_kqrrkr   (BASE_42 + 44)
#  define tbid_kqqnkr   (BASE_42 + 45)
#  define tbid_kqqbkr   (BASE_42 + 46)
#  define tbid_kqqrkr   (BASE_42 + 47)
#  define tbid_kqqqkr   (BASE_42 + 48)
#  define tbid_knnnkq   (BASE_42 + 49)
#  define tbid_kbnnkq   (BASE_42 + 50)
#  define tbid_kbbnkq   (BASE_42 + 51)
#  define tbid_kbbbkq   (BASE_42 + 52)
#  define tbid_krnnkq   (BASE_42 + 53)
#  define tbid_krbbkq   (BASE_42 + 54)
#  define tbid_krrnkq   (BASE_42 + 55)
#  define tbid_krrbkq   (BASE_42 + 56)
#  define tbid_krrrkq   (BASE_42 + 57)
#  define tbid_kqnnkq   (BASE_42 + 58)
#  define tbid_kqbbkq   (BASE_42 + 59)
#  define tbid_kqrrkq   (BASE_42 + 60)
#  define tbid_kqqnkq   (BASE_42 + 61)
#  define tbid_kqqbkq   (BASE_42 + 62)
#  define tbid_kqqrkq   (BASE_42 + 63)
#  define tbid_kqqqkq   (BASE_42 + 64)
#  if defined (T_INDEX64)
#    define tbid_krbnkn (BASE_42 + 65)
#    define tbid_kqbnkn (BASE_42 + 66)
#    define tbid_kqrnkn (BASE_42 + 67)
#    define tbid_kqrbkn (BASE_42 + 68)
#    define tbid_krbnkb (BASE_42 + 69)
#    define tbid_kqbnkb (BASE_42 + 70)
#    define tbid_kqrnkb (BASE_42 + 71)
#    define tbid_kqrbkb (BASE_42 + 72)
#    define tbid_krbnkr (BASE_42 + 73)
#    define tbid_kqbnkr (BASE_42 + 74)
#    define tbid_kqrnkr (BASE_42 + 75)
#    define tbid_kqrbkr (BASE_42 + 76)
#    define tbid_krbnkq (BASE_42 + 77)
#    define tbid_kqbnkq (BASE_42 + 78)
#    define tbid_kqrnkq (BASE_42 + 79)
#    define tbid_kqrbkq (BASE_42 + 80)

#    define tbid_kpppkp (BASE_42 + 81)
#    define tbid_knppkp (BASE_42 + 82)
#    define tbid_knnpkp (BASE_42 + 83)
#    define tbid_knnnkp (BASE_42 + 84)
#    define tbid_kbppkp (BASE_42 + 85)
#    define tbid_kbnpkp (BASE_42 + 86)
#    define tbid_kbnnkp (BASE_42 + 87)
#    define tbid_kbbpkp (BASE_42 + 88)
#    define tbid_kbbnkp (BASE_42 + 89)
#    define tbid_kbbbkp (BASE_42 + 90)
#    define tbid_krppkp (BASE_42 + 91)
#    define tbid_krnpkp (BASE_42 + 92)
#    define tbid_krnnkp (BASE_42 + 93)
#    define tbid_krbpkp (BASE_42 + 94)
#    define tbid_krbnkp (BASE_42 + 95)
#    define tbid_krbbkp (BASE_42 + 96)
#    define tbid_krrpkp (BASE_42 + 97)
#    define tbid_krrnkp (BASE_42 + 98)
#    define tbid_krrbkp (BASE_42 + 99)
#    define tbid_krrrkp (BASE_42 + 100)
#    define tbid_kqppkp (BASE_42 + 101)
#    define tbid_kqnpkp (BASE_42 + 102)
#    define tbid_kqnnkp (BASE_42 + 103)
#    define tbid_kqbpkp (BASE_42 + 104)
#    define tbid_kqbnkp (BASE_42 + 105)
#    define tbid_kqbbkp (BASE_42 + 106)
#    define tbid_kqrpkp (BASE_42 + 107)
#    define tbid_kqrnkp (BASE_42 + 108)
#    define tbid_kqrbkp (BASE_42 + 109)
#    define tbid_kqrrkp (BASE_42 + 110)
#    define tbid_kqqpkp (BASE_42 + 111)
#    define tbid_kqqnkp (BASE_42 + 112)
#    define tbid_kqqbkp (BASE_42 + 113)
#    define tbid_kqqrkp (BASE_42 + 114)
#    define tbid_kqqqkp (BASE_42 + 115)
#    define tbid_kpppkn (BASE_42 + 116)
#    define tbid_knppkn (BASE_42 + 117)
#    define tbid_knnpkn (BASE_42 + 118)
#    define tbid_kbppkn (BASE_42 + 119)
#    define tbid_kbnpkn (BASE_42 + 120)
#    define tbid_kbbpkn (BASE_42 + 121)
#    define tbid_krppkn (BASE_42 + 122)
#    define tbid_krnpkn (BASE_42 + 123)
#    define tbid_krbpkn (BASE_42 + 124)
#    define tbid_krrpkn (BASE_42 + 125)
#    define tbid_kqppkn (BASE_42 + 126)
#    define tbid_kqnpkn (BASE_42 + 127)
#    define tbid_kqbpkn (BASE_42 + 128)
#    define tbid_kqrpkn (BASE_42 + 129)
#    define tbid_kqqpkn (BASE_42 + 130)
#    define tbid_kpppkb (BASE_42 + 131)
#    define tbid_knppkb (BASE_42 + 132)
#    define tbid_knnpkb (BASE_42 + 133)
#    define tbid_kbppkb (BASE_42 + 134)
#    define tbid_kbnpkb (BASE_42 + 135)
#    define tbid_kbbpkb (BASE_42 + 136)
#    define tbid_krppkb (BASE_42 + 137)
#    define tbid_krnpkb (BASE_42 + 138)
#    define tbid_krbpkb (BASE_42 + 139)
#    define tbid_krrpkb (BASE_42 + 140)
#    define tbid_kqppkb (BASE_42 + 141)
#    define tbid_kqnpkb (BASE_42 + 142)
#    define tbid_kqbpkb (BASE_42 + 143)
#    define tbid_kqrpkb (BASE_42 + 144)
#    define tbid_kqqpkb (BASE_42 + 145)
#    define tbid_kpppkr (BASE_42 + 146)
#    define tbid_knppkr (BASE_42 + 147)
#    define tbid_knnpkr (BASE_42 + 148)
#    define tbid_kbppkr (BASE_42 + 149)
#    define tbid_kbnpkr (BASE_42 + 150)
#    define tbid_kbbpkr (BASE_42 + 151)
#    define tbid_krppkr (BASE_42 + 152)
#    define tbid_krnpkr (BASE_42 + 153)
#    define tbid_krbpkr (BASE_42 + 154)
#    define tbid_krrpkr (BASE_42 + 155)
#    define tbid_kqppkr (BASE_42 + 156)
#    define tbid_kqnpkr (BASE_42 + 157)
#    define tbid_kqbpkr (BASE_42 + 158)
#    define tbid_kqrpkr (BASE_42 + 159)
#    define tbid_kqqpkr (BASE_42 + 160)
#    define tbid_kpppkq (BASE_42 + 161)
#    define tbid_knppkq (BASE_42 + 162)
#    define tbid_knnpkq (BASE_42 + 163)
#    define tbid_kbppkq (BASE_42 + 164)
#    define tbid_kbnpkq (BASE_42 + 165)
#    define tbid_kbbpkq (BASE_42 + 166)
#    define tbid_krppkq (BASE_42 + 167)
#    define tbid_krnpkq (BASE_42 + 168)
#    define tbid_krbpkq (BASE_42 + 169)
#    define tbid_krrpkq (BASE_42 + 170)
#    define tbid_kqppkq (BASE_42 + 171)
#    define tbid_kqnpkq (BASE_42 + 172)
#    define tbid_kqbpkq (BASE_42 + 173)
#    define tbid_kqrpkq (BASE_42 + 174)
#    define tbid_kqqpkq (BASE_42 + 175)

#    define C42 175
#  else
#    define C42 64
#  endif
#endif

#if defined (T42_INCLUDE)
#  define cTb   (BASE_42 + C42 + 1)
#else
#  define cTb   (BASE_42 + 1)
#endif

// Compression

#include "tbdecode.h"

#if !defined (CPUS)
#  define   CPUS    1
#endif

#if (CPUS > 1)
  static    lock_t  lockDecode;
#endif
/*extern "C" int TB_CRC_CHECK = 0; */
int TB_CRC_CHECK = 0;
static int cCompressed = 0;
static decode_block *rgpdbDecodeBlocks[CPUS];

// Information about tablebases

#define MAX_EXTENTS                 18  /* Maximum # of 2Gb file extents */

#if defined (T33_INCLUDE) || defined (T42_INCLUDE)
#  define   MAX_TOTAL_PIECES        6   /* Maximum # of pieces on the board */
#else
#  define   MAX_TOTAL_PIECES        5   /* Maximum # of pieces on the board */
#endif
#define MAX_NON_KINGS           (MAX_TOTAL_PIECES - 2)

#if !defined (TB_DIRECTORY_SIZE)
#  define   TB_DIRECTORY_SIZE       32  /* # of cache buckets */
#endif

#if !defined (PFNCALCINDEX_DECLARED)
typedef INDEX (TB_FASTCALL * PfnCalcIndex) (square *psqW, square *psqB,
                                         square sqEnP, int fInverse);
#  define   PFNCALCINDEX_DECLARED
#endif

struct CTbCache;

typedef struct      // Hungarian: tbcb
    {
#if (CPUS > 1)
    lock_t                       m_lock;            // Lock on this cache bucket list
#endif
    volatile CTbCache * volatile m_ptbcFirst;   // Cached file chunks in LRU order
    }
    CTbCacheBucket;

typedef struct      // Hungarian: tbd
    {
    int             m_iTbId;
    unsigned int    m_fSymmetric:1;
    unsigned int    m_f16bit:1;
    unsigned int    m_fSplit:1;
    PfnCalcIndex    m_rgpfnCalcIndex[2];
    char            m_rgchName[MAX_TOTAL_PIECES+1];
    INDEX           m_rgcbLength[2];
    char            *m_rgpchFileName[2][MAX_EXTENTS];
#if (CPUS > 1)
    lock_t          m_rglockFiles[2];
#endif
    FILE            *m_rgfpFiles[2][MAX_EXTENTS];
    decode_info     *m_rgpdiDecodeInfo[2][MAX_EXTENTS];
    CTbCacheBucket  *m_prgtbcbBuckets[2];   // Cached file chunks in LRU order
    BYTE            *m_rgpbRead[2];
    }
    CTbDesc;

#if defined (T_INDEX64) && defined (_MSC_VER)
#  define   TB(name, fSym, f16bit, fSplit, funW, funB, cbW, cbB)\
                    { tbid_##name, fSym, f16bit, fSplit, { funW, funB }, #name, { cbW##ui64, cbB##ui64 } },
#elif defined (T_INDEX64)
#  define   TB(name, fSym, f16bit, fSplit, funW, funB, cbW, cbB)\
                    { tbid_##name, fSym, f16bit, fSplit, { funW, funB }, #name, { cbW##llu, cbB##llu } },
#else
#  define   TB(name, fSym, f16bit, fSplit, funW, funB, cbW, cbB)\
                    { tbid_##name, fSym, f16bit, fSplit, { funW, funB }, #name, { cbW##u, cbB##u } },
#endif

#define P   x_piecePawn
#define N   x_pieceKnight
#define B   x_pieceBishop
#define R   x_pieceRook
#define Q   x_pieceQueen

CTbDesc rgtbdDesc[cTb] =
    {
    TB (kk, true, false, false, NULL, NULL, 0, 0)

    TB (kpk, false, false, false, (T21<P>::IndCalcW), (T21<P>::IndCalcB), 81664, 84012)
    TB (knk, false, false, false, (T21<N>::IndCalcW), (T21<N>::IndCalcB), 26282, 28644)
    TB (kbk, false, false, false, (T21<B>::IndCalcW), (T21<B>::IndCalcB), 27243, 28644)
    TB (krk, false, false, false, (T21<R>::IndCalcW), (T21<R>::IndCalcB), 27030, 28644)
    TB (kqk, false, false, false, (T21<Q>::IndCalcW), (T21<Q>::IndCalcB), 25629, 28644)

    TB (kpkp, false, false, false, (T22<P, P>::IndCalcW), (T22<P, P>::IndCalcB), 3863492, 3863492)
    TB (knkp, false, false, false, (T22<N, P>::IndCalcW), (T22<N, P>::IndCalcB), 4931904, 4981504)
    TB (knkn, true,  false, false, (T22<N, N>::IndCalcW), (T22<N, N>::IndCalcB), 1603202, 1603202)
    TB (kbkp, false, false, false, (T22<B, P>::IndCalcW), (T22<B, P>::IndCalcB), 5112000, 4981504)
    TB (kbkn, false, false, false, (T22<B, N>::IndCalcW), (T22<B, N>::IndCalcB), 1661823, 1603202)
    TB (kbkb, true,  false, false, (T22<B, B>::IndCalcW), (T22<B, B>::IndCalcB), 1661823, 1661823)
    TB (krkp, false, false, false, (T22<R, P>::IndCalcW), (T22<R, P>::IndCalcB), 5072736, 4981504)
    TB (krkn, false, false, false, (T22<R, N>::IndCalcW), (T22<R, N>::IndCalcB), 1649196, 1603202)
    TB (krkb, false, false, false, (T22<R, B>::IndCalcW), (T22<R, B>::IndCalcB), 1649196, 1661823)
    TB (krkr, true,  false, false, (T22<R, R>::IndCalcW), (T22<R, R>::IndCalcB), 1649196, 1649196)
    TB (kqkp, false, false, false, (T22<Q, P>::IndCalcW), (T22<Q, P>::IndCalcB), 4810080, 4981504)
    TB (kqkn, false, false, false, (T22<Q, N>::IndCalcW), (T22<Q, N>::IndCalcB), 1563735, 1603202)
    TB (kqkb, false, false, false, (T22<Q, B>::IndCalcW), (T22<Q, B>::IndCalcB), 1563735, 1661823)
    TB (kqkr, false, false, false, (T22<Q, R>::IndCalcW), (T22<Q, R>::IndCalcB), 1563735, 1649196)
    TB (kqkq, true,  false, false, (T22<Q, Q>::IndCalcW), (T22<Q, Q>::IndCalcB), 1563735, 1563735)
    TB (kppk, false, false, false, (T31<P, P>::IndCalcW), (T31<P, P>::IndCalcB), 1806671, 1912372)
    TB (knpk, false, false, false, (T31<N, P>::IndCalcW), (T31<N, P>::IndCalcB), 4648581, 5124732)
    TB (knnk, false, false, false, (T31<N, N>::IndCalcW), (T31<N, N>::IndCalcB),  735304,  873642)
    TB (kbpk, false, false, false, (T31<B, P>::IndCalcW), (T31<B, P>::IndCalcB), 4817128, 5124732)
    TB (kbnk, false, false, false, (T31<B, N>::IndCalcW), (T31<B, N>::IndCalcB), 1550620, 1747284)
    TB (kbbk, false, false, false, (T31<B, B>::IndCalcW), (T31<B, B>::IndCalcB),  789885,  873642)
    TB (krpk, false, false, false, (T31<R, P>::IndCalcW), (T31<R, P>::IndCalcB), 4779530, 5124732)
    TB (krnk, false, false, false, (T31<R, N>::IndCalcW), (T31<R, N>::IndCalcB), 1538479, 1747284)
    TB (krbk, false, false, false, (T31<R, B>::IndCalcW), (T31<R, B>::IndCalcB), 1594560, 1747284)
    TB (krrk, false, false, false, (T31<R, R>::IndCalcW), (T31<R, R>::IndCalcB),  777300,  873642)
    TB (kqpk, false, false, false, (T31<Q, P>::IndCalcW), (T31<Q, P>::IndCalcB), 4533490, 5124732)
    TB (kqnk, false, false, false, (T31<Q, N>::IndCalcW), (T31<Q, N>::IndCalcB), 1459616, 1747284)
    TB (kqbk, false, false, false, (T31<Q, B>::IndCalcW), (T31<Q, B>::IndCalcB), 1512507, 1747284)
    TB (kqrk, false, false, false, (T31<Q, R>::IndCalcW), (T31<Q, R>::IndCalcB), 1500276, 1747284)
    TB (kqqk, false, false, false, (T31<Q, Q>::IndCalcW), (T31<Q, Q>::IndCalcB),  698739,  873642)

#if !defined (KPPKP_16BIT)
    TB (kppkp, false, false, false, (T32<P, P, P>::IndCalcW), (T32<P, P, P>::IndCalcB),  84219361,  89391280)
#else
    TB (kppkp, false, true,  false, (T32<P, P, P>::IndCalcW), (T32<P, P, P>::IndCalcB),  84219361,  89391280)
#endif
    TB (kppkn, false, false, false, (T32<P, P, N>::IndCalcW), (T32<P, P, N>::IndCalcB), 108400260, 115899744)
    TB (kppkb, false, false, false, (T32<P, P, B>::IndCalcW), (T32<P, P, B>::IndCalcB), 108400260, 120132000)
    TB (kppkr, false, false, false, (T32<P, P, R>::IndCalcW), (T32<P, P, R>::IndCalcB), 108400260, 119209296)
    TB (kppkq, false, false, false, (T32<P, P, Q>::IndCalcW), (T32<P, P, Q>::IndCalcB), 108400260, 113036880)
    TB (knpkp, false, false, false, (T32<N, P, P>::IndCalcW), (T32<N, P, P>::IndCalcB), 219921779, 231758952)
    TB (knpkn, false, false, false, (T32<N, P, N>::IndCalcW), (T32<N, P, N>::IndCalcB), 278914860, 295914240)
    TB (knpkb, false, false, false, (T32<N, P, B>::IndCalcW), (T32<N, P, B>::IndCalcB), 278914860, 306720000)
    TB (knpkr, false, false, false, (T32<N, P, R>::IndCalcW), (T32<N, P, R>::IndCalcB), 278914860, 304369920)
    TB (knpkq, false, false, false, (T32<N, P, Q>::IndCalcW), (T32<N, P, Q>::IndCalcB), 278914860, 288610560)
    TB (knnkp, false, false, false, (T32<N, N, P>::IndCalcW), (T32<N, N, P>::IndCalcB), 137991648, 149445120)
    TB (knnkn, false, false, false, (T32<N, N, N>::IndCalcW), (T32<N, N, N>::IndCalcB),  44118240,  48096060)
    TB (knnkb, false, false, false, (T32<N, N, B>::IndCalcW), (T32<N, N, B>::IndCalcB),  44118240,  49854690)
    TB (knnkr, false, false, false, (T32<N, N, R>::IndCalcW), (T32<N, N, R>::IndCalcB),  44118240,  49475880)
    TB (knnkq, false, false, false, (T32<N, N, Q>::IndCalcW), (T32<N, N, Q>::IndCalcB),  44118240,  46912050)
    TB (kbpkp, false, false, false, (T32<B, P, P>::IndCalcW), (T32<B, P, P>::IndCalcB), 227896016, 231758952)
    TB (kbpkn, false, false, false, (T32<B, P, N>::IndCalcW), (T32<B, P, N>::IndCalcB), 289027680, 295914240)
    TB (kbpkb, false, false, false, (T32<B, P, B>::IndCalcW), (T32<B, P, B>::IndCalcB), 289027680, 306720000)
    TB (kbpkr, false, false, false, (T32<B, P, R>::IndCalcW), (T32<B, P, R>::IndCalcB), 289027680, 304369920)
    TB (kbpkq, false, false, false, (T32<B, P, Q>::IndCalcW), (T32<B, P, Q>::IndCalcB), 289027680, 288610560)
    TB (kbnkp, false, false, false, (T32<B, N, P>::IndCalcW), (T32<B, N, P>::IndCalcB), 290989584, 298890240)
    TB (kbnkn, false, false, false, (T32<B, N, N>::IndCalcW), (T32<B, N, N>::IndCalcB),  93037200,  96192120)
    TB (kbnkb, false, false, false, (T32<B, N, B>::IndCalcW), (T32<B, N, B>::IndCalcB),  93037200,  99709380)
    TB (kbnkr, false, false, false, (T32<B, N, R>::IndCalcW), (T32<B, N, R>::IndCalcB),  93037200,  98951760)
    TB (kbnkq, false, false, false, (T32<B, N, Q>::IndCalcW), (T32<B, N, Q>::IndCalcB),  93037200,  93824100)
    TB (kbbkp, false, false, false, (T32<B, B, P>::IndCalcW), (T32<B, B, P>::IndCalcB), 148223520, 149445120)
    TB (kbbkn, false, false, false, (T32<B, B, N>::IndCalcW), (T32<B, B, N>::IndCalcB),  47393100,  48096060)
    TB (kbbkb, false, false, false, (T32<B, B, B>::IndCalcW), (T32<B, B, B>::IndCalcB),  47393100,  49854690)
    TB (kbbkr, false, false, false, (T32<B, B, R>::IndCalcW), (T32<B, B, R>::IndCalcB),  47393100,  49475880)
    TB (kbbkq, false, false, false, (T32<B, B, Q>::IndCalcW), (T32<B, B, Q>::IndCalcB),  47393100,  46912050)
    TB (krpkp, false, false, false, (T32<R, P, P>::IndCalcW), (T32<R, P, P>::IndCalcB), 226121876, 231758952)
    TB (krpkn, false, false, false, (T32<R, P, N>::IndCalcW), (T32<R, P, N>::IndCalcB), 286777440, 295914240)
    TB (krpkb, false, false, false, (T32<R, P, B>::IndCalcW), (T32<R, P, B>::IndCalcB), 286777440, 306720000)
    TB (krpkr, false, false, false, (T32<R, P, R>::IndCalcW), (T32<R, P, R>::IndCalcB), 286777440, 304369920)
    TB (krpkq, false, false, false, (T32<R, P, Q>::IndCalcW), (T32<R, P, Q>::IndCalcB), 286777440, 288610560)
    TB (krnkp, false, false, false, (T32<R, N, P>::IndCalcW), (T32<R, N, P>::IndCalcB), 288692928, 298890240)
    TB (krnkn, false, false, false, (T32<R, N, N>::IndCalcW), (T32<R, N, N>::IndCalcB),  92308740,  96192120)
    TB (krnkb, false, false, false, (T32<R, N, B>::IndCalcW), (T32<R, N, B>::IndCalcB),  92308740,  99709380)
    TB (krnkr, false, false, false, (T32<R, N, R>::IndCalcW), (T32<R, N, R>::IndCalcB),  92308740,  98951760)
    TB (krnkq, false, false, false, (T32<R, N, Q>::IndCalcW), (T32<R, N, Q>::IndCalcB),  92308740,  93824100)
    TB (krbkp, false, false, false, (T32<R, B, P>::IndCalcW), (T32<R, B, P>::IndCalcB), 299203200, 298890240)
    TB (krbkn, false, false, false, (T32<R, B, N>::IndCalcW), (T32<R, B, N>::IndCalcB),  95673600,  96192120)
    TB (krbkb, false, false, false, (T32<R, B, B>::IndCalcW), (T32<R, B, B>::IndCalcB),  95673600,  99709380)
    TB (krbkr, false, false, false, (T32<R, B, R>::IndCalcW), (T32<R, B, R>::IndCalcB),  95673600,  98951760)
    TB (krbkq, false, false, false, (T32<R, B, Q>::IndCalcW), (T32<R, B, Q>::IndCalcB),  95673600,  93824100)
    TB (krrkp, false, false, false, (T32<R, R, P>::IndCalcW), (T32<R, R, P>::IndCalcB), 145901232, 149445120)
    TB (krrkn, false, false, false, (T32<R, R, N>::IndCalcW), (T32<R, R, N>::IndCalcB),  46658340,  48096060)
    TB (krrkb, false, false, false, (T32<R, R, B>::IndCalcW), (T32<R, R, B>::IndCalcB),  46658340,  49854690)
    TB (krrkr, false, false, false, (T32<R, R, R>::IndCalcW), (T32<R, R, R>::IndCalcB),  46658340,  49475880)
    TB (krrkq, false, false, false, (T32<R, R, Q>::IndCalcW), (T32<R, R, Q>::IndCalcB),  46658340,  46912050)
    TB (kqpkp, false, false, false, (T32<Q, P, P>::IndCalcW), (T32<Q, P, P>::IndCalcB), 214481388, 231758952)
    TB (kqpkn, false, false, false, (T32<Q, P, N>::IndCalcW), (T32<Q, P, N>::IndCalcB), 272015040, 295914240)
    TB (kqpkb, false, false, false, (T32<Q, P, B>::IndCalcW), (T32<Q, P, B>::IndCalcB), 272015040, 306720000)
    TB (kqpkr, false, false, false, (T32<Q, P, R>::IndCalcW), (T32<Q, P, R>::IndCalcB), 272015040, 304369920)
    TB (kqpkq, false, false, false, (T32<Q, P, Q>::IndCalcW), (T32<Q, P, Q>::IndCalcB), 272015040, 288610560)
    TB (kqnkp, false, false, false, (T32<Q, N, P>::IndCalcW), (T32<Q, N, P>::IndCalcB), 273904512, 298890240)
    TB (kqnkn, false, false, false, (T32<Q, N, N>::IndCalcW), (T32<Q, N, N>::IndCalcB),  87576960,  96192120)
    TB (kqnkb, false, false, false, (T32<Q, N, B>::IndCalcW), (T32<Q, N, B>::IndCalcB),  87576960,  99709380)
    TB (kqnkr, false, false, false, (T32<Q, N, R>::IndCalcW), (T32<Q, N, R>::IndCalcB),  87576960,  98951760)
    TB (kqnkq, false, false, false, (T32<Q, N, Q>::IndCalcW), (T32<Q, N, Q>::IndCalcB),  87576960,  93824100)
    TB (kqbkp, false, false, false, (T32<Q, B, P>::IndCalcW), (T32<Q, B, P>::IndCalcB), 283818240, 298890240)
    TB (kqbkn, false, false, false, (T32<Q, B, N>::IndCalcW), (T32<Q, B, N>::IndCalcB),  90750420,  96192120)
    TB (kqbkb, false, false, false, (T32<Q, B, B>::IndCalcW), (T32<Q, B, B>::IndCalcB),  90750420,  99709380)
    TB (kqbkr, false, false, false, (T32<Q, B, R>::IndCalcW), (T32<Q, B, R>::IndCalcB),  90750420,  98951760)
    TB (kqbkq, false, false, false, (T32<Q, B, Q>::IndCalcW), (T32<Q, B, Q>::IndCalcB),  90750420,  93824100)
    TB (kqrkp, false, false, false, (T32<Q, R, P>::IndCalcW), (T32<Q, R, P>::IndCalcB), 281568240, 298890240)
    TB (kqrkn, false, false, false, (T32<Q, R, N>::IndCalcW), (T32<Q, R, N>::IndCalcB),  90038460,  96192120)
    TB (kqrkb, false, false, false, (T32<Q, R, B>::IndCalcW), (T32<Q, R, B>::IndCalcB),  90038460,  99709380)
    TB (kqrkr, false, false, false, (T32<Q, R, R>::IndCalcW), (T32<Q, R, R>::IndCalcB),  90038460,  98951760)
    TB (kqrkq, false, false, false, (T32<Q, R, Q>::IndCalcW), (T32<Q, R, Q>::IndCalcB),  90038460,  93824100)
    TB (kqqkp, false, false, false, (T32<Q, Q, P>::IndCalcW), (T32<Q, Q, P>::IndCalcB), 131170128, 149445120)
    TB (kqqkn, false, false, false, (T32<Q, Q, N>::IndCalcW), (T32<Q, Q, N>::IndCalcB),  41944320,  48096060)
    TB (kqqkb, false, false, false, (T32<Q, Q, B>::IndCalcW), (T32<Q, Q, B>::IndCalcB),  41944320,  49854690)
    TB (kqqkr, false, false, false, (T32<Q, Q, R>::IndCalcW), (T32<Q, Q, R>::IndCalcB),  41944320,  49475880)
    TB (kqqkq, false, false, false, (T32<Q, Q, Q>::IndCalcW), (T32<Q, Q, Q>::IndCalcB),  41944320,  46912050)

#if defined (T41_INCLUDE)
    TB (kpppk, false, false, false, (T41<P, P, P>::IndCalcW), (T41<P, P, P>::IndCalcB),  26061704,  28388716)
    TB (knppk, false, false, false, (T41<N, P, P>::IndCalcW), (T41<N, P, P>::IndCalcB), 102898651, 114742320)
    TB (knnpk, false, false, false, (T41<N, N, P>::IndCalcW), (T41<N, N, P>::IndCalcB), 130135501, 153741960)
    TB (knnnk, false, false, false, (T41<N, N, N>::IndCalcW), (T41<N, N, N>::IndCalcB),  13486227,  17472840)
    TB (kbppk, false, false, false, (T41<B, P, P>::IndCalcW), (T41<B, P, P>::IndCalcB), 106602156, 114742320)
    TB (kbnpk, false, false, false, (T41<B, N, P>::IndCalcW), (T41<B, N, P>::IndCalcB), 274352939, 307483920)
    TB (kbnnk, false, false, false, (T41<B, N, N>::IndCalcW), (T41<B, N, N>::IndCalcB),  43406294, 52418520)
    TB (kbbpk, false, false, false, (T41<B, B, P>::IndCalcW), (T41<B, B, P>::IndCalcB), 139715040, 153741960)
    TB (kbbnk, false, false, false, (T41<B, B, N>::IndCalcW), (T41<B, B, N>::IndCalcB),  44983618,  52418520)
    TB (kbbbk, false, false, false, (T41<B, B, B>::IndCalcW), (T41<B, B, B>::IndCalcB),  15010230,  17472840)
    TB (krppk, false, false, false, (T41<R, P, P>::IndCalcW), (T41<R, P, P>::IndCalcB), 105758666, 114742320)
    TB (krnpk, false, false, false, (T41<R, N, P>::IndCalcW), (T41<R, N, P>::IndCalcB), 272153675, 307483920)
    TB (krnnk, false, false, false, (T41<R, N, N>::IndCalcW), (T41<R, N, N>::IndCalcB),  43056198,  52418520)
    TB (krbpk, false, false, false, (T41<R, B, P>::IndCalcW), (T41<R, B, P>::IndCalcB), 281991360, 307483920)
    TB (krbnk, false, false, false, (T41<R, B, N>::IndCalcW), (T41<R, B, N>::IndCalcB),  90787358, 104837040)
    TB (krbbk, false, false, false, (T41<R, B, B>::IndCalcW), (T41<R, B, B>::IndCalcB),  46242089,  52418520)
    TB (krrpk, false, false, false, (T41<R, R, P>::IndCalcW), (T41<R, R, P>::IndCalcB), 137491197, 153741960)
    TB (krrnk, false, false, false, (T41<R, R, N>::IndCalcW), (T41<R, R, N>::IndCalcB),  44265261,  52418520)
    TB (krrbk, false, false, false, (T41<R, R, B>::IndCalcW), (T41<R, R, B>::IndCalcB),  45873720,  52418520)
    TB (krrrk, false, false, false, (T41<R, R, R>::IndCalcW), (T41<R, R, R>::IndCalcB),  14644690,  17472840)
    TB (kqppk, false, false, false, (T41<Q, P, P>::IndCalcW), (T41<Q, P, P>::IndCalcB), 100347220, 114742320)
    TB (kqnpk, false, false, false, (T41<Q, N, P>::IndCalcW), (T41<Q, N, P>::IndCalcB), 258294639, 307483920)
    TB (kqnnk, false, false, false, (T41<Q, N, N>::IndCalcW), (T41<Q, N, N>::IndCalcB),  40873646,  52418520)
    TB (kqbpk, false, false, false, (T41<Q, B, P>::IndCalcW), (T41<Q, B, P>::IndCalcB), 267576632, 307483920)
    TB (kqbnk, false, false, false, (T41<Q, B, N>::IndCalcW), (T41<Q, B, N>::IndCalcB),  86166717, 104837040)
    TB (kqbbk, false, false, false, (T41<Q, B, B>::IndCalcW), (T41<Q, B, B>::IndCalcB),  43879679,  52418520)
    TB (kqrpk, false, false, false, (T41<Q, R, P>::IndCalcW), (T41<Q, R, P>::IndCalcB), 265421907, 307483920)
    TB (kqrnk, false, false, false, (T41<Q, R, N>::IndCalcW), (T41<Q, R, N>::IndCalcB),  85470603, 104837040)
    TB (kqrbk, false, false, false, (T41<Q, R, B>::IndCalcW), (T41<Q, R, B>::IndCalcB),  88557959, 104837040)
    TB (kqrrk, false, false, false, (T41<Q, R, R>::IndCalcW), (T41<Q, R, R>::IndCalcB),  43157690,  52418520)
    TB (kqqpk, false, false, false, (T41<Q, Q, P>::IndCalcW), (T41<Q, Q, P>::IndCalcB), 123688859, 153741960)
    TB (kqqnk, false, false, false, (T41<Q, Q, N>::IndCalcW), (T41<Q, Q, N>::IndCalcB),  39840787,  52418520)
    TB (kqqbk, false, false, false, (T41<Q, Q, B>::IndCalcW), (T41<Q, Q, B>::IndCalcB),  41270973,  52418520)
    TB (kqqrk, false, false, false, (T41<Q, Q, R>::IndCalcW), (T41<Q, Q, R>::IndCalcB),  40916820,  52418520)
    TB (kqqqk, false, false, false, (T41<Q, Q, Q>::IndCalcW), (T41<Q, Q, Q>::IndCalcB),  12479974,  17472840)
#endif

#if defined (T33_INCLUDE)
    TB (knnknn, true,  false, false, (T33<N, N, N, N>::IndCalcW), (T33<N, N, N, N>::IndCalcB),  1301488080,  1301488080)
    TB (kbnknn, false, false, false, (T33<B, N, N, N>::IndCalcW), (T33<B, N, N, N>::IndCalcB),  2744597400,  2602976160)
    TB (kbbknn, false, false, false, (T33<B, B, N, N>::IndCalcW), (T33<B, B, N, N>::IndCalcB),  1398096450,  1301488080)
    TB (kbbkbn, false, false, false, (T33<B, B, B, N>::IndCalcW), (T33<B, B, B, N>::IndCalcB),  2796192900,  2744597400)
    TB (kbbkbb, true,  false, false, (T33<B, B, B, B>::IndCalcW), (T33<B, B, B, B>::IndCalcB),  1398096450,  1398096450)
    TB (krnknn, false, true,  false, (T33<R, N, N, N>::IndCalcW), (T33<R, N, N, N>::IndCalcB),  2723107830,  2602976160)
    TB (krnkbb, false, true,  false, (T33<R, N, B, B>::IndCalcW), (T33<R, N, B, B>::IndCalcB),  2723107830,  2796192900)
    TB (krbknn, false, true,  false, (T33<R, B, N, N>::IndCalcW), (T33<R, B, N, N>::IndCalcB),  2822371200,  2602976160)
    TB (krbkbb, false, false, false, (T33<R, B, B, B>::IndCalcW), (T33<R, B, B, B>::IndCalcB),  2822371200,  2796192900)
    TB (krrknn, false, false, false, (T33<R, R, N, N>::IndCalcW), (T33<R, R, N, N>::IndCalcB),  1376421030,  1301488080)
    TB (krrkbn, false, false, false, (T33<R, R, B, N>::IndCalcW), (T33<R, R, B, N>::IndCalcB),  2752842060,  2744597400)
    TB (krrkbb, false, false, false, (T33<R, R, B, B>::IndCalcW), (T33<R, R, B, B>::IndCalcB),  1376421030,  1398096450)
    TB (krrkrn, false, false, false, (T33<R, R, R, N>::IndCalcW), (T33<R, R, R, N>::IndCalcB),  2752842060,  2723107830)
    TB (krrkrb, false, false, false, (T33<R, R, R, B>::IndCalcW), (T33<R, R, R, B>::IndCalcB),  2752842060,  2822371200)
    TB (krrkrr, true,  false, false, (T33<R, R, R, R>::IndCalcW), (T33<R, R, R, R>::IndCalcB),  1376421030,  1376421030)
    TB (kqnknn, false, false, false, (T33<Q, N, N, N>::IndCalcW), (T33<Q, N, N, N>::IndCalcB),  2583520320,  2602976160)
    TB (kqnkbb, false, false, false, (T33<Q, N, B, B>::IndCalcW), (T33<Q, N, B, B>::IndCalcB),  2583520320,  2796192900)
    TB (kqnkrr, false, true,  false, (T33<Q, N, R, R>::IndCalcW), (T33<Q, N, R, R>::IndCalcB),  2583520320,  2752842060)
    TB (kqbknn, false, false, false, (T33<Q, B, N, N>::IndCalcW), (T33<Q, B, N, N>::IndCalcB),  2677137390,  2602976160)
    TB (kqbkbb, false, false, false, (T33<Q, B, B, B>::IndCalcW), (T33<Q, B, B, B>::IndCalcB),  2677137390,  2796192900)
    TB (kqbkrr, false, false, false, (T33<Q, B, R, R>::IndCalcW), (T33<Q, B, R, R>::IndCalcB),  2677137390,  2752842060)
    TB (kqrknn, false, false, false, (T33<Q, R, N, N>::IndCalcW), (T33<Q, R, N, N>::IndCalcB),  2656134570,  2602976160)
    TB (kqrkbb, false, false, false, (T33<Q, R, B, B>::IndCalcW), (T33<Q, R, B, B>::IndCalcB),  2656134570,  2796192900)
    TB (kqrkrr, false, false, false, (T33<Q, R, R, R>::IndCalcW), (T33<Q, R, R, R>::IndCalcB),  2656134570,  2752842060)
    TB (kqqknn, false, false, false, (T33<Q, Q, N, N>::IndCalcW), (T33<Q, Q, N, N>::IndCalcB),  1237357440,  1301488080)
    TB (kqqkbn, false, false, false, (T33<Q, Q, B, N>::IndCalcW), (T33<Q, Q, B, N>::IndCalcB),  2474714880,  2744597400)
    TB (kqqkbb, false, false, false, (T33<Q, Q, B, B>::IndCalcW), (T33<Q, Q, B, B>::IndCalcB),  1237357440,  1398096450)
    TB (kqqkrn, false, false, false, (T33<Q, Q, R, N>::IndCalcW), (T33<Q, Q, R, N>::IndCalcB),  2474714880,  2723107830)
    TB (kqqkrb, false, false, false, (T33<Q, Q, R, B>::IndCalcW), (T33<Q, Q, R, B>::IndCalcB),  2474714880,  2822371200)
    TB (kqqkrr, false, false, false, (T33<Q, Q, R, R>::IndCalcW), (T33<Q, Q, R, R>::IndCalcB),  1237357440,  1376421030)
    TB (kqqkqn, false, false, false, (T33<Q, Q, Q, N>::IndCalcW), (T33<Q, Q, Q, N>::IndCalcB),  2474714880,  2583520320)
    TB (kqqkqb, false, false, false, (T33<Q, Q, Q, B>::IndCalcW), (T33<Q, Q, Q, B>::IndCalcB),  2474714880,  2677137390)
    TB (kqqkqr, false, false, false, (T33<Q, Q, Q, R>::IndCalcW), (T33<Q, Q, Q, R>::IndCalcB),  2474714880,  2656134570)
    TB (kqqkqq, true,  false, false, (T33<Q, Q, Q, Q>::IndCalcW), (T33<Q, Q, Q, Q>::IndCalcB),  1237357440,  1237357440)
#if defined (T_INDEX64)
    TB (kbnkbn, true,  false, false, (T33<B, N, B, N>::IndCalcW), (T33<B, N, B, N>::IndCalcB),  5489194800,  5489194800)
    TB (krnkrn, true,  false, false, (T33<R, N, R, N>::IndCalcW), (T33<R, N, R, N>::IndCalcB),  5446215660,  5446215660)
    TB (krbkrb, true,  false, false, (T33<R, B, R, B>::IndCalcW), (T33<R, B, R, B>::IndCalcB),  5644742400,  5644742400)
    TB (kqnkqn, true,  false, false, (T33<Q, N, Q, N>::IndCalcW), (T33<Q, N, Q, N>::IndCalcB),  5167040640,  5167040640)
    TB (kqbkqb, true,  false, false, (T33<Q, B, Q, B>::IndCalcW), (T33<Q, B, Q, B>::IndCalcB),  5354274780,  5354274780)
    TB (kqrkqr, true,  false, false, (T33<Q, R, Q, R>::IndCalcW), (T33<Q, R, Q, R>::IndCalcB),  5312269140,  5312269140)
    TB (krnkbn, false, true,  true,  (T33<R, N, B, N>::IndCalcW), (T33<R, N, B, N>::IndCalcB),  5446215660,  5489194800)
    TB (krbkbn, false, false, true,  (T33<R, B, B, N>::IndCalcW), (T33<R, B, B, N>::IndCalcB),  5644742400,  5489194800)
    TB (krbkrn, false, false, false, (T33<R, B, R, N>::IndCalcW), (T33<R, B, R, N>::IndCalcB),  5644742400,  5446215660)
    TB (kqnkbn, false, false, false, (T33<Q, N, B, N>::IndCalcW), (T33<Q, N, B, N>::IndCalcB),  5167040640,  5489194800)
    TB (kqnkrn, false, false, true,  (T33<Q, N, R, N>::IndCalcW), (T33<Q, N, R, N>::IndCalcB),  5167040640,  5446215660)
    TB (kqnkrb, false, false, true,  (T33<Q, N, R, B>::IndCalcW), (T33<Q, N, R, B>::IndCalcB),  5167040640,  5644742400)
    TB (kqbkbn, false, false, false, (T33<Q, B, B, N>::IndCalcW), (T33<Q, B, B, N>::IndCalcB),  5354274780,  5489194800)
    TB (kqbkrn, false, false, true,  (T33<Q, B, R, N>::IndCalcW), (T33<Q, B, R, N>::IndCalcB),  5354274780,  5446215660)
    TB (kqbkrb, false, false, true,  (T33<Q, B, R, B>::IndCalcW), (T33<Q, B, R, B>::IndCalcB),  5354274780,  5644742400)
    TB (kqbkqn, false, false, false, (T33<Q, B, Q, N>::IndCalcW), (T33<Q, B, Q, N>::IndCalcB),  5354274780,  5167040640)
    TB (kqrkbn, false, false, false, (T33<Q, R, B, N>::IndCalcW), (T33<Q, R, B, N>::IndCalcB),  5312269140,  5489194800)
    TB (kqrkrn, false, false, false, (T33<Q, R, R, N>::IndCalcW), (T33<Q, R, R, N>::IndCalcB),  5312269140,  5446215660)
    TB (kqrkrb, false, false, false, (T33<Q, R, R, B>::IndCalcW), (T33<Q, R, R, B>::IndCalcB),  5312269140,  5644742400)
    TB (kqrkqn, false, false, false, (T33<Q, R, Q, N>::IndCalcW), (T33<Q, R, Q, N>::IndCalcB),  5312269140,  5167040640)
    TB (kqrkqb, false, false, false, (T33<Q, R, Q, B>::IndCalcW), (T33<Q, R, Q, B>::IndCalcB),  5312269140,  5354274780)

    TB (kppkpp, true,  true,  false, (T33<P, P, P, P>::IndCalcW), (T33<P, P, P, P>::IndCalcBF),  1917741812,  1917741812)
    TB (knpkpp, false, true,  true,  (T33<N, P, P, P>::IndCalcW), (T33<N, P, P, P>::IndCalcB),   5088408829,  4966717366)
    TB (knpknp, true,  false, true,  (T33<N, P, N, P>::IndCalcW), (T33<N, P, N, P>::IndCalcBF), 12972508017, 12972508017)
    TB (knnkpp, false, true,  false, (T33<N, N, P, P>::IndCalcW), (T33<N, N, P, P>::IndCalcB),   3242803728,  3197807670)
    TB (knnknp, false, true,  true,  (T33<N, N, N, P>::IndCalcW), (T33<N, N, N, P>::IndCalcB),   8141507232,  8227988370)
    TB (kbpkpp, false, true,  true,  (T33<B, P, P, P>::IndCalcW), (T33<B, P, P, P>::IndCalcB),   5272919368,  4966717366)
    TB (kbpknp, false, true,  true,  (T33<B, P, N, P>::IndCalcW), (T33<B, P, N, P>::IndCalcB),  13442882944, 12972508017)
    TB (kbpknn, false, true,  true,  (T33<B, P, N, N>::IndCalcW), (T33<B, P, N, N>::IndCalcB),   8526316560,  8141507232)
    TB (kbpkbp, true,  false, true,  (T33<B, P, B, P>::IndCalcW), (T33<B, P, B, P>::IndCalcBF), 13442882944, 13442882944)
    TB (kbnkpp, false, true,  true,  (T33<B, N, P, P>::IndCalcW), (T33<B, N, P, P>::IndCalcB),   6838255224,  6395615340)
    TB (kbnknp, false, true,  true,  (T33<B, N, N, P>::IndCalcW), (T33<B, N, N, P>::IndCalcB),  17168385456, 16455976740)
    TB (kbnkbp, false, false, true,  (T33<B, N, B, P>::IndCalcW), (T33<B, N, B, P>::IndCalcB),  17168385456, 17052633120)
    TB (kbbkpp, false, false, false, (T33<B, B, P, P>::IndCalcW), (T33<B, B, P, P>::IndCalcB),   3483252720,  3197807670)
    TB (kbbknp, false, true,  true,  (T33<B, B, N, P>::IndCalcW), (T33<B, B, N, P>::IndCalcB),   8745187680,  8227988370)
    TB (kbbkbp, false, false, true,  (T33<B, B, B, P>::IndCalcW), (T33<B, B, B, P>::IndCalcB),   8745187680,  8526316560)
    TB (krpkpp, false, true,  true,  (T33<R, P, P, P>::IndCalcW), (T33<R, P, P, P>::IndCalcB),   5231873656,  4966717366)
    TB (krpknp, false, true,  true,  (T33<R, P, N, P>::IndCalcW), (T33<R, P, N, P>::IndCalcB),  13338233184, 12972508017)
    TB (krpknn, false, true,  true,  (T33<R, P, N, N>::IndCalcW), (T33<R, P, N, N>::IndCalcB),   8459934480,  8141507232)
    TB (krpkbp, false, true,  true,  (T33<R, P, B, P>::IndCalcW), (T33<R, P, B, P>::IndCalcB),  13338233184, 13442882944)
    TB (krpkbn, false, true,  true,  (T33<R, P, B, N>::IndCalcW), (T33<R, P, B, N>::IndCalcB),  16919868960, 17168385456)
    TB (krpkbb, false, true,  true,  (T33<R, P, B, B>::IndCalcW), (T33<R, P, B, B>::IndCalcB),   8459934480,  8745187680)
    TB (krpkrp, true,  false, true,  (T33<R, P, R, P>::IndCalcW), (T33<R, P, R, P>::IndCalcBF), 13338233184, 13338233184)
    TB (krnkpp, false, true,  true,  (T33<R, N, P, P>::IndCalcW), (T33<R, N, P, P>::IndCalcB),   6784283808,  6395615340)
    TB (krnknp, false, true,  true,  (T33<R, N, N, P>::IndCalcW), (T33<R, N, N, P>::IndCalcB),  17032882752, 16455976740)
    TB (krnkbp, false, true,  true,  (T33<R, N, B, P>::IndCalcW), (T33<R, N, B, P>::IndCalcB),  17032882752, 17052633120)
    TB (krnkrp, false, false, true,  (T33<R, N, R, P>::IndCalcW), (T33<R, N, R, P>::IndCalcB),  17032882752, 16919868906)
    TB (krbkpp, false, true,  true,  (T33<R, B, P, P>::IndCalcW), (T33<R, B, P, P>::IndCalcB),   7031275200,  6395615340)
    TB (krbknp, false, true,  true,  (T33<R, B, N, P>::IndCalcW), (T33<R, B, N, P>::IndCalcB),  17652988800, 16455976740)
    TB (krbkbp, false, false, true,  (T33<R, B, B, P>::IndCalcW), (T33<R, B, B, P>::IndCalcB),  17652988800, 17052633120)
    TB (krbkrp, false, false, true,  (T33<R, B, R, P>::IndCalcW), (T33<R, B, R, P>::IndCalcB),  17652988800, 16919868906)
    TB (krrkpp, false, true,  false, (T33<R, R, P, P>::IndCalcW), (T33<R, R, P, P>::IndCalcB),   3428678952,  3197807670)
    TB (krrknp, false, true,  true,  (T33<R, R, N, P>::IndCalcW), (T33<R, R, N, P>::IndCalcB),   8608504032,  8227988370)
    TB (krrkbp, false, false, true,  (T33<R, R, B, P>::IndCalcW), (T33<R, R, B, P>::IndCalcB),   8608504032,  8526316560)
    TB (krrkrp, false, false, true,  (T33<R, R, R, P>::IndCalcW), (T33<R, R, R, P>::IndCalcB),   8608504032,  8459934426)
    TB (kqpkpp, false, true,  true,  (T33<Q, P, P, P>::IndCalcW), (T33<Q, P, P, P>::IndCalcB),   4962533664,  4966717366)
    TB (kqpknp, false, false, true,  (T33<Q, P, N, P>::IndCalcW), (T33<Q, P, N, P>::IndCalcB),  12651597608, 12972508017)
    TB (kqpknn, false, false, true,  (T33<Q, P, N, N>::IndCalcW), (T33<Q, P, N, N>::IndCalcB),   8024443680,  8141507232)
    TB (kqpkbp, false, false, true,  (T33<Q, P, B, P>::IndCalcW), (T33<Q, P, B, P>::IndCalcB),  12651597608, 13442882944)
    TB (kqpkbn, false, false, true,  (T33<Q, P, B, N>::IndCalcW), (T33<Q, P, B, N>::IndCalcB),  16048887360, 17168385456)
    TB (kqpkbb, false, false, true,  (T33<Q, P, B, B>::IndCalcW), (T33<Q, P, B, B>::IndCalcB),   8024443680,  8745187680)
    TB (kqpkrp, false, false, true,  (T33<Q, P, R, P>::IndCalcW), (T33<Q, P, R, P>::IndCalcB),  12651597608, 13338233184)
    TB (kqpkrn, false, false, true,  (T33<Q, P, R, N>::IndCalcW), (T33<Q, P, R, N>::IndCalcB),  16048887360, 17032882752)
    TB (kqpkrb, false, true,  true,  (T33<Q, P, R, B>::IndCalcW), (T33<Q, P, R, B>::IndCalcB),  16048887360, 17652988800)
    TB (kqpkrr, false, true,  true,  (T33<Q, P, R, R>::IndCalcW), (T33<Q, P, R, R>::IndCalcB),   8024443680,  8608504032)
    TB (kqpkqp, true,  true,  true,  (T33<Q, P, Q, P>::IndCalcW), (T33<Q, P, Q, P>::IndCalcBF), 12651597608, 12651597608)
    TB (kqnkpp, false, true,  true,  (T33<Q, N, P, P>::IndCalcW), (T33<Q, N, P, P>::IndCalcB),   6436756032,  6395615340)
    TB (kqnknp, false, false, true,  (T33<Q, N, N, P>::IndCalcW), (T33<Q, N, N, P>::IndCalcB),  16160366208, 16455976740)
    TB (kqnkbp, false, false, true,  (T33<Q, N, B, P>::IndCalcW), (T33<Q, N, B, P>::IndCalcB),  16160366208, 17052633120)
    TB (kqnkrp, false, false, true,  (T33<Q, N, R, P>::IndCalcW), (T33<Q, N, R, P>::IndCalcB),  16160366208, 16919868906)
    TB (kqnkqp, false, true,  true,  (T33<Q, N, Q, P>::IndCalcW), (T33<Q, N, Q, P>::IndCalcB),  16160366208, 16048887306)
    TB (kqbkpp, false, false, true,  (T33<Q, B, P, P>::IndCalcW), (T33<Q, B, P, P>::IndCalcB),   6669728640,  6395615340)
    TB (kqbknp, false, false, true,  (T33<Q, B, N, P>::IndCalcW), (T33<Q, B, N, P>::IndCalcB),  16745276160, 16455976740)
    TB (kqbkbp, false, false, true,  (T33<Q, B, B, P>::IndCalcW), (T33<Q, B, B, P>::IndCalcB),  16745276160, 17052633120)
    TB (kqbkrp, false, false, true,  (T33<Q, B, R, P>::IndCalcW), (T33<Q, B, R, P>::IndCalcB),  16745276160, 16919868906)
    TB (kqbkqp, false, true,  true,  (T33<Q, B, Q, P>::IndCalcW), (T33<Q, B, Q, P>::IndCalcB),  16745276160, 16048887306)
    TB (kqrkpp, false, false, true,  (T33<Q, R, P, P>::IndCalcW), (T33<Q, R, P, P>::IndCalcB),   6616853640,  6395615340)
    TB (kqrknp, false, false, true,  (T33<Q, R, N, P>::IndCalcW), (T33<Q, R, N, P>::IndCalcB),  16612871664, 16455976740)
    TB (kqrkbp, false, false, true,  (T33<Q, R, B, P>::IndCalcW), (T33<Q, R, B, P>::IndCalcB),  16612871664, 17052633120)
    TB (kqrkrp, false, false, true,  (T33<Q, R, R, P>::IndCalcW), (T33<Q, R, R, P>::IndCalcB),  16612871664, 16919868906)
    TB (kqrkqp, false, true,  true,  (T33<Q, R, Q, P>::IndCalcW), (T33<Q, R, Q, P>::IndCalcB),  16612871664, 16048887306)
    TB (kqqkpp, false, false, false, (T33<Q, Q, P, P>::IndCalcW), (T33<Q, Q, P, P>::IndCalcB),   3082498008,  3197807670)
    TB (kqqknp, false, false, true,  (T33<Q, Q, N, P>::IndCalcW), (T33<Q, Q, N, P>::IndCalcB),   7739363232,  8227988370)
    TB (kqqkbp, false, false, true,  (T33<Q, Q, B, P>::IndCalcW), (T33<Q, Q, B, P>::IndCalcB),   7739363232,  8526316560)
    TB (kqqkrp, false, false, true,  (T33<Q, Q, R, P>::IndCalcW), (T33<Q, Q, R, P>::IndCalcB),   7739363232,  8459934426)
    TB (kqqkqp, false, false, true,  (T33<Q, Q, Q, P>::IndCalcW), (T33<Q, Q, Q, P>::IndCalcB),   7739363232,  8024443626)
#endif  // T_INDEX64
#endif  // T33_INCLUDE

#if defined (T42_INCLUDE)
    TB (knnnkn, false, false, false, (T42<N, N, N, N>::IndCalcW), (T42<N, N, N, N>::IndCalcB),  795687393,  945889180)
    TB (kbnnkn, false, false, false, (T42<B, N, N, N>::IndCalcW), (T42<B, N, N, N>::IndCalcB), 2560971346, 2837667540)
    TB (kbbnkn, false, false, false, (T42<B, B, N, N>::IndCalcW), (T42<B, B, N, N>::IndCalcB), 2654033462, 2837667540)
    TB (kbbbkn, false, false, false, (T42<B, B, B, N>::IndCalcW), (T42<B, B, B, N>::IndCalcB),  885603570,  945889180)
    TB (krnnkn, false, false, false, (T42<R, N, N, N>::IndCalcW), (T42<R, N, N, N>::IndCalcB), 2540315682, 2837667540)
    TB (krbbkn, false, false, false, (T42<R, B, B, N>::IndCalcW), (T42<R, B, B, N>::IndCalcB), 2728283251, 2837667540)
    TB (krrnkn, false, false, false, (T42<R, R, N, N>::IndCalcW), (T42<R, R, N, N>::IndCalcB), 2611650399, 2837667540)
    TB (krrbkn, false, false, false, (T42<R, R, B, N>::IndCalcW), (T42<R, R, B, N>::IndCalcB), 2706549480, 2837667540)
    TB (krrrkn, false, false, false, (T42<R, R, R, N>::IndCalcW), (T42<R, R, R, N>::IndCalcB),  864592254,  945889180)
    TB (kqnnkn, false, false, false, (T42<Q, N, N, N>::IndCalcW), (T42<Q, N, N, N>::IndCalcB), 2411545114, 2837667540)
    TB (kqbbkn, false, false, false, (T42<Q, B, B, N>::IndCalcW), (T42<Q, B, B, N>::IndCalcB), 2588901061, 2837667540)
    TB (kqrrkn, false, false, false, (T42<Q, R, R, N>::IndCalcW), (T42<Q, R, R, N>::IndCalcB), 2547484064, 2837667540)
    TB (kqqnkn, false, false, false, (T42<Q, Q, N, N>::IndCalcW), (T42<Q, Q, N, N>::IndCalcB), 2350606433, 2837667540)
    TB (kqqbkn, false, false, false, (T42<Q, Q, B, N>::IndCalcW), (T42<Q, Q, B, N>::IndCalcB), 2434987407, 2837667540)
    TB (kqqrkn, false, false, false, (T42<Q, Q, R, N>::IndCalcW), (T42<Q, Q, R, N>::IndCalcB), 2415271436, 2837667540)
    TB (kqqqkn, false, false, false, (T42<Q, Q, Q, N>::IndCalcW), (T42<Q, Q, Q, N>::IndCalcB),  736854363,  945889180)
    TB (knnnkb, false, false, false, (T42<N, N, N, B>::IndCalcW), (T42<N, N, N, B>::IndCalcB),  795687393,  980475570)
    TB (kbnnkb, false, false, false, (T42<B, N, N, B>::IndCalcW), (T42<B, N, N, B>::IndCalcB), 2560971346, 2941426710)
    TB (kbbnkb, false, false, false, (T42<B, B, N, B>::IndCalcW), (T42<B, B, N, B>::IndCalcB), 2654033462, 2941426710)
    TB (kbbbkb, false, false, false, (T42<B, B, B, B>::IndCalcW), (T42<B, B, B, B>::IndCalcB),  885603570,  980475570)
    TB (krnnkb, false, false, false, (T42<R, N, N, B>::IndCalcW), (T42<R, N, N, B>::IndCalcB), 2540315682, 2941426710)
    TB (krbbkb, false, false, false, (T42<R, B, B, B>::IndCalcW), (T42<R, B, B, B>::IndCalcB), 2728283251, 2941426710)
    TB (krrnkb, false, false, false, (T42<R, R, N, B>::IndCalcW), (T42<R, R, N, B>::IndCalcB), 2611650399, 2941426710)
    TB (krrbkb, false, false, false, (T42<R, R, B, B>::IndCalcW), (T42<R, R, B, B>::IndCalcB), 2706549480, 2941426710)
    TB (krrrkb, false, false, false, (T42<R, R, R, B>::IndCalcW), (T42<R, R, R, B>::IndCalcB),  864592254,  980475570)
    TB (kqnnkb, false, false, false, (T42<Q, N, N, B>::IndCalcW), (T42<Q, N, N, B>::IndCalcB), 2411545114, 2941426710)
    TB (kqbbkb, false, false, false, (T42<Q, B, B, B>::IndCalcW), (T42<Q, B, B, B>::IndCalcB), 2588901061, 2941426710)
    TB (kqrrkb, false, false, false, (T42<Q, R, R, B>::IndCalcW), (T42<Q, R, R, B>::IndCalcB), 2547484064, 2941426710)
    TB (kqqnkb, false, false, false, (T42<Q, Q, N, B>::IndCalcW), (T42<Q, Q, N, B>::IndCalcB), 2350606433, 2941426710)
    TB (kqqbkb, false, false, false, (T42<Q, Q, B, B>::IndCalcW), (T42<Q, Q, B, B>::IndCalcB), 2434987407, 2941426710)
    TB (kqqrkb, false, false, false, (T42<Q, Q, R, B>::IndCalcW), (T42<Q, Q, R, B>::IndCalcB), 2415271436, 2941426710)
    TB (kqqqkb, false, false, false, (T42<Q, Q, Q, B>::IndCalcW), (T42<Q, Q, Q, B>::IndCalcB),  736854363,  980475570)
    TB (knnnkr, false, false, false, (T42<N, N, N, R>::IndCalcW), (T42<N, N, N, R>::IndCalcB),  795687393,  973025640)
    TB (kbnnkr, false, false, false, (T42<B, N, N, R>::IndCalcW), (T42<B, N, N, R>::IndCalcB), 2560971346, 2919076920)
    TB (kbbnkr, false, false, false, (T42<B, B, N, R>::IndCalcW), (T42<B, B, N, R>::IndCalcB), 2654033462, 2919076920)
    TB (kbbbkr, false, false, false, (T42<B, B, B, R>::IndCalcW), (T42<B, B, B, R>::IndCalcB),  885603570,  973025640)
    TB (krnnkr, false, false, false, (T42<R, N, N, R>::IndCalcW), (T42<R, N, N, R>::IndCalcB), 2540315682, 2919076920)
    TB (krbbkr, false, false, false, (T42<R, B, B, R>::IndCalcW), (T42<R, B, B, R>::IndCalcB), 2728283251, 2919076920)
    TB (krrnkr, false, false, false, (T42<R, R, N, R>::IndCalcW), (T42<R, R, N, R>::IndCalcB), 2611650399, 2919076920)
    TB (krrbkr, false, false, false, (T42<R, R, B, R>::IndCalcW), (T42<R, R, B, R>::IndCalcB), 2706549480, 2919076920)
    TB (krrrkr, false, false, false, (T42<R, R, R, R>::IndCalcW), (T42<R, R, R, R>::IndCalcB),  864592254,  973025640)
    TB (kqnnkr, false, false, false, (T42<Q, N, N, R>::IndCalcW), (T42<Q, N, N, R>::IndCalcB), 2411545114, 2919076920)
    TB (kqbbkr, false, false, false, (T42<Q, B, B, R>::IndCalcW), (T42<Q, B, B, R>::IndCalcB), 2588901061, 2919076920)
    TB (kqrrkr, false, false, false, (T42<Q, R, R, R>::IndCalcW), (T42<Q, R, R, R>::IndCalcB), 2547484064, 2919076920)
    TB (kqqnkr, false, false, false, (T42<Q, Q, N, R>::IndCalcW), (T42<Q, Q, N, R>::IndCalcB), 2350606433, 2919076920)
    TB (kqqbkr, false, false, false, (T42<Q, Q, B, R>::IndCalcW), (T42<Q, Q, B, R>::IndCalcB), 2434987407, 2919076920)
    TB (kqqrkr, false, false, false, (T42<Q, Q, R, R>::IndCalcW), (T42<Q, Q, R, R>::IndCalcB), 2415271436, 2919076920)
    TB (kqqqkr, false, false, false, (T42<Q, Q, Q, R>::IndCalcW), (T42<Q, Q, Q, R>::IndCalcB),  736854363,  973025640)
    TB (knnnkq, false, false, false, (T42<N, N, N, Q>::IndCalcW), (T42<N, N, N, Q>::IndCalcB),  795687393,  922603650)
    TB (kbnnkq, false, false, false, (T42<B, N, N, Q>::IndCalcW), (T42<B, N, N, Q>::IndCalcB), 2560971346, 2767810950)
    TB (kbbnkq, false, false, false, (T42<B, B, N, Q>::IndCalcW), (T42<B, B, N, Q>::IndCalcB), 2654033462, 2767810950)
    TB (kbbbkq, false, false, false, (T42<B, B, B, Q>::IndCalcW), (T42<B, B, B, Q>::IndCalcB),  885603570,  922603650)
    TB (krnnkq, false, false, false, (T42<R, N, N, Q>::IndCalcW), (T42<R, N, N, Q>::IndCalcB), 2540315682, 2767810950)
    TB (krbbkq, false, false, false, (T42<R, B, B, Q>::IndCalcW), (T42<R, B, B, Q>::IndCalcB), 2728283251, 2767810950)
    TB (krrnkq, false, false, false, (T42<R, R, N, Q>::IndCalcW), (T42<R, R, N, Q>::IndCalcB), 2611650399, 2767810950)
    TB (krrbkq, false, false, false, (T42<R, R, B, Q>::IndCalcW), (T42<R, R, B, Q>::IndCalcB), 2706549480, 2767810950)
    TB (krrrkq, false, false, false, (T42<R, R, R, Q>::IndCalcW), (T42<R, R, R, Q>::IndCalcB),  864592254,  922603650)
    TB (kqnnkq, false, false, false, (T42<Q, N, N, Q>::IndCalcW), (T42<Q, N, N, Q>::IndCalcB), 2411545114, 2767810950)
    TB (kqbbkq, false, false, false, (T42<Q, B, B, Q>::IndCalcW), (T42<Q, B, B, Q>::IndCalcB), 2588901061, 2767810950)
    TB (kqrrkq, false, false, false, (T42<Q, R, R, Q>::IndCalcW), (T42<Q, R, R, Q>::IndCalcB), 2547484064, 2767810950)
    TB (kqqnkq, false, false, false, (T42<Q, Q, N, Q>::IndCalcW), (T42<Q, Q, N, Q>::IndCalcB), 2350606433, 2767810950)
    TB (kqqbkq, false, false, false, (T42<Q, Q, B, Q>::IndCalcW), (T42<Q, Q, B, Q>::IndCalcB), 2434987407, 2767810950)
    TB (kqqrkq, false, false, false, (T42<Q, Q, R, Q>::IndCalcW), (T42<Q, Q, R, Q>::IndCalcB), 2415271436, 2767810950)
    TB (kqqqkq, false, false, false, (T42<Q, Q, Q, Q>::IndCalcW), (T42<Q, Q, Q, Q>::IndCalcB),  736854363,  922603650)
#if defined (T_INDEX64)
    TB (krbnkn, false, false, false, (T42<R, B, N, N>::IndCalcW), (T42<R, B, N, N>::IndCalcB), 5356454122, 5675335080)
    TB (kqbnkn, false, false, false, (T42<Q, B, N, N>::IndCalcW), (T42<Q, B, N, N>::IndCalcB), 5083836303, 5675335080)
    TB (kqrnkn, false, false, false, (T42<Q, R, N, N>::IndCalcW), (T42<Q, R, N, N>::IndCalcB), 5042765577, 5675335080)
    TB (kqrbkn, false, false, false, (T42<Q, R, B, N>::IndCalcW), (T42<Q, R, B, N>::IndCalcB), 5224919581, 5675335080)
    TB (krbnkb, false, false, false, (T42<R, B, N, B>::IndCalcW), (T42<R, B, N, B>::IndCalcB), 5356454122, 5882853420)
    TB (kqbnkb, false, false, false, (T42<Q, B, N, B>::IndCalcW), (T42<Q, B, N, B>::IndCalcB), 5083836303, 5882853420)
    TB (kqrnkb, false, false, false, (T42<Q, R, N, B>::IndCalcW), (T42<Q, R, N, B>::IndCalcB), 5042765577, 5882853420)
    TB (kqrbkb, false, false, false, (T42<Q, R, B, B>::IndCalcW), (T42<Q, R, B, B>::IndCalcB), 5224919581, 5882853420)
    TB (krbnkr, false, false, false, (T42<R, B, N, R>::IndCalcW), (T42<R, B, N, R>::IndCalcB), 5356454122, 5838153840)
    TB (kqbnkr, false, false, false, (T42<Q, B, N, R>::IndCalcW), (T42<Q, B, N, R>::IndCalcB), 5083836303, 5838153840)
    TB (kqrnkr, false, false, false, (T42<Q, R, N, R>::IndCalcW), (T42<Q, R, N, R>::IndCalcB), 5042765577, 5838153840)
    TB (kqrbkr, false, false, false, (T42<Q, R, B, R>::IndCalcW), (T42<Q, R, B, R>::IndCalcB), 5224919581, 5838153840)
    TB (krbnkq, false, false, false, (T42<R, B, N, Q>::IndCalcW), (T42<R, B, N, Q>::IndCalcB), 5356454122, 5535621900)
    TB (kqbnkq, false, false, true,  (T42<Q, B, N, Q>::IndCalcW), (T42<Q, B, N, Q>::IndCalcB), 5083836303, 5535621900)
    TB (kqrnkq, false, false, false, (T42<Q, R, N, Q>::IndCalcW), (T42<Q, R, N, Q>::IndCalcB), 5042765577, 5535621900)
    TB (kqrbkq, false, false, false, (T42<Q, R, B, Q>::IndCalcW), (T42<Q, R, B, Q>::IndCalcB), 5224919581, 5535621900)

    TB (kpppkp, false, true,  false, (T42<P, P, P, P>::IndCalcW), (T42<P, P, P, P>::IndCalcB),  1196695343,  1348100424)
    TB (knppkp, false, true,  true,  (T42<N, P, P, P>::IndCalcW), (T42<N, P, P, P>::IndCalcB),  4796630713,  5271860528)
    TB (knnpkp, false, false, true,  (T42<N, N, P, P>::IndCalcW), (T42<N, N, P, P>::IndCalcB),  6156616111,  6835422612)
    TB (knnnkp, false, false, false, (T42<N, N, N, P>::IndCalcW), (T42<N, N, N, P>::IndCalcB),  2531022144,  2939087360)
    TB (kbppkp, false, true,  true,  (T42<B, P, P, P>::IndCalcW), (T42<B, P, P, P>::IndCalcB),  4969303175,  5271860528)
    TB (kbnpkp, false, true,  true,  (T42<B, N, P, P>::IndCalcW), (T42<B, N, P, P>::IndCalcB), 12979462304, 13670845224)
    TB (kbnnkp, false, false, true,  (T42<B, N, N, P>::IndCalcW), (T42<B, N, N, P>::IndCalcB),  8146120416,  8817262080)
    TB (kbbpkp, false, true,  true,  (T42<B, B, P, P>::IndCalcW), (T42<B, B, P, P>::IndCalcB),  6609838740,  6835422612)
    TB (kbbnkp, false, false, true,  (T42<B, B, N, P>::IndCalcW), (T42<B, B, N, P>::IndCalcB),  8441899104,  8817262080)
    TB (kbbbkp, false, false, false, (T42<B, B, B, P>::IndCalcW), (T42<B, B, B, P>::IndCalcB),  2816801280,  2939087360)
    TB (krppkp, false, true,  true,  (T42<R, P, P, P>::IndCalcW), (T42<R, P, P, P>::IndCalcB),  4929998839,  5271860528)
    TB (krnpkp, false, true,  true,  (T42<R, N, P, P>::IndCalcW), (T42<R, N, P, P>::IndCalcB), 12875424829, 13670845224)
    TB (krnnkp, false, false, true,  (T42<R, N, N, P>::IndCalcW), (T42<R, N, N, P>::IndCalcB),  8079921360,  8817262080)
    TB (krbpkp, false, true,  true,  (T42<R, B, P, P>::IndCalcW), (T42<R, B, P, P>::IndCalcB), 13340861520, 13670845224)
    TB (krbnkp, false, false, true,  (T42<R, B, N, P>::IndCalcW), (T42<R, B, N, P>::IndCalcB), 17036639904, 17634524160)
    TB (krbbkp, false, false, true,  (T42<R, B, B, P>::IndCalcW), (T42<R, B, B, P>::IndCalcB),  8677177872,  8817262080)
    TB (krrpkp, false, true,  true,  (T42<R, R, P, P>::IndCalcW), (T42<R, R, P, P>::IndCalcB),  6504899238,  6835422612)
    TB (krrnkp, false, false, true,  (T42<R, R, N, P>::IndCalcW), (T42<R, R, N, P>::IndCalcB),  8306047872,  8817262080)
    TB (krrbkp, false, false, true,  (T42<R, R, B, P>::IndCalcW), (T42<R, R, B, P>::IndCalcB),  8607504960,  8817262080)
    TB (krrrkp, false, false, false, (T42<R, R, R, P>::IndCalcW), (T42<R, R, R, P>::IndCalcB),  2749283520,  2939087360)
    TB (kqppkp, false, true,  true,  (T42<Q, P, P, P>::IndCalcW), (T42<Q, P, P, P>::IndCalcB),  4677701571,  5271860528)
    TB (kqnpkp, false, true,  true,  (T42<Q, N, P, P>::IndCalcW), (T42<Q, N, P, P>::IndCalcB), 12219736849, 13670845224)
    TB (kqnnkp, false, false, true,  (T42<Q, N, N, P>::IndCalcW), (T42<Q, N, N, P>::IndCalcB),  7670559696,  8817262080)
    TB (kqbpkp, false, true,  true,  (T42<Q, B, P, P>::IndCalcW), (T42<Q, B, P, P>::IndCalcB), 12658882024, 13670845224)
    TB (kqbnkp, false, false, true,  (T42<Q, B, N, P>::IndCalcW), (T42<Q, B, N, P>::IndCalcB), 16170070752, 17634524160)
    TB (kqbbkp, false, false, true,  (T42<Q, B, B, P>::IndCalcW), (T42<Q, B, B, P>::IndCalcB),  8234170512,  8817262080)
    TB (kqrpkp, false, false, true,  (T42<Q, R, P, P>::IndCalcW), (T42<Q, R, P, P>::IndCalcB), 12557225406, 13670845224)
    TB (kqrnkp, false, false, true,  (T42<Q, R, N, P>::IndCalcW), (T42<Q, R, N, P>::IndCalcB), 16038464256, 17634524160)
    TB (kqrbkp, false, false, true,  (T42<Q, R, B, P>::IndCalcW), (T42<Q, R, B, P>::IndCalcB), 16617170832, 17634524160)
    TB (kqrrkp, false, false, true,  (T42<Q, R, R, P>::IndCalcW), (T42<Q, R, R, P>::IndCalcB),  8101097520,  8817262080)
    TB (kqqpkp, false, false, true,  (T42<Q, Q, P, P>::IndCalcW), (T42<Q, Q, P, P>::IndCalcB),  5851888362,  6835422612)
    TB (kqqnkp, false, false, true,  (T42<Q, Q, N, P>::IndCalcW), (T42<Q, Q, N, P>::IndCalcB),  7476276864,  8817262080)
    TB (kqqbkp, false, false, true,  (T42<Q, Q, B, P>::IndCalcW), (T42<Q, Q, B, P>::IndCalcB),  7744392000,  8817262080)
    TB (kqqrkp, false, false, true,  (T42<Q, Q, R, P>::IndCalcW), (T42<Q, Q, R, P>::IndCalcB),  7680886080,  8817262080)
    TB (kqqqkp, false, false, false, (T42<Q, Q, Q, P>::IndCalcW), (T42<Q, Q, Q, P>::IndCalcB),  2343300048,  2939087360)
    TB (kpppkn, false, false, false, (T42<P, P, P, N>::IndCalcW), (T42<P, P, P, N>::IndCalcB),  1537640536,  1777129408)
    TB (knppkn, false, false, true,  (T42<N, P, P, N>::IndCalcW), (T42<N, P, P, N>::IndCalcB),  6071020409,  6838084896)
    TB (knnpkn, false, false, true,  (T42<N, N, P, N>::IndCalcW), (T42<N, N, P, N>::IndCalcB),  7677994559,  8729470080)
    TB (kbppkn, false, false, true,  (T42<B, P, P, N>::IndCalcW), (T42<B, P, P, N>::IndCalcB),  6289527204,  6838084896)
    TB (kbnpkn, false, false, true,  (T42<B, N, P, N>::IndCalcW), (T42<B, N, P, N>::IndCalcB), 16186823401, 17458940160)
    TB (kbbpkn, false, false, true,  (T42<B, B, P, N>::IndCalcW), (T42<B, B, P, N>::IndCalcB),  8243187360,  8729470080)
    TB (krppkn, false, false, true,  (T42<R, P, P, N>::IndCalcW), (T42<R, P, P, N>::IndCalcB),  6239761412,  6838084896)
    TB (krnpkn, false, false, true,  (T42<R, N, P, N>::IndCalcW), (T42<R, N, P, N>::IndCalcB), 16057066825, 17458940160)
    TB (krbpkn, false, false, true,  (T42<R, B, P, N>::IndCalcW), (T42<R, B, P, N>::IndCalcB), 16637490240, 17458940160)
    TB (krrpkn, false, false, true,  (T42<R, R, P, N>::IndCalcW), (T42<R, R, P, N>::IndCalcB),  8112305064,  8729470080)
    TB (kqppkn, false, false, true,  (T42<Q, P, P, N>::IndCalcW), (T42<Q, P, P, N>::IndCalcB),  5920486098,  6838084896)
    TB (kqnpkn, false, false, true,  (T42<Q, N, P, N>::IndCalcW), (T42<Q, N, P, N>::IndCalcB), 15239383701, 17458940160)
    TB (kqbpkn, false, false, true,  (T42<Q, B, P, N>::IndCalcW), (T42<Q, B, P, N>::IndCalcB), 15787021288, 17458940160)
    TB (kqrpkn, false, false, true,  (T42<Q, R, P, N>::IndCalcW), (T42<Q, R, P, N>::IndCalcB), 15660230819, 17458940160)
    TB (kqqpkn, false, false, true,  (T42<Q, Q, P, N>::IndCalcW), (T42<Q, Q, P, N>::IndCalcB),  7297961576,  8729470080)
    TB (kpppkb, false, false, false, (T42<P, P, P, B>::IndCalcW), (T42<P, P, P, B>::IndCalcB),  1537640536,  1842024000)
    TB (knppkb, false, false, true,  (T42<N, P, P, B>::IndCalcW), (T42<N, P, P, B>::IndCalcB),  6071020409,  7087788000)
    TB (knnpkb, false, false, true,  (T42<N, N, P, B>::IndCalcW), (T42<N, N, P, B>::IndCalcB),  7677994559,  9048240000)
    TB (kbppkb, false, false, true,  (T42<B, P, P, B>::IndCalcW), (T42<B, P, P, B>::IndCalcB),  6289527204,  7087788000)
    TB (kbnpkb, false, false, true,  (T42<B, N, P, B>::IndCalcW), (T42<B, N, P, B>::IndCalcB), 16186823401, 18096480000)
    TB (kbbpkb, false, false, true,  (T42<B, B, P, B>::IndCalcW), (T42<B, B, P, B>::IndCalcB),  8243187360,  9048240000)
    TB (krppkb, false, false, true,  (T42<R, P, P, B>::IndCalcW), (T42<R, P, P, B>::IndCalcB),  6239761412,  7087788000)
    TB (krnpkb, false, false, true,  (T42<R, N, P, B>::IndCalcW), (T42<R, N, P, B>::IndCalcB), 16057066825, 18096480000)
    TB (krbpkb, false, false, true,  (T42<R, B, P, B>::IndCalcW), (T42<R, B, P, B>::IndCalcB), 16637490240, 18096480000)
    TB (krrpkb, false, false, true,  (T42<R, R, P, B>::IndCalcW), (T42<R, R, P, B>::IndCalcB),  8112305064,  9048240000)
    TB (kqppkb, false, false, true,  (T42<Q, P, P, B>::IndCalcW), (T42<Q, P, P, B>::IndCalcB),  5920486098,  7087788000)
    TB (kqnpkb, false, false, true,  (T42<Q, N, P, B>::IndCalcW), (T42<Q, N, P, B>::IndCalcB), 15239383701, 18096480000)
    TB (kqbpkb, false, false, true,  (T42<Q, B, P, B>::IndCalcW), (T42<Q, B, P, B>::IndCalcB), 15787021288, 18096480000)
    TB (kqrpkb, false, false, true,  (T42<Q, R, P, B>::IndCalcW), (T42<Q, R, P, B>::IndCalcB), 15660230819, 18096480000)
    TB (kqqpkb, false, false, true,  (T42<Q, Q, P, B>::IndCalcW), (T42<Q, Q, P, B>::IndCalcB),  7297961576,  9048240000)
    TB (kpppkr, false, false, false, (T42<P, P, P, R>::IndCalcW), (T42<P, P, P, R>::IndCalcB),  1537640536,  1827875872)
    TB (knppkr, false, false, true,  (T42<N, P, P, R>::IndCalcW), (T42<N, P, P, R>::IndCalcB),  6071020409,  7033481568)
    TB (knnpkr, false, false, true,  (T42<N, N, P, R>::IndCalcW), (T42<N, N, P, R>::IndCalcB),  7677994559,  8978912640)
    TB (kbppkr, false, true,  true,  (T42<B, P, P, R>::IndCalcW), (T42<B, P, P, R>::IndCalcB),  6289527204,  7033481568)
    TB (kbnpkr, false, true,  true,  (T42<B, N, P, R>::IndCalcW), (T42<B, N, P, R>::IndCalcB), 16186823401, 17957825280)
    TB (kbbpkr, false, true,  true,  (T42<B, B, P, R>::IndCalcW), (T42<B, B, P, R>::IndCalcB),  8243187360,  8978912640)
    TB (krppkr, false, false, true,  (T42<R, P, P, R>::IndCalcW), (T42<R, P, P, R>::IndCalcB),  6239761412,  7033481568)
    TB (krnpkr, false, false, true,  (T42<R, N, P, R>::IndCalcW), (T42<R, N, P, R>::IndCalcB), 16057066825, 17957825280)
    TB (krbpkr, false, false, true,  (T42<R, B, P, R>::IndCalcW), (T42<R, B, P, R>::IndCalcB), 16637490240, 17957825280)
    TB (krrpkr, false, false, true,  (T42<R, R, P, R>::IndCalcW), (T42<R, R, P, R>::IndCalcB),  8112305064,  8978912640)
    TB (kqppkr, false, false, true,  (T42<Q, P, P, R>::IndCalcW), (T42<Q, P, P, R>::IndCalcB),  5920486098,  7033481568)
    TB (kqnpkr, false, false, true,  (T42<Q, N, P, R>::IndCalcW), (T42<Q, N, P, R>::IndCalcB), 15239383701, 17957825280)
    TB (kqbpkr, false, false, true,  (T42<Q, B, P, R>::IndCalcW), (T42<Q, B, P, R>::IndCalcB), 15787021288, 17957825280)
    TB (kqrpkr, false, false, true,  (T42<Q, R, P, R>::IndCalcW), (T42<Q, R, P, R>::IndCalcB), 15660230819, 17957825280)
    TB (kqqpkr, false, false, true,  (T42<Q, Q, P, R>::IndCalcW), (T42<Q, Q, P, R>::IndCalcB),  7297961576,  8978912640)
    TB (kpppkq, false, true,  false, (T42<P, P, P, Q>::IndCalcW), (T42<P, P, P, Q>::IndCalcB),  1537640536,  1733232160)
    TB (knppkq, false, true,  true,  (T42<N, P, P, Q>::IndCalcW), (T42<N, P, P, Q>::IndCalcB),  6071020409,  6669309024)
    TB (knnpkq, false, false, true,  (T42<N, N, P, Q>::IndCalcW), (T42<N, N, P, Q>::IndCalcB),  7677994559,  8514011520)
    TB (kbppkq, false, true,  true,  (T42<B, P, P, Q>::IndCalcW), (T42<B, P, P, Q>::IndCalcB),  6289527204,  6669309024)
    TB (kbnpkq, false, true,  true,  (T42<B, N, P, Q>::IndCalcW), (T42<B, N, P, Q>::IndCalcB), 16186823401, 17028023040)
    TB (kbbpkq, false, true,  true,  (T42<B, B, P, Q>::IndCalcW), (T42<B, B, P, Q>::IndCalcB),  8243187360,  8514011520)
    TB (krppkq, false, true,  true,  (T42<R, P, P, Q>::IndCalcW), (T42<R, P, P, Q>::IndCalcB),  6239761412,  6669309024)
    TB (krnpkq, false, true,  true,  (T42<R, N, P, Q>::IndCalcW), (T42<R, N, P, Q>::IndCalcB), 16057066825, 17028023040)
    TB (krbpkq, false, true,  true,  (T42<R, B, P, Q>::IndCalcW), (T42<R, B, P, Q>::IndCalcB), 16637490240, 17028023040)
    TB (krrpkq, false, true,  true,  (T42<R, R, P, Q>::IndCalcW), (T42<R, R, P, Q>::IndCalcB),  8112305064,  8514011520)
    TB (kqppkq, false, true,  true,  (T42<Q, P, P, Q>::IndCalcW), (T42<Q, P, P, Q>::IndCalcB),  5920486098,  6669309024)
    TB (kqnpkq, false, true,  true,  (T42<Q, N, P, Q>::IndCalcW), (T42<Q, N, P, Q>::IndCalcB), 15239383701, 17028023040)
    TB (kqbpkq, false, true,  true,  (T42<Q, B, P, Q>::IndCalcW), (T42<Q, B, P, Q>::IndCalcB), 15787021288, 17028023040)
    TB (kqrpkq, false, true,  true,  (T42<Q, R, P, Q>::IndCalcW), (T42<Q, R, P, Q>::IndCalcB), 15660230819, 17028023040)
    TB (kqqpkq, false, false, true,  (T42<Q, Q, P, Q>::IndCalcW), (T42<Q, Q, P, Q>::IndCalcB),  7297961576,  8514011520)

#endif
#endif
    };

#undef  P
#undef  N
#undef  B
#undef  R
#undef  Q

//  Helper structure
//  Used to classify on-board position

union CUTbReference     // Hungarian: utbr
    {
    int             m_iDesc;            // Negative if have to inverse
    int             m_cPieces;
    CUTbReference   *m_utbReference;
    };

//  Root of the search tree

static CUTbReference rgutbReference [MAX_NON_KINGS + 2];

// Convert TB name (e.g. KQKR) into set of counters

static const char *PchSetHalfCounters
    (
    int         *piCounters,
    const char  *pch
    )
    {
    memset (piCounters, 0, 5 * sizeof (int));
    while ('\0' != *pch && 'k' != *pch)
        {
        piece pi;

        pi = x_piecePawn;   // To make compiler happy
        switch (*pch)
            {
        case 'p':
            pi = x_piecePawn;
            break;
        case 'n':
            pi = x_pieceKnight;
            break;
        case 'b':
            pi = x_pieceBishop;
            break;
        case 'r':
            pi = x_pieceRook;
            break;
        case 'q':
            pi = x_pieceQueen;
            break;
        default:
            assert (0);
            }
        piCounters [pi-1] ++;
        pch ++;
        }
        return pch;
    };

static void VSetCounters
    (
    int         *piCounters,
    const char  *pch
    )
    {
    assert ('k' == *pch);
    pch = PchSetHalfCounters (piCounters, pch+1);
    assert ('k' == *pch);
    pch = PchSetHalfCounters (piCounters+5, pch+1);
    assert ('\0' == *pch);
    }

//  Following functions return TB index
//  They differ by input arguments

extern "C" int IDescFindFromCounters
    (
    int *piCount
    )
    {
    CUTbReference *putbr = rgutbReference;

    if (piCount[0] > putbr->m_cPieces)
        goto not_found;
    putbr = putbr[1 + piCount[0]].m_utbReference;
    if (piCount[1] > putbr->m_cPieces)
        goto not_found;
    putbr = putbr[1 + piCount[1]].m_utbReference;
    if (piCount[2] > putbr->m_cPieces)
        goto not_found;
    putbr = putbr[1 + piCount[2]].m_utbReference;
    if (piCount[3] > putbr->m_cPieces)
        goto not_found;
    putbr = putbr[1 + piCount[3]].m_utbReference;
    if (piCount[4] > putbr->m_cPieces)
        goto not_found;
    putbr = putbr[1 + piCount[4]].m_utbReference;
    if (piCount[5] > putbr->m_cPieces)
        goto not_found;
    putbr = putbr[1 + piCount[5]].m_utbReference;
    if (piCount[6] > putbr->m_cPieces)
        goto not_found;
    putbr = putbr[1 + piCount[6]].m_utbReference;
    if (piCount[7] > putbr->m_cPieces)
        goto not_found;
    putbr = putbr[1 + piCount[7]].m_utbReference;
    if (piCount[8] > putbr->m_cPieces)
        goto not_found;
    putbr = putbr[1 + piCount[8]].m_utbReference;
    if (piCount[9] <= putbr->m_cPieces)
        return putbr[1 + piCount[9]].m_iDesc;
not_found:
    return 0;
    }

int IDescFind
    (
    square  *p_piW, // IN | Pointer to array of white pieces (king excluded)
    square  *p_piB, // IN | Pointer to array of black pieces (king excluded)
    int     cWhite, // IN | Counter of white pieces (king excluded)
    int     cBlack  // IN | Counter of black pieces (king excluded)
    )
    {
    int rgiCount[10];

    // Set pieces counters
    rgiCount[0] =
    rgiCount[1] =
    rgiCount[2] =
    rgiCount[3] =
    rgiCount[4] =
    rgiCount[5] =
    rgiCount[6] =
    rgiCount[7] =
    rgiCount[8] =
    rgiCount[9] = 0;
    while (cWhite)
        {
        rgiCount[(*p_piW)-1] ++;
        p_piW ++;
        cWhite --;
        }
    while (cBlack)
        {
        rgiCount[5-1+(*p_piB)] ++;
        p_piB ++;
        cBlack --;
        }
    return IDescFindFromCounters (rgiCount);
    }

int IDescFindByName
    (
    char    *pchName
    )
    {
    int rgiCount[10];

    VSetCounters (rgiCount, pchName);
    return IDescFindFromCounters (rgiCount);
    }

//-----------------------------------------------------------------------------
//
//  Function used during initialization

//  Set of functions to create search table

static CUTbReference *PutbrCreateSubtable
    (
    int cPieces,    //  IN | # of pieces ramaining on board
    int iDepth      //  IN | Recursion depth (# of piece classes left)
    )
    {
    CUTbReference *putbr;

    putbr = (CUTbReference *) PvMalloc ((cPieces + 2) * sizeof (CUTbReference));
    putbr[0].m_cPieces = cPieces;
    if (0 == iDepth)
        {
        for (int i = 0; i <= cPieces; i ++)
            putbr[i+1].m_iDesc = 0;
        }
    else
        {
        for (int i = 0; i <= cPieces; i ++)
            putbr[i+1].m_utbReference = PutbrCreateSubtable (cPieces-i, iDepth-1);
        }
    return putbr;
    }

static bool fTbTableCreated = false;

static void VCreateEmptyTbTable (void)
    {
    if (fTbTableCreated)
        return;
    fTbTableCreated = true;
    rgutbReference[0].m_cPieces = MAX_NON_KINGS;
    for (int i = 0; i <= MAX_NON_KINGS; i ++)
        rgutbReference[i+1].m_utbReference = PutbrCreateSubtable (MAX_NON_KINGS - i, 8);
    }

// Insert TB (e.g. KQKR) into search table

static bool FRegisterHalf
    (
    int     iTb,
    int     *piCount
    )
    {
    CUTbReference   *putbr;

    putbr = rgutbReference;
    for (int i = 0; i < 9; i ++)
        {
        if (piCount[i] > putbr->m_cPieces)
            return false;
        putbr = putbr[1 + piCount[i]].m_utbReference;
        }
    if (piCount[9] > putbr->m_cPieces)
        return false;
    putbr[1 + piCount[9]].m_iDesc = iTb;
    return true;
    }

// Insert TB (both, e.g. KQKR and KRKQ) into search table

static bool FRegisterTb
    (
    CTbDesc *ptbDesc
    )
    {
    int     rgiCount[10];
    bool    fInserted;

    VSetCounters (rgiCount, ptbDesc->m_rgchName);
    fInserted = FRegisterHalf (ptbDesc->m_iTbId, rgiCount);
    if (fInserted)
        {
        if (ptbDesc->m_fSymmetric)
            return true;
        for (int i = 0; i < 5; i ++)
            {
            int iTemp;

            iTemp = rgiCount[i];
            rgiCount[i] = rgiCount[i+5];
            rgiCount[i+5] = iTemp;
            }
        fInserted = FRegisterHalf (-ptbDesc->m_iTbId, rgiCount);
        assert (fInserted);
        }
    return fInserted;
    }

// File mapping - Win32 code only

#if defined (_WIN32) || defined(_WIN64)

static BYTE * PbMapFileForRead
    (
    char    *szName,
    HANDLE  *phFile,
    HANDLE  *phFileMapping
    )
    {
    HANDLE  hFile;
    HANDLE  hFileMapping;
    LPVOID  lpFileBase;

    hFile = CreateFile (szName, GENERIC_READ, FILE_SHARE_READ,
                        NULL, OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS, NULL);
    if (INVALID_HANDLE_VALUE == hFile)
        {
        printf("*** Couldn't open file %s with CreateFile()\n", szName);
        exit (1);
        }
    hFileMapping = CreateFileMapping (hFile, NULL, PAGE_READONLY, 0, 0, NULL);
    if (0 == hFileMapping)
        {
        CloseHandle (hFile);
        printf ("*** Couldn't open file %s mapping with CreateFileMapping()\n", szName);
        exit (1);
        }
    lpFileBase = MapViewOfFile (hFileMapping, FILE_MAP_READ, 0, 0, 0);
    if (0 == lpFileBase)
        {
        CloseHandle (hFileMapping);
        CloseHandle (hFile);
        printf ("*** Couldn't map view of file %s with MapViewOfFile()\n", szName);
        exit (1);
        }
    if (NULL != phFile)
        *phFile = hFile;
    if (NULL != phFileMapping)
        *phFileMapping = hFileMapping;
    return (BYTE*) lpFileBase;
    }

static void VUnmapFile
    (
    BYTE    *pbFileBase,
    HANDLE  hFile,
    HANDLE  hFileMapping
    )
    {
    BOOL fFailed;

    fFailed = (0 == UnmapViewOfFile (pbFileBase)) |
              (0 == CloseHandle (hFileMapping)) |
              (0 == CloseHandle (hFile));
    if (fFailed) 
        {
        printf ("*** Couldn't unmap file\n");
        exit (1);
        }
    }

#endif

//-----------------------------------------------------------------------------
//
//  TB caching

#if !defined (TB_CB_CACHE_CHUNK)
#define TB_CB_CACHE_CHUNK           8192 /* Must be power of 2 */
#define LOG2_TB_CB_CACHE_CHUNK      13
#endif

#define TB_CHUNK(index)             ((index) >> LOG2_TB_CB_CACHE_CHUNK)
#define TB_OFFSET(index)            ((index) % TB_CB_CACHE_CHUNK)
#define TB_DIRECTORY_ENTRY(chunk)   ((chunk) % TB_DIRECTORY_SIZE)

#define WIDE_TB_CHUNK(index)        ((index) >> (LOG2_TB_CB_CACHE_CHUNK-1))
#define WIDE_TB_OFFSET(index)       ((index) % (TB_CB_CACHE_CHUNK/2))*2

struct CTbCache         //Hungarian: tbc
    {
    int volatile                m_iTb;
    color volatile              m_color;
    unsigned volatile           m_indChunk;
    volatile CTbCache *volatile m_ptbcNext;     // Next element in double-linked general LRU list
    volatile CTbCache *volatile m_ptbcPrev;     // Previous element in double-linked general LRU list
    volatile CTbCache *volatile m_ptbcTbNext;   // Next element in double-linked cache bucket LRU list
    volatile CTbCache *volatile m_ptbcTbPrev;   // Previous element in double-linked cache bucket LRU list
    BYTE                        *m_pbData;
    };

static CTbCache *ptbcTbCache;   // Cache memory
static ULONG    ctbcTbCache;    // Cache size (in entries)

static volatile CTbCache * volatile ptbcHead;       // Head of that list
static volatile CTbCache * volatile ptbcTail;       // Last element in that list
static volatile CTbCache * volatile ptbcFree;       // First free cache header

static INLINE void VTbCloseFile
    (
    int     iTb,
    color   side
    )
    {
    for (int iExtent = 0; iExtent < MAX_EXTENTS; iExtent ++) {
        if (NULL != rgtbdDesc[iTb].m_rgfpFiles[side][iExtent])
            {
            Lock (rgtbdDesc[iTb].m_rglockFiles[side]);
            if (NULL != rgtbdDesc[iTb].m_rgfpFiles[side][iExtent])
                {
                fclose (rgtbdDesc[iTb].m_rgfpFiles[side][iExtent]);
                rgtbdDesc[iTb].m_rgfpFiles[side][iExtent] = NULL;
                }
            Unlock (rgtbdDesc[iTb].m_rglockFiles[side]);
            }
        }
    }

extern "C" void VTbCloseFiles (void)
    {
    // Initialized?
    if (0 == ctbcTbCache)
        return;

    // Walk through TB cache and close all opened files
    for (int iTb = 1; iTb < cTb; iTb ++)
        {
        VTbCloseFile (iTb, x_colorWhite);
        VTbCloseFile (iTb, x_colorBlack);
        }
    }

void VTbClearCache (void)
    {
    CTbCacheBucket  *prgtbcbBuckets;
    CTbCache *ptbc;
    BYTE *pb;
    ULONG i;

    // Initialized?
    if (0 == ctbcTbCache)
        return;
    VTbCloseFiles();
    
    // Initialize all lists
    pb = (BYTE *) & ptbcTbCache [ctbcTbCache];
    for (i = 0, ptbc = ptbcTbCache; i < ctbcTbCache; i ++, ptbc ++)
        {
        ptbc->m_pbData = pb + i*(TB_CB_CACHE_CHUNK+32+4);
        ptbc->m_ptbcTbPrev =
        ptbc->m_ptbcTbNext =
        ptbc->m_ptbcPrev = NULL;
        ptbc->m_ptbcNext = (ptbc + 1);
        }
    ptbc[-1].m_ptbcNext = NULL;

    // Clear references from TBs
    for (int iTb = 1; iTb < cTb; iTb ++)
        {
        for (color sd=x_colorWhite; sd <= x_colorBlack; sd ++)
            {
            prgtbcbBuckets = rgtbdDesc[iTb].m_prgtbcbBuckets[sd];
            if (NULL != prgtbcbBuckets)
                {
#if (CPUS > 1)
                for (i = 0; i < TB_DIRECTORY_SIZE; i ++)
                    LockFree (prgtbcbBuckets[i].m_lock);
#endif
                memset (prgtbcbBuckets, 0, TB_DIRECTORY_SIZE * sizeof (CTbCacheBucket));
#if (CPUS > 1)
                for (i = 0; i < TB_DIRECTORY_SIZE; i ++)
                    LockInit (prgtbcbBuckets[i].m_lock);
#endif
                }
            }
        }

    // Set globals
    ptbcHead = ptbcTail = NULL;
    ptbcFree = ptbcTbCache;
    }

extern "C" int FTbSetCacheSize
    (
    void    *pv,
    ULONG   cbSize
    )
    {
    VTbCloseFiles();
    ctbcTbCache = 0;
    ptbcHead = NULL;
    if (cbSize < sizeof (CTbCache))
        return false;
    ptbcTbCache = (CTbCache*) pv;
    ctbcTbCache = cbSize / (sizeof (CTbCache) + TB_CB_CACHE_CHUNK+32+4);
    VTbClearCache();
    return true;
    }

// Table registered

INLINE int FRegisteredExtent
    (
    int     iTb,
    color   side,
    int     iExtent
    )
    {
    if (rgtbdDesc[iTb].m_fSymmetric)
        side = x_colorWhite;
    return (NULL != rgtbdDesc[iTb].m_rgpchFileName[side][iExtent]);
    }

INLINE int FRegistered
    (
    int     iTb,
    color   side
    )
    {
    int iExtent, cExtents;
    INDEX cBytes;

    if (rgtbdDesc[iTb].m_fSplit)
        {
        cBytes = rgtbdDesc[iTb].m_rgcbLength[side];
        if (rgtbdDesc[iTb].m_f16bit)
            cBytes *= 2;
        cExtents = (int) (cBytes >> 31) + 1;
        }
    else
        cExtents = 1;
    for (iExtent = 0; iExtent < cExtents; iExtent ++)
        {
        if (FRegisteredExtent (iTb, side, iExtent))
            return true;
        }
    return false;
    }

extern "C" int FRegisteredFun
    (
    int     iTb,
    color   side
    )
    {
    return FRegistered (iTb, side);
    }

// Return function that calculates the necessary index:

#define PfnIndCalc(iTb, side)   (rgtbdDesc[iTb].m_rgpfnCalcIndex[side])
extern "C" PfnCalcIndex PfnIndCalcFun
    (
    int     iTb,
    color   side
    )
    {
    return PfnIndCalc (iTb, side);
    }

// Read whole file into memory

extern "C" int FReadTableToMemory
    (
    int     iTb,    // IN | Tablebase
    color   side,   // IN | Side to move
    BYTE    *pb     // IN | Either buffer or NULL
    )
    {
    char    *pszName;
    INDEX   cb;
    FILE    *fp;

    if (rgtbdDesc[iTb].m_fSymmetric)
        side = x_colorWhite;
    if (!FRegistered (iTb, side))
        return false;
    if (rgtbdDesc[iTb].m_fSplit)
        return false;
    for (int iExtent = 0; iExtent < MAX_EXTENTS; iExtent ++)
        {
        if (NULL != rgtbdDesc[iTb].m_rgpdiDecodeInfo[side][iExtent])
            return false;
        }
    if (NULL != rgtbdDesc[iTb].m_rgpbRead[side])
        return true;
    pszName = rgtbdDesc[iTb].m_rgpchFileName[side][0];
    fp = fopen (pszName, "rb");
    if (NULL == fp)
        return false;

    // Find database size
#if defined (NEW)
    cb = rgtbdDesc[iTb].m_rgcbLength[side];
    if (0 == cb)
        {
#endif
        if (0 != fseek (fp, 0L, SEEK_END))
            {
            printf ("*** Seek in %s failed\n", pszName);
            exit (1);
            }
        cb = ftell (fp);
        if (-1 == (int) cb)
            {
            printf ("*** Cannot find length of %s\n", pszName);
            exit (1);
            }
        if (0 != fseek (fp, 0L, SEEK_SET))
            {
            printf ("*** Seek in %s failed\n", pszName);
            exit (1);
            }
#if defined (NEW)
        }
#if defined (T33_INCLUDE) || defined (KPPKP_16BIT)
    else if (rgtbdDesc[iTb].m_f16bit)
        {
        if ((size_t) cb != cb)   // Overflow
            {
            printf ("*** %s too big to read into memory\n", pszName);
            exit (1);
            }
        }
#endif
#endif
    
    // If buffer not specified, allocate memory for it
    if (NULL == pb)
        pb = (BYTE*) PvMalloc (cb);

    // Read file into memory
    if (cb != (INDEX) fread (pb, 1, cb, fp))
        {
        printf ("*** Read from %s failed\n", pszName);
        exit (1);
        }
    fclose (fp);

    // All done
    rgtbdDesc[iTb].m_rgpbRead[side] = pb;
    return true;
    }

#if defined (_WIN32) || defined(_WIN64)

// Map whole file into memory

extern "C" int FMapTableToMemory
    (
    int     iTb,    // IN | Tablebase
    color   side    // IN | Side to move
    )
    {
    char *pszName;

    if (rgtbdDesc[iTb].m_fSymmetric)
        side = x_colorWhite;
    if (!FRegistered (iTb, side))
        return false;
    if (rgtbdDesc[iTb].m_fSplit)
        return false;
    for (int iExtent = 0; iExtent < MAX_EXTENTS; iExtent ++)
        {
        if (NULL != rgtbdDesc[iTb].m_rgpdiDecodeInfo[side][iExtent])
            return false;
        }
    pszName = rgtbdDesc[iTb].m_rgpchFileName[side][0];
    if (NULL == rgtbdDesc[iTb].m_rgpbRead[side])
        {
        rgtbdDesc[iTb].m_rgpbRead[side] = PbMapFileForRead (pszName, NULL, NULL);
        if (fVerbose)
            printf ("%s mapped\n", pszName);
        }
    return true;
    }

// Map whole file into memory

int FMapTableToMemory
    (
    int     iTb,            // IN  | Tablebase
    color   side,           // IN  | Side to move
    HANDLE  *phFile,        // OUT | File handle will be written here
    HANDLE  *phFileMapping  // OUT | File mapping handle will be written here
    )
    {
    char *pszName;

    if (rgtbdDesc[iTb].m_fSymmetric)
        side = x_colorWhite;
    if (!FRegistered (iTb, side))
        return false;
    if (rgtbdDesc[iTb].m_fSplit)
        return false;
    pszName = rgtbdDesc[iTb].m_rgpchFileName[side][0];
    if (NULL == rgtbdDesc[iTb].m_rgpbRead[side])
        {
        rgtbdDesc[iTb].m_rgpbRead[side] = PbMapFileForRead (pszName, phFile, phFileMapping);
        if (fVerbose)
            printf ("%s mapped\n", pszName);
        }
    return true;
    }

// Unmap whole file from memory

int FUnMapTableFromMemory
    (
    int     iTb,            // IN | Tablebase
    color   side,           // IN | Side to move
    HANDLE  hFile,          // IN | File handle will be written here
    HANDLE  hFileMapping    // IN | File mapping handle will be written here
    )
    {
    char *pszName;

    if (rgtbdDesc[iTb].m_fSymmetric)
        side = x_colorWhite;
    if (!FRegistered (iTb, side))
        return false;
    if (rgtbdDesc[iTb].m_fSplit)
        return false;
    pszName = rgtbdDesc[iTb].m_rgpchFileName[side][0];
    if (NULL != rgtbdDesc[iTb].m_rgpbRead[side])
        {
        VUnmapFile (rgtbdDesc[iTb].m_rgpbRead[side], hFile, hFileMapping);
        rgtbdDesc[iTb].m_rgpbRead[side] = NULL;
        if (fVerbose)
            printf ("%s unmapped\n", pszName);
        }
    return true;
    }

#endif

// Probe TB - lower level (not exportable) function

static int TB_FASTCALL TbtProbeTable
    (
    int      iTb,
    color    side,
    unsigned indChunk,
    unsigned indInChunk
    )
    {
    CTbDesc *ptbd;
    int iDirectory, iExtent, iPhysicalChunk;
    volatile CTbCache * ptbc;
    volatile CTbCache * ptbcTbFirst;
    const char *pszFileName;

    ptbd = & rgtbdDesc[iTb];
    iDirectory = TB_DIRECTORY_ENTRY (indChunk);

    // Head of the cache bucket LRU list
    Lock (ptbd->m_prgtbcbBuckets[side][iDirectory].m_lock);
    ptbcTbFirst = ptbd->m_prgtbcbBuckets[side][iDirectory].m_ptbcFirst;

    // First, search entry in the cache
    for (ptbc = ptbcTbFirst; NULL != ptbc; ptbc = ptbc->m_ptbcTbNext)
        {
        if (indChunk == ptbc->m_indChunk)
            {
            // Found - move cache entry to the head of the general LRU list
            Lock (lockLRU);
            if (ptbc != ptbcHead)
                {
                // Remove it from its current position
                ptbc->m_ptbcPrev->m_ptbcNext = ptbc->m_ptbcNext;
                if (NULL == ptbc->m_ptbcNext)
                    ptbcTail = ptbc->m_ptbcPrev;
                else
                    ptbc->m_ptbcNext->m_ptbcPrev = ptbc->m_ptbcPrev;
                // Insert it at the head
                ptbc->m_ptbcPrev = NULL;
                ptbc->m_ptbcNext = ptbcHead;
                ptbcHead->m_ptbcPrev = ptbc;
                ptbcHead = ptbc;
                }
            Unlock (lockLRU);
            // Move cache entry to the head of the cache bucket LRU list
            if (ptbc != ptbcTbFirst)
                {
                // Remove it from list
                ptbc->m_ptbcTbPrev->m_ptbcTbNext = ptbc->m_ptbcTbNext;
                if (NULL != ptbc->m_ptbcTbNext)
                    ptbc->m_ptbcTbNext->m_ptbcTbPrev = ptbc->m_ptbcTbPrev;
                // Insert it at head
                ptbc->m_ptbcTbPrev = NULL;
                ptbc->m_ptbcTbNext = ptbcTbFirst;
                ptbcTbFirst->m_ptbcTbPrev = ptbc;
                ptbd->m_prgtbcbBuckets[side][iDirectory].m_ptbcFirst = ptbc;
                }
            int tb;

            tb = (tb_t) (ptbc->m_pbData[(ULONG)indInChunk]);
            Unlock (ptbd->m_prgtbcbBuckets[side][iDirectory].m_lock);
            return tb;
            }
        }
    // Not in the cache - have to read it from disk.
    // I decided to write simple code - so sometimes it's possible that
    // 2 threads will simultaneously read exactly the same chunk into 2
    // different cache entries. In that case, all subsequent cache probes
    // will hit the first cache entry, so the second one will 'drift' to
    // the end of general LRU list and will be reused.

    // Unlock cache bucket, so other threads can continue execution
    Unlock (ptbd->m_prgtbcbBuckets[side][iDirectory].m_lock);
    // First, find cache entry we can use
    Lock (lockLRU);
    // Get it either from a free list, or reuse last element of the LRU list
    if (NULL != ptbcFree)
        {
        ptbc = ptbcFree;
        ptbcFree = ptbc->m_ptbcNext;
        Unlock (lockLRU);
        }
    else
        {
        unsigned iTailDirectory;
        int      iTailTb;
        color    colorTail;

        assert (NULL != ptbcTail);
#if (CPUS > 1)
        // "Optimistic" model - assuming that there is low content
        // (not hundreds of threads)
        for (;;)
            {
            ptbc = ptbcTail;
            iTailTb = ptbc->m_iTb;
            iTailDirectory = TB_DIRECTORY_ENTRY (ptbc->m_indChunk);
            colorTail = ptbc->m_color;
            // To avoid deadlocks, have to first acquire cache buckets lock,
            // and only then general LRU lock. So, free general LRU lock and
            // acquire 2 locks in a proper order.
            Unlock (lockLRU);
            Lock (rgtbdDesc[iTailTb].m_prgtbcbBuckets[colorTail][iTailDirectory].m_lock);
            Lock (lockLRU);
            // Have structures been modified while we re-acquired locks? 
            // (to be more precise, it's Ok, if structures were modified,
            // but cache entry again become the last element of the list,
            // and TB, color, and cache bucket did not changed, so we locked
            // proper locks).
            if (ptbc == ptbcTail && ptbc->m_iTb == iTailTb &&
                ptbc->m_color == colorTail &&
                TB_DIRECTORY_ENTRY (ptbc->m_indChunk) == iTailDirectory)
                break;
            // Sorry - try once again...
            Unlock (rgtbdDesc[iTailTb].m_prgtbcbBuckets[colorTail][iTailDirectory].m_lock);
            }
#else
        ptbc = ptbcTail;
        iTailTb = ptbc->m_iTb;
        iTailDirectory = TB_DIRECTORY_ENTRY (ptbc->m_indChunk);
        colorTail = ptbc->m_color;
#endif

        // Remove cache entry from the general LRU list
        ptbcTail = ptbc->m_ptbcPrev;
        if (NULL == ptbcTail)
            ptbcHead = NULL;
        else
            ptbcTail->m_ptbcNext = NULL;
        Unlock (lockLRU);
        
        // Remove it from cache bucket list
        if (NULL != ptbc->m_ptbcTbNext)
            ptbc->m_ptbcTbNext->m_ptbcTbPrev = ptbc->m_ptbcTbPrev;
        if (NULL == ptbc->m_ptbcTbPrev)
            rgtbdDesc[iTailTb].m_prgtbcbBuckets[colorTail][iTailDirectory].m_ptbcFirst = ptbc->m_ptbcTbNext;
        else
            ptbc->m_ptbcTbPrev->m_ptbcTbNext = ptbc->m_ptbcTbNext;
        Unlock (rgtbdDesc[iTailTb].m_prgtbcbBuckets[colorTail][iTailDirectory].m_lock);
        }

    // Ok, now we have "orphan" cache entry - it's excluded from all lists,
    // so other threads will never touch it.
    ptbc->m_iTb = iTb;
    ptbc->m_color = side;
    ptbc->m_indChunk = indChunk;

    // Now read it from the disk
    FILE    *fp;
    size_t  cb;

    // First, check: is necessary file opened?
    // As files are not thread-safe, lock file
    Lock (ptbd->m_rglockFiles[side]);
    if (ptbd->m_fSplit)
        {
        iExtent = indChunk >> (31 - LOG2_TB_CB_CACHE_CHUNK);
        iPhysicalChunk = indChunk - (iExtent << (31 - LOG2_TB_CB_CACHE_CHUNK));
        }
    else
        {
        iExtent = 0;
        iPhysicalChunk = indChunk;
        }
    fp = ptbd->m_rgfpFiles[side][iExtent];
    if (NULL == fp)
        {
        // Not - try to open it
        pszFileName = ptbd->m_rgpchFileName[side][iExtent];
        if (NULL != pszFileName)
            {
            fp = fopen (pszFileName, "rb");
            if (NULL == fp)
                {
                // Failed. Close all the opened files and retry
                Unlock (ptbd->m_rglockFiles[side]);
                VTbCloseFiles ();
                Lock (ptbd->m_rglockFiles[side]);
                // Theoretically, it's possible that other threads opened a lot of
                // files in the interval between VTbCloseFiles() and Lock(). If
                // so, we'll fail - I don't like to have one more global lock
                // especially for file open, at least not in first version.
                // Problem can happen only on systems with small limit of
                // simultaneously open files and high number of threads - unlikely
                // combination.
                fp = ptbd->m_rgfpFiles[side][iExtent];
                if (NULL == fp)
                    {
                    fp = fopen (pszFileName, "rb");
                    if (NULL == fp)
                        {
#if defined (STOP_ON_ERROR)
                        printf ("*** Unable to open file %s\n", pszFileName);
                        fflush (stdout);
                        exit(1);
#endif
                        goto ERROR_LABEL;
                        }
                    }
                }
            ptbd->m_rgfpFiles[side][iExtent] = fp;
            }
        else
            goto ERROR_LABEL;
        }

    // File opened. Now seek and read necessary chunk
    if (NULL == ptbd->m_rgpdiDecodeInfo[side][iExtent])
        {
        long lPos;
        int  iResult;

        lPos = (long) (iPhysicalChunk*TB_CB_CACHE_CHUNK);
#if defined (T33_INCLUDE) || defined (T42_INCLUDE)
        if (lPos < 0)
            {
            iResult = fseek (fp, 0L, SEEK_SET);
            if (iResult)
                {
#if defined (STOP_ON_ERROR)
                printf ("*** Unable to seek file %s offset %08X\n",
                        pszFileName, 0);
                fflush (stdout);
                exit(1);
#endif
                goto ERROR_LABEL;
                }
            do
                {
                iResult = fseek (fp, 0x40000000, SEEK_CUR);
                if (iResult)
                    {
#if defined (STOP_ON_ERROR)
                    printf ("*** Unable to seek file %s offset %08X\n",
                            pszFileName, 0x40000000);
                    fflush (stdout);
                    exit(1);
#endif
                    goto ERROR_LABEL;
                    }
                lPos -= 0x40000000;
                } while (lPos < 0);
            iResult = fseek (fp, lPos, SEEK_CUR);
            }
        else
#endif
            iResult = fseek (fp, lPos, SEEK_SET);
        // Read uncompressed file
        if (iResult)
            {
#if defined (STOP_ON_ERROR)
            printf ("*** Unable to seek file %s offset %08X\n",
                    pszFileName, lPos);
            fflush (stdout);
            exit(1);
#endif
            goto ERROR_LABEL;
            }
        cb = fread (ptbc->m_pbData, 1, TB_CB_CACHE_CHUNK, fp);
        if (cb != TB_CB_CACHE_CHUNK)
            {
            // Could not read TB_CB_CACHE_CHUNK - check for error
            if (ferror (fp) || ((size_t) -1 == cb))
                {
#if defined (STOP_ON_ERROR)
                printf ("*** Read error, file %s\n", pszFileName);
                fflush (stdout);
                exit(1);
#endif
                goto ERROR_LABEL;
                }
            }
        Unlock (ptbd->m_rglockFiles[side]);
        }
    else
        {
        // Read compressed file
        int fWasError;
        decode_block    *block;
        decode_info     *info = ptbd->m_rgpdiDecodeInfo[side][iExtent];

#if (CPUS > 1)
        // Find free decode block
        decode_block    **pBlock;

        Lock (lockDecode);
        pBlock = rgpdbDecodeBlocks;
        while (NULL == *pBlock)
            pBlock ++;
        block = *pBlock;
        *pBlock = NULL;
        Unlock (lockDecode);
#else
        block = rgpdbDecodeBlocks[0];
#endif

        // Initialize decode block and read chunk
        fWasError = 0 != comp_init_block (block, TB_CB_CACHE_CHUNK, ptbc->m_pbData) ||
                    0 != comp_read_block (block, info, fp, iPhysicalChunk);
        
        // Release lock on file, so other threads can proceed with that file
        Unlock (ptbd->m_rglockFiles[side]);

        // Decompress chunk
        if (!fWasError)
            fWasError |= (0 != comp_decode_and_check_crc (block, info, block->orig.size, TB_CRC_CHECK));

        // Release block
#if (CPUS > 1)
        Lock (lockDecode);
        *pBlock = block;
        Unlock (lockDecode);
#endif

        // Read Ok?
        if (fWasError)
            {
#if defined (STOP_ON_ERROR)
            printf ("*** Decompression error, file %s\n", pszFileName);
            fflush (stdout);
            exit(1);
#endif
            goto ERROR_LABEL_2;
            }
        }

    // Read - now acquire locks and insert cache entry in both lists
    Lock (ptbd->m_prgtbcbBuckets[side][iDirectory].m_lock);
    Lock (lockLRU);

    // Insert cache entry into general LRU list
    ptbc->m_ptbcPrev = NULL;
    ptbc->m_ptbcNext = ptbcHead;
    if (NULL == ptbcHead)
        ptbcTail = ptbc;
    else
        ptbcHead->m_ptbcPrev = ptbc;
    ptbcHead = ptbc;

    // Insert cache entry into cache bucket LRU list
    ptbc->m_ptbcTbPrev = NULL;
    ptbc->m_ptbcTbNext = ptbd->m_prgtbcbBuckets[side][iDirectory].m_ptbcFirst;
    if (NULL != ptbc->m_ptbcTbNext)
        ptbc->m_ptbcTbNext->m_ptbcTbPrev = ptbc;
    ptbd->m_prgtbcbBuckets[side][iDirectory].m_ptbcFirst = ptbc;

    // All done
    int tb;

    tb = (tb_t) (ptbc->m_pbData[indInChunk]);
    // Release locks
    Unlock (ptbd->m_prgtbcbBuckets[side][iDirectory].m_lock);
    Unlock (lockLRU);
    return tb;

    // I/O error. Here I don't want to halt the program, because that can
    // happen in the middle of the important game. Just return failure.
ERROR_LABEL:
    Unlock (ptbd->m_rglockFiles[side]);
ERROR_LABEL_2:
    Lock (lockLRU);
    ptbd->m_rgpchFileName[side][iExtent] = NULL;
    ptbc->m_ptbcNext = ptbcFree;
    ptbcFree = ptbc;
    Unlock (lockLRU);
    return L_bev_broken;
    }

// Probe TB - upper level function

static int  TB_FASTCALL TbtProbeTable
    (
    int     iTb,
    color   side,
    INDEX   indOffset
    )
    {
    CTbDesc *ptbd;

    assert (iTb > 0 && iTb < cTb);
    ptbd = & rgtbdDesc[iTb];
    if (ptbd->m_fSymmetric)
        side = x_colorWhite;

    // It's better for offset be smaller than TB size
    assert (!FRegistered (iTb, side) || indOffset < ptbd->m_rgcbLength[side]);

    // Entire file read/mapped to memory?
    if (NULL != ptbd->m_rgpbRead[side])
        return (tb_t) ptbd->m_rgpbRead[side][indOffset];

    // Cache initialized? TB registered?
    if (0 == ctbcTbCache || NULL == ptbd->m_prgtbcbBuckets[side])
        return bev_broken;

#if defined (T33_INCLUDE) || defined (KPPKP_16BIT)
    if (ptbd->m_f16bit)
        return bev_broken;
#endif

    int tb;

    tb = TbtProbeTable (iTb, side,(unsigned) TB_CHUNK (indOffset), (unsigned) TB_OFFSET (indOffset));
    return (L_bev_broken == tb) ? bev_broken : (tb_t) tb;
    }

// 16-bit version (recommended)

#define FOutOfBound(iTb, side, indOffset)\
        (tbid_kppkp == iTb && x_colorBlack == side &&\
         (indOffset == 0x0362BC7C || indOffset == 0x0362DE44 || indOffset == 0x03637648 ||\
          indOffset == 0x03639810 || indOffset == 0x038D4F29 || indOffset == 0x040A2CAB ||\
          indOffset == 0x043C778C))

extern "C" int TB_FASTCALL L_TbtProbeTable
    (
    int     iTb,
    color   side,
    INDEX   indOffset
    )
    {
    int tbtScore;
    CTbDesc *ptbd;

    assert (iTb > 0 && iTb < cTb);
    ptbd = & rgtbdDesc[iTb];
    if (ptbd->m_fSymmetric)
        side = x_colorWhite;

    // Entire file read/mapped to memory?
    if (NULL != ptbd->m_rgpbRead[side])
        {
#if defined (KPPKP_16BIT)
        if (!ptbd->m_f16bit)
            {
            tbtScore = (tb_t) ptbd->m_rgpbRead[side][indOffset];
            return S_to_L (tbtScore);
            }
        else
            return (((int) (((signed char) ptbd->m_rgpbRead[side][indOffset*2+1]))) << 8) +
                    ptbd->m_rgpbRead[side][indOffset*2];
#elif defined (T33_INCLUDE)
        if (!ptbd->m_f16bit)
            {
            if (FOutOfBound (iTb, side, indOffset))
                return -32639;
            else
                {
                tbtScore = (tb_t) ptbd->m_rgpbRead[side][indOffset];
                return S_to_L (tbtScore);
                }
            }
        else
            return (((int) (((signed char) ptbd->m_rgpbRead[side][indOffset*2+1]))) << 8) +
                    ptbd->m_rgpbRead[side][indOffset*2];
#else
        if (FOutOfBound (iTb, side, indOffset))
            return -32639;
        else
            {
            tbtScore = (tb_t) ptbd->m_rgpbRead[side][indOffset];
            return S_to_L (tbtScore);
            }
#endif
        }

    // Cache initialized? TB registered?
    if (0 == ctbcTbCache || NULL == ptbd->m_prgtbcbBuckets[side])
        return L_bev_broken;

#if defined (T33_INCLUDE) || defined (T42_INCLUDE)
    if (ptbd->m_f16bit)
        {
        // Inefficient, but very simple, code
        int iLo;
        int iHi;
        
        iLo = TbtProbeTable (iTb, side, (unsigned) WIDE_TB_CHUNK (indOffset), (unsigned) WIDE_TB_OFFSET (indOffset));
        iHi = TbtProbeTable (iTb, side, (unsigned) WIDE_TB_CHUNK (indOffset), (unsigned) WIDE_TB_OFFSET (indOffset)+1);
        tbtScore = (L_bev_broken == iLo || L_bev_broken == iHi) ? L_bev_broken : ((iHi << 8) + (iLo & 0xFF));
        }
    else
        {
#if !defined (KPPKP_16BIT)
        if (FOutOfBound (iTb, side, indOffset))
            return -32639;
#endif
        tbtScore = TbtProbeTable (iTb, side, (unsigned) TB_CHUNK (indOffset), (unsigned) TB_OFFSET (indOffset));
        tbtScore = L_bev_broken == tbtScore ? L_bev_broken : S_to_L (tbtScore);
        }
#elif !defined (KPPKP_16BIT)
    if (FOutOfBound (iTb, side, indOffset))
        return -32639;
    tbtScore = TbtProbeTable (iTb, side, (unsigned) TB_CHUNK (indOffset), (unsigned) TB_OFFSET (indOffset));
    tbtScore = L_bev_broken == tbtScore ? L_bev_broken : S_to_L (tbtScore);
#else
    if (tbid_kppkp != iTb)
        {
        // All tables but kppkp are 8-bit tables
        tbtScore = TbtProbeTable (iTb, side, (unsigned) TB_CHUNK (indOffset), (unsigned) TB_OFFSET (indOffset));
        tbtScore = L_bev_broken == tbtScore ? L_bev_broken : S_to_L (tbtScore);
        }
    else
        {
        // Special handling of kppkp - it's 16-bit table
        // Inefficient, but very simple, code
        int iLo;
        int iHi;
        
        iLo = TbtProbeTable (iTb, side, (unsigned) WIDE_TB_CHUNK (indOffset), (unsigned) WIDE_TB_OFFSET (indOffset));
        iHi = TbtProbeTable (iTb, side, (unsigned) WIDE_TB_CHUNK (indOffset), (unsigned) WIDE_TB_OFFSET (indOffset)+1);
        tbtScore = (L_bev_broken == iLo || L_bev_broken == iHi) ? L_bev_broken : ((iHi << 8) + (iLo & 0xFF));
        }
#endif
    return tbtScore;
    }

//-----------------------------------------------------------------------------
//
//  Global initialization

// TODO: Check size of split tables, too

static void VCheckSize
    (
    int     iTb,
    color   side,
    int     iExtent,
    INDEX   cb,
    char    *rgchTbName
    )
    {
#if defined (NEW)
    INDEX   cbOk1, cbOk2;

    if (0 == rgtbdDesc[iTb].m_rgcbLength[side])
        return;

    cbOk1 = rgtbdDesc[iTb].m_rgcbLength[side];
    cbOk2 = cbOk1;
#if defined (T_INDEX64)
    if (rgtbdDesc[iTb].m_fSplit)
        {
        cbOk1 = (1u << (rgtbdDesc[iTb].m_f16bit ? 30 : 31));
        cbOk2 = cbOk2 % (1u << (rgtbdDesc[iTb].m_f16bit ? 30 : 31));
        }
#endif

    if (cb != cbOk1 && cb != cbOk2)
        {
        printf ("*** %s corrupted "
                DEC_INDEX_FORMAT" "DEC_INDEX_FORMAT" "DEC_INDEX_FORMAT"\n",
                rgchTbName, cbOk1, cbOk2, cb);
        exit (1);
        }
#endif
    }


#if defined (_WIN32) || defined (_WIN64)
int     iDrivesMap;
BOOL    rgfAccessed[26];
BOOL    rgfNotReady[26];
#endif

static int FCheckExtentExistance
    (
    char    *pszPath,
    int     iTb,
    color   side,
    int     iExtent
    )
    {
    FILE            *fp;
    char            *pchCopy;
    const char      *pchExt = PchExt (side);
    char            rgchTbName[1024];
    char            rgchExtent[4];
    CTbCacheBucket  *prgtbcbBuckets;
    INDEX           cb;
    decode_info     *comp_info = NULL;
    int             fWasError;
    decode_block    *block;
    BYTE            rgbBuffer[TB_CB_CACHE_CHUNK+32+4];

    if (FRegisteredExtent (iTb, side, iExtent) || NULL != rgtbdDesc[iTb].m_rgpbRead[side])
        return true;

#if defined (_WIN32) || defined (_WIN64)
    // Do not repeatedely probe device that is not ready
    // (i.e. unmapped disk or CD-ROM that does not contain the disk).
    if (NULL != pszPath && 0 != pszPath[0] && ':' == pszPath[1])
        {
        int  iDrive;
        char szRoot[5];
        WIN32_FIND_DATA fd;
        HANDLE hResult;

        iDrive = tolower(pszPath[0]) - 'a';
        if (iDrive >= 0 && iDrive < 26)
            {
            if (rgfNotReady[iDrive])
                return false;
            if (!rgfAccessed[iDrive])
                {
                if (iDrivesMap && 0 == (iDrivesMap & (1 << iDrive)))
                    return false;
                szRoot[0] = pszPath[0];
                szRoot[1] = pszPath[1];
                szRoot[2] = '\\';
                szRoot[3] = '*';
                szRoot[4] = 0;
                hResult = FindFirstFile (szRoot, &fd);
                if (INVALID_HANDLE_VALUE == hResult)
                    {
                    if (ERROR_NOT_READY == GetLastError())
                        {
                        rgfNotReady[iDrive] = true;
                        return false;
                        }
                    }
                else
                    FindClose (hResult);
                rgfAccessed[iDrive] = true;
                }
            }
        }
#endif

    strcpy (rgchTbName, pszPath);
    if (0 != pszPath[0] && DELIMITER[0] != pszPath[strlen(pszPath)-1])
        strcat (rgchTbName, DELIMITER);
    strcat (rgchTbName, rgtbdDesc[iTb].m_rgchName);
    if (rgtbdDesc[iTb].m_fSplit)
        {
        rgchExtent[0] = '.';
        if (iExtent >= 10)
            rgchExtent[1] = (char) (iExtent + 'a' - 10);
        else
            rgchExtent[1] = (char) (iExtent + '0');
        rgchExtent[2] = '\0';
        strcat (rgchTbName, rgchExtent);
        }
    strcat (rgchTbName, pchExt);
    cOpenFilesAttempts ++;
    fp = fopen (rgchTbName, "rb");
    //printf (">>> Opening %s\n", rgchTbName);
    cOpenFilesSuceed += (NULL != fp);
#if !defined (NEW) && !defined (_WIN32) && !defined(_WIN64)
    // For case-sensitive systems, have to try once more
    if (NULL == fp)
        {
        for (int i = strchr(rgchTbName,'.')-rgchTbName-1; i>=0 && isalpha(rgchTbName[i]); i--)
            rgchTbName[i] = toupper (rgchTbName[i]);
        cOpenFilesAttempts ++;
        fp = fopen (rgchTbName, "rb");
        //printf (">>> Opening %s\n", rgchTbName);
        cOpenFilesSuceed += (NULL != fp);
        }
#endif
    if (NULL != fp)
        {
        // Found uncompressed table
        if (rgtbdDesc[iTb].m_f16bit ||
            ((int)(cb = rgtbdDesc[iTb].m_rgcbLength[side])) < 0 ||
            cb != (unsigned) cb)
            {
            // Do not check the length for too large files
            cb = rgtbdDesc[iTb].m_rgcbLength[side];
            }
        else
            {
            if (0 != fseek (fp, 0L, SEEK_END))
                {
                printf ("*** Seek in %s failed\n", rgchTbName);
                exit (1);
                }
            cb = (INDEX) ftell (fp);
            VCheckSize (iTb, side, iExtent, cb, rgchTbName);
            }
        }
    else
        {
        // Check for compressed table.
        // First, check for kxykz.nb?.emd
        strcat (rgchTbName, ".emd");
        cOpenFilesAttempts ++;
        fp = fopen (rgchTbName, "rb");
        //printf (">>> Opening %s\n", rgchTbName);
        cOpenFilesSuceed += (NULL != fp);
        if ((NULL == fp) && !rgtbdDesc[iTb].m_fSplit)
            {
            // Check for kxykz_nb?.emd
            rgchTbName [strlen(rgchTbName)-8] = '_';
            cOpenFilesAttempts ++;
            fp = fopen (rgchTbName, "rb");
            //printf (">>> Opening %s\n", rgchTbName);
            cOpenFilesSuceed += (NULL != fp);
            }
        if ((NULL == fp) && !rgtbdDesc[iTb].m_fSplit)
            {
            // Check for kxykz_nb?_emd
            rgchTbName [strlen(rgchTbName)-4] = '_';
            cOpenFilesAttempts ++;
            fp = fopen (rgchTbName, "rb");
            //printf (">>> Opening %s\n", rgchTbName);
            cOpenFilesSuceed += (NULL != fp);
            }
#if defined (T41_INCLUDE)
        if ((NULL == fp) && (iTb <= tbid_kqqqk))
#else
        if ((NULL == fp) && (iTb <= tbid_kqqkq))
#endif
            {
            // Check for kxykznb?.emd (8+3 format)
            int cch;
            
            rgchTbName [strlen(rgchTbName)-4] = '.';
            cch = strlen (rgchTbName);
            memmove (rgchTbName+cch-8, rgchTbName+cch-7, 8);
            cOpenFilesAttempts ++;
            fp = fopen (rgchTbName, "rb");
            //printf (">>> Opening %s\n", rgchTbName);
            cOpenFilesSuceed += (NULL != fp);
            }
        if (NULL == fp)
            return false;
        cCompressed ++;
        int iResult = comp_open_file (&comp_info, fp, TB_CRC_CHECK);
        if (0 != iResult)
            {
            printf ("*** Unable to read %s - ", rgchTbName);
            switch (iResult & 0xFF)
                {
            case COMP_ERR_READ:
                printf ("read error\n");
                break;
            case COMP_ERR_NOMEM:
                printf ("out of memory\n");
                break;
            case COMP_ERR_BROKEN:
                printf ("file broken\n");
                break;
            default:
                printf ("error %d\n", iResult);
                break;
                }
            exit (1);
            }
        if (comp_info->block_size != TB_CB_CACHE_CHUNK)
            {
            printf ("*** %s: Unsupported block size %d\n", rgchTbName, comp_info->block_size);
            exit (1);
            }
        if (rgtbdDesc[iTb].m_f16bit)
            {
            cb = ((INDEX)comp_info->block_size/2)*(comp_info->n_blk-1) +
                  (INDEX)comp_info->last_block_size/2;
            VCheckSize (iTb, side, iExtent, cb, rgchTbName);
            }
        else
            {
            cb = ((INDEX)comp_info->block_size)*(comp_info->n_blk-1) +
                 (INDEX)comp_info->last_block_size;
            VCheckSize (iTb, side, iExtent, cb, rgchTbName);
            }

#if 0
        block = rgpdbDecodeBlocks[0];
        if (NULL == block)
            {
            int iResult = comp_alloc_block (&rgpdbDecodeBlocks[0], TB_CB_CACHE_CHUNK);
            if (0 != iResult)
                {
                printf ("*** Cannot allocate decode block: error code %d\n", iResult);
                exit (1);
                }
            block = rgpdbDecodeBlocks[0];
            }
        // Initialize decode block and read chunk
        fWasError = 0 != comp_init_block (block, TB_CB_CACHE_CHUNK, rgbBuffer) ||
                    0 != comp_read_block (block, comp_info, fp, TB_CHUNK (cb-1)) ||
                    0 != comp_decode_and_check_crc (block, comp_info, block->orig.size, TB_CRC_CHECK);
        if (fWasError)
            {
            printf ("*** Sanity check on %s failed\n", rgchTbName);
            exit (1);
            }
#endif
        }

    fclose (fp);
    if (FRegisterTb (& (rgtbdDesc[iTb])))
        {
        pchCopy = (char*) PvMalloc (strlen(rgchTbName)+1);
        strcpy (pchCopy, rgchTbName);
        free (rgtbdDesc[iTb].m_rgpchFileName[side][iExtent]);
        rgtbdDesc[iTb].m_rgpchFileName[side][iExtent] = pchCopy;
        if (NULL == rgtbdDesc[iTb].m_prgtbcbBuckets[side])
            {
            prgtbcbBuckets = (CTbCacheBucket*) PvMalloc (TB_DIRECTORY_SIZE*sizeof(CTbCacheBucket));
            memset (prgtbcbBuckets, 0, TB_DIRECTORY_SIZE*sizeof(CTbCacheBucket));
#if (CPUS > 1)
            for (int i = 0; i < TB_DIRECTORY_SIZE; i ++)
                LockInit (prgtbcbBuckets[i].m_lock);
#endif
            rgtbdDesc[iTb].m_prgtbcbBuckets[side] = prgtbcbBuckets;
            if (fVerbose)
                printf ("%s registered\n", pchCopy);
            }
        else
            {
            if (fVerbose)
                printf ("%s found\n", pchCopy);
            }
        rgtbdDesc[iTb].m_rgpdiDecodeInfo[side][iExtent] = comp_info;
        return true;
        }
    else
        {
        printf ("*** Unable to register %s\n", rgchTbName);
        exit (1);
        }
    return false;
    }

int FCheckExistance
    (
    char    *pszPath,
    int     iTb,
    color   side
    )
    {
    int  fFound;
    int  cExtents;
    INDEX cBytes;

    if (rgtbdDesc[iTb].m_fSplit)
        {
        cBytes = rgtbdDesc[iTb].m_rgcbLength[side];
        if (rgtbdDesc[iTb].m_f16bit)
            cBytes *= 2;
        cExtents = (int) (cBytes >> 31) + 1;
        }
    else
        cExtents = 1;
    fFound = false;
    for (int iExtent = 0; iExtent < cExtents; iExtent++)
        {
        if (!FCheckExtentExistance (pszPath, iTb, side, iExtent))
            break;
        fFound = true;
        }
    return fFound;
    }

extern "C" int IInitializeTb
    (
    char *pszPath
    )
    {
    char    szTemp[1024];
    color   sd;
    int     iTb, iMaxTb, iExtent, i;
    CTbCacheBucket  *prgtbcbBuckets;

#if defined (_WIN32) || defined (_WIN64)
    // For Windows, get bit map of ready devices
    iDrivesMap = GetLogicalDrives();
    memset (rgfAccessed, 0, sizeof(rgfAccessed));
    memset (rgfNotReady, 0, sizeof(rgfNotReady));
#endif

    cOpenFilesAttempts = cOpenFilesSuceed = 0;
    cbAllocated = cbEGTBCompBytes = 0;
    // If there are open files, close those
    VTbCloseFiles ();
#if (CPUS > 1)
    // Init all locks
    LockInit (lockLRU);
    LockInit (lockDecode);
    for (iTb = 1; iTb < cTb; iTb ++)
        {
        LockInit (rgtbdDesc[iTb].m_rglockFiles[x_colorWhite]);
        LockInit (rgtbdDesc[iTb].m_rglockFiles[x_colorBlack]);
        }
#endif
#if defined (NEW)
    // Create enumeration tables
    VInitEnumerations ();
#endif
    // Create empty TB search table
    VCreateEmptyTbTable ();
    // Free memory from TB table
    for (iTb = 1; iTb < cTb; iTb ++)
        {
        for (sd = x_colorWhite; sd <= x_colorBlack; sd = (color) (sd + 1))
            {
            if (NULL != rgtbdDesc[iTb].m_prgtbcbBuckets[sd] &&
                NULL == rgtbdDesc[iTb].m_rgpbRead[sd])
                {
                prgtbcbBuckets = rgtbdDesc[iTb].m_prgtbcbBuckets[sd];
#if (CPUS > 1)
                for (i = 0; i < TB_DIRECTORY_SIZE; i ++)
                    LockFree (prgtbcbBuckets[i].m_lock);
#endif
                free (prgtbcbBuckets);
                rgtbdDesc[iTb].m_prgtbcbBuckets[sd] = NULL;
                }
            for (iExtent = 0; iExtent < MAX_EXTENTS; iExtent ++)
                {
                if (NULL != rgtbdDesc[iTb].m_rgpchFileName[sd][iExtent])
                    {
                    free (rgtbdDesc[iTb].m_rgpchFileName[sd][iExtent]);
                    rgtbdDesc[iTb].m_rgpchFileName[sd][iExtent] = NULL;
                    }
                if (NULL != rgtbdDesc[iTb].m_rgpdiDecodeInfo[sd][iExtent])
                    {
                    free (rgtbdDesc[iTb].m_rgpdiDecodeInfo[sd][iExtent]);
                    rgtbdDesc[iTb].m_rgpdiDecodeInfo[sd][iExtent] = NULL;
                    }
                }
            }
        }
    // Free compressed blocks
    for (i = 0; i < CPUS; i ++)
        {
        if (NULL != rgpdbDecodeBlocks[i])
            {
            free (rgpdbDecodeBlocks[i]);
            rgpdbDecodeBlocks[i] = NULL;
            }
        }

    if(pszPath == NULL)
        return 0;

    // Search for existing TBs
    iMaxTb = 0;
    for (;;)
        {
        for (i = 0; pszPath[i] != '\0' && pszPath[i] != ',' && pszPath[i] != ';'
#if !defined (_WIN32) && !defined (__MWERKS__) && !defined(_WIN64)
             && pszPath[i] != ':'
#endif
             ; i ++)
            {
            szTemp[i] = pszPath[i];
            }
        szTemp[i] = '\0';
        for (iTb = 1; iTb < cTb; iTb ++)
            {
            if (FCheckExistance (szTemp, iTb, x_colorWhite))
                {
                if (iTb > iMaxTb)
                    iMaxTb = iTb;
                }
        if (! rgtbdDesc[iTb].m_fSymmetric && FCheckExistance (szTemp, iTb, x_colorBlack))
                {
                if (iTb > iMaxTb)
                    iMaxTb = iTb;
                }
            }
        pszPath += i;
        if ('\0' == *pszPath)
            break;
        pszPath ++;
        }
    
    // If there were compressed files, have to allocate buffer(s)
    if (0 != cCompressed)
        {
        for (i = 0; i < CPUS; i ++)
            {
            if (NULL == rgpdbDecodeBlocks[i])
                {
                int iResult = comp_alloc_block (&rgpdbDecodeBlocks[i], TB_CB_CACHE_CHUNK);
                if (0 != iResult)
                    {
                    printf ("*** Cannot allocate decode block: error code %d\n", iResult);
                    exit (1);
                    }
                }
            }
        if (fVerbose)
            printf ("Allocated %dKb for decompression tables, indices, and buffers.\n",
                    (cbEGTBCompBytes+1023)/1024);
        }

    if (fVerbose)
        printf ("Tried to open %d files. Opened %d files.\n",
                cOpenFilesAttempts, cOpenFilesSuceed);
    // All done!
#if defined T33_INCLUDE
    if (iMaxTb >= tbid_knnknn)
        return 6;
#elif defined (T42_INCLUDE)
    if (iMaxTb >= tbid_knnnkn)
        return 6;
#endif
    if (iMaxTb >= tbid_kppkp)
        return 5;
    if (iMaxTb >= tbid_kpkp)
        return 4;
    if (iMaxTb >= tbid_kpk)
        return 3;
    return 0;
    }
