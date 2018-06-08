alignment = 4

        .text
        .align  alignment, 0x90
        .globl  _PopCnt
_PopCnt:
        movl    4(%esp), %eax
        movl    8(%esp), %edx
        pushl   %ebx
        pushl   %esi
# x = (x >> 1  & 0x55555555) + (x & 0x55555555);
        movl    $0x55555555, %esi
        movl    %eax, %ecx
        shrl    $1, %eax
        movl    %edx, %ebx
        shrl    $1, %edx
        andl    %esi, %ecx
        andl    %esi, %ebx
        andl    %esi, %eax
        andl    %esi, %edx
        addl    %ecx, %eax
        addl    %ebx, %edx
# x = ((x >> 2) & 0x33333333) + (x & 0x33333333);
        movl    $0x33333333, %esi
        movl    %eax, %ecx
        shrl    $2, %eax
        movl    %edx, %ebx
        shrl    $2, %edx
        andl    %esi, %ecx
        andl    %esi, %ebx
        andl    %esi, %eax
        andl    %esi, %edx
        addl    %ecx, %eax
        addl    %ebx, %edx
# x = ((x >> 4) + x) & 0x0f0f0f0f;
        movl    %eax, %ecx
        shrl    $4, %eax
        movl    %edx, %ebx
        shrl    $4, %edx
        addl    %ecx, %eax
        addl    %ebx, %edx
        andl    $0x0f0f0f0f, %eax
        andl    $0x0f0f0f0f, %edx
# x = ((x >> 8) + x);
        movl    %eax, %ecx
        shrl    $8, %eax
        movl    %edx, %ebx
        shrl    $8, %edx
        addl    %ecx, %eax
        addl    %ebx, %edx
# return (x + (x >> 16)) & 0xff;
        movl    %eax, %ecx
        shrl    $16, %eax
        movl    %edx, %ebx
        shrl    $16, %edx
        addl    %ecx, %eax
        addl    %ebx, %edx
        popl    %esi
        popl    %ebx
        addl    %edx, %eax
        andl    $0x7f, %eax
        ret

/*----------------------------------------------------------------------------*/

        .align  alignment, 0x90
        .globl  _FirstOne
_FirstOne:
        cmpl    $1, 8(%esp)
        sbbl    %eax, %eax
        movl    8(%esp,%eax,4), %edx
        bsr     %edx, %ecx
        jz      l2
        andl    $32, %eax
        subl    $31, %ecx
        subl    %ecx, %eax
        ret
l2:     movl    $64, %eax
        ret

        .globl  _LastOne

_LastOne:
        bsf     4(%esp),%edx
        jz      l3
        movl    $63, %eax
        subl    %edx, %eax
        ret
l3:     bsf     8(%esp), %edx
        jz      l4
        movl    $31, %eax
        subl    %edx, %eax
        ret
l4:     mov     $64, %eax
        ret

/*----------------------------------------------------------------------------*/

        .align  alignment, 0x90

/*----------------------------------------------------------------------------*/

        .comm   _first_ones_8bit, 256
        .comm   _last_ones_8bit, 256

/*----------------------------------------------------------------------------*/

/* Diag info offsets */
    
D_ATTACKS       = 0             # Must be 0 - hard-coded (see comments)
D_MOBILITY      = 4
D_WHICH_ATTACK  = 8
D_SHIFT         = 12
D_MASK          = 13
AD_SHIFT        = 14
AD_MASK         = 15
AD_WHICH_ATTACK = 16
AD_MOBILITY     = 20
AD_ATTACKS      = 24

/* Position offsets */
    
W_OCCUPIED       =  0
B_OCCUPIED       =  8
RL90             = 16
RL45             = 24
RR45             = 32
    
/* Struct at offsets:
        struct at {
          unsigned char which_attack[8][64];
          BITBOARD      file_attack_bitboards[8][12];
          unsigned char rank_attack_bitboards[8][12];
          unsigned char length8_mobility[8][12];
          unsigned char short_mobility[116];
        } at;
*/

WHICH_ATTACK     = 0
FILE_ATTACKS     = 512
RANK_ATTACKS     = 1280
LEN8_MOBILITY    = 1376
SHRT_MOBILITY    = 1472

        .text

