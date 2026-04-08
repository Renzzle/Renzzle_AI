#pragma once

#include "../evaluate/evaluator.h"
#include "../test/test.h"
#include "search_monitor.h"
#include "transposition_table.h"
#include <algorithm>
#include <atomic>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <future>
#include <limits>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

namespace search_detail {
    struct RootParallelResult {
        size_t order = 0;
        Pos move;
        Value value;
        size_t nodeCount = 0;
        double elapsedTime = 0.0;
        bool wasTTBest = false;
        bool wasResearched = false;
        bool causedCutoff = false;
        bool searched = false;
        MoveList pv;
    };

    struct LazySMPSnapshot {
        std::mutex mutex;
        int completedDepth = 0;
        Value bestValue;
        std::vector<Pos> rootMoves;
    };

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
    using RootParallelResult = search_detail::RootParallelResult;
    using LazySMPSnapshot = search_detail::LazySMPSnapshot;
    using ChildSearchResult = search_detail::ChildSearchResult;

    struct SearchOptions {
        size_t rootThreadCount = 0;
        size_t lazyThreadCount = 0;
        bool lightweightTTOrdering = false;
        bool trackMonitorStats = true;
        bool nonBlockingTTAccess = false;
        bool exactOnlyTTStores = false;
        int minTTStoreDepth = 0;
    };

    struct SearchState {
        bool isRunning = false;
        MoveList bestPath;
        Value bestValue;
        std::vector<RootMoveStat> lastRootStats;
        std::array<std::array<int, BOARD_SIZE * BOARD_SIZE>, 2> historyScores = {};
        std::shared_ptr<std::atomic_bool> sharedRunning;
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
    static constexpr int ROOT_PARALLEL_MIN_DEPTH = 11;
    static constexpr size_t ROOT_PARALLEL_MIN_MOVES = 4;
    static constexpr size_t ROOT_PARALLEL_WORKER_TT_BYTES = 8ull * 1024ull * 1024ull;
    static constexpr int LAZY_SMP_MIN_COMPLETED_DEPTH = 15;
    static constexpr int LAZY_SMP_CHILD_DEPTH_BONUS = 1;
    static constexpr int LAZY_SMP_ASPIRATION_DELTA = 64;

    Value abp(int depth, bool isMax, Value alpha, Value beta, MoveList* pv = nullptr);
    Value searchRootWithAspiration(int depth, MoveList* pv);
    Value evaluateNode(Evaluator& evaluator);
    bool searchActive() const;
    void syncSharedRunningFlag();
    size_t getEffectiveRootThreadCount() const;
    size_t getEffectiveLazyThreadCount() const;
    bool shouldUseRootParallel(bool isRootNode, bool isMax, int depth, size_t moveCount) const;
    void lazySMPHelperLoop(size_t helperIndex, std::shared_ptr<LazySMPSnapshot> snapshot,
        std::shared_ptr<std::atomic_bool> helperRunning);
    RootParallelResult searchRootMoveFullWindow(int depth, const Pos& move, Value alpha, Value beta,
        size_t order, bool wasTTBest);
    Value searchRootParallel(int depth, Value alpha, Value beta, const MoveList& moves,
        const TTEntry* entry, MoveList* pv);
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
    Search(Board& board, SearchMonitor& monitor, size_t rootThreadCount = 0,
        size_t ttBytes = 64ull * 1024ull * 1024ull,
        std::shared_ptr<std::atomic_bool> sharedRunning = nullptr,
        const TranspositionTable* sharedTT = nullptr);
    void ids();
    void stop();
    void setRootThreadCount(size_t count);
    void setLazyThreadCount(size_t count);
    void setLightweightTTOrdering(bool enabled);
    size_t getNodeCount() const;
    size_t getEstimatedMemoryBytes() const;
    const std::vector<RootMoveStat>& getLastRootStats() const;

};

#include "detail/runtime.h"
#include "detail/lazy_smp.h"
#include "detail/root_parallel.h"
#include "detail/core.h"
#include "detail/lifecycle.h"
#include "detail/tt.h"
#include "detail/history.h"
