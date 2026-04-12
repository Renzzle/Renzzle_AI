#include "../util.h"
#include "../test.h"
#include "../../search/search.h"
#include <algorithm>
#include <array>
#include <iomanip>
#include <limits>
#include <numeric>

class SearchTest : public TestBase {

PRIVATE
    struct CandidateShape {
        bool attackerTurn = false;
        bool hasSureMove = false;
        bool oppoMateExist = false;
        size_t rawCandidateCount = 0;
        size_t uniqueCandidateCount = 0;
        size_t threatCount = 0;
        size_t defendCount = 0;
        size_t fourCount = 0;
    };

    int posKey(const Pos& pos) {
        return (pos.getX() << 4) | pos.getY();
    }

    MoveList makeUniqueMoves(const MoveList& moves) {
        MoveList unique;
        std::array<uint8_t, 256> seen = {};
        unique.reserve(moves.size());

        for (const Pos& move : moves) {
            if (move.isDefault()) {
                continue;
            }

            const int key = posKey(move);
            if (seen[key]) {
                continue;
            }

            seen[key] = 1;
            unique.push_back(move);
        }

        return unique;
    }

    string posToString(Pos pos) {
        if (pos.isDefault()) {
            return "(none)";
        }

        string text;
        text += static_cast<char>(pos.getY() + 96);
        text += to_string(pos.getX());
        return text;
    }

    string valueToString(Value value) {
        ostringstream oss;

        if (value.isWin()) {
            oss << "WIN(" << value.getResultDepth() << ")";
        } else if (value.isLose()) {
            oss << "LOSE(" << value.getResultDepth() << ")";
        } else if (value.getType() == Value::Type::UNKNOWN) {
            oss << "UNKNOWN(" << value.getValue() << ")";
        } else {
            oss << value.getValue();
        }

        oss << " ";
        switch (value.getType()) {
            case Value::Type::EXACT:       oss << "[EXACT]"; break;
            case Value::Type::LOWER_BOUND: oss << "[LOWER]"; break;
            case Value::Type::UPPER_BOUND: oss << "[UPPER]"; break;
            case Value::Type::UNKNOWN:     oss << "[UNKNOWN]"; break;
        }

        return oss.str();
    }

    string formatMilliseconds(double seconds) {
        ostringstream oss;
        oss << fixed << setprecision(3) << (seconds * 1000.0) << " ms";
        return oss.str();
    }

    string formatDouble(double value, int precision = 2) {
        ostringstream oss;
        oss << fixed << setprecision(precision) << value;
        return oss.str();
    }

    string previewMoves(const MoveList& moves, size_t limit) {
        if (moves.empty()) {
            return "(none)";
        }

        ostringstream oss;
        const size_t count = min(limit, moves.size());
        for (size_t i = 0; i < count; ++i) {
            if (i != 0) {
                oss << ", ";
            }
            oss << posToString(moves[i]);
        }

        if (moves.size() > limit) {
            oss << ", ... +" << (moves.size() - limit);
        }

        return oss.str();
    }

    MoveList collectSearchLikeCandidates(bool rootBlackTurn, Board& probeBoard, CandidateShape* shape = nullptr) {
        Evaluator evaluator(probeBoard);
        const Pos sureMove = evaluator.getSureMove();
        const MoveList threats = evaluator.getThreats();
        const MoveList defendMoves = evaluator.getThreatDefend();
        const MoveList fours = evaluator.getFours();
        const bool oppoMateExist = evaluator.isOppoMateExist();
        const bool attackerTurn = (probeBoard.isBlackTurn() == rootBlackTurn);

        MoveList moves;
        if (!sureMove.isDefault()) {
            moves.push_back(sureMove);
        } else if (oppoMateExist) {
            if (attackerTurn) {
                moves = threats;
            } else {
                moves = defendMoves;
                moves.insert(moves.end(), fours.begin(), fours.end());
            }
        } else if (attackerTurn) {
            moves = threats;
        }

        if (shape != nullptr) {
            shape->attackerTurn = attackerTurn;
            shape->hasSureMove = !sureMove.isDefault();
            shape->oppoMateExist = oppoMateExist;
            shape->rawCandidateCount = moves.size();
            shape->uniqueCandidateCount = makeUniqueMoves(moves).size();
            shape->threatCount = threats.size();
            shape->defendCount = defendMoves.size();
            shape->fourCount = fours.size();
        }

        return moves;
    }

