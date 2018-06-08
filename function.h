#if !defined(FUNCTION_INCLUDED)
#  define FUNCTION_INCLUDED
#  if !defined(CRAY1)
    BITBOARD     Mask(int);
    int          PopCnt(BITBOARD);
    int          FirstOne(BITBOARD);
    int          LastOne(BITBOARD);
#  endif
  
  void           Annotate(void);
  int            Attacked(int, int);
  BITBOARD       AttacksFrom(int, int);
  BITBOARD       AttacksTo(int);
  int            Book(int);
  int            BookMask(char*);
  void           BookUp(char*);
  int            BookUpCompare(const void *, const void *);
  BOOK_POSITION  BookUpNextPosition(int,int);
  int            CheckInput(void);
  void           ComputeAttacksAndMobility(void);
  void           DisplayBitBoard(BITBOARD);
  void           DisplayChessBoard(FILE*, CHESS_POSITION);
  char*          DisplayEvaluation(int);
  char*          DisplayEvaluationWhisper(int);
  char*          DisplayHHMM(unsigned int);
  void           DisplayPieceBoards(int*, int*);
  char*          DisplayTime(unsigned int);
  char*          DisplayTimeWhisper(unsigned int);
  void           Display2BitBoards(BITBOARD, BITBOARD);
  void           DisplayChessMove(char*, int);
  int            DrawScore(void);
  int            Drawn(int);
  void           Edit(void);
  int            EnPrise(int, int);
  int            Evaluate(int, int, int, int);
  int            EvaluateDevelopment(int);
  int            EvaluateDraws();
  int            EvaluateMate(void);
  int            EvaluateOutsidePassedPawns(void);
  int            EvaluatePassedPawns(void);
  int            EvaluatePassedPawnRaces(int);
  int            EvaluatePawns(void);
  int*           GenerateCaptures(int, int, int*);
  int*           GenerateCheckEvasions(int, int, int*);
  int*           GenerateMoves(int, int, int, BITBOARD, int, int*);
  unsigned int   GetTime(TIME_TYPE);
  int            HasOpposition(int, int, int);
  void           HistoryBest(int, int, int);
  void           HistoryRefutation(int, int, int);
  void           Initialize(int);
  void           InitializeAttackBoards(void);
  void           InitializeChessBoard(SEARCH_POSITION*);
  int            InitializeFindAttacks(int, int, int);
  void           InitializeHashTables(void);
  void           InitializeMasks(void);
  void           InitializePawnMasks(void);
  void           InitializePieceMasks(void);
  void           InitializeRandomHash(void);
  void           InitializeZeroMasks(void);
  int            InputMove(char*, int, int, int, int);
  int            InputMoveICS(char*, int, int, int, int);
  BITBOARD       InterposeSquares(int, int, int);
  void           Interrupt(int);
  int            Iterate(int, int);
  int            KingPawnSquare(int, int, int, int);
  int            LookUp(int, int, int, int*, int, int);
  void           MakeMove(int, int, int);
  void           MakeMovePawn(int, int, int, int, int, int);
  void           MakeMoveKnight(int, int, int);
  void           MakeMoveBishop(int, int, int);
  void           MakeMoveRook(int, int, int, int);
  void           MakeMoveQueen(int, int, int);
  void           MakeMoveKing(int, int, int, int);
  void           MakeMoveRoot(int, int);
  int            NextEvasion(int, int);
  int            NextMove(int, int, int);
  char*          Normal(void);
  int            Option(char*);
  void           OptionGet(char**, char**, char*, int*);
  int            OptionMatch(char*, char*);
  char*          OptionNext(int, char*);
  void           OptionPerft(int,int,int);
  char*          OutputMove(int*, int, int);
  char*          OutputMoveICS(int*);
  int            OutputGood(char*, int, int);
  int            ParseTime(char*);
  void           Phase(void);
  int            PinnedOnKing(int,int);
  int            Ponder(int);
  void           PreEvaluate(int);
  void           Print(int, char*, ...);
  int            Quiesce(int, int, int, int);
  unsigned int   Random32(void);
  BITBOARD       Random64(void);
  int            ReadChessMove(FILE*, int);
  int            RepetitionCheck(int,int);
  int            RepetitionDraw(int);
  void           ResignOrDraw(int,int);
  char*          Reverse(void);
  void           RootMoveList(int);
  int            Search(int, int, int, int, int, int);
  void           SearchOutput(int,int);
  int            SearchRoot(int, int, int, int, int);
  void           SearchTrace(int,int,int,int,int,char*,int);
  void           SetBoard(void);
  void           SetChessBitBoards(SEARCH_POSITION*);
  void           StoreBest(int,int,int,int,int);
  void           StorePV(int,int);
  void           StoreRefutation(int,int,int,int);
  int            Swap(int, int, int);
  BITBOARD       SwapXray(BITBOARD, int, int);
  void           Test(void);
  void           TimeAdjust(int,PLAYER);
  int            TimeCheck(int);
  void           TimeSet(int);
  int            TtoI(char*);
  void           UnMakeMove(int, int, int);
  void           UnMakeMovePawn(int, int, int, int, int, int);
  void           UnMakeMoveKnight(int, int, int);
  void           UnMakeMoveBishop(int, int, int);
  void           UnMakeMoveRook(int, int, int);
  void           UnMakeMoveQueen(int, int, int);
  void           UnMakeMoveKing(int, int, int);
  int            ValidMove(int, int, int);
  void           ValidatePosition(int,int,char*);
  BITBOARD       ValidateComputeBishopAttacks(int);
  BITBOARD       ValidateComputeRookAttacks(int);
  void           Whisper(int, int, int, int, unsigned int, int, char*);
  
