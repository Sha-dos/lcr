// =========================================================================
// helpers.h
// =========================================================================
#ifndef LCR_HELPERS_H
#define LCR_HELPERS_H

#include <vector>
#include <string>
#include <iostream>
#include <numeric>
#include <algorithm>
#include <map>
#include <stdexcept>
#include <limits>
#include <fstream>
#include <iomanip>
#include "result.h"

// Contains utility functions for the LCR game
class Helpers {
public:
    // Enum to specify direction around the circle
    enum Direction {
        Left,
        Right
    };

    // Calculates the index of the player to the left or right of the current player
    // Handles wrapping around the circle.
    static int calculateNeededPlayerIndex(int numOfPlayers, int currentIndex, Direction direction);
};

int Helpers::calculateNeededPlayerIndex(int numOfPlayers, int currentIndex, Helpers::Direction direction) {
    if (numOfPlayers <= 1) {
        return currentIndex; // No change if only one player or less
    }

    if (direction == Direction::Right) {
        // Move right, wrap around if necessary (index N-1 goes to 0)
        return (currentIndex + 1) % numOfPlayers;
    } else { // Direction::Left
        // Move left, wrap around if necessary (index 0 goes to N-1)
        // Adding numOfPlayers before modulo handles negative result correctly
        return (currentIndex - 1 + numOfPlayers) % numOfPlayers;
    }
}

#endif //LCR_HELPERS_H