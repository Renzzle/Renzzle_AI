#pragma once

#include "../test/profiler.h"
#include "line.h"
#include "pos.h"
#include "zobrist.h"
#include "../test/test.h"
#include <array>
#include <vector>

using namespace std;
using MoveList = vector<Pos>;
using CellArray = array<array<Cell, BOARD_SIZE + 2>, BOARD_SIZE + 2>;

class Board {

PRIVATE
    CellArray cells;
    MoveList path;
    Result result;
    size_t currentHash;

    void clearPattern(Cell& cell);
    void setPatterns(const Pos& p);
    void setResult(const Pos& p);
    Line getLine(int x, int y, Direction dir);
    Pattern getPattern(const Line& line, Color color);

PUBLIC
    Board();
    bool isBlackTurn();
    CellArray& getBoardStatus();
    Cell& getCell(int x, int y);
    Cell& getCell(const Pos& p);
    bool move(const Pos& p);
    void undo();
    bool pass();
    Result getResult();
    bool isForbidden(const Pos& p);
    MoveList& getPath();
    size_t getCurrentHash() const;
    size_t getChildHash(const Pos& p);
    
};

Board::Board() {
    currentHash = 0;
    result = ONGOING;
    path.reserve(BOARD_SIZE * BOARD_SIZE);

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
}

bool Board::isBlackTurn() {
    return path.size() % 2 == 0;
}

CellArray& Board::getBoardStatus() {
    return cells;
}

Cell& Board::getCell(int x, int y) {
    return cells[x][y];
}

Cell& Board::getCell(const Pos& p) {
    return cells[p.x][p.y];
}

