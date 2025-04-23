// =========================================================================
// game.h
// =========================================================================
#ifndef LCR_GAME_H
#define LCR_GAME_H

#include <vector>
#include <string>
#include <iostream>
#include <numeric>
#include <algorithm>
#include <map>      // Include map for roll counts
#include "player.h" // Includes Player::PlayStyle
#include "dice.h"
#include "helpers.h"

class Game {
private:
    std::vector<Player> players;
    int pot;
    int numOfPlayers;

    bool keepPlay();

public:
    // Default style can be chosen, StealFromOpposite is a reasonable default
    Game(int numPlayers, Player::PlayStyle defaultStyle = Player::PlayStyle::StealFromOpposite);
    Game(std::vector<Player> initialPlayers); // Keep constructor taking players

    void play();
    int getNumOfPlayers() const { return numOfPlayers; }
};

// Constructor implementation (taking numPlayers)
Game::Game(int numPlayers, Player::PlayStyle defaultStyle) : numOfPlayers(numPlayers), pot(0) {
    if (numPlayers < 2) {
        throw std::invalid_argument("Game requires at least 2 players.");
    }
    for (int i = 0; i < numPlayers; ++i) {
        std::string name = "Player ";
        name += std::to_string(i + 1);
        // Use the provided defaultStyle
        this->players.emplace_back(name, 3, i, defaultStyle, numPlayers); // Start with 3 chips
    }
    std::cout << "Game created with " << numPlayers << " players using strategy: ";
    switch(defaultStyle) {
        case Player::PlayStyle::StealFromHighest: std::cout << "StealFromHighest" << std::endl; break;
        case Player::PlayStyle::StealFromLowest: std::cout << "StealFromLowest" << std::endl; break;
        case Player::PlayStyle::StealFromOpposite: std::cout << "StealFromOpposite (W cancels C>L>R, else steals)" << std::endl; break;
        case Player::PlayStyle::StealOppositeConditional: std::cout << "StealOppositeConditional (W cancels C only, else steals)" << std::endl; break; // Added back
    }
}

Game::Game(std::vector<Player> initialPlayers) : players(std::move(initialPlayers)), pot(0) {
    this->numOfPlayers = players.size();
    if (this->numOfPlayers < 2) {
        throw std::invalid_argument("Game requires at least 2 players.");
    }
    std::cout << "Game created with " << numOfPlayers << " pre-defined players." << std::endl;
}


bool Game::keepPlay() {
    int count = 0;
    for (const Player& p : players) {
        if (p.getChips() > 0) {
            count++;
        }
    }
    return count > 1;
}

