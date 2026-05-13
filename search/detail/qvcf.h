#pragma once

bool Search::isRootAttackerTurn(Board& targetBoard) {
    return targetBoard.isBlackTurn() == rootBoard.isBlackTurn();
}

Result Search::getRootWinResult() {
    return rootBoard.isBlackTurn() ? BLACK_WIN : WHITE_WIN;
}

uint64_t Search::getQVCFTTKey(Board& targetBoard) const {
    return getTTKey(targetBoard) ^ QVCF_TT_KEY;
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

void Search::moveQVCFTTBestFirst(MoveList& moves, const Pos& bestMove) const {
    if (bestMove.isDefault()) {
        return;
    }

    const auto it = std::find(moves.begin(), moves.end(), bestMove);
    if (it != moves.end() && it != moves.begin()) {
        std::iter_swap(moves.begin(), it);
    }
}

bool Search::probeQVCFTT(Board& targetBoard, int remainingPly, Value& value, Pos& bestMove) const {
    TTEntry entryStorage;
    const uint64_t key = getQVCFTTKey(targetBoard);
    if (!tt.probeCopy(key, &entryStorage)) {
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

void Search::storeQVCFWin(Board& targetBoard, Value value, int remainingPly, const Pos& bestMove) {
    tt.store(
        getQVCFTTKey(targetBoard),
        encodeTTScore(value),
        getTTDepth(remainingPly),
        TTFlag::EXACT,
        TranspositionTable::encodeMove(bestMove)
    );
}

void Search::storeQVCFFail(Board& targetBoard, int remainingPly) {
    tt.store(
        getQVCFTTKey(targetBoard),
        0,
        getTTDepth(remainingPly),
        TTFlag::UPPER_BOUND,
        TranspositionTable::INVALID_MOVE
    );
}

void Search::appendQVCFTTPV(Board tempBoard, MoveList& pv, int remainingPly) const {
    for (int ply = 0; ply < remainingPly; ++ply) {
        TTEntry entryStorage;
        const uint64_t key = getQVCFTTKey(tempBoard);
        if (!tt.probeCopy(key, &entryStorage)) {
            break;
        }
        if (entryStorage.getFlag() != TTFlag::EXACT || entryStorage.score == 0) {
            break;
        }
        if (entryStorage.bestMove == TranspositionTable::INVALID_MOVE) {
            break;
        }

        const Pos move = TranspositionTable::decodeMove(entryStorage.bestMove);
        if (move.isDefault() || !tempBoard.move(move)) {
            break;
        }

        pv.push_back(move);
        if (tempBoard.getResult() != ONGOING) {
            break;
        }
    }
}

bool Search::tryResolveLeafVCF(MoveList* pv, Value& resolvedValue) {
    if (!options.leafVCFEnabled || state.qvcfDisabledAfterWin || options.leafVCFMaxPly <= 0) {
        return false;
    }

    QVCFContext context;

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

Value Search::qvcfSearch(int remainingPly, QVCFContext& context, MoveList* pv) {
    return isRootAttackerTurn(board)
        ? qvcfAttack(remainingPly, context, pv)
        : qvcfDefend(remainingPly, context, pv);
}

Value Search::qvcfAttack(int remainingPly, QVCFContext& context, MoveList* pv) {
    if (pv != nullptr) {
        pv->clear();
    }

    const Result result = board.getResult();
    if (result == getRootWinResult()) {
        return Value(Value::Result::WIN, 0);
    }
    if (result != ONGOING || remainingPly <= 0) {
        return Value();
    }

    Value cachedValue;
    Pos cachedBestMove;
    if (probeQVCFTT(board, remainingPly, cachedValue, cachedBestMove)) {
        if (cachedValue.isWin() && pv != nullptr) {
            appendQVCFTTPV(board, *pv, remainingPly);
        }
        return cachedValue;
    }

    if (!enterQVCFNode(context)) {
        return Value();
    }

    Evaluator evaluator(board);
    Pos quickMove;
    Value quickValue = evaluator.quickWinCheck(&quickMove);
    if (quickValue.isWin() && quickValue.getResultDepth() <= remainingPly) {
        if (pv != nullptr && !quickMove.isDefault()) {
            pv->push_back(quickMove);
        }
        storeQVCFWin(board, quickValue, remainingPly, quickMove);
        return quickValue;
    }
    if (quickValue.isLose()) {
        storeQVCFFail(board, remainingPly);
        return Value();
    }

    MoveList moves = evaluator.getFours();
    moveQVCFTTBestFirst(moves, cachedBestMove);
    if (moves.empty()) {
        storeQVCFFail(board, remainingPly);
        return Value();
    }

    for (const Pos& move : moves) {
        if (!board.move(move)) {
            continue;
        }
        tt.prefetch(getQVCFTTKey(board));

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
        storeQVCFWin(board, childValue, remainingPly, move);
        return childValue;
    }

    if (!context.stopped) {
        storeQVCFFail(board, remainingPly);
    }
    return Value();
}

Value Search::qvcfDefend(int remainingPly, QVCFContext& context, MoveList* pv) {
    if (pv != nullptr) {
        pv->clear();
    }

    const Result result = board.getResult();
    if (result == getRootWinResult()) {
        return Value(Value::Result::WIN, 0);
    }
    if (result != ONGOING || remainingPly <= 0) {
        return Value();
    }

    Value cachedValue;
    Pos cachedBestMove;
    if (probeQVCFTT(board, remainingPly, cachedValue, cachedBestMove)) {
        if (cachedValue.isWin() && pv != nullptr) {
            appendQVCFTTPV(board, *pv, remainingPly);
        }
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
            storeQVCFWin(board, quickValue, remainingPly, quickMove);
            return quickValue;
        }
    }
    if (quickValue.isWin()) {
        storeQVCFFail(board, remainingPly);
        return Value();
    }
    const Piece rootPiece = rootBoard.isBlackTurn() ? BLACK : WHITE;
    if (!board.hasCompositePattern(rootPiece, WINNING)) {
        storeQVCFFail(board, remainingPly);
        return Value();
    }

    const Pos defendMove = evaluator.getOppoWinningDefend();
    if (defendMove.isDefault()) {
        Value value(Value::Result::WIN, 0);
        storeQVCFWin(board, value, remainingPly, Pos());
        return value;
    }

    if (!board.move(defendMove)) {
        Value value(Value::Result::WIN, 0);
        storeQVCFWin(board, value, remainingPly, Pos());
        return value;
    }
    tt.prefetch(getQVCFTTKey(board));

    MoveList childPV;
    Value childValue = qvcfAttack(remainingPly - 1, context, pv != nullptr ? &childPV : nullptr);
    board.undo();

    if (!searchActive() || context.stopped) {
        return Value();
    }
    if (!childValue.isWin()) {
        storeQVCFFail(board, remainingPly);
        return Value();
    }

    childValue.increaseResultDepth();
    if (pv != nullptr) {
        pv->push_back(defendMove);
        pv->insert(pv->end(), childPV.begin(), childPV.end());
    }
    storeQVCFWin(board, childValue, remainingPly, defendMove);
    return childValue;
}
