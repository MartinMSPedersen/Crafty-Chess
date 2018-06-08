#if !defined(FUNCTION_INCLUDED)
  #define FUNCTION_INCLUDED
  #if !defined(HAS_64BITS)
    BITBOARD     Mask(int);
    int          Popcnt(BITBOARD);
    int          Popcntl(BITBOARD);
    int          First_One(BITBOARD);
    int          Last_One(BITBOARD);
  #endif
  
  int            Attack(int,int,int);
  int            Attacked(int, int, int);
  BITBOARD       Attacks_From(int, int, int);
  BITBOARD       Attacks_To(int, int);
  int            Book_Mask(char*);
  int            Book(int,int);
  void           Book_Up(char*);
  int            Book_Up_Compare(const void *, const void *);
  BOOK_POSITION  Book_Up_Next_Position(int files);
  int            Check_Input(void);
  void           Display_Bit_Board(BITBOARD);
  void           Display_Chess_Board(FILE*, CHESS_BOARD);
  char*          Display_Evaluation(int);
  void           Display_Piece_Boards(int*, int*);
  char*          Display_Time(unsigned int);
  void           Display_2_Bit_Boards(BITBOARD, BITBOARD);
  void           Display_Chess_Move(char*, int);
  int            Draw_Score(void);
  int            Drawn(int);
  void           Edit(void);
  int            EnPrise(int, int, int);
  int            Evaluate(int, int, int, int);
  int            Evaluate_Development(int);
  int            Evaluate_Draws(int, int);
  int            Evaluate_King_Safety_W(int,int);
  int            Evaluate_King_Safety_B(int,int);
  int            Evaluate_Mate(int);
  int            Evaluate_Outside_Passed_Pawns(int, int, int);
  int            Evaluate_Passed_Pawns(int, int, int);
  int            Evaluate_Passed_Pawn_Races(int, int, int, int);
  int            Evaluate_Pawns(int, int*, int*);
  int            Evaluate_Tempo(int);
  int            Evaluate_Trades(int);
  int*           Generate_Check_Evasions(int, int, BITBOARD, int,
                                         int, int, int*);
  int*           Generate_Moves(int, int, int, BITBOARD, int, int*);
  unsigned int   Get_Time(TIME_TYPE);
  int            Give_Check(int, int, int*);
  int            Has_Opposition(int, int, int);
  void           History_Best(int, int, int);
  void           History_Refutation(int, int, int);
  void           Initialize(int);
  void           Initialize_Attack_Boards(void);
  void           Initialize_Chess_Board(CHESS_POSITION*);
  int            Initialize_Find_Attacks(int, int, int);
  void           Initialize_Hash_Tables(void);
  void           Initialize_Masks(void);
  void           Initialize_Pawn_Masks(void);
  void           Initialize_Piece_Masks(void);
  void           Initialize_Random_Hash(void);
  int            Input_Move(char*, int, int, int);
  int            Input_Move_ICS(char*, int, int, int);
  void           Interrupt(int);
  int            Iterate(int);
  int            King_Pawn_Square(int, int, int, int);
  int            Lookup(int, int, int, int*, int, int);
  void           Make_Move(int, int, int);
  void           Make_Move_Pawn(int, int, int, int, int, int);
  void           Make_Move_Knight(int, int, int, int);
  void           Make_Move_Bishop(int, int, int, int);
  void           Make_Move_Rook(int, int, int, int);
  void           Make_Move_Queen(int, int, int, int);
  void           Make_Move_King(int, int, int, int);
  void           Make_Move_Root(int, int);
  int            Next_Capture(int, int, int);
  int            Next_Evasion(int, int);
  BITBOARD       Next_Evasion_Interpose_Squares(int, int, int, int, int, int);
  int            Next_Move(int, int, int);
  char*          Normal(void);
  int            Option(char*);
  int            Option_Match(char*, char*);
  void           Option_Get(char**, char**, char*, char*, int*);
  char*          Output_Move(int*, int, int);
  char*          Output_Move_ICS(int*);
  int            Output_Good(char*, int, int);
  void           Phase(void);
  int            Ponder(int);
  void           Pre_Evaluate(int,int);
  void           Print(int, char*, ...);
  int            Quiesce(int, int, int, int, int);
  int            Quiesce_Full(int, int, int, int, int);
  unsigned int   Random32(void);
  BITBOARD       Random64(void);
  int            Repetition_Check(int);
  int            Repetition_Draw(void);
  void           Resign_or_Draw(int);
  char*          Reverse(void);
  void           Root_Move_List(int);
  int            Search(int, int, int, int, int, int, NODES);
  void           Search_Output(int,int,int);
  void           Set_Board(void);
  void           Set_Chess_Bit_Boards(CHESS_POSITION*);
  void           Store_Best(int,int,int,int,int);
  void           Store_Refutation(int,int,int,int);
  int            Swap(int, int, int, int);
  BITBOARD       Swap_Xray(int, BITBOARD, int, int);
  void           Test(void);
  void           Time_Adjust(int);
  int            Time_Check(void);
  void           Time_Set(void);
  int            TtoI(char*);
  int            Valid_Move(int, int, int);
  void           Validate_Hash_Key(int);
  void           Validate_Position(int);
  
  #if !defined(HAS_64BITS)
    void         Initialize_Zero_Masks(void);
    void         Initialize_Population_Count(void);
  #endif
  
  #if defined(HAS_64BITS) || defined(HAS_LONGLONG)
    #define And(a,b)    ((a) & (b))
    #define Or(a,b)     ((a) | (b))
    #define Xor(a,b)    ((a) ^ (b))
    #define Compl(a)    (~(a))
    #define Shiftl(a,b) ((a) << (b))
    #define Shiftr(a,b) ((a) >> (b))
    #if defined(HAS_64BITS)
      #define Popcnt(a)     _popcnt(a)
      #define Popcntl(a)    _popcnt(a)
      #define First_One(a)  _leadz(a)
      #define Last_One(a)   _leadz((a)^(a-1))
      #define Mask(a)       _mask(a)
      #define mask_1        _mask(1)
      #define mask_2        _mask(2)
      #define mask_3        _mask(3)
      #define mask_4        _mask(4)
      #define mask_8        _mask(8)
      #define mask_32       _mask(32)
      #define mask_80       _mask(80)
      #define mask_96       _mask(96)
      #define mask_107      _mask(107)
      #define mask_108      _mask(108)
      #define mask_112      _mask(112)
      #define mask_118      _mask(118)
      #define mask_120      _mask(120)
      #define mask_121      _mask(121)
      #define mask_127      _mask(127)
    #endif
  #endif

  #define Max(a,b)  (((a) > (b)) ? (a) : (b))
  #define Min(a,b)  (((a) < (b)) ? (a) : (b))
  #define File_Distance(a,b) abs(((a)&7) - ((b)&7))
  #define Rank_Distance(a,b) abs(((a)>>3) - ((b)>>3))
  #define Distance(a,b) Max(File_Distance(a,b),Rank_Distance(a,b))

