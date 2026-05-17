#pragma once

bool Search::tryResolveFromTT(int depth, Value& alpha, Value& beta, MoveList* pv,
    TTEntry& ttEntryStorage, const TTEntry*& ttEntry, Value& resolvedValue) {
    const uint64_t key = getTTKey(board);
    ttEntry = tt.probeCopy(key, &ttEntryStorage) ? &ttEntryStorage : nullptr;

    if (ttEntry == nullptr) {
        return false;
    }

    Value ttValue = getTTValue(*ttEntry);
    const TTFlag ttFlag = ttEntry->getFlag();
    const bool isRootNode = board.getPath().size() == rootBoard.getPath().size();
    const bool allowTerminalExactDepthBypass = !isRootNode;

    if (allowTerminalExactDepthBypass &&
        ttFlag == TTFlag::EXACT &&
        (ttValue.isWin() || ttValue.isLose())) {
        if (pv != nullptr) {
            appendTTPV(board, *pv);
        }
        resolvedValue = ttValue;
        return true;
    }

    if (ttEntry->depth < getTTDepth(depth)) {
        return false;
    }

    if (ttFlag == TTFlag::EXACT) {
        if (pv != nullptr) {
            appendTTPV(board, *pv);
        }
        resolvedValue = ttValue;
        return true;
    }
    if (ttFlag == TTFlag::LOWER_BOUND && ttValue >= beta) {
        if (pv != nullptr) appendTTPV(board, *pv);
        resolvedValue = ttValue;
        return true;
    }
    if (ttFlag == TTFlag::UPPER_BOUND && ttValue <= alpha) {
        if (pv != nullptr) appendTTPV(board, *pv);
        resolvedValue = ttValue;
        return true;
    }

    if (ttFlag == TTFlag::LOWER_BOUND && ttValue > alpha) {
        alpha = ttValue;
    } else if (ttFlag == TTFlag::UPPER_BOUND && ttValue < beta) {
        beta = ttValue;
    }

    return false;
}

int Search::getShallowMoveLimit(Evaluator& evaluator, int depth, bool attackerTurn) {
    // defender turns must enumerate fully (true longest defense)
    if (!attackerTurn) {
        return std::numeric_limits<int>::max();
    }
    // DEFENSIVE root uses broad candidates; truncating would miss non-LOSE choices
    if (options.mode == Mode::DEFENSIVE
        && board.getPath().size() == rootBoard.getPath().size()) {
        return std::numeric_limits<int>::max();
    }
    // defending against an opposing mate or 4-3 threat: enumerate all candidates,
    // otherwise the search may cut off the only saving line and falsely report LOSE.
    if (evaluator.isOppoMateExist() || evaluator.isOppoFourThreeExist()) {
        return std::numeric_limits<int>::max();
    }
    // attacker forcing with no opposing threat — shallow truncation allowed
    return (depth <= 3) ? 4 : (depth <= 5) ? 6 : std::numeric_limits<int>::max();
}

Value Search::evaluateLeafNode(bool isMax, int depth) {
    Value val = evaluateTacticalSummary(board);
    if (!isMax) {
        val.invert();
    }
    if (searchActive()) {
        storeTT(board, val, depth, Pos());
    }
    return val;
}

Value Search::evaluateThreatBrokenLeaf(bool isMax, int depth) {
    static_cast<void>(depth);
    Value val(Value::Result::WIN, 0);
    if (!isMax) {
        val.invert();
    }
    if (searchActive()) {
        storeTT(board, val, depth, Pos());
    }
    return val;
}

bool Search::tryResolveQuickWin(Evaluator& evaluator, bool isMax, int depth, MoveList* pv, Value& resolvedValue) {
    Pos bestMove;
    Value value = evaluator.quickWinCheck(&bestMove);
    if (value.isOnGoing()) {
        return false;
    }

    if (!isMax) {
        value.invert();
    }

    if (pv != nullptr && !bestMove.isDefault()) {
        pv->push_back(bestMove);
    }
    if (searchActive()) {
        storeTT(board, value, depth, bestMove);
    }

    resolvedValue = value;
    return true;
}

