#include "../test.h"
#include "../util.h"
#include "../../search/vcf_candidate_finder.h"
#include "../../search/vcf_replay.h"
#include <algorithm>
#include <iomanip>
#include <random>
#include <sstream>

class VCFCandidateFinderTest : public TestBase {

PRIVATE
    string posToString(Pos pos) {
        if (pos.isDefault()) {
            return "(none)";
        }

        string text;
        text += static_cast<char>(pos.getY() + 96);
        text += to_string(pos.getX());
        return text;
    }

    string movesToString(const MoveList& moves) {
        if (moves.empty()) {
            return "(none)";
        }

        ostringstream oss;
        for (size_t i = 0; i < moves.size(); ++i) {
            if (i != 0) {
                oss << ", ";
            }
            oss << posToString(moves[i]);
        }
        return oss.str();
    }

    void printProbeResults(const VCFCandidateFinder& finder) {
        for (const VCFCandidateProbeResult& probe : finder.getLastProbeResults()) {
            TEST_PRINT("  " << posToString(probe.move)
                << " | createsVCF=" << (probe.createsVCF ? "Y" : "N")
                << " | stopped=" << (probe.stopped ? "Y" : "N")
                << " | nodes=" << probe.nodeCount
                << " | time=" << fixed << setprecision(3) << (probe.elapsedTime * 1000.0) << " ms"
                << " | vcf=" << convertPath2String(probe.vcfPath));
        }
    }

    double elapsedSeconds(std::chrono::high_resolution_clock::time_point start,
        std::chrono::high_resolution_clock::time_point end) {
        const auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        return duration.count() / 1e9;
    }

    size_t sumProbeNodes(const VCFCandidateFinder& finder) {
        size_t total = 0;
        for (const VCFCandidateProbeResult& probe : finder.getLastProbeResults()) {
            total += probe.nodeCount;
        }
        return total;
    }

    void knownVCFPositionsProduceCandidateTest() {
        const string processArr[] = {
            "h8h9i8i9g9g8j9i7j8j10h5h7"
        };

        VCFCandidateFinderOptions options;
        options.perMoveTimeLimitSeconds = 1.0;

        for (const string& process : processArr) {
            Board board = getBoard(process);

            const auto initStart = std::chrono::high_resolution_clock::now();
            VCFCandidateFinder finder(board, options);
            const auto initEnd = std::chrono::high_resolution_clock::now();
            const double ttInitSeconds = elapsedSeconds(initStart, initEnd);

            TEST_PRINT("process: " << process);
            printBoard(board);
            TEST_PRINT("");

            const auto searchStart = std::chrono::high_resolution_clock::now();
            MoveList creators = finder.findFromEvaluatorCandidates();
            const auto searchEnd = std::chrono::high_resolution_clock::now();
            const double candidateSearchSeconds = elapsedSeconds(searchStart, searchEnd);
            const double ttMemoryMB = finder.getEstimatedMemoryBytes() / (1024.0 * 1024.0);
            const VCFCandidateProbeResult& rootProbe = finder.getRootProbeResult();

            TEST_PRINT("Existing root VCF: " << (finder.rootAlreadyHasVCF() ? "Y" : "N")
                << " | nodes=" << rootProbe.nodeCount
                << " | time=" << fixed << setprecision(3) << (rootProbe.elapsedTime * 1000.0) << " ms"
                << " | vcf=" << convertPath2String(rootProbe.vcfPath));
            TEST_PRINT("VCF creators: " << movesToString(creators));
            TEST_PRINT("TT init time: "
                << fixed << setprecision(3) << (ttInitSeconds * 1000.0) << " ms"
                << " | TT memory: " << ttMemoryMB << " MB");
            TEST_PRINT("Root-check + candidate VCF search time: "
                << fixed << setprecision(3) << (candidateSearchSeconds * 1000.0) << " ms"
                << " | candidate visited nodes: " << sumProbeNodes(finder)
                << " | cached TT entries: " << finder.getCachedNodeCount());
            printProbeResults(finder);

            TEST_ASSERT(finder.rootAlreadyHasVCF() || !creators.empty());
        }
    }

