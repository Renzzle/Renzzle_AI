#include "../util.h"
#include "../test.h"
#include "../../engine/engine.h"
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

    void runSuite(const vector<PuzzleCase>& cases, const string& suiteName) {
        TEST_PRINT("");
        TEST_PRINT("########## SUITE: " << suiteName << " (" << cases.size() << " cases) ##########");
        for (const auto& tc : cases) {
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
            { "h8h9i8g8i10i9j9k10j7i7",            true, "search_test #1" },
            { "h8h9i8g8i10i9j9k8k10l11i7j6",       true, "search_test #2" },
            { "h8h9j9g8j10g7i10",                  true, "search_test #3" },
            { "h8i9h9h10g9g8f8g7e7f7g6f6i6h6j5j6k7j8k9i10j10i11i12h11g11e8f9", true, "search_test #4" },
            { "h8i8i9i7j8j7j6k7k6i6h7h6i5g7f8h10i10d8f6", true, "search_test #5" },
            { "h8h9f6g7e9g8f7h6f8f9h7d6",          true, "search_test #6" },
            { "h8h9g7i9g9i7g10i8i10h10e8f9d8j7",   true, "search_test #7" },
            { "h8h9i8g8i10j8i6h7h6k7j6f7f8",       true, "search_test #8" },
            { "h8h9i9i8g10h10f7g7h6h7g5i7k7j6k6k8l7j9k10", true, "search_test #9" },
            { "h8h7i7i8j7j8j9i9h9h11i10g7i6f7j6k5i5h4k7l9k8m7l7m6", true, "search_test #10" },
            { "h8h7i7i8j7j8j9i9h9h11i10g7i6f7j6k5i5h4k7l9k8m7l7m6k10k9", true, "search_test #11" },
            { "h8h9i10j9k9i8j7j10j11j12g7g9g6h10i11g11i9", true, "search_test #12" },
            { "h8i9f6g8g6h9f9f7e6d6i10j9k9j7j8i7i8l8h7f5e7d8", true, "search_test #13" },
            { "h8h9i8g8i10i9j9k10k8l7j8l8j6j7i7k5h6g5g6i6h7f5", true, "search_test #14" },
            { "h8h9i8g8h7i7j7j9k9l9k10l10j11k11i12i11h13g14g12f11g11f12f10e9g9i10", true, "search_test #15" },
        };
        runSuite(cases, "winning_cases");
    }

    // findNextMove cases. expectedMove is optional ("h8" form) — fill in
    // when you have a known correct answer (forced moves, VCT first move, etc.).
    // Leave empty to just inspect output.
    void testFindNextMoveCases() {
        const vector<NextMoveCase> cases = {
            { "h8h9i8g8i10i9j9k10j7i7", "", "VCT win exists" },
            { "h8h9j9g8j10g7i10", "", "VCT win exists" },
            { "h8h9f6g7e9g8f7h6f8f9h7d6", "", "VCT win exists" },
            { "h8h9i8i9j8j9g8g9f8", "", "quiet midgame" },
            { "h8h9i8g8i10i9j9k10k8l7j8l8j6j7i7k5h6g5g6i6h7f5h5h4f7e8f8", "", "VCT lose — longest delay defense" },
            { "h8h9i9g7i10i8g10h10h11i12g12f13j10j9g11g9i11h12f12", "", "one defend" },
            { "h8h9i8i9j8k8h7j9k9i10h11g8f7h10j7i7", "", "one defend" },
            { "h8h9i8g8i10i9j9k10k8", "", "lose but need correct defense" },
        };
        runNextMoveSuite(cases, "find_next_move_cases");
    }

};

int main() {
    EngineTest engineTest;
    engineTest.runAllTests();

    return 0;
}
