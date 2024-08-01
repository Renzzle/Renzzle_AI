#pragma once

#include "../game/board.h"  // Include game classes
#include "../evaluate/evaluator.h"
#include <vector>

using namespace std;

class Node {
public:
    Board board;             // The state of the game board at this node.
    Pos move;                // The move that led to this node.
    Value score;             // The evaluated score of this node.
    vector<Pos> path;        // Path from the start to this node.
    int depth;               // Depth of the node, default is -1.

    // Constructor for the Node class.
    Node(Board b, Pos m, Value s, int d = -1) : board(b), move(m), score(s), depth(d) {}
};