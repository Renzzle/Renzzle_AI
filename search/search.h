#pragma once

#include "../evaluate/evaluator.h"
#include "../tree/tree_manager.h"
#include "../test/test.h"
#include "search_win.h"
#include "search_monitor.h"
#include "../test/util.h"
#include <limits>
#include <stack>

enum class SearchMode {
    FULL_WINDOW,
    NULL_WINDOW
};

struct ABPNode {
    int depth;
    bool isMax;
    Value alpha;
    Value beta;
    Value bestVal;
    int childIdx;
    MoveList childMoves;
    SearchMode searchMode;
};

class Search {

PRIVATE
    TreeManager treeManager;
    Color targetColor;
    SearchMonitor& monitor;

    Value abp(int depth);
    Value evaluateNode(Evaluator& evaluator);
    MoveList getCandidates(Evaluator& evaluator, bool isMax);
    bool cutOffSearchedNode(stack<ABPNode>& stk);
    void searchNextNode(stack<ABPNode>& stk);
    void sortChildNodes(MoveList& moves, bool isTarget);
    void updateParent(stack<ABPNode>& stk, Value childValue, SearchMode childMode);
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
    monitor.setBestValueProvider([this]() {
        return treeManager.getRootNode()->value;
    });
}

bool Search::cutOffSearchedNode(stack<ABPNode>& stk) {
    ABPNode &cur = stk.top();
    Node* currentNode = treeManager.getNode();
    bool cut = false;

    if (currentNode->searchedDepth >= cur.depth && 
        currentNode->value.getValue() != INITIAL_VALUE && 
        currentNode->value.getType() != Value::Type::UNKNOWN && 
        cur.childIdx == 0) {
        Value cv = currentNode->value;

        if (currentNode->value.getType() == Value::Type::EXACT) {
            cut = true;
        } else if (currentNode->value.getType() == Value::Type::LOWER_BOUND) {
            if (cv >= cur.beta) cut = true;
        } else if (currentNode->value.getType() == Value::Type::UPPER_BOUND) {
            if (cv <= cur.alpha) cut = true;
        }

        if (cut) {
            treeManager.undo();
            stk.pop();

            if (!stk.empty()) {
                updateParent(stk, cv, cur.searchMode);
            }
        }
    }

    return cut;
}

void Search::searchNextNode(stack<ABPNode>& stk) {
    ABPNode &cur = stk.top();

    Pos move;
    Node* child = nullptr;

    // cut off if already searched win node and child node searchedDepth >= cur.depth - 2
    if (cur.isMax && cur.alpha.getResult() == Value::Result::WIN && cur.alpha.getType() == Value::Type::EXACT) {
        // if (treeManager.currentNode == treeManager.rootNode) {
        //     printBoard(treeManager.getBoard());
        //     TEST_PRINT("childIdx: " << cur.childIdx);
        // }
        while (cur.childIdx < cur.childMoves.size()) {
            move = cur.childMoves[cur.childIdx++];
            child = treeManager.getChildNode(move);

            if (child == nullptr || child->searchedDepth < cur.alpha.resultDepth - 3) {
                break;
            }
        }
        // if (treeManager.currentNode == treeManager.rootNode) {
        //     TEST_PRINT("childIdx: " << cur.childIdx);
        // }
        if (child != nullptr && child->searchedDepth >= cur.alpha.resultDepth - 3) {
            return;
        }
    } else {
        move = cur.childMoves[cur.childIdx++];
    }

    //move = cur.childMoves[cur.childIdx++];
    treeManager.move(move);

    Value nextAlpha = cur.alpha;
    Value nextBeta = cur.beta;
    nextAlpha.decreaseResultDepth();
    nextBeta.decreaseResultDepth();

    if (cur.searchMode == SearchMode::NULL_WINDOW) { // null window search in progress
        stk.push({cur.depth - 1, !cur.isMax, nextAlpha, nextBeta, Value(), 0, {}, SearchMode::NULL_WINDOW});
    } else if (cur.childIdx > 1) { // start new null window search
        if (cur.isMax) {
            nextBeta = nextAlpha;
            nextBeta += 1;
        } else {
            nextAlpha = nextBeta;
            nextAlpha -= 1;
        }
        stk.push({cur.depth - 1, !cur.isMax, nextAlpha, nextBeta, Value(), 0, {}, SearchMode::NULL_WINDOW});
    } else {
        stk.push({cur.depth - 1, !cur.isMax, nextAlpha, nextBeta, Value(), 0, {}, SearchMode::FULL_WINDOW});
    }

    monitor.incVisitCnt();
}

