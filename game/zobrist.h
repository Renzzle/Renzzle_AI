#pragma once

#include "board.h"
#include <random>
#include <mutex>

#define NUM_PIECE_TYPES 4

static size_t zobristTable[BOARD_SIZE + 2][BOARD_SIZE + 2][NUM_PIECE_TYPES];
static std::once_flag zobristInitFlag;

static void initializeZobristTable() {
    std::random_device rd;
    std::mt19937_64 rng(rd());
    std::uniform_int_distribution<size_t> dist(0, std::numeric_limits<size_t>::max());

    for (int i = 0; i <= BOARD_SIZE + 1; ++i) {
        for (int j = 0; j <= BOARD_SIZE + 1; ++j) {
            for (int k = 0; k < NUM_PIECE_TYPES; ++k) {
                zobristTable[i][j][k] = dist(rng);
            }
        }
    }
}

static size_t getZobristValue(int x, int y, Piece piece) {
    std::call_once(zobristInitFlag, initializeZobristTable);
    return zobristTable[x][y][piece];
}
