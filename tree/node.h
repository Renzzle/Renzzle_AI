#pragma once

#include "../game/board.h"
#include "../evaluate/evaluator.h"

class Node {

public:
    Board board;
    Value value;

    Node(Board b, Value v) : board(b), value(v) {}

};