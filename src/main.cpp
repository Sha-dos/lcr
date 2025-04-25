#include <iomanip>
#include <fstream>
#include <sstream>
#include "../include/game.h"
#include "../include/json.hpp"
#include "../include/output.h"

using nlohmann::json;

int main(int argc, char* argv[]) {
//    std::vector<Player> testPlayers = {
//        Player("Player 1", 0, 0, Player::PlayStyle::StealFromHighest, 3),
//        Player("Player 2", 1, 1, Player::PlayStyle::StealFromHighest, 3),
//        Player("Player 3", 0, 2, Player::PlayStyle::StealFromHighest, 3)
//    };
//
//    Game game(testPlayers);
//
//    game.play(0);
//
//    return 0;

    // --- Initialize Game Parameters ---
    int numPlayers;
    int numSimulations;
    const int STARTING_CHIPS = 3;
    int i_specialStrategy;
    Player::PlayStyle specialStrategy = Player::PlayStyle::StealOppositeConditional;
    int i_defaultStrategy;
    Player::PlayStyle defaultStrategy = Player::PlayStyle::StealOppositeConditional;
    Output::OutputType outputType = Output::OutputType::All;
    int replayCount = 0;

    int barWidth = 70;

    std::vector<Player> players;

    int startingPlayer = 1;

    std::string jsonFilePath = argv[1];
    std::ifstream jsonFile(jsonFilePath);
    if (jsonFile.is_open()) {
        try {
            json configData;
            jsonFile >> configData;

            // Read the number of simulations
            numSimulations = configData.at("numSimulations").get<int>();

            startingPlayer = configData.at("startingPlayer").get<int>();

            outputType = Output::stringToOutputType(configData.at("outputType").get<std::string>());

            replayCount = configData.at("replayCount").get<int>();

            // Read the players array
            int index = 0;
            for (const auto& player : configData.at("players")) {
                std::string name = player.at("name").get<std::string>();
                int chips = player.at("chips").get<int>();
                Player::PlayStyle strategy;

                bool randomStrategy = false;
                if (player.at("strategy").get<int>() == -1) {
                    randomStrategy = true;
                    strategy = static_cast<Player::PlayStyle>(rand() % (Player::PlayStyle::StealOppositeConditional + 1)); // Random strategy
                } else {
                    strategy = static_cast<Player::PlayStyle>(player.at("strategy").get<int>() - 1);
                }

                int totalPlayers = player.at("totalPlayers").get<int>();

                players.emplace_back(name, chips, index, strategy, totalPlayers, randomStrategy);

                index++;
            }

            std::cout << "Imported " << players.size() << " players and " << numSimulations << " simulations from JSON file." << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Error parsing JSON file: " << e.what() << std::endl;
            return 1;
        }
    }

    // --- Run Simulations ---
    std::vector<Result> allResults;
    allResults.reserve(numSimulations); // Reserve space for results

    int totalGamesRun = 0;
    std::cout << "\nRunning simulations..." << std::endl;

    std::rotate(players.begin(), players.begin() + startingPlayer - 1, players.end());
    std::cout << "Starting player: " << players[0].getName() << std::endl;

    for (int j = 0; j <= replayCount; ++j) {
        for (int i = 0; i < numSimulations; ++i) {
            try {
                Game lcrGame(std::vector<Player>(players.begin(), players.end()));

                // Play the game and store the result
                Result result = lcrGame.play(totalGamesRun++);

                std::find_if(players.begin(), players.end(), [&result](const Player &p) {
                    return p.getName() == result.winnerName;
                })->addWin();

                allResults.push_back(result);
            } catch (const std::exception &e) {
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

        if (replayCount > 0) {
            for (Player &p : players) {
                if (p.randomStrategy) {
                    p.setStrategy(static_cast<Player::PlayStyle>(rand() % (Player::PlayStyle::StealOppositeConditional + 1)));
                }
            }
        }
    }

    std::cout << std::endl;

    std::cout << "\nSimulations complete. Total games run: " << totalGamesRun << std::endl;

    int winsStealFromHighest = 0;
    int winsStealFromLowest = 0;
    int winsStealFromOpposite = 0;
    int winsStealOppositeConditional = 0;

    for (Result r : allResults) {
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
    for (const Player& player : players) {
        std::cout << "  " << player.getName() << ": " << player.getWins() << " wins (" << Player::playStyleToString(player.getPlayStyle()) << ")" << std::endl;
    }
    // --- Export Results to JSON ---
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

    // --- Export Results to CSV ---
    std::string outputFilename = "lcr_simulation_results.csv";
    try {
        std::cout << "Exporting results to CSV..." << std::endl;

        bool writeHeader = false;
        if (!std::filesystem::exists(outputFilename)) {
            writeHeader = true;
        }

        std::ofstream outFile(outputFilename, std::ios::app);

        if (!outFile.is_open()) {
            throw std::runtime_error("Could not open file for writing: " + outputFilename);
        }

        switch (outputType) {
            case Output::OutputType::All:
                if (writeHeader) {
                    outFile << "gameId,winnerName,winnerStrategy,numberOfRounds,numberOfPlayers,initialChipsPerPlayer" << std::endl;
                }

                for (const Result& result : allResults) {
                    outFile << result.gameId << ","
                            << result.winnerName << ","
                            << Player::playStyleToString(result.winnerStrategy) << ","
                            << result.numberOfRounds << ","
                            << result.numberOfPlayers << ","
                            << result.initialChipsPerPlayer
                            << std::endl;
                }
                break;
            case Output::OutputType::Totals:
                if (writeHeader) {
                    outFile << "Highest,Lowest,Opposite, Opposite Conditional" << std::endl;
                }

                outFile << winsStealFromHighest << ","
                        << winsStealFromLowest << ","
                        << winsStealFromOpposite << ","
                        << winsStealOppositeConditional
                        << std::endl;
                break;
        }

        outFile.close();
        std::cout << "Results successfully exported to CSV." << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "File I/O Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}