void Game::play() {
    int round = 1;
    while (keepPlay()) {
        std::cout << "\n--- Round " << round++ << " ---" << std::endl;
        for (int i = 0; i < numOfPlayers; ++i) {
            Player &p = players[i]; // Get reference to current player

            if (!keepPlay()) break; // Check if game ended mid-round
            if (p.getChips() == 0) { // Skip players with no chips
                std::cout << "\n" << p.getName() << " has no chips, skipping turn" << std::endl;
            };

            std::cout << "\n" << p.getName() << "'s turn (Chips: " << p.getChips() << ")" << std::endl;

            // 1. Determine number of dice rolls
            int numOfRolls = std::min(p.getChips(), 3);
            if (numOfRolls == 0) continue;
            std::cout << "  Rolling " << numOfRolls << " dice: ";

            // 2. Roll dice and count results
            std::map<Dice::Side, int> rollCounts;
            for (int j = 0; j < numOfRolls; ++j) {
                Dice::Side result = Dice::roll();
                rollCounts[result]++;
                // Print roll results
                switch(result) {
                    case Dice::L: std::cout << "L "; break;
                    case Dice::C: std::cout << "C "; break;
                    case Dice::R: std::cout << "R "; break;
                    case Dice::Dot: std::cout << ". "; break;
                    case Dice::Wild: std::cout << "W "; break;
                }
            }
            std::cout << std::endl;

            // 3. Initialize net actions based on counts
            int netPassLeft = rollCounts[Dice::L];
            int netPassRight = rollCounts[Dice::R];
            int netToPot = rollCounts[Dice::C];
            int netWilds = rollCounts[Dice::Wild];

            // 4. Apply Wild Logic based on PlayStyle
            int stealsToAttempt = 0;
            int chipsKeptFromCancellation = 0;

            if (netWilds > 0) { // Only apply special logic if wilds were rolled
                switch (p.getPlayStyle()) {
                    case Player::PlayStyle::StealFromHighest:
                    case Player::PlayStyle::StealFromLowest:
                        // These styles always attempt to steal for every Wild rolled
                        stealsToAttempt = netWilds;
                        std::cout << "    (" << (p.getPlayStyle() == Player::PlayStyle::StealFromHighest ? "Highest" : "Lowest") << ") "
                                  << netWilds << " Wild(s) trigger steal attempts." << std::endl;
                        break;

                    case Player::PlayStyle::StealFromOpposite: { // W cancels C > L > R, else steals
                        std::cout << "    (Opposite C>L>R) Processing " << netWilds << " Wild(s)..." << std::endl;
                        int currentWilds = netWilds; // Work with a copy

                        // Priority 1: Cancel Center
                        int cancelC = std::min(currentWilds, netToPot);
                        if (cancelC > 0) {
                            std::cout << "      - " << cancelC << " Wild(s) cancel " << cancelC << " Center(s). Player keeps chip(s)." << std::endl;
                            currentWilds -= cancelC;
                            netToPot -= cancelC;
                            chipsKeptFromCancellation += cancelC;
                        }

                        // Priority 2: Cancel Left
                        int cancelL = std::min(currentWilds, netPassLeft);
                        if (cancelL > 0) {
                            std::cout << "      - " << cancelL << " Wild(s) cancel " << cancelL << " Left(s). Player keeps chip(s)." << std::endl;
                            currentWilds -= cancelL;
                            netPassLeft -= cancelL;
                            chipsKeptFromCancellation += cancelL;
                        }

                        // Priority 3: Cancel Right
                        int cancelR = std::min(currentWilds, netPassRight);
                        if (cancelR > 0) {
                            std::cout << "      - " << cancelR << " Wild(s) cancel " << cancelR << " Right(s). Player keeps chip(s)." << std::endl;
                            currentWilds -= cancelR;
                            netPassRight -= cancelR;
                            chipsKeptFromCancellation += cancelR;
                        }

                        // Remaining wilds attempt steals
                        stealsToAttempt = currentWilds;
                        if (stealsToAttempt > 0) {
                            std::cout << "      - " << stealsToAttempt << " remaining Wild(s) trigger steal attempts." << std::endl;
                        }
                        if (chipsKeptFromCancellation > 0 && stealsToAttempt == 0) { // Adjusted condition slightly
                            std::cout << "      - All Wilds cancelled other dice. No steals attempted." << std::endl;
                        }
                        break; // End StealFromOpposite case
                    } // End StealFromOpposite block

                    case Player::PlayStyle::StealOppositeConditional: { // W cancels C only, else steals
                        std::cout << "    (Opposite Conditional C) Processing " << netWilds << " Wild(s)..." << std::endl;
                        int currentWilds = netWilds; // Work with a copy

                        // Priority 1: Cancel Center
                        int cancelC = std::min(currentWilds, netToPot);
                        if (cancelC > 0) {
                            std::cout << "      - " << cancelC << " Wild(s) cancel " << cancelC << " Center(s). Player keeps chip(s)." << std::endl;
                            currentWilds -= cancelC;
                            netToPot -= cancelC; // Update netToPot
                            chipsKeptFromCancellation += cancelC;
                        }

                        // Remaining wilds attempt steals
                        stealsToAttempt = currentWilds;
                        if (stealsToAttempt > 0) {
                            std::cout << "      - " << stealsToAttempt << " remaining Wild(s) trigger steal attempts." << std::endl;
                        }
                        if (chipsKeptFromCancellation > 0 && stealsToAttempt == 0) { // Adjusted condition slightly
                            std::cout << "      - All Wilds cancelled Center dice. No steals attempted." << std::endl;
                        }
                        break; // End StealOppositeConditional case
                    } // End StealOppositeConditional block

                } // End switch(PlayStyle)
            } // End if (netWilds > 0)

            // 5. Execute Actions (Passes, Pot, Steals) respecting chip limits

            int chipsAvailable = p.getChips();
            int chipsToRemoveTotal = 0; // Track chips player actually loses this turn (from L, C, R)

            // --- Pass Left --- (Using potentially updated netPassLeft)
            int actualPassLeft = std::min(netPassLeft, chipsAvailable - chipsToRemoveTotal);
            if (actualPassLeft > 0) {
                std::cout << "    Passes " << actualPassLeft << " chip(s) left." << std::endl;
                int leftPlayerIndex = Helpers::calculateNeededPlayerIndex(numOfPlayers, p.getIndex(), Helpers::Direction::Left);
                players.at(leftPlayerIndex).addChips(actualPassLeft);
                chipsToRemoveTotal += actualPassLeft;
            }

            // --- To Pot --- (Using potentially updated netToPot)
            int actualToPot = std::min(netToPot, chipsAvailable - chipsToRemoveTotal);
            if (actualToPot > 0) {
                std::cout << "    Adds " << actualToPot << " chip(s) to pot." << std::endl;
                this->pot += actualToPot;
                chipsToRemoveTotal += actualToPot;
            }

            // --- Pass Right --- (Using potentially updated netPassRight)
            int actualPassRight = std::min(netPassRight, chipsAvailable - chipsToRemoveTotal);
            if (actualPassRight > 0) {
                std::cout << "    Passes " << actualPassRight << " chip(s) right." << std::endl;
                int rightPlayerIndex = Helpers::calculateNeededPlayerIndex(numOfPlayers, p.getIndex(), Helpers::Direction::Right);
                players.at(rightPlayerIndex).addChips(actualPassRight);
                chipsToRemoveTotal += actualPassRight;
            }

            // --- Remove chips lost to L, C, R ---
            if (chipsToRemoveTotal > 0) {
                p.removeChips(chipsToRemoveTotal);
                std::cout << "    Player loses " << chipsToRemoveTotal << " chip(s) to L/C/R." << std::endl;
            }

            // --- Report chips kept from cancellation ---
            if (chipsKeptFromCancellation > 0 && chipsToRemoveTotal == 0) { // Only report if no other chips were lost
                std::cout << "    Player kept " << chipsKeptFromCancellation << " chip(s) due to Wild cancellations." << std::endl;
            } else if (chipsKeptFromCancellation > 0 && chipsToRemoveTotal > 0) {
                std::cout << "    Player also kept " << chipsKeptFromCancellation << " chip(s) due to Wild cancellations." << std::endl;
            } else if (chipsToRemoveTotal == 0 && stealsToAttempt == 0 && rollCounts[Dice::Dot] > 0) { // If only dots/failed steals/no cancels
                std::cout << "    Player keeps remaining chips (Dot/Failed Steal)." << std::endl;
            }


            // --- Attempt Steals (Wilds) ---
            int successfulSteals = 0;
            if (stealsToAttempt > 0) {
                std::cout << "    Attempting " << stealsToAttempt << " steal(s)..." << std::endl;
                for (int k = 0; k < stealsToAttempt; ++k) {
                    if (p.attemptSteal(players)) { // Pass the vector by reference
                        successfulSteals++;
                    } else {
                        std::cout << "    Steal attempt " << (k + 1) << " failed (no valid target with chips)." << std::endl;
                    }
                }
            }

            // --- Log final state for the turn ---
            std::cout << "    Turn End: " << p.getName() << " has " << p.getChips() << " chips. Pot: " << this->pot << "." << std::endl;


            // Check if game ended after this player's turn
            if (!keepPlay()) break;
        } // End player turn loop
    } // End game loop (while keepPlay)

    // Game over: Find and announce the winner
    std::cout << "\n--- Game Over! ---" << std::endl;
    Player* winner = nullptr;
    for (Player &p : this->players) {
        if (p.getChips() > 0) {
            winner = &p;
            break;
        }
    }

    if (winner) {
        std::cout << winner->getName() << " is the last player with chips!" << std::endl;
        if (this->pot > 0) {
            std::cout << "Adding " << this->pot << " chips from the pot." << std::endl;
            winner->addChips(this->pot);
            this->pot = 0; // Clear the pot
        }
        std::cout << winner->getName() << " wins with " << winner->getChips() << " chips!" << std::endl;
    } else {
        std::cout << "No winner? All players ran out of chips simultaneously. Pot remains with " << this->pot << " chips." << std::endl;
    }
}

#endif //LCR_GAME_H