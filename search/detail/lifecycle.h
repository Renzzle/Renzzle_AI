#pragma once

void Search::ids() {
    state.isRunning = true;
    state.bestPath.clear();
    state.bestValue = Value();
    state.nodesSinceMonitorPoll = 0;
    clearHistory();
    monitor.incDepth(5);
    monitor.initStartTime();
    tt.clear();

    while (true) {
        tt.nextGeneration();

        MoveList iterationPV;
        Value result = searchRootWithAspiration(monitor.getDepth(), &iterationPV);

        if (!state.isRunning) {
            break;
        }

        const bool provenWin = result.isWin() && result.getType() == Value::Type::EXACT;
        state.bestValue = result;
        state.bestPath = iterationPV;
        if (state.bestValue.isWin()
            && state.bestPath.size() < static_cast<size_t>(state.bestValue.getResultDepth())) {
            const Board savedBoard = board;
            Board tempBoard = rootBoard;
            for (const Pos& move : state.bestPath) {
                if (!tempBoard.move(move)) {
                    break;
                }
            }

            while (searchActive()
                && tempBoard.getResult() == ONGOING
                && state.bestPath.size() < static_cast<size_t>(state.bestValue.getResultDepth())) {
                board = tempBoard;
                MoveList tailPV;
                const int remainDepth =
                    state.bestValue.getResultDepth() - static_cast<int>(state.bestPath.size());
                const bool tailIsMax = (state.bestPath.size() % 2 == 0);
                static_cast<void>(abp(
                    remainDepth,
                    tailIsMax,
                    Value(MIN_VALUE, Value::Type::UNKNOWN),
                    Value(MAX_VALUE + 1, Value::Type::UNKNOWN),
                    &tailPV
                ));

                size_t appended = 0;
                for (const Pos& move : tailPV) {
                    if (state.bestPath.size() >= static_cast<size_t>(state.bestValue.getResultDepth())) {
                        break;
                    }
                    if (!tempBoard.move(move)) {
                        break;
                    }
                    state.bestPath.push_back(move);
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
        monitor.setBestPath(state.bestPath);

        if (provenWin) {
            break;
        }

        monitor.incDepth(2);
    }

    updateMonitorElapsedTime();
    state.isRunning = false;
}

void Search::stop() {
    state.isRunning = false;
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