/*
  BITBOARD AttacksDiaga1Func (DIAG_INFO *diag, CHESS_POSITION *board)
*/
/*
  #define AttacksDiaga1Int(diagp,boardp)                \
    (diagp)->ad_attacks [               \
      (diagp)->ad_which_attack [            \
    And (SplitShiftr((boardp)->occupied_rl45,   \
             (diagp)->ad_shift),        \
         (diagp)->ad_mask) ] ]
*/
        .align  alignment, 0x90
        .globl  _AttacksDiaga1Func
_AttacksDiaga1Func:
        pushl   %esi
        movl    8(%esp), %esi                   /* diag_info     */
        movl    12(%esp), %eax                  /* boardp        */
        movb    AD_SHIFT(%esi), %cl             /* shift         */
        cmpb    $32, %cl
        sbbl    %edx, %edx
        movl    RL45+4(%eax,%edx,4), %eax       /* occupied      */
        movzbl  AD_MASK(%esi), %edx             /* mask          */
        shrl    %cl, %eax
        andl    %edx, %eax
        addl    AD_WHICH_ATTACK(%esi), %eax     /* which_attack  */
        movl    AD_ATTACKS(%esi), %ecx          /* attack_vector */
        movzbl  (%eax), %edx                    /* attack_index  */
        popl    %esi
        movl    (%ecx,%edx,8), %eax
        movl    4(%ecx,%edx,8), %edx
        ret

/*
  BITBOARD AttacksDiagh1Func(DIAG_INFO *diag, CHESS_POSITION *board)
*/
/*
  #define AttacksDiagh1Int(diagp,boardp)        \
    (diagp)->d_attacks [                \
      (diagp)->d_which_attack [             \
    And (SplitShiftr((boardp)->occupied_rr45,   \
             (diagp)->d_shift),     \
         (diagp)->d_mask) ] ]
*/
        .align  alignment, 0x90
        .globl  _AttacksDiagh1Func
_AttacksDiagh1Func:
        pushl   %esi
        movl    8(%esp), %esi                   /* diag_info     */
        movl    12(%esp), %eax                  /* boardp        */
        movb    D_SHIFT(%esi), %cl              /* shift         */
        cmpb    $32, %cl
        sbbl    %edx, %edx
        movl    RR45+4(%eax,%edx,4), %eax       /* occupied      */
        movzbl  D_MASK(%esi), %edx              /* mask          */
        shrl    %cl, %eax
        andl    %edx, %eax
        addl    D_WHICH_ATTACK(%esi), %eax      /* which_attack  */
        movl    (%esi), %ecx                    /* D_ATTACKS     */
                                                # attack_vector
        movzbl  (%eax), %edx                    /* attack_index  */
        popl    %esi
        movl    (%ecx,%edx,8), %eax
        movl    4(%ecx,%edx,8), %edx
        ret

/*
  BITBOARD AttacksBishopFunc(DIAG_INFO *diag, CHESS_POSITION *board)
*/
/*
  return Or(AttacksDiaga1Int(diag,board), AttacksDiagh1Int(diag,board));
*/
        .align  alignment, 0x90
        .globl  _AttacksBishopFunc
_AttacksBishopFunc:
        pushl   %ebx
        pushl   %esi
        pushl   %edi
        movl    16(%esp), %esi                  /* diag_info     */
        movl    20(%esp), %edi                  /* boardp        */
        movb    AD_SHIFT(%esi), %cl             /* shift         */
        cmpb    $32, %cl
        sbbl    %edx, %edx
        movzbl  AD_MASK(%esi), %ebx             /* mask          */
        movl    RL45+4(%edi,%edx,4), %edi       /* occupied      */
        shrl    %cl, %edi
        andl    %ebx, %edi
        addl    AD_WHICH_ATTACK(%esi), %edi     /* which_attack  */
        movl    AD_ATTACKS(%esi), %ecx          /* attack_vector */
        movzbl  (%edi), %ebx                    /* attack_index  */
        movl    20(%esp), %edi                  /* again boardp  */
        leal    (%ecx,%ebx,8), %edx
        movb    D_SHIFT(%esi), %cl              /* shift         */
        cmpb    $32, %cl
        sbbl    %ebx, %ebx
        movl    RR45+4(%edi,%ebx,4), %edi       /* occupied      */
        movzbl  D_MASK(%esi), %ebx              /* mask          */
        shrl    %cl, %edi
        movl    (%esi), %ecx                    /* D_ATTACKS     */
                                                # attack_vector
        andl    %ebx, %edi
        movl    (%edx), %eax
        addl    D_WHICH_ATTACK(%esi), %edi      /* which_attack  */
        movl    4(%edx), %edx
        movzbl  (%edi), %ebx                    /* attack_index  */
        popl    %edi
        orl     (%ecx,%ebx,8), %eax
        popl    %esi
        orl     4(%ecx,%ebx,8), %edx
        popl    %ebx
        ret

