#pragma once

#include "../evaluate/evaluator.h"
#include "../test/test.h"
#include "search_monitor.h"
#include "transposition_table.h"
#include "vcf_replay.h"
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
        // quiet moves that leave a VCF (VCT: widened stage; DEFENSIVE: root threat check)
        bool vcfThreats = true;
        Mode mode = Mode::VCT;
    };

    struct SearchState {
        bool isRunning = false;
        bool qvcfDisabledAfterWin = false;
        MoveList bestPath;
        Value bestValue;
        std::vector<RootMoveStat> lastRootStats;
        // true only when the last root abp call searched every root candidate
        // (no cutoff, no truncation, not stopped) — lastRootStats covers all moves
        bool lastRootSearchComplete = false;
        std::array<std::array<int, BOARD_SIZE * BOARD_SIZE>, 2> historyScores = {};
        // two killer slots per ply from root; entries are packed (x<<4)|y codes, 0 = empty
        std::array<std::array<uint8_t, 2>, BOARD_SIZE * BOARD_SIZE + 4> killerMoves = {};
        size_t nodesSinceMonitorPoll = 0;
        // the running stage plays and defends VCF-threat moves
        bool vcfThreats = false;
        // XORed into every TT key, so each VCT stage keeps its own entries in one table
        uint64_t ttSalt = 0;
    };

    // One iterative-deepening track. VCT runs a classic track (4 / 3 / 4-3 threats) and a
    // widened one (plus VCF-threat moves); ids() alternates them so neither restarts.
    struct SearchStage {
        bool enabled = false;
        bool done = false;
        int depth = 5;
        bool vcfThreats = false;
        uint64_t ttSalt = 0;
        size_t cost = 0;
        bool qvcfDisabledAfterWin = false;
        Value bestValue;
        MoveList bestPath;
        std::array<std::array<int, BOARD_SIZE * BOARD_SIZE>, 2> historyScores = {};
        std::array<std::array<uint8_t, 2>, BOARD_SIZE * BOARD_SIZE + 4> killerMoves = {};
    };

    enum ThreatKind : uint8_t { NO_THREAT, PATTERN_THREAT, VCF_THREAT };

    // getThreatDefense result per position, so revisits skip the line and the scan
    struct ThreatDefenseEntry {
        uint64_t key = 0;
        bool valid = false;
        uint8_t kind = NO_THREAT;
        uint8_t count = 0;
        std::array<uint8_t, 32> moves = {};  // larger sets are not cached
    };

    struct QVCFContext {
        size_t nodeCount = 0;
        size_t nodeLimit = 0;
        bool stopped = false;
        Piece attacker = BLACK;
        uint64_t keySalt = 0;
        // threat probes need a replayable line, so they skip the 4-3 / 3-3 shortcuts
        bool foursOnly = false;
        TranspositionTable* table = nullptr;
    };

    Board rootBoard;
    Board board;
    SearchMonitor& monitor;
    TranspositionTable tt;
    TranspositionTable threatTT;
    SearchOptions options;
    SearchState state;
    std::vector<ThreatDefenseEntry> threatDefenseCache;

    static constexpr uint64_t TURN_KEY_BLACK = 0x9e3779b97f4a7c15ULL;
    static constexpr uint64_t TURN_KEY_WHITE = 0xc2b2ae3d27d4eb4fULL;
    static constexpr uint64_t QVCF_TT_KEY = 0x6a09e667f3bcc909ULL;
    static constexpr uint64_t QVCF_THREAT_TT_KEY_BLACK = 0xbb67ae8584caa73bULL;
    static constexpr uint64_t QVCF_THREAT_TT_KEY_WHITE = 0x3c6ef372fe94f82bULL;
    static constexpr int QVCF_HEURISTIC_WIN_SCORE = MAX_VALUE - (BOARD_SIZE * BOARD_SIZE) - 1024;
    static constexpr int HISTORY_ABS_LIMIT = 16384;
    // attackScore[B4] — a killer must make at least a four (for the mover) to be
    // stored and to outrank the static cell score during move ordering
    static constexpr int KILLER_MIN_FOUR_SCORE = 400;
    static constexpr int ASPIRATION_START_DELTA = 32;
    static constexpr size_t THREAT_DEFENSE_CACHE_SIZE = 1u << 16;
    // VCF-threat probes are many small searches (tens of nodes each): a table this size
    // stays in cache, and their entries no longer push main-tree entries out of tt
    static constexpr size_t THREAT_TT_BYTES = 4ull * 1024ull * 1024ull;
    static constexpr uint64_t WIDENED_STAGE_TT_SALT = 0xa54ff53a5f1d36f1ULL;
    // classic VCT runs alone for this many nodes before the widened stage gets iterations;
    // every search_test case keeps its classic node count
    static constexpr size_t VCF_THREAT_WIDEN_COST = 1000000;
    // after that, the stage with less cost so far runs next; widened cost counts double so
    // classic keeps about two thirds of the time
    static constexpr size_t WIDENED_STAGE_COST_WEIGHT = 2;
    // a VCF-threat move needs a defense ply and then an attacker ply to cash the VCF in
    static constexpr int VCF_THREAT_MIN_DEPTH = 2;
    Value abp(int depth, bool isMax, Value alpha, Value beta, MoveList* pv = nullptr);
    Value searchRootWithAspiration(int depth, MoveList* pv);
    int pickSearchStage(SearchStage* stages);
    void enterSearchStage(SearchStage* stages, int from, int to);
    void publishSearchStages(SearchStage* stages, int current);
    void setMonitorDepth(int depth);
    Value evaluateNode(Evaluator& evaluator);
    bool searchActive() const;
    CandidateList getCandidates(Evaluator& evaluator, bool isMax, int depth, bool& vcfThreatNode);
    ThreatKind getThreatDefense(CandidateList& moves, bool patternThreat, bool vcfThreats);
    void appendFirstLegalMove(CandidateList& moves);
    void getPatternThreatDefend(Evaluator& evaluator, CandidateList& moves);
    void appendUniqueMoves(CandidateList& moves, const CandidateList& extraMoves) const;
    void sortChildNodes(CandidateList& moves, bool isMax, bool defending, const TTEntry* entry);
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
    size_t getSearchPly();
    static uint8_t packMoveCode(const Pos& move);
    void updateKillerMove(size_t ply, uint8_t moveCode);
    bool tryResolveFromTT(int depth, Value& alpha, Value& beta, MoveList* pv,
        TTEntry& ttEntryStorage, const TTEntry*& ttEntry, Value& resolvedValue);
    int getShallowMoveLimit(Evaluator& evaluator, int depth, bool attackerTurn);
    Value evaluateLeafNode(bool isMax, int depth);
    Value evaluateThreatBrokenLeaf(bool isMax, int depth);
    bool tryResolveQuickWin(Evaluator& evaluator, bool isMax, int depth, MoveList* pv, Value& resolvedValue);
    bool tryResolveLosingRootMove(int depth, bool isMax, Value& resolvedValue);
    ChildSearchResult searchChildPVS(int depth, bool isMax, size_t moveIndex, Value alpha, Value beta,
        Value bestVal, MoveList* pv, bool requireExactBest);
    void updateBestFromChild(bool isMax, const Pos& move, const ChildSearchResult& childResult,
        Value& bestVal, Pos& bestMove, MoveList& bestChildPV, Value& alpha, Value& beta);
    void recordRootMoveStat(size_t order, const Pos& move, const Value& childValue,
        const MoveList& childPV, bool hasExactChildPV, bool wasResearched, bool causedCutoff,
        size_t nodeCountBeforeMove, double elapsedBeforeMove, const Pos& ttBestMove);
    Value finalizeNodeValue(bool isMax, Value originalAlpha, Value originalBeta,
        Value alpha, Value beta, Value bestVal) const;
    void updateHistoryFromNode(int depth, const Pos& bestMove,
        const uint8_t* searchedMoveCodes, size_t searchedMoveCount,
        bool causedCutoff, Value result, bool sideToMoveIsBlack);
    void rebuildPV(const Pos& bestMove, const MoveList& bestChildPV, Value result, MoveList* pv) const;
    void completeWinPV(Value value, MoveList& pv);
    QVCFContext makeLeafQVCFContext();
    QVCFContext makeThreatQVCFContext(Piece attacker);
    bool isQVCFAttackerTurn(Board& targetBoard, const QVCFContext& context) const;
    Result getQVCFWinResult(const QVCFContext& context) const;
    uint64_t getQVCFTTKey(Board& targetBoard, const QVCFContext& context) const;
    bool enterQVCFNode(QVCFContext& context);
    void moveQVCFTTBestFirst(CandidateList& moves, const Pos& bestMove) const;
    bool probeQVCFTT(Board& targetBoard, const QVCFContext& context, int remainingPly,
        Value& value, Pos& bestMove) const;
    void storeQVCFWin(Board& targetBoard, const QVCFContext& context, Value value,
        int remainingPly, const Pos& bestMove);
    void storeQVCFFail(Board& targetBoard, const QVCFContext& context, int remainingPly);
    Value qvcfSearch(int remainingPly, QVCFContext& context, MoveList* pv);
    bool tryResolveLeafVCF(MoveList* pv, Value& resolvedValue);
    bool findVCFThreatLine(MoveList& line);
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
