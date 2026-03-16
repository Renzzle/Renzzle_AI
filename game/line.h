#pragma once

#include "cell.h"
#include <array>
#include <tuple>
using namespace std;

#define LINE_LENGTH 11

class Line {

private:
    array<Piece, LINE_LENGTH> pieces;

public:
    Line() {
        pieces.fill(WALL);
    }
    Piece& operator[](size_t idx) {
        return pieces[idx];
    }
    const Piece& operator[](size_t idx) const {
        return pieces[idx];
    }
    tuple<int, int, int, int> countLine() const;
    Line shiftLine(int n) const;

};

tuple<int, int, int, int> Line::countLine() const {
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

    Piece self = pieces[mid];
    Piece oppo = self == BLACK ? WHITE : BLACK;
    Piece piece;

    for (int i = mid - 1; i >=0; i--) {
        piece = pieces[i];
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
        piece = pieces[i];
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

Line Line::shiftLine(int n) const {
    constexpr auto len = LINE_LENGTH;

    Line shiftedLine;
    for (int i = 0; i < len; i++) {
        int idx = i + n - len / 2;
        shiftedLine[i] = idx >= 0 && idx < len ? pieces[idx] : WALL;
    }
    return shiftedLine;
}
