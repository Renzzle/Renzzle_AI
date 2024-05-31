#include <iostream>
#include <stdlib.h>
#include <string>
#include "board.h"

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

int main(void)
{
    Cell cells[BOARD_SIZE + 2][BOARD_SIZE + 2];

    string test = "h8i9i8h9j8m7g14j14l14l13m12m11l9";
    putBoardStatus(cells, test);

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