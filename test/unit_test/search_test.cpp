#include "../test.h"
#include "../util.h"
#include "../../search/search.h"

class SearchTest : public TestBase {

private:
    void searchTest(string process) {
        Board board = getBoard(process);
        int maxDepth = 4;
        Search searcher(board, maxDepth);

        printBoard(board);

        TEST_TIME_START();
        Pos bestMove = searcher.findBestMove();
        TEST_TIME_END("alpha-beta search");

        cout << "Best move found: (" << (char)(bestMove.getY() + 96) << ", " << bestMove.getX() << ")" << endl << endl;
    }

    void iterativeDeepeningSearchTest(string process) {
        Board board = getBoard(process);
        int maxDepth = 4;
        Search searcher(board, maxDepth);

        printBoard(board);

        TEST_TIME_START();
        Pos bestMove = searcher.iterativeDeepeningSearch();
        TEST_TIME_END("iterative deepening search");

        cout << "Best move found using IDS: (" << (char)(bestMove.getY() + 96) << ", " << bestMove.getX() << ")" << endl << endl;
    }

public:
    SearchTest() {
        registerTestMethod([this]() { testAlphaBetaSearch(); });
        registerTestMethod([this]() { testIDSSearch(); });
    }

    void testAlphaBetaSearch() {
        //searchTest("h8h9i8g8i10j9i9i7j10k11h10k10j8k7g10f10g11f12g7f6f7f11");
        //searchTest("h8h9i9g10e12f11g8g9i10");
        //searchTest("h8g9i9f9c9e9g8g7i10g10e10f11e12");
        searchTest("h8i9i8g8j8i7j7k6i6j9");
    }

    void testIDSSearch() {
        //iterativeDeepeningSearchTest("h8h9i8g8i10j9i9i7j10k11h10k10j8k7g10f10g11f12g7f6f7f11");
        //iterativeDeepeningSearchTest("h8h9i9g10e12f11g8g9i10");
        //iterativeDeepeningSearchTest("h8g9i9f9c9e9g8g7i10g10e10f11e12");
        iterativeDeepeningSearchTest("h8i9i8g8j8i7j7k6i6j9");
    }
};

int main() {
    SearchTest searchTest;
    searchTest.runAllTests();

    return 0;
}