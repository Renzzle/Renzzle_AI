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

        printBoard(board);

        TEST_TIME_START();
        Value result = searcher.abp(13);
        TEST_TIME_END("alpha-beta search");
        TEST_PRINT("value: " << result);
    }

PUBLIC
    SearchTest() {
        registerTestMethod([this]() { testAlphaBetaSearch(); });
    }

    void testAlphaBetaSearch() {
        const string processArr[] = {
            //"h8h9i8g8i10i9j9k10j7i7",
            //"h8h9i8g8i10i9j9k8k10l11i7j6",
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

    return 0;
}