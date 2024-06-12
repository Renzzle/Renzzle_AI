#include "../game/board.h"

#include <cassert>
#include <iostream>
#include <chrono>

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

void print_board(int put[15][15])
{
	for (int i = 0; i < 15; i++) {
		printf("");
		for (int j = 0; j < 15; j++) {
			if (put[i][j] == 0 && i == 0 && j == 0)
				printf("�� ");
			else if (put[i][j] == 0 && i == 0 && j == 14)
				printf("�� ");
			else if (put[i][j] == 0 && i == 0)
				printf("�� ");
			else if (put[i][j] == 0 && i == 14 && j == 0)
				printf("�� ");
			else if (put[i][j] == 0 && i == 14 && j == 14)
				printf("�� ");
			else if (put[i][j] == 0 && i == 14)
				printf("�� ");
			else if (put[i][j] == 0 && j == 0)
				printf("�� ");
			else if (put[i][j] == 0 && j == 14)
				printf("�� ");
			else if (put[i][j] == 0)
				printf("�� ");
			else if (put[i][j] == 1) // black
				printf("��"); 
			else if (put[i][j] == -1) // white
				printf("��");
		}
		printf("%02d\n", 15 - i);
	}

	printf("A B C D E F G H I J K L M N O\n");
}

void printBoard(Board& board) {
    CellArray cells = board.getBoardStatus();
    for (int i = 0; i < BOARD_SIZE + 2; i++) {
        for (int j = 0; j < BOARD_SIZE + 2; j++) {
            Piece p = cells[i][j].getPiece();
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
    CellArray cells = board.getBoardStatus();
    for (int k = 0; k < 4; k++) {
        for (int i = 0; i < BOARD_SIZE + 2; i++) {
            for (int j = 0; j < BOARD_SIZE + 2; j++) {
                if (cells[i][j].getPiece() != EMPTY) {
                    char c;
                    if(cells[i][j].getPiece() == WALL) c = '|';
                    else if(cells[i][j].getPiece() == BLACK) c = 'o';
                    else if(cells[i][j].getPiece() == WHITE) c = 'x';
                    std::cout << c << "\t"; 
                } else {
                    if (cells[i][j].getPattern(p, static_cast<Direction>(k)) == PATTERN_SIZE) {
                        std::cout << "-\t";
                    } else {
                        std::cout << patternNames[cells[i][j].getPattern(p, static_cast<Direction>(k))] << "\t";
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

    cout << "start!" << endl;
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
    printBoardPattern(board, BLACK);
    printBoardPattern(board, WHITE);
    std::cout << "testMove: Passed" << std::endl;
    cout << "Time taken: " << duration.count() << " seconds" << endl;

    // board.move(Pos(1, 5));
    // printBoard(board);
    // cout << board.getResult() << endl;
    // if(!board.move(Pos(4, 4)));
    //     cout << "game is finished" << endl;

    board.move(Pos(2, 1));
    board.move(Pos(11, 12));
    board.move(Pos(3, 1));
    board.move(Pos(12, 12));
    printBoard(board);
    cout << board.getResult() << endl;
    board.undo();
    printBoard(board);

    int b[15][15];
    b[7][7] = 1;
    b[7][8] = -1;
    print_board(b);

    return 0;
}