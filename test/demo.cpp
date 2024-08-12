#include "util.h"
//#include "../search/search.h"
#include "../search/vcf_search.h"


#ifdef _WIN32
#include <Windows.h>
#include <vector>
#endif

const char* patternNames[] = { " D", "OL", "B1", " 1", "B2", " 2", "2A", "2B", "B3", " 3", "3A", "B4", " 4", " 5", " P" };

void patternDemo();
void getCandidatesDemo();
void printCell(CellArray& cells);
void printBoardPattern(Board& board, Piece p);
void printBoard(Board& board);
void printPatternCells(CellArray& cells, Piece p, Direction k);
void abpDemo();

int main() {
    #ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    #endif

    //patternDemo();
    //getCandidatesDemo();
    abpDemo();

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

void getCandidatesDemo() {
    cout << "<Candidates test>" << endl;

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

    printBoard(board);

    EvaluatorV1 evaluator;
    evaluator.setBoard(board);
    list<Pos> moves = evaluator.getCandidates();

    cout << "Candidates: " << endl;
    for(auto move : moves) {
        cout << move.getX() << ", " << (char)(move.getY() + 64) << endl;
    }
}

void printBoard(Board& board) {
    CellArray cells = board.getBoardStatus();
    
    for (int i = 0; i < BOARD_SIZE + 2; i++) {
        for (int j = 0; j < BOARD_SIZE + 2; j++) {
            Piece p = cells[i][j].getPiece();
            switch (p) {
                case WALL:
                    if (i == BOARD_SIZE + 1 && j < BOARD_SIZE) printf("%2c", j + 65);
                    else if (i != 0 && i != BOARD_SIZE + 1 && j != 0) printf(" %02d", i);
                    break;
                case BLACK:
                    cout << "⚫";
                    break;
                case WHITE:
                    cout << "⚪";
                    break;
                case EMPTY:
                    cout << "─┼";
                    break;
                default:
                    // Handle unexpected cases
                    break;
            }  
        }
        cout << endl;
    }
    
    return;
}

void printPatternCells(CellArray& cells, Piece p, Direction k) {
    for (int i = 0; i < BOARD_SIZE + 2; i++) {
        for (int j = 0; j < BOARD_SIZE + 2; j++) {
            if (cells[i][j].getPiece() != EMPTY) {
                if (cells[i][j].getPiece() == BLACK) {
                    cout << "⚫"; 
                } else if (cells[i][j].getPiece() == WHITE) {
                    cout << "⚪"; 
                } else {
                    if (i == BOARD_SIZE + 1 && j < BOARD_SIZE) 
                        printf("%2c", j + 65);
                    else if (i != 0 && i != BOARD_SIZE + 1 && j != 0) 
                        printf(" %02d", i);
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
}

void printBoardPattern(Board& board, Piece p) {
    CellArray cells = board.getBoardStatus();
    const char* directionName[] = {"Horizontal", "Vertical", "Upward", "Downward"};
    const char* pieceName[] = {"Black", "White"};
    for (int k = 0; k < 4; k++) {
        cout << pieceName[p] << ": " << directionName[k];
        printPatternCells(cells, p, static_cast<Direction>(k));
        cout << "---------------------------------------" << endl;
    }
}

Board getBoard(string moves) {
    vector<pair<int, int>> v = processString(moves);
    Board board;
    for (auto p : v) {
        board.move(Pos(p.second, p.first));
    }
    return board;
}

void abpDemo() {
    Board board;
    EvaluatorV1 eval;
    VCFSearch vcfSearcher;

    board = getBoard("h8g9i9f9c9e9g8g7i10g10e10f11e12");
    printBoard(board);
    cout << endl;

    eval.setBoard(board);
    vcfSearcher.setEvaluator(eval);

    Depth searchDepth = 10;

    TEST_TIME_START();    
    Pos bestMove = vcfSearcher.findBestMove();
    TEST_TIME_END("VCF DFS");

    TEST_PRINT("Best Move: " << bestMove.getX() << (char)(bestMove.getY() + 64));

    vcfSearcher.printWinningPath();
}