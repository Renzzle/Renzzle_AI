#pragma once

uint64_t Search::getTTKey(Board& board) const {
    uint64_t key = static_cast<uint64_t>(board.getCurrentHash());
    key ^= board.isBlackTurn() ? TURN_KEY_BLACK : TURN_KEY_WHITE;
    return key;
}

uint64_t Search::getChildTTKey(const Pos& move) {
    uint64_t key = static_cast<uint64_t>(board.getChildHash(move));
    key ^= board.isBlackTurn() ? TURN_KEY_WHITE : TURN_KEY_BLACK;
    return key;
}

uint8_t Search::getTTDepth(int depth) const {
    if (depth < 0) return 0;
    if (depth > std::numeric_limits<uint8_t>::max()) {
        return std::numeric_limits<uint8_t>::max();
    }
    return static_cast<uint8_t>(depth);
}

TTFlag Search::getTTFlag(Value::Type type) const {
    if (type == Value::Type::EXACT) return TTFlag::EXACT;
    if (type == Value::Type::LOWER_BOUND) return TTFlag::LOWER_BOUND;
    if (type == Value::Type::UPPER_BOUND) return TTFlag::UPPER_BOUND;
    return TTFlag::NONE;
}

Value Search::getTTValue(const TTEntry& entry) const {
    Value value;
    if (entry.score >= (MAX_VALUE - (BOARD_SIZE * BOARD_SIZE))) {
        value = Value(Value::Result::WIN, MAX_VALUE - entry.score);
    } else if (entry.score <= (MIN_VALUE + (BOARD_SIZE * BOARD_SIZE))) {
        value = Value(Value::Result::LOSE, entry.score - MIN_VALUE);
    } else {
        value = Value(entry.score);
    }

    if (entry.getFlag() == TTFlag::LOWER_BOUND) {
        value.setType(Value::Type::LOWER_BOUND);
    } else if (entry.getFlag() == TTFlag::UPPER_BOUND) {
        value.setType(Value::Type::UPPER_BOUND);
    } else {
        value.setType(Value::Type::EXACT);
    }

    return value;
}

int32_t Search::encodeTTScore(Value value) const {
    if (value.getResult() == Value::Result::WIN) {
        return MAX_VALUE - value.getResultDepth();
    }
    if (value.getResult() == Value::Result::LOSE) {
        return MIN_VALUE + value.getResultDepth();
    }
    return value.getValue();
}

void Search::storeTT(Board& board, Value value, int depth, const Pos& bestMove) {
    const TTFlag flag = getTTFlag(value.getType());
    if (flag == TTFlag::NONE) return;
    if (depth < options.minTTStoreDepth) return;
    if (options.exactOnlyTTStores && flag != TTFlag::EXACT) return;

    tt.store(
        getTTKey(board),
        encodeTTScore(value),
        getTTDepth(depth),
        flag,
        TranspositionTable::encodeMove(bestMove),
        options.nonBlockingTTAccess
    );
}

void Search::appendTTPV(Board tempBoard, MoveList& pv) const {
    for (int ply = 0; ply < BOARD_SIZE * BOARD_SIZE; ++ply) {
        TTEntry entryStorage;
        const TTEntry* entry =
            tt.probeCopy(getTTKey(tempBoard), &entryStorage, options.nonBlockingTTAccess)
                ? &entryStorage
                : nullptr;
        if (entry == nullptr || entry->bestMove == TranspositionTable::INVALID_MOVE) {
            break;
        }

        const Pos move = TranspositionTable::decodeMove(entry->bestMove);
        if (move.isDefault() || !tempBoard.move(move)) {
            break;
        }

        pv.push_back(move);
        if (tempBoard.getResult() != ONGOING) {
            break;
        }
    }
}
