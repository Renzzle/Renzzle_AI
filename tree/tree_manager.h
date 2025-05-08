#pragma once

#include "tree.h"
#include "../test/test.h"
#include <stack>

class TreeManager {

PRIVATE
    Tree tree;
    Node* currentNode;
    stack<Node*> nodeHistory;

PUBLIC
    TreeManager(Board initialBoard);
    bool move(Pos p);
    void undo();
    void cleanCache();
    Board& getBoard();
    Node* getChildNode(Pos p);
    Node* getNode();

};

TreeManager::TreeManager(Board initialBoard) {
    auto rootNode = tree.addNodeAsRoot(initialBoard);
    currentNode = rootNode;
    nodeHistory.push(currentNode);
}

bool TreeManager::move(Pos p) {
    Node* previousNode = currentNode;

    // if child node exist
    for (const auto& pair : previousNode->childNodes) {
        Node* node = pair.second;
        if (node->board.getPath().back() == p) {
            currentNode = node;
            nodeHistory.push(currentNode);
            return true;
        }
    }

    // new child node
    Board newBoard = previousNode->board;
    bool result = newBoard.move(p);
    if (!result) return result; // move failed

    currentNode = tree.addNode(previousNode, newBoard);
    nodeHistory.push(currentNode);
    return result;
}

void TreeManager::undo() {
    // remain root node
    if (nodeHistory.size() > 1) {
        nodeHistory.pop();
        currentNode = nodeHistory.top();
    }
}

void TreeManager::cleanCache() {
    tree.cleanTree();
}

Board& TreeManager::getBoard() {
    return currentNode->board;
}

Node* TreeManager::getChildNode(Pos p) {
    for (const auto& pair : currentNode->childNodes) {
        Node* node = pair.second;
        if (node->board.getPath().back() == p) {
            return node;
        }
    }
    return nullptr; // if cannot find
}

Node* TreeManager::getNode() {
    return currentNode;
}