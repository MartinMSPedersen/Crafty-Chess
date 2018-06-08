#include <stdio.h>
#include <stdlib.h>
#include "chess.h"
#include "data.h"

/* last modified 08/20/98 */
/*
 *******************************************************************************
 *                                                                             *
 *   Store_*() is used to store entries into the transposition table so that   *
 *   this sub-tree won't have to be searched again if the same position is     *
 *   reached.  a transposition/refutation table position contains the following*
 *   data packed into 128 bits, with each item taking the number of bits given *
 *   in the table below.                                                       *
 *                                                                             *
 *     bits     name  SL  description                                          *
 *       3       age  61  search id to identify old trans/ref entried.         *
 *       2      type  59  0->value is worthless; 1-> value represents a fail-  *
 *                        low bound; 2-> value represents a fail-high bound;   *
 *                        3-> value is an exact score.                         *
 *       1    threat  58  threat extension flag, 1 -> extend this position.    *
 *      16     value  21  unsigned integer value of this position + 32767.     *
 *                        this might be a good score or search bound.          *
 *      21      move   0  best move from the current position, according to the*
 *                        search at the time this position was stored.         *
 *                                                                             *
 *      16     draft  48  the depth of the search below this position, which is*
 *                        used to see if we can use this entry at the current  *
 *                        position.  note that this is in units of 1/8th of a  *
 *                        ply.                                                 *
 *      48       key   0  leftmost 48 bits of the 64 bit hash key.  this is    *
 *                        used to "verify" that this entry goes with the       *
 *                        current board position.                              *
 *                                                                             *
 *   StoreBest() is called when a ply has been completed, and we are ready to  *
 *   back up a new best move and score to the previous ply.  we can extract the*
 *   best move from the current search path.                                   *
 *                                                                             *
 *******************************************************************************
 */
void StoreBest(TREE * tree, int ply, int depth, int wtm, int value, int alpha,
    int threat)
{
  register HASH_ENTRY *htablea, *htableb;
  register BITBOARD word1, word2;
  register int draft, age;

/*
 ************************************************************
 *                                                          *
 *   "fill in the blank" and build a table entry from       *
 *   current search information.                            *
 *                                                          *
 ************************************************************
 */
  if (value > alpha) {
    if (abs(value) < MATE - 100)
      word1 = Shiftl((BITBOARD) (value + 65536), 21);
    else if (value > 0)
      word1 = Shiftl((BITBOARD) (value + ply - 1 + 65536), 21);
    else
      word1 = Shiftl((BITBOARD) (value - ply + 1 + 65536), 21);
    word1 =
        Or(word1, Shiftl((BITBOARD) ((transposition_id << 2) + EXACT_SCORE),
            59));
    if ((int) tree->pv[ply].path_length >= ply)
      word1 = Or(word1, (BITBOARD) tree->pv[ply].path[ply]);
  } else {
    word1 = Shiftl((BITBOARD) (value + 65536), 21);
    word1 =
        Or(word1, Shiftl((BITBOARD) ((transposition_id << 2) + UPPER_BOUND),
            59));
  }
  if (threat)
    word1 = Or(word1, threat_flag);

  word2 = Or(HashKey >> 16, Shiftl((BITBOARD) depth, 48));
/*
 ************************************************************
 *                                                          *
 *   if the draft of this entry is greater than the draft   *
 *   of the entry in the "depth-priority" table, or if the  *
 *   entry in the depth-priority table is from an old       *
 *   search, move that entry to the always-store table and  *
 *   then replace the depth-priority table entry by the new *
 *   hash result.                                           *
 *                                                          *
 ************************************************************
 */
  if (wtm) {
    htablea = trans_ref_wa + (((int) HashKey) & hash_maska);
    htableb = trans_ref_wb + (((int) HashKey) & hash_maskb);
  } else {
    htablea = trans_ref_ba + (((int) HashKey) & hash_maska);
    htableb = trans_ref_bb + (((int) HashKey) & hash_maskb);
  }
  draft = (int) Shiftr(htablea->word2, 48);
  age = (unsigned int) Shiftr(htablea->word1, 61);
  age = age && (age != transposition_id);
  if (age ** (depth >= draft)) {
#if defined(SMP)
    Lock(lock_hasha);
    Lock(lock_hashb);
#endif
    htableb->word1 = htablea->word1;
    htableb->word2 = htablea->word2;
#if defined(SMP)
    UnLock(lock_hashb);
#endif
    htablea->word1 = word1;
    htablea->word2 = word2;
#if defined(SMP)
    UnLock(lock_hasha);
#endif
  } else {
#if defined(SMP)
    Lock(lock_hashb);
#endif
    htableb->word1 = word1;
    htableb->word2 = word2;
#if defined(SMP)
    UnLock(lock_hashb);
#endif
  }
}

