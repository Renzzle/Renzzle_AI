#include "../evaluate/evaluator.h"
#include <iostream>
#include <windows.h>

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
    cout << "<Candidates>" << endl;
    for(auto move : moves) {
        cout << move.getX() << ", " << (char)(move.getY() + 64) << endl;
    }
    //cout << evaluator.evaluate(COLOR_BLACK) << " / " << evaluator.evaluate(COLOR_WHITE) << endl;
    //printBoardPattern(board, BLACK);

    return 0;
}