/*  
    the following macro is used to determine if one side is in check.  it
    simply returns the result of Attacked().
*/
#define Check(ply,wtm)                                                       \
    Attacked((wtm) ? position[ply].board.white_king :                        \
                     position[ply].board.black_king,ply,!wtm)
/*  
    the following macros are used to construct the attacks from a square.
    the attacks are computed as four separate bit vectors, one for each of the
    two diagonals, and one for the ranks and one for the files.  these can be
    Or'ed together to produce the attack bitmaps for bishops, rooks and queens.
*/
  #define Attacks_Rook(a)                                                    \
      Or(Attacks_Rank(a),Attacks_File(a))

  #define Attacks_Bishop(a)                                                  \
      Or(Attacks_Diaga1(a),Attacks_Diagh1(a))

  #define Attacks_Queen(a)                                                   \
      Or(Attacks_Bishop(a),Attacks_Rook(a))

  #define Attacks_Rank(a)                                                    \
      rook_attacks_r0[(a)][And(Shiftr(Or(position[ply].board.w_occupied,     \
                                         position[ply].board.b_occupied),    \
                                      56-((a)&56)),255)]
  #define Attacks_File(a)                                                    \
      rook_attacks_rl90[(a)][And(Shiftr(position[ply].board.occupied_rl90,   \
                                        56-(((a)&7)<<3)),255)]
  #define Attacks_Diaga1(a)                                                  \
      bishop_attacks_rl45[(a)][And(Shiftr(position[ply].board.occupied_rl45, \
                                          bishop_shift_rl45[(a)]),255)]
  #define Attacks_Diagh1(a)                                                  \
      bishop_attacks_rr45[(a)][And(Shiftr(position[ply].board.occupied_rr45, \
                                          bishop_shift_rr45[(a)]),255)]
/*  
    the following macros are used to compute the mobility for a sliding piece.
    The basic idea is the same as the attack vectors above, but the result is 
    an integer mobility factor rather than a bitboard.  this saves having to 
    do a Popcnt() on the attack bit vector, which is much slower.
*/
  #define Mobility_Rook(a)                                                   \
      Mobility_Rank(a)+Mobility_File(a)

  #define Mobility_Bishop(a)                                                 \
      Mobility_Diaga1(a)+Mobility_Diagh1(a)

  #define Mobility_Queen(a)                                                  \
      Mobility_Bishop(a)+Mobility_Rook(a)

  #define Mobility_Rank(a)                                                   \
     rook_mobility_r0[(a)][And(Shiftr(Or(position[ply].board.w_occupied,     \
                                         position[ply].board.b_occupied),    \
                                      56-((a)&56)),255)]
  #define Mobility_File(a)                                                   \
     rook_mobility_rl90[(a)][And(Shiftr(position[ply].board.occupied_rl90,   \
                                        56-(((a)&7)<<3)),255)]
  #define Mobility_Diaga1(a)                                                 \
     bishop_mobility_rl45[(a)][And(Shiftr(position[ply].board.occupied_rl45, \
                                          bishop_shift_rl45[(a)]),255)]
  #define Mobility_Diagh1(a)                                                 \
     bishop_mobility_rr45[(a)][And(Shiftr(position[ply].board.occupied_rr45, \
                                          bishop_shift_rr45[(a)]),255)]
