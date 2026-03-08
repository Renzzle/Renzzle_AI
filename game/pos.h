#pragma once

#include "types.h"
#include <cstdint>

#define BOARD_SIZE 15

inline int getDirectionDx(Direction dir) {
    static constexpr int dx[DIRECTION_SIZE] = {0, 1, 1, 1};
    return dx[static_cast<int>(dir)];
}

inline int getDirectionDy(Direction dir) {
    static constexpr int dy[DIRECTION_SIZE] = {1, 0, 1, -1};
    return dy[static_cast<int>(dir)];
}

inline bool isBoardCoord(int x, int y) {
    return x >= 1 && x <= BOARD_SIZE && y >= 1 && y <= BOARD_SIZE;
}

class Pos {

    friend class Board;

private:
    int8_t x, y;
    Direction dir;

    static int getDx(Direction dir) {
        return getDirectionDx(dir);
    }

    static int getDy(Direction dir) {
        return getDirectionDy(dir);
    }

    bool isValid() {
        return isBoardCoord(x, y);
    }

public:
    Pos() {
        x = static_cast<int8_t>(-1);
        y = static_cast<int8_t>(-1);
        dir = HORIZONTAL;
    };

    Pos(int x, int y) {
        this->x = static_cast<int8_t>(x);
        this->y = static_cast<int8_t>(y);
        if (!isValid()) {
            this->x = static_cast<int8_t>(-1);
            this->y = static_cast<int8_t>(-1);
        }
        dir = HORIZONTAL;
    }

    Pos(int x, int y, Direction dir) {
        this->x = static_cast<int8_t>(x);
        this->y = static_cast<int8_t>(y);
        this->dir = dir;
        if (!isValid()) {
            this->x = static_cast<int8_t>(-1);
            this->y = static_cast<int8_t>(-1);
        }
    }
    
    int getX() const {
        return x;
    }

    int getY() const {
        return y;
    }

    void setDirection(Direction dir) {
        this->dir = dir;
    }
    
    bool operator+(const int n) {
        const int dx = getDx(dir);
        const int dy = getDy(dir);
        this->x += dx * n;
        this->y += dy * n;
        if (!isValid()) {
            this->x -= dx * n;
            this->y -= dy * n;
            return false;
        } else return true;
    }
    
    bool operator-(const int n) {
        const int dx = getDx(dir);
        const int dy = getDy(dir);
        this->x -= dx * n;
        this->y -= dy * n;
        if (!isValid()) {
            this->x += dx * n;
            this->y += dy * n;
            return false;
        } else return true;
    }

    bool operator<(const Pos& other) const {
        if (x == other.x) return y < other.y;
        return x < other.x;
    }

    bool operator==(const Pos& other) const {
        return x == other.x && y == other.y;
    }

    bool isDefault() {
        return (x == -1) && (y == -1);
    }
    
};
