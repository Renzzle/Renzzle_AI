#pragma once

#include "../test/profiler.h"
#include "line.h"
#include "pos.h"
#include "zobrist.h"
#include "../test/test.h"
#include <array>
#include <vector>
#include <unordered_set>
#include <unordered_map>

#define STATIC_WALL &cells[0][0];

using namespace std;
using MoveList = vector<Pos>;
using CellArray = array<array<Cell, BOARD_SIZE + 2>, BOARD_SIZE + 2>;

struct LineCacheKey {
    std::array<Piece, LINE_LENGTH> pieces;
    Color analyzeColor;

    bool operator==(const LineCacheKey& other) const {
        return pieces == other.pieces && analyzeColor == other.analyzeColor;
    }
};

namespace std {
    template <>
    struct hash<LineCacheKey> {
        size_t operator()(const LineCacheKey& k) const {
            size_t seed = k.pieces.size();
            for(Piece p : k.pieces) {
                seed ^= hash<int>()(static_cast<int>(p)) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            }
            seed ^= hash<int>()(static_cast<int>(k.analyzeColor)) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            return seed;
        }
    };
}

class Board {

PRIVATE
    CellArray cells;
    MoveList path;
    Result result;
    size_t currentHash;

    void clearPattern(Cell& cell);
    void setPatterns(Pos& p);
    void setResult(Pos& p);
    Line getLine(Pos& p);
    Pattern getPattern(Line& line, Color color);
    void updatePatternMap(const Pos& p, Piece piece, CompositePattern oldPattern, CompositePattern newPattern);
    unordered_set<Pos, PosHash> patternMap[2][COMPOSITE_PATTERN_SIZE];
    

PUBLIC
    Board();
    bool isBlackTurn();
    CellArray& getBoardStatus();
    Cell& getCell(const Pos p);
    bool move(Pos p);
    void undo();
    bool pass();
    Result getResult();
    bool isForbidden(Pos p);
    MoveList& getPath();
    size_t getCurrentHash() const;
    size_t getChildHash(Pos p);
    const unordered_set<Pos, PosHash>& getMovesByPattern(Piece piece, CompositePattern pattern) const;
};

Board::Board() {
    currentHash = 0;
    result = ONGOING;

    for (int i = 0; i < BOARD_SIZE + 2; i++) {
        for (int j = 0; j < BOARD_SIZE + 2; j++) {
            if (i == 0 || i == BOARD_SIZE + 1 || j == 0 || j == BOARD_SIZE + 1) {
                cells[i][j].setPiece(WALL);
                currentHash ^= getZobristValue(i, j, WALL);
            } else {
                cells[i][j].setPiece(EMPTY);
            }
        }
    }

    for (int i = 1; i <= BOARD_SIZE; i++) {
        for (int j = 1; j <= BOARD_SIZE; j++) {
            Pos p(i, j);
            if (getCell(p).getPiece() == EMPTY) {
                patternMap[BLACK][ETC].insert(p);
                patternMap[WHITE][ETC].insert(p);
            }
        }
    }
}

bool Board::isBlackTurn() {
    return path.size() % 2 == 0;
}

CellArray& Board::getBoardStatus() {
    return cells;
}

Cell& Board::getCell(const Pos p) {
    return cells[p.x][p.y];
}

bool Board::move(Pos p) {

    if (getResult() != ONGOING) {
        return false;
    }

    if (getCell(p).getPiece() != EMPTY) return false;
    if (result != ONGOING) return false;
    if (path.size() == BOARD_SIZE * BOARD_SIZE) return false;

    Cell& c = getCell(p);
    CompositePattern oldBlackPattern = c.getCompositePattern(BLACK);
    CompositePattern oldWhitePattern = c.getCompositePattern(WHITE);

    path.push_back(p);

    setResult(p);

    Piece piece = isBlackTurn() ? WHITE : BLACK;
    currentHash ^= getZobristValue(p.x, p.y, piece);
    getCell(p).setPiece(piece);

    clearPattern(getCell(p));
    c.clearCompositePattern();

    updatePatternMap(p, BLACK, oldBlackPattern, NOT_EMPTY);
    updatePatternMap(p, WHITE, oldWhitePattern, NOT_EMPTY);

    setPatterns(p);

    return true;
}

void Board::undo() {
    if (path.empty()) return;
    Pos p = path.back();

    // if passed
    if (p.isDefault()) {
        path.pop_back();
        return;
    }

    Piece piece = getCell(p).getPiece();
    currentHash ^= getZobristValue(p.x, p.y, piece);
    
    getCell(p).setPiece(EMPTY);

    path.pop_back();

    setPatterns(p);

    result = ONGOING;
}

