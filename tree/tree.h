#pragma once

#include "../game/board.h" 
#include "node.h"
#include <unordered_map>
#include <vector>
#include <functional>
#include <algorithm>
#include <memory>

using namespace std;

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
// private:

public:
    unordered_map<size_t, shared_ptr<Node>> nodeMap; // Hash map to store nodes with board state keys.

    // Method to generate the key for the hash map.
    size_t generateKey(Board& board);

    // Helper method to convert Pos to string
    // string posToString(const Pos& pos) const;

    // Method to add a node to the hash map.
    void addNode(shared_ptr<Node> node);

    // Method to retrieve a node from the hash map using a board state.
    shared_ptr<Node> getNode(Board& board);

    // Method to create a new node.
    shared_ptr<Node> createNode(const vector<Pos>& parent, Board board, Pos move, Value score, int depth);

    // Method to create a new VCF node without depth.
    shared_ptr<Node> createVcfNode(const vector<Pos>& parent, Board board, Pos move, Value score);
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
    for (const auto& pos : whiteStones) {
        hashValue ^= hash<Pos>()(pos) ^ (hash<int>()(WHITE) << 1);
    }
    for (const auto& pos : blackStones) {
        hashValue ^= hash<Pos>()(pos) ^ (hash<int>()(BLACK) << 1);
    }

    return hashValue;
}

// Helper method to convert Pos to string
// string Tree::posToString(const Pos& pos) const {
//     return "(" + to_string(pos.getX()) + "," + to_string(pos.getY()) + ")";
// }

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
shared_ptr<Node> Tree::createNode(const vector<Pos>& parentPath, Board board, Pos move, Value score, int depth) {
    size_t key = generateKey(board);
    if (nodeMap.find(key) != nodeMap.end()) {
        return nodeMap[key]; // Node already exists, return it
    }
    auto newNode = make_shared<Node>(board, move, score, depth);
    newNode->path = parentPath;
    newNode->path.push_back(move);
    nodeMap[key] = newNode;
    // std::cout << "New node created: " << key << std::endl;
    return newNode;
}

// Definition of the createVcfNode method.
shared_ptr<Node> Tree::createVcfNode(const vector<Pos>& parentPath, Board board, Pos move, Value score) {
    size_t key = generateKey(board);
    if (nodeMap.find(key) != nodeMap.end()) {
        return nodeMap[key]; // Node already exists, return it
    }
    auto newNode = make_shared<Node>(board, move, score);
    newNode->path = parentPath;
    newNode->path.push_back(move);
    nodeMap[key] = newNode;
    // std::cout << "New VCF node created: " << key << std::endl;
    return newNode;
}