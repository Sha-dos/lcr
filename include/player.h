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
#include "json.hpp"
#include <atomic>
#include <utility>

class Player {
public:
    // Enum defining different strategies for the 'Wild' dice roll
    enum PlayStyle {
        StealFromHighest,          // Always steal from the player with the most chips
        StealFromLowest,           // Always steal from the player with the fewest chips (but > 0)
        StealFromOpposite,         // Wild cancels C > L > R, otherwise steals from opposite
        StealOppositeConditional,  // RESTORED: Wild cancels C only, otherwise steals from opposite
        Random
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
    PlayStyle getPlayStyle() const;

    void setStrategy(PlayStyle newStrategy) {
        this->playStyle = newStrategy;
    }

    // Handles the logic for attempting a steal when a 'Wild' is determined to result in a steal.
    // Returns true if a steal was successful, false otherwise.
    bool attemptSteal(std::vector<Player> &players); // Renamed from handleWild

    // Comparison operator for sorting
    bool operator<(const Player& other) const {
        return chips < other.chips;
    }

    // --- Static members for PlayStyle mapping ---
    // Helper to convert PlayStyle enum to string
    static std::string playStyleToString(PlayStyle style) {
        switch (style) {
            case StealFromHighest: return "StealFromHighest";
            case StealFromLowest: return "StealFromLowest";
            case StealFromOpposite: return "StealFromOpposite";
            case StealOppositeConditional: return "StealOppositeConditional";
            case Random: return "Random";
            default: return "Unknown";
        }
    }

    void addWin() {
        this->wins.fetch_add(1, std::memory_order_relaxed);
    }

    int getWins() const {
        return this->wins.load(std::memory_order_relaxed);
    }

    Player(Player&& other) noexcept
            : name(std::move(other.name)),
              chips(other.chips),
              index(other.index),
              playStyle(other.playStyle),
              totalNumPlayers(other.totalNumPlayers),
              wins(other.wins.load(std::memory_order_relaxed))
    {}
    Player& operator=
            (Player&& other) noexcept {
        if (this != &other) {
            name = std::move(other.name);
            chips = other.chips;
            index = other.index;
            playStyle = other.playStyle;
            totalNumPlayers = other.totalNumPlayers;

            wins.store(other.wins.load(std::memory_order_relaxed), std::memory_order_relaxed);
        }
        return *this;
    }

    Player(const Player& other)
            : name(other.name),
              chips(other.chips),
              index(other.index),
              playStyle(other.playStyle),
              totalNumPlayers(other.totalNumPlayers),
              // Load value from other's atomic and initialize this one
              wins(other.wins.load(std::memory_order_relaxed))
    {}

    Player& operator=(const Player& other) {
        if (this != &other) {
            name = other.name;
            chips = other.chips;
            index = other.index;
            playStyle = other.playStyle;
            totalNumPlayers = other.totalNumPlayers;
            wins.store(other.wins.load(std::memory_order_relaxed), std::memory_order_relaxed);
        }
        return *this;
    }

private:
    std::string name;
    int chips;
    int index;
    PlayStyle playStyle;
    int totalNumPlayers;
    std::atomic<int> wins;
};

NLOHMANN_JSON_SERIALIZE_ENUM(Player::PlayStyle, {
    {Player::PlayStyle::StealFromHighest, "StealFromHighest"},
    {Player::PlayStyle::StealFromLowest, "StealFromLowest"},
    {Player::PlayStyle::StealFromOpposite, "StealFromOpposite"},
    {Player::PlayStyle::StealOppositeConditional, "StealOppositeConditional"},
    {Player::PlayStyle::Random, "Random"}
})

// Constructor implementation
Player::Player(std::string name, int chips, int index, PlayStyle playStyle, int totalPlayers)
        : name(name), chips(chips), index(index), playStyle(playStyle), totalNumPlayers(totalPlayers), wins(0) {}

inline void Player::addChips(int num) {
    this->chips += num;
}

inline void Player::removeChips(int num) {
    this->chips -= num;
    if (this->chips < 0) {
        this->chips = 0;
    }
}

inline int Player::getChips() const {
    return this->chips;
}

inline int Player::getIndex() const {
    return this->index;
}

inline std::string Player::getName() const {
    return this->name;
}

inline Player::PlayStyle Player::getPlayStyle() const {
    return this->playStyle;
}

inline bool Player::attemptSteal(std::vector<Player> &players) {
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
//        std::cout << "    (Wild) Steals 1 chip from " << targetPlayer->getName() << "." << std::endl;
        targetPlayer->removeChips(1);
        self->addChips(1);
        return true; // Steal successful
    } else {
        return false; // Steal failed
    }
}


#endif //LCR_PLAYER_H