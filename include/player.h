//
// Created by 64001830 on 12/28/24.
//

#ifndef LCR_PLAYER_H
#define LCR_PLAYER_H

#include <string>
#include <vector>
#include <algorithm>
#include <iostream>
#include <limits>

// Forward declaration of Game to use its methods/members if needed
// class Game;
// Forward declaration not strictly needed here if we pass players vector directly

class Player {
public:
    // Enum defining different strategies for the 'Wild' dice roll
    enum PlayStyle {
        StealFromHighest, // Steal from the player with the most chips
        StealFromLowest,  // Steal from the player with the fewest chips (but > 0)
        StealFromOpposite // Steal from the player farthest away (opposite side)
    };

    Player(std::string name, int chips, int index, PlayStyle playStyle, int totalPlayers);

    // Modifiers for player's chips
    void addChips(int num);
    void removeChips(int num);

    // Getters
    int getChips() const;
    int getIndex() const;
    std::string getName() const;

    // Handles the logic when a 'Wild' is rolled, based on playStyle
    void handleWild(std::vector<Player> &players);

    // Comparison operator for sorting players based on chips (useful for StealFromHighest/Lowest)
    bool operator<(const Player& other) const {
        return chips < other.chips;
    }

private:
    std::string name;
    int chips;
    int index; // Player's position/index in the circle (0 to N-1)
    PlayStyle playStyle;
    int totalNumPlayers;
};

Player::Player(std::string name, int chips, int index, PlayStyle playStyle, int totalPlayers)
        : name(name), chips(chips), index(index), playStyle(playStyle), totalNumPlayers(totalPlayers) {}

void Player::addChips(int num) {
    this->chips += num;
}

void Player::removeChips(int num) {
    this->chips -= num;
    if (this->chips < 0) {
        this->chips = 0;
    }
}

int Player::getChips() const {
    return this->chips;
}

int Player::getIndex() const {
    return this->index;
}


std::string Player::getName() const {
    return this->name;
}

void Player::handleWild(std::vector<Player> &players) {
    // Find the current player in the vector to exclude them from potential targets
    Player* self = nullptr;
    for(Player& p : players) {
        if(p.getIndex() == this->index) {
            self = &p;
            break;
        }
    }

    // Create a list of potential targets (other players with chips)
    std::vector<Player*> potentialTargets;
    for (Player &p : players) {
        // Target must have chips and not be the current player
        if (p.getChips() > 0 && p.getIndex() != this->index) {
            potentialTargets.push_back(&p); // Store pointers to modify original players
        }
    }

    // If no other players have chips, the wild roll does nothing
    if (potentialTargets.empty()) {
        std::cout << "    No other players have chips to steal from." << std::endl;
        return;
    }

    // --- Logic for different PlayStyles ---
    Player* targetPlayer = nullptr; // Pointer to the player to steal from

    switch (this->playStyle) {
        case PlayStyle::StealFromHighest: {
            std::cout << "    Strategy: Steal from Highest." << std::endl;
            // Sort potential targets by chips descending
            std::sort(potentialTargets.begin(), potentialTargets.end(), [](const Player* a, const Player* b) {
                return a->getChips() > b->getChips();
            });
            targetPlayer = potentialTargets.front(); // Highest is now at the front
            break;
        }
        case PlayStyle::StealFromLowest: {
            std::cout << "    Strategy: Steal from Lowest (with chips)." << std::endl;
            // Sort potential targets by chips ascending
            std::sort(potentialTargets.begin(), potentialTargets.end(), [](const Player* a, const Player* b) {
                return a->getChips() < b->getChips();
            });
            // The lowest player with chips (>0) will be at the front
            targetPlayer = potentialTargets.front();
            break;
        }
        case PlayStyle::StealFromOpposite: {
            std::cout << "    Strategy: Steal from Opposite." << std::endl;
            int numPlayers = this->totalNumPlayers; // Use stored total number of players
            int currentIdx = this->index;

            // Calculate the index directly opposite
            int oppositeIndex = (currentIdx + numPlayers / 2) % numPlayers;

            // Search outwards from the opposite index
            int targetIndex = -1;
            for (int offset = 0; offset <= numPlayers / 2; ++offset) {
                // Check right first (relative to opposite)
                int checkRightIndex = (oppositeIndex + offset + numPlayers) % numPlayers; // Ensure positive index
                // Find player with this index *among potential targets*
                auto it_right = std::find_if(potentialTargets.begin(), potentialTargets.end(),
                                             [checkRightIndex](const Player* p){ return p->getIndex() == checkRightIndex; });
                if (it_right != potentialTargets.end()) { // Found a valid target to the right
                    targetIndex = checkRightIndex;
                    targetPlayer = *it_right;
                    break; // Found the closest target
                }

                // Check left (relative to opposite), but only if offset > 0 (don't check opposite twice). Same as above
                if (offset > 0) {
                    int checkLeftIndex = (oppositeIndex - offset + numPlayers) % numPlayers;
                    auto it_left = std::find_if(potentialTargets.begin(), potentialTargets.end(),
                                                [checkLeftIndex](const Player* p){ return p->getIndex() == checkLeftIndex; });
                    if (it_left != potentialTargets.end()) {
                        targetIndex = checkLeftIndex;
                        targetPlayer = *it_left;
                        break;
                    }
                }
            }
            // If targetPlayer is still null after the loop, it means no valid target was found
            // (This shouldn't happen if potentialTargets was not empty, but good for safety)
            if (!targetPlayer) {
                std::cout << "    Error: Could not find a valid opposite target." << std::endl;
                return; // Exit without stealing
            }
            break; // End of StealFromOpposite case
        } // End switch case StealFromOpposite
    } // End switch(playStyle)

    // --- Perform the Steal ---
    if (targetPlayer && self) { // Ensure both target and self are valid
        std::cout << "    Stealing 1 chip from " << targetPlayer->getName() << "." << std::endl;
        targetPlayer->removeChips(1);
        self->addChips(1); // Use the 'self' pointer to modify the current player
    } else if (!targetPlayer) {
        // This case might occur if the chosen strategy leads to no valid target
        // (e.g., trying to steal from lowest but everyone else has 0 chips - handled earlier)
        // Or if the opposite logic somehow failed.
        std::cout << "    Could not determine a target player for stealing." << std::endl;
    } else if (!self) {
        std::cout << "    Error: Could not find self in player list." << std::endl;
    }
}


#endif //LCR_PLAYER_H
