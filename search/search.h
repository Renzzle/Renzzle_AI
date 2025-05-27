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

    MoveList alphaBeta(int depth);
    Value evaluateNode(Evaluator& evaluator);
    MoveList getCandidates(Evaluator& evaluator, bool isMax);
    void sortChildNodes(MoveList& moves, bool isTarget);
    void updateParent(stack<ABPNode>& stk, Value val);
    MoveList getBestPathFromRoot();
    bool isGameOver(Board& board);
    bool isTargetTurn();

PUBLIC
    Search(Board& board, SearchMonitor& monitor);

};

Search::Search(Board& board, SearchMonitor& monitor) : treeManager(board), monitor(monitor) {
    targetColor = board.isBlackTurn() ? COLOR_BLACK : COLOR_WHITE;
}

MoveList Search::alphaBeta(int depth) {
    stack<ABPNode> stk;
    stk.push({depth, true, MIN_VALUE, MAX_VALUE, 0, {}});

    while (!stk.empty()) {
        ABPNode &cur = stk.top();
        Node* currentNode = treeManager.getNode();
        // printBoard(currentNode->board);
        // printPath(currentNode->board.getPath());
        // TEST_PRINT("depth: " << cur.depth << " isMax: " << cur.isMax);

        // calculate child nodes
        if (cur.childMoves.empty()) {
            Evaluator evaluator(currentNode->board);
            cur.childMoves = getCandidates(evaluator, cur.isMax);
            sortChildNodes(cur.childMoves, cur.isMax);
        }
        
        // if current node is leaf node
        if (cur.depth == 0 || 
            isGameOver(currentNode->board) || 
            cur.childMoves.empty()) {
            // evaluate leaf node value
            Evaluator evaluator(currentNode->board);
            Value val = evaluateNode(evaluator);
            //TEST_PRINT("leaf node original value: " << val);
            if (!cur.isMax) val *= -1;
            currentNode->value = val;
            //TEST_PRINT("leaf node value: " << val);

            treeManager.undo();
            stk.pop();

            if (!stk.empty()) {
                updateParent(stk, val);
            }

            continue;
        }

        // TEST_PRINT("<Candidates>");
        // printPath(cur.childMoves);
        // TEST_PRINT("total childs: " << cur.childMoves.size() << " child index: " << cur.childIdx);
        // TEST_STOP();

        if (cur.childIdx < cur.childMoves.size()) {
            Pos move = cur.childMoves[cur.childIdx++];
            treeManager.move(move);
            stk.push({cur.depth - 1, !cur.isMax, cur.alpha, cur.beta, 0, {}});
        } else { // if search every child nodes
            Value result = cur.isMax ? cur.alpha : cur.beta;
            currentNode->value = result;
            
            treeManager.undo();
            stk.pop();

            if (!stk.empty()) {
                updateParent(stk, result);
            }
        }
    }

    return getBestPathFromRoot();
}

void Search::updateParent(stack<ABPNode>& stk, Value val) {
    //TEST_PRINT("in updateParent method.");
    ABPNode& parent = stk.top();
    Node* parentNode = treeManager.getNode();
    Pos lastMove;
    if (parent.childIdx > 0 && parent.childIdx - 1 < parent.childMoves.size()) {
        lastMove = parent.childMoves[parent.childIdx - 1];
    }

    if (parent.isMax) {
        if (val > parent.alpha) {
            parent.alpha = val;
            parentNode->value = val;
            parentNode->bestMove = lastMove;
            // TEST_PRINT("update best: ");
            // printPos(parentNode->bestMove);
        }
    } else {
        if (val < parent.beta) {
            parent.beta = val;
            parentNode->value = val;
            parentNode->bestMove = lastMove;
            // TEST_PRINT("update best: ");
            // printPos(parentNode->bestMove);
        }
    }

    if (parent.beta <= parent.alpha) {
        //TEST_PRINT("\npruning -> " << "alpha: " << parent.alpha << " beta: " << parent.beta);
        // pruning
        treeManager.undo();
        stk.pop();
    }
}

MoveList Search::getBestPathFromRoot() {
    MoveList path;
    Node* node = treeManager.getNode();
    TEST_PRINT(node->value);

    while (node != nullptr && !node->bestMove.isDefault()) {
        path.push_back(node->bestMove);
        treeManager.move(node->bestMove);
        node = treeManager.getNode();
    }

    return path;
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
    }
    sortChildNodes(moves, isMax);
    return moves;
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