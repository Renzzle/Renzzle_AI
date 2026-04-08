#pragma once

#include "../game/board.h"
#include "../game/fixed_move_list.h"
#include "value.h"
#include <vector>
#include <algorithm>
#include <tuple>

class Evaluator {

PRIVATE
    Board& board;
    Piece self = BLACK;
    Piece oppo = WHITE;

    const MoveBucket* patternMap[2][COMPOSITE_PATTERN_SIZE] = {};

    void classify();
    int evaluatePatternBalance();
    const MoveBucket& bucket(Piece piece, CompositePattern pattern) const;
    bool hasPattern(Piece piece, CompositePattern pattern) const;
    int patternCount(Piece piece, CompositePattern pattern) const;
    
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
    Value evaluateTactical();

}; 

inline Value evaluateTacticalSummary(Board& board) {
    Piece self = board.isBlackTurn() ? BLACK : WHITE;
    Piece oppo = !board.isBlackTurn() ? BLACK : WHITE;

    Result result = board.getResult();
    if (result != ONGOING) {
        if (result == DRAW) return Value(0);

        if (self == BLACK && result == WHITE_WIN)
            return Value(Value::Result::LOSE);
        if (self == WHITE && result == BLACK_WIN)
            return Value(Value::Result::LOSE);
        if (self == WHITE && result == WHITE_WIN)
            return Value(Value::Result::WIN);
    }

    if (board.getCompositePatternCount(self, WINNING) > 0) {
        return Value(Value::Result::WIN, 1);
    }
    if (board.getCompositePatternCount(oppo, WINNING) > 1) {
        return Value(Value::Result::LOSE, 2);
    }
    if (board.getCompositePatternCount(self, MATE) > 0 &&
        board.getCompositePatternCount(oppo, WINNING) == 0) {
        return Value(Value::Result::WIN, 3);
    }

    int val = 0;
    val += board.getCompositePatternCount(self, F2_ANY) * 4;
    val += board.getCompositePatternCount(self, B3_ANY) * 5;
    val += board.getCompositePatternCount(self, F2_2X) * 9;
    val += board.getCompositePatternCount(self, B3_PLUS) * 10;
    val += board.getCompositePatternCount(self, F3_ANY) * 25;
    val += board.getCompositePatternCount(self, F3_PLUS) * 25;
    val += board.getCompositePatternCount(self, F3_2X) * 35;
    val += board.getCompositePatternCount(self, B4_ANY) * 25;
    val += board.getCompositePatternCount(self, B4_PLUS) * 25;
    val += board.getCompositePatternCount(self, B4_F3) * 150;

    val -= board.getCompositePatternCount(oppo, F2_ANY) * 2;
    val -= board.getCompositePatternCount(oppo, B3_ANY) * 3;
    val -= board.getCompositePatternCount(oppo, F2_2X) * 7;
    val -= board.getCompositePatternCount(oppo, B3_PLUS) * 8;
    val -= board.getCompositePatternCount(oppo, F3_ANY) * 20;
    val -= board.getCompositePatternCount(oppo, F3_PLUS) * 20;
    val -= board.getCompositePatternCount(oppo, F3_2X) * 20;
    val -= board.getCompositePatternCount(oppo, B4_ANY) * 20;
    val -= board.getCompositePatternCount(oppo, B4_PLUS) * 20;

    return Value(val);
}

Evaluator::Evaluator(Board& board) : board(board) {
    classify();
}

const MoveBucket& Evaluator::bucket(Piece piece, CompositePattern pattern) const {
    return *patternMap[piece][pattern];
}

bool Evaluator::hasPattern(Piece piece, CompositePattern pattern) const {
    return !bucket(piece, pattern).empty();
}

int Evaluator::patternCount(Piece piece, CompositePattern pattern) const {
    return bucket(piece, pattern).size();
}

