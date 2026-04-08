#pragma once

#include "../test/profiler.h"
#include "line.h"
#include "move_bucket.h"
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
    static constexpr int LINE_PADDING = LINE_LENGTH / 2;
    static constexpr int BIT_LINE_SIZE = BOARD_SIZE + (LINE_PADDING * 2);
    static constexpr int BIT_DIAG_SIZE = (BIT_LINE_SIZE * 2) - 1;
    static constexpr uint64_t LINE_KEY_MASK = (1ull << (LINE_LENGTH * 2)) - 1ull;
    static constexpr int CENTER_SHIFT = (LINE_LENGTH / 2) * 2;

    CellArray cells;
    MoveList path;
    Result result;
    size_t currentHash;
    array<uint64_t, BIT_LINE_SIZE> horizontalKeys;
    array<uint64_t, BIT_LINE_SIZE> verticalKeys;
    array<uint64_t, BIT_DIAG_SIZE> upwardKeys;
    array<uint64_t, BIT_DIAG_SIZE> downwardKeys;
    MoveBucket patternBuckets[2][COMPOSITE_PATTERN_SIZE];

    void clearPattern(Cell& cell);
    void setPatterns(const Pos& p);
    void setResult(const Pos& p);
    void addPatternBucketState(const Pos& p, const Cell& cell);
    void removePatternBucketState(const Pos& p, const Cell& cell);
    Line getLine(int x, int y, Direction dir);
    Pattern getPattern(const Line& line, Color color);
    Pattern getPattern(uint32_t lineKey, Color color);
    void setBitKeys(int x, int y, Piece piece);
    uint32_t getLineKey(int x, int y, Direction dir) const;
    static int toBitCoord(int coord);
    static uint64_t makeFilledBitLine(Piece piece);
    static Piece getLineKeyPiece(uint32_t lineKey, int index);
    static uint32_t setLineKeyPiece(uint32_t lineKey, int index, Piece piece);
    static tuple<int, int, int, int> countLineKey(uint32_t lineKey);
    static uint32_t shiftLineKey(uint32_t lineKey, int n);

PUBLIC
    Board();
    bool isBlackTurn();
    CellArray& getBoardStatus();
    Cell& getCell(int x, int y);
    const Cell& getCell(int x, int y) const;
    Cell& getCell(const Pos& p);
    const Cell& getCell(const Pos& p) const;
    bool move(const Pos& p);
    void undo();
    bool pass();
    Result getResult();
    bool isForbidden(const Pos& p);
    bool hasCompositePattern(Piece piece, CompositePattern pattern) const;
    int getCompositePatternCount(Piece piece, CompositePattern pattern) const;
    Pos getFirstPatternPos(Piece piece, CompositePattern pattern) const;
    const MoveBucket& getPatternBucket(Piece piece, CompositePattern pattern) const;
    MoveList& getPath();
    size_t getCurrentHash() const;
    size_t getChildHash(const Pos& p);
    
};

