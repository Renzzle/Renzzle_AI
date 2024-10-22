#include "../test.h"
#include "../util.h"
#include "../../tree/tree_manager.h"
#include "../../game/board.h"
#include <set>
#include <unordered_set>

using namespace std;

class TreeManagerTest : public TestBase {

private:
    Board getBoard(string moves) {
        vector<pair<int, int>> v = processString(moves);
        Board board;
        for (auto p : v) {
            board.move(Pos(p.first, p.second));
        }
        return board;
    }

public:
    TreeManagerTest() {
        registerTestMethod([this]() { moveMethodTest(); });
        registerTestMethod([this]() { undoMethodTest(); });
        registerTestMethod([this]() { isVisitedMethodTest(); });
        registerTestMethod([this]() { isomorphicTest(); });
        registerTestMethod([this]() { zobristHashUpdateTest(); });
        registerTestMethod([this]() { zobristHashConsistencyTest(); });
        registerTestMethod([this]() { zobristHashAfterUndoTest(); });
        //registerTestMethod([this]() { zobristHashCollisionTest(); }); // take 2 min
    }

    void moveMethodTest() {
        // set up the board state: "h8h9i8i9j8j9k8k9"
        Board initialBoard = getBoard("h8h9i8i9j8j9k8k9");

        TreeManager treeManager(initialBoard);
        
        // move l8 (black turn)
        Pos l8Move(12, 8);
        treeManager.move(l8Move);
        
        // check that the node is added
        TEST_ASSERT(treeManager.tree.exist(treeManager.getBoard()));
        
        // check if the board state is correctly updated
        TEST_ASSERT(treeManager.currentNode->board.getCell(l8Move).getPiece() == BLACK);
    }

    void undoMethodTest() {
        // initialize
        Board initialBoard = getBoard("h8i9g7i7f6i8e5i6");
        TreeManager treeManager(initialBoard);

        // move
        Pos move(4, 4);
        treeManager.move(move);

        // undo x3
        treeManager.undo();
        treeManager.undo();
        treeManager.undo();

        // check same board
        TEST_ASSERT(treeManager.getBoard().getCurrentHash() == initialBoard.getCurrentHash());
    }

    void isVisitedMethodTest() {
        Board initialBoard = getBoard("h8h9i8i9j8j9k8k9");

        TreeManager treeManager(initialBoard);
        Pos l8Move(12, 8);
        treeManager.move(l8Move);
        treeManager.undo();

        TEST_ASSERT(treeManager.isVisited(l8Move));
    }

    void isomorphicTest() {
        // same board status in two different sequence
        Board board1 = getBoard("k8k9j8j9i8h9h8i9l8");
        Board board2 = getBoard("h8h9i8i9j8j9k8k9l8");

        TreeManager treeManager1(board1);
        TreeManager treeManager2(board2);

        // check same address
        TEST_ASSERT(treeManager1.currentNode == treeManager2.currentNode);
    }

    void zobristHashUpdateTest() {
        TEST_PRINT("Starting Zobrist Hash Update Test...");

        Board board;
        size_t initialHash = board.getCurrentHash();
        TEST_PRINT("Initial hash: " << initialHash);

        Pos movePos(8, 8);
        board.move(movePos);
        size_t hashAfterMove = board.getCurrentHash();
        TEST_PRINT("Hash after move at (" << movePos.getX() << ", " << movePos.getY() << "): " << hashAfterMove);

        TEST_ASSERT(initialHash != hashAfterMove);

        size_t expectedHash = initialHash ^ getZobristValue(movePos.getX(), movePos.getY(), BLACK);
        TEST_PRINT("Expected Hash after move: " << expectedHash);

        TEST_ASSERT(hashAfterMove == expectedHash);

        TEST_PRINT("Zobrist Hash Update Test Passed!\n");
    }

