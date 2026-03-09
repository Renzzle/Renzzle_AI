#pragma once

#include "../game/board.h"
#include "value.h"
#include <array>
#include <vector>
#include <algorithm>
#include <tuple>

class MoveBucket {

private:
    static constexpr int CAPACITY = BOARD_SIZE * BOARD_SIZE;

    array<uint8_t, CAPACITY> data;
    int count = 0;

    static uint8_t encode(const Pos& p) {
        return static_cast<uint8_t>((p.getX() << 4) | p.getY());
    }

    static Pos decode(uint8_t code) {
        return Pos(code >> 4, code & 0x0F);
    }

public:
    void clear() {
        count = 0;
    }

    void push_back(const Pos& p) {
        data[count++] = encode(p);
    }

    bool empty() const {
        return count == 0;
    }

    int size() const {
        return count;
    }

    Pos front() const {
        return decode(data[0]);
    }

    Pos operator[](int index) const {
        return decode(data[index]);
    }

    void appendTo(MoveList& result) const {
        for (int i = 0; i < count; ++i) {
            result.push_back(decode(data[i]));
        }
    }

};

class Evaluator {

PRIVATE
    Board& board;
    Piece self = BLACK;
    Piece oppo = WHITE;

    MoveBucket patternMap[2][COMPOSITE_PATTERN_SIZE];

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

    for (int color = 0; color < 2; ++color) {
        for (int pattern = 0; pattern < COMPOSITE_PATTERN_SIZE; ++pattern) {
            patternMap[color][pattern].clear();
        }
    }

    if (board.getResult() != ONGOING) return;

