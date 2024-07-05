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
    Pos findBestMove(Depth depth, bool maximizingPlayer);
};

void Search::setEvaluator(Evaluator eval) {
    this->evaluator = eval;
}

Pos Search::findBestMove(Depth depth, bool maximizingPlayer) {
    Value bestValue = -INF;
    Pos bestMove;

    //cout << "findBestMove ";
    auto candidates = evaluator.getCandidates();
    cout << endl;
    
    for (const auto& move : candidates) {
        evaluator.next(move);
        //cout << "-------------- move: [" << move.getX() << ", " << (char)(move.getY() + 64) << "] --------------" << endl;
        cout << "move: [" << move.getX() << ", " << (char)(move.getY() + 64) << "] ";
        Value moveValue = alphaBeta(depth - 1, -INF, INF, !maximizingPlayer);
        cout << ">> moveValue: " << moveValue << endl;
        evaluator.prev();

        if (moveValue > bestValue) {
            bestValue = moveValue;
            bestMove = move;
        }
    }
    return bestMove;
}

Value Search::alphaBeta(Depth depth, Value alpha, Value beta, bool maximizingPlayer) {
    Value currentEval = evaluator.evaluate();

    if (depth == 0 || currentEval == 20000) {
        return currentEval + depth;
    }

    auto candidates = evaluator.getCandidates();
    if (candidates.empty()) {
        return currentEval;
    }

    if (maximizingPlayer) {
        Value maxEval = -INF;
        for (const auto& move : candidates) {
            evaluator.next(move);
            Value eval = alphaBeta(depth - 1, alpha, beta, !maximizingPlayer);
            evaluator.prev();
            maxEval = std::max(maxEval, eval);
            alpha = std::max(alpha, eval);
            // if(eval >= 20000) 
            //     TEST_PRINT("depth: " << depth << "/ move: " << move.getX() << ", " << (char)(move.getY() + 64));
            if (beta <= alpha)    break;
        }
        return maxEval * -1;
    } else {
        Value minEval = INF;
        for (const auto& move : candidates) {
            evaluator.next(move);
            Value eval = alphaBeta(depth - 1, alpha, beta, !maximizingPlayer);
            evaluator.prev();
            minEval = std::min(minEval, eval);
            beta = std::min(beta, eval);
            // if(eval >= 20000) 
            //     TEST_PRINT("depth:" << depth << " move:" << move.getX() << ", " << (char)(move.getY() + 64));
            if (beta <= alpha)    break;
        }
        return minEval * -1;
    }
}