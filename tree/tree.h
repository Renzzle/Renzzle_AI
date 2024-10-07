#pragma once

#include "../game/board.h" 
#include "node.h"
#include <unordered_map>
#include <vector>
#include <functional>
#include <algorithm>
#include <memory>
#include <random>
#include <limits>


#define NUM_PIECE_TYPES 4

using namespace std;
using Value = int;

// hash specialization for Pos
namespace std {
    template <>
    struct hash<Pos> {
        size_t operator()(const Pos &pos) const {
            return hash<int>()(pos.getX()) ^ (hash<int>()(pos.getY()) << 1);
        }
    };
}

class Tree {

private:
    unordered_map<size_t, shared_ptr<Node>> nodeMap;

public:
    Tree();

    // Method to generate the key for the hash map.
    size_t generateKey(Board& board);

    // Method to add a node to the hash map.
    void addNode(shared_ptr<Node> node);

    // Method to retrieve a node from the hash map using a board state.
    shared_ptr<Node> getNode(Board& board);

    // Method to create a new node.
    shared_ptr<Node> createNode(Board board, Pos move, Value score, int depth);

};

Tree::Tree() { }

// Definition of the generateKey method.
size_t Tree::generateKey(Board& board) {
    return board.getCurrentHash();
}

// Definition of the addNode method.
void Tree::addNode(shared_ptr<Node> node) {
    size_t key = generateKey(node->board);

    nodeMap[key] = node;
}

// Definition of the getNode method.
shared_ptr<Node> Tree::getNode(Board& board) {
    size_t key = generateKey(board);
    auto it = nodeMap.find(key);
    if (it != nodeMap.end()) {
        return it->second;
    }
    return nullptr; // or handle the case where the node is not found.
}

// Definition of the createNode method.
shared_ptr<Node> Tree::createNode(Board board, Pos move, Value score, int depth) {
    size_t key = generateKey(board);
    if (nodeMap.find(key) != nodeMap.end()) {
        return nodeMap[key]; // Node already exists, return it
    }
    auto newNode = make_shared<Node>(board, move, score, depth);
    return newNode;
}