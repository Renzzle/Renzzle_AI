#pragma once

bool Search::tryResolveFromTT(int depth, Value& alpha, Value& beta, MoveList* pv,
    TTEntry& ttEntryStorage, const TTEntry*& ttEntry, Value& resolvedValue) {
    const uint64_t key = getTTKey(board);
    ttEntry = tt.probeCopy(key, &ttEntryStorage, options.nonBlockingTTAccess) ? &ttEntryStorage : nullptr;

    if (ttEntry == nullptr || ttEntry->depth < getTTDepth(depth)) {
        return false;
    }

    Value ttValue = getTTValue(*ttEntry);

    if (ttEntry->getFlag() == TTFlag::EXACT) {
        if (pv != nullptr) {
            appendTTPV(board, *pv);
        }
        resolvedValue = ttValue;
        return true;
    }
    if (ttEntry->getFlag() == TTFlag::LOWER_BOUND && ttValue >= beta) {
        resolvedValue = ttValue;
        return true;
    }
    if (ttEntry->getFlag() == TTFlag::UPPER_BOUND && ttValue <= alpha) {
        resolvedValue = ttValue;
        return true;
    }

    if (ttEntry->getFlag() == TTFlag::LOWER_BOUND && ttValue > alpha) {
        alpha = ttValue;
    } else if (ttEntry->getFlag() == TTFlag::UPPER_BOUND && ttValue < beta) {
        beta = ttValue;
    }

    return false;
}

int Search::getShallowMoveLimit(Evaluator& evaluator, int depth) const {
    const bool isDefendingNode = evaluator.isOppoMateExist();
    return isDefendingNode
        ? std::numeric_limits<int>::max()
        : (depth <= 3) ? 4 : (depth <= 5) ? 6 : std::numeric_limits<int>::max();
}

Value Search::evaluateLeafNode(Evaluator& evaluator, bool isMax, int depth) {
    Value val = evaluateNode(evaluator);
    if (!isMax) {
        val.invert();
    }
    if (searchActive()) {
        storeTT(board, val, depth, Pos());
    }
    return val;
}

Search::ChildSearchResult Search::searchChildPVS(int depth, bool isMax, size_t moveIndex, Value alpha, Value beta,
    Value bestVal, MoveList* pv) {
    ChildSearchResult result;

    Value nextAlpha = alpha;
    Value nextBeta = beta;
    nextAlpha.decreaseResultDepth();
    nextBeta.decreaseResultDepth();

    if (moveIndex == 0) {
        result.value = abp(depth - 1, !isMax, nextAlpha, nextBeta, &result.pv);
        result.hasExactPV = result.value.getType() == Value::Type::EXACT;
        return result;
    }

    Value nullAlpha = nextAlpha;
    Value nullBeta = nextBeta;
    if (isMax) {
        nullBeta = nullAlpha;
        nullBeta += 1;
    } else {
        nullAlpha = nullBeta;
        nullAlpha -= 1;
    }

    result.value = abp(depth - 1, !isMax, nullAlpha, nullBeta, nullptr);

    if (!searchActive()) {
        return result;
    }

    const bool needResearch =
        (isMax && result.value > alpha && result.value < beta) ||
        (!isMax && result.value < beta && result.value > alpha);
    const bool couldBecomeBest =
        pv != nullptr
            && ((isMax && result.value > bestVal) || (!isMax && result.value < bestVal));

    if (needResearch || couldBecomeBest) {
        result.value = abp(depth - 1, !isMax, nextAlpha, nextBeta, &result.pv);
        result.hasExactPV = result.value.getType() == Value::Type::EXACT;
        result.wasResearched = true;
    }

    return result;
}

void Search::updateBestFromChild(bool isMax, const Pos& move, const ChildSearchResult& childResult,
    Value& bestVal, Pos& bestMove, MoveList& bestChildPV, Value& alpha, Value& beta) {
    if (isMax) {
        if (childResult.value > bestVal) {
            bestVal = childResult.value;
            bestMove = move;
            bestChildPV = childResult.hasExactPV ? childResult.pv : MoveList();
        }
        if (childResult.value > alpha) {
            alpha = childResult.value;
        }
        return;
    }

    if (childResult.value < bestVal) {
        bestVal = childResult.value;
        bestMove = move;
        bestChildPV = childResult.hasExactPV ? childResult.pv : MoveList();
    }
    if (childResult.value < beta) {
        beta = childResult.value;
    }
}

