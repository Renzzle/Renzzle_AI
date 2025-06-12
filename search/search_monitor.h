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
    MoveList bestPath; // final best path determined after one complete search iteration
    MoveList bestLine; // best line, updated when getBestLine() is explicitly called
    Value bestValue;
    int depth;
    size_t visitCnt;
    function<bool(SearchMonitor&)> trigger;
    function<void(SearchMonitor&)> searchListener;
    function<MoveList(int)> bestLineProvider;
    function<Value()> bestValueProvider;

PUBLIC
    SearchMonitor();

    void initStartTime() { startTime = chrono::high_resolution_clock::now(); };
    void setTrigger(function<bool(SearchMonitor&)> newTrigger) { trigger = newTrigger; };
    void setSearchListener(function<void(SearchMonitor&)> newSearchListener) { searchListener = newSearchListener; };
    void executeTrigger();
    void setBestLineProvider(std::function<MoveList(int)> provider) { bestLineProvider = provider; }
    void setBestValueProvider(std::function<Value()> provider) { bestValueProvider = provider; }
    
    // update data function, executeTrigger function execute
    void updateElapsedTime();
    void incDepth(int val) { depth += val; executeTrigger(); };
    void decDepth(int val) { depth -= val; executeTrigger(); };
    void incVisitCnt() { visitCnt++; executeTrigger(); };
    void setBestPath(MoveList path) { bestPath = path; executeTrigger(); };

    // getter
    double getElapsedTime() { return elapsedTime; }
    int getDepth() { return depth; }
    size_t getVisitCnt() { return visitCnt; }
    MoveList getBestPath() { return bestPath; }
    MoveList getBestLine(int i) { return bestLineProvider ? bestLineProvider(i) : MoveList(); }
    Value getBestValue() { return bestValueProvider ? bestValueProvider() : Value(); }

};

SearchMonitor::SearchMonitor() {
    elapsedTime = 0.0;
    depth = 0;
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