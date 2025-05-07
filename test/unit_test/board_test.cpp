#include "../test.h"
#include "../util.h"
#include "../../game/board.h"

class BoardTest : public TestBase {

public:
    BoardTest() {
        registerTestMethod([this]() { getTurnTest(); });
        registerTestMethod([this]() { setPatternTest(); });
        registerTestMethod([this]() { moveTest(); });
        registerTestMethod([this]() { undoTest(); });
        registerTestMethod([this]() { passTest(); });
        registerTestMethod([this]() { resultTest(); });
        registerTestMethod([this]() { patternTest(); });
        registerTestMethod([this]() { forbiddenTest(); });
    }

    void getTurnTest() {
        Board board;
        bool ibt = board.isBlackTurn();
        TEST_ASSERT(ibt);
    }

    void setPatternTest() {
        Board board;
        Pos p(8, 8);
        TEST_TIME_START();
        for (int i = 0; i < 1000; i++) {
            board.setPatterns(p);
        }
        TEST_TIME_END("setPatterns(x1000)");
    }

    void moveTest() {
        Board board;

        // check first move
        bool result = board.move(Pos(1, 1));
        TEST_ASSERT(result);
        TEST_ASSERT(!board.isBlackTurn());
        TEST_ASSERT(board.getCell(Pos(1, 1)).getPiece() == BLACK);
        TEST_ASSERT(board.getCell(Pos(1, 1)).getPattern(BLACK, VERTICAL) == PATTERN_SIZE);

        // move same position
        result = board.move(Pos(1, 1));
        TEST_ASSERT(!result);

        // move after win
        board = getBoard("h8h9i8i9j8j9k8k9l8");
        result = board.move(Pos(15, 15));
        TEST_ASSERT(!result);

        // move when board is full
        vector<pair<int, int>> v = processString("h8j8h15o12o14o10m9n9o6n8o5o4m4n4m5o2k1i2i3f4f3f8g7e8e11g14g11j11k13d15d14c15b15a12a14b9a10a8b12a6a5c6b5a2d4b1g1l8e5f6c5d5e3f2b6a7g3h3c3d3b2a1b4b3e1d2e4e2e7e6c4a4b8b7d6a3c7a9f7d7i7h7g9f10k5j6l5n5i5j5e9d8i6i4i9i8k11j10f9h9c9d9d10f12c11a13f11d11i11h11j12h10i13l10g15h14g8g10i10i12g5g6j4h2g2g4f5h5c1c2c10c8e10b10c13c12e13e12f1d1h4j2k3h6d12f14b14a15e14c14i1h1m1l2k2k4l1j1m3m2o1n1l3j3n3o3l4n2b13b11f13d13h13g13e15a11l7l6j7k6i15f15k15j15k14k12l14m15n14m14j14h12g12i14l12m11l13j13m13o15n13o13l11l15m6m7k8j9k10m12l9m10n7m8k9k7o7n6n12o11o9o8n11n10n15");
        TEST_TIME_START();
        for (int i = 0; i < 100; i++) {
            board = Board();
            for (auto p : v) {
                board.move(Pos(p.first, p.second));
            }
        }
        TEST_TIME_END("full board(x100)")
        result = board.move(Pos(1, 1));
        TEST_ASSERT(!result);
    }

    void undoTest() {
        Board board;
        board.move(Pos(1, 1));
        board.undo();
        TEST_ASSERT(board.isBlackTurn());
        TEST_ASSERT(board.getCell(Pos(1, 1)).getPiece() == EMPTY);

        for (int i = 1; i <= 7; i++) {
            board.move(Pos(i , i));
        }
        for (int i = 1; i <= 7; i++) {
            board.undo();
        }
        TEST_ASSERT(board.isBlackTurn());
        for (int i = 1; i <= 7; i++) {
            TEST_ASSERT(board.getCell(Pos(i, i)).getPiece() == EMPTY);
        }
    }

    void passTest() {
        Board board;
        board.move(Pos(1, 1));
        board.pass();
        TEST_ASSERT(board.isBlackTurn());

        for (int i = 1; i <= 15; i++) {
            for (int j = 1; j <= 15; j++) {
                if (i != 1 || j != 1) {
                    TEST_ASSERT(board.getCell(Pos(i, j)).getPiece() == EMPTY);
                }
            }
        }

        board.undo();
        TEST_ASSERT(!board.isBlackTurn());
    }

