/*
 *******************************************************************************
 *                                                                             *
 *   RecoverPV() is used to probe the hash table and add any positions found   *
 *   to the end.  This is primarily used when we get a fail-high which does    *
 *   not return a PV.  This reconstructed PV at least gives a clue about where *
 *   the game is going.                                                        *
 *                                                                             *
 *******************************************************************************
 */
void RecoverPV(TREE * RESTRICT tree, int level, int wtm, int time, int value,
    PATH * pv) {
  char buffer[4096], *buffp, *bufftemp;
  int i, t_move_number, type, j, dummy = 0;
  int nskip = 0, twtm = wtm;

  root_print_ok = root_print_ok || tree->nodes_searched > noise_level;
/*
 ************************************************************
 *                                                          *
 *   Initialize.                                            *
 *                                                          *
 ************************************************************
 */
  for (i = 1; i <= (int) pv->pathl; i++) {
    if ((display_options & 64) && i > 1 && wtm)
    MakeMove(tree, i, pv->path[i], wtm);
    wtm = Flip(wtm);
  }
/*
 ************************************************************
 *                                                          *
 *   If the pv was terminated prematurely by a trans/ref    *
 *   hit, see if any more moves are in the trans/ref table  *
 *   and if so, add 'em to the end of the PV so we will     *
 *   have better move ordering next iteration.              *
 *                                                          *
 ************************************************************
 */
  for (i = pv->pathl + 1; i < MAXPLY; i++) {
    HashProbe(tree, i, 0, wtm, &dummy, dummy);
    if (tree->hash_move[i] && ValidMove(tree, i, wtm, tree->hash_move[i])) {
      pv->path[i] = tree->hash_move[i];
      for (j = 1; j < i; j++)
        if (pv->path[i] == pv->path[j])
          break;
      if (j < i)
        break;
      pv->pathl++;
      MakeMove(tree, i, pv->path[i], wtm);
    } else
      break;
    wtm = Flip(wtm);
  }
  for (i = pv->pathl; i > 0; i--) {
    wtm = Flip(wtm);
    UnmakeMove(tree, i, pv->path[i], wtm);
  }
}

