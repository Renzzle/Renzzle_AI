#include "test.h"
#include "util.h"
#include "../search/vcf_search.h"

using namespace std;

class VCFSearchTest : public TestBase {

public:
    VCFSearchTest() {
        registerTestMethod([this]() {  });
    }

};

int main() {
    Board board;
    // board = getBoard("h8h9i8g8i10j9i9i7j10k11h10k10j8k7g10f10g11f12g7f6f7f11");
    // board = getBoard("h8h9i8g8i10i9h11g12j9i11j11k10h7i7");
    board = getBoard("h8g9h9g10h10h6i8i9g11i10f13h12d10e11d9e9d8b10c9f8e8i13e7e13i6");
    // board = getBoard("h8h9h10g8f9i9j9g10i8k10g9f7e6f8g6f5f6h6e9d8e8e7e11f10c9d9d10b8c11e12d11f11d13d7");
    // board = getBoard("h8h9i9g10e12f11g8g9i10");
    // board = getBoard("h8g9i9f9c9e9g8g7i10g10e10f11e12");
    printBoard(board);
    cout << endl;

    TEST_TIME_START();
    VCFSearch vcfSearcher(board);
    vector<Pos> winningPath = vcfSearcher.getVCFPath();
    printPath(winningPath);
    TEST_TIME_END("find vcf");

    return 0;
}