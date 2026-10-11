#pragma once

// leaf probes look for the root side's VCF and may cash in 4-3 / 3-3 shortcuts
Search::QVCFContext Search::makeLeafQVCFContext() {
    QVCFContext context;
    context.attacker = rootBoard.isBlackTurn() ? BLACK : WHITE;
    context.keySalt = QVCF_TT_KEY;
    context.table = &tt;
    return context;
}

// threat probes serve either side, keep their own key space per attacker and their own table
Search::QVCFContext Search::makeThreatQVCFContext(Piece attacker) {
    QVCFContext context;
    context.attacker = attacker;
    context.keySalt = attacker == BLACK ? QVCF_THREAT_TT_KEY_BLACK : QVCF_THREAT_TT_KEY_WHITE;
    context.foursOnly = true;
    context.table = &threatTT;
    context.nodeLimit = THREAT_QVCF_NODE_LIMIT;
    return context;
}

bool Search::isQVCFAttackerTurn(Board& targetBoard, const QVCFContext& context) const {
    return (targetBoard.isBlackTurn() ? BLACK : WHITE) == context.attacker;
}

Result Search::getQVCFWinResult(const QVCFContext& context) const {
    return context.attacker == BLACK ? BLACK_WIN : WHITE_WIN;
}

uint64_t Search::getQVCFTTKey(Board& targetBoard, const QVCFContext& context) const {
    return getTTKey(targetBoard) ^ context.keySalt;
}

bool Search::enterQVCFNode(QVCFContext& context) {
    if (context.stopped) {
        return false;
    }

    const size_t nodeLimit = context.nodeLimit > 0 ? context.nodeLimit : options.leafVCFNodeLimit;
    if (nodeLimit > 0 && context.nodeCount >= nodeLimit) {
        context.stopped = true;
        return false;
    }

    context.nodeCount += 1;
    recordNodeVisit();
    if (!searchActive()) {
        context.stopped = true;
        return false;
    }

    return true;
}

void Search::moveQVCFTTBestFirst(CandidateList& moves, const Pos& bestMove) const {
    if (bestMove.isDefault()) {
        return;
    }

    const auto it = std::find(moves.begin(), moves.end(), bestMove);
    if (it != moves.end() && it != moves.begin()) {
        std::iter_swap(moves.begin(), it);
    }
}

bool Search::probeQVCFTT(Board& targetBoard, const QVCFContext& context, int remainingPly,
    Value& value, Pos& bestMove) const {
    TTEntry entryStorage;
    const uint64_t key = getQVCFTTKey(targetBoard, context);
    if (!context.table->probeCopy(key, &entryStorage)) {
        return false;
    }

    bestMove = entryStorage.bestMove != TranspositionTable::INVALID_MOVE
        ? TranspositionTable::decodeMove(entryStorage.bestMove)
        : Pos();

    if (entryStorage.getFlag() == TTFlag::EXACT && entryStorage.score != 0) {
        value = getTTValue(entryStorage);
        if (value.isWin() && value.getResultDepth() > remainingPly) {
            return false;
        }
        return true;
    }

    if (entryStorage.depth < getTTDepth(remainingPly)) {
        return false;
    }

    if (entryStorage.getFlag() == TTFlag::UPPER_BOUND && entryStorage.score == 0) {
        value = Value(0);
        return true;
    }

    return false;
}

void Search::storeQVCFWin(Board& targetBoard, const QVCFContext& context, Value value,
    int remainingPly, const Pos& bestMove) {
    context.table->store(
        getQVCFTTKey(targetBoard, context),
        encodeTTScore(value),
        getTTDepth(remainingPly),
        TTFlag::EXACT,
        TranspositionTable::encodeMove(bestMove)
    );
}

void Search::storeQVCFFail(Board& targetBoard, const QVCFContext& context, int remainingPly) {
    context.table->store(
        getQVCFTTKey(targetBoard, context),
        0,
        getTTDepth(remainingPly),
        TTFlag::UPPER_BOUND,
        TranspositionTable::INVALID_MOVE
    );
}

bool Search::tryResolveLeafVCF(MoveList* pv, Value& resolvedValue) {
    if (!options.leafVCFEnabled || state.qvcfDisabledAfterWin || options.leafVCFMaxPly <= 0) {
        return false;
    }

    QVCFContext context = makeLeafQVCFContext();

    MoveList vcfPV;
    Value value = qvcfSearch(options.leafVCFMaxPly, context, pv != nullptr ? &vcfPV : nullptr);

    if (!value.isWin()) {
        return false;
    }

    if (pv != nullptr) {
        *pv = vcfPV;
    }

    const Pos bestMove = !vcfPV.empty() ? vcfPV.front() : Pos();
    value.markQVCFDerived();
    state.qvcfDisabledAfterWin = true;
    if (searchActive()) {
        storeTT(board, value, 0, bestMove);
    }

    resolvedValue = value;
    return true;
}

// The opponent's VCF if the side to move passed, as a replayable fours-only line: VCF_THREAT
// with the line, NO_THREAT when there is none, UNSETTLED_THREAT when the probe ran out
// of nodes or its line does not replay.
Search::ThreatKind Search::findVCFThreatLine(MoveList& line) {
    const Piece attacker = board.isBlackTurn() ? WHITE : BLACK;
    if (!board.pass()) {
        return NO_THREAT;
    }

    QVCFContext context = makeThreatQVCFContext(attacker);
    Value value = qvcfAttack(options.leafVCFMaxPly, context, &line);
    ThreatKind kind = NO_THREAT;
    if (value.isWin()) {
        kind = searchActive() && replayVCFWins(board, line) ? VCF_THREAT : UNSETTLED_THREAT;
    } else if (context.stopped) {
        kind = UNSETTLED_THREAT;
    }
    board.undo();
    return kind;
}

