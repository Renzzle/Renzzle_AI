#include "util.h"
#include <vector>

using namespace std;

void patternDemo();
void getCandidatesDemo();

int main() {
    patternDemo();

    return 0;
}

void patternDemo() {
    cout << "<Pattern test>" << endl;

    Board board;
    bool passed = true;

    board.move(Pos(7, 7));
    board.move(Pos(7, 8));
    board.move(Pos(8, 7));
    board.move(Pos(6, 7));
    board.move(Pos(8, 9));
    board.move(Pos(8, 8));
    board.move(Pos(9, 8));
    board.move(Pos(10, 9));
    board.move(Pos(10, 7));
    board.move(Pos(11, 6));
    board.move(Pos(6, 8));
    board.move(Pos(5, 7));
    board.move(Pos(5, 9));
    board.move(Pos(4, 10));
    board.move(Pos(6, 9));
    board.move(Pos(7, 9));
    
    printBoardPattern(board, BLACK);
    printBoardPattern(board, WHITE);
}