    // white-first target puzzle after n11: white threatens this VCF if black passes
    const string N11_THREAT = "h8g8h7h6i8i9j8j9k9k10i10j10i11j11i12l11k11l10j13i7j6j5k6n11";
    const string N11_VCF = "n10m10n13m12n12n9n14";
    // black to move; white's four at g11 or l6 forces black onto the 4-4 forbidden j8
    const string FORBIDDEN_TRAP = "h8h9i8i9j9i10j10k10j11j12g8e8i13h10k13k7";
    const string GAP_SEED = "h8j8j6j7i7h7k5l4j9g9k7f9k6h6k4k3l3j10k10k8i6h4i3h3h5l6j5m2";

    MoveList parseMoves(const string& moves) {
        MoveList result;
        for (const auto& p : processString(moves)) {
            result.push_back(Pos(p.first, p.second));
        }
        return result;
    }

    // full VCF search for the side to move, independent of the replay under test
    bool sideToMoveHasVCF(Board& board, MoveList* line = nullptr) {
        VCFCandidateFinderOptions options;
        options.ttBytes = 1ull * 1024ull * 1024ull;
        VCFCandidateFinder finder(board, options);
        finder.findFromCandidates(MoveList());
        if (line != nullptr) {
            *line = finder.getRootProbeResult().vcfPath;
        }
        return finder.rootAlreadyHasVCF();
    }

    bool containsMove(const CandidateList& moves, const Pos& move) {
        return std::find(moves.begin(), moves.end(), move) != moves.end();
    }

    void replayKeepsKnownVCFTest() {
        Board board = getBoard(N11_THREAT);
        const MoveList line = parseMoves(N11_VCF);
        board.pass();

        const uint64_t hashBefore = board.getCurrentHash();
        const size_t pathBefore = board.getPath().size();
        TEST_ASSERT(replayVCFWins(board, line));
        TEST_ASSERT(board.getCurrentHash() == hashBefore);
        TEST_ASSERT(board.getPath().size() == pathBefore);

        // blocks come from the board, so recorded replies do not matter
        TEST_ASSERT(replayVCFWins(board, parseMoves("n10a1n13a1n12a1n14")));
        board.undo();

        // a black stone on the line breaks it; one far away does not
        Board blocked = board;
        blocked.move(parseMoves("n12").front());
        TEST_ASSERT(!replayVCFWins(blocked, line));

        Board far = board;
        far.move(parseMoves("a1").front());
        TEST_ASSERT(replayVCFWins(far, line));
    }

    void replayWinsThroughForbiddenBlockTest() {
        Board board = getBoard(FORBIDDEN_TRAP);
        board.pass();

        MoveList line;
        TEST_ASSERT(sideToMoveHasVCF(board, &line));
        TEST_PRINT("white VCF after black pass: " << convertPath2String(line));
        TEST_ASSERT(replayVCFWins(board, line));
    }

    // every defender move left out of the refutation set must still lose to a VCF
    void checkRefutationsComplete(const string& process, const MoveList& line) {
        Board board = getBoard(process);
        CandidateList refutations;
        collectVCFRefutations(board, line, refutations);

        TEST_PRINT("process: " << process);
        TEST_PRINT("line: " << convertPath2String(line));
        TEST_PRINT("refutations (" << refutations.size() << "): " << movesToString(refutations.toMoveList()));

        const bool defenderIsBlack = board.isBlackTurn();
        for (size_t i = 0; i < line.size(); i += 2) {
            if (defenderIsBlack && board.isForbidden(line[i])) {
                continue;
            }
            TEST_ASSERT(containsMove(refutations, line[i]));
        }

        int verified = 0;
        for (int x = 1; x <= BOARD_SIZE; ++x) {
            for (int y = 1; y <= BOARD_SIZE; ++y) {
                const Pos move(x, y);
                if (board.getCell(move).getPiece() != EMPTY) continue;
                if (defenderIsBlack && board.isForbidden(move)) continue;
                if (containsMove(refutations, move)) continue;

                board.move(move);
                TEST_ASSERT(sideToMoveHasVCF(board));
                board.undo();
                ++verified;
            }
        }
        TEST_PRINT("left-out moves verified by independent VCF search: " << verified);
    }

