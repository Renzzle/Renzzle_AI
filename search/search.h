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

    enum class Mode {
        VCT,        // attacker-focused; finds forced wins (validatePuzzle, search_test)
        DEFENSIVE,  // 범용 수비: defends opponent threats first;
                    // when none, broadens to Evaluator::getCandidates() at root only (findNextMove)
    };

PRIVATE
    using ChildSearchResult = search_detail::ChildSearchResult;

    struct SearchOptions {
        bool trackMonitorStats = true;
        bool exactOnlyTTStores = false;
        int minTTStoreDepth = 0;
        size_t monitorPollNodeInterval = 1024;
        bool leafVCFEnabled = true;
        int leafVCFMaxPly = 17;
        size_t leafVCFNodeLimit = 4096;
        Mode mode = Mode::VCT;
    };

    struct SearchState {
        bool isRunning = false;
        bool qvcfDisabledAfterWin = false;
        MoveList bestPath;
        Value bestValue;
        std::vector<RootMoveStat> lastRootStats;
        std::array<std::array<int, BOARD_SIZE * BOARD_SIZE>, 2> historyScores = {};
        size_t nodesSinceMonitorPoll = 0;
    };

    struct QVCFContext {
        size_t nodeCount = 0;
        size_t nodeLimit = 0;
        bool stopped = false;
    };

    Board rootBoard;
    Board board;
    SearchMonitor& monitor;
    TranspositionTable tt;
    SearchOptions options;
    SearchState state;

    static constexpr uint64_t TURN_KEY_BLACK = 0x9e3779b97f4a7c15ULL;
    static constexpr uint64_t TURN_KEY_WHITE = 0xc2b2ae3d27d4eb4fULL;
    static constexpr uint64_t QVCF_TT_KEY = 0x6a09e667f3bcc909ULL;
    static constexpr int QVCF_HEURISTIC_WIN_SCORE = MAX_VALUE - (BOARD_SIZE * BOARD_SIZE) - 1024;
    static constexpr int HISTORY_ABS_LIMIT = 16384;
    static constexpr int ASPIRATION_START_DELTA = 32;
    Value abp(int depth, bool isMax, Value alpha, Value beta, MoveList* pv = nullptr);
    Value searchRootWithAspiration(int depth, MoveList* pv);
    Value evaluateNode(Evaluator& evaluator);
    bool searchActive() const;
    MoveList getCandidates(Evaluator& evaluator, bool isMax);
    void appendUniqueMoves(MoveList& moves, const MoveList& extraMoves) const;
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
    int getShallowMoveLimit(Evaluator& evaluator, int depth, bool attackerTurn);
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
    void completeWinPV(Value value, MoveList& pv);
    bool isRootAttackerTurn(Board& targetBoard);
    Result getRootWinResult();
    uint64_t getQVCFTTKey(Board& targetBoard) const;
    bool enterQVCFNode(QVCFContext& context);
    void moveQVCFTTBestFirst(MoveList& moves, const Pos& bestMove) const;
    bool probeQVCFTT(Board& targetBoard, int remainingPly, Value& value, Pos& bestMove) const;
    void storeQVCFWin(Board& targetBoard, Value value, int remainingPly, const Pos& bestMove);
    void storeQVCFFail(Board& targetBoard, int remainingPly);
    void appendQVCFTTPV(Board tempBoard, MoveList& pv, int remainingPly) const;
    Value qvcfSearch(int remainingPly, QVCFContext& context, MoveList* pv);
    bool tryResolveLeafVCF(MoveList* pv, Value& resolvedValue);
    Value qvcfAttack(int remainingPly, QVCFContext& context, MoveList* pv);
    Value qvcfDefend(int remainingPly, QVCFContext& context, MoveList* pv);

PUBLIC
    Search(Board& board, SearchMonitor& monitor, size_t ttBytes = 64ull * 1024ull * 1024ull);
    void ids();
    void stop();
    void setQVCFEnabled(bool enabled);
    void setMode(Mode mode);
    void setMonitorPollNodeInterval(size_t nodeInterval);
    size_t getNodeCount() const;
    size_t getEstimatedMemoryBytes() const;
    const std::vector<RootMoveStat>& getLastRootStats() const;

};

#include "detail/runtime.h"
#include "detail/qvcf.h"
#include "detail/core.h"
#include "detail/lifecycle.h"
#include "detail/tt.h"
#include "detail/history.h"
