#pragma once
#include "../game/board.h"
#include <list>
#include <tuple>
#include <random>
#include <cstdint>
#include <array>

using Value = int;
using Depth = int;
using HashKey = uint64_t;

class EvaluatorV1 {
private:
    Board board;
    Color vcfColor;
    HashKey zobristHash;
    array<array<HashKey, 3>, BOARD_SIZE * BOARD_SIZE> zobristTable;

    void initZobristTable();

public:
    EvaluatorV1() { initZobristTable(); }
    void setBoard(Board board);
    void setVCFColor();
    list<Pos> getCandidates();
    Value evaluate();
    Value evaluateVCF();
    void next(Pos p);
    void prev();
    bool isGameOver();
    HashKey getZobristKey() const;
};

void EvaluatorV1::initZobristTable() {
    mt19937_64 rng(0xdeadbeef);
    for (int i = 0; i < BOARD_SIZE * BOARD_SIZE; ++i) {
        for (int j = 0; j < 3; ++j) {
            zobristTable[i][j] = rng();
        }
    }
}

void EvaluatorV1::setBoard(Board board) {
    this->board = board;
    this->zobristHash = 0;
    for (int i = 1; i <= BOARD_SIZE; ++i) {
        for (int j = 1; j <= BOARD_SIZE; ++j) {
            Pos pos(i, j);
            Piece piece = board.getCell(pos).getPiece();
            if (piece != EMPTY) {
                zobristHash ^= zobristTable[(i - 1) * BOARD_SIZE + (j - 1)][piece];
            }
        }
    }
}

void EvaluatorV1::setVCFColor() {
    this->vcfColor = board.isBlackTurn() ? COLOR_BLACK : COLOR_WHITE;
}

void EvaluatorV1::next(Pos p) {
    Piece oldPiece = board.getCell(p).getPiece();
    board.move(p);
    Piece newPiece = board.getCell(p).getPiece();
    int index = (p.getX() - 1) * BOARD_SIZE + (p.getY() - 1);
    if (oldPiece != EMPTY) {
        zobristHash ^= zobristTable[index][oldPiece];
    }
    if (newPiece != EMPTY) {
        zobristHash ^= zobristTable[index][newPiece];
    }
}

void EvaluatorV1::prev() {
    Pos lastMove = board.getLastMove();
    Piece newPiece = EMPTY;
    Piece oldPiece = board.getCell(lastMove).getPiece();
    int index = (lastMove.getX() - 1) * BOARD_SIZE + (lastMove.getY() - 1);
    if (oldPiece != EMPTY) {
        zobristHash ^= zobristTable[index][oldPiece];
    }
    if (newPiece != EMPTY) {
        zobristHash ^= zobristTable[index][newPiece];
    }
    board.undo();
}

HashKey EvaluatorV1::getZobristKey() const {
    return zobristHash;
}

list<Pos> EvaluatorV1::getCandidates() {
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

Value EvaluatorV1::evaluate() {
    Result result = board.getResult();
    if (result == BLACK_WIN) {
        if (vcfColor == COLOR_BLACK) return 20000;
        else return -20000;
    }
    else if (result == WHITE_WIN) {
        if (vcfColor == COLOR_BLACK) return -20000;
        else return 20000;
    }
    return 0;
}

Value EvaluatorV1::evaluateVCF() {
    Result result = board.getResult();
    if (result == BLACK_WIN) {
        if (vcfColor == COLOR_BLACK) return 20000;
        else return -20000;
    }
    else if (result == WHITE_WIN) {
        if (vcfColor == COLOR_BLACK) return -20000;
        else return 20000;
    }
    return 0;
}

bool EvaluatorV1::isGameOver() {
    return board.getResult() != ONGOING;
}