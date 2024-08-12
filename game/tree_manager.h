#pragma once

#include "../tree/tree.h"
#include "board.h"
#include <stack>
#include<iomanip>

class TreeManager {
private:
    Board board;
    Tree tree;
    shared_ptr<Node> currentNode;
     std::stack<shared_ptr<Node>> nodeHistory;

public:
    TreeManager(Board initialBoard);

    void move(Pos p);
    void undo();
    Board& getBoard();
    void printBoard();

    void addNode(shared_ptr<Node> node);
    shared_ptr<Node> getNode(Board& board);
    shared_ptr<Node> createNode(const vector<Pos>& parentPath, Board board, Pos move, Value score, int depth);
    // shared_ptr<Node> createVcfNode(const vector<Pos>& parentPath, Board board, Pos move, Value score);
};

TreeManager::TreeManager(Board initialBoard) : board(initialBoard), tree() {

    Pos initialMove;
    Value initialScore = 0;
    int initialDepth = 0;

    auto rootNode = createNode( /*path*/ {}, board, initialMove, initialScore, initialDepth);
    addNode(rootNode);
    currentNode = rootNode;
    nodeHistory.push(currentNode);
}

void TreeManager::move(Pos p) {     

    shared_ptr<Node> previousNode = currentNode;

    Board newBoard = previousNode->board;
    newBoard.move(p);

    vector<Pos> newPath = previousNode->path;

    newPath.push_back(p);
    currentNode = createNode(newPath, newBoard, p, /*score*/ 0, previousNode->depth + 1);
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

shared_ptr<Node> TreeManager::createNode(const vector<Pos>& parentPath, Board board, Pos move, Value score, int depth) {
    return tree.createNode(parentPath, board, move, score, depth);
}

// shared_ptr<Node> TreeManager::createVcfNode(const vector<Pos>& parentPath, Board board, Pos move, Value score) {
//     return tree.createVcfNode(parentPath, board, move, score);
// }

void TreeManager::printBoard() {
    CellArray cells = board.getBoardStatus();
    
    for (int i = 0; i < BOARD_SIZE + 2; i++) {

        for (int j = 0; j < BOARD_SIZE + 2; j++) {
            Piece p = cells[i][j].getPiece();
            switch (p) {
                case WALL:
                    if (i == BOARD_SIZE + 1 && j < BOARD_SIZE) printf("%2c", j + 65);
                    else if (i != 0 && i != BOARD_SIZE + 1 && j != 0) printf(" %02d", i);
                    break;
                case BLACK:
                    cout << "⚫";
                    break;
                case WHITE:
                    cout << "⚪";
                    break;
                case EMPTY:
                    cout << "─┼";
                    break;
                default:
                    // Handle unexpected cases
                    break;
            }  
        }
        cout << endl;
    } 
    return;
}

