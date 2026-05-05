#pragma once

Search::Search(Board& initialBoard, SearchMonitor& monitor, size_t ttBytes)
    : rootBoard(initialBoard),
      board(initialBoard),
      monitor(monitor),
      tt(ttBytes, 4) {
    monitor.setBestLineProvider([this](int i) {
        return i == 0 ? state.bestPath : MoveList();
    });
    monitor.setBestValueProvider([this]() {
        return state.bestValue;
    });
}

bool Search::searchActive() const {
    return state.isRunning;
}

void Search::updateMonitorElapsedTime() {
    if (options.trackMonitorStats) {
        monitor.updateElapsedTime();
    }
}

void Search::recordNodeVisit() {
    if (options.trackMonitorStats) {
        monitor.incVisitCnt();
    }
}
