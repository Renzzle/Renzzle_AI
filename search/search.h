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
    Value bestVal;
    int childIdx;
    MoveList childMoves;
};

class Search {

PRIVATE
    TreeManager treeManager;
    Color targetColor;
    SearchMonitor& monitor;

    Value abp(int depth);
    Value evaluateNode(Evaluator& evaluator);
    MoveList getCandidates(Evaluator& evaluator, bool isMax);
    void sortChildNodes(MoveList& moves, bool isTarget);
    void updateParent(stack<ABPNode>& stk, Value val);
    bool isGameOver(Board& board);

PUBLIC
    Search(Board& board, SearchMonitor& monitor);
    void ids();

};

Search::Search(Board& board, SearchMonitor& monitor) : treeManager(board), monitor(monitor) {
    targetColor = board.isBlackTurn() ? COLOR_BLACK : COLOR_WHITE;
    monitor.setBestLineProvider([this](int i) {
        return treeManager.getBestLine(i);
    });
}

Value Search::abp(int depth) {
    stack<ABPNode> stk;
    stk.push({depth, true, MIN_VALUE - 1, MAX_VALUE + 1, 0, {}});

    while (!stk.empty()) {
        monitor.updateElapsedTime();
        ABPNode &cur = stk.top();
        Node* currentNode = treeManager.getNode();

        // calculate child nodes & initialize best value
        if (cur.childMoves.empty()) { // when first visit
            Evaluator evaluator(currentNode->board);
            cur.childMoves = getCandidates(evaluator, cur.isMax);
            cur.bestVal = cur.isMax ? MIN_VALUE - 1 : MAX_VALUE + 1;
        }
        
        // if current node is leaf node
        if (cur.depth == 0 || 
            isGameOver(currentNode->board) || 
            cur.childMoves.empty()) {
            // evaluate leaf node value
            Evaluator evaluator(currentNode->board);
            Value val = evaluateNode(evaluator);
            if (!cur.isMax) val *= -1;
            currentNode->value = val;

            treeManager.undo();
            stk.pop();

            if (!stk.empty()) {
                updateParent(stk, val);
            }

            continue;
        }

        if (cur.childIdx < cur.childMoves.size()) {
            Pos move = cur.childMoves[cur.childIdx++];
            treeManager.move(move);
            stk.push({cur.depth - 1, !cur.isMax, cur.alpha, cur.beta, 0, {}});
            monitor.incVisitCnt();
        } else { // if search every child nodes
            currentNode->value = cur.bestVal;
            
            treeManager.undo();
            stk.pop();

            if (!stk.empty()) {
                updateParent(stk, cur.bestVal);
            }
        }
    }

    return treeManager.getNode()->value;
}

void Search::updateParent(stack<ABPNode>& stk, Value val) {
    ABPNode& parent = stk.top();
    Node* parentNode = treeManager.getNode();
    Pos lastMove;
    if (parent.childIdx > 0 && parent.childIdx - 1 < parent.childMoves.size()) {
        lastMove = parent.childMoves[parent.childIdx - 1];
    }

    if (parent.isMax) {
        if (val > parent.bestVal) {
            parent.bestVal = val;
            parentNode->value = val;
            parentNode->bestMove = lastMove;
        }
        if (val > parent.alpha) {
            parent.alpha = val;
        }
    } else {
        if (val < parent.bestVal) {
            parent.bestVal = val;
            parentNode->value = val;
            parentNode->bestMove = lastMove;
        }
        if (val < parent.beta) {
            parent.beta = val;
        }
    }

    if (parent.beta <= parent.alpha) {
        // pruning
        treeManager.undo();
        stk.pop();
    }
}

Value Search::evaluateNode(Evaluator& evaluator) {
    treeManager.getNode()->value = evaluator.evaluate();
    return treeManager.getNode()->value;
}

MoveList Search::getCandidates(Evaluator& evaluator, bool isMax) {
    MoveList moves;
    
    Pos sureMove = evaluator.getSureMove();
    if (!sureMove.isDefault()) {
        moves.push_back(sureMove);
        return moves;
    }

    if (isMax) {
        moves = evaluator.getThreats();
    } else {
        moves = evaluator.getThreatDefend();
        moves.emplace_back(evaluator.getFours());
    }
    sortChildNodes(moves, isMax);
    return moves;
}

void Search::sortChildNodes(MoveList& moves, bool isTarget) {
    if (!treeManager.getNode()->childNodes.empty()) {
        sort(moves.begin(), moves.end(), [&](const Pos& a, const Pos& b) {
            Node* aNode = treeManager.getChildNode(a);
            Node* bNode = treeManager.getChildNode(b);

            bool aIsNull = (aNode == nullptr);
            bool bIsNull = (bNode == nullptr);

            if (aIsNull && bIsNull) {
                return false;
            }
            if (aIsNull) {
                return false;
            }
            if (bIsNull) {
                return true;
            }

            if(isTarget) return aNode->value > bNode->value;
            else return aNode->value < bNode->value;
        });
    }   
}

bool Search::isGameOver(Board& board) {
    Result result = board.getResult();
    return result != ONGOING;
}

void Search::ids() {
    monitor.incDepth(5);
    monitor.initStartTime();

    while (true) {
        Value result = abp(monitor.getDepth());
        monitor.setBestPath(treeManager.getBestLine(0));
        
        if (result == MAX_VALUE) break;
        monitor.incDepth(2);
    }
}