void Evaluator::classify() {
    self = board.isBlackTurn() ? BLACK : WHITE;
    oppo = !board.isBlackTurn() ? BLACK : WHITE;

    for (int color = 0; color < 2; ++color) {
        for (int pattern = 0; pattern < COMPOSITE_PATTERN_SIZE; ++pattern) {
            patternMap[color][pattern] =
                &board.getPatternBucket(static_cast<Piece>(color), static_cast<CompositePattern>(pattern));
        }
    }
}

int Evaluator::evaluatePatternBalance() {
    int val = 0;
    val += patternCount(self, B4_F3) * 150;
    val += patternCount(self, B4_PLUS) * 25;
    val += patternCount(self, F3_2X) * 35;
    val += patternCount(self, F3_PLUS) * 25;

    val += patternCount(self, B4_ANY) * 25;
    val += patternCount(self, F3_ANY) * 25;
    val += patternCount(self, B3_PLUS) * 10;
    val += patternCount(self, F2_2X) * 9;
    val += patternCount(self, B3_ANY) * 5;
    val += patternCount(self, F2_ANY) * 4;

    val -= patternCount(oppo, B4_PLUS) * 20;
    val -= patternCount(oppo, B4_ANY) * 20;
    val -= patternCount(oppo, F3_2X) * 20;
    val -= patternCount(oppo, F3_PLUS) * 20;
    val -= patternCount(oppo, F3_ANY) * 20;

    val -= patternCount(oppo, B3_PLUS) * 8;
    val -= patternCount(oppo, F2_2X) * 7;
    val -= patternCount(oppo, B3_ANY) * 3;
    val -= patternCount(oppo, F2_ANY) * 2;

    return val;
}

MoveList Evaluator::getCandidates() {
    FixedMoveList<BOARD_SIZE * BOARD_SIZE> result;

    Pos sureMove = getSureMove();
    if (!sureMove.isDefault()) {
        result.push_back(sureMove);
        return result.toMoveList();
    }

    if (isOppoMateExist()) {
        return getThreatDefend();
    }

    result.reserve(
        patternCount(self, B4_F3) +
        patternCount(oppo, MATE) +
        patternCount(self, F3_2X) +
        patternCount(self, B4_PLUS) +
        patternCount(self, B4_ANY) +
        patternCount(self, F3_PLUS) +
        patternCount(self, F3_ANY) +
        patternCount(self, B3_PLUS) +
        patternCount(self, F2_2X) +
        patternCount(self, B3_ANY) +
        patternCount(self, F2_ANY));
    
    bucket(self, B4_F3).forEach([&](const Pos& p) { result.push_back(p); });
    bucket(oppo, MATE).forEach([&](const Pos& p) {
        if (!isMoveForbidden(p)) result.push_back(p);
    });
    bucket(self, F3_2X).forEach([&](const Pos& p) { result.push_back(p); });
    bucket(self, B4_PLUS).forEach([&](const Pos& p) { result.push_back(p); });
    bucket(self, B4_ANY).forEach([&](const Pos& p) { result.push_back(p); });
    bucket(self, F3_PLUS).forEach([&](const Pos& p) { result.push_back(p); });
    bucket(self, F3_ANY).forEach([&](const Pos& p) { result.push_back(p); });
    bucket(self, B3_PLUS).forEach([&](const Pos& p) { result.push_back(p); });
    bucket(self, F2_2X).forEach([&](const Pos& p) { result.push_back(p); });
    bucket(self, B3_ANY).forEach([&](const Pos& p) { result.push_back(p); });
    bucket(self, F2_ANY).forEach([&](const Pos& p) { result.push_back(p); });
    sort(result.begin(), result.end(), [&](const Pos& a, const Pos& b) {
        return board.getCell(a).getScore(self) > board.getCell(b).getScore(self);
    });

    return result.toMoveList();
}

