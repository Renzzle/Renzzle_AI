#pragma once

#define TEST

#ifdef TEST
#define PRIVATE public:
#endif
#define PUBLIC public:

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