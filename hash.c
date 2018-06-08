#include "chess.h"
#include "data.h"
/* last modified 01/14/09 */
/*
 *******************************************************************************
 *                                                                             *
 *   HashProbe() is used to retrieve entries from the transposition table so   *
 *   this sub-tree won't have to be searched again if we reach a position that *
 *   has been searched previously.  A transposition table position contains    *
 *   the following data packed into 128 bits with each item taking the number  *
 *   of bits given in the table below:                                         *
 *                                                                             *
 *     off  bits     name description                                          *
 *       0   3       age  search id to identify old trans/ref entries.         *
 *       3   2      type  0->value is worthless; 1-> value represents a        *
 *                        fail-low bound; 2-> value represents a fail-high     *
 *                        bound; 3-> value is an exact score.                  *
 *       5   6    unused  unused at present time.                              *
 *      11  21      move  best move from the current position, according to    *
 *                        the search at the time this position was stored.     *
 *      32  15     draft  the depth of the search below this position, which   *
 *                        is used to see if we can use this entry at the       *
 *                        current position.                                    *
 *      47  17     value  unsigned integer value of this position + 65536.     *
 *                        this might be a good score or search bound.          *
 *      64  64       key  64 bit hash signature, used to verify that this      *
 *                        entry goes with the current board position.          *
 *                                                                             *
 *   The underlying scheme here is that there are essentially two separate     *
 *   transposition/refutation (hash) tables that are interleaved into one      *
 *   large table.  We have a depth-priority table where the draft of an entry  *
 *   is used to decide whether to keep or overwrite that entry when a store    *
 *   is done.  We have a 2x larger always-store table where entries that       *
 *   either (a) don't have enough draft to be stored in the depth-priority     *
 *   table are stored or (b) entries overwritten in the depth-priority table   *
 *   get pushed into the always-store table to avoid losing them completely.   *
 *                                                                             *
 *   Each logical entry is a triplet, one depth-preferred entry and two        *
 *   always-store entries.  To choose which always-store entry to overwrite,   *
 *   we simply use the LSB of the hash signature that was not used in the      *
 *   initial table address computation.                                        *
 *                                                                             *
 *******************************************************************************
 */
int HashProbe(TREE * RESTRICT tree, int ply, int depth, int wtm, int *alpha,
    int beta) {
  register BITBOARD word1, word2;
  register int type, draft, avoid_null = 0, val, probe;
  BITBOARD temp_hashkey;
  HASH_ENTRY *htable;

/*
 ************************************************************
 *                                                          *
 *   We loop thru two entries.  The first entry is the      *
 *   depth-priority entry (the first in a triplet of        *
 *   entries).  The second is one of the two following      *
 *   entries, dictated by the rightmost bit of the hash     *
 *   signature that was not used to address the table       *
 *   triplet.                                               *
 *                                                          *
 *   We also return an "avoid_null" status if neither hash  *
 *   entry has enough draft to terminate the current search *
 *   but one of them does have enough draft to prove that   *
 *   a null-move search would not fail high.  This avoids   *
 *   the null-move search overhead in positions where it is *
 *   simply a waste of time to try it.                      *
 *                                                          *
 ************************************************************
 */
  tree->hash_move[ply] = 0;
  temp_hashkey = (wtm) ? HashKey : ~HashKey;
  htable = trans_ref + 3 * (temp_hashkey & hash_mask);
  for (probe = 0; probe < 2; probe++) {
    word1 = htable->word1;
    word2 = htable->word2;
    word2 ^= word1;
    if (word2 == temp_hashkey) {
      word1 =
          (word1 & 0x1fffffffffffffffULL) | ((BITBOARD) transposition_id <<
          61);
      htable->word1 = word1;
      htable->word2 = word1 ^ word2;
      val = (word1 & 0x1ffff) - 65536;
      draft = (word1 >> 17) & 0x7fff;
      if (!tree->hash_move[ply])
        tree->hash_move[ply] = (word1 >> 32) & 0x1fffff;
      type = (word1 >> 59) & 3;
      if ((type & UPPER) && depth - null_depth - 1 <= draft && val < beta)
        avoid_null = AVOID_NULL_MOVE;
      if (depth <= draft) {
        if (val > MATE - 300)
          val -= ply - 1;
        else if (val < -MATE + 300)
          val += ply - 1;
        switch (type) {
          case EXACT:
            *alpha = val;
            if (draft != MAX_DRAFT)
              return (EXACT);
            else
              return (EXACTEGTB);
          case UPPER:
            if (val <= *alpha)
              return (UPPER);
            break;
          case LOWER:
            if (val >= beta)
              return (LOWER);
            break;
        }
      }
    }
    htable += ((temp_hashkey >> log_hash) & 1) + 1;
  }
  return (avoid_null);
}

