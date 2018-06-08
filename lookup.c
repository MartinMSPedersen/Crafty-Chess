#include <stdio.h>
#include <stdlib.h>
#include "types.h"
#include "function.h"
#include "data.h"
/*
********************************************************************************
*                                                                              *
*   Lookup() is used to retrieve entries from the transposition table so that  *
*   this sub-tree won't have to be searched again if we reach a position that  *
*   has been searched previously.  a transposition table position contains the *
*   following data packed into 128 bits with each item taking the number of    *
*   bits given in the table below:                                             *
*                                                                              *
*     bits     name  SL  description                                           *
*       1       age  63  0->old position, 1-> position is from current search. *
*       2      type  61  0->value is a backed-up good score, 1->value is a     *
*                        backed-up search bound, 2->value is a bound and this  *
*                        table position failed high on it.                     *
*      20     value  41  unsigned integer value of this position + 131072.     *
*                        this might be a good score or search bound.           *
*      20    pvalue  21  unsigned integer value the evaluate() procedure       *
*                        produced for this position (0=none)+131072.           *
*      21      move   0  best move from the current position, according to the *
*                        search at the time this position was stored.          *
*                                                                              *
*       8    unused  56  currently the final 8 bits are unused.                *
*       8     draft  48  the depth of the search below this position, which is *
*                        used to see if we can use this entry at the current   *
*                        position.  (+128 to make it positive.)                *
*      48       key   0  leftmost 48 bits of the 64 bit hash key.  this is     *
*                        used to "verify" that this entry goes with the        *
*                        current board position.                               *
*                                                                              *
********************************************************************************
*/
int Lookup(int ply, int depth, int wtm, int *value, int alpha, int beta)
{
  BITBOARD temp_hash_key;
  HASH_ENTRY *htable;
  int i, found, rehash;
  int draft, type, val;
/*
 ----------------------------------------------------------
|                                                          |
|   first, "adjust" the hash key to include both castling  |
|   status and en passant status.                          |
|                                                          |
 ----------------------------------------------------------
*/
  positional_evaluation[ply]=0;
  hash_move[ply]=0;
  if (!trans_ref_w) return(worthless);
  temp_hash_key=Hash_Key(ply);
  if (EnPassant_Target(ply))
    temp_hash_key=Xor(Hash_Key(ply),
                      enpassant_random[First_One(
                        EnPassant_Target(ply))]);
  if (White_Castle(ply))
    temp_hash_key=Xor(temp_hash_key,
                      castle_random_w[(int) White_Castle(ply)]);
  if (Black_Castle(ply))
    temp_hash_key=Xor(temp_hash_key,
                      castle_random_b[(int) Black_Castle(ply)]);
  if (wtm)
    htable=trans_ref_w+And(temp_hash_key,hash_mask);
  else
    htable=trans_ref_b+And(temp_hash_key,hash_mask);
  rehash=And(Shiftr(temp_hash_key,log_hash_table_size),mask_118)+1;

/*
 ----------------------------------------------------------
|                                                          |
|   now, search for the current position by selecting a    |
|   "set" of entries that might contain this position.     |
|                                                          |
 ----------------------------------------------------------
*/
  found=0;
  for (i=0;i<4;i++) {
    if (!Xor(And(htable->word2,mask_80),Shiftr(temp_hash_key,16))) {
      found=1;
      break;
    }
    htable+=rehash;
  }
  if (!found) return(worthless);
  transposition_hashes++;
/*
 ----------------------------------------------------------
|                                                          |
|   if we found the current position, remember the move so |
|   that we can try it first if we have to search beyond   |
|   this point.  also save the positional evaluation so    |
|   we might avoid doing an evaluate() for this node if    |
|   it is a quiescence search node.                        |
|                                                          |
 ----------------------------------------------------------
*/
  positional_evaluation[ply]=And(Shiftr(htable->word1,21),
                                  mask_108)-131072;
  hash_move[ply]=And(htable->word1,mask_107);
  if (hash_move[ply])
    (void) Valid_Move(ply,wtm,hash_move[ply]);
/*
 ----------------------------------------------------------
|                                                          |
|   we've found the current position in the table.  the    |
|   *big* question is, did the search done from the        |
|   position before it was stored proceed deep enough to   |
|   satisfy the current depth requirement?                 |
|                                                          |
 ----------------------------------------------------------
*/
  if (depth < 0) depth=0;
  draft=Shiftr(htable->word2,48) & 255;
  if (depth > draft) return(worthless);
  type=Shiftr(htable->word1,61) & 3;
  val=And(Shiftr(htable->word1,41),mask_108)-131072;
  switch (type) {
/*
 ----------------------------------------------------------
|                                                          |
|   we found the position, but it now represents a         |
|   "worthless" value since some scoring component has     |
|   been modified.                                         |
|                                                          |
 ----------------------------------------------------------
*/
    case worthless:
      return(worthless);
/*
 ----------------------------------------------------------
|                                                          |
|   we found the position, and it represented a "true"     |
|   value when it was stored.  We can simply return this   |
|   value to search.                                       |
|                                                          |
 ----------------------------------------------------------
*/
    case backed_up_value:
      transposition_hashes_value++;
      if (abs(val) > MATE-100) {
        if (val > 0) 
          val-=(ply-1);
        else
          val+=(ply-1);
      }
      if (val >= beta)
        *value=beta;
      else {
        if (current_move[ply-1] != 0)
          *value=val;
        else
          return(worthless);
      }
      return(backed_up_value);
/*
 ----------------------------------------------------------
|                                                          |
|   we found the position, however, it represents a case   |
|   where every move at this position failed low.  as a    |
|   result, the lower bound (alpha) was returned which     |
|   caused a cutoff.  return this alpha value to the       |
|   search to see if it will either cutoff or at least     |
|   improve the lower bound somewhat.                      |
|                                                          |
 ----------------------------------------------------------
*/
    case backed_up_bound:
      transposition_hashes_bound++;
      if (val <= alpha) 
        return(backed_up_bound);
      else
        return(worthless);
/*
 ----------------------------------------------------------
|                                                          |
|   we found the position, however, it represents a case   |
|   where the first move at this position failed high.  as |
|   a result, the upper bound (beta) was returned which    |
|   caused a cutoff.  return this beta value to the search |
|   to see if it will either cutoff or at least improve    |
|   the upper bound somewhat.                              |
|                                                          |
 ----------------------------------------------------------
*/
    case cutoff_bound:
      transposition_hashes_cutoff++;
      if (val >= beta) 
        return(cutoff_bound);
      else
        return(worthless);
  }
  return(worthless);
}
