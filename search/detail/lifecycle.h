#pragma once

void Search::ids() {
    state.isRunning = true;
    syncSharedRunningFlag();
    state.bestPath.clear();
    state.bestValue = Value();
    clearHistory();
    monitor.incDepth(5);
    monitor.initStartTime();
    const size_t helperCount = getEffectiveLazyThreadCount() > 1
        ? getEffectiveLazyThreadCount() - 1
        : 0;
    bool sharedTTModeActive = false;
    tt.clear();
    tt.disableSharedMode();

    auto lazySnapshot = std::make_shared<LazySMPSnapshot>();
    auto helperRunning = std::make_shared<std::atomic_bool>(true);
    std::vector<std::future<void>> helperWorkers;
    helperWorkers.reserve(helperCount);

    for (size_t helperIndex = 0; helperIndex < helperCount; ++helperIndex) {
        helperWorkers.emplace_back(std::async(std::launch::async, [&, helperIndex]() {
            SearchMonitor helperMonitor;
            Board helperBoard = rootBoard;
            Search helper(helperBoard, helperMonitor, 0, 1, state.sharedRunning, &tt);
            helper.setLightweightTTOrdering(true);
            helper.options.trackMonitorStats = false;
            helper.options.nonBlockingTTAccess = true;
            helper.lazySMPHelperLoop(helperIndex, lazySnapshot, helperRunning);
        }));
    }

    while (true) {
        tt.nextGeneration();

        MoveList iterationPV;
        Value result = searchRootWithAspiration(monitor.getDepth(), &iterationPV);

        if (!state.isRunning) {
            break;
        }

        state.bestValue = result;
        state.bestPath = iterationPV;
        if (helperCount > 0) {
            if (!sharedTTModeActive && monitor.getDepth() >= LAZY_SMP_MIN_COMPLETED_DEPTH) {
                tt.enableSharedMode();
                sharedTTModeActive = true;
            }
            std::lock_guard<std::mutex> lock(lazySnapshot->mutex);
            lazySnapshot->completedDepth = monitor.getDepth();
            lazySnapshot->bestValue = state.bestValue;
            lazySnapshot->rootMoves.clear();
            for (const RootMoveStat& stat : state.lastRootStats) {
                if (!stat.move.isDefault()) {
                    lazySnapshot->rootMoves.push_back(stat.move);
                }
            }
            if (lazySnapshot->rootMoves.empty() && !state.bestPath.empty()) {
                lazySnapshot->rootMoves.push_back(state.bestPath.front());
            }
        }
        if (state.bestValue.isWin()
            && state.bestPath.size() < static_cast<size_t>(state.bestValue.getResultDepth())) {
            const Board savedBoard = board;
            Board tempBoard = rootBoard;
            for (const Pos& move : state.bestPath) {
                if (!tempBoard.move(move)) {
                    break;
                }
            }

            if (tempBoard.getResult() == ONGOING) {
                board = tempBoard;
                MoveList tailPV;
                const int remainDepth =
                    state.bestValue.getResultDepth() - static_cast<int>(state.bestPath.size());
                const bool tailIsMax = (state.bestPath.size() % 2 == 0);
                abp(
                    remainDepth,
                    tailIsMax,
                    Value(MIN_VALUE, Value::Type::UNKNOWN),
                    Value(MAX_VALUE + 1, Value::Type::UNKNOWN),
                    &tailPV
                );
                state.bestPath.insert(state.bestPath.end(), tailPV.begin(), tailPV.end());
                board = savedBoard;
            }
        }
        monitor.setBestPath(state.bestPath);

        if (result.isWin() && result.getResultDepth() <= monitor.getDepth()) {
            break;
        }

        monitor.incDepth(2);
    }

    state.isRunning = false;
    if (state.sharedRunning != nullptr) {
        state.sharedRunning->store(false, std::memory_order_relaxed);
    }
    helperRunning->store(false, std::memory_order_relaxed);
    for (auto& helper : helperWorkers) {
        helper.get();
    }
}

void Search::stop() {
    state.isRunning = false;
    if (state.sharedRunning != nullptr) {
        state.sharedRunning->store(false, std::memory_order_relaxed);
    }
}

void Search::setRootThreadCount(size_t count) {
    options.rootThreadCount = count;
}

void Search::setLazyThreadCount(size_t count) {
    options.lazyThreadCount = count;
}

void Search::setLightweightTTOrdering(bool enabled) {
    options.lightweightTTOrdering = enabled;
}

size_t Search::getNodeCount() const {
    return tt.getUsedEntryCount();
}

size_t Search::getEstimatedMemoryBytes() const {
    return tt.getMemoryBytes()
        + (sizeof(Board) * 2)
        + (state.bestPath.capacity() * sizeof(Pos))
        + sizeof(state.historyScores);
}

const std::vector<Search::RootMoveStat>& Search::getLastRootStats() const {
    return state.lastRootStats;
}
