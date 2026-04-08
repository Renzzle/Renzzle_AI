#pragma once

int Search::getHistoryIndex(const Pos& move) const {
    const uint16_t encoded = TranspositionTable::encodeMove(move);
    return encoded == TranspositionTable::INVALID_MOVE ? -1 : static_cast<int>(encoded);
}

int Search::getHistoryScore(const Pos& move, bool isBlackTurn) const {
    const int index = getHistoryIndex(move);
    if (index < 0) {
        return 0;
    }
    return state.historyScores[isBlackTurn ? BLACK : WHITE][index];
}

void Search::updateHistoryScore(const Pos& move, bool isBlackTurn, int delta) {
    const int index = getHistoryIndex(move);
    if (index < 0) {
        return;
    }

    int& score = state.historyScores[isBlackTurn ? BLACK : WHITE][index];
    score = std::max(-HISTORY_ABS_LIMIT, std::min(score + delta, HISTORY_ABS_LIMIT));
}

void Search::clearHistory() {
    for (auto& sideHistory : state.historyScores) {
        sideHistory.fill(0);
    }
}