/*
  unsigned MobilityDiaga1Func(DIAG_INFO *diag, CHESS_POSITION *board)
*/
/*
  #define MobilityDiaga1Int(diagp,boardp)               \
    (diagp)->ad_mobility [              \
      (diagp)->ad_which_attack [            \
    And (SplitShiftr ((boardp)->occupied_rl45,  \
              (diagp)->ad_shift),       \
             (diagp)->ad_mask) ] ]
*/

        .align  alignment, 0x90
        .globl  _MobilityDiaga1Func
_MobilityDiaga1Func:
        pushl   %esi
        movl    8(%esp), %esi                   /* diag_info     */
        movl    12(%esp), %eax                  /* boardp        */
        movb    AD_SHIFT(%esi), %cl             /* shift         */
        cmpb    $32, %cl
        sbbl    %edx, %edx
        movl    RL45+4(%eax,%edx,4), %eax       /* occupied      */
        movzbl  AD_MASK(%esi), %edx             /* mask          */
        shrl    %cl, %eax
        andl    %edx, %eax
        addl    AD_WHICH_ATTACK(%esi), %eax     /* which_attack  */
        movl    AD_MOBILITY(%esi), %ecx         /* mobility_vector */
        movzbl  (%eax), %edx                    /* attack_index  */
        popl    %esi
        movzbl  (%ecx,%edx,1), %eax
        ret

/*
  unsigned MobilityDiagh1Func(DIAG_INFO *diag, CHESS_POSITION *board)
*/
/*
  #define MobilityDiagh1Int(diagp,boardp)       \
    (diagp)->d_mobility [               \
      (diagp)->d_which_attack [             \
    And (SplitShiftr ((boardp)->occupied_rr45,  \
              (diagp)->d_shift),        \
         (diagp)->d_mask) ] ]
*/

        .align  alignment, 0x90
        .globl  _MobilityDiagh1Func
_MobilityDiagh1Func:
        pushl   %esi
        movl    8(%esp), %esi                   /* diag_info     */
        movl    12(%esp), %eax                  /* boardp        */
        movb    D_SHIFT(%esi), %cl              /* shift         */
        cmpb    $32, %cl
        sbbl    %edx, %edx
        movl    RR45+4(%eax,%edx,4), %eax       /* occupied      */
        movzbl  D_MASK(%esi), %edx              /* mask          */
        shrl    %cl, %eax
        andl    %edx, %eax
        addl    D_WHICH_ATTACK(%esi), %eax      /* which_attack  */
        movl    D_MOBILITY(%esi), %ecx          /* mobility_vector */
        movzbl  (%eax), %edx                    /* attack_index  */
        popl    %esi
        movzbl  (%ecx,%edx,1), %eax
        ret

/*
  BITBOARD AttacksRankFunc(int square, CHESS_POSITION *board)
*/
/*
  #define AttacksRankInt(a,boardp)              \
    SplitShiftl(                \
      at.rank_attack_bitboards[File(a)] [   \
    at.which_attack[File(a)] [      \
      And(SplitShiftr(          \
        Or((boardp)->w_occupied,    \
           (boardp)->b_occupied),   \
        (Rank(~(a))<<3)+1),     \
          0x3f) ] ],            \
      Rank(~(a))<<3)
*/
        .align  alignment, 0x90
        .globl  _AttacksRankFunc
