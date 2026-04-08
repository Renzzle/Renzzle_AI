#pragma once

#include "../game/pos.h"
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <vector>

enum class TTFlag : uint8_t {
    NONE = 0,
    EXACT = 1,
    LOWER_BOUND = 2,
    UPPER_BOUND = 3
};

struct TTEntry {
    static constexpr uint8_t FLAG_MASK = 0x03;
    static constexpr uint8_t AGE_MASK = 0x3F;
    static constexpr uint8_t AGE_SHIFT = 2;

    uint64_t key = 0;
    int32_t score = 0;
    uint16_t bestMove = 0xFFFF;
    uint8_t depth = 0;
    uint8_t meta = 0; // [0..1]: TTFlag, [2..7]: generation(age)

    TTFlag getFlag() const {
        return static_cast<TTFlag>(meta & FLAG_MASK);
    }

    uint8_t getAge() const {
        return (meta >> AGE_SHIFT) & AGE_MASK;
    }

    void setMeta(TTFlag flag, uint8_t age) {
        meta = (static_cast<uint8_t>(flag) & FLAG_MASK)
             | ((age & AGE_MASK) << AGE_SHIFT);
    }

    bool isEmpty() const {
        return getFlag() == TTFlag::NONE;
    }
};

// TTEntry size must be 16 bytes
static_assert(sizeof(TTEntry) == 16, "TTEntry must stay compact.");

class TranspositionTable {
private:
    static constexpr size_t DEFAULT_BYTES = 64ull * 1024ull * 1024ull; // 64MB

    struct SharedState {
        std::vector<TTEntry> entries;
        size_t bucketCount = 0;
        size_t associativity = 4;
        size_t memoryBytes = 0;
        std::atomic<size_t> usedEntries {0};
        std::atomic<unsigned int> generation {0};
        mutable std::atomic<size_t> probeCount {0};
        mutable std::atomic<size_t> hitCount {0};
        bool sharedMode = false;
        mutable std::vector<std::shared_mutex> stripeLocks;
    };

    std::shared_ptr<SharedState> state = std::make_shared<SharedState>();

    static size_t floorPowerOfTwo(size_t v) {
        size_t p = 1;
        while (p <= v / 2) {
            p <<= 1;
        }
        return p;
    }

    size_t getBucketBase(size_t bucketIndex) const {
        return bucketIndex * state->associativity;
    }

    size_t getBucketIndex(uint64_t key) const {
        return static_cast<size_t>(key) & (state->bucketCount - 1);
    }

    int getReplacementScore(const TTEntry& entry) const {
        const unsigned int generation = state->generation.load(std::memory_order_relaxed) & TTEntry::AGE_MASK;
        const int ageDist = static_cast<int>((generation - entry.getAge()) & TTEntry::AGE_MASK);
        // Keep deeper and newer entries. Low score means easy victim.
        return static_cast<int>(entry.depth) - (ageDist * 2);
    }

    bool isSharedModeEnabled() const {
        return state->sharedMode && !state->stripeLocks.empty();
    }

    std::unique_lock<std::shared_mutex> lockBucket(size_t bucketIndex, bool nonBlocking = false) const {
        if (!isSharedModeEnabled()) {
            return std::unique_lock<std::shared_mutex>();
        }

        const size_t stripeIndex = bucketIndex % state->stripeLocks.size();
        if (nonBlocking) {
            return std::unique_lock<std::shared_mutex>(
                state->stripeLocks[stripeIndex], std::try_to_lock);
        }
        return std::unique_lock<std::shared_mutex>(state->stripeLocks[stripeIndex]);
    }

    std::shared_lock<std::shared_mutex> lockBucketShared(size_t bucketIndex, bool nonBlocking = false) const {
        if (!isSharedModeEnabled()) {
            return std::shared_lock<std::shared_mutex>();
        }

        const size_t stripeIndex = bucketIndex % state->stripeLocks.size();
        if (nonBlocking) {
            return std::shared_lock<std::shared_mutex>(
                state->stripeLocks[stripeIndex], std::try_to_lock);
        }
        return std::shared_lock<std::shared_mutex>(state->stripeLocks[stripeIndex]);
    }

public:
    static constexpr uint16_t INVALID_MOVE = 0xFFFF;

