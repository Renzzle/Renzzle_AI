#include "../evaluate/evaluator.h"
#include "../evaluate/evaluator_v1.h"
#include "transposition_table.h"
#include <vector>
#include <algorithm>
#include <limits>
#include <stack>
#include <list>

class VCFSearch {
    EvaluatorV1 evaluator;
    stack<Pos> path;
    vector<Pos> winningPath;
    TranspositionTable TT;

    Value alphaBeta(bool maximizingPlayer, Value alpha, Value beta, int depth);

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

    Value alpha = MIN_VALUE;
    Value beta = MAX_VALUE;
    Pos bestMove;

    for (const auto& move : candidates) {
        evaluator.next(move);
        path.push(move);

        Value currentEval = alphaBeta(false, alpha, beta, 0);

        if (currentEval > alpha) {
            alpha = currentEval;
            bestMove = move;
        }

        path.pop();
        evaluator.prev();
    }
    return bestMove;
}

Value VCFSearch::alphaBeta(bool maximizingPlayer, Value alpha, Value beta, int depth) {
    HashKey posKey = evaluator.getZobristKey();
    Value ttValue;
    bool ttHit = TT.probe(posKey, ttValue, depth);

    if (ttHit) {
        if (ttValue >= beta) {
            return ttValue;
        }
        alpha = max(alpha, ttValue);
    }

    auto candidates = evaluator.getCandidates();

    if (candidates.empty()) {
        return maximizingPlayer ? MIN_VALUE : MAX_VALUE;
    }

    for (const auto& move : candidates) {
        evaluator.next(move);
        path.push(move);
        Value currentEval = evaluator.evaluate();

        if (currentEval >= 20000) {
            winningPath.clear();
            stack<Pos> tmp = path;
            while (!tmp.empty()) {
                winningPath.push_back(tmp.top());
                tmp.pop();
            }
            reverse(winningPath.begin(), winningPath.end());
            path.pop();
            evaluator.prev();
            TT.store(posKey, currentEval, depth);
            return currentEval;
        }

        if (maximizingPlayer) {
            alpha = max(alpha, alphaBeta(false, alpha, beta, depth + 1));
        } else {
            beta = min(beta, alphaBeta(true, alpha, beta, depth + 1));
        }

        path.pop();
        evaluator.prev();

        if (alpha >= beta) {
            break;
        }
    }

    Value result = maximizingPlayer ? alpha : beta;
    TT.store(posKey, result, depth);
    return result;
}

void VCFSearch::printWinningPath() {
    cout << "Winning path size: " << winningPath.size() << endl;
    cout << "Winning path: ";
    for (const auto& move : winningPath) {
        cout << "[" << move.getX() << ", " << (char)(move.getY() + 64) << "]" << ' ';
    }
    cout << endl;
}