#include "chess.h"
#include "data.h"
/* last modified 08/13/09 */
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
 *   The underlying scheme here is that we use a "bucket" of 4 entries.  In    *
 *   HashProbe() we simply compare against each of the four entries for a      *
 *   match.  Each "bucket" is carefully aligned to a 64-byte boundary so that  *
 *   the bucket fits into a single cache line for efficiency.                  *
 *                                                                             *
 *   Crafty uses the lockless hashing approach to avoid lock overhead in the   *
 *   hash table accessing (reading or writing).  What we do is store the key   *
 *   and the information in two successive writes to memory.  But since there  *
 *   is nothing that prevents another CPU from interlacing its writes with     *
 *   ours, we want to make sure that the bound/draft/etc really goes with the  *
 *   key.  Consider thread 1 trying to store A1 and A2 in two successive 64    *
 *   words, while thread 2 is trying to store B1 and B2.  Since the two cpus   *
 *   are fully independent, we could end up with {A1,A2}, {A1,B2}, {B1,A2} or  *
 *   {B1,B2}.  The two cases with one word of entry A and one word of entry B  *
 *   are problematic since the information part does not belong with the       *
 *   signature part, and a hash hit (signature match) will retrieve data that  *
 *   does not match the position.  Let's assume that the first word is the     *
 *   signature (A1 or B1) and the second word is the data (A2 or B2).  What we *
 *   do is store A1^A2 (exclusive-or the two parts) in the 1 (key) slot of the *
 *   entry, and store A2 in the data part.  Now, before we try to compare the  *
 *   signatures, we have to "un-corrupt" the stored signature by again using   *
 *   xor, since A1^A2^A2 gives us the original A1 signature again.  But if we  *
 *   store A1^A2, and the data part gets replaced by B2, then we try to match  *
 *   against A1^A2^B2 and that won't match unless we are lucky and A2 == B2    *
 *   which means the match is OK anyway.  This eliminates the need to lock the *
 *   hash table while storing the two values, which would be a big performance *
 *   hit since hash entries are stored in almost every node of the tree.       *
 *                                                                             *
 *******************************************************************************
 */
int HashProbe(TREE * RESTRICT tree, int ply, int depth, int wtm, int *alpha,
    int beta) {
  register BITBOARD word1, word2;
  register int type, draft, avoid_null = 0, val, entry;
  BITBOARD temp_hashkey;
  HASH_ENTRY *htable;

/*
 ************************************************************
 *                                                          *
 *   All we have to do is loop through four entries to see  *
 *   there is a signature match.  There can only be one     *
 *   instance of any single signature, so the first match   *
 *   is all we need.                                        *
 *                                                          *
 ************************************************************
 */
  tree->hash_move[ply] = 0;
  temp_hashkey = (wtm) ? HashKey : ~HashKey;
  htable = trans_ref + 4 * (temp_hashkey & hash_mask);
  for (entry = 0; entry < 4; entry++) {
    word1 = htable->word1;
    word2 = htable->word2 ^ word1;
    if (word2 == temp_hashkey)
      break;
    htable++;
  }
/*
 ************************************************************
 *                                                          *
 *   If we found a match, we have to verify that the draft  *
 *   is at least equal to the current depth, if not higher, *
 *   and that the bound/score will let us terminate the     *
 *   search early.                                          *
 *                                                          *
 *   We also return an "avoid_null" status if the matched   *
 *   entry does not have enough draft to terminate the      *
 *   current search but does have enough draft to prove     *
 *   that a null-move search would not fail high.  This     *
 *   avoids the null-move search overhead in positions      *
 *   where it is simply a waste of time to try it.          *
 *                                                          *
 ************************************************************
 */
  if (entry < 4) {
    word1 =
        (word1 & 0x1fffffffffffffffULL) | ((BITBOARD) transposition_id << 61);
    htable->word1 = word1;
    htable->word2 = word1 ^ word2;
    val = (word1 & 0x1ffff) - 65536;
    draft = (word1 >> 17) & 0x7fff;
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
    return (avoid_null);
  }
  return (0);
}

/* last modified 09/15/09 */
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
 *   For storing, we may require two passes.  We make our first pass looking   *
 *   for the entry with the lowest draft (depth remaining) and which was from  *
 *   a previous search (old entry).  We choose the lowest draft old entry, if  *
 *   there is one, otherwise we make a second pass over the bucket and choose  *
 *   the entry with the shallowest draft, period.                              *
 *                                                                             *
 *******************************************************************************
 */
