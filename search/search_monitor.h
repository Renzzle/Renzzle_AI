#pragma once

#include "../game/board.h"
#include "../tree/tree_manager.h"
#include "../test/test.h"
#include <chrono>
#include <functional>

using Timestamp = chrono::time_point<chrono::high_resolution_clock>;

class SearchMonitor {

PRIVATE
    Timestamp startTime;
    double elapsedTime;
    MoveList bestPath;
    Value bestValue;
    int depth;
    int maxDepth;
    size_t visitCnt;
    function<bool(SearchMonitor&)> trigger;
    function<void(SearchMonitor&)> searchListener;

PUBLIC
    SearchMonitor();

    void initStartTime() { startTime = chrono::high_resolution_clock::now(); };
    void setTrigger(function<bool(SearchMonitor&)> newTrigger) { trigger = newTrigger; };
    void setSearchListener(function<void(SearchMonitor&)> newSearchListener) { searchListener = newSearchListener; };
    void executeTrigger();
    
    // update data function, executeTrigger function execute
    void updateElapsedTime();
    void incDepth() { depth++; executeTrigger(); };
    void decDepth() { depth--; executeTrigger(); };
    void incVisitCnt() { visitCnt++; executeTrigger(); };
    void updateMaxDepth(int maxDepth) {
        if (this->maxDepth < maxDepth)
            this->maxDepth = maxDepth;
        executeTrigger();
    }
    void setBestPath(MoveList path) { bestPath = path; executeTrigger(); };

    // getter
    double getElapsedTime() { return elapsedTime; }
    int getDepth() { return depth; }
    int getMaxDepth() { return maxDepth; }
    size_t getVisitCnt() { return visitCnt; }
    MoveList getBestPath() { return bestPath; }
    Value getBestValue() { return bestValue; }

};

SearchMonitor::SearchMonitor() {
    elapsedTime = 0.0;
    bestValue = INITIAL_VALUE;
    depth = 0;
    maxDepth = 0;
    visitCnt = 0;

    trigger = [](SearchMonitor monitor) { return false; };
    searchListener = [](SearchMonitor monitor) { return; };
}

void SearchMonitor::updateElapsedTime() {
    Timestamp now = chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(now - startTime);
    elapsedTime = duration.count() / 1e9;
    executeTrigger();
}

void SearchMonitor::executeTrigger() {
    if(trigger(*this)) {
        searchListener(*this);
    }
}