#pragma once

void Search::ids() {
    state.isRunning = true;
    state.bestPath.clear();
    state.bestValue = Value();
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
}

void Search::stop() {
    state.isRunning = false;
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
