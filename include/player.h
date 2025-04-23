//
// Created by 64001830 on 12/28/24.
//

#ifndef LCR_PLAYER_H
#define LCR_PLAYER_H

#include <string>

class Player {
private:
    std::string name;
    int chips;

public:
    Player(std::string name, int chips, int);

    void addChips(int num);
    void removeChips(int num);
    int getChips();
    int index;
    std::string getName();
};

Player::Player(std::string name, int chips, int index) {
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

int Player::getChips() {
    return this->chips;
}

std::string Player::getName() {
    return this->name;
}

#endif //LCR_PLAYER_H
