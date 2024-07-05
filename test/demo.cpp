#include "util.h"
#include "../search/search.h"

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

    TEST_TIME_START();
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
    TEST_TIME_END("Pattern test");
    
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

    Evaluator evaluator;
    evaluator.setBoard(board);
    list<Pos> moves = evaluator.getCandidates();

    cout << "Candidates: " << endl;
    for(auto move : moves) {
        cout << move.getX() << ", " << (char)(move.getY() + 64) << endl;
    }
}

void printCell(CellArray& cells){
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
}

void printBoard(Board& board) {
    CellArray cells = board.getBoardStatus();
    printCell(cells);

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

void abpDemo() {
    Board board;
    Evaluator eval;
    Search searcher;

    // #0: [9, E] (depth=4)
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

    // #1: [8, F] (depth=6)
    // board.move(Pos(BOARD_SIZE + 1 - 8, 8));
    // board.move(Pos(BOARD_SIZE + 1 - 9, 8));
    // board.move(Pos(BOARD_SIZE + 1 - 8, 9));
    // board.move(Pos(BOARD_SIZE + 1 - 9, 9));
    // board.move(Pos(BOARD_SIZE + 1 - 9, 7));
    // board.move(Pos(BOARD_SIZE + 1 - 10, 7));
    // board.move(Pos(BOARD_SIZE + 1 - 10, 8));
    // board.move(Pos(BOARD_SIZE + 1 - 10, 9));
    // board.move(Pos(BOARD_SIZE + 1 - 11, 9));
    // board.move(Pos(BOARD_SIZE + 1 - 12, 10));
    // board.move(Pos(BOARD_SIZE + 1 - 7, 8));
    // board.move(Pos(BOARD_SIZE + 1 - 8, 10));
    // board.move(Pos(BOARD_SIZE + 1 - 7, 7));
    // board.move(Pos(BOARD_SIZE + 1 - 7, 6));

    // #2: [5, I] (depth=4)
    // board.move(Pos(BOARD_SIZE + 1 - 8, 8));
    // board.move(Pos(BOARD_SIZE + 1 - 9, 8));
    // board.move(Pos(BOARD_SIZE + 1 - 8, 9));
    // board.move(Pos(BOARD_SIZE + 1 - 8, 7));
    // board.move(Pos(BOARD_SIZE + 1 - 10, 9));
    // board.move(Pos(BOARD_SIZE + 1 - 9, 10));
    // board.move(Pos(BOARD_SIZE + 1 - 9, 9));
    // board.move(Pos(BOARD_SIZE + 1 - 7, 9));
    // board.move(Pos(BOARD_SIZE + 1 - 10, 10));
    // board.move(Pos(BOARD_SIZE + 1 - 11, 11));
    // board.move(Pos(BOARD_SIZE + 1 - 10, 8));
    // board.move(Pos(BOARD_SIZE + 1 - 10, 11));
    // board.move(Pos(BOARD_SIZE + 1 - 8, 10));
    // board.move(Pos(BOARD_SIZE + 1 - 7, 11));

    printBoard(board);
    cout << endl;

    Depth searchDepth;
    for (searchDepth = 7; searchDepth <= 10; searchDepth++) {
        eval.setBoard(board);
        searcher.setEvaluator(eval);

        cout << "----- searchDepth = " << searchDepth << " -----";
        Pos bestMove = searcher.findBestMove(searchDepth, board.isBlackTurn());
        
        cout << "\n<Best Move> ";
        cout << "[" << bestMove.getX() << ", " << (char)(bestMove.getY() + 64) << "] (for " << (board.isBlackTurn() ? "BLACK" : "WHITE") << ")" << endl << endl;
    }
}