#include "cell.h"

#define BOARD_SIZE 15
#define LINE_LENGTH 11
#define STATIC_WALL &cells[0][0];

using PieceArray = array<array<Piece, BOARD_SIZE + 2>, BOARD_SIZE + 2>;
using CellArray = array<array<Cell, BOARD_SIZE + 2>, BOARD_SIZE + 2>;

class Line {

private:
    array<Cell*, LINE_LENGTH> cells;

public:
    Direction dir;
    Cell*& operator[](size_t idx) {
        return this->cells[idx];
    }

};

class Board {

public:
    CellArray cells;
    unsigned int moveCnt;
    Line getLine(int x, int y, Direction d);
    tuple<int, int, int, int> countLine(Line& line);
    Line shiftLine(Line& line, int n);
    Pattern getPattern(Line& line, bool isBlack); 
    void clearPattern(Cell& cell);

// public:
    Board();
    PieceArray getBoardStatus();
    bool move(int x, int y);
    // TODO: undo 함수
};

Board::Board() {
    moveCnt = 0;

    for (int i = 0; i < BOARD_SIZE + 2; i++) {
        for (int j = 0; j < BOARD_SIZE + 2; j++) {
            if (i == 0 || i == BOARD_SIZE + 1 || j == 0 || j == BOARD_SIZE + 1)
                cells[i][j].piece = WALL;
            else
                cells[i][j].piece = EMPTY;
        }
    }
}

void Board::clearPattern(Cell& cell) {
    for(int i = 0; i < 2; i++) {
        for(int j = 0; j < DIRECTION_SIZE; j++) {
            cell.patterns[i][j] = PATTERN_SIZE;
        }
    }
}

bool Board::move(int x, int y) {
    assert(1 <= x && x <= 15);
    assert(1 <= y && y <= 15);

    if (cells[x][y].piece != EMPTY)
        return false;

    moveCnt++;

    cells[x][y].piece = (moveCnt % 2 == 1) ? BLACK : WHITE;
    clearPattern(cells[x][y]);

    const int startX = x - LINE_LENGTH / 2;
    const int startY = y - LINE_LENGTH / 2;
    const int endY = y + LINE_LENGTH / 2;

    const int starts[4][2] = {{x, startY},{startX, y},{startX, startY},{startX, endY}};
    const int inc[4][2] = {{0,1},{1,0},{1,1},{1,-1}};

    for(int j = 0; j < DIRECTION_SIZE; j++) {
        for (int i = 0; i < LINE_LENGTH; i++) {
            int cx = starts[j][0] + inc[j][0] * i;
            int cy = starts[j][1] + inc[j][1] * i;
            if (cx < 1 || cx > 15 || cy < 1 || cy > 15) {
                continue;
            }

            if(cells[cx][cy].piece == EMPTY) {
                Line line = getLine(cx, cy, static_cast<Direction>(j));
                cells[cx][cy].piece = BLACK;
                cells[cx][cy].patterns[BLACK][j] = getPattern(line, true);
                cells[cx][cy].piece = WHITE;
                cells[cx][cy].patterns[WHITE][j] = getPattern(line, false);
                cells[cx][cy].piece = EMPTY;
            }
        }
    }

    return true;
}

Line Board::getLine(int x, int y, Direction d) {
    assert(1 <= x && x <= 15);
    assert(1 <= y && y <= 15);
    
    Line line;

    const int startX = x - LINE_LENGTH / 2;
    const int startY = y - LINE_LENGTH / 2;
    const int endY = y + LINE_LENGTH / 2;

    const int starts[4][2] = {{x, startY},{startX, y},{startX, startY},{startX, endY}};
    const int inc[4][2] = {{0,1},{1,0},{1,1},{1,-1}};

    line.dir = d;

    for (int i = 0; i < LINE_LENGTH; i++) {
        int cx = starts[d][0] + inc[d][0] * i;
        int cy = starts[d][1] + inc[d][1] * i;
        if (cx < 1 || cx > 15 || cy < 1 || cy > 15) {
            line[i] = STATIC_WALL;
            continue; 
        }
        line[i] = &cells[cx][cy];
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

    int self = (line[mid]->piece % 2 == 0) ? BLACK : WHITE;
    int oppo = !self;

    for (int i = mid - 1; i >=0; i--) {
        if (line[i]->piece == self)
            realLen += realLenInc;
        else if (line[i]->piece == oppo || line[i]->piece == WALL)
            break;
        else
            realLenInc = 0;

        fullLen++;
        start = i;        
    }

    realLenInc = 1;

    for (int i = mid + 1; i < LINE_LENGTH; i++) {
        if (line[i]->piece == self)
            realLen += realLenInc;
        else if (line[i]->piece == oppo || line[i]->piece == WALL)
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
        if(line[i]->piece == EMPTY) {
            Line sl = shiftLine(line, i);
            sl[mid]->piece = self;

            Pattern slp = getPattern(sl, isBlack);
            sl[mid]->piece = EMPTY;
        
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


