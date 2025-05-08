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
    unordered_map<size_t, Node*> childNodes;
    Pos bestMove;
    int visitedCnt;

    Node(Board b) : board(b), value(INITIAL_VALUE), result(ONGOING), visitedCnt(0) {}
};

class Tree {

PRIVATE
    unordered_map<size_t, unique_ptr<Node>> nodeMap;

PUBLIC
    Node* addNodeAsRoot(Board& board);
    Node* addNode(Node* parentNode, Board& newNode);
    void cleanTree();
    bool exist(Board& board);

};

Node* Tree::addNodeAsRoot(Board& board) {
    size_t key = board.getCurrentHash();
    nodeMap[key] = unique_ptr<Node>(new Node(board));
    return nodeMap[key].get();
}

Node* Tree::addNode(Node* parentNode, Board& newNode) {
    size_t key = newNode.getCurrentHash();

    auto it = nodeMap.find(key);
    if (it != nodeMap.end()) { // node already exists
        parentNode->childNodes[key] = it->second.get();
        return it->second.get();
    }

    nodeMap[key] = unique_ptr<Node>(new Node(newNode));
    parentNode->childNodes[key] = nodeMap[key].get();
    return parentNode->childNodes[key];
}

void Tree::cleanTree() {
    nodeMap.clear();
}

bool Tree::exist(Board& board) {
    size_t key = board.getCurrentHash();
    auto it = nodeMap.find(key);
    if (it != nodeMap.end()) {
        return true;
    }
    return false;
}