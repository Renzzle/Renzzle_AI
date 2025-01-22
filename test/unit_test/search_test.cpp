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
        //registerTestMethod([this]() { testAlphaBetaSearch(); });
        registerTestMethod([this]() { testFindNextMove(); });
        //registerTestMethod([this]() { playAlone(); });
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

    void testFindNextMove() {
        const string processArr[] = {
            "h8h9i8g8i10i9j9h7k8l7j8l8j10j11l10",
            "h8h9i8g8i10i9j9h7k8l7j8l8j10j11l10k10m11",
            "h8h9i8g8i10i9j9h7k8l7j8l8j10j11j7",
            "h8h9i8g8i10i9j9h7k8l7j8l8j10j11",
            "h8h9i8g8i10i9j9h7j10k10j11k11g7",
            "h8h9i8g8i10i9j9g9g7h7i6f11g11g12h10j5j6k6j7k10i7j11d9h6h5"
        };

        for (auto process : processArr) {
            TEST_PRINT("=================================");
            Board board = getBoard(process);
            printBoard(board);
            SearchMonitor monitor;
            Search search(board, monitor);
            Pos move = search.findNextMove(board);
            board.move(move);
            TEST_PRINT("<result>");
            printBoard(board);
        }
    }

    void playAlone() {
        const string process = "h8h9i9i8";

        Board board = getBoard(process);
        printBoard(board);
        SearchMonitor monitor;
        Search search(board, monitor);
        for (int i = 0; i < 40; i++) {
            Pos move = search.findNextMove(board);
            if (move.isDefault()) break;
            board.move(move);
            printBoard(board);
        }
        TEST_PRINT("finish play alone");
    }

};

int main() {
    SearchTest searchTest;
    searchTest.runAllTests();

    return 0;
}