    explicit TranspositionTable(size_t bytes = DEFAULT_BYTES, size_t assoc = 4) {
        resize(bytes, assoc);
    }

    void resize(size_t bytes, size_t assoc = 4) {
        if (assoc == 0) assoc = 1;

        const size_t entryBytes = sizeof(TTEntry);
        size_t rawBucketCount = bytes / (entryBytes * assoc);
        if (rawBucketCount == 0) rawBucketCount = 1;

        state->associativity = assoc;
        state->bucketCount = floorPowerOfTwo(rawBucketCount);
        state->entries.assign(state->bucketCount * state->associativity, TTEntry());
        state->memoryBytes = state->entries.size() * sizeof(TTEntry);
        state->usedEntries.store(0, std::memory_order_relaxed);
        state->generation.store(0, std::memory_order_relaxed);
        resetStats();
    }

    void clear() {
        for (auto& entry : state->entries) {
            entry = TTEntry();
        }
        state->usedEntries.store(0, std::memory_order_relaxed);
        state->generation.store(0, std::memory_order_relaxed);
        resetStats();
    }

    void nextGeneration() {
        const unsigned int next =
            (state->generation.load(std::memory_order_relaxed) + 1u) & TTEntry::AGE_MASK;
        state->generation.store(next, std::memory_order_relaxed);
    }

    void enableSharedMode(size_t stripeCount = 256) {
        if (stripeCount == 0) {
            stripeCount = 1;
        }

        state->sharedMode = true;
        state->stripeLocks = std::vector<std::shared_mutex>(stripeCount);
    }

    void disableSharedMode() {
        state->sharedMode = false;
        state->stripeLocks.clear();
    }

    bool probeCopy(uint64_t key, TTEntry* out, bool nonBlocking = false) const {
        if (state->bucketCount == 0 || out == nullptr) {
            return false;
        }

        state->probeCount.fetch_add(1, std::memory_order_relaxed);
        const size_t bucketIndex = getBucketIndex(key);
        const auto lock = lockBucketShared(bucketIndex, nonBlocking);
        if (isSharedModeEnabled() && nonBlocking && !lock.owns_lock()) {
            return false;
        }
        const size_t base = getBucketBase(bucketIndex);

        for (size_t i = 0; i < state->associativity; i++) {
            const TTEntry& entry = state->entries[base + i];
            if (!entry.isEmpty() && entry.key == key) {
                *out = entry;
                state->hitCount.fetch_add(1, std::memory_order_relaxed);
                return true;
            }
        }

        return false;
    }

    const TTEntry* probe(uint64_t key, bool nonBlocking = false) const {
        static thread_local TTEntry entryBuffer;
        return probeCopy(key, &entryBuffer, nonBlocking) ? &entryBuffer : nullptr;
    }

    bool isSharedMode() const {
        return state->sharedMode;
    }

