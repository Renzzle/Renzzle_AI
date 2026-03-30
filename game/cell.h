#pragma once

#include "types.h"
#include <array>
#include <cassert>

using Score = int;

// score table                            D  OL  B1  F1  B2  F2  F2  F2  B3  F3  F3  B4  F4   F5     UNUSED
const Score attackScore[PATTERN_SIZE] = { 0, 00, 00, 01, 01, 04, 05, 06, 07, 20, 20, 20, 400, 10000, 50000};
const Score defendScore[PATTERN_SIZE] = { 0, 00, 00, 00, 00, 02, 02, 02, 02, 07, 07, 07,  90, 02000, 50000};

using PatternKey = uint16_t;
constexpr int PATTERN_KEY_BITS = 4;
constexpr int PATTERN_KEY_SIZE = 1 << (DIRECTION_SIZE * PATTERN_KEY_BITS);

inline Pattern decodePatternKey(PatternKey key, int dir) {
    return static_cast<Pattern>((key >> (dir * PATTERN_KEY_BITS)) & 0xF);
}

inline Score computeAttackScore(PatternKey key) {
    Score score = 0;
    for (int dir = 0; dir < DIRECTION_SIZE; ++dir) {
        score += attackScore[decodePatternKey(key, dir)];
    }
    return score;
}

inline Score computeDefendScore(PatternKey key) {
    Score score = 0;
    for (int dir = 0; dir < DIRECTION_SIZE; ++dir) {
        score += defendScore[decodePatternKey(key, dir)];
    }
    return score;
}

inline CompositePattern computeBlackComposite(PatternKey key) {
    int pc[PATTERN_SIZE] = {0};
    for (int dir = 0; dir < DIRECTION_SIZE; ++dir) {
        pc[decodePatternKey(key, dir)]++;
    }

    if (pc[FIVE] > 0) return WINNING;
    if (pc[OVERLINE] > 0 || pc[FREE_4] + pc[BLOCKED_4] >= 2) return FORBID;
    if (pc[FREE_3] + pc[FREE_3A] >= 2) return FORBID_33;
    if (pc[FREE_4] > 0 || pc[BLOCKED_4] >= 2) return MATE;
    if (pc[BLOCKED_4] > 0 && pc[FREE_3] + pc[FREE_3A] > 0) return B4_F3;
    if (pc[BLOCKED_4] > 0 && pc[FREE_2] + pc[FREE_2A] + pc[FREE_2B] + pc[BLOCKED_3] > 0) return B4_PLUS;
    if (pc[BLOCKED_4] > 0) return B4_ANY;
    if (pc[FREE_3] + pc[FREE_3A] > 0 && pc[FREE_2] + pc[FREE_2A] + pc[FREE_2B] + pc[BLOCKED_3] > 0) return F3_PLUS;
    if (pc[FREE_3] + pc[FREE_3A] > 0) return F3_ANY;
    if (pc[BLOCKED_3] > 0 && pc[FREE_2] + pc[FREE_2A] + pc[FREE_2B] + pc[BLOCKED_3] >= 2) return B3_PLUS;
    if (pc[BLOCKED_3] > 0) return B3_ANY;
    if (pc[FREE_2] + pc[FREE_2A] + pc[FREE_2B] >= 2) return F2_2X;
    if (pc[FREE_2] + pc[FREE_2A] + pc[FREE_2B] > 0) return F2_ANY;
    return ETC;
}

inline CompositePattern computeWhiteComposite(PatternKey key) {
    int pc[PATTERN_SIZE] = {0};
    for (int dir = 0; dir < DIRECTION_SIZE; ++dir) {
        pc[decodePatternKey(key, dir)]++;
    }

    if (pc[FIVE] > 0) return WINNING;
    if (pc[FREE_4] > 0 || pc[BLOCKED_4] >= 2) return MATE;
    if (pc[BLOCKED_4] > 0 && pc[FREE_3] + pc[FREE_3A] > 0) return B4_F3;
    if (pc[BLOCKED_4] > 0 && pc[FREE_2] + pc[FREE_2A] + pc[FREE_2B] + pc[BLOCKED_3] > 0) return B4_PLUS;
    if (pc[BLOCKED_4] > 0) return B4_ANY;
    if (pc[FREE_3] + pc[FREE_3A] >= 2) return F3_2X;
    if (pc[FREE_3] + pc[FREE_3A] > 0 && pc[FREE_2] + pc[FREE_2A] + pc[FREE_2B] + pc[BLOCKED_3] > 0) return F3_PLUS;
    if (pc[FREE_3] + pc[FREE_3A] > 0) return F3_ANY;
    if (pc[BLOCKED_3] > 0 && pc[FREE_2] + pc[FREE_2A] + pc[FREE_2B] + pc[BLOCKED_3] >= 2) return B3_PLUS;
    if (pc[BLOCKED_3] > 0) return B3_ANY;
    if (pc[FREE_2] + pc[FREE_2A] + pc[FREE_2B] >= 2) return F2_2X;
    if (pc[FREE_2] + pc[FREE_2A] + pc[FREE_2B] > 0) return F2_ANY;
    return ETC;
}

