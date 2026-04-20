#include "../test.h"
#include "../util.h"
#include "../../search/vcf_candidate_finder.h"
#include <algorithm>
#include <iomanip>
#include <sstream>

class VCFCandidateFinderTest : public TestBase {

PRIVATE
    string posToString(Pos pos) {
        if (pos.isDefault()) {
            return "(none)";
        }

        string text;
        text += static_cast<char>(pos.getY() + 96);
        text += to_string(pos.getX());
        return text;
    }

    string movesToString(const MoveList& moves) {
        if (moves.empty()) {
            return "(none)";
        }

        ostringstream oss;
        for (size_t i = 0; i < moves.size(); ++i) {
            if (i != 0) {
                oss << ", ";
            }
            oss << posToString(moves[i]);
        }
        return oss.str();
    }

    void printProbeResults(const VCFCandidateFinder& finder) {
        for (const VCFCandidateProbeResult& probe : finder.getLastProbeResults()) {
            TEST_PRINT("  " << posToString(probe.move)
                << " | createsVCF=" << (probe.createsVCF ? "Y" : "N")
                << " | stopped=" << (probe.stopped ? "Y" : "N")
                << " | nodes=" << probe.nodeCount
                << " | time=" << fixed << setprecision(3) << (probe.elapsedTime * 1000.0) << " ms"
                << " | vcf=" << convertPath2String(probe.vcfPath));
        }
    }

    double elapsedSeconds(std::chrono::high_resolution_clock::time_point start,
        std::chrono::high_resolution_clock::time_point end) {
        const auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        return duration.count() / 1e9;
    }

    size_t sumProbeNodes(const VCFCandidateFinder& finder) {
        size_t total = 0;
        for (const VCFCandidateProbeResult& probe : finder.getLastProbeResults()) {
            total += probe.nodeCount;
        }
        return total;
    }

    void knownVCFPositionsProduceCandidateTest() {
        const string processArr[] = {
            "h8h9i8i9g9g8j9i7j8j10h5h7"
        };

        VCFCandidateFinderOptions options;
        options.perMoveTimeLimitSeconds = 1.0;

        for (const string& process : processArr) {
            Board board = getBoard(process);

            const auto initStart = std::chrono::high_resolution_clock::now();
            VCFCandidateFinder finder(board, options);
            const auto initEnd = std::chrono::high_resolution_clock::now();
            const double ttInitSeconds = elapsedSeconds(initStart, initEnd);

            TEST_PRINT("process: " << process);
            printBoard(board);
            TEST_PRINT("");

            const auto searchStart = std::chrono::high_resolution_clock::now();
            MoveList creators = finder.findFromEvaluatorCandidates();
            const auto searchEnd = std::chrono::high_resolution_clock::now();
            const double candidateSearchSeconds = elapsedSeconds(searchStart, searchEnd);
            const double ttMemoryMB = finder.getEstimatedMemoryBytes() / (1024.0 * 1024.0);

            TEST_PRINT("VCF creators: " << movesToString(creators));
            TEST_PRINT("TT init time: "
                << fixed << setprecision(3) << (ttInitSeconds * 1000.0) << " ms"
                << " | TT memory: " << ttMemoryMB << " MB");
            TEST_PRINT("Candidate-only VCF search time: "
                << fixed << setprecision(3) << (candidateSearchSeconds * 1000.0) << " ms"
                << " | visited nodes: " << sumProbeNodes(finder)
                << " | cached TT entries: " << finder.getCachedNodeCount());
            printProbeResults(finder);

            TEST_ASSERT(!creators.empty());
        }
    }

PUBLIC
    VCFCandidateFinderTest() {
        registerTestMethod("known_vcf_positions_produce_candidate", [this]() { knownVCFPositionsProduceCandidateTest(); });
    }
};

int main() {
    VCFCandidateFinderTest test;
    test.runAllTests();

    return 0;
}
