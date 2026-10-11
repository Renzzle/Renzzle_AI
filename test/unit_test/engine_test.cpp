#include "../util.h"
#include "../test.h"
#include "../../engine/engine.h"
#include <algorithm>
#include <iomanip>
#include <sstream>

class EngineTest : public TestBase {

PRIVATE
    struct PuzzleCase {
        string process;
        bool expectWin = true;   // true: validatePuzzle should return a non-empty path
        string note;             // optional label/comment shown in output
    };

    struct NextMoveCase {
        string process;
        string expectedMove;     // optional ("h8" form); empty = print-only, no assertion
        string note;
    };

    string formatSeconds(double seconds) {
        ostringstream oss;
        oss << fixed << setprecision(3) << seconds << " s";
        return oss.str();
    }

    string intToMoveString(int move) {
        if (move < 0) return "(none)";
        int x = (move % BOARD_SIZE) + 1;
        int y = (move / BOARD_SIZE) + 1;
        string s;
        s += static_cast<char>(y + 96);
        s += to_string(x);
        return s;
    }

    string posToMoveString(const Pos& move) {
        if (move.isDefault()) return "(none)";
        string s;
        s += static_cast<char>(move.getY() + 96);
        s += to_string(move.getX());
        return s;
    }

    string valueTypeToString(Value::Type type) {
        if (type == Value::Type::EXACT) return "EXACT";
        if (type == Value::Type::LOWER_BOUND) return "LOWER_BOUND";
        if (type == Value::Type::UPPER_BOUND) return "UPPER_BOUND";
        return "UNKNOWN";
    }

    string valueToString(Value value) {
        ostringstream oss;
        if (value.isWin()) {
            oss << "WIN(" << value.getResultDepth() << ")";
        } else if (value.isLose()) {
            oss << "LOSE(" << value.getResultDepth() << ")";
        } else if (value.getType() == Value::Type::UNKNOWN) {
            oss << "UNKNOWN";
        } else {
            oss << "ONGOING(" << value.getValue() << ")";
        }
        oss << " [" << valueTypeToString(value.getType()) << "]";
        if (value.isQVCFDerived()) oss << " QVCF";
        return oss.str();
    }

    string pathToString(const MoveList& path) {
        if (path.empty()) return "(none)";
        return convertPath2String(path);
    }

    string judgementReason(const FindNextMoveAnalysis& analysis) {
        Value value = analysis.value;
        if (analysis.usedSureMove) {
            return "Evaluator::getSureMove() returned an immediate forced move";
        }
        if (analysis.usedFallback) {
            return "no completed search PV; used first Evaluator::getCandidates() fallback";
        }
        if (value.isWin()) {
            return "DEFENSIVE search found a winning line for side to move";
        }
        if (value.isLose()) {
            return "all searched root candidates lose; selected the candidate with the largest LOSE depth";
        }
        if (value.getType() == Value::Type::UNKNOWN) {
            return "search ended before a completed result was available";
        }
        return "no forced result within completed depth; selected best tactical search value";
    }

    void printSelectedRootStat(const FindNextMoveAnalysis& analysis) {
        if (analysis.path.empty()) return;

        const Pos selected = analysis.path.front();
        for (const auto& stat : analysis.rootStats) {
            if (!(stat.move == selected)) continue;
            TEST_PRINT("selected root: "
                << posToMoveString(stat.move)
                << " -> " << valueToString(stat.value)
                << ", pv=" << pathToString(stat.pv)
                << ", nodes=" << stat.nodeCount
                << ", time=" << formatSeconds(stat.elapsedTime));
            return;
        }

        TEST_PRINT("selected root: "
            << posToMoveString(selected)
            << " -> " << valueToString(analysis.value)
            << ", pv=" << pathToString(analysis.path)
            << " (root candidate stats unavailable; result came from completed PV/TT)");
    }

