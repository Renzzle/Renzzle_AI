#pragma once

#include "types.h"
#include <array>
#include <cassert>

using Score = int;

// score table                                N  D   OL  B1  F1  B2  F2  F2  F2  B3  F3  F3  B4   F4     F5     UNUSED
constexpr Score attackScore[PATTERN_SIZE + 1] = { 0, 00, 00, 01, 01, 04, 05, 06, 07, 21, 21, 20, 400, 10000, 50000, 0 };
constexpr Score defendScore[PATTERN_SIZE + 1] = { 0, 00, 00, 00, 00, 02, 02, 02, 02, 07, 07, 07,  90, 02000, 50000, 0 };

using PatternKey = uint16_t;
constexpr int PATTERN_KEY_BITS = 4;
constexpr int PATTERN_KEY_SIZE = 1 << (DIRECTION_SIZE * PATTERN_KEY_BITS);
static_assert(PATTERN_SIZE < (1 << PATTERN_KEY_BITS),
    "Every Pattern value must fit in one packed nibble.");

constexpr Pattern decodePatternKey(PatternKey key, int dir) {
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

constexpr CompositePattern computeBlackComposite(PatternKey key) {
    int pc[PATTERN_SIZE + 1] = {0};
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

constexpr CompositePattern computeWhiteComposite(PatternKey key) {
    int pc[PATTERN_SIZE + 1] = {0};
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

// updateDerived tables small enough to stay in L1: per key byte (two directions)
// partial scores and a composite class, then composite by the four classes.
constexpr int COMPOSITE_CLASS_BITS = 3;
constexpr int COMPOSITE_CLASS_KEY_SIZE = 1 << (DIRECTION_SIZE * COMPOSITE_CLASS_BITS);

// Patterns the composite rules tell apart; everything else is class 0.
constexpr int getCompositeClass(Pattern pattern) {
    switch (pattern) {
        case BLOCKED_3: return 1;
        case FREE_2: case FREE_2A: case FREE_2B: return 2;
        case FREE_3: case FREE_3A: return 3;
        case BLOCKED_4: return 4;
        case FREE_4: return 5;
        case OVERLINE: return 6;
        case FIVE: return 7;
        default: return 0;
    }
}

struct DerivedTables {
    struct KeyByte {
        Score attack;
        Score defend;
        uint16_t compositeClass;
    };
    KeyByte keyBytes[1 << (2 * PATTERN_KEY_BITS)];
    CompositePattern blackComposite[COMPOSITE_CLASS_KEY_SIZE];
    CompositePattern whiteComposite[COMPOSITE_CLASS_KEY_SIZE];
};

constexpr DerivedTables buildDerivedTables() {
    DerivedTables tables = {};
    for (int byte = 0; byte < (1 << (2 * PATTERN_KEY_BITS)); ++byte) {
        const Pattern low = static_cast<Pattern>(byte & 0xF);
        const Pattern high = static_cast<Pattern>(byte >> 4);
        tables.keyBytes[byte].attack = attackScore[low] + attackScore[high];
        tables.keyBytes[byte].defend = defendScore[low] + defendScore[high];
        tables.keyBytes[byte].compositeClass = static_cast<uint16_t>(
            getCompositeClass(low) | (getCompositeClass(high) << COMPOSITE_CLASS_BITS));
    }
    constexpr Pattern classPatterns[1 << COMPOSITE_CLASS_BITS] =
        { DEAD, BLOCKED_3, FREE_2, FREE_3, BLOCKED_4, FREE_4, OVERLINE, FIVE };
    for (int classKey = 0; classKey < COMPOSITE_CLASS_KEY_SIZE; ++classKey) {
        PatternKey key = 0;
        for (int dir = 0; dir < DIRECTION_SIZE; ++dir) {
            const int cls = (classKey >> (dir * COMPOSITE_CLASS_BITS)) & 0x7;
            key = static_cast<PatternKey>(key | (classPatterns[cls] << (dir * PATTERN_KEY_BITS)));
        }
        tables.blackComposite[classKey] = computeBlackComposite(key);
        tables.whiteComposite[classKey] = computeWhiteComposite(key);
    }
    return tables;
}

inline constexpr DerivedTables DERIVED_TABLES = buildDerivedTables();

class Cell {

private:
    Score score[2];
    PatternKey patternKeys[2];
    CompositePattern cPattern[2];
    Piece piece;

public:
    Cell();
    Piece getPiece() const;
    void setPiece(const Piece& piece);
    Pattern getPattern(Piece piece, Direction dir) const;
    CompositePattern getCompositePattern(Piece piece) const;
    Score getScore(Piece piece) const;
    void setPattern(Piece piece, Direction dir, Pattern pattern);
    bool hasSamePatternsExcept(const Cell& other, Direction dir) const;
    void clearCompositePattern();
    void setCompositePattern();
    void setScore();
    void updateDerived();
    
};
static_assert(sizeof(Cell) == 16, "Cell must stay compact.");

Cell::Cell() {
    this->piece = EMPTY;
    patternKeys[BLACK] = 0;
    patternKeys[WHITE] = 0;
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

Pattern Cell::getPattern(Piece piece, Direction dir) const {
    return decodePatternKey(patternKeys[piece], static_cast<int>(dir));
}

CompositePattern Cell::getCompositePattern(Piece piece) const {
    return cPattern[piece];
}

Score Cell::getScore(Piece piece) const {
    return score[piece];
}

void Cell::setPattern(Piece piece, Direction dir, Pattern pattern) {
    const int shift = static_cast<int>(dir) * PATTERN_KEY_BITS;
    const PatternKey mask = static_cast<PatternKey>(0xFu << shift);
    patternKeys[piece] = static_cast<PatternKey>(
        (patternKeys[piece] & static_cast<PatternKey>(~mask))
        | (static_cast<PatternKey>(pattern) << shift));
}

bool Cell::hasSamePatternsExcept(const Cell& other, Direction dir) const {
    const PatternKey mask = static_cast<PatternKey>(~(0xFu << (static_cast<int>(dir) * PATTERN_KEY_BITS)));
    return ((patternKeys[BLACK] ^ other.patternKeys[BLACK]) & mask) == 0
        && ((patternKeys[WHITE] ^ other.patternKeys[WHITE]) & mask) == 0;
}

void Cell::clearCompositePattern() {
    cPattern[BLACK] = NOT_EMPTY;
    cPattern[WHITE] = NOT_EMPTY;
}

void Cell::setCompositePattern() {
    const DerivedTables::KeyByte& black0 = DERIVED_TABLES.keyBytes[patternKeys[BLACK] & 0xFF];
    const DerivedTables::KeyByte& black1 = DERIVED_TABLES.keyBytes[patternKeys[BLACK] >> 8];
    const DerivedTables::KeyByte& white0 = DERIVED_TABLES.keyBytes[patternKeys[WHITE] & 0xFF];
    const DerivedTables::KeyByte& white1 = DERIVED_TABLES.keyBytes[patternKeys[WHITE] >> 8];
    cPattern[BLACK] = DERIVED_TABLES.blackComposite[
        black0.compositeClass | (black1.compositeClass << (2 * COMPOSITE_CLASS_BITS))];
    cPattern[WHITE] = DERIVED_TABLES.whiteComposite[
        white0.compositeClass | (white1.compositeClass << (2 * COMPOSITE_CLASS_BITS))];
}

void Cell::setScore() {
    const DerivedTables::KeyByte& black0 = DERIVED_TABLES.keyBytes[patternKeys[BLACK] & 0xFF];
    const DerivedTables::KeyByte& black1 = DERIVED_TABLES.keyBytes[patternKeys[BLACK] >> 8];
    const DerivedTables::KeyByte& white0 = DERIVED_TABLES.keyBytes[patternKeys[WHITE] & 0xFF];
    const DerivedTables::KeyByte& white1 = DERIVED_TABLES.keyBytes[patternKeys[WHITE] >> 8];
    score[BLACK] = black0.attack + black1.attack + white0.defend + white1.defend;
    score[WHITE] = white0.attack + white1.attack + black0.defend + black1.defend;
}

void Cell::updateDerived() {
    const DerivedTables::KeyByte& black0 = DERIVED_TABLES.keyBytes[patternKeys[BLACK] & 0xFF];
    const DerivedTables::KeyByte& black1 = DERIVED_TABLES.keyBytes[patternKeys[BLACK] >> 8];
    const DerivedTables::KeyByte& white0 = DERIVED_TABLES.keyBytes[patternKeys[WHITE] & 0xFF];
    const DerivedTables::KeyByte& white1 = DERIVED_TABLES.keyBytes[patternKeys[WHITE] >> 8];
    score[BLACK] = black0.attack + black1.attack + white0.defend + white1.defend;
    score[WHITE] = white0.attack + white1.attack + black0.defend + black1.defend;
    cPattern[BLACK] = DERIVED_TABLES.blackComposite[
        black0.compositeClass | (black1.compositeClass << (2 * COMPOSITE_CLASS_BITS))];
    cPattern[WHITE] = DERIVED_TABLES.whiteComposite[
        white0.compositeClass | (white1.compositeClass << (2 * COMPOSITE_CLASS_BITS))];
}
