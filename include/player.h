//
// Created by 64001830 on 12/28/24.
//

#ifndef LCR_PLAYER_H
#define LCR_PLAYER_H

#include <string>

class Player {
public:
    enum PlayStyle {
        StealFromHighest,
        StealFromLowest,
        StealFromOpposite
    };

    Player(std::string name, int chips, int, PlayStyle);

    void addChips(int num);
    void removeChips(int num);
    int getChips() const;
    int index;
    std::string getName();

    void handleWild(std::vector<Player> &players);

    bool operator<(const Player& other) const {
        return chips < other.chips;
    }

private:
    std::string name;
    int chips;
    PlayStyle playStyle;
};

Player::Player(std::string name, int chips, int index, PlayStyle playStyle) {
    this->playStyle = playStyle;
    this->index = index;
    this->name = name;
    this->chips = chips;
}

void Player::addChips(int num) {
    this->chips += num;
}

void Player::removeChips(int num) {
    this->chips -= num;
}

int Player::getChips() const {
    return this->chips;
}

std::string Player::getName() {
    return this->name;
}

void Player::handleWild(std::vector<Player> &players) {
    std::vector<Player> sortedPlayers = players;

    std::sort(sortedPlayers.begin(), sortedPlayers.end(), [](const Player &a, const Player &b) {
        return a.getChips() > b.getChips();
    });

    switch (this->playStyle) {
        case PlayStyle::StealFromHighest:
            std::cout << "Stealing from highest" << std::endl;
            if (!sortedPlayers.empty() && sortedPlayers.front().getChips() > 0) {
                int targetIndex = sortedPlayers.front().index;
                auto it = std::find_if(players.begin(), players.end(), [targetIndex](const Player &p) {
                    return p.index == targetIndex;
                });
                if (it != players.end()) {
                    it->removeChips(1);
                    this->addChips(1);
                }
            }
            break;
        case PlayStyle::StealFromLowest:
            std::cout << "Stealing from lowest" << std::endl;
            while (!sortedPlayers.empty() && sortedPlayers.back().getChips() == 0) {
                sortedPlayers.pop_back();
            }
            if (!sortedPlayers.empty()) {
                int targetIndex = sortedPlayers.back().index;
                auto it = std::find_if(players.begin(), players.end(), [targetIndex](const Player &p) {
                    return p.index == targetIndex;
                });
                if (it != players.end()) {
                    it->removeChips(1);
                    this->addChips(1);
                }
            }
            break;
        case PlayStyle::StealFromOpposite:
            std::cout << "Stealing from opposite" << std::endl;
            int oppositeIndex = (this->index + players.size() / 2) % players.size();
            auto it = std::find_if(players.begin(), players.end(), [oppositeIndex](const Player &p) {
                return p.index == oppositeIndex;
            });
            if (it != players.end() && it->getChips() > 0) {
                it->removeChips(1);
                this->addChips(1);
            } else {
                // Todo: Handle the case where the opposite player has no chips
                std::cout << "Opposite player has no chips, skipping action." << std::endl;
            }
            break;
    }
}

#endif //LCR_PLAYER_H