Value Search::qvcfSearch(int remainingPly, QVCFContext& context, MoveList* pv) {
    return isQVCFAttackerTurn(board, context)
        ? qvcfAttack(remainingPly, context, pv)
        : qvcfDefend(remainingPly, context, pv);
}

Value Search::qvcfAttack(int remainingPly, QVCFContext& context, MoveList* pv) {
    if (pv != nullptr) {
        pv->clear();
    }

    const Result result = board.getResult();
    if (result == getQVCFWinResult(context)) {
        return Value(Value::Result::WIN, 0);
    }
    if (result != ONGOING || remainingPly <= 0) {
        return Value();
    }

    Value cachedValue;
    Pos cachedBestMove;
    // a win can outlive the entries of its line, so a caller that needs the line searches
    // the node again, the table's move first
    if (probeQVCFTT(board, context, remainingPly, cachedValue, cachedBestMove)
        && (pv == nullptr || !cachedValue.isWin())) {
        return cachedValue;
    }

    if (!enterQVCFNode(context)) {
        return Value();
    }

    Evaluator evaluator(board);
    Pos quickMove;
    Value quickValue = evaluator.quickWinCheck(&quickMove);
    // WIN(5) is a 4-3 / 3-3 shortcut; fours-only probes expand it into real fours instead
    const bool quickWinAllowed = !context.foursOnly || quickValue.getResultDepth() <= 3;
    if (quickValue.isWin() && quickValue.getResultDepth() <= remainingPly && quickWinAllowed) {
        if (pv != nullptr && !quickMove.isDefault()) {
            pv->push_back(quickMove);
        }
        storeQVCFWin(board, context, quickValue, remainingPly, quickMove);
        return quickValue;
    }
    if (quickValue.isLose()) {
        storeQVCFFail(board, context, remainingPly);
        return Value();
    }

    CandidateList moves;
    evaluator.getFours(moves);
    moveQVCFTTBestFirst(moves, cachedBestMove);
    if (moves.empty()) {
        storeQVCFFail(board, context, remainingPly);
        return Value();
    }

    for (const Pos& move : moves) {
        if (!board.move(move)) {
            continue;
        }
        context.table->prefetch(getQVCFTTKey(board, context));

        MoveList childPV;
        Value childValue = qvcfDefend(remainingPly - 1, context, pv != nullptr ? &childPV : nullptr);
        board.undo();

        if (!searchActive() || context.stopped) {
            return Value();
        }
        if (!childValue.isWin()) {
            continue;
        }

        childValue.increaseResultDepth();
        if (pv != nullptr) {
            pv->push_back(move);
            pv->insert(pv->end(), childPV.begin(), childPV.end());
        }
        storeQVCFWin(board, context, childValue, remainingPly, move);
        return childValue;
    }

    if (!context.stopped) {
        storeQVCFFail(board, context, remainingPly);
    }
    return Value();
}

Value Search::qvcfDefend(int remainingPly, QVCFContext& context, MoveList* pv) {
    if (pv != nullptr) {
        pv->clear();
    }

    const Result result = board.getResult();
    if (result == getQVCFWinResult(context)) {
        return Value(Value::Result::WIN, 0);
    }
    if (result != ONGOING || remainingPly <= 0) {
        return Value();
    }

    Value cachedValue;
    Pos cachedBestMove;
    // as in qvcfAttack: a caller that needs the line does not take a win from the table
    if (probeQVCFTT(board, context, remainingPly, cachedValue, cachedBestMove)
        && (pv == nullptr || !cachedValue.isWin())) {
        return cachedValue;
    }

    if (!enterQVCFNode(context)) {
        return Value();
    }

    Evaluator evaluator(board);
    Pos quickMove;
    Value quickValue = evaluator.quickWinCheck(&quickMove);
    if (quickValue.isLose()) {
        quickValue.invert();
        if (quickValue.getResultDepth() <= remainingPly) {
            if (pv != nullptr && !quickMove.isDefault()) {
                pv->push_back(quickMove);
            }
            storeQVCFWin(board, context, quickValue, remainingPly, quickMove);
            return quickValue;
        }
    }
    if (quickValue.isWin()) {
        storeQVCFFail(board, context, remainingPly);
        return Value();
    }
    if (!board.hasCompositePattern(context.attacker, WINNING)) {
        storeQVCFFail(board, context, remainingPly);
        return Value();
    }

    const Pos defendMove = evaluator.getOppoWinningDefend();
    if (defendMove.isDefault()) {
        Value value(Value::Result::WIN, 0);
        storeQVCFWin(board, context, value, remainingPly, Pos());
        return value;
    }

    if (!board.move(defendMove)) {
        Value value(Value::Result::WIN, 0);
        storeQVCFWin(board, context, value, remainingPly, Pos());
        return value;
    }
    context.table->prefetch(getQVCFTTKey(board, context));

    MoveList childPV;
    Value childValue = qvcfAttack(remainingPly - 1, context, pv != nullptr ? &childPV : nullptr);
    board.undo();

    if (!searchActive() || context.stopped) {
        return Value();
    }
    if (!childValue.isWin()) {
        storeQVCFFail(board, context, remainingPly);
        return Value();
    }

    childValue.increaseResultDepth();
    if (pv != nullptr) {
        pv->push_back(defendMove);
        pv->insert(pv->end(), childPV.begin(), childPV.end());
    }
    storeQVCFWin(board, context, childValue, remainingPly, defendMove);
    return childValue;
}
