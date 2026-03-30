#include "../test.h"
#include "../util.h"
#include "../../search/search.h"
#include <algorithm>
#include <iomanip>

class SearchTest : public TestBase {

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

    string valueToString(Value value) {
        ostringstream oss;

        if (value.isWin()) {
            oss << "WIN(" << value.getResultDepth() << ")";
        } else if (value.isLose()) {
            oss << "LOSE(" << value.getResultDepth() << ")";
        } else if (value.getType() == Value::Type::UNKNOWN) {
            oss << "UNKNOWN(" << value.getValue() << ")";
        } else {
            oss << value.getValue();
        }

        oss << " ";
        switch (value.getType()) {
            case Value::Type::EXACT:       oss << "[EXACT]"; break;
            case Value::Type::LOWER_BOUND: oss << "[LOWER]"; break;
            case Value::Type::UPPER_BOUND: oss << "[UPPER]"; break;
            case Value::Type::UNKNOWN:     oss << "[UNKNOWN]"; break;
        }

        return oss.str();
    }

    string formatMilliseconds(double seconds) {
        ostringstream oss;
        oss << fixed << setprecision(3) << (seconds * 1000.0) << " ms";
        return oss.str();
    }

    void printRootStats(const vector<Search::RootMoveStat>& stats, size_t limit) {
        if (stats.empty()) {
            TEST_PRINT("  (no root stats)");
            return;
        }

        const size_t count = min(limit, stats.size());
        for (size_t i = 0; i < count; ++i) {
            const Search::RootMoveStat& stat = stats[i];
            TEST_PRINT("  [" << stat.order << "] " << posToString(stat.move)
                << " | nodes=" << stat.nodeCount
                << " | time=" << formatMilliseconds(stat.elapsedTime)
                << " | value=" << valueToString(stat.value)
                << " | ttBest=" << (stat.wasTTBest ? "Y" : "N")
                << " | research=" << (stat.wasResearched ? "Y" : "N")
                << " | cutoff=" << (stat.causedCutoff ? "Y" : "N"));
            if (!stat.pv.empty()) {
                TEST_PRINT("      pv: " << convertPath2String(stat.pv));
            }
        }

        if (stats.size() > limit) {
            TEST_PRINT("  ... +" << (stats.size() - limit) << " more root candidates");
        }
    }

    void searchTest(string process) {
        Board board = getBoard(process);
        SearchMonitor monitor;
        Search searcher(board, monitor);

        monitor.setTrigger([](SearchMonitor& monitor) {
            static int depth = 0;
            const int curDepth = monitor.getDepth();
            if (depth != curDepth) {
                depth = curDepth;
                return true;
            }
            return false;
        });

        monitor.setSearchListener([&searcher, this](SearchMonitor& monitor) {
            const double memMB = searcher.getEstimatedMemoryBytes() / (1024.0 * 1024.0);
            TEST_PRINT("Depth: " << monitor.getDepth() << ", Time: " << formatMilliseconds(monitor.getElapsedTime()) <<
            ", Node: " << monitor.getVisitCnt() << ", TT entries: " << searcher.getNodeCount() <<
            ", Mem: " << memMB << " MB, Value: " << monitor.getBestValue().getValue());
            printPath(monitor.getBestPath());
        });

        printBoard(board);

        TEST_TIME_START();
        searcher.ids();
        TEST_TIME_END("alpha-beta search");
        TEST_PRINT("Final visited node: " << monitor.getVisitCnt());
        TEST_PRINT("Final TT entries: " << searcher.getNodeCount() << 
            ", Mem: " << (searcher.getEstimatedMemoryBytes() / (1024.0 * 1024.0)) << " MB");
        printPath(monitor.getBestLine(0));
    }

