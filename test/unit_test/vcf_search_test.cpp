#include "../test.h"
#include "../util.h"
#include "../../search/vcf_search.h"

class VCFSearchTest : public TestBase {

private:
    void vcfTest(string process) {
        Board board = getBoard(process);
        VCFSearch vcfSearcher(board);

        TEST_PRINT("[Single-thread vcf search]");
        printBoard(board);
        TEST_PRINT("");

        TEST_TIME_START();
        bool result = vcfSearcher.findVCF();
        TEST_TIME_END("Single-thread vcf search");

        if (!result) {
            TEST_PRINT("Single-thread: There is no VCF");
            TEST_PRINT("==================================");
            return;
        }

        MoveList resultPath = vcfSearcher.getVCFPath();
        int depth = resultPath.size() - board.getPath().size();
        TEST_PRINT("Single-thread: VCF Found. Depth: " << depth);
        printPath(resultPath);
        TEST_PRINT("==================================");
    }

    void vcfMultiThreadTest(string process) {
        Board boardMulti = getBoard(process);
        VCFSearch vcfSearcherMulti(boardMulti);

        TEST_PRINT("[Multi-thread vcf search]");
        printBoard(boardMulti);

        TEST_TIME_START();
        bool multiResult = vcfSearcherMulti.findVCFMultiThreaded();
        TEST_TIME_END("Multi-thread vcf search");

        if (!multiResult) {
            TEST_PRINT("Multi-thread: There is no VCF");
            TEST_PRINT("==================================");
            return;
        } 
        
        MoveList resultPath = vcfSearcherMulti.getVCFPath();
        int depth = resultPath.size() - boardMulti.getPath().size();
        TEST_PRINT("Multi-thread: VCF Found. Depth: " << depth);
        printPath(resultPath);
        TEST_PRINT("==================================");
    }

public:
    VCFSearchTest() {
        registerTestMethod([this]() { findExistBlackVCFs(); });
        registerTestMethod([this]() { findExistWhiteVCFs(); });
        registerTestMethod([this]() { findMultiThreadedVCFs(); });
    }
 
    void findExistBlackVCFs() {
        const string processArr[] = {
            "h8h9i8g8i10j9i9i7j10k11h10k10j8k7g10f10g11f12g7f6f7f11", 
            "h8h9i8g8i10i9h11g12j9i11j11k10h7i7", 
            "h8h9h10g8f9i9j9g10i8k10g9f7e6f8g6f5f6h6e9d8e8e7e11f10c9d9d10b8c11e12d11f11d13d7"
        };

        for (auto process : processArr) {
            vcfTest(process);
        }
    }

    void findExistWhiteVCFs() {
        const string processArr[] = {
            "h8g9h9g10h10h6i8i9g11i10f13h12d10e11d9e9d8b10c9f8e8i13e7e13i6",
            "h8h9i9g10e12f11g8g9i10",
            "h8g9i9f9c9e9g8g7i10g10e10f11e12"
        };

        for (auto process : processArr) {
            vcfTest(process);
        }
    }

    void findMultiThreadedVCFs() {
        const string processArr[] = {
            // findExistBlackVCFs()에서 사용한 processArr
            "h8h9i8g8i10j9i9i7j10k11h10k10j8k7g10f10g11f12g7f6f7f11", 
            "h8h9i8g8i10i9h11g12j9i11j11k10h7i7", 
            "h8h9h10g8f9i9j9g10i8k10g9f7e6f8g6f5f6h6e9d8e8e7e11f10c9d9d10b8c11e12d11f11d13d7", 
            // findExistWhiteVCFs()에서 사용한 processArr
            "h8g9h9g10h10h6i8i9g11i10f13h12d10e11d9e9d8b10c9f8e8i13e7e13i6",
            "h8h9i9g10e12f11g8g9i10",
            "h8g9i9f9c9e9g8g7i10g10e10f11e12"
        };

        for (auto process : processArr) {
            vcfMultiThreadTest(process);
        }
    }

};

int main() {
    VCFSearchTest vcfSearchTest;
    vcfSearchTest.runAllTests();

    return 0;
}

/*

Before Adding CV

Test #	Depth(Single)	Time<(Single)	Depth(Multi)	Time(Multi)	    Depth Diff(S−M)	    Time Diff(S−M)

1       7	            0.00436	        5	            0.01644	        7−5 = +2	        0.00436−0.01644 = -0.01208
2       7	            0.00144	        9	            0.00179	        7−9 = -2	        0.00144−0.00179 = -0.00035
3       19	            0.03860	        19	            0.05150	        19−19 = 0	        0.03860−0.05150 = -0.01290
4       32	            0.01730	        30	            0.14600	        32−30 = +2	        0.01730−0.14600 = -0.12870
5       2	            0.00327	        2	            0.00508	        2−2 = 0	            0.00327−0.00508 = -0.00181
6       4	            0.00552	        4	            0.01200	        4−4 = 0	            0.00552−0.01200 = -0.00648

*/

/*

After Adding CV

Test #	Depth(Single)	Time(Single)	Depth(Multi)	Time(Multi)	    Depth Diff(S−M)	    Time Diff(S−M)

1	    7	            0.00710425	    5	            0.0488032	    7−5 = +2	        0.00710425−0.0488032 = -0.04170
2	    7	            0.00182125	    9	            0.00167146	    7−9 = -2	        0.00182125−0.00167146 = +0.00015
3	    19	            0.0410194	    19	            0.044769	    19−19 = 0	        0.0410194−0.044769 = -0.00375
4	    32	            0.017292	    30	            0.125576	    32−30 = +2	        0.017292−0.125576 = -0.10828
5	    2	            0.00337196	    2	            0.00475183	    2−2 = 0	            0.00337196−0.00475183 = -0.00138
6	    4	            0.00583446	    4	            0.0106141	    4−4 = 0	            0.00583446−0.0106141 = -0.00478

*/