Search::ChildSearchResult Search::searchChildPVS(int depth, bool isMax, size_t moveIndex, Value alpha, Value beta,
    Value bestVal, MoveList* pv, bool requireExactBest) {
    ChildSearchResult result;

    Value nextAlpha = alpha;
    Value nextBeta = beta;
    Value nextBestVal = bestVal;
    nextAlpha.decreaseResultDepth();
    nextBeta.decreaseResultDepth();
    nextBestVal.decreaseResultDepth();

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
        (isMax && result.value > nextAlpha && result.value < nextBeta) ||
        (!isMax && result.value < nextBeta && result.value > nextAlpha);
    const bool couldBecomeBest =
        requireExactBest
            && pv != nullptr
            && ((isMax && result.value > nextBestVal) || (!isMax && result.value < nextBestVal));

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
    pollMonitorIfDue();
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

    if (isGameOver(board)) {
        return evaluateLeafNode(isMax, depth);
    }

    Evaluator evaluator(board);
    Value quickValue;
    if (tryResolveQuickWin(evaluator, isMax, depth, pv, quickValue)) {
        return quickValue;
    }

    if (depth == 0) {
        Value vcfValue;
        if (tryResolveLeafVCF(pv, vcfValue)) {
            return vcfValue;
        }
        return evaluateLeafNode(isMax, depth);
    }

    const bool attackerTurn = (board.isBlackTurn() == rootBoard.isBlackTurn());
    // threat-broken WIN is a VCT-only conclusion ("attacker can no longer force"):
    // applying it in DEFENSIVE mode would invent false LOSE/WIN at quiet positions.
    const bool threatBrokenLeaf = options.mode == Mode::VCT
        && !attackerTurn && !evaluator.isOppoMateExist();
    MoveList moves = getCandidates(evaluator, isMax);
    if (moves.empty()) {
        return threatBrokenLeaf
            ? evaluateThreatBrokenLeaf(isMax, depth)
            : evaluateLeafNode(isMax, depth);
    }

    sortChildNodes(moves, isMax, ttEntry);

    // sentinel sits outside [MIN_VALUE, MAX_VALUE] so any real child (incl. LOSE/WIN)
    // wins the first comparison — without this, LOSE-only nodes leave bestMove unset.
    Value bestVal = isMax
        ? Value(MIN_VALUE - 1, Value::Type::UNKNOWN)
        : Value(MAX_VALUE + 1, Value::Type::UNKNOWN);
    Pos bestMove;
    MoveList bestChildPV;
    MoveList searchedMoves;
    searchedMoves.reserve(moves.size());

    const int shallowMoveLimit = getShallowMoveLimit(evaluator, depth, attackerTurn);
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
        tt.prefetch(getTTKey(board));

        searchedAny = true;
        searchedMoves.push_back(move);
        recordNodeVisit();
        const size_t nodeCountBeforeMove = isRootNode ? monitor.getVisitCnt() : 0;
        const double elapsedBeforeMove = isRootNode ? monitor.getElapsedTime() : 0.0;

        ChildSearchResult childResult =
            searchChildPVS(depth, isMax, i, alpha, beta, bestVal, pv, isRootNode);

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
        return evaluateLeafNode(isMax, depth);
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

    Pos sureMove = evaluator.getSureMove();
    if (!sureMove.isDefault()) {
        moves.push_back(sureMove);
        return moves;
    }

    // DEFENSIVE: at root broaden to full candidates when opp is quiet;
    // non-root stays threats-only to limit fanout.
    if (options.mode == Mode::DEFENSIVE) {
        const bool atRoot = (board.getPath().size() == rootBoard.getPath().size());

        if (evaluator.isOppoMateExist()) {
            moves = evaluator.getThreatDefend();
            MoveList fours = evaluator.getFours();
            moves.insert(moves.end(), fours.begin(), fours.end());
        } else if (evaluator.isOppoFourThreeExist()) {
            moves = evaluator.getFourThreeDefend();
            MoveList fours = evaluator.getFours();
            moves.insert(moves.end(), fours.begin(), fours.end());
        } else {
            if (atRoot) {
                moves = evaluator.getCandidates();
                return moves;
            }
            moves = evaluator.getThreats();
            MoveList makers = evaluator.getFourThreeMakers();
            moves.insert(moves.end(), makers.begin(), makers.end());
        }
        return moves;
    }

    if (options.mode == Mode::VCT) {
        const bool attackerTurn = (board.isBlackTurn() == rootBoard.isBlackTurn());
        if (attackerTurn) {
            moves = evaluator.getThreats();
            MoveList makers = evaluator.getFourThreeMakers();
            moves.insert(moves.end(), makers.begin(), makers.end());
        } else if (evaluator.isOppoMateExist()) {
            moves = evaluator.getThreatDefend();
            MoveList fours = evaluator.getFours();
            moves.insert(moves.end(), fours.begin(), fours.end());
        } else if (evaluator.isOppoFourThreeExist()) {
            moves = evaluator.getFourThreeDefend();
            MoveList fours = evaluator.getFours();
            moves.insert(moves.end(), fours.begin(), fours.end());
        }
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
        int cellScore;
    };

    const Pos ttBestMove = (entry != nullptr && entry->bestMove != TranspositionTable::INVALID_MOVE)
        ? TranspositionTable::decodeMove(entry->bestMove)
        : Pos();
    const bool sideToMoveIsBlack = board.isBlackTurn();
    bool hasHistorySignal = false;

    for (const Pos& move : moves) {
        if (getHistoryScore(move, sideToMoveIsBlack) != 0) {
            hasHistorySignal = true;
            break;
        }
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

    // Attacker (isMax) prefers self-attack score; defender wants to block opponent's most
    // threatening spot, so uses opponent's score. Matches the evaluator's previous sort intent.
    const Piece sideToMovePiece = sideToMoveIsBlack ? BLACK : WHITE;
    const Piece opposingPiece   = sideToMoveIsBlack ? WHITE : BLACK;
    const Piece scorePiece = isMax ? sideToMovePiece : opposingPiece;
    const bool shouldProbeChildren = (entry != nullptr || hasHistorySignal);
    for (size_t mi = 0; mi < moves.size(); ++mi) {
        const Pos& move = moves[mi];
        if (shouldProbeChildren && mi + 1 < moves.size()) {
            tt.prefetch(getChildTTKey(moves[mi + 1]));
        }
        TTEntry childEntryStorage;
        const TTEntry* childEntry =
            shouldProbeChildren
                && tt.probeCopy(getChildTTKey(move), &childEntryStorage)
                ? &childEntryStorage
                : nullptr;
        MoveOrderInfo info;
        info.move = move;
        info.isTTBest = !ttBestMove.isDefault() && move == ttBestMove;
        info.hasTT = childEntry != nullptr;
        info.flagPriority = childEntry != nullptr ? getValueTypePriority(childEntry->getFlag()) : getValueTypePriority(TTFlag::NONE);
        info.ttScore = childEntry != nullptr ? childEntry->score : 0;
        info.historyScore = getHistoryScore(move, sideToMoveIsBlack);
        info.cellScore = board.getCell(move).getScore(scorePiece);
        infos.push_back(info);
    }

    std::stable_sort(infos.begin(), infos.end(), [&](const MoveOrderInfo& a, const MoveOrderInfo& b) {
        if (a.isTTBest != b.isTTBest) {
            return a.isTTBest;
        }
        if (a.hasTT != b.hasTT) {
            return a.hasTT;
        }
        if (a.ttScore != b.ttScore) {
            return isMax ? (a.ttScore > b.ttScore) : (a.ttScore < b.ttScore);
        }
        // bound type refines tied ttScore (esp. for mate values):
        //   Max prefers LOWER_BOUND (actual value could be even higher → faster mate)
        //   Min prefers UPPER_BOUND (actual value could be even lower → slower mate)
        // getValueTypePriority(flag) already encodes that via isMax. Smaller = better.
        if (a.flagPriority != b.flagPriority) {
            return a.flagPriority < b.flagPriority;
        }
        if (a.cellScore != b.cellScore) {
            return a.cellScore > b.cellScore;
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