    // just for make five or overline(white)
    void resultTest() {
        // black make horizontal five
        Board board = getBoard("h8h9i8i9j8j9k8k9l8");
        Result result = board.getResult();
        TEST_ASSERT(result == BLACK_WIN);

        // white make vertical five
        board = getBoard("e5f5e6f6e7f7e8f8e10f9");
        result = board.getResult();
        TEST_ASSERT(result == WHITE_WIN);

        // white make vertical six
        board = getBoard("e5f5e6f6e7f7e8f8e10f10e11f9");
        result = board.getResult();
        TEST_ASSERT(result == WHITE_WIN);

        // white make upward nine
        board = getBoard("h8h7g7i8i9j9j10k10e5f5d4e4c3d3b2c2f9g6");
        result = board.getResult();
        TEST_ASSERT(result == WHITE_WIN);
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
        TEST_ASSERT(board.getCell(Pos(13, 9)).getPattern(BLACK, HORIZONTAL) == DEAD);
        // normal overline: ooo?oo
        TEST_ASSERT(board.getCell(Pos(2, 8)).getPattern(BLACK, HORIZONTAL) == OVERLINE);
        // 4-4 overline: ooo.?.ooo
        TEST_ASSERT(board.getCell(Pos(6, 5)).getPattern(BLACK, VERTICAL) == OVERLINE);
        // blocked 1: x?....
        TEST_ASSERT(board.getCell(Pos(12, 6)).getPattern(BLACK, VERTICAL) == BLOCKED_1);
        // free 1: x.?...
        TEST_ASSERT(board.getCell(Pos(11, 6)).getPattern(BLACK, VERTICAL) == FREE_1);
        // blocked 2: xo?....
        TEST_ASSERT(board.getCell(Pos(13, 12)).getPattern(BLACK, HORIZONTAL) == BLOCKED_2);
        // free 2: .o..?.
        TEST_ASSERT(board.getCell(Pos(10, 8)).getPattern(BLACK, UPWARD) == FREE_2);
        // free 2a: .o.?..
        TEST_ASSERT(board.getCell(Pos(11, 9)).getPattern(BLACK, UPWARD) == FREE_2A);
        // free 2b: .o?...
        TEST_ASSERT(board.getCell(Pos(8, 12)).getPattern(BLACK, VERTICAL) == FREE_2B);
        // blocked 3: ooxx?..
        TEST_ASSERT(board.getCell(Pos(5, 14)).getPattern(WHITE, VERTICAL) == BLOCKED_3);
        // free 3: .x?.x.
        TEST_ASSERT(board.getCell(Pos(13, 4)).getPattern(WHITE, HORIZONTAL) == FREE_3);
        // free 3a: ..xx?..
        TEST_ASSERT(board.getCell(Pos(11, 8)).getPattern(BLACK, VERTICAL) == FREE_3A);
        // blocked 4: |?xxx..
        TEST_ASSERT(board.getCell(Pos(1, 3)).getPattern(WHITE, VERTICAL) == BLOCKED_4);
        // free 4: .xxx?.
        TEST_ASSERT(board.getCell(Pos(5, 3)).getPattern(WHITE, VERTICAL) == FREE_4);
        // five: ?xxxx
        TEST_ASSERT(board.getCell(Pos(7, 3)).getPattern(WHITE, VERTICAL) == FIVE);
        // white overline five: xxxx?x
        TEST_ASSERT(board.getCell(Pos(12, 3)).getPattern(WHITE, VERTICAL) == FIVE);
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
        TEST_ASSERT(board.getResult() == WHITE_WIN);
        // 4-4: .ooo?. & .ooo?.
        board = getBoard("h8h9i9h10i10g9g8g10i11h11f8f9i8");
        TEST_ASSERT(board.getResult() == WHITE_WIN);
        // 4-4: .ooo?. & .o.oo?.
        board = getBoard("h8h9i8i9g8g9i7h7h6i6f4e3j8");
        TEST_ASSERT(board.getResult() == WHITE_WIN);
        // 4-4: .oo.o?.oo.
        board = getBoard("h8h9i8i9k8k9n8l9o8n9l8");
        TEST_ASSERT(board.getResult() == WHITE_WIN);
        // five > 4-4
        board = getBoard("h8h9i8i9g8g9i7h7h6i6f4e3j9i10j10k9j7k6j6j5j8");
        TEST_ASSERT(board.getResult() == BLACK_WIN);
        // 3-3: ..oo?.. & ..oo?..
        board = getBoard("g8g9h9g10h10f9f8f10h8");
        TEST_ASSERT(board.getResult() == WHITE_WIN);
        // 3-3: .oo.?. & .oo.?.
        board = getBoard("g8g9h9g10h11h10g11f10j11");
        TEST_ASSERT(board.getResult() == WHITE_WIN);
        // fake 3-3 (1)
        board = getBoard("h7g7j11i12l9j12m8l8k7l6k6l5m7l4l7");
        TEST_ASSERT(board.getResult() != WHITE_WIN);
        // fake 3-3 (2)
        board = getBoard("g10h11f11f9i9k11j9m9l10m6l5k4m4h4k8f6k7a1g5a2f4b1h7b2h8c1i6a3j6d1m11c2h9");
        TEST_ASSERT(board.getResult() != WHITE_WIN);
        // looks like fake 3-3 but real 3-3 (3)
        board = getBoard("h8h11h10j10g10f10f9e9i9k9j9j8j7j6i6h5h6f6g6g7h7");
        TEST_ASSERT(board.getResult() == WHITE_WIN);
        // fake 3-3 (4)
        board = getBoard("c14c15d14b14e14f15d13c13c12b11e13g14f13g13f12h10h13o1i14n1d12");
        TEST_ASSERT(board.getResult() != WHITE_WIN);
        // fake 3-3 (5)
        board = getBoard("d14e14c13c12d12a9c11e11f12g12f13o15h12o14i11e12d13");
        TEST_ASSERT(board.getResult() != WHITE_WIN);
    }

};

int main() {
    BoardTest boardTest;
    boardTest.runAllTests();

    return 0;
}