    void deepAnalysisTest(const string& process, int maxDepth, size_t rootPreviewCount) {
        Board board = getBoard(process);
        SearchMonitor monitor;
        Search searcher(board, monitor);

        TEST_PRINT("=================================");
        TEST_PRINT("[DEEP] process: " << process);
        printBoard(board);
        TEST_PRINT("");

        searcher.isRunning = true;
        searcher.bestPath.clear();
        searcher.bestValue = Value();
        searcher.clearHistory();
        monitor.initStartTime();
        searcher.tt.clear();

        int completedDepth = 0;

        for (int depth = 5; depth <= maxDepth; depth += 2) {
            searcher.tt.nextGeneration();

            const size_t nodesBefore = monitor.getVisitCnt();
            const double timeBefore = monitor.getElapsedTime();
            MoveList pv;
            Value result = searcher.searchRootWithAspiration(depth, &pv);

            if (!searcher.isRunning) {
                break;
            }

            searcher.bestValue = result;
            searcher.bestPath = pv;
            monitor.setBestPath(pv);
            monitor.updateElapsedTime();
            completedDepth = depth;

            TEST_PRINT("[DEEP][depth " << depth << "] totalNodes=" << monitor.getVisitCnt()
                << ", deltaNodes=" << (monitor.getVisitCnt() - nodesBefore)
                << ", totalTime=" << formatMilliseconds(monitor.getElapsedTime())
                << ", deltaTime=" << formatMilliseconds(monitor.getElapsedTime() - timeBefore)
                << ", ttEntries=" << searcher.getNodeCount()
                << ", ttHitRate=" << (searcher.tt.getHitRate() * 100.0) << "%"
                << ", value=" << valueToString(result));
            TEST_PRINT("[DEEP][depth " << depth << "] best pv: " << convertPath2String(pv));
            TEST_PRINT("[DEEP][depth " << depth << "] root profile (final aspiration pass)");
            printRootStats(searcher.getLastRootStats(), rootPreviewCount);

            if (result.isWin() && result.getResultDepth() <= depth) {
                break;
            }
        }

        TEST_ASSERT(completedDepth > 0);
        TEST_PRINT("[DEEP] completed depth: " << completedDepth);
        TEST_PRINT("[DEEP] final visited node: " << monitor.getVisitCnt());
        TEST_PRINT("[DEEP] final TT entries: " << searcher.getNodeCount()
            << ", Mem: " << (searcher.getEstimatedMemoryBytes() / (1024.0 * 1024.0)) << " MB");
        TEST_PRINT("=================================");

        searcher.stop();
    }

PUBLIC
    SearchTest() {
        registerTestMethod("alpha_beta_search_suite", [this]() { testAlphaBetaSearch(); });
        //registerTestMethod("deep_single_case_profile", [this]() { testDeepSingleCaseProfile(); });
    }

    void testAlphaBetaSearch() {
        const string processArr[] = {
            "h8h9i8g8i10i9j9k10j7i7",
            "h8h9i8g8i10i9j9k8k10l11i7j6",
            "h8h9j9g8j10g7i10",
            "h8i9h9h10g9g8f8g7e7f7g6f6i6h6j5j6k7j8k9i10j10i11i12h11g11e8f9",
            "h8i8i9i7j8j7j6k7k6i6h7h6i5g7f8h10i10d8f6",
            "h8h9f6g7e9g8f7h6f8f9h7d6",
            "h8h9g7i9g9i7g10i8i10h10e8f9d8j7",
            "h8h9i8g8i10j8i6h7h6k7j6f7f8",
            "h8h9i9i8g10h10f7g7h6h7g5i7k7j6k6k8l7j9k10",
            "h8h7i7i8j7j8j9i9h9h11i10g7i6f7j6k5i5h4k7l9k8m7l7m6"
        };

        for (auto process : processArr) {
            TEST_PRINT("=================================");
            searchTest(process);
        }
    }

    void testDeepSingleCaseProfile() {
        // Replace this string when you want to inspect a different root position in detail.
        const string process = "h8h7i7i8j7j8j9i9h9h11i10g7i6f7j6k5i5h4k7l9k8m7l7m6";
        deepAnalysisTest(process, 17, 15);
    }

};

int main() {
    SearchTest searchTest;
    searchTest.runAllTests();

    return 0;
}
