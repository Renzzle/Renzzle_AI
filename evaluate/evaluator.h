#include "../game/board.h"
#include <list>

using Value = int;

class Evaluator {

private:
    Board board;

public:
    list<Pos> getCandidates();
    Value evaluate();
    void next(Pos p);
    void prev();

};

void Evaluator::next(Pos p) {
    board.move(p);
}