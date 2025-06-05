#include "../test/test.h"

#define MAX_VALUE 50000
#define MIN_VALUE -50000
#define INITIAL_VALUE -99999

class Value {

PRIVATE    
    enum class Type {
        EXACT, LOWER_BOUND, UPPER_BOUND, UNKNOWN
    };
    enum class Result {
        ONGOING, WIN, LOSE
    };
    int value;
    Type type;
    Result result;
    int resultDepth;

PUBLIC
    Value() : Value(INITIAL_VALUE, Type::UNKNOWN, Result::ONGOING, INITIAL_VALUE) {}

    Value(int val) : Value(val, Type::EXACT, Result::ONGOING, INITIAL_VALUE) {}

    Value(int val, Type type) : Value(val, type, Result::ONGOING, INITIAL_VALUE) {}

    Value(Result result) : Value(result, 0) {}

    Value(Result result, int resultDepth) {
        int value = INITIAL_VALUE;
        Type type = Type::UNKNOWN;
        if (result == Result::WIN) {
            value = MAX_VALUE;
            type = Type::EXACT;
        } else if (result == Result::LOSE) {
            value = MIN_VALUE;
            type = Type::EXACT;
        }

        this->value = value;
        this->type = type;
        this->result = result;
        this->resultDepth = resultDepth;
    }

    Value(int val, Type type, Result result, int resultDepth) {
        this->value = val;
        this->type = type;
        this->result = result;
        this->resultDepth = resultDepth;
    }

    bool isWin() {
        if (result == Result::WIN) return true;
        else return false;
    }

    int getValue() {
        return value;
    }

    Type getType() {
        return type;
    }

    Result getResult() {
        return result;
    }

    int getResultDepth() {
        return resultDepth;
    }

    void setType(Type type) {
        this->type = type;
    }

    void invert() {
        value *= -1;
        if (result == Result::WIN) {
            result = Result::LOSE;
        } else if (result == Result::LOSE) {
            result = Result::WIN;
        }
    }

    void increaseResultDepth() {
        if (result != Result::ONGOING) {
            resultDepth += 1;
        }
    }

    void decreaseResultDepth() {
        if (result != Result::ONGOING) {
            resultDepth -= 1;
        }
    }

    Value& operator+=(int n) {
        value += n;
        return *this;
    }

    Value& operator-=(int n) {
        value -= n;
        return *this;
    }

    Value& operator*=(int n) {
        value *= n;
        return *this;
    }

    Value& operator/=(int n) {
        value /= n;
        return *this;
    }

    bool operator<(const Value& other) const {
        if (value != other.value) {
            return value < other.value;
        } else {
            if (resultDepth == INITIAL_VALUE || other.resultDepth == INITIAL_VALUE) {
                return false;
            }
            if (value == MAX_VALUE) {
                return resultDepth > other.resultDepth;
            } else if (value == MIN_VALUE) {
                return resultDepth < other.resultDepth;
            } else return false;
        }
    }

    bool operator==(const Value& other) const {
        if ((value != MAX_VALUE && value != MIN_VALUE) || 
            (other.value != MAX_VALUE && other.value != MIN_VALUE))
            return (value == other.value);
        if (resultDepth == INITIAL_VALUE || other.resultDepth == INITIAL_VALUE)
            return (value == other.value);
        else
            return (value == other.value) && (resultDepth == other.resultDepth);
    }

    bool operator!=(const Value& other) const {
        return !(*this == other);
    }

    bool operator>(const Value& other) const {
        return !(*this < other) && !(*this == other);
    }

    bool operator<=(const Value& other) const {
        return (*this < other) || (*this == other);
    }

    bool operator>=(const Value& other) const {
        return !(*this < other);
    }

    Value& operator=(const Value& other) {
        if (this == &other) return *this;
        value = other.value;
        type = other.type;
        result = other.result;
        resultDepth = other.resultDepth;
        return *this;
    }

    Value& operator=(int val) {
        value = val;
        type = Type::EXACT;
        if (val == MAX_VALUE) {
            result = Result::WIN;
            resultDepth = 0;
        } else if (val == MIN_VALUE) {
            result = Result::LOSE;
            resultDepth = 0;
        } else {
            result = Result::ONGOING;
            resultDepth = INITIAL_VALUE;
        }
        return *this;
    }

};