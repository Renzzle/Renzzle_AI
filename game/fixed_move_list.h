#pragma once

#include "pos.h"
#include <array>
#include <cstddef>
#include <utility>
#include <vector>

template <size_t Capacity>
class FixedMoveList {

private:
    std::array<Pos, Capacity> data = {};
    size_t count = 0;

public:
    void clear() {
        count = 0;
    }

    void reserve(size_t) {
        // no-op: storage is fixed at compile time
    }

    void push_back(const Pos& p) {
        data[count++] = p;
    }

    template <typename... Args>
    void emplace_back(Args&&... args) {
        data[count++] = Pos(std::forward<Args>(args)...);
    }

    bool empty() const {
        return count == 0;
    }

    size_t size() const {
        return count;
    }

    Pos& front() {
        return data[0];
    }

    const Pos& front() const {
        return data[0];
    }

    Pos& operator[](size_t index) {
        return data[index];
    }

    const Pos& operator[](size_t index) const {
        return data[index];
    }

    Pos* begin() {
        return data.data();
    }

    const Pos* begin() const {
        return data.data();
    }

    Pos* end() {
        return data.data() + count;
    }

    const Pos* end() const {
        return data.data() + count;
    }

    std::vector<Pos> toMoveList() const {
        return std::vector<Pos>(begin(), end());
    }
};
