#pragma once

#include "../search/search.h"
#include "../search/search_win.h"
#include "../test/util.h"

#define MAX_THINKING_TIME 10.0    // validatePuzzle soft timeout
#define HARD_THINKING_TIME 20.0   // validatePuzzle hard timeout (grace for QVCF win → EXACT)
#define FIND_NEXT_MOVE_TIME 10.0   // findNextMove timeout

struct FindNextMoveAnalysis {
    int move = -1;
    Value value;
    MoveList path;
    int completedDepth = 0;
    size_t visitedNodes = 0;
    double elapsedSeconds = 0.0;
    bool usedSureMove = false;
    bool usedFallback = false;
    vector<Search::RootMoveStat> rootStats;
};

string validatePuzzle(string boardStr) {
    Board board = getBoard(boardStr);

    if (board.getResult() != ONGOING) return "";

    SearchMonitor monitor;
    Search searcher(board, monitor);

    monitor.setTrigger([](SearchMonitor& m) {
        const double t = m.getElapsedTime();
        if (t >= HARD_THINKING_TIME) return true;
        if (t >= MAX_THINKING_TIME) return !m.getBestValue().isWin();
        return false;
    });
    monitor.setSearchListener([&searcher](SearchMonitor&) {
        searcher.stop();
    });

    searcher.ids();
    if (monitor.getBestValue().isWin())
        return convertPath2String(monitor.getBestPath());

    return "";
}

int convertMoveToInt(Pos& move) {
    if (move.isDefault()) return -1;
    int result = (move.getY() - 1) * BOARD_SIZE + move.getX() - 1;
    return result;
}

FindNextMoveAnalysis analyzeNextMove(string boardStr) {
    FindNextMoveAnalysis analysis;

    Board board = getBoard(boardStr);
    if (board.getResult() != ONGOING) return analysis;

    Evaluator evaluator(board);
    Pos sureMove = evaluator.getSureMove();
    if (!sureMove.isDefault()) {
        analysis.move = convertMoveToInt(sureMove);
        analysis.path.push_back(sureMove);
        analysis.usedSureMove = true;
        return analysis;
    }

    SearchMonitor monitor;
    Search searcher(board, monitor);
    searcher.setMode(Search::Mode::DEFENSIVE);

    monitor.setTrigger([](SearchMonitor& m) {
        Value bv = m.getBestValue();
        if (bv.isWin()) return true;
        // LOSE in DEFENSIVE is now genuine (threatBrokenLeaf is VCT-only, defender
        // candidates aren't shallow-truncated), so longest-delay LOSE is confirmed
        // once ABP reports it — no need to keep deepening.
        if (bv.isLose() && !bv.isQVCFDerived()
            && (bv.getType() == Value::Type::EXACT || bv.getType() == Value::Type::UPPER_BOUND)) {
            return true;
        }
        return m.getElapsedTime() >= FIND_NEXT_MOVE_TIME;
    });
    monitor.setSearchListener([&searcher](SearchMonitor&) {
        searcher.stop();
    });

    searcher.ids();

    analysis.value = monitor.getBestValue();
    analysis.path = monitor.getBestPath();
    analysis.completedDepth = monitor.getDepth();
    analysis.visitedNodes = monitor.getVisitCnt();
    analysis.elapsedSeconds = monitor.getElapsedTime();
    analysis.rootStats = searcher.getLastRootStats();

    if (!analysis.path.empty()) {
        analysis.move = convertMoveToInt(analysis.path[0]);
        return analysis;
    }

    // safety net: no completed iteration → pick any reasonable candidate
    MoveList moves = evaluator.getCandidates();
    if (!moves.empty()) {
        Pos fallback = moves[0];
        analysis.move = convertMoveToInt(fallback);
        analysis.path.push_back(fallback);
        analysis.usedFallback = true;
        return analysis;
    }
    return analysis;
}

int findNextMove(string boardStr) {
    return analyzeNextMove(boardStr).move;
}