#  if defined(HAS_64BITS) || defined(HAS_LONGLONG)
#    define And(a,b)    ((a) & (b))
#    define Or(a,b)     ((a) | (b))
#    define Xor(a,b)    ((a) ^ (b))
#    define Compl(a)    (~(a))
#    define Shiftl(a,b) ((a) << (b))
#    define Shiftr(a,b) ((a) >> (b))
#    if defined(CRAY1)
#      define PopCnt(a)     _popcnt(a)
#      define FirstOne(a)   _leadz(a)
#      define LastOne(a)    _leadz((a)^(a-1))
#      define Mask(a)       _mask(a)
#      define mask_1        _mask(1)
#      define mask_2        _mask(2)
#      define mask_3        _mask(3)
#      define mask_4        _mask(4)
#      define mask_8        _mask(8)
#      define mask_16       _mask(16)
#      define mask_32       _mask(32)
#      define mask_72       _mask(72)
#      define mask_80       _mask(80)
#      define mask_85       _mask(85)
#      define mask_96       _mask(96)
#      define mask_107      _mask(107)
#      define mask_108      _mask(108)
#      define mask_112      _mask(112)
#      define mask_118      _mask(118)
#      define mask_120      _mask(120)
#      define mask_121      _mask(121)
#      define mask_127      _mask(127)
#    endif
#  endif

#  define Max(a,b)  (((a) > (b)) ? (a) : (b))
#  define Min(a,b)  (((a) < (b)) ? (a) : (b))
#  define FileDistance(a,b) abs(((a)&7) - ((b)&7))
#  define RankDistance(a,b) abs(((a)>>3) - ((b)>>3))
#  define Distance(a,b) Max(FileDistance(a,b),RankDistance(a,b))

/*  
    the following macro is used to determine if one side is in check.  it
    simply returns the result of Attacked().
*/
#  define Check(wtm)                                                     \
    Attacked((wtm)?WhiteKingSQ:BlackKingSQ,ChangeSide(wtm))
