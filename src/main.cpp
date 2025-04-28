#include <iomanip>
#include <fstream>
#include <sstream>
#include "../include/game.h"
#include "../include/json.hpp"
#include "../include/output.h"
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <queue>
#include <functional>
#include "../include/threadPool.h"

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

    // --- Random ---
    std::random_device rd;
    std::mt19937 rng(rd());

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
                    std::uniform_int_distribution<int> dist(0, Player::PlayStyle::StealOppositeConditional);
                    strategy = static_cast<Player::PlayStyle>(dist(rng)); // Random strategy
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
    allResults.reserve(numSimulations * (replayCount + 1)); // Reserve space for all results
    std::mutex results_mutex; // Protect access to allResults

    std::atomic<int> totalGamesRun{0};
    std::cout << "\nRunning simulations..." << std::endl;

    std::rotate(players.begin(), players.begin() + startingPlayer - 1, players.end());

    int totalSimulations = numSimulations * (replayCount + 1);
    int maxThreads = std::thread::hardware_concurrency(); // Use available CPU cores
    maxThreads = maxThreads > 0 ? maxThreads : 4; // Fallback if detection fails

    ThreadPool pool(maxThreads);

// Create atomic counters for tracking wins by strategy
    std::atomic<int> winsStealFromHighest{0};
    std::atomic<int> winsStealFromLowest{0};
    std::atomic<int> winsStealFromOpposite{0};
    std::atomic<int> winsStealOppositeConditional{0};

    std::mutex playerMutex; // For updating player win counts

// Submit all tasks to thread pool
    for (int j = 0; j <= replayCount; ++j) {
        std::vector<Player> replayPlayers = players;

        // If replay count > 0, randomize strategies at each replay
        if (j > 0 && replayCount > 0) {
            for (Player &p : replayPlayers) {
                if (p.randomStrategy) {
                    std::uniform_int_distribution<int> dist(0, Player::PlayStyle::StealOppositeConditional);
                    p.setStrategy(static_cast<Player::PlayStyle>(dist(rng)));
                }
            }
        }

        for (int i = 0; i < numSimulations; ++i) {
            pool.enqueue([&, replayPlayers]() {
                try {
                    Game lcrGame((std::vector<Player>(replayPlayers)));
                    int gameId = totalGamesRun.fetch_add(1);

                    // Play the game and store the result
                    Result result = lcrGame.play(gameId);

                    // Update strategy win counts
                    if (result.winnerStrategy == Player::PlayStyle::StealFromHighest) {
                        winsStealFromHighest++;
                    } else if (result.winnerStrategy == Player::PlayStyle::StealFromLowest) {
                        winsStealFromLowest++;
                    } else if (result.winnerStrategy == Player::PlayStyle::StealFromOpposite) {
                        winsStealFromOpposite++;
                    } else if (result.winnerStrategy == Player::PlayStyle::StealOppositeConditional) {
                        winsStealOppositeConditional++;
                    }

                    {
                        std::lock_guard<std::mutex> lock(playerMutex);
                        // Update player win count
                        auto it = std::find_if(players.begin(), players.end(), [&result](const Player &p) {
                            return p.getName() == result.winnerName;
                        });

                        if (it != players.end()) {
                            it->addWin();
                        } else {
                            // Probably a draw
                            // If a winner from a simulation isn't found in the main player list. This indicates a potential logic issue.
//                            std::cerr << "Error: Winner '" << result.winnerName << "' from game "
//                                      << result.gameId << " not found in main player list!" << std::endl;
                        }
                    }

                    {
                        std::lock_guard<std::mutex> lock(results_mutex);
                        allResults.push_back(result);
                    }
                } catch (const std::exception &e) {
                    std::cerr << "Error during simulation: " << e.what() << std::endl;
                }
            });
        }
    }

