
/*
 ----------------------------------------------------------
|                                                          |
|   if white has a bishop and pawn(s) then the pawn had    |
|   better not be only rook pawns, or else the bishop had  |
|   better be the right color, otherwise its a DRAW if the |
|   black king can block the pawn.                         |
|                                                          |
 ----------------------------------------------------------
*/
  if (TotalBlackPieces==0 && TotalWhitePawns &&
      !(WhitePawns&not_rook_pawns)) {
    if (WhiteBishops) {
      if (WhiteBishops&dark_squares) {
        if (file_mask[FILEH]&WhitePawns) return(0);
      }
      else if (file_mask[FILEA]&WhitePawns) return(0);
    }
    else if (TotalWhitePieces==0) {
      if (file_mask[FILEA]&WhitePawns &&
          file_mask[FILEH]&WhitePawns) return(0);
    }
    else return(0);

    if (!(WhitePawns&file_mask[FILEA]) ||
        !(WhitePawns&file_mask[FILEH])) {
      square=LastOne(WhitePawns);
      if (Rank(BlackKingSQ)>=Rank(square) &&
          FileDistance(BlackKingSQ,square)<=2) return(1);
      return(0);
    }
  }
/*
 ----------------------------------------------------------
|                                                          |
|   if black has a bishop and pawn(s) then the pawn had    |
|   better not be only rook pawns, or else the bishop had  |
|   better be the right color, otherwise its a DRAW if the |
|   white king can block the pawn.                         |
|                                                          |
 ----------------------------------------------------------
*/
  if (TotalWhitePieces==0 && TotalBlackPawns &&
      !(BlackPawns&not_rook_pawns)) {
    if (BlackBishops) {
      if (BlackBishops&dark_squares) {
        if (file_mask[FILEA]&BlackPawns) return(0);
      }
      else if (file_mask[FILEH]&BlackPawns) return(0);
    }
    else if (TotalBlackPieces==0) {
      if (file_mask[FILEA]&BlackPawns &&
          file_mask[FILEH]&BlackPawns) return(0);
    }
    else return(0);

    if (!(BlackPawns&file_mask[FILEA]) ||
        !(BlackPawns&file_mask[FILEH])) {
      square=FirstOne(BlackPawns);
      if (Rank(WhiteKingSQ)<=Rank(square) &&
          FileDistance(WhiteKingSQ,square)<=2) return(1);
      return(0);
    }
  }
