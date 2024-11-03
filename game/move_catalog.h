#pragma once

#include "line.h"
#include "pos.h"
#include <vector>

using namespace std;
using MoveList = vector<Pos>;

class MoveCatalog {

    // sure win candidates
    MoveList five;

    // my open four, double four
    MoveList mate;

    // win move
    MoveList fourThree;
    MoveList doubleThree;

    // attack candidates
    MoveList four;
    MoveList openThree;

    // one step before attack
    MoveList setUp;

    // one step before set up move
    MoveList other;

};