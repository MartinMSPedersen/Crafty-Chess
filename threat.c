#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "chess.h"
#include "data.h"
#define THREAT_MARGIN 150

/* last modified 01/23/01 */
/*
********************************************************************************
*                                                                              *
*   Threat() is used to extend the depth for positions that appear to have     *
*   some threat.  the basic algorithm used is a null move search, but with the *
*   alpha/beta window set to (v-n,v-n+1) where n is some significant score     *
*   (normally 1.5 pawns).  Threat() is called when search returns a value that *
*   is > alpha (>= beta means this is a fail-hi node, while > alpha < beta     *
*   means this is a PV node.)  Threat() simply does a null move search with    *
*   the sharply lowered bounds (given above) to see if this move is preventing *
*   the score from dropping dramatically (such as happens for horizon effect   *
*   moves.)  if the proposed move fails hi, but "passing" fails low by a large *
*   amount, then something is "fishy" and the position needs to be searched    *
*   again, but with an increased search depth to try and resolve what's going  *
*   on.                                                                        *
*                                                                              *
********************************************************************************
*/
int Threat(TREE *tree, int value, int alpha, int beta, int wtm,
           int depth, int ply) {
  BITBOARD save_hash_key;
  register int new_value, temp, null_depth, pieces, incheck;
  PATH temp_p;
/*
 ----------------------------------------------------------
|                                                          |
|   if the current ply has already been extended by this   |
|   algorithm, we just research the current fail-high move |
|   to depth+1, the null-move threat test is not necessary |
|   again.                                                 |
|                                                          |
 ----------------------------------------------------------
*/
  if (!(tree->extended_reason[ply]&threat_extension)) {
/*
 ----------------------------------------------------------
|                                                          |
|   if the remaining full-width depth is not at least 4,   |
|   then don't try threat extensions.                      |
|                                                          |
 ----------------------------------------------------------
*/
    if (depth < 4*INCPLY) return(value);
/*
 ----------------------------------------------------------
|                                                          |
|   if the remaining search has already been extended an   |
|   excessive amount, don't do threat extensions too.      |
|                                                          |
 ----------------------------------------------------------
*/
    if (ply > 2*iteration_depth) return(value);
/*
 ----------------------------------------------------------
|                                                          |
|   if the side on move is winning significantly, then the |
|   test is worthless and should be skipped.               |
|                                                          |
 ----------------------------------------------------------
*/
    if (( wtm && Material-THREAT_MARGIN > beta) ||
        (!wtm && -Material-THREAT_MARGIN > beta))
      return(value);
/*
 ----------------------------------------------------------
|                                                          |
|   if the current move is capturing a piece moved at the  |
|   previous ply, then the threat extension will probably  |
|   fail low, since we would then not be re-capturing this |
|   piece and let it slip away.                            |
|                                                          |
 ----------------------------------------------------------
*/
    if (Captured(tree->current_move[ply]) &&
        (To(tree->current_move[ply]) == To(tree->current_move[ply-1])))
      return(value);
/*
 ----------------------------------------------------------
|                                                          |
|   first, do a null-move search with the lowered window   |
|   to see if this fails low or high.                      |
|                                                          |
 ----------------------------------------------------------
*/
    incheck=tree->in_check[ply+1];
    temp=tree->current_move[ply];
    UnMakeMove(tree,ply,temp,wtm);
    temp_p=tree->pv[ply];
    tree->current_move[ply]=0;
    tree->phase[ply]=NULL_MOVE;
    tree->in_check[ply+1]=0;
#if defined(TRACE)
    if (ply <= trace_level)
      SearchTrace(tree,ply,depth,wtm,alpha,beta,"Threat",0);
#endif
    tree->position[ply+1]=tree->position[ply];
    Rule50Moves(ply+1)++;
    save_hash_key=HashKey;
    if (EnPassant(ply)) {
      HashEP(EnPassant(ply+1),HashKey);
      EnPassant(ply+1)=0;
    }
    pieces=(wtm) ? TotalWhitePieces : TotalBlackPieces;
    null_depth=(depth>6*INCPLY && pieces>8) ? null_max : null_min;
    new_value=-Search(tree, THREAT_MARGIN-value-1, THREAT_MARGIN-value,
                      ChangeSide(wtm),depth-null_depth,
                      ply+1,NO_NULL);
    HashKey=save_hash_key;
    tree->current_move[ply]=temp;
    MakeMove(tree,ply,tree->current_move[ply],wtm);
    tree->pv[ply]=temp_p;
    if (abort_search) return(0);
    tree->extended_reason[ply]|=threat_extension;
    if (new_value >= value-THREAT_MARGIN) return(value);
  }
/*
 ----------------------------------------------------------
|                                                          |
|   if the null-move searched failed low, then something   |
|   is wrong with this move/position.  search it again,    |
|   but one ply deeper than before.                        |
|                                                          |
 ----------------------------------------------------------
*/
  tree->threat_extensions_done++;
  UnMakeMove(tree,ply,tree->current_move[ply],wtm);
#if defined(TRACE)
  if (ply <= trace_level)
    SearchTrace(tree,ply,depth,wtm,alpha,beta,"ThreatMove",tree->phase[ply]);
#endif
  MakeMove(tree,ply,tree->current_move[ply],wtm);
  tree->in_check[ply+1]=incheck;
  value=-Search(tree,-beta,-alpha,ChangeSide(wtm),depth,ply+1,1);
  if (abort_search) return(0);
  return(value);
}
