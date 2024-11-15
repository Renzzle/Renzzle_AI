#pragma once

#include "../game/board.h"
#include <vector>
#include <algorithm>
#include <tuple>
 
#define MAX_VALUE 50000
#define MIN_VALUE -50000
#define INITIAL_VALUE -99999

// for sort positions value
using Score = int;
// evaluate value
using Value = int;

class Evaluator {

PRIVATE
    Piece self = BLACK;
    Piece oppo = WHITE;

    MoveList patternMap[2][COMPOSITE_PATTERN_SIZE];

    void init();
    void classify(Board& board);
    
PUBLIC
    MoveList getCandidates(Board& board);
    MoveList getSureMove(Board& board);
    MoveList getFours(Board& board);
    MoveList getThreats(Board& board);
    MoveList getThreatDefend(Board& board);
    bool isQuiescence(Board& board, Piece piece);
    Value evaluate(Board& board);

}; 

void Evaluator::init() {
    for (int i = 0; i < COMPOSITE_PATTERN_SIZE; i++) {
        patternMap[0][i].clear();
        patternMap[1][i].clear();
    }
    return;
}

void Evaluator::classify(Board& board) {
    init();
    if (board.getResult() != ONGOING) return;

    self = board.isBlackTurn() ? BLACK : WHITE;
    oppo = !board.isBlackTurn() ? BLACK : WHITE;

    for (int i = 1; i <= BOARD_SIZE; i++) {
        for (int j = 1; j <= BOARD_SIZE; j++) {
            Pos p = Pos(i, j);
            Cell& c = board.getCell(p);
            if (c.getPiece() == EMPTY) {
                patternMap[BLACK][c.getCompositePattern(BLACK)].push_back(p);
                patternMap[WHITE][c.getCompositePattern(WHITE)].push_back(p);
            }
        }
    }
}

MoveList Evaluator::getCandidates(Board& board) {
    classify(board);

    MoveList result;
    if (!patternMap[self][WINNING].empty()) {
        result.push_back(patternMap[self][WINNING].front());
        return result;
    }
    if (!patternMap[oppo][WINNING].empty()) {
        result.push_back(patternMap[oppo][WINNING].front());
        return result;
    }
    if (!patternMap[self][MATE].empty()) {
        result.push_back(patternMap[self][MATE].front());
        return result;
    }
    
    result.insert(result.end(), patternMap[self][B4_F3].begin(), patternMap[self][B4_F3].end());
    result.insert(result.end(), patternMap[oppo][MATE].begin(), patternMap[oppo][MATE].end());
    result.insert(result.end(), patternMap[self][F3_2X].begin(), patternMap[self][F3_2X].end());
    result.insert(result.end(), patternMap[self][B4_PLUS].begin(), patternMap[self][B4_PLUS].end());
    result.insert(result.end(), patternMap[self][B4_ANY].begin(), patternMap[self][B4_ANY].end());
    result.insert(result.end(), patternMap[self][F3_PLUS].begin(), patternMap[self][F3_PLUS].end());
    result.insert(result.end(), patternMap[self][F3_ANY].begin(), patternMap[self][F3_ANY].end());
    result.insert(result.end(), patternMap[self][B3_PLUS].begin(), patternMap[self][B3_PLUS].end());
    result.insert(result.end(), patternMap[self][F2_2X].begin(), patternMap[self][F2_2X].end());
    result.insert(result.end(), patternMap[self][B3_ANY].begin(), patternMap[self][B3_ANY].end());
    result.insert(result.end(), patternMap[self][F2_ANY].begin(), patternMap[self][F2_ANY].end());
    sort(result.begin(), result.end(), [&](const Pos& a, const Pos& b) {
        return board.getCell(a).getScore(self) > board.getCell(b).getScore(self);
    });

    return result;
}

