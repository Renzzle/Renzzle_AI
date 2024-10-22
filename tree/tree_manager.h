#pragma once

#include "tree.h"
#include "../test/test.h"
#include <stack>

class TreeManager {

PRIVATE
    static Tree tree;
    shared_ptr<Node> currentNode;
    stack<shared_ptr<Node>> nodeHistory;

PUBLIC
    TreeManager(Board initialBoard);
    bool move(Pos p);
    void undo();
    Board& getBoard();

};

Tree TreeManager::tree;

TreeManager::TreeManager(Board initialBoard) {
    auto rootNode = tree.createNode(initialBoard);
    tree.addNodeAsRoot(rootNode);
    currentNode = rootNode;
    nodeHistory.push(currentNode);
}

bool TreeManager::move(Pos p) {     
    shared_ptr<Node> previousNode = currentNode;

    // if child node exist
    for(const auto& pair : previousNode->childNodes) {
        shared_ptr<Node> node = pair.second;
        if (node->board.getPath().back() == p) {
            currentNode = node;
            nodeHistory.push(currentNode);
            return true;
        }
    }

    // new child node
    Board newBoard = previousNode->board;
    bool result = newBoard.move(p);
    if(!result) return result; // move failed

    currentNode = tree.createNode(newBoard);
    tree.addNode(previousNode, currentNode);
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

Board& TreeManager::getBoard() {
    return currentNode->board;
}