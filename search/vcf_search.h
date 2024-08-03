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

    Value alphaBeta(std::shared_ptr<Node> node, Value alpha, Value beta, bool maximizingPlayer);

public:
    VCFSearch(Board initialBoard);

    // void setEvaluator(const Evaluator& eval);
    Pos findBestMove();
    void printWinningPath();
    int getNodesExplored() const;
    void incrementNodesExplored();
};

VCFSearch::VCFSearch(Board initialBoard) : evaluator(), nodesExplored(0), boardManager(initialBoard) {
    currentNode = std::make_shared<Node>(initialBoard, Pos(), 0);
    tree.addNode(currentNode);
}

void VCFSearch::incrementNodesExplored() {
    nodesExplored++;
}

// void VCFSearch::setEvaluator(const Evaluator& eval) {
//     this->evaluator = eval;
// }

Pos VCFSearch::findBestMove() {
    Value bestValue = -INF;
    Pos bestMove;

    // auto candidates = evaluator.getCandidates(boardManager.getBoard());
    auto candidates = evaluator.getFours(boardManager.getBoard());

    
    for (const auto& move : candidates) {
        cout<<"move X : " << move.getX() << " move Y : " << move.getY() << endl;
        boardManager.move(move); // 보드 상태 업데이트
        auto newBoard = boardManager.getBoard(); // 현재 보드 상태 가져오기
        auto childNode = tree.createVcfNode(currentNode->path, newBoard, move, 0);
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
        if (moveValue >= 20000) {
            break;
        }
    }
    return bestMove;
}

Value VCFSearch::alphaBeta(std::shared_ptr<Node> node, Value alpha, Value beta, bool maximizingPlayer) {
    incrementNodesExplored();
    Value currentEval = evaluator.evaluate(boardManager.getBoard());
    auto candidates = evaluator.getCandidates(boardManager.getBoard());

    if (currentEval != 0) {
        node->score = currentEval;
        if (currentEval >= 20000) {
            winningPath.clear();
            std::stack<std::shared_ptr<Node>> tmp = path;

            while (!tmp.empty()) {
                winningPath.push_back(tmp.top()->move); // Pos 정보를 winningPath에 저장
                tmp.pop();
            }
            std::reverse(winningPath.begin(), winningPath.end());
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
