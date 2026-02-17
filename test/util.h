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

        result.emplace_back(number, letterValue);
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
    
    for (int i = BOARD_SIZE + 1; i >= 0; i--) {
        for (int j = 0; j < BOARD_SIZE + 2; j++) {
            Piece p = cells[i][j].getPiece();
            switch (p) {
                case WALL:
                    if (i == 0 && j < BOARD_SIZE) printf("%2c", j + 65);
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
    #ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    #endif

    const char* patternNames[] = { "─┼", " D", "OL", "B1", " 1", "B2", " 2", "2A", "2B", "B3", " 3", "3A", "B4", " 4", " 5", " P" };
    for (int i = BOARD_SIZE + 1; i >= 0; i--) {
        for (int j = 0; j < BOARD_SIZE + 2; j++) {
            if (cells[i][j].getPiece() != EMPTY) {
                if (cells[i][j].getPiece() == BLACK) {
                    cout << "⚫"; 
                } else if (cells[i][j].getPiece() == WHITE) {
                    cout << "⚪"; 
                } else {
                    if (i == 0 && j < BOARD_SIZE) 
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
        cout << pieceName[p] << " " << directionName[k];
        printPatternCells(cells, p, static_cast<Direction>(k));
        cout << "---------------------------------------" << endl;
        Sleep(1000);
    }
}

void printPath(MoveList path) {
    for (const auto& pos : path) {
        cout << (char)(pos.getY() + 96) << pos.getX();
    }
    cout << endl;
}

string convertPath2String(MoveList path) {
    string str = "";
    for (const auto& pos : path) {
        str += (char)(pos.getY() + 96);
        str += to_string(pos.getX());
    }
    return str;
}

void printPos(Pos pos) {
    cout << (char)(pos.getY() + 96) << pos.getX();
}