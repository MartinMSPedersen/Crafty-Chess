#if !defined(EVALUATE_INCLUDED)
#  define    EVALUATE_INCLUDED
 
#  define                                 DRAW           (0)

#  define                            BAD_TRADE         (120)

#  define                       PAWN_UNBLOCKED           (4)
#  define                         PAWN_BLOCKED           (3)
#  define                        PAWN_ISOLATED          (20)
#  define                             PAWN_JAM          (20)
#  define                         PAWN_DOUBLED           (2)
#  define                         PAWN_WEAK_P1          (12)
#  define                         PAWN_WEAK_P2          (16)
#  define                          PAWN_PASSED           (8)
#  define           PAWN_PROTECTED_PASSER_WINS          (11)
#  define                         CENTER_PAWNS           (5)
 
#  define                       PAWN_ADVANCE_A          (-3)
#  define                       PAWN_ADVANCE_B          (-3)
#  define                       PAWN_ADVANCE_C           (1)
#  define                       PAWN_ADVANCE_D           (1)
#  define                       PAWN_ADVANCE_E           (1)
#  define                       PAWN_ADVANCE_F           (1)
#  define                       PAWN_ADVANCE_G          (-3)
#  define                       PAWN_ADVANCE_H          (-3)
 
#  define                    PAWN_ADVANCE_BC_A          (-3)
#  define                    PAWN_ADVANCE_BC_B          (-3)
#  define                    PAWN_ADVANCE_BC_C           (1)
#  define                    PAWN_ADVANCE_BC_D           (1)
#  define                    PAWN_ADVANCE_BC_E           (1)
#  define                    PAWN_ADVANCE_BC_F           (1)
#  define                    PAWN_ADVANCE_BC_G          (-4)
#  define                    PAWN_ADVANCE_BC_H          (-4)

#  define                    PAWN_ADVANCE_KING           (1)
  
#  define                    PAWN_ADVANCE_EG_A           (3)
#  define                    PAWN_ADVANCE_EG_B           (3)
#  define                    PAWN_ADVANCE_EG_C           (3)
#  define                    PAWN_ADVANCE_EG_D           (4)
#  define                    PAWN_ADVANCE_EG_E           (4)
#  define                    PAWN_ADVANCE_EG_F           (3)
#  define                    PAWN_ADVANCE_EG_G           (3)
#  define                    PAWN_ADVANCE_EG_H           (3)

#  define          PAWN_CONNECTED_PASSED_6TH (PAWN_VALUE+20)
  
#  define          PAWN_SUPPORTED_PASSED_RANK2           (0)
#  define          PAWN_SUPPORTED_PASSED_RANK3           (0)
#  define          PAWN_SUPPORTED_PASSED_RANK4           (0)
#  define          PAWN_SUPPORTED_PASSED_RANK5          (12)
#  define          PAWN_SUPPORTED_PASSED_RANK6          (60)
#  define          PAWN_SUPPORTED_PASSED_RANK7         (100)

#  define                  KNIGHT_KING_TROPISM           (3)
#  define                  BISHOP_KING_TROPISM           (3)
#  define                    ROOK_KING_TROPISM           (3)
#  define                   QUEEN_KING_TROPISM           (5)
#  define                    KING_KING_TROPISM          (15)
  
#  define                       KNIGHT_OUTPOST           (2)
  
#  define                       BISHOP_TRAPPED         (150)
#  define                       BISHOP_OUTPOST           (2)
#  define                      BISHOP_MOBILITY           (2)
#  define                          BISHOP_PAIR          (10)
  
#  define                          ROOK_ON_7TH          (20)
#  define                    ROOK_ABSOLUTE_7TH          (25)
#  define              ROOK_CONNECTED_7TH_RANK          (25)
#  define                       ROOK_OPEN_FILE          (12)
#  define                  ROOK_HALF_OPEN_FILE           (6)
#  define             ROOK_CONNECTED_OPEN_FILE           (5)
#  define              ROOK_BEHIND_PASSED_PAWN           (4)
#  define                         ROOK_TRAPPED          (50)
#  define                       ROOK_WEAK_PAWN           (3)
  
#  define               QUEEN_ROOK_ON_7TH_RANK          (20)
 
#  define              KING_SAFETY_GOOD_BISHOP           (2)
#  define                KING_SAFETY_MATE_G2G7           (5)
#  define              KING_SAFETY_MATE_THREAT        (1200)
#  define                KING_SAFETY_OPEN_FILE           (4)
#  define                       KING_BACK_RANK           (4)
#  define          KING_SAFETY_PAWN_ATTACK_4TH           (1)
#  define          KING_SAFETY_PAWN_ATTACK_5TH           (2)
#  define                KING_SAFETY_STONEWALL           (4)
 
#  define                  KING_SAFETY_RP_ADV1           (2)
#  define               KING_SAFETY_RP_TOO_FAR           (4)
#  define        KING_SAFETY_RP_FILE_HALF_OPEN           (4)
 
#  define                  KING_SAFETY_NP_ADV1           (2)
#  define               KING_SAFETY_NP_TOO_FAR           (4)
#  define        KING_SAFETY_NP_FILE_HALF_OPEN           (4)
 
#  define                  KING_SAFETY_BP_ADV1           (1)
#  define               KING_SAFETY_BP_TOO_FAR           (3)
#  define        KING_SAFETY_BP_FILE_HALF_OPEN           (2)

#  define               KING_SAFETY_RP_BP_ADV1           (4)
 
#  define                 DEVELOPMENT_THEMATIC           (6)
#  define                  DEVELOPMENT_UNMOVED          (10)
#  define             DEVELOPMENT_BLOCKED_PAWN          (12)
#  define            DEVELOPMENT_LOSING_CASTLE          (20)
#  define              DEVELOPMENT_NOT_CASTLED          (20)
#  define              DEVELOPMENT_QUEEN_EARLY          (12)
 
#  define                         TRADE_PIECES           (6)
#  define                          TRADE_PAWNS          (10)

#endif
