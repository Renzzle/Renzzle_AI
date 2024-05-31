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

enum Direction {
    HORIZONTAL,
    VERTICAL,
    UPWARD,
    DOWNWARD,
    DIRECTION_SIZE
};