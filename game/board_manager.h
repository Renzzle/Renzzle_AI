#pragma once
#include "board.h"
#include <stack>

class BoardManager {
private:
    Board board;
    std::stack<Board> history;

public:
    BoardManager(Board initialBoard);

    void move(Pos p);
    void undo();

    Board& getBoard(); // Removed const
    bool isBlackTurn();
    Result getResult();
    bool isForbidden(Pos p);
};

BoardManager::BoardManager(Board initialBoard) : board(initialBoard) {}

void BoardManager::move(Pos p) {
    history.push(board);
    board.move(p);
}

void BoardManager::undo() {
    if (!history.empty()) {
        board = history.top();
        history.pop();
    }
}

Board& BoardManager::getBoard() { // Removed const
    return board;
}

bool BoardManager::isBlackTurn() {
    return board.isBlackTurn();
}

Result BoardManager::getResult() {
    return board.getResult();
}

bool BoardManager::isForbidden(Pos p) {
    return board.isForbidden(p);
}