Pos Evaluator::getSureMove() {
    if (board.hasCompositePattern(self, WINNING)) {
        return board.getFirstPatternPos(self, WINNING);
    }
    if (board.hasCompositePattern(oppo, WINNING)) {
        MoveList result;
        result.reserve(board.getCompositePatternCount(oppo, WINNING));
        bucket(oppo, WINNING).forEach([&](const Pos& p) {
            if (!isMoveForbidden(p)) result.push_back(p);
        });

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
    if (board.hasCompositePattern(self, MATE)) {
        return board.getFirstPatternPos(self, MATE);
    }

    return Pos();
}

MoveList Evaluator::getFours() {
    FixedMoveList<BOARD_SIZE * BOARD_SIZE> result;
    if (hasPattern(self, WINNING)) {
        result.push_back(bucket(self, WINNING).front()); 
        return result.toMoveList();
    }
    if (hasPattern(self, MATE)) {
        result.push_back(bucket(self, MATE).front());
        return result.toMoveList();
    }
    result.reserve(
        patternCount(self, B4_F3) +
        patternCount(self, B4_PLUS) +
        patternCount(self, B4_ANY));
    bucket(self, B4_F3).forEach([&](const Pos& p) { result.push_back(p); });
    bucket(self, B4_PLUS).forEach([&](const Pos& p) { result.push_back(p); });
    bucket(self, B4_ANY).forEach([&](const Pos& p) { result.push_back(p); });

    stable_sort(result.begin(), result.end(), [&](const Pos& a, const Pos& b) {
        return board.getCell(a).getScore(self) > board.getCell(b).getScore(self);
    });

    return result.toMoveList();
}

MoveList Evaluator::getThreats() {
    FixedMoveList<BOARD_SIZE * BOARD_SIZE> result;
    if (hasPattern(self, WINNING)) {
        result.push_back(bucket(self, WINNING).front()); 
        return result.toMoveList();
    }
    if (hasPattern(self, MATE)) {
        result.push_back(bucket(self, MATE).front());
        return result.toMoveList();
    }
    result.reserve(
        patternCount(self, B4_F3) +
        patternCount(self, F3_2X) +
        patternCount(self, F3_PLUS) +
        patternCount(self, F3_ANY) +
        patternCount(self, B4_PLUS) +
        patternCount(self, B4_ANY));
    bucket(self, B4_F3).forEach([&](const Pos& p) { result.push_back(p); });
    bucket(self, F3_2X).forEach([&](const Pos& p) { result.push_back(p); });
    bucket(self, F3_PLUS).forEach([&](const Pos& p) { result.push_back(p); });
    bucket(self, F3_ANY).forEach([&](const Pos& p) { result.push_back(p); });
    bucket(self, B4_PLUS).forEach([&](const Pos& p) { result.push_back(p); });
    bucket(self, B4_ANY).forEach([&](const Pos& p) { result.push_back(p); });

    stable_sort(result.begin(), result.end(), [&](const Pos& a, const Pos& b) {
        return board.getCell(a).getScore(self) > board.getCell(b).getScore(self);
    });

    return result.toMoveList();
}

MoveList Evaluator::getThreatDefend() {
    FixedMoveList<BOARD_SIZE * BOARD_SIZE> result;
    array<uint8_t, 256> seen = {};
    auto appendUniqueLegal = [&](const Pos& p) {
        if (isMoveForbidden(p)) return;
        const uint8_t key = static_cast<uint8_t>((p.getX() << 4) | p.getY());
        if (seen[key]) return;
        seen[key] = 1;
        result.push_back(p);
    };

    if (!isOppoMateExist()) return result.toMoveList();

    if (hasPattern(oppo, WINNING)) {
        result.reserve(patternCount(oppo, WINNING));
        bucket(oppo, WINNING).forEach([&](const Pos& p) {
            appendUniqueLegal(p);
        });
        if (!result.empty()) {
            return result.toMoveList();
        }
    }

    result.reserve(patternCount(oppo, MATE) * 2);

    if (hasPattern(oppo, MATE)) {
        bucket(oppo, MATE).forEach([&](const Pos& p) {
            appendUniqueLegal(p);
        });
    }

    if (patternCount(oppo, MATE) == 1) {
        Pos p = bucket(oppo, MATE).front();
        // check which direction has free 3
        for (Direction dir = DIRECTION_START; dir < DIRECTION_SIZE; dir++) {
            Cell& c = board.getCell(p);
            if (c.getPiece() == EMPTY && c.getPattern(oppo, dir) == FREE_4) {
                p.setDirection(dir);
                FixedMoveList<LINE_LENGTH> defendB4;
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
                            appendUniqueLegal(p);
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
                        return result.toMoveList();
                    }
                }
            }
        }
    }

    return result.toMoveList();
}

