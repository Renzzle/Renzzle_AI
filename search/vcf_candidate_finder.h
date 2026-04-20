#pragma once

#include "../evaluate/evaluator.h"
#include "../game/board.h"
#include "../game/fixed_move_list.h"
#include "transposition_table.h"
#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <limits>
#include <vector>

struct VCFCandidateFinderOptions {
    double perMoveTimeLimitSeconds = 0.0;
    size_t perMoveNodeLimit = 0;
    size_t ttBytes = 8ull * 1024ull * 1024ull;
    size_t ttAssociativity = 4;
};

struct VCFCandidateProbeResult {
    Pos move;
    bool createsVCF = false;
    bool stopped = false;
    size_t nodeCount = 0;
    double elapsedTime = 0.0;
    MoveList vcfPath;
};

class VCFCandidateFinder {

PRIVATE
    Board rootBoard;
    VCFCandidateFinderOptions options;
    TranspositionTable vcfTT;
    std::vector<VCFCandidateProbeResult> lastProbeResults;

    struct VCFProbeContext {
        Color targetColor = COLOR_BLACK;
        Result targetResult = BLACK_WIN;
        size_t rootPathSize = 0;
        size_t nodeCount = 0;
        bool stopped = false;
        std::chrono::high_resolution_clock::time_point startTime;
        MoveList bestPath;
    };

    static constexpr uint64_t TURN_KEY_BLACK = 0x9e3779b97f4a7c15ULL;
    static constexpr uint64_t TURN_KEY_WHITE = 0xc2b2ae3d27d4eb4fULL;

    static uint8_t moveKey(const Pos& move);
    static MoveList makeUniqueMoves(const MoveList& moves);
    static bool isTargetTurn(Board& board, Color targetColor);
    static uint8_t getDepthLeft(Board& board);
    static double getElapsedSeconds(const VCFProbeContext& context);
    static bool isWin(Board& board, VCFProbeContext& context);
    static uint64_t getVCFTTKey(Board& board);
    static void moveTTBestFirst(MoveList& moves, const TTEntry* entry);
    bool shouldStopProbe(VCFProbeContext& context) const;
    bool dfsVCF(Board& board, VCFProbeContext& context);
    bool createsVCFAfterPass(const Pos& move, VCFCandidateProbeResult* probeResult);

PUBLIC
    explicit VCFCandidateFinder(Board& board, VCFCandidateFinderOptions options = VCFCandidateFinderOptions());

    MoveList findFromCandidates(const MoveList& candidates);
    MoveList findFromEvaluatorCandidates();
    MoveList findFromAllLegalMoves();

    const std::vector<VCFCandidateProbeResult>& getLastProbeResults() const;
    size_t getCachedNodeCount() const;
    size_t getEstimatedMemoryBytes() const;
};

VCFCandidateFinder::VCFCandidateFinder(Board& board, VCFCandidateFinderOptions options)
    : rootBoard(board), options(options), vcfTT(options.ttBytes, options.ttAssociativity) {
    vcfTT.nextGeneration();
}

uint8_t VCFCandidateFinder::moveKey(const Pos& move) {
    return static_cast<uint8_t>((move.getX() << 4) | move.getY());
}

MoveList VCFCandidateFinder::makeUniqueMoves(const MoveList& moves) {
    FixedMoveList<BOARD_SIZE * BOARD_SIZE> unique;
    std::array<uint8_t, 256> seen = {};

    unique.reserve(moves.size());
    for (const Pos& move : moves) {
        if (move.isDefault()) {
            continue;
        }

        const uint8_t key = moveKey(move);
        if (seen[key]) {
            continue;
        }

        seen[key] = 1;
        unique.push_back(move);
    }

    return unique.toMoveList();
}

bool VCFCandidateFinder::isTargetTurn(Board& board, Color targetColor) {
    return board.isBlackTurn()
        ? targetColor == COLOR_BLACK
        : targetColor == COLOR_WHITE;
}

uint8_t VCFCandidateFinder::getDepthLeft(Board& board) {
    const size_t pathSize = board.getPath().size();
    size_t remain = 0;
    if (BOARD_SIZE * BOARD_SIZE > pathSize) {
        remain = (BOARD_SIZE * BOARD_SIZE) - pathSize;
    }
    if (remain > std::numeric_limits<uint8_t>::max()) {
        remain = std::numeric_limits<uint8_t>::max();
    }
    return static_cast<uint8_t>(remain);
}

double VCFCandidateFinder::getElapsedSeconds(const VCFProbeContext& context) {
    const auto now = std::chrono::high_resolution_clock::now();
    const auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(now - context.startTime);
    return duration.count() / 1e9;
}

bool VCFCandidateFinder::isWin(Board& board, VCFProbeContext& context) {
    const Result result = board.getResult();
    if (result != context.targetResult) {
        return false;
    }

    const MoveList& fullPath = board.getPath();
    context.bestPath.clear();
    if (fullPath.size() >= context.rootPathSize) {
        context.bestPath.assign(fullPath.begin() + context.rootPathSize, fullPath.end());
    }

    return true;
}

uint64_t VCFCandidateFinder::getVCFTTKey(Board& board) {
    uint64_t key = static_cast<uint64_t>(board.getCurrentHash());
    key ^= board.isBlackTurn() ? TURN_KEY_BLACK : TURN_KEY_WHITE;
    return key;
}

