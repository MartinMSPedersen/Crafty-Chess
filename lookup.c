#include <stdio.h>
#include <stdlib.h>
#include "chess.h"
#include "data.h"

/* last modified 08/20/98 */
/*
********************************************************************************
*                                                                              *
*   LookUp() is used to retrieve entries from the transposition table so that  *
*   this sub-tree won't have to be searched again if we reach a position that  *
*   has been searched previously.  a transposition table position contains the *
*   following data packed into 128 bits with each item taking the number of    *
*   bits given in the table below:                                             *
*                                                                              *
*     bits     name  SL  description                                           *
*       3       age  61  search id to identify old trans/ref entried.          *
*       2      type  59  0->value is worthless; 1-> value represents a fail-   *
*                        low bound; 2-> value represents a fail-high bound;    *
*                        3-> value is an exact score.                          *
*       1    threat  58  threat extension flag, 1 -> extend this position.     *
*      16     value  21  unsigned integer value of this position + 65536.      *
*                        this might be a good score or search bound.           *
*      21      move   0  best move from the current position, according to the *
*                        search at the time this position was stored.          *
*                                                                              *
*      16     draft  48  the depth of the search below this position, which is *
*                        used to see if we can use this entry at the current   *
*                        position.  note that this is in units of 1/8th of a   *
*                        ply.                                                  *
*      48       key   0  leftmost 48 bits of the 64 bit hash key.  this is     *
*                        used to "verify" that this entry goes with the        *
*                        current board position.                               *
*                                                                              *
********************************************************************************
*/
int LookUp(TREE *tree, int ply, int depth, int wtm, int *alpha,
           int *beta, int *threat) {
  register BITBOARD temp_hash_key;
  register BITBOARD word1, word2;
  register HASH_ENTRY *htable;
  register int type, draft, avoid_null=WORTHLESS, val;
/*
 ----------------------------------------------------------
|                                                          |
|   first, compute the initial hash address and choose     |
|   which hash table (based on color) to probe.            |
|                                                          |
 ----------------------------------------------------------
*/
  tree->hash_move[ply]=0;
  temp_hash_key=HashKey>>16;
#if !defined(FAST)
  tree->transposition_probes++;
#endif
/*
 ----------------------------------------------------------
|                                                          |
|   now, check both "parts" of the hash table to see if    |
|   this position is in either one.                        |
|                                                          |
 ----------------------------------------------------------
*/
  htable=((wtm)?trans_ref_wa:trans_ref_ba)+(((int) HashKey) & hash_maska);
# if defined(SMP)
  Lock(lock_hasha);
#endif
  word1=htable->word1;
  word2=htable->word2;
# if defined(SMP)
  UnLock(lock_hasha);
#endif
  if (!Xor(And(word2,mask_80),temp_hash_key)) do {
#if !defined(FAST)
    tree->transposition_hits++;
#endif
    tree->hash_move[ply]=((int) word1) & 07777777;
    type=((int) Shiftr(word1,59)) & 03;
    draft=(int) Shiftr(word2,48);
    val=(((int) Shiftr(word1,21)) & 01777777)-65536;
    *threat=((int) Shiftr(word1,58)) & 01;
    if ((type & UPPER_BOUND) &&
        depth-NULL_MOVE_DEPTH-INCPLY <= draft &&
        val < *beta) avoid_null=AVOID_NULL_MOVE;
    if (depth > draft) break;

    switch (type) {
      case EXACT_SCORE:
        if (abs(val) > MATE-100) {
          if (val > 0) val-=(ply-1);
          else val+=(ply-1);
        }
        *alpha=val;
        return(EXACT_SCORE);
      case UPPER_BOUND:
        if (val <= *alpha) {
          *alpha=val;
          return(UPPER_BOUND);
        }
        break;
      case LOWER_BOUND:
        if (val >= *beta) {
          *beta=val;
          return(LOWER_BOUND);
        }
        break;
    }
  } while(0);

  htable=((wtm)?trans_ref_wb:trans_ref_bb)+(((int) HashKey) & hash_maskb);
# if defined(SMP)
  Lock(lock_hashb);
#endif
  word1=htable->word1;
  word2=htable->word2;
# if defined(SMP)
  UnLock(lock_hashb);
#endif
  if (!Xor(And(word2,mask_80),temp_hash_key)) {
#if !defined(FAST)
    tree->transposition_hits++;
#endif
    if (tree->hash_move[ply]==0)
      tree->hash_move[ply]=((int) word1) & 07777777;
    type=((int) Shiftr(word1,59)) & 03;
    draft=Shiftr(word2,48);
    val=(((int) Shiftr(word1,21)) & 01777777)-65536;
    *threat=((int) Shiftr(word1,58)) & 01;
    if ((type & UPPER_BOUND) &&
        depth-NULL_MOVE_DEPTH-INCPLY <= draft &&
        val < *beta) avoid_null=AVOID_NULL_MOVE;
    if (depth > draft) return(avoid_null);

    switch (type) {
      case EXACT_SCORE:
        if (abs(val) > MATE-100) {
          if (val > 0) val-=(ply-1);
          else val+=(ply-1);
        }
        *alpha=val;
        return(EXACT_SCORE);
      case UPPER_BOUND:
        if (val <= *alpha) {
          *alpha=val;
          return(UPPER_BOUND);
        }
        return(avoid_null);
      case LOWER_BOUND:
        if (val >= *beta) {
          *beta=val;
          return(LOWER_BOUND);
        }
        return(avoid_null);
    }
  }
  return(avoid_null);
}
