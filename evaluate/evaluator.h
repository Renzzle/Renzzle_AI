#pragma once

#include "../game/board.h"
#include <vector>
#include <algorithm>
#include <tuple>
 
#define MAX_VALUE 50000
#define MIN_VALUE -50000
#define INITIAL_VALUE -99999

using Value = int;

class Evaluator {

PRIVATE
    Board& board;
    Piece self = BLACK;
    Piece oppo = WHITE;

    MoveList patternMap[2][COMPOSITE_PATTERN_SIZE];

    void classify();
    
PUBLIC
    Evaluator(Board& board);
    MoveList getCandidates();
    Pos getSureMove();
    MoveList getFours();
    MoveList getThreats();
    MoveList getThreatDefend();
    bool isOppoMateExist();
    bool isMoveForbidden(const Pos& p);
    Value evaluate();

}; 

Evaluator::Evaluator(Board& board) : board(board) {
    classify();
}

void Evaluator::classify() {
    self = board.isBlackTurn() ? BLACK : WHITE;
    oppo = !board.isBlackTurn() ? BLACK : WHITE;

    if (board.getResult() != ONGOING) return;

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

MoveList Evaluator::getCandidates() {
    MoveList result;

    Pos sureMove = getSureMove();
    if (!sureMove.isDefault()) {
        result.push_back(sureMove);
        return result;
    }

    if (isOppoMateExist()) {
        return getThreatDefend();
    }
    
    result.insert(result.end(), patternMap[self][B4_F3].begin(), patternMap[self][B4_F3].end());
    for (const auto& p : patternMap[oppo][MATE]) {
        if (!isMoveForbidden(p)) result.push_back(p);
    }
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

Pos Evaluator::getSureMove() {
    if (!patternMap[self][WINNING].empty()) {
        return patternMap[self][WINNING].front();
    }
    if (!patternMap[oppo][WINNING].empty()) {
        MoveList result;
        for (const auto& p : patternMap[oppo][WINNING]) {
            if (!isMoveForbidden(p)) result.push_back(p);
        }
        
        if (!result.empty()) return result.front();
        
        for (int r = 1; r <= BOARD_SIZE; ++r) {
            for (int c = 1; c <= BOARD_SIZE; ++c) {
                Pos cp(r, c);
                if (board.getCell(cp).getPiece() == EMPTY && !isMoveForbidden(cp)) {
                    return cp;
                }
            }
        }
    }
    if (!patternMap[self][MATE].empty()) {
        return patternMap[self][MATE].front();
    }

    return Pos();
}

MoveList Evaluator::getFours() {
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

    return result;
}

MoveList Evaluator::getThreats() {
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
    if (!patternMap[self][F3_2X].empty()) {
        result.insert(result.end(), patternMap[self][F3_2X].begin(), patternMap[self][F3_2X].end());
    }
    if (!patternMap[self][F3_PLUS].empty()) {
        result.insert(result.end(), patternMap[self][F3_PLUS].begin(), patternMap[self][F3_PLUS].end());
    }
    if (!patternMap[self][F3_ANY].empty()) {
        result.insert(result.end(), patternMap[self][F3_ANY].begin(), patternMap[self][F3_ANY].end());
    }
    if (!patternMap[self][B4_PLUS].empty()) {
        result.insert(result.end(), patternMap[self][B4_PLUS].begin(), patternMap[self][B4_PLUS].end());
    }
    if (!patternMap[self][B4_ANY].empty()) {
        result.insert(result.end(), patternMap[self][B4_ANY].begin(), patternMap[self][B4_ANY].end());
    }

    return result;
}

MoveList Evaluator::getThreatDefend() {
    MoveList result;

    if (!isOppoMateExist()) return result;

    if (!patternMap[oppo][WINNING].empty()) {
        for (const auto& p : patternMap[oppo][WINNING]) {
            if (!isMoveForbidden(p)) result.push_back(p);
        }
    }

    if (!patternMap[oppo][MATE].empty()) {
        for (const auto& p : patternMap[oppo][MATE]) {
            if (!isMoveForbidden(p)) result.push_back(p);
        }
    }

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
                    if (!defendB4.empty()) {
                        for (const auto& p : defendB4) {
                            if (!isMoveForbidden(p)) result.push_back(p);
                        }
                    }
                } 
            }
        }
    }

    if (result.empty()) {
        for (int r = 1; r <= BOARD_SIZE; ++r) {
            for (int c = 1; c <= BOARD_SIZE; ++c) {
                Pos cp(r, c);
                if (board.getCell(cp).getPiece() == EMPTY && !isMoveForbidden(cp)) {
                    result.push_back(cp);
                    return result;
                }
            }
        }
    }
    
    return result;
}

bool Evaluator::isOppoMateExist() {
    if (!patternMap[oppo][WINNING].empty()) return true;
    if (!patternMap[oppo][MATE].empty()) return true;
    return false;
}

bool Evaluator::isMoveForbidden(const Pos& p) {
    if (self == BLACK) {
        return board.isForbidden(p);
    }
    return false;
}

Value Evaluator::evaluate() {
    // case 1: finish
    Result result = board.getResult();
    if (result != ONGOING) {
        if (result == DRAW) return 0;
        
        // black turn & white win = white 5
        if (self == BLACK && result == WHITE_WIN)
            return MIN_VALUE;
        // white turn & black win = black 5
        if (self == WHITE && result == BLACK_WIN)
            return MIN_VALUE;
        // white turn & white win = black forbidden
        if (self == WHITE && result == WHITE_WIN)
            return MAX_VALUE;
    }
    
    // case 2: there is sure winning path
    // 1 step before win
    if (!patternMap[self][WINNING].empty()) {
        return MAX_VALUE - 1;
    }
    // 2 step before lose
    if (patternMap[oppo][WINNING].size() > 1) {
        return MIN_VALUE + 2;
    }
    // 3 step before win
    if (!patternMap[self][MATE].empty() && patternMap[oppo][WINNING].empty()) {
        return MAX_VALUE - 3;
    }

    Value val = 0;
    val += patternMap[self][B4_F3].size() * 150;
    val += patternMap[self][B4_PLUS].size() * 25;
    val += patternMap[self][F3_2X].size() * 35;
    val += patternMap[self][F3_PLUS].size() * 25;

    val += patternMap[self][B4_ANY].size() * 25;
    val += patternMap[self][F3_ANY].size() * 25;
    val += patternMap[self][B3_PLUS].size() * 10;
    val += patternMap[self][F2_2X].size() * 9;
    val += patternMap[self][B3_ANY].size() * 5;
    val += patternMap[self][F2_ANY].size() * 4;

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