#pragma once

#include "../test/util.h"
#include "../evaluate/evaluator.h"
#include "../tree/tree.h"
#include <vector>

class TreeManager {

private:
    Board board;
    vector<Pos> path;

public:
    TreeManager(Board& board) {
        this->board = board;
    }
    Board& next(Pos pos) {
        path.push_back(pos);
        board.move(pos);
        return board;
    }
    Board& prev() {
        path.pop_back();
        board.undo();
        return board;
    }
    Board& getBoard() {
        return board;
    }
    vector<Pos> getPath() {
        return path;
    }

};

class VCFSearchV2 {

private:
    TreeManager treeManager;
    Evaluator evaluator;
    Color targetColor;
    bool isWin();
    bool isTargetTurn();

public:
    VCFSearchV2(Board& board);
    bool findVCF();
    vector<Pos> getVCFPath();

};

VCFSearchV2::VCFSearchV2(Board& board) : treeManager(board) {
    targetColor = board.isBlackTurn() ? COLOR_BLACK : COLOR_WHITE;
}

bool VCFSearchV2::isWin() {
    Result result = treeManager.getBoard().getResult();
    if (result == BLACK_WIN && targetColor == COLOR_BLACK)
        return true;
    if (result == WHITE_WIN && targetColor == COLOR_WHITE)
        return true;
    return false;
}

bool VCFSearchV2::isTargetTurn() {
    if (treeManager.getBoard().isBlackTurn()) {
        if (targetColor == COLOR_BLACK) return true;
        else return false;
    } else {
        if (targetColor == COLOR_BLACK) return false;
        else return true;
    }
}

bool VCFSearchV2::findVCF() {
    if (isWin()) return true;
    
    vector<Pos> moves;
    if (isTargetTurn())
        moves = evaluator.getFours(treeManager.getBoard());
    else 
        moves = evaluator.getCandidates(treeManager.getBoard());

    if (moves.empty()) return false;

    for (auto move : moves) {
        treeManager.next(move);
        if (findVCF()) return true;
        treeManager.prev();
    }

    return false;
}

vector<Pos> VCFSearchV2::getVCFPath() {
    findVCF();
    return treeManager.getPath();
}