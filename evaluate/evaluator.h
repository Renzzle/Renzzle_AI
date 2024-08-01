#pragma once
#include "../game/board.h"
#include <vector>
#include <algorithm>
#include <tuple>

#define MAX_VALUE 50000
#define MIN_VALUE -50000

using Score = int;

class Evaluator {

private:
    Piece self = BLACK;
    Piece oppo = WHITE;

    // sure win candidates
    vector<Pos> myFive;

    // sure win if there is no opponent's five
    // my open four, double four
    vector<Pos> myMate;

    // sure win if there is no four when block my four
    vector<Pos> myFourThree;
    // sure win if there is no opponent's four 
    // & it is white turn
    vector<Pos> myDoubleThree;

    // attack candidates
    vector<tuple<Pos, Score>> myFour;
    vector<tuple<Pos, Score>> myOpenThree;

    // must defense candidates if there is no sure win
    vector<Pos> oppoFive;

    // not mate move or forced move or attack move
    vector<tuple<Pos, Score>> etc;

    // collect for information (not direct candidates)
    vector<Pos> oppoMate; // opponent's open four, double four
    vector<Pos> oppoForbidden;
    vector<Pos> oppoFour;
    vector<Pos> oppoOpenThree;
    vector<tuple<Pos, Score>> myStrategicMove;
    vector<tuple<Pos, Score>> oppoStrategicMove;
    
    // score table                            D  OL  B1  F1  B2  F2  F2  F2  B3  F3  F3  B4   F4   F5   
    const Score attackScore[PATTERN_SIZE] = { 0, 00, 00, 01, 01, 04, 05, 06, 07, 30, 37, 160, 700, 3000};
    const Score defendScore[PATTERN_SIZE] = { 0, 00, 00, 00, 00, 00, 00, 00, 00, 05, 07, 007, 160, 1000};

    void classify(Board& board);
    void classify(Board& board, Pos pos);
    Score calculateUtilScore(int myPatternCnt[], int oppoPatternCnt[]);

public:
    vector<Pos> getCandidates(Board& board);
    vector<Pos> getFours(Board& board);
    int evaluate(Board& board);

}; 

void Evaluator::classify(Board& board) {
    self = board.isBlackTurn() ? BLACK : WHITE;
    oppo = !board.isBlackTurn() ? BLACK : WHITE;

    for (int i = 1; i <= BOARD_SIZE; i++) {
        for (int j = 1; j <= BOARD_SIZE; j++) {
            classify(board, Pos(i, j));
        }
    }
}

void Evaluator::classify(Board& board, Pos pos) {
    // if forbidden move
    if (self == BLACK && board.isForbidden(pos)) return;

    int myPatternCnt[PATTERN_SIZE] = {0};
    int oppoPatternCnt[PATTERN_SIZE] = {0};

    for (Direction dir = DIRECTION_START; dir < DIRECTION_SIZE; dir++) {
        Pattern p = board.getCell(pos).getPattern(self, dir);
        myPatternCnt[p]++;
        p = board.getCell(pos).getPattern(oppo, dir);
        oppoPatternCnt[p]++;
    }

    if (myPatternCnt[FIVE] > 0) {
        myFive.push_back(pos);
    } else if (oppoPatternCnt[FIVE] > 0) {
        oppoFive.push_back(pos);
    } else if (myPatternCnt[FREE_4] > 0 || myPatternCnt[BLOCKED_4] >= 2) {
        myMate.push_back(pos);
    } else if (myPatternCnt[BLOCKED_4] + myPatternCnt[FREE_3] + myPatternCnt[FREE_3A] >= 2) {
        myFourThree.push_back(pos);
    } else if (myPatternCnt[FREE_3] + myPatternCnt[FREE_3A] >= 2) {
        myDoubleThree.push_back(pos);
    } else if (myPatternCnt[BLOCKED_4] == 1) {
        Score score = calculateUtilScore(myPatternCnt, oppoPatternCnt);
        myFour.push_back(make_tuple(pos, score));
    } else if (myPatternCnt[FREE_3] + myPatternCnt[FREE_3A] == 1) {
        Score score = calculateUtilScore(myPatternCnt, oppoPatternCnt);
        myOpenThree.push_back(make_tuple(pos, score));
    } else {
        Score score = calculateUtilScore(myPatternCnt, oppoPatternCnt);
        etc.push_back(make_tuple(pos, score));
    }

    Score connectionsScore = myPatternCnt[FREE_2] + myPatternCnt[FREE_2A] + myPatternCnt[FREE_2B] + myPatternCnt[BLOCKED_3];
    if (connectionsScore > 0) {
        myStrategicMove.push_back(make_tuple(pos, connectionsScore));
    }

    // if opponent's forbidden
    if (self == WHITE && board.isForbidden(pos)) {
        oppoForbidden.push_back(pos);
        return;
    }
    if (oppoPatternCnt[FREE_4] > 0 || oppoPatternCnt[BLOCKED_4] >= 2) {
        oppoMate.push_back(pos);
    } else if (oppoPatternCnt[BLOCKED_4] + oppoPatternCnt[FREE_4] > 0) {
        oppoFour.push_back(pos);
    } else if (oppoPatternCnt[FREE_3] + oppoPatternCnt[FREE_3A] > 0) {
        oppoOpenThree.push_back(pos);
    }
    
    connectionsScore = oppoPatternCnt[FREE_2] + oppoPatternCnt[FREE_2A] + oppoPatternCnt[FREE_2B] + oppoPatternCnt[BLOCKED_3];
    if (connectionsScore > 0) {
        oppoStrategicMove.push_back(make_tuple(pos, connectionsScore));
    }
}

