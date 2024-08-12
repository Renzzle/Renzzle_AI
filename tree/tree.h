#pragma once

#include "../game/board.h" 
#include "node.h"
#include <unordered_map>
#include <vector>
#include <functional>
#include <algorithm>
#include <memory>

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
    unordered_map<size_t, shared_ptr<Node>> nodeMap; // Hash map to store nodes with board state keys.

public:
    // Method to generate the key for the hash map.
    size_t generateKey(Board& board);

    // Method to add a node to the hash map.
    void addNode(shared_ptr<Node> node);

    // Method to retrieve a node from the hash map using a board state.
    shared_ptr<Node> getNode(Board& board);

    // Method to create a new node.
    shared_ptr<Node> createNode(Board board, Pos move, Value score, int depth);

};

// Definition of the generateKey method.
size_t Tree::generateKey(Board& board) {
    vector<Pos> whiteStones;
    vector<Pos> blackStones;

    auto cells = board.getBoardStatus();
    for (int i = 1; i <= BOARD_SIZE; i++) {
        for (int j = 1; j <= BOARD_SIZE; j++) {
            Pos p(i, j);
            if (cells[i][j].getPiece() == WHITE) {
                whiteStones.push_back(p);
            } else if (cells[i][j].getPiece() == BLACK) {
                blackStones.push_back(p);
            }
        }
    }
    
    sort(whiteStones.begin(), whiteStones.end());
    sort(blackStones.begin(), blackStones.end());

    size_t hashValue = 0;
    size_t prime = 31; // small decimal value add to the hash value
    
    // Combine the positions of white stones
    for (const auto& pos : whiteStones) {
        hashValue ^= (hash<Pos>()(pos) + prime + (hashValue << 6) + (hashValue >> 2));
    }

    // Combine the positions of black stones
    for (const auto& pos : blackStones) {
        hashValue ^= (hash<Pos>()(pos) + prime + (hashValue << 6) + (hashValue >> 2));
    }

    return hashValue;
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