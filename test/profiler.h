#pragma once

#include <chrono>
#include <iostream>
#include <string>
#include <map>
#include <iomanip> // PrintResults에서 std::fixed, std::setprecision 사용 시

namespace SimpleProfiler {

// 각 함수별 누적 시간과 호출 횟수를 저장
static std::map<std::string, double> accumulatedTime;
static std::map<std::string, long long> callCounts;

class Timer {
public:
    Timer(const std::string& funcName) : functionName(funcName), start_time(std::chrono::high_resolution_clock::now()) {
        callCounts[functionName]++;
    }

    ~Timer() {
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
        accumulatedTime[functionName] += duration.count() / 1e9; // 초 단위로 저장
    }

private:
    std::string functionName;
    std::chrono::time_point<std::chrono::high_resolution_clock> start_time;
};

// 프로파일링 결과 출력 함수
void PrintResults() {
    std::cout << "\n--- Profiling Results ---" << std::endl;
    std::cout << std::fixed << std::setprecision(9); // 전체 출력에 정밀도 적용
    for (const auto& pair : accumulatedTime) {
        const std::string& funcName = pair.first;
        double totalTime = pair.second;
        long long count = callCounts[funcName];
        double avgTime = (count > 0) ? (totalTime / count) : 0;
        double avgTimeMicroseconds = avgTime * 1e6;
        std::cout << "Function: " << funcName
                  << " | Total Time: " << totalTime << " s"
                  << " | Call Count: " << count
                  << " | Avg Time: " << std::fixed << std::setprecision(3) << avgTimeMicroseconds << " us"
                  << std::endl;
    }
    std::cout << "-------------------------" << std::endl;
}

// 프로파일링 데이터 초기화 함수
void Reset() {
    accumulatedTime.clear();
    callCounts.clear();
}

} // namespace SimpleProfiler

// 매크로를 사용하여 함수 시작 시 타이머 객체 생성
#define PROFILE_FUNCTION() SimpleProfiler::Timer timer##__LINE__(__func__)
// 특정 코드 블록 프로파일링을 위한 매크로 (이름을 직접 지정)
#define PROFILE_SCOPE(name_str) SimpleProfiler::Timer timer_scope_##name_str(#name_str)