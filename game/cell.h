#include "types.h"
#include <cassert>
using namespace std;

class Cell {

private:
    Piece piece;
    Pattern patterns[2][DIRECTION_SIZE];

public:
    Cell();
    Piece getPiece();
    void setPiece(const Piece& piece);
    Pattern getPattern(Piece piece, Direction dir);
    void setPattern(Piece piece, Direction dir, Pattern pattern);
    
};

Cell::Cell() {
    this->piece = EMPTY;
    for(Direction dir = DIRECTION_START; dir < DIRECTION_SIZE; dir++) {
        setPattern(BLACK, dir, PATTERN_SIZE);
        setPattern(WHITE, dir, PATTERN_SIZE);
    }
}

Piece Cell::getPiece() {
    return piece;
}

void Cell::setPiece(const Piece& piece) {
    this->piece = piece;
}

Pattern Cell::getPattern(Piece piece, Direction dir) {
    assert(piece == BLACK || piece == WHITE);
    return patterns[piece][dir];
}

void Cell::setPattern(Piece piece, Direction dir, Pattern pattern) {
    assert(piece == BLACK || piece == WHITE);
    this->patterns[piece][dir] = pattern;
}