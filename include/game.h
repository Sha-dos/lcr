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
#include <map>
#include "player.h"
#include "dice.h"
#include "helpers.h"
#include "result.h" // Include the new Result class definition

class Game {
private:
    std::vector<Player> players;
    int pot;
    int numOfPlayers;
    int initialChips; // Store initial chips per player
    std::vector<Player::PlayStyle> initialStrategies; // Store initial strategies

    bool keepPlay();

public:
    Game(int numPlayers, int startingChips = 3, Player::PlayStyle defaultStyle = Player::PlayStyle::StealFromOpposite);
    // Constructor allowing mixed strategies
    Game(const std::vector<Player::PlayStyle>& strategies, int startingChips = 3);
    // Constructor taking full player objects (less used now but kept for flexibility)
    Game(std::vector<Player> initialPlayers);

    // Play the game and return the result
    Result play(int gameId); // Takes gameId for result tracking
    int getNumOfPlayers() const { return numOfPlayers; }
};

// Constructor implementation (uniform style)
Game::Game(int numPlayers, int startingChips, Player::PlayStyle defaultStyle)
        : numOfPlayers(numPlayers), pot(0), initialChips(startingChips) {
    if (numPlayers < 2) throw std::invalid_argument("Game requires at least 2 players.");
    if (startingChips <= 0) throw std::invalid_argument("Players must start with chips.");

    initialStrategies.reserve(numPlayers);
    players.reserve(numPlayers);
    for (int i = 0; i < numPlayers; ++i) {
        std::string name = "Player ";
        name += std::to_string(i + 1);
        players.emplace_back(name, startingChips, i, defaultStyle, numPlayers);
        initialStrategies.push_back(defaultStyle);
    }
    // std::cout << "Game created with " << numPlayers << " players, " << startingChips << " chips each, using strategy: "
    //           << Player::playStyleToString(defaultStyle) << std::endl; // Verbose logging removed for bulk runs
}

// Constructor implementation (mixed styles)
Game::Game(const std::vector<Player::PlayStyle>& strategies, int startingChips)
        : numOfPlayers(strategies.size()), pot(0), initialChips(startingChips), initialStrategies(strategies) {
    if (numOfPlayers < 2) throw std::invalid_argument("Game requires at least 2 players.");
    if (startingChips <= 0) throw std::invalid_argument("Players must start with chips.");

    players.reserve(numOfPlayers);
    for (int i = 0; i < numOfPlayers; ++i) {
        std::string name = "Player ";
        name += std::to_string(i + 1);
        players.emplace_back(name, startingChips, i, strategies[i], numOfPlayers);
    }
    // std::cout << "Game created with " << numOfPlayers << " players, " << startingChips << " chips each, with mixed strategies." << std::endl;
}


// Constructor implementation (taking vector of Player objects)
Game::Game(std::vector<Player> initialPlayers) : players(std::move(initialPlayers)), pot(0) {
    this->numOfPlayers = players.size();
    if (this->numOfPlayers < 2) throw std::invalid_argument("Game requires at least 2 players.");
    // Infer initial state (assuming uniform start for simplicity here)
    this->initialChips = (players.empty() ? 0 : players[0].getChips());
    this->initialStrategies.reserve(numOfPlayers);
    for(const auto& p : players) {
        this->initialStrategies.push_back(p.getPlayStyle());
        // Basic check for consistency - real use might need more robust handling
        if (p.getChips() != this->initialChips) {
            std::cerr << "Warning: Constructing Game from players with non-uniform starting chips." << std::endl;
        }
    }
    // std::cout << "Game created from pre-defined player vector." << std::endl;
}


// keepPlay implementation
bool Game::keepPlay() {
    int count = 0;
    for (const Player& p : players) { if (p.getChips() > 0) { count++; } }
    return count >= 1;
}

