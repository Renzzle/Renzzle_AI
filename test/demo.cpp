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
void testGenKey();

int main() {
    #ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    #endif

    //patternDemo();
    //getCandidatesDemo();
    // abpDemo();
    testGenKey();

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

    Evaluator evaluator;
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

void testGenKey(){

    Board board;
    Tree tree;
    board = getBoard("    a14a15c15d14h9e4e9c12j4k7f10b5m11i5g11c7e12g8k11g9c4d11f7k10k4f4e10d5f12i7i11i4c2k8d12g2d8m12g7g13g6j13j8l13h5i14e5e14l11l5k9f3l10f5e7k5i10c5c13i2j9m2h14a10a4b12b7d13f8k13n5m8j3f9b4a8d15d10c10g14l9o12j2k6m3a7j15d7e13g1j7l2c9n12n15o8n13o2n6m1m15j11n10n7n2o6");
    TEST_TIME_START();
    for (int i = 0; i < 10000; i++)
        tree.generateKey(board);
    TEST_TIME_END("genKey : ");


}

void abpDemo() {
    Board board;
    Evaluator eval;
    VCFSearch vcfSearcher(board);

    /*
    a14a15c15d14h9e4e9c12j4k7f10b5m11i5g11c7e12g8k11g9c4d11f7k10k4f4e10d5f12i7i11i4c2k8d12g2d8m12g7g13g6j13j8l13h5i14e5e14l11l5k9f3l10f
    5e7k5i10c5c13i2j9m2h14a10a4b12b7d13f8k13n5m8j3f9b4a8d15d10c10g14l9o12j2k6m3a7j15d7e13g1j7l2c9n12n15o8n13o2n6m1m15j11n10n7n2o6
    */

    // #0: [9, E] (depth=4)
    // board.move(Pos(7, 7));
    // board.move(Pos(7, 8));
    // board.move(Pos(8, 7));
    // board.move(Pos(6, 7));
    // board.move(Pos(8, 9));
    // board.move(Pos(8, 8));
    // board.move(Pos(9, 8));
    // board.move(Pos(10, 9));
    // board.move(Pos(10, 7));
    // board.move(Pos(11, 6));
    // board.move(Pos(6, 8));
    // board.move(Pos(5, 7));
    // board.move(Pos(5, 9));
    // board.move(Pos(4, 10));
    // board.move(Pos(6, 9));
    // board.move(Pos(7, 9));

    // #1: [8, F] (depth=9)
    // board.move(Pos(BOARD_SIZE + 1 - 8, 8));
    // board.move(Pos(BOARD_SIZE + 1 - 9, 8));
    // board.move(Pos(BOARD_SIZE + 1 - 8, 9));
    // board.move(Pos(BOARD_SIZE + 1 - 9, 9));
    // board.move(Pos(BOARD_SIZE + 1 - 9, 7));
    // board.move(Pos(BOARD_SIZE + 1 - 10, 7));
    // board.move(Pos(BOARD_SIZE + 1 - 10, 8));
    // board.move(Pos(BOARD_SIZE + 1 - 10, 9));
    // board.move(Pos(BOARD_SIZE + `1 - 11, 9));
    // board.move(Pos(BOARD_SIZE + 1 - 12, 10));
    // board.move(Pos(BOARD_SIZE + 1 - 7, 8));
    // board.move(Pos(BOARD_SIZE + 1 - 8, 10));
    // board.move(Pos(BOARD_SIZE + 1 - 7, 7));
    // board.move(Pos(BOARD_SIZE + 1 - 7, 6));

    // board = getBoard("h8h9i8g8i10j9i9i7j10k11h10k10j8k7g10f10g11f12g7f6f7f11");
    //board = getBoard("h8h9i8g8i10i9h11g12j9i11j11k10h7i7");
    board = getBoard("h8g9h9g10h10h6i8i9g11i10f13h12d10e11d9e9d8b10c9f8e8i13e7e13i6");
    //board = getBoard("h8h9h10g8f9i9j9g10i8k10g9f7e6f8g6f5f6h6e9d8e8e7e11f10c9d9d10b8c11e12d11f11d13d7");
    //board = getBoard("h8h9i9g10e12f11g8g9i10");
    //board = getBoard("h8g9i9f9c9e9g8g7i10g10e10f11e12");
    printBoard(board);
    cout << endl;

    Depth searchDepth;

    eval.setBoard(board);
    vcfSearcher.setEvaluator(eval);

    TEST_TIME_START();    
    Pos bestMove = vcfSearcher.findBestMove();
    TEST_TIME_END("VCF DFS");
    TEST_PRINT(bestMove.getX() << (char)(bestMove.getY() + 64));

    vcfSearcher.printWinningPath();
    cout << "Nodes explored: " << vcfSearcher.getNodesExplored() << endl;

    // for (searchDepth = 9; searchDepth <= 11; searchDepth++) {
    //     TEST_TIME_START();
    //     eval.setBoard(board);
    //     vcfSearcher.setEvaluator(eval);

    //     cout << "----- searchDepth = " << searchDepth << " -----" << endl;
        
    //     Pos bestMove = vcfSearcher.findBestMove(searchDepth, board.isBlackTurn());
    //     cout << "\n<Best Move> ";
    //     cout << "[" << bestMove.getX() << ", " << (char)(bestMove.getY() + 64) << "] (for " << (board.isBlackTurn() ? "BLACK" : "WHITE") << ")" << endl << endl;
    //     TEST_TIME_END("VCF DFS: depth: " << searchDepth << " / ");
    // }
}