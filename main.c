#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "chess.h"
#include "data.h"
#if defined(UNIX) || defined(AMIGA)
#  include <unistd.h>
#  include <pwd.h>
#  include <sys/types.h>
#endif
#include <signal.h>

/* last modified 09/29/96 */
/*
*******************************************************************************
*                                                                             *
*  Crafty, copyrighted 1996 by Robert M. Hyatt, Ph.D., Associate Professor    *
*  of Computer and Information Sciences, University of Alabama at Birmingham. *
*                                                                             *
*  All rights reserved.  No part of this program may be reproduced in any     *
*  form or by any means, for any commercial (for profit/sale) reasons.  This  *
*  program may be freely distributed, used, and modified, so long as such use *
*  does not in any way result in the sale of all or any part of the source,   *
*  the executables, or other distributed materials that are a part of this    *
*  package.                                                                   *
*                                                                             *
*  Crafty is the "son" (direct descendent) of Cray Blitz.  it is designed     *
*  totally around the bit-board data structure for reasons of speed of ex-    *
*  ecution, ease of adding new knowledge, and a *significantly* cleaner       *
*  overall design.  it is written totally in ANSI C with some few UNIX system *
*  calls required for I/O, etc.                                               *
*                                                                             *
*  main() is the driver for the chess program.  its primary function is to    *
*  cycle between asking the user for a move and calling for a tree search     *
*  to produce a move for the program.  after accepting an input string from   *
*  the user, this string is first passed to Option() which checks to see if   *
*  it is a command to the program.  if Option() returns a non-zero result,    *
*  the input was a command and was executed, otherwise the input should be    *
*  passed to input() for conversion to the internal move format.              *
*                                                                             *
*  version  description                                                       *
*                                                                             *
*    1.0    first version of the bit-board program to play a legitimate game  *
*           even though significant features are missing:  transposition      *
*           table, sophisticated scoring, etc.                                *
*                                                                             *
*    1.1    added the minimal window search.  at each ply in the tree, the    *
*           first branch is searched with the normal alpha/beta window, while *
*           remaining nodes are searched with value/value+1 which was         *
*           returned from searching the first branch.  If such a search       *
*           returns the upper bound, this position is once again searched     *
*           with the normal window.                                           *
*                                                                             *
*    1.2    added the classic null-move search.  the program searches the     *
*           sub-tree below the null move (typically) one play shallower than  *
*                                                                             *
*    1.3    added the transposition table support.  this also includes the    *
*           code in Iterate() necessary to propagate the principal variation  *
*           from one iteration to the next to improve move ordering.          *
*                                                                             *
*    1.4    modified the transposition table to use three tables, one for 64  *
*           bits of white entry, one for 64 bits of black entry, and one with *
*           the remaining 32 bits of white and black entry combined into one  *
*           64 bit word.  eliminated the "bit fields" since they seemed to be *
*           erratic on most compilers and made portability nearly impossible. *
*                                                                             *
*    1.5    pawn scoring added.  this required three additions to the code:   *
*           (1) InitializePawnMasks() which produces the necessary masks to   *
*           let us efficiently detect isolated, backward, passed, etc. pawns; *
*           (2) a pawn hash table to store recently computed pawn scores so   *
*           it won't be too expensive to do complex analysis;  (3) Evaluate() *
*           now evaluates pawns if the current pawn position is not in the    *
*           pawn hash table.                                                  *
*                                                                             *
*    1.6    piece scoring added, although it is not yet complete.  also, the  *
*           search now uses the familiar zero-width window search for all but *
*           the first move at the root, in order to reduce the size of the    *
*           tree to a nearly optimal size.  this means that a move that is    *
*           only one point better than the first move will "fail high" and    *
*           have to be re-searched a second time to get the true value.  if   *
*           the new best move is significantly better, it may have to be      *
*           searched a third time to increase the window enough to return the *
*           score.                                                            *
*                                                                             *
*    1.7    replaced the old "killer" move ordering heuristic with the newer  *
*           "history" ordering heuristic.   this version uses the 12-bit key  *
*           formed by <from><to> to index into the history table.             *
*                                                                             *
*    1.8    added pondering for both PC and UNIX-based machines.  also other  *
*           improvements include the old Cray Blitz algorithm that takes the  *
*           P.V. returned by the tree search, and "extends" it by looking up  *
*           positions in the transposition table, which improves move         *
*           ordering significantly.  repetitions are now handled correctly.   *
*                                                                             *
*    1.9    added an opening book with flags to control which moves are       *
*           selected for play.  the book maintenance code is stubbed directly *
*           into the program, so that no additional executables are needed.   *
*                                                                             *
*    1.10   added king safety + hashing (as done in the old Cray Blitz).      *
*           nothing novel other than the use of bit-masks to speed up some of *
*           the tests.                                                        *
*                                                                             *
*    1.11   additional evaluation knowledge plus log_file so that program     *
*           saves a record of the game statistics for later analysis.  added  *
*           the "annotate" command to make the program analyze a game or part *
*           of a game for testing/debugging.                                  *
*                                                                             *
*    1.12   added ICS (Internet Chess Server) support for X-Board support so  *
*           that Crafty can play on ICS in an automated manner.  added new    *
*           commands to be compatible with X-Board.  Crafty also has a new    *
*           command-line interface that allows options to be entered on the   *
*           command-line directly, ie:  "crafty alarm=off verbose=2" will     *
*           start the program, set the move alarm off, and set verbose to     *
*           2.  this allows an "alias" to easily customize the program.       *
*                                                                             *
*    1.13   added time-control logic to the program.  it now monitors how     *
*           much time both it and its opponent uses when thinking about a     *
*           move to make.  when pondering, it only times itself from the      *
*           point that a move is actually entered.                            *
*                                                                             *
*    1.14   added the piece/square tables to the program to move static       *
*           scoring to the root of the tree since it never changes (things    *
*           like centralization of pieces, etc.)  also some minor tuning of   *
*           numbers to bring positional score "swings" down some.             *
*                                                                             *
*    1.15   added edit so that positions can be altered (and so ICS games     *
*           can be resumed after they are adjourned).  also added a "force"   *
*           command to force the program to play a different move than the    *
*           search selected.  search timing allocation slightly altered to    *
*           improve ICS performance.                                          *
*                                                                             *
*    1.16   significant adjustments to evaluation weights to bring positional *
*           scores down to below a pawn for "normal" positions.  some small   *
*           "tweaks" to king safety as well to keep the king safer.           *
*                                                                             *
*    1.17   added "development" evaluation routine to encourage development   *
*           and castling before starting on any tactical "sorties."  also     *
*           repaired many "bugs" in evaluation routines.                      *
*                                                                             *
*    1.18   added the famous Cray Blitz "square of the king" passed pawn      *
*           evaluation routine.  this will evaluate positions where one       *
*           side has passed pawns and the other side has no pieces, to see    *
*           if the pawn can outrun the defending king and promote.  note that *
*           this is rediculously fast because it only requires one And()      *
*           to declare that a pawn can't be caught!                           *
*                                                                             *
*    2.0    initial version preparing for search extension additions.  this   *
*           version has a new "next_evasion()" routine that is called when    *
*           the king is in check.  it generates sensible (nearly all are      *
*           legal) moves which include capturing the checking piece (if there *
*           is only one), moving the king (but not along the checking ray),   *
*           and interpositions that block the checking ray (if there is only  *
*           one checking piece.                                               *
*                                                                             *
*    2.1    search is now broken down into three cleanly-defined phases:      *
*           (1) basic full-width search [Search()]; (2) extended tactical     *
*           search [extend()] which includes winning/even captures and then   *
*           passed pawn pushes to the 6th or 7th rank (if the pawn pushes     *
*           are safe);  (3) normal quiescence search [Quiesce()] which only   *
*           includes captures that appear to gain material.                   *
*                                                                             *
*    2.2    king safety code re-written and cleaned up.  Crafty was giving    *
*           multiple penalties which was causing the king safety score to be  *
*           extremely large (and unstable.)  now, if it finds something wrong *
*           with king safety, it only penalizes a weakness once.              *
*                                                                             *
*    2.3    king safety code modified further, penalties were significant     *
*           enough to cause search anomolies.  modified rook scoring to avoid *
*           having the rook trapped in the corner when the king is forced to  *
*           move rather than castle, and to keep the rook in a position to be *
*           able to reach open files in one move, which avoids getting them   *
*           into awkward positions where they become almost worthless.        *
*                                                                             *
*    2.4    king safety code completely rewritten.  it now analyzes "defects" *
*           in the king safety field and counts them as appropriate.  these   *
*           defects are then summed up and used as an index into a vector of  *
*           of values that turns them into a score in a non-linear fashion.   *
*           increasing defects quickly "ramp up" the score to about 1/3+ of   *
*           a pawn, then additional faults slowly increase the penalty.       *
*                                                                             *
*    2.5    first check extensions added.  in the extension search (stage     *
*           between full-width and captures-only, up to two checking moves    *
*           can be included, if there are checks in the full-width part of    *
*           the search.  if only one check occurs in the full-width, then     *
*           only one check will be included in the extension phase of the     *
*           selective search.                                                 *
*                                                                             *
*    2.6    evaluation modifications attempting to cure Crafty's frantic      *
*           effort to develop all its pieces without giving a lot of regard   *
*           to the resulting position until after the pieces are out.         *
*                                                                             *
*    2.7    new evaluation code to handle the "outside passed pawn" concept.  *
*           Crafty now understands that a passed pawn on the side away from   *
*           the rest of the pawns is a winning advantage due to decoying the  *
*           king away from the pawns to prevent the passer from promoting.    *
*           the advantage increases as material is removed from the board.    *
*                                                                             *
*    3.0    the 3.* series of versions will primarily be performance en-      *
*           hancements.  the first version (3.0) has a highly modified        *
*           version of MakeMove() that tries to do no unnecessary work.  it   *
*           is about 25% faster than old version of MakeMove() which makes    *
*           Crafty roughly 10% faster.  also calls to Mask() have been        *
*           replaced by constants (which are replaced by calls to Mask() on   *
*           the Crays for speed) to eliminate function calls.                 *
*                                                                             *
*    3.1    significantly modified king safety again, to better detect and    *
*           react to king-side attacks.  Crafty now uses the recursive null-  *
*           move search to depth-2 instead of depth-1, which results in       *
*           slightly improved speed.                                          *
*                                                                             *
*    3.2    null-move restored to depth-1.  depth-2 proved unsafe as Crafty   *
*           was overlooking tactical moves, particularly against itself,      *
*           which lost several "won" games.                                   *
*                                                                             *
*    3.3    additional king-safety work.  Crafty now uses the king-safety     *
*           evaluation routines to compare king safety for both sides.  if    *
*           one side is "in distress" pieces are attracted to that king in    *
*           "big hurry" to either attack or defend.                           *
*                                                                             *
*    3.4    "threat extensions" added.  simply, this is a null-move search    *
*           used to determine if a move is good only because it is a horizon  *
*           effect type move.  we do a null move search after a move fails    *
*           high anywhere in the tree.  the window is normally lowered by 1.5 *
*           pawns, the idea being that if the fail-high move happens in a     *
*           position that fails "really low" with a null move, then this move *
*           might be a horizon move.  to test this, re-search this move with  *
*           the depth increased by one.                                       *
*                                                                             *
*    3.5    50-move rule implemented.  a count of moves since the last pawn   *
*           move or capture is kept as part of the position[] structure.  it  *
*           is updated by MakeMove().  when this number reaches 100 (which    *
*           in plies is 50 moves) RepetitionCheck() will return the draw      *
*           score immediately, just as though the position was a repetition   *
*           draw.                                                             *
*                                                                             *
*    3.6    search extensions cleaned up to avoid excessive extensions which  *
*           produced some wild variations, but which was also slowing things  *
*           down excessively.                                                 *
*                                                                             *
*    3.7    endgame strategy added.  two specifics: KBN vs K has a piece/sq   *
*           table that will drive the losing king to the correct corner.  for *
*           positions with no pawns, scoring is altered to drive the losing   *
*           king to the edge and corner for mating purposes.                  *
*                                                                             *
*    3.8    hashing strategy modified.  Crafty now stores search value or     *
*           bound, *and* positional evaluation in the transposition table.    *
*           this avoids about 10-15% of the "long" evaluations during the     *
*           middlegame, and avoids >75% of them in endgames.                  *
*                                                                             *
*    4.0    evaluation units changed to "millipawns" where a pawn is now      *
*           1000 rather than the prior 100 units.  this provides more         *
*           "resolution" in the evaluation and will let Crafty have large     *
*           positional bonuses for significant things, without having the     *
*           small positional advantages add up and cause problems.  V3.8      *
*           exhibited a propensity to "sac the exchange" frequently because   *
*           of this.  it is hoped that this is a thing of the past now.       *
*           also, the "one legal reply to check" algorithm is now being used. *
*           in short, if one side has only one legal reply to a checking move *
*           then the other side is free to check again on the next ply. this  *
*           only applies in the "extension" search, and allows some checks    *
*           that extend() would normally not follow.                          *
*                                                                             *
*    4.1    extension search modified.  it now "tracks" the basic iteration   *
*           search depth up to some user-set limit (default=6).  for shallow  *
*           depths, this speeds things up, while at deeper depths it lets the *
*           program analyze forcing moves somewhat deeper.                    *
*                                                                             *
*    4.2    extension search modified.  fixed a problem where successive      *
*           passed-pawn pushes were not extended correctly by extend() code.  *
*           threat-extension margin was wrong after changing the value of a   *
*           pawn to 1000, it is now back to 1.5 pawns.                        *
*                                                                             *
*    4.3    hash scoring "repaired."  as the piece/square tables changed, the *
*           transposition table (positional evaluation component) along with  *
*           the pawn and king-safety hash tables prevented the changes from   *
*           affecting the score, since the hashed scores were based on the    *
*           old piece/square table values.  now, when any piece/square table  *
*           is modified, the affected hash tables are cleared fo evaluation   *
*           information.                                                      *
*                                                                             *
*    4.4    piece/square tables simplified, king tropism scoring moved to     *
*           Evaluate() where it is computed dynamically now as it should be.  *
*                                                                             *
*    4.5    book move selection algorithm replaced.  Crafty now counts the    *
*           number of times each book move was played as the book file is     *
*           created.  since these moves come (typically) from GM games, the   *
*           more frequently a move appears, the more likely it is to lead to  *
*           a sound position.  Crafty then enumerates all possible book moves *
*           for the current position, computes a probability distribution for *
*           each move so that Crafty will select a move proportional to the   *
*           number of times it was played in GM games (for example, if e4 was *
*           played 55% of the time, then Crafty will play e4 55% of the time) *
*           although the special case of moves played too infrequently is     *
*           handled by letting the operator set a minimum threshold (say 5)   *
*           Crafty won't play moves that are not popular (or are outright     *
*           blunders as the case may be.)  pushing a passed pawn to the 7th   *
*           rank in the basic search [Search() module] now extends the search *
*           by two plies unless we have extended this ply already (usually a  *
*           check evasion) or unless we have extended the whole line to more  *
*           than twice the nominal iteration depth.                           *
*                                                                             *
*    5.0    selective search extensions removed.  with the extensions now in  *
*           the basic full-width search, these became more a waste of time    *
*           than anything useful, and removing them simplifies things quite   *
*           a bit.                                                            *
*                                                                             *
*    5.1    pondering logic now has a "puzzling" feature that is used when    *
*           the search has no move to ponder.  it does a short search to find *
*           a move and then ponders that move, permanently eliminating the    *
*           "idle" time when it has nothing to ponder.                        *
*                                                                             *
*    5.2    evaluation terms scaled down to prevent positional scores from    *
*           reaching the point that sacrificing material to avoid simple      *
*           positional difficulties begins to look attractive.                *
*                                                                             *
*    5.3    performance improvement produced by avoiding calls to MakeMove()  *
*           when the attack information is not needed.  since the first test  *
*           done in Quiesce() is to see if the material score is so bad that  *
*           a normal evaluation and/or further search is futile, this test    *
*           has been moved to inside the loop *before* Quiesce() is           *
*           recursively called, avoiding the MakeMove() work only to          *
*           discover that the resulting position won't be searched further.   *
*                                                                             *
*    5.4    new Swap() function that now understands indirect attacks through *
*           the primary attacking piece(s).  this corrects a lot of sloppy    *
*           move ordering as well as make the "futility" cutoffs added in     *
*           in version 5.3 much more reliable.                                *
*                                                                             *
*    5.5    checks are now back in the quiescence search.  Crafty now stores  *
*           a negative "draft" in the transposition table rather than forcing *
*           any depth < 0 to be zero.  the null-move search now searches the  *
*           null-move to depth-1 (R=1) rather than depth-2 (R=2) which seemed *
*           to cause some tactical oversights in the search.                  *
*                                                                             *
*    5.6    improved move ordering by using the old "killer move" idea.  an   *
*           additional advantage is that the killers can be tried before      *
*           generating any moves (after the captures.)  quiescence now only   *
*           includes "safe" checks (using Swap() to determine if it is a safe *
*           checking move.                                                    *
*                                                                             *
*    5.7    king safety now "hates" a pawn at b3/g3 (white) or b6/g6 (black)  *
*           to try and avoid the resulting mate threats.                      *
*                                                                             *
*    5.8    EvaluateOutsidePassedPawns() fixed to correctly evaluate those    *
*           positions where both sides have outside passed pawns.  before     *
*           this fix, it only evaluated positions where one side had a passed *
*           pawn, which overlooked those won/lost positions where both sides  *
*           have passed pawns but one is "distant" or "outside" the other.    *
*                                                                             *
*    5.9    removed "futility" forward pruning.  exhaustive testing has       *
*           proved that this causes significant tactical oversights.          *
*                                                                             *
*    5.10   added code to handle king and pawn vs king, which understands     *
*           the cases where the pawn can't outrun the king, but has to be     *
*           assisted along by the king.                                       *
*                                                                             *
*    5.11   two king-safety changes.  (1) the program's king safety score is  *
*           now "doubled" to make it much less likely that it will try to win *
*           a pawn but destroy it's king-side.  (2) the attack threshold has  *
*           been lowered which is used to key the program that it is now      *
*           necessary to move pieces to defend the king-side, in an effort to *
*           protect a king-side that is deemed somewhat unsafe.               *
*                                                                             *
*    5.12   improved null-move code by computing the Knuth/Moore node type    *
*           and then only trying null-moves at type 2 nodes.  this has        *
*           resulted in a 2x speedup in test positions.                       *
*                                                                             *
*    5.13   optimization to avoid doing a call to MakeMove() if it can be     *
*           avoided by noting that the move is not good enough to bring the   *
*           score up to an acceptable level.                                  *
*                                                                             *
*    5.14   artificially isolated pawns recognized now, so that Crafty won't  *
*           push a pawn so far it can't be defended.  also, the bonus for a   *
*           outside passed pawn was nearly doubled.                           *
*                                                                             *
*    5.15   passed pawns supported by a king, or connected passed pawns now   *
*           get a large bonus as they advance.  minor fix to the ICC resume   *
*           feature to try to avoid losing games on time.                     *
*                                                                             *
*    6.0    converted to rotated bitboards to make attack generation *much*   *
*           faster.  MakeMove() now can look up the attack bit vectors        *
*           rather than computing them which is significantly faster.         *
*                                                                             *
*    6.1    added a "hung piece" term to Evaluate() which penalizes the side  *
*           to move if a piece is attacked by a lesser piece, or a piece is   *
*           attacked and not defended at all.  additionally, a new scoring    *
*           term was added to detect a weak back rank (where the king does    *
*           not attack an empty square on the 2nd rank, and there are no      *
*           horizontally sliding pieces [rook or queen] on the first rank to  *
*           guard against back-rank mates.                                    *
*                                                                             *
*    6.2    modified the "futility" cutoff to (a) emulate the way the program *
*           normally searches, but avoiding MakeMove() calls when possible,   *
*           and (2) not searching a move near the horizon if the material     *
*           score is hopeless.                                                *
*                                                                             *
*    6.3    null-move code moved from NextMove() directly into Search().      *
*           this results is a "cleaner" implementation, as well as fixing an  *
*           error in the node type, caused by Search() thinking that after    *
*           the first move tried fails to cause a cutoff, that suddenly this  *
*           node is a type=3 node (Knuth and Moore).  this bug would then     *
*           introduce null-moves into later nodes that are a waste of time.   *
*                                                                             *
*    6.4    pawn scoring modified.  passed pawns were scored low enough that  *
*           Crafty would let the opponent create one in the endgame (as long  *
*           as it wasn't an "outside" passed pawn).  this required adjusting  *
*           isolated pawns as well.  all "weak" pawns (pawns on open files    *
*           that aren't defended by a pawn) are not penalized so much if      *
*           there aren't any rooks/queens to attack on the file.              *
*                                                                             *
*    7.0    removed the to.attack and from.attack bitboards.  these are now   *
*           computed as needed, using the rotated bitboards.  hung piece      *
*           stuff removed due to lack of any verified benefit.                *
*                                                                             *
*    7.1    modified next.c and nextc.c so that they check "mvv_lva_ordering" *
*           to determine which capture ordering strategy to use.  note that   *
*           setting this to "1" (default at present) disables the forward     *
*           pruning in quiescence search that wants to cull losing captures.  *
*                                                                             *
*    7.2    major clean-up of MakeMove() using macros to make it easier to    *
*           read and understand.  ditto for other modules as well.  fixed a   *
*           bug in Evaluate() where bishop scoring failed in endgame          *
*           situations, and discouraged king centralization.                  *
*                                                                             *
*    7.3    Evaluate() is no longer called if material is too far outside the *
*           current alpha/beta window.  the time-consuming part of Evaluate() *
*           is no longer done if the major scoring contributors in Evaluate() *
*           haven't pulled the score within the alpha/beta window, and the    *
*           remainder of Evaluate() can't possible accomplish this either.    *
*           gross error in EvaluatePawns() fixed, which would "forget" all    *
*           of the pawn scoring done up to the point where doubled white      *
*           pawns were found.  error was xx=+ rather than xx+=, which had     *
*           an extremely harmful on pawn structure evaluation.                *
*                                                                             *
*    7.4    performance improvements produced by elimination of bit-fields.   *
*           this was accomplished by hand-coding the necessary ands, ors, and *
*           shifts necessary to accomplish the same thing, only faster.       *
*                                                                             *
*    7.5    repetition code modified.  it could store at repetition_list[-1]  *
*           which clobbered the long long word just in front of this array.   *
*                                                                             *
*    7.6    null move search now searches with R=2, unless within 3 plies     *
*           of the quiescence search, then it searches nulls with R=1.        *
*                                                                             *
*    8.0    re-vamp of evaluation, bringing scores back down so that Crafty   *
*           will stop sacrificing the exchange, or a piece for two pawns just *
*           it thinks it has lots of positional compensation.  the next few   *
*           versions are going to be tuning or coding modifications to eval.  *
*                                                                             *
*    8.1    Futility() re-written to be more efficient, as well as to stop    *
*           tossing moves out based on the +/- 2 pawn window, which was too   *
*           speculative.  it still saves roughly 20% in speed over the same   *
*           searches without futility, but seems to have *no* harmful effects *
*           in tests run to date.                                             *
*                                                                             *
*    8.2    Futility() removed once and for all.  since MakeMove() is so      *
*           fast after the new attack generation algorithm, this was actually *
*           slower than not using it.                                         *
*                                                                             *
*    8.3    EvaluatePawns() weak pawn analysis bug fixed.  other minor        *
*           performance enhancements and cleanup.                             *
*                                                                             *
*    8.4    dynamic "king tropism" evaluation term now used, which varies     *
*           based on how exposed the target king is.  the more exposed, the   *
*           larger the bonus for placing pieces near the king.  new "analyze" *
*           feature that complements the "annotate" feature that Crafty has   *
*           had for a while.  "annotate" is used to play over moves that are  *
*           either in the current game history, or have been read in using    *
*           the "read <filename>" command.  analyze, on the other hand, will  *
*           immediately begin searching the current position.  each time the  *
*           operator enters a move, it will make that move on the board, and  *
*           then search the resulting position for the other side.  in effect *
*           this gives a running commentary on a "game in progress".  these   *
*           moves can be pumped into Crafty in many ways so that "on the fly" *
*           analysis of a game-in-progress is possible.                       *
*                                                                             *
*    8.5    more cleanup.  pawn promotions were searched twice, thanks to the *
*           killer moves, although often they resulted in no search overhead  *
*           due to transposition table hits the second time around.  other    *
*           minor fixes, including one serious "undefined variable" in        *
*           Evaluate() that caused grief in endings.                          *
*                                                                             *
*    8.6    new book file structure.  it is no longer necessary to enter the  *
*           "number of records" as Crafty now uses a new compressed hashing   *
*           technique so that the book has no empty space in it, and the size *
*           is computed "on the fly" producing a smaller book without the     *
*           user having to compute the size.  tweaks to Evaluate() to cure    *
*           minor irritating habits.                                          *
*                                                                             *
*    8.7    repaired optimization made in null-move search, that forgot to    *
*           clear "EnPassant_Target".  a double pawn push, followed by a null *
*           left the side on move "very confused" and would let it play an    *
*           illegal enpassant capture in the tree. sort.<n> files are now     *
*           removed after book.bin is created.  also, minor change to book    *
*           format makes it incompatible with older book versions, but also   *
*           allows book lines to be as long as desired, and the book size to  *
*           reach any reasonable size.  dynamic king tropism replaced by a    *
*           dynamic computation, but with a static king tropism score, rather *
*           than a score based on king exposure.  there was simply too much   *
*           interaction, although this may prove workable later.              *
*                                                                             *
*    8.8    tweaks to passed pawn scoring.  scores were too low, making       *
*           Crafty "ignore" them too much.                                    *
*                                                                             *
*    8.9    internal iterative deepening is now used in those rare positions  *
*           where a PV node has no hash move to search.  this does a shallow  *
*           search to depth-2 to find a good move for ordering.               *
*                                                                             *
*    8.10   internal interative deepening modified to handle cases where lots *
*           of tactics make the deepening search fail high or low, and        *
*           occasionally end up with no move to try.  this produced a "bad    *
*           move from hash table" error.                                      *
*                                                                             *
*    8.11   minor bug in internal iterative deepening fixed.  also, macros    *
*           for accessing the "position" data structure are now used to make  *
*           things a little more readable.                                    *
*                                                                             *
*    8.12   fixed minor bugs in quiescence checks, which let Crafty include   *
*           more checks than intended (default quiescence_checks=2, but the   *
*           comparison was <= before including a check, which made it include *
*           three.  Additionally, a hashed check was *always* tried, even if  *
*           quiescence_checks had already been satisfied.  pawn scoring also  *
*           had a bug, in that there was a "crack" between opening and middle *
*           game, where Crafty would conclude that it was in an endgame, and  *
*           adjust the pawn advance scores accordingly, sometimes causing an  *
*           odd a4/a5/etc move "out of the blue."  minor bug in next_capture  *
*           was pruning even exchanges very early in quiescence search.  that *
*           has now been removed, so only losing exchanges are pruned.        *
*                                                                             *
*    8.13   NextCapture() now *always* tries checking moves if the move at    *
*           the previous ply was a null-move.  this avoids letting the null   *
*           move hide some serious mating threats.  a minor bug where Crafty  *
*           could draw by repetition after announcing a mate.  this was a     *
*           result of finding a mate score in hash table, and, after the      *
*           search iteration completed, the mate score would terminate the    *
*           search completely.  now, the search won't terminate until it      *
*           finds a mate shorter than the previous search did.  minor eval    *
*           tweaks and bugfixes as well.                                      *
*                                                                             *
*    8.14   checks after null moves discontinued.  worked in some cases, but, *
*           in general made the tree larger for nothing.  eval tweaks to stop *
*           sacs that resulted in connected passed pawns, often at the        *
*           expense of a minor piece for a pawn or two, which often lost.     *
*           mobility coeffecient increased for all pieces.  asymmetric king   *
*           safety discontinued.  king safety for both sides is now equally   *
*           important.  king safety is now also proportional to the number of *
*           pieces the other side has, so that a disrupted king-side will     *
*           encourage trading pieces to reduce attacking chances.             *
*                                                                             *
*    8.15   weak pawn scoring modified further.  the changes are designed to  *
*           cause Crafty to keep pawns "mobile" where they can advance,       *
*           rather than letting them become blocked or locked.                *
*                                                                             *
*    8.16   still more weak pawn modifications.  in addition, there is no     *
*           "rook on half-open file" any longer.  if there are only enemy     *
*           pawns on the file, and the most advanced one is weak, then the    *
*           file is treated as though it were open.  technical error in the   *
*           EvaluateDevelopment() code that caused screwey evaluations is     *
*           fixed, making Crafty more likely to castle.  :)  Book() now       *
*           verifies that the current position is in book, before trying any  *
*           moves to see if the resulting positions are in book.  This avoids *
*           something like e4 e5 Bb5 a6 Nf3 from causing Crafty to play Nc6   *
*           which takes it back into book, rather than axb5 winning a piece.  *
*           this will occasionally backfire and prevent Crafty from trans-    *
*           posing back into book, but seems safer at present.                *
*                                                                             *
*    8.17   piece values changed somewhat, to avoid Crafty's propensity to    *
*           make unfavorable trades.  an example:  Crafty is faced with the   *
*           loss of a pawn.  instead, it trades the queen for a rook and      *
*           bishop, which used to be the same as losing a pawn.  this is not  *
*           so good, and often would result in a slow loss.  this version     *
*           also implements several "user-supplied" patches to make Crafty    *
*           compile cleanly on various machines.  In addition, you no longer  *
*           have to continually modify types.h for different configurations.  *
*           the Makefile now supplies a -D<target> to the compiler.  you need *
*           to edit Makefile and set "target" to the appropriate value from   *
*           the list provided.  then, when getting a new version, save your   *
*           Makefile, extract the new source, copy in your makefile and you   *
*           will be ready, except for those rare occasions where I add a new  *
*           source module.  other changes are performance tweaks.  one is a   *
*           simple trick to order captures while avoiding Swap() if possible. *
*           if the captured piece is more valuable than the capturing piece,  *
*           we can simply use the difference (pessimistic value) rather than  *
*           calling Swap() since this pessimistic value is still > 0.  other  *
*           tweaks to the various Next_*() routines to avoid a little un-     *
*           necessary work.                                                   *
*                                                                             *
*    8.18   book move selection algorithm changed.  note that this is highly  *
*           speculative, but when best book play is selected, Crafty will     *
*           use the books.bin file as always.  if there is no move in books,  *
*           Crafty reverts to book.bin, but now finds all known book moves    *
*           and puts them into the root move list.  It then does a fairly     *
*           short search, only considering these moves, and it will play the  *
*           best one so long as the evaluation is acceptable.  if not, it     *
*           simply returns as though there was no book move, and executes a   *
*           search as it normally would.  the book hashing algorithm was also *
*           modified so that the 64-bit hash key is slightly different from   *
*           the real hash key.  in effect, the upper 16 bits are only set as  *
*           a result of the side-not-to-move's pieces.  this means that for a *
*           given position, all the book moves will be in the same "cluster"  *
*           which makes the book dramatically faster, since one seek and read *
*           produces all the possible book moves (plus some others too, since *
*           the upper 16 bits are not unique enough.)  A significant speed    *
*           improvement is noticable, particularly with a 60mb opening book.  *
*           Crafty now understands (if that's the right word) that endings    *
*           with bishops of opposite colors are to be avoided.  when such an  *
*           ending is encountered, the total score is simply divided by two   *
*           to make the ending look more drawish.                             *
*                                                                             *
*    8.19   book selection algorithm further modified, so that the result of  *
*           each game in the GM database is known.  now Crafty will not play  *
*           a move from book, if all games were lost by the side on move,     *
*           hopefully avoiding many blunders.  Crafty now understands how to  *
*           attack if both players castle on opposite sides, by encouraging   *
*           pawn advances against the kings.  other minor modifications to    *
*           the eval values.                                                  *
*                                                                             *
*    8.20   PVS search finally implemented fully.  problems several months    *
*           ago prevented doing the PVS search along the PV itself.  this is  *
*           therefore quite a bit faster, now that it's fully implemented and *
*           working properly.  elapsed time code modified so that Crafty now  *
*           uses gettimeofday() to get fractions of a second elapsed time.    *
*           slight modification time allocation algorithm to avoid using too  *
*           much time early in the game.                                      *
*                                                                             *
*    8.21   courtesy of Mark Bromley, Crafty may run significantly faster on  *
*           your machine.  there are some options in the Makefile that will   *
*           eliminate many of the large attack computation long longs, which  *
*           helps prevent cache thrashing.  on some machines this will be     *
*           slower, on others 20% (or maybe more) faster.  you'll have to     *
*           try -DCOMPACT_ATTACKS, and if that's faster, then it's time to    *
*           try -DUSE_SPLIT_SHIFTS which may help even more.  finally, if you *
*           are running on a supersparc processor, you can use the fastattack *
*           assembly module fastattack.s and get another big boost.  (attack  *
*           function is largest compute user in Crafty at present.) serious   *
*           search problem fixed.  it turns out that Crafty uses the hash     *
*           table to pass PV moves from one iteration to the next, but for    *
*           moves at the root of the tree the hash table has no effect, so a  *
*           special case was added to RootMoveList to check the list of moves *
*           and if one matches the PV move, move it to the front of the list. *
*           this turned out to be critical because after completing a search, *
*           (say to 9 plies) Crafty makes its move, then chops the first two  *
*           moves off of the PV and then passes that to iterate to start a    *
*           search.  this search starts at lastdepth-1 (8 in this case) since *
*           the n-2 search was already done.  the "bug" showed up however, in *
*           that RootMoveList() was not checking for the PV move correctly,   *
*           which meant the root moves were ordered by static eval and static *
*           exchange evaluation.  Normally not a serious problem, just that   *
*           move ordering is not so good.  however, suppose that the opponent *
*           takes a real long time to make a move (say 30 seconds) so that    *
*           Crafty completes a 10 ply search.  it then starts the next search *
*           at depth=9, but with the wrong move first.  if the target time is *
*           not large enough to let it resolve this wrong move and get to the *
*           other moves on the list, Crafty is "confused" into playing this   *
*           move knowing absolutely nothing about it.  the result is that it  *
*           can play a blunder easily.  the circumstances leading to this are *
*           not common, but in a 60 move game, once is enough.  PV was pretty *
*           well mis-handled in carrying moves from one search to another     *
*           (not from one iteration to another, but from one complete search  *
*           to another) and has been fixed.  a cute glitch concerning storing *
*           PV from one iteration to another was also fixed, where the score  *
*           stored was confusing the following search.                        *
*                                                                             *
*    8.22   EPD support (courtesy of Steven Edwards [thanks]) is now standard *
*           in Crafty.  for porting, I'll provide a small test run which can  *
*           be used to validate Crafty once it's been compiled.               *
*                                                                             *
*    8.23   cleanup/speedup in hashing.  LookUp() and Store*() now carefully  *
*           cast the boolean operations to the most efficient size to avoid   *
*           64bit operations when only the right 32 bits are significant.     *
*           RepetitionCheck() code completely re-written to maintain two      *
*           repetition lists, one for each side.  Quiesce() now handles the   *
*           repetition check a little different, being careful to not call    *
*           it when it's unimportant, but calling it when repetitions are     *
*           possible.                                                         *
*                                                                             *
*    8.24   tweaks for king tropism to encourage pieces to collect near the   *
*           king, or to encourage driving them away when being attacked.  a   *
*           modification to Evaluate() to address the problem where Crafty    *
*           is forced to play Kf1 or Kf8, blocking the rook in and getting    *
*           into tactical difficulties as a result.  Book problem fixed where *
*           BookUp was attempting to group moves with a common ancestor       *
*           position together.  unfortunately, captures were separated from   *
*           this group, meaning capture moves in the book could never be      *
*           played.  if a capture was the only move in book, it sort of       *
*           worked because Crafty would drop out of book (not finding the     *
*           capture) and then the search would "save" it.  however, if there  *
*           was a non-capture alternative, it would be forced to play it,     *
*           making it play gambits, and, often, bad ones.  tablebase support  *
*           is now in.  the tablebase files can be downloaded from the ftp    *
*           machine at chess.onenet.net, pub/chess/TB.  currently, Steven     *
*           Edwards has all interesting 4 piece endings done.  to make this   *
*           work, compile with -DTABLEBASES, and create a TB directory where  *
*           Crafty is run, and locate the tablebase files in that directory.  *
*           if you are running under UNIX, TB can be a symbolic or hard link  *
*           to a directory anywhere you want.                                 *
*                                                                             *
*    8.25   minor repair on draw by repetition.  modified how books.bin was   *
*           being used.  now, a move in books.bin has its flags merged with   *
*           the corresponding move from book.bin, but if the move does not    *
*           exist in book.bin, it is still kept as a book move.  repetitions  *
*           are now counted as a draw if they occur two times, period.  the   *
*           last approach was causing problems due to hashing, since the hash *
*           approach used in Crafty is fairly efficient and frequently will   *
*           carry scores across several searches.  this resulted in Crafty    *
*           stumbling into a draw by repetition without knowing.  the con-    *
*           figuration switch HAS-64BITS has been cleaned up and should be    *
*           set for any 64bit architecture now, not just for Cray machines.   *
*                                                                             *
*    8.26   new search extension added, the well-known "one legal response to *
*           check" idea.  if there's only one legal move when in check, we    *
*           extend two plies rather than one, since this is a very forcing    *
*           move.  also, when one side has less than a rook, and the other    *
*           has passed pawns, pushing these pawns to the 6th or 7th rank      *
*           will extend the search one ply, where in normal positions, we     *
*           only extend when a pawn reaches the 7th rank.                     *
*                                                                             *
*    9.0    minor constraint added to most extensions:  we no longer extend   *
*           if the side on move is either significantly ahead or behind,      *
*           depending on the extension.  for example, getting out of check    *
*           won't extend if the side on move is a rook behind, since it's     *
*           already lost anyway.  we don't extend on passed pawn pushes if    *
*           the side on move is ahead a rook, since he's already winning.     *
*           minor adjustments for efficiency as well.  the next few           *
*           versions in this series will have module names in these comments  *
*           indicating which modules have been "cleaned" up.  this is an      *
*           effort at optimizing, although it is currently directed at a line *
*           by line analysis within modules, rather than major changes that   *
*           effect more global ideas.  this type of optimization will come at *
*           a later point in time.                                            *
*                                                                             *
*    9.1    NextMove(), NextCapture() and NextEvasion() were re-structured    *
*           to eliminate unnecessary testing and speed them up significantly. *
*           EvaluateTrades() was completely removed, since Crafty already has *
*           positional scores that encourage/discourage trading based on the  *
*           status of the game.  EvaluateTempo() was evaluated to be a        *
*           failure and was removed completely.                               *
*                                                                             *
*    9.2    Quiesce()) and Search() were modified to be more efficient.  in   *
*           addition, the null-move search was relaxed so that it always does *
*           a search to depth-R, even if close to the end of the tree.        *
*                                                                             *
*    9.3    Check() is no longer used to make sure a position is legal after  *
*           MakeMove() called, but before Search() is called recursively.  We *
*           now use the same approach as Cray Blitz, we make a move and then  *
*           capture the king at the next ply to prove the position is not a   *
*           valid move.  if the side-on-move is already in check, we use      *
*           NextEvasion() which only generates legal moves anyway, so this    *
*           basically eliminates 1/2 of the calls to Check(), a big win.      *
*           this resulted in modifications to Search(), Quiesce(), and        *
*           QuiesceFull().  connected passed pawns on 6th-7th no longer       *
*           trigger search extensions.  solved some problems like Win at      *
*           Chess #2, but overall was a loser.                                *
*                                                                             *
*    9.4    Lookup now checks the transposition table entry and then informs  *
*           Search() when it appears that a null move is useless.  this is    *
*           found when the draft is too low to use the position, but it is at *
*           least as deep as a null-move search would go, and the position    *
*           did not cause a fail-high when it was stored.  king-safety has    *
*           modified again.  the issue here is which king should attract the  *
*           pieces, and for several months the answer has been the king that  *
*           is most exposed attracts *all* pieces.  this has been changed to  *
*           always attract one side's pieces to the other king.  Crafty's     *
*           asymmetric king-safety was making it overly-defensive, even when  *
*           the opponent's king was more exposed.  this should cure that and  *
*           result in more aggressive play.                                   *
*                                                                             *
*    9.5    "vestigial code" (left over from who-knows-when) rmoved from      *
*           Evaluate().  this code used an undefined variable when there was  *
*           no bishop or knight left on the board, and could add in a penalty *
*           on random occasions.  quiescence checks removed completely to see *
*           how this works, since checks extend the normal search significant *
*           amounts in any case.  QuiesceFull() removed and merged into       *
*           Quiesce() which is faster and smaller.  first impression: faster, *
*           simpler, better.  the benefit of no quiescence checks is that the *
*           exhaustive search goes deeper to find positional gains, rather    *
*           than following checks excessively.  No noticable loss in tactical *
*           strength (so far).                                                *
*                                                                             *
*    9.6    legal move test re-inserted in Search() since null-moves could    *
*           make it overlook the fact that a move was illegal.  new assembly  *
*           code for X86 improves performance about 1/3, very similarly to    *
*           the results obtained with the sparc-20 assembly code.  this was   *
*           contributed by Eugene Nalimov and is a welcome addition.          *
*                                                                             *
*    9.7    optimizations continuing.  minor bug in pawn evaluation repaired. *
*           Crafty looked at most advanced white pawn, but least-advanced     *
*           black pawn on a file when doing its weak pawn evaluation.  the    *
*           history heuristic was slightly broken, in that it was supposed to *
*           be incrementing the history count by depth*depth, but this was    *
*           somehow changed to 7*depth, which was not as good.  assembly      *
*           language interface fixed to cleanly make Crafty on all target     *
*           architectures.                                                    *
*                                                                             *
*    9.8    the first change was to borrow a time-allocation idea from Cray   *
*           Blitz.  essentially, when Crafty notices it has used the normal   *
*           time allotment, rather than exiting the search immediately, it    *
*           will wait until it selects the next move at the root of the tree. *
*           the intent of this is that if it has started searching a move     *
*           that is going to be better than the best found so far, this will  *
*           take more time, while a worse move will fail low quickly.  Crafty *
*           simply spends more time to give this move a chance to fail high   *
*           or else quickly fail low.  a fairly serious bug, that's been      *
*           around for a long time, has finally been found/fixed.  in simple  *
*           terms, if the "noise" level was set too high, it could make       *
*           Crafty actually play a bad move.  the more likely effect was to   *
*           see "bad move hashed" messages and the first few lines of output  *
*           from the search often looked funny.  the bug was "noise" was used *
*           as a limit, until that many nodes had been searched, no output    *
*           would be produced.  unfortunately, on a fail-high condition, the  *
*           same code that produced the "++  Nh5" message also placed the     *
*           fail-high move in the PV.  failing to do this meant that the      *
*           search could fail high, but not play the move it found.  most     *
*           often this happened in very fast games, or near the end of long   *
*           zero-increment games.  it now always saves this move as it should *
*           regardless of whether it prints the message or not.               *
*                                                                             *
*    9.9    interface to xboard changed from -ics to -xboard, so that -ics    *
*           can be used with the custom interface.  additional code to        *
*           communicate with custom interface added.  Search() extensions are *
*           restricted so that there can not be more than one extension at a  *
*           node, rather than the two or (on occasion) more of the last       *
*           version.                                                          *
*                                                                             *
*    9.10   converted to Ken Thompson's "Belle" hash table algorithm.  simply *
*           put, each side has two tables, one that has entries replaced on a *
*           depth-priority only, the other is an "always replace" table, but  *
*           is twice as big.  this allows "deep" entries to be kept along     *
*           with entries close to search point.                               *
*                                                                             *
*    9.11   bug in Search() that could result in the "draft" being recorded   *
*           incorrectly in the hash table.  caused by passing depth, which    *
*           could be one or two plies more than it should be due to the last  *
*           move extending the search.  RepetitionCheck() completely removed  *
*           from Quiesce() since only capture moves are included, and it's    *
*           impossible to repeat a position once a capture has been made.     *
*                                                                             *
*    9.12   optimizations: history.c, other minor modifications.              *
*                                                                             *
*    9.13   EvaluatePawns() modified significantly to simplify and be more    *
*           accurate as well.  pawns on open files are now not directly       *
*           penalized if they are weak, rather the penalty is added when the  *
*           rooks are evaluated.  pawn hashing modified to add more info to   *
*           the data that is hashed.  king safety scores toned down almost    *
*           50% although it is still asymmetric.                              *
*                                                                             *
*    9.14   EvaluatePawns() modified further so that it now does the same     *
*           computations for king safety as the old EvaluateKingSafety*()     *
*           modules did, only it produces four values, two for each king,     *
*           for each side of the board (king or queen-side).  this is now     *
*           hashed with the rest of the pawn scoring, resulting in less       *
*           hashing and computation, and more hits for king-safety too.  king *
*           safety modified further.  asymmetric scoring was accidentally     *
*           disabled several versions ago, this is now enabled again.         *
*                                                                             *
*    9.15   Evaluate() now attempts to recognize the case where a knight is   *
*           trapped on one of the four corner squares, much like a bishop     *
*           trapped at a2/a7/h2/h7.  if a knight is on a corner square, and   *
*           neither of the two potential flight squares are safe (checked by  *
*           Swap()) then a large penalty is given.  this was done to avoid    *
*           positions where Crafty would give up a piece to fork the king and *
*           rook at (say) c7, and pick up the rook at a8 and think it was an  *
*           exchange ahead, when often the knight was trapped and it was      *
*           two pieces for a rook and pawn (or worse.)  two timing bugs fixed *
*           in this version:  (1) nodes_per_second was getting clobbered at   *
*           the end of iterate, which would then corrupt the value stored in  *
*           nodes_between_time_checks and occasionally make Crafty not check  *
*           the time very often and use more time than intended; (2) the      *
*           "don't stop searching after time is out, until the current move   *
*           at the root of the tree has been searched" also would let Crafty  *
*           use time unnecessarily.                                           *
*                                                                             *
*    9.16   significant changes to the way MakeMove() operates.  rather than  *
*           copying the large position[ply] structure around, the piece       *
*           location bitmaps and the array of which piece is on which square  *
*           are now simple global variables.  this means that there is now an *
*           UnMakeMove() function that must be used to restore these after a  *
*           a move has been made.  the benefit is that we avoid copying 10    *
*           bitmaps for piece locations when typically only one is changed,   *
*           ditto for the which piece is on which square array which is 64    *
*           bytes long.  the result is much less memory traffic, with the     *
*           probability that the bitmaps now stay in cache all the time. the  *
*           EvaluatePawns() code now has an exponential-type penalty for      *
*           isolated pawns, but it penalizes the difference between white and *
*           black isolated pawns in this manner.  king evaluation now         *
*           evaluates cases where the king moves toward one of the rooks and  *
*           traps it in either corner.                                        *
*                                                                             *
*    9.17   Xboard compatibility version!  now supports, so far as I know,    *
*           the complete xboard interface, including hint, show thinking,     *
*           taking back moves, "move now" and so forth.  additionally, Crafty *
*           now works with standard xboard.  notice that this means that a    *
*           ^C (interrupt) will no long terminate crafty, because xboard uses *
*           this to implement the "move now" facility.  this should also      *
*           work with winboard and allow pondering, since there is now a      *
*           special windows version of CheckInput() that knows how to check   *
*           Win95 or WinNT pipes when talking with winboard.                  *
*                                                                             *
*    9.18   Xboard compatibility update.  "force" (gnu) mode was broken, so   *
*           that reading in a PGN file (via xboard) would break.  now, Crafty *
*           can track the game perfectly as xboard reads the moves in.  king  *
*           safety modified to discourage g3/g6 type moves without the bishop *
*           to defend the weak squares.  significant other changes to the     *
*           Evaluate() procedure and its derivatives.  very nasty trans/ref   *
*           bug fixed.  been there since transposition tables added.  when    *
*           storing a mate score, it has to be adjusted since it's backed up  *
*           as "mate in n plies from root" but when storing, it must be saved *
*           as "mate in n plies from current position".  trivial, really, but *
*           I overlooked one important fact:  mate scores can also be upper   *
*           or lower search bounds, and I was correcting them in the same way *
*           which is an error.  the effect was that crafty would often find a *
*           mate in 2, fail high on a move that should not fail high, and end *
*           up making a move that would mate in 3.  in particularly bad cases *
*           this would make the search oscillate back and forth and draw a    *
*           won position.  the fix was to only adjust the score if it's a     *
*           score, not if it's a bound.  the "age" field of the trans/ref     *
*           table was expanded to two bits.  iterate now increments a counter *
*           modulo 3, and this counter is stored in the two ID bits of the    *
*           trans/ref table.  if they are == the transposition_id counter,    *
*           we know that this is a position stored during the current search. *
*           if not, it's an "old" position.  this avoids the necessity of     *
*           stepping through each entry in the trans/ref table (at the begin- *
*           ning of a search) to set the age bit.  much faster in blitz games *
*           as stepping through the trans/ref table, entry by entry, would    *
*           blow out cache.  this idea was borrowed from Cray Blitz, which    *
*           used this same technique for many years.                          *
*                                                                             *
*    9.19   new evaluation code for passed pawns.  Crafty now gives a large   *
*           bonus for two or more connected passed pawns, once they reach the *
*           sixth rank, when the opponent has less than a queen remaining.    *
*           this will probably be tuned as experience produces the special    *
*           cases that "break" it.  such things as the king too far away or   *
*           the amount of enemy material left might be used to further im-    *
*           prove the evaluation.                                             *
*                                                                             *
*    9.20   bug in SetBoard() fixed.  validity checks for castling status and *
*           en passant status was broken for castle status.  it was checking  *
*           the wrong rook to match castling status, and would therefore not  *
*           accept some valid positions, including #8 in Win At Chess suite.  *
*           minor repetition bug fixed, where Crafty would recognize that a   *
*           three-fold repetition had occurred, but would not claim it when   *
*           playing on a chess server.  Crafty now does not immediately re-   *
*           search the first move when it fails low.  rather, it behaves like *
*           Cray Blitz, and searches all moves.  if all fail low, Crafty will *
*           then relax (lower) alpha and search the complete list again.      *
*           this is a simple gamble that there is another move that will      *
*           "save the day" so that we avoid the overhead of finding out just  *
*           how bad the first move is, only to have more overhead when a good *
*           move fails high.  minor repetition bug fixed.  when running test  *
*           suites of problems, SetBoard() neglected to put the starting      *
*           position in the repetition list, which would occasionally waste   *
*           time.  one notable case was Win At Chess #8, where crafty would   *
*           find Nf7+ Kg8 Nh6+ Kh8 (repeating the original position) followed *
*           by Rf7 (the correct move, but 4 tempi behind.)  Display() bug     *
*           fixed.  Crafty now displays the current board position, rather    *
*           than the position that has been modified by a search in progress. *
*           castling evaluation modified including a bug in the black side    *
*           that would tend to make Crafty not castle queen-side as black if  *
*           the king-side was shredded.  minor bug Book().  if the book was   *
*           turned off (which is done automatically when using the test com-  *
*           mand since several of the Win At Chess positions are from games   *
*           in Crafty's large opening book) Book() would still try to read    *
*           the file and then use the data from the uninitialized buffer.     *
*           this would, on occasion, cause Crafty to crash in the middle of   *
*           a test suite run.                                                 *
*                                                                             *
*    9.21   castling evaluation has been significantly modified to handle the *
*           following three cases better:                                     *
*           (a) one side can castle at the root of the tree, but can not      *
*           castle at a tip position.  if the move that gave up the right to  *
*           castle was *not* a castle move, penalize the score.               *
*           (b) one side can castle short or long at the root, but has al-    *
*           ready castled by the time the search reaches a tip position.      *
*           here the goal is to make sure the player castled to the "better"  *
*           side by comparing king safety where the king really is, to king   *
*           safety had the king castled to the other side.  if the latter is  *
*           "safer" then penalize the current position by the "difference"    *
*           in king safety to encourage that side to delay castling.  this is *
*           a problem when Crafty can castle kingside *now*, but the kingside *
*           has been disrupted somehow.  if the search is not deep enough to  *
*           see clearing out the queenside and castling long, then it will    *
*           often castle short rather than not castle at all.  this fix will  *
*           make it delay, develop the queenside, and castle to a safer king  *
*           shelter, unless tactics force it to castle short.                 *
*           (c) one side can castle short or long at the root, but can only   *
*           castle one way at a tip position, because a rook has been moved.  *
*           if the remaining castle option is worse than castling to the side *
*           where the rook was moved, then we penalize the rook move to keep  *
*           castling to that side open as an option.                          *
*                                                                             *
*    9.22   more castling evaluation changes, to tune how/when Crafty chooses *
*           to castle.  bugs in Option() where certain commands could be ex-  *
*           ecuted while a search was in progress.  if these commands did     *
*           anything to the board position, it would break Crafty badly.      *
*           all that was needed was to check for an active search before some *
*           commands were attempted, and if a search was in progress, return  *
*           to abort the search then re-try the command.  when playing games  *
*           with computers (primarily a chess-server consideration) Crafty is *
*           now defaulting to "book random 0" to use a short search to help   *
*           avoid some ugly book lines on occasion.  in this version, *all*   *
*           positional scores were divided by 2 to reduce the liklihood that  *
*           crafty would sacrifice material and make up the loss with some    *
*           sort of positional gain that was often only temporary.  note that *
*           king safety scores were not reduced while doing this.  new book   *
*           random options:  0=search, 1=most popular move, 2=move that       *
*           produces best positional value, 3=choose from most frequently     *
*           played moves, 4=emulate GM frequency of move selection, 5=use     *
*           sqrt() on frequencies to give more randomness, 6=random.  for     *
*           now, computers get book_random=1, while GM's now get 4 (same as   *
*           before), everyone else gets 5.  up until this point, there has    *
*           penalty for doubled/tripled pawns.  it now exists.  also, if the  *
*           king stands on E1/E8, then the king-side king safety term is used *
*           to discourage disrupting the king-side pawns before castling.     *
*           Crafty now has a start-up initialization file (as most Unix       *
*           programs do).  this file is named ".craftyrc" if the macro UNIX   *
*           is defined, otherwise a "dossy" filename "crafty.rc" is used.     *
*           Crafty opens this file and executes every command in it as though *
*           they were typed by the operator.  the last command *must* be      *
*           "exit" to revert input back to STDIN.                             *
*                                                                             *
*    9.23   more evaluation tuning, including the complete removal of the     *
*           mobility term for rooks/queens.  since rooks already have lots    *
*           of scoring terms like open files, connected, etc, mobility is     *
*           redundant at times and often misleading.  The queen's mobility    *
*           is so variable it is nearly like introducing a few random points  *
*           at various places in the search.  minor xboard compatibility bug  *
*           fixed where you could not load a game file, then have crafty play *
*           by clicking the "machine white" or "machine black" options.       *
*                                                                             *
*    9.24   minor bug in Quiesce() repaired.  when several mate-in-1 moves    *
*           were available at the root, Crafty always took the last one,      *
*           which was often an under-promotion to a rook rather than to a     *
*           queen.  It looked silly and has been fixed so that it will play   *
*           the first mate-in-1, not the last.  RootMoveList() went to great  *
*           pains to make sure that promotion to queen occurs before rook.    *
*           bug in EvaluateOutsidePassedPawns() would fail to recognize that  *
*           one side had an outside passer, unless both sides had at least    *
*           one passer, which failed on most cases seen in real games.        *
*           minor "tweak" to move ordering.  GenerateMoves() now uses the     *
*           LastOne() function to enumerate white moves, while still using    *
*           FirstOne() for black moves.  this has the effect of moving pieces *
*           toward your opponent before moving them away.  a severe oversight *
*           regarding the use of "!" (as in wtm=!wtm) was found and corrected *
*           to speed things up some.  !wtm was replaced by wtm^1 (actually,   *
*           a macro ChangeSide(wtm) is used to make it more readable) which   *
*           resulted in significant speedup on the sparc, but a more modest   *
*           improvement on the pentium.                                       *
*                                                                             *
*    9.25   minor bug in Book() fixed where it was possible for crafty to     *
*           play a book move that was hardly ever played.  minor change to    *
*           time utilization to use a little more time "up front".  tuned     *
*           piece/square tables to improve development some.  saw Crafty lose *
*           an occasional game because (for example) the bishop at c1 or c8   *
*           had not moved, but the king had already castled, which turned the *
*           EvaluateDevelopment() module "off".  first[ply] removed, since it *
*           was redundant with last[ply-1].  the original intent was to save  *
*           a subscript math operation, but the compilers are quite good with *
*           the "strength-reduction" optimization, making referencing a value *
*           by first[ply] or last[ply-1] exactly the same number of cycles.   *
*           passed pawn scores increased.  the reduction done to reduce all   *
*           scores seemed to make Crafty less attentive to passed pawns and   *
*           the threats they incur.  Minor bug in Book() fixed, the variable  *
*           min_percent_played was inoperative due to incorrect test on the   *
*           book_status[n] value.  pawn rams now scored a little differently, *
*           so that pawn rams on Crafty's side of the board are much worse    *
*           than rams elsewhere.  This avoids positional binds where, for ex- *
*           ample Crafty would like black with pawns at e6 d5 c6, white with  *
*           pawns at e5 and c5.  black has a protected passed pawn at d5 for  *
*           sure, but the cramp makes it easy for white to attack black, and  *
*           makes it difficult for black to defend because the black pawns    *
*           block the position badly.  king safety tweaked up about 30%, due  *
*           to using nearly symmetrical scoring, the values had gotten too    *
*           small and were not overriding minor positional things.            *
*                                                                             *
*    9.26   minor bug fixes in time allocation.  this mainly affects Crafty   *
*           as it plays on a chess server.  the problem is, simply, that the  *
*           current internal timing resolution is 1/10 of a second, kept in   *
*           an integer variable.  .5 seconds is stored as 5, for example.     *
*           this became a problem when the target time for a search dropped   *
*           below .3 seconds, because the "easy move" code would terminate    *
*           the search after 1/3 of the target time elapsed if there were no  *
*           fail lows or PV changes.  unfortunately, 2 divided by 3 (.2 secs  *
*           /3) is "zero" which would let the search quickly exit, possibly   *
*           without producing a PV with a move for Crafty to play.  Crafty    *
*           would play the first PV move anyway, which was likely the PV move *
*           from the last search.  this would corrupt the board information,  *
*           often produce the error "illegal move" when the opponent tried to *
*           move, or, on occasion, blow crafty up.  on a chess server, Crafty *
*           would simply flag and lose.  after this fix, I played several     *
*           hundred games with a time control of 100 moves in 1 second and    *
*           there were no failures.  before the fix, this time control could  *
*           not result in a single completed game.  if you don't run Crafty   *
*           on a chess server, this version is not important, probably. a     *
*           minor fix for move input, so that moves like e2e4 will always be  *
*           accepted, where before they would sometimes be flagged as errors. *
*                                                                             *
*    9.27   this version has an interesting modification to NextCapture().    *
*           Quiesce() now passes alpha to NextCapture() and if a capture      *
*           appears to win material, but doesn't bring the score up to the    *
*           value of alpha, the capture is ignored.  in effect, if we are so  *
*           far behind that a capture still leaves us below alpha, then there *
*           is nothing to gain by searching the capture because our opponent  *
*           can simply stand pat at the next ply leaving the score very bad.  *
*           one exception is if the previous ply was in check, we simply look *
*           for any safe-looking capture just in case it leads to mate.  bad  *
*           book bug with book random=1 or 2 fixed (this is set when playing  *
*           a computer).  this bug would cause crafty to play an illegal move *
*           and bust wide open with illegal move errors, bad move hashed, and *
*           other things.  book randomness also quite a bit better now.       *
*                                                                             *
*    9.28   piece/square table added for rooks.  primary purpose is to dis-   *
*           courage rooks moving to the edge of the board but in front of     *
*           pawns, where it is easy to trap it.  Crafty now understands the   *
*           concept of blockading a passed pawn to prevent it from advancing. *
*           it always understood that advancing one was good, which would     *
*           tend to encourage blockading, but it would also often ignore the  *
*           pawn and let it advance a little here, a little there, until it   *
*           suddenly realized that it was a real problem.  the command to set *
*           the size of the hash table has been modified.  this command is    *
*           now "hash n", "hash nK" or "hash nM" to set the hash table to one *
*           of bytes, Kbytes, or Mbytes.  Note that if the size is not an     *
*           exact multiple of what crafty needs, it silently reduces the size *
*           to an optimum value as close to the suggested size as possible.   *
*           trade bonus/penalty is now back in, after having been removed     *
*           when the new UnMake() code was added.  crafty now tries to trade  *
*           pawns but not pieces when behind, and tries to trade pieces but   *
*           not pawns when ahead.  EvaluateDraws() now understands that rook  *
*           pawns + the wrong bishop is a draw.  minor adjustment to the      *
*           quiescence-search pruning introduced in 9.27, to avoid excluding  *
*           captures that didn't lose material, but looked bad because the    *
*           positional score was low.  slightly increased the values of the   *
*           pieces relative to that of a pawn to discourage the tendency to   *
*           trade a piece for two or three pawns plus some positional plusses *
*           that often would slowly vanish.  null-move search modified to try *
*           null-moves, even after captures.  this seems to hurt some prob-   *
*           lem positions, but speeds up others significantly.  seems better  *
*           but needs close watching.  EvaluateDraws() call moved to the top  *
*           of Evaluate() to check for absolute draws first.  Quiesce() now   *
*           always calls Evaluate() which checks for draws before trying the  *
*           early exit tests to make sure that draws are picked up first.     *
*           EvaluateDraws() substantially re-written to get rid of lots of    *
*           unnecessary instructions, and to also detect RP+B of wrong color  *
*           even if the losing side has a pawn of its own.  Crafty would      *
*           often refuse to capture that last pawn to avoid reaching an end-  *
*           game it knew was drawn.                                           *
*                                                                             *
*    9.29   new time.c as contributed by Mike Byrne.  basically makes Crafty  *
*           use more time up front, which is a method of anticipating that    *
*           Crafty will predict/ponder pretty well and save time later on     *
*           anyway.  InCheck() is never called in Quiesce() now.  as a        *
*           result, Crafty won't recognize mates in the q-search.  this has   *
*           speeded the code up with no visible penalty, other than it will   *
*           occasionally find a win of material rather than a mate score, but *
*           this has not affected the problem suite results in a measurable   *
*           way, other than crafty is now about 10% faster in average type    *
*           position, and much faster in others.  the latest epd code from    *
*           Steven Edwards is a part of this version, which includes updated  *
*           tablebase access code.                                            *
*                                                                             *
*   10.0    new time.c with a "monitoring feature" that has two distinct      *
*           functions:  (1) monitor crafty's time remaining and if it is too  *
*           far behind the opponent, force it to speed up, or if it is well   *
*           ahead on time, slow down some;  (2) if opponent starts moving     *
*           quickly, crafty will speed up as well to avoid getting too far    *
*           behind.  EvaluateDraws() modified to detect that if one side has  *
*           insufficient material to win, the score can never favor that side *
*           which will make Crafty avoid trading into an ending where it has  *
*           KNN vs KB for example, where KNN seems to be ahead.               *
*                                                                             *
*   10.1    new book.c.  crafty now counts wins, losses and draws for each    *
*           possible book move and can use these to play a move that had a    *
*           high percentage of wins, vs the old approach of playing a move    *
*           just because it was played a lot in the past.                     *
*                                                                             *
*   10.2    evaluation tuning.  rook on 7th, connected rooks on 7th, etc.,    *
*           are now more valuable.  Crafty was mistakenly evaluating this     *
*           too weakly.  book() changed so that the list of moves is first    *
*           sorted based on winning percentages, but then, after selecting    *
*           the sub-set of this group, the list is re-sorted based on the     *
*           raw total games won so that the most popular opening is still the *
*           most likely to be played.                                         *
*                                                                             *
*   10.3    timer resolution changed to 1/100th of a second units, with all   *
*           timing variables conforming to this, so that it is unnecessary    *
*           to multiply by some constant to convert one type of timing info   *
*           to the standard resolution.  bug in evaluate.c fixed.  trapped    *
*           rook (rook at h1, king at g1 for example) code had a bad sub-     *
*           script that caused erroneous detection of a trapped rook when     *
*           there was none.                                                   *
*                                                                             *
*   10.4    Quiesce() no longer checks for time exceeded, nor does it do any  *
*           hashing whatsoever, which makes it significantly faster, while    *
*           making the trees only marginally bigger.                          *
*                                                                             *
*   10.5    Quiesce() now calls GenerateCaptures() to generate capture moves. *
*           this new move generator module is significantly faster, which     *
*           speeds the quiescence search up somewhat.                         *
*                                                                             *
*   10.6    CastleStatus is now handled slightly different.  the right two    *
*           bits still indicate whether or not that side can castle to either *
*           side, and 0 -> no castling at all, but now, <0 means that no      *
*           castling can be done, and that castling rights were lost by       *
*           making a move that was not castling.  this helps in Evaluate()    *
*           by eliminating a loop to see if one side castled normally or had  *
*           to give up the right to castle for another (bad) reason.  this is *
*           simply an efficiency issue, and doesn't affect play or anything   *
*           at all other than to make the search faster.                      *
*                                                                             *
*   10.7    NextMove() now doesn't try history moves forever, rather it will  *
*           try the five most popular history moves and if it hasn't gotten a *
*           cutoff by then, simply tries the rest of the moves without the    *
*           history testing, which is much faster.                            *
*                                                                             *
*   10.8    Book() now "puzzles" differently.  In puzzling, it is trying to   *
*           find a move for the opponent.  since a book move by the opponent  *
*           will produce an instant move, puzzle now enumerates the set of    *
*           legal opponent moves, and from this set, removes the set of known *
*           book moves and then does a quick search to choose the best.  we   *
*           then ponder this move just in case the opponent drops out of book *
*           and uses a normal search to find a move.  Crafty now penalizes    *
*           a pawn that advances, leaving its neighbors behind where both     *
*           the advancing pawn, and the ones being left behind become weaker. *
*           the opening phase of the game has been modified to "stay active"  *
*           until all minor pieces are developed and Crafty has castled, to   *
*           keep EvaluateDevelopment() active monitoring development.  minor  *
*           change to DrawScore() so that when playing a computer, it will    *
*           return "default_draw_score" under all circumstances rather than   *
*           adjusting this downward, which makes Crafty accept positional     *
*           weaknesses rather than repeat a position.                         *
*                                                                             *
*   10.9    Evaluate() now handles king safety slightly differently, and      *
*           increases the penalty for an exposed king quite a bit when there  *
*           is lots of material present, but scales this increase down as the *
*           legal opponent moves, and from this set, removes the set of known *
*           material comes off.  this seems to help when attacking or getting *
*           attacked.  since Quiesce() no longer probes the hash table, the   *
*           idea of storing static evaluations there became worthless and was *
*           burning cycles that were wasted.  this has been removed. check    *
*           extensions relaxed slightly so that if ply is < iteration+10 it   *
*           will allow the extension.  old limit was 2*iteration.  connected  *
*           passed pawns on 6th now evaluated differently.  they get a 1 pawn *
*           bonus, then if material is less than a queen, another 1 pawn      *
*           bonus, and if not greater than a rook is left, and the king can't *
*           reach the queening square of either of the two pawns, 3 more      *
*           more pawns are added in for the equivalent of + a rook.           *
*                                                                             *
*   10.10   Evaluate() modified.  king safety was slightly broken, in that    *
*           an uncastled king did not get a penalty for open center files.    *
*           new evaluation term for controlling the "absolute 7th rank", a    *
*           concept from "My System".                                         *
*                                                                             *
*   10.11   lazy evaluation implemented.  now as the Evaluate() code is ex-   *
*           ecuted, crafty will "bail out" when it becomes obvious that the   *
*           remainder of the evaluation can't bring the score back inside the *
*           alpha/beta window, saving time.                                   *
*                                                                             *
*   10.12   more Jakarta time control commands added so operator can set the  *
*           time accurately if something crashes, and so the operator can set *
*           a "cushion" to compensate for operator inattention.  "easy" is    *
*           more carefully qualified to be a recapture, or else a move that   *
*           preserves the score, rather than one that appears to win material *
*           as was done in the past.  king safety tweaked a little so that if *
*           one side gives up the right to castle, and could castle to a safe *
*           position, the penalty for giving up the right is substantially    *
*           larger than before.  a minor bug also made it more likely for     *
*           black to give up the right to castle than for white.              *
*                                                                             *
*   10.13   modifications to Book() so that when using "book random 0" mode   *
*           (planned for Jakarta) and the search value indicates that we are  *
*           losing a pawn, we return "out of book" to search *all* the moves, *
*           not just the moves that are "in book."  This hopefully avoids     *
*           some book lines that lose (gambit) material unsoundly.            *
*                                                                             *
*   10.14   final timing modifications.  puzzling/booking searches now take   *
*           1/30th of the normal time, rather than 1/10th.  new command       *
*           "mode=tournament" makes crafty assess DRAW as "0" and also makes  *
*           it prompt the operator for time corrections as required by WMCCC  *
*           rules.                                                            *
*                                                                             *
*   10.15   Evaluate() tuning to reduce development bonuses, which were able  *
*           to overwhelm other scores and lead to poor positions.             *
*                                                                             *
*   10.16   "lazy" (early exit) caused some problems in Evaluate() since so   *
*           many scores are very large.  scaled this back significantly and   *
*           also EvaluateOutsidePassedPawns() could double the score if one   *
*           side had passers on both sides.  this was fixed.                  *
*                                                                             *
*   10.17   crafty's Book() facility has been significantly modified to be    *
*           more understandable.  book random <n> has the following options:  *
*           (0) do a search on set of book moves; (1) choose from moves that  *
*           are played most frequently; (2) choose from moves with best win   *
*           ratio; (3) choose from moves with best static evaluation; and     *
*           (4) choose completely randomly.  book width <n> can be used to    *
*           control how large the "set" of moves will be, with the moves in   *
*           the "set" being the best (according to the criteria chosen) from  *
*           the known book moves.  minor tweak to Search() so that it extends *
*           passed pawn pushes to the 6th or 7th rank if the move is safe,    *
*           and this is an endgame.                                           *
*                                                                             *
*   10.18   new annotate command produces a much nicer analysis file and also *
*           lets you exert control over what has to happen to trigger a com-  *
*           ment among other things.  AKA Crafty/Jakarta version.             *
*                                                                             *
*   11.1    fractional depth allows fractional ply increments.  the reso-     *
*           lution is 1/8 of a ply, so all of the older "depth++" lines now   *
*           read depth+=PLY, where #define PLY 8 is used everywhere.  this    *
*           gives added flexibility in that some things can increment the     *
*           depth by < 1 ply, so that it takes two such things before the     *
*           search actually extends one ply deeper.  Crafty now has full PGN  *
*           support.  a series of pgn commands (pgn Event 14th WMCCC) allow   *
*           the PGN tags to be defined, when crafty reads or annotates a PGN  *
*           file it reads and parses the headers and will display the info    *
*           in the annotation (.can) file or in the file you specify when you *
*           execute a "savegame <filename>" command.                          *
*                                                                             *
*   11.2    fractional ply increments now implemented.  11.1 added the basic  *
*           fractional ply extension capabilities, but this version now uses  *
*           them in Search().  there are several fractional ply extensions    *
*           being used, all are #defined in types.h to make playing with them *
*           somewhat easier.  one advantage is that now all extensions are    *
*           3/4 of a ply, which means the first time any extension is hit, it *
*           has no effect, but the next three extensions will result in an    *
*           additional ply, followed by another ply where the extension does  *
*           nothing, followed by ...  this allowd me to remove the limit on   *
*           how deep in the tree the extensions were allowed, and also seems  *
*           to not extend as much unless there's "something" going on.  when  *
*           the position gets interesting, these kick in.  asymmetric king    *
*           safety is back in.  Crafty's king safety is a little more im-     *
*           portant than the opponent's now to help avoid getting its king-   *
*           side shredded to win a pawn.  a new term, ROOK_ON_7TH_WITH_PAWN   *
*           to help Crafty understand how dangerout a rook on the 7th is if   *
*           it is supported by a pawn.  it is difficult to drive off and the  *
*           pawn provides additional threats as well.                         *
*                                                                             *
*   11.3    king safety modifications.  one fairly serious bug is that Crafty *
*           considered that king safety became unimportant when the total     *
*           pieces dropped below 16 material points.  note that this was      *
*           pieces only, not pieces and pawns, which left plenty of material  *
*           on the board to attack with.  as a result, Crafty would often     *
*           allow its king-side to be weakened because it was either in an    *
*           endgame, or close to an endgame.  *incorrectly*, it turns out. :) *
*           going back to a very old Crafty idea, king tropism is now tied to *
*           how exposed each king is, with pieces being attracted to the most *
*           exposed king for attack/defense.                                  *
*                                                                             *
*   11.4    tuning on outside passed pawn code.  values were not large enough *
*           and didn't emphasize how strong such a pawn is.  general passed   *
*           pawn value increased as well.  minor bug in edit that would let   *
*           crafty flag in some wild games was fixed.  basically, edit has no *
*           way to know if castling is legal, and has to assume if a rook and *
*           king are on the original squares, castling is o.k.  for wild2     *
*           games in particular, this can fail because castling is illegal    *
*           (by the rules for wild 2) while it might be that rooks and king   *
*           are randomly configured to make it look possible.  crafty would   *
*           then try o-o or o-o-o and flag when the move was rejected as      *
*           illegal.  Richard Lloyd suggested a speed-up for the annotate     *
*           command which I'm using temporarily.  Crafty now searches all     *
*           legal moves in a position, and if the best move matches the game  *
*           move, the second search of just the move played is not needed.  I *
*           was searching just the game move first, which was a waste of time *
*           in many positions.  I'll resume this later, because I want to see *
*           these values:  (1) score for move actually played; (2) score for  *
*           the rest of the moves only, not including the move actually       *
*           played, to see how often a player finds a move that is the *only* *
*           good move in a position.                                          *
*                                                                             *
*   11.5    more fractional ply extension tuning.  a few positions could blow *
*           up the search and go well beyond 63 plies, the max that Crafty    *
*           can handle.  now, extensions are limited to no more than one ply  *
*           of extensions for every ply of search.  note that "in check" is   *
*           extended on the checking move ply, not on the out of check ply,   *
*           so that one-reply-to-check still extends further.  minor changes  *
*           to time.c to further help on long fast games.  another important  *
*           timing bug repaired.  if Crafty started an iteration, and failed  *
*           high on the first move, it would not terminate the search until   *
*           this move was resolved and a score returned.  this could waste a  *
*           lot of time in already won positions.  interesting bug in the     *
*           capture search pruning fixed.  at times, a piece would be hung,   *
*           but the qsearch would refuse to try the capture because it would  *
*           not bring the score back to above alpha.  unfortunately, taking   *
*           the piece could let passed pawns race in undisturbed which would  *
*           affect the score.  this pruning is turned off when there are not  *
*           at least two pieces left on the board.  another bug in the move   *
*           generator produced pawn promotions twice, once when captures were *
*           generated, then again when the rest of the moves are produced.    *
*           this was also repaired.  small book now will work.  the BookUp()  *
*           code now assumes white *and* black won in the absence of a valid  *
*           result PGN tag, so that the small book will work correctly.       *
*                                                                             *
*   11.6    modifications to time allocation in a further attempt to make     *
*           Crafty speed up if time gets short, to avoid flagging.  it is now *
*           *very* difficult to flag this thing.  :)  minor evaluation tweaks *
*           and a fix to Quiesce() to speed it up again after a couple of bad *
*           fixes slowed things down.                                         *
*                                                                             *
*   11.7    minor modification to Evaluate().  rooks behind a passed pawn are *
*           good, but two rooks behind the pawn don't help if the pawn is     *
*           blockaded and can't move, unless the two rooks are not the same   *
*           color as the pawn.                                                *
*                                                                             *
*   11.8    first stage of "book learning" implemented.  Crafty monitors the  *
*           search evaluation for the first 10 moves out of book.  it then    *
*           computes a "learn value" if it thinks this set of values shows    *
*           that the book line played was very good or very bad.  this value  *
*           is added to a "learn count" for each move in the set of book      *
*           moves it played, with the last move getting the full learn value, *
*           and moves closer to the start of the game getting a smaller       *
*           percentage.  (see learn.c for more details).  these values are    *
*           used by Book() to help in selecting/avoiding book lines.  Crafty  *
*           produces a "book.lrn" file that synthesizes this information into *
*           a portable format that will be used by other crafty programs,     *
*           once the necessary code is added later on.                        *
*                                                                             *
*   11.9    an age-old problem caused by null-move searching was eliminated   *
*           in this version.  often the null-move can cause a move to fail    *
*           high when it should not, but then the move will fail low on the   *
*           re-search when beta is relaxed.  this would, on occasion, result  *
*           in Crafty playing a bad move for no reason.  now, if a fail high  *
*           is followed by a fail-low, the fail high condition is ignored.    *
*           this can be wrong, but with null-move pruning, it can also be     *
*           even "more wrong" to accept a fail high move after it fails low   *
*           due to a null-move failure, than it would be right to accept the  *
*           fail high/fail low that is caused by trans/ref table anomalies.   *
*                                                                             *
*   11.10   Crafty is now using the Kent, et. al, "razoring" algorithm, which *
*           simply eliminates the last ply of full-width searching and goes   *
*           directly to the capture search if the move before the last full-  *
*           width ply is uninteresting, and the Material eval is 1.3 pawns    *
*           below alpha.  It's a "modest futility" idea that doesn't give up  *
*           on the uninteresting move, but only tries captures after the move *
*           is played in the tree.  net result is 20-30% average faster times *
*           to reach the same depth.  minor mod to book.lrn to include Black, *
*           White and Date PGN tags just for reference.  next learning mod is *
*           another "book random n" option, n=3.  this will use the "learned" *
*           score to order book moves, *if* any of them are > 0.  what this   *
*           does is to encourage Crafty to follow opening lines that have     *
*           been good, even if the learn count hasn't reached the current     *
*           threshold of 1,000.  this makes learning "activate" faster.  this *
*           has one hole in it, in that once crafty learns that one move has  *
*           produced a positive learn value, it will keep playing that move   *
*           (since no others will yet have a positive value) until it finally *
*           loses enough to take the learn value below zero.  this will be    *
*           taken care of in the next version, hopefully.                     *
*                                                                             *
*   11.11   new "book random 4" mode that simply takes *all* learning scores  *
*           for the set of legal book moves, translates them so that the      *
*           worst value is +1 (by adding -Min(all_scores)+1 to every learn    *
*           value) and then squaring the result.  this encourages Crafty to   *
*           follow book lines that it has learned to be good, while still     *
*           letting it try openings that it has not yet tried (learn value    *
*           =0) and even lets it try (albeit rarely) moves that it has        *
*           learned to be bad.  minor evaluation tweaking for king placement  *
*           in endgames to encourage penetration in the center rather than on *
*           a wing where the king might get too far from the "action."        *
*                                                                             *
*   11.12   LearnFunction() now keeps positional score, rather than using a   *
*           constant value for scores < PAWN_VALUE, to improve learning       *
*           accuracy.  book learn <filename> [clear] command implemented to   *
*           import learning files from other Crafty programs.                 *
*                                                                             *
*   11.13   endgame threshold lowered to the other side having a queen or     *
*           less (9) rather than (13) points.  queen+bishop can be very       *
*           dangerous to the king for example, or two rooks and a bishop.     *
*           learning "curve" modified.  it was accidentally left in a sort    *
*           of "geometric" shape, with the last move getting the full learn   *
*           value, the next one back getting 1/2, the next 1/3, etc.   now    *
*           it is uniformly distributed from front to back.  if there are 20  *
*           moves, the last gets the full value, the next gets 19/20, etc..   *
*           also the "percentage played" was hosed and is fixed.              *
*                                                                             *
*******************************************************************************
*/
void main(int argc, char **argv)
{
#if defined(UNIX)
  char *craftyrcfile = 0;
  struct passwd *passwdbuffer = 0;
#endif
  int avoid_pondering=1;
  int move;
  int value=0, displayed, i;
  extern void InterruptSignal(int);
/*
 ----------------------------------------------------------
|                                                          |
|   initialize chess board to starting position.  if the   |
|   first option on the command line is a "c" then we will |
|   continue the last game that was in progress.           |
|                                                          |
 ----------------------------------------------------------
*/
  input_stream=stdin;
  if (argc > 1) {
    if (!strcmp(argv[1],"c")) Initialize(1);
    else Initialize(0);
  }
  else Initialize(0);
  if (argc > 1)
    for (i=1;i<argc;i++) if (strcmp(argv[i],"c")) {
      if ((argv[i][0]>='0') && (argv[i][0] <= '9')) {
        tc_moves=atoi(argv[i]);
        tc_time=atoi(argv[i+1]);
        tc_increment=0;
        tc_secondary_moves=tc_moves;
        tc_secondary_time=tc_time;
        Print(0,"%d moves/%d minutes primary time control\n",
              tc_moves, tc_time);
        Print(0,"%d moves/%d minutes secondary time control\n",
              tc_secondary_moves, tc_secondary_time);
        tc_time*=60;
        tc_time_remaining=tc_time;
        tc_secondary_time*=60;
        i++;
      }
      else {
        (void) Option(argv[i]);
        display=search;
      }
    }
#if defined(UNIX)
  passwdbuffer = getpwuid(getuid());
  if (passwdbuffer == 0) exit(1);
  craftyrcfile = malloc(strlen(passwdbuffer->pw_dir)+11);
  if (craftyrcfile == 0) exit(1);
  strcpy(craftyrcfile, passwdbuffer->pw_dir);
  strcat(craftyrcfile, "/.craftyrc");
  if (!(input_stream=fopen(craftyrcfile,"r")))
    if (!(input_stream=fopen(BOOKDIR "/.craftyrc","r")))
      if (!(input_stream=fopen(".craftyrc","r")))
#else
    if (!(input_stream=fopen("crafty.rc","r")))
#endif

  input_stream=stdin;

#if defined(UNIX)
  free(craftyrcfile);
#endif

  if (xboard) signal(SIGINT,InterruptSignal);
  Print(1,"\nCrafty v%s\n\n",version);
  if (ics) printf("*whisper Hello from Crafty v%s !\n",version);
/*
  if (xboard) printf("kibitz Hello from Crafty v%s !\n",version);
*/

  while (1) {
/*
 ----------------------------------------------------------
|                                                          |
|   prompt user and read input string for processing.  as  |
|   long as Option() returns a non-zero value, continue    |
|   reading lines and calling Option().  when Option()     |
|   fails to recogize a command, then try InputMove() to   |
|   determine if this is a legal move.                     |
|                                                          |
 ----------------------------------------------------------
*/
    opponent_start_time=GetTime(elapsed);
    made_predicted_move=0;
    ponder_completed=0;
    display=search;
    do {
      do {
        display=search;
        displayed=0;
        if (do_ponder && !ponder_completed &&
            !over && !force && !avoid_pondering) Ponder(wtm);
        if (((!do_ponder || over || !ponder_move || ponder_completed) &&
            !made_predicted_move) || avoid_pondering) {
          if (!xboard && !ics) {
            if (wtm) printf("White(%d): ",move_number);
            else printf("Black(%d): ",move_number);
          }
          fflush(stdout);
          strcpy(input,"");
          fscanf(input_stream,"%s",input);
          displayed=1;
          if (!strcmp(input,".") && ponder_move) {
            made_predicted_move=1;
            opponent_end_time=GetTime(elapsed);
          }
        }
        if (wtm) {
          if (!displayed && !ics && !xboard) Print(0,"White(%d): %s\n",move_number,input);
          else if (log_file) fprintf(log_file,"White(%d): %s\n",move_number,input);
        }
        else {
          if (!displayed && !ics && !xboard) Print(0,"Black(%d): %s\n",move_number,input);
          else if (log_file) fprintf(log_file,"Black(%d): %s\n",move_number,input);
        }
        if (made_predicted_move) break;
        opponent_end_time=GetTime(elapsed);
      } while (Option(input));
      if (made_predicted_move) {
        move=ponder_move;
        break;
      }
      if (strcmp(input,"move") && strcmp(input,"go") &&
          !(autoplay && !strcmp(input,"SP")))
        if (!ics && !xboard && !autoplay) move=InputMove(input,0,wtm,0,0);
        else {
          move=InputMoveICS(input,0,wtm,1,0);
          if (!move) {
            move=InputMoveICS(input,0,ChangeSide(wtm),0,0);
            if (move) wtm=ChangeSide(wtm);
          }
        }
        last_opponent_move=move;
    } while (!move && strcmp(input,"move") && strcmp(input,"go") &&
             !(autoplay && !strcmp(input,"SP")));
/*
 ----------------------------------------------------------
|                                                          |
|   make the move (using internal form returned by         |
|   InputMove() and then complement wtm (white to move).   |
|                                                          |
 ----------------------------------------------------------
*/
    if(strcmp(input,"move") && strcmp(input,"go") &&
       !(autoplay && !strcmp(input,"SP"))) {
      fseek(history_file,((move_number-1)*2+1-wtm)*10,SEEK_SET);
      fprintf(history_file,"%10s ",OutputMove(&move,0,wtm));
      if (xboard)
        if (wtm) printf("%d. %s\n",move_number,OutputMoveICS(&move));
        else printf("%d. %s\n",move_number,OutputMoveICS(&move));
      if (autoplay) {
        char *mv=OutputMoveICS(&move);
        if (!wtm)
        fprintf(auto_file,"\t");
        fprintf(auto_file, " %c%c-%c%c", mv[0], mv[1], mv[2], mv[3]);
        if ((mv[4] != ' ') && (mv[4] != 0))
        fprintf(auto_file, "/%c", mv[4]);
        fprintf(auto_file, "\n");
        fflush(auto_file);
      }
      MakeMoveRoot(move,wtm);
      if (RepetitionDraw(ChangeSide(wtm))==1) {
        Print(0,"%sgame is a draw by repetition.%s\n",Reverse(),Normal());
        value=DrawScore();
        if (xboard) Print(0,"Draw\n");
      }
      if (RepetitionDraw(ChangeSide(wtm))==2) {
        Print(0,"%sgame is a draw by the 50 move rule.%s\n",Reverse(),Normal());
        value=DrawScore();
      }
      ResignOrDraw(value,wtm);
      wtm=ChangeSide(wtm);
      if (wtm) move_number++;
      time_used_opponent=opponent_end_time-opponent_start_time;
      Print(1,"              time used: %s\n",DisplayTime(time_used_opponent));
      TimeAdjust(time_used_opponent,opponent);
    }
    else position[1]=position[0];
    ValidatePosition(0,move,"Main(1)");
    if (!force) {
      if (ponder_completed) {
        if((From(ponder_move) == From(move)) && (To(ponder_move) == To(move)) &&
           (Piece(ponder_move) == Piece(move)) &&
           (Captured(ponder_move) == Captured(move)) &&
           (Promote(ponder_move) == Promote(move))) {
          made_predicted_move=1;
        }
      }
      ponder_move=0;
      thinking=1;
      if (made_predicted_move) value=last_search_value;
      else {
        if (whisper || kibitz) strcpy(whisper_text,"n/a");
        last_pv.path_iteration_depth=0;
        last_pv.path_length=0;
        display=search;
        value=Iterate(wtm,think);
        avoid_pondering=0;
      }
/*
 ----------------------------------------------------------
|                                                          |
|   we've now completed a search and need to do some basic |
|   bookkeeping, and then use the LearnBook() function to  |
|   update the book if needed.                             |
|                                                          |
 ----------------------------------------------------------
*/
      last_pv=pv[0];
      last_value=value;
      if (abs(last_value) > (MATE-100)) last_mate_score=last_value;
      if (book_move) last_move_in_book=move_number;
      thinking=0;
      if (!last_pv.path_length) {
        if (value == -MATE) {
          over=1;
          printf("%s",Reverse());
          Print(0,"checkmate\n");
          printf("%s",Normal());
          if (xboard)
            if(wtm) printf("White mates\n");
            else printf("Black mates\n");
        }
      }
      else {
        if ((value > MATE-100) && (value < MATE-2)) {
          Print(1,"\nCrafty will mate in %d moves.\n\n",(MATE-value)/2);
          Whisper(1,0,0,(MATE-value)/2,nodes_searched+q_nodes_searched,0," ");
        }
        else if ((-value > MATE-100) && (-value < MATE-1)) {
          Print(1,"\nmated in %d moves.\n\n",(MATE+value)/2);
          Whisper(1,0,0,-(MATE+value)/2,nodes_searched+q_nodes_searched,0," ");
        }
        if (wtm) {
          if (!xboard && !ics) {
            Print(0,"\n");
            printf("%s",Reverse());
            printf("%c",audible_alarm);
            Print(0,"White(%d): %s ",move_number,
                  OutputMove(&last_pv.path[1],0,wtm));
            printf("%s",Normal());
            Print(0,"\n");
            if (autoplay) { 
              char *mv=OutputMoveICS(&last_pv.path[1]);
              fprintf(auto_file, " %c%c-%c%c", mv[0],mv[1],mv[2],mv[3]);
              if ((mv[4]!=' ') && (mv[4]!=0))
                fprintf(auto_file, "/%c", mv[4]);
              fprintf(auto_file, "\n");
              fflush(auto_file);
            }
          }
          else if (xboard) {
            if (log_file) fprintf(log_file,"White(%d): %s\n",move_number,
                                  OutputMove(&last_pv.path[1],0,wtm));
            printf("%d. ... %s\n",move_number,OutputMoveICS(&last_pv.path[1]));
          }
          else Print(0,"*%s\n",OutputMove(&last_pv.path[1],0,wtm));
        }
        else {
          if (!xboard && !ics) {
            Print(0,"\n");
            printf("%s",Reverse());
            printf("%c",audible_alarm);
            Print(0,"Black(%d): %s ",move_number,OutputMove(&last_pv.path[1],0,wtm));
            printf("%s",Normal());
            Print(0,"\n");
            if (autoplay) { 
              char *mv=OutputMoveICS(&last_pv.path[1]);
              fprintf(auto_file,"\t");
              fprintf(auto_file, "%c", mv[0]);
              fprintf(auto_file, "%c-", mv[1]);
              fprintf(auto_file, "%c", mv[2]);
              fprintf(auto_file, "%c", mv[3]);
              if ((mv[4]!=' ') && (mv[4]!=0))
                fprintf(auto_file, "/%c", mv[4]);
              fprintf(auto_file, "\n");
              fflush(auto_file);
            }
          }
          else {
            if (log_file) fprintf(log_file,"Black(%d): %s\n",move_number,
                                  OutputMove(&last_pv.path[1],0,wtm));
            printf("%d. ... %s\n",move_number,OutputMoveICS(&last_pv.path[1]));
          }
        }
        if (book_learning && last_move_in_book)
          LearnBook(wtm,last_value,last_pv.path_iteration_depth,0);
        if (value == MATE-2) {
          Print(0,"checkmate\n");
          if (xboard)
            if(wtm) printf("White mates\n");
            else printf("Black mates\n");
        }
        time_used=program_end_time-program_start_time;
        Print(1,"              time used: %s\n",DisplayTime(time_used));
        TimeAdjust(time_used,crafty);
        fseek(history_file,((move_number-1)*2+1-wtm)*10,SEEK_SET);
        fprintf(history_file,"%10s ",OutputMove(&last_pv.path[1],0,wtm));
        MakeMoveRoot(last_pv.path[1],wtm);
        if (RepetitionDraw(ChangeSide(wtm))==1) {
          Print(0,"%sgame is a draw by repetition.%s\n",Reverse(),Normal());
          value=DrawScore();
        }
        if (RepetitionDraw(ChangeSide(wtm))==2) {
          Print(0,"%sgame is a draw by the 50 move rule.%s\n",Reverse(),Normal());
          value=DrawScore();
        }
        ResignOrDraw(value,wtm);
/*
        if (log_file) DisplayChessBoard(log_file,search);
*/
/*
 ----------------------------------------------------------
|                                                          |
|   save the ponder_move from the current principal        |
|   variation, then shift it left two moves to use as the  |
|   starting point for the next search.  adjust the depth  |
|   to start the next search at the right iteration.       |
|                                                          |
 ----------------------------------------------------------
*/
        if ((last_pv.path_length > 1) && ValidMove(0,ChangeSide(wtm),last_pv.path[2])) {
          ponder_move=last_pv.path[2];
          for (i=1;i<=last_pv.path_length-2;i++) last_pv.path[i]=last_pv.path[i+2];
          last_pv.path_length=(last_pv.path_length > 2) ? last_pv.path_length-2 : 0;
          last_pv.path_iteration_depth-=2;
          if (last_pv.path_iteration_depth > last_pv.path_length)
            last_pv.path_iteration_depth=last_pv.path_length;
          if (last_pv.path_length <= 0) last_pv.path_iteration_depth=0;
        }
        else {
          last_pv.path_iteration_depth=0;
          last_pv.path_length=0;
          ponder_move=0;
        }
      }
      wtm=ChangeSide(wtm);
      if (wtm) move_number++;
      ValidatePosition(0,last_pv.path[1],"Main(2)");
      if (kibitz || whisper) {
        if ((nodes_searched+q_nodes_searched))
          Whisper(2,whisper_depth,end_time-start_time,whisper_value,
                  nodes_searched+q_nodes_searched,cpu_percent,whisper_text);
      }
    }
    for (i=0;i<4096;i++) {
      history_w[i]=history_w[i]>>8;
      history_b[i]=history_b[i]>>8;
    }
    for (i=0;i<MAXPLY;i++) {
      killer_move_count[i][0]=0;
      killer_move_count[i][1]=0;
    }
    if (mode == tournament_mode) {
      strcpy(input,"clock");
      Option(input);
      Print(0,"if clocks are wrong, use 'clock' command to adjust them\n");
    }
  }
}
