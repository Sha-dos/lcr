#include <iomanip>
#include <fstream>
#include <sstream>
#include "../include/game.h"
#include "../include/json.hpp"

using nlohmann::json;

int main(int argc, char* argv[]) {
    std::vector<Player> testPlayers = {
        Player("Player 1", 0, 0, Player::PlayStyle::StealFromHighest, 3),
        Player("Player 2", 1, 1, Player::PlayStyle::StealFromHighest, 3),
        Player("Player 3", 0, 2, Player::PlayStyle::StealFromHighest, 3)
    };

    Game game(testPlayers);

    game.play(0);

    return 0;

    // --- Initialize Game Parameters ---
    int numPlayers;
    int numSimulations;
    const int STARTING_CHIPS = 3;
    int i_specialStrategy;
    Player::PlayStyle specialStrategy = Player::PlayStyle::StealOppositeConditional;
    int i_defaultStrategy;
    Player::PlayStyle defaultStrategy = Player::PlayStyle::StealOppositeConditional;

    int barWidth = 70;

    std::vector<Player> players;

    int startingPlayer = 1;

    // --- Check for JSON Import ---
    if (argc > 1) {
        std::string jsonFilePath = argv[1];
        std::ifstream jsonFile(jsonFilePath);
        if (jsonFile.is_open()) {
            try {
                json configData;
                jsonFile >> configData;

                // Read the number of simulations
                numSimulations = configData.at("numSimulations").get<int>();

                startingPlayer = configData.at("startingPlayer").get<int>();

                // Read the players array
                int index = 0;
                for (const auto& player : configData.at("players")) {
                    std::string name = player.at("name").get<std::string>();
                    int chips = player.at("chips").get<int>();
                    Player::PlayStyle strategy = static_cast<Player::PlayStyle>(player.at("strategy").get<int>());
                    int totalPlayers = player.at("totalPlayers").get<int>();

                    players.emplace_back(name, chips, index, strategy, totalPlayers);

                    index++;
                }

                std::cout << "Imported " << players.size() << " players and " << numSimulations << " simulations from JSON file." << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "Error parsing JSON file: " << e.what() << std::endl;
                return 1;
            }
        }
    } else {
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

        for (int j = 0; j < numPlayers; ++j) {
            if (j == 0) {
                // First player gets the special strategy
                players.push_back(Player("Player " + std::to_string(j + 1), STARTING_CHIPS, j, specialStrategy, numPlayers));
            } else {
                // Other players get the default strategy
                players.push_back(Player("Player " + std::to_string(j + 1), STARTING_CHIPS, j, defaultStrategy, numPlayers));
            }
        }
    }

    // --- Run Simulations ---
    std::vector<Result> allResults;
    allResults.reserve(numSimulations * 4); // Reserve space for all results (4 strategies)

    int totalGamesRun = 0;
    std::cout << "\nRunning simulations..." << std::endl;

    std::rotate(players.begin(), players.begin() + startingPlayer - 1, players.end());
    std::cout << "Starting player: " << players[0].getName() << std::endl;

    for (int i = 0; i < numSimulations; ++i) {
        try {
            Game lcrGame(players);
            // Play the game and store the result
            allResults.push_back(lcrGame.play(totalGamesRun++));
        } catch (const std::exception& e) {
            std::cerr << "Error during simulation " << totalGamesRun << ": " << e.what() << std::endl;
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

    int winsStealFromHighest = 0;
    int winsStealFromLowest = 0;
    int winsStealFromOpposite = 0;
    int winsStealOppositeConditional = 0;
    std::map<std::string, int> playerWins; // Map to store wins for each player

    for (Result r : allResults) {
        if (!r.draw) { // Only count wins if the game was not a draw
            playerWins[r.winnerName]++;
        }

        if (r.winnerStrategy == Player::PlayStyle::StealFromHighest) {
            winsStealFromHighest++;
        } else if (r.winnerStrategy == Player::PlayStyle::StealFromLowest) {
            winsStealFromLowest++;
        } else if (r.winnerStrategy == Player::PlayStyle::StealFromOpposite) {
            winsStealFromOpposite++;
        } else if (r.winnerStrategy == Player::PlayStyle::StealOppositeConditional) {
            winsStealOppositeConditional++;
        }
    }

    std::cout << "Wins by strategy:" << std::endl;
    std::cout << "  Steal From Highest: " << winsStealFromHighest << std::endl;
    std::cout << "  Steal From Lowest: " << winsStealFromLowest << std::endl;
    std::cout << "  Steal From Opposite: " << winsStealFromOpposite << std::endl;
    std::cout << "  Steal Opposite Conditional: " << winsStealOppositeConditional << std::endl;

    std::cout << "\nWins by player:" << std::endl;
    for (const auto& [playerName, wins] : playerWins) {
        std::cout << "  " << playerName << ": " << wins << " wins" << std::endl;
    }

    // --- Export Results to JSON ---
    std::string outputFilename = "lcr_simulation_results.json";
    std::cout << "Exporting results to " << outputFilename << "..." << std::endl;

    try {
        json resultsJson = allResults; // Use the nlohmann magic to convert vector<Result> to json array

        std::ofstream outFile(outputFilename);
        if (!outFile.is_open()) {
            throw std::runtime_error("Could not open file for writing: " + outputFilename);
        }
        // Write the JSON to the file with pretty printing (indentation)
        outFile << std::setw(4) << resultsJson << std::endl;
        outFile.close();
        std::cout << "Results successfully exported." << std::endl;

    } catch (const json::exception& e) {
        std::cerr << "JSON Error: " << e.what() << std::endl;
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "File I/O Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}