    void printRootStatsPreview(const FindNextMoveAnalysis& analysis) {
        if (analysis.rootStats.empty()) return;

        const size_t limit = min<size_t>(analysis.rootStats.size(), 8);
        TEST_PRINT("root candidates (" << analysis.rootStats.size() << ", first " << limit << "):");
        for (size_t i = 0; i < limit; ++i) {
            const auto& stat = analysis.rootStats[i];
            TEST_PRINT("  [" << stat.order << "] "
                << posToMoveString(stat.move)
                << " -> " << valueToString(stat.value)
                << ", pv=" << pathToString(stat.pv));
        }
    }

    void validatePuzzleTest(const PuzzleCase& tc) {
        TEST_PRINT("==================================");
        if (!tc.note.empty()) TEST_PRINT("[CASE] " << tc.note);
        TEST_PRINT("process: " << tc.process);
        TEST_PRINT("expect:  " << (tc.expectWin ? "WIN" : "NO-WIN"));

        Board board = getBoard(tc.process);
        printBoard(board);
        TEST_PRINT("");

        auto startTime = chrono::high_resolution_clock::now();
        string solution = validatePuzzle(tc.process);
        auto endTime = chrono::high_resolution_clock::now();
        double seconds = chrono::duration_cast<chrono::nanoseconds>(endTime - startTime).count() / 1e9;

        const bool foundWin = !solution.empty();
        const bool passed = (foundWin == tc.expectWin);

        TEST_PRINT("elapsed: " << formatSeconds(seconds));
        if (foundWin) {
            TEST_PRINT("solution: " << solution);
        } else {
            TEST_PRINT("solution: (none within budget)");
        }
        if (passed) {
            TEST_PRINT("[PASS]");
        } else {
            TEST_PRINT("[FAIL] expected " << (tc.expectWin ? "WIN" : "NO-WIN")
                << " but got " << (foundWin ? "WIN" : "NO-WIN"));
        }
        TEST_ASSERT(passed);
        TEST_PRINT("==================================");
    }

    // cases with an empty process are unfilled slots
    void runSuite(const vector<PuzzleCase>& cases, const string& suiteName) {
        const size_t filled = count_if(cases.begin(), cases.end(),
            [](const PuzzleCase& tc) { return !tc.process.empty(); });
        TEST_PRINT("");
        TEST_PRINT("########## SUITE: " << suiteName << " (" << filled << " cases) ##########");
        for (const auto& tc : cases) {
            if (tc.process.empty()) continue;
            validatePuzzleTest(tc);
        }
    }

    void findNextMoveTest(const NextMoveCase& tc) {
        TEST_PRINT("==================================");
        if (!tc.note.empty()) TEST_PRINT("[CASE] " << tc.note);
        TEST_PRINT("process: " << tc.process);
        if (!tc.expectedMove.empty()) TEST_PRINT("expect:  " << tc.expectedMove);

        Board board = getBoard(tc.process);
        printBoard(board);
        TEST_PRINT("");

        auto startTime = chrono::high_resolution_clock::now();
        FindNextMoveAnalysis analysis = analyzeNextMove(tc.process);
        auto endTime = chrono::high_resolution_clock::now();
        double seconds = chrono::duration_cast<chrono::nanoseconds>(endTime - startTime).count() / 1e9;

        const string moveStr = intToMoveString(analysis.move);
        TEST_PRINT("elapsed:  " << formatSeconds(seconds));
        TEST_PRINT("returned: " << moveStr);
        TEST_PRINT("judgement: " << judgementReason(analysis));
        TEST_PRINT("value:    " << valueToString(analysis.value));
        TEST_PRINT("depth:    " << analysis.completedDepth
            << ", nodes=" << analysis.visitedNodes
            << ", searchTime=" << formatSeconds(analysis.elapsedSeconds));
        TEST_PRINT("pv:       " << pathToString(analysis.path));
        printSelectedRootStat(analysis);
        printRootStatsPreview(analysis);

        if (!tc.expectedMove.empty()) {
            const bool match = (moveStr == tc.expectedMove);
            if (match) {
                TEST_PRINT("[PASS]");
            } else {
                TEST_PRINT("[FAIL] expected " << tc.expectedMove << " got " << moveStr);
            }
            TEST_ASSERT(match);
        } else {
            TEST_PRINT("[INFO] no expected move; inspect output");
        }
        TEST_PRINT("==================================");
    }

