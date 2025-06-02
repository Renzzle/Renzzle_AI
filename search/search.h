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
    stk.push({depth, true, Value(MIN_VALUE), Value(MAX_VALUE + 1), Value(), {}});

    while (!stk.empty()) {
        monitor.updateElapsedTime();
        ABPNode &cur = stk.top();
        Node* currentNode = treeManager.getNode();

        // if already searched
        if (currentNode->searchedDepth >= cur.depth && 
            currentNode->value.getType() != Value::Type::UNKNOWN) {
            Value cv = currentNode->value;
            bool cut = false;

            if (currentNode->value.getType() == Value::Type::EXACT) {
                cut = true;
            } else if (currentNode->value.getType() == Value::Type::LOWER_BOUND) {
                if (cv >= cur.beta) cut = true;
                else cur.alpha = cur.alpha > cv ? cur.alpha : cv;
            } else if (currentNode->value.getType() == Value::Type::UPPER_BOUND) {
                if (cv <= cur.alpha) cut = true;
                else cur.beta = cur.beta < cv ? cur.beta : cv;
            }

            if (cut) {
                treeManager.undo();
                stk.pop();

                if (!stk.empty()) {
                    updateParent(stk, cv);
                }

                continue;
            }
        }

        // calculate child nodes & initialize best value
        if (cur.childMoves.empty()) { // when first visit
            Evaluator evaluator(currentNode->board);
            cur.childMoves = getCandidates(evaluator, cur.isMax);
            cur.bestVal = cur.isMax ? Value(MIN_VALUE - 1) : Value(MAX_VALUE + 1);
        }
        
        // if current node is leaf node
        if (cur.depth == 0 || 
            isGameOver(currentNode->board) || 
            cur.childMoves.empty()) {
            // evaluate leaf node value
            Evaluator evaluator(currentNode->board);
            Value val = evaluateNode(evaluator);
            if (!cur.isMax) val.invert();
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
            currentNode->searchedDepth = cur.depth;
            currentNode->value = cur.bestVal;
            currentNode->value.setType(Value::Type::EXACT);
            
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
        parentNode->value.setType(parent.isMax ? Value::Type::LOWER_BOUND : Value::Type::UPPER_BOUND);
        parentNode->searchedDepth = parent.depth;

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
        MoveList fours = evaluator.getFours();
        moves.insert(moves.end(), fours.begin(), fours.end());
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
        
        if (result.isWin()) break;
        monitor.incDepth(2);
    }
}