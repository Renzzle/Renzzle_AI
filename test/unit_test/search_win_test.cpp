#include "../test.h"
#include "../util.h"
#include "../../search/search_win.h"
#include <cmath>

class VCFSearchTest : public TestBase {

private:
    void vcfTest(string process) {
        Board board = getBoard(process);
        SearchMonitor monitor;
        SearchWin vcfSearcher(board, monitor);

        // do not use previous search data
        vcfSearcher.treeManager.cleanCache();

        // print status every 10sec
        static double lastTriggerTime = 0.0;
        monitor.setTrigger([](SearchMonitor& monitor) {
            if (monitor.getElapsedTime() - lastTriggerTime >= 10.0) {
                lastTriggerTime = monitor.getElapsedTime();
                return true;
            }
            return false;
        });

        monitor.setSearchListener([](SearchMonitor& monitor) {
            TEST_PRINT("Time: " << monitor.getElapsedTime() << "sec, Node: " << monitor.getVisitCnt());
            printPath(monitor.getBestPath());
        });

        TEST_PRINT("==================================");
        printBoard(board);
        TEST_PRINT("");

        TEST_TIME_START();
        bool result = vcfSearcher.findVCF();
        TEST_TIME_END("vcf search");

        if(!result) {
            TEST_PRINT("There is no VCF");
            return;
        }

        MoveList resultPath = monitor.getBestPath();
        int depth = resultPath.size() - board.getPath().size();
        size_t node = monitor.getVisitCnt();
        TEST_PRINT("Find VCF. Depth: " << depth << ", Node: " << node);
        printPath(resultPath);
        TEST_PRINT("==================================");
    }

    void vctTest(string process) {
        Board board = getBoard(process);
        SearchMonitor monitor;
        SearchWin vctSearcher(board, monitor);

        // do not use previous search data
        vctSearcher.treeManager.cleanCache();

        // print status every 10sec
        static double lastTriggerTime = 0.0;
        monitor.setTrigger([](SearchMonitor& monitor) {
            if (monitor.getElapsedTime() - lastTriggerTime >= 10.0) {
                lastTriggerTime = monitor.getElapsedTime();
                return true;
            }
            return false;
        });

        monitor.setSearchListener([](SearchMonitor& monitor) {
            TEST_PRINT("Time: " << monitor.getElapsedTime() << "sec, Node: " << monitor.getVisitCnt());
            printPath(monitor.getBestPath());
        });

        TEST_PRINT("==================================");
        printBoard(board);
        TEST_PRINT("");

        TEST_TIME_START();
        bool result = vctSearcher.findVCT();
        TEST_TIME_END("vct search");

        if(!result) {
            TEST_PRINT("There is no VCT");
            return;
        }

        MoveList resultPath = monitor.getBestPath();
        int depth = resultPath.size() - board.getPath().size();
        size_t node = monitor.getVisitCnt();
        TEST_PRINT("\033[31mFind WIN. Depth: " << depth << ", Node: " << node << "\033[0m");
        printPath(resultPath);
        TEST_PRINT("==================================");
    }

public:
    VCFSearchTest() {
        //registerTestMethod([this]() { findExistBlackVCFs(); });
        //registerTestMethod([this]() { findExistWhiteVCFs(); });
        registerTestMethod([this]() { findExistVCTs(); });
    }
 