/*  
    Attack() is used to determine if a newly promoted pawn (queen)
    attacks <square>.  normally <square> will be the location of the opposing
    king, but it can also be the location of the opposing side's queening
    square in case this pawn prevents the other pawn from safely queening on
    the next move.
*/
#  define Attack(square,queen,ply) !And(obstructed[square][queen],Occupied)
/*  
    the following macros are used to construct the attacks from a square.
    the attacks are computed as four separate bit vectors, one for each of the
    two diagonals, and one for the ranks and one for the files.  these can be
    Or'ed together to produce the attack bitmaps for bishops, rooks and queens.
*/
#  if defined(COMPACT_ATTACKS) && defined(USE_ATTACK_FUNCTIONS)
     extern BITBOARD AttacksRookFunc(int, CHESS_POSITION *);
     extern BITBOARD AttacksBishopFunc(DIAG_INFO *, CHESS_POSITION *);
#    define AttacksRook(a) AttacksRookFunc(a,&search)
#    define AttacksBishop(a) AttacksBishopFunc(&diag_info[a],&search)
#  else
#    define AttacksRook(a)    Or(AttacksRank(a),AttacksFile(a))
#    define AttacksBishop(a)  Or(AttacksDiaga1(a),AttacksDiagh1(a))
#  endif

#  define AttacksQueen(a)   Or(AttacksBishop(a),AttacksRook(a))
#  define Rank(x)       (((x)>>3)&7)
#  define File(x)       ((x)&7)
#  define ChangeSide(x) ((x)^1)

#  if defined(COMPACT_ATTACKS)
/*
  On a 32 bit machine optimizes the right shift of a long long
  where it is known that desired piece lies completely in one of
  the 32 bit words.
*/
#  if defined(USE_SPLIT_SHIFTS)
#    define SplitShiftr(value,shift) ((shift) >= 32 ?      \
       Shiftr ((unsigned long) Shiftr((value), 32), (shift)-32) : \
       Shiftr ((unsigned long) (value), (shift)))
#  else
#    define SplitShiftr(value,shift) Shiftr(value,shift)
#  endif

#  define AttacksDiaga1Int(diagp,boardp) \
     (diagp)->ad_attacks[(diagp)->ad_which_attack[ \
      And (SplitShiftr((boardp)->occupied_rl45,(diagp)->ad_shift),\
      (diagp)->ad_mask)]]

#  define AttacksDiagh1Int(diagp,boardp)\
     (diagp)->d_attacks[(diagp)->d_which_attack[ \
      And (SplitShiftr((boardp)->occupied_rr45,(diagp)->d_shift), \
      (diagp)->d_mask)]]

/*
  On a 32 bit machine optimizes promoting a smaller value to a long
  long where it is known that the smaller piece will be completely
  in one of the 32 bit words.
*/
#  if defined(USE_SPLIT_SHIFTS)
#    define SplitShiftl(value,shift)     \
       ((shift) >= 32 ? Shiftl((BITBOARD)     \
        Shiftl((unsigned long) (value), (shift)-32), 32) :  \
       (BITBOARD) Shiftl((unsigned long) (value), (shift)))
#  else
#    define SplitShiftl(value,shift) Shiftl((BITBOARD) value,shift)
#  endif

/*
  The length of the following macro is a little excessive as the
  rank_attacks lookup is duplicated.  Making it a function, or passing
  in a temporary variable would simplify it significantly, although
  hopefully the compiler recognizes the common subexpression.
*/
#  define AttacksRankInt(a,boardp)  \
     SplitShiftl(at.rank_attack_bitboards[File(a)][ \
     at.which_attack[File(a)][And(SplitShiftr(   \
     Or((boardp)->w_occupied,(boardp)->b_occupied), \
     (Rank(~(a))<<3)+1),0x3f)]],Rank(~(a))<<3)

