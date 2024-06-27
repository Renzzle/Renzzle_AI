#include <iostream>
#include <vector>
#include <limits>
#include <algorithm> // for std::max and std::min

using namespace std;
using Value = int; 
using Depth = int; 

constexpr Value INF = numeric_limits<Value>::max();

using Moves = vector<pair<int, int>>;

using EvalFunc = Value(*)();
using MoveFunc = Moves(*)();
using NextFunc = void(*)(int, int);
using UndoFunc = void(*)(int, int);

Value alphaBeta(Depth depth, Value alpha, Value beta, bool maximizingPlayer,
                EvalFunc eval, MoveFunc move, NextFunc next, 
                UndoFunc undo);

// Tree node structure
struct Node {
    Value value;
    vector<Node*> children;
};

// Global variables for the tree and current position
Node* root;
Node* currentNode;
vector<Node*> pathStack; // to track the path for undo operation

// Evaluation function: return the value of the current node
Value eval() {
    return currentNode->value;
}

// Move function: return the possible moves (indices of children)
Moves move() {
    Moves moves;
    for (int i = 0; i < currentNode->children.size(); ++i) {
        moves.push_back({i, 0});
    }
    return moves;
}

// Next function: move to the specified child
void next(int childIndex, int) {
    pathStack.push_back(currentNode);
    currentNode = currentNode->children[childIndex];
}

// Undo function: move back to the parent node
void undo(int, int) {
    currentNode = pathStack.back();
    pathStack.pop_back();
}

// Function to print the tree structure
void printTree(Node* node, int depth = 0) {
    for (int i = 0; i < depth; ++i) {
        cout << "  ";
    }
    cout << node->value << endl;
    for (Node* child : node->children) {
        printTree(child, depth + 1);
    }
}

// Function to print the tree structure with current path highlighted
void printTreeWithCurrentPath(Node* node, int depth = 0, vector<Node*> highlightPath = {}) {
    if (node == nullptr) return;

    for (int i = 0; i < depth; ++i) {
        cout << "  ";
    }

    if (!highlightPath.empty() && node == highlightPath.front()) {
        cout << "[" << node->value << "]" << endl;
        highlightPath.erase(highlightPath.begin());
    } else {
        cout << node->value << endl;
    }

    for (Node* child : node->children) {
        printTreeWithCurrentPath(child, depth + 1, highlightPath);
    }
}

// Function to get the current path in the tree
vector<Node*> getCurrentPath() {
    vector<Node*> path = pathStack;
    path.push_back(currentNode);
    return path;
}

// Function to print the current path in the tree
void printCurrentPath() {
    for (Node* node : pathStack) {
        cout << node->value << " -> ";
    }
    cout << currentNode->value << endl;
}

Value alphaBeta(Depth depth, Value alpha, Value beta, bool maximizingPlayer,
                EvalFunc eval, MoveFunc move, NextFunc next, 
                UndoFunc undo) {
    if (depth == 0 || move().empty()) {
        return eval();
    }

    Moves moves = move();

    if (maximizingPlayer) {
        Value maxEval = -INF;
        for (const auto &m : moves) {
            next(m.first, m.second);
            cout << "Maximizing: Depth: " << depth << " Path: ";
            printCurrentPath();
            cout << "Alpha: " << alpha << " Beta: " << beta << endl;
            printTreeWithCurrentPath(root, 0, getCurrentPath());
            Value evalValue = alphaBeta(depth - 1, alpha, beta, false, eval, move, next, undo);
            undo(m.first, m.second);
            maxEval = max(maxEval, evalValue);
            alpha = max(alpha, maxEval); // Update alpha
            cout << "Updated Alpha: " << alpha << endl;
            if (alpha >= beta) {
                cout << "Pruning at node " << currentNode->value << " with beta: " << beta << endl;
                break;
            }
            cout << endl;
        }
        return maxEval;
    } else {
        Value minEval = INF;
        for (const auto &m : moves) {
            next(m.first, m.second);
            cout << "Minimizing: Depth: " << depth << " Path: ";
            printCurrentPath();
            cout << "Alpha: " << alpha << " Beta: " << beta << endl;
            printTreeWithCurrentPath(root, 0, getCurrentPath());
            Value evalValue = alphaBeta(depth - 1, alpha, beta, true, eval, move, next, undo);
            undo(m.first, m.second);
            minEval = min(minEval, evalValue);
            beta = min(beta, minEval); // Update beta
            cout << "Updated Beta: " << beta << endl;
            if (beta <= alpha) {
                cout << "Pruning at node " << currentNode->value << " with alpha: " << alpha << endl;
                break;
            }
            cout << endl;
        }
        return minEval;
    }
}

// int main() {
//     // Create a sample tree
//     Node n1 = {3, {}};
//     Node n2 = {5, {}};
//     Node n3 = {6, {}};
//     Node n4 = {9, {}};
//     Node n5 = {1, {}};
//     Node n6 = {2, {}};
//     Node n7 = {0, {}};
//     Node n8 = {7, {}};
//     Node n9 = {4, {}};

//     Node n10 = {0, {&n1, &n2, &n3}};
//     Node n11 = {0, {&n4, &n5}};
//     Node n12 = {0, {&n6, &n7, &n8, &n9}};

//     Node rootNode = {0, {&n10, &n11, &n12}};

//     // Set global root and currentNode
//     root = &rootNode;
//     currentNode = root;

//     // Print the tree structure
//     cout << "Tree structure:" << endl;
//     printTree(root);

//     // Perform alpha-beta pruning
//     Value result = alphaBeta(3, -INF, INF, true, eval, move, next, undo);

//     cout << "\nFinal Tree structure with evaluated path:" << endl;
//     printTreeWithCurrentPath(root, 0, getCurrentPath());

//     cout << "Best value: " << result << endl;

//     return 0;
// }