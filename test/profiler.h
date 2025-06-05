#pragma once

#include <chrono>
#include <iostream>
#include <string>
#include <map>
#include <iomanip>

namespace SimpleProfiler {

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
        accumulatedTime[functionName] += duration.count() / 1e9;
    }

private:
    std::string functionName;
    std::chrono::time_point<std::chrono::high_resolution_clock> start_time;
    
};

void printResults() {
    std::cout << "\n--- Profiling Results ---" << std::endl;
    std::cout << std::fixed << std::setprecision(9);
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
                  << std::endl;                                            // us(microSec) = 1 / 1,000,000 s
    }
    std::cout << "-------------------------" << std::endl;
}

void reset() {
    accumulatedTime.clear();
    callCounts.clear();
}

}

#define PROFILE_FUNCTION() SimpleProfiler::Timer timer##__LINE__(__func__)
#define PROFILE_SCOPE(name_str) SimpleProfiler::Timer timer_scope_##name_str(#name_str)