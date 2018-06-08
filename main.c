#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "types.h"
#include "function.h"
#include "data.h"
#include "evaluate.h"
#if defined(UNIX)
  #include <unistd.h>
#endif
/*
*******************************************************************************
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
*           (1) Initialize_Pawn_Masks() which produces the necessary masks to *
*           let us efficiently detect isolated, backward, passed, etc. pawns; *
*           (2) a pawn hash table to store recently computed pawn scores so   *
*           it won't be too expensive to do complex analysis;  (3) Evaluate() *
*           now evaluates pawns if the current pawn position is not in the    *
*                                                                             *
*    1.6    piece scoring added, although it is not yet complete.  also, the  *
*           search now used the familiar zero-width window search for all but *
*           the first move at the root, in order to reduce the size of the    *
*           tree to a nearly optimal size.  this means that a move that is    *
*           only one point better than the first move will "fail high" and    *
*           have to be re-searched a second time to get the true value.  if   *
*           the new best move is significantly better, it may have to be      *
*           searched a third time to increase the window enough to retain the *
*           score.                                                            *
*                                                                             *
*    1.7    replaced the old "killer" move ordering heuristic with the newer  *
*           "history" ordering heuristic.   this version uses the 12-bit key  *
*           formed by <from><to> to index into the history table.             *
*                                                                             *
*    1.8    added pondering for both PC and UNIX-based machines.  also other  *
*           improvements include the old program's algorithm that takes the   *
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
*           if the pawn can outrun the defending king and promote.            *
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
*    2.2    king safety code re-written and cleaned up.  crafty was giving    *
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
*    2.4    king safety code completely rewritten.  it now analyzes "faults"  *
*           in the king safety field and counts them as appropriate.  these   *
*           faults are then summed up and used as an index into a vector of   *
*           of values that turns them into a score in a non-linear fashion.   *
*           increasing faults  quickly "ramp up" the score to about 1/3+ of   *
*           a pawn, then additional faults slowly increase the penalty.       *
*                                                                             *
*    2.5    first check extensions added.  in the extension search (stage     *
*           between full-width and captures-only, up to two checking moves    *
*           can be included, if there are checks in the full-width part of    *
*           the search.  if only one check occurs in the full-width, then     *
*           only one check will be included in the extension phase of the     *
*           selective search.                                                 *
*                                                                             *
*    2.6    evaluation modifications attempting to cure crafty's frantic      *
*           effort to develop all its pieces without giving a lot of regard   *
*           to the resulting position until after the pieces are out.         *
*                                                                             *
*    2.7    new evaluation code to handle the "outside passed pawn" concept.  *
*           crafty now understands that a passed pawn on the side away from   *
*           the rest of the pawns is a winning advantage due to decoying the  *
*           king away from the pawns to prevent the passer from promoting.    *
*           the advantage increases as material is removed from the board.    *
*                                                                             *
*    3.0    the 3.* series of versions will primarily be performance en-      *
*           hancements.  the first version (3.0) has a highly modified        *
*           version of Make_Move() that tries to do no unnecessary work.  it  *
*           is about 25% faster than old version of Make_Move() which makes   *
*           Crafty roughly 10% faster.  also calls to Mask() have been        *
*           replaced by constants (which are replaced by calls to Mask() on   *
*           the Crays for speed) to eliminate function calls.                 *
*                                                                             *
*    3.1    significantly modified king safety again, to better detect and    *
*           react to king-side attacks.  crafty now uses the recursive null-  *
*           move search to depth-2 instead of depth-1, which results in       *
*           slightly improved speed.                                          *
*                                                                             *
*    3.2    null-move restored to depth-1.  depth-1 proved unsafe as Crafty   *
*           was overlooking tactical moves, particularly against it, which    *
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
*           is updated by Make_Move().  when this number reaches 100 (which   *
*           in plies is 50 moves) Repetition_Check() will return the draw     *
*           score immediately, just as though the position was a repetition   *
*           draw.                                                             *
*                                                                             *
*    3.6    search extensions cleaned up to avoid excessive extensions which  *
*           producing some wild variations, but which was also slowing things *
*           down excessively.                                                 *
*                                                                             *
*    3.7    endgame strategy added.  two specifics: KBN vs K has a piece/sq   *
*           table that will drive the losing king to the correct corner.  for *
*           positions with no pawns, scoring is altered to drive the losing   *
*           king to the edge and corner for mating purposes.                  *
*                                                                             *
*    3.8    hashing strategy modified.  crafty now stores search value or     *
*           bound, *and* positional evaluation in the transposition table.    *
*           this avoids about 10-15% of the "long" evaluations during the     *
*           middlegame, and avoids >75% of them in endgames.                  *
*                                                                             *
*    4.0    evaluation units changed to "millipawns" where a pawn is now      *
*           1000 rather than the prior 100 units.  this provides more         *
*           "resolution" in the evaluation and will let crafty have large     *
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
*    4.5    book move selection algorithm replaced.  crafty now counts the    *
*           number of times each book move was played as the book file is     *
*           created.  since these moves come (typically) from GM games, the   *
*           more frequently a move appears, the more likely it is to lead to  *
*           a sound position.  crafty then enumerates all possible book moves *
*           for the current position, computes a probability distribution for *
*           each move so that crafty will select a move proportional to the   *
*           number of times it was played in GM games (for example, if e4 was *
*           played 55% of the time, then crafty will play e4 55% of the time) *
*           although the special case of moves played too infrequently is     *
*           handled by letting the operator set a minimum threshold (say 5)   *
*           crafty won't play moves that are not popular (or are outright     *
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
*    5.3    performance improvement produced by avoiding calls to Make_Move() *
*           when the attack information is not needed.  since the first test  *
*           done in Quiesce() is to see if the material score is so bad that  *
*           a normal evaluation and/or further search is futile, this test    *
*           has been moved to inside the loop *before* Quiesce() is           *
*           recursively called, avoiding the Make_Move() work only to         *
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
*           null-move to depth-1 (R=1) rather than depth-1 (R=2) which seemed *
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
*    5.8    Evaluate_Outside_Passed_Pawns() fixed to correctly evaluate those *
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
*    5.13   optimization to avoid doing a call to Make_Move() if it can be    *
*           avoided by noting that the move is not good enough to bring the   *
*           score up to an acceptable level.                                  *
*                                                                             *
*    5.14   artificially isolated pawns recognized now, so that crafty won't  *
*           push a pawn so far it can't be defended.  also, the bonus for a   *
*           outside passed pawn was nearly doubled.                           *
*                                                                             *
*    5.15   passed pawns supported by a king, or connected passed pawns now   *
*           get a large bonus as they advance.  minor fix to the ICC resume   *
*           feature to try to avoid losing games on time.                     *
*                                                                             *
*    6.0    converted to rotated bitboards to make attack generation *much*   *
*           faster.  Make_Move() now can look up the attack bit vectors       *
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
*           normally searches, but avoiding Make_Move() calls when possible,  *
*           and (2) not searching a move near the horizon if the material     *
*           score is hopeless.                                                *
*                                                                             *
*    6.3    null-move code moved from Next_Move() directly into Search().     *
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
*    7.2    major clean-up of Make_Move() using macros to make it easier to   *
*           read and understand.  ditto for other modules as well.  fixed a   *
*           bug in Evaluate() where bishop scoring failed in endgame          *
*           situations, and discouraged king centralization.                  *
*                                                                             *
*    7.3    Evaluate() is no longer called if material is too far outside the *
*           current alpha/beta window.  the time-consuming part of Evaluate() *
*           is no longer done if the major scoring contributors in Evaluate() *
*           haven't pulled the score within the alpha/beta window, and the    *
*           remainder of Evaluate() can't possible accomplish this either.    *
*           gross error in Evaluate_Pawns() fixed, which would "forget" all   *
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
*    7.6    null move search now searches to with R=2, unless within 3 plies  *
*           of the quiescence search, then it searches nulls with R=1.        *
*                                                                             *
*    8.0    re-vamp of evaluation, bringing scores back down so that crafty   *
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
*    8.2    Futility() removed once and for all.  since Make_Move() is so     *
*           fast after the new attack generation algorithm, this was actually *
*           slower than not using it.                                         *
*                                                                             *
*    8.3    Evaluate_Pawns() weak pawn analysis bug fixed.  other minor       *
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
*           moves can be pumped into crafty in many ways so that "on the fly" *
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
*           crafty "ignore" them too much.                                    *
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
*******************************************************************************
*/
void main(int argc, char **argv)
{
  int move;
  int value=0, displayed, i;
  unsigned int tu;
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
    if (!strcmp(argv[1],"c"))
      Initialize(1);
    else
      Initialize(0);
  }
  else
    Initialize(0);
  if (argc > 1)
    for (i=1;i<argc;i++)
      if (strcmp(argv[i],"c"))
        (void) Option(argv[i]);

  if (!ics)
    Print(0,"\nCrafty v%s\n\n",version);
  else
    Print(1,"\nCrafty v%s\n\n",version);

  while (1) {
/*
 ----------------------------------------------------------
|                                                          |
|   prompt user and read input string for processing.  as  |
|   long as Option() returns a non-zero value, continue    |
|   reading lines and calling Option().  when Option()     |
|   fails to recogize a command, then try Input_Move() to  |
|   determine if this is a legal move.                     |
|                                                          |
 ----------------------------------------------------------
*/
    opponent_start_time=Get_Time(elapsed);
    made_predicted_move=0;
    ponder_completed=0;
    do {
      do {
        displayed=0;
        if (do_ponder && !ponder_completed && !over)
          Ponder(wtm);
        if ((!do_ponder || over || !ponder_move || 
             ponder_completed) && !made_predicted_move) {
          if (!ics) {
            if (wtm) {
              printf("White(%d): ",move_number);
            }
            else {
              printf("Black(%d): ",move_number);
            }
          }
          fflush(stdout);
          fscanf(input_stream,"%s",input);
          displayed=1;
          if (!strcmp(input,".") && ponder_move) {
            made_predicted_move=1;
            opponent_end_time=Get_Time(elapsed);
          }
        }
        if (wtm) {
          if (!displayed && !ics)
            Print(0,"White(%d): %s\n",move_number,input);
          else
            if (log_file) fprintf(log_file,"White(%d): %s\n",move_number,input);
        }
        else {
          if (!displayed && !ics)
            Print(0,"Black(%d): %s\n",move_number,input);
          else
            if (log_file) fprintf(log_file,"Black(%d): %s\n",move_number,input);
        }
        if (made_predicted_move) break;
        opponent_end_time=Get_Time(elapsed);
      } while (Option(input));
      if (made_predicted_move) {
        move=ponder_move;
        break;
      }
      if (strcmp(input,"move") && strcmp(input,"go"))
        if (!ics)
          move=Input_Move(input,0,wtm,0);
        else {
          move=Input_Move_ICS(input,0,wtm,1);
          if (!move) {
            move=Input_Move_ICS(input,0,!wtm,0);
            if (move) wtm=!wtm;
          }
        }
    } while (!move && 
             strcmp(input,"move") &&
             strcmp(input,"go"));
/*
 ----------------------------------------------------------
|                                                          |
|   make the move (using internal form returned by         |
|   Input_Move() and then complement wtm (white to move).  |
|                                                          |
 ----------------------------------------------------------
*/
    if(strcmp(input,"move") && strcmp(input,"go")) {
      fseek(history_file,((move_number-1)*2+1-wtm)*10,SEEK_SET);
      fprintf(history_file,"%10s ",Output_Move(&move,0,wtm));
      if (ics)
        if (wtm)
          printf("%d. %s\n",move_number,Output_Move_ICS(&move));
        else
          printf("%d. %s\n",move_number,Output_Move_ICS(&move));
      Make_Move_Root(move,wtm);
      if (Repetition_Draw()) Print(0,"game is a draw by repetition\n");
      Resign_or_Draw(value);
      wtm=!wtm;
      if (wtm) move_number++;
      tu=opponent_end_time-opponent_start_time;
      Print(1,"              time used: %s\n",Display_Time(tu));
    }
    else
      position[1]=position[0];
    Validate_Position(0);
    if (!force) {
      if (ponder_completed) {
        if((From(ponder_move) == From(move)) &&
           (To(ponder_move) == To(move)) &&
           (Piece(ponder_move) == Piece(move)) &&
           (Captured(ponder_move) == Captured(move)) &&
           (Promote(ponder_move) == Promote(move))) {
          made_predicted_move=1;
        }
      }
      ponder_move=0;
      thinking=1;
      if (made_predicted_move)
        value=last_search_value;
      else {
        if (whisper || kibitz)
          strcpy(whisper_text,"n/a");
        pv[0].path_iteration_depth=0;
        value=Iterate(wtm);
      }
      thinking=0;
      if (!pv[0].path_length) {
        over=1;
        printf("%s",Reverse());
        if (-value == MATE-1) {
          Print(0,"checkmate\n");
          if (ics)
            if(wtm) printf("White mates\n");
            else printf("Black mates\n");
        }
      }
      else {
        if ((value > MATE-100) && (value < MATE-2)) {
          Print(1,"\nCrafty will mate in %d moves.\n\n",(MATE-value)/2);
          if (kibitz)
            printf("kibitz Crafty will mate in %d moves.\n\n",(MATE-value)/2);
          else if (whisper)
            printf("whisper Crafty will mate in %d moves.\n\n",(MATE-value)/2);
        }
        else if ((-value > MATE-100) && (-value < MATE-1)) {
          Print(1,"\nCrafty will be mated in %d moves.\n\n",(MATE+value)/2);
          if (whisper)
            printf("whisper Crafty will be mated in %d moves.\n\n",
                   (MATE+value)/2);
        }
        if (wtm) {
          if (!ics) {
            Print(0,"\n");
            printf("%s",Reverse());
            printf("%s",audible_alarm);
            Print(0,"White(%d): %s",move_number,
                  Output_Move(&pv[1].path[1],0,wtm));
            printf("%s",Normal());
            Print(0,"\n");
          }
          else {
            if (log_file) fprintf(log_file,"White(%d): %s\n",move_number,
                                  Output_Move(&pv[1].path[1],0,wtm));
            printf("%d. ... %s\n",move_number,
                   Output_Move_ICS(&pv[1].path[1]));
          }
        }
        else {
          if (!ics) {
            Print(0,"\n");
            printf("%s",Reverse());
            printf("%s",audible_alarm);
            Print(0,"Black(%d): %s",move_number,
                   Output_Move(&pv[1].path[1],0,wtm));
            printf("%s",Normal());
            Print(0,"\n");
          }
          else {
            if (log_file) fprintf(log_file,"Black(%d): %s\n",move_number,
                                  Output_Move(&pv[1].path[1],0,wtm));
            printf("%d. ... %s\n",move_number,
                   Output_Move_ICS(&pv[1].path[1]));
          }
        }
        if (value == MATE-2) {
          Print(0,"checkmate\n");
          if (ics)
            if(wtm) printf("White mates\n");
            else printf("Black mates\n");
        }
        tu=program_end_time-program_start_time;
        Print(1,"              time used: %s\n",Display_Time(tu));
        Time_Adjust(tu);
        fseek(history_file,((move_number-1)*2+1-wtm)*10,SEEK_SET);
        fprintf(history_file,"%10s ",
                Output_Move(&pv[1].path[1],0,wtm));
        Make_Move_Root(pv[1].path[1],wtm);
        if (Repetition_Draw()) {
          Print(0,"%sgame is a draw by repetition%s\n",Reverse(),Normal());
        }
        Resign_or_Draw(value);
        if (log_file)
          Display_Chess_Board(log_file,position[0].board);
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
        if ((pv[1].path_length > 1) &&
            Valid_Move(0,!wtm,pv[1].path[2])) {
          ponder_move=pv[1].path[2];
          for (i=1;i<=pv[1].path_length-2;i++)
            pv[1].path[i]=pv[1].path[i+2];
          pv[1].path_length-=2;
          pv[1].path_iteration_depth-=2;
          if (pv[1].path_iteration_depth >
              pv[1].path_length)
            pv[1].path_iteration_depth=
              pv[1].path_length;
          if (pv[1].path_length <= 0)
            pv[1].path_iteration_depth=0;
        }
        else {
          pv[1].path_iteration_depth=0;
          ponder_move=0;
        }
        pv[0]=pv[1];
      }
      wtm=!wtm;
      if (wtm) move_number++;
      Validate_Position(0);
      if (kibitz > 1) {
        if (nodes_searched)
          printf("kibitz depth:%d  score:%s  nodes:%d  nps:%d\n",
                 whisper_depth,Display_Evaluation(whisper_value),
                 nodes_searched,nodes_per_second);
        if (kibitz == 3) {
          if (nodes_searched)
            printf("kibitz pv:%s\n",whisper_text);
          else
            printf("kibitz book move\n");
        }
      }
      else {
        if (whisper && nodes_searched)
          printf("whisper depth:%d  score:%s  nodes:%d  nps:%d\n",
                 whisper_depth,Display_Evaluation(whisper_value),
                 nodes_searched,nodes_per_second);
        if (whisper == 2) {
          if (nodes_searched)
            printf("whisper pv:%s\n",whisper_text);
          else
            printf("whisper book move\n");
        }
      }
      fflush(stdout);
    }
    for (i=0;i<4096;i++) {
      history_w[i]=history_w[i]>>8;
      history_b[i]=history_b[i]>>8;
    }
    for (i=0;i<MAXPLY;i++) {
      killer_move_count[i][0]=0;
      killer_move_count[i][1]=0;
    }
  }
}
