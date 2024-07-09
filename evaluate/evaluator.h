#pragma once
#include "../game/board.h"
#include <list>
#include <tuple>

using Value = int;
using Depth = int;

class Evaluator {

private:
    Board board;
    Color vcfColor;

public:
    void setBoard(Board board);
    void setVCFColor();
    list<Pos> getCandidates();
    Value evaluate();
    void next(Pos p);
    void prev();
    bool isGameOver();
};

void Evaluator::setBoard(Board board) {
    this->board = board;
}

void Evaluator::setVCFColor() {
    this->vcfColor = board.isBlackTurn() ? COLOR_BLACK : COLOR_WHITE;
}

void Evaluator::next(Pos p) {
    board.move(p);
}

void Evaluator::prev() {
    board.undo();
}

list<Pos> Evaluator::getCandidates() {
    list<Pos> moves;
    list<tuple<Pos, int>> tmp;

    Piece self = board.isBlackTurn() ? BLACK : WHITE;
    Piece oppo = !board.isBlackTurn() ? BLACK : WHITE;
    bool isVCFColorTurn = (board.isBlackTurn() ? COLOR_BLACK : COLOR_WHITE) == vcfColor;

    for (int i = 1; i <= BOARD_SIZE; i++) {
        for (int j = 1; j <= BOARD_SIZE; j++) {
            Cell cell = board.getCell(Pos(i, j));
            if (cell.getPiece() != EMPTY) continue;
            int val = 0;
            for (Direction dir = DIRECTION_START; dir < DIRECTION_SIZE; dir++) {
                Pattern p = cell.getPattern(self, dir);
                if (p != PATTERN_SIZE && isVCFColorTurn) {
                    if (p == FIVE) val += 1000000;
                    else if (p == FREE_4) val += 40000;
                    else if (p == BLOCKED_4) val += 5000;
                    else if (p == FREE_3A) val += 100;
                    else if (p == FREE_3) val += 100;
                    else val += (int)p * (int)p;
                }
                p = cell.getPattern(oppo, dir);
                if (p != PATTERN_SIZE) {
                    if (p == FIVE) val += 100000;
                }
            }
            if (val >= 5000)
                tmp.push_back(make_tuple(Pos(i, j), val));
        }
    }

    tmp.sort([](const tuple<Pos, int>& a, const tuple<Pos, int>& b) {
        return get<1>(a) > get<1>(b);
    });
    for (const auto& item : tmp) {
        moves.push_back(get<0>(item));
        if (get<1>(item) >= 40000) break;
    }

    return moves;
}

Value Evaluator::evaluate() {
    Result result = board.getResult();
    if (result == ONGOING) return 0;
    else if (result == BLACK_WIN) {
        if (vcfColor == COLOR_BLACK) return 20000;
        else return -20000;
    }
    else if (result == WHITE_WIN) {
        if (vcfColor == COLOR_BLACK) return -20000;
        else return 20000;
    }
}

bool Evaluator::isGameOver() {
    return board.getResult() != ONGOING;
}