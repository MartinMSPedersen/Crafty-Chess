#include <stdio.h>
#include <stdlib.h>
#include "types.h"
#include "function.h"
#include "data.h"
/*
********************************************************************************
*                                                                              *
*   Store_*() is used to store entries into the transposition table so that    *
*   this sub-tree won't have to be searched again if the same position is      *
*   reached.  a transposition/refutation table position contains the following *
*   data packed into 128 bits, with each item taking the number of bits given  *
*   in the table below.                                                        *
*                                                                              *
*     bits     name  SL  description                                           *
*       1       age  63  0->old position, 1-> position is from current search. *
*       2      type  61  0->value is worthless; 1-> value is a good score;     *
*                        2-> value is a backed up bound; 3-> value is a bound  *
*                        and the current position faild high.                  *
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
*                        position.                                             *
*      48       key   0  leftmost 48 bits of the 64 bit hash key.  this is     *
*                        used to "verify" that this entry goes with the        *
*                        current board position.                               *
*                                                                              *
*   StoreBest() is called when a ply has been completed, and we are ready to   *
*   back up a new best move and score to the previous ply.  we can extract the *
*   best move from the current search path.                                    *
*                                                                              *
********************************************************************************
*/
void StoreBest(int ply, int depth, int wtm, int value, int alpha)
{
  register BITBOARD temp_hash_key;
  register HASH_ENTRY *ht, *htable, *best;
  register int i, found, best_draft, rehash, type;
  register int eval;
  register int draft, age;
  register int old_move;
/*
 ----------------------------------------------------------
|                                                          |
|   first, "adjust" the hash key to include both castling  |
|   status and en passant status.                          |
|                                                          |
 ----------------------------------------------------------
*/
  if (!trans_ref_w) return;
  temp_hash_key=HashKey(ply);
  if (wtm) ht=trans_ref_w+And(temp_hash_key,hash_mask);
  else ht=trans_ref_b+And(temp_hash_key,hash_mask);
  rehash=And(Shiftr(temp_hash_key,log_hash_table_size),mask_118)+1;
/*
 ----------------------------------------------------------
|                                                          |
|   now, locate the most "useless entry" which will be     |
|   replaced by this call to store.best().  the first pass |
|   searches for an exact match.  if one is found, we must |
|   store in that entry.                                   |
|                                                          |
 ----------------------------------------------------------
*/
  if (static_eval[ply]) eval=static_eval[ply]+131072;
  else eval=131072;
  htable=ht;
  found=0;
  old_move=0;
  if (depth < 0) depth=0;
  for (i=0;i<4;i++) {
    if (!Xor(And(htable->word2,mask_80),Shiftr(temp_hash_key,16))) {
      found=1;
      if (eval == 131072) eval=And(Shiftr(htable->word1,21),mask_108);
      draft=((int) Shiftr(htable->word2,48)) & 255;
      type=Shiftr(htable->word1,61) & 3;
      if ((draft > depth) && (type == good_score)) return;
      old_move=And(htable->word1,mask_107);
      break;
    }
    htable+=rehash;
  }
/*
 ----------------------------------------------------------
|                                                          |
|   if that failed, try to find one that was stored from   |
|   a previous search or search iteration.  if so, we will |
|   replace that entry.                                    |
|                                                          |
 ----------------------------------------------------------
*/
  if (!found) {
    htable=ht;
    for (i=0;i<4;i++) {
      age=Shiftr(htable->word1,63);
      if (age) {
        found=1;
        break;
      }
      else {
        type=Shiftr(htable->word1,61) & 3;
        if (type == worthless) {
          found=1;
          break;
        }
      }
      htable+=rehash;
    }
  }
/*
 ----------------------------------------------------------
|                                                          |
|   if that also failed, try to find one that was stored   |
|   from the current search, but which contains less       |
|   useful information than what we want to store so we    |
|   will lose as little as possible.                       |
|                                                          |
 ----------------------------------------------------------
*/
  if (!found) {
    best_draft=999;
    htable=ht;
    for (i=0;i<4;i++) {
      draft=((int) Shiftr(htable->word2,48)) & 255;
      if (best_draft > draft) {
        best_draft=draft;
        best=htable;
      }
      htable+=rehash;
    }
    htable=best;
  }
/*
 ----------------------------------------------------------
|                                                          |
|   now "fill in the blank" and build a table entry from   |
|   current search information.                            |
|                                                          |
 ----------------------------------------------------------
*/
  if (abs(value) < MATE-100) htable->word1=Shiftl((BITBOARD) value+131072,41);
  else if (value > 0) htable->word1=Shiftl((BITBOARD) value+ply-1+131072,41);
  else htable->word1=Shiftl((BITBOARD) value-ply+1+131072,41);
  htable->word1=Or(htable->word1,Shiftl((BITBOARD) eval,21));
  if (value > alpha)
    htable->word1=Or(htable->word1,Shiftl((BITBOARD) good_score,61));
  else
    htable->word1=Or(htable->word1,Shiftl((BITBOARD) failed_low,61));
  if ((value != alpha) && (pv[ply].path_length >= ply)) {
    htable->word1=Or(htable->word1,pv[ply].path[ply]);
#if defined(DEBUG)
    if (!ValidMove(ply,wtm,pv[ply].path[ply]))
      printf("StoreBest() storing an illegal move\n");
#endif
  }
  else if (old_move) htable->word1=Or(htable->word1,old_move);

  htable->word2=Or(Shiftr(temp_hash_key,16),Shiftl((BITBOARD) depth,48));
}

