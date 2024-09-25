#pragma once

#include "../game/board.h" 
#include "node.h"
#include <unordered_map>
#include <vector>
#include <functional>
#include <algorithm>
#include <memory>
#include <random>
#include <limits>


#define NUM_PIECE_TYPES 4

using namespace std;
using Value = int;

// hash specialization for Pos
namespace std {
    template <>
    struct hash<Pos> {
        size_t operator()(const Pos &pos) const {
            return hash<int>()(pos.getX()) ^ (hash<int>()(pos.getY()) << 1);
        }
    };
}

class Tree {

private:
    unordered_map<size_t, shared_ptr<Node>> nodeMap; // Hash map to store nodes with board state keys.

    static size_t zobristTable[BOARD_SIZE + 2][BOARD_SIZE + 2][NUM_PIECE_TYPES];
    static bool zobristInitialized;

    static void initializeZobristTable();

public:
    Tree();

    // Method to generate the key for the hash map.
    size_t generateKey(Board& board);

    // Method to add a node to the hash map.
    void addNode(shared_ptr<Node> node);

    // Method to retrieve a node from the hash map using a board state.
    shared_ptr<Node> getNode(Board& board);

    // Method to create a new node.
    shared_ptr<Node> createNode(Board board, Pos move, Value score, int depth);

};

size_t Tree::zobristTable[BOARD_SIZE + 2][BOARD_SIZE + 2][NUM_PIECE_TYPES];
bool Tree::zobristInitialized = false;

Tree::Tree() {
    if (!zobristInitialized) {
        initializeZobristTable();
        zobristInitialized = true;
    }
}

void Tree::initializeZobristTable() {
    random_device rd;
    mt19937_64 rng(rd());
    uniform_int_distribution<size_t> dist(0, numeric_limits<size_t>::max());

    for (int i = 0; i <= BOARD_SIZE + 1; ++i) {
        for (int j = 0; j <= BOARD_SIZE + 1; ++j) {
            for (int k = 0; k < NUM_PIECE_TYPES; ++k) {
                zobristTable[i][j][k] = dist(rng);
            }
        }
    }
}

// Definition of the generateKey method.
size_t Tree::generateKey(Board& board) {
    size_t hashValue = 0;

    auto& cells = board.getBoardStatus();

    for (int i = 0; i <= BOARD_SIZE + 1; ++i) {
        for (int j = 0; j <= BOARD_SIZE + 1; ++j) {
            Piece piece = cells[i][j].getPiece();

            int pieceIndex = 0;

            switch (piece) {
                case EMPTY:
                    pieceIndex = 0;
                    break;
                case BLACK:
                    pieceIndex = 1;
                    break;
                case WHITE:
                    pieceIndex = 2;
                    break;
                case WALL:
                    pieceIndex = 3;
                    break;
                default:
                    pieceIndex = 0;
            }

            if (piece != EMPTY) {
                hashValue ^= zobristTable[i][j][pieceIndex];
            }
        }
    }

    return hashValue;
}

// Definition of the addNode method.
void Tree::addNode(shared_ptr<Node> node) {
    size_t key = generateKey(node->board);

    nodeMap[key] = node;
}

// Definition of the getNode method.
shared_ptr<Node> Tree::getNode(Board& board) {
    size_t key = generateKey(board);
    auto it = nodeMap.find(key);
    if (it != nodeMap.end()) {
        return it->second;
    }
    return nullptr; // or handle the case where the node is not found.
}

// Definition of the createNode method.
shared_ptr<Node> Tree::createNode(Board board, Pos move, Value score, int depth) {
    size_t key = generateKey(board);
    if (nodeMap.find(key) != nodeMap.end()) {
        return nodeMap[key]; // Node already exists, return it
    }
    auto newNode = make_shared<Node>(board, move, score, depth);
    return newNode;
}