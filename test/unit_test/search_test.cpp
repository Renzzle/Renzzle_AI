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
            TEST_PRINT("Depth: " << monitor.getDepth() << ", Time: " << monitor.getElapsedTime() << "sec, Node: " << monitor.getVisitCnt());
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
            "h8h9j9g8j10g7i10"
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
    // SimpleProfiler::PrintResults();

    #ifdef TEST // TEST 매크로가 정의된 경우에만 프로파일러 관련 코드 실행
    SimpleProfiler::PrintResults();
    #endif

    return 0;
}