#pragma once

#include "../evaluate/evaluator.h"
#include "../tree/tree_manager.h"
#include "../test/test.h"
#include "vcf_search.h"
#include "search_monitor.h"
#include <limits>

class Search {

PRIVATE
    TreeManager treeManager;
    Color targetColor;
    SearchMonitor& monitor;

    Value alphaBeta(Board& board, int depth, int alpha, int beta, bool maximizingPlayer);
    int ids(Board& board, int depthLimit);
    bool isGameOver(Board& board);
    bool isTargetTurn();

PUBLIC
    Search(Board& board, SearchMonitor& monitor);
    Pos findBestMove();
    Pos iterativeDeepeningSearch();
    Pos findNextMove(Board board);
    MoveList getPath();
    MoveList getSimulatedPath();

};

Search::Search(Board& board, SearchMonitor& monitor) : treeManager(board), monitor(monitor) {
    targetColor = board.isBlackTurn() ? COLOR_BLACK : COLOR_WHITE;
}

Value Search::alphaBeta(Board& board, int depth, int alpha, int beta, bool maximizingPlayer) {
    monitor.incVisitCnt();

    Evaluator evaluator(treeManager.getBoard());

    // end condition
    if (depth <= 0 || isGameOver(board)) {
        Value val = evaluator.evaluate();
        if (!maximizingPlayer) {
            val = -val;
        }
        if (val > monitor.getBestValue()) {
            monitor.setBestValue(val);
            monitor.setBestPath(treeManager.getBoard().getPath());
        }
        return val;
    }

    MoveList moves = evaluator.getCandidates();

    if (moves.empty()) return evaluator.evaluate();

    if (maximizingPlayer) {
        int maxEval = MIN_VALUE;

        for (Pos move : moves) {
            treeManager.move(move);
            Value eval = alphaBeta(treeManager.getBoard(), depth - 1, alpha, beta, false);
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
            Value eval = alphaBeta(treeManager.getBoard(), depth - 1, alpha, beta, true);
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
    Evaluator evaluator(treeManager.getBoard());
    return evaluator.evaluate();
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
    monitor.getBestPath().clear();

    int bestValue = MIN_VALUE;
    Pos bestMove;

    Evaluator evaluator(treeManager.getBoard());
    vector<Pos> moves = evaluator.getCandidates();

    for (Pos move : moves) {
        treeManager.move(move);
        monitor.getBestPath().push_back(move);
        int moveValue = alphaBeta(treeManager.getBoard(), monitor.getMaxDepth() - 1, MIN_VALUE, MAX_VALUE, false);
        treeManager.undo();
        monitor.getBestPath().pop_back();

        if (moveValue > bestValue) {
            bestValue = moveValue;
            bestMove = move;
        }
    }

    return bestMove;
}

Pos Search::findNextMove(Board board) {
    if (board.getResult() != ONGOING) return Pos();

    Evaluator evaluator(board);
    Pos sureMove = evaluator.getSureMove();
    if (!sureMove.isDefault()) {
        return sureMove;
    }

    SearchMonitor vcfMonitor;
    VCFSearch vcfSearcher(board, vcfMonitor);
    if (vcfSearcher.findVCF()) {
        return vcfMonitor.getBestPath()[board.getPath().size()];
    }

    if (evaluator.isOppoMateExist()) {
        MoveList defends = evaluator.getThreatDefend();
        MoveList candidates;
        for (auto move : defends) {
            board.move(move);
            Board tmpBoard = board;
            SearchMonitor vctMonitor;
            VCFSearch vctSearcher(tmpBoard, vctMonitor);
            if (!vctSearcher.findVCT(7)) {
                candidates.push_back(move);
            }
            board.undo();
        }
        if (candidates.empty()) {
            return defends.front();
        }
        else {
            return candidates.front();
        }
    }

    SearchMonitor vctMonitor;
    VCFSearch vctSearcher(board, vctMonitor);
    if (vctSearcher.findVCT(9)) {
        return vctMonitor.getBestPath()[board.getPath().size()];
    }

    MoveList candidates = evaluator.getCandidates();
    if (!candidates.empty())
        return candidates.front();
    
    return Pos();
}

Pos Search::iterativeDeepeningSearch() {
    int depthLimit = monitor.getMaxDepth();
    return findBestMove();
}

MoveList Search::getPath() {
    return monitor.getBestPath();
}