/*
 **********************************************************************
 * Slide functions.                                                   *
 **********************************************************************
 */
int SlideWR(TREE * RESTRICT tree, int square, int dir, int num)
{
  register int pc, score = 0;

  while (num--) {
    square += dir;
    pc = PcOnSq(square);

    if (pc) {
      if (pc < 0)       // Enemy piece.
      {
        num = 0;        // Exit loop.
        if (pc == -queen || pc == -king)
          score += attacks_enemy;
      } else if (pc == rook || pc == queen)
        score += supports_slider;
      else
        break;
    }

    score += sqrvalR[square];
  }     // While

  return score;
}

int SlideBR(TREE * RESTRICT tree, int square, int dir, int num)
{
  register int pc, score = 0;

  while (num--) {
    square += dir;
    pc = PcOnSq(square);

    if (pc) {
      if (pc > 0)       // Enemy piece.
      {
        num = 0;        // Exit loop.
        if (pc == queen || pc == king)
          score += attacks_enemy;
      } else if (pc == -rook || pc == -queen)
        score += supports_slider;
      else
        break;
    }

    score += sqrvalR[square];
  }     // While

  return score;
}

int SlideWB(TREE * RESTRICT tree, int square, int dir, int num)
{
  register int pc, score = 0;

  while (num--) {
    square += dir;
    pc = PcOnSq(square);

    if (pc) {
      if (pc < 0)       // Enemy piece.
      {
        num = 0;        // Exit loop.
        if (pc == -rook || pc == -queen || pc == -king)
          score += attacks_enemy;
      } else if (pc == queen)
        score += supports_slider;
      else
        break;
    }

    score += sqrvalB[square];
  }     // While

  return score;
}

int SlideBB(TREE * RESTRICT tree, int square, int dir, int num)
{
  register int pc, score = 0;

  while (num--) {
    square += dir;
    pc = PcOnSq(square);

    if (pc) {
      if (pc > 0)       // Enemy piece.
      {
        num = 0;        // Exit loop.
        if (pc == rook || pc == queen || pc == king)
          score += attacks_enemy;
      } else if (pc == -queen)
        score += supports_slider;
      else
        break;
    }

    score += sqrvalB[square];
  }     // While

  return score;
}

int BishopSlideW(TREE * RESTRICT tree, int square)
{
  register int score = sqrvalB[square];

  score += SlideWB(tree, square, UP_RIGHT, Num_up_right(square));
  score += SlideWB(tree, square, UP_LEFT, Num_up_left(square));
  score += SlideWB(tree, square, DOWN_RIGHT, Num_down_right(square));
  score += SlideWB(tree, square, DOWN_LEFT, Num_down_left(square));

  return score;
}

int BishopSlideB(TREE * RESTRICT tree, int square)
{
  register int score = sqrvalB[square];

  score += SlideBB(tree, square, DOWN_RIGHT, Num_down_right(square));
  score += SlideBB(tree, square, DOWN_LEFT, Num_down_left(square));
  score += SlideBB(tree, square, UP_RIGHT, Num_up_right(square));
  score += SlideBB(tree, square, UP_LEFT, Num_up_left(square));

  return score;
}

int RookSlideW(TREE * RESTRICT tree, int square)
{
  register int score = sqrvalR[square];

  score += SlideWR(tree, square, UP, Num_up(square));
  score += SlideWR(tree, square, RIGHT, Num_right(square));
  score += SlideWR(tree, square, DOWN, Num_down(square));
  score += SlideWR(tree, square, LEFT, Num_left(square));

  return score * lower_r_percent / 10;
}

int RookSlideB(TREE * RESTRICT tree, int square)
{
  register int score = sqrvalR[square];

  score += SlideBR(tree, square, DOWN, Num_down(square));
  score += SlideBR(tree, square, RIGHT, Num_right(square));
  score += SlideBR(tree, square, UP, Num_up(square));
  score += SlideBR(tree, square, LEFT, Num_left(square));

  return score * lower_r_percent / 10;
}
