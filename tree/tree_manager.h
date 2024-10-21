#pragma once

#include "tree.h"
#include "../game/board.h"
#include "../test/test.h"
#include <stack>
#include <iomanip>

class TreeManager {

PRIVATE
    Board board;
    Tree tree;
    shared_ptr<Node> currentNode;
    stack<shared_ptr<Node>> nodeHistory;

PUBLIC
    TreeManager(Board initialBoard);
    void move(Pos p);
    void undo();
    Board& getBoard();
    void addNode(shared_ptr<Node> node);
    shared_ptr<Node> getNode(Board& board);
    shared_ptr<Node> createNode(Board board, Value score);
    vector<Pos> getPath();

};

TreeManager::TreeManager(Board initialBoard) : board(initialBoard), tree() {
    Pos initialMove;
    Value initialScore = 0;
    int initialDepth = 0;

    auto rootNode = createNode(board, initialScore);
    addNode(rootNode);
    currentNode = rootNode;
    nodeHistory.push(currentNode);
}

void TreeManager::move(Pos p) {     
    shared_ptr<Node> previousNode = currentNode;

    Board newBoard = previousNode->board;
    newBoard.move(p);

    currentNode = createNode(newBoard, /*score*/ 0);
    addNode(currentNode);
    nodeHistory.push(currentNode);
}

void TreeManager::undo() {
    if (nodeHistory.size() > 1) {
        nodeHistory.pop();
        currentNode = nodeHistory.top();
    }
}

Board& TreeManager::getBoard() {
    return currentNode -> board;
}

void TreeManager::addNode(shared_ptr<Node> node) {
    tree.addNode(node);
}

shared_ptr<Node> TreeManager::getNode(Board& board) {
    return tree.getNode(board);
}

shared_ptr<Node> TreeManager::createNode(Board board, Value score) {
    return tree.createNode(board, score);
}

vector<Pos> TreeManager::getPath() {
    return currentNode->board.getPath();
}