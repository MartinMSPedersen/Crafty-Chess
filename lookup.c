#include <stdio.h>
#include <stdlib.h>
#include "types.h"
#include "function.h"
#include "data.h"

/* last modified 08/27/96 */
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
*       2       age  62  search id to identify old trans/ref entried.          *
*       2      type  60  0->value is worthless; 1-> value represents a fail-   *
*                        low bound; 2-> value represents a fail-high bound;    *
*                        3-> value is an exact score.                          *
*      19     value  40  unsigned integer value of this position + 131072.     *
*                        this might be a good score or search bound.           *
*      20    pvalue  21  unsigned integer value the evaluate() procedure       *
*                        produced for this position (0=none)+131072.           *
*      21      move   0  best move from the current position, according to the *
*                        search at the time this position was stored.          *
*                                                                              *
*       8    unused  56  currently the leftmost 8 bits are unused.             *
*       8     draft  48  the depth of the search below this position, which is *
*                        used to see if we can use this entry at the current   *
*                        position.                                             *
*      48       key   0  leftmost 48 bits of the 64 bit hash key.  this is     *
*                        used to "verify" that this entry goes with the        *
*                        current board position.                               *
*                                                                              *
********************************************************************************
*/
int LookUp(int ply, int depth, int wtm, int *value, int alpha, int beta)
{
  register BITBOARD temp_hash_key;
  register HASH_ENTRY *htablea, *htableb;
  register int type, draft, avoid_null=WORTHLESS, val;
/*
 ----------------------------------------------------------
|                                                          |
|   first, compute the initial hash address and choose     |
|   which hash table (based on color) to probe.            |
|                                                          |
 ----------------------------------------------------------
*/
  hash_move[ply]=0;
  temp_hash_key=HashKey;
  if (wtm) {
    htablea=trans_ref_wa+(((int) temp_hash_key) & hash_maska);
    htableb=trans_ref_wb+(((int) temp_hash_key) & hash_maskb);
  }
  else {
    htablea=trans_ref_ba+(((int) temp_hash_key) & hash_maska);
    htableb=trans_ref_bb+(((int) temp_hash_key) & hash_maskb);
  }
  temp_hash_key=temp_hash_key>>16;
/*
 ----------------------------------------------------------
|                                                          |
|   now, check both "parts" of the hash table to see if    |
|   this position is in either one.                        |
|                                                          |
 ----------------------------------------------------------
*/
  if (!Xor(And(htablea->word2,mask_80),temp_hash_key)) {
    hash_move[ply]=((int) htablea->word1) & 07777777;
    type=((int) Shiftr(htablea->word1,60)) & 03;
    draft=((int) Shiftr(htablea->word2,48)) & 0377;
    val=(((int) Shiftr(htablea->word1,41)) & 01777777)-131072;
    if ((type & LOWER_BOUND) && ((depth-NULL_MOVE_DEPTH-1) <= draft) &&
          (val < beta)) avoid_null=AVOID_NULL_MOVE;
    if (depth > draft) return(avoid_null);
    switch (type) {
      case EXACT_SCORE:
        if (abs(val) > MATE-100) {
          if (val > 0) val-=(ply-1);
          else val+=(ply-1);
        }
        *value=val;
#if !defined(FAST)
        transposition_hashes++;
#endif
        return(EXACT_SCORE);
      case LOWER_BOUND:
        if (val <= alpha) {
#if !defined(FAST)
          transposition_hashes++;
#endif
          return(LOWER_BOUND);
        }
        return(avoid_null);
      case UPPER_BOUND:
        if (val >= beta) {
#if !defined(FAST)
          transposition_hashes++;
#endif
          return(UPPER_BOUND);
        }
        return(avoid_null);
    }
    return(avoid_null);
  }
  if (!Xor(And(htableb->word2,mask_80),temp_hash_key)) {
    if (hash_move[ply]==0)
      hash_move[ply]=((int) htableb->word1) & 07777777;
    type=((int) Shiftr(htableb->word1,60)) & 03;
    draft=((int) Shiftr(htableb->word2,48)) & 0377;
    val=(((int) Shiftr(htableb->word1,41)) & 01777777)-131072;
    if ((type & LOWER_BOUND) && ((depth-NULL_MOVE_DEPTH-1) <= draft) &&
          (val < beta)) avoid_null=AVOID_NULL_MOVE;
    if (depth > draft) return(avoid_null);
    switch (type) {
      case EXACT_SCORE:
        if (abs(val) > MATE-100) {
          if (val > 0) val-=(ply-1);
          else val+=(ply-1);
        }
        *value=val;
#if !defined(FAST)
        transposition_hashes++;
#endif
        return(EXACT_SCORE);
      case LOWER_BOUND:
        if (val <= alpha) {
#if !defined(FAST)
          transposition_hashes++;
#endif
          return(LOWER_BOUND);
        }
        return(avoid_null);
      case UPPER_BOUND:
        if (val >= beta) {
#if !defined(FAST)
          transposition_hashes++;
#endif
          return(UPPER_BOUND);
        }
        return(avoid_null);
    }
  }

  return(WORTHLESS);
}