    void refutationsAreCompleteTest() {
        checkRefutationsComplete(N11_THREAT, parseMoves(N11_VCF));

        Board trap = getBoard(FORBIDDEN_TRAP);
        trap.pass();
        MoveList trapLine;
        TEST_ASSERT(sideToMoveHasVCF(trap, &trapLine));
        checkRefutationsComplete(FORBIDDEN_TRAP, trapLine);
    }

    // Open threes and 4-3s whose defense the pattern-based sets used to miss: the replay
    // set must hold the missed blocks, and every move it leaves out must lose to a VCF.
    void threatDefenseCoversMissedBlocksTest() {
        struct Case { string process; string mustRefute; };
        const Case cases[] = {
            { GAP_SEED + "a15n7j3", "i5j4" },   // 4-3 at i4 / l5; j4 turns the block i5 into an open four
            { GAP_SEED + "a15h9i9", "i5" },     // open three i6-i7-_-i9; i5 is black's overline point
            { "h8g7k8g9f7g6g5k7g8d9g10j7e8", "i8" },  // open three e8-_-g8-h8; i8 is black's 4-4 point
        };
        for (const Case& c : cases) {
            Board board = getBoard(c.process);
            MoveList line;
            TEST_ASSERT(findThreatLine(board, line));
            CandidateList refutations;
            collectVCFRefutations(board, line, refutations);
            for (const Pos& move : parseMoves(c.mustRefute)) {
                TEST_ASSERT(containsMove(refutations, move));
            }
            checkRefutationsComplete(c.process, line);
        }
    }

    // Random positions near known threats: wherever the side to move faces a VCF or an
    // open three / 4-3 line, the filtered scan must keep every move the full scan refutes.
    void refutationFilterMatchesFullScanTest() {
        const string seeds[] = {
            N11_THREAT,
            FORBIDDEN_TRAP,
            "h8g8h7h6i8i9j8j9k9k10i10j10i11j11i12l11k11l10j13i7j6j5k6",
            "h8g9h10h5h6g8g7f6i7i5l7a1d7",
            "h8h9i8g8i10i9j9k10j7i7",
            "h8h9g7i9g9i7g10i8i10h10e8f9d8j7",
            "h8h7i7i8j7j8j9i9h9h11i10g7i6f7j6k5i5h4k7l9k8m7l7m6",
            "h8h7h10h11f6g7f7f8h6i7j7g6j9",
            GAP_SEED + "a15n7",
            GAP_SEED + "a15h9",
            "h8g7k8g9f7g6g5k7g8d9g10j7",
        };

        std::mt19937 rng(20260926);
        int positions = 0;
        int threats = 0;
        int filtered = 0;
        int missing = 0;

        // full-scan refutations must all survive the filter
        auto compare = [&](Board& board, const MoveList& line) {
            ++threats;
            std::array<uint8_t, 256> mask;
            if (markVCFRefutationCandidates(board, line, mask)) ++filtered;

            CandidateList fast;
            CandidateList full;
            collectVCFRefutations(board, line, fast, true);
            collectVCFRefutations(board, line, full, false);
            bool kept = true;
            for (const Pos& move : full) {
                if (!containsMove(fast, move)) kept = false;
            }
            if (!kept) {
                ++missing;
                TEST_PRINT("filter dropped a refutation at " << convertPath2String(board.getPath())
                    << " line " << convertPath2String(line));
            }
            TEST_ASSERT(kept);
        };
        for (const string& seed : seeds) {
            for (int trial = 0; trial < 150; ++trial) {
                Board board = getBoard(seed);
                const int extra = static_cast<int>(rng() % 6);
                for (int k = 0; k < extra && board.getResult() == ONGOING; ++k) {
                    // a random empty cell within two cells of some stone
                    MoveList near;
                    for (int x = 1; x <= BOARD_SIZE; ++x) {
                        for (int y = 1; y <= BOARD_SIZE; ++y) {
                            if (board.getCell(x, y).getPiece() != EMPTY) continue;
                            bool close = false;
                            for (int ddx = -2; ddx <= 2 && !close; ++ddx) {
                                for (int ddy = -2; ddy <= 2 && !close; ++ddy) {
                                    const int nx = x + ddx;
                                    const int ny = y + ddy;
                                    if (!isBoardCoord(nx, ny)) continue;
                                    const Piece piece = board.getCell(nx, ny).getPiece();
                                    close = piece == BLACK || piece == WHITE;
                                }
                            }
                            if (close) near.push_back(Pos(x, y));
                        }
                    }
                    if (near.empty()) break;
                    const Pos move = near[rng() % near.size()];
                    if (board.isBlackTurn() && board.isForbidden(move)) continue;
                    board.move(move);
                }
                if (board.getResult() != ONGOING) continue;
                ++positions;

                MoveList threatLine;
                if (findThreatLine(board, threatLine)) {
                    compare(board, threatLine);
                }
                checkOpenFourDefense(board);

                Board probe = board;
                if (!probe.pass()) continue;
                MoveList line;
                if (!sideToMoveHasVCF(probe, &line) || !replayVCFWins(probe, line)) continue;
                compare(board, line);
            }
        }
        TEST_PRINT("positions: " << positions << ", lines checked: " << threats
            << ", filtered: " << filtered << ", refutations dropped: " << missing
            << ", open-four defenses checked: " << openFourChecks);
        TEST_ASSERT(filtered > 0);
        TEST_ASSERT(openFourChecks > 0);
    }

