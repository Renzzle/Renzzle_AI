#include "cell.h"
#include <array>
#include <tuple>

#define BOARD_SIZE 15
#define LINE_LENGTH 11
#define STATIC_WALL &cells[0][0];

using CellArray = array<array<Cell, BOARD_SIZE + 2>, BOARD_SIZE + 2>;

class Line {

private:
    array<Cell*, LINE_LENGTH> cells;

public:
    Cell*& operator[](size_t idx) {
        return this->cells[idx];
    }

};

class Pos {

    friend class Board;

private:
    int x, y;
    Direction dir; 
    const int inc[DIRECTION_SIZE][2] = {{0,1},{1,0},{1,1},{1,-1}};
    bool isValid() {
        return x >= 1 && x <= BOARD_SIZE && y >= 1 && y <= BOARD_SIZE;
    }

public:
    Pos() = delete;

    Pos(int x, int y) {
        this->x = x;
        this->y = y;
        dir = HORIZONTAL;
    }

    Pos(int x, int y, Direction dir) {
        this->x = x;
        this->y = y;
        this->dir = dir;
    }
    
    bool operator+(const int n) {
        this->x += inc[dir][0] * n;
        this->y += inc[dir][1] * n;
        if (!isValid()) {
            this->x -= inc[dir][0] * n;
            this->y -= inc[dir][1] * n;
            return false;
        } else return true;
    }
    
    bool operator-(const int n) {
        this->x -= inc[dir][0] * n;
        this->y -= inc[dir][1] * n;
        if (!isValid()) {
            this->x += inc[dir][0] * n;
            this->y += inc[dir][1] * n;
            return false;
        } else return true;
    }

};

class Board {

private:
    CellArray cells;
    unsigned int moveCnt;

    Line getLine(Pos& p);
    tuple<int, int, int, int> countLine(Line& line);
    Line shiftLine(Line& line, int n);
    Pattern getPattern(Line& line, bool isBlack);
    void setPatterns(Pos& p);
    void clearPattern(Cell& cell);

public:
    Board();
    CellArray& getBoardStatus();
    Cell& getCell(const Pos p);
    bool move(Pos p);
};

Board::Board() {
    moveCnt = 0;

    for (int i = 0; i < BOARD_SIZE + 2; i++) {
        for (int j = 0; j < BOARD_SIZE + 2; j++) {
            if (i == 0 || i == BOARD_SIZE + 1 || j == 0 || j == BOARD_SIZE + 1)
                cells[i][j].setPiece(WALL);
            else
                cells[i][j].setPiece(EMPTY);
        }
    }
}

CellArray& Board::getBoardStatus() {
    return cells;
}

Cell& Board::getCell(const Pos p) {
    return cells[p.x][p.y];
}

void Board::clearPattern(Cell& cell) {
    for(Direction dir = DIRECTION_START; dir < DIRECTION_SIZE; dir++) {
        cell.setPattern(BLACK, dir, PATTERN_SIZE);
        cell.setPattern(WHITE, dir, PATTERN_SIZE);
    }
}

bool Board::move(Pos p) {
    if (getCell(p).getPiece() != EMPTY)
        return false;

    moveCnt++;

    getCell(p).setPiece((moveCnt % 2 == 1) ? BLACK : WHITE);
    clearPattern(getCell(p));
    setPatterns(p);

    return true;
}

void Board::setPatterns(Pos& p) {
    for(Direction dir = DIRECTION_START; dir < DIRECTION_SIZE; dir++) {
        p.dir = dir;
        for (int i = 0; i < LINE_LENGTH; i++) {
            if (!(p + (i - (LINE_LENGTH / 2)))) {
                continue;
            }

            if(getCell(p).getPiece() == EMPTY) {
                Line line = getLine(p);
                getCell(p).setPiece(BLACK);
                getCell(p).setPattern(BLACK, dir, getPattern(line, true));
                getCell(p).setPiece(WHITE);
                getCell(p).setPattern(WHITE, dir, getPattern(line, false));
                getCell(p).setPiece(EMPTY);
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

tuple<int, int, int, int> Board::countLine(Line& line) {
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

    int self = line[mid]->getPiece();
    int oppo = !self;
    Piece piece;

    for (int i = mid - 1; i >=0; i--) {
        piece = line[i]->getPiece();
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
        piece = line[i]->getPiece();
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

Line Board::shiftLine(Line& line, int n) {
    constexpr auto len = LINE_LENGTH;

    Line shiftedLine;
    for (int i = 0; i < len; i++) {
        int idx = i + n - len / 2;
        shiftedLine[i] = idx >= 0 && idx < len ? line[idx] : STATIC_WALL;
    }
    return shiftedLine;
}

Pattern Board::getPattern(Line& line, bool isBlack) {
    constexpr auto mid = LINE_LENGTH / 2;
    Piece self = isBlack ? BLACK : WHITE;

    int realLen, fullLen, start, end;
    tie(realLen, fullLen, start, end) = countLine(line);

    if(isBlack && realLen >= 6) 
        return OVERLINE;
    else if(realLen >= 5)
        return FIVE;
    else if(fullLen < 5)
        return DEAD;

    int patternCnt[PATTERN_SIZE] = {0};
    int fiveIdx[2] = {0};
    Pattern p = DEAD;

    for(int i = start; i <= end; i++) {
        Piece piece = line[i]->getPiece();
        if(piece == EMPTY) {
            Line sl = shiftLine(line, i);
            sl[mid]->setPiece(self);

            Pattern slp = getPattern(sl, isBlack);
            sl[mid]->setPiece(EMPTY);
        
            if(slp == FIVE && patternCnt[FIVE] < 2) {
                fiveIdx[patternCnt[FIVE]] = i;
            }
            patternCnt[slp]++;
        }
    }

    if (patternCnt[FIVE] >= 2) {
        p = FREE_4;
        if(isBlack && fiveIdx[1] - fiveIdx[0] < 5) {
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

    return p;
}