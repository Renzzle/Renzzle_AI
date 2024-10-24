#include "../test.h"
#include "../util.h"
#include "../../search/vcf_search.h"

class VCFSearchTest : public TestBase {

private:
    void vcfTest(string process) {
        Board board = getBoard(process);
        VCFSearch vcfSearcher(board);

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

        MoveList resultPath = vcfSearcher.getVCFPath();
        int depth = resultPath.size() - board.getPath().size();
        TEST_PRINT("Find VCF. Depth: " << depth);
        printPath(resultPath);
        TEST_PRINT("==================================");
    }


public:
    VCFSearchTest() {
        registerTestMethod([this]() { findExistBlackVCFs(); });
        registerTestMethod([this]() { findExistWhiteVCFs(); });
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
            "h8g9i9f9c9e9g8g7i10g10e10f11e12",
        };

        for (auto process : processArr) {
            vcfTest(process);
        }
    }

};

int main() {
    VCFSearchTest vcfSearchTest;
    vcfSearchTest.runAllTests();

    return 0;
}