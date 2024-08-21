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
    int alphaBeta(Board& board, int depth, int alpha, int beta, bool maximizingPlayer);
    int ids(Board& board, int depthLimit);
    bool isGameOver(Board& board);
    bool isTargetTurn();

public:
    Search(Board& board, int maxDepth);
    Pos findBestMove();
    Pos iterativeDeepeningSearch();

};

Search::Search(Board& board, int maxDepth) : treeManager(board), maxDepth(maxDepth) {
    targetColor = board.isBlackTurn() ? COLOR_BLACK : COLOR_WHITE;
}

int Search::alphaBeta(Board& board, int depth, int alpha, int beta, bool maximizingPlayer) {
    if (depth == 0 || isGameOver(board)) {
        return evaluator.evaluate(board);
    }

    vector<Pos> moves = treeManager.getBoard().getAllEmptyPositions();

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
    int bestValue = MIN_VALUE;
    Pos bestMove;

    vector<Pos> moves = treeManager.getBoard().getAllEmptyPositions();

    for (Pos move : moves) {
        treeManager.move(move);
        int moveValue = alphaBeta(treeManager.getBoard(), maxDepth - 1, MIN_VALUE, MAX_VALUE, false);
        treeManager.undo();

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