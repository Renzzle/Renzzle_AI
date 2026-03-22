#pragma once

#include "../evaluate/evaluator.h"
#include "../test/test.h"
#include "search_monitor.h"
#include "transposition_table.h"
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

class Search {

PRIVATE
    Board rootBoard;
    Board board;
    SearchMonitor& monitor;
    bool isRunning = false;
    TranspositionTable tt;
    MoveList bestPath;
    Value bestValue;
    std::array<std::array<int, BOARD_SIZE * BOARD_SIZE>, 2> historyScores = {};

    //  Random salts to distinguish the side to move in TT keys
    static constexpr uint64_t TURN_KEY_BLACK = 0x9e3779b97f4a7c15ULL;
    static constexpr uint64_t TURN_KEY_WHITE = 0xc2b2ae3d27d4eb4fULL;
    static constexpr int HISTORY_ABS_LIMIT = 16384;
    static constexpr int ASPIRATION_START_DELTA = 32;

    // Principal variation search with TT-assisted alpha-beta pruning
    Value abp(int depth, bool isMax, Value alpha, Value beta, MoveList* pv = nullptr);
    Value searchRootWithAspiration(int depth, MoveList* pv);
    Value evaluateNode(Evaluator& evaluator);
    MoveList getCandidates(Evaluator& evaluator, bool isMax);
    void sortChildNodes(MoveList& moves, bool isMax, const TTEntry* entry);
    bool isGameOver(Board& board);
    uint64_t getTTKey(Board& board) const;
    uint64_t getChildTTKey(const Pos& move);
    uint8_t getTTDepth(int depth) const;
    TTFlag getTTFlag(Value::Type type) const;
    Value getTTValue(const TTEntry& entry) const;
    int32_t encodeTTScore(Value value) const;
    void storeTT(Board& board, Value value, int depth, const Pos& bestMove);
    void appendTTPV(Board tempBoard, MoveList& pv) const;
    int getHistoryIndex(const Pos& move) const;
    int getHistoryScore(const Pos& move, bool isBlackTurn) const;
    void updateHistoryScore(const Pos& move, bool isBlackTurn, int delta);
    void clearHistory();

PUBLIC
    Search(Board& board, SearchMonitor& monitor);
    void ids();
    void stop();
    size_t getNodeCount() const;
    size_t getEstimatedMemoryBytes() const;

};

Search::Search(Board& initialBoard, SearchMonitor& monitor)
    : rootBoard(initialBoard), board(initialBoard), monitor(monitor), tt(64ull * 1024ull * 1024ull, 4) {
    monitor.setBestLineProvider([this](int i) {
        return i == 0 ? bestPath : MoveList();
    });
    monitor.setBestValueProvider([this]() {
        return bestValue;
    });
}