    void findExistBlackVCFs() {
        const string processArr[] = {
            "h8h9i8g8i10j9i9i7j10k11h10k10j8k7g10f10g11f12g7f6f7f11",
            "h8h9i8g8i10i9h11g12j9i11j11k10h7i7",
            "h8h9h10g8f9i9j9g10i8k10g9f7e6f8g6f5f6h6e9d8e8e7e11f10c9d9d10b8c11e12d11f11d13d7",
            "h8j8h15o12o14o10m9n9o6n8o5o4m4n4m5o2k1i2i3f4f3f8g7e8e11g14g11j11k13d15d14c15b15a12a14b9a10a8b12a6a5c6b5a2d4b1g1l8e5f6c5d5e3f2b6a7g3h3c3d3b2a1b4b3e1d2e4e2e7e6c4a4b8b7d6a3c7a9f7d7i7h7g9f10k5j6l5n5i5j5e9d8i6i4i9i8k11j10f9h9c9d9d10f12c11a13f11d11i11h11j12h10i13l10g15h14g8g10i10i12g5g6j4h2g2g4f5h5c1c2c10c8e10b10c13c12e13e12f1d1h4j2k3h6d12f14b14a15e14c14i1h1m1l2k2k4l1j1m3m2o1n1l3j3n3o3",
            "h8j8h15o12o14o10m9n9o6n8o5o4m4n4m5o2k1i2i3f4f3f8g7e8e11g14g11j11k13d15d14c15b15a12a14b9a10a8b12a6a5c6b5a2d4b1g1l8e5f6c5d5e3f2b6a7g3h3c3d3b2a1b4b3e1d2e4e2e7e6c4a4b8b7d6a3c7a9f7d7i7h7g9f10k5j6l5n5i5j5e9d8i6i4i9i8k11j10f9h9c9d9d10f12c11a13f11d11i11h11j12h10i13l10g15h14g8g10i10i12g5g6j4h2g2g4f5h5c1c2c10c8e10b10c13c12e13e12f1d1h4j2k3h6d12f14b14a15e14c14i1h1m1l2k2k4l1j1m3m2o1n1l3j3n3o3l4n2b13b11f13d13h13g13e15a11l7l6j7k6i15f15k15j15k14k12l14m15n14m14j14h12g12i14l12m11l13j13m13o15n13o13",
            "h8j8h15o12o14o10m9n9o6n8o5o4m4n4m5o2k1i2i3f4f3f8g7e8e11g14g11j11k13d15d14c15b15a12a14b9a10a8b12a6a5c6b5a2d4b1g1l8e5f6c5d5e3f2b6a7g3h3c3d3b2a1b4b3e1d2e4e2e7e6c4a4b8b7d6a3c7a9f7d7i7h7g9f10k5j6l5n5i5j5e9d8i6i4i9i8k11j10f9h9c9d9d10f12c11a13f11d11i11h11j12h10i13l10g15h14g8g10i10i12g5g6j4h2g2g4f5h5c1c2c10c8e10b10c13c12e13e12f1d1h4j2k3h6d12f14b14a15e14c14i1h1m1l2k2k4l1j1m3m2o1n1l3j3n3o3l4n2b13b11f13d13h13g13e15a11l7l6j7k6i15f15k15j15k14k12l14m15n14m14"
        };

        for (auto process : processArr) {
            vcfTest(process);
        }
    }

    void findExistWhiteVCFs() {
        const string processArr[] = {
            "h8g9h9g10h10h6i8i9g11i10f13h12d10e11d9e9d8b10c9f8e8i13e7e13i6",
            "h8h9i9g10e12f11g8g9i10",
            "h8g9i9f9c9e9g8g7i10g10e10f11e12",
        };

        for (auto process : processArr) {
            vcfTest(process);
        }
    }

    void findExistVCTs() {
        const string processArr[] = {
            "h8h9i8g8i10i9j9h7k8l7j8l8j10j11",
            //"h8i9i7g9j7k7j8i8j9j10h6k9j6j5g5f4k6l6",
            //"h8i9i7g9j7k7j8i8j9j10h6g5h5h7",
            //"h8i9i7g9j7k7j8i8j9j10",
            "h8i9i7g9j7k7j8h6i8k8",
            "h8h9i8j8f8g8g7j11i10"
            //"h8h9i7g9g8f8i9i8h10j8g10j9e10i10h7f9e9k8l7j10j7k7"
        };

        for (auto process : processArr) {
            vctTest(process);
            Sleep(1000);
        }
    }

};

int main() {
    VCFSearchTest vcfSearchTest;
    vcfSearchTest.runAllTests();

    return 0;
}