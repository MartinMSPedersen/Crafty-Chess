#include <stdio.h>
#include <stdlib.h>
#include "chess.h"
#include "data.h"

/* last modified 01/30/99 */
/*
********************************************************************************
*                                                                              *
*   HashProbe() is used to retrieve entries from the transposition table so    *
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
*       5    unused  53  unused at present time.                               *
*      21      move  32  best move from the current position, according to the *
*                        search at the time this position was stored.          *
*      15     draft  17  the depth of the search below this position, which is *
*                        used to see if we can use this entry at the current   *
*                        position.  note that this is in units of 1/60th of a  *
*                        ply.                                                  *
*      17     value   0  unsigned integer value of this position + 65536.      *
*                        this might be a good score or search bound.           *
*                                                                              *
*      64       key   0  64 bit hash signature, used to verify that this entry *
*                        goes with the current board position.                 *
*                                                                              *
********************************************************************************
*/
int HashProbe(TREE *tree, int ply, int depth, int wtm, int *alpha,
           int *beta, int *threat) {
  register BITBOARD word1, word2;
  register int type, draft, avoid_null=WORTHLESS, val, word1l, word1r;
  BITBOARD temp_hashkey;
  HASH_ENTRY *htable;
/*
 ----------------------------------------------------------
|                                                          |
|   first, compute the initial hash address and choose     |
|   which hash table (based on color) to probe.            |
|                                                          |
 ----------------------------------------------------------
*/
  tree->hash_move[ply]=0;
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
  temp_hashkey=(wtm) ? HashKey : ~HashKey;
  htable=trans_ref_a+((int) temp_hashkey&hash_maska);
  Lock(lock_hasha);
  word1=htable->word1;
  word2=htable->word2;
  UnLock(lock_hasha);
  if (word2 == temp_hashkey) do {
#if !defined(FAST)
    tree->transposition_hits++;
#endif
    word1l=word1>>32;
    word1r=word1;
    val=(word1r & 0377777)-65536;
    draft=word1r>>17;
    tree->hash_move[ply]=word1l & 07777777;
    *threat=(word1l>>26) & 01;
    type=(word1l>>27) & 03;
    if ((type & UPPER) &&
        depth-NULL_MOVE_DEPTH-INCPLY <= draft &&
        val < *beta) avoid_null=AVOID_NULL_MOVE;
    if (depth > draft) break;

    switch (type) {
      case EXACT:
        if (abs(val) > MATE-300) {
          if (val > 0) val-=(ply-1);
          else val+=(ply-1);
        }
        *alpha=val;
        return(EXACT);
      case UPPER:
        if (val <= *alpha) {
          *alpha=val;
          return(UPPER);
        }
        break;
      case LOWER:
        if (val >= *beta) {
          *beta=val;
          return(LOWER);
        }
        break;
    }
  } while(0);

  htable=trans_ref_b+((int)temp_hashkey&hash_maskb);
  Lock(lock_hashb);
  word1=htable->word1;
  word2=htable->word2;
  UnLock(lock_hashb);
  if (word2 == temp_hashkey) {
#if !defined(FAST)
    tree->transposition_hits++;
#endif
    word1l=word1>>32;
    word1r=word1;
    val=(word1r & 0377777)-65536;
    draft=word1r>>17;
    if (tree->hash_move[ply]==0) tree->hash_move[ply]=word1l & 07777777;
    *threat=(word1l>>26) & 01;
    type=(word1l>>27) & 03;
    if ((type & UPPER) &&
        depth-NULL_MOVE_DEPTH-INCPLY <= draft &&
        val < *beta) avoid_null=AVOID_NULL_MOVE;
    if (depth > draft) return(avoid_null);

    switch (type) {
      case EXACT:
        if (abs(val) > MATE-300) {
          if (val > 0) val-=(ply-1);
          else val+=(ply-1);
        }
        *alpha=val;
        return(EXACT);
      case UPPER:
        if (val <= *alpha) {
          *alpha=val;
          return(UPPER);
        }
        return(avoid_null);
      case LOWER:
        if (val >= *beta) {
          *beta=val;
          return(LOWER);
        }
        return(avoid_null);
    }
  }
  return(avoid_null);
}

