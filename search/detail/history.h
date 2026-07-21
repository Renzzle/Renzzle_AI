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
    for (auto& killers : state.killerMoves) {
        killers.fill(0);
    }
}

size_t Search::getSearchPly() {
    return board.getPath().size() - rootBoard.getPath().size();
}

uint8_t Search::packMoveCode(const Pos& move) {
    return static_cast<uint8_t>((move.getX() << 4) | move.getY());
}

void Search::updateKillerMove(size_t ply, uint8_t moveCode) {
    if (ply >= state.killerMoves.size() || moveCode == 0) {
        return;
    }

    auto& killers = state.killerMoves[ply];
    if (killers[0] != moveCode) {
        killers[1] = killers[0];
        killers[0] = moveCode;
    }
}