    void printPVShapeProfile(const Board& rootBoard, const MoveList& pv, size_t maxPly, size_t previewMoveCount) {
        if (pv.empty()) {
            TEST_PRINT("  (no pv)");
            return;
        }

        Board probeBoard(rootBoard);
        const bool rootBlackTurn = probeBoard.isBlackTurn();
        const size_t nodeCount = min(maxPly, pv.size() + 1);
        for (size_t ply = 0; ply < nodeCount; ++ply) {
            CandidateShape shape;
            MoveList candidates = makeUniqueMoves(collectSearchLikeCandidates(rootBlackTurn, probeBoard, &shape));

            TEST_PRINT("  ply " << ply
                << " | role=" << (shape.attackerTurn ? "ATTACK" : "DEFEND")
                << " | raw=" << shape.rawCandidateCount
                << " | unique=" << shape.uniqueCandidateCount
                << " | sure=" << (shape.hasSureMove ? "Y" : "N")
                << " | oppoMate=" << (shape.oppoMateExist ? "Y" : "N")
                << " | threats=" << shape.threatCount
                << " | defend=" << shape.defendCount
                << " | fours=" << shape.fourCount
                << " | preview=" << previewMoves(candidates, previewMoveCount));

            if (ply >= pv.size()) {
                break;
            }
            if (!probeBoard.move(pv[ply])) {
                TEST_PRINT("  ply " << ply << " -> invalid PV move: " << posToString(pv[ply]));
                break;
            }
        }
    }

    void printRootShapeProfile(const Board& rootBoard, const vector<Search::RootMoveStat>& stats,
        size_t moveLimit, size_t defensePreviewCount) {
        if (stats.empty()) {
            TEST_PRINT("  (no root stats)");
            return;
        }

        Board rootProbe(rootBoard);
        const bool rootBlackTurn = rootProbe.isBlackTurn();
        const size_t count = min(moveLimit, stats.size());
        for (size_t i = 0; i < count; ++i) {
            const Search::RootMoveStat& stat = stats[i];
            Board attackBoard(rootBoard);
            if (!attackBoard.move(stat.move)) {
                TEST_PRINT("  " << posToString(stat.move) << " | invalid root move");
                continue;
            }

            CandidateShape defenseShape;
            MoveList defenseMoves = makeUniqueMoves(collectSearchLikeCandidates(rootBlackTurn, attackBoard, &defenseShape));

            int nextMin = std::numeric_limits<int>::max();
            int nextMax = 0;
            double nextSum = 0.0;
            int nextSureCount = 0;
            int nextEscalationCount = 0;
            vector<string> previews;

            for (const Pos& defense : defenseMoves) {
                Board replyBoard(attackBoard);
                if (!replyBoard.move(defense)) {
                    continue;
                }

                CandidateShape nextAttackShape;
                MoveList nextAttackMoves = makeUniqueMoves(collectSearchLikeCandidates(rootBlackTurn, replyBoard, &nextAttackShape));
                const int nextWidth = static_cast<int>(nextAttackShape.uniqueCandidateCount);

                nextMin = min(nextMin, nextWidth);
                nextMax = max(nextMax, nextWidth);
                nextSum += nextWidth;
                if (nextAttackShape.hasSureMove) {
                    nextSureCount++;
                }
                if (nextAttackShape.fourCount > 0) {
                    nextEscalationCount++;
                }

                if (previews.size() < defensePreviewCount) {
                    ostringstream oss;
                    oss << posToString(defense)
                        << "->" << nextWidth
                        << (nextAttackShape.hasSureMove ? " (sure)" : "")
                        << (nextAttackShape.fourCount > 0 ? " [4s]" : "");
                    previews.push_back(oss.str());
                }
            }

            const size_t defenseCount = defenseMoves.size();
            const double nextAvg = defenseCount == 0 ? 0.0 : nextSum / static_cast<double>(defenseCount);
            if (defenseCount == 0) {
                nextMin = 0;
            }

            TEST_PRINT("  " << posToString(stat.move)
                << " | value=" << valueToString(stat.value)
                << " | defense raw/unique=" << defenseShape.rawCandidateCount << "/" << defenseShape.uniqueCandidateCount
                << " | defense(threat/defend/four)=" << defenseShape.threatCount << "/"
                << defenseShape.defendCount << "/" << defenseShape.fourCount
                << " | nextAttack min/avg/max=" << nextMin << "/" << formatDouble(nextAvg, 2) << "/" << nextMax
                << " | nextSure=" << nextSureCount << "/" << defenseCount
                << " | next4+=" << nextEscalationCount << "/" << defenseCount);

            if (!previews.empty()) {
                TEST_PRINT("      sample: " << [&]() {
                    ostringstream oss;
                    for (size_t j = 0; j < previews.size(); ++j) {
                        if (j != 0) {
                            oss << ", ";
                        }
                        oss << previews[j];
                    }
                    return oss.str();
                }());
            }
        }
    }

