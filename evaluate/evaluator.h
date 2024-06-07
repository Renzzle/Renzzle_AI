#include "../game/board.h"

using Value = int;

class Evaluator {

private:
    Board board;
    bool isForbidden(Pos p);

public:
    bool winCheck(Piece p);
    Value evaluate();
    void next(Pos p);
    void prev(Pos p);

};