bool Board::pass() {
    if (result != ONGOING) return false;
    if (path.size() == BOARD_SIZE * BOARD_SIZE) return false;

    Pos p;
    path.push_back(p);
    return true;    
}

Result Board::getResult() {
    return result;
}

bool Board::isForbidden(Pos p) {
    Cell c = getCell(p);
    if (c.getPiece() != EMPTY) return false;
    if (c.getCompositePattern(BLACK) == FORBID) return true;
    if (c.getCompositePattern(BLACK) != FORBID_33) return false;

    int winByThree = 0;

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

MoveList& Board::getPath() {
    return path;
}

size_t Board::getCurrentHash() const {
    return currentHash;
}

size_t Board::getChildHash(Pos p) {
    size_t result = currentHash;
    Piece piece = isBlackTurn() ? BLACK : WHITE;
    result ^= getZobristValue(p.x, p.y, piece);
    return result;
}

void Board::clearPattern(Cell& cell) {
    for(Direction dir = DIRECTION_START; dir < DIRECTION_SIZE; dir++) {
        cell.setPattern(BLACK, dir, PATTERN_SIZE);
        cell.setPattern(WHITE, dir, PATTERN_SIZE);
    }
}

void Board::updatePatternMap(const Pos& p, Piece piece, CompositePattern oldPattern, CompositePattern newPattern) {
     patternMap[piece][oldPattern].erase(p);
     patternMap[piece][newPattern].insert(p);
}

const std::unordered_set<Pos, PosHash>& Board::getMovesByPattern(Piece piece, CompositePattern pattern) const {
    if (pattern < COMPOSITE_PATTERN_SIZE) {
        return patternMap[piece][pattern];
    }

    static const std::unordered_set<Pos, PosHash> empty_set;
    return empty_set;
}

void Board::setPatterns(Pos& p) {
    PROFILE_FUNCTION();
    for (Direction dir = DIRECTION_START; dir < DIRECTION_SIZE; dir++) {
        p.dir = dir;
        for (int i = 0; i < LINE_LENGTH; i++) {
            if (!(p + (i - (LINE_LENGTH / 2)))) {
                continue;
            }

            Pos curP = p;
            Cell& c = getCell(p);
            if (c.getPiece() == EMPTY) {
                CompositePattern oldBlackPattern = c.getCompositePattern(BLACK);
                CompositePattern oldWhitePattern = c.getCompositePattern(WHITE);
                
                Line line = getLine(p);
                c.setPiece(BLACK);
                c.setPattern(BLACK, dir, getPattern(line, COLOR_BLACK));
                c.setPiece(WHITE);
                c.setPattern(WHITE, dir, getPattern(line, COLOR_WHITE));
                c.setPiece(EMPTY);

                c.setScore();
                c.setCompositePattern();

                CompositePattern newBlackPattern = c.getCompositePattern(BLACK);
                CompositePattern newWhitePattern = c.getCompositePattern(WHITE);

                if (oldBlackPattern != newBlackPattern) {
                    updatePatternMap(curP, BLACK, oldBlackPattern, newBlackPattern);
                }
                if (oldWhitePattern != newWhitePattern) {
                    updatePatternMap(curP, WHITE, oldWhitePattern, newWhitePattern);
                }
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
    PROFILE_FUNCTION();
    static std::unordered_map<LineCacheKey, Pattern> patternCache;

    LineCacheKey key;
    for (int i = 0; i < LINE_LENGTH; ++i) {
        key.pieces[i] = line[i]->getPiece();
    }
    key.analyzeColor = color;

    auto it = patternCache.find(key);
    if (it != patternCache.end()) {
        return it->second; // cache hit
    }

    constexpr auto mid = LINE_LENGTH / 2;
    bool isBlack = color == COLOR_BLACK;
    Piece self = isBlack ? BLACK : WHITE;

    int realLen, fullLen, start, end;
    tie(realLen, fullLen, start, end) = line.countLine();

    if (isBlack && realLen >= 6) {
        patternCache[key] = OVERLINE;
        return OVERLINE;
    }
    else if (realLen >= 5) {
        patternCache[key] = FIVE;
        return FIVE;
    }
    else if (fullLen < 5) {
        patternCache[key] = DEAD;
        return DEAD;
    }

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
    
    patternCache[key] = p;
    return p;
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