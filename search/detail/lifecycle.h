#pragma once

void Search::ids() {
    state.isRunning = true;
    state.bestPath.clear();
    state.bestValue = Value();
    state.lastRootSearchComplete = false;
    state.nodesSinceMonitorPoll = 0;
    state.qvcfDisabledAfterWin = false;
    clearHistory();
    monitor.incDepth(5);
    monitor.initStartTime();
    tt.clear();
    threatTT.clear();
    std::fill(threatDefenseCache.begin(), threatDefenseCache.end(), ThreatDefenseEntry());

    // no line outlasts the empty cells; beyond this, iterations only replay the TT
    const int maxDepth = BOARD_SIZE * BOARD_SIZE - static_cast<int>(rootBoard.getPath().size());

    // stage 0 is the classic threat set (DEFENSIVE's only stage, where it also watches for
    // VCF threats at the root); stage 1 widens VCT with VCF-threat moves
    SearchStage stages[2];
    stages[0].enabled = true;
    stages[0].depth = monitor.getDepth();
    stages[0].vcfThreats = options.mode != Mode::VCT && options.vcfThreats;
    stages[1].enabled = options.mode == Mode::VCT && options.vcfThreats;
    stages[1].depth = monitor.getDepth();
    stages[1].vcfThreats = true;
    stages[1].ttSalt = WIDENED_STAGE_TT_SALT;
    int current = 0;
    enterSearchStage(stages, current, current);

    while (true) {
        // refresh wall-clock + fire time-based trigger; guards against TT-cached
        // iterations that visit too few nodes to hit pollMonitorIfDue's interval
        monitor.updateElapsedTime();
        if (!state.isRunning) break;

        const int next = pickSearchStage(stages);
        if (next < 0) break;
        enterSearchStage(stages, current, next);
        current = next;
        SearchStage& stage = stages[current];

        // only the classic stage ages the table, so widened iterations don't evict its entries
        if (current == 0) {
            tt.nextGeneration();
        }

        const size_t nodesBefore = monitor.getVisitCnt();
        MoveList iterationPV;
        Value result = searchRootWithAspiration(stage.depth, &iterationPV);

        if (!state.isRunning) break;
        stage.cost += monitor.getVisitCnt() - nodesBefore;

        const bool provenWin =
            result.isWin() && result.getType() == Value::Type::EXACT && !result.isQVCFDerived();
        state.bestValue = result;
        state.bestPath = iterationPV;
        completeWinPV(state.bestValue, state.bestPath);
        stage.bestValue = state.bestValue;
        stage.bestPath = state.bestPath;

        monitor.setBestPath(state.bestPath);

        if (provenWin) {
            break;
        }

        // DEFENSIVE elimination: if exactly one root candidate is non-LOSE,
        // the answer is decided regardless of further deepening. Only valid when
        // the last root call actually searched every candidate — a partial
        // lastRootStats (root cutoff/stop) says nothing about unsearched moves.
        if (options.mode == Mode::DEFENSIVE && state.lastRootSearchComplete) {
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

        stage.depth += 2;
        stage.done = stage.depth > maxDepth;
        setMonitorDepth(stage.depth);
    }

    publishSearchStages(stages, current);
    updateMonitorElapsedTime();
    state.isRunning = false;
}

int Search::pickSearchStage(SearchStage* stages) {
    auto runnable = [&](int i) { return stages[i].enabled && !stages[i].done; };

    // a stage holding an unproven (QVCF-derived) win keeps going alone until it proves it
    for (int i = 0; i < 2; ++i) {
        if (runnable(i) && stages[i].bestValue.isWin()) return i;
    }
    if (!runnable(1)) return runnable(0) ? 0 : -1;
    if (!runnable(0)) return 1;
    if (stages[0].cost < VCF_THREAT_WIDEN_COST) return 0;
    return stages[0].cost <= stages[1].cost * WIDENED_STAGE_COST_WEIGHT ? 0 : 1;
}

// swaps the per-stage search state in; the TT salt keeps each stage's entries apart, so
// classic "threat broken" results never read as losses for the widened stage
void Search::enterSearchStage(SearchStage* stages, int from, int to) {
    if (from != to) {
        stages[from].historyScores = state.historyScores;
        stages[from].killerMoves = state.killerMoves;
        stages[from].qvcfDisabledAfterWin = state.qvcfDisabledAfterWin;
        state.historyScores = stages[to].historyScores;
        state.killerMoves = stages[to].killerMoves;
        state.qvcfDisabledAfterWin = stages[to].qvcfDisabledAfterWin;
    }
    state.vcfThreats = stages[to].vcfThreats;
    state.ttSalt = stages[to].ttSalt;
    // aspiration windows center on the stage's own last value
    state.bestValue = stages[to].bestValue;
    state.bestPath = stages[to].bestPath;
    setMonitorDepth(stages[to].depth);
}

// the caller sees a win from either stage, otherwise the classic stage's result
void Search::publishSearchStages(SearchStage* stages, int current) {
    int shown = current;
    if (!stages[shown].bestValue.isWin()) {
        shown = 0;
        for (int i = 0; i < 2; ++i) {
            if (stages[i].enabled && stages[i].bestValue.isWin()) {
                shown = i;
                break;
            }
        }
    }
    if (shown == current) {
        return;
    }
    state.bestValue = stages[shown].bestValue;
    state.bestPath = stages[shown].bestPath;
    state.ttSalt = stages[shown].ttSalt;
    monitor.setBestPath(state.bestPath);
}

void Search::setMonitorDepth(int depth) {
    const int current = monitor.getDepth();
    if (depth > current) {
        monitor.incDepth(depth - current);
    } else if (depth < current) {
        monitor.decDepth(current - depth);
    }
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
        + threatTT.getMemoryBytes()
        + (sizeof(Board) * 2)
        + (state.bestPath.capacity() * sizeof(Pos))
        + sizeof(state.historyScores)
        + sizeof(state.killerMoves)
        + (threatDefenseCache.capacity() * sizeof(ThreatDefenseEntry));
}

const std::vector<Search::RootMoveStat>& Search::getLastRootStats() const {
    return state.lastRootStats;
}
