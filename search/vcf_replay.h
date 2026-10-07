#pragma once

#include "../game/board.h"
#include "../game/fixed_move_list.h"

// What a replay touched, to bound which defender stones could change it.
struct VCFReplayTrace {
    CandidateList played;                         // attacker moves and forced blocks
    CandidateList finalFives;                     // attacker five points at the win
    std::array<uint8_t, 256> defenderFours = {};  // defender four points at any step
    bool forbiddenBlock = false;                  // won through a forbidden black block
    bool forbiddenSensitive = false;              // a black attacker move was a 3-3 candidate
};

namespace vcf_replay_detail {
    inline uint8_t cellCode(int x, int y) {
        return static_cast<uint8_t>((x << 4) | y);
    }

    // cells where `piece` makes a four or five (black 4-3-3 is classed FORBID_33); newly
    // marked cells are also appended to `added` when given
    inline void markFourPoints(Board& board, Piece piece, std::array<uint8_t, 256>& marks,
        CandidateList* added = nullptr) {
        for (CompositePattern composite : {WINNING, MATE, B4_F3, B4_PLUS, B4_ANY, FORBID_33}) {
            board.getPatternBucket(piece, composite).forEach([&](const Pos& p) {
                uint8_t& mark = marks[cellCode(p.getX(), p.getY())];
                if (!mark && added != nullptr) added->push_back(p);
                mark = 1;
            });
        }
    }
}

// Replays the line's attacker moves (even indices) for the side to move, taking each
// forced block from the board; true if it still wins. Restores the board.
inline bool replayVCFWins(Board& board, const MoveList& line, VCFReplayTrace* trace = nullptr) {
    const Piece attacker = board.isBlackTurn() ? BLACK : WHITE;
    const Piece defender = attacker == BLACK ? WHITE : BLACK;
    const Result attackerWin = attacker == BLACK ? BLACK_WIN : WHITE_WIN;

    if (board.getResult() != ONGOING) {
        return board.getResult() == attackerWin;
    }

    auto recordFives = [&]() {
        if (trace == nullptr) return;
        board.getPatternBucket(attacker, WINNING).forEach([&](const Pos& p) {
            trace->finalFives.push_back(p);
        });
    };

    int played = 0;
    bool wins = false;
    for (size_t i = 0; i < line.size(); i += 2) {
        // trace first: a defender stone here could still be a four or a five
        if (trace != nullptr) {
            vcf_replay_detail::markFourPoints(board, defender, trace->defenderFours);
        }
        if (board.hasCompositePattern(attacker, WINNING)) {
            recordFives();
            wins = true;
            break;
        }
        // a defender five threat outranks the next four
        if (board.hasCompositePattern(defender, WINNING)) {
            break;
        }
        if (trace != nullptr && attacker == BLACK
            && board.getCell(line[i]).getCompositePattern(BLACK) == FORBID_33) {
            trace->forbiddenSensitive = true;
        }
        if (!board.move(line[i])) {
            break;
        }
        ++played;
        if (trace != nullptr) trace->played.push_back(line[i]);
        if (board.getResult() != ONGOING) {
            wins = board.getResult() == attackerWin;
            break;
        }

        const MoveBucket& fives = board.getPatternBucket(attacker, WINNING);
        if (fives.size() != 1) {
            if (fives.size() > 1) {
                recordFives();
                wins = true;
            }
            break;
        }
        // a forbidden black block is played and loses for black
        const Pos block = fives.front();
        if (!board.move(block)) {
            break;
        }
        ++played;
        if (trace != nullptr) trace->played.push_back(block);
        if (board.getResult() != ONGOING) {
            wins = board.getResult() == attackerWin;
            if (trace != nullptr && wins) trace->forbiddenBlock = true;
            break;
        }
    }

    for (int k = 0; k < played; ++k) {
        board.undo();
    }
    return wins;
}

// Cells a defender stone must take to change the replay: the line's cells, the final five
// points (attacker fives depend on attacker stones only) and defender four points. False
// when forbidden status could flip (a black 3-3 candidate, a forbidden-block win).
inline bool markVCFRefutationCandidates(Board& board, const MoveList& line,
    std::array<uint8_t, 256>& candidate) {
    if (!board.pass()) {
        return false;
    }
    VCFReplayTrace trace;
    const bool wins = replayVCFWins(board, line, &trace);
    board.undo();
    if (!wins || trace.forbiddenBlock || trace.forbiddenSensitive) {
        return false;
    }

    candidate = trace.defenderFours;
    for (const Pos& p : trace.played) candidate[vcf_replay_detail::cellCode(p.getX(), p.getY())] = 1;
    for (const Pos& p : trace.finalFives) candidate[vcf_replay_detail::cellCode(p.getX(), p.getY())] = 1;
    return true;
}

