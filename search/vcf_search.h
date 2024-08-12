#pragma once

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

class VCFSearch {

private:
    TreeManager treeManager;
    Evaluator evaluator;
    Color targetColor;
    bool isWin();
    bool isTargetTurn();

public:
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
        treeManager.next(move);
        if (findVCF()) return true;
        treeManager.prev();
    }

    return false;
}

vector<Pos> VCFSearch::getVCFPath() {
    return treeManager.getPath();
}