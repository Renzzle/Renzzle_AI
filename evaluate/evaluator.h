#include "../game/board.h"
#include <list>
#include <tuple>

using Value = int;

class Evaluator {

private:
    Board board;

public:
    void setBoard(Board board);
    list<Pos> getCandidates();
    Value evaluate(Color color);
    void next(Pos p);
    void prev();

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
                    if (p == FIVE) val += 1000000;
                    else if (p == FREE_4) val += 10000;
                    else if (p == BLOCKED_4) val += 5000;
                    else if (p == FREE_3A) val += 100;
                    else if (p == FREE_3) val += 100;
                    else val += (int)p * (int)p;
                }
                p = cell.getPattern(oppo, dir);
                if (p != PATTERN_SIZE) {
                    if (p == FIVE) val += 100000;
                }
            }
            if (val >= 5000)
                tmp.push_back(make_tuple(Pos(i, j), val));
        }
    }

    tmp.sort([](const tuple<Pos, int>& a, const tuple<Pos, int>& b) {
        return get<1>(a) > get<1>(b);
    });
    for (const auto& item : tmp) {
        moves.push_back(get<0>(item));
    }

    return moves;
}

Value Evaluator::evaluate(Color color) {
    Result result = board.getResult();
    if (result == ONGOING) return 0;
    else if (result == BLACK_WIN) {
        if (color == COLOR_BLACK)
            return 20000;
        else
            return -20000;
    } else if (result == WHITE_WIN) {
        if (color == COLOR_BLACK)
            return -20000;
        else
            return 20000;
    }
    return 0;
}