/* last modified 02/02/99 */
/*
********************************************************************************
*                                                                              *
*   HashStore() is used to store entries into the transposition table so that  *
*   this sub-tree won't have to be searched again if the same position is      *
*   reached.  a transposition/refutation table position contains the following *
*   data packed into 128 bits, with each item taking the number of bits given  *
*   in the table below.                                                        *
*                                                                              *
*     bits     name  SL  description                                           *
*       3       age  61  search id to identify old trans/ref entried.          *
*       2      type  59  0->value is worthless; 1-> value represents a fail-   *
*                        low bound; 2-> value represents a fail-high bound;    *
*                        3-> value is an exact score.                          *
*       1    threat  58  threat extension flag, 1 -> extend this position.     *
*       5    unused  53  unused at present time.                               *
*      21      move  32  best move from the current position, according to the *
*                        search at the time this position was stored.          *
*      15     draft  17  the depth of the search below this position, which is *
*                        used to see if we can use this entry at the current   *
*                        position.  note that this is in units of 1/60th of a  *
*                        ply.                                                  *
*      17     value   0  unsigned integer value of this position + 65536.      *
*                        this might be a good score or search bound.           *
*                                                                              *
*      64       key   0  64 bit hash signature, used to verify that this entry *
*                        goes with the current board position.                 *
*                                                                              *
********************************************************************************
*/
void HashStore(TREE *tree, int ply, int depth, int wtm, int type,
               int value, int threat) {
  register BITBOARD word1, word2;
  register HASH_ENTRY *htablea, *htableb;
  register int draft, age, word1l, word1r;
/*
 ----------------------------------------------------------
|                                                          |
|   "fill in the blank" and build a table entry from       |
|   current search information.                            |
|                                                          |
 ----------------------------------------------------------
*/
  word1l=((((transposition_id<<2)+type)<<1)+threat)<<26;
  if (type == EXACT) {
    if (value > MATE-300) value=value+ply-1;
    else if (value < -MATE+300) value=value-ply+1;
    if ((int) tree->pv[ply].pathl >= ply) 
      word1l|=tree->pv[ply].path[ply];
  }
  else if (type == LOWER) {
    word1l|=tree->current_move[ply];
  }
  word1r=(depth<<17)+value+65536;
  word1=word1r+((BITBOARD)word1l<<32);
  word2=(wtm) ? HashKey : ~HashKey;
/*
 ----------------------------------------------------------
|                                                          |
|   if the draft of this entry is greater than the draft   |
|   of the entry in the "depth-priority" table, or if the  |
|   entry in the depth-priority table is from an old       |
|   search, move that entry to the always-store table and  |
|   then replace the depth-priority table entry by the new |
|   hash result.                                           |
|                                                          |
 ----------------------------------------------------------
*/
  htablea=trans_ref_a+((int) word2&hash_maska);
  draft=(int) Shiftr(htablea->word1,17) & 077777;
  age=(unsigned int) Shiftr(htablea->word1,61);
  age=age && (age!=transposition_id);
  if (age || (depth >= draft)) {
    Lock(lock_hasha);
    if (word2 != htablea->word2) {
      Lock(lock_hashb);
      htableb=trans_ref_b+((int) (htablea->word2)&hash_maskb);
      htableb->word1=htablea->word1;
      htableb->word2=htablea->word2;
      UnLock(lock_hashb);
    }
    htablea->word1=word1;
    htablea->word2=word2;
    UnLock(lock_hasha);
  }
  else {
    htableb=trans_ref_b+((int) word2&hash_maskb);
    Lock(lock_hashb);
    htableb->word1=word1;
    htableb->word2=word2;
    UnLock(lock_hashb);
  }
}

/* last modified 01/30/99 */
/*
********************************************************************************
*                                                                              *
*   HashStorePV() is called by Iterate() to insert the PV moves so they will   *
*   be searched before any other moves.                                        *
*                                                                              *
********************************************************************************
*/
void HashStorePV(TREE *tree, int ply, int wtm) {
  register HASH_ENTRY *htablea, *htableb;
  register BITBOARD temp_hashkey;
/*
 ----------------------------------------------------------
|                                                          |
|   first, compute the initial hash address and choose     |
|   which hash table (based on color) to probe.            |
|                                                          |
 ----------------------------------------------------------
*/
  temp_hashkey=(wtm) ? HashKey : ~HashKey;
  htablea=trans_ref_a+(((int) temp_hashkey)&hash_maska);
  htableb=trans_ref_b+(((int) temp_hashkey)&hash_maskb);
/*
 ----------------------------------------------------------
|                                                          |
|   now "fill in the blank" and build a table entry from   |
|   current search information.                            |
|                                                          |
 ----------------------------------------------------------
*/
  if (htablea->word2 == HashKey) {
    htablea->word1&=~((BITBOARD) 07777777<<32);
    htablea->word1|=Shiftl((BITBOARD) tree->pv[ply].path[ply],32);
  }
  else if (htableb->word2 == HashKey) {
    htableb->word1&=~((BITBOARD) 07777777<<32);
    htableb->word1|=Shiftl((BITBOARD) tree->pv[ply].path[ply],32);
  }
  else {
    htableb->word1=(BITBOARD) 65536;
    htableb->word1|=Shiftl((BITBOARD) ((transposition_id<<2)+WORTHLESS),59);
    htableb->word1|=Shiftl((BITBOARD) tree->pv[ply].path[ply],32);
    htableb->word2=HashKey;
  }
}
