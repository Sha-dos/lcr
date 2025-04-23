//
// Created by 64001830 on 12/28/24.
//

#ifndef LCR_GAME_H
#define LCR_GAME_H

#include <vector>
#include <string> // Added for std::string
#include <iostream> // Added for std::cout
#include <numeric> // Added for std::accumulate (optional, for potential future use)
#include <algorithm> // Added for std::find_if
#include "player.h"
#include "dice.h"
#include "helpers.h"

// Forward declaration of Player to resolve circular dependency if Game needs Player info
class Player;

class Game {
private:
    std::vector<Player> players;
    int pot;
    int numOfPlayers; // Store the number of players

    // Checks if the game should continue (more than one player has chips)
    bool keepPlay();

public:
    // Constructor taking a vector of players
    Game(std::vector<Player> players);
    // Constructor taking the number of players to create
    Game(int numPlayers);

    // Main game loop
    void play();

    // Getter for the number of players (needed by Player::handleWild)
    int getNumOfPlayers() const { return numOfPlayers; }
};

// Constructor implementation
Game::Game(std::vector<Player> players) : players(players), pot(0) {
    this->numOfPlayers = players.size(); // Initialize numOfPlayers
}

// Constructor implementation
Game::Game(int numPlayers) : numOfPlayers(numPlayers), pot(0) {
    // Create players with default names, chips, index, and playstyle
    for (int i = 0; i < numPlayers; ++i) {
        std::string name = "Player ";
        name += std::to_string(i + 1); // Player numbers start from 1
        // Pass the total number of players to the Player constructor
        this->players.emplace_back(name, 5, i, Player::PlayStyle::StealFromOpposite, numPlayers);
    }
}

// keepPlay implementation
bool Game::keepPlay() {
    int count = 0;
    // Count how many players still have chips
    for (const Player& p : players) { // Use const reference
        if (p.getChips() > 0) {
            count++;
        }
    }
    // Game continues if more than one player has chips
    return count > 1;
}

// play implementation
void Game::play() {
    int round = 1; // Keep track of rounds for output
    while (keepPlay()) {
        std::cout << "\n--- Round " << round++ << " ---" << std::endl;
        for (int i = 0; i < numOfPlayers; ++i) { // Iterate using index to safely modify players vector via reference
            Player &p = players[i];

            // Check if the game ended mid-round
            if (!keepPlay()) break;

            // Skip players with no chips
            if (p.getChips() == 0) {
                std::cout << p.getName() << " has no chips, skips turn." << std::endl;
                continue;
            }

            std::cout << "\n" << p.getName() << "'s turn (Chips: " << p.getChips() << ")" << std::endl;

            // Determine number of dice rolls (max 3)
            int numOfRolls = std::min(p.getChips(), 3);
            std::cout << p.getName() << " rolls " << numOfRolls << " dice." << std::endl;

            // Perform dice rolls
            for (int j = 0; j < numOfRolls; ++j) {
                // Check if player ran out of chips mid-turn due to previous rolls
                if (p.getChips() == 0) break;

                Dice::Side rollResult = Dice::roll();
                switch (rollResult) {
                    case Dice::L: { // Pass chip to the left
                        std::cout << "  Rolled L. ";
                        p.removeChips(1);
                        int leftPlayerIndex = Helpers::calculateNeededPlayerIndex(numOfPlayers, p.getIndex(), Helpers::Direction::Left);
                        players.at(leftPlayerIndex).addChips(1);
                        std::cout << "Passes 1 chip to " << players.at(leftPlayerIndex).getName() << "." << std::endl;
                        break;
                    }
                    case Dice::C: { // Add chip to the pot
                        std::cout << "  Rolled C. ";
                        p.removeChips(1);
                        this->pot += 1;
                        std::cout << "Adds 1 chip to the pot (Pot: " << this->pot << ")." << std::endl;
                        break;
                    }
                    case Dice::R: { // Pass chip to the right
                        std::cout << "  Rolled R. ";
                        p.removeChips(1);
                        int rightPlayerIndex = Helpers::calculateNeededPlayerIndex(numOfPlayers, p.getIndex(), Helpers::Direction::Right);
                        players.at(rightPlayerIndex).addChips(1);
                        std::cout << "Passes 1 chip to " << players.at(rightPlayerIndex).getName() << "." << std::endl;
                        break;
                    }
                    case Dice::Dot: { // Keep chip
                        std::cout << "  Rolled Dot. Keeps chip." << std::endl;
                        break;
                    }
                    case Dice::Wild: { // Steal a chip based on playstyle
                        std::cout << "  Rolled Wild! " << p.getName() << " attempts to steal." << std::endl;
//                         std::cout << "    Chips before Wild:" << std::endl;
//                         for (const Player &pl : this->players) {
//                             std::cout << "      " << pl.getName() << ": " << pl.getChips() << std::endl;
//                         }

                        // Handle the wild roll based on the player's strategy
                        p.handleWild(this->players); // Pass the players vector by reference

//                         std::cout << "    Chips after Wild:" << std::endl;
//                         for (const Player &pl : this->players) {
//                             std::cout << "      " << pl.getName() << ": " << pl.getChips() << std::endl;
//                         }

                        break;
                    }
                }
                // If the player has no more chips after this roll, they can't roll again (should never happen)
                if (p.getChips() == 0) {
                    std::cout << "  " << p.getName() << " ran out of chips." << std::endl;
                    break;
                }
            }
            // Check if game ended after this player's turn
            if (!keepPlay()) break;
        }
    }

    // Todo: implement needing to roll all dots to win
    // Game over: Find and announce the winner
    std::cout << "\n--- Game Over! ---" << std::endl;
    Player* winner = nullptr; // Pointer to the winning player
    for (Player &p : this->players) { // Find the player with chips remaining
        if (p.getChips() > 0) {
            winner = &p;
            break;
        }
    }

    if (winner) {
        winner->addChips(this->pot); // Winner gets the pot
        std::cout << winner->getName() << " wins with " << winner->getChips() << " chips (including " << this->pot << " from the pot)!" << std::endl;
    } else {
        // This case should ideally not happen if keepPlay() works correctly,
        // but it's good practice to handle it. It might occur if somehow
        // the last two players lose their chips simultaneously.
        std::cout << "No winner? All players ran out of chips simultaneously. Pot: " << this->pot << std::endl;
    }

    // Final chip counts
     std::cout << "\nFinal Chip Counts:" << std::endl;
     for (const Player& p : this->players) {
         std::cout << p.getName() << ": " << p.getChips() << std::endl;
     }
}

#endif //LCR_GAME_H