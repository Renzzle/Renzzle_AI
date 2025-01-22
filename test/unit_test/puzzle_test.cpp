#include "../test.h"
#include "../util.h"
#include "../../generator/puzzle_generator.h"

int main() {
    PuzzleGenerator generator;
    Board board = generator.generatePuzzle();
    TEST_PRINT("<Puzzle>");
    printBoard(board);
    printPath(board.getPath());
}