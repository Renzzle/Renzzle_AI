#pragma once

#include "../evaluate/evaluator.h"
#include "../tree/tree_manager.h"
#include "../test/test.h"
#include "search_monitor.h"
#include <vector>
#include <functional>
#include <chrono>

using Timestamp = chrono::time_point<chrono::high_resolution_clock>;

class VCFSearch {

PRIVATE
    TreeManager treeManager;
    Evaluator evaluator;
    SearchMonitor& monitor;
    Color targetColor;
    
    bool isWin();
    bool isTargetTurn();

PUBLIC
    VCFSearch(Board& board, SearchMonitor& monitor);
    bool findVCF();

};

VCFSearch::VCFSearch(Board& board, SearchMonitor& monitor) : treeManager(board), monitor(monitor) {
    targetColor = board.isBlackTurn() ? COLOR_BLACK : COLOR_WHITE;
}

bool VCFSearch::findVCF() {
    monitor.incVisitCnt();
    monitor.updateElapsedTime();
    if (isWin()) return true;
    
    MoveList moves;
    if (isTargetTurn())
        moves = evaluator.getFours(treeManager.getBoard());
    else 
        moves = evaluator.getCandidates(treeManager.getBoard());

    if (moves.empty()) return false;

    for (auto move : moves) {
        // if visited child node, prune except win path 
        if (treeManager.isVisited(move)) {
            shared_ptr<Node> childNode = treeManager.getChildNode(move);
            if (childNode == nullptr) continue;

            Result targetResult = targetColor == COLOR_BLACK ? BLACK_WIN : WHITE_WIN;
            if (childNode->result != targetResult) continue;
        } else {
            if (treeManager.currentNode->result != ONGOING) continue;
        }

        treeManager.move(move);
        
        if (findVCF()) {
            Result result = treeManager.getNode()->result;
            treeManager.undo();
            treeManager.getNode()->result = result;
            return true;
        }
        
        treeManager.undo();
    }

    return false;
}

bool VCFSearch::isWin() {
    Result result;
    bool isWin = false;

    result = treeManager.getBoard().getResult();
    
    if (result == BLACK_WIN && targetColor == COLOR_BLACK)
        isWin = true;
    if (result == WHITE_WIN && targetColor == COLOR_WHITE)
        isWin = true;

    // if win, update current node result & set vcf path
    if (isWin) {
        treeManager.getNode()->result = result;
        monitor.setBestPath(treeManager.getNode()->board.getPath());
    }

    return isWin;
}

bool VCFSearch::isTargetTurn() {
    if (treeManager.getBoard().isBlackTurn()) {
        if (targetColor == COLOR_BLACK) return true;
        else return false;
    } else {
        if (targetColor == COLOR_BLACK) return false;
        else return true;
    }
}