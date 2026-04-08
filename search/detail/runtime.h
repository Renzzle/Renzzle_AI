#pragma once

Search::Search(Board& initialBoard, SearchMonitor& monitor, size_t rootThreadCount,
    size_t ttBytes, std::shared_ptr<std::atomic_bool> sharedRunning,
    const TranspositionTable* sharedTT)
    : rootBoard(initialBoard),
      board(initialBoard),
      monitor(monitor),
      tt(ttBytes, 4) {
    options.rootThreadCount = rootThreadCount;
    state.sharedRunning =
        sharedRunning ? sharedRunning : std::make_shared<std::atomic_bool>(true);
    monitor.setBestLineProvider([this](int i) {
        return i == 0 ? state.bestPath : MoveList();
    });
    monitor.setBestValueProvider([this]() {
        return state.bestValue;
    });

    if (sharedTT != nullptr) {
        tt = *sharedTT;
    }
}

bool Search::searchActive() const {
    return state.isRunning
        && state.sharedRunning != nullptr
        && state.sharedRunning->load(std::memory_order_relaxed);
}

void Search::syncSharedRunningFlag() {
    if (state.sharedRunning == nullptr) {
        state.sharedRunning = std::make_shared<std::atomic_bool>(state.isRunning);
        return;
    }

    if (state.isRunning) {
        state.sharedRunning->store(true, std::memory_order_relaxed);
    }
}

size_t Search::getEffectiveRootThreadCount() const {
    return options.rootThreadCount == 0 ? 1 : std::max<size_t>(1, options.rootThreadCount);
}

size_t Search::getEffectiveLazyThreadCount() const {
    return options.lazyThreadCount == 0 ? 1 : std::max<size_t>(1, options.lazyThreadCount);
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
