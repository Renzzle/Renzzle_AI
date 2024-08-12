#pragma once

#include "tree.h"
#include "../game/board.h"
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
    shared_ptr<Node> createNode(Board board, Pos move, Value score, int depth);
    vector<Pos> getVCFPath();

};

TreeManager::TreeManager(Board initialBoard) : board(initialBoard), tree() {
    Pos initialMove;
    Value initialScore = 0;
    int initialDepth = 0;

    auto rootNode = createNode(board, initialMove, initialScore, initialDepth);
    addNode(rootNode);
    currentNode = rootNode;
    nodeHistory.push(currentNode);
}

void TreeManager::move(Pos p) {     
    shared_ptr<Node> previousNode = currentNode;

    Board newBoard = previousNode->board;
    newBoard.move(p);

    currentNode = createNode(newBoard, p, /*score*/ 0, previousNode->depth + 1);
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

shared_ptr<Node> TreeManager::createNode(Board board, Pos move, Value score, int depth) {
    return tree.createNode(board, move, score, depth);
}

vector<Pos> TreeManager::getVCFPath() {
    return currentNode->board.getPath();
}