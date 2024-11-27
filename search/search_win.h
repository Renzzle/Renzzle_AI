#pragma once

#include "../evaluate/evaluator.h"
#include "../tree/tree_manager.h"
#include "../test/test.h"
#include "search_monitor.h"

class SearchWin {

PRIVATE
    TreeManager treeManager;
    SearchMonitor& monitor;
    Color targetColor;
    Result targetResult;
    bool isInitTime = false;
    
    bool isWin();
    bool isTargetTurn();
    void visit(Pos& p);
    void sortChildNodes(MoveList& moves, bool isTarget);

PUBLIC
    SearchWin(Board& board, SearchMonitor& monitor);
    bool findVCF();
    bool findVCT();
    bool findVCT(int limit);

};

SearchWin::SearchWin(Board& board, SearchMonitor& monitor) : treeManager(board), monitor(monitor) {
    targetColor = board.isBlackTurn() ? COLOR_BLACK : COLOR_WHITE;
    targetResult = (targetColor == COLOR_BLACK) ? BLACK_WIN : WHITE_WIN;
}

bool SearchWin::findVCF() {
    if (!isInitTime) {
        monitor.initStartTime();
        isInitTime = true;
    }
    monitor.incVisitCnt();
    monitor.updateElapsedTime();
    if (isWin()) return true;
    if (treeManager.getBoard().getResult() != ONGOING) return false;
    
    // find candidates
    Evaluator evaluator(treeManager.getBoard());
    MoveList moves;

    if (isTargetTurn())
        moves = evaluator.getFours();
    else 
        moves = evaluator.getCandidates();

    // dfs
    for (auto move : moves) {
        shared_ptr<Node> childNode = treeManager.getChildNode(move);
        if (childNode != nullptr) { // child node exist
            // prune except win path
            if (childNode->result != targetResult) continue;
        } else {
            // if a winning path exists, skip searching other nodes
            if (treeManager.currentNode->result != ONGOING) continue;
        }

        treeManager.move(move);
        
        if (findVCF()) {
            // if find vcf, update parent node result
            Result result = treeManager.getNode()->result;
            treeManager.undo();
            treeManager.getNode()->result = result;
            return true;
        }
        
        treeManager.undo();
    }

    return false;
}

bool SearchWin::findVCT(int limit) {
    // update monitor info
    monitor.incVisitCnt();
    monitor.updateElapsedTime();

    // ending condtion
    if (isWin()) return true;
    if (limit == 0) return false;
    if (treeManager.getBoard().getResult() != ONGOING) return false;

    Evaluator evaluator(treeManager.getBoard());
    // if there is no threat, return false
    if (isTargetTurn() && evaluator.getThreats().empty()) return false;
    MoveList moves;

    // if only move, just move
    Pos sureMove = evaluator.getSureMove();
    if (!sureMove.isDefault()) {
        visit(sureMove);
        bool result = findVCT(limit - 1);
        treeManager.undo();
        return result;
    }

    // target turn
    if (isTargetTurn()) {
        moves = evaluator.getThreats();
        sortChildNodes(moves, true);

        int minVisitedCnt = INT32_MAX;
        for (const auto& pair : treeManager.getNode()->childNodes) {
            shared_ptr<Node> node = pair.second;
            if (node->visitedCnt < minVisitedCnt)
                minVisitedCnt = node->visitedCnt;
        }

        Value childVal;
        Value minVal = INT32_MAX;
        for (auto move : moves) {
            shared_ptr<Node> childNode = treeManager.getChildNode(move);
            if (childNode != nullptr && minVisitedCnt < childNode->visitedCnt) {
                if (childNode->result == targetResult) return true;
                else continue;
            }
            visit(move);
            childVal = treeManager.getNode()->actualValue;
            if (minVal > childVal) minVal = childVal;
            if (findVCT(limit - 1)) {
                treeManager.undo();
                treeManager.getNode()->actualValue = childVal - 1;
                treeManager.getNode()->result = targetResult;
                return true;
            }
            treeManager.undo();
        }
        treeManager.getNode()->actualValue = minVal * -1;
        return false;
    }
    // opponent turn 
    else {
        if (!evaluator.isOppoMateExist()) return false;
        SearchMonitor vcfMonitor;
        SearchWin vcfSearch(treeManager.getBoard(), vcfMonitor);
        if (vcfSearch.findVCF()) return false;

        MoveList defend = evaluator.getThreatDefend();
        moves.insert(moves.end(), defend.begin(), defend.end());
        MoveList fours = evaluator.getFours();
        moves.insert(moves.end(), fours.begin(), fours.end());
        sortChildNodes(moves, false);

        int minVisitedCnt = INT32_MAX;
        for (const auto& pair : treeManager.getNode()->childNodes) {
            shared_ptr<Node> node = pair.second;
            if (node->visitedCnt < minVisitedCnt)
                minVisitedCnt = node->visitedCnt;
        }
        
        Value childVal;
        for (auto move : moves) {
            shared_ptr<Node> childNode = treeManager.getChildNode(move);
            if (childNode != nullptr && minVisitedCnt < childNode->visitedCnt) {
                if (childNode->result != targetResult) return false;
                else continue;
            }
            visit(move);
            childVal = treeManager.getNode()->actualValue;
            if (!findVCT(limit - 1)) {
                treeManager.undo();
                treeManager.getNode()->actualValue = childVal;
                return false;
            }
            treeManager.undo();
        }
        treeManager.getNode()->actualValue = childVal - 1;
        treeManager.getNode()->result = targetResult;
        return true;
    }
}

