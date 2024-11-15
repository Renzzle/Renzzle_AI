#include "../test.h"
#include "../util.h"
#include "../../search/search.h"

class SearchTest : public TestBase {

PRIVATE
    void searchTest(string process) {
        Board board = getBoard(process);
        int maxDepth = 4;
        SearchMonitor monitor;
        Search searcher(board, monitor);

        static Value lastBestValue = -9999999;
        monitor.setTrigger([](SearchMonitor& monitor) {
            if (monitor.bestValue != lastBestValue) {
                lastBestValue = monitor.getBestValue();
                return true;
            }
            return false;
        });

        monitor.setSearchListener([](SearchMonitor& monitor) {
            TEST_PRINT("Value: " << monitor.getBestValue() << ", Node: " << monitor.getVisitCnt());
            printPath(monitor.getBestPath());
        });

        printBoard(board);

        TEST_TIME_START();
        searcher.alphaBeta(board, 9, MIN_VALUE, MAX_VALUE, true);
        TEST_TIME_END("∙ alpha-beta search");

        printPath(monitor.getBestPath());
    }

PUBLIC
    SearchTest() {
        registerTestMethod([this]() { testAlphaBetaSearch(); });
    }

    void testAlphaBetaSearch() {
        const string processArr[] = {
            "h8h9i8g8i10i9j9h7k8l7j8l8j10j11l10k10i7k9j6"
        };

        for (auto process : processArr) {
            TEST_PRINT("=================================");
            searchTest(process);
        }
    }

};

int main() {
    SearchTest searchTest;
    searchTest.runAllTests();

    return 0;
}