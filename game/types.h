enum Piece {
    BLACK,
    WHITE,
    EMPTY,
    WALL
};

enum Pattern {
    DEAD,        // can never make a five
    BLOCKED_1,   // one step before BLOCKED_2
    FREE_1,      // one step before FREE_2
    BLOCKED_2,   // one step before BLOCKED_3
    FREE_2,      // one step before two FREE_3
    FREE_2A,     // one step before three FREE_3
    FREE_2B,     // one step before four FREE_3
    BLOCKED_3,   // one step before BLOCKED_4
    FREE_3,      // one step before one FREE_4
    FREE_3A,     // one step before two FREE_4
    BLOCKED_4,   // one step before FREE_5
    FREE_4,      // one step before two FREE_5
    FIVE,        // five
    OVERLINE,    // overline
    PATTERN_SIZE
};

#define DIRECTION_START HORIZONTAL

enum Direction {
    HORIZONTAL,
    VERTICAL,
    UPWARD,
    DOWNWARD,
    DIRECTION_SIZE
};

Direction operator++(Direction& dir, int) {
    int d = static_cast<int>(dir);
    d++;
    if (d > 4) d = 4;
    dir = static_cast<Direction>(d);
    return static_cast<Direction>(d);
}

Direction operator--(Direction& dir, int) {
    int d = static_cast<int>(dir);
    d--;
    if (d < 0) d = 0;
    dir = static_cast<Direction>(d);
    return static_cast<Direction>(d);
}

bool operator<(Direction dir1, Direction dir2) {
    return static_cast<int>(dir1) < static_cast<int>(dir2);
}

bool operator>(Direction dir1, Direction dir2) {
    return static_cast<int>(dir1) > static_cast<int>(dir2);
}