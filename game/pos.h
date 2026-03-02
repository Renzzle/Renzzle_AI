#pragma once

#include "types.h"
#include <cstdint>

#define BOARD_SIZE 15

class Pos {

    friend class Board;

private:
    int8_t x, y;
    Direction dir;

    static int getDx(Direction dir) {
        switch (dir) {
            case HORIZONTAL: return 0;
            case VERTICAL:   return 1;
            case UPWARD:     return 1;
            case DOWNWARD:   return 1;
            default:         return 0;
        }
    }

    static int getDy(Direction dir) {
        switch (dir) {
            case HORIZONTAL: return 1;
            case VERTICAL:   return 0;
            case UPWARD:     return 1;
            case DOWNWARD:   return -1;
            default:         return 0;
        }
    }

    bool isValid() {
        return x >= 1 && x <= BOARD_SIZE && y >= 1 && y <= BOARD_SIZE;
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