    void runNextMoveSuite(const vector<NextMoveCase>& cases, const string& suiteName) {
        TEST_PRINT("");
        TEST_PRINT("########## SUITE: " << suiteName << " (" << cases.size() << " cases) ##########");
        for (const auto& tc : cases) {
            findNextMoveTest(tc);
        }
    }

PUBLIC
    EngineTest() {
        registerTestMethod("validate_puzzle_winning_cases", [this]() { testWinningCases(); });
        registerTestMethod("find_next_move_cases",          [this]() { testFindNextMoveCases(); });
    }

    // Cases that should be solvable within HARD_THINKING_TIME (20s).
    // Borrowed from search_test for parity; edit/add/remove freely.
    void testWinningCases() {
        const vector<PuzzleCase> cases = {
            // { "h8h9i8g8i10i9j9k10j7i7",            true, "search_test #1" },
            // { "h8h9i8g8i10i9j9k8k10l11i7j6",       true, "search_test #2" },
            // { "h8h9j9g8j10g7i10",                  true, "search_test #3" },
            // { "h8i9h9h10g9g8f8g7e7f7g6f6i6h6j5j6k7j8k9i10j10i11i12h11g11e8f9", true, "search_test #4" },
            // { "h8i8i9i7j8j7j6k7k6i6h7h6i5g7f8h10i10d8f6", true, "search_test #5" },
            // { "h8h9f6g7e9g8f7h6f8f9h7d6",          true, "search_test #6" },
            // { "h8h9g7i9g9i7g10i8i10h10e8f9d8j7",   true, "search_test #7" },
            // { "h8h9i8g8i10j8i6h7h6k7j6f7f8",       true, "search_test #8" },
            // { "h8h9i9i8g10h10f7g7h6h7g5i7k7j6k6k8l7j9k10", true, "search_test #9" },
            // { "h8h7i7i8j7j8j9i9h9h11i10g7i6f7j6k5i5h4k7l9k8m7l7m6", true, "search_test #10" },
            // { "h8h7i7i8j7j8j9i9h9h11i10g7i6f7j6k5i5h4k7l9k8m7l7m6k10k9", true, "search_test #11" },
            // { "h8h9i10j9k9i8j7j10j11j12g7g9g6h10i11g11i9", true, "search_test #12" },
            // { "h8i9f6g8g6h9f9f7e6d6i10j9k9j7j8i7i8l8h7f5e7d8", true, "search_test #13" },
            // { "h8h9i8g8i10i9j9k10k8l7j8l8j6j7i7k5h6g5g6i6h7f5", true, "search_test #14" },
            // { "h8h9i8g8h7i7j7j9k9l9k10l10j11k11i12i11h13g14g12f11g11f12f10e9g9i10", true, "search_test #15" },
            // VCT puzzles: fill in the process; empty slots are skipped
            { "h8h9i9j10j8i8j7i7k8l9l8j6l7k6i5j5i3i4k4j4", true, "vct #1" },
            { "h8h9i10h10g7g8e7f7g6f6g5h6h5i5h4g3g4i7i4j4", true, "vct #2" },
            { "h8h9i8i9j10i10j7j8k10k8l10l11m10n10l9m11m9k6m8n7", true, "vct #3" },
            { "h8i9i8j9k10j7l10k7i7m8l9i10j11i11i12h13g12h11j12k12j13j14", true, "vct #4" },
            { "h8h9i9i8j10k11k9l8i11i10i12h11h12g13g11h10g10g9g8g7f8f9", true, "vct #5" },
            { "h8h9i8g8h7j9j7i7k9i10j11i11i12k11k10l9h13g14g12f12g11f11f10e9g9l10", true, "vct #6" },
            { "h8h9i9i8j9j10i11j11g8i7h7f6g7g9g10f9g11g12h11e11d9e9d8f10", true, "vct #7" },
            { "h8h9i8i9j9k9i10h11h10i11h12h7k11i7g7f7i6j7j5j6", true, "vct #8" },
            { "h8g8j8i9k7k9k6l7l9l6l10l11k10j10j11m8k12k11l12j12m11i10i11j7", true, "vct #9" },
            { "h8h9i10i9h7i8h6i7i5i6g5h5g7f8f9g8f6h4e7d6d8c9", true, "vct #10" },
            { "h8h9h10i11i10j10j9k8g10i8g11f12l9j8l8l11l10m11m12j12l13", true, "vct #11" },
            { "h8i8h7g8g6h5i6f6f5f4g5e4d4c4e3e7d6d7d8f8e6", true, "vct #12" },
            { "h8h9i9j10i10i11j11k11k12k10l11m11l10l9m10j13h13i12h11k13k9", true, "vct #13" },
            { "h8h9h7i9g7g9f9g8f6f8h5h6h10i10j11j9k9h11g12j6i12g6k6", true, "vct #14" },
            { "h8i9i8h7j6j7k7k8k9m8k10i6h5i5j5i4i3k5h3", true, "vct #15" },
            { "h8i9j10j9k9i8i10j8i7k8j7j6i6h7g6l7k7m6n5", true, "vct #16" },
            { "h8h9h10i10j11i11i12k10j10j9i8k8l7l9k9l10m10l11l8k11m9m7h11", true, "vct #17" },
            { "h8i9i7g9j8k8j9j10k9l10l9k10i10k11l12m8m9n9i11j11i12j12h11i13h14g12k12", true, "vct #18" },
            { "h8h9i8i9j8k8j9j7i7i6h5h7h6k9j10k7k6m7l7i11i10f8g9f10f9g6f7", true, "vct #19" },
            { "h8i9j9i7j8j7k7k8i6h6j6l9m10k9i11i10g10h10i12j10j12", true, "vct #20" },
            { "h8h9i9i8j7i7h5g7g8g9f8f7e7d8e9e6f6g5g6e11e10f10d10g11g10d7c7", true, "vct #21*" },
            { "h8h9i9j10k9j8i10i11j11k12l12h11h10f10g10h12f11g12g13f13f12e12d12f14e14k14i13", true, "vct #22" },
            { "h8h7i8i9h9j9j8k8k9l10l8k10j10i11h11i10i12k11k7l11m10l9l13m8n7l7m6", true, "vct #23" },
            { "h8i9j9i7j7j6i6h4i5j4h6k5l6m5l4l7l8l9m8k8n6m7n4", true, "vct #24" },
            { "h8i9j9i10i11j10h10h11j11k11k10l11l9m8k9l8k7n10l10m11m12l12m13k13j14k12j12j15j13h12o9", true, "vct #25*" },
            { "h8h7h9i9g8i8f7i10i11i7i6j8j7g6j9e6k8l9k9k10k7m6k6k5j6l8i5f8", true, "vct #26" },
            { "h8h9i9i8h10j10k10j9j11k11i12h13j12h12g13i11l10g11l8m8n10m10", true, "vct #27" },
            { "h8h7i8i9h9h10j7k6k7j8i6h5j5h6g5g6g7f7e8e6f6g9e7h4h3g3", true, "vct #28*" },
            { "h8h9i9g7i8j8g9i7i10h10i11i12j10j9k9l8l10k10m11j12k12j14k13l14k7n10", true, "vct #29" },
            { "h8h9g8g9f8e8h7g7i7i8j7i6k8h6g5h5j4h2i3l4k5m7l6k9", true, "vct #30" },
            { "h8h9g8i8g10h10i10h11h12g12g11f11f12f13e13d14e12d11h13f10f9h7e8e9d9d6d7c6", true, "vct #31" },
            { "h8h9h6h7i6i7j7j6k6k7l6l7l8m7n8m8l5m4k5i5k4i3h4g4h5g6g5n9", true, "vct #32" },
            { "h8h9g7i9g9f8f9g10f10i7e9g11h10f7e8e7d8c7b7e10d11d12e12e13f13g14g12h12i12d6", true, "vct #33" },
            { "h8i9i8j8k9i7k7l8l10m9l11k10k11k12j12i13", true, "vct #34*" },
            { "h8h9h10j9i10j10j11i11k9i12h12i13k10j8l10l9m10k7j7l6m5k5", true, "vct #35" },
            { "h8h9h7i8i7i6j7k7j6g9k5l4k6l6j4j5j3i5i3h5h4f4g5f6h6f8f9g7", true, "vct #36" },
            { "h8h9i9i8i7j8j7h7g7f6g6g9g5g4h5i5j5i4f7i6e8d9d7h10", true, "vct #37" },
            { "h8h9i8j8h7g8i6i7j7j6k8l9k7k6k9j9l8m8k10k11m9m7l7g6m6h5l5l11", true, "vct #38*" },
            { "h8h9i9i10h10j10i11g9j11k11l11l12k12k13j12i8i12j8k6j7l10l9", true, "vct #39" },
            { "h8h9i8h7i9i7j7j6k9k8i10j10i11i12j11k11j12j13l10k13l9k6", true, "vct #40" },
            { "h8i9h6h7i6i7j7j6k8l9g7g8f6f7g9f8f9e7e9d9f10g11d8e10", true, "vct #41*" },
            { "h8h9i7i9g7g9j9i8j7h7k7j8j6k5k6l6m7l7l5m5n7h6i5h4f6g6", true, "vct #42" },
            { "h8h9h10i9i10j9g9j10i11j12j11g8g7f7e6f6g6e8f9e9f11f10g11h11g12g10f12e12d11f13d8c9c8d7", true, "vct #43" },
            { "h8i9h9g9j9h7i7j7g7k7m8k8i6k6k5l7l5i4j4g3h3h2i3h6j3l4k3l3", true, "vct #44*" },
            { "h8h9i9i8j8j9k10k9k8l8i7h7j6g9j7h10k7j4h5i6", true, "vct #45" },
            { "h8h9i10i8h7i7g7f7e6f6f4g6h5h6i5i6j6k7j5g5h4i4j4j3l4k4e4e5", true, "vct #46*" },
            { "h8h9j9i9i7g9j6k9i6h6i5i4k6k7j4j5k3k5j3h4g4g5m6l6m5m7l8l7n7k4", true, "vct #47*" },
            { "h8i9h9h6g9f10h10h11i11j12g7f8f6e7e6d6e8e5e9d10e11f7f12g11g13i10", true, "vct #48" },
            { "h8i9g7g8f7h7f9g9f8f10f6f5g6h6i6h5g5i5e5d4e9e7d9d10g10h11", true, "vct #49" },
            { "h8h9i8g8i10i11i9j9j8k8k9i7h7g7h6h5g9f9f8f10e8e9e6f7", true, "vct #50" },
        };
        runSuite(cases, "winning_cases");
    }

