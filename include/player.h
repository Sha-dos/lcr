// =========================================================================
// player.h
// =========================================================================
#ifndef LCR_PLAYER_H
#define LCR_PLAYER_H

#include <string>
#include <vector>
#include <algorithm>
#include <iostream>
#include <limits>

// Forward declaration
// class Game; // Not strictly needed if we pass players vector

class Player {
public:
    // Enum defining different strategies for the 'Wild' dice roll
    enum PlayStyle {
        StealFromHighest,         // Always steal from the player with the most chips
        StealFromLowest,          // Always steal from the player with the fewest chips (but > 0)
        StealFromOpposite,        // Wild cancels C > L > R, otherwise steals from opposite
        StealOppositeConditional  // RESTORED: Wild cancels C only, otherwise steals from opposite
    };

    // Constructor
    Player(std::string name, int chips, int index, PlayStyle playStyle, int totalPlayers);

    // Modifiers for player's chips
    void addChips(int num);
    void removeChips(int num);

    // Getters
    int getChips() const;
    int getIndex() const;
    std::string getName() const;
    PlayStyle getPlayStyle() const; // Added getter for playstyle

    // Handles the logic for attempting a steal when a 'Wild' is determined to result in a steal.
    // Returns true if a steal was successful, false otherwise.
    bool attemptSteal(std::vector<Player> &players); // Renamed from handleWild

    // Comparison operator for sorting
    bool operator<(const Player& other) const {
        return chips < other.chips;
    }

private:
    std::string name;
    int chips;
    int index;
    PlayStyle playStyle;
    int totalNumPlayers;
};

// Constructor implementation
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

Player::PlayStyle Player::getPlayStyle() const {
    return this->playStyle;
}

bool Player::attemptSteal(std::vector<Player> &players) {
    // Find the current player ('self')
    Player* self = nullptr;
    for(Player& p : players) {
        if(p.getIndex() == this->index) {
            self = &p;
            break;
        }
    }
    if (!self) {
        std::cerr << "Error: Could not find self in player list during steal attempt." << std::endl;
        return false;
    }

    // Create a list of potential targets (other players with chips)
    std::vector<Player*> potentialTargets;
    for (Player &p : players) {
        if (p.getChips() > 0 && p.getIndex() != this->index) {
            potentialTargets.push_back(&p);
        }
    }

    if (potentialTargets.empty()) {
        // Message handled in Game::play
        return false; // Steal fails if no valid targets
    }

    // --- Logic for finding the target based on PlayStyle ---
    Player* targetPlayer = nullptr;

    switch (this->playStyle) {
        case PlayStyle::StealFromHighest: {
            std::sort(potentialTargets.begin(), potentialTargets.end(), [](const Player* a, const Player* b) {
                return a->getChips() > b->getChips();
            });
            targetPlayer = potentialTargets.front();
            break;
        }
        case PlayStyle::StealFromLowest: {
            std::sort(potentialTargets.begin(), potentialTargets.end(), [](const Player* a, const Player* b) {
                return a->getChips() < b->getChips();
            });
            targetPlayer = potentialTargets.front();
            break;
        }
        case PlayStyle::StealFromOpposite:          // Target finding is the same for both opposite styles
        case PlayStyle::StealOppositeConditional: {
            int numPlayers = this->totalNumPlayers;
            int currentIdx = this->index;
            int oppositeIndex = (currentIdx + numPlayers / 2) % numPlayers;

            for (int offset = 0; offset <= numPlayers / 2; ++offset) {
                // Check right
                int checkRightIndex = (oppositeIndex + offset + numPlayers) % numPlayers;
                auto it_right = std::find_if(potentialTargets.begin(), potentialTargets.end(),
                                             [checkRightIndex](const Player* p){ return p->getIndex() == checkRightIndex; });
                if (it_right != potentialTargets.end()) {
                    targetPlayer = *it_right;
                    break;
                }
                // Check left (if not the same as right)
                if (offset > 0) {
                    int checkLeftIndex = (oppositeIndex - offset + numPlayers) % numPlayers;
                    auto it_left = std::find_if(potentialTargets.begin(), potentialTargets.end(),
                                                [checkLeftIndex](const Player* p){ return p->getIndex() == checkLeftIndex; });
                    if (it_left != potentialTargets.end()) {
                        targetPlayer = *it_left;
                        break;
                    }
                }
            }
            // If targetPlayer is still null after search
            if (!targetPlayer) {
                return false; // Steal fails
            }
            break;
        } // End Opposite cases
    } // End switch(playStyle)

    // --- Perform the Steal ---
    if (targetPlayer) {
        std::cout << "    (Wild) Steals 1 chip from " << targetPlayer->getName() << "." << std::endl;
        targetPlayer->removeChips(1);
        self->addChips(1);
        return true; // Steal successful
    } else {
        return false; // Steal failed
    }
}


#endif //LCR_PLAYER_H