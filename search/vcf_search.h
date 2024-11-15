#pragma once

#include "../evaluate/evaluator.h"
#include "../tree/tree_manager.h"
#include "../test/test.h"
#include "search_monitor.h"

class VCFSearch {

PRIVATE
    TreeManager treeManager;
    Evaluator evaluator;
    SearchMonitor& monitor;
    Color targetColor;
    bool isInitTime = false;
    
    bool isWin();
    bool isTargetTurn();

PUBLIC
    VCFSearch(Board& board, SearchMonitor& monitor);
    bool findVCF();
    bool findVCT();

};

VCFSearch::VCFSearch(Board& board, SearchMonitor& monitor) : treeManager(board), monitor(monitor) {
    targetColor = board.isBlackTurn() ? COLOR_BLACK : COLOR_WHITE;
}

bool VCFSearch::findVCF() {
    if (!isInitTime) {
        monitor.initStartTime();
        isInitTime = true;
    }
    monitor.incVisitCnt();
    monitor.updateElapsedTime();
    if (isWin()) return true;
    
    // find candidates
    MoveList moves;

    if (isTargetTurn())
        moves = evaluator.getFours(treeManager.getBoard());
    else 
        moves = evaluator.getCandidates(treeManager.getBoard());

    if (moves.empty()) return false;

    // dfs
    for (auto move : moves) {
        shared_ptr<Node> childNode = treeManager.getChildNode(move);
        if (childNode != nullptr) { // child node exist
            Result targetResult = (targetColor == COLOR_BLACK) ? BLACK_WIN : WHITE_WIN;
            // prune except win path
            if (childNode->result != targetResult) continue;
        } else {
            // if a winning path exists, skip searching other nodes
            if (treeManager.currentNode->result != ONGOING) continue;
        }

        treeManager.move(move);
        
        if (findVCF()) {
            // if find vcf, update parent node result
            Result result = treeManager.getNode()->result;
            treeManager.undo();
            treeManager.getNode()->result = result;
            return true;
        }
        
        treeManager.undo();
    }

    return false;
}

bool VCFSearch::findVCT() {
    if (!isInitTime) {
        monitor.initStartTime();
        isInitTime = true;
    }
    monitor.incVisitCnt();
    monitor.updateElapsedTime();
    if (isWin()) return true;

    MoveList moves;
    moves = evaluator.getSureMove(treeManager.getBoard());
    if (!moves.empty()) {
        treeManager.move(moves.front());
        return findVCT();
    }

    if (isTargetTurn()) {
        moves = evaluator.getThreats(treeManager.getBoard());
    } else {
        SearchMonitor vcfMonitor;
        VCFSearch vcfSearch(treeManager.getBoard(), vcfMonitor);
        if (vcfSearch.findVCF()) return false;
        
    }
}

bool VCFSearch::isWin() {
    Result result = treeManager.getBoard().getResult();;
    bool isWin = false;
    
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