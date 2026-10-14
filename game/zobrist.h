#pragma once

#include "pos.h"
#include <cstdint>

#define NUM_PIECE_TYPES 4

inline constexpr uint64_t ZOBRIST_SEED = 0x9E3779B97F4A7C15ULL;

namespace zobrist_detail {
    struct Table {
        uint64_t values[BOARD_SIZE + 2][BOARD_SIZE + 2][NUM_PIECE_TYPES];
    };

    // std::mt19937_64(ZOBRIST_SEED) output, built at compile time: no init guard per lookup
    constexpr Table makeTable() {
        constexpr int N = 312;
        constexpr int M = 156;
        constexpr uint64_t MATRIX_A = 0xB5026F5AA96619E9ULL;
        constexpr uint64_t UPPER_MASK = 0xFFFFFFFF80000000ULL;
        constexpr uint64_t LOWER_MASK = 0x7FFFFFFFULL;

        uint64_t mt[N] = {};
        mt[0] = ZOBRIST_SEED;
        for (int i = 1; i < N; ++i) {
            mt[i] = 6364136223846793005ULL * (mt[i - 1] ^ (mt[i - 1] >> 62)) + static_cast<uint64_t>(i);
        }

        Table table = {};
        int index = N;
        for (int i = 0; i <= BOARD_SIZE + 1; ++i) {
            for (int j = 0; j <= BOARD_SIZE + 1; ++j) {
                for (int k = 0; k < NUM_PIECE_TYPES; ++k) {
                    if (index == N) {
                        for (int q = 0; q < N; ++q) {
                            const uint64_t y = (mt[q] & UPPER_MASK) | (mt[(q + 1) % N] & LOWER_MASK);
                            mt[q] = mt[(q + M) % N] ^ (y >> 1) ^ ((y & 1) != 0 ? MATRIX_A : 0);
                        }
                        index = 0;
                    }
                    uint64_t y = mt[index++];
                    y ^= (y >> 29) & 0x5555555555555555ULL;
                    y ^= (y << 17) & 0x71D67FFFEDA60000ULL;
                    y ^= (y << 37) & 0xFFF7EEE000000000ULL;
                    y ^= y >> 43;
                    table.values[i][j][k] = y;
                }
            }
        }
        return table;
    }
}

inline constexpr zobrist_detail::Table ZOBRIST_TABLE = zobrist_detail::makeTable();

inline uint64_t getZobristValue(int x, int y, Piece piece) {
    return ZOBRIST_TABLE.values[x][y][piece];
}
