#include <iostream>
#include <cassert>
#include <windows.h>
#include <chrono>
#include "../game/board.h"
#include "../game/cell.h"
#include "../game/line.h"
#include "../game/pos.h"
#include "../game/types.h"
#include "../search/search.h"
#include "../evaluate/evaluator.h"

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
array<array<Value, BOARD_SIZE + 2>, BOARD_SIZE + 2> values;
Value minVal = 5000;

list<Pos> getCandidates(Board board, int minVal) {
    list<Pos> moves;
    list<tuple<Pos, int>> tmp;

    Piece self = board.isBlackTurn() ? WHITE : BLACK;
    Piece oppo = !board.isBlackTurn() ? WHITE : BLACK;

    for (int i = 1; i <= BOARD_SIZE; i++) {
        for (int j = 1; j <= BOARD_SIZE; j++) {
            Cell cell = board.getCell(Pos(i, j));
            if (cell.getPiece() != EMPTY) continue;
            int val = 0;
            for (Direction dir = DIRECTION_START; dir < DIRECTION_SIZE; dir++) {
                Pattern p = cell.getPattern(self, dir);
                if (p != PATTERN_SIZE) {
                    if (p == FIVE) val += 1000000;
                    else if (p == FREE_4) val += 10000;
                    else if (p == BLOCKED_4) val += 5000;
                    else if (p == FREE_3A) val += 100;
                    else if (p == FREE_3) val += 100;
                    else val += (int)p * (int)p;
                }
                p = cell.getPattern(oppo, dir);
                if (p != PATTERN_SIZE) {
                    if (p == FIVE) val += 100000;
                }
            }
            if (val >= minVal)
                tmp.push_back(make_tuple(Pos(i, j), val));
            values[i][j] = val;
        }
    }

    tmp.sort([](const tuple<Pos, int>& a, const tuple<Pos, int>& b) {
        return get<1>(a) > get<1>(b);
    });
    for (const auto& item : tmp) {
        moves.push_back(get<0>(item));
    }

    return moves;
}

std::pair<Pos, Value> alphaBeta(Pos bestMove, int depth, Value alpha, Value beta, Board board, Evaluator evaluator, bool maximizingPlayer, Color color, std::vector<Pos>& path) {
    Result result = board.getResult();

    // cout << "Depth: " << depth << ", MaximizingPlayer: " << maximizingPlayer;
    
    // cout << ", Color: ";
    // if (color == 0) cout << "COLOR_BLACK";
    // else            cout << "COLOR_WHITE";
    
    // cout << ", Result: ";
    // if (result == 0)        cout << "ONGOING";
    // else if (result == 1)   cout << "BLACK_WIN";
    // else if (result == 2)   cout << "WHITE_WIN";
    // else                    cout << "DRAW";

    // cout << ", values[bestMove.getX()][bestMove.getY()]: " << values[bestMove.getX()][bestMove.getY()];
    // cout << ", evaluator.evaluate(color): " << evaluator.evaluate(color) << endl;

    if (result != ONGOING || depth == 0 || values[bestMove.getX()][bestMove.getY()] >= 10000) {
    //if (result != ONGOING || depth == 0 || evaluator.evaluate(color) >= 10000) {
    //if (result != ONGOING || depth == 0) {
        //return {bestMove, values[bestMove.getX()][bestMove.getY()]};
        //return {bestMove, evaluator.evaluate(color)};
        return {bestMove, values[bestMove.getX()][bestMove.getY()]};
    }

    list<Pos> moves = getCandidates(board, minVal);
    if (maximizingPlayer) {
        Value maxEval = -INF;
        for (Pos move : moves) {
            board.move(move);  
            evaluator.next(move);
            path.push_back(move);
            cout << ">> Maximizing Depth: " << depth << " / Path: ";
            for (const auto& p : path) cout << "[" << p.getX() << ", " << (char)(p.getY() + 64) << "] ";
            cout << "/ Alpha: " << (alpha == -INF ? "-INF" : to_string(alpha));
            cout << " / Beta: " << (beta == INF ? "INF" : to_string(beta)) << endl;
            Value eval = alphaBeta(bestMove, depth - 1, alpha, beta, board, evaluator, !maximizingPlayer, color, path).second;
            path.pop_back();
            evaluator.prev();
            board.undo();  
            if (eval > maxEval) {
                maxEval = eval;
                bestMove = move;
            }
            alpha = std::max(alpha, eval);

            if (alpha >= beta) {
                cout << "** Pruning at move [" << move.getX() << ", " << (char)(move.getY() + 64) << "] within the path {";
                for (Pos p : path)  cout << " [" << p.getX() << ", " << (char)(p.getY() + 64) << "] ";
                cout << "} with beta: " << beta << " **" << endl;
                break;  
            }
        }
        return {bestMove, maxEval};
    } else {
        Value minEval = INF;
        for (Pos move : moves) {
            board.move(move);  
            evaluator.next(move);
            path.push_back(move);
            cout << ">> Minimizing Depth: " << depth << " / Path: ";
            for (const auto& p : path) cout << "[" << p.getX() << ", " << (char)(p.getY() + 64) << "] ";
            cout << "/ Alpha: " << (alpha == -INF ? "-INF" : to_string(alpha));
            cout << " / Beta: " << (beta == INF ? "INF" : to_string(beta)) << endl;
            Value eval = alphaBeta(bestMove, depth - 1, alpha, beta, board, evaluator, true, color, path).second;
            path.pop_back();
            evaluator.prev();
            board.undo();  
            if (eval < minEval) {
                minEval = eval;
                bestMove = move;
            }
            beta = std::min(beta, eval);
            if (beta <= alpha) {
                cout << "** Pruning at move [" << move.getX() << ", " << (char)(move.getY() + 64) << "] within the path {";
                for (Pos p : path)  cout << " [" << p.getX() << ", " << (char)(p.getY() + 64) << "] ";
                cout << "} with alpha: " << alpha << " **" << endl;
                break;  
            }
        }
        return {bestMove, minEval};
    }
}