// Progress bar update thread
    std::thread progressThread([&]() {
        using namespace std::chrono;
        auto startTime = high_resolution_clock::now();
        long long lastGamesRun = 0;
        double simsPerSecond = 0.0;

        while (true) {
            long long currentGamesRun = totalGamesRun.load(std::memory_order_relaxed);
            if (currentGamesRun >= totalSimulations) {
                break;
            }

            auto now = high_resolution_clock::now();
            duration<double> elapsedSinceStart = now - startTime;
            double totalElapsedSeconds = elapsedSinceStart.count();

            if (totalElapsedSeconds > 0.5) { // Start calculating after a short delay
                simsPerSecond = static_cast<double>(currentGamesRun) / totalElapsedSeconds;
            }

            // Calculate ETR
            std::string etrString = "Calculating...";
            if (simsPerSecond > 0.1) { // Avoid ETR calculation if rate is too low/unstable
                long long remainingGames = totalSimulations - currentGamesRun;
                double etrSeconds = static_cast<double>(remainingGames) / simsPerSecond;

                if (etrSeconds >= 0) {
                    long long etrSecsInt = static_cast<long long>(std::round(etrSeconds));
                    long long etrMins = etrSecsInt / 60;
                    etrSecsInt %= 60;
                    std::ostringstream etrStream;
                    etrStream << std::setfill('0') << std::setw(2) << etrMins << ":"
                              << std::setfill('0') << std::setw(2) << etrSecsInt;
                    etrString = etrStream.str();
                } else {
                    etrString = "Done soon";
                }
            }

            // Calculate overall progress
            double progress = static_cast<double>(currentGamesRun) / totalSimulations;
            int pos = static_cast<int>(barWidth * progress);

            // --- Display ---
            // Clear terminal and reset cursor
            std::cout << "\033[H\033[J";

            // Overall Progress Bar
            std::cout << "Overall Progress: [" << std::flush;
            for (int i = 0; i < barWidth; ++i) {
                if (i < pos) std::cout << "=" << std::flush;
                else if (i == pos) std::cout << ">" << std::flush;
                else std::cout << " " << std::flush;
            }
            std::cout << "] " << static_cast<int>(progress * 100.0) << "% " << std::flush;
            std::cout << "(" << currentGamesRun << "/" << totalSimulations << ")\n" << std::flush;

            // Stats
            std::cout << std::fixed << std::setprecision(1); // For sims/sec formatting
            std::cout << "Rate: " << simsPerSecond << " sims/sec | ETR: " << etrString << "\n" << std::flush;
            std::cout << "Threads: " << pool.getActiveTasks() << " active / " << maxThreads
                      << " | Queue: " << pool.getQueueSize() << "\n\n" << std::flush;

            // Live Strategy Wins
            std::cout << "Current Wins by Strategy:\n" << std::flush;
            std::cout << "  Steal From Highest:         " << winsStealFromHighest.load(std::memory_order_relaxed) << "\n" << std::flush;
            std::cout << "  Steal From Lowest:          " << winsStealFromLowest.load(std::memory_order_relaxed) << "\n" << std::flush;
            std::cout << "  Steal From Opposite:        " << winsStealFromOpposite.load(std::memory_order_relaxed) << "\n" << std::flush;
            std::cout << "  Steal Opposite Conditional: " << winsStealOppositeConditional.load(std::memory_order_relaxed) << "\n" << std::flush;

            // Update interval
            std::this_thread::sleep_for(std::chrono::milliseconds(200)); // Update 5 times/sec
        }

        // --- Final 100% display ---
        std::cout << "\033[H\033[J";
        std::cout << "Overall Progress: [" << std::flush;
        for (int i = 0; i < barWidth; ++i) {
            std::cout << "=" << std::flush;
        }
        std::cout << "] 100% (" << totalGamesRun.load(std::memory_order_relaxed) << "/" << totalSimulations << ")\n" << std::flush;

        std::cout.flush();
    });

// Wait for all tasks to complete
    while(totalGamesRun < totalSimulations) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

// Complete the progress bar
    if (progressThread.joinable()) {
        progressThread.join();
    }

    std::cout << std::endl;

    std::cout << "\nSimulations complete. Total games run: " << totalGamesRun << std::endl;

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