// #pragma once

// #include "../evaluate/evaluator.h"
// #include "../tree/tree.h"
// #include <vector>
// #include <algorithm>
// #include <limits>
// #include <memory>
// #include <stack>
// #include <queue>
// #include <memory>
// #include <iostream>

// using Value = int;

// constexpr Value INF = std::numeric_limits<Value>::max();

// class Search {
//     Evaluator evaluator;
//     Tree tree;
//     shared_ptr<Node> currentNode;
//     vector<Pos> winningPath;
//     stack<shared_ptr<Node>> path; // 경로를 저장하는 스택
//     int nodesExplored;

//     Value alphaBeta(shared_ptr<Node> node, int depth, Value alpha, Value beta, bool maximizingPlayer);

// public:
//     Search(Board initialBoard) : nodesExplored(0) {
//         currentNode = make_shared<Node>(initialBoard, Pos(), 0);
//         tree.addNode(currentNode);
//         evaluator.setBoard(initialBoard); // 초기 보드 설정
//     }

//     void setEvaluator(Evaluator eval);
//     Pos findBestMove(int depth);
//     void printWinningPath();
//     int getNodesExplored() const { return nodesExplored; }
//     void incrementNodesExplored();
// };

// void Search::incrementNodesExplored() {
//     nodesExplored++;
// }

// void Search::setEvaluator(Evaluator eval) {
//     this->evaluator = eval;
// }

// Pos Search::findBestMove(int depth) {
//     Value bestValue = -INF;
//     Pos bestMove;

//     evaluator.setVCFColor();
//     auto candidates = evaluator.getCandidates();
    
//     for (const auto& move : candidates) {
//         evaluator.next(move); // 보드 상태 업데이트
//         auto newBoard = evaluator.getBoard(); // 현재 보드 상태 가져오기
//         auto childNode = tree.createNode(currentNode->path, newBoard, move, 0, depth - 1);
//         tree.addNode(childNode);
//         path.push(childNode);

//         Value moveValue = alphaBeta(childNode, depth - 1, -INF, INF, false);
//         path.pop();
//         evaluator.prev(); // 이전 보드 상태로 되돌리기

//         std::cout << "moveValue : " << moveValue << std::endl;
//         if (moveValue > bestValue) {
//             bestValue = moveValue;
//             bestMove = move;
//         }
//         if (moveValue >= 20000) { // 승리라고 판단하는 값 변경하기
//             break;
//         }
//     }
//     return bestMove;
// }

// Value Search::alphaBeta(shared_ptr<Node> node, int depth, Value alpha, Value beta, bool maximizingPlayer) {
//     incrementNodesExplored();
//     evaluator.setBoard(node->board); // 노드의 보드 상태 설정
//     Value currentEval = evaluator.evaluate();
//     auto candidates = evaluator.getCandidates();

//     if (currentEval != 0) {
//         node->score = currentEval;
//         if (depth == 0 || currentEval >= 20000) { // 승리라고 판단하는 값 변경하기
//             winningPath.clear();
//             stack<shared_ptr<Node>> tmp = path;

//             while (!tmp.empty()) {
//                 winningPath.push_back(tmp.top()->move); // Pos 정보를 winningPath에 저장
//                 tmp.pop();
//             }
//             reverse(winningPath.begin(), winningPath.end());
//         }
//         return currentEval;
//     }

//     if (candidates.empty()) {
//         node->score = currentEval;
//         return currentEval;
//     }

//     if (maximizingPlayer) {
//         Value maxEval = -INF;
//         for (const auto& move : candidates) {
//             incrementNodesExplored();
//             evaluator.next(move); // 보드 상태 업데이트
//             auto newBoard = evaluator.getBoard(); // 현재 보드 상태 가져오기
//             auto childNode = tree.createNode(node->path, newBoard, move, 0, depth - 1);
//             tree.addNode(childNode);
//             path.push(childNode);

//             Value eval = alphaBeta(childNode, depth - 1, alpha, beta, !maximizingPlayer);
//             maxEval = std::max(maxEval, eval);
//             alpha = std::max(alpha, eval);
//             path.pop();
//             evaluator.prev(); // 이전 보드 상태로 되돌리기
//             if (beta <= alpha) break;
//         }
//         node->score = maxEval;
//         return maxEval;
//     } else {
//         Value minEval = INF;
//         for (const auto& move : candidates) {
//             incrementNodesExplored();
//             evaluator.next(move); // 보드 상태 업데이트
//             auto newBoard = evaluator.getBoard(); // 현재 보드 상태 가져오기
//             auto childNode = tree.createNode(node->path, newBoard, move, 0, depth - 1);
//             tree.addNode(childNode);
//             path.push(childNode);

//             Value eval = alphaBeta(childNode, depth - 1, alpha, beta, !maximizingPlayer);
//             minEval = std::min(minEval, eval);
//             beta = std::min(beta, eval);
//             path.pop();
//             evaluator.prev(); // 이전 보드 상태로 되돌리기
//             if (beta <= alpha) break;
//         }
//         node->score = minEval;
//         return minEval;
//     }
// }

// void Search::printWinningPath() {
//     std::cout << "Winning path size: " << winningPath.size() << std::endl;
//     std::cout << "Winning path: ";
//     for (const auto& pos : winningPath) {
//         std::cout << "[" << pos.getX() << ", " << (char)(pos.getY() + 64) << "] ";
//     }
//     std::cout << std::endl;
// }
