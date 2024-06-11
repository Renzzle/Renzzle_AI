#include <vector>
#include <limits>
#include <algorithm> // for std::max and std::min

using namespace std;
using Value = int; 
using Depth = int; 

constexpr Value INF = numeric_limits<Value>::max();

using Moves = vector<pair<int, int>>;

using EvalFunc = Value(*)();
using MoveFunc = Moves(*)();
using NextFunc = void(*)(int, int);
using UndoFunc = void(*)(int, int);



// using IsGameOverFunc = bool(*)();

Value alphaBeta(Depth depth, Value alpha, Value beta, bool maximizingPlayer,
                EvalFunc eval, MoveFunc move, NextFunc next, 
                UndoFunc undo) {
                    //, IsGameOverFunc isGameOver
    if (depth == 0) {//|| isGameOver()
        return eval();
    }

    // if (depth == 0 || isGameOver()) {
    //     return eval();
    // }

    Moves moves = move();

    if (maximizingPlayer) {
        Value maxEval = -INF;
        for (const auto &m : moves) {
            next(m.first, m.second);
            Value evalValue = alphaBeta(depth - 1, alpha, beta, false, eval, move, next, undo);
            undo(m.first, m.second);
            maxEval = max(maxEval, evalValue);
            if (maxEval >= beta) {
                break;
            }
            alpha = max(alpha, maxEval);
        }
        return maxEval;
    } else {
        Value minEval = INF;
        for (const auto &m : moves) {
            next(m.first, m.second);
            Value evalValue = alphaBeta(depth - 1, alpha, beta, true, eval, move, next, undo);
            undo(m.first, m.second);
            minEval = min(minEval, evalValue);
            if (minEval <= alpha) {
                break;
            }
            beta = min(beta, minEval);
        }
        return minEval;
    }
}

void main(){
    
}