MoveList Evaluator::getSureMove(Board& board) {
    classify(board);

    MoveList result;
    if (!patternMap[self][WINNING].empty()) {
        result.push_back(patternMap[self][WINNING].front());
        return result;
    }
    if (!patternMap[oppo][WINNING].empty()) {
        result.push_back(patternMap[oppo][WINNING].front());
        return result;
    }
    if (!patternMap[self][MATE].empty()) {
        result.push_back(patternMap[self][MATE].front());
        return result;
    }

    return result;
}

MoveList Evaluator::getFours(Board& board) {
    classify(board);

    MoveList result;
    if (!patternMap[self][WINNING].empty()) {
        result.push_back(patternMap[self][WINNING].front()); 
        return result;
    }
    if (!patternMap[self][MATE].empty()) {
        result.push_back(patternMap[self][MATE].front());
        return result;
    }
    if (!patternMap[self][B4_F3].empty()) {
        result.insert(result.end(), patternMap[self][B4_F3].begin(), patternMap[self][B4_F3].end());
    }
    if (!patternMap[self][B4_PLUS].empty()) {
        result.insert(result.end(), patternMap[self][B4_PLUS].begin(), patternMap[self][B4_PLUS].end());
    }
    if (!patternMap[self][B4_ANY].empty()) {
        result.insert(result.end(), patternMap[self][B4_ANY].begin(), patternMap[self][B4_ANY].end());
    }
    sort(result.begin(), result.end(), [&](const Pos& a, const Pos& b) {
        return board.getCell(a).getScore(self) > board.getCell(b).getScore(self);
    });

    return result;
}

MoveList Evaluator::getThreats(Board& board) {
    classify(board);

    MoveList result;
    if (!patternMap[self][WINNING].empty()) {
        result.push_back(patternMap[self][WINNING].front()); 
        return result;
    }
    if (!patternMap[self][MATE].empty()) {
        result.push_back(patternMap[self][MATE].front());
        return result;
    }
    if (!patternMap[self][B4_F3].empty()) {
        result.insert(result.end(), patternMap[self][B4_F3].begin(), patternMap[self][B4_F3].end());
    }
    if (!patternMap[self][B4_PLUS].empty()) {
        result.insert(result.end(), patternMap[self][B4_PLUS].begin(), patternMap[self][B4_PLUS].end());
    }
    if (!patternMap[self][B4_ANY].empty()) {
        result.insert(result.end(), patternMap[self][B4_ANY].begin(), patternMap[self][B4_ANY].end());
    }
    if (!patternMap[self][F3_2X].empty()) {
        result.insert(result.end(), patternMap[self][B4_F3].begin(), patternMap[self][B4_F3].end());
    }
    if (!patternMap[self][F3_PLUS].empty()) {
        result.insert(result.end(), patternMap[self][B4_PLUS].begin(), patternMap[self][B4_PLUS].end());
    }
    if (!patternMap[self][F3_ANY].empty()) {
        result.insert(result.end(), patternMap[self][B4_ANY].begin(), patternMap[self][B4_ANY].end());
    }
    sort(result.begin(), result.end(), [&](const Pos& a, const Pos& b) {
        return board.getCell(a).getScore(self) > board.getCell(b).getScore(self);
    });

    return result;
}

MoveList Evaluator::getThreatDefend(Board& board) {
    classify(board);

    MoveList result;
    if (!patternMap[oppo][WINNING].empty()) {
        result.push_back(patternMap[oppo][WINNING].front());
        return result;
    }
 
    result.insert(result.end(), patternMap[oppo][MATE].begin(), patternMap[oppo][MATE].end());

    // check every mate move (free 3)
    for (auto p : patternMap[oppo][MATE]) {
        // check which direction has free 3
        for (Direction dir = DIRECTION_START; dir < DIRECTION_SIZE; dir++) {
            Cell& c = board.getCell(p);
            if (c.getPiece() == EMPTY && c.getPattern(oppo, dir) == FREE_4) {
                p.setDirection(dir);
                MoveList defendB4;
                int f4Cnt = 0;
                // check the number of free 4 move and if 1, blocked 4 move also can defend
                for (int i = 0; i < LINE_LENGTH; i++) {
                    if (!(p + (i - (LINE_LENGTH / 2)))) continue;

                    if (board.getCell(p).getPattern(oppo, dir) == BLOCKED_4)
                        defendB4.push_back(p);
                    else if (board.getCell(p).getPattern(oppo, dir) == FREE_4)
                        f4Cnt++;

                    p - (i - (LINE_LENGTH / 2));
                }

                if (f4Cnt == 1) {
                    result.insert(result.end(), defendB4.begin(), defendB4.end());
                } 
            }
        }
    }
    
    return result;
}

