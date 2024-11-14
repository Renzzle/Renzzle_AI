#pragma once

enum Result {
    ONGOING,
    BLACK_WIN,
    WHITE_WIN,
    DRAW
};

enum Color {
    COLOR_BLACK,
    COLOR_WHITE
};

enum Piece {
    BLACK,
    WHITE,
    EMPTY,
    WALL
};

enum Pattern {
    NONE,       // 00
    DEAD,       // 01. can never make a five
    OVERLINE,   // 02. overline
    BLOCKED_1,  // 03. one step before BLOCKED_2
    FREE_1,     // 04. one step before FREE_2
    BLOCKED_2,  // 05. one step before BLOCKED_3
    FREE_2,     // 06. one step before two FREE_3
    FREE_2A,    // 07. one step before three FREE_3
    FREE_2B,    // 08. one step before four FREE_3
    BLOCKED_3,  // 09. one step before BLOCKED_4
    FREE_3,     // 10. one step before one FREE_4
    FREE_3A,    // 11. one step before two FREE_4
    BLOCKED_4,  // 12. one step before FREE_5
    FREE_4,     // 13. one step before two FREE_5
    FIVE,       // 14. five
    PATTERN_SIZE
};

enum CompositePattern {
    NOT_EMPTY,  // 00
    ETC,        // 01. etc
    FORBID,     // 02. forbidden move
    F2_ANY,     // 03. FREE_2 + etc
    B3_ANY,     // 04. BLOCKED_3 + etc
    F2_2X,      // 05. FREE_2 x 2
    B3_PLUS,    // 06. BLOCKED_3 + FREE_2 or BLOCKED_3
    F3_ANY,     // 07. FREE_3 + etc
    F3_PLUS,    // 08. FREE_3 + FREE_2 or BLOCKED_3
    F3_2X,      // 09. FREE_3 x 2
    B4_ANY,     // 10. BLOCKED_4 + etc
    B4_PLUS,    // 11. BLOCKED_4 + FREE_2 or BLOCKED_3
    B4_F3,      // 12. BLOCKED_4 + FREE_3
    MATE,       // 13. FREE_4 or BLOCKED_4 x 2
    WINNING,    // 14. FIVE
    COMPOSITE_PATTERN_SIZE
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