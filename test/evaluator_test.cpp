#include "../evaluate/evaluator.h"
#include <iostream>
#include <windows.h>

using namespace std;

const char* patternNames[] = {
    "D",   
    "OL",  
    "B1", 
    "F1",
    "B2",
    "F2",
    "F2A",
    "F2B",
    "B3",
    "F3",
    "F3A",
    "B4",
    "F4",
    "F5", 
    "P"
};

void printBoard(Board& board) {
    CellArray cells = board.getBoardStatus();
    for (int i = 0; i < BOARD_SIZE + 2; i++) {
        for (int j = 0; j < BOARD_SIZE + 2; j++) {
            Piece p = cells[i][j].getPiece();
            if(p == WALL) {
                if (i == 0 && j < BOARD_SIZE) printf("%2c", j + 65);
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
    for (int k = 0; k < 4; k++) {
        for (int i = 0; i < BOARD_SIZE + 2; i++) {
            for (int j = 0; j < BOARD_SIZE + 2; j++) {
                if (cells[i][j].getPiece() != EMPTY) {
                    if(cells[i][j].getPiece() == BLACK)  {
                        cout << "O\t"; 
                    } else if(cells[i][j].getPiece() == WHITE) {
                        cout << "X\t"; 
                    } else {
                        if(i == 0 || i == BOARD_SIZE + 1) cout << j << "\t";
                        else cout << i << "\t";
                    }
                } else {
                    if (cells[i][j].getPattern(p, static_cast<Direction>(k)) == PATTERN_SIZE) {
                        std::cout << "-\t";
                    } else {
                        std::cout << patternNames[cells[i][j].getPattern(p, static_cast<Direction>(k))] << "\t";
                    }
                }
            }
            std::cout << std::endl << endl;
        }
        std::cout << "---------------------------------------------------------------------";
        std::cout << "------------------------------------------------------------" << std::endl;
    }
}

int main() {
    SetConsoleOutputCP(CP_UTF8);
    Board board;

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
    // board.move(Pos(6, 11));
    // board.move(Pos(1, 1));
    // board.move(Pos(7, 10));
    printBoard(board);

    Evaluator evaluator;
    evaluator.setBoard(board);
    list<Pos> moves = evaluator.getCandidates();
    for(auto move : moves) {
        cout << move.getX() << ", " << (char)(move.getY() + 64) << endl;
    }
    //cout << evaluator.evaluate(COLOR_BLACK) << " / " << evaluator.evaluate(COLOR_WHITE) << endl;

    return 0;
}