/* last modified 03/11/98 */
/*
 *******************************************************************************
 *                                                                             *
 *   StorePV() is called by Iterate() to insert the PV moves so they will be   *
 *   searched before any other moves.                                          *
 *                                                                             *
 *******************************************************************************
 */
void StorePV(TREE * tree, int ply, int wtm)
{
  register BITBOARD temp_hash_key;
  register HASH_ENTRY *htable;

/*
 ************************************************************
 *                                                          *
 *   first, compute the initial hash address and choose     *
 *   which hash table (based on color) to probe.            *
 *                                                          *
 ************************************************************
 */
  temp_hash_key = HashKey;
  htable =
      ((wtm) ? trans_ref_wb : trans_ref_bb) +
      (((int) temp_hash_key) & hash_maskb);
  temp_hash_key = temp_hash_key >> 16;
/*
 ************************************************************
 *                                                          *
 *   now "fill in the blank" and build a table entry from   *
 *   current search information.                            *
 *                                                          *
 ************************************************************
 */
  htable->word1 = Shiftl((BITBOARD) 65536, 21);
  htable->word1 =
      Or(htable->word1, Shiftl((BITBOARD) ((transposition_id << 2) + WORTHLESS),
          59));
  htable->word1 = Or(htable->word1, (BITBOARD) tree->pv[ply].path[ply]);
  htable->word2 = temp_hash_key;
}

/* last modified 08/20/98 */
/*
 *******************************************************************************
 *                                                                             *
 *   StoreRefutation() is called when a move at the current ply is so good it  *
 *   "refutes" the move made at the previous ply.  we then remember this so it *
 *   can be used to do the same the next time this position is reached.        *
 *                                                                             *
 *******************************************************************************
 */
void StoreRefutation(TREE * tree, int ply, int depth, int wtm, int bound,
    int threat)
{
  register HASH_ENTRY *htablea, *htableb;
  register BITBOARD word1, word2;
  register int draft, age;

/*
 ************************************************************
 *                                                          *
 *   "fill in the blank" and build a table entry from       *
 *   current search information.                            *
 *                                                          *
 ************************************************************
 */
  word1 = Shiftl((BITBOARD) (bound + 65536), 21);
  word1 =
      Or(word1, Shiftl((BITBOARD) ((transposition_id << 2) + LOWER_BOUND), 59));
  word1 = Or(word1, (BITBOARD) tree->current_move[ply]);
  if (threat)
    word1 = Or(word1, threat_flag);

  word2 = Or(HashKey >> 16, Shiftl((BITBOARD) depth, 48));
/*
 ************************************************************
 *                                                          *
 *   if the draft of this entry is greater than the draft   *
 *   of the entry in the "depth-priority" table, or if the  *
 *   entry in the depth-priority table is from an old       *
 *   search, move that entry to the always-store table and  *
 *   then replace the depth-priority table entry by the new *
 *   hash result.                                           *
 *                                                          *
 ************************************************************
 */
  if (wtm) {
    htablea = trans_ref_wa + (((int) HashKey) & hash_maska);
    htableb = trans_ref_wb + (((int) HashKey) & hash_maskb);
  } else {
    htablea = trans_ref_ba + (((int) HashKey) & hash_maska);
    htableb = trans_ref_bb + (((int) HashKey) & hash_maskb);
  }
  draft = (int) Shiftr(htablea->word2, 48);
  age = (unsigned int) Shiftr(htablea->word1, 61);
  age = age && (age != transposition_id);

  if (age ** (depth >= draft)) {
#if defined(SMP)
    Lock(lock_hasha);
    Lock(lock_hashb);
#endif
    htableb->word1 = htablea->word1;
    htableb->word2 = htablea->word2;
#if defined(SMP)
    UnLock(lock_hashb);
#endif
    htablea->word1 = word1;
    htablea->word2 = word2;
#if defined(SMP)
    UnLock(lock_hasha);
#endif
  } else {
#if defined(SMP)
    Lock(lock_hashb);
#endif
    htableb->word1 = word1;
    htableb->word2 = word2;
#if defined(SMP)
    UnLock(lock_hashb);
#endif
  }
}