    void zobristHashConsistencyTest() {
        TEST_PRINT("Starting Zobrist Hash Consistency Test...");

        Board board1;
        Board board2;

        string moves1 = "h8h9i8i9j8j9k8k9";
        vector<pair<int, int>> v1 = processString(moves1);

        for (auto p : v1) {
            board1.move(Pos(p.first, p.second));
        }

        string moves2 = "j8j9k8k9h8h9i8i9";
        vector<pair<int, int>> v2 = processString(moves2);

        for (auto p : v2) {
            board2.move(Pos(p.first, p.second));
        }

        size_t hash1 = board1.getCurrentHash();
        size_t hash2 = board2.getCurrentHash();

        TEST_PRINT("Hash of board1: " << hash1);
        TEST_PRINT("Hash of board2: " << hash2);

        TEST_ASSERT(hash1 == hash2);

        board1.move(Pos(12, 8));
        size_t hash1After = board1.getCurrentHash();
        TEST_PRINT("Hash of board1 after additional move at (12,8): " << hash1After);

        TEST_ASSERT(hash1After != hash2);

        TEST_PRINT("Zobrist Hash Consistency Test Passed!\n");
    }

    void zobristHashAfterUndoTest() {
        TEST_PRINT("Starting Zobrist Hash After Undo Test...");

        Board board;

        size_t initialHash = board.getCurrentHash();
        TEST_PRINT("Initial Hash: " << initialHash);

        vector<Pos> moves = {Pos(8, 8), Pos(9, 8), Pos(8, 7)};
        vector<size_t> hashHistory = {initialHash};

        for (auto move : moves) {
            board.move(move);
            hashHistory.push_back(board.getCurrentHash());
            TEST_PRINT("Moved to (" << move.getX() << ", " << move.getY() << "), Hash: " << board.getCurrentHash());
        }

        for (int i = moves.size() - 1; i >= 0; --i) {
            board.undo();
            size_t currentHash = board.getCurrentHash();
            TEST_PRINT("After undo, Hash: " << currentHash << ", Expected Hash: " << hashHistory[i]);
            TEST_ASSERT(currentHash == hashHistory[i]);
        }

        size_t finalHash = board.getCurrentHash();
        TEST_PRINT("Final Hash after all undos: " << finalHash);

        TEST_ASSERT(finalHash == initialHash);

        TEST_PRINT("Zobrist Hash After Undo Test Passed!\n");
    }

    void zobristHashCollisionTest() {
        TEST_PRINT("Starting Zobrist Hash Collision Test...");

        const size_t NUM_POSITIONS = 10000;
        unordered_set<size_t> hashSet;
        unordered_set<string> boardSet;
        size_t collisionCount = 0;

        TEST_PRINT("Generating and hashing " << NUM_POSITIONS << " unique board positions...");

        TEST_TIME_START();

        random_device rd;
        mt19937 gen(rd());
        uniform_int_distribution<> moveDist(1, BOARD_SIZE);

        size_t generatedBoards = 0;

        while (generatedBoards < NUM_POSITIONS) {
            Board board;
            size_t numMoves = (generatedBoards % (BOARD_SIZE * BOARD_SIZE / 2)) + 1;

            set<pair<int, int>> occupiedPositions;
            vector<pair<int, int>> moves;

            for (size_t moveNum = 0; moveNum < numMoves; ++moveNum) {
                int x, y;
                do {
                    x = moveDist(gen);
                    y = moveDist(gen);
                } while (occupiedPositions.find({x, y}) != occupiedPositions.end());

                occupiedPositions.insert({x, y});
                moves.push_back({x, y});
                board.move(Pos(x, y));
            }

            string boardKey;
            for (int i = 1; i <= BOARD_SIZE; ++i) {
                for (int j = 1; j <= BOARD_SIZE; ++j) {
                    Piece piece = board.getCell(Pos(i, j)).getPiece();
                    boardKey += to_string(static_cast<int>(piece)) + ",";
                }
            }

            if (boardSet.find(boardKey) != boardSet.end()) {
                continue;
            } else {
                boardSet.insert(boardKey);
                generatedBoards++;

                size_t hashValue = board.getCurrentHash();

                if (hashSet.find(hashValue) != hashSet.end()) {
                    collisionCount++;
                } else {
                    hashSet.insert(hashValue);
                }

                if (generatedBoards % 1000 == 0) {
                    TEST_PRINT("Processed " << generatedBoards << " unique positions...");
                }
            }
        }

        TEST_TIME_END("Hashing and collision detection");

        TEST_PRINT("Total unique positions hashed: " << NUM_POSITIONS);
        TEST_PRINT("Total collisions found: " << collisionCount);
        TEST_PRINT("Unique hashes: " << hashSet.size());

        TEST_PRINT("Zobrist Hash Collision Test Completed.\n");
    }

};

int main() {
    TreeManagerTest treeManagerTest;
    treeManagerTest.runAllTests();

    return 0;
}