/*
********************************************************************************
*                                                                              *
*   StorePV() is called by Iterate() to insert the PV moves so they will be    *
*   searched before any other moves.                                           *
*                                                                              *
********************************************************************************
*/
void StorePV(int ply, int wtm)
{
  register BITBOARD temp_hash_key;
  register HASH_ENTRY *ht, *htable, *best;
  register int i, found, rehash;
/*
 ----------------------------------------------------------
|                                                          |
|   make sure the move being stored is legal, so that a    |
|   bad move doesn't get into hash table.                  |
|                                                          |
 ----------------------------------------------------------
*/
  if (!ValidMove(ply,wtm,pv[ply].path[ply])) {
    fprintf(log_file,"\ninstalling bogus move...ply=%d  len=%d\n",ply,pv[ply].path_length);
    fprintf(log_file,"installing %s\n",OutputMove(&pv[ply].path[ply],ply,wtm));
    return;
  }
/*
 ----------------------------------------------------------
|                                                          |
|   first, "adjust" the hash key to include both castling  |
|   status and en passant status.                          |
|                                                          |
 ----------------------------------------------------------
*/
  if (!trans_ref_w) return;
  temp_hash_key=HashKey(ply);
  if (wtm) ht=trans_ref_w+And(temp_hash_key,hash_mask);
  else ht=trans_ref_b+And(temp_hash_key,hash_mask);
  rehash=And(Shiftr(temp_hash_key,log_hash_table_size),mask_118)+1;
/*
 ----------------------------------------------------------
|                                                          |
|   now, locate the most "useless entry" which will be     |
|   replaced by this call to store.best().  the first pass |
|   searches for an exact match.  if one is found, we must |
|   store in that entry.                                   |
|                                                          |
 ----------------------------------------------------------
*/
  htable=ht;
  found=0;
  for (i=0;i<4;i++) {
    if (!Xor(And(htable->word2,mask_80),Shiftr(temp_hash_key,16))) {
      found=1;
      break;
    }
    htable+=rehash;
  }
  if (!found) htable=ht;
/*
 ----------------------------------------------------------
|                                                          |
|   now "fill in the blank" and build a table entry from   |
|   current search information.                            |
|                                                          |
 ----------------------------------------------------------
*/
  if (found)
    htable->word1=Or(And(htable->word1,Compl(mask_107)),pv[ply].path[ply]);
  else {
    htable->word1=Shiftl((BITBOARD) 131072,21);
    htable->word1=Or(htable->word1,Shiftl((BITBOARD) worthless,61));
    htable->word1=Or(htable->word1,pv[ply].path[ply]);
    htable->word2=Shiftr(temp_hash_key,16);
  }
}

