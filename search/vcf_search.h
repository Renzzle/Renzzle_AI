#pragma once

#include "../evaluate/evaluator.h"
#include "../tree/tree_manager.h"
#include "../test/test.h"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <future>
#include <mutex>
#include <sstream>
#include <thread>
#include <vector>
#include <queue>

class VCFSearch {

PRIVATE
    TreeManager treeManager;
    Evaluator evaluator;
    Color targetColor;
    MoveList solutionPath;  // 멀티스레드 승리 경로 저장 변수
    bool isWin();
    bool isTargetTurn();

PUBLIC
    VCFSearch(Board& board);
    VCFSearch(Board& board, Color color);
    bool findVCF();                 // 기존 싱글스레드 DFS 탐색 함수
    bool findVCFMultiThreaded();    // 작업 큐 기반 멀티스레드 DFS 탐색 함수
    MoveList getVCFPath();
};

VCFSearch::VCFSearch(Board& board) : treeManager(board) {
    targetColor = board.isBlackTurn() ? COLOR_BLACK : COLOR_WHITE;
}

VCFSearch::VCFSearch(Board& board, Color color) : treeManager(board), targetColor(color) { }

bool VCFSearch::findVCF() {
    if (isWin()) return true;
    
    MoveList moves;
    if (isTargetTurn())
        moves = evaluator.getFours(treeManager.getBoard());
    else 
        moves = evaluator.getCandidates(treeManager.getBoard());
    
    if (moves.empty()) return false;
    
    for (auto move : moves) {
        if (treeManager.isVisited(move))
            continue;
        treeManager.move(move);
        if (findVCF()) return true;
        treeManager.undo();
    }
    
    return false;
}

bool VCFSearch::findVCFMultiThreaded() {    // 작업 큐 기반 멀티스레드 DFS 탐색 함수
    atomic<bool> solved(false);     // 승리 여부 저장 변수
    MoveList finalSolution;         // 최종 승리 경로 저장 변수
    queue<Board> workQueue;         // 탐색할 보드 상태 저장 큐
    mutex queueMutex;               // 작업 큐 보호 뮤텍스
    condition_variable cv;          // 작업 큐 상태 알림 조건변수
    
    {
        lock_guard<mutex> lock(queueMutex);
        workQueue.push(treeManager.getBoard()); // 초기 보드 상태를 작업 큐에 삽입
    }
    
    int numThreads = thread::hardware_concurrency();    // 사용 가능한 스레드 수
    if (numThreads == 0) numThreads = 4;
    
    vector<thread> workers;     // 워커 스레드 저장 벡터
    vector<string> workerLogs;  // 스레드별 로그 저장 벡터
    mutex logMutex;             // 로그 보호 뮤텍스

    auto workerFunc = [&]() {   // 각 스레드가 수행할 함수
        auto tid = this_thread::get_id();  
        auto startWorker = chrono::steady_clock::now();
        int processedNodes = 0;     // 처리한 보드 상태 개수 카운터
        
        while (!solved.load()) {    // 승리 경로를 찾을 때까지 반복
            Board currentBoard;
            {
                unique_lock<mutex> lock(queueMutex);
                cv.wait(lock, [&]() {   // 작업 큐가 비어있지 않거나, 승리 여부가 발생할 때까지 wait
                    return !workQueue.empty() || solved.load();
                });
                if (solved.load()) break;
                currentBoard = workQueue.front();   // 작업 큐로부터 보드 상태 가져옴
                workQueue.pop();
            }
            processedNodes++;       // 처리한 노드 개수 증가
            
            VCFSearch localSearch(currentBoard, targetColor);   // 로컬 DFS 탐색 객체 생성
            if (localSearch.findVCF()) {    // 승리 경로 발견 시, 승리 여부 설정하고, 다른 스레드 깨운 후 종료
                solved.store(true);
                finalSolution = localSearch.getVCFPath();
                cv.notify_all();
                break;
            } else {                        // 승리 경로 미발견 시, 후보에 대한 자식 상태를 생성하여 작업 큐에 추가
                MoveList moves;
                if (localSearch.isTargetTurn())
                    moves = evaluator.getFours(currentBoard);
                else
                    moves = evaluator.getCandidates(currentBoard);
                    
                for (auto move : moves) {
                    Board childBoard = currentBoard;
                    if (childBoard.move(move)) {
                        lock_guard<mutex> lock(queueMutex);
                        workQueue.push(childBoard);
                        cv.notify_one();    // 새 작업 생성 알림
                    }
                }
            }
        }
        
        auto endWorker = chrono::steady_clock::now();
        auto duration = chrono::duration_cast<chrono::nanoseconds>(endWorker - startWorker);
        double seconds = duration.count() / 1e9;
        
        ostringstream oss;
        oss << "[Tid " << tid << "] \tProcessed nodes: " << processedNodes << ", Duration: " << seconds << " sec";
        string logStr = oss.str();
        
        {
            lock_guard<mutex> lock(logMutex);
            workerLogs.push_back(logStr);   // 스레드 로그 동기화
        }
    };
    
    for (int i = 0; i < numThreads; i++)
        workers.emplace_back(workerFunc);   // 스레드 생성
    
    for (auto &th : workers)
        if (th.joinable())
            th.join();      // 모든 스레드 종료 대기
    
    for (const auto &log : workerLogs)
        TEST_PRINT(log);    // 모든 스레드가 종료된 후, 스레드 로그 일괄 출력

    if (solved.load()) {
        solutionPath = finalSolution;
        return true;
    }
    return false;
}

bool VCFSearch::isWin() {
    Result result = treeManager.getBoard().getResult();
    if (result == BLACK_WIN && targetColor == COLOR_BLACK)
        return true;
    if (result == WHITE_WIN && targetColor == COLOR_WHITE)
        return true;
    return false;
}

bool VCFSearch::isTargetTurn() {
    if (treeManager.getBoard().isBlackTurn())
        return (targetColor == COLOR_BLACK);
    else
        return (targetColor == COLOR_WHITE);
}

MoveList VCFSearch::getVCFPath() {
    if (!solutionPath.empty())
        return solutionPath;
    return treeManager.getBoard().getPath();
}