#include "../test.h"
#include "../util.h"
#include "../../search/search.h"

class SearchTest : public TestBase {

PRIVATE
    void printPath(const vector<Pos>& path, Board& board) {
        for (size_t i = 0; i < path.size(); ++i) {
            const Pos& p = path[i];

            string player = (i % 2 == 0) ? "BLACK" : "WHITE";
            TEST_PRINT(i + 1 << ": (" << (char)(p.getY() + 96) << ", " << p.getX() << ") - " << player);
        }
    }

    void printSimulatedPath(const vector<Pos>& path, Board& board) {
        size_t originalMoveCount = board.getPath().size();
        
        for (size_t i = originalMoveCount; i < path.size(); ++i) {
            const Pos& p = path[i];

            string player = (i % 2 == 0) ? "BLACK" : "WHITE";
            TEST_PRINT(i + 1 << ": (" << (char)(p.getY() + 96) << ", " << p.getX() << ") - " << player);
        }
    }

    void searchTest(string process) {
        Board board = getBoard(process);
        int maxDepth = 4;
        Search searcher(board, maxDepth);

        printBoard(board);
        TEST_PRINT("");

        TEST_TIME_START();
        Pos bestMove = searcher.findBestMove();
        TEST_TIME_END("∙ alpha-beta search");
        TEST_PRINT("");

        vector<Pos> path = searcher.getPath();

        TEST_PRINT("∙ printPath");
        printPath(path, board);
        TEST_PRINT("");

        TEST_PRINT("∙ printSimulatedPath");
        printSimulatedPath(path, board);
        TEST_PRINT("");

        TEST_PRINT("∙ Best move found: (" << (char)(bestMove.getY() + 96) << ", " << bestMove.getX() << ")" << endl);

        for (const auto& pos : path) {
            process += (char)(pos.getY() + 96);
            process.append(to_string(pos.getX()));
        }
        TEST_PRINT("∙ Best path found: " << process << endl);
    }

    void iterativeDeepeningSearchTest(string process) {
        Board board = getBoard(process);
        int maxDepth = 4;
        Search searcher(board, maxDepth);

        printBoard(board);
        TEST_PRINT("");

        TEST_TIME_START();
        Pos bestMove = searcher.iterativeDeepeningSearch();
        TEST_TIME_END("∙ iterative deepening search");
        TEST_PRINT("");

        vector<Pos> path = searcher.getPath();

        TEST_PRINT("∙ printPath");
        printPath(path, board);
        TEST_PRINT("");

        TEST_PRINT("∙ printSimulatedPath");
        printSimulatedPath(path, board);
        TEST_PRINT("");

        TEST_PRINT("∙ Best move found using IDS: (" << (char)(bestMove.getY() + 96) << ", " << bestMove.getX() << ")" << endl);

        for (const auto& pos : path) {
            process += (char)(pos.getY() + 96);
            process.append(to_string(pos.getX()));
        }
        TEST_PRINT("∙ Best path found using IDS: " << process << endl);
    }

PUBLIC
    SearchTest() {
        registerTestMethod([this]() { testAlphaBetaSearch(); });
        registerTestMethod([this]() { testIDSSearch(); });
    }

    void testAlphaBetaSearch() {
        const string processArr[] = {
            //"h8h9i8g8i10j9i9i7j10k11h10k10j8k7g10f10g11f12g7f6f7f11",
            //"h8h9i9g10e12f11g8g9i10",
            //"h8g9i9f9c9e9g8g7i10g10e10f11e12",
            "h8i9i8g8j8i7j7k6i6j9"
        };

        for (auto process : processArr) {
            TEST_PRINT("=================================\n");
            TEST_PRINT("∙ path: " << process);
            searchTest(process);
        }
    }

    void testIDSSearch() {
        const string processArr[] = {
            //"h8h9i8g8i10j9i9i7j10k11h10k10j8k7g10f10g11f12g7f6f7f11",
            //"h8h9i9g10e12f11g8g9i10",
            //"h8g9i9f9c9e9g8g7i10g10e10f11e12",
            "h8i9i8g8j8i7j7k6i6j9"
        };

        for (auto process : processArr) {
            TEST_PRINT("=================================\n");
            TEST_PRINT("∙ path: " << process);
            iterativeDeepeningSearchTest(process);
        }
    }
};

int main() {
    SearchTest searchTest;
    searchTest.runAllTests();

    return 0;
}