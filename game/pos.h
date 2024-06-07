#pragma once

#include "types.h"

#define BOARD_SIZE 15

class Pos {

    friend class Board;

private:
    int x, y;
    Direction dir; 
    const int inc[DIRECTION_SIZE][2] = {{0,1},{1,0},{1,1},{1,-1}};
    bool isValid() {
        return x >= 1 && x <= BOARD_SIZE && y >= 1 && y <= BOARD_SIZE;
    }

public:
    Pos() = delete;

    Pos(int x, int y) {
        this->x = x;
        this->y = y;
        dir = HORIZONTAL;
    }

    Pos(int x, int y, Direction dir) {
        this->x = x;
        this->y = y;
        this->dir = dir;
    }
    
    bool operator+(const int n) {
        this->x += inc[dir][0] * n;
        this->y += inc[dir][1] * n;
        if (!isValid()) {
            this->x -= inc[dir][0] * n;
            this->y -= inc[dir][1] * n;
            return false;
        } else return true;
    }
    
    bool operator-(const int n) {
        this->x -= inc[dir][0] * n;
        this->y -= inc[dir][1] * n;
        if (!isValid()) {
            this->x += inc[dir][0] * n;
            this->y += inc[dir][1] * n;
            return false;
        } else return true;
    }

};