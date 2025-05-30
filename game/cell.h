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
    CompositePattern cPattern[2];
    Score score[2];

public:
    Cell();
    Piece getPiece() const;
    void setPiece(const Piece& piece);
    Pattern getPattern(Piece piece, Direction dir);
    CompositePattern getCompositePattern(Piece piece);
    Score getScore(Piece piece);
    void setPattern(Piece piece, Direction dir, Pattern pattern);
    void clearCompositePattern();
    void setCompositePattern();
    void setScore();
    
};

Cell::Cell() {
    this->piece = EMPTY;
    for(Direction dir = DIRECTION_START; dir < DIRECTION_SIZE; dir++) {
        setPattern(BLACK, dir, NONE);
        setPattern(WHITE, dir, NONE);
    }
    score[BLACK] = 0;
    score[WHITE] = 0;
    cPattern[BLACK] = ETC;
    cPattern[WHITE] = ETC;
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

CompositePattern Cell::getCompositePattern(Piece piece) {
    return cPattern[piece];
}

Score Cell::getScore(Piece piece) {
    return score[piece];
}

void Cell::setPattern(Piece piece, Direction dir, Pattern pattern) {
    this->patterns[piece][dir] = pattern;
}

void Cell::clearCompositePattern() {
    cPattern[BLACK] = NOT_EMPTY;
    cPattern[WHITE] = NOT_EMPTY;
}

void Cell::setCompositePattern() {
    int bpc[PATTERN_SIZE] = {0};
    int wpc[PATTERN_SIZE] = {0};

    for (Direction dir = DIRECTION_START; dir < DIRECTION_SIZE; dir++) {
        Pattern p = patterns[BLACK][dir];
        bpc[p]++;
        p = patterns[WHITE][dir];
        wpc[p]++;
    }

    if (bpc[FIVE] > 0) {
        cPattern[BLACK] = WINNING;
    } else if (bpc[OVERLINE] > 0 || bpc[FREE_4] + bpc[BLOCKED_4] >= 2) {
        cPattern[BLACK] = FORBID;
    } else if (bpc[FREE_3] + bpc[FREE_3A] >= 2) {
        cPattern[BLACK] = FORBID_33;
    } else if (bpc[FREE_4] > 0 || bpc[BLOCKED_4] >= 2) {
        cPattern[BLACK] = MATE;
    } else if (bpc[BLOCKED_4] > 0 && bpc[FREE_3] + bpc[FREE_3A] > 0) {
        cPattern[BLACK] = B4_F3;
    } else if (bpc[BLOCKED_4] > 0 && bpc[FREE_2] + bpc[FREE_2A] + bpc[FREE_2B] + bpc[BLOCKED_3] > 0) {
        cPattern[BLACK] = B4_PLUS;
    } else if (bpc[BLOCKED_4] > 0) {
        cPattern[BLACK] = B4_ANY;
    } else if (bpc[FREE_3] + bpc[FREE_3A] > 0 && bpc[FREE_2] + bpc[FREE_2A] + bpc[FREE_2B] + bpc[BLOCKED_3] > 0) {
        cPattern[BLACK] = F3_PLUS;
    } else if (bpc[FREE_3] + bpc[FREE_3A] > 0) {
        cPattern[BLACK] = F3_ANY;
    } else if (bpc[BLOCKED_3] > 0 && bpc[FREE_2] + bpc[FREE_2A] + bpc[FREE_2B] + bpc[BLOCKED_3] >= 2) {
        cPattern[BLACK] = B3_PLUS;
    } else if (bpc[BLOCKED_3] > 0) {
        cPattern[BLACK] = B3_ANY;
    } else if (bpc[FREE_2] + bpc[FREE_2A] + bpc[FREE_2B] >= 2) {
        cPattern[BLACK] = F2_2X;
    } else if (bpc[FREE_2] + bpc[FREE_2A] + bpc[FREE_2B] > 0) {
        cPattern[BLACK] = F2_ANY;
    } else {
        cPattern[BLACK] = ETC;
    }

    if (wpc[FIVE] > 0) {
        cPattern[WHITE] = WINNING;
    } else if (wpc[FREE_4] > 0 || wpc[BLOCKED_4] >= 2) {
        cPattern[WHITE] = MATE;
    } else if (wpc[BLOCKED_4] > 0 && wpc[FREE_3] + wpc[FREE_3A] > 0) {
        cPattern[WHITE] = B4_F3;
    } else if (wpc[BLOCKED_4] > 0 && wpc[FREE_2] + wpc[FREE_2A] + wpc[FREE_2B] + wpc[BLOCKED_3] > 0) {
        cPattern[WHITE] = B4_PLUS;
    } else if (wpc[BLOCKED_4] > 0) {
        cPattern[WHITE] = B4_ANY;
    } else if (wpc[FREE_3] + wpc[FREE_3A] >= 2) {
        cPattern[WHITE] = F3_2X;
    } else if (wpc[FREE_3] + wpc[FREE_3A] > 0 && wpc[FREE_2] + wpc[FREE_2A] + wpc[FREE_2B] + wpc[BLOCKED_3] > 0) {
        cPattern[WHITE] = F3_PLUS;
    } else if (wpc[FREE_3] + wpc[FREE_3A] > 0) {
        cPattern[WHITE] = F3_ANY;
    } else if (wpc[BLOCKED_3] > 0 && wpc[FREE_2] + wpc[FREE_2A] + wpc[FREE_2B] + wpc[BLOCKED_3] >= 2) {
        cPattern[WHITE] = B3_PLUS;
    } else if (wpc[BLOCKED_3] > 0) {
        cPattern[WHITE] = B3_ANY;
    } else if (wpc[FREE_2] + wpc[FREE_2A] + wpc[FREE_2B] >= 2) {
        cPattern[WHITE] = F2_2X;
    } else if (wpc[FREE_2] + wpc[FREE_2A] + wpc[FREE_2B] > 0) {
        cPattern[WHITE] = F2_ANY;
    } else {
        cPattern[WHITE] = ETC;
    }
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
    score[BLACK] = bs;
    score[WHITE] = ws;
}