#pragma once

#include "../evaluate/evaluator.h"
#include "../tree/tree.h"
#include "../tree/tree_manager.h"
#include <vector>

class VCFSearch {

PRIVATE
    TreeManager treeManager;
    Evaluator evaluator;
    Color targetColor;
    bool isWin();
    bool isTargetTurn();

PUBLIC
    VCFSearch(Board& board);
    bool findVCF();
    vector<Pos> getVCFPath();

};

VCFSearch::VCFSearch(Board& board) : treeManager(board) {
    targetColor = board.isBlackTurn() ? COLOR_BLACK : COLOR_WHITE;
}

bool VCFSearch::isWin() {
    Result result = treeManager.getBoard().getResult();
    if (result == BLACK_WIN && targetColor == COLOR_BLACK)
        return true;
    if (result == WHITE_WIN && targetColor == COLOR_WHITE)
        return true;
    return false;
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

bool VCFSearch::findVCF() {
    if (isWin()) return true;
    
    vector<Pos> moves;
    if (isTargetTurn())
        moves = evaluator.getFours(treeManager.getBoard());
    else 
        moves = evaluator.getCandidates(treeManager.getBoard());

    if (moves.empty()) return false;

    for (auto move : moves) {
        treeManager.move(move);
        if (findVCF()) return true;
        treeManager.undo();
    }

    return false;
}

vector<Pos> VCFSearch::getVCFPath() {
    return treeManager.getVCFPath();
}