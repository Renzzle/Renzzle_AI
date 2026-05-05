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

namespace search_detail {
    struct ChildSearchResult {
        Value value;
        MoveList pv;
        bool hasExactPV = false;
        bool wasResearched = false;
    };
}

class Search {

PUBLIC
    struct RootMoveStat {
        size_t order = 0;
        Pos move;
        Value value;
        size_t nodeCount = 0;
        double elapsedTime = 0.0;
        bool wasTTBest = false;
        bool wasResearched = false;
        bool causedCutoff = false;
        MoveList pv;
    };

PRIVATE
    using ChildSearchResult = search_detail::ChildSearchResult;

    struct SearchOptions {
        bool trackMonitorStats = true;
        bool exactOnlyTTStores = false;
        int minTTStoreDepth = 0;
        size_t monitorPollNodeInterval = 1024;
    };

    struct SearchState {
        bool isRunning = false;
        MoveList bestPath;
        Value bestValue;
        std::vector<RootMoveStat> lastRootStats;
        std::array<std::array<int, BOARD_SIZE * BOARD_SIZE>, 2> historyScores = {};
        size_t nodesSinceMonitorPoll = 0;
    };

    Board rootBoard;
    Board board;
    SearchMonitor& monitor;
    TranspositionTable tt;
    SearchOptions options;
    SearchState state;

    static constexpr uint64_t TURN_KEY_BLACK = 0x9e3779b97f4a7c15ULL;
    static constexpr uint64_t TURN_KEY_WHITE = 0xc2b2ae3d27d4eb4fULL;
    static constexpr int HISTORY_ABS_LIMIT = 16384;
    static constexpr int ASPIRATION_START_DELTA = 32;
    Value abp(int depth, bool isMax, Value alpha, Value beta, MoveList* pv = nullptr);
    Value searchRootWithAspiration(int depth, MoveList* pv);
    Value evaluateNode(Evaluator& evaluator);
    bool searchActive() const;
    MoveList getCandidates(Evaluator& evaluator, bool isMax);
    void sortChildNodes(MoveList& moves, bool isMax, const TTEntry* entry);
    bool isGameOver(Board& board);
    uint64_t getTTKey(Board& board) const;
    uint64_t getChildTTKey(const Pos& move);
    uint8_t getTTDepth(int depth) const;
    TTFlag getTTFlag(Value::Type type) const;
    Value getTTValue(const TTEntry& entry) const;
    int32_t encodeTTScore(Value value) const;
    void updateMonitorElapsedTime();
    void pollMonitorIfDue();
    void recordNodeVisit();
    void storeTT(Board& board, Value value, int depth, const Pos& bestMove);
    void appendTTPV(Board tempBoard, MoveList& pv) const;
    int getHistoryIndex(const Pos& move) const;
    int getHistoryScore(const Pos& move, bool isBlackTurn) const;
    void updateHistoryScore(const Pos& move, bool isBlackTurn, int delta);
    void clearHistory();
    bool tryResolveFromTT(int depth, Value& alpha, Value& beta, MoveList* pv,
        TTEntry& ttEntryStorage, const TTEntry*& ttEntry, Value& resolvedValue);
    int getShallowMoveLimit(Evaluator& evaluator, int depth) const;
    Value evaluateLeafNode(bool isMax, int depth);
    Value evaluateThreatBrokenLeaf(bool isMax, int depth);
    bool tryResolveQuickWin(Evaluator& evaluator, bool isMax, int depth, MoveList* pv, Value& resolvedValue);
    ChildSearchResult searchChildPVS(int depth, bool isMax, size_t moveIndex, Value alpha, Value beta,
        Value bestVal, MoveList* pv, bool requireExactBest);
    void updateBestFromChild(bool isMax, const Pos& move, const ChildSearchResult& childResult,
        Value& bestVal, Pos& bestMove, MoveList& bestChildPV, Value& alpha, Value& beta);
    void recordRootMoveStat(size_t order, const Pos& move, const Value& childValue,
        const MoveList& childPV, bool hasExactChildPV, bool wasResearched, bool causedCutoff,
        size_t nodeCountBeforeMove, double elapsedBeforeMove, const Pos& ttBestMove);
    Value finalizeNodeValue(bool isMax, Value originalAlpha, Value originalBeta,
        Value alpha, Value beta, Value bestVal) const;
    void updateHistoryFromNode(int depth, const Pos& bestMove, const MoveList& searchedMoves,
        bool causedCutoff, Value result, bool sideToMoveIsBlack);
    void rebuildPV(const Pos& bestMove, const MoveList& bestChildPV, Value result, MoveList* pv) const;

PUBLIC
    Search(Board& board, SearchMonitor& monitor, size_t ttBytes = 64ull * 1024ull * 1024ull);
    void ids();
    void stop();
    void setMonitorPollNodeInterval(size_t nodeInterval);
    size_t getNodeCount() const;
    size_t getEstimatedMemoryBytes() const;
    const std::vector<RootMoveStat>& getLastRootStats() const;

};

#include "detail/runtime.h"
#include "detail/core.h"
#include "detail/lifecycle.h"
#include "detail/tt.h"
#include "detail/history.h"
