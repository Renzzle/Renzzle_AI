#pragma once

void Search::lazySMPHelperLoop(size_t helperIndex, std::shared_ptr<LazySMPSnapshot> snapshot,
    std::shared_ptr<std::atomic_bool> helperRunning) {
    state.isRunning = true;
    monitor.initStartTime();

    int lastCompletedDepth = 0;
    Pos lastRootMove;
    while (state.sharedRunning != nullptr
        && state.sharedRunning->load(std::memory_order_relaxed)
        && helperRunning != nullptr
        && helperRunning->load(std::memory_order_relaxed)) {
        int completedDepth = 0;
        Value publishedValue;
        Pos targetRootMove;

        if (snapshot != nullptr) {
            std::lock_guard<std::mutex> lock(snapshot->mutex);
            completedDepth = snapshot->completedDepth;
            publishedValue = snapshot->bestValue;
            if (helperIndex < snapshot->rootMoves.size()) {
                targetRootMove = snapshot->rootMoves[helperIndex];
            }
        }

        if (completedDepth < LAZY_SMP_MIN_COMPLETED_DEPTH || targetRootMove.isDefault()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }

        if (completedDepth == lastCompletedDepth && targetRootMove == lastRootMove) {
            std::this_thread::yield();
            continue;
        }

        board = rootBoard;
        if (!board.move(targetRootMove)) {
            lastCompletedDepth = completedDepth;
            lastRootMove = targetRootMove;
            continue;
        }

        Value alpha(MIN_VALUE, Value::Type::UNKNOWN);
        Value beta(MAX_VALUE + 1, Value::Type::UNKNOWN);
        if (publishedValue.getType() == Value::Type::EXACT) {
            if (publishedValue.isOnGoing()) {
                const int center = publishedValue.getValue();
                alpha = Value(std::max(MIN_VALUE, center - LAZY_SMP_ASPIRATION_DELTA), Value::Type::UNKNOWN);
                beta = Value(std::min(MAX_VALUE + 1, center + LAZY_SMP_ASPIRATION_DELTA), Value::Type::UNKNOWN);
            } else if (publishedValue.isWin()) {
                alpha = Value(Value::Result::WIN, publishedValue.getResultDepth() + 2);
                beta = Value(Value::Result::WIN, std::max(0, publishedValue.getResultDepth() - 2));
            } else if (publishedValue.isLose()) {
                alpha = Value(Value::Result::LOSE, std::max(0, publishedValue.getResultDepth() - 2));
                beta = Value(Value::Result::LOSE, publishedValue.getResultDepth() + 2);
            }
        }
        alpha.decreaseResultDepth();
        beta.decreaseResultDepth();

        abp(completedDepth + LAZY_SMP_CHILD_DEPTH_BONUS, false, alpha, beta, nullptr);
        board = rootBoard;

        lastCompletedDepth = completedDepth;
        lastRootMove = targetRootMove;
    }
}
