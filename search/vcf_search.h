#include "../evaluate/evaluator.h"
#include <vector>
#include <algorithm>
#include <limits>

constexpr Value INF = std::numeric_limits<Value>::max();

class VCFSearch {
    Evaluator evaluator;
    stack<Pos> path;
    vector<Pos> winningPath;

    Value alphaBeta(Value alpha, Value beta, bool maximizingPlayer);

public:
    void setEvaluator(Evaluator eval);
    Pos findBestMove();
    void printWinningPath();
};

void VCFSearch::setEvaluator(Evaluator eval) {
    this->evaluator = eval;
}

Pos VCFSearch::findBestMove() {
    Value bestValue = -INF;
    Pos bestMove;

    evaluator.setVCFColor();
    auto candidates = evaluator.getCandidates();
    
    for (const auto& move : candidates) {
        evaluator.next(move);
        path.push(move);
        Value moveValue = alphaBeta(-INF, INF, false);
        TEST_PRINT("bestMove: " << move.getX() << (char)(move.getY() + 64) << "'s eval: " << moveValue);
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

Value VCFSearch::alphaBeta(Value alpha, Value beta, bool maximizingPlayer) {
    Value currentEval = evaluator.evaluate();
    auto candidates = evaluator.getCandidates();

    // for (const auto& move : candidates) {
    //     auto move1 = candidates.front();
    //     if(move1.getX() != 8 || move1.getY() != 7) {
    //         cout << "only h7\n";
    //         break;
    //     } else cout << "no\n";
    //     cout << move.getX() << (char)(move.getY() + 64) << ", ";
    // }

    if(!winningPath.empty() && winningPath.size() < path.size()) {
        return currentEval;
    }

    if (currentEval != 0) {
        if(currentEval >= 20000) {
            winningPath.clear();
            stack<Pos> tmp = path;
            while (!tmp.empty()) {
                winningPath.push_back(tmp.top()); 
                tmp.pop();
            }
            reverse(winningPath.begin(), winningPath.end());
            //printWinningPath();
        }
        return currentEval;
    }

    if (candidates.empty()) {
        return currentEval;
    }

    if (maximizingPlayer) {
        Value maxEval = -INF;
        for (const auto& move : candidates) {
            evaluator.next(move);
            path.push(move);
            Value eval = alphaBeta(alpha, beta, !maximizingPlayer);
            //printWinningPath();
            //TEST_PRINT("max: " << move.getX() << (char)(move.getY() + 64) << "'s eval: " << eval);
            path.pop();
            evaluator.prev();
            maxEval = std::max(maxEval, eval);
            alpha = std::max(alpha, eval);
            if (beta <= alpha) break;
        }
        return maxEval;
    } else {
        Value minEval = INF;
        for (const auto& move : candidates) {
            evaluator.next(move);
            path.push(move);
            Value eval = alphaBeta(alpha, beta, !maximizingPlayer);
            //printWinningPath();
            //TEST_PRINT("min: " << move.getX() << (char)(move.getY() + 64) << "'s eval: " << eval);
            path.pop();
            evaluator.prev();
            minEval = std::min(minEval, eval);
            beta = std::min(beta, eval);
            if (beta <= alpha) break;
        }
        return minEval;
    }
}

void VCFSearch::printWinningPath() {
    cout << "Winning path size: " << winningPath.size() << endl;
    cout << "Winning path: ";
    for (const auto& move : winningPath) {
        cout << "[" << move.getX() << ", " << (char)(move.getY() + 64) << "]" << ' ';
    }
    cout << endl;
    // cout << "Winning path: ";
    // stack<Pos> path = this->path;
    // while (!path.empty()) {
    //     cout << "[" << path.top().getX() << ", " << (char)(path.top().getY() + 64) << "]" << ' ';
    //     path.pop();
    // }
    // cout << endl;
}