bool Board::move(const Pos& p) {
    Cell& targetCell = getCell(p);
    if (targetCell.getPiece() != EMPTY) return false;
    if (result != ONGOING) return false;
    if (path.size() == BOARD_SIZE * BOARD_SIZE) return false;

    path.push_back(p);

    setResult(p);

    Piece piece = isBlackTurn() ? WHITE : BLACK;
    currentHash ^= getZobristValue(p.x, p.y, piece);
    targetCell.setPiece(piece);

    clearPattern(targetCell);
    targetCell.clearCompositePattern();
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

    Cell& targetCell = getCell(p);
    Piece piece = targetCell.getPiece();
    currentHash ^= getZobristValue(p.x, p.y, piece);
    
    targetCell.setPiece(EMPTY);

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

bool Board::isForbidden(const Pos& p) {
    Cell& targetCell = getCell(p);
    if (targetCell.getPiece() != EMPTY) return false;

    const CompositePattern baseCompositePattern = targetCell.getCompositePattern(BLACK);
    if (baseCompositePattern == FORBID) return true;
    if (baseCompositePattern != FORBID_33) return false;

    std::array<Pattern, DIRECTION_SIZE> basePatterns;
    for (Direction dir = DIRECTION_START; dir < DIRECTION_SIZE; dir++) {
        basePatterns[dir] = targetCell.getPattern(BLACK, dir);
    }

    int winByThree = 0;

    // recursive 3-3
    // move
    targetCell.setPiece(BLACK);
    setPatterns(p);

    const int originX = p.x;
    const int originY = p.y;

    for (Direction dir = DIRECTION_START; dir < DIRECTION_SIZE; dir++) {
        Pattern pattern = basePatterns[dir];

        // double three forbidden type
        if (pattern != FREE_3 && pattern != FREE_3A)
            continue;

        const int dx = getDirectionDx(dir);
        const int dy = getDirectionDy(dir);
        for (int i = 0; i < LINE_LENGTH; i++) {
            const int offset = i - (LINE_LENGTH / 2);
            const int x = originX + (dx * offset);
            const int y = originY + (dy * offset);
            if (!isBoardCoord(x, y))
                continue;

            Cell& cell = getCell(x, y);
            if (cell.getPiece() == EMPTY) {
                bool isFive = false;
                Pos posi(x, y);
                if (cell.getPattern(BLACK, dir) == FREE_4 && !isForbidden(posi)) {
                    for (Direction eDir = DIRECTION_START; eDir < DIRECTION_SIZE; eDir++) {
                        Pattern nextPattern = cell.getPattern(BLACK, eDir);
                        if (nextPattern == FIVE)
                            isFive = true;
                    }
                    // made 5 with an empty space -> not a forbidden position
                    if (!isFive) {
                        winByThree++;
                        break;
                    }
                }
            }
        }

        if (winByThree >= 2) {
            break;
        }
    }

    // undo
    targetCell.setPiece(EMPTY);
    setPatterns(p);

    return winByThree >= 2;
}

MoveList& Board::getPath() {
    return path;
}

size_t Board::getCurrentHash() const {
    return currentHash;
}

size_t Board::getChildHash(const Pos& p) {
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

void Board::setPatterns(const Pos& p) {
    const int originX = p.x;
    const int originY = p.y;

    if (getCell(p).getPiece() != EMPTY) {
        for (Direction dir = DIRECTION_START; dir < DIRECTION_SIZE; dir++) {
            const int dx = getDirectionDx(dir);
            const int dy = getDirectionDy(dir);
            for (int i = 0; i < LINE_LENGTH; i++) {
                const int offset = i - (LINE_LENGTH / 2);
                const int x = originX + (dx * offset);
                const int y = originY + (dy * offset);
                if (!isBoardCoord(x, y)) {
                    continue;
                }

                Cell& c = getCell(x, y);
                if (c.getPiece() == EMPTY) {
                    Line line = getLine(x, y, dir);
                    constexpr int mid = LINE_LENGTH / 2;
                    line[mid] = BLACK;
                    c.setPattern(BLACK, dir, getPattern(line, COLOR_BLACK));
                    line[mid] = WHITE;
                    c.setPattern(WHITE, dir, getPattern(line, COLOR_WHITE));
                    c.setScore();
                    c.setCompositePattern();
                }
            }
        }
        return;
    }

    std::array<Pos, LINE_LENGTH * DIRECTION_SIZE> touchedCells;
    size_t touchedCount = 0;
    std::array<std::array<bool, BOARD_SIZE + 2>, BOARD_SIZE + 2> isTouched = {};

    for (Direction dir = DIRECTION_START; dir < DIRECTION_SIZE; dir++) {
        const int dx = getDirectionDx(dir);
        const int dy = getDirectionDy(dir);
        for (int i = 0; i < LINE_LENGTH; i++) {
            const int offset = i - (LINE_LENGTH / 2);
            const int x = originX + (dx * offset);
            const int y = originY + (dy * offset);
            if (!isBoardCoord(x, y)) {
                continue;
            }

            Cell& c = getCell(x, y);
            if (c.getPiece() == EMPTY) {
                Line line = getLine(x, y, dir);
                constexpr int mid = LINE_LENGTH / 2;
                line[mid] = BLACK;
                c.setPattern(BLACK, dir, getPattern(line, COLOR_BLACK));
                line[mid] = WHITE;
                c.setPattern(WHITE, dir, getPattern(line, COLOR_WHITE));

                if (!isTouched[x][y]) {
                    isTouched[x][y] = true;
                    touchedCells[touchedCount++] = Pos(x, y);
                }
            }
        }
    }

    for (size_t i = 0; i < touchedCount; i++) {
        Cell& c = getCell(touchedCells[i]);
        c.setScore();
        c.setCompositePattern();
    }
}

Line Board::getLine(int x, int y, Direction dir) {
    Line line;
    const int dx = getDirectionDx(dir);
    const int dy = getDirectionDy(dir);

    for (int i = 0; i < LINE_LENGTH; i++) {
        const int offset = i - (LINE_LENGTH / 2);
        const int nx = x + (dx * offset);
        const int ny = y + (dy * offset);
        if (!isBoardCoord(nx, ny)) {
            line[i] = WALL;
            continue; 
        }
        line[i] = getCell(nx, ny).getPiece();
    }

    return line;
}

Pattern Board::getPattern(const Line& line, Color color) {
    constexpr uint32_t PIECE_BITS = 2;
    constexpr uint32_t COLOR_SHIFT = LINE_LENGTH * PIECE_BITS;
    constexpr uint32_t CACHE_SIZE = 1u << (COLOR_SHIFT + 1);
    constexpr uint8_t CACHE_UNSET = 0;

    static array<uint8_t, CACHE_SIZE> patternCache = {};

    uint32_t key = (color == COLOR_WHITE) ? (1u << COLOR_SHIFT) : 0u;
    for (int i = 0; i < LINE_LENGTH; ++i) {
        key |= static_cast<uint32_t>(line[i]) << (i * PIECE_BITS);
    }

    const uint8_t cachedPattern = patternCache[key];
    if (cachedPattern != CACHE_UNSET) {
        return static_cast<Pattern>(cachedPattern - 1);
    }

    constexpr auto mid = LINE_LENGTH / 2;
    bool isBlack = color == COLOR_BLACK;
    Piece self = isBlack ? BLACK : WHITE;

    int realLen, fullLen, start, end;
    tie(realLen, fullLen, start, end) = line.countLine();

    if (isBlack && realLen >= 6) {
        patternCache[key] = static_cast<uint8_t>(OVERLINE) + 1;
        return OVERLINE;
    }
    else if (realLen >= 5) {
        patternCache[key] = static_cast<uint8_t>(FIVE) + 1;
        return FIVE;
    }
    else if (fullLen < 5) {
        patternCache[key] = static_cast<uint8_t>(DEAD) + 1;
        return DEAD;
    }

    int patternCnt[PATTERN_SIZE] = {0};
    int fiveIdx[2] = {0};
    Pattern p = DEAD;

    for (int i = start; i <= end; i++) {
        Piece piece = line[i];
        if (piece == EMPTY) {
            Line sl = line.shiftLine(i);
            sl[mid] = self;
            Pattern slp = getPattern(sl, color);
        
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
    
    patternCache[key] = static_cast<uint8_t>(p) + 1;
    return p;
}

void Board::setResult(const Pos& p) {
    bool isBlackTurn = this->isBlackTurn();
    Cell& targetCell = getCell(p);

    if (!isBlackTurn && isForbidden(p)) {
        result = WHITE_WIN;
        return;
    }

    Piece self = isBlackTurn ? WHITE : BLACK;
    for (Direction dir = DIRECTION_START; dir < DIRECTION_SIZE; dir++) {
        if (targetCell.getPattern(self, dir) == FIVE) {
            result = isBlackTurn ? WHITE_WIN : BLACK_WIN;
            return;
        }
    }

    result = ONGOING;
}
