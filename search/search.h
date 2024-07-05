#include "../evaluate/evaluator.h"
#include <vector>
#include <limits>
#include <algorithm>

Value INF = std::numeric_limits<Value>::max();

class Search {
private:
    Evaluator evaluator;
    stack<Pos> path;
    vector<Pos> winningPath;

    Value alphaBeta(Depth depth, Value alpha, Value beta, bool maximizingPlayer);

public:
    void setEvaluator(Evaluator eval);
    Pos findBestMove(Depth depth, bool maximizingPlayer);
    void printWinningPath();
};

void Search::setEvaluator(Evaluator eval) {
    this->evaluator = eval;
}

Pos Search::findBestMove(Depth depth, bool maximizingPlayer) {
    Value bestValue = -INF;
    Pos bestMove;

    auto candidates = evaluator.getCandidates();
    for (const auto& move : candidates) {
        evaluator.next(move);
        path.push(move);
        Value moveValue = alphaBeta(depth - 1, -INF, INF, !maximizingPlayer);
        path.pop();
        evaluator.prev();

        if (moveValue > bestValue) {
            bestValue = moveValue;
            bestMove = move;
           
        }
        if(moveValue >= 20000){
            break;
        }
    }
    return bestMove;
}

Value Search::alphaBeta(Depth depth, Value alpha, Value beta, bool maximizingPlayer) {
    Value currentEval = evaluator.evaluate();
    auto candidates = evaluator.getCandidates();


    if (depth == 0 || currentEval >= 20000) {
        if(currentEval >= 20000){
            winningPath.clear();
            stack<Pos> tmp = path;
            while (!tmp.empty()) {
                winningPath.push_back(tmp.top());// tmp > winningPath
                tmp.pop();
            }
            reverse(winningPath.begin(), winningPath.end());  // reverse
            printWinningPath();
        }
        return currentEval + depth;
    }

    if (candidates.empty()) {
        return currentEval;
    }

    if (maximizingPlayer) {
        Value maxEval = -INF;
        for (const auto& move : candidates) {
            evaluator.next(move);
            path.push(move);
            Value eval = alphaBeta(depth - 1, alpha, beta, !maximizingPlayer);
            path.pop();
            evaluator.prev();
            maxEval = std::max(maxEval, eval);
            alpha = std::max(alpha, eval);
            if (beta <= alpha)    break;
        }
        return maxEval * -1;
    } else {
        Value minEval = INF;
        for (const auto& move : candidates) {
            evaluator.next(move);
            path.push(move);
            Value eval = alphaBeta(depth - 1, alpha, beta, !maximizingPlayer);
            path.pop();
            evaluator.prev();
            minEval = std::min(minEval, eval);
            beta = std::min(beta, eval);
            if (beta <= alpha)    break;
        }
        return minEval * -1;
    }
}

void Search::printWinningPath() {
    cout << "Winning path size: " << winningPath.size() << endl;
    cout << "Winning path: ";
    for (const auto& move : winningPath) {
        cout << "[" << move.getX() << ", " << (char)(move.getY() + 64) << "]" << '\t';
    }
    cout << endl;
}