_AttacksRankFunc:
        movl    4(%esp), %ecx                   /* square               */
        movl    8(%esp), %edx                   /* boardp               */
        pushl   %esi
        movl    %ecx, %esi
        andl    $7, %esi                        /* file                 */
        notl    %ecx
        andl    $0x38, %ecx
        pushl   %ebp
        cmpl    $32, %ecx
        pushl   %edi
        sbbl    %ebp, %ebp
        leal    (%esi,%esi,2), %edi             /* file * 3             */
        shll    $6, %esi                        /* file * 64            */
        movl    W_OCCUPIED+4(%edx,%ebp,4), %eax
        incl    %ecx
        orl     B_OCCUPIED+4(%edx,%ebp,4), %eax
        shrl    %cl, %eax
        andl    $0x3f, %eax
        movzbl  _at+WHICH_ATTACK(%eax,%esi), %edx
        decl    %ecx
        movzbl  _at+RANK_ATTACKS(%edx,%edi,4), %eax
        shll    %cl, %eax
        popl    %edi
        movl    %eax, %edx
        andl    %ebp, %eax
        notl    %ebp
        andl    %ebp, %edx
        popl    %ebp
        popl    %esi
        ret

/*
  BITBOARD AttacksFileFunc(int square, CHESS_POSITION *board)
*/
/*
  #define AttacksFileInt(a,boardp)                      \
    Shiftl(at.file_attack_bitboards[Rank(a)] [      \
         at.which_attack[Rank(a)] [         \
           And(SplitShiftr((boardp)->occupied_rl90, \
                   (File(~(a))<<3)+1),  \
           0x3f) ] ],               \
       File(~(a)) )
*/
        .align  alignment, 0x90
        .globl  _AttacksFileFunc
_AttacksFileFunc:
        movl    4(%esp), %ecx                   /* square               */
        movl    8(%esp), %edx                   /* boardp               */
        pushl   %esi
        movl    %ecx, %esi
        notl    %ecx
        andl    $7, %ecx                        /* file                 */
        shrl    $3, %esi                        /* rank                 */
        pushl   %edi
        leal    (%esi,%esi,2), %edi             /* rank * 3             */
        shll    $6, %esi                        /* rank * 64            */
        leal    1(,%ecx,8), %ecx                /* (file << 3) + 1      */
        cmpl    $32, %ecx
        sbbl    %eax, %eax
        shll    $5, %edi
        movl    RL90+4(%edx,%eax,4), %eax
        shrl    %cl, %eax
        andl    $0x3f, %eax
        movzbl  _at+WHICH_ATTACK(%eax,%esi), %edx
        decl    %ecx
        movl    _at+FILE_ATTACKS(%edi,%edx,8), %eax
        shrl    $3, %ecx
        movl    _at+FILE_ATTACKS+4(%edi,%edx,8), %edx
        popl    %edi
        shll    %cl, %eax
        popl    %esi
        shll    %cl, %edx
        ret

/*
  BITBOARD AttacksRookFunc(int square, CHESS_POSITION *board)
*/
/*
  return Or(AttacksRankInt(square, board), AttacksFileInt(square, board));
*/
        .align  alignment, 0x90
        .globl  _AttacksRookFunc
_AttacksRookFunc:
        movl    4(%esp), %ecx                   /* square               */
        movl    8(%esp), %edx                   /* boardp               */
        pushl   %ebp
        pushl   %esi
        movl    %ecx, %esi
        notl    %ecx
        andl    $7, %esi                        /* file                 */
        pushl   %ebx
        andl    $0x38, %ecx                     /* rank << 3            */
        pushl   %edi
        cmpl    $32, %ecx
        leal    (%esi,%esi,2), %edi             /* file * 3             */
        sbbl    %eax, %eax
        shll    $6, %esi                        /* file * 64            */
        movl    W_OCCUPIED+4(%edx,%eax,4), %ebp
        incl    %ecx
        orl     B_OCCUPIED+4(%edx,%eax,4), %ebp
        shrl    %cl, %ebp
        andl    $0x3f, %ebp
        movzbl  _at+WHICH_ATTACK(%esi,%ebp,1), %ebx
        decl    %ecx
        movl    20(%esp), %esi                  /* square               */
        movzbl  _at+RANK_ATTACKS(%ebx,%edi,4), %ebp
        shll    %cl, %ebp
        movl    %esi, %ecx
        movl    %ebp, %ebx
        notl    %ecx
        andl    %eax, %ebp
        andl    $7, %ecx                        /* file                 */
        notl    %eax
        shrl    $3, %esi                        /* rank                 */
        andl    %eax, %ebx
