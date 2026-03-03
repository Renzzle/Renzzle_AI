#pragma once

#include "../game/pos.h"
#include <cstddef>
#include <cstdint>
#include <limits>
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

    std::vector<TTEntry> entries;
    size_t bucketCount = 0;
    size_t associativity = 4;
    size_t memoryBytes = 0;
    size_t usedEntries = 0;
    uint8_t generation = 0;

    mutable size_t probeCount = 0;
    mutable size_t hitCount = 0;

    static size_t floorPowerOfTwo(size_t v) {
        size_t p = 1;
        while (p <= v / 2) {
            p <<= 1;
        }
        return p;
    }

    size_t getBucketBase(size_t bucketIndex) const {
        return bucketIndex * associativity;
    }

    size_t getBucketIndex(uint64_t key) const {
        return static_cast<size_t>(key) & (bucketCount - 1);
    }

    int getReplacementScore(const TTEntry& entry) const {
        const int ageDist = static_cast<int>((generation - entry.getAge()) & TTEntry::AGE_MASK);
        // Keep deeper and newer entries. Low score means easy victim.
        return static_cast<int>(entry.depth) - (ageDist * 2);
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

        associativity = assoc;
        bucketCount = floorPowerOfTwo(rawBucketCount);

        entries.assign(bucketCount * associativity, TTEntry());
        memoryBytes = entries.size() * sizeof(TTEntry);
        usedEntries = 0;
        generation = 0;
        resetStats();
    }

    void clear() {
        for (auto& entry : entries) {
            entry = TTEntry();
        }
        usedEntries = 0;
        generation = 0;
        resetStats();
    }

    void nextGeneration() {
        generation = (generation + 1) & TTEntry::AGE_MASK;
    }

    const TTEntry* probe(uint64_t key) const {
        if (bucketCount == 0) return nullptr;

        probeCount++;
        const size_t base = getBucketBase(getBucketIndex(key));

        for (size_t i = 0; i < associativity; i++) {
            const TTEntry& entry = entries[base + i];
            if (!entry.isEmpty() && entry.key == key) {
                hitCount++;
                return &entry;
            }
        }
        return nullptr;
    }

    void store(uint64_t key, int32_t score, uint8_t depth, TTFlag flag, uint16_t bestMove = INVALID_MOVE) {
        if (bucketCount == 0) return;

        const size_t base = getBucketBase(getBucketIndex(key));

        size_t slotIdx = std::numeric_limits<size_t>::max();
        size_t emptyIdx = std::numeric_limits<size_t>::max();
        size_t victimIdx = base;
        int victimScore = std::numeric_limits<int>::max();

        for (size_t i = 0; i < associativity; i++) {
            const size_t idx = base + i;
            const TTEntry& entry = entries[idx];

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

        TTEntry& target = entries[slotIdx];
        const bool wasEmpty = target.isEmpty();

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
            usedEntries++;
        }
    }

    size_t getUsedEntryCount() const {
        return usedEntries;
    }

    double getLoadFactor() const {
        if (entries.empty()) return 0.0;
        return static_cast<double>(usedEntries) / static_cast<double>(entries.size());
    }

    size_t getEntryCount() const {
        return entries.size();
    }

    size_t getBucketCount() const {
        return bucketCount;
    }

    size_t getAssociativity() const {
        return associativity;
    }

    size_t getMemoryBytes() const {
        return memoryBytes;
    }

    uint8_t getGeneration() const {
        return generation;
    }

    void resetStats() const {
        probeCount = 0;
        hitCount = 0;
    }

    size_t getProbeCount() const {
        return probeCount;
    }

    size_t getHitCount() const {
        return hitCount;
    }

    double getHitRate() const {
        if (probeCount == 0) return 0.0;
        return static_cast<double>(hitCount) / static_cast<double>(probeCount);
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
