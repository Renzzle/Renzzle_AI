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

using namespace std;
using Value = int;

class Tree {

private:
    unordered_map<size_t, shared_ptr<Node>> nodeMap;

public:
    Tree();

    size_t generateKey(Board& board);
    void addNode(shared_ptr<Node> node);
    shared_ptr<Node> getNode(Board& board);
    shared_ptr<Node> createNode(Board board, Value score);

};

Tree::Tree() { }

size_t Tree::generateKey(Board& board) {
    return board.getCurrentHash();
}

void Tree::addNode(shared_ptr<Node> node) {
    size_t key = generateKey(node->board);

    nodeMap[key] = node;
}

shared_ptr<Node> Tree::getNode(Board& board) {
    size_t key = generateKey(board);
    auto it = nodeMap.find(key);
    if (it != nodeMap.end()) {
        return it->second;
    }
    return nullptr; // or handle the case where the node is not found.
}

shared_ptr<Node> Tree::createNode(Board board, Value score) {
    size_t key = generateKey(board);
    if (nodeMap.find(key) != nodeMap.end()) {
        return nodeMap[key]; // Node already exists, return it
    }
    auto newNode = make_shared<Node>(board, score);
    return newNode;
}