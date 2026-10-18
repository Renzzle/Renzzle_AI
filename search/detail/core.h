#pragma once

bool Search::tryResolveFromTT(int depth, Value& alpha, Value& beta, MoveList* pv,
    TTEntry& ttEntryStorage, const TTEntry*& ttEntry, Value& resolvedValue) {
    const uint64_t key = getTTKey(board);
    ttEntry = tt.probeCopy(key, &ttEntryStorage) ? &ttEntryStorage : nullptr;

    if (ttEntry == nullptr) {
        return false;
    }

    // root: keep ttEntry for move ordering only — never resolve or tighten the
    // window from it. The root's own stale bound (e.g. an UPPER_BOUND left by a
    // failed aspiration window) can shrink beta enough that the first move causes
    // a cutoff, leaving lastRootStats partial and the PV rebuilt from stale TT
    // breadcrumbs instead of a verified line.
    const bool isRootNode = board.getPath().size() == rootBoard.getPath().size();
    if (isRootNode) {
        return false;
    }

    Value ttValue = getTTValue(*ttEntry);
    const TTFlag ttFlag = ttEntry->getFlag();

    if (ttFlag == TTFlag::EXACT &&
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

int Search::getShallowMoveLimit(Evaluator& evaluator, int depth, bool attackerTurn, ThreatKind threat) {
    // defender turns must enumerate fully (true longest defense)
    if (!attackerTurn) {
        return std::numeric_limits<int>::max();
    }
    // DEFENSIVE root uses broad candidates; truncating would miss non-LOSE choices
    if (options.mode == Mode::DEFENSIVE
        && board.getPath().size() == rootBoard.getPath().size()) {
        return std::numeric_limits<int>::max();
    }
    // defending against an opposing mate, 4-3 or VCF threat: enumerate all candidates,
    // otherwise the search may cut off the only saving line and falsely report LOSE.
    if (evaluator.isOppoMateExist() || evaluator.isOppoFourThreeExist() || threat == VCF_THREAT) {
        return std::numeric_limits<int>::max();
    }
    // attacker forcing with no opposing threat — shallow truncation allowed
    return (depth <= 3) ? 4 : (depth <= 5) ? 6 : std::numeric_limits<int>::max();
}

Value Search::evaluateStatic() {
    if (options.mode != Mode::DEFENSIVE) {
        return evaluateTacticalSummary(board);
    }
    const Piece attacker = rootBoard.isBlackTurn() ? WHITE : BLACK;
    return evaluateTacticalSummary(board, attacker, DEFENSIVE_ATTACKER_WEIGHT);
}

Value Search::evaluateLeafNode(bool isMax, int depth) {
    Value val = evaluateStatic();
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

// null window on the LOSE boundary: proves a losing root move LOSE regardless of order
bool Search::tryResolveLosingRootMove(int depth, bool isMax, Value& resolvedValue) {
    const Value loseAlpha(MIN_VALUE, Value::Type::UNKNOWN);
    Value loseBeta = loseAlpha;
    loseBeta += 1;

    Value value = abp(depth - 1, !isMax, loseAlpha, loseBeta, nullptr);
    if (!searchActive() || !value.isLose()) {
        return false;
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
        MoveList* childPV = pv != nullptr ? &result.pv : nullptr;
        result.value = abp(depth - 1, !isMax, nextAlpha, nextBeta, childPV);
        result.hasExactPV = childPV != nullptr && result.value.getType() == Value::Type::EXACT;
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

void Search::updateHistoryFromNode(int depth, const Pos& bestMove,
    const uint8_t* searchedMoveCodes, size_t searchedMoveCount,
    bool causedCutoff, Value result, bool sideToMoveIsBlack) {
    if (!searchActive() || bestMove.isDefault()) {
        return;
    }

    const int bonus = std::max(1, std::min(depth * depth, 256));
    if (causedCutoff || result.getType() == Value::Type::LOWER_BOUND) {
        updateHistoryScore(bestMove, sideToMoveIsBlack, bonus);
        // killer slots only remember cutoff moves that make at least a four for the
        // mover: those transfer to sibling nodes, while quiet cutoff moves would just
        // displace the static block-the-biggest-threat order and grow the tree
        if (board.getCell(bestMove).getScore(sideToMoveIsBlack ? BLACK : WHITE)
            >= KILLER_MIN_FOUR_SCORE) {
            updateKillerMove(getSearchPly(), packMoveCode(bestMove));
        }

        const int penalty = std::max(1, bonus / 2);
        for (size_t i = 0; i < searchedMoveCount; ++i) {
            const uint8_t code = searchedMoveCodes[i];
            const Pos move(code >> 4, code & 0x0F);
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
        state.lastRootSearchComplete = false;
    }

    // mate-distance pruning: an ongoing position can never win faster than
    // WIN(1) (game ends on the very next move) nor lose faster than LOSE(0).
    // If the window already guarantees an equal-or-faster result, this subtree
    // cannot change the outcome — return the bound before paying for TT probes,
    // Evaluator construction, and child moves. Terminal positions are excluded:
    // they may evaluate to WIN(0)/LOSE(0) and are handled by isGameOver below.
    if (board.getResult() == ONGOING) {
        Value fastestWin(Value::Result::WIN, 1);
        if (alpha >= fastestWin) {
            fastestWin.setType(Value::Type::UPPER_BOUND);
            return fastestWin;
        }
        Value fastestLose(Value::Result::LOSE, 0);
        if (beta <= fastestLose) {
            fastestLose.setType(Value::Type::LOWER_BOUND);
            return fastestLose;
        }
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
    ThreatKind threat = NO_THREAT;
    CandidateList moves = getCandidates(evaluator, isMax, depth, threat);
    if (moves.empty()) {
        // unsettled: a plain leaf, so a deeper iteration probes again
        return threatBrokenLeaf && threat != UNSETTLED_THREAT
            ? evaluateThreatBrokenLeaf(isMax, depth)
            : evaluateLeafNode(isMax, depth);
    }

    // DEFENSIVE root defends when the opponent threatens; elsewhere Max attacks
    const bool defensiveRoot = options.mode == Mode::DEFENSIVE && isRootNode;
    const bool defending = defensiveRoot
        ? (evaluator.isOppoMateExist() || evaluator.isOppoFourThreeExist() || threat == VCF_THREAT)
        : !isMax;
    sortChildNodes(moves, isMax, defending, ttEntry);

    // sentinel sits outside [MIN_VALUE, MAX_VALUE] so any real child (incl. LOSE/WIN)
    // wins the first comparison — without this, LOSE-only nodes leave bestMove unset.
    Value bestVal = isMax
        ? Value(MIN_VALUE - 1, Value::Type::UNKNOWN)
        : Value(MAX_VALUE + 1, Value::Type::UNKNOWN);

    // stand pat: an unforced DEFENSIVE interior side may play quietly instead, so its
    // threats all failing doesn't lose; the static value stands in for that move
    const bool standPat = options.mode == Mode::DEFENSIVE && !isRootNode && threat != VCF_THREAT
        && !evaluator.isOppoMateExist() && !evaluator.isOppoFourThreeExist();
    if (standPat) {
        Value standValue = evaluateStatic();
        if (!isMax) {
            standValue.invert();
        }
        if (isMax ? standValue >= beta : standValue <= alpha) {
            standValue.setType(isMax ? Value::Type::LOWER_BOUND : Value::Type::UPPER_BOUND);
            return standValue;
        }
        bestVal = standValue;
        if (isMax && standValue > alpha) {
            alpha = standValue;
        }
        if (!isMax && standValue < beta) {
            beta = standValue;
        }
    }
    Pos bestMove;
    MoveList bestChildPV;
    uint8_t searchedMoveCodes[BOARD_SIZE * BOARD_SIZE];
    size_t searchedMoveCount = 0;

    const int shallowMoveLimit = getShallowMoveLimit(evaluator, depth, attackerTurn, threat);
    const bool sideToMoveIsBlack = board.isBlackTurn();
    const Pos ttBestMove = (ttEntry != nullptr && ttEntry->bestMove != TranspositionTable::INVALID_MOVE)
        ? TranspositionTable::decodeMove(ttEntry->bestMove)
        : Pos();

    // elimination needs all but one move proven LOSE; stop checking after a 2nd survivor
    int rootSurvivorCount = 1;
    bool searchedAny = false;
    bool causedCutoff = false;
    bool searchedAll = true;

    for (size_t i = 0; i < moves.size(); ++i) {
        if (static_cast<int>(i) >= shallowMoveLimit) {
            searchedAll = false;
            break;
        }

        const Pos move = moves[i];
        // prefetch before moving: the load overlaps the board update
        tt.prefetch(getChildTTKey(move));
        if (!board.move(move)) {
            continue;
        }

        searchedAny = true;
        searchedMoveCodes[searchedMoveCount++] =
            static_cast<uint8_t>((move.getX() << 4) | move.getY());
        recordNodeVisit();
        const size_t nodeCountBeforeMove = isRootNode ? monitor.getVisitCnt() : 0;
        const double elapsedBeforeMove = isRootNode ? monitor.getElapsedTime() : 0.0;

        ChildSearchResult childResult;
        bool provenLose = false;
        if (defensiveRoot && defending && i > 0 && rootSurvivorCount < 2
            && bestVal.isOnGoing() && bestVal.getType() != Value::Type::UNKNOWN) {
            provenLose = tryResolveLosingRootMove(depth, isMax, childResult.value);
            if (!provenLose) {
                ++rootSurvivorCount;
            }
        }
        if (!provenLose) {
            childResult = searchChildPVS(depth, isMax, i, alpha, beta, bestVal, pv, isRootNode);
        }

        board.undo();
        if (!searchActive()) {
            searchedAll = false;
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
            searchedAll = false;
            break;
        }
    }

    if (isRootNode) {
        state.lastRootSearchComplete = searchedAny && searchedAll;
    }

    if (!searchedAny) {
        return evaluateLeafNode(isMax, depth);
    }

    Value result = finalizeNodeValue(isMax, originalAlpha, originalBeta, alpha, beta, bestVal);
    updateHistoryFromNode(depth, bestMove, searchedMoveCodes, searchedMoveCount,
        causedCutoff, result, sideToMoveIsBlack);

    if (searchActive()) {
        storeTT(board, result, depth, bestMove);
    }

    rebuildPV(bestMove, bestChildPV, result, pv);
    return result;
}

Value Search::evaluateNode(Evaluator& evaluator) {
    return evaluator.evaluateTactical();
}

CandidateList Search::getCandidates(Evaluator& evaluator, bool isMax, int depth, ThreatKind& threat) {
    CandidateList moves;

    Pos sureMove = evaluator.getSureMove();
    if (!sureMove.isDefault()) {
        moves.push_back(sureMove);
        return moves;
    }

    const bool patternThreat = evaluator.isOppoMateExist() || evaluator.isOppoFourThreeExist();

    // DEFENSIVE: the side to move answers an open three, 4-3 or VCF threat as VCT's
    // defender does. With none, root broadens to full candidates; deeper nodes keep to
    // their own threats and may stand pat.
    if (options.mode == Mode::DEFENSIVE) {
        const bool atRoot = (board.getPath().size() == rootBoard.getPath().size());

        if (patternThreat) {
            // a move still has to be chosen when nothing refutes or the threat is fake
            if (getThreatDefense(moves, true, false) == NO_THREAT || moves.empty()) {
                getPatternThreatDefend(evaluator, moves);
            }
            return moves;
        }

        const bool vcfThreat = state.vcfThreats
            && getThreatDefense(moves, false, true) == VCF_THREAT;
        if (atRoot) {
            // with no refutation the broad set picks the longest defense
            if (vcfThreat && !moves.empty()) {
                threat = VCF_THREAT;
                return moves;
            }
            evaluator.getCandidates(moves);
            // pure blocks too: holding out can mean cutting the opponent's shapes first
            CandidateList blocks;
            evaluator.getShapeBlocks(blocks);
            appendUniqueMoves(moves, blocks);
            return moves;
        }

        // the refutations, if any, plus the side's own threats, which may win first
        if (vcfThreat) {
            threat = VCF_THREAT;
        }
        CandidateList ownMoves;
        evaluator.getThreats(ownMoves);
        appendUniqueMoves(moves, ownMoves);
        evaluator.getFourThreeMakers(ownMoves);
        appendUniqueMoves(moves, ownMoves);
        return moves;
    }

    if (options.mode == Mode::VCT) {
        const bool attackerTurn = (board.isBlackTurn() == rootBoard.isBlackTurn());
        if (attackerTurn) {
            evaluator.getThreats(moves);
            CandidateList makers;
            evaluator.getFourThreeMakers(makers);
            appendUniqueMoves(moves, makers);
            if (depth >= VCF_THREAT_MIN_DEPTH && state.vcfThreats) {
                CandidateList vcfMakers;
                evaluator.getVCFThreatMakers(vcfMakers);
                appendUniqueMoves(moves, vcfMakers);
            }
        } else {
            // no threat leaves moves empty: the attacker's threat is broken (or not real)
            threat = getThreatDefense(moves, patternThreat, state.vcfThreats);
            if ((threat == PATTERN_THREAT || threat == VCF_THREAT) && moves.empty()) {
                // every move loses to the line; search some move to prove it
                if (patternThreat) {
                    getPatternThreatDefend(evaluator, moves);
                } else {
                    appendFirstLegalMove(moves);
                }
            }
        }
    }

    return moves;
}

// The side to move's complete defense against the opponent's quickest win: every move
// that breaks it. Open threes and 4-3s (patternThreat) come first; with vcfThreats, any
// VCF the opponent would have after a pass counts too. The result is cached per position.
Search::ThreatKind Search::getThreatDefense(CandidateList& moves, bool patternThreat, bool vcfThreats) {
    moves.clear();
    if (!patternThreat && !vcfThreats) {
        return NO_THREAT;
    }

    // the TT key carries the stage salt, and a stage always asks with the same flags
    const uint64_t key = getTTKey(board);
    ThreatDefenseEntry& cached = threatDefenseCache[key & (THREAT_DEFENSE_CACHE_SIZE - 1)];
    if (cached.valid && cached.key == key) {
        for (uint8_t i = 0; i < cached.count; ++i) {
            moves.push_back(Pos(cached.moves[i] >> 4, cached.moves[i] & 0x0F));
        }
        return static_cast<ThreatKind>(cached.kind);
    }

    ThreatKind kind = NO_THREAT;
    if (patternThreat) {
        // open threes come straight from line patterns; 4-3s are played out
        MoveList line;
        if (collectOpenFourDefense(board, moves)) {
            kind = PATTERN_THREAT;
        } else if (findThreatLine(board, line)) {
            collectVCFRefutations(board, line, moves);
            kind = PATTERN_THREAT;
        }
    }
    if (kind == NO_THREAT && vcfThreats) {
        MoveList line;
        kind = findVCFThreatLine(line);
        if (kind == VCF_THREAT) {
            collectVCFRefutations(board, line, moves);
        }
    }
    // unsettled results are not cached
    if (!searchActive() || kind == UNSETTLED_THREAT) {
        return kind;
    }

    if (moves.size() <= cached.moves.size()) {
        cached.key = key;
        cached.valid = true;
        cached.kind = static_cast<uint8_t>(kind);
        cached.count = static_cast<uint8_t>(moves.size());
        for (size_t i = 0; i < moves.size(); ++i) {
            cached.moves[i] = static_cast<uint8_t>((moves[i].getX() << 4) | moves[i].getY());
        }
    }
    return kind;
}

void Search::appendFirstLegalMove(CandidateList& moves) {
    const bool sideIsBlack = board.isBlackTurn();
    for (int x = 1; x <= BOARD_SIZE; ++x) {
        for (int y = 1; y <= BOARD_SIZE; ++y) {
            const Pos move(x, y);
            if (board.getCell(move).getPiece() != EMPTY) continue;
            if (sideIsBlack && board.isForbidden(move)) continue;
            moves.push_back(move);
            return;
        }
    }
}

// Pattern-based defenses (blocks guessed from the threat's shape) plus own fours.
void Search::getPatternThreatDefend(Evaluator& evaluator, CandidateList& moves) {
    if (evaluator.isOppoMateExist()) {
        evaluator.getThreatDefend(moves);
    } else {
        evaluator.getFourThreeDefend(moves);
    }
    CandidateList fours;
    evaluator.getFours(fours);
    appendUniqueMoves(moves, fours);
}

void Search::appendUniqueMoves(CandidateList& moves, const CandidateList& extraMoves) const {
    if (extraMoves.empty()) {
        return;
    }

    moves.reserve(moves.size() + extraMoves.size());
    for (const Pos& move : extraMoves) {
        if (std::find(moves.begin(), moves.end(), move) == moves.end()) {
            moves.push_back(move);
        }
    }
}

// highest cell score: the top attack and defend score in every direction
constexpr Score maxCellScore() {
    Score attack = 0;
    Score defend = 0;
    for (const Score score : attackScore) attack = std::max(attack, score);
    for (const Score score : defendScore) defend = std::max(defend, score);
    return (attack + defend) * DIRECTION_SIZE;
}

void Search::sortChildNodes(CandidateList& moves, bool isMax, bool defending, const TTEntry* entry) {
    if (moves.size() < 2) {
        return;
    }

    const Pos ttBestMove = (entry != nullptr && entry->bestMove != TranspositionTable::INVALID_MOVE)
        ? TranspositionTable::decodeMove(entry->bestMove)
        : Pos();
    const bool sideToMoveIsBlack = board.isBlackTurn();
    const size_t searchPly = getSearchPly();
    const uint8_t killerCode0 =
        searchPly < state.killerMoves.size() ? state.killerMoves[searchPly][0] : 0;
    const uint8_t killerCode1 =
        searchPly < state.killerMoves.size() ? state.killerMoves[searchPly][1] : 0;
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

    // One ascending key per move, in priority order: TT best, has a child entry, TT score
    // (high first for Max), bound type (Max prefers LOWER_BOUND, Min UPPER_BOUND), killer,
    // cell score, history. The sort is stable, so ties keep the generation order.
    struct KeyedMove {
        uint64_t key;
        uint8_t moveCode;
    };
    constexpr int64_t TT_SCORE_HALF = int64_t{1} << 19;     // 20-bit field
    constexpr int64_t CELL_SCORE_MAX = (int64_t{1} << 19) - 1;  // 19-bit field
    static_assert(MAX_VALUE + 1 < TT_SCORE_HALF, "TT scores must fit their sort-key field");
    static_assert(maxCellScore() <= CELL_SCORE_MAX, "cell scores must fit their sort-key field");
    static_assert(2 * HISTORY_ABS_LIMIT < (1 << 16), "history scores must fit their sort-key field");
    KeyedMove keyed[BOARD_SIZE * BOARD_SIZE];
    size_t keyedCount = 0;

    // Attacker prefers self-attack score; defender wants to block opponent's most
    // threatening spot, so uses opponent's score. Matches the evaluator's previous sort intent.
    const Piece sideToMovePiece = sideToMoveIsBlack ? BLACK : WHITE;
    const Piece opposingPiece   = sideToMoveIsBlack ? WHITE : BLACK;
    const Piece scorePiece = defending ? opposingPiece : sideToMovePiece;
    const bool shouldProbeChildren = (entry != nullptr || hasHistorySignal);
    // prefetch every child before probing so the loads overlap
    uint64_t childKeys[BOARD_SIZE * BOARD_SIZE];
    if (shouldProbeChildren) {
        for (size_t mi = 0; mi < moves.size(); ++mi) {
            childKeys[mi] = getChildTTKey(moves[mi]);
            tt.prefetch(childKeys[mi]);
        }
    }
    for (size_t mi = 0; mi < moves.size(); ++mi) {
        const Pos& move = moves[mi];
        TTEntry childEntryStorage;
        const TTEntry* childEntry =
            shouldProbeChildren
                && tt.probeCopy(childKeys[mi], &childEntryStorage)
                ? &childEntryStorage
                : nullptr;
        const uint8_t moveCode = static_cast<uint8_t>((move.getX() << 4) | move.getY());
        const bool isTTBest = !ttBestMove.isDefault() && move == ttBestMove;
        const int flagPriority = getValueTypePriority(childEntry != nullptr ? childEntry->getFlag() : TTFlag::NONE);
        const int64_t ttScore = std::clamp<int64_t>(
            childEntry != nullptr ? childEntry->score : 0, -(TT_SCORE_HALF - 1), TT_SCORE_HALF - 1);
        const int64_t cellScore = std::clamp<int64_t>(board.getCell(move).getScore(scorePiece), 0, CELL_SCORE_MAX);
        int killerRank = 0;
        if (moveCode == killerCode0 || moveCode == killerCode1) {
            // stored killers made a four where they cut off; re-validate on this
            // board since the same cell may no longer make one here
            if (board.getCell(move).getScore(sideToMovePiece) >= KILLER_MIN_FOUR_SCORE) {
                killerRank = moveCode == killerCode0 ? 2 : 1;
            }
        }
        const int64_t historyScore = getHistoryScore(move, sideToMoveIsBlack);

        // bits: TT best 60 | has TT 59 | TT score 39-58 | bound 37-38 | killer 35-36 |
        // cell score 16-34 | history 0-15
        uint64_t key = 0;
        key |= static_cast<uint64_t>(isTTBest ? 0 : 1) << 60;
        key |= static_cast<uint64_t>(childEntry != nullptr ? 0 : 1) << 59;
        key |= static_cast<uint64_t>(isMax ? (TT_SCORE_HALF - 1 - ttScore) : (ttScore + TT_SCORE_HALF)) << 39;
        key |= static_cast<uint64_t>(flagPriority) << 37;
        key |= static_cast<uint64_t>(2 - killerRank) << 35;
        key |= static_cast<uint64_t>(CELL_SCORE_MAX - cellScore) << 16;
        key |= static_cast<uint64_t>(HISTORY_ABS_LIMIT - historyScore);
        keyed[keyedCount++] = {key, moveCode};
    }

    if (keyedCount <= 32) {
        // insertion sort: stable and fastest for the usual short lists
        for (size_t i = 1; i < keyedCount; ++i) {
            const KeyedMove item = keyed[i];
            size_t j = i;
            while (j > 0 && keyed[j - 1].key > item.key) {
                keyed[j] = keyed[j - 1];
                --j;
            }
            keyed[j] = item;
        }
    } else {
        std::stable_sort(keyed, keyed + keyedCount,
            [](const KeyedMove& a, const KeyedMove& b) { return a.key < b.key; });
    }

    for (size_t i = 0; i < moves.size(); ++i) {
        const uint8_t code = keyed[i].moveCode;
        moves[i] = Pos(code >> 4, code & 0x0F);
    }
}

bool Search::isGameOver(Board& board) {
    Result result = board.getResult();
    return result != ONGOING;
}

Value Search::searchRootWithAspiration(int depth, MoveList* pv) {
    const Value fullAlpha(MIN_VALUE - 1, Value::Type::UNKNOWN);
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
