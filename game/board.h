#pragma once

#include "../test/util.h"
#include "line.h"
#include "pos.h"
#include <array>
#include <stack>
#include <iostream>

#define BOARD_SIZE 15
#define STATIC_WALL &cells[0][0];

using CellArray = array<array<Cell, BOARD_SIZE + 2>, BOARD_SIZE + 2>;
using MoveList = stack<Pos>;

class Board {

private:
    CellArray cells;
    MoveList moves;
    unsigned int moveCnt;
    Result result;

    Line getLine(Pos& p);
    Pattern getPattern(Line& line, Color color);
    void setPatterns(Pos& p);
    void clearPattern(Cell& cell);
    void setResult(Pos& p);

public:
    Board();
    bool isBlackTurn();
    CellArray& getBoardStatus();
    Cell& getCell(const Pos p);
    bool move(Pos p);
    void undo();
    Result getResult();
    bool isForbidden(Pos p);
    
};

Board::Board() {
    moveCnt = 0;
    result = ONGOING;

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
    return moveCnt % 2 == 0;
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
    if (getCell(p).getPiece() != EMPTY) return false;
    if (result != ONGOING) return false;
    if (moveCnt == BOARD_SIZE * BOARD_SIZE) return false;

    moveCnt++;
    moves.push(p);
    
    setResult(p);
    getCell(p).setPiece(isBlackTurn() ? WHITE : BLACK);
    clearPattern(getCell(p));
    setPatterns(p);

    return true;
}

void Board::setPatterns(Pos& p) {
    for (Direction dir = DIRECTION_START; dir < DIRECTION_SIZE; dir++) {
        p.dir = dir;
        for (int i = 0; i < LINE_LENGTH; i++) {
            if (!(p + (i - (LINE_LENGTH / 2)))) {
                continue;
            }

            if (getCell(p).getPiece() == EMPTY) {
                Line line = getLine(p);
                getCell(p).setPiece(BLACK);
                getCell(p).setPattern(BLACK, dir, getPattern(line, COLOR_BLACK));
                getCell(p).setPiece(WHITE);
                getCell(p).setPattern(WHITE, dir, getPattern(line, COLOR_WHITE));
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

Pattern Board::getPattern(Line& line, Color color) {
    constexpr auto mid = LINE_LENGTH / 2;
    bool isBlack = color == COLOR_BLACK;
    Piece self = isBlack ? BLACK : WHITE;

    int realLen, fullLen, start, end;
    tie(realLen, fullLen, start, end) = line.countLine();

    if (isBlack && realLen >= 6) 
        return OVERLINE;
    else if (realLen >= 5)
        return FIVE;
    else if (fullLen < 5)
        return DEAD;

    int patternCnt[PATTERN_SIZE] = {0};
    int fiveIdx[2] = {0};
    Pattern p = DEAD;

    for (int i = start; i <= end; i++) {
        Piece piece = line[i]->getPiece();
        if (piece == EMPTY) {
            Line sl = line.shiftLine(line, i);
            sl[mid]->setPiece(self);

            Pattern slp = getPattern(sl, color);
            sl[mid]->setPiece(EMPTY);
        
            if (slp == FIVE && patternCnt[FIVE] < 2) {
                fiveIdx[patternCnt[FIVE]] = i;
            }
            patternCnt[slp]++;
        }
    }

    if (patternCnt[FIVE] >= 2) {
        p = FREE_4;
        if (isBlack && fiveIdx[1] - fiveIdx[0] < 5) {
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
    Pos p = moves.top();

    moveCnt--;
    getCell(p).setPiece(EMPTY);
    setPatterns(p);
    result = ONGOING;

    moves.pop();
}

bool Board::isForbidden(Pos p) {
    Cell c = getCell(p);
    if (c.getPiece() != EMPTY) return false;

    int winByFour = 0;
    int winByThree = 0;
    int pThreeCnt = 0;

    // five, overline, 4-4
    for (Direction dir = DIRECTION_START; dir < DIRECTION_SIZE; dir++) {
        p.dir = dir;
        Pattern pattern = c.getPattern(BLACK, p.dir);
        if (pattern == FIVE)
            return false;
        else if (pattern == OVERLINE)
            return true;
        else if (pattern == BLOCKED_4 || pattern == FREE_4) {
            if (++winByFour >= 2)
                return true;
        }

        if (pattern == FREE_3 || pattern == FREE_3A)
            pThreeCnt++;
    }
    
    // if there is not open three more than two
    if (pThreeCnt < 2)
        return false;

    // recursive 3-3
    // move
    getCell(p).setPiece(BLACK);
    setPatterns(p);

    for (Direction dir = DIRECTION_START; dir < DIRECTION_SIZE; dir++) {
        p.dir = dir;
        Pattern pattern = c.getPattern(BLACK, p.dir);

        // double three forbidden type
        if (pattern != FREE_3 && pattern != FREE_3A)
            continue;
        
        Pos posi = p;
        for (int i = 0; i < LINE_LENGTH; i++) {
            if (!(posi + (i - (LINE_LENGTH / 2))))
                continue;

            Cell &c = getCell(posi);
            if (c.getPiece() == EMPTY) {
                bool isFive = false;
                if (c.getPattern(BLACK, dir) == FREE_4 && !isForbidden(posi)) {
                    for (Direction eDir = DIRECTION_START; eDir < DIRECTION_SIZE; eDir++) {
                        Pattern pattern = c.getPattern(BLACK, eDir);
                        if (pattern == FIVE)
                            isFive = true;
                    }
                    // made 5 with an empty space -> not a forbidden position
                    if (!isFive) {
                        winByThree++;
                        break;
                    }
                }
            }
            posi - (i - (LINE_LENGTH / 2));
        }

        if (winByThree >= 2) {
            break;
        }
    }

    // undo
    getCell(p).setPiece(EMPTY);
    setPatterns(p);

    return winByThree >= 2;
}

void Board::setResult(Pos& p) {
    bool isBlackTurn = this->isBlackTurn();

    if (!isBlackTurn && isForbidden(p)) {
        result = WHITE_WIN;
        return;
    }

    Piece self = isBlackTurn ? WHITE : BLACK;
    for (Direction dir = DIRECTION_START; dir < DIRECTION_SIZE; dir++) {
        if(getCell(p).getPattern(self, dir) == FIVE) {
            result = isBlackTurn ? WHITE_WIN : BLACK_WIN;
            return;
        }
    }

    result = ONGOING;
}

Result Board::getResult() {
    return result;
}