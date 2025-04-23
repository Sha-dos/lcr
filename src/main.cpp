#include <iomanip>
#include <fstream>
#include "../include/game.h"
#include "../include/json.hpp"

using nlohmann::json;

int main() {
    int numPlayers;
    int numSimulations;
    const int STARTING_CHIPS = 3;
    int i_specialStrategy;
    Player::PlayStyle specialStrategy = Player::PlayStyle::StealOppositeConditional;
    int i_defaultStrategy;
    Player::PlayStyle defaultStrategy = Player::PlayStyle::StealOppositeConditional;

    int barWidth = 70;

    // --- Get Simulation Parameters ---
    std::cout << "LCR Strategy Simulation" << std::endl;
    std::cout << "=======================" << std::endl;

    std::cout << "Enter the number of players per game (e.g., 3-8): ";
    while (!(std::cin >> numPlayers) || numPlayers < 2) {
        std::cout << "Invalid input. Please enter a whole number >= 2: ";
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }

    std::cout << "Enter the number of simulations per strategy (e.g., 1000): ";
    while (!(std::cin >> numSimulations) || numSimulations < 1) {
        std::cout << "Invalid input. Please enter a positive whole number: ";
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }

    std::cout << "Special Strategy (one player will have it)" << std::endl;
    std::cout << "1. Steal From Highest" << std::endl;
    std::cout << "2. Steal From Lowest" << std::endl;
    std::cout << "3. Steal From Opposite" << std::endl;
    std::cout << "4. Steal Opposite Conditional" << std::endl;
    std::cout << "Enter the number corresponding to the special strategy (1-4): ";
    while (!(std::cin >> i_specialStrategy)) {
        std::cout << "Invalid input. Please enter a positive whole number: ";
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }

    switch (i_specialStrategy) {
        case 1:
            specialStrategy = Player::PlayStyle::StealFromHighest;
            break;
        case 2:
            specialStrategy = Player::PlayStyle::StealFromLowest;
            break;
        case 3:
            specialStrategy = Player::PlayStyle::StealFromOpposite;
            break;
        case 4:
            specialStrategy = Player::PlayStyle::StealOppositeConditional;
            break;
        default:
            std::cout << "Invalid choice. Defaulting to Steal Opposite Conditional." << std::endl;
            specialStrategy = Player::PlayStyle::StealOppositeConditional;
            break;
    }

    std::cout << "Default Strategy (other players will have it)" << std::endl;
    std::cout << "1. Steal From Highest" << std::endl;
    std::cout << "2. Steal From Lowest" << std::endl;
    std::cout << "3. Steal From Opposite" << std::endl;
    std::cout << "4. Steal Opposite Conditional" << std::endl;
    std::cout << "Enter the number corresponding to the special strategy (1-4): ";
    while (!(std::cin >> i_defaultStrategy)) {
        std::cout << "Invalid input. Please enter a positive whole number: ";
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }

    switch (i_defaultStrategy) {
        case 1:
            defaultStrategy = Player::PlayStyle::StealFromHighest;
            break;
        case 2:
            defaultStrategy = Player::PlayStyle::StealFromLowest;
            break;
        case 3:
            defaultStrategy = Player::PlayStyle::StealFromOpposite;
            break;
        case 4:
            defaultStrategy = Player::PlayStyle::StealOppositeConditional;
            break;
        default:
            std::cout << "Invalid choice. Defaulting to Steal Opposite Conditional." << std::endl;
            defaultStrategy = Player::PlayStyle::StealOppositeConditional;
            break;
    }


    // --- Run Simulations ---
    std::vector<Result> allResults;
    allResults.reserve(numSimulations * 4); // Reserve space for all results (4 strategies)

    // Define the strategies to test
    std::vector<Player::PlayStyle> strategiesToTest = {
            Player::PlayStyle::StealFromHighest,
            Player::PlayStyle::StealFromLowest,
            Player::PlayStyle::StealFromOpposite,
            Player::PlayStyle::StealOppositeConditional
    };

    int totalGamesRun = 0;
    std::cout << "\nRunning simulations..." << std::endl;

    std::vector<Player> players;

    for (int j = 0; j < numPlayers; ++j) {
        if (j == 0) {
            // First player gets the special strategy
            players.push_back(Player("Player " + std::to_string(j + 1), STARTING_CHIPS, j, specialStrategy, numPlayers));
        } else {
            // Other players get the default strategy
            players.push_back(Player("Player " + std::to_string(j + 1), STARTING_CHIPS, j, defaultStrategy, numPlayers));
        }
    }

    for (int i = 0; i < numSimulations; ++i) {
        try {
            Game lcrGame(players);
            // Play the game and store the result
            allResults.push_back(lcrGame.play(totalGamesRun++));
        } catch (const std::exception& e) {
            std::cerr << "Error during simulation " << totalGamesRun << ": " << e.what() << std::endl;
            // Decide whether to stop or continue
            // return 1; // Stop on error
        }

        std::cout << "[";
        double progress = static_cast<double>(totalGamesRun) / numSimulations;
        int pos = static_cast<int>(barWidth * progress);
        for (int i = 0; i < barWidth; ++i) {
            if (i < pos) std::cout << "=";
            else if (i == pos) std::cout << ">";
            else std::cout << " ";
        }
        std::cout << "] " << static_cast<int>(progress * 100.0) << " %\r";
        std::cout.flush();
    }

    std::cout << std::endl;

    std::cout << "\nSimulations complete. Total games run: " << totalGamesRun << std::endl;

    int stealFromHighestWon = 0;
    int stealFromLowestWon = 0;
    int stealFromOppositeWon = 0;
    int stealOppositeConditionalWon = 0;

    for (Result r : allResults) {
        switch (r.winnerStrategy) {
            case Player::PlayStyle::StealFromHighest:
                stealFromHighestWon++;
                break;
            case Player::PlayStyle::StealFromLowest:
                stealFromLowestWon++;
                break;
            case Player::PlayStyle::StealFromOpposite:
                stealFromOppositeWon++;
                break;
            case Player::PlayStyle::StealOppositeConditional:
                stealOppositeConditionalWon++;
                break;
        }
    }

    std::cout << "Steal From Highest Wins: " << stealFromHighestWon << std::endl;
    std::cout << "Steal From Lowest Wins: " << stealFromLowestWon << std::endl;
    std::cout << "Steal From Opposite Wins: " << stealFromOppositeWon << std::endl;
    std::cout << "Steal Opposite Conditional Wins: " << stealOppositeConditionalWon << std::endl;

//    // --- Export Results to JSON ---
//    std::string outputFilename = "lcr_simulation_results.json";
//    std::cout << "Exporting results to " << outputFilename << "..." << std::endl;
//
//    try {
//        json resultsJson = allResults; // Use the nlohmann magic to convert vector<Result> to json array
//
//        std::ofstream outFile(outputFilename);
//        if (!outFile.is_open()) {
//            throw std::runtime_error("Could not open file for writing: " + outputFilename);
//        }
//        // Write the JSON to the file with pretty printing (indentation)
//        outFile << std::setw(4) << resultsJson << std::endl;
//        outFile.close();
//        std::cout << "Results successfully exported." << std::endl;
//
//    } catch (const json::exception& e) {
//        std::cerr << "JSON Error: " << e.what() << std::endl;
//        return 1;
//    } catch (const std::exception& e) {
//        std::cerr << "File I/O Error: " << e.what() << std::endl;
//        return 1;
//    }
//
//    // --- Basic Analysis (Optional) ---
//    std::cout << "\nBasic Win Analysis:" << std::endl;
//    std::map<Player::PlayStyle, int> winCounts;
//    int drawCount = 0;
//    for (const auto& result : allResults) {
//        if (result.draw) {
//            drawCount++;
//        } else {
//            // Assumption: In these simulations, all players in a game had the same strategy as the winner.
//            winCounts[result.winnerStrategy]++;
//        }
//    }
//
//    for (const auto& pair : winCounts) {
//        std::cout << "  - " << Player::playStyleToString(pair.first) << ": "
//                  << pair.second << " wins ("
//                  << (double)pair.second / numSimulations * 100.0 << "%)" << std::endl;
//    }
//    if (drawCount > 0) {
//        std::cout << "  - Draws: " << drawCount << std::endl;
//    }


    return 0;
}
