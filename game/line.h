#pragma once

#include "cell.h"
#include <array>
#include <tuple>

#define LINE_LENGTH 11

class Line {

private:
    array<Cell*, LINE_LENGTH> cells;
    Cell deadCell;

public:
    Line() {
        deadCell.setPiece(WALL);
    }
    Cell*& operator[](size_t idx) {
        return this->cells[idx];
    }
    tuple<int, int, int, int> countLine();
    Line shiftLine(Line& line, int n);

};

tuple<int, int, int, int> Line::countLine() {
    constexpr auto mid = LINE_LENGTH / 2;
    
    /*
    realLen: length of the continuous stone including the starting stone
    fullLen: between the other side
    start: start index of fullLen
    end: end index of fullLen
    */
    int realLen = 1, fullLen = 1; 
    int realLenInc = 1;
    int start = mid, end = mid;

    int self = cells[mid]->getPiece();
    int oppo = !self;
    Piece piece;

    for (int i = mid - 1; i >=0; i--) {
        piece = cells[i]->getPiece();
        if (piece == self)
            realLen += realLenInc;
        else if (piece == oppo || piece == WALL)
            break;
        else
            realLenInc = 0;

        fullLen++;
        start = i;        
    }

    realLenInc = 1;

    for (int i = mid + 1; i < LINE_LENGTH; i++) {
        piece = cells[i]->getPiece();
        if (piece == self)
            realLen += realLenInc;
        else if (piece == oppo || piece == WALL)
            break;
        else
            realLenInc = 0;

        fullLen++;
        end = i;    
    }

    return make_tuple(realLen, fullLen, start, end);
}

Line Line::shiftLine(Line& line, int n) {
    constexpr auto len = LINE_LENGTH;

    Line shiftedLine;
    for (int i = 0; i < len; i++) {
        int idx = i + n - len / 2;
        shiftedLine[i] = idx >= 0 && idx < len ? line[idx] : &deadCell;
    }
    return shiftedLine;
}