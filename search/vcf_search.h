#pragma once

#include "../evaluate/evaluator.h"
#include "../tree/tree_manager.h"
#include "../test/test.h"
#include <vector>
#include <functional>
#include <chrono>

using Timestamp = chrono::time_point<chrono::high_resolution_clock>;

class VCFSearch {

PRIVATE
    TreeManager treeManager;
    Evaluator evaluator;
    Color targetColor;
    MoveList vcfPath;
    
    bool isWin();
    bool isTargetTurn();
    void executeTrigger();

    once_flag initTimeFlag;
    Timestamp startTime;
    size_t visitCnt;
    function<bool(double)> trigger;
    function<void(double, size_t, MoveList)> searchListener;

PUBLIC
    VCFSearch(Board& board);
    bool findVCF();
    MoveList getVCFPath();
    size_t getVisitCnt();
    void setTrigger(function<bool(double)> newTrigger);
    void setSearchListener(function<void(double, size_t, MoveList)> newSearchListener);

};

VCFSearch::VCFSearch(Board& board) : treeManager(board) {
    targetColor = board.isBlackTurn() ? COLOR_BLACK : COLOR_WHITE;

    trigger = [](double elapsedTime) { return false; };

    searchListener = [](double elapsedTime, size_t visitCnt, MoveList path) {
        return;
    };
}

bool VCFSearch::findVCF() {
    executeTrigger();
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
        vcfPath = treeManager.getNode()->board.getPath();
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

MoveList VCFSearch::getVCFPath() {
    return vcfPath;
}

size_t VCFSearch::getVisitCnt() {
    return visitCnt;
}

void VCFSearch::executeTrigger() {
    call_once(initTimeFlag, [this]() {
        startTime = chrono::high_resolution_clock::now();
        visitCnt = 0;
    });

    visitCnt++;
    Timestamp now = chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(now - startTime);
    double seconds = duration.count() / 1e9;
    if(trigger(seconds)) {
        searchListener(seconds, visitCnt, treeManager.getBoard().getPath());
    }
}

void VCFSearch::setTrigger(std::function<bool(double)> newTrigger) {
    trigger = newTrigger;
}

void VCFSearch::setSearchListener(std::function<void(double, size_t, MoveList)> newSearchListener) {
    searchListener = newSearchListener;
}