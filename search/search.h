#ifndef SEARCH_H
#define SEARCH_H

#include <memory>

using Value = int;
using Depth = int;

class Search {

public:
    Value alphaBeta(Depth depth, Value alpha, Value beta, bool maximizingPlayer);

};

#endif // SEARCH_H