bool Evaluator::isQuiescence(Board& board, Piece piece) {
    classify(board);

    if (!patternMap[piece][WINNING].empty()) return false;
    if (!patternMap[piece][MATE].empty()) return false;
    if (!patternMap[piece][B4_F3].empty()) return false;
    if (!patternMap[piece][B4_PLUS].empty()) return false;
    if (!patternMap[piece][F3_2X].empty()) return false;
    if (!patternMap[piece][F3_PLUS].empty()) return false;

    Piece oppoPiece = (piece == BLACK) ? WHITE : BLACK;
    if (!patternMap[oppoPiece][WINNING].empty()) return true;
    if (!patternMap[oppoPiece][MATE].empty()) return true;

    return true;
}

Value Evaluator::evaluate(Board& board) {
    classify(board);

    // case 1: finish
    Result result = board.getResult();
    if (result != ONGOING) {
        if (result == DRAW) return 0;
        if (self == BLACK && result == BLACK_WIN)
            return MAX_VALUE;
        if (self == BLACK && result == WHITE_WIN)
            return MIN_VALUE;
        if (self == WHITE && result == BLACK_WIN)
            return MIN_VALUE;
        if (self == WHITE && result == WHITE_WIN)
            return MAX_VALUE;
    }
    
    // case 2: there is sure winning path
    // 1 step before win
    if (!patternMap[self][WINNING].empty()) {
        return MAX_VALUE - 1;
    }
    // 1 step before lose
    if (!patternMap[oppo][WINNING].size() > 1) {
        return MIN_VALUE + 1;
    }
    // 3 step before win
    if (!patternMap[self][MATE].empty()) {
        return MAX_VALUE - 3;
    }

    Value val = 0;
    val += patternMap[self][WINNING].size() * 100;
    val += patternMap[self][MATE].size() * 100;

    val += patternMap[self][B4_F3].size() * 20;
    val += patternMap[self][B4_PLUS].size() * 20;
    val += patternMap[self][F3_2X].size() * 20;
    val += patternMap[self][F3_PLUS].size() * 20;

    val += patternMap[self][B4_ANY].size() * 13;
    val += patternMap[self][F3_ANY].size() * 12;
    val += patternMap[self][B3_PLUS].size() * 10;
    val += patternMap[self][F2_2X].size() * 9;
    val += patternMap[self][B3_ANY].size() * 5;
    val += patternMap[self][F2_ANY].size() * 4;

    val -= patternMap[oppo][WINNING].size() * 100;
    val -= patternMap[oppo][MATE].size() * 100;

    val -= patternMap[oppo][B4_F3].size() * 20;
    val -= patternMap[oppo][B4_PLUS].size() * 20;
    val -= patternMap[oppo][B4_ANY].size() * 20;
    val -= patternMap[oppo][F3_2X].size() * 20;
    val -= patternMap[oppo][F3_PLUS].size() * 20;
    val -= patternMap[oppo][F3_ANY].size() * 20;

    val -= patternMap[oppo][B3_PLUS].size() * 8;
    val -= patternMap[oppo][F2_2X].size() * 7;
    val -= patternMap[oppo][B3_ANY].size() * 3;
    val -= patternMap[oppo][F2_ANY].size() * 2;

    return val;
}