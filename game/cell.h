#pragma once

#include "types.h"
#include <cassert>
using namespace std;

class Cell {

private:
    Piece piece;
    Pattern patterns[2][DIRECTION_SIZE];

public:
    Cell();
    Piece getPiece() const;
    void setPiece(const Piece& piece);
    Pattern getPattern(Piece piece, Direction dir);
    void setPattern(Piece piece, Direction dir, Pattern pattern);
    
};

Cell::Cell() {
    this->piece = EMPTY;
    for(Direction dir = DIRECTION_START; dir < DIRECTION_SIZE; dir++) {
        setPattern(BLACK, dir, NONE);
        setPattern(WHITE, dir, NONE);
    }
}

Piece Cell::getPiece() const {
    return piece;
}

void Cell::setPiece(const Piece& piece) {
    this->piece = piece;
}

Pattern Cell::getPattern(Piece piece, Direction dir) {
    return patterns[piece][dir];
}

void Cell::setPattern(Piece piece, Direction dir, Pattern pattern) {
    this->patterns[piece][dir] = pattern;
}