void VCFCandidateFinder::moveTTBestFirst(MoveList& moves, const TTEntry* entry) {
    if (entry == nullptr) return;
    if (entry->bestMove == TranspositionTable::INVALID_MOVE) return;

    const Pos bestMove = TranspositionTable::decodeMove(entry->bestMove);
    const auto it = std::find(moves.begin(), moves.end(), bestMove);
    if (it != moves.end() && it != moves.begin()) {
        std::iter_swap(moves.begin(), it);
    }
}

bool VCFCandidateFinder::shouldStopProbe(VCFProbeContext& context) const {
    if (context.stopped) {
        return true;
    }

    if (options.perMoveNodeLimit > 0 && context.nodeCount >= options.perMoveNodeLimit) {
        context.stopped = true;
        return true;
    }

    if (options.perMoveTimeLimitSeconds > 0.0 &&
        getElapsedSeconds(context) >= options.perMoveTimeLimitSeconds) {
        context.stopped = true;
        return true;
    }

    return false;
}

bool VCFCandidateFinder::dfsVCF(Board& board, VCFProbeContext& context) {
    if (isWin(board, context)) {
        return true;
    }
    if (shouldStopProbe(context)) {
        return false;
    }
    if (board.getResult() != ONGOING) {
        return false;
    }

    const uint64_t key = getVCFTTKey(board);
    TTEntry entryStorage;
    const TTEntry* ttEntry = vcfTT.probeCopy(key, &entryStorage) ? &entryStorage : nullptr;

    if (ttEntry != nullptr && ttEntry->getFlag() == TTFlag::EXACT && ttEntry->score == 0) {
        return false;
    }

    context.nodeCount++;
    if (shouldStopProbe(context)) {
        return false;
    }

    Evaluator evaluator(board);
    MoveList moves = isTargetTurn(board, context.targetColor)
        ? evaluator.getFours()
        : evaluator.getCandidates();
    moveTTBestFirst(moves, ttEntry);

    for (const Pos& nextMove : moves) {
        if (!board.move(nextMove)) {
            continue;
        }

        if (dfsVCF(board, context)) {
            board.undo();
            vcfTT.store(key, 1, getDepthLeft(board), TTFlag::EXACT, TranspositionTable::encodeMove(nextMove));
            return true;
        }

        board.undo();
        if (context.stopped) {
            return false;
        }
    }

    vcfTT.store(key, 0, getDepthLeft(board), TTFlag::EXACT, TranspositionTable::INVALID_MOVE);
    return false;
}

bool VCFCandidateFinder::createsVCFAfterPass(const Pos& move, VCFCandidateProbeResult* probeResult) {
    VCFCandidateProbeResult localResult;
    localResult.move = move;

    Board probeBoard(rootBoard);
    if (!probeBoard.move(move)) {
        if (probeResult != nullptr) {
            *probeResult = localResult;
        }
        return false;
    }

    if (!probeBoard.pass()) {
        if (probeResult != nullptr) {
            *probeResult = localResult;
        }
        return false;
    }

    VCFProbeContext context;
    context.targetColor = rootBoard.isBlackTurn() ? COLOR_BLACK : COLOR_WHITE;
    context.targetResult = (context.targetColor == COLOR_BLACK) ? BLACK_WIN : WHITE_WIN;
    context.rootPathSize = probeBoard.getPath().size();
    context.startTime = std::chrono::high_resolution_clock::now();

    const bool createsVCF = dfsVCF(probeBoard, context);

    localResult.createsVCF = createsVCF;
    localResult.stopped = context.stopped;
    localResult.nodeCount = context.nodeCount;
    localResult.elapsedTime = getElapsedSeconds(context);
    localResult.vcfPath = context.bestPath;

    if (probeResult != nullptr) {
        *probeResult = localResult;
    }

    return createsVCF;
}

MoveList VCFCandidateFinder::findFromCandidates(const MoveList& candidates) {
    MoveList result;
    MoveList uniqueCandidates = makeUniqueMoves(candidates);
    lastProbeResults.clear();
    lastProbeResults.reserve(uniqueCandidates.size());

    for (const Pos& move : uniqueCandidates) {
        VCFCandidateProbeResult probeResult;
        if (createsVCFAfterPass(move, &probeResult)) {
            result.push_back(move);
        }
        lastProbeResults.push_back(probeResult);
    }

    return result;
}

MoveList VCFCandidateFinder::findFromEvaluatorCandidates() {
    Board board(rootBoard);
    Evaluator evaluator(board);
    return findFromCandidates(evaluator.getCandidates());
}

MoveList VCFCandidateFinder::findFromAllLegalMoves() {
    MoveList candidates;
    candidates.reserve(BOARD_SIZE * BOARD_SIZE);

    for (int r = 1; r <= BOARD_SIZE; ++r) {
        for (int c = 1; c <= BOARD_SIZE; ++c) {
            const Pos move(r, c);
            if (rootBoard.getCell(move).getPiece() == EMPTY) {
                candidates.push_back(move);
            }
        }
    }

    return findFromCandidates(candidates);
}

const std::vector<VCFCandidateProbeResult>& VCFCandidateFinder::getLastProbeResults() const {
    return lastProbeResults;
}

size_t VCFCandidateFinder::getCachedNodeCount() const {
    return vcfTT.getUsedEntryCount();
}

size_t VCFCandidateFinder::getEstimatedMemoryBytes() const {
    return vcfTT.getMemoryBytes();
}