    // findNextMove cases. expectedMove is optional ("h8" form) — fill in
    // when you have a known correct answer (forced moves, VCT first move, etc.).
    // Leave empty to just inspect output.
    void testFindNextMoveCases() {
        const vector<NextMoveCase> cases = {
            // { "h8h9i8g8i10i9j9k10j7i7", "", "VCT win exists" },
            // { "h8h9j9g8j10g7i10", "", "VCT win exists" },
            // { "h8h9f6g7e9g8f7h6f8f9h7d6", "", "VCT win exists" },
            // { "h8h9i8i9j8j9g8g9f8", "", "quiet midgame" },
            // { "h8h9i8g8i10i9j9k10k8l7j8l8j6j7i7k5h6g5g6i6h7f5h5h4f7e8f8", "", "VCT lose — longest delay defense" },
            // { "h8h9i9g7i10i8g10h10h11i12g12f13j10j9g11g9i11h12f12", "", "one defend" },
            // { "h8h9i8i9j8k8h7j9k9i10h11g8f7h10j7i7", "", "one defend" },
            // { "h8h9i8g8i10i9j9k10k8", "l7", "lose but need correct defense" },
            // { "h8i7j7i8k9j9i6h7g6k10l11f6h5g4k8l9i4j3k6j5j6h6e7f7f8g9f4k4k3g7k7k5l8", "m9", "one defend" },
            // { "h8h9i8i9j9i10j10k10j11j12g8e8i13h10k13k7", "g11", "44 defend" }
        };
        runNextMoveSuite(cases, "find_next_move_cases");
    }

};

int main() {
    EngineTest engineTest;
    engineTest.runAllTests();

    return 0;
}
