#pragma once

#include "pos.h"
#include <cstdint>
#include <mutex>
#include <random>

#define NUM_PIECE_TYPES 4

inline constexpr uint64_t ZOBRIST_SEED = 0x9E3779B97F4A7C15ULL;
inline uint64_t zobristTable[BOARD_SIZE + 2][BOARD_SIZE + 2][NUM_PIECE_TYPES];
inline std::once_flag zobristInitFlag;

inline void initializeZobristTable() {
    std::mt19937_64 rng(ZOBRIST_SEED);

    for (int i = 0; i <= BOARD_SIZE + 1; ++i) {
        for (int j = 0; j <= BOARD_SIZE + 1; ++j) {
            for (int k = 0; k < NUM_PIECE_TYPES; ++k) {
                zobristTable[i][j][k] = rng();
            }
        }
    }
}

inline uint64_t getZobristValue(int x, int y, Piece piece) {
    std::call_once(zobristInitFlag, initializeZobristTable);
    return zobristTable[x][y][piece];
}
