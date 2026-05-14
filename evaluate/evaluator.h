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
    bool hasFourLevelPressure(Piece piece) const;
    int patternCount(Piece piece, CompositePattern pattern) const;
    
PUBLIC
    Evaluator(Board& board);
    Value quickWinCheck(Pos* bestMove = nullptr);
    MoveList getCandidates();
    Pos getSureMove();
    MoveList getFours();
    MoveList getThreats();
    MoveList getThreatDefend();
    MoveList getFourThreeMakers();
    MoveList getFourThreeDefend();
    Pos getOppoWinningDefend();
    bool isOppoMateExist();
    bool isOppoFourThreeExist();
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

bool Evaluator::hasFourLevelPressure(Piece piece) const {
    return hasPattern(piece, WINNING)
        || hasPattern(piece, MATE)
        || hasPattern(piece, B4_F3)
        || hasPattern(piece, B4_PLUS)
        || hasPattern(piece, B4_ANY);
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

Value Evaluator::quickWinCheck(Pos* bestMove) {
    if (bestMove != nullptr) {
        *bestMove = Pos();
    }

    if (hasPattern(self, WINNING)) {
        if (bestMove != nullptr) {
            *bestMove = bucket(self, WINNING).front();
        }
        return Value(Value::Result::WIN, 1);
    }

    if (patternCount(oppo, WINNING) > 1) {
        if (bestMove != nullptr) {
            *bestMove = getSureMove();
        }
        return Value(Value::Result::LOSE, 2);
    }

    if (hasPattern(self, MATE) && !hasPattern(oppo, WINNING)) {
        if (bestMove != nullptr) {
            *bestMove = bucket(self, MATE).front();
        }
        return Value(Value::Result::WIN, 3);
    }

    if (hasPattern(self, B4_F3) && !hasPattern(oppo, WINNING)) {
        const Pos p43 = bucket(self, B4_F3).front();
        Cell& cP = board.getCell(p43);

        Pos W;
        for (Direction dir = DIRECTION_START; dir < DIRECTION_SIZE; dir++) {
            if (cP.getPattern(self, dir) != BLOCKED_4) continue;
            const int baseX = p43.getX();
            const int baseY = p43.getY();
            const int dx = getDirectionDx(dir);
            const int dy = getDirectionDy(dir);
            for (int offset = -4; offset <= 4; offset++) {
                if (offset == 0) continue;
                const int x = baseX + (dx * offset);
                const int y = baseY + (dy * offset);
                if (!isBoardCoord(x, y)) continue;
                Cell& lineCell = board.getCell(x, y);
                if (lineCell.getPiece() != EMPTY) continue;
                if (lineCell.getPattern(self, dir) == BLOCKED_4) {
                    W = Pos(x, y);
                    break;
                }
            }
            if (!W.isDefault()) break;
        }

        bool oppoCanInterceptAtW = false;
        if (!W.isDefault()) {
            const CompositePattern wOppoComp = board.getCell(W).getCompositePattern(oppo);
            oppoCanInterceptAtW =
                (wOppoComp == WINNING || wOppoComp == MATE ||
                 wOppoComp == B4_F3 || wOppoComp == B4_PLUS || wOppoComp == B4_ANY);
        }

        if (!oppoCanInterceptAtW) {
            if (bestMove != nullptr) {
                *bestMove = p43;
            }
            return Value(Value::Result::WIN, 5);
        }
    }

    if (hasPattern(self, F3_2X) && !hasFourLevelPressure(oppo)) {
        if (bestMove != nullptr) {
            *bestMove = bucket(self, F3_2X).front();
        }
        return Value(Value::Result::WIN, 5);
    }

    return Value();
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
        return board.getCell(a).getScore(oppo) > board.getCell(b).getScore(oppo);
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

    // sort omitted — Search::sortChildNodes handles ordering uniformly (cellScore + TT/history)
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
            if (c.getPiece() == EMPTY && (c.getPattern(oppo, dir) == FREE_4 || c.getPattern(oppo, dir) == BLOCKED_4)) {
                p.setDirection(dir);
                FixedMoveList<LINE_LENGTH> defendB4;
                defendB4.reserve(LINE_LENGTH);
                int f4Cnt = 0;
                const int baseX = p.getX();
                const int baseY = p.getY();
                const int dx = getDirectionDx(dir);
                const int dy = getDirectionDy(dir);
                constexpr int SCAN_RADIUS = 4;  // ±4 cells around mate spot
                // check the number of free 4 move and if 1, blocked 4 move also can defend
                for (int offset = -SCAN_RADIUS; offset <= SCAN_RADIUS; offset++) {
                    const int x = baseX + (dx * offset);
                    const int y = baseY + (dy * offset);
                    if (!isBoardCoord(x, y)) continue;

                    Cell& lineCell = board.getCell(x, y);
                    if (lineCell.getPattern(oppo, dir) == BLOCKED_4)
                        defendB4.emplace_back(x, y);
                    else if (lineCell.getPattern(oppo, dir) == FREE_4)
                        f4Cnt++;
                }

                if (f4Cnt == 1 || f4Cnt == 0) {
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

    // sort omitted — Search::sortChildNodes handles ordering uniformly (cellScore + TT/history)
    return result.toMoveList();
}

MoveList Evaluator::getFourThreeMakers() {
    FixedMoveList<BOARD_SIZE * BOARD_SIZE> result;
    array<uint8_t, 256> seen = {};

    auto appendUniqueLegal = [&](const Pos& p) {
        if (isMoveForbidden(p)) return;
        const uint8_t key = static_cast<uint8_t>((p.getX() << 4) | p.getY());
        if (seen[key]) return;
        seen[key] = 1;
        result.push_back(p);
    };

    // qualifying "extra" pattern of B4_PLUS = (FREE_2 family), plus BLOCKED_3 when self is white
    // (for white, B3→F3 transition is direct since white has no 3-3 forbidden rule)
    auto isFourThreeSeed = [&](Pattern pat) {
        if (pat == FREE_2 || pat == FREE_2A || pat == FREE_2B) return true;
        if (self == WHITE && pat == BLOCKED_3) return true;
        return false;
    };

    bucket(self, B4_PLUS).forEach([&](const Pos& center) {
        for (Direction dir = DIRECTION_START; dir < DIRECTION_SIZE; dir++) {
            Cell& c = board.getCell(center);
            if (c.getPiece() != EMPTY) continue;
            if (!isFourThreeSeed(c.getPattern(self, dir))) continue;

            // Center has the F2 (or B3 for white) "extra" in this direction.
            // Scan the line through `center` in this direction; every empty cell whose
            // pattern in this direction is also F2-family (B3 for white) is a follow-up
            // candidate that can extend the line toward F3 — i.e., a 4-3 maker.
            const int baseX = center.getX();
            const int baseY = center.getY();
            const int dx = getDirectionDx(dir);
            const int dy = getDirectionDy(dir);
            constexpr int SCAN_RADIUS = 3;  // ±3 cells around center → 6 non-center cells per direction
            for (int offset = -SCAN_RADIUS; offset <= SCAN_RADIUS; offset++) {
                const int x = baseX + (dx * offset);
                const int y = baseY + (dy * offset);
                if (!isBoardCoord(x, y)) continue;

                Cell& lineCell = board.getCell(x, y);
                if (lineCell.getPiece() != EMPTY) continue;
                if (!isFourThreeSeed(lineCell.getPattern(self, dir))) continue;
                if (lineCell.getCompositePattern(self) == B4_PLUS) continue;  // already a B4_PLUS spot — handled elsewhere

                appendUniqueLegal(Pos(x, y));
            }
        }
    });

    // sort omitted — Search::sortChildNodes handles ordering uniformly (cellScore + TT/history)
    return result.toMoveList();
}

MoveList Evaluator::getFourThreeDefend() {
    FixedMoveList<BOARD_SIZE * BOARD_SIZE> result;
    array<uint8_t, 256> seen = {};

    if (!isOppoFourThreeExist()) return result.toMoveList();

    auto appendUniqueLegal = [&](const Pos& p) {
        if (isMoveForbidden(p)) return;
        const uint8_t key = static_cast<uint8_t>((p.getX() << 4) | p.getY());
        if (seen[key]) return;
        seen[key] = 1;
        result.push_back(p);
    };

    FixedMoveList<BOARD_SIZE * BOARD_SIZE> raw;

    bucket(oppo, B4_F3).forEach([&](const Pos& p) {
        if (oppo == BLACK && board.isForbidden(p)) return;

        appendUniqueLegal(p);

        // simulate opponent playing at p: pass our turn first so move() places opponent's piece
        if (!board.pass()) return;
        if (!board.move(p)) {
            board.undo();  // undo the pass
            return;
        }

        raw.clear();
        board.getPatternBucket(oppo, WINNING).forEach([&](const Pos& q) {
            raw.push_back(q);

            // if q makes self a three, we'd play q ourselves: denies opponent's 5 AND opens a
            // VCF-style counter (our F3 forces a defense). Simulate self playing q, then collect
            // self's four/mate/winning cells in THAT state — those are the counter-attack options.
            const CompositePattern selfPat = board.getCell(q).getCompositePattern(self);
            const bool qMakesSelfThree =
                selfPat == F3_ANY || selfPat == F3_PLUS || selfPat == F3_2X || selfPat == B3_ANY || selfPat == B3_PLUS;
            if (!qMakesSelfThree) return;
            if (isMoveForbidden(q)) return;
            if (!board.move(q)) return;

            board.getPatternBucket(self, WINNING).forEach([&](const Pos& f) { raw.push_back(f); });
            board.getPatternBucket(self, MATE).forEach([&](const Pos& f) { raw.push_back(f); });
            board.getPatternBucket(self, B4_F3).forEach([&](const Pos& f) { raw.push_back(f); });
            board.getPatternBucket(self, B4_PLUS).forEach([&](const Pos& f) { raw.push_back(f); });
            board.getPatternBucket(self, B4_ANY).forEach([&](const Pos& f) { raw.push_back(f); });

            board.undo();
        });

        int mateCount = 0;
        board.getPatternBucket(oppo, MATE).forEach([&](const Pos& q) {
            raw.push_back(q);
            mateCount++;
        });
        if (mateCount == 1) {
            // single mate — widen defense net to all of opponent's blocked-four cells
            board.getPatternBucket(oppo, B4_F3).forEach([&](const Pos& q) { raw.push_back(q); });
            board.getPatternBucket(oppo, B4_PLUS).forEach([&](const Pos& q) { raw.push_back(q); });
            board.getPatternBucket(oppo, B4_ANY).forEach([&](const Pos& q) { raw.push_back(q); });
        }

        board.undo();
        board.undo();

        for (size_t i = 0; i < raw.size(); ++i) {
            appendUniqueLegal(raw[i]);
        }
    });

    // sort omitted — Search::sortChildNodes handles ordering uniformly (cellScore + TT/history)
    return result.toMoveList();
}

Pos Evaluator::getOppoWinningDefend() {
    if (patternCount(oppo, WINNING) != 1) {
        return Pos();
    }

    const Pos p = bucket(oppo, WINNING).front();
    if (isMoveForbidden(p)) {
        return Pos();
    }

    return p;
}

bool Evaluator::isOppoMateExist() {
    if (hasPattern(oppo, WINNING)) return true;
    if (hasPattern(oppo, MATE)) return true;
    return false;
}

bool Evaluator::isOppoFourThreeExist() {
    return hasPattern(oppo, B4_F3);
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
