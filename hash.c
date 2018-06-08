#include <stdio.h>
#include <stdlib.h>
#include "chess.h"
#include "data.h"

/* last modified 09/30/98 */
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
int HashProbe(TREE *tree, int ply, int depth, int wtm, int *alpha,
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
  Lock(lock_hasha);
  word1=htable->word1;
  word2=htable->word2;
  UnLock(lock_hasha);
  if (!Xor(And(word2,mask_80),temp_hash_key)) do {
#if !defined(FAST)
    tree->transposition_hits++;
#endif
    tree->hash_move[ply]=((int) word1) & 07777777;
    type=((int) Shiftr(word1,59)) & 03;
    draft=(int) Shiftr(word2,48);
    val=(((int) Shiftr(word1,21)) & 01777777)-65536;
    *threat=((int) Shiftr(word1,58)) & 01;
    if ((type & UPPER) &&
        depth-NULL_MOVE_DEPTH-INCREMENT_PLY <= draft &&
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

  htable=((wtm)?trans_ref_wb:trans_ref_bb)+(((int) HashKey) & hash_maskb);
  Lock(lock_hashb);
  word1=htable->word1;
  word2=htable->word2;
  UnLock(lock_hashb);
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
    if ((type & UPPER) &&
        depth-NULL_MOVE_DEPTH-INCREMENT_PLY <= draft &&
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

/* last modified 12/17/98 */
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
*      16     value  21  unsigned integer value of this position + 32767.      *
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
void HashStore(TREE *tree, int ply, int depth, int wtm, int type,
               int value, int threat) {
  register HASH_ENTRY *htablea, *htableb;
  register BITBOARD word1, word2;
  register int draft, age, temp_addr;
/*
 ----------------------------------------------------------
|                                                          |
|   "fill in the blank" and build a table entry from       |
|   current search information.                            |
|                                                          |
 ----------------------------------------------------------
*/
  if (type == EXACT) {
    if (abs(value) < MATE-300) word1=Shiftl((BITBOARD) (value+65536),21);
    else if (value > 0) word1=Shiftl((BITBOARD) (value+ply-1+65536),21);
    else word1=Shiftl((BITBOARD) (value-ply+1+65536),21);
    word1=Or(word1,Shiftl((BITBOARD) ((transposition_id<<2)+EXACT),59));
    if ((int) tree->pv[ply].path_length >= ply) 
      word1=Or(word1,(BITBOARD) tree->pv[ply].path[ply]);
  }
  else if (type == UPPER) {
    word1=Shiftl((BITBOARD) (value+65536),21);
    word1=Or(word1,Shiftl((BITBOARD) ((transposition_id<<2)+UPPER),59));
  }
  else {
    word1=Shiftl((BITBOARD) (value+65536),21);
    word1=Or(word1,Shiftl((BITBOARD) ((transposition_id<<2)+LOWER),59));
    word1=Or(word1,(BITBOARD) tree->current_move[ply]);
  }
  if (threat) word1=Or(word1,threat_flag);

  word2=Or(HashKey>>16,Shiftl((BITBOARD) depth,48));
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
  htablea=((wtm) ? trans_ref_wa:trans_ref_ba)+(((int) HashKey) & hash_maska);
  draft=(int) Shiftr(htablea->word2,48);
  age=(unsigned int) Shiftr(htablea->word1,61);
  age=age && (age!=transposition_id);
  if (age || (depth >= draft)) {
    Lock(lock_hasha);
    Lock(lock_hashb);
    temp_addr=(((int)htablea->word2)<<16)+(((int) HashKey) & 0177777);
    htableb=((wtm) ? trans_ref_wb:trans_ref_bb)+(temp_addr & hash_maskb);
    htableb->word1=htablea->word1;
    htableb->word2=htablea->word2;
    UnLock(lock_hashb);
    htablea->word1=word1;
    htablea->word2=word2;
    UnLock(lock_hasha);
  }
  else {
    htableb=((wtm) ? trans_ref_wb:trans_ref_bb)+(((int) HashKey) & hash_maskb);
    Lock(lock_hashb);
    htableb->word1=word1;
    htableb->word2=word2;
    UnLock(lock_hashb);
  }
}

/* last modified 03/11/98 */
/*
********************************************************************************
*                                                                              *
*   HashStorePV() is called by Iterate() to insert the PV moves so they will   *
*   be searched before any other moves.                                        *
*                                                                              *
********************************************************************************
*/
void HashStorePV(TREE *tree, int ply, int wtm) {
  register BITBOARD temp_hash_key;
  register HASH_ENTRY *htable;
/*
 ----------------------------------------------------------
|                                                          |
|   first, compute the initial hash address and choose     |
|   which hash table (based on color) to probe.            |
|                                                          |
 ----------------------------------------------------------
*/
  temp_hash_key=HashKey;
  htable=((wtm) ? trans_ref_wb : trans_ref_bb)+(((int) temp_hash_key)&hash_maskb);
  temp_hash_key=temp_hash_key>>16;
/*
 ----------------------------------------------------------
|                                                          |
|   now "fill in the blank" and build a table entry from   |
|   current search information.                            |
|                                                          |
 ----------------------------------------------------------
*/
  htable->word1=Shiftl((BITBOARD) 65536,21);
  htable->word1=Or(htable->word1,Shiftl((BITBOARD) ((transposition_id<<2)+WORTHLESS),59));
  htable->word1=Or(htable->word1,(BITBOARD) tree->pv[ply].path[ply]);
  htable->word2=temp_hash_key;
}