// play implementation - Now returns a Result object
Result Game::play(int gameId) {
    std::vector<std::vector<int>> chipHistory;
    std::vector<int> initialState;
    for (const auto& player : players) {
        initialState.push_back(player.getChips());
    }
    chipHistory.push_back(initialState);

    int round = 0; // Start at round 0, increment at start of loop
    while (keepPlay()) {
        round++;

        // Check if only one player has chips, if so, they need to roll all dots or wilds
        if (std::count_if(players.begin(), players.end(), [](const Player& p) { return p.getChips() > 0; }) == 1) {
            for (Player& p : players) {
                if (p.getChips() > 0) {
                    int numOfRolls = std::min(p.getChips(), 3);
                    std::vector<Dice::Side> rollResults;
                    rollResults.reserve(numOfRolls);

                    for (int x = 0; x < numOfRolls; ++x) {
                       rollResults.push_back(Dice::roll());
                    }

//                    std::cout << Dice::sideToString(rollResults.front()) << std::endl;

                    // Check if all rolls are dots or wilds
                    bool allDotsOrWilds = std::all_of(rollResults.begin(), rollResults.end(), [](Dice::Side side) {
                        return side == Dice::Dot || side == Dice::Wild;
                    });

                    if (allDotsOrWilds) {
                        return Result(gameId, p.getName(), p.getPlayStyle(), round, numOfPlayers, initialChips, initialStrategies, chipHistory);
                    } else {
                        break;
                    }
                }
            }
        }

        // std::cout << "\n--- Round " << round << " ---" << std::endl; // Verbose logging removed
        for (int i = 0; i < numOfPlayers; ++i) {
            Player &p = players[i];
            if (!keepPlay()) break;
            if (p.getChips() == 0) continue;

            // std::cout << "\n" << p.getName() << "'s turn (Chips: " << p.getChips() << ")" << std::endl; // Verbose

            int numOfRolls = std::min(p.getChips(), 3);
            if (numOfRolls == 0) continue;
            // std::cout << "  Rolling " << numOfRolls << " dice: "; // Verbose

            std::map<Dice::Side, int> rollCounts;
            for (int j = 0; j < numOfRolls; ++j) {
                Dice::Side result = Dice::roll();
                rollCounts[result]++;
            }
            // std::cout << std::endl; // Verbose

            int netPassLeft = rollCounts[Dice::L];
            int netPassRight = rollCounts[Dice::R];
            int netToPot = rollCounts[Dice::C];
            int netWilds = rollCounts[Dice::Wild];
            int stealsToAttempt = 0;
            int chipsKeptFromCancellation = 0;

            if (netWilds > 0) {
                switch (p.getPlayStyle()) {
                    case Player::PlayStyle::StealFromHighest:
                    case Player::PlayStyle::StealFromLowest:
                        stealsToAttempt = netWilds;
                        // std::cout << "    (Wild logic: Always Steal)" << std::endl; // Verbose
                        break;
                    case Player::PlayStyle::StealFromOpposite: { // W cancels C > L > R
                        // std::cout << "    (Wild logic: Opposite C>L>R)" << std::endl; // Verbose
                        int currentWilds = netWilds;
                        int cancelC = std::min(currentWilds, netToPot); if (cancelC > 0) { currentWilds -= cancelC; netToPot -= cancelC; chipsKeptFromCancellation += cancelC; }
                        int cancelL = std::min(currentWilds, netPassLeft); if (cancelL > 0) { currentWilds -= cancelL; netPassLeft -= cancelL; chipsKeptFromCancellation += cancelL; }
                        int cancelR = std::min(currentWilds, netPassRight); if (cancelR > 0) { currentWilds -= cancelR; netPassRight -= cancelR; chipsKeptFromCancellation += cancelR; }
                        stealsToAttempt = currentWilds;
                        break;
                    }
                    case Player::PlayStyle::StealOppositeConditional: { // W cancels C only
                        // std::cout << "    (Wild logic: Opposite Conditional C)" << std::endl; // Verbose
                        int currentWilds = netWilds;
                        int cancelC = std::min(currentWilds, netToPot); if (cancelC > 0) { currentWilds -= cancelC; netToPot -= cancelC; chipsKeptFromCancellation += cancelC; }
                        stealsToAttempt = currentWilds;
                        break;
                    }
                } // End switch(PlayStyle)
            } // End if (netWilds > 0)

            int chipsAvailable = p.getChips();
            int chipsToRemoveTotal = 0;

            int actualPassLeft = std::min(netPassLeft, chipsAvailable - chipsToRemoveTotal);
            if (actualPassLeft > 0) { int leftIdx = Helpers::calculateNeededPlayerIndex(numOfPlayers, i, Helpers::Direction::Left); players.at(leftIdx).addChips(actualPassLeft); chipsToRemoveTotal += actualPassLeft; }
            int actualToPot = std::min(netToPot, chipsAvailable - chipsToRemoveTotal);
            if (actualToPot > 0) { this->pot += actualToPot; chipsToRemoveTotal += actualToPot; }
            int actualPassRight = std::min(netPassRight, chipsAvailable - chipsToRemoveTotal);
            if (actualPassRight > 0) { int rightIdx = Helpers::calculateNeededPlayerIndex(numOfPlayers, i, Helpers::Direction::Right); players.at(rightIdx).addChips(actualPassRight); chipsToRemoveTotal += actualPassRight; }

            if (chipsToRemoveTotal > 0) { p.removeChips(chipsToRemoveTotal); }

            // --- Attempt Steals ---
            if (stealsToAttempt > 0) {
                // std::cout << "    Attempting " << stealsToAttempt << " steal(s)..." << std::endl; // Verbose
                for (int k = 0; k < stealsToAttempt; ++k) {
                    p.attemptSteal(players); // Ignore return value for bulk runs, just attempt
                }
            }
            // std::cout << "    Turn End: " << p.getName() << " has " << p.getChips() << " chips. Pot: " << this->pot << "." << std::endl; // Verbose
            std::vector<int> currentState;
            for (const auto& player : players) {
                currentState.push_back(player.getChips());
            }
            chipHistory.push_back(currentState);
        } // End player turn loop
    } // End game loop

    // Game over: Determine winner and create Result
    Player* winner = nullptr;
    int winnerCount = 0;
    for (Player &p : this->players) {
        if (p.getChips() > 0) {
            winner = &p;
            winnerCount++;
        }
    }

    if (winner && winnerCount == 1) {
        // Normal win
        winner->addChips(this->pot); // Winner gets the pot
        // std::cout << "\n--- Game Over! Winner: " << winner->getName() << " ---" << std::endl; // Verbose
        return Result(gameId, winner->getName(), winner->getPlayStyle(), round, numOfPlayers, initialChips, initialStrategies, chipHistory, false);
    } else {
        // Draw or unexpected state
        // std::cout << "\n--- Game Over! Draw or Error ---" << std::endl; // Verbose
        // In a draw, pot is lost? Or split? We'll assume lost for now.
        // Return a result indicating a draw, using a placeholder strategy or the first player's strategy.
        Player::PlayStyle placeholderStrat = initialStrategies.empty() ? Player::PlayStyle::StealFromHighest : initialStrategies[0];
        return Result(gameId, "DRAW", placeholderStrat, round, numOfPlayers, initialChips, initialStrategies, chipHistory, true);
    }
}

#endif //LCR_GAME_H