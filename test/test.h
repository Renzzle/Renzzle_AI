#pragma once

#define TEST

#ifdef TEST

#define PRIVATE public:
#define PUBLIC public:

#define TEST_PRINT(msg) std::cout << msg << std::endl

#define TEST_TIME_START() \
    auto start_time = std::chrono::high_resolution_clock::now();

#define TEST_TIME_END(message) \
    auto end_time = std::chrono::high_resolution_clock::now(); \
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time); \
    double seconds = duration.count() / 1e9; \
    std::cout << message << " is taken " << seconds  << " sec" << std::endl;

#define TEST_ASSERT(expression) assert(expression)

#else

#define PRIVATE private:
#define PUBLIC public:

#define TEST_PRINT(msg)
#define TEST_TIME_START()
#define TEST_TIME_END(message)
#define TEST_ASSERT(expression)\

#endif

#include <vector>
#include <functional>

using namespace std;

class TestBase {
public:
    virtual ~TestBase() = default;
    using TestMethod = function<void()>;

    void registerTestMethod(const TestMethod& method) {
        testMethods.push_back(method);
    }

    void runAllTests() {
        for (const auto& method : testMethods) {
            method();
        }
    }

protected:
    vector<TestMethod> testMethods;
};