inline const std::array<Score, PATTERN_KEY_SIZE>& attackScoreLUT() {
    static const std::array<Score, PATTERN_KEY_SIZE> lut = []() {
        std::array<Score, PATTERN_KEY_SIZE> table = {};
        for (int key = 0; key < PATTERN_KEY_SIZE; ++key) {
            table[key] = computeAttackScore(static_cast<PatternKey>(key));
        }
        return table;
    }();
    return lut;
}

inline const std::array<Score, PATTERN_KEY_SIZE>& defendScoreLUT() {
    static const std::array<Score, PATTERN_KEY_SIZE> lut = []() {
        std::array<Score, PATTERN_KEY_SIZE> table = {};
        for (int key = 0; key < PATTERN_KEY_SIZE; ++key) {
            table[key] = computeDefendScore(static_cast<PatternKey>(key));
        }
        return table;
    }();
    return lut;
}

inline const std::array<CompositePattern, PATTERN_KEY_SIZE>& blackCompositeLUT() {
    static const std::array<CompositePattern, PATTERN_KEY_SIZE> lut = []() {
        std::array<CompositePattern, PATTERN_KEY_SIZE> table = {};
        for (int key = 0; key < PATTERN_KEY_SIZE; ++key) {
            table[key] = computeBlackComposite(static_cast<PatternKey>(key));
        }
        return table;
    }();
    return lut;
}

inline const std::array<CompositePattern, PATTERN_KEY_SIZE>& whiteCompositeLUT() {
    static const std::array<CompositePattern, PATTERN_KEY_SIZE> lut = []() {
        std::array<CompositePattern, PATTERN_KEY_SIZE> table = {};
        for (int key = 0; key < PATTERN_KEY_SIZE; ++key) {
            table[key] = computeWhiteComposite(static_cast<PatternKey>(key));
        }
        return table;
    }();
    return lut;
}

class Cell {

private:
    Score score[2];
    Pattern patterns[2][DIRECTION_SIZE];
    CompositePattern cPattern[2];
    Piece piece;
    
    PatternKey getPatternKey(Piece piece) const;

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
    void updateDerived();
    
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

PatternKey Cell::getPatternKey(Piece piece) const {
    PatternKey key = 0;
    for (int dir = 0; dir < DIRECTION_SIZE; ++dir) {
        key |= static_cast<PatternKey>(patterns[piece][dir]) << (dir * PATTERN_KEY_BITS);
    }
    return key;
}

void Cell::setCompositePattern() {
    const PatternKey blackKey = getPatternKey(BLACK);
    const PatternKey whiteKey = getPatternKey(WHITE);
    cPattern[BLACK] = blackCompositeLUT()[blackKey];
    cPattern[WHITE] = whiteCompositeLUT()[whiteKey];
}

void Cell::setScore() {
    const PatternKey blackKey = getPatternKey(BLACK);
    const PatternKey whiteKey = getPatternKey(WHITE);
    score[BLACK] = attackScoreLUT()[blackKey] + defendScoreLUT()[whiteKey];
    score[WHITE] = attackScoreLUT()[whiteKey] + defendScoreLUT()[blackKey];
}

void Cell::updateDerived() {
    const PatternKey blackKey = getPatternKey(BLACK);
    const PatternKey whiteKey = getPatternKey(WHITE);
    score[BLACK] = attackScoreLUT()[blackKey] + defendScoreLUT()[whiteKey];
    score[WHITE] = attackScoreLUT()[whiteKey] + defendScoreLUT()[blackKey];
    cPattern[BLACK] = blackCompositeLUT()[blackKey];
    cPattern[WHITE] = whiteCompositeLUT()[whiteKey];
}
