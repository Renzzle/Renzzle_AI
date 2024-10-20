#pragma once

#include "../game/board.h"
#include "../evaluate/evaluator.h"
#include <vector>

using namespace std;
using Value = int;

class Node {

public:
    Board board;
    Pos move;    // The move that led to this node
    Value val;
    int depth;   // default -1

    Node(Board b, Pos m, Value s, int d = -1) : board(b), move(m), val(s), depth(d) {}

};