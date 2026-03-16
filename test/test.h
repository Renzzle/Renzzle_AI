#pragma once

#define TEST

#ifdef TEST

#include "profiler.h"
#include <chrono>
#include <ctime>
#include <functional>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <streambuf>
#include <string>
#include <vector>

#ifdef _WIN32
#include <direct.h>
#include <process.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#define PROFILE_FUNCTION() SimpleProfiler::Timer timer##__LINE__(__func__)
#define PROFILE_SCOPE(name_str) SimpleProfiler::Timer timer_scope_##name_str(#name_str)

#define PRIVATE public:
#define PUBLIC public:

#define TEST_PRINT(msg) std::cout << msg << std::endl

#define TEST_TIME_START() \
    auto start_time = std::chrono::high_resolution_clock::now();

#define TEST_TIME_END(message) \
    auto end_time = std::chrono::high_resolution_clock::now(); \
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time); \
    double seconds = duration.count() / 1e9; \
    std::cout << message << " is taken " << seconds << " sec" << std::endl;

#define TEST_ASSERT(expr) TestBase::assertState((expr), #expr, __FILE__, __LINE__)

#define TEST_STOP() \
    std::cout << "press enter" << std::endl; \
    std::cin.get();

#else

#define PRIVATE private:
#define PUBLIC public:

#define TEST_PRINT(msg)
#define TEST_TIME_START()
#define TEST_TIME_END(message)
#define TEST_ASSERT(expression)\

#endif

using namespace std;

class TeeStreamBuf : public std::streambuf {
private:
    std::streambuf* first;
    std::streambuf* second;

public:
    TeeStreamBuf(std::streambuf* first, std::streambuf* second)
        : first(first), second(second) {}

protected:
    int overflow(int ch) override {
        if (ch == EOF) return !EOF;
        const int firstResult = first->sputc(static_cast<char>(ch));
        const int secondResult = second->sputc(static_cast<char>(ch));
        if (firstResult == EOF || secondResult == EOF) return EOF;
        return ch;
    }

    int sync() override {
        const int firstSync = first->pubsync();
        const int secondSync = second->pubsync();
        return (firstSync == 0 && secondSync == 0) ? 0 : -1;
    }
};

class TestLogger {
private:
    ofstream logFile;
    string logPath;
    std::streambuf* oldCoutBuf = nullptr;
    std::streambuf* oldCerrBuf = nullptr;
    unique_ptr<TeeStreamBuf> coutTee;
    unique_ptr<TeeStreamBuf> cerrTee;
    bool initialized = false;

    static void createDir(const char* path) {
#ifdef _WIN32
        _mkdir(path);
#else
        mkdir(path, 0755);
#endif
    }

    static string buildLogPath() {
        time_t now = time(nullptr);
        tm localTime;
#ifdef _WIN32
        localtime_s(&localTime, &now);
        const int pid = _getpid();
#else
        localtime_r(&now, &localTime);
        const int pid = getpid();
#endif

        char timeBuf[32];
        strftime(timeBuf, sizeof(timeBuf), "%Y%m%d_%H%M%S", &localTime);

        ostringstream oss;
        oss << "test/logs/test_" << timeBuf << "_" << pid << ".log";
        return oss.str();
    }

public:
    static TestLogger& instance() {
        static TestLogger logger;
        return logger;
    }

    void init() {
        if (initialized) return;

        createDir("test");
        createDir("test/logs");

        logPath = buildLogPath();
        logFile.open(logPath.c_str(), ios::out | ios::app);

        if (!logFile.is_open()) {
            logPath = "test_log_fallback.log";
            logFile.open(logPath.c_str(), ios::out | ios::app);
        }

        oldCoutBuf = std::cout.rdbuf();
        oldCerrBuf = std::cerr.rdbuf();
        coutTee.reset(new TeeStreamBuf(oldCoutBuf, logFile.rdbuf()));
        cerrTee.reset(new TeeStreamBuf(oldCerrBuf, logFile.rdbuf()));

        std::cout.rdbuf(coutTee.get());
        std::cerr.rdbuf(cerrTee.get());
        initialized = true;
    }

