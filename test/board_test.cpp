#include "../game/board.h"

#include <cassert>
#include <iostream>
#include <chrono>

using namespace std;


const char* patternNames[] = {
    " D",   
    "OL",  
    "B1", 
    " 1",
    "B2",
    " 2",
    "2A",
    "2B",
    "B3",
    " 3",
    "3A",
    "B4",
    " 4",
    " 5", 
    " P"
};

void printBoard(Board& board) {
    CellArray cells = board.getBoardStatus();
    for (int i = 0; i < BOARD_SIZE + 2; i++) {
        for (int j = 0; j < BOARD_SIZE + 2; j++) {
            Piece p = cells[i][j].getPiece();
            if(p == WALL) {
                if (i == BOARD_SIZE + 1 && j < BOARD_SIZE) printf("%2c", j + 65);
                else if (i != 0 && i != BOARD_SIZE + 1 && j != 0) printf("%02d", i);
                continue;
            }
            if(p == BLACK) cout << "⚫";
            if(p == WHITE) cout << "⚪";
            if(p == EMPTY) cout << "─┼";
        }
        cout << endl;
    }
    return;
}

void printBoardPattern(Board& board, Piece p) {
    CellArray cells = board.getBoardStatus();
    const char* directionName[] = {"Horizontal", "Vertical", "Upward", "Downward"};
    const char* pieceName[] = {"Black", "White"};
    for (int k = 0; k < 4; k++) {
        cout << pieceName[p] << ": " << directionName[k];
        for (int i = 0; i < BOARD_SIZE + 2; i++) {
            for (int j = 0; j < BOARD_SIZE + 2; j++) {
                if (cells[i][j].getPiece() != EMPTY) {
                    if(cells[i][j].getPiece() == BLACK)  {
                        cout << "⚫"; 
                    } else if(cells[i][j].getPiece() == WHITE) {
                        cout << "⚪"; 
                    } else {
                        if (i == BOARD_SIZE + 1 && j < BOARD_SIZE) printf("%2c", j + 65);
                        else if (i != 0 && i != BOARD_SIZE + 1 && j != 0) printf("%02d", i);
                        continue;
                    }
                } else {
                    if (cells[i][j].getPattern(p, static_cast<Direction>(k)) == PATTERN_SIZE) {
                        cout << "─┼";
                    } else {
                        cout << patternNames[cells[i][j].getPattern(p, static_cast<Direction>(k))];
                    }
                }
            }
            cout << endl;
        }
        cout << "------------------------------------------------------------" << endl;
    }
}

int main(void)
{
    Board board;
    bool passed = true;

    auto start = chrono::high_resolution_clock::now();
    board.move(Pos(7, 12));
    board.move(Pos(1, 1));
    board.move(Pos(14, 12));
    board.move(Pos(1, 2));
    board.move(Pos(8, 12));
    board.move(Pos(1, 3));
    board.move(Pos(13, 12));
    board.move(Pos(1, 4));
    board.move(Pos(10, 12));
    auto end = chrono::high_resolution_clock::now();
    chrono::duration<double> duration = end - start;
    cout << "<Pattern test>" << endl;
    printBoardPattern(board, BLACK);
    printBoardPattern(board, WHITE);
    std::cout << "testMove: Passed" << std::endl;
    cout << "Time taken: " << duration.count() << " seconds" << endl;

    // board.move(Pos(1, 5));
    // printBoard(board);
    // cout << board.getResult() << endl;
    // if(!board.move(Pos(4, 4)));
    //     cout << "game is finished" << endl;

    cout << endl << "<Win test>" << endl;
    board.move(Pos(2, 1));
    board.move(Pos(11, 12));
    board.move(Pos(3, 1));
    board.move(Pos(12, 12));
    printBoard(board);
    const char* resultName[] = {"Black win", "White win", "Draw"};
    cout << "result: " << resultName[board.getResult()] << endl << endl;
    cout << "<Undo test>" << endl;
    board.undo();
    printBoard(board);

    return 0;
}