    void printRootStats(const vector<Search::RootMoveStat>& stats, size_t limit) {
        if (stats.empty()) {
            TEST_PRINT("  (no root stats)");
            return;
        }

        const size_t count = min(limit, stats.size());
        for (size_t i = 0; i < count; ++i) {
            const Search::RootMoveStat& stat = stats[i];
            TEST_PRINT("  [" << stat.order << "] " << posToString(stat.move)
                << " | nodes=" << stat.nodeCount
                << " | time=" << formatMilliseconds(stat.elapsedTime)
                << " | value=" << valueToString(stat.value)
                << " | ttBest=" << (stat.wasTTBest ? "Y" : "N")
                << " | research=" << (stat.wasResearched ? "Y" : "N")
                << " | cutoff=" << (stat.causedCutoff ? "Y" : "N"));
            if (!stat.pv.empty()) {
                TEST_PRINT("      pv: " << convertPath2String(stat.pv));
            }
        }

        if (stats.size() > limit) {
            TEST_PRINT("  ... +" << (stats.size() - limit) << " more root candidates");
        }
    }

    void searchTest(string process) {
        Board board = getBoard(process);
        SearchMonitor monitor;
        Search searcher(board, monitor);
        //searcher.setLazyThreadCount(8);

        monitor.setTrigger([](SearchMonitor& monitor) {
            static int depth = 0;
            const int curDepth = monitor.getDepth();
            if (depth != curDepth) {
                depth = curDepth;
                return true;
            }
            return false;
        });

        monitor.setSearchListener([&searcher, this](SearchMonitor& monitor) {
            const double memMB = searcher.getEstimatedMemoryBytes() / (1024.0 * 1024.0);
            TEST_PRINT("Depth: " << monitor.getDepth() << ", Time: " << formatMilliseconds(monitor.getElapsedTime()) <<
            ", Node: " << monitor.getVisitCnt() << ", TT entries: " << searcher.getNodeCount() <<
            ", Mem: " << memMB << " MB, Value: " << monitor.getBestValue().getValue());
            printPath(monitor.getBestPath());
        });

        printBoard(board);

        TEST_TIME_START();
        searcher.ids();
        TEST_TIME_END("alpha-beta search");
        TEST_PRINT("Final visited node: " << monitor.getVisitCnt());
        TEST_PRINT("Final TT entries: " << searcher.getNodeCount() << 
            ", Mem: " << (searcher.getEstimatedMemoryBytes() / (1024.0 * 1024.0)) << " MB");
        printPath(monitor.getBestLine(0));
    }