bool SearchWin::findVCT() {
    // set monitor
    if (!isInitTime) {
        monitor.initStartTime();
        isInitTime = true;
    }

    for (int i = 3; i <= 31; i += 2) {
        if (findVCT(i)) return true;
        TEST_PRINT("Depth: " << i << ", Time: " << monitor.getElapsedTime() << "sec, Node: " << monitor.getVisitCnt());
        // for (const auto& pair : treeManager.getNode()->childNodes) {
        //     shared_ptr<Node> node = pair.second;
        //     printBoard(node->board);
        //     TEST_PRINT("Value: " << node->actualValue << " Visit: " << node->visitedCnt);
        // }
    }

    return false;
}

bool SearchWin::isWin() {
    Result result = treeManager.getBoard().getResult();;
    bool isWin = false;
    
    if (result == BLACK_WIN && targetColor == COLOR_BLACK)
        isWin = true;
    if (result == WHITE_WIN && targetColor == COLOR_WHITE)
        isWin = true;

    // if win, update current node result & set vcf path
    if (isWin) {
        treeManager.getNode()->result = result;
        monitor.setBestPath(treeManager.getNode()->board.getPath());
    }

    return isWin;
}

bool SearchWin::isTargetTurn() {
    if (treeManager.getBoard().isBlackTurn()) {
        if (targetColor == COLOR_BLACK) return true;
        else return false;
    } else {
        if (targetColor == COLOR_BLACK) return false;
        else return true;
    }
}

void SearchWin::visit(Pos& p) {
    treeManager.move(p);
    shared_ptr<Node> currentNode = treeManager.getNode();

    // initialize evaluated value
    if (currentNode->evaluatedValue == INITIAL_VALUE) {
        Evaluator evaluator(treeManager.getBoard());
        currentNode->evaluatedValue = evaluator.evaluate();
        if (currentNode->actualValue == INITIAL_VALUE) 
            currentNode->actualValue = currentNode->evaluatedValue;
    }

    currentNode->result = treeManager.getBoard().getResult();
    currentNode->visitedCnt++;
}

void SearchWin::sortChildNodes(MoveList& moves, bool isTarget) {
    if (!treeManager.getNode()->childNodes.empty()) {
        sort(moves.begin(), moves.end(), [&](const Pos& a, const Pos& b) {
            shared_ptr<Node> aNode = treeManager.getChildNode(a);
            shared_ptr<Node> bNode = treeManager.getChildNode(b);
            if (aNode == nullptr || bNode == nullptr) return true;

            if(isTarget) return aNode->actualValue > bNode->actualValue;
            else return aNode->actualValue < bNode->actualValue;
        });
    }   
}