Value Search::abp(int depth, bool isMax, Value alpha, Value beta, MoveList* pv) {
    monitor.updateElapsedTime();
    if (!isRunning) return Value();
    if (pv != nullptr) {
        pv->clear();
    }

    const Value originalAlpha = alpha;
    const Value originalBeta = beta;
    const uint64_t key = getTTKey(board);
    const TTEntry* ttEntry = tt.probe(key);

    // Reuse cached bounds or an exact score when the stored depth is sufficient
    if (ttEntry != nullptr && ttEntry->depth >= getTTDepth(depth)) {
        Value ttValue = getTTValue(*ttEntry);

        if (ttEntry->getFlag() == TTFlag::EXACT) {
            if (pv != nullptr) {
                appendTTPV(board, *pv);
            }
            return ttValue;
        }
        if (ttEntry->getFlag() == TTFlag::LOWER_BOUND && ttValue >= beta) {
            return ttValue;
        }
        if (ttEntry->getFlag() == TTFlag::UPPER_BOUND && ttValue <= alpha) {
            return ttValue;
        }

        if (ttEntry->getFlag() == TTFlag::LOWER_BOUND && ttValue > alpha) {
            alpha = ttValue;
        } else if (ttEntry->getFlag() == TTFlag::UPPER_BOUND && ttValue < beta) {
            beta = ttValue;
        }
    }

    Evaluator evaluator(board);
    MoveList moves = getCandidates(evaluator, isMax);

    // Stop on leaf, terminal, or forced-empty nodes and evaluate statically
    if (depth == 0 || isGameOver(board) || moves.empty()) {
        Value val = evaluateNode(evaluator);
        if (!isMax) val.invert();
        if (isRunning) {
            storeTT(board, val, depth, Pos());
        }
        return val;
    }

    // Good move ordering makes PVS and cutoffs much cheaper
    sortChildNodes(moves, isMax, ttEntry);

    Value bestVal = isMax
        ? Value(MIN_VALUE, Value::Type::UNKNOWN)
        : Value(MAX_VALUE + 1, Value::Type::UNKNOWN);
    Pos bestMove;
    MoveList bestChildPV;
    MoveList searchedMoves;
    bool searchedAny = false;
    const bool sideToMoveIsBlack = board.isBlackTurn();
    bool causedCutoff = false;

    searchedMoves.reserve(moves.size());

    for (size_t i = 0; i < moves.size(); ++i) {
        const Pos move = moves[i];
        if (!board.move(move)) {
            continue;
        }

        searchedAny = true;
        searchedMoves.push_back(move);
        monitor.incVisitCnt();

        Value nextAlpha = alpha;
        Value nextBeta = beta;
        nextAlpha.decreaseResultDepth();
        nextBeta.decreaseResultDepth();

        Value childValue;
        MoveList childPV;
        bool hasExactChildPV = false;
        if (i == 0) {
            // Search the first move with the full window
            childValue = abp(depth - 1, !isMax, nextAlpha, nextBeta, &childPV);
            hasExactChildPV = childValue.getType() == Value::Type::EXACT;
        } else {
            // Probe later moves with a null window, then re-search only on improvement
            Value nullAlpha = nextAlpha;
            Value nullBeta = nextBeta;
            if (isMax) {
                nullBeta = nullAlpha;
                nullBeta += 1;
            } else {
                nullAlpha = nullBeta;
                nullAlpha -= 1;
            }

            childValue = abp(depth - 1, !isMax, nullAlpha, nullBeta, nullptr);

            if (isRunning) {
                const bool needResearch =
                    (isMax && childValue > alpha && childValue < beta) ||
                    (!isMax && childValue < beta && childValue > alpha);
                if (needResearch) {
                    childValue = abp(depth - 1, !isMax, nextAlpha, nextBeta, &childPV);
                    hasExactChildPV = childValue.getType() == Value::Type::EXACT;
                } else if (pv != nullptr) {
                    const bool couldBecomeBest =
                        (isMax && childValue > bestVal) ||
                        (!isMax && childValue < bestVal);
                    if (couldBecomeBest) {
                        childValue = abp(depth - 1, !isMax, nextAlpha, nextBeta, &childPV);
                        hasExactChildPV = childValue.getType() == Value::Type::EXACT;
                    }
                }
            }
        }

        board.undo();

        if (!isRunning) {
            break;
        }

        childValue.increaseResultDepth();

        if (isMax) {
            if (childValue > bestVal) {
                bestVal = childValue;
                bestMove = move;
                bestChildPV = hasExactChildPV ? childPV : MoveList();
            }
            if (childValue > alpha) {
                alpha = childValue;
            }
        } else {
            if (childValue < bestVal) {
                bestVal = childValue;
                bestMove = move;
                bestChildPV = hasExactChildPV ? childPV : MoveList();
            }
            if (childValue < beta) {
                beta = childValue;
            }
        }

        // Cut once the current node can no longer improve the parent decision
        if (beta <= alpha) {
            causedCutoff = true;
            break;
        }
    }

    if (!searchedAny) {
        Value val = evaluateNode(evaluator);
        if (!isMax) val.invert();
        if (isRunning) {
            storeTT(board, val, depth, Pos());
        }
        return val;
    }

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

    if (isRunning && !bestMove.isDefault()) {
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

    if (isRunning) {
        storeTT(board, result, depth, bestMove);
    }

    if (pv != nullptr && !bestMove.isDefault()) {
        // Rebuild the PV from the best child or fall back to TT continuation
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

    return result;
}

Value Search::evaluateNode(Evaluator& evaluator) {
    // Use the evaluator directly for static node scoring
    return evaluator.evaluate();
}

MoveList Search::getCandidates(Evaluator& evaluator, bool isMax) {
    MoveList moves;
    
    // A forced winning or forced-blocking move takes priority over everything else
    Pos sureMove = evaluator.getSureMove();
    if (!sureMove.isDefault()) {
        moves.push_back(sureMove);
        return moves;
    }

    // Defend immediate threats first; otherwise expand tactical attacking moves
    if (evaluator.isOppoMateExist()) {
        moves = evaluator.getThreatDefend();
        MoveList fours = evaluator.getFours();
        moves.insert(moves.end(), fours.begin(), fours.end());
    } else {
        moves = evaluator.getThreats();
    }

    return moves;
}

void Search::sortChildNodes(MoveList& moves, bool isMax, const TTEntry* entry) {
    // Prefer TT-backed or history-backed moves while preserving evaluator order by default.
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
        const TTEntry* childEntry = (entry != nullptr || hasHistorySignal) ? tt.probe(getChildTTKey(move)) : nullptr;
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
    // Any resolved board result is terminal for search
    Result result = board.getResult();
    return result != ONGOING;
}

Value Search::searchRootWithAspiration(int depth, MoveList* pv) {
    const Value fullAlpha(MIN_VALUE, Value::Type::UNKNOWN);
    const Value fullBeta(MAX_VALUE + 1, Value::Type::UNKNOWN);

    if (bestValue.getType() == Value::Type::UNKNOWN || !bestValue.isOnGoing()) {
        return abp(depth, true, fullAlpha, fullBeta, pv);
    }

    const int center = bestValue.getValue();
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

        if (!isRunning || result.getType() == Value::Type::EXACT) {
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

void Search::ids() {
    isRunning = true;
    bestPath.clear();
    bestValue = Value();
    clearHistory();
    monitor.incDepth(5);
    monitor.initStartTime();
    tt.clear();

    // Deepen gradually and keep the latest principal variation as the best line
    while (true) {
        tt.nextGeneration();

        MoveList iterationPV;
        Value result = searchRootWithAspiration(monitor.getDepth(), &iterationPV);

        if (!isRunning) {
            break;
        }

        bestValue = result;
        bestPath = iterationPV;
        if (bestValue.isWin() && bestPath.size() < static_cast<size_t>(bestValue.getResultDepth())) {
            const Board savedBoard = board;
            Board tempBoard = rootBoard;
            for (const Pos& move : bestPath) {
                if (!tempBoard.move(move)) {
                    break;
                }
            }

            if (tempBoard.getResult() == ONGOING) {
                board = tempBoard;
                MoveList tailPV;
                const int remainDepth = bestValue.getResultDepth() - static_cast<int>(bestPath.size());
                const bool tailIsMax = (bestPath.size() % 2 == 0);
                abp(
                    remainDepth,
                    tailIsMax,
                    Value(MIN_VALUE, Value::Type::UNKNOWN),
                    Value(MAX_VALUE + 1, Value::Type::UNKNOWN),
                    &tailPV
                );
                bestPath.insert(bestPath.end(), tailPV.begin(), tailPV.end());
                board = savedBoard;
            }
        }
        monitor.setBestPath(bestPath);

        if (result.isWin() && result.getResultDepth() <= monitor.getDepth()) {
            break;
        }

        monitor.incDepth(2);
    }
}

void Search::stop() {
    isRunning = false;
}

size_t Search::getNodeCount() const {
    return tt.getUsedEntryCount();
}

size_t Search::getEstimatedMemoryBytes() const {
    return tt.getMemoryBytes()
        + (sizeof(Board) * 2)
        + (bestPath.capacity() * sizeof(Pos))
        + sizeof(historyScores);
}

uint64_t Search::getTTKey(Board& board) const {
    // Mix side-to-move into the board hash to avoid key collisions
    uint64_t key = static_cast<uint64_t>(board.getCurrentHash());
    key ^= board.isBlackTurn() ? TURN_KEY_BLACK : TURN_KEY_WHITE;
    return key;
}

uint64_t Search::getChildTTKey(const Pos& move) {
    // Predict the child key without making the move on the board
    uint64_t key = static_cast<uint64_t>(board.getChildHash(move));
    key ^= board.isBlackTurn() ? TURN_KEY_WHITE : TURN_KEY_BLACK;
    return key;
}

uint8_t Search::getTTDepth(int depth) const {
    // Clamp the depth into the compact TT storage range
    if (depth < 0) return 0;
    if (depth > std::numeric_limits<uint8_t>::max()) {
        return std::numeric_limits<uint8_t>::max();
    }
    return static_cast<uint8_t>(depth);
}

TTFlag Search::getTTFlag(Value::Type type) const {
    // Convert search result semantics into TT flag semantics
    if (type == Value::Type::EXACT) return TTFlag::EXACT;
    if (type == Value::Type::LOWER_BOUND) return TTFlag::LOWER_BOUND;
    if (type == Value::Type::UPPER_BOUND) return TTFlag::UPPER_BOUND;
    return TTFlag::NONE;
}

Value Search::getTTValue(const TTEntry& entry) const {
    // Decode mate-distance-aware TT scores back into a search Value
    Value value;
    if (entry.score >= (MAX_VALUE - (BOARD_SIZE * BOARD_SIZE))) {
        value = Value(Value::Result::WIN, MAX_VALUE - entry.score);
    } else if (entry.score <= (MIN_VALUE + (BOARD_SIZE * BOARD_SIZE))) {
        value = Value(Value::Result::LOSE, entry.score - MIN_VALUE);
    } else {
        value = Value(entry.score);
    }

    if (entry.getFlag() == TTFlag::LOWER_BOUND) {
        value.setType(Value::Type::LOWER_BOUND);
    } else if (entry.getFlag() == TTFlag::UPPER_BOUND) {
        value.setType(Value::Type::UPPER_BOUND);
    } else {
        value.setType(Value::Type::EXACT);
    }

    return value;
}

int32_t Search::encodeTTScore(Value value) const {
    // Encode mate distance so shorter wins and longer losses stay ordered
    if (value.getResult() == Value::Result::WIN) {
        return MAX_VALUE - value.getResultDepth();
    }
    if (value.getResult() == Value::Result::LOSE) {
        return MIN_VALUE + value.getResultDepth();
    }
    return value.getValue();
}

void Search::storeTT(Board& board, Value value, int depth, const Pos& bestMove) {
    // Skip unknown values because TT entries must carry a usable bound
    const TTFlag flag = getTTFlag(value.getType());
    if (flag == TTFlag::NONE) return;

    tt.store(
        getTTKey(board),
        encodeTTScore(value),
        getTTDepth(depth),
        flag,
        TranspositionTable::encodeMove(bestMove)
    );
}

void Search::appendTTPV(Board tempBoard, MoveList& pv) const {
    // Follow stored best moves to extend the PV after the searched prefix
    for (int ply = 0; ply < BOARD_SIZE * BOARD_SIZE; ++ply) {
        const TTEntry* entry = tt.probe(getTTKey(tempBoard));
        if (entry == nullptr || entry->bestMove == TranspositionTable::INVALID_MOVE) {
            break;
        }

        const Pos move = TranspositionTable::decodeMove(entry->bestMove);
        if (move.isDefault() || !tempBoard.move(move)) {
            break;
        }

        pv.push_back(move);
        if (tempBoard.getResult() != ONGOING) {
            break;
        }
    }
}

int Search::getHistoryIndex(const Pos& move) const {
    const uint16_t encoded = TranspositionTable::encodeMove(move);
    return encoded == TranspositionTable::INVALID_MOVE ? -1 : static_cast<int>(encoded);
}

int Search::getHistoryScore(const Pos& move, bool isBlackTurn) const {
    const int index = getHistoryIndex(move);
    if (index < 0) {
        return 0;
    }
    return historyScores[isBlackTurn ? BLACK : WHITE][index];
}

void Search::updateHistoryScore(const Pos& move, bool isBlackTurn, int delta) {
    const int index = getHistoryIndex(move);
    if (index < 0) {
        return;
    }

    int& score = historyScores[isBlackTurn ? BLACK : WHITE][index];
    score = std::max(-HISTORY_ABS_LIMIT, std::min(score + delta, HISTORY_ABS_LIMIT));
}

void Search::clearHistory() {
    for (auto& sideHistory : historyScores) {
        sideHistory.fill(0);
    }
}
