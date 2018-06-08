#include <stdio.h>
#include <stdlib.h>
#include "types.h"
#include "function.h"
#include "data.h"

/* last modified 08/27/96 */
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
*       2       age  62  search id to identify old trans/ref entried.          *
*       2      type  60  0->value is worthless; 1-> value represents a fail-   *
*                        low bound; 2-> value represents a fail-high bound;    *
*                        3-> value is an exact score.                          *
*      19     value  41  unsigned integer value of this position + 131072.     *
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
  register HASH_ENTRY *htablea, *htableb;
  register BITBOARD word1, word2;
  register int draft, age;
/*
 ----------------------------------------------------------
|                                                          |
|   first, compute the initial hash address and choose     |
|   which hash table (based on color) to probe.            |
|                                                          |
 ----------------------------------------------------------
*/
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
|   now "fill in the blank" and build a table entry from   |
|   current search information.                            |
|                                                          |
 ----------------------------------------------------------
*/
  word1=0;

  if (value > alpha) {
    if (abs(value) < MATE-100) word1=Or(word1,Shiftl((BITBOARD) (value+131072),41));
    else if (value > 0) word1=Or(word1,Shiftl((BITBOARD) (value+ply-1+131072),41));
    else word1=Or(word1,Shiftl((BITBOARD) (value-ply+1+131072),41));
    word1=Or(word1,Shiftl((BITBOARD) ((transposition_id<<2)+EXACT_SCORE),60));
    if (pv[ply].path_length >= ply) 
      word1=Or(word1,(BITBOARD) pv[ply].path[ply]);
  }
  else {
    word1=Or(word1,Shiftl((BITBOARD) (value+131072),41));
    word1=Or(word1,Shiftl((BITBOARD) ((transposition_id<<2)+LOWER_BOUND),60));
  }

  word2=Or(temp_hash_key,Shiftl((BITBOARD) depth,48));

  draft=((int) Shiftr(htablea->word2,48)) & 0377;
  age=((unsigned int) Shiftr(htablea->word1,62))!=transposition_id;
  if (age || (depth >= draft)) {
    htablea->word1=word1;
    htablea->word2=word2;
  }
  htableb->word1=word1;
  htableb->word2=word2;
}

/* last modified 08/27/96 */
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
  register HASH_ENTRY *htable;
/*
 ----------------------------------------------------------
|                                                          |
|   make sure the move being stored is legal, so that a    |
|   bad move doesn't get into hash table.                  |
|                                                          |
 ----------------------------------------------------------
*/
  if (!ValidMove(ply,wtm,pv[ply].path[ply])) {
    Print(0,"\ninstalling bogus move...ply=%d\n",ply);
    Print(0,"installing %s\n",OutputMove(&pv[ply].path[ply],ply,wtm));
    return;
  }
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
  htable->word1=Shiftl((BITBOARD) 131072,21);
  htable->word1=Or(htable->word1,Shiftl((BITBOARD) ((transposition_id<<2)+WORTHLESS),61));
  htable->word1=Or(htable->word1,(BITBOARD) pv[ply].path[ply]);
  htable->word2=temp_hash_key;
}

/* last modified 08/27/96 */
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
  register HASH_ENTRY *htablea, *htableb;
  register BITBOARD word1, word2;
  register int draft, age;
/*
 ----------------------------------------------------------
|                                                          |
|   first, compute the initial hash address and choose     |
|   which hash table (based on color) to probe.            |
|                                                          |
 ----------------------------------------------------------
*/
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
|   now "fill in the blank" and build a table entry from   |
|   current search information.                            |
|                                                          |
 ----------------------------------------------------------
*/
  word1=Shiftl((BITBOARD) (bound+131072),41);
  word1=Or(word1,Shiftl((BITBOARD) ((transposition_id<<2)+UPPER_BOUND),60));
  word1=Or(word1,(BITBOARD) current_move[ply]);

  word2=Or(temp_hash_key,Shiftl((BITBOARD) depth,48));

  draft=((int) Shiftr(htablea->word2,48)) & 0377;
  age=((unsigned int) Shiftr(htablea->word1,62))!=transposition_id;

  if (age || (depth >= draft)) {
    htablea->word1=word1;
    htablea->word2=word2;
  }
  htableb->word1=word1;
  htableb->word2=word2;
}
