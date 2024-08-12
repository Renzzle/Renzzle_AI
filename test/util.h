#pragma once

#include <iostream>
#include <chrono>
#include <cassert>
#include <vector>
#include <string>
#include "../game/board.h"

#ifdef _WIN32
#include <Windows.h>
#endif

#define DEBUG 1

#if DEBUG

#define TEST_PRINT(msg) std::cout << msg << std::endl

#define TEST_TIME_START() \
    auto start_time = std::chrono::high_resolution_clock::now();

#define TEST_TIME_END(message) \
    auto end_time = std::chrono::high_resolution_clock::now(); \
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time); \
    double seconds = duration.count() / 1e9; \
    std::cout << message << " is taken " << seconds  << " sec" << std::endl;

#define TEST_ASSERT(expression) assert(expression)

#else

#define TEST_PRINT(msg)
#define TEST_TIME_START()
#define TEST_TIME_END(message)
#define TEST_ASSERT(expression)

#endif

using namespace std;

vector<pair<int, int>> processString(const string& input) {
    vector<pair<int, int>> result;
    for (size_t i = 0; i < input.length(); i += 2) {
        char letter = input[i];
        int number;
        if (isdigit(input[i + 1]) && isdigit(input[i + 2])) {
            number = (input[i + 1] - '0') * 10 + (input[i + 2] - '0');
            i++; 
        } else {
            number = input[i + 1] - '0';
        }

        int letterValue = letter - 'a' + 1;

        result.emplace_back(letterValue, number);
    }
    return result;
}

Board getBoard(string moves) {
    vector<pair<int, int>> v = processString(moves);
    Board board;
    for (auto p : v) {
        board.move(Pos(p.first, p.second));
    }
    return board;
}

void printBoard(Board& board) {
    #ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    #endif

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
    const char* patternNames[] = { " D", "OL", "B1", " 1", "B2", " 2", "2A", "2B", "B3", " 3", "3A", "B4", " 4", " 5", " P" };
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

void printPath(vector<Pos> path) {
    for (const auto& pos : path) {
        cout << (char)(pos.getY() + 96) << pos.getX();
    }
    cout << endl;
}