/*  
    the following macros are used to extract the pieces of a move that are
    kept compressed into the rightmost 21 bits of a simple integer.
*/
  #define From(a)           ((a)&63)
  #define To(a)             (((a)>>6)&63)
  #define Piece(a)   (((a)>>12)&7)
  #define Captured(a) (((a)>>15)&7)
  #define Promote(a)     (((a)>>18)&7)
/*  
    the following macros are used to extract the correct bits for the piece
    type desired.
*/
  #define Black_Pawns(ply) position[ply].board.b_pawn
  #define Black_Knights(ply) position[ply].board.b_knight
  #define Black_Bishops(ply) position[ply].board.b_bishop
  #define Black_Rooks(ply) position[ply].board.b_rook
  #define Black_Queens(ply) position[ply].board.b_queen
  #define Black_King(ply) position[ply].board.b_king
  #define Black_King_SQ(ply) position[ply].board.black_king
  #define Black_Castle(ply) position[ply].board.b_castle
  #define Total_Black_Pawns(ply) position[ply].board.black_pawns
  #define Total_Black_Pieces(ply) position[ply].board.black_pieces
  #define Black_Pieces(ply) position[ply].board.b_occupied

  #define White_Pawns(ply) position[ply].board.w_pawn
  #define White_Knights(ply) position[ply].board.w_knight
  #define White_Bishops(ply) position[ply].board.w_bishop
  #define White_Rooks(ply) position[ply].board.w_rook
  #define White_Queens(ply) position[ply].board.w_queen
  #define White_King(ply) position[ply].board.w_king
  #define White_King_SQ(ply) position[ply].board.white_king
  #define White_Castle(ply) position[ply].board.w_castle
  #define Total_White_Pawns(ply) position[ply].board.white_pawns
  #define Total_White_Pieces(ply) position[ply].board.white_pieces
  #define White_Pieces(ply) position[ply].board.w_occupied

  #define Material(ply) position[ply].board.material_evaluation
  #define Hash_Key(ply) position[ply].board.hash_key
  #define Pawn_Hash_Key(ply) position[ply].board.pawn_hash_key
  #define EnPassant_Target(ply) position[ply].board.enpassant_target
  #define Piece_On_Square(ply,sq) position[ply].board.board[sq]
  #define Bishops_Queens(ply) position[ply].board.bishops_queens
  #define Rooks_Queens(ply) position[ply].board.rooks_queens
  #define Occupied(ply) Or(position[ply].board.w_occupied,\
                           position[ply].board.b_occupied)
  #define Occupied_RL90(ply) position[ply].board.occupied_rl90
  #define Occupied_RL45(ply) position[ply].board.occupied_rl45
  #define Occupied_RR45(ply) position[ply].board.occupied_rr45
/*  
    the following macros are used to Set and Clear a specific bit in the
    second argument.  this is done to make the code more readable, rather
    than to make it faster.
*/
  #define Clear_Set(a,b)  b=Xor(a,b)
  #define Clear(a,b)      b=And(clear_mask[a],b)
  #define Clear_rl90(a,b) b=And(clear_mask_rl90[a],b)
  #define Clear_rl45(a,b) b=And(clear_mask_rl45[a],b)
  #define Clear_rr45(a,b) b=And(clear_mask_rr45[a],b)
  #define Set(a,b)        b=Or(set_mask[a],b)
  #define Set_rl90(a,b)   b=Or(set_mask_rl90[a],b)
  #define Set_rl45(a,b)   b=Or(set_mask_rl45[a],b)
  #define Set_rr45(a,b)   b=Or(set_mask_rr45[a],b)

  #define Hash_Pb(a,b)    b=Xor(b_pawn_random[a],b)
  #define Hash_Pw(a,b)    b=Xor(w_pawn_random[a],b)
  #define Hash_Nb(a,b)    b=Xor(b_knight_random[a],b)
  #define Hash_Nw(a,b)    b=Xor(w_knight_random[a],b)
  #define Hash_Bb(a,b)    b=Xor(b_bishop_random[a],b)
  #define Hash_Bw(a,b)    b=Xor(w_bishop_random[a],b)
  #define Hash_Rb(a,b)    b=Xor(b_rook_random[a],b)
  #define Hash_Rw(a,b)    b=Xor(w_rook_random[a],b)
  #define Hash_Qb(a,b)    b=Xor(b_queen_random[a],b)
  #define Hash_Qw(a,b)    b=Xor(w_queen_random[a],b)
  #define Hash_Kb(a,b)    b=Xor(b_king_random[a],b)
  #define Hash_Kw(a,b)    b=Xor(w_king_random[a],b)
#endif