/* 
  The final left shift in this is optimizable, but the optimization is
  a little ugly to express.  There is no information that crosses the
  word boundary in the shift so it can be implemented as two separate
  word shifts that are joined together in a long long.
*/
#  define AttacksFileInt(a,boardp)   \
     Shiftl(at.file_attack_bitboards[Rank(a)] [  \
     at.which_attack[Rank(a)] [   \
     And(SplitShiftr((boardp)->occupied_rl90, \
     (File(~(a))<<3)+1),0x3f)]],File(~(a)) )

# if defined(USE_ATTACK_FUNCTIONS)
   extern BITBOARD AttacksRankFunc(int, CHESS_POSITION *);
   extern BITBOARD AttacksFileFunc(int, CHESS_POSITION *);
   extern BITBOARD AttacksDiaga1Func(DIAG_INFO *, CHESS_POSITION *);
   extern BITBOARD AttacksDiagh1Func(DIAG_INFO *, CHESS_POSITION *);

#  define AttacksRank(a) AttacksRankFunc(a,&search)
#  define AttacksFile(a) AttacksFileFunc(a,&search)
#  define AttacksDiaga1(a) AttacksDiaga1Func(&diag_info[a],&search)
#  define AttacksDiagh1(a) AttacksDiagh1Func(&diag_info[a],&search)
# else
#   define AttacksRank(a) AttacksRankInt(a,&search)
#   define AttacksFile(a) AttacksFileInt(a,&search)
#   define AttacksDiaga1(a) AttacksDiaga1Int(&diag_info[a],&search)
#   define AttacksDiagh1(a) AttacksDiagh1Int(&diag_info[a],&search)
# endif

#else

#  define AttacksRank(a)                                               \
      rook_attacks_r0[(a)][And(Shiftr(Or(search.w_occupied,search.b_occupied),\
                                      56-((a)&56)),255)]
#  define AttacksFile(a)                                               \
      rook_attacks_rl90[(a)][And(Shiftr(search.occupied_rl90,   \
                                        56-(((a)&7)<<3)),255)]
#  define AttacksDiaga1(a)                                             \
      bishop_attacks_rl45[(a)][And(Shiftr(search.occupied_rl45, \
                                          bishop_shift_rl45[(a)]),255)]
#  define AttacksDiagh1(a)                                             \
      bishop_attacks_rr45[(a)][And(Shiftr(search.occupied_rr45, \
                                          bishop_shift_rr45[(a)]),255)]
#endif
/*  
    the following macros are used to compute the mobility for a sliding piece.
    The basic idea is the same as the attack vectors above, but the result is 
    an integer mobility factor rather than a bitboard.  this saves having to 
    do a PopCnt() on the attack bit vector, which is much slower.
*/
#  define MobilityRook(a)   (MobilityRank(a)+MobilityFile(a))

#  define MobilityBishop(a) (MobilityDiaga1(a)+MobilityDiagh1(a))

#  define MobilityQueen(a)  (MobilityBishop(a)+MobilityRook(a))

#if defined(COMPACT_ATTACKS)

#  define MobilityDiaga1Int(diagp,boardp)  \
     (diagp)->ad_mobility[(diagp)->ad_which_attack[\
     And (SplitShiftr ((boardp)->occupied_rl45, \
     (diagp)->ad_shift),(diagp)->ad_mask)]]

#  define MobilityDiagh1Int(diagp,boardp)  \
     (diagp)->d_mobility[(diagp)->d_which_attack [    \
     And (SplitShiftr ((boardp)->occupied_rr45, \
     (diagp)->d_shift),(diagp)->d_mask)]]

#  define MobilityRankInt(a,boardp)   \
     at.length8_mobility[File(a)][   \
     at.which_attack[File(a)][   \
     And(SplitShiftr(Or((boardp)->w_occupied, \
     (boardp)->b_occupied),(Rank(~(a))<<3)+1),0x3f)]]

