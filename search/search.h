#include "../evaluate/evaluator.h"
#include <vector>
#include <limits>

constexpr Value INF = std::numeric_limits<Value>::max();

class Search {
private:
    Evaluator evaluator;
    Value alphaBeta(Depth depth, Value alpha, Value beta, bool maximizingPlayer);

public:
    void setEvaluator(Evaluator eval);
    Pos findBestMove(Depth depth);
};

void Search::setEvaluator(Evaluator eval) {
    this->evaluator = eval;
}

Pos Search::findBestMove(Depth depth) {
    Value bestValue = -INF;
    Pos bestMove;
    auto candidates = evaluator.getCandidates();
    
    for (const auto& move : candidates) {
        evaluator.next(move);
        Value moveValue = alphaBeta(depth - 1, -INF, INF, false);
        cout << "move: [" << move.getX() << ", " << (char)(move.getY() + 64) << "],\tmoveValue: " << moveValue << endl;
        evaluator.prev();

        if (moveValue > bestValue) {
            bestValue = moveValue;
            bestMove = move;
        }
    }
    return bestMove;
}

Value Search::alphaBeta(Depth depth, Value alpha, Value beta, bool maximizingPlayer) {
    if (depth == 0) {
        return evaluator.evaluate(maximizingPlayer ? COLOR_BLACK : COLOR_WHITE, depth);
    }

    auto candidates = evaluator.getCandidates();
    if (candidates.empty()) {
        return evaluator.evaluate(maximizingPlayer ? COLOR_BLACK : COLOR_WHITE, depth);
    }

    if (maximizingPlayer) {
        Value maxEval = -INF;
        for (const auto& move : candidates) {
            evaluator.next(move);
            cout << ">> Maximizing Depth: " << depth;
            cout << " / Alpha: " << (alpha == -INF ? "-INF" : to_string(alpha));
            cout << " / Beta: " << (beta == INF ? "INF" : to_string(beta)) << endl;
            Value eval = alphaBeta(depth - 1, alpha, beta, false);
            evaluator.prev();
            maxEval = std::max(maxEval, eval);
            alpha = std::max(alpha, eval);
            if (beta <= alpha) {
                cout << "** Pruning at move [" << move.getX() << ", " << (char)(move.getY() + 64) << "] with beta: " << beta << " **" << endl;
                break;  
            }
        }
        return maxEval;
    } else {
        Value minEval = INF;
        for (const auto& move : candidates) {
            evaluator.next(move);
            cout << ">> Minimizing Depth: " << depth;
            cout << " / Alpha: " << (alpha == -INF ? "-INF" : to_string(alpha));
            cout << " / Beta: " << (beta == INF ? "INF" : to_string(beta)) << endl;
            Value eval = alphaBeta(depth - 1, alpha, beta, true);
            evaluator.prev();
            minEval = std::min(minEval, eval);
            beta = std::min(beta, eval);
            if (beta <= alpha) {
                cout << "** Pruning at move [" << move.getX() << ", " << (char)(move.getY() + 64) << "] with alpha: " << alpha << " **" << endl;
                break;  
            }
        }
        return minEval;
    }
}
