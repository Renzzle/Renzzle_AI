#pragma once

#include <vector>
#include <functional>

class TestBase {
public:
    virtual ~TestBase() = default;
    using TestMethod = std::function<void()>;

    void registerTestMethod(const TestMethod& method) {
        testMethods.push_back(method);
    }

    void runAllTests() {
        for (const auto& method : testMethods) {
            method();
        }
    }

protected:
    std::vector<TestMethod> testMethods;
};