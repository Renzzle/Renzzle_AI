#pragma once

#include "../game/board.h"
#include "../evaluate/evaluator.h"
#include "../test/test.h"
#include <unordered_map>
#include <memory>

struct Node {
    enum class ValueType {
        EXACT, LOWER_BOUND, UPPER_BOUND, UNKNOWN
    };

    Board board;
    Value value;
    ValueType valueType;
    int searchedDepth;
    unordered_map<size_t, Node*> childNodes;
    Pos bestMove;

    Node(Board b) : board(b), value(INITIAL_VALUE), searchedDepth(0), valueType(ValueType::UNKNOWN) {}
};

class Tree {

PRIVATE
    unordered_map<size_t, unique_ptr<Node>> nodeMap;

PUBLIC
    Node* addNodeAsRoot(Board& board);
    Node* addNode(Node* parentNode, Board& newNode);
    Node* findNode(size_t hash);

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

Node* Tree::findNode(size_t hash) {
    auto it = nodeMap.find(hash);
    if (it != nodeMap.end()) {
        return it->second.get();
    }
    return nullptr;
}