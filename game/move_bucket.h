#pragma once

#include "pos.h"
#include <array>
#include <cstdint>
#include <vector>

class MoveBucket {

private:
    static constexpr int BIT_COUNT = 256;
    static constexpr int WORD_BITS = 64;
    static constexpr int WORD_COUNT = BIT_COUNT / WORD_BITS;

    std::array<uint64_t, WORD_COUNT> words = {};
    int count = 0;

    static uint8_t encode(const Pos& p) {
        return static_cast<uint8_t>((p.getX() << 4) | p.getY());
    }

    static Pos decode(uint8_t code) {
        return Pos(code >> 4, code & 0x0F);
    }

    static int popcount(uint64_t value) {
        return static_cast<int>(__builtin_popcountll(value));
    }

public:
    void clear() {
        words.fill(0);
        count = 0;
    }

    void push_back(const Pos& p) {
        insert(p);
    }

    void insert(const Pos& p) {
        const uint8_t code = encode(p);
        const int wordIndex = code / WORD_BITS;
        const uint64_t mask = 1ull << (code % WORD_BITS);
        if ((words[wordIndex] & mask) == 0) {
            words[wordIndex] |= mask;
            ++count;
        }
    }

    void erase(const Pos& p) {
        const uint8_t code = encode(p);
        const int wordIndex = code / WORD_BITS;
        const uint64_t mask = 1ull << (code % WORD_BITS);
        if ((words[wordIndex] & mask) != 0) {
            words[wordIndex] &= ~mask;
            --count;
        }
    }

    bool empty() const {
        return count == 0;
    }

    int size() const {
        return count;
    }

    Pos front() const {
        for (int wordIndex = 0; wordIndex < WORD_COUNT; ++wordIndex) {
            const uint64_t word = words[wordIndex];
            if (word != 0) {
                const int bitIndex = __builtin_ctzll(word);
                return decode(static_cast<uint8_t>((wordIndex * WORD_BITS) + bitIndex));
            }
        }
        return Pos();
    }

    Pos back() const {
        for (int wordIndex = WORD_COUNT - 1; wordIndex >= 0; --wordIndex) {
            const uint64_t word = words[wordIndex];
            if (word != 0) {
                const int bitIndex = 63 - __builtin_clzll(word);
                return decode(static_cast<uint8_t>((wordIndex * WORD_BITS) + bitIndex));
            }
        }
        return Pos();
    }

    Pos operator[](int index) const {
        for (int wordIndex = 0; wordIndex < WORD_COUNT; ++wordIndex) {
            uint64_t word = words[wordIndex];
            const int wordCount = popcount(word);
            if (index >= wordCount) {
                index -= wordCount;
                continue;
            }

            while (word != 0) {
                const int bitIndex = __builtin_ctzll(word);
                if (index == 0) {
                    return decode(static_cast<uint8_t>((wordIndex * WORD_BITS) + bitIndex));
                }
                word &= (word - 1);
                --index;
            }
        }
        return Pos();
    }

    void appendTo(std::vector<Pos>& result) const {
        for (int wordIndex = 0; wordIndex < WORD_COUNT; ++wordIndex) {
            uint64_t word = words[wordIndex];
            while (word != 0) {
                const int bitIndex = __builtin_ctzll(word);
                result.push_back(decode(static_cast<uint8_t>((wordIndex * WORD_BITS) + bitIndex)));
                word &= (word - 1);
            }
        }
    }
};
