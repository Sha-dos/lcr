//
// Created by 64001830 on 12/28/24.
//

#ifndef LCR_GAME_H
#define LCR_GAME_H

#include <vector>
#include "player.h"
#include "dice.h"

class Game {
private:
    std::vector<Player> players;
    int pot;

    bool keepPlay();

    int numOfPlayers;

    enum Direction {
        Left,
        Right
    };

    int calculateNeededPlayerIndex(int currentIndex, Direction direction);

public:
    Game(std::vector<Player> players);
    Game(int numPlayers);

    void play();
};

Game::Game(std::vector<Player> players) {
    this->players = players;
    this->pot = 0;
}

Game::Game(int numPlayers) {
    this->numOfPlayers = numPlayers;

    for (int i = 0; i < numPlayers; i++) {
        std::string name = "Player ";
        name += std::to_string(i);
        this->players.push_back(Player(name, 5, i));
    }

    this->pot = 0;
}

bool Game::keepPlay() {
    int count = 0;

    for (Player p : players) {
        if (p.getChips() > 0) {
            count++;
        }
    }

   return count > 1;
}

void Game::play() {
    while (keepPlay()) {
        for (Player &p : players) {
            if (p.getChips() == 0) {
                continue;
            }

            int numOfRolls = p.getChips();
            if (numOfRolls > 3) {
                numOfRolls = 3;
            }

            for (int i = 1; i <= numOfRolls; i++) {
                switch (Dice::roll()) {
                    case Dice::L:
                        p.removeChips(1);
                        this->players.at(calculateNeededPlayerIndex(p.index, Direction::Left)).addChips(1);
                        break;
                    case Dice::C:
                        p.removeChips(1);
                        this->pot += 1;
                        break;
                    case Dice::R:
                        p.removeChips(1);
                        this->players.at(calculateNeededPlayerIndex(p.index, Direction::Right)).addChips(1);
                        break;
                    case Dice::Dot:
                        break;
                    case Dice::Wild:
                        // Todo: Implement stealing
                        break;
                }
            }
        }
    }

    std::cout << "Game over!" << std::endl;

    for (Player p : this->players) {
        std::cout << p.getName() + " " + std::to_string(p.getChips()) << std::endl;
    }
}

int Game::calculateNeededPlayerIndex(int currentIndex, Direction direction) {
    if (direction == Direction::Right) {
        if (this->numOfPlayers - 1 == currentIndex) {
            return 0;
        } else {
            return currentIndex + 1;
        }
    } else {
        if (currentIndex == 0) {
            return this->numOfPlayers - 1;
        } else {
            return currentIndex - 1;
        }
    }
}

#endif //LCR_GAME_H
