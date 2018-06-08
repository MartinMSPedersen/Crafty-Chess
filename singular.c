#if defined(SINGULAR)
#  include <stdio.h>
#  include <stdlib.h>
#  include <string.h>
#  include "chess.h"
#  include "data.h"
#  include "epdglue.h"

#  define SINGULAR_MARGIN 75

/* modified 01/07/04 */
/*
********************************************************************************
*                                                                              *
*   Singular() is a routine used by Search() to determine if a move appears to *
*   be "singular".  IE is there one move that appears to be better than all of *
*   the other moves by a significant margin (SINGULAR_MARGIN).  if so, this    *
*   move is passed back to Search() which will extend this one move by the     *
*   normal singular extension amount (default = 3/4 ply).                      *
*                                                                              *
********************************************************************************
*/
int Singular(TREE * tree, int alpha, int wtm, int depth, int ply)
{
  register int singular = 0;
  int singular_move = 0, tried = 0;
  register int value, extended = 0;
  register int save_hash_move;

/*
************************************************************
*                                                          *
*   if this is an endgame, with limited material, return   *
*   as the test becomes too expensive.                     *
*                                                          *
************************************************************
*/
  if (((wtm) ? TotalWhitePieces : TotalBlackPieces) < 5)
    return (0);
/*
************************************************************
*                                                          *
*   initialize.                                            *
*                                                          *
************************************************************
*/
  tree->in_check[ply + 1] = 0;
  tree->last[ply] = tree->last[ply - 1];
  tree->next_status[ply].phase = HASH_MOVE;
  save_hash_move = tree->hash_move[ply];
/*
************************************************************
*                                                          *
*   now iterate through the move list and search the       *
*   resulting positions.  note that Search() culls any     *
*   move that is not legal by using Check().  the special  *
*   case is that we must find one legal move to search to  *
*   confirm that it's not a mate or draw.                  *
*                                                          *
************************************************************
*/
  while (tree->phase[ply] = NextMove(tree, ply, wtm)) {
    if (!CaptureOrPromote(tree->current_move[ply])) {
      if (++tried >= 4 && singular == 0)
        break;
    }
#  if !defined(FAST)
    if (ply <= trace_level)
      SearchTrace(tree, ply, depth, wtm, alpha - SINGULAR_MARGIN - 1,
          alpha - SINGULAR_MARGIN, "Singular", tree->phase[ply]);
#  endif
/*
************************************************************
*                                                          *
*   make the move and search the resulting position.       *
*   make sure the side-on-move is not in check after the   *
*   move to weed out illegal moves and save time.          *
*                                                          *
************************************************************
*/
    MakeMove(tree, ply, tree->current_move[ply], wtm);
    if (!Check(wtm)) {
/*
************************************************************
*                                                          *
*   if the move to be made checks the opponent, then we    *
*   need to remember that he's in check.                   *
*                                                          *
************************************************************
*/
      extended = 0;
      tree->in_check[ply + 1] = 0;
      if (Check(Flip(wtm))) {
        tree->in_check[ply + 1] = 1;
        extended += incheck_depth;
      }
      value =
          -Search(tree, -alpha + SINGULAR_MARGIN + 1, -alpha + SINGULAR_MARGIN,
          Flip(wtm), depth - 240 + extended, ply + 1, 1, 0);
      if (abort_search) {
        UnmakeMove(tree, ply, tree->current_move[ply], wtm);
        return (0);
      }
      if (value > alpha - SINGULAR_MARGIN - 1 &&
          !CaptureOrPromote(tree->current_move[ply])) {
        singular++;
        singular_move = tree->current_move[ply];
      }
    }
    UnmakeMove(tree, ply, tree->current_move[ply], wtm);
    if (singular > 1)
      break;
  }
/*
************************************************************
*                                                          *
*   all moves have been searched.                          *
*                                                          *
************************************************************
*/
  tree->hash_move[ply] = save_hash_move;
  if (singular != 1)
    return (0);
#  if defined(TRACE)
  if (ply <= trace_level)
    Print(4095, "ply=%d move %s appears to be 'singular'\n", ply,
        OutputMove(tree, singular_move, ply, wtm));
#  endif
  return (singular_move);
}
#endif
