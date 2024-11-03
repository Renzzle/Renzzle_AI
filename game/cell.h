#pragma once

#include "types.h"
#include <cassert>

using Score = int;

// score table                            D  OL  B1  F1  B2  F2  F2  F2  B3  F3  F3  B4   F4    F5   
const Score attackScore[PATTERN_SIZE] = { 0, 00, 00, 01, 01, 04, 05, 06, 07, 40, 40, 800, 4000, 100000};
const Score defendScore[PATTERN_SIZE] = { 0, 00, 00, 00, 00, 02, 02, 02, 02, 10, 10, 10,   160, 20000};

class Cell {

private:
    Piece piece;
    Pattern patterns[2][DIRECTION_SIZE];

    Score blackScore;
    Score whiteScore;

public:
    Cell();
    Piece getPiece() const;
    void setPiece(const Piece& piece);
    Pattern getPattern(Piece piece, Direction dir);
    void setPattern(Piece piece, Direction dir, Pattern pattern);
    void setScore();
    
};

Cell::Cell() {
    this->piece = EMPTY;
    for(Direction dir = DIRECTION_START; dir < DIRECTION_SIZE; dir++) {
        setPattern(BLACK, dir, NONE);
        setPattern(WHITE, dir, NONE);
    }
    blackScore = 0;
    whiteScore = 0;
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

void Cell::setScore() {
    Score bs = 0;
    Score ws = 0;
    for (int i = 0; i < DIRECTION_SIZE; i++) {
        bs += attackScore[patterns[BLACK][i]];
        bs += defendScore[patterns[WHITE][i]];
        ws += attackScore[patterns[WHITE][i]];
        ws += defendScore[patterns[BLACK][i]];
    }
    blackScore = bs;
    whiteScore = ws;
}