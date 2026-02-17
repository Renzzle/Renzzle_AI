#pragma once

#include "../evaluate/evaluator.h"
#include "../tree/tree_manager.h"
#include "../test/test.h"
#include "search_monitor.h"

class SearchWin {

PRIVATE
    TreeManager treeManager;
    SearchMonitor& monitor;
    Color targetColor;
    Result targetResult;
    bool isRunning = false;
    
    bool isWin();
    bool isTargetTurn();

PUBLIC
    SearchWin(Board& board, SearchMonitor& monitor);
    bool findVCF();
    void stop();
    size_t getNodeCount() const;
    size_t getEstimatedMemoryBytes() const;

};

SearchWin::SearchWin(Board& board, SearchMonitor& monitor) : treeManager(board), monitor(monitor) {
    targetColor = board.isBlackTurn() ? COLOR_BLACK : COLOR_WHITE;
    targetResult = (targetColor == COLOR_BLACK) ? BLACK_WIN : WHITE_WIN;
}

bool SearchWin::findVCF() {
    if (!isRunning) {
        monitor.initStartTime();
        isRunning = true;
    }
    monitor.incVisitCnt();
    monitor.updateElapsedTime();
    if (isWin()) return true;
    if (!isRunning) return false;
    if (treeManager.getBoard().getResult() != ONGOING) return false;
    
    // find candidates
    Evaluator evaluator(treeManager.getBoard());
    MoveList moves;

    if (isTargetTurn())
        moves = evaluator.getFours();
    else 
        moves = evaluator.getCandidates();

    // dfs
    for (auto move : moves) {
        Node* childNode = treeManager.getChildNode(move);
        if (childNode != nullptr) { // child node exist
            // prune except win path
            continue;
        }

        treeManager.move(move);
        
        if (findVCF()) {
            // if find vcf, update parent node result
            return true;
        }
        
        treeManager.undo();
        if (!isRunning) break;
    }

    return false;
}

bool SearchWin::isWin() {
    Result result = treeManager.getBoard().getResult();;
    bool isWin = false;
    
    if (result == BLACK_WIN && targetColor == COLOR_BLACK)
        isWin = true;
    if (result == WHITE_WIN && targetColor == COLOR_WHITE)
        isWin = true;

    // if win, update current node result & set vcf path
    if (isWin) {        
        const auto& fullPath = treeManager.getNode()->board.getPath();
        const auto& rootPath = treeManager.getRootNode()->board.getPath();

        int rootSize = static_cast<int>(rootPath.size());
        int totalSize = static_cast<int>(fullPath.size());

        MoveList bestPath(fullPath.begin() + rootSize, fullPath.end());

        monitor.setBestPath(bestPath);
        monitor.setBestValueProvider([&bestPath]() {
            int resultDepth = bestPath.size();
            return Value(Value::Result::WIN, resultDepth);
        });
    }

    return isWin;
}

bool SearchWin::isTargetTurn() {
    if (treeManager.getBoard().isBlackTurn()) {
        if (targetColor == COLOR_BLACK) return true;
        else return false;
    } else {
        if (targetColor == COLOR_BLACK) return false;
        else return true;
    }
}

void SearchWin::stop() {
    isRunning = false;
}

size_t SearchWin::getNodeCount() const {
    return treeManager.getNodeCount();
}

size_t SearchWin::getEstimatedMemoryBytes() const {
    return treeManager.getEstimatedMemoryBytes();
}