    void deepAnalysisTest(const string& process, int maxDepth, size_t rootPreviewCount) {
        Board board = getBoard(process);
        SearchMonitor monitor;
        Search searcher(board, monitor);

        TEST_PRINT("=================================");
        TEST_PRINT("[DEEP] process: " << process);
        printBoard(board);
        TEST_PRINT("");

        searcher.state.isRunning = true;
        searcher.state.bestPath.clear();
        searcher.state.bestValue = Value();
        searcher.clearHistory();
        monitor.initStartTime();
        searcher.tt.clear();

        int completedDepth = 0;

        for (int depth = 5; depth <= maxDepth; depth += 2) {
            searcher.tt.nextGeneration();

            const size_t nodesBefore = monitor.getVisitCnt();
            const double timeBefore = monitor.getElapsedTime();
            MoveList pv;
            Value result = searcher.searchRootWithAspiration(depth, &pv);

            if (!searcher.state.isRunning) {
                break;
            }

            searcher.state.bestValue = result;
            searcher.state.bestPath = pv;
            monitor.setBestPath(pv);
            monitor.updateElapsedTime();
            completedDepth = depth;

            TEST_PRINT("[DEEP][depth " << depth << "] totalNodes=" << monitor.getVisitCnt()
                << ", deltaNodes=" << (monitor.getVisitCnt() - nodesBefore)
                << ", totalTime=" << formatMilliseconds(monitor.getElapsedTime())
                << ", deltaTime=" << formatMilliseconds(monitor.getElapsedTime() - timeBefore)
                << ", ttEntries=" << searcher.getNodeCount()
                << ", ttHitRate=" << (searcher.tt.getHitRate() * 100.0) << "%"
                << ", value=" << valueToString(result));
            TEST_PRINT("[DEEP][depth " << depth << "] best pv: " << convertPath2String(pv));
            TEST_PRINT("[DEEP][depth " << depth << "] root profile (final aspiration pass)");
            printRootStats(searcher.getLastRootStats(), rootPreviewCount);
            TEST_PRINT("[DEEP][depth " << depth << "] pv corridor shape");
            printPVShapeProfile(board, pv, 6, 5);
            TEST_PRINT("[DEEP][depth " << depth << "] root move shape (2-ply residual branching)");
            printRootShapeProfile(board, searcher.getLastRootStats(), min<size_t>(5, rootPreviewCount), 3);

            if (result.isWin() && result.getResultDepth() <= depth) {
                break;
            }
        }

        TEST_ASSERT(completedDepth > 0);
        TEST_PRINT("[DEEP] completed depth: " << completedDepth);
        TEST_PRINT("[DEEP] final visited node: " << monitor.getVisitCnt());
        TEST_PRINT("[DEEP] final TT entries: " << searcher.getNodeCount()
            << ", Mem: " << (searcher.getEstimatedMemoryBytes() / (1024.0 * 1024.0)) << " MB");
        TEST_PRINT("=================================");

        searcher.stop();
    }

PUBLIC
    SearchTest() {
        registerTestMethod("alpha_beta_search_suite", [this]() { testAlphaBetaSearch(); });
        //registerTestMethod("deep_single_case_profile", [this]() { testDeepSingleCaseProfile(); });
    }

    void testAlphaBetaSearch() {
        const string processArr[] = {
            "h8h9i8g8i10i9j9k10j7i7",
            "h8h9i8g8i10i9j9k8k10l11i7j6",
            "h8h9j9g8j10g7i10",
            "h8i9h9h10g9g8f8g7e7f7g6f6i6h6j5j6k7j8k9i10j10i11i12h11g11e8f9",
            "h8i8i9i7j8j7j6k7k6i6h7h6i5g7f8h10i10d8f6",
            "h8h9f6g7e9g8f7h6f8f9h7d6",
            "h8h9g7i9g9i7g10i8i10h10e8f9d8j7",
            "h8h9i8g8i10j8i6h7h6k7j6f7f8",
            "h8h9i9i8g10h10f7g7h6h7g5i7k7j6k6k8l7j9k10",
            "h8h7i7i8j7j8j9i9h9h11i10g7i6f7j6k5i5h4k7l9k8m7l7m6",
            "h8h9i10j9k9i8j7j10j11j12g7g9g6h10i11g11i9",
            "h8i9f6g8g6h9f9f7e6d6i10j9k9j7j8i7i8l8h7f5e7d8"
        };

        for (auto process : processArr) {
            TEST_PRINT("=================================");
            searchTest(process);
        }
    }

    void testDeepSingleCaseProfile() {
        // Replace this string when you want to inspect a different root position in detail.
        const string process = "h8h7i7i8j7j8j9i9h9h11i10g7i6f7j6k5i5h4k7l9k8m7l7m6";
        deepAnalysisTest(process, 17, 15);
    }

};

int main() {
    SearchTest searchTest;
    searchTest.runAllTests();

    return 0;
}
