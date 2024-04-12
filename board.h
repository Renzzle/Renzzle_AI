#define BOARD_SIZE 15

enum Direction {
    HORIZONTAL,
    VERTICAL,
    UPWARD,
    DOWNWARD
};

struct Pos {
    int x;
    int y;
};

enum Piece {
    WALL, BLACK, WHITE, EMPTY
};  

// enum Pattern {
//     OVERLINE,
//     FIVE,
//     FOUR,
//     STRAIGHT_FOUR,
//     THREE,
//     ELSE
// };

struct Cell {
    Piece piece;
    //Pattern patterns[4];
} typedef Cell;

class Board {
    private:
        int moveCount;
        Cell cells[BOARD_SIZE + 2][BOARD_SIZE + 2]; // add wall 추가
    public:
        Cell* getBoard();
        void move(Pos pos);
};

/* Pattern
 * o: stone, x: opponent stone or wall, -: empty point, f: forbidden move, make five
 *
 * <Overline>
 * oooooo
 * ooooooo
 * oooooooo
 * ooooooooo
 * 
 * <Four Four>
 * o-ooo-o # if last stone is 3 or 4 or 5
 * oo-oo-oo # if last stone is 4 or 5
 * ooo-o-ooo # if last stone is 5
 * 
 * <Five> 
 * ooooo
 * 
 * <Straight Four>
 * -oooo-
 * 
 * <Closed Four>
 * o-ooo
 * oo-oo
 * ooo-o
 * xoooo-
 * -oooox
 * 
 * <Three>
 * --ooo-- # need to judge 2, 6
 * --ooo-x # need to judge 2
 * x-ooo-- # need to judge 6
 * -oo-o- # need to judge 4
 * -o-oo- # need to judge 3
 */