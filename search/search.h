#pragma once

#include "../evaluate/evaluator.h"
#include "../tree/tree_manager.h"
#include "../test/test.h"
#include <vector>
#include <limits>
#include <unordered_map>

class Search {

private:
    TreeManager treeManager;
    Evaluator evaluator;
    Color targetColor;
    int maxDepth;
    vector<Pos> path;
    int alphaBeta(Board& board, int depth, int alpha, int beta, bool maximizingPlayer);
    int ids(Board& board, int depthLimit);
    bool isGameOver(Board& board);
    bool isTargetTurn();
    unordered_map<size_t, int> transpositionTable;  // 탐색 결과 캐싱 테이블

public:
    Search(Board& board, int maxDepth);
    Pos findBestMove();
    Pos iterativeDeepeningSearch();
    vector<Pos> getPath();
    vector<Pos> getSimulatedPath();

};

Search::Search(Board& board, int maxDepth) : treeManager(board), maxDepth(maxDepth) {
    targetColor = board.isBlackTurn() ? COLOR_BLACK : COLOR_WHITE;
}

int Search::alphaBeta(Board& board, int depth, int alpha, int beta, bool maximizingPlayer) {
    size_t boardHash = board.getCurrentHash();

    // 이미 탐색한 보드 상태라면, 해당 보드 상태에 캐시된 값을 반환
    if (transpositionTable.find(boardHash) != transpositionTable.end()) {
        return transpositionTable[boardHash];
    }

    // 탐색 트리의 terminal node에 도달하거나, 게임 종료 시, 해당 보드 상태에 대한 평가 값 캐싱 후 리턴
    if (depth == 0 || isGameOver(board)) {
        if (depth == 0) path = treeManager.getBoard().getPath();

        transpositionTable[boardHash] = evaluator.evaluate(board);
        return evaluator.evaluate(board);
    }

    vector<Pos> moves = evaluator.getCandidates(treeManager.getBoard());

    // 더 이상 탐색할 후보지가 없는 경우, 해당 보드 상태에 대한 평가 값 캐싱 후 리턴
    if (moves.empty()) {
        transpositionTable[boardHash] = evaluator.evaluate(board);
        return evaluator.evaluate(board);
    }

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
        transpositionTable[boardHash] = maxEval;
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
        transpositionTable[boardHash] = minEval;
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
    path.clear();

    int bestValue = MIN_VALUE;
    Pos bestMove;

    vector<Pos> moves = evaluator.getCandidates(treeManager.getBoard());

    for (Pos move : moves) {
        treeManager.move(move);
        path.push_back(move);
        int moveValue = alphaBeta(treeManager.getBoard(), maxDepth - 1, MIN_VALUE, MAX_VALUE, false);
        treeManager.undo();
        path.pop_back();

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
    return path;
}