#include "../evaluate/evaluator.h"
#include "../evaluate/evaluator_v1.h"
#include <vector>
#include <algorithm>
#include <limits>

constexpr Value INF = std::numeric_limits<Value>::max();

class VCFSearch {
    EvaluatorV1 evaluator;
    stack<Pos> path;
    vector<Pos> winningPath;

public:
    void setEvaluator(EvaluatorV1 eval);
    Pos findBestMove();
    void printWinningPath();
};

void VCFSearch::setEvaluator(EvaluatorV1 eval) {
    this->evaluator = eval;
}

Pos VCFSearch::findBestMove() {
    evaluator.setVCFColor();
    auto candidates = evaluator.getCandidates();

    for (const auto& move : candidates) {
        evaluator.next(move);
        path.push(move);
        Value moveValue = evaluator.evaluate();
        path.pop();
        evaluator.prev();

        if (moveValue >= 20000) {
            winningPath.clear();
            stack<Pos> tmp = path;
            while (!tmp.empty()) {
                winningPath.push_back(tmp.top());
                tmp.pop();
            }
            reverse(winningPath.begin(), winningPath.end());
            winningPath.push_back(move);
            printWinningPath();
            return move;
        }
    }

    return candidates.empty() ? Pos(-1, -1) : candidates.front(); // 실패 시 첫 후보 반환
}

void VCFSearch::printWinningPath() {
    cout << "Winning path size: " << winningPath.size() << endl;
    cout << "Winning path: ";
    for (const auto& move : winningPath) {
        cout << "[" << move.getX() << ", " << (char)(move.getY() + 64) << "] ";
    }
    cout << endl;
}
