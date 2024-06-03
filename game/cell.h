#include "types.h"
#include <array>
#include <cassert>
#include <tuple>
#include <conio.h>
#include <iostream>
#include <chrono>

using namespace std;

class Cell {

public:
    Cell();
    Cell(Piece piece);
    Piece piece;
    Pattern patterns[2][DIRECTION_SIZE];
    
};

Cell::Cell() {
    this->piece = EMPTY;
    for(int i = 0; i < DIRECTION_SIZE; i++) {
        patterns[BLACK][i] = PATTERN_SIZE; 
        patterns[WHITE][i] = PATTERN_SIZE;
    }
}

Cell::Cell(Piece piece) {
    this->piece = piece;
    for(int i = 0; i < DIRECTION_SIZE; i++) {
        patterns[BLACK][i] = PATTERN_SIZE; 
        patterns[WHITE][i] = PATTERN_SIZE;
    }
}
