#pragma once

#include "tree.h"
#include "../test/test.h"
#include <stack>

class TreeManager {

PRIVATE
    Tree tree;
    Node* rootNode;
    Node* currentNode;
    stack<Node*> nodeHistory;

PUBLIC
    TreeManager(Board initialBoard);
    bool move(Pos p);
    void undo();
    Board& getBoard();
    Node* getChildNode(Pos p);
    Node* getNode();
    Node* getRootNode();

};

TreeManager::TreeManager(Board initialBoard) {
    rootNode = tree.addNodeAsRoot(initialBoard);
    currentNode = rootNode;
    nodeHistory.push(currentNode);
}

bool TreeManager::move(Pos p) {
    Node* previousNode = currentNode;

    // if child node exist
    for (const auto& pair : previousNode->childNodes) {
        Node* node = pair.second;
        if (node->board.getPath().back() == p) {
            // but if child node is forbidden move
            if (previousNode->board.isForbidden(p))
                break;

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

Node* TreeManager::getRootNode() {
    return rootNode;
}