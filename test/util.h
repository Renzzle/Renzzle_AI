#pragma once

#include <iostream>
#include <chrono>
#include <cassert>
#include <vector>
#include <string>


#define DEBUG 1

#if DEBUG

#define TEST_PRINT(msg) std::cout << msg << std::endl

#define TEST_TIME_START() \
    auto start_time = std::chrono::high_resolution_clock::now();

#define TEST_TIME_END(message) \
    auto end_time = std::chrono::high_resolution_clock::now(); \
    std::chrono::duration<double> duration = (end_time - start_time); \
    std::cout << message << " is taken " << duration.count() << " sec" << std::endl;

#define TEST_ASSERT(expression) assert(expression)

#else

#define TEST_PRINT(msg)
#define TEST_TIME_START()
#define TEST_TIME_END(message)
#define TEST_ASSERT(expression)

#endif

std::vector<std::pair<int, int>> processString(const std::string& input) {
    std::vector<std::pair<int, int>> result;
    for (size_t i = 0; i < input.length(); i += 2) {
        char letter = input[i];
        int number;
        if (isdigit(input[i + 1]) && isdigit(input[i + 2])) {
            number = (input[i + 1] - '0') * 10 + (input[i + 2] - '0');
            i++; 
        } else {
            number = input[i + 1] - '0';
        }

        int letterValue = letter - 'a' + 1;

        result.emplace_back(letterValue, number);
    }
    return result;
}