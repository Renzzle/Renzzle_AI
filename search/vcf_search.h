#include "../evaluate/evaluator.h"
#include "../evaluate/evaluator_v1.h"
#include <vector>
#include <algorithm>
#include <limits>
#include <stack>
#include <list>

class VCFSearch {
    EvaluatorV1 evaluator;
    std::stack<Pos> path;
    std::vector<Pos> winningPath;

    bool dfs(bool maximizingPlayer);

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
        if (dfs(false)) {  // dfs 탐색을 통해 VCF 승리가 가능한지 확인
            printWinningPath();
            path.pop();
            evaluator.prev();
            return move;  // VCF 승리가 가능한 첫번째 착수 위치를 반환
        }
        path.pop();
        evaluator.prev();
    }
    return Pos();  // VCF 승리가 가능한 위치가 없는 경우 빈 위치 반환
}

bool VCFSearch::dfs(bool maximizingPlayer) {
    auto candidates = evaluator.getCandidates();

    if (candidates.empty()) {
        return false;  // 더 이상 후보가 없는 경우
    }

    for (const auto& move : candidates) {
        evaluator.next(move);
        path.push(move);
        Value currentEval = evaluator.evaluate();
        
        if (currentEval >= 20000) {  // VCF 승리가 확인된 경우
            winningPath.clear();
            std::stack<Pos> tmp = path;
            while (!tmp.empty()) {
                winningPath.push_back(tmp.top());
                tmp.pop();
            }
            std::reverse(winningPath.begin(), winningPath.end());
            path.pop();
            evaluator.prev();
            return true;
        }

        if (dfs(!maximizingPlayer)) {  // 재귀적으로 DFS 탐색
            path.pop();
            evaluator.prev();
            return true;
        }
        path.pop();
        evaluator.prev();
    }
    return false;
}

void VCFSearch::printWinningPath() {
    std::cout << "Winning path size: " << winningPath.size() << std::endl;
    std::cout << "Winning path: ";
    for (const auto& move : winningPath) {
        std::cout << "[" << move.getX() << ", " << (char)(move.getY() + 64) << "]" << ' ';
    }
    std::cout << std::endl;
}
