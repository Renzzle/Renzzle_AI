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

        monitor.setTrigger([](SearchMonitor& monitor) {
            static int depth = 0;
            const int curDepth = monitor.getDepth();
            if (depth != curDepth) {
                depth = curDepth;
                return true;
            }
            return false;
        });

        monitor.setSearchListener([&searcher](SearchMonitor& monitor) {
            TEST_PRINT("Depth: " << monitor.getDepth() << ", Time: " << monitor.getElapsedTime() << 
            "sec, Node: " << monitor.getVisitCnt() << ", Value: " << monitor.getBestValue().getValue());
            printPath(monitor.getBestPath());
        });

        printBoard(board);

        TEST_TIME_START();
        searcher.ids();
        TEST_TIME_END("alpha-beta search");
        TEST_PRINT("Final visited node: " << monitor.getVisitCnt());
        printPath(monitor.getBestLine(0));
    }

PUBLIC
    SearchTest() {
        registerTestMethod([this]() { testAlphaBetaSearch(); });
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