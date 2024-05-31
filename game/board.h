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

using Lines = array<Line, DIRECTION_SIZE>;

class Board {

public:
    CellArray cells;
    unsigned int moveCnt;
    Lines getLines(int x, int y);
    tuple<int, int, int, int> countLine(Line &line);
    Line shiftLine(Line &line, int n);
    Pattern setPattern(Line &line); 

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

bool Board::move(int x, int y) {
    assert(1 <= x && x <= 15);
    assert(1 <= y && y <= 15);

    moveCnt++;

    if (cells[x][y].piece != EMPTY)
        return false;

    cout << "1" << endl;

    cells[x][y].piece = (moveCnt % 2 == 1) ? BLACK : WHITE;

    Lines lines = getLines(x, y);

    cout << "2" << endl;
    for(int i = 0; i < DIRECTION_SIZE; i++) {
        cout << "3" << endl;
        setPattern(lines[i]);
        cout << "4" << endl;
    }
    return true;
}

Lines Board::getLines(int x, int y) {
    assert(1 <= x && x <= 15);
    assert(1 <= y && y <= 15);
    
    Lines lines;

    const int startX = x - LINE_LENGTH / 2;
    const int startY = y - LINE_LENGTH / 2;
    const int endY = y + LINE_LENGTH / 2;

    lines[HORIZONTAL].dir = HORIZONTAL;
    for (int i = 0; i < LINE_LENGTH; i++) {
        if (startY + i < 1 || startY + i > 15) {
            lines[HORIZONTAL][i] = STATIC_WALL;
            cout << lines[HORIZONTAL][i] << "\t";
            continue; 
        }
        lines[HORIZONTAL][i] = &cells[x][startY + i];
        cout << lines[HORIZONTAL][i] << "\t";
    }

    cout << endl;

    lines[VERTICAL].dir = VERTICAL;
    for (int i = 0; i < LINE_LENGTH; i++) {
        if (startX + i < 1 || startX + i > 15) {
            lines[VERTICAL][i] = STATIC_WALL;
            cout << lines[VERTICAL][i] << "\t";
            continue;
        }
        lines[VERTICAL][i] = &cells[startX + i][y];   
        cout << lines[VERTICAL][i] << "\t";     
    }

    cout << endl;

    lines[UPWARD].dir = UPWARD;
    for (int i = 0; i < LINE_LENGTH; i++) {
        if (startX + i < 1 || startX + i > 15) {
            lines[UPWARD][i] = STATIC_WALL;
            cout << lines[UPWARD][i] << "\t";   
            continue;
        }
        if (startY + i < 1 || startY + i > 15) {
            lines[UPWARD][i] = STATIC_WALL;
            cout << lines[UPWARD][i] << "\t";   
            continue;
        }
        lines[UPWARD][i] = &cells[startX + i][startY + i];
        cout << lines[UPWARD][i] << "\t";   
    }

    cout << endl;
    
    lines[DOWNWARD].dir = DOWNWARD;
    for (int i = 0; i < LINE_LENGTH; i++) {
        if (startX + i < 1 || startX + i > 15) {
            lines[DOWNWARD][i] = STATIC_WALL;
            cout << lines[DOWNWARD][i] << "\t";   
            continue;
        }
        if (endY - i < 1 || endY - i > 15) {
            lines[DOWNWARD][i] = STATIC_WALL;
            cout << lines[DOWNWARD][i] << "\t";   
            continue;
        }
        lines[DOWNWARD][i] = &cells[startX + i][endY - i];
        cout << lines[DOWNWARD][i] << "\t";   
    }

    cout << endl;

    return lines;
}

tuple<int, int, int, int> Board::countLine(Line &line) {
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

    int self = (line[mid]->piece % 2 == 1) ? BLACK : WHITE;
    int oppo = (line[mid]->piece % 2 == 1) ? WHITE : BLACK;

    for (int i = mid - 1; i >=0; i--) {
        if (line[i]->piece == self)
            realLen += realLenInc;
        else if (line[i]->piece == oppo)
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
        else if (line[i]->piece == oppo)
            break;
        else
            realLenInc = 0;

        fullLen++;
        end = i;    
    }

    return make_tuple(realLen, fullLen, start, end);
}

Line Board::shiftLine(Line &line, int n) {
    constexpr auto len = LINE_LENGTH;

    Line shiftedLine;
    for (int i = 0; i < len; i++) {
        int idx = i + n - len / 2;
        shiftedLine[i] = idx >= 0 && idx < len ? line[idx] : STATIC_WALL;
    }
    return shiftedLine;
}

Pattern Board::setPattern(Line &line) {
    constexpr auto mid = LINE_LENGTH / 2;
    bool isBlack = this->moveCnt % 2 == 1;

    int realLen, fullLen, start, end;
    tie(realLen, fullLen, start, end) = countLine(line);

    cout << "3.1" << endl;

    if(isBlack && realLen >= 6) 
        return OVERLINE;
    else if(realLen >= 5)
        return FIVE;
    else if(fullLen < 5)
        return DEAD;

    int patternCnt[PATTERN_SIZE] = {0};
    Pattern p = DEAD;

    cout << "3.2" << endl;
    cout << "line.dir" << line.dir << endl;
    for(int i = start; i <= end; i++) {
        if(line[i]->piece == EMPTY) {
            Line sl = shiftLine(line, i);
            sl.dir = DIRECTION_SIZE;

            cout << "3.2.1" << endl;
            cout << "i: " << i << endl;

            Pattern slp = setPattern(sl);

            cout << "3.2.2" << endl;
            if(line.dir != DIRECTION_SIZE) {
                Piece self = isBlack ? BLACK : WHITE;
                line[i]->patterns[self][line.dir] = slp;
            }
            patternCnt[slp]++;
        }
    }

    cout << "3.3" << endl;
    if (patternCnt[FIVE] >= 2) {
        p = FREE_4;
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


