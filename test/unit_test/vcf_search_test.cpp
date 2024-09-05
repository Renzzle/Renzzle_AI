#include "../test.h"
#include "../util.h"
#include "../../search/vcf_search.h"

class VCFSearchTest : public TestBase {

private:
    void printVCFPath(const vector<Pos>& path, Board& board) {
        for (size_t i = 0; i < path.size(); ++i) {
            const Pos& p = path[i];

            string player = (i % 2 == 0) ? "BLACK" : "WHITE";
            TEST_PRINT(i + 1 << ": (" << (char)(p.getY() + 96) << ", " << p.getX() << ") - " << player);
        }
    }

    void printSimulatedVCFPath(const vector<Pos>& path, Board& board) {
        size_t originalMoveCount = board.getPath().size();

        for (size_t i = originalMoveCount; i < path.size(); ++i) {
            const Pos& p = path[i];

            string player = (i % 2 == 0) ? "BLACK" : "WHITE";
            TEST_PRINT(i + 1 << ": (" << (char)(p.getY() + 96) << ", " << p.getX() << ") - " << player);
        }
    }

    void vcfTest(string process, bool isExist) {
        Board board = getBoard(process);
        VCFSearch vcfSearcher(board);

        printBoard(board);

        TEST_TIME_START();
        bool result = vcfSearcher.findVCF();
        TEST_TIME_END("vcf search");

        vector<Pos> path = vcfSearcher.getVCFPath();

        TEST_PRINT("printVCFPath");
        printVCFPath(path, board);

        TEST_PRINT("printSimulatedVCFPath");
        printSimulatedVCFPath(path, board);
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
            vcfTest(process, true);
        }
    }

    void findExistWhiteVCFs() {
        const string processArr[] = {
            "h8g9h9g10h10h6i8i9g11i10f13h12d10e11d9e9d8b10c9f8e8i13e7e13i6",
            "h8h9i9g10e12f11g8g9i10",
            "h8g9i9f9c9e9g8g7i10g10e10f11e12"
        };

        for (auto process : processArr) {
            vcfTest(process, true);
        }
    }

};

int main() {
    VCFSearchTest vcfSearchTest;
    vcfSearchTest.runAllTests();

    return 0;
}