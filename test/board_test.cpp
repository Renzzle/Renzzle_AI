#include "test.h"
#include "util.h"
#include "../game/board.h"

class BoardTest : public TestBase {

public:
    BoardTest() {
        registerTestMethod([this]() { getTurnTest(); });
    }

    void getTurnTest() {
        Board board;
        bool ibt = board.isBlackTurn();
        TEST_ASSERT(ibt);
    }

};


int main() {
    BoardTest boardTest;
    boardTest.runAllTests();

    return 0;
}