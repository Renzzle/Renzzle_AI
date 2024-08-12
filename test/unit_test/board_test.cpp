#include "../test.h"
#include "../util.h"
#include "../../game/board.h"

class BoardTest : public TestBase {

public:
    BoardTest() {
        registerTestMethod([this]() { getTurnTest(); });
        registerTestMethod([this]() { setPatternTest(); });
        registerTestMethod([this]() { moveTest(); });
        registerTestMethod([this]() { resultTest(); });
        registerTestMethod([this]() { patternTest(); });
        registerTestMethod([this]() { forbiddenTest(); });
    }

    void getTurnTest() {
        Board board;
        bool ibt = board.isBlackTurn();
        assert(ibt);
    }

    void setPatternTest() {
        Board board;
        Pos p(8, 8);
        TEST_TIME_START();
        for (int i = 0; i < 1000; i++) {
            board.setPatterns(p);
        }
        TEST_TIME_END("setPatturns(x1000)");
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
        // free 2: .o..?.
        assert(board.getCell(Pos(8, 10)).getPattern(BLACK, UPWARD) == FREE_2);
        // free 2a: .o.?..
        assert(board.getCell(Pos(9, 11)).getPattern(BLACK, UPWARD) == FREE_2A);
        // free 2b: .o?...
        assert(board.getCell(Pos(12, 8)).getPattern(BLACK, HORIZONTAL) == FREE_2B);
        // blocked 3: ooxx?..
        assert(board.getCell(Pos(14, 5)).getPattern(WHITE, HORIZONTAL) == BLOCKED_3);
        // free 3: .x?.x.
        assert(board.getCell(Pos(4, 13)).getPattern(WHITE, VERTICAL) == FREE_3);
        // free 3a: ..xx?..
        assert(board.getCell(Pos(8, 11)).getPattern(BLACK, HORIZONTAL) == FREE_3A);
        // blocked 4: |?xxx..
        assert(board.getCell(Pos(3, 1)).getPattern(WHITE, HORIZONTAL) == BLOCKED_4);
        // free 4: .xxx?.
        assert(board.getCell(Pos(3, 5)).getPattern(WHITE, HORIZONTAL) == FREE_4);
        // five: ?xxxx
        assert(board.getCell(Pos(3, 7)).getPattern(WHITE, HORIZONTAL) == FIVE);
        // white overline five: xxxx?x
        assert(board.getCell(Pos(3, 12)).getPattern(WHITE, HORIZONTAL) == FIVE);
    }

    void forbiddenTest() {
        Board board;

        /* black: o
         * white: x
         * empty: .
         * check: ?
         * wall : |
         */
        // overline: .ooo?oo.
        board = getBoard("h8h9i8i9j8j9l8l9m8m9k8");
        assert(board.getResult() == WHITE_WIN);
        // 4-4: .ooo?. & .ooo?.
        board = getBoard("h8h9i9h10i10g9g8g10i11h11f8f9i8");
        assert(board.getResult() == WHITE_WIN);
        // 4-4: .ooo?. & .o.oo?.
        board = getBoard("h8h9i8i9g8g9i7h7h6i6f4e3j8");
        assert(board.getResult() == WHITE_WIN);
        // 4-4: .oo.o?.oo.
        board = getBoard("h8h9i8i9k8k9n8l9o8n9l8");
        assert(board.getResult() == WHITE_WIN);
        // five > 4-4
        board = getBoard("h8h9i8i9g8g9i7h7h6i6f4e3j9i10j10k9j7k6j6j5j8");
        assert(board.getResult() == BLACK_WIN);
        // 3-3: ..oo?.. & ..oo?..
        board = getBoard("g8g9h9g10h10f9f8f10h8");
        assert(board.getResult() == WHITE_WIN);
        // 3-3: .oo.?. & .oo.?.
        board = getBoard("g8g9h9g10h11h10g11f10j11");
        assert(board.getResult() == WHITE_WIN);
        // fake 3-3 (1)
        board = getBoard("h7g7j11i12l9j12m8l8k7l6k6l5m7l4l7");
        assert(board.getResult() != WHITE_WIN);
        // fake 3-3 (2)
        board = getBoard("g10h11f11f9i9k11j9m9l10m6l5k4m4h4k8f6k7a1g5a2f4b1h7b2h8c1i6a3j6d1m11c2h9");
        assert(board.getResult() != WHITE_WIN);
        // looks like fake 3-3 but real 3-3 (3)
        board = getBoard("h8h11h10j10g10f10f9e9i9k9j9j8j7j6i6h5h6f6g6g7h7");
        assert(board.getResult() == WHITE_WIN);
        // fake 3-3 (4)
        board = getBoard("c14c15d14b14e14f15d13c13c12b11e13g14f13g13f12h10h13o1i14n1d12");
        assert(board.getResult() != WHITE_WIN);
        // fake 3-3 (5)
        board = getBoard("d14e14c13c12d12a9c11e11f12g12f13o15h12o14i11e12d13");
        assert(board.getResult() != WHITE_WIN);
    }

};


int main() {
    BoardTest boardTest;
    boardTest.runAllTests();

    return 0;
}