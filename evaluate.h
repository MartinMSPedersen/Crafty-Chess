#if !defined(EVALUATE_INCLUDED)
  #define    EVALUATE_INCLUDED
 
  #define                                 DRAW           0

  #define                           PAWN_VALUE        1000
  #define                         KNIGHT_VALUE        3200
  #define                         BISHOP_VALUE        3200
  #define                           ROOK_VALUE        5000
  #define                          QUEEN_VALUE        9000
  #define                           KING_VALUE      100000

  #define                             PAWN_RAM          50
  #define                        PAWN_ISOLATED         100
  #define                   PAWN_ISOLATED_WEAK          50
  #define                         PAWN_WEAK_P1          50
  #define                         PAWN_WEAK_P2         100
  #define                    PAWN_VERY_WEAK_P1         100
  #define                    PAWN_VERY_WEAK_P2         150
  #define                          PAWN_PASSED         100
  #define                 PAWN_ISOLATED_PASSED          60
  #define           PAWN_PROTECTED_PASSER_WINS         300
 
  #define                       PAWN_ADVANCE_A         -20
  #define                       PAWN_ADVANCE_B         -10
  #define                       PAWN_ADVANCE_C           0
  #define                       PAWN_ADVANCE_D          10
  #define                       PAWN_ADVANCE_E          10
  #define                       PAWN_ADVANCE_F         -10
  #define                       PAWN_ADVANCE_G         -20
  #define                       PAWN_ADVANCE_H         -10
 
  #define                    PAWN_ADVANCE_BC_A         -20
  #define                    PAWN_ADVANCE_BC_B         -20
  #define                    PAWN_ADVANCE_BC_C          10
  #define                    PAWN_ADVANCE_BC_D          10
  #define                    PAWN_ADVANCE_BC_E          10
  #define                    PAWN_ADVANCE_BC_F         -20
  #define                    PAWN_ADVANCE_BC_G         -50
  #define                    PAWN_ADVANCE_BC_H         -50
  
  #define                    PAWN_ADVANCE_EG_A           0
  #define                    PAWN_ADVANCE_EG_B           0
  #define                    PAWN_ADVANCE_EG_C           0
  #define                    PAWN_ADVANCE_EG_D           0
  #define                    PAWN_ADVANCE_EG_E           0
  #define                    PAWN_ADVANCE_EG_F           0
  #define                    PAWN_ADVANCE_EG_G           0
  #define                    PAWN_ADVANCE_EG_H           0

  #define          PAWN_CONNECTED_PASSED_RANK2         100
  #define          PAWN_CONNECTED_PASSED_RANK3         100
  #define          PAWN_CONNECTED_PASSED_RANK4         200
  #define          PAWN_CONNECTED_PASSED_RANK5         500
  #define          PAWN_CONNECTED_PASSED_RANK6         800
  #define          PAWN_CONNECTED_PASSED_RANK7        1000
  
  #define          PAWN_SUPPORTED_PASSED_RANK2           0
  #define          PAWN_SUPPORTED_PASSED_RANK3           0
  #define          PAWN_SUPPORTED_PASSED_RANK4           0
  #define          PAWN_SUPPORTED_PASSED_RANK5         200
  #define          PAWN_SUPPORTED_PASSED_RANK6         500
  #define          PAWN_SUPPORTED_PASSED_RANK7        1000

  #define                         KING_TROPISM           4
  
  #define                           KNIGHT_C_0          20
  #define                           KNIGHT_C_1          10
  #define                           KNIGHT_C_2           0
  #define                           KNIGHT_C_3         -50
  #define                       KNIGHT_OUTPOST          75
  
  #define                       BISHOP_TRAPPED        1500
  #define                      BISHOP_MOBILITY           4
  #define                          BISHOP_PAIR         300
  #define                           BISHOP_C_0          20
  #define                           BISHOP_C_1          10
  #define                           BISHOP_C_2          00
  #define                           BISHOP_C_3         -10
  
  #define                        ROOK_MOBILITY           4
  #define                   ROOK_POORLY_PLACED         100
  #define                          ROOK_ON_7TH         200
  #define              ROOK_CONNECTED_7TH_RANK         100
  #define                       ROOK_OPEN_FILE         100
  #define             ROOK_CONNECTED_OPEN_FILE          50
  #define                  ROOK_HALF_OPEN_FILE          50
  #define              ROOK_BEHIND_PASSED_PAWN         100
  
  #define                       QUEEN_MOBILITY           4
  #define                            QUEEN_C_0          20
  #define                            QUEEN_C_1          10
  #define                            QUEEN_C_2           0
  #define                            QUEEN_C_3         -10
  #define                      QUEEN_IS_STRONG         200
  #define               QUEEN_ROOK_ON_7TH_RANK         200
 
  #define              KING_SAFETY_GOOD_BISHOP         200
  #define                          KING_SAFETY         -25
  #define                    ACCEPTABLE_FAULTS           5
 
  #define                KING_SAFETY_IN_CENTER           5
  #define                KING_SAFETY_OPEN_FILE           6
  #define                       KING_BACK_RANK         100
  #define               KING_SAFETY_NOT_CORNER           2
  #define                KING_SAFETY_IN_CORNER           3
  #define              KING_SAFETY_PAWN_ATTACK          10
 
  #define                  KING_SAFETY_RP_ADV1           3
  #define                  KING_SAFETY_RP_ADV2           6
  #define               KING_SAFETY_RP_MISSING           8
  #define               KING_SAFETY_RP_TOO_FAR           8
  #define             KING_SAFETY_RP_FILE_OPEN          10
 
  #define                  KING_SAFETY_NP_ADV1           2
  #define                  KING_SAFETY_NP_ADV2           5
  #define               KING_SAFETY_NP_MISSING           8
  #define               KING_SAFETY_NP_TOO_FAR           8
  #define             KING_SAFETY_NP_FILE_OPEN          10
 
  #define                  KING_SAFETY_BP_ADV1           1
  #define                  KING_SAFETY_BP_ADV2           2
  #define               KING_SAFETY_BP_MISSING           2
  #define               KING_SAFETY_BP_TOO_FAR           2
 
  #define                             KING_C_0          80
  #define                             KING_C_1          60
  #define                             KING_C_2          40
  #define                             KING_C_3           0
  #define                          KING_ATTACK          10
  
  #define                 DEVELOPMENT_THEMATIC        -100
  #define             DEVELOPMENT_BLOCKED_PAWN        -100
  #define           DEVELOPMENT_UNMOVED_PIECES         -50
  #define            DEVELOPMENT_UNMOVED_PAWNS         -50
  #define              DEVELOPMENT_QUEEN_EARLY         -50
  #define            DEVELOPMENT_LOSING_CASTLE        -300
  #define              DEVELOPMENT_NOT_CASTLED        -150
  #define             DEVELOPMENT_WASTED_TEMPO         -50
  #define                  DEVELOPMENT_CRAMPED         -50
 
  #define                         TRADE_PIECES          10
  #define                          TRADE_PAWNS          30

#endif