// A complete defense set against `line`: every move left out loses to it. With the filter
// the candidates are taken as is; `filter` = false replays every move (test reference).
inline void collectVCFRefutations(Board& board, const MoveList& line, CandidateList& result,
    bool filter = true) {
    result.clear();
    const bool defenderIsBlack = board.isBlackTurn();

    std::array<uint8_t, 256> candidate;
    const bool filtered = filter && markVCFRefutationCandidates(board, line, candidate);

    for (int x = 1; x <= BOARD_SIZE; ++x) {
        for (int y = 1; y <= BOARD_SIZE; ++y) {
            const Pos move(x, y);
            if (board.getCell(move).getPiece() != EMPTY) {
                continue;
            }
            if (filtered && !candidate[vcf_replay_detail::cellCode(x, y)]) {
                continue;
            }
            // a forbidden move loses on the spot, so it never refutes
            if (defenderIsBlack && board.isForbidden(move)) {
                continue;
            }
            if (filtered) {
                result.push_back(move);
                continue;
            }
            if (!board.move(move)) {
                continue;
            }
            const bool refutes = !replayVCFWins(board, line);
            board.undo();
            if (refutes) {
                result.push_back(move);
            }
        }
    }
}

// Defense against an open three without playing it out: the opponent's open-four point,
// the five points it would leave and the side to move's four points. This is the set the
// replay filter gives for that one-move line. False if there is no clean open-four point.
inline bool collectOpenFourDefense(Board& board, CandidateList& result) {
    const Piece defender = board.isBlackTurn() ? BLACK : WHITE;
    const Piece attacker = defender == BLACK ? WHITE : BLACK;
    if (!board.hasCompositePattern(attacker, MATE)
        || board.hasCompositePattern(attacker, WINNING)
        || board.hasCompositePattern(defender, WINNING)) {
        return false;
    }

    // a black MATE cell is never forbidden: FORBID and FORBID_33 are classed first
    const Pos openFour = board.getFirstPatternPos(attacker, MATE);
    CandidateList cells;
    cells.push_back(openFour);
    board.forEachFivePointAfter(openFour, attacker, [&](const Pos& p) { cells.push_back(p); });
    if (cells.size() < 3) {
        return false;
    }

    std::array<uint8_t, 256> seen = {};
    for (const Pos& p : cells) seen[vcf_replay_detail::cellCode(p.getX(), p.getY())] = 1;
    vcf_replay_detail::markFourPoints(board, defender, seen, &cells);

    result.clear();
    for (const Pos& p : cells) {
        // a forbidden move loses on the spot, so it never refutes
        if (defender == BLACK && board.isForbidden(p)) continue;
        result.push_back(p);
    }
    return true;
}

// The opponent's quickest win behind an open three or 4-3 (open four, or 4-3, block,
// open four), as a line for the side to move; false if none replays.
inline bool findThreatLine(Board& board, MoveList& line) {
    const Piece attacker = board.isBlackTurn() ? WHITE : BLACK;
    if (!board.pass()) {
        return false;
    }

    CandidateList openFours;
    board.getPatternBucket(attacker, MATE).forEach([&](const Pos& p) { openFours.push_back(p); });
    bool found = false;
    for (const Pos& m : openFours) {
        line.assign(1, m);
        if (replayVCFWins(board, line)) {
            found = true;
            break;
        }
    }

    if (!found) {
        CandidateList fourThrees;
        board.getPatternBucket(attacker, B4_F3).forEach([&](const Pos& p) { fourThrees.push_back(p); });
        for (const Pos& p : fourThrees) {
            // the four alone wins when black's block is forbidden
            line.assign(1, p);
            if (replayVCFWins(board, line)) {
                found = true;
                break;
            }
            if (!board.move(p)) continue;
            CandidateList tails;
            Pos block;
            if (board.getResult() == ONGOING && board.getCompositePatternCount(attacker, WINNING) == 1) {
                block = board.getFirstPatternPos(attacker, WINNING);
                if (board.move(block)) {
                    if (board.getResult() == ONGOING) {
                        board.getPatternBucket(attacker, MATE).forEach([&](const Pos& m) { tails.push_back(m); });
                    }
                    board.undo();
                }
            }
            board.undo();
            for (const Pos& m : tails) {
                line = {p, block, m};
                if (replayVCFWins(board, line)) {
                    found = true;
                    break;
                }
            }
            if (found) break;
        }
    }

    board.undo();
    if (!found) line.clear();
    return found;
}