#  define MobilityFileInt(a,boardp)   \
     at.length8_mobility[Rank(a)][   \
     at.which_attack[Rank(a)] [   \
     And(SplitShiftr((boardp)->occupied_rl90,(File(~(a))<<3)+1),0x3f)]]

# if defined(USE_ATTACK_FUNCTIONS)
     extern unsigned MobilityRankFunc(int, CHESS_POSITION *);
     extern unsigned MobilityFileFunc(int, CHESS_POSITION *);
     extern unsigned MobilityDiaga1Func(DIAG_INFO *, CHESS_POSITION *);
     extern unsigned MobilityDiagh1Func(DIAG_INFO *, CHESS_POSITION *);

#    define MobilityRank(a) MobilityRankFunc(a,&search)
#    define MobilityFile(a) MobilityFileFunc(a,&search)
#    define MobilityDiaga1(a) MobilityDiaga1Func(&diag_info[a],&search)
#    define MobilityDiagh1(a) MobilityDiagh1Func(&diag_info[a],&search)
#  else
#    define MobilityRank(a) MobilityRankInt(a,&search)
#    define MobilityFile(a) MobilityFileInt(a,&search)
#    define MobilityDiaga1(a) MobilityDiaga1Int(&diag_info[a],&search)
#    define MobilityDiagh1(a) MobilityDiagh1Int(&diag_info[a],&search)
#  endif

#else
#  define MobilityRank(a)                                                   \
     rook_mobility_r0[(a)][And(Shiftr(Or(search.w_occupied,search.b_occupied),            \
                                      56-((a)&56)),255)]
#  define MobilityFile(a)                                                   \
     rook_mobility_rl90[(a)][And(Shiftr(search.occupied_rl90,   \
                                        56-(((a)&7)<<3)),255)]
#  define MobilityDiaga1(a)                                                 \
     bishop_mobility_rl45[(a)][And(Shiftr(search.occupied_rl45, \
                                          bishop_shift_rl45[(a)]),255)]
#  define MobilityDiagh1(a)                                                 \
     bishop_mobility_rr45[(a)][And(Shiftr(search.occupied_rr45, \
                                          bishop_shift_rr45[(a)]),255)]
#endif

/*  
    the following macros are used to extract the pieces of a move that are
    kept compressed into the rightmost 21 bits of a simple integer.
*/

#  define From(a)             ((a)&63)
#  define To(a)               (((a)>>6)&63)
#  define Piece(a)            (((a)>>12)&7)
#  define Captured(a)         (((a)>>15)&7)
#  define Promote(a)          (((a)>>18)&7)
#  define CaptureOrPromote(a) (((a)>>15)&63)
#  define SetMask(a)          (set_mask[a])
#  define ClearMask(a)        (clear_mask[a])

/*  
    the following macros are used to extract the correct bits for the piece
    type desired.
*/

#  define BlackPawns            search.b_pawn
#  define BlackKnights          search.b_knight
#  define BlackBishops          search.b_bishop
#  define BlackRooks            search.b_rook
#  define BlackQueens           search.b_queen
#  define BlackKing             set_mask[search.black_king]
#  define BlackKingSQ           search.black_king
#  define BlackCastle(ply)      position[ply].b_castle
#  define TotalBlackPawns       search.black_pawns
#  define TotalBlackPieces      search.black_pieces
#  define BlackPieces           search.b_occupied

#  define WhitePawns            search.w_pawn
#  define WhiteKnights          search.w_knight
#  define WhiteBishops          search.w_bishop
#  define WhiteRooks            search.w_rook
#  define WhiteQueens           search.w_queen
#  define WhiteKing             set_mask[search.white_king]
#  define WhiteKingSQ           search.white_king
#  define WhiteCastle(ply)      position[ply].w_castle
#  define TotalWhitePawns       search.white_pawns
#  define TotalWhitePieces      search.white_pieces
#  define WhitePieces           search.w_occupied