bool Evaluator::isOppoMateExist() {
    if (hasPattern(oppo, WINNING)) return true;
    if (hasPattern(oppo, MATE)) return true;
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
    if (hasPattern(self, WINNING)) {
        return Value(Value::Result::WIN, 1);
    }
    // 2 step before lose
    if (patternCount(oppo, WINNING) > 1) {
        return Value(Value::Result::LOSE, 2);
    }
    // 3 step before win
    if (hasPattern(self, MATE) && !hasPattern(oppo, WINNING)) {
        return Value(Value::Result::WIN, 3);
    }

    return Value(evaluatePatternBalance());
}

Value Evaluator::evaluateTactical() {
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
    if (hasPattern(self, WINNING)) {
        return Value(Value::Result::WIN, 1);
    }
    // 2 step before lose
    if (patternCount(oppo, WINNING) > 1) {
        return Value(Value::Result::LOSE, 2);
    }
    // 3 step before win
    if (hasPattern(self, MATE) && !hasPattern(oppo, WINNING)) {
        return Value(Value::Result::WIN, 3);
    }

    int val = 0;
    // Self tactical pressure
    val += patternCount(self, NOT_EMPTY) * 0;
    val += patternCount(self, ETC) * 0;
    val += patternCount(self, FORBID) * 0;
    val += patternCount(self, FORBID_33) * 0;
    val += patternCount(self, F2_ANY) * 4;
    val += patternCount(self, B3_ANY) * 5;
    val += patternCount(self, F2_2X) * 9;
    val += patternCount(self, B3_PLUS) * 10;
    val += patternCount(self, F3_ANY) * 25;
    val += patternCount(self, F3_PLUS) * 25;
    val += patternCount(self, F3_2X) * 35;
    val += patternCount(self, B4_ANY) * 25;
    val += patternCount(self, B4_PLUS) * 25;
    val += patternCount(self, B4_F3) * 150;
    val += patternCount(self, MATE) * 0;
    val += patternCount(self, WINNING) * 0;

    // Opponent tactical pressure
    val -= patternCount(oppo, NOT_EMPTY) * 0;
    val -= patternCount(oppo, ETC) * 0;
    val -= patternCount(oppo, FORBID) * 0;
    val -= patternCount(oppo, FORBID_33) * 0;
    val -= patternCount(oppo, F2_ANY) * 2;
    val -= patternCount(oppo, B3_ANY) * 3;
    val -= patternCount(oppo, F2_2X) * 7;
    val -= patternCount(oppo, B3_PLUS) * 8;
    val -= patternCount(oppo, F3_ANY) * 20;
    val -= patternCount(oppo, F3_PLUS) * 20;
    val -= patternCount(oppo, F3_2X) * 20;
    val -= patternCount(oppo, B4_ANY) * 20;
    val -= patternCount(oppo, B4_PLUS) * 20;
    val -= patternCount(oppo, B4_F3) * 0;
    val -= patternCount(oppo, MATE) * 0;
    val -= patternCount(oppo, WINNING) * 0;

    return Value(val);
}
