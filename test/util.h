#pragma once

#include <iostream>
#include <chrono>
#include <cassert>

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

