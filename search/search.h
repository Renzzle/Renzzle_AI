#pragma once

#include "../evaluate/evaluator.h"
#include "../tree/tree_manager.h"
#include "../test/test.h"
#include "search_win.h"
#include "search_monitor.h"
#include "../test/util.h"
#include <limits>
#include <stack>

struct ABPNode {
    int depth;
    bool isMax;
    Value alpha;
    Value beta;
    int childIdx;
    MoveList childMoves;
};

class Search {

PRIVATE
    TreeManager treeManager;
    Color targetColor;
    SearchMonitor& monitor;

    Value alphaBetaLegacy(Board& board, int depth, int alpha, int beta, bool maximizingPlayer);
    int ids(Board& board, int depthLimit);
    bool isGameOver(Board& board);
    bool isTargetTurn();

    MoveList alphaBeta(int depth);
    Value evaluateNode(Evaluator& evaluator);
    MoveList getCandidates(Evaluator& evaluator, bool isMax);
    void sortChildNodes(MoveList& moves, bool isTarget);
    void pruning(stack<ABPNode>& stk, Value val, const Pos& move);
    MoveList getBestPathFromRoot();

PUBLIC
    Search(Board& board, SearchMonitor& monitor);
    Pos findBestMove();
    Pos iterativeDeepeningSearch();
    MoveList getPath();
    MoveList getSimulatedPath();

};

Search::Search(Board& board, SearchMonitor& monitor) : treeManager(board), monitor(monitor) {
    targetColor = board.isBlackTurn() ? COLOR_BLACK : COLOR_WHITE;
}

Value Search::alphaBetaLegacy(Board& board, int depth, int alpha, int beta, bool maximizingPlayer) {
    monitor.incVisitCnt();

    Evaluator evaluator(treeManager.getBoard());

    // end condition
    if (depth <= 0 || isGameOver(board)) {
        Value val = evaluator.evaluate();
        if (!maximizingPlayer) {
            val = -val;
        }
        if (val > monitor.getBestValue()) {
            monitor.setBestValue(val);
            monitor.setBestPath(treeManager.getBoard().getPath());
        }
        return val;
    }

    MoveList moves = evaluator.getCandidates();

    if (moves.empty()) return evaluator.evaluate();

    if (maximizingPlayer) {
        int maxEval = MIN_VALUE;

        for (Pos move : moves) {
            treeManager.move(move);
            Value eval = alphaBetaLegacy(treeManager.getBoard(), depth - 1, alpha, beta, false);
            treeManager.undo();
            maxEval = max(maxEval, eval);
            alpha = max(alpha, eval);
            if (beta <= alpha) break;
        }

        return maxEval;
    } else {
        int minEval = MAX_VALUE;

        for (Pos move : moves) {
            treeManager.move(move);
            Value eval = alphaBetaLegacy(treeManager.getBoard(), depth - 1, alpha, beta, true);
            treeManager.undo();
            minEval = min(minEval, eval);
            beta = min(beta, eval);
            if (beta <= alpha) break;
        }

        return minEval;
    }
}

// MoveList Search::alphaBeta(int depth) {
//     stack<ABPNode> stk;
//     MoveList bestPath;

//     stk.push(ABPNode{depth, true, MIN_VALUE, MAX_VALUE, 0});

//     while (!stk.empty()) {
//         ABPNode &cur = stk.top();
//         Evaluator evaluator(treeManager.getBoard());

//         // when leaf node
//         if (cur.depth == 0 || isGameOver(treeManager.getBoard())) {
//             Value val = evaluateNode(evaluator);
//             if (!cur.isMax) val *= -1;

//             treeManager.undo();
//             stk.pop();
            
//             pruning(stk, cur, val);
//             continue;
//         }

//         cur.childMoves = getCandidates(evaluator, cur.isMax);

//         if (cur.childIdx < cur.childMoves.size()) {
//             Pos move = cur.childMoves[cur.childIdx++];
//             treeManager.move(move);
//             stk.push(ABPNode{cur.depth - 1, !cur.isMax, cur.alpha, cur.beta, 0});
//         } else { // when search all child nodes
//             Value result = cur.isMax ? cur.alpha : cur.beta;
//             treeManager.getNode()->value = result;

