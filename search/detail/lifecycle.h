#pragma once

void Search::ids() {
    state.isRunning = true;
    state.bestPath.clear();
    state.bestValue = Value();
    state.nodesSinceMonitorPoll = 0;
    state.qvcfDisabledAfterWin = false;
    clearHistory();
    monitor.incDepth(5);
    monitor.initStartTime();
    tt.clear();

    while (true) {
        // refresh wall-clock + fire time-based trigger; guards against TT-cached
        // iterations that visit too few nodes to hit pollMonitorIfDue's interval
        monitor.updateElapsedTime();
        if (!state.isRunning) break;

        tt.nextGeneration();

        MoveList iterationPV;
        Value result = searchRootWithAspiration(monitor.getDepth(), &iterationPV);

        if (!state.isRunning) break;

        const bool provenWin =
            result.isWin() && result.getType() == Value::Type::EXACT && !result.isQVCFDerived();
        state.bestValue = result;
        state.bestPath = iterationPV;
        completeWinPV(state.bestValue, state.bestPath);

        monitor.setBestPath(state.bestPath);

        if (provenWin) {
            break;
        }

        // DEFENSIVE elimination: if exactly one root candidate is non-LOSE,
        // the answer is decided regardless of further deepening.
        if (options.mode == Mode::DEFENSIVE) {
            int nonLoseCount = 0;
            for (auto& stat : state.lastRootStats) {
                if (!stat.value.isLose()) {
                    ++nonLoseCount;
                    if (nonLoseCount > 1) break;
                }
            }
            if (nonLoseCount == 1) {
                break;
            }
        }

        monitor.incDepth(2);
    }

    updateMonitorElapsedTime();
    state.isRunning = false;
}

void Search::completeWinPV(Value value, MoveList& pv) {
    if (!value.isWin() || pv.size() >= static_cast<size_t>(value.getResultDepth())) {
        return;
    }

    const Board savedBoard = board;
    Board tempBoard = rootBoard;
    for (const Pos& move : pv) {
        if (!tempBoard.move(move)) {
            board = savedBoard;
            return;
        }
    }

    while (searchActive()
        && tempBoard.getResult() == ONGOING
        && pv.size() < static_cast<size_t>(value.getResultDepth())) {
        board = tempBoard;
        MoveList tailPV;
        const int remainDepth = value.getResultDepth() - static_cast<int>(pv.size());
        const bool tailIsMax = (pv.size() % 2 == 0);
        static_cast<void>(abp(
            remainDepth,
            tailIsMax,
            Value(MIN_VALUE, Value::Type::UNKNOWN),
            Value(MAX_VALUE + 1, Value::Type::UNKNOWN),
            &tailPV
        ));

        size_t appended = 0;
        for (const Pos& move : tailPV) {
            if (pv.size() >= static_cast<size_t>(value.getResultDepth())) {
                break;
            }
            if (!tempBoard.move(move)) {
                break;
            }
            pv.push_back(move);
            appended += 1;
            if (tempBoard.getResult() != ONGOING) {
                break;
            }
        }

        if (appended == 0) {
            break;
        }
    }

    board = savedBoard;
}

void Search::stop() {
    state.isRunning = false;
}

void Search::setQVCFEnabled(bool enabled) {
    options.leafVCFEnabled = enabled;
    if (enabled) {
        state.qvcfDisabledAfterWin = false;
    }
}

void Search::setMode(Mode mode) {
    options.mode = mode;
}

void Search::setMonitorPollNodeInterval(size_t nodeInterval) {
    options.monitorPollNodeInterval = std::max<size_t>(1, nodeInterval);
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
