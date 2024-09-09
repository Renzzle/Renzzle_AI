#pragma once

#include "../evaluate/evaluator.h"
#include "../tree/tree_manager.h"
#include "../test/test.h"
#include <vector>
#include <limits>

class Search {

private:
    TreeManager treeManager;
    Evaluator evaluator;
    Color targetColor;
    int maxDepth;
    vector<Pos> simulatedPath;
    int alphaBeta(Board& board, int depth, int alpha, int beta, bool maximizingPlayer);
    int ids(Board& board, int depthLimit);
    bool isGameOver(Board& board);
    bool isTargetTurn();

public:
    Search(Board& board, int maxDepth);
    Pos findBestMove();
    Pos iterativeDeepeningSearch();
    vector<Pos> getPath();
    // vector<Pos> getSimulatedPath();

};

Search::Search(Board& board, int maxDepth) : treeManager(board), maxDepth(maxDepth) {
    targetColor = board.isBlackTurn() ? COLOR_BLACK : COLOR_WHITE;
}

int Search::alphaBeta(Board& board, int depth, int alpha, int beta, bool maximizingPlayer) {
    if (depth == 0 || isGameOver(board)) {
        if (depth == 0) simulatedPath = treeManager.getPath();
        return          evaluator.evaluate(board);
    }

    vector<Pos> moves = evaluator.getCandidates(treeManager.getBoard());

    if (moves.empty()) return evaluator.evaluate(board);

    if (maximizingPlayer) {
        int maxEval = MIN_VALUE;

        for (Pos move : moves) {
            treeManager.move(move);
            int eval = alphaBeta(treeManager.getBoard(), depth - 1, alpha, beta, false);
            treeManager.undo();
            maxEval = max(maxEval, eval);
            alpha = max(alpha, eval);
            if (beta <= alpha) break;
        }

        return maxEval;
    } else {
        int minEval = MAX_VALUE;

        for (Pos move : moves) {
            treeManager.move(move);
            int eval = alphaBeta(treeManager.getBoard(), depth - 1, alpha, beta, true);
            treeManager.undo();
            minEval = min(minEval, eval);
            beta = min(beta, eval);
            if (beta <= alpha) break;
        }

        return minEval;
    }
}

int Search::ids(Board& board, int depthLimit) {
    Pos bestMove;
    for (int depth = 1; depth <= depthLimit; depth++) {
        bestMove = findBestMove();
    }
    return evaluator.evaluate(treeManager.getBoard());
}

bool Search::isGameOver(Board& board) {
    Result result = board.getResult();
    return result != ONGOING;
}

bool Search::isTargetTurn() {
    if (treeManager.getBoard().isBlackTurn()) {
        return targetColor == COLOR_BLACK;
    } else {
        return targetColor == COLOR_WHITE;
    }
}

Pos Search::findBestMove() {
    simulatedPath.clear();

    int bestValue = MIN_VALUE;
    Pos bestMove;

    vector<Pos> moves = evaluator.getCandidates(treeManager.getBoard());

    for (Pos move : moves) {
        treeManager.move(move);
        simulatedPath.push_back(move);
        int moveValue = alphaBeta(treeManager.getBoard(), maxDepth - 1, MIN_VALUE, MAX_VALUE, false);
        treeManager.undo();
        simulatedPath.pop_back();

        if (moveValue > bestValue) {
            bestValue = moveValue;
            bestMove = move;
        }
    }

    return bestMove;
}

Pos Search::iterativeDeepeningSearch() {
    int depthLimit = maxDepth;
    return findBestMove();
}

vector<Pos> Search::getPath() {
    vector<Pos> path = treeManager.getPath();
    path.insert(path.end(), simulatedPath.begin(), simulatedPath.end());
    return path;
}

// vector<Pos> Search::getSimulatedPath() {
//     return simulatedPath;
// }