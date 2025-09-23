#include "../search/search.h"
#include <thread>
#include <future>
#include <atomic>
#include <condition_variable>
#include <optional>

#define MAX_THINKING_TIME 10.0 // second

SearchMonitor findVCF(Board board, atomic<bool>& stopFlag) {
    SearchMonitor monitor;
    SearchWin vcfSearcher(board, monitor);

    double lastTriggerTime = 0.0;
    monitor.setTrigger([&stopFlag, &lastTriggerTime](SearchMonitor& monitor) {
        if (monitor.getElapsedTime() >= MAX_THINKING_TIME) {
            lastTriggerTime = monitor.getElapsedTime();
            return true;
        }
        return stopFlag.load();
    });
    monitor.setSearchListener([&vcfSearcher](SearchMonitor& monitor) {
        vcfSearcher.stop();
    });

    bool result = vcfSearcher.findVCF();
    stopFlag = result; 

    return monitor;
}

SearchMonitor findVCTorBestMove(Board board, atomic<bool>& stopFlag) {
    SearchMonitor monitor;
    Search vctSearcher(board, monitor);

    double lastTriggerTime = 0.0;
    monitor.setTrigger([&stopFlag, &lastTriggerTime](SearchMonitor& monitor) {
        if (monitor.getElapsedTime() >= MAX_THINKING_TIME) {
            lastTriggerTime = monitor.getElapsedTime();
            return true;
        }
        return stopFlag.load();
    });
    monitor.setSearchListener([&vctSearcher](SearchMonitor& monitor) {
        vctSearcher.stop();
    });

    vctSearcher.ids();
    
    if (monitor.getBestValue().isWin()) stopFlag = true;
    else stopFlag = false;

    return monitor;
}

int validatePuzzle(string boardStr) {
    Board board = getBoard(boardStr);
    Evaluator evaluator(board);

    // if game is already over
    Value value = evaluator.evaluate();
    if (value.isLose()) return -1;
    else if (value.isWin()) return value.getResultDepth();

    atomic<bool> stopFlag(false);
    vector<future<SearchMonitor>> futures;

    // start vcf thread
    future<SearchMonitor> future_vcf = async(launch::async, findVCF, board, ref(stopFlag));
    futures.push_back(move(future_vcf));

    // if there is no opponent mate start vct thread
    if (!evaluator.isOppoMateExist()) {
        future<SearchMonitor> future_vct = async(launch::async, findVCTorBestMove, board, ref(stopFlag));
        futures.push_back(move(future_vct));
    }

    vector<optional<SearchMonitor>> results;

    while (results.size() < futures.size()) {
        bool flag = false;
        for (auto& fut : futures) {
            if (fut.valid() && fut.wait_for(chrono::milliseconds(1)) == future_status::ready) {
                results.push_back(fut.get());
                if (fut.get().getBestValue().isWin()) {
                    flag = true; break;
                }
            }
        }
        if (flag) break;
    }
    
    int depth = -1;
    if (!results.empty()) {
        if (results.back().value().getBestValue().isWin())
            depth = results.back().value().getBestPath().size();
    }

    for (auto& fut : futures) {
        if (fut.valid()) {
            fut.wait();
        }
    }

    return depth;
}

int convertMoveToInt(Pos& move) {
    if (move.isDefault()) return -1;
    int result = (move.getY() - 1) * 15 + move.getX() - 1;
    return result;
}

int findNextMove(string boardStr) {
    Board board = getBoard(boardStr);
    Evaluator evaluator(board);

    // if game is already over
    if (!evaluator.evaluate().isOnGoing()) return -1;

    // if there is sure move
    Pos nextMove = evaluator.getSureMove();
    if (!nextMove.isDefault()) return convertMoveToInt(nextMove);

    atomic<bool> stopFlag(false);
    vector<future<SearchMonitor>> futures;

    // start vcf thread
    future<SearchMonitor> future_vcf = async(launch::async, findVCF, board, ref(stopFlag));
    futures.push_back(move(future_vcf));

    // start search thread when defend or threat exist
    if (evaluator.isOppoMateExist() || !evaluator.getThreats().empty()) {
        future<SearchMonitor> future_vctOrDefend = async(launch::async, findVCTorBestMove, board, ref(stopFlag));
        futures.push_back(move(future_vctOrDefend));
    }

    vector<optional<SearchMonitor>> results;

    while (results.size() < futures.size()) {
        bool flag = false;
        for (auto& fut : futures) {
            if (fut.valid() && fut.wait_for(chrono::milliseconds(1)) == future_status::ready) {
                results.push_back(fut.get());
                if (fut.get().getBestValue().isWin()) {
                    flag = true; break;
                }
            }
        }
        if (flag) break;
    }

    for (auto& fut : futures) {
        if (fut.valid()) {
            fut.wait();
        }
    }

    Value tmp;
    for (auto& result : results) {
        if (result.has_value()) {
            SearchMonitor& resultMonitor = result.value();
            if (tmp < resultMonitor.getBestValue()) {
                tmp = resultMonitor.getBestValue();
                if (!resultMonitor.getBestPath().empty()) {
                    nextMove = result.value().getBestPath()[0];
                }
            }
        }
    }

    if (nextMove.isDefault()) {
        MoveList moves = evaluator.getCandidates();
        if (!moves.empty()) nextMove = moves[0];
    }

    return convertMoveToInt(nextMove);
}

int main() {
    
}