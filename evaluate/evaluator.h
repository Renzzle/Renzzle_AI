#pragma once

#include "../game/board.h"
#include "value.h"
#include <vector>
#include <algorithm>
#include <tuple>
#include "../test/test.h"

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
    // classify();
    self = board.isBlackTurn() ? BLACK : WHITE;
    oppo = !board.isBlackTurn() ? BLACK : WHITE;
}

// void Evaluator::classify() {
//     self = board.isBlackTurn() ? BLACK : WHITE;
//     oppo = !board.isBlackTurn() ? BLACK : WHITE;

//     if (board.getResult() != ONGOING) return;

//     for (int i = 1; i <= BOARD_SIZE; i++) {
//         for (int j = 1; j <= BOARD_SIZE; j++) {
//             Pos p = Pos(i, j);
//             Cell& c = board.getCell(p);
//             if (c.getPiece() == EMPTY) {
//                 patternMap[BLACK][c.getCompositePattern(BLACK)].push_back(p);
//                 patternMap[WHITE][c.getCompositePattern(WHITE)].push_back(p);
                
//             }
//         }
//     }
// }

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
    
    auto& b4_f3_moves = board.getMovesByPattern(self, B4_F3);
    result.insert(result.end(), b4_f3_moves.begin(), b4_f3_moves.end());
    for (const auto& p : board.getMovesByPattern(oppo, MATE)) {
        if (!isMoveForbidden(p)) result.push_back(p);
    }
    result.insert(result.end(), board.getMovesByPattern(self, F3_2X).begin(), board.getMovesByPattern(self, F3_2X).end());
    result.insert(result.end(), board.getMovesByPattern(self, B4_PLUS).begin(), board.getMovesByPattern(self, B4_PLUS).end());
    result.insert(result.end(), board.getMovesByPattern(self, B4_ANY).begin(), board.getMovesByPattern(self, B4_ANY).end());
    result.insert(result.end(), board.getMovesByPattern(self, F3_PLUS).begin(), board.getMovesByPattern(self, F3_PLUS).end());
    result.insert(result.end(), board.getMovesByPattern(self, F3_ANY).begin(), board.getMovesByPattern(self, F3_ANY).end());
    result.insert(result.end(), board.getMovesByPattern(self, B3_PLUS).begin(), board.getMovesByPattern(self, B3_PLUS).end());
    result.insert(result.end(), board.getMovesByPattern(self, F2_2X).begin(), board.getMovesByPattern(self, F2_2X).end());
    result.insert(result.end(), board.getMovesByPattern(self, B3_ANY).begin(), board.getMovesByPattern(self, B3_ANY).end());
    result.insert(result.end(), board.getMovesByPattern(self, F2_ANY).begin(), board.getMovesByPattern(self, F2_ANY).end());
    sort(result.begin(), result.end(), [&](const Pos& a, const Pos& b) {
        return board.getCell(a).getScore(self) > board.getCell(b).getScore(self);
    });

    return result;
}

