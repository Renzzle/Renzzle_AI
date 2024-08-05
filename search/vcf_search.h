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

using Value = int;

constexpr Value INF = std::numeric_limits<Value>::max();

class VCFSearch {
    Evaluator evaluator;
    Tree tree;
    std::shared_ptr<Node> currentNode;
    std::vector<Pos> winningPath;
    std::stack<std::shared_ptr<Node>> path;
    int nodesExplored;
    BoardManager boardManager;
    Piece targetColor; // The color we are trying to win with

    Value alphaBeta(std::shared_ptr<Node> node, Value alpha, Value beta, bool maximizingPlayer);

public:
    VCFSearch(Board initialBoard);

    Pos findBestMove();
    void printWinningPath();
    int getNodesExplored() const;
    void incrementNodesExplored();
};

VCFSearch::VCFSearch(Board initialBoard) : evaluator(), nodesExplored(0), boardManager(initialBoard) {
    currentNode = std::make_shared<Node>(initialBoard, Pos(), 0);
    tree.addNode(currentNode);
    targetColor = initialBoard.isBlackTurn() ? BLACK : WHITE;
}

void VCFSearch::incrementNodesExplored() {
    nodesExplored++;
}

Pos VCFSearch::findBestMove() {
    Value bestValue = -INF;
    Pos bestMove;

    bool isTargetTurn = (targetColor == BLACK && boardManager.isBlackTurn()) || (targetColor == WHITE && !boardManager.isBlackTurn());
    auto candidates = isTargetTurn ? evaluator.getFours(boardManager.getBoard()) : evaluator.getCandidates(boardManager.getBoard());

    // Print the candidates
    std::cout << "Candidates: ";
    for (const auto& pos : candidates) {
        std::cout << "[" << pos.getX() << ", " << (char)(pos.getY() + 64) << "] ";
    }
    std::cout << std::endl;

    for (const auto& move : candidates) {
        std::cout << "move X : " << move.getX() << " move Y : " << move.getY() << std::endl;
        boardManager.move(move); // 보드 상태 업데이트
        auto newBoard = boardManager.getBoard(); // 현재 보드 상태 가져오기
        auto childNode = tree.createVcfNode(currentNode->path, newBoard, move, 0);

        // Output the path of the child node
        std::cout << "Child node path: ";
        for (const auto& pos : childNode->path) {
            std::cout << "[" << pos.getX() << ", " << (char)(pos.getY() + 64) << "] ";
        }
        std::cout << std::endl;

        tree.addNode(childNode);
        path.push(childNode);

        Value moveValue = alphaBeta(childNode, -INF, INF, false);
        path.pop();
        boardManager.undo(); // 이전 보드 상태로 되돌리기

        std::cout << "moveValue : " << moveValue << std::endl;
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

Value VCFSearch::alphaBeta(std::shared_ptr<Node> node, Value alpha, Value beta, bool maximizingPlayer) {
    incrementNodesExplored();
    Value currentEval = evaluator.evaluate(boardManager.getBoard());

    bool isTargetTurn = (targetColor == BLACK && boardManager.isBlackTurn()) || (targetColor == WHITE && !boardManager.isBlackTurn());
    auto candidates = isTargetTurn ? evaluator.getFours(boardManager.getBoard()) : evaluator.getCandidates(boardManager.getBoard());

    // Print the candidates
    std::cout << "Candidates: ";
    for (const auto& pos : candidates) {
        std::cout << "[" << pos.getX() << ", " << (char)(pos.getY() + 64) << "] ";
    }
    std::cout << std::endl;

    if (currentEval != 0) {
        node->score = currentEval;
        if (currentEval == 50000) {
            std::cout << " WIN : " << std::endl;
            winningPath = node->path;
            // Output the path of the final node
            std::cout << "Final node path: ";
            for (const auto& pos : node->path) {
                std::cout << "[" << pos.getX() << ", " << (char)(pos.getY() + 64) << "] ";
            }
            std::cout << std::endl;
        }
        return currentEval;
    }

    if (candidates.empty()) {
        node->score = currentEval;
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
            std::cout << "Child node path: ";
            for (const auto& pos : childNode->path) {
                std::cout << "[" << pos.getX() << ", " << (char)(pos.getY() + 64) << "] ";
            }
            std::cout << std::endl;

            tree.addNode(childNode);
            path.push(childNode);

            Value eval = alphaBeta(childNode, alpha, beta, !maximizingPlayer);
            maxEval = std::max(maxEval, eval);
            alpha = std::max(alpha, eval);
            path.pop();
            boardManager.undo(); // 이전 보드 상태로 되돌리기
            if (beta <= alpha) break;
        }
        node->score = maxEval;
        return maxEval;
    } else {
        Value minEval = INF;
        for (const auto& move : candidates) {
            incrementNodesExplored();
            boardManager.move(move); // 보드 상태 업데이트
            auto newBoard = boardManager.getBoard(); // 현재 보드 상태 가져오기
            auto childNode = tree.createVcfNode(node->path, newBoard, move, 0);

            // Output the path of the child node
            std::cout << "Child node path: ";
            for (const auto& pos : childNode->path) {
                std::cout << "[" << pos.getX() << ", " << (char)(pos.getY() + 64) << "] ";
            }
            std::cout << std::endl;

            tree.addNode(childNode);
            path.push(childNode);

            Value eval = alphaBeta(childNode, alpha, beta, !maximizingPlayer);
            minEval = std::min(minEval, eval);
            beta = std::min(beta, eval);
            path.pop();
            boardManager.undo(); // 이전 보드 상태로 되돌리기
            if (beta <= alpha) break;
        }
        node->score = minEval;
        return minEval;
    }
}

void VCFSearch::printWinningPath() {
    std::cout << "Winning path size: " << winningPath.size() << std::endl;
    std::cout << "Winning path: ";
    for (const auto& pos : winningPath) {
        std::cout << "[" << pos.getX() << ", " << (char)(pos.getY() + 64) << "] ";
    }
    std::cout << std::endl;
}

int VCFSearch::getNodesExplored() const {
    return nodesExplored;
}