//             treeManager.undo();
//             stk.pop();

//             pruning(stk, cur, result);
//         }
//     }

//     return bestPath;
// }

MoveList Search::alphaBeta(int depth) {
    stack<ABPNode> stk;
    stk.push({depth, true, MIN_VALUE, MAX_VALUE, 0, {}});

    while (!stk.empty()) {
        ABPNode &cur = stk.top();
        Node* currentNode = treeManager.getNode();
        printBoard(currentNode->board);
        TEST_PRINT(cur.depth << ", " << cur.isMax << ", " << cur.childMoves.size());

        if (cur.depth == 0 || isGameOver(currentNode->board)) {
            Evaluator evaluator(currentNode->board);
            Value val = evaluateNode(evaluator);
            if (!cur.isMax) val *= -1;
            currentNode->value = val;

            treeManager.undo();
            Pos lastMove = (cur.childIdx > 0 ? cur.childMoves[cur.childIdx - 1] : Pos());
            stk.pop();

            if (!stk.empty()) {
                pruning(stk, val, lastMove);
            }

            continue;
        }

        if (cur.childMoves.empty()) {
            TEST_PRINT("is child empty");
            Evaluator evaluator(currentNode->board);
            cur.childMoves = getCandidates(evaluator, cur.isMax);
            sortChildNodes(cur.childMoves, cur.isMax);
        }

        if (cur.childIdx < cur.childMoves.size()) {
            TEST_PRINT("go to next move");
            Pos move = cur.childMoves[cur.childIdx++];
            treeManager.move(move);
            stk.push({cur.depth - 1, !cur.isMax, cur.alpha, cur.beta, 0, {}});
        } else {
            TEST_PRINT("go to pruning");
            Value result = cur.isMax ? cur.alpha : cur.beta;
            currentNode->value = result;
            treeManager.undo();
            Pos lastMove = (cur.childIdx > 0 ? cur.childMoves[cur.childIdx - 1] : Pos());
            stk.pop();

            if (!stk.empty()) {
                pruning(stk, result, lastMove);
            }
        }
    }

    return getBestPathFromRoot();
}

MoveList Search::getBestPathFromRoot() {
    MoveList path;
    Node* node = treeManager.getNode();

    while (!node->bestMove.isDefault()) {
        path.push_back(node->bestMove);
        node = treeManager.getChildNode(node->bestMove);
    }
    return path;
}


Value Search::evaluateNode(Evaluator& evaluator) {
    treeManager.getNode()->value = evaluator.evaluate();
    return treeManager.getNode()->value;
}

MoveList Search::getCandidates(Evaluator& evaluator, bool isMax) {
    MoveList moves;
    if (isMax) {
        moves = evaluator.getThreats();
    } else {
        moves = evaluator.getThreatDefend();
    }
    sortChildNodes(moves, isMax);
    return moves;
}

void Search::pruning(stack<ABPNode>& stk, Value val, const Pos& move) {
    ABPNode& parent = stk.top();
    Node* parentNode = treeManager.getNode();

    if (parent.isMax) {
        if (val > parent.alpha) {
            parent.alpha = val;
            parentNode->value = val;
            parentNode->bestMove = move;
        }
    } else {
        if (val < parent.beta) {
            parent.beta = val;
            parentNode->value = val * -1;
            parentNode->bestMove = move;
        }
    }

    if (parent.beta <= parent.alpha) {
        stk.pop();  // pruning
    }
}


void Search::sortChildNodes(MoveList& moves, bool isTarget) {
    if (!treeManager.getNode()->childNodes.empty()) {
        sort(moves.begin(), moves.end(), [&](const Pos& a, const Pos& b) {
            Node* aNode = treeManager.getChildNode(a);
            Node* bNode = treeManager.getChildNode(b);
            if (aNode == nullptr || bNode == nullptr) return true;

            if(isTarget) return aNode->value > bNode->value;
            else return aNode->value < bNode->value;
        });
    }   
}

int Search::ids(Board& board, int depthLimit) {
    Pos bestMove;
    for (int depth = 1; depth <= depthLimit; depth++) {
        bestMove = findBestMove();
    }
    Evaluator evaluator(treeManager.getBoard());
    return evaluator.evaluate();
}