Board::Board() {
    currentHash = 0;
    result = ONGOING;
    path.reserve(BOARD_SIZE * BOARD_SIZE);
    horizontalKeys.fill(makeFilledBitLine(WALL));
    verticalKeys.fill(makeFilledBitLine(WALL));
    upwardKeys.fill(makeFilledBitLine(WALL));
    downwardKeys.fill(makeFilledBitLine(WALL));

    for (int i = 0; i < BOARD_SIZE + 2; i++) {
        for (int j = 0; j < BOARD_SIZE + 2; j++) {
            if (i == 0 || i == BOARD_SIZE + 1 || j == 0 || j == BOARD_SIZE + 1) {
                cells[i][j].setPiece(WALL);
                currentHash ^= getZobristValue(i, j, WALL);
            } else {
                cells[i][j].setPiece(EMPTY);
                setBitKeys(i, j, EMPTY);
            }
        }
    }

    for (int i = 1; i <= BOARD_SIZE; ++i) {
        for (int j = 1; j <= BOARD_SIZE; ++j) {
            addPatternBucketState(Pos(i, j), cells[i][j]);
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

const Cell& Board::getCell(int x, int y) const {
    return cells[x][y];
}

Cell& Board::getCell(const Pos& p) {
    return cells[p.x][p.y];
}

const Cell& Board::getCell(const Pos& p) const {
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
    removePatternBucketState(p, targetCell);
    currentHash ^= getZobristValue(p.x, p.y, piece);
    targetCell.setPiece(piece);
    setBitKeys(p.x, p.y, piece);

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
    setBitKeys(p.x, p.y, EMPTY);

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
    removePatternBucketState(p, targetCell);
    targetCell.setPiece(BLACK);
    setBitKeys(p.x, p.y, BLACK);
    clearPattern(targetCell);
    targetCell.clearCompositePattern();
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
    setBitKeys(p.x, p.y, EMPTY);
    setPatterns(p);

    return winByThree >= 2;
}

MoveList& Board::getPath() {
    return path;
}

bool Board::hasCompositePattern(Piece piece, CompositePattern pattern) const {
    return !patternBuckets[piece][pattern].empty();
}

int Board::getCompositePatternCount(Piece piece, CompositePattern pattern) const {
    return patternBuckets[piece][pattern].size();
}

Pos Board::getFirstPatternPos(Piece piece, CompositePattern pattern) const {
    return patternBuckets[piece][pattern].front();
}

const MoveBucket& Board::getPatternBucket(Piece piece, CompositePattern pattern) const {
    return patternBuckets[piece][pattern];
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

int Board::toBitCoord(int coord) {
    return coord + LINE_PADDING - 1;
}

uint64_t Board::makeFilledBitLine(Piece piece) {
    uint64_t line = 0;
    for (int i = 0; i < BIT_LINE_SIZE; ++i) {
        line |= static_cast<uint64_t>(piece) << (i * 2);
    }
    return line;
}

Piece Board::getLineKeyPiece(uint32_t lineKey, int index) {
    return static_cast<Piece>((lineKey >> (index * 2)) & 0x3u);
}

uint32_t Board::setLineKeyPiece(uint32_t lineKey, int index, Piece piece) {
    const uint32_t mask = 0x3u << (index * 2);
    return (lineKey & ~mask) | (static_cast<uint32_t>(piece) << (index * 2));
}

tuple<int, int, int, int> Board::countLineKey(uint32_t lineKey) {
    constexpr int mid = LINE_LENGTH / 2;

    int realLen = 1;
    int fullLen = 1;
    int realLenInc = 1;
    int start = mid;
    int end = mid;

    Piece self = getLineKeyPiece(lineKey, mid);
    Piece oppo = self == BLACK ? WHITE : BLACK;

    for (int i = mid - 1; i >= 0; --i) {
        Piece piece = getLineKeyPiece(lineKey, i);
        if (piece == self) {
            realLen += realLenInc;
        } else if (piece == oppo || piece == WALL) {
            break;
        } else {
            realLenInc = 0;
        }

        fullLen++;
        start = i;
    }

    realLenInc = 1;

    for (int i = mid + 1; i < LINE_LENGTH; ++i) {
        Piece piece = getLineKeyPiece(lineKey, i);
        if (piece == self) {
            realLen += realLenInc;
        } else if (piece == oppo || piece == WALL) {
            break;
        } else {
            realLenInc = 0;
        }

        fullLen++;
        end = i;
    }

    return make_tuple(realLen, fullLen, start, end);
}

uint32_t Board::shiftLineKey(uint32_t lineKey, int n) {
    uint32_t shiftedLineKey = 0;
    for (int i = 0; i < LINE_LENGTH; ++i) {
        int idx = i + n - (LINE_LENGTH / 2);
        Piece piece = (idx >= 0 && idx < LINE_LENGTH)
            ? getLineKeyPiece(lineKey, idx)
            : WALL;
        shiftedLineKey = setLineKeyPiece(shiftedLineKey, i, piece);
    }
    return shiftedLineKey;
}

void Board::addPatternBucketState(const Pos& p, const Cell& cell) {
    if (cell.getPiece() != EMPTY) return;
    const CompositePattern blackPattern = cell.getCompositePattern(BLACK);
    const CompositePattern whitePattern = cell.getCompositePattern(WHITE);
    if (blackPattern != NOT_EMPTY) {
        patternBuckets[BLACK][blackPattern].insert(p);
    }
    if (whitePattern != NOT_EMPTY) {
        patternBuckets[WHITE][whitePattern].insert(p);
    }
}

void Board::removePatternBucketState(const Pos& p, const Cell& cell) {
    if (cell.getPiece() != EMPTY) return;
    const CompositePattern blackPattern = cell.getCompositePattern(BLACK);
    const CompositePattern whitePattern = cell.getCompositePattern(WHITE);
    if (blackPattern != NOT_EMPTY) {
        patternBuckets[BLACK][blackPattern].erase(p);
    }
    if (whitePattern != NOT_EMPTY) {
        patternBuckets[WHITE][whitePattern].erase(p);
    }
}

void Board::setBitKeys(int x, int y, Piece piece) {
    const int bx = toBitCoord(x);
    const int by = toBitCoord(y);
    const uint64_t value = static_cast<uint64_t>(piece);
    const uint64_t xShift = static_cast<uint64_t>(bx * 2);
    const uint64_t yShift = static_cast<uint64_t>(by * 2);
    const uint64_t xMask = 0x3ull << xShift;
    const uint64_t yMask = 0x3ull << yShift;

    horizontalKeys[bx] = (horizontalKeys[bx] & ~yMask) | (value << yShift);
    verticalKeys[by] = (verticalKeys[by] & ~xMask) | (value << xShift);
    upwardKeys[bx - by + (BIT_LINE_SIZE - 1)] =
        (upwardKeys[bx - by + (BIT_LINE_SIZE - 1)] & ~xMask) | (value << xShift);
    downwardKeys[bx + by] = (downwardKeys[bx + by] & ~xMask) | (value << xShift);
}

uint32_t Board::getLineKey(int x, int y, Direction dir) const {
    const int bx = toBitCoord(x);
    const int by = toBitCoord(y);

    switch (dir) {
        case HORIZONTAL:
            return static_cast<uint32_t>((horizontalKeys[bx] >> ((by - LINE_PADDING) * 2)) & LINE_KEY_MASK);
        case VERTICAL:
            return static_cast<uint32_t>((verticalKeys[by] >> ((bx - LINE_PADDING) * 2)) & LINE_KEY_MASK);
        case UPWARD:
            return static_cast<uint32_t>((upwardKeys[bx - by + (BIT_LINE_SIZE - 1)] >> ((bx - LINE_PADDING) * 2)) & LINE_KEY_MASK);
        case DOWNWARD:
            return static_cast<uint32_t>((downwardKeys[bx + by] >> ((bx - LINE_PADDING) * 2)) & LINE_KEY_MASK);
        default:
            return 0;
    }
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
                if (!isTouched[x][y]) {
                    removePatternBucketState(Pos(x, y), c);
                }
                const uint32_t lineKey = getLineKey(x, y, dir);
                const uint32_t blackLineKey =
                    (lineKey & ~(0x3u << CENTER_SHIFT)) | (static_cast<uint32_t>(BLACK) << CENTER_SHIFT);
                const uint32_t whiteLineKey =
                    (lineKey & ~(0x3u << CENTER_SHIFT)) | (static_cast<uint32_t>(WHITE) << CENTER_SHIFT);

                c.setPattern(BLACK, dir, getPattern(blackLineKey, COLOR_BLACK));
                c.setPattern(WHITE, dir, getPattern(whiteLineKey, COLOR_WHITE));

                if (!isTouched[x][y]) {
                    isTouched[x][y] = true;
                    touchedCells[touchedCount++] = Pos(x, y);
                }
            }
        }
    }

    for (size_t i = 0; i < touchedCount; i++) {
        const Pos touchedPos = touchedCells[i];
        Cell& c = getCell(touchedPos);
        c.updateDerived();
        addPatternBucketState(touchedPos, c);
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
    uint32_t lineKey = 0;
    for (int i = 0; i < LINE_LENGTH; ++i) {
        lineKey |= static_cast<uint32_t>(line[i]) << (i * 2);
    }
    return getPattern(lineKey, color);
}

Pattern Board::getPattern(uint32_t lineKey, Color color) {
    constexpr uint32_t PIECE_BITS = 2;
    constexpr uint32_t COLOR_SHIFT = LINE_LENGTH * PIECE_BITS;
    constexpr uint32_t CACHE_SIZE = 1u << (COLOR_SHIFT + 1);
    constexpr uint8_t CACHE_UNSET = 0;

    static array<uint8_t, CACHE_SIZE> patternCache = {};

    uint32_t key = (color == COLOR_WHITE) ? (1u << COLOR_SHIFT) : 0u;
    key |= lineKey;

    const uint8_t cachedPattern = patternCache[key];
    if (cachedPattern != CACHE_UNSET) {
        return static_cast<Pattern>(cachedPattern - 1);
    }

    constexpr auto mid = LINE_LENGTH / 2;
    bool isBlack = color == COLOR_BLACK;
    Piece self = isBlack ? BLACK : WHITE;

    int realLen, fullLen, start, end;
    tie(realLen, fullLen, start, end) = countLineKey(lineKey);

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
        Piece piece = getLineKeyPiece(lineKey, i);
        if (piece == EMPTY) {
            uint32_t shiftedLineKey = shiftLineKey(lineKey, i);
            shiftedLineKey = setLineKeyPiece(shiftedLineKey, mid, self);
            Pattern slp = getPattern(shiftedLineKey, color);
        
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
