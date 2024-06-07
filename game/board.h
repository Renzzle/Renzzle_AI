#pragma once

#include "line.h"
#include "pos.h"
#include <array>

#define BOARD_SIZE 15
#define STATIC_WALL &cells[0][0];

using CellArray = array<array<Cell, BOARD_SIZE + 2>, BOARD_SIZE + 2>;

class Board {

private:
    CellArray cells;
    unsigned int moveCnt;

    Line getLine(Pos& p);
    Pattern getPattern(Line& line, bool isBlack);
    void setPatterns(Pos& p);
    void clearPattern(Cell& cell);

public:
    Board();
    bool isBlackTurn();
    CellArray& getBoardStatus();
    Cell& getCell(const Pos p);
    bool move(Pos p);
    void undo();
    
};

Board::Board() {
    moveCnt = 0;

    for (int i = 0; i < BOARD_SIZE + 2; i++) {
        for (int j = 0; j < BOARD_SIZE + 2; j++) {
            if (i == 0 || i == BOARD_SIZE + 1 || j == 0 || j == BOARD_SIZE + 1)
                cells[i][j].setPiece(WALL);
            else
                cells[i][j].setPiece(EMPTY);
        }
    }
}

bool Board::isBlackTurn() {
    return moveCnt % 2 == 1;
}

CellArray& Board::getBoardStatus() {
    return cells;
}

Cell& Board::getCell(const Pos p) {
    return cells[p.x][p.y];
}

void Board::clearPattern(Cell& cell) {
    for(Direction dir = DIRECTION_START; dir < DIRECTION_SIZE; dir++) {
        cell.setPattern(BLACK, dir, PATTERN_SIZE);
        cell.setPattern(WHITE, dir, PATTERN_SIZE);
    }
}

bool Board::move(Pos p) {
    if (getCell(p).getPiece() != EMPTY)
        return false;

    moveCnt++;

    getCell(p).setPiece((moveCnt % 2 == 1) ? BLACK : WHITE);
    clearPattern(getCell(p));
    setPatterns(p);

    return true;
}

void Board::setPatterns(Pos& p) {
    for(Direction dir = DIRECTION_START; dir < DIRECTION_SIZE; dir++) {
        p.dir = dir;
        for (int i = 0; i < LINE_LENGTH; i++) {
            if (!(p + (i - (LINE_LENGTH / 2)))) {
                continue;
            }

            if(getCell(p).getPiece() == EMPTY) {
                Line line = getLine(p);
                getCell(p).setPiece(BLACK);
                getCell(p).setPattern(BLACK, dir, getPattern(line, true));
                getCell(p).setPiece(WHITE);
                getCell(p).setPattern(WHITE, dir, getPattern(line, false));
                getCell(p).setPiece(EMPTY);
            }
            p - (i - (LINE_LENGTH / 2));
        }
    }
}

Line Board::getLine(Pos& p) {    
    Line line;

    for (int i = 0; i < LINE_LENGTH; i++) {
        if (!(p + (i - (LINE_LENGTH / 2)))) {
            line[i] = STATIC_WALL;
            continue; 
        }
        line[i] = &getCell(p);
        p - (i - (LINE_LENGTH / 2));
    }

    return line;
}

Pattern Board::getPattern(Line& line, bool isBlack) {
    constexpr auto mid = LINE_LENGTH / 2;
    Piece self = isBlack ? BLACK : WHITE;

    int realLen, fullLen, start, end;
    tie(realLen, fullLen, start, end) = line.countLine();

    if(isBlack && realLen >= 6) 
        return OVERLINE;
    else if(realLen >= 5)
        return FIVE;
    else if(fullLen < 5)
        return DEAD;

    int patternCnt[PATTERN_SIZE] = {0};
    int fiveIdx[2] = {0};
    Pattern p = DEAD;

    for(int i = start; i <= end; i++) {
        Piece piece = line[i]->getPiece();
        if(piece == EMPTY) {
            Line sl = line.shiftLine(line, i);
            sl[mid]->setPiece(self);

            Pattern slp = getPattern(sl, isBlack);
            sl[mid]->setPiece(EMPTY);
        
            if(slp == FIVE && patternCnt[FIVE] < 2) {
                fiveIdx[patternCnt[FIVE]] = i;
            }
            patternCnt[slp]++;
        }
    }

    if (patternCnt[FIVE] >= 2) {
        p = FREE_4;
        if(isBlack && fiveIdx[1] - fiveIdx[0] < 5) {
            p = OVERLINE;
        }
    }
    else if (patternCnt[FIVE])
        p = BLOCKED_4;
    else if (patternCnt[FREE_4] >= 2)
        p = FREE_3A;
    else if (patternCnt[FREE_4])
        p = FREE_3;
    else if (patternCnt[BLOCKED_4])
        p = BLOCKED_3;
    else if (patternCnt[FREE_3A] + patternCnt[FREE_3] >= 4)
        p = FREE_2B;
    else if (patternCnt[FREE_3A] + patternCnt[FREE_3] >= 3)
        p = FREE_2A;
    else if (patternCnt[FREE_3A] + patternCnt[FREE_3])
        p = FREE_2;
    else if (patternCnt[BLOCKED_3])
        p = BLOCKED_2;
    else if (patternCnt[FREE_2] + patternCnt[FREE_2A] + patternCnt[FREE_2B])
        p = FREE_1;
    else if (patternCnt[BLOCKED_2])
        p = BLOCKED_1;

    return p;
}

void Board::undo() {

}