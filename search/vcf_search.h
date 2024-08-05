#pragma once

#include "../evaluate/evaluator.h"
#include "../tree/tree.h"
#include "../game/board_manager.h"
#include <vector>
#include <algorithm>
#include <limits>
#include <memory>
#include <stack>
#include <iostream>

using namespace std;
using Value = int;

constexpr Value INF = numeric_limits<Value>::max();

class VCFSearch {

    Evaluator evaluator;
    Tree tree;
    shared_ptr<Node> currentNode;
    vector<Pos> winningPath;
    stack<shared_ptr<Node>> path;
    int nodesExplored;
    BoardManager boardManager;
    Color targetColor; // The color we are trying to win with

    Value alphaBeta(shared_ptr<Node> node, Value alpha, Value beta, bool maximizingPlayer);
    void incrementNodesExplored();

public:
    VCFSearch(Board initialBoard);

    Pos findBestMove();
    void printWinningPath();
    int getNodesExplored() const;
    
};

VCFSearch::VCFSearch(Board initialBoard) : evaluator(), nodesExplored(0), boardManager(initialBoard) {
    currentNode = make_shared<Node>(initialBoard, Pos(), 0);
    tree.addNode(currentNode);
    targetColor = initialBoard.isBlackTurn() ? COLOR_BLACK : COLOR_WHITE;
}

void VCFSearch::incrementNodesExplored() {
    nodesExplored++;
}

Pos VCFSearch::findBestMove() {
    Value bestValue = -INF;
    Pos bestMove;

    bool isTargetTurn = (targetColor == COLOR_BLACK && boardManager.isBlackTurn()) || (targetColor == COLOR_WHITE && !boardManager.isBlackTurn());
    auto candidates = isTargetTurn ? evaluator.getFours(boardManager.getBoard()) : evaluator.getCandidates(boardManager.getBoard());

    // Print the candidates
    cout << "Candidates: ";
    for (const auto& pos : candidates) {
        cout << "[" << pos.getX() << ", " << (char)(pos.getY() + 64) << "] ";
    }
    cout << endl;

    for (const auto& move : candidates) {
        boardManager.move(move); // 보드 상태 업데이트
        auto newBoard = boardManager.getBoard(); // 현재 보드 상태 가져오기
        auto childNode = tree.createVcfNode(currentNode->path, newBoard, move, 0);

        // Output the path of the child node
        cout << "Child node path: ";
        for (const auto& pos : childNode->path) {
            cout << "[" << pos.getX() << ", " << (char)(pos.getY() + 64) << "] ";
        }
        cout << endl;

        tree.addNode(childNode);
        path.push(childNode);

        Value moveValue = alphaBeta(childNode, -INF, INF, false);
        path.pop();
        boardManager.undo(); // 이전 보드 상태로 되돌리기

        cout << "moveValue : " << moveValue << endl;
        if (moveValue > bestValue) {
            bestValue = moveValue;
            bestMove = move;
        }
        if (moveValue == 50000) {
            break;
        }
    }
    return bestMove;
}

Value VCFSearch::alphaBeta(shared_ptr<Node> node, Value alpha, Value beta, bool maximizingPlayer) {
    incrementNodesExplored();
    Value currentEval = evaluator.vcfEvaluate(boardManager.getBoard(), targetColor);

    bool isTargetTurn = (targetColor == COLOR_BLACK && boardManager.isBlackTurn()) || (targetColor == COLOR_WHITE && !boardManager.isBlackTurn());
    auto candidates = isTargetTurn ? evaluator.getFours(boardManager.getBoard()) : evaluator.getCandidates(boardManager.getBoard());

    // Print the candidates
    cout << "Candidates: " << isTargetTurn << ": ";
    for (const auto& pos : candidates) {
        cout << "[" << pos.getX() << ", " << (char)(pos.getY() + 64) << "] ";
    }
    cout << endl;

    if (currentEval != 0) {
        node->val = currentEval;
        if (currentEval == MAX_VALUE) {
            cout << " WIN : " << endl;
            winningPath = node->path;
            // Output the path of the final node
            cout << "Final node path: ";
            for (const auto& pos : node->path) {
                cout << "[" << pos.getX() << ", " << (char)(pos.getY() + 64) << "] ";
            }
            cout << endl;
        }
        return currentEval;
    }

    if (candidates.empty()) {
        node->val = currentEval;
        return currentEval;
    }

    if (maximizingPlayer) {
        Value maxEval = -INF;
        for (const auto& move : candidates) {
            incrementNodesExplored();
            boardManager.move(move); // 보드 상태 업데이트
            auto newBoard = boardManager.getBoard(); // 현재 보드 상태 가져오기
            auto childNode = tree.createVcfNode(node->path, newBoard, move, 0);

            // Output the path of the child node
            cout << "Child node path: ";
            for (const auto& pos : childNode->path) {
                cout << "[" << pos.getX() << ", " << (char)(pos.getY() + 64) << "] ";
            }
            cout << endl;

            tree.addNode(childNode);
            path.push(childNode);

            Value eval = alphaBeta(childNode, alpha, beta, !maximizingPlayer);
            maxEval = max(maxEval, eval);
            alpha = max(alpha, eval);
            path.pop();
            boardManager.undo(); // 이전 보드 상태로 되돌리기
            if (beta <= alpha) break;
        }
        node->val = maxEval;
        return maxEval;
    } else {
        Value minEval = INF;
        for (const auto& move : candidates) {
            incrementNodesExplored();
            boardManager.move(move); // 보드 상태 업데이트
            auto newBoard = boardManager.getBoard(); // 현재 보드 상태 가져오기
            auto childNode = tree.createVcfNode(node->path, newBoard, move, 0);

            // Output the path of the child node
            cout << "Child node path: ";
            for (const auto& pos : childNode->path) {
                cout << "[" << pos.getX() << ", " << (char)(pos.getY() + 64) << "] ";
            }
            cout << endl;

            tree.addNode(childNode);
            path.push(childNode);

            Value eval = alphaBeta(childNode, alpha, beta, !maximizingPlayer);
            minEval = min(minEval, eval);
            beta = min(beta, eval);
            path.pop();
            boardManager.undo(); // 이전 보드 상태로 되돌리기
            if (beta <= alpha) break;
        }
        node->val = minEval;
        return minEval;
    }
}

void VCFSearch::printWinningPath() {
    cout << "Winning path size: " << winningPath.size() << endl;
    cout << "Winning path: ";
    for (const auto& pos : winningPath) {
        cout << "[" << pos.getX() << ", " << (char)(pos.getY() + 64) << "] ";
    }
    cout << endl;
}

int VCFSearch::getNodesExplored() const {
    return nodesExplored;
}
