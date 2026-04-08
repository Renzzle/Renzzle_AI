#pragma once

bool Search::shouldUseRootParallel(bool isRootNode, bool isMax, int depth, size_t moveCount) const {
    return isRootNode
        && isMax
        && depth >= ROOT_PARALLEL_MIN_DEPTH
        && moveCount >= ROOT_PARALLEL_MIN_MOVES
        && getEffectiveLazyThreadCount() <= 1
        && getEffectiveRootThreadCount() > 1;
}

Search::RootParallelResult Search::searchRootMoveFullWindow(
    int depth, const Pos& move, Value alpha, Value beta, size_t order, bool wasTTBest) {
    RootParallelResult result;
    result.order = order;
    result.move = move;
    result.wasTTBest = wasTTBest;

    if (!searchActive()) {
        return result;
    }

    updateMonitorElapsedTime();
    const size_t nodeCountBeforeMove = monitor.getVisitCnt();
    const double elapsedBeforeMove = monitor.getElapsedTime();

    if (!board.move(move)) {
        return result;
    }

    recordNodeVisit();

    Value nextAlpha = alpha;
    Value nextBeta = beta;
    nextAlpha.decreaseResultDepth();
    nextBeta.decreaseResultDepth();

    MoveList childPV;
    Value childValue = abp(depth - 1, false, nextAlpha, nextBeta, &childPV);

    board.undo();

    if (!searchActive()) {
        return result;
    }

    childValue.increaseResultDepth();
    updateMonitorElapsedTime();

    result.value = childValue;
    result.nodeCount = monitor.getVisitCnt() - nodeCountBeforeMove;
    result.elapsedTime = monitor.getElapsedTime() - elapsedBeforeMove;
    result.searched = true;
    result.causedCutoff = childValue >= beta;
    result.pv.push_back(move);

    if (childValue.getType() == Value::Type::EXACT && !childPV.empty()) {
        result.pv.insert(result.pv.end(), childPV.begin(), childPV.end());
    } else {
        Board tempBoard = board;
        if (tempBoard.move(move)) {
            appendTTPV(tempBoard, result.pv);
        }
    }

    return result;
}