Pos Evaluator::getSureMove() {
    if (!board.getMovesByPattern(self, WINNING).empty()) {
        return *board.getMovesByPattern(self, WINNING).begin();
    }
    if (!board.getMovesByPattern(oppo, WINNING).empty()) {
        MoveList result;
        for (const auto& p : board.getMovesByPattern(oppo, WINNING)) {
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
    if (!board.getMovesByPattern(self, MATE).empty()) {
        return *board.getMovesByPattern(self, MATE).begin();
    }

    return Pos();
}

MoveList Evaluator::getFours() {
    MoveList result;
    if (!board.getMovesByPattern(self, WINNING).empty()) {
        result.push_back(*board.getMovesByPattern(self, WINNING).begin()); 
        return result;
    }
    if (!board.getMovesByPattern(self, MATE).empty()) {
        result.push_back(*board.getMovesByPattern(self, MATE).begin());
        return result;
    }
    if (!board.getMovesByPattern(self, B4_F3).empty()) {
        result.insert(result.end(), board.getMovesByPattern(self, B4_F3).begin(), board.getMovesByPattern(self, B4_F3).end());
    }
    if (!board.getMovesByPattern(self, B4_PLUS).empty()) {
        result.insert(result.end(), board.getMovesByPattern(self, B4_PLUS).begin(), board.getMovesByPattern(self, B4_PLUS).end());
    }
    if (!board.getMovesByPattern(self, B4_ANY).empty()) {
        result.insert(result.end(), board.getMovesByPattern(self, B4_ANY).begin(), board.getMovesByPattern(self, B4_ANY).end());
    }

    return result;
}

MoveList Evaluator::getThreats() {
    MoveList result;
    if (!board.getMovesByPattern(self, WINNING).empty()) {
        result.push_back(*board.getMovesByPattern(self, WINNING).begin()); 
        return result;
    }
    if (!board.getMovesByPattern(self, MATE).empty()) {
        result.push_back(*board.getMovesByPattern(self, MATE).begin());
        return result;
    }
    if (!board.getMovesByPattern(self, B4_F3).empty()) {
        result.insert(result.end(), board.getMovesByPattern(self, B4_F3).begin(), board.getMovesByPattern(self, B4_F3).end());
    }
    if (!board.getMovesByPattern(self, F3_2X).empty()) {
        result.insert(result.end(), board.getMovesByPattern(self, F3_2X).begin(), board.getMovesByPattern(self, F3_2X).end());
    }
    if (!board.getMovesByPattern(self, F3_PLUS).empty()) {
        result.insert(result.end(), board.getMovesByPattern(self, F3_PLUS).begin(), board.getMovesByPattern(self, F3_PLUS).end());
    }
    if (!board.getMovesByPattern(self, F3_ANY).empty()) {
        result.insert(result.end(), board.getMovesByPattern(self, F3_ANY).begin(), board.getMovesByPattern(self, F3_ANY).end());
    }
    if (!board.getMovesByPattern(self, B4_PLUS).empty()) {
        result.insert(result.end(), board.getMovesByPattern(self, B4_PLUS).begin(), board.getMovesByPattern(self, B4_PLUS).end());
    }
    if (!board.getMovesByPattern(self, B4_ANY).empty()) {
        result.insert(result.end(), board.getMovesByPattern(self, B4_ANY).begin(), board.getMovesByPattern(self, B4_ANY).end());
    }

    return result;
}

MoveList Evaluator::getThreatDefend() {
    PROFILE_FUNCTION();
    MoveList result;

    if (!isOppoMateExist()) return result;

    if (!board.getMovesByPattern(oppo, WINNING).empty()) {
        for (const auto& p : board.getMovesByPattern(oppo, WINNING)) {
            if (!isMoveForbidden(p)) result.push_back(p);
        }
    }

    if (!board.getMovesByPattern(oppo, MATE).empty()) {
        for (const auto& p : board.getMovesByPattern(oppo, MATE)) {
            if (!isMoveForbidden(p)) result.push_back(p);
        }
    }

    // check every mate move (free 3)
    for (auto p : board.getMovesByPattern(oppo, MATE)) {
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
    if (!board.getMovesByPattern(oppo, WINNING).empty()) return true;
    if (!board.getMovesByPattern(oppo, MATE).empty()) return true;
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
    if (!board.getMovesByPattern(self, WINNING).empty()) {
        return Value(Value::Result::WIN, 1);
    }
    // 2 step before lose
    if (board.getMovesByPattern(oppo, WINNING).size() > 1) {
        return Value(Value::Result::LOSE, 2);
    }
    // 3 step before win
    if (!board.getMovesByPattern(self, MATE).empty() && board.getMovesByPattern(oppo, WINNING).empty()) {
        return Value(Value::Result::WIN, 3);
    }

    int val = 0;
    val += board.getMovesByPattern(self, B4_F3).size() * 150;
    val += board.getMovesByPattern(self, B4_PLUS).size() * 25;
    val += board.getMovesByPattern(self, F3_2X).size() * 35;
    val += board.getMovesByPattern(self, F3_PLUS).size() * 25;

    val += board.getMovesByPattern(self, B4_ANY).size() * 25;
    val += board.getMovesByPattern(self, F3_ANY).size() * 25;
    val += board.getMovesByPattern(self, B3_PLUS).size() * 10;
    val += board.getMovesByPattern(self, F2_2X).size() * 9;
    val += board.getMovesByPattern(self, B3_ANY).size() * 5;
    val += board.getMovesByPattern(self, F2_ANY).size() * 4;

    val -= board.getMovesByPattern(oppo, B4_PLUS).size() * 20;
    val -= board.getMovesByPattern(oppo, B4_ANY).size() * 20;
    val -= board.getMovesByPattern(oppo, F3_2X).size() * 20;
    val -= board.getMovesByPattern(oppo, F3_PLUS).size() * 20;
    val -= board.getMovesByPattern(oppo, F3_ANY).size() * 20;

    val -= board.getMovesByPattern(oppo, B3_PLUS).size() * 8;
    val -= board.getMovesByPattern(oppo, F2_2X).size() * 7;
    val -= board.getMovesByPattern(oppo, B3_ANY).size() * 3;
    val -= board.getMovesByPattern(oppo, F2_ANY).size() * 2;

    return Value(val);
}