#  define Material              search.material_evaluation
#  define Rule50Moves(ply)      position[ply].rule_50_moves
#  define HashKey               search.hash_key
#  define PawnHashKey           search.pawn_hash_key
#  define EnPassant(ply)        position[ply].enpassant_target
#  define EnPassantTarget(ply)  (EnPassant(ply) ? set_mask[EnPassant(ply)] : 0)
#  define PieceOnSquare(sq)     search.board[sq]
#  define BishopsQueens         search.bishops_queens
#  define RooksQueens           search.rooks_queens
#  define Occupied              Or(search.w_occupied,search.b_occupied)
#  define OccupiedRL90          search.occupied_rl90
#  define OccupiedRL45          search.occupied_rl45
#  define OccupiedRR45          search.occupied_rr45

#  define Sliding(piece)        ((piece) & 4)
#  define SlidingDiag(piece)    (((piece) & 5) == 5)
#  define SlidingRow(piece)     (((piece) & 6) == 6)
/*  
    the following macros are used to Set and Clear a specific bit in the
    second argument.  this is done to make the code more readable, rather
    than to make it faster.
*/
#  define ClearSet(a,b)  b=Xor(a,b)
#  define Clear(a,b)     b=And(clear_mask[a],b)
#  define ClearRL90(a,b) b=And(clear_mask_rl90[a],b)
#  define ClearRL45(a,b) b=And(clear_mask_rl45[a],b)
#  define ClearRR45(a,b) b=And(clear_mask_rr45[a],b)
#  define Set(a,b)       b=Or(set_mask[a],b)
#  define SetRL90(a,b)   b=Or(set_mask_rl90[a],b)
#  define SetRL45(a,b)   b=Or(set_mask_rl45[a],b)
#  define SetRR45(a,b)   b=Or(set_mask_rr45[a],b)

#  define HashPB(a,b)         b=Xor(b_pawn_random[a],b)
#  define HashPW(a,b)         b=Xor(w_pawn_random[a],b)
#  define HashNB(a,b)         b=Xor(b_knight_random[a],b)
#  define HashNW(a,b)         b=Xor(w_knight_random[a],b)
#  define HashBB(a,b)         b=Xor(b_bishop_random[a],b)
#  define HashBW(a,b)         b=Xor(w_bishop_random[a],b)
#  define HashRB(a,b)         b=Xor(b_rook_random[a],b)
#  define HashRW(a,b)         b=Xor(w_rook_random[a],b)
#  define HashQB(a,b)         b=Xor(b_queen_random[a],b)
#  define HashQW(a,b)         b=Xor(w_queen_random[a],b)
#  define HashKB(a,b)         b=Xor(b_king_random[a],b)
#  define HashKW(a,b)         b=Xor(w_king_random[a],b)
#  define HashEP(a,b)         b=Xor(enpassant_random[a],b)
#  define HashCastleW(a,b)    b=Xor(castle_random_w[a],b);
#  define HashCastleB(a,b)    b=Xor(castle_random_b[a],b);
#  define SavePV(ply,value,ph) do {\
      for (i=1;i<ply+1;i++) pv[ply].path[i]=current_move[i];\
      pv[ply].path_length=ply; \
      pv[ply].path_hashed=ph; \
      pv[ply].path_iteration_depth=iteration_depth;} while(0)
#  define SavePVS(ply,value,ph) do {\
      for (i=1;i<ply+1;i++) pv[ply].path[i]=current_move[i];\
      pv[ply].path_length=ply; \
      pv[ply].path_hashed=ph; \
      pv[ply].path_iteration_depth=iteration_depth; \
      SearchOutput(value,beta);} while(0)

#  define FutileAhead(wtm) ((wtm) ? Material >= ROOK_VALUE : \
                                    Material <= -ROOK_VALUE)
#  define FutileBehind(wtm) ((wtm) ? Material <= -ROOK_VALUE : \
                                     Material >= ROOK_VALUE)
#endif
