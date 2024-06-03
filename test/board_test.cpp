#include <assert.h>
#include <iostream>
#include "../game/board.h"

using namespace std;

void printBoard(Board& board) {
    for (int i = 0; i < BOARD_SIZE + 2; i++) {
        for (int j = 0; j < BOARD_SIZE + 2; j++) {
            Piece p = board.cells[i][j].piece;
            const char* c;
            if(p == WALL) c = "|";
            if(p == BLACK) c = "o";
            if(p == WHITE) c = "x";
            if(p == EMPTY) c = "-";
            cout << c << "\t";
        }
        cout << endl;
    }
    return;
}

void printBoardPattern(Board& board, Piece p) {
    for (int k = 0; k < 4; k++) {
        for (int i = 0; i < BOARD_SIZE + 2; i++) {
            for (int j = 0; j < BOARD_SIZE + 2; j++) {
                if (board.cells[i][j].piece != EMPTY) {
                    char c;
                    if(board.cells[i][j].piece == WALL) c = '|';
                    else if(board.cells[i][j].piece == BLACK) c = 'o';
                    else if(board.cells[i][j].piece == WHITE) c = 'x';
                    std::cout << c << "\t"; 
                } else {
                    if (board.cells[i][j].patterns[p][k] == PATTERN_SIZE) {
                        std::cout << "-\t";
                    } else {
                        std::cout << pattenrNames[board.cells[i][j].patterns[p][k]] << "\t";
                    }
                }
            }
            std::cout << std::endl;
        }
        std::cout << "---------------------------------------------------------------------";
        std::cout << "------------------------------------------------------------" << std::endl;
    }
}

int main(void)
{
    Board board;

    bool passed = true;

    auto start = chrono::high_resolution_clock::now();
    board.move(7, 12);
    board.move(1, 1);
    board.move(14, 12);
    board.move(1, 2);
    board.move(8, 12);
    board.move(1, 3);
    board.move(13, 12);
    board.move(1, 4);
    board.move(10, 12);
    auto end = chrono::high_resolution_clock::now();
    chrono::duration<double> duration = end - start;
    printBoardPattern(board, BLACK);
    printBoardPattern(board, WHITE);
    printBoard(board);
    std::cout << "testMove: Passed" << std::endl;
    cout << "Time taken: " << duration.count() << " seconds" << endl;

    return 0;
}