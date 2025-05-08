#pragma once

#include "../evaluate/evaluator.h"
#include "../tree/tree_manager.h"
#include "../test/test.h"
#include "search_win.h"
#include "search_monitor.h"
#include <limits>

class Search {

PRIVATE
    TreeManager treeManager;
    Color targetColor;
    SearchMonitor& monitor;

    Value alphaBetaLegacy(Board& board, int depth, int alpha, int beta, bool maximizingPlayer);
    Value alphaBeta(Board& board, int depth, Value alpha, Value beta, bool isMax);
    int ids(Board& board, int depthLimit);
    bool isGameOver(Board& board);
    bool isTargetTurn();

PUBLIC
    Search(Board& board, SearchMonitor& monitor);
    Pos findBestMove();
    Pos iterativeDeepeningSearch();
    Pos findNextMove(Board board);
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

Value Search::alphaBeta(Board& board, int depth, Value alpha, Value beta, bool isMax) {
    // 종료 조건
    // 후보 산출
    /*
    이동 
        1. max 경우: 
            [모든 후보지에게 반복]
            이동 (move)
            eval = 새 알파베타 재귀 호출 (depth -1, 알파, 베타)
            뒤로가기 (undo)
            maxEval = maxEval과 eval 중 큰값으로 설정
            알파 = 알파와 eval중 큰값으로 설정
            만약 베타가 알파보다 작으면 break

            최종적으로 다 이동하면 maxEval을 반환
        2. min 경우:
    */
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
        int moveValue = alphaBeta(treeManager.getBoard(), monitor.getMaxDepth() - 1, MIN_VALUE, MAX_VALUE, false);
        treeManager.undo();
        monitor.getBestPath().pop_back();

        if (moveValue > bestValue) {
            bestValue = moveValue;
            bestMove = move;
        }
    }

    return bestMove;
}

Pos Search::findNextMove(Board board) {
    if (board.getResult() != ONGOING) return Pos();

    Evaluator evaluator(board);
    Pos sureMove = evaluator.getSureMove();
    if (!sureMove.isDefault()) {
        return sureMove;
    }

    SearchMonitor vcfMonitor;
    SearchWin vcfSearcher(board, vcfMonitor);

    double lastTriggerTime = 0.0;
    vcfMonitor.setTrigger([&lastTriggerTime](SearchMonitor& monitor) {
        if (monitor.getElapsedTime() - lastTriggerTime >= 5.0) {
            return true;
        }
        return false;
    });
    vcfMonitor.setSearchListener([&vcfSearcher](SearchMonitor& monitor) {
        vcfSearcher.stop();
    });

    if (vcfSearcher.findVCF()) {
        return vcfMonitor.getBestPath()[board.getPath().size()];
    }

    if (evaluator.isOppoMateExist()) {
        MoveList defends = evaluator.getThreatDefend();
        MoveList candidates;
        for (auto move : defends) {
            board.move(move);
            Board tmpBoard = board;
            SearchMonitor vctMonitor;
            SearchWin vctSearcher(tmpBoard, vctMonitor);

            lastTriggerTime = 0.0;
            vcfMonitor.setTrigger([&lastTriggerTime](SearchMonitor& monitor) {
                if (monitor.getElapsedTime() - lastTriggerTime >= 5.0) {
                    return true;
                }
                return false;
            });
            vcfMonitor.setSearchListener([&vcfSearcher](SearchMonitor& monitor) {
                vcfSearcher.stop();
            });

            if (!vctSearcher.findVCT()) {
                candidates.push_back(move);
            }
            board.undo();
        }
        if (candidates.empty()) {
            return defends.front();
        }
        else {
            return candidates.front();
        }
    }

    SearchMonitor vctMonitor;
    SearchWin vctSearcher(board, vctMonitor);

    lastTriggerTime = 0.0;
    vcfMonitor.setTrigger([&lastTriggerTime](SearchMonitor& monitor) {
        if (monitor.getElapsedTime() - lastTriggerTime >= 5.0) {
            return true;
        }
        return false;
    });
    vcfMonitor.setSearchListener([&vcfSearcher](SearchMonitor& monitor) {
        vcfSearcher.stop();
    });

    if (vctSearcher.findVCT()) {
        return vctMonitor.getBestPath()[board.getPath().size()];
    }

    MoveList candidates = evaluator.getCandidates();
    if (!candidates.empty())
        return candidates.front();
    
    return Pos();
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