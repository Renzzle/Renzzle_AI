#pragma once

#include "../evaluate/evaluator.h"
#include "../test/test.h"
#include "search_monitor.h"
#include "transposition_table.h"
#include <algorithm>
#include <limits>

class SearchWin {

PRIVATE
    Board board;
    MoveList rootPath;
    SearchMonitor& monitor;
    Color targetColor;
    Result targetResult;
    bool isRunning = false;
    TranspositionTable tt;

    static constexpr uint64_t TURN_KEY_BLACK = 0x9e3779b97f4a7c15ULL;
    static constexpr uint64_t TURN_KEY_WHITE = 0xc2b2ae3d27d4eb4fULL;

    bool dfsVCF();
    bool isWin();
    bool isTargetTurn();
    uint64_t getTTKey();
    uint8_t getDepthLeft();
    void moveTTBestFirst(MoveList& moves, const TTEntry* entry);

PUBLIC
    SearchWin(Board& board, SearchMonitor& monitor);
    bool findVCF();
    void stop();
    size_t getNodeCount() const;
    size_t getEstimatedMemoryBytes() const;

};

SearchWin::SearchWin(Board& initialBoard, SearchMonitor& monitor)
    : board(initialBoard), rootPath(this->board.getPath()), monitor(monitor), tt(64ull * 1024ull * 1024ull, 4) {
    targetColor = this->board.isBlackTurn() ? COLOR_BLACK : COLOR_WHITE;
    targetResult = (targetColor == COLOR_BLACK) ? BLACK_WIN : WHITE_WIN;
}

bool SearchWin::findVCF() {
    if (!isRunning) {
        monitor.initStartTime();
        isRunning = true;
        tt.clear();
        tt.nextGeneration();
    }
    return dfsVCF();
}

bool SearchWin::dfsVCF() {
    monitor.updateElapsedTime();

    if (isWin()) return true;
    if (!isRunning) return false;
    if (board.getResult() != ONGOING) return false;

    const uint64_t key = getTTKey();
    const TTEntry* probed = tt.probe(key);

    // score=0 means this state was already proven as no VCF path.
    if (probed != nullptr && probed->getFlag() == TTFlag::EXACT && probed->score == 0) {
        return false;
    }

    monitor.incVisitCnt();

    Evaluator evaluator(board);
    MoveList moves = isTargetTurn() ? evaluator.getFours() : evaluator.getCandidates();
    moveTTBestFirst(moves, probed);

    for (const auto& move : moves) {
        if (!board.move(move)) continue;

        if (dfsVCF()) {
            board.undo();
            tt.store(key, 1, getDepthLeft(), TTFlag::EXACT, TranspositionTable::encodeMove(move));
            return true;
        }

        board.undo();
        if (!isRunning) break;
    }

    tt.store(key, 0, getDepthLeft(), TTFlag::EXACT, TranspositionTable::INVALID_MOVE);
    return false;
}

bool SearchWin::isWin() {
    Result result = board.getResult();
    bool win = false;

    if (result == BLACK_WIN && targetColor == COLOR_BLACK)
        win = true;
    if (result == WHITE_WIN && targetColor == COLOR_WHITE)
        win = true;

    if (win) {
        const auto& fullPath = board.getPath();

        const size_t rootSize = rootPath.size();
        const size_t totalSize = fullPath.size();
        MoveList bestPath;
        if (totalSize >= rootSize) {
            bestPath.assign(fullPath.begin() + rootSize, fullPath.end());
        }

        monitor.setBestPath(bestPath);
        monitor.setBestValueProvider([bestPath]() {
            return Value(Value::Result::WIN, static_cast<int>(bestPath.size()));
        });
    }

    return win;
}

bool SearchWin::isTargetTurn() {
    if (board.isBlackTurn()) {
        return targetColor == COLOR_BLACK;
    }
    return targetColor == COLOR_WHITE;
}

uint64_t SearchWin::getTTKey() {
    uint64_t key = static_cast<uint64_t>(board.getCurrentHash());
    key ^= board.isBlackTurn() ? TURN_KEY_BLACK : TURN_KEY_WHITE;
    return key;
}

uint8_t SearchWin::getDepthLeft() {
    const size_t pathSize = board.getPath().size();
    size_t remain = 0;
    if (BOARD_SIZE * BOARD_SIZE > pathSize) {
        remain = (BOARD_SIZE * BOARD_SIZE) - pathSize;
    }
    if (remain > std::numeric_limits<uint8_t>::max()) {
        remain = std::numeric_limits<uint8_t>::max();
    }
    return static_cast<uint8_t>(remain);
}

void SearchWin::moveTTBestFirst(MoveList& moves, const TTEntry* entry) {
    if (entry == nullptr) return;
    if (entry->bestMove == TranspositionTable::INVALID_MOVE) return;

    const Pos bestMove = TranspositionTable::decodeMove(entry->bestMove);
    const auto it = std::find(moves.begin(), moves.end(), bestMove);
    if (it != moves.end() && it != moves.begin()) {
        std::iter_swap(moves.begin(), it);
    }
}

void SearchWin::stop() {
    isRunning = false;
}

size_t SearchWin::getNodeCount() const {
    return tt.getUsedEntryCount();
}

size_t SearchWin::getEstimatedMemoryBytes() const {
    return tt.getMemoryBytes();
}