void Search::recordRootMoveStat(size_t order, const Pos& move, const Value& childValue,
    const MoveList& childPV, bool hasExactChildPV, bool wasResearched, bool causedCutoff,
    size_t nodeCountBeforeMove, double elapsedBeforeMove, const Pos& ttBestMove) {
    updateMonitorElapsedTime();

    RootMoveStat stat;
    stat.order = order;
    stat.move = move;
    stat.value = childValue;
    stat.nodeCount = (monitor.getVisitCnt() - nodeCountBeforeMove) + 1;
    stat.elapsedTime = monitor.getElapsedTime() - elapsedBeforeMove;
    stat.wasTTBest = !ttBestMove.isDefault() && move == ttBestMove;
    stat.wasResearched = wasResearched;
    stat.causedCutoff = causedCutoff;
    stat.pv.push_back(move);
    if (hasExactChildPV) {
        stat.pv.insert(stat.pv.end(), childPV.begin(), childPV.end());
    } else {
        Board tempBoard = board;
        if (tempBoard.move(move)) {
            appendTTPV(tempBoard, stat.pv);
        }
    }
    state.lastRootStats.push_back(stat);
}

Value Search::finalizeNodeValue(bool isMax, Value originalAlpha, Value originalBeta,
    Value alpha, Value beta, Value bestVal) const {
    Value result = bestVal;
    if (result.getType() == Value::Type::UNKNOWN) {
        result = isMax ? alpha : beta;
    }

    if (result <= originalAlpha) {
        result.setType(Value::Type::UPPER_BOUND);
    } else if (result >= originalBeta) {
        result.setType(Value::Type::LOWER_BOUND);
    } else {
        result.setType(Value::Type::EXACT);
    }

    return result;
}