Value Search::searchRootParallel(int depth, Value alpha, Value beta, const MoveList& moves,
    const TTEntry* entry, MoveList* pv) {
    const Value originalAlpha = alpha;
    const Value originalBeta = beta;
    const Pos ttBestMove = (entry != nullptr && entry->bestMove != TranspositionTable::INVALID_MOVE)
        ? TranspositionTable::decodeMove(entry->bestMove)
        : Pos();

    std::vector<RootParallelResult> results(moves.size());

    results[0] = searchRootMoveFullWindow(
        depth,
        moves[0],
        alpha,
        beta,
        1,
        !ttBestMove.isDefault() && moves[0] == ttBestMove
    );

    if (!results[0].searched) {
        return Value();
    }

    Value parallelAlpha = alpha;
    if (results[0].value > parallelAlpha) {
        parallelAlpha = results[0].value;
    }
    std::mutex parallelAlphaMutex;

    const size_t maxThreads = std::min(getEffectiveRootThreadCount(), moves.size());
    const size_t workerCount = maxThreads > 1 ? std::min(maxThreads - 1, moves.size() - 1) : 0;
    std::atomic<size_t> nextIndex(1);
    std::vector<std::future<void>> workers;
    workers.reserve(workerCount);

    for (size_t workerIdx = 0; workerIdx < workerCount; ++workerIdx) {
        workers.emplace_back(std::async(std::launch::async, [&, workerIdx]() {
            SearchMonitor workerMonitor;
            Board workerBoard = rootBoard;
            Search worker(workerBoard, workerMonitor, 1, ROOT_PARALLEL_WORKER_TT_BYTES, state.sharedRunning);
            worker.state.isRunning = true;
            worker.state.historyScores = state.historyScores;
            workerMonitor.initStartTime();

            while (state.sharedRunning->load(std::memory_order_relaxed)) {
                const size_t moveIndex = nextIndex.fetch_add(1, std::memory_order_relaxed);
                if (moveIndex >= moves.size()) {
                    break;
                }

                Value workerAlpha;
                {
                    std::lock_guard<std::mutex> lock(parallelAlphaMutex);
                    workerAlpha = parallelAlpha;
                }

                worker.board = worker.rootBoard;
                RootParallelResult result = worker.searchRootMoveFullWindow(
                    depth,
                    moves[moveIndex],
                    workerAlpha,
                    beta,
                    moveIndex + 1,
                    !ttBestMove.isDefault() && moves[moveIndex] == ttBestMove
                );

                if (result.searched) {
                    std::lock_guard<std::mutex> lock(parallelAlphaMutex);
                    if (result.value > parallelAlpha) {
                        parallelAlpha = result.value;
                    }
                }
                results[moveIndex] = result;
            }
        }));
    }

    for (auto& worker : workers) {
        worker.get();
    }

    for (size_t moveIndex = 1; moveIndex < moves.size() && searchActive(); ++moveIndex) {
        if (!results[moveIndex].searched) {
            results[moveIndex] = searchRootMoveFullWindow(
                depth,
                moves[moveIndex],
                parallelAlpha,
                beta,
                moveIndex + 1,
                !ttBestMove.isDefault() && moves[moveIndex] == ttBestMove
            );
        }
    }

    size_t workerNodeCount = 0;
    Value bestVal = results[0].value;
    Pos bestMove = results[0].move;
    MoveList bestRootPV = results[0].pv;
    MoveList searchedMoves;
    searchedMoves.reserve(moves.size());
    state.lastRootStats.clear();

    for (const RootParallelResult& result : results) {
        if (!result.searched) {
            continue;
        }

        RootMoveStat stat;
        stat.order = result.order;
        stat.move = result.move;
        stat.value = result.value;
        stat.nodeCount = result.nodeCount;
        stat.elapsedTime = result.elapsedTime;
        stat.wasTTBest = result.wasTTBest;
        stat.wasResearched = result.wasResearched;
        stat.causedCutoff = result.causedCutoff;
        stat.pv = result.pv;
        state.lastRootStats.push_back(stat);

        searchedMoves.push_back(result.move);
        if (result.order != 1) {
            workerNodeCount += result.nodeCount;
        }

        if (result.value > bestVal) {
            bestVal = result.value;
            bestMove = result.move;
            bestRootPV = result.pv;
        }
    }

    if (workerNodeCount > 0) {
        monitor.addVisitCnt(workerNodeCount);
    }

    updateMonitorElapsedTime();

    Value result = bestVal;
    if (result.getType() == Value::Type::UNKNOWN) {
        result = parallelAlpha;
    }

    const bool causedCutoff = result >= originalBeta;
    if (result <= originalAlpha) {
        result.setType(Value::Type::UPPER_BOUND);
    } else if (result >= originalBeta) {
        result.setType(Value::Type::LOWER_BOUND);
    } else {
        result.setType(Value::Type::EXACT);
    }

    if (searchActive() && !bestMove.isDefault()) {
        const bool sideToMoveIsBlack = board.isBlackTurn();
        const int bonus = std::max(1, std::min(depth * depth, 256));
        if (causedCutoff || result.getType() == Value::Type::LOWER_BOUND) {
            updateHistoryScore(bestMove, sideToMoveIsBlack, bonus);

            const int penalty = std::max(1, bonus / 2);
            for (const Pos& move : searchedMoves) {
                if (!(move == bestMove)) {
                    updateHistoryScore(move, sideToMoveIsBlack, -penalty);
                }
            }
        } else if (result.getType() == Value::Type::EXACT) {
            updateHistoryScore(bestMove, sideToMoveIsBlack, std::max(1, bonus / 2));
        }
    }

    if (searchActive()) {
        storeTT(board, result, depth, bestMove);
    }

    if (pv != nullptr) {
        pv->clear();
        if (!bestRootPV.empty()) {
            pv->insert(pv->end(), bestRootPV.begin(), bestRootPV.end());
        }
    }

    return result;
}
