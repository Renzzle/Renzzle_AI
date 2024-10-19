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
        // registerTestMethod([this]() { rootNodeInitializationTest(); });
        // registerTestMethod([this]() { moveMethodTest(); });
        // registerTestMethod([this]() { generateKeyTest(); });
        // registerTestMethod([this]() { undoMethodTest(); });
        // registerTestMethod([this]() { boardStateTest(); });
        // registerTestMethod([this]() { movePathTest(); });
        // registerTestMethod([this]() { sequentialMovePathTest(); });
        // registerTestMethod([this]() { testGenKey(); });

        registerTestMethod([this]() { zobristHashUpdateTest(); });
        registerTestMethod([this]() { zobristHashConsistencyTest(); });
        registerTestMethod([this]() { zobristHashAfterUndoTest(); });
        // registerTestMethod([this]() { zobristHashCollisionTest(); });
    }

    void rootNodeInitializationTest() {
        Board initialBoard;
        
        TreeManager treeManager(initialBoard);

        Board& board = treeManager.getBoard();

        shared_ptr<Node> rootNode = treeManager.getNode(board);

        assert(rootNode != nullptr);
        assert(rootNode->move.getX() == 0 && rootNode->move.getY() == 0);
        assert(rootNode->depth == 0);
        assert(rootNode->val == 0);

        for (int i = 1; i <= BOARD_SIZE; i++) 
            for (int j = 1; j <= BOARD_SIZE; j++) 
                assert(rootNode->board.getCell(Pos(i, j)).getPiece() == board.getCell(Pos(i, j)).getPiece());
    }

    void moveMethodTest() {
        // Set up the board state: "h8h9i8i9j8j9k8k9"
        Board initialBoard = getBoard("h8h9i8i9j8j9k8k9");

        TreeManager treeManager(initialBoard);
        
        // position l8 with a black stone
        Pos movePos(12, 8);
        treeManager.move(movePos);
        
        // Retrieve the current node
        shared_ptr<Node> currentNode = treeManager.getNode(treeManager.getBoard());

        // Verify the state of the newly created node
        assert(currentNode != nullptr);
        
        // 1. Check if the board state is correctly updated
        assert(currentNode->board.getCell(movePos).getPiece() == BLACK);

        // 2. Check if the movePos is correctly added to the path
        assert(!currentNode->board.getPath().empty());
        assert(currentNode->board.getPath().back().getX() == movePos.getX() && currentNode->board.getPath().back().getY() == movePos.getY());

        // 3. Verify that the depth is correctly incremented
        assert(currentNode->depth == 1); // Initial depth was 0, so it should be 1 now
    }

    void generateKeyTest() {
        // same board status in two different steps
        Board board1 = getBoard("k8k9j8j9i8h9h8i9l8");
        Board board2 = getBoard("h8h9i8i9j8j9k8k9l8");

        Tree tree;

        // generate two different key
        size_t key1 = tree.generateKey(board1);
        size_t key2 = tree.generateKey(board2);

        assert(key1 == key2);
    }

    void undoMethodTest() {
        // "h8i9g7i7f6i8e5i6"
        Board initialBoard = getBoard("h8i9g7i7f6i8e5i6");
        TreeManager treeManager(initialBoard);

        // move d4
        Pos d4Move(4, 4); //
        treeManager.move(Pos(4, 4));

        // "h8i7g7i9f6i8e5"
        Board board1 = getBoard("h8i7g7i9f6i8e5");
        treeManager = TreeManager(board1);

        // move i6
        treeManager.move(Pos(9, 6));

        // Merge a Tree to h8i9g7i7f6i8e5i6
        shared_ptr<Node> currentNode = treeManager.getNode(treeManager.getBoard());
        assert(currentNode != nullptr);
        assert(currentNode->board.getCell(Pos(9, 6)).getPiece() == WHITE);
        assert(currentNode->board.getPath().back().getX() == Pos(9, 6).getX() && currentNode->board.getPath().back().getY() == Pos(9, 6).getY());

        // move c3 , undo twice
        treeManager.move(Pos(3, 3));
        treeManager.undo(); // c3 undo
        treeManager.undo(); // i6 undo

        // board == h8i7g7i9f6i8e5

        currentNode = treeManager.getNode(treeManager.getBoard());
        assert(currentNode != nullptr);

        // h8i7g7i9f6i8e5
        Board expectedBoard = getBoard("h8i7g7i9f6i8e5");
        for (int i = 1; i <= BOARD_SIZE; i++) {
            for (int j = 1; j <= BOARD_SIZE; j++) {
                assert(currentNode->board.getCell(Pos(i, j)).getPiece() == expectedBoard.getCell(Pos(i, j)).getPiece());
            }
        }
    }

    void boardStateTest() {
        Board initialBoard = getBoard("h8g9i9f9c9e9g8g7i10g10e10f11e12");
        TreeManager treeManager(initialBoard);

        shared_ptr<Node> currentNode = treeManager.getNode(treeManager.getBoard());
        assert(currentNode != nullptr);

        vector<pair<Pos, Piece>> expectedPieces = {
            {Pos(8, 8), BLACK},  // h8
            {Pos(7, 9), WHITE},  // g9
            {Pos(9, 9), BLACK},  // i9
            {Pos(6, 9), WHITE},  // f9
            {Pos(3, 9), BLACK},  // c9
            {Pos(5, 9), WHITE},  // e9
            {Pos(7, 8), BLACK},  // g8
            {Pos(7, 7), WHITE},  // g7
            {Pos(9, 10), BLACK}, // i10
            {Pos(7, 10), WHITE}, // g10
            {Pos(5, 10), BLACK}, // e10
            {Pos(6, 11), WHITE}, // f11
            {Pos(5, 12), BLACK}  // e12
        };

        for (const auto& pieceInfo : expectedPieces) {
            Pos pos = pieceInfo.first;
            Piece piece = pieceInfo.second;
            assert(currentNode->board.getCell(pos).getPiece() == piece);
        }

        assert(currentNode->depth == 0);
        assert(currentNode->val == 0);
    }

    void movePathTest() {
        // Initialize the board with the given moves: "h8g9i9f9c9e9g8g7i10g10e10f11e12"
        Board initialBoard = getBoard("h8g9i9f9c9e9g8g7i10g10e10f11e12");
        TreeManager treeManager(initialBoard);

        // Place a stone at h7
        treeManager.move(Pos(8, 7));

        // Place a stone at i6
        treeManager.move(Pos(9, 6));

        // Place a stone at j5
        treeManager.move(Pos(10, 5));

        // Retrieve the current node
        shared_ptr<Node> currentNode = treeManager.getNode(treeManager.getBoard());
        assert(currentNode != nullptr);

        // Verify that the path is correctly stored
        assert(currentNode->board.getPath().size() == 16);

        // First move: h7
        assert(currentNode->board.getPath()[13].getX() == 8 && currentNode->board.getPath()[13].getY() == 7);

        // Second move: i6
        assert(currentNode->board.getPath()[14].getX() == 9 && currentNode->board.getPath()[14].getY() == 6);

        // Third move: j5
        assert(currentNode->board.getPath()[15].getX() == 10 && currentNode->board.getPath()[15].getY() == 5);

        cout << "Move path test passed!" << endl;
    }

    void sequentialMovePathTest() {
        cout << "Sequential move path test passed!" << endl;

        // Initialize an empty board
        Board initialBoard;
        TreeManager treeManager(initialBoard);

        // Perform a series of moves: h8, i8, h7, i7, h6, i6, h5, i5
        treeManager.move(Pos(8, 8)); // h8
        treeManager.move(Pos(9, 8)); // i8
        treeManager.move(Pos(8, 7)); // h7
        treeManager.move(Pos(9, 7)); // i7
        treeManager.move(Pos(8, 6)); // h6
        treeManager.move(Pos(9, 6)); // i6
        treeManager.move(Pos(8, 5)); // h5
        treeManager.move(Pos(9, 5)); // i5

        // Retrieve the current node
        shared_ptr<Node> currentNode = treeManager.getNode(treeManager.getBoard());
        assert(currentNode != nullptr);

        // Verify that the path is correctly stored
        TEST_PRINT("currentNode->board.getPath().size() : " << currentNode->board.getPath().size());
        assert(currentNode->board.getPath().size() == 8);

        // Check each move in the path 
        assert(currentNode->board.getPath()[0].getX() == 8 && currentNode->board.getPath()[0].getY() == 8); // h8
        assert(currentNode->board.getPath()[1].getX() == 9 && currentNode->board.getPath()[1].getY() == 8); // i8
        assert(currentNode->board.getPath()[2].getX() == 8 && currentNode->board.getPath()[2].getY() == 7); // h7
        assert(currentNode->board.getPath()[3].getX() == 9 && currentNode->board.getPath()[3].getY() == 7); // i7
        assert(currentNode->board.getPath()[4].getX() == 8 && currentNode->board.getPath()[4].getY() == 6); // h6
        assert(currentNode->board.getPath()[5].getX() == 9 && currentNode->board.getPath()[5].getY() == 6); // i6
        assert(currentNode->board.getPath()[6].getX() == 8 && currentNode->board.getPath()[6].getY() == 5); // h5
        assert(currentNode->board.getPath()[7].getX() == 9 && currentNode->board.getPath()[7].getY() == 5); // i5
    }

    void testGenKey(){
        Board board = getBoard("a14a15c15d14h9e4e9c12j4k7f10b5m11i5g11c7e12g8k11g9c4d11f7k10k4f4e10d5f12i7i11i4c2k8d12g2d8m12g7g13g6j13j8l13h5i14e5e14l11l5k9f3l10f5e7k5i10c5c13i2j9m2h14a10a4b12b7d13f8k13n5m8j3f9b4a8d15d10c10g14l9o12j2k6m3a7j15d7e13g1j7l2c9n12n15o8n13o2n6m1m15j11n10n7n2o6");
        Tree tree;  
        TEST_TIME_START();
        for (int i = 0; i < 10000; i++) {
            tree.generateKey(board);
        }
        TEST_TIME_END("genKey(x10000)");
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