void Search::updateHistoryFromNode(int depth, const Pos& bestMove, const MoveList& searchedMoves,
    bool causedCutoff, Value result, bool sideToMoveIsBlack) {
    if (!searchActive() || bestMove.isDefault()) {
        return;
    }

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

void Search::rebuildPV(const Pos& bestMove, const MoveList& bestChildPV, Value result, MoveList* pv) const {
    if (pv == nullptr || bestMove.isDefault()) {
        return;
    }

    pv->push_back(bestMove);
    if (result.getType() == Value::Type::EXACT) {
        if (!bestChildPV.empty()) {
            pv->insert(pv->end(), bestChildPV.begin(), bestChildPV.end());
        } else {
            Board tempBoard = board;
            if (tempBoard.move(bestMove)) {
                appendTTPV(tempBoard, *pv);
            }
        }
    } else {
        Board tempBoard = board;
        if (tempBoard.move(bestMove)) {
            appendTTPV(tempBoard, *pv);
        }
    }
}

Value Search::abp(int depth, bool isMax, Value alpha, Value beta, MoveList* pv) {
    monitor.updateElapsedTime();
    if (!searchActive()) return Value();
    if (pv != nullptr) {
        pv->clear();
    }

    const bool isRootNode = board.getPath().size() == rootBoard.getPath().size();
    if (isRootNode) {
        state.lastRootStats.clear();
    }

    const Value originalAlpha = alpha;
    const Value originalBeta = beta;

    TTEntry ttEntryStorage;
    const TTEntry* ttEntry = nullptr;
    Value resolvedValue;
    if (tryResolveFromTT(depth, alpha, beta, pv, ttEntryStorage, ttEntry, resolvedValue)) {
        return resolvedValue;
    }

    Evaluator evaluator(board);
    MoveList moves = getCandidates(evaluator, isMax);
    if (depth == 0 || isGameOver(board) || moves.empty()) {
        return evaluateLeafNode(evaluator, isMax, depth);
    }

    sortChildNodes(moves, isMax, ttEntry);
    if (shouldUseRootParallel(isRootNode, isMax, depth, moves.size())) {
        return searchRootParallel(depth, alpha, beta, moves, ttEntry, pv);
    }

    Value bestVal = isMax
        ? Value(MIN_VALUE, Value::Type::UNKNOWN)
        : Value(MAX_VALUE + 1, Value::Type::UNKNOWN);
    Pos bestMove;
    MoveList bestChildPV;
    MoveList searchedMoves;
    searchedMoves.reserve(moves.size());

    const int shallowMoveLimit = getShallowMoveLimit(evaluator, depth);
    const bool sideToMoveIsBlack = board.isBlackTurn();
    const Pos ttBestMove = (ttEntry != nullptr && ttEntry->bestMove != TranspositionTable::INVALID_MOVE)
        ? TranspositionTable::decodeMove(ttEntry->bestMove)
        : Pos();

    bool searchedAny = false;
    bool causedCutoff = false;

    for (size_t i = 0; i < moves.size(); ++i) {
        if (static_cast<int>(i) >= shallowMoveLimit) {
            break;
        }

        const Pos move = moves[i];
        if (!board.move(move)) {
            continue;
        }

        searchedAny = true;
        searchedMoves.push_back(move);
        recordNodeVisit();
        const size_t nodeCountBeforeMove = isRootNode ? monitor.getVisitCnt() : 0;
        const double elapsedBeforeMove = isRootNode ? monitor.getElapsedTime() : 0.0;

        ChildSearchResult childResult = searchChildPVS(depth, isMax, i, alpha, beta, bestVal, pv);

        board.undo();
        if (!searchActive()) {
            break;
        }

        childResult.value.increaseResultDepth();
        updateBestFromChild(isMax, move, childResult, bestVal, bestMove, bestChildPV, alpha, beta);

        const bool moveCausedCutoff = beta <= alpha;
        if (isRootNode) {
            recordRootMoveStat(
                i + 1,
                move,
                childResult.value,
                childResult.pv,
                childResult.hasExactPV,
                childResult.wasResearched,
                moveCausedCutoff,
                nodeCountBeforeMove,
                elapsedBeforeMove,
                ttBestMove
            );
        }

        if (moveCausedCutoff) {
            causedCutoff = true;
            break;
        }
    }

    if (!searchedAny) {
        return evaluateLeafNode(evaluator, isMax, depth);
    }

    Value result = finalizeNodeValue(isMax, originalAlpha, originalBeta, alpha, beta, bestVal);
    updateHistoryFromNode(depth, bestMove, searchedMoves, causedCutoff, result, sideToMoveIsBlack);

    if (searchActive()) {
        storeTT(board, result, depth, bestMove);
    }

    rebuildPV(bestMove, bestChildPV, result, pv);
    return result;
}

Value Search::evaluateNode(Evaluator& evaluator) {
    return evaluator.evaluateTactical();
}

MoveList Search::getCandidates(Evaluator& evaluator, bool isMax) {
    MoveList moves;
    const bool attackerTurn = (board.isBlackTurn() == rootBoard.isBlackTurn());

    Pos sureMove = evaluator.getSureMove();
    if (!sureMove.isDefault()) {
        moves.push_back(sureMove);
        return moves;
    }

    if (evaluator.isOppoMateExist()) {
        if (attackerTurn) {
            return evaluator.getThreats();
        }

        moves = evaluator.getThreatDefend();
        MoveList fours = evaluator.getFours();
        moves.insert(moves.end(), fours.begin(), fours.end());
    } else {
        moves = evaluator.getThreats();
    }

    return moves;
}

void Search::sortChildNodes(MoveList& moves, bool isMax, const TTEntry* entry) {
    if (moves.size() < 2) {
        return;
    }

    struct MoveOrderInfo {
        Pos move;
        bool isTTBest;
        bool hasTT;
        int flagPriority;
        int32_t ttScore;
        int historyScore;
    };

    const Pos ttBestMove = (entry != nullptr && entry->bestMove != TranspositionTable::INVALID_MOVE)
        ? TranspositionTable::decodeMove(entry->bestMove)
        : Pos();
    const bool sideToMoveIsBlack = board.isBlackTurn();
    const bool allowChildTTProbe = !options.lightweightTTOrdering;
    bool hasHistorySignal = false;

    for (const Pos& move : moves) {
        if (getHistoryScore(move, sideToMoveIsBlack) != 0) {
            hasHistorySignal = true;
            break;
        }
    }

    if (entry == nullptr && !hasHistorySignal) {
        return;
    }

    auto getValueTypePriority = [&](TTFlag flag) {
        if (isMax) {
            switch (flag) {
                case TTFlag::EXACT:       return 0;
                case TTFlag::LOWER_BOUND: return 1;
                case TTFlag::NONE:        return 2;
                case TTFlag::UPPER_BOUND: return 3;
            }
        } else {
            switch (flag) {
                case TTFlag::EXACT:       return 0;
                case TTFlag::UPPER_BOUND: return 1;
                case TTFlag::NONE:        return 2;
                case TTFlag::LOWER_BOUND: return 3;
            }
        }
        return 2;
    };

    vector<MoveOrderInfo> infos;
    infos.reserve(moves.size());

    for (const Pos& move : moves) {
        TTEntry childEntryStorage;
        const TTEntry* childEntry =
            allowChildTTProbe
                && (entry != nullptr || hasHistorySignal)
                && tt.probeCopy(getChildTTKey(move), &childEntryStorage, options.nonBlockingTTAccess)
                ? &childEntryStorage
                : nullptr;
        MoveOrderInfo info;
        info.move = move;
        info.isTTBest = !ttBestMove.isDefault() && move == ttBestMove;
        info.hasTT = childEntry != nullptr;
        info.flagPriority = childEntry != nullptr ? getValueTypePriority(childEntry->getFlag()) : getValueTypePriority(TTFlag::NONE);
        info.ttScore = childEntry != nullptr ? childEntry->score : 0;
        info.historyScore = getHistoryScore(move, sideToMoveIsBlack);
        infos.push_back(info);
    }

    std::stable_sort(infos.begin(), infos.end(), [&](const MoveOrderInfo& a, const MoveOrderInfo& b) {
        if (a.isTTBest != b.isTTBest) {
            return a.isTTBest;
        }
        if (a.hasTT != b.hasTT) {
            return a.hasTT;
        }
        if (a.flagPriority != b.flagPriority) {
            return a.flagPriority < b.flagPriority;
        }
        if (a.ttScore != b.ttScore) {
            return isMax ? (a.ttScore > b.ttScore) : (a.ttScore < b.ttScore);
        }
        if (a.historyScore != b.historyScore) {
            return a.historyScore > b.historyScore;
        }
        return false;
    });

    for (size_t i = 0; i < moves.size(); ++i) {
        moves[i] = infos[i].move;
    }
}

bool Search::isGameOver(Board& board) {
    Result result = board.getResult();
    return result != ONGOING;
}

Value Search::searchRootWithAspiration(int depth, MoveList* pv) {
    syncSharedRunningFlag();
    const Value fullAlpha(MIN_VALUE, Value::Type::UNKNOWN);
    const Value fullBeta(MAX_VALUE + 1, Value::Type::UNKNOWN);

    if (state.bestValue.getType() == Value::Type::UNKNOWN || !state.bestValue.isOnGoing()) {
        return abp(depth, true, fullAlpha, fullBeta, pv);
    }

    const int center = state.bestValue.getValue();
    int delta = ASPIRATION_START_DELTA;

    while (delta < (MAX_VALUE - MIN_VALUE)) {
        const int alphaScore = std::max(MIN_VALUE, center - delta);
        const int betaScore = std::min(MAX_VALUE + 1, center + delta);

        Value result = abp(
            depth,
            true,
            Value(alphaScore, Value::Type::UNKNOWN),
            Value(betaScore, Value::Type::UNKNOWN),
            pv
        );

        if (!searchActive() || result.getType() == Value::Type::EXACT) {
            return result;
        }

        const bool canWidenLow = (result.getType() == Value::Type::UPPER_BOUND) && alphaScore > MIN_VALUE;
        const bool canWidenHigh = (result.getType() == Value::Type::LOWER_BOUND) && betaScore < (MAX_VALUE + 1);
        if (!canWidenLow && !canWidenHigh) {
            break;
        }

        delta *= 2;
    }

    return abp(depth, true, fullAlpha, fullBeta, pv);
}