    int openFourChecks = 0;

    // The direct open-three defense must match a real open-four move: its five points are
    // the ones the board shows after the move, and it keeps every move the full scan refutes.
    void checkOpenFourDefense(Board& board) {
        CandidateList defense;
        if (!collectOpenFourDefense(board, defense)) return;
        ++openFourChecks;

        const Piece attacker = board.isBlackTurn() ? WHITE : BLACK;
        const Pos openFour = board.getFirstPatternPos(attacker, MATE);
        CandidateList predicted;
        board.forEachFivePointAfter(openFour, attacker, [&](const Pos& p) { predicted.push_back(p); });
        Board played = board;
        played.pass();
        played.move(openFour);
        CandidateList actual;
        played.getPatternBucket(attacker, WINNING).forEach([&](const Pos& p) { actual.push_back(p); });
        bool sameFives = predicted.size() == actual.size();
        for (const Pos& p : predicted) sameFives = sameFives && containsMove(actual, p);
        TEST_ASSERT(sameFives);

        CandidateList full;
        collectVCFRefutations(board, MoveList(1, openFour), full, false);
        bool kept = true;
        for (const Pos& move : full) kept = kept && containsMove(defense, move);
        if (!kept) {
            TEST_PRINT("open-four defense dropped a refutation at " << convertPath2String(board.getPath()));
        }
        TEST_ASSERT(kept);
    }

PUBLIC
    VCFCandidateFinderTest() {
        registerTestMethod("known_vcf_positions_produce_candidate", [this]() { knownVCFPositionsProduceCandidateTest(); });
        registerTestMethod("vcf_replay_keeps_known_vcf", [this]() { replayKeepsKnownVCFTest(); });
        registerTestMethod("vcf_replay_wins_through_forbidden_block", [this]() { replayWinsThroughForbiddenBlockTest(); });
        registerTestMethod("vcf_refutations_are_complete", [this]() { refutationsAreCompleteTest(); });
        registerTestMethod("threat_defense_covers_missed_blocks", [this]() { threatDefenseCoversMissedBlocksTest(); });
        registerTestMethod("vcf_refutation_filter_matches_full_scan", [this]() { refutationFilterMatchesFullScanTest(); });
    }
};

int main() {
    VCFCandidateFinderTest test;
    test.runAllTests();

    return 0;
}