    for (int i = 1; i <= BOARD_SIZE; i++) {
        for (int j = 1; j <= BOARD_SIZE; j++) {
            Cell& c = board.getCell(i, j);
            if (c.getPiece() == EMPTY) {
                Pos p(i, j);
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

    result.reserve(
        patternMap[self][B4_F3].size() +
        patternMap[oppo][MATE].size() +
        patternMap[self][F3_2X].size() +
        patternMap[self][B4_PLUS].size() +
        patternMap[self][B4_ANY].size() +
        patternMap[self][F3_PLUS].size() +
        patternMap[self][F3_ANY].size() +
        patternMap[self][B3_PLUS].size() +
        patternMap[self][F2_2X].size() +
        patternMap[self][B3_ANY].size() +
        patternMap[self][F2_ANY].size());
    
    patternMap[self][B4_F3].appendTo(result);
    for (int i = 0; i < patternMap[oppo][MATE].size(); ++i) {
        Pos p = patternMap[oppo][MATE][i];
        if (!isMoveForbidden(p)) result.push_back(p);
    }
    patternMap[self][F3_2X].appendTo(result);
    patternMap[self][B4_PLUS].appendTo(result);
    patternMap[self][B4_ANY].appendTo(result);
    patternMap[self][F3_PLUS].appendTo(result);
    patternMap[self][F3_ANY].appendTo(result);
    patternMap[self][B3_PLUS].appendTo(result);
    patternMap[self][F2_2X].appendTo(result);
    patternMap[self][B3_ANY].appendTo(result);
    patternMap[self][F2_ANY].appendTo(result);
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
        result.reserve(patternMap[oppo][WINNING].size());
        for (int i = 0; i < patternMap[oppo][WINNING].size(); ++i) {
            Pos p = patternMap[oppo][WINNING][i];
            if (!isMoveForbidden(p)) result.push_back(p);
        }
        
        if (!result.empty()) return result.front();
        
        for (int r = 1; r <= BOARD_SIZE; ++r) {
            for (int c = 1; c <= BOARD_SIZE; ++c) {
                if (board.getCell(r, c).getPiece() == EMPTY) {
                    Pos cp(r, c);
                    if (!isMoveForbidden(cp)) {
                        return cp;
                    }
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
    result.reserve(
        patternMap[self][B4_F3].size() +
        patternMap[self][B4_PLUS].size() +
        patternMap[self][B4_ANY].size());
    patternMap[self][B4_F3].appendTo(result);
    patternMap[self][B4_PLUS].appendTo(result);
    patternMap[self][B4_ANY].appendTo(result);

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
    result.reserve(
        patternMap[self][B4_F3].size() +
        patternMap[self][F3_2X].size() +
        patternMap[self][F3_PLUS].size() +
        patternMap[self][F3_ANY].size() +
        patternMap[self][B4_PLUS].size() +
        patternMap[self][B4_ANY].size());
    patternMap[self][B4_F3].appendTo(result);
    patternMap[self][F3_2X].appendTo(result);
    patternMap[self][F3_PLUS].appendTo(result);
    patternMap[self][F3_ANY].appendTo(result);
    patternMap[self][B4_PLUS].appendTo(result);
    patternMap[self][B4_ANY].appendTo(result);

    return result;
}

MoveList Evaluator::getThreatDefend() {
    MoveList result;

    if (!isOppoMateExist()) return result;

    result.reserve(patternMap[oppo][WINNING].size() + patternMap[oppo][MATE].size() * 2);

    if (!patternMap[oppo][WINNING].empty()) {
        for (int i = 0; i < patternMap[oppo][WINNING].size(); ++i) {
            Pos p = patternMap[oppo][WINNING][i];
            if (!isMoveForbidden(p)) result.push_back(p);
        }
    }

    if (!patternMap[oppo][MATE].empty()) {
        for (int i = 0; i < patternMap[oppo][MATE].size(); ++i) {
            Pos p = patternMap[oppo][MATE][i];
            if (!isMoveForbidden(p)) result.push_back(p);
        }
    }

    // check every mate move (free 3)
    for (int i = 0; i < patternMap[oppo][MATE].size(); ++i) {
        Pos p = patternMap[oppo][MATE][i];
        // check which direction has free 3
        for (Direction dir = DIRECTION_START; dir < DIRECTION_SIZE; dir++) {
            Cell& c = board.getCell(p);
            if (c.getPiece() == EMPTY && c.getPattern(oppo, dir) == FREE_4) {
                p.setDirection(dir);
                MoveList defendB4;
                defendB4.reserve(LINE_LENGTH);
                int f4Cnt = 0;
                const int baseX = p.getX();
                const int baseY = p.getY();
                const int dx = getDirectionDx(dir);
                const int dy = getDirectionDy(dir);
                // check the number of free 4 move and if 1, blocked 4 move also can defend
                for (int i = 0; i < LINE_LENGTH; i++) {
                    const int offset = i - (LINE_LENGTH / 2);
                    const int x = baseX + (dx * offset);
                    const int y = baseY + (dy * offset);
                    if (!isBoardCoord(x, y)) continue;

                    Cell& lineCell = board.getCell(x, y);
                    if (lineCell.getPattern(oppo, dir) == BLOCKED_4)
                        defendB4.emplace_back(x, y);
                    else if (lineCell.getPattern(oppo, dir) == FREE_4)
                        f4Cnt++;
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
                if (board.getCell(r, c).getPiece() == EMPTY) {
                    Pos cp(r, c);
                    if (!isMoveForbidden(cp)) {
                        result.push_back(cp);
                        return result;
                    }
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
        if (result == DRAW) return Value(0);
        
        // black turn & white win = white 5
        if (self == BLACK && result == WHITE_WIN)
            return Value(Value::Result::LOSE);
        // white turn & black win = black 5
        if (self == WHITE && result == BLACK_WIN)
            return Value(Value::Result::LOSE);
        // white turn & white win = black forbidden
        if (self == WHITE && result == WHITE_WIN)
            return Value(Value::Result::WIN);
    }
    
    // case 2: there is sure winning path
    // 1 step before win
    if (!patternMap[self][WINNING].empty()) {
        return Value(Value::Result::WIN, 1);
    }
    // 2 step before lose
    if (patternMap[oppo][WINNING].size() > 1) {
        return Value(Value::Result::LOSE, 2);
    }
    // 3 step before win
    if (!patternMap[self][MATE].empty() && patternMap[oppo][WINNING].empty()) {
        return Value(Value::Result::WIN, 3);
    }

    int val = 0;
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

    return Value(val);
}
