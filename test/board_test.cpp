#include "test.h"
#include "util.h"
#include "../game/board.h"

using namespace std;

class BoardTest : public TestBase {

private:
    Board getBoard(string moves) {
        vector<pair<int, int>> v = processString(moves);
        Board board;
        for (auto p : v) {
            board.move(Pos(p.first, p.second));
        }
        return board;
    }

public:
    BoardTest() {
        registerTestMethod([this]() { getTurnTest(); });
        registerTestMethod([this]() { moveTest(); });
        registerTestMethod([this]() { resultTest(); });
        registerTestMethod([this]() { patternTest(); });
    }

    void getTurnTest() {
        Board board;
        bool ibt = board.isBlackTurn();
        assert(ibt);
    }

    void moveTest() {
        Board board;

        // check first move
        bool result = board.move(Pos(1, 1));
        assert(result);
        assert(!board.isBlackTurn());
        assert(board.getCell(Pos(1, 1)).getPiece() == BLACK);
        assert(board.getCell(Pos(1, 1)).getPattern(BLACK, VERTICAL) == PATTERN_SIZE);

        // move same position
        result = board.move(Pos(1, 1));
        assert(!result);

        // move after win
        board = getBoard("h8h9i8i9j8j9k8k9l8");
        result = board.move(Pos(15, 15));
        assert(!result);

        // move when board is full
        board = Board();
        TEST_TIME_START();
        for (int i = 1; i <= 15; i++) {
            for (int j = 1; j <= 15; j++) {
                board.move(Pos(i, j));
            }
        }
        TEST_TIME_END("board_test.cpp/moveTest/full board")
        result = board.move(Pos(1, 1));
        assert(!result);
    }

    void undoTest() {
        Board board;
        board.move(Pos(1, 1));
        board.undo();
        assert(board.isBlackTurn());
        assert(board.getCell(Pos(1, 1)).getPiece() == EMPTY);

        for(int i = 1; i <= 7; i++) {
            board.move(Pos(i , i));
        }
        for(int i = 1; i <= 7; i++) {
            board.undo();
        }
        assert(board.isBlackTurn());
        for(int i = 1; i <= 7; i++) {
            assert(board.getCell(Pos(i, i)).getPiece() == EMPTY);
        }
    }

    // just for make five or overline(white)
    void resultTest() {
        // black make horizontal five
        Board board = getBoard("h8h9i8i9j8j9k8k9l8");
        Result result = board.getResult();
        assert(result == BLACK_WIN);

        // white make vertical five
        board = getBoard("e5f5e6f6e7f7e8f8e10f9");
        result = board.getResult();
        assert(result == WHITE_WIN);

        // white make vertical six
        board = getBoard("e5f5e6f6e7f7e8f8e10f10e11f9");
        result = board.getResult();
        assert(result == WHITE_WIN);

        // white make upward nine
        board = getBoard("h8h7g7i8i9j9j10k10e5f5d4e4c3d3b2c2f9g6");
        result = board.getResult();
        assert(result == WHITE_WIN);
    }

    void patternTest() {
        Board board = getBoard("h13f13h12j13k13c10e10c9e9c8e8c4e4c3e3c2e2c11l7c13n8n7n9n6f2f3g2g3i2i3j2");
        /* black: o
         * white: x
         * empty: .
         * check: ?
         * wall : |
         */
        // dead: x.o?x
        assert(board.getCell(Pos(9, 13)).getPattern(BLACK, VERTICAL) == DEAD);
        // normal overline: ooo?oo
        assert(board.getCell(Pos(8, 2)).getPattern(BLACK, VERTICAL) == OVERLINE);
        // 4-4 overline: ooo.?.ooo
        assert(board.getCell(Pos(5, 6)).getPattern(BLACK, HORIZONTAL) == OVERLINE);
        // blocked 1: x?....
        assert(board.getCell(Pos(6, 12)).getPattern(BLACK, HORIZONTAL) == BLOCKED_1);
        // free 1: x.?...
        assert(board.getCell(Pos(6, 11)).getPattern(BLACK, HORIZONTAL) == FREE_1);
        // blocked 2: xo?....
        assert(board.getCell(Pos(12, 13)).getPattern(BLACK, VERTICAL) == BLOCKED_2);
    }

};


int main() {
    BoardTest boardTest;
    boardTest.runAllTests();

    return 0;
}