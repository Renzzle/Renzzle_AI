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
    MoveList getBestLine(int i);

};

TreeManager::TreeManager(Board initialBoard) {
    rootNode = tree.addNodeAsRoot(initialBoard);
    currentNode = rootNode;
    nodeHistory.push(currentNode);
}

bool TreeManager::move(Pos p) {
    Node* previousNode = currentNode;

    // if child node exist
    Node* childNode = getChildNode(p);
    if (childNode != nullptr) {
        currentNode = childNode;
        nodeHistory.push(currentNode);
        return true;
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
    size_t childHash = currentNode->board.getChildHash(p);
    return tree.findNode(childHash);
}

Node* TreeManager::getNode() {
    return currentNode;
}

Node* TreeManager::getRootNode() {
    return rootNode;
}

MoveList TreeManager::getBestLine(int i) {
    MoveList result;
    if (!rootNode || rootNode->childNodes.empty()) 
        return result;

    std::vector<std::pair<Pos, Node*>> rankedChildren;
    for (const auto& entry : rootNode->childNodes) {
        Node* child = entry.second;
        if (child) {
            rankedChildren.emplace_back(child->board.getPath().back(), child);
        }
    }

    std::sort(rankedChildren.begin(), rankedChildren.end(),
        [](const std::pair<Pos, Node*>& a, const std::pair<Pos, Node*>& b) {
            return a.second->value > b.second->value;
    });

    if (i < 0 || i >= static_cast<int>(rankedChildren.size())) 
        return result;

    Node* node = rankedChildren[i].second;
    result.push_back(rankedChildren[i].first);
    while (node != nullptr && !node->bestMove.isDefault()) {
        result.push_back(node->bestMove);
        node = tree.findNode(node->board.getChildHash(node->bestMove));
    }

    return result;
}