    void store(uint64_t key, int32_t score, uint8_t depth, TTFlag flag,
        uint16_t bestMove = INVALID_MOVE, bool nonBlocking = false) {
        if (state->bucketCount == 0) return;

        const size_t bucketIndex = getBucketIndex(key);
        const auto lock = lockBucket(bucketIndex, nonBlocking);
        if (isSharedModeEnabled() && nonBlocking && !lock.owns_lock()) {
            return;
        }
        const size_t base = getBucketBase(bucketIndex);

        size_t slotIdx = std::numeric_limits<size_t>::max();
        size_t emptyIdx = std::numeric_limits<size_t>::max();
        size_t victimIdx = base;
        int victimScore = std::numeric_limits<int>::max();

        for (size_t i = 0; i < state->associativity; i++) {
            const size_t idx = base + i;
            const TTEntry& entry = state->entries[idx];

            if (!entry.isEmpty() && entry.key == key) {
                slotIdx = idx;
                break;
            }

            if (entry.isEmpty() && emptyIdx == std::numeric_limits<size_t>::max()) {
                emptyIdx = idx;
            }

            const int replacementScore = getReplacementScore(entry);
            if (replacementScore < victimScore) {
                victimScore = replacementScore;
                victimIdx = idx;
            }
        }

        if (slotIdx == std::numeric_limits<size_t>::max()) {
            slotIdx = (emptyIdx != std::numeric_limits<size_t>::max()) ? emptyIdx : victimIdx;
        }

        TTEntry& target = state->entries[slotIdx];
        const bool wasEmpty = target.isEmpty();
        const uint8_t generation =
            static_cast<uint8_t>(state->generation.load(std::memory_order_relaxed) & TTEntry::AGE_MASK);

        // Existing key: keep deeper info unless new one is EXACT.
        if (!target.isEmpty() && target.key == key) {
            const bool shouldOverwrite = (depth >= target.depth) || (flag == TTFlag::EXACT);
            if (!shouldOverwrite) {
                target.setMeta(target.getFlag(), generation);
                if (target.bestMove == INVALID_MOVE && bestMove != INVALID_MOVE) {
                    target.bestMove = bestMove;
                }
                return;
            }
        }

        target.key = key;
        target.score = score;
        target.bestMove = bestMove;
        target.depth = depth;
        target.setMeta(flag, generation);
        if (wasEmpty) {
            state->usedEntries.fetch_add(1, std::memory_order_relaxed);
        }
    }

    size_t getUsedEntryCount() const {
        return state->usedEntries.load(std::memory_order_relaxed);
    }

    double getLoadFactor() const {
        if (state->entries.empty()) return 0.0;
        return static_cast<double>(state->usedEntries.load(std::memory_order_relaxed))
            / static_cast<double>(state->entries.size());
    }

    size_t getEntryCount() const {
        return state->entries.size();
    }

    size_t getBucketCount() const {
        return state->bucketCount;
    }

    size_t getAssociativity() const {
        return state->associativity;
    }

    size_t getMemoryBytes() const {
        return state->memoryBytes;
    }

    uint8_t getGeneration() const {
        return static_cast<uint8_t>(state->generation.load(std::memory_order_relaxed) & TTEntry::AGE_MASK);
    }

    void resetStats() const {
        state->probeCount.store(0, std::memory_order_relaxed);
        state->hitCount.store(0, std::memory_order_relaxed);
    }

    size_t getProbeCount() const {
        return state->probeCount.load(std::memory_order_relaxed);
    }

    size_t getHitCount() const {
        return state->hitCount.load(std::memory_order_relaxed);
    }

    double getHitRate() const {
        const size_t probeCount = state->probeCount.load(std::memory_order_relaxed);
        if (probeCount == 0) return 0.0;
        return static_cast<double>(state->hitCount.load(std::memory_order_relaxed))
            / static_cast<double>(probeCount);
    }

    static uint16_t encodeMove(const Pos& move) {
        const int x = move.getX();
        const int y = move.getY();

        if (x < 1 || x > BOARD_SIZE || y < 1 || y > BOARD_SIZE) {
            return INVALID_MOVE;
        }

        const int idx = (y - 1) * BOARD_SIZE + (x - 1);
        return static_cast<uint16_t>(idx);
    }

    static Pos decodeMove(uint16_t encodedMove) {
        if (encodedMove == INVALID_MOVE) return Pos();

        const int idx = static_cast<int>(encodedMove);
        if (idx < 0 || idx >= BOARD_SIZE * BOARD_SIZE) return Pos();

        const int x = (idx % BOARD_SIZE) + 1;
        const int y = (idx / BOARD_SIZE) + 1;
        return Pos(x, y);
    }
};
