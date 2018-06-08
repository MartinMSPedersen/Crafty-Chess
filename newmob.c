int EvaluateMobility(TREE * RESTRICT tree) {
  register long long bishops, rooks, moves;
  register int square, score = 0;

  bishops = WhiteBishops;
  while (bishops) {
    square = LSB(bishops);
    bishops &= bishops - 1;
    moves = AttacksBishopSpecial(square, Occupied ^ WhiteQueens);
    score += attacks_enemy *
      PopCnt(moves & (BlackRooks | BlackKing | BlackQueens));
    score += supports_slider * PopCnt(moves & WhiteQueens);
    for (i=0; i<5; i++) {
      score += PopCnt(moves & mobility_mask_b[i]) * mobility_score_b[i];
    }
  }
}
