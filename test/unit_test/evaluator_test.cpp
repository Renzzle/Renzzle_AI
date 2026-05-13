#include "../util.h"
#include "../test.h"
#include "../../evaluate/evaluator.h"

class EvaluatorTest : public TestBase {

public:
    EvaluatorTest() {
        registerTestMethod([this]() { checkTime(); });
        registerTestMethod([this]() { checkDefendTime(); });
        registerTestMethod([this]() { threatDefendTest(); });
        registerTestMethod([this]() { fourThreeDefendTest(); });
        registerTestMethod([this]() { fourThreeMakersTest(); });
    }

    void checkDefendTime() {
        int iteration = 100000;
        {
            Board board = getBoard("h8h9i8f8j8");
            Evaluator evaluator(board);
            TEST_TIME_START();
            for (int i = 0; i <= iteration; i++) {
                evaluator.getThreatDefend();
            }
            TEST_TIME_END("getThreatDefend(x100,000)");
        }
        {
            Board board = getBoard("h8g9h10h5h6g8g7i9j7");
            Evaluator evaluator(board);
            TEST_TIME_START();
            for (int i = 0; i <= iteration; i++) {
                evaluator.getFourThreeDefend();
            }
            TEST_TIME_END("getFourThreeDefend(x100,000)");
        }
        {
            Board board = getBoard("h8h9i8g8i10i9j9k10k8l7j8l8j6j7i7k5h6g5g6i6h7f5h5h4f7e8");
            Evaluator evaluator(board);
            TEST_TIME_START();
            for (int i = 0; i <= iteration; i++) {
                evaluator.getFourThreeMakers();
            }
            TEST_TIME_END("getFourThreeMakers(x100,000)");
        }
    }

    void checkTime() {
        Board board = getBoard("h8h9h10g8f9i9j9g10i8k10g9f7e6f8g6f5f6h6e9d8e8e7e11f10c9d9d10b8c11e12d11f11d13d7");
        int iteration = 100000;
        {
            TEST_TIME_START();
            for (int i = 0; i <= iteration; i++) {
                Evaluator evaluator(board);
            }
            TEST_TIME_END("classify(x100,000)");
        }
        Evaluator evaluator(board);
        {
            TEST_TIME_START();
            for (int i = 0; i <= iteration; i++) {
                evaluator.getCandidates();
            }
            TEST_TIME_END("getCandidates(x100,000)");
        }
        {
            TEST_TIME_START();
            for (int i = 0; i <= iteration; i++) {
                evaluator.getFours();
            }
            TEST_TIME_END("getFours(x100,000)");
        }
    }

    void threatDefendTest() {
        const vector<pair<string, string>> cases = {
            {"baseline: oppo mate (-oo-o-)", "h8h9i8i9k8"},
            {"baseline: oppo mate (-ooo-)", "h8h9i8i9j8"},
            {"baseline: oppo mate (x-ooo-)", "h8h9i8f8j8"},
            {"baseline: oppo mate 44", "h8h9i8i9j9g9g8j8h10f8e8f7g7f6f5h7"},
            {"baseline: oppo mate 44", "h8h9o15j9j8i8j10i10k8i6j7l9"},
        };

        for (size_t i = 0; i < cases.size(); ++i) {
            const string& label = cases[i].first;
            const string& process = cases[i].second;
            TEST_PRINT("=================================");
            TEST_PRINT("[threatDefend] " << label);

            Board board = getBoard(process);
            printBoard(board);

            Evaluator evaluator(board);
            TEST_PRINT("isOppoMateExist=" << (evaluator.isOppoMateExist() ? "Y" : "N"));

            MoveList defends = evaluator.getThreatDefend();
            TEST_PRINT("defends(" << defends.size() << "):");
            printPath(defends);
        }
    }

    void fourThreeMakersTest() {
        // Cases should be positions where self has B4_PLUS (BLOCKED_4 + F2/B3 extra in another direction).
        // adjust/replace these to focus on the patterns you want to exercise.
        const vector<pair<string, string>> cases = {
            {"#1", "h8h9i8g8i10i9j9k10k8l7j8l8j6j7i7k5h6g5g6i6h7f5h5h4f7e8"},
        };

        for (size_t i = 0; i < cases.size(); ++i) {
            const string& label = cases[i].first;
            const string& process = cases[i].second;
            TEST_PRINT("=================================");
            TEST_PRINT("[fourThreeMakers] " << label);

            Board board = getBoard(process);
            printBoard(board);

            Evaluator evaluator(board);
            MoveList makers = evaluator.getFourThreeMakers();
            TEST_PRINT("makers(" << makers.size() << "):");
            printPath(makers);
        }
    }

    void fourThreeDefendTest() {
        const vector<pair<string, string>> cases = {
            {"#1", "d13d14d12c12d10c10c11e13e11"},
            {"#2", "h8g9g7f6i8j8i6h7j10"},
            {"#3", "h8g9h10h5h6g8g7i9j7"},
            {"#4", "h8g9h10h5h6g8g7i9j7h4j9"},
            {"#5", "h8g9h10h5h6g8g7f6i7k7l7"},
            {"#6", "h8g9h10h5h6g8g7f6i7i5l7"},
            {"#7", "h8h9i8g8i10i9h11h7j8"},
            {"#8", "h8i9f6g8g6h9f9f7e6d6i10j9k9j7j8i7i8l8h7f5e7d8e5e8h6i6h4h5g4"},
        };

        for (size_t i = 0; i < cases.size(); ++i) {
            const string& label = cases[i].first;
            const string& process = cases[i].second;
            TEST_PRINT("=================================");
            TEST_PRINT("[fourThreeDefend] " << label);

            Board board = getBoard(process);
            printBoard(board);

            Evaluator evaluator(board);
            TEST_PRINT("isOppoFourThreeExist=" << (evaluator.isOppoFourThreeExist() ? "Y" : "N")
                << " | isOppoMateExist=" << (evaluator.isOppoMateExist() ? "Y" : "N"));

            MoveList defends = evaluator.getFourThreeDefend();
            TEST_PRINT("defends(" << defends.size() << "):");
            printPath(defends);
        }
    }

};

int main() {
    EvaluatorTest evaluatorTest;
    evaluatorTest.runAllTests();

    return 0;
}
