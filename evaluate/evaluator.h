#include "../game/board.h"
#include <list>
#include <tuple>

using Value = int;
using Depth = int;

class Evaluator {

private:
    Board board;

public:
    void setBoard(Board board);
    list<Pos> getCandidates();
    Value evaluate();
    void next(Pos p);
    void prev();
    bool isGameOver();
};

void Evaluator::setBoard(Board board) {
    this->board = board;
}

void Evaluator::next(Pos p) {
    board.move(p);
}

void Evaluator::prev() {
    board.undo();
}

list<Pos> Evaluator::getCandidates() {
    list<Pos> moves;
    list<tuple<Pos, int>> tmp;

    Piece self = board.isBlackTurn() ? BLACK : WHITE;
    Piece oppo = !board.isBlackTurn() ? BLACK : WHITE;

    for (int i = 1; i <= BOARD_SIZE; i++) {
        for (int j = 1; j <= BOARD_SIZE; j++) {
            Cell cell = board.getCell(Pos(i, j));
            if (cell.getPiece() != EMPTY) continue;
            int val = 0;
            for (Direction dir = DIRECTION_START; dir < DIRECTION_SIZE; dir++) {
                Pattern p = cell.getPattern(self, dir);
                if (p != PATTERN_SIZE) {
                    if (p == FIVE)              val += 1000000;
                    else if (p == FREE_4)       val += 100000;
                    else if (p == BLOCKED_4)    val += 5000;
                    else if (p == FREE_3A)      val += 300;
                    else if (p == FREE_3)       val += 300;
                    else if (p == BLOCKED_3)    val += 100;
                    else if (p == FREE_2)       val += 50;
                    else                        val += (int)p * (int)p;
                }

                p = cell.getPattern(oppo, dir);
                if (p != PATTERN_SIZE) {
                    if (p == FIVE)              val += 900000;
                    else if (p == FREE_4)       val += 80000;
                    else if (p == BLOCKED_4)    val += 4000;
                }
            }
            if (val >= 5000)
                tmp.push_back(make_tuple(Pos(i, j), val));
        }
    }

    tmp.sort([](const tuple<Pos, int>& a, const tuple<Pos, int>& b) {
        return get<1>(a) > get<1>(b);
    });

    //cout << "candidates: " ;
    for (const auto& item : tmp) {
        moves.push_back(get<0>(item));
        //cout << "[" << get<0>(item).getX() << ", " << (char)(get<0>(item).getY() + 64) << "] (" << get<1>(item) << ")\t";
    }
    //if (!tmp.size())  cout << "none";
    //cout << endl;

    return moves;
}

Value Evaluator::evaluate() {
    int val = 0;

    Piece self = board.isBlackTurn() ? BLACK : WHITE;
    Piece oppo = !board.isBlackTurn() ? BLACK : WHITE;

    int selfFree3Count = 0;
    int selfFree4Count = 0;
    //int oppoFree3Count = 0;
    //int oppoFree4Count = 0;

    bool isVCT = false;
    bool isVCF = false;

    for (int i = 1; i <= BOARD_SIZE; i++) {
        for (int j = 1; j <= BOARD_SIZE; j++) {
            Cell cell = board.getCell(Pos(i, j));
            if (cell.getPiece() != EMPTY) continue;

            for (Direction dir = DIRECTION_START; dir < DIRECTION_SIZE; dir++) {
                Pattern p = cell.getPattern(self, dir);
                if (p != PATTERN_SIZE) {
                    if (p == FIVE) {
                        val += 1000000;
                    } else if (p == FREE_4) {
                        val += 100000;
                        selfFree4Count++;
                    } else if (p == BLOCKED_4) {
                        val += 5000;
                    } else if (p == FREE_3A || p == FREE_3) {
                        val += 300;
                        selfFree3Count++;
                    } else if (p == BLOCKED_3) {
                        val += 100;
                    } else if (p == FREE_2) {
                        val += 50;
                    } else {
                        val += (int)p * (int)p;
                    }
                }

                p = cell.getPattern(oppo, dir);
                if (p != PATTERN_SIZE) {
                    if (p == FIVE) {
                        val -= 900000;
                    } else if (p == FREE_4) {
                        val += 80000;
                        //oppoFree4Count++;
                    } else if (p == BLOCKED_4) {
                        val += 4000;
                    } else if (p == FREE_3A || p == FREE_3) {
                        //oppoFree3Count++;
                    }
                }
            }
        }
    }

    // Add extra conditions for VCF and VCT
    if (selfFree4Count >= 1 && selfFree3Count >= 1) {
        isVCF = true;  // VCF potential win
        //cout << "Self VCF" << endl;
    }
    if (selfFree4Count >= 2) {
        isVCT = true;  // VCT potential win
        //cout << "Self VCT" << endl;
    }
    //if (oppoFree4Count >= 1 && oppoFree3Count >= 1) {
        // Opponent VCF potential win
        //cout << "Oppo VCF" << endl;
    //}
    //if (oppoFree4Count >= 2) {
        // Opponent VCT potential win
        //cout << "Oppo VCT" << endl;
    //}

    return val;
}

bool Evaluator::isGameOver() {
    return board.getResult() != ONGOING;
}