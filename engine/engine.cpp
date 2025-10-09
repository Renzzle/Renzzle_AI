#include "../search/search.h"
#include <thread>
#include <future>
#include <atomic>
#include <condition_variable>
#include <optional>

#define MAX_THINKING_TIME 10.0 // second

string validatePuzzle(string boardStr) {
    Board board = getBoard(boardStr);
    Evaluator evaluator(board);

    // if game is already over
    Result status = board.getResult();
    if (status != ONGOING) return "";

    SearchMonitor vcfMonitor;
    SearchWin vcfSearcher(board, vcfMonitor);

    double lastTriggerTime = 0.0;
    vcfMonitor.setTrigger([&lastTriggerTime](SearchMonitor& monitor) {
        if (monitor.getElapsedTime() >= MAX_THINKING_TIME) {
            lastTriggerTime = monitor.getElapsedTime();
            return true;
        }
        return false;
    });
    vcfMonitor.setSearchListener([&vcfSearcher](SearchMonitor& monitor) {
        vcfSearcher.stop();
    });

    bool result = vcfSearcher.findVCF();
    if (result) return convertPath2String(vcfMonitor.getBestPath());
    
    SearchMonitor vctMonitor;
    Search vctSearcher(board, vctMonitor);

    lastTriggerTime = 0.0;
    vctMonitor.setTrigger([&lastTriggerTime](SearchMonitor& monitor) {
        if (monitor.getElapsedTime() >= MAX_THINKING_TIME) {
            lastTriggerTime = monitor.getElapsedTime();
            return true;
        }
        return false;
    });
    vctMonitor.setSearchListener([&vctSearcher](SearchMonitor& monitor) {
        vctSearcher.stop();
    });

    vctSearcher.ids();
    if (vctMonitor.getBestValue().isWin())
        return convertPath2String(vctMonitor.getBestPath());

    return "";
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
    Result status = board.getResult();
    if (status != ONGOING) return -1;

    // if there is sure move
    Pos nextMove = evaluator.getSureMove();
    if (!nextMove.isDefault()) return convertMoveToInt(nextMove);

    SearchMonitor vcfMonitor;
    SearchWin vcfSearcher(board, vcfMonitor);

    vcfMonitor.setTrigger([](SearchMonitor& monitor) {
        if (monitor.getElapsedTime() >= MAX_THINKING_TIME) {
            return true;
        }
        return false;
    });
    vcfMonitor.setSearchListener([&vcfSearcher](SearchMonitor& monitor) {
        vcfSearcher.stop();
    });

    bool result = vcfSearcher.findVCF();
    if (result) return convertMoveToInt(vcfMonitor.getBestPath()[0]);
    
    SearchMonitor searchMonitor;
    Search searcher(board, searchMonitor);

    searchMonitor.setTrigger([](SearchMonitor& monitor) {
        if (monitor.getElapsedTime() >= 3) {
            return true;
        }
        return false;
    });
    searchMonitor.setSearchListener([&searcher](SearchMonitor& monitor) {
        searcher.stop();
    });

    searcher.ids();
    
    if (!searchMonitor.getBestPath().empty())
        nextMove = searchMonitor.getBestPath()[0];
    
    if (nextMove.isDefault()) {
        MoveList moves = evaluator.getCandidates();
        if (!moves.empty()) nextMove = moves[0];
    }

    return convertMoveToInt(nextMove);
}

int main() {
    string move = validatePuzzle("h8h9i8g8i10j9i9i7j10k11h10k10j8k7g10f10g11f12g7f6f7f11");
    //int move = findNextMove("h8h9i8i9j8j9g8g9f8");
    TEST_PRINT(move);
}