# Now we have: ebp - lo, ebx - hi
        leal    1(,%ecx,8), %ecx                /* (file << 3) + 1      */
        leal    (%esi,%esi,2), %edi             /* rank * 3             */
        shll    $6, %esi                        /* rank * 64            */
        cmpl    $32, %ecx
        sbbl    %eax, %eax
        shll    $5, %edi
        movl    RL90+4(%edx,%eax,4), %eax
        shrl    %cl, %eax
        andl    $0x3f, %eax
        movzbl  _at+WHICH_ATTACK(%eax,%esi), %edx
        decl    %ecx
        movl    _at+FILE_ATTACKS(%edi,%edx,8), %eax
        shrl    $3, %ecx
        shll    %cl, %eax
        movl    _at+FILE_ATTACKS+4(%edi,%edx,8), %edx
        shll    %cl, %edx
        popl    %edi
        orl     %ebx, %edx
        popl    %ebx
        orl     %ebp, %eax
        popl    %esi
        popl    %ebp
        ret

/*
  unsigned MobilityRankFunc(int square, CHESS_POSITION *board)
*/
/*
  #define MobilityRankInt(a,boardp)                     \
    at.length8_mobility[File(a)][           \
      at.which_attack[File(a)] [            \
    And(SplitShiftr(Or((boardp)->w_occupied,    \
               (boardp)->b_occupied),   \
            (Rank(~(a))<<3)+1),     \
        0x3f) ] ]
*/
        .align  alignment, 0x90
        .globl  _MobilityRankFunc
_MobilityRankFunc:
        movl    4(%esp), %ecx                   /* square               */
        pushl   %esi
        movl    %ecx, %esi
        notl    %ecx
        andl    $0x38, %ecx
        cmpl    $32, %ecx
        sbbl    %edx, %edx
        shll    $2, %edx
        addl    12(%esp), %edx                  /* boardp               */
        andl    $7, %esi                        /* file                 */
        movl    W_OCCUPIED+4(%edx), %eax
        incl    %ecx
        orl     B_OCCUPIED+4(%edx), %eax
        shrl    %cl, %eax
        leal    (%esi,%esi,2), %ecx             /* file * 3             */
        shll    $6, %esi                        /* file * 64            */
        andl    $0x3f, %eax
        movzbl  _at+WHICH_ATTACK(%eax,%esi), %edx
        popl    %esi
        movzbl  _at+LEN8_MOBILITY(%edx,%ecx,4), %eax
        ret

/*
  unsigned MobilityFileFunc(int square, CHESS_POSITION *board)
*/
/*
  #define MobilityFileInt(a,boardp)                     \
    at.length8_mobility[Rank(a)][           \
      at.which_attack[Rank(a)] [            \
    And(SplitShiftr((boardp)->occupied_rl90,    \
            (File(~(a))<<3)+1 ),        \
        0x3f) ] ]
*/
        .align  alignment, 0x90
        .globl  _MobilityFileFunc
_MobilityFileFunc:
        movl    4(%esp), %ecx                   /* square               */
        pushl   %esi
        movl    %ecx, %esi
        notl    %ecx
        andl    $7, %ecx                        /* file                 */
        shrl    $3, %esi                        /* rank                 */
        leal    1(,%ecx,8), %ecx                /* (file << 3) + 1      */
        cmpl    $32, %ecx
        sbbl    %edx, %edx
        shll    $2, %edx
        addl    12(%esp), %edx                  /* boardp               */
        movl    RL90+4(%edx), %eax
        shrl    %cl, %eax
        leal    (%esi,%esi,2), %ecx             /* rank * 3             */
        shll    $6, %esi                        /* rank * 64            */
        andl    $0x3f, %eax
        movzbl  _at+WHICH_ATTACK(%eax,%esi), %edx
        popl    %esi
        movzbl  _at+LEN8_MOBILITY(%edx,%ecx,4), %eax
        ret

        .align  alignment, 0x90
        .end