void printBoard(Board& board, int bestMove_X, int bestMove_Y) {
    CellArray cells = board.getBoardStatus();
    for (int i = 0; i < BOARD_SIZE + 2; i++) {
        for (int j = 0; j < BOARD_SIZE + 2; j++) {
            Piece p = cells[i][j].getPiece();
            if (i == bestMove_X && j == bestMove_Y) cout << "🔵";   // Best move (temporary)
            else if (p == WALL) {
                if (i == BOARD_SIZE + 1 && j < BOARD_SIZE) printf("%2c", j + 65);
                else if (i != 0 && i != BOARD_SIZE + 1 && j != 0) printf("%02d", i);
                continue;
            }
            else if (p == BLACK) cout << "⚫";
            else if (p == WHITE) cout << "⚪";
            else if (p == EMPTY) cout << "─┼";
        }
        cout << endl;
    }
    return;
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

    printBoard(board, -1, -1);
    cout << endl;

    Evaluator evaluator;
    evaluator.setBoard(board);
    list<Pos> moves = getCandidates(board, minVal);
    cout << "<Candidates>" << endl;
    for (auto move : moves) {
        cout << "[" << move.getX() << ", " << (char)(move.getY() + 64) << "] (evaluation = " << values[move.getX()][move.getY()] << ")" << endl;
    }
    cout << endl;;

    std::vector<Pos> path;
    cout << "<Alpha Beta Pruning>" << endl;
    auto start = chrono::high_resolution_clock::now();
    std::pair<Pos, Value> bestMove = alphaBeta(Pos(-1, -1), 4, -INF, INF, board, evaluator, true, board.isBlackTurn() ? COLOR_WHITE : COLOR_BLACK, path);
    auto end = chrono::high_resolution_clock::now();
    chrono::duration<double> duration = end - start;
    cout << "Searching time: " << duration.count() << "s" << endl;
    cout << endl;

    cout << "<Best Move>" << endl;
    cout << "[" << bestMove.first.getX() << ", " << (char)(bestMove.first.getY() + 64) << "] (evaluation = " << bestMove.second << ")";
    cout << endl;

    printBoard(board, bestMove.first.getX(), bestMove.first.getY());
    cout << endl;

    return 0;
}