void Search::updateParent(stack<ABPNode>& stk, Value childValue, SearchMode childMode) {
    ABPNode& parent = stk.top();
    Node* parentNode = treeManager.getNode();
    Pos lastMove;
    if (parent.childIdx > 0 && parent.childIdx - 1 < parent.childMoves.size()) {
        lastMove = parent.childMoves[parent.childIdx - 1];
    }
    childValue.increaseResultDepth();

    // if null window search failed, re-search with full window
    if (parent.searchMode == SearchMode::FULL_WINDOW && childMode == SearchMode::NULL_WINDOW) {
        Value nextAlpha = parent.alpha;
        Value nextBeta = parent.beta;
        nextAlpha.decreaseResultDepth();
        nextBeta.decreaseResultDepth();

        stk.push({parent.depth - 1, !parent.isMax, nextAlpha, nextBeta, Value(), 0, {}, SearchMode::FULL_WINDOW});
        Pos move = parent.childMoves[parent.childIdx - 1];
        treeManager.move(move);
        return;
    }

    if (parent.isMax) {
        if (childValue > parent.bestVal && childValue.getType() == Value::Type::EXACT) {
            parent.bestVal = childValue;
            parentNode->value = childValue;
            parentNode->bestMove = lastMove;
        }
        if (childValue > parent.alpha) {
            parent.alpha = childValue;
        }
    } else {
        if (childValue < parent.bestVal && childValue.getType() == Value::Type::EXACT) {
            parent.bestVal = childValue;
            parentNode->value = childValue;
            parentNode->bestMove = lastMove;
        }
        if (childValue < parent.beta) {
            parent.beta = childValue;
        }
    }

    if (parent.beta <= parent.alpha) {
        // pruning
        if (parent.bestVal.getType() == Value::Type::UNKNOWN) {
            parentNode->value = parent.isMax ? parent.alpha : parent.beta;
            parentNode->bestMove = Pos();
        } else {
            parentNode->value.setType(parent.isMax ? Value::Type::LOWER_BOUND : Value::Type::UPPER_BOUND);
        }
        parentNode->searchedDepth = parent.depth;

        treeManager.undo();
        stk.pop();
    }
}

Value Search::abp(int depth) {
    stack<ABPNode> stk;
    stk.push(ABPNode{depth, true, Value(MIN_VALUE, Value::Type::UNKNOWN), Value(MAX_VALUE + 1, Value::Type::UNKNOWN), Value(), 0, {}, SearchMode::FULL_WINDOW});

    while (!stk.empty()) {
        monitor.updateElapsedTime();
        ABPNode &cur = stk.top();
        Node* currentNode = treeManager.getNode();

        // if already searched
        if (cutOffSearchedNode(stk)) continue;

        // calculate child nodes & initialize best value
        if (cur.childMoves.empty()) { // when first visit
            Evaluator evaluator(currentNode->board);
            cur.childMoves = getCandidates(evaluator, cur.isMax);
            cur.bestVal = cur.isMax ? Value(MIN_VALUE, Value::Type::UNKNOWN) : Value(MAX_VALUE + 1, Value::Type::UNKNOWN);
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
                updateParent(stk, val, cur.searchMode);
            }

            continue;
        }

        if (cur.childIdx < cur.childMoves.size()) {
            searchNextNode(stk);
        } else { // if search every child nodes
            if (cur.bestVal.getType() == Value::Type::UNKNOWN) {
                cur.bestVal = cur.isMax ? cur.alpha : cur.beta;
                cur.bestVal.setType(cur.isMax ? Value::Type::UPPER_BOUND : Value::Type::LOWER_BOUND);
                currentNode->bestMove = Pos();
            }
            currentNode->searchedDepth = cur.depth;
            currentNode->value = cur.bestVal;
            
            treeManager.undo();
            stk.pop();

            if (!stk.empty()) {
                updateParent(stk, cur.bestVal, cur.searchMode);
            }
        }
    }

    return treeManager.getNode()->value;
}

Value Search::evaluateNode(Evaluator& evaluator) {
    return evaluator.evaluate();
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

void Search::sortChildNodes(MoveList& moves, bool isMax) {
    if (treeManager.getNode()->childNodes.empty()) {
        return;
    }
    Pos bestMove = treeManager.getNode()->bestMove;

    auto getValueTypePriority = [&](Value::Type type) {
        if (isMax) {
            switch (type) {
                case Value::Type::EXACT:       return 0;
                case Value::Type::LOWER_BOUND: return 1;
                case Value::Type::UNKNOWN:     return 2;
                case Value::Type::UPPER_BOUND: return 3;
                default: return 2;
            }
        } else { 
            switch (type) {
                case Value::Type::EXACT:       return 0;
                case Value::Type::UPPER_BOUND: return 1;
                case Value::Type::UNKNOWN:     return 2;
                case Value::Type::LOWER_BOUND: return 3;
                default: return 2;
            }
        }
    };

    sort(moves.begin(), moves.end(), [&](const Pos& a, const Pos& b) {
        if (!bestMove.isDefault()) {
            if (a == bestMove) return true;
            if (b == bestMove) return false;
        }

        Node* aNode = treeManager.getChildNode(a);
        Node* bNode = treeManager.getChildNode(b);

        bool aIsUnknown = (aNode == nullptr || aNode->value.getType() == Value::Type::UNKNOWN);
        bool bIsUnknown = (bNode == nullptr || bNode->value.getType() == Value::Type::UNKNOWN);

        if (aIsUnknown && bIsUnknown) return false;
        if (aIsUnknown) return false;
        if (bIsUnknown) return true;

        Value aValue = aNode->value;
        Value bValue = bNode->value;

        int aPriority = getValueTypePriority(aValue.getType());
        int bPriority = getValueTypePriority(bValue.getType());

        if (aPriority != bPriority) {
            return aPriority < bPriority;
        }

        if (isMax) {
            return aValue > bValue;
        } else {
            return aValue < bValue;
        }
    });
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

        if (result.isWin() && result.getResultDepth() <= monitor.getDepth()) 
            break;

        monitor.incDepth(2);
    }
}