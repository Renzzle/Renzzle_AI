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

#define TEST_ASSERT(expr) TestBase::assertState((expr), #expr, __FILE__, __LINE__)

#else

#define PRIVATE private:
#define PUBLIC public:

#define TEST_PRINT(msg)
#define TEST_TIME_START()
#define TEST_TIME_END(message)
#define TEST_ASSERT(expression)\

#endif

#include <iostream>
#include <vector>
#include <functional>
#include <stdexcept>

using namespace std;

class TestBase {
public:
    virtual ~TestBase() = default;
    using TestMethod = function<void()>;

    static void assertState(bool state, const string& expression, const char* file, int line) {
        try {
            if (!state) {
                throw runtime_error(expression + 
                        ", File: " + string(file) + 
                        ", Line: " + to_string(line));
            } 
        } catch (const exception& e) {
            cerr << "\033[31mAssertion Error: " << e.what() << "\033[0m" << endl;
        }
    }

    void registerTestMethod(const TestMethod& method) {
        testMethods.push_back(method);
        return;
    }

    void runAllTests() {
        for (const auto& method : testMethods) {
            method();
        }
        return;
    }

protected:
    vector<TestMethod> testMethods;
};