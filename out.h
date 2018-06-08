chess.h
27d26
< #  include "lock.h"
129a129
> #  include "lock.h"
307a308
>   unsigned int no_limit;
316a318,319
>   unsigned int one_reply_extensions_done;
>   unsigned int mate_extensions_done;
333c336
<   volatile int stop;
---
>   volatile char stop;
343a347
>   int mate_threat;
415,416c419,420
< void CopyFromChild(TREE * RESTRICT, TREE * RESTRICT, int);
< TREE *CopyToChild(TREE * RESTRICT, int);
---
> void CopyFromSMP(TREE * RESTRICT, TREE * RESTRICT, int);
> TREE *CopyToSMP(TREE * RESTRICT, int);
465,466c469,470
< int HashProbe(TREE * RESTRICT, int, int, int, int *, int);
< void HashStore(TREE * RESTRICT, int, int, int, int, int);
---
> int HashProbe(TREE * RESTRICT, int, int, int, int *, int, int *);
> void HashStore(TREE * RESTRICT, int, int, int, int, int, int);
472a477
> void InitializeEvaluation(void);
540c545
< int SearchExtensions(TREE * RESTRICT, int, int, int);
---
> int SearchExtensions(TREE * RESTRICT, int, int, int, int);
542c547
< int SearchParallel(TREE * RESTRICT, int, int, int, int, int, int);
---
> int SearchSMP(TREE * RESTRICT, int, int, int, int, int, int, int);
604c609
< #  define Distance(a,b) Max(FileDistance(a,b), RankDistance(a,b))
---
> #  define Distance(a,b) Max(FileDistance(a,b),RankDistance(a,b))
613,615c618,620
< #  define LimitExtensions(extended,ply)                              \
<       extended=Min(extended,PLY);                                    \
<       if (ply > 2*iteration_depth) {                                 \
---
> #  define LimitExtensions(extended,ply)                                    \
>       extended=Min(extended,PLY);                                            \
>       if (ply > 2*iteration_depth && !tree->no_limit) {              \
619,620c624,625
<         else                                                         \
<           extended=0;                                                \
---
>         else                                                                 \
>           extended=0;                                                        \
data.h
49c49,51
< extern int check_depth;
---
> extern int incheck_depth;
> extern int onerep_depth;
> extern int mate_depth;
97a100
> extern int razor_depth;
164c167
< extern TREE *block[MAX_BLOCKS + 1];
---
> extern TREE *local[MAX_BLOCKS + 1];
172a176
> extern volatile unsigned int splitting;
251c255
< extern int connected_passed_pawn_value[2];
---
> extern int connected_passed_pawn_value[2][2][8];
252a257
> extern int supported_passer[2][2][8];
342c347
< extern BITBOARD mask_pawn_connected[64];
---
> extern BITBOARD mask_pawn_protected[2][64];
epddefs.h
epdglue.h
epd.h
inline32.h
inline64.h
lock.h
tbdecode.h
76c76
<                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     /* --------------------- Constants, types, etc. ----------------------- *//*                       ----------------------                         */
---
>                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   /* --------------------- Constants, types, etc. ----------------------- *//*                       ----------------------                         */
80c80
<                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     /* LOG2 (max size of block to compress) *//* max. integer we can take LOG2 by table       */
---
>                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   /* LOG2 (max size of block to compress) *//* max. integer we can take LOG2 by table       */
vcinline.h