    const string& getLogPath() const {
        return logPath;
    }

    ~TestLogger() {
        if (!initialized) return;
        std::cout.flush();
        std::cerr.flush();
        std::cout.rdbuf(oldCoutBuf);
        std::cerr.rdbuf(oldCerrBuf);
        logFile.flush();
        logFile.close();
    }
};

class TestBase {
public:
    virtual ~TestBase() = default;
    using TestMethod = function<void()>;
    
    struct TestCase {
        string name;
        TestMethod method;
    };

private:
    static size_t& totalMethodsRun() {
        static size_t value = 0;
        return value;
    }

    static size_t& totalMethodsFailed() {
        static size_t value = 0;
        return value;
    }

    static size_t& totalAssertions() {
        static size_t value = 0;
        return value;
    }

    static size_t& totalAssertionFailures() {
        static size_t value = 0;
        return value;
    }

    static size_t& totalUnhandledExceptions() {
        static size_t value = 0;
        return value;
    }

    static bool& sessionInitialized() {
        static bool initialized = false;
        return initialized;
    }

    static void initSessionIfNeeded() {
        if (sessionInitialized()) return;

        TestLogger::instance().init();
        sessionInitialized() = true;

        time_t now = time(nullptr);
        tm localTime;
#ifdef _WIN32
        localtime_s(&localTime, &now);
#else
        localtime_r(&now, &localTime);
#endif
        char timeBuf[32];
        strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", &localTime);

        cout << "[TEST_SESSION] started at " << timeBuf << endl;
        cout << "[TEST_SESSION] log file: " << TestLogger::instance().getLogPath() << endl;
    }

    static void printSummary() {
        cout << "[TEST_SUMMARY] methods run: " << totalMethodsRun()
             << ", failed: " << totalMethodsFailed()
             << ", assertions: " << totalAssertions()
             << ", assertion failures: " << totalAssertionFailures()
             << ", unhandled exceptions: " << totalUnhandledExceptions() << endl;
        cout << "[TEST_SUMMARY] log file: " << TestLogger::instance().getLogPath() << endl;
    }

public:
    static void assertState(bool state, const string& expression, const char* file, int line) {
        initSessionIfNeeded();
        totalAssertions()++;

        if (!state) {
            totalAssertionFailures()++;
            cerr << "\033[31mAssertion Error: "
                 << expression
                 << ", File: " << file
                 << ", Line: " << line
                 << "\033[0m" << endl;
        }
    }

    void registerTestMethod(const TestMethod& method) {
        const size_t testNum = testMethods.size() + 1;
        testMethods.push_back({"test_" + to_string(testNum), method});
    }

    void registerTestMethod(const string& name, const TestMethod& method) {
        testMethods.push_back({name, method});
    }

    void runAllTests() {
        initSessionIfNeeded();

        const size_t total = testMethods.size();
        for (size_t i = 0; i < total; ++i) {
            const auto& test = testMethods[i];
            const size_t failuresBefore = totalAssertionFailures();
            bool methodFailed = false;

            cout << "[RUN] (" << (i + 1) << "/" << total << ") " << test.name << endl;

            try {
                test.method();
            } catch (const exception& e) {
                methodFailed = true;
                totalUnhandledExceptions()++;
                cerr << "\033[31mUnhandled Exception in " << test.name << ": "
                     << e.what() << "\033[0m" << endl;
            } catch (...) {
                methodFailed = true;
                totalUnhandledExceptions()++;
                cerr << "\033[31mUnhandled Non-std Exception in "
                     << test.name << "\033[0m" << endl;
            }

            if (totalAssertionFailures() > failuresBefore) {
                methodFailed = true;
            }

            totalMethodsRun()++;
            if (methodFailed) {
                totalMethodsFailed()++;
                cout << "[FAIL] " << test.name << endl;
            } else {
                cout << "[PASS] " << test.name << endl;
            }
        }

        printSummary();
    }

protected:
    vector<TestCase> testMethods;
};