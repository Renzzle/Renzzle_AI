#pragma once

#include "../game/board.h"
#include "../evaluate/evaluator.h"
#include "../test/test.h"
#include <unordered_map>
#include <memory>

struct Node {
    Board board;
    Value value;
    Result result;
    unordered_map<size_t, shared_ptr<Node>> childNodes;

    Node(Board b) : board(b), value(INITIAL_VALUE), result(ONGOING) {}
};

class Tree {

PRIVATE
    unordered_map<size_t, shared_ptr<Node>> nodeMap;

PUBLIC
    void addNodeAsRoot(shared_ptr<Node> node);
    void addNode(shared_ptr<Node> parentNode, shared_ptr<Node> node);
    shared_ptr<Node> createNode(Board board);
    bool exist(Board& board);

};

void Tree::addNodeAsRoot(shared_ptr<Node> root) {
    size_t key = root->board.getCurrentHash();
    nodeMap[key] = root;
}

void Tree::addNode(shared_ptr<Node> parentNode, shared_ptr<Node> node) {
    size_t key = node->board.getCurrentHash();
    nodeMap[key] = node;
    parentNode->childNodes[key] = node;
}

shared_ptr<Node> Tree::createNode(Board board) {
    size_t key = board.getCurrentHash();
    auto it = nodeMap.find(key);
    if (it != nodeMap.end()) {
        return it->second; // node already exists
    }
    auto newNode = make_shared<Node>(board);
    return newNode;
}

bool Tree::exist(Board& board) {
    size_t key = board.getCurrentHash();
    auto it = nodeMap.find(key);
    if (it != nodeMap.end()) {
        return true;
    }
    return false;
}