bool Search::isGameOver(Board& board) {
    Result result = board.getResult();
    return result != ONGOING;
}

bool Search::isTargetTurn() {
    if (treeManager.getBoard().isBlackTurn()) {
        return targetColor == COLOR_BLACK;
    } else {
        return targetColor == COLOR_WHITE;
    }
}

Pos Search::findBestMove() {
    monitor.getBestPath().clear();

    int bestValue = MIN_VALUE;
    Pos bestMove;

    Evaluator evaluator(treeManager.getBoard());
    vector<Pos> moves = evaluator.getCandidates();

    for (Pos move : moves) {
        treeManager.move(move);
        monitor.getBestPath().push_back(move);
        int moveValue = alphaBetaLegacy(treeManager.getBoard(), monitor.getMaxDepth() - 1, MIN_VALUE, MAX_VALUE, false);
        treeManager.undo();
        monitor.getBestPath().pop_back();

        if (moveValue > bestValue) {
            bestValue = moveValue;
            bestMove = move;
        }
    }

    return bestMove;
}

Pos Search::iterativeDeepeningSearch() {
    int depthLimit = monitor.getMaxDepth();
    return findBestMove();
}

MoveList Search::getPath() {
    return monitor.getBestPath();
}


/*

#include <vector>
#include <stack>
#include <iostream>
#include <limits>

constexpr int INF = std::numeric_limits<int>::max();
constexpr int NEG_INF = std::numeric_limits<int>::min();

struct Node {
    int depth;
    bool maximizingPlayer;
    int alpha;
    int beta;
    int boardState;  // 예제용: 실제 오목 엔진이라면 보드 전체 상태가 들어가야 함
    int childIndex;  // 몇 번째 자식을 보는 중인지 기록
};

// 예제용: 어떤 상태에서 가능한 다음 상태를 반환하는 함수
std::vector<int> generateMoves(int boardState) {
    return {boardState + 1, boardState + 2};  // 더미
}

// 예제용: 리프 노드 평가 함수
int evaluate(int boardState) {
    return boardState;  // 더미
}

int alphaBetaIterative(int rootState, int maxDepth) {
    std::stack<Node> stk;
    int bestValue = NEG_INF;

    stk.push(Node{maxDepth, true, NEG_INF, INF, rootState, 0});

    while (!stk.empty()) {
        Node &curr = stk.top();

        // 리프 노드이거나 깊이 0이면 평가
        if (curr.depth == 0 || generateMoves(curr.boardState).empty()) {
            int val = evaluate(curr.boardState);
            stk.pop();

            if (!stk.empty()) {
                Node &parent = stk.top();
                if (parent.maximizingPlayer) {
                    parent.alpha = std::max(parent.alpha, val);
                } else {
                    parent.beta = std::min(parent.beta, val);
                }

                // Alpha-Beta 컷
                if (parent.beta <= parent.alpha) {
                    stk.pop();  // 부모 노드도 중단
                }
            } else {
                bestValue = val;
            }
            continue;
        }

        // 자식 노드 생성
        std::vector<int> moves = generateMoves(curr.boardState);

        if (curr.childIndex < moves.size()) {
            int nextState = moves[curr.childIndex++];
            stk.push(Node{curr.depth - 1, !curr.maximizingPlayer, curr.alpha, curr.beta, nextState, 0});
        } else {
            // 모든 자식을 탐색 완료했으면 결과 계산
            int result = curr.maximizingPlayer ? curr.alpha : curr.beta;
            stk.pop();

            if (!stk.empty()) {
                Node &parent = stk.top();
                if (parent.maximizingPlayer) {
                    parent.alpha = std::max(parent.alpha, result);
                } else {
                    parent.beta = std::min(parent.beta, result);
                }

                // Alpha-Beta 컷
                if (parent.beta <= parent.alpha) {
                    stk.pop();  // 부모 노드도 중단
                }
            } else {
                bestValue = result;
            }
        }
    }

    return bestValue;
}

int main() {
    int result = alphaBetaIterative(0, 4);
    std::cout << "Best value: " << result << std::endl;
}

*/