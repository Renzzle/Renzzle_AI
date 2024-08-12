#ifndef TRANSPOSITION_TABLE_H
#define TRANSPOSITION_TABLE_H

#include <unordered_map>
#include <cstdint>
#include <mutex>

using HashKey = uint64_t;
using Value = int;

struct TTEntry {
    Value value;
    int depth;
    HashKey key;
};

class TranspositionTable {
    unordered_map<HashKey, TTEntry> table;
    mutex mtx;

public:
    void store(HashKey key, Value value, int depth) {
        lock_guard<std::mutex> lock(mtx);
        table[key] = TTEntry{value, depth, key};
    }

    bool probe(HashKey key, Value &value, int depth) {
        lock_guard<std::mutex> lock(mtx);
        auto it = table.find(key);
        if (it != table.end() && it->second.depth >= depth) {
            value = it->second.value;
            return true;
        }
        return false;
    }
};

#endif // TRANSPOSITION_TABLE_H