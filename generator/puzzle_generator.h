#pragma once

#include "../search/search.h"
#include "../test/util.h"
#include <random>
#include <thread>

#define SEED_NUMBER 10

class PuzzleGenerator {

private:
    const string seeds[SEED_NUMBER] = {
        "h8h9i8g8i10",
        "h8h9i9i8g10",
        "h8h9h6i9g9",
        "h8h9j8g8i9",
        "h8h9h10i9g9",
        "h8h9i8i9g9",
        "h8i9j7j8h10",
        "h8i9j9i8i7",
        "h8i9i7g9j8",
        "h8i9h7h9g9"
    };
    Board puzzle;
    mutex puzzleMutex;

    void findWin(Board board);

public:
    Board generatePuzzle();

};

void PuzzleGenerator::findWin(Board board) {
    SearchMonitor vcfMonitor;
    SearchWin vcfSearcher(board, vcfMonitor);

    SearchMonitor vctMonitor;
    SearchWin vctSearcher(board, vctMonitor);

    double lastTriggerTime = 0.0;
    vcfMonitor.setTrigger([&lastTriggerTime](SearchMonitor& monitor) {
        if (monitor.getElapsedTime() - lastTriggerTime >= 5.0) {
            return true;
        }
        return false;
    });
    vcfMonitor.setSearchListener([&vcfSearcher](SearchMonitor& monitor) {
        vcfSearcher.stop();
    });
    bool existVCF = vcfSearcher.findVCF();
    if (existVCF) {
        lock_guard<mutex> lock(puzzleMutex);
        puzzle = board;
        return;
    }

    lastTriggerTime = 0.0;
    vctMonitor.setTrigger([&lastTriggerTime](SearchMonitor& monitor) {
        if (monitor.getElapsedTime() - lastTriggerTime >= 10.0) {
            return true;
        }
        return false;
    });
    vctMonitor.setSearchListener([&vctSearcher](SearchMonitor& monitor) {
        vctSearcher.stop();
    });
    bool existVCT = vctSearcher.findVCT();
    if (existVCT) {
        lock_guard<mutex> lock(puzzleMutex);
        puzzle = board;
    }

    return;
}

Board PuzzleGenerator::generatePuzzle() {
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<int> dis(0, SEED_NUMBER - 1);

    int idx = dis(gen);
    Board board = getBoard(seeds[idx]);
    vector<thread> threads;
    
    for (int i = 0; i < 20; i++) {
        Evaluator evaluator(board);
        if (evaluator.evaluate() > 30) {
            threads.emplace_back([this, board]() mutable { findWin(board); }); 
        }
        MoveList moves = evaluator.getCandidates();
        if (moves.empty()) break;
        int n = 0;
        if (moves.size() > 5) n = 4;
        else n = moves.size() - 1;
        uniform_int_distribution<int> tdis(0, n);
        idx = tdis(gen);
        board.move(moves[idx]);
    }

    for (auto& t : threads) {
        if (t.joinable()) t.join();
    }

    if (puzzle.getPath().empty())
        puzzle = getBoard("h8h9i8g8i10i9j9k10h11g12j11i11");
    return puzzle;
}