/* last modified 01/14/09 */
/*
 *******************************************************************************
 *                                                                             *
 *   HashStore() is used to store entries into the transposition table so that *
 *   this sub-tree won't have to be searched again if the same position is     *
 *   reached.  We basically store three types of entries:                      *
 *                                                                             *
 *     (1) EXACT.  This entry is stored when we complete a search at some ply  *
 *        and end up with a score that is greater than alpha and less than     *
 *        beta, which is an exact score, which also has a best move to try if  *
 *        we encounter this position again.                                    *
 *                                                                             *
 *     (2) LOWER.  This entry is stored when we complete a search at some ply  *
 *        and end up with a score that is greater than or equal to beta.  We   *
 *        know know that this score should be at least equal to beta and may   *
 *        well be even higher.  So this entry represents a lower bound on the  *
 *        score for this node, and we also have a good move to try since it    *
 *        caused the cutoff, although we do not know if it is the best move or *
 *        not since not all moves were search.                                 *
 *                                                                             *
 *     (3) UPPER.  This entry is stored when we complete a search at some ply  *
 *        and end up with a score that is less than or equal to alpha.  We     *
 *        know know that this score should be at least equal to alpha and may  *
 *        well be even lower.  So this entry represents an upper bound on the  *
 *        score for this node.  We have no idea about which move is best in    *
 *        this position since they all failed low, so we store a best move of  *
 *        zero.                                                                *
 *                                                                             *
 *******************************************************************************
 */
void HashStore(TREE * RESTRICT tree, int ply, int depth, int wtm, int type,
    int value, int bestmove) {
  register BITBOARD word1, word2;
  register HASH_ENTRY *htable;
  register int draft, age, hwhich;

/*
 ************************************************************
 *                                                          *
 *   "Fill in the blank" and build a table entry from       *
 *   current search information.                            *
 *                                                          *
 ************************************************************
 */
  word1 = transposition_id;
  word1 = (word1 << 2) | type;
  if (value > MATE - 300)
    value += ply - 1;
  else if (value < -MATE + 300)
    value -= ply - 1;
  word1 = (word1 << 27) | bestmove;
  word1 = (word1 << 15) | depth;
  word1 = (word1 << 17) | (value + 65536);
  word2 = (wtm) ? HashKey : ~HashKey;
/*
 ************************************************************
 *                                                          *
 *   If the draft of this entry is greater than the draft   *
 *   of the entry in the "depth-priority" table, or if the  *
 *   entry in the depth-priority table is from an old       *
 *   search, move that entry to the always-store table and  *
 *   then replace the depth-priority table entry by the new *
 *   hash result.                                           *
 *                                                          *
 ************************************************************
 */
  htable = trans_ref + 3 * (word2 & hash_mask);
  draft = (htable->word1 >> 17) & 0x7fff;
  age = htable->word1 >> 61;
  if (age != transposition_id || (depth >= draft)) {
    if (word2 != (htable->word2 ^ htable->word1)) {
      hwhich = (((htable->word2 ^ htable->word1) >> log_hash) & 1) + 1;
      (htable + hwhich)->word1 = htable->word1;
      (htable + hwhich)->word2 = htable->word2;
    }
    htable->word1 = word1;
    htable->word2 = word2 ^ word1;
  } else {
    hwhich = ((word2 >> log_hash) & 1) + 1;
    (htable + hwhich)->word1 = word1;
    (htable + hwhich)->word2 = word2 ^ word1;
  }
}

/* last modified 01/14/09 */
/*
 *******************************************************************************
 *                                                                             *
 *   HashStorePV() is called by Iterate() to insert the PV moves so they will  *
 *   be searched before any other moves.                                       *
 *                                                                             *
 *******************************************************************************
 */
void HashStorePV(TREE * RESTRICT tree, int ply, int wtm, int bestmove) {
  register int hwhich;
  register HASH_ENTRY *htable;
  register BITBOARD temp_hashkey;

/*
 ************************************************************
 *                                                          *
 *   First, compute the initial hash address.               *
 *                                                          *
 ************************************************************
 */
  temp_hashkey = (wtm) ? HashKey : ~HashKey;
  htable = trans_ref + 3 * (temp_hashkey & hash_mask);
  hwhich = ((temp_hashkey >> log_hash) & 1) + 1;
/*
 ************************************************************
 *                                                          *
 *   Now we check the depth-priority table first.  If the   *
 *   correct hash signature is already there, we simply     *
 *   insert the PV move and leave the other information     *
 *   alone.                                                 *
 *                                                          *
 *   Then we check the proper always-store table entry.  If *
 *   the correct hash signature is already there, we simply *
 *   insert the PV move and leave the other information     *
 *   alone.                                                 *
 *                                                          *
 *   Finally, if the correct signature is not in either     *
 *   table, we store a fake entry in the always-store table *
 *   with the best move, but we set the type to WORTHLESS   *
 *   as it is not a real entry.                             *
 *                                                          *
 ************************************************************
 */
  if ((htable->word2 ^ htable->word1) == temp_hashkey) {
    htable->word1 &= ~((BITBOARD) 0x1fffff << 32);
    htable->word1 |= (BITBOARD) bestmove << 32;
    htable->word2 = temp_hashkey ^ htable->word1;
  } else if ((htable + hwhich)->word2 == temp_hashkey) {
    (htable + hwhich)->word1 &= ~((BITBOARD) 0x1fffff << 32);
    (htable + hwhich)->word1 |= (BITBOARD) bestmove << 32;
    (htable + hwhich)->word2 = temp_hashkey ^ (htable + hwhich)->word1;
  } else {
    htable += hwhich;
    htable->word1 = transposition_id;
    htable->word1 = (htable->word1 << 2) | WORTHLESS;
    htable->word1 = (htable->word1 << 27) | bestmove;
    htable->word1 = (htable->word1 << 32) | 65536;;
    htable->word2 = temp_hashkey ^ htable->word1;
  }
}