void HashStore(TREE * RESTRICT tree, int ply, int depth, int wtm, int type,
    int value, int bestmove) {
  register BITBOARD word1, word2;
  register HASH_ENTRY *htable, *replace = 0;
  register int entry, draft, age, replace_draft;

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
 *   Now we search for an entry to overwrite in three       *
 *   passes.                                                *
 *                                                          *
 *   Pass 1:  If any signature in the table matches the     *
 *     current signature, we are going to overwrite this    *
 *     entry, period.  It might seem worthwhile to check    *
 *     the draft and not overwrite if the table draft is    *
 *     greater than the current remaining depth, but after  *
 *     you think about it, this is a bad idea.  If the      *
 *     draft is greater than or equal the current remaining *
 *     depth, then we should never get here unless the      *
 *     stored bound or score is unusable because of the     *
 *     current alpha/beta window.  So we are overwriting to *
 *     avoid losing the current result.                     *
 *                                                          *
 *   Pass 2:  If any of the entries come from a previous    *
 *     search (not iteration) then we choose the entry from *
 *     this set that has the smallest draft, since it is    *
 *     the least potentially usable result.                 *
 *                                                          *
 *   Pass 3:  If neither of the above two found an entry to *
 *     overwrite, we simply choose the entry from the       *
 *     bucket with the smallest draft and overwrite that.   *
 *                                                          *
 ************************************************************
 */
  htable = trans_ref + 4 * (word2 & hash_mask);
  for (entry = 0; entry < 4; entry++) {
    if (word2 == (htable->word1 ^ htable->word2)) {
      replace = htable;
      break;
    }
    htable++;
  }
  if (!replace) {
    replace_draft = 99999;
    htable = trans_ref + 4 * (word2 & hash_mask);
    for (entry = 0; entry < 4; entry++) {
      age = htable->word1 >> 61;
      draft = (htable->word1 >> 17) & 0x7fff;
      if (age != transposition_id && replace_draft > draft) {
        replace = htable;
        replace_draft = draft;
      }
      htable++;
    }
    if (!replace) {
      htable = trans_ref + 4 * (word2 & hash_mask);
      for (entry = 0; entry < 4; entry++) {
        draft = (htable->word1 >> 17) & 0x7fff;
        if (replace_draft > draft) {
          replace = htable;
          replace_draft = draft;
        }
        htable++;
      }
    }
  }
/*
 ************************************************************
 *                                                          *
 *   Now that we know which entry to replace, we simply     *
 *   stuff the values and exit.  Note that the two 64 bit   *
 *   words are xor'ed together and stored as the signature  *
 *   for the "lockless-hash" approach.                      *
 *                                                          *
 ************************************************************
 */
  replace->word1 = word1;
  replace->word2 = word2 ^ word1;
}

/* last modified 08/13/09 */
/*
 *******************************************************************************
 *                                                                             *
 *   HashStorePV() is called by Iterate() to insert the PV moves so they will  *
 *   be searched before any other moves.  Normally the PV moves would be in    *
 *   the table, but on occasion they can be overwritten, particularly the ones *
 *   that are a significant distance from the root since those table entries   *
 *   will have a low draft.                                                    *
 *                                                                             *
 *******************************************************************************
 */
void HashStorePV(TREE * RESTRICT tree, int wtm, int bestmove) {
  register int entry, draft, replace_draft, age;
  register HASH_ENTRY *htable, *replace;
  register BITBOARD temp_hashkey, word1, word2;

/*
 ************************************************************
 *                                                          *
 *   First, compute the initial hash address and the fake   *
 *   entry we will store if we don't find a valid match     *
 *   already in the table.                                  *
 *                                                          *
 ************************************************************
 */
  temp_hashkey = (wtm) ? HashKey : ~HashKey;
  word1 = transposition_id;
  word1 = (word1 << 2) | WORTHLESS;
  word1 = (word1 << 27) | bestmove;
  word1 = (word1 << 32) | 65536;
  word2 = temp_hashkey ^ word1;
/*
 ************************************************************
 *                                                          *
 *   Now we search for an entry to overwrite in three       *
 *   passes.                                                *
 *                                                          *
 *   Pass 1:  If any signature in the table matches the     *
 *     current signature, we are going to overwrite this    *
 *     entry, period.  It might seem worthwhile to check    *
 *     the draft and not overwrite if the table draft is    *
 *     greater than the current remaining depth, but after  *
 *     you think about it, this is a bad idea.  If the      *
 *     draft is greater than or equal the current remaining *
 *     depth, then we should never get here unless the      *
 *     stored bound or score is unusable because of the     *
 *     current alpha/beta window.  So we are overwriting to *
 *     avoid losing the current result.                     *
 *                                                          *
 *   Pass 2:  If any of the entries come from a previous    *
 *     search (not iteration) then we choose the entry from *
 *     this set that has the smallest draft, since it is    *
 *     the least potentially usable result.                 *
 *                                                          *
 *   Pass 3:  If neither of the above two found an entry to *
 *     overwrite, we simply choose the entry from the       *
 *     bucket with the smallest draft and overwrite that.   *
 *                                                          *
 ************************************************************
 */
  htable = trans_ref + 4 * (temp_hashkey & hash_mask);
  for (entry = 0; entry < 4; entry++) {
    if ((htable->word2 ^ htable->word1) == temp_hashkey) {
      htable->word1 &= ~((BITBOARD) 0x1fffff << 32);
      htable->word1 |= (BITBOARD) bestmove << 32;
      htable->word2 = temp_hashkey ^ htable->word1;
      break;
    }
    htable++;
  }
  if (entry == 4) {
    htable = trans_ref + 4 * (word2 & hash_mask);
    replace = 0;
    replace_draft = 99999;
    for (entry = 0; entry < 4; entry++) {
      age = htable->word1 >> 61;
      draft = (htable->word1 >> 17) & 0x7fff;
      if (age != transposition_id && replace_draft > draft) {
        replace = htable;
        replace_draft = draft;
      }
      htable++;
    }
    if (!replace) {
      htable = trans_ref + 4 * (word2 & hash_mask);
      for (entry = 0; entry < 4; entry++) {
        draft = (htable->word1 >> 17) & 0x7fff;
        if (replace_draft > draft) {
          replace = htable;
          replace_draft = draft;
        }
        htable++;
      }
    }
    replace->word1 = word1;
    replace->word2 = word2;
  }
}
