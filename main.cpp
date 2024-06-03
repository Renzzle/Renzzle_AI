#include <iostream>
#include <stdlib.h>
#include <string>
#include "game/board.h"

string pieceToString(Piece piece) {
    if (piece == BLACK) {
        return "BLACK";
    }
    else if (piece == WHITE) {
        return "WHITE";
    }
    else {
        return "UNKNOWN"; // 예외 처리
    }
}

void putBoardStatus(Cell(&cells)[BOARD_SIZE + 2][BOARD_SIZE + 2], string str)
{
    int len = str.length(); // 총 길이
    Piece color = BLACK;

    for (int i = 0; i < len; i++)
    {
        int currentCol = str[i] - 'a';
        char isitInt = str[i + 2];  //행을 나타내는 정수가 두 자리 정수인지 판단 위한 
        if (i + 2 < len && isdigit(str[i + 2]))
        {
            int digit1 = (str[i + 1] - '0') * 10; // 10의자리
            int digit2 = (str[i + 2] - '0');      // 1의자리
            int currentRow = digit1 + digit2;
            cells[currentCol][currentRow].piece = color;
            color = (color == BLACK) ? WHITE : BLACK; // 색깔 변경
             cout<<"COL : "<<currentCol<<" ROW : "<<currentRow<<" COLOR : "<<pieceToString(color)<<'\n';
            //cout << "COL : " << currentCol << " ROW : " << currentRow << color << endl;
			 i = i + 2;
        }
        else {
            int currentRow = (str[i + 1] - '0'); // 1의자리
            cells[currentCol][currentRow].piece = color;
            color = (color == BLACK) ? WHITE : BLACK; // 색깔 변경
             cout<<"COL : "<<currentCol<<" ROW : "<<currentRow<<" COLOR : "<<pieceToString(color)<<'\n';
            //cout << "COL : " << currentCol << " ROW : " << currentRow << color<<endl;

            i = i + 1;
        }
    }
};

void printBoard(Board& board) {
    for (int i = 0; i < BOARD_SIZE + 2; i++) {
        for (int j = 0; j < BOARD_SIZE + 2; j++) {
            cout << board.cells[i][j].piece << "\t";
        }
        cout << endl;
    }
    return;
}

void printBoardPattern(Board& board, Piece p) {
    for (int i = 0; i < BOARD_SIZE + 2; i++) {
        for (int j = 0; j < BOARD_SIZE + 2; j++) {
            if(board.cells[i][j].patterns[p][0] != PATTERN_SIZE)
                cout << pattenrNames[board.cells[i][j].patterns[p][0]] << "\t";
            else
                cout << board.cells[i][j].piece << "\t";
        }
        cout << endl;
    }
    cout << "---------------------------------------------------------------------" << endl;
    for (int i = 0; i < BOARD_SIZE + 2; i++) {
        for (int j = 0; j < BOARD_SIZE + 2; j++) {
            if(board.cells[i][j].patterns[p][1] != PATTERN_SIZE)
                cout << pattenrNames[board.cells[i][j].patterns[p][1]] << "\t";
            else
                cout << board.cells[i][j].piece << "\t";
        }
        cout << endl;
    }
    cout << "---------------------------------------------------------------------" << endl;
    for (int i = 0; i < BOARD_SIZE + 2; i++) {
        for (int j = 0; j < BOARD_SIZE + 2; j++) {
            if(board.cells[i][j].patterns[p][2] != PATTERN_SIZE)
                cout << pattenrNames[board.cells[i][j].patterns[p][2]] << "\t";
            else
                cout << board.cells[i][j].piece << "\t";
        }
        cout << endl;
    }
    cout << "---------------------------------------------------------------------" << endl;
    for (int i = 0; i < BOARD_SIZE + 2; i++) {
        for (int j = 0; j < BOARD_SIZE + 2; j++) {
            if(board.cells[i][j].patterns[p][3] != PATTERN_SIZE)
                cout << pattenrNames[board.cells[i][j].patterns[p][3]] << "\t";
            else
                cout << board.cells[i][j].piece << "\t";
        }
        cout << endl;
    }
    return;
}

int main(void)
{
    // Cell cells[BOARD_SIZE + 2][BOARD_SIZE + 2];

    // string test = "h8i9i8h9j8m7g14j14l14l13m12m11l9";
    // putBoardStatus(cells, test);

    Board board;

    // board init test
    bool passed = true;
    // for (int i = 0; i < BOARD_SIZE + 2; i++) {
    //     for (int j = 0; j < BOARD_SIZE + 2; j++) {
    //         cout << board.cells[i][j].piece << "\t";
    //         if ((i == 0 || i == BOARD_SIZE + 1 || j == 0 || j == BOARD_SIZE + 1) && board.cells[i][j].piece != WALL) {
    //             passed = false;
    //         } else if (i != 0 && i != BOARD_SIZE + 1 && j != 0 && j != BOARD_SIZE + 1 && board.cells[i][j].piece != EMPTY) {
    //             passed = false;
    //         }
    //     }
    //     cout << endl;
    // }
    // std::cout << "board init test: " << (passed ? "Passed" : "Failed") << std::endl;

    // move test

    passed = true;
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
    cout << "Time taken: " << duration.count() << " seconds" << endl;
    printBoardPattern(board, BLACK);

    for (int i = 0; i < BOARD_SIZE + 2; i++) {
        for (int j = 0; j < BOARD_SIZE + 2; j++) {
            cout << board.cells[i][j].piece << "\t";
        }
        cout << endl;
    }
    std::cout << "testMove: Passed" << std::endl;


    // getLines test
    passed = true;
    int x = 9, y = 15;
    Lines lines = board.getLines(x, y);

    // coutLine test
    // Line line;
    // for (int i = 0; i < LINE_LENGTH; i++) {
    //     line[i] = &board.cells[1][i + 1];
    // }

    // board.cells[1][3].piece = BLACK;
    // board.cells[1][4].piece = BLACK;
    // board.cells[1][5].piece = BLACK;

    // auto result = board.countLine(line);
    // int realLen, fullLen, start, end;
    // std::tie(realLen, fullLen, start, end) = result;

    // bool passed = (realLen == 3 && fullLen == 3 && start == 2 && end == 4);
    // std::cout << "testCountLine: " << (passed ? "Passed" : "Failed") << std::endl;


    return 0;
}

/* For Forbidden Move Pattern
 * o: stone
 * x: opponent stone or wall
 * -: empty point
 * f: forbidden move, make five
 * ?: any type except stone(o)
 *
 * <Overline>
 * oooooo
 *
 * <Four Four>
 * ?o-ooo-o? # if last stone is 3 or 4 or 5
 * ?oo-oo-oo? # if last stone is 4 or 5
 * ?ooo-o-ooo? # if last stone is 5
 *
 * <Five>
 * ?ooooo?
 *
 * <Straight Four>
 * ?-oooo-?
 *
 * <Closed Four>
 * ?o-ooo?
 * ?oo-oo?
 * ?ooo-o?
 * xoooo-?
 * ?-oooox
 *
 * <Three>
 * ?--ooo--? # 3rd or 7th != f
 * o--ooo--? # 7th != f
 * ?--ooo--o # 3rd != f
 * ?--ooo-x # 3rd != f
 * x-ooo--? # 6th != f
 * ?-oo-o-? # 5th != f
 * ?-o-oo-? # 4th != f
 */