#include "../test.h"
#include "../util.h"
#include "../../search/search.h"

class SearchTest : public TestBase {

private:
    void searchTest(string process) {
        Board board = getBoard(process);
        int maxDepth = 5;
        Search searcher(board, maxDepth);

        printBoard(board);

        TEST_TIME_START();
        Pos bestMove = searcher.findBestMove();
        TEST_TIME_END("alpha-beta search");

        cout << "Best move found: (" << bestMove.getX() << ", " << bestMove.getY() << ")" << endl << endl;
    }

    void iterativeDeepeningSearchTest(string process) {
        Board board = getBoard(process);
        int maxDepth = 5;
        Search searcher(board, maxDepth);

        printBoard(board);

        TEST_TIME_START();
        Pos bestMove = searcher.iterativeDeepeningSearch();
        TEST_TIME_END("iterative deepening search");

        cout << "Best move found using IDS: (" << bestMove.getX() << ", " << bestMove.getY() << ")" << endl << endl;
    }

public:
    SearchTest() {
        registerTestMethod([this]() { testAlphaBetaSearch(); });
        registerTestMethod([this]() { testIDSSearch(); });
    }

    void testAlphaBetaSearch() {
        searchTest("h8h9i8g8i10j9i9i7j10k11h10k10j8k7g10f10g11f12g7f6f7f11");
        //searchTest("h8h9i9g10e12f11g8g9i10", Pos(4, 4));
        //searchTest("h8g9i9f9c9e9g8g7i10g10e10f11e12", Pos(3, 3));
    }

    void testIDSSearch() {
        iterativeDeepeningSearchTest("h8h9i8g8i10j9i9i7j10k11h10k10j8k7g10f10g11f12g7f6f7f11");
        //iterativeDeepeningSearchTest("h8h9i9g10e12f11g8g9i10", Pos(4, 4));
        //iterativeDeepeningSearchTest("h8g9i9f9c9e9g8g7i10g10e10f11e12", Pos(3, 3));
    }
};

int main() {
    SearchTest searchTest;
    searchTest.runAllTests();

    return 0;
}