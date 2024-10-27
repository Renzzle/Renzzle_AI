#include "../test.h"
#include "../util.h"
#include "../../evaluate/evaluator.h"

class EvaluatorTest : public TestBase {

public:
    EvaluatorTest() {
        registerTestMethod([this]() { checkTime(); });
    }

    void checkTime() {
        Evaluator evaluator;
        Board board = getBoard("h8h9h10g8f9i9j9g10i8k10g9f7e6f8g6f5f6h6e9d8e8e7e11f10c9d9d10b8c11e12d11f11d13d7");
        int iteration = 100000;
        {
            TEST_TIME_START();
            for (int i = 0; i <= iteration; i++) {
                evaluator.classify(board);
            }
            TEST_TIME_END("classify(x100,000)");
        }
        {
            TEST_TIME_START();
            for (int i = 0; i <= iteration; i++) {
                evaluator.getCandidates(board);
            }
            TEST_TIME_END("getCandidates(x100,000)");
        }
        {
            TEST_TIME_START();
            for (int i = 0; i <= iteration; i++) {
                evaluator.getFours(board);
            }
            TEST_TIME_END("getFours(x100,000)");
        }
    }

};

int main() {
    EvaluatorTest evaluatorTest;
    evaluatorTest.runAllTests();

    return 0;
}