Score Evaluator::calculateUtilScore(int myPatternCnt[], int oppoPatternCnt[]) {
    Score s = 0;
    for (int i = 0; i < PATTERN_SIZE; i++) {
        if (i < FREE_3) // if it is not direct attack
            s += myPatternCnt[i] * attackScore[i];
        s += oppoPatternCnt[i] * defendScore[i];
    }
    return s;
}

vector<Pos> Evaluator::getCandidates(Board& board) {
    classify(board);

    vector<Pos> result;
    if (!myFive.empty()) {
        result.push_back(myFive.front());
        return result;
    }
    if (!oppoFive.empty()) {
        result.push_back(oppoFive.front());
        return result;
    }
    if (!myMate.empty()) {
        result.push_back(myMate.front());
        return result;
    }
    
    if (!myFourThree.empty()) {
        result.insert(result.end(), myFourThree.begin(), myFourThree.end());
    }
    if (!oppoMate.empty()) {
        result.insert(result.end(), oppoMate.begin(), oppoMate.end());
    }
    if (!myDoubleThree.empty()) {
        result.insert(result.end(), myDoubleThree.begin(), myDoubleThree.end());
    }
    
    vector<Pos> attacks;
    if (!myFour.empty()) {
        attacks.insert(attacks.end(), myFour.begin(), myFour.end());
    }
    if (!myOpenThree.empty()) {
        attacks.insert(attacks.end(), myOpenThree.begin(), myOpenThree.end());
    }
    sort(attacks.begin(), attacks.end(), [](const tuple<Pos, Score>& a, const tuple<Pos, Score>& b) {
        return get<1>(a) > get<1>(b); 
    });
    result.insert(result.end(), attacks.begin(), attacks.end());

    sort(etc.begin(), etc.end(), [](const tuple<Pos, Score>& a, const tuple<Pos, Score>& b) {
        return get<1>(a) > get<1>(b); 
    });
    if (!etc.empty()) {
        result.insert(result.end(), etc.begin(), etc.end());
    }
    return result;
}

vector<Pos> Evaluator::getFours(Board& board) {
    classify(board);

    vector<Pos> result;
    if (!myFive.empty()) {
        result.push_back(myFive.front());
        return result;
    }
    if (!myMate.empty()) {
        result.push_back(myMate.front());
        return result;
    }
    if (!myFourThree.empty()) {
        result.insert(result.end(), myFourThree.begin(), myFourThree.end());
    }
    sort(myFour.begin(), myFour.end(), [](const tuple<Pos, Score>& a, const tuple<Pos, Score>& b) {
        return get<1>(a) > get<1>(b); 
    });
    if (!myFour.empty()) {
        result.insert(result.end(), myFour.begin(), myFour.end());
    }

    return result;
}

int Evaluator::evaluate(Board& board) {
    classify(board);

    // case 1: finish
    Result result = board.getResult();
    if (result != ONGOING) {
        if (result == DRAW) return 0;
        if (self == BLACK && result == BLACK_WIN)
            return MAX_VALUE;
        if (self == BLACK && result == WHITE_WIN)
            return MIN_VALUE;
        if (self == WHITE && result == BLACK_WIN)
            return MIN_VALUE;
        if (self == WHITE && result == WHITE_WIN)
            return MAX_VALUE;
    }
    
    // case 2: there is winning path
    // 1 step before win
    if (!myFive.empty()) {
        return MAX_VALUE - 1;
    }
    // 3 step before win
    if (oppoFive.empty() && !myMate.empty()) {
        return MAX_VALUE - 3;
    }
    // 5 step before win
    if (oppoFive.empty() && oppoFour.empty() && 
    (!myFourThree.empty() || !myDoubleThree.empty())) {
        return MAX_VALUE - 5;
    }

    // case 3: else
    int val = 0;
    val += myFour.size() * 5;
    val += myOpenThree.size() * 5;
    val -= oppoFour.size() * 5;
    val -= oppoOpenThree.size() * 5;
    
    for (auto move : myStrategicMove) {
        val += get<1>(move);
    }
    for (auto move : oppoStrategicMove) {
        val -= get<1>(move);
    }

    return val;
}