/*
********************************************************************************
*                                                                              *
*   StoreRefutation() is called when a move at the current ply is so good it   *
*   "refutes" the move made at the previous ply.  we then remember this so it  *
*   can be used to do the same the next time this position is reached.         *
*                                                                              *
********************************************************************************
*/
void StoreRefutation(int ply, int depth, int wtm, int bound)
{
  register BITBOARD temp_hash_key;
  register HASH_ENTRY *ht, *htable, *best;
  register int i, found, best_draft, rehash, type;
  register int eval;
  register int draft, age;
/*
 ----------------------------------------------------------
|                                                          |
|   first, "adjust" the hash key to include both castling  |
|   status and en passant status.                          |
|                                                          |
 ----------------------------------------------------------
*/
  if (!trans_ref_w) return;
  temp_hash_key=HashKey(ply);
  if (wtm) ht=trans_ref_w+And(temp_hash_key,hash_mask);
  else ht=trans_ref_b+And(temp_hash_key,hash_mask);
  rehash=And(Shiftr(temp_hash_key,log_hash_table_size),mask_118)+1;
/*
 ----------------------------------------------------------
|                                                          |
|   now, locate the most "useless entry" which will be     |
|   replaced by this call to store.best().  the first pass |
|   searches for an exact match.  if one is found, we must |
|   store in that entry.                                   |
|                                                          |
 ----------------------------------------------------------
*/
  htable=ht;
  found=0;
  if (static_eval[ply]) eval=static_eval[ply]+131072;
  else eval=131072;
  if (depth < 0) depth=0;
  for (i=0;i<4;i++) {
    if (!Xor(And(htable->word2,mask_80),Shiftr(temp_hash_key,16))) {
      found=1;
      if (eval == 131072) eval=And(Shiftr(htable->word1,21),mask_108);
      draft=((int) Shiftr(htable->word2,48)) & 255;
      type=Shiftr(htable->word1,61) & 3;
      if (draft > depth) return;
      else if ((draft == depth) && (type == good_score)) return;
      break;
    }
    htable+=rehash;
  }
/*
 ----------------------------------------------------------
|                                                          |
|   if that failed, try to find one that was stored from   |
|   a previous search or search iteration.  if so, we will |
|   replace that entry.                                    |
|                                                          |
 ----------------------------------------------------------
*/
  if (!found) {
    htable=ht;
    for (i=0;i<4;i++) {
      age=Shiftr(htable->word1,63);
      if (age) {
        found=1;
        break;
      }
      else {
        type=Shiftr(htable->word1,61) & 3;
        if (type == worthless) {
          found=1;
          break;
        }
      }
      htable+=rehash;
    }
  }
/*
 ----------------------------------------------------------
|                                                          |
|   if that also failed, try to find one that was stored   |
|   from the current search, but which contains less       |
|   usefull information than what we want to store so we   |
|   will lose as little as possible.                       |
|                                                          |
 ----------------------------------------------------------
*/
  if (!found) {
    best_draft=999;
    htable=ht;
    for (i=0;i<4;i++) {
      draft=((int) Shiftr(htable->word2,48)) & 255;
      if (best_draft > draft) {
        best_draft=draft;
        best=htable;
      }
      htable+=rehash;
    }
    htable=best;
  }
/*
 ----------------------------------------------------------
|                                                          |
|   now "fill in the blank" and build a table entry from   |
|   current search information.                            |
|                                                          |
 ----------------------------------------------------------
*/
  htable->word1=Shiftl((BITBOARD) bound+131072,41);
  htable->word1=Or(htable->word1,Shiftl((BITBOARD) eval,21));
  htable->word1=Or(htable->word1,Shiftl((BITBOARD) failed_high,61));
  if (Piece(current_move[ply])) {
    htable->word1=Or(htable->word1,current_move[ply]);
#if defined(DEBUG)
    if (!ValidMove(ply,wtm,current_move[ply]))
      printf("StoreRefutation() storing an illegal move\n");
#endif
  }

  htable->word2=Or(Shiftr(temp_hash_key,16),Shiftl((BITBOARD) depth,48));
}
