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

/**
 * @brief Main function that runs LCR (Left Center Right) game simulations
 *
 * This function can operate in two modes:
 * 1. With JSON configuration file provided as command line argument
 * 2. With default hardcoded parameters if no JSON file is provided
 *
 * The program supports multithreaded simulations with progress tracking,
 * strategy analysis, and CSV output of results.
 *
 * @param argc Number of command line arguments
 * @param argv Array of command line arguments, argv[1] should be JSON config file path
 * @return int Exit status (0 for success, 1 for error)
 */
int main(int argc, char* argv[]) {
    // --- Random ---
    std::random_device rd;
    std::mt19937 rng(rd());

    // -- Timer ---
    auto start = std::chrono::high_resolution_clock::now();

    // --- Initialize Game Parameters ---
    int numSimulations;
    Output::OutputType outputType = Output::OutputType::All;
    int runEachSim;
    bool randomStarter = false;

    int barWidth = 70;

    std::vector<Player> players;

    int startingPlayer = 1;

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

                outputType = Output::stringToOutputType(configData.at("outputType").get<std::string>());

                runEachSim = configData.at("runEachSim").get<int>();

                int totalPlayers = configData.at("totalPlayers").get<int>();

                // Read the players array
                int index = 0;
                for (const auto& player : configData.at("players")) {
                    std::string name = player.at("name").get<std::string>();
                    int chips = player.at("chips").get<int>();
                    Player::PlayStyle strategy;

                    if (player.at("strategy").get<int>() == -1) {
                        strategy = Player::PlayStyle::Random;
                    } else {
                        strategy = static_cast<Player::PlayStyle>(player.at("strategy").get<int>() - 1);
                    }

                    players.emplace_back(name, chips, index, strategy, totalPlayers);

                    index++;
                }

                std::cout << "Imported " << players.size() << " players and " << numSimulations << " simulations from JSON file." << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "Error parsing JSON file: " << e.what() << std::endl;
                return 1;
            }
        } else {
            std::cerr << "Error opening JSON file: " << jsonFilePath << std::endl;
            return 1;
        }
    } else {
        std::cout << "No JSON file provided" << std::endl;

        numSimulations = 10'000;
        startingPlayer = -1;
        randomStarter = false;
        outputType = Output::OutputType::Totals;
        runEachSim = 100;

        players = {
            Player("Player 1", 3, 0, Player::Random, 10),
            Player("Player 2", 3, 1, Player::Random, 10),
            Player("Player 3", 3, 2, Player::Random, 10),
            Player("Player 4", 3, 3, Player::Random, 10),
            Player("Player 5", 3, 4, Player::Random, 10),
            Player("Player 6", 3, 5, Player::Random, 10),
            Player("Player 7", 3, 6, Player::Random, 10),
            Player("Player 8", 3, 7, Player::Random, 10),
            Player("Player 9", 3, 8, Player::Random, 10),
            Player("Player 10", 3, 9, Player::Random, 10),
        };

        players = {
            Player("Player 1", 3, 0, Player::Random, 2),
            Player("Player 2", 3, 1, Player::Random, 2)
        };
    }

    // --- Run Simulations ---
    std::vector<Result> allResults;
    allResults.reserve(numSimulations * (runEachSim)); // Reserve space for all results
    std::mutex results_mutex; // Protect access to allResults

    std::atomic<int> totalGamesRun{0};
    std::cout << "\nRunning simulations..." << std::endl;

    if (startingPlayer < 0) {
        randomStarter = true;
        std::uniform_int_distribution<int> playerDist(1, players.size());
        startingPlayer = playerDist(rng);
    }
    std::rotate(players.begin(), players.begin() + startingPlayer - 1, players.end());

    int totalSimulations = numSimulations * (runEachSim);
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
    for (int i = 0; i < numSimulations; ++i) {
        std::vector<Player> batchPlayers = players;
        int batchStartingPlayer = startingPlayer;

        // Randomize once per batch (not per simulation)
        std::random_device batch_rd;
        std::mt19937 batch_rng(batch_rd());

        // Set random starting player for this batch
        if (randomStarter) {
            std::uniform_int_distribution<int> playerDist(1, players.size());
            batchStartingPlayer = playerDist(batch_rng);
        }

        // Set random strategies for this batch
        for (Player &p : batchPlayers) {
            if (p.getPlayStyle() == Player::PlayStyle::Random) {
                std::uniform_int_distribution<int> dist(0, Player::PlayStyle::StealOppositeConditional);
                p.setStrategy(static_cast<Player::PlayStyle>(dist(batch_rng)));
            }
        }

        for (int j = 0; j < runEachSim; ++j) {
            pool.enqueue([&, batchPlayers, batchStartingPlayer]() {
                try {
                    // Create identical copy for this replay
                    std::vector<Player> simPlayers = batchPlayers;

                    // Apply the same starting player rotation for all replays in this batch
                    if (batchStartingPlayer != startingPlayer) {
                        std::rotate(simPlayers.begin(), simPlayers.begin() + batchStartingPlayer - 1, simPlayers.end());
                    }

                    Game lcrGame(simPlayers);
                    int gameId = totalGamesRun.fetch_add(1);

                    // Play the game and store the result
                    Result result = lcrGame.play(gameId);

                    // Update strategy win counts
                    if (!result.draw) {
                        if (result.winnerStrategy == Player::PlayStyle::StealFromHighest) {
                            winsStealFromHighest++;
                        } else if (result.winnerStrategy == Player::PlayStyle::StealFromLowest) {
                            winsStealFromLowest++;
                        } else if (result.winnerStrategy == Player::PlayStyle::StealFromOpposite) {
                            winsStealFromOpposite++;
                        } else if (result.winnerStrategy == Player::PlayStyle::StealOppositeConditional) {
                            winsStealOppositeConditional++;
                        }
                    }

                    {
                        std::lock_guard<std::mutex> lock(playerMutex);
                        // Update player win count
                        auto it = std::find_if(players.begin(), players.end(), [&result](const Player &p) {
                            return p.getName() == result.winnerName;
                        });

                        if (it != players.end()) {
                            it->addWin();
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

    std::sort(players.begin(), players.end(), [](const Player& a, const Player& b) {
        return a.getIndex() < b.getIndex();
    });

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsedSinceStart = end - start;

    std::cout << "\nSimulations complete. " << totalSimulations << " simulations ran in " << elapsedSinceStart.count() << "s" << std::endl;

    // --- Display Results ---
    std::cout << "\nWins by strategy (sorted by most to least):" << std::endl;

    const int columnWidth = 30;
    const int numberWidth = 8;

    std::vector<std::pair<std::string, int>> strategyWins = {
            {"Steal From Highest", winsStealFromHighest.load()},
            {"Steal From Lowest", winsStealFromLowest.load()},
            {"Steal From Opposite", winsStealFromOpposite.load()},
            {"Steal Opposite Conditional", winsStealOppositeConditional.load()}
    };

    std::sort(strategyWins.begin(), strategyWins.end(), [](const auto& a, const auto& b) {
        return b.second < a.second;
    });

    int totalWins = winsStealFromHighest + winsStealFromLowest + winsStealFromOpposite + winsStealOppositeConditional;
    int totalGames = totalGamesRun.load();
    int draws = totalGames - totalWins;

    for (const auto& [strategy, wins] : strategyWins) {
        double percentage = (totalWins > 0) ? (static_cast<double>(wins) / (totalWins + draws)) * 100.0 : 0.0;
        std::cout << "  " << std::left << std::setw(columnWidth) << strategy
                  << std::setw(numberWidth) << wins << " "
                  << std::fixed << std::setprecision(2) << percentage << "%" << std::endl;
    }

    double drawPercentage = (totalGames > 0) ? (static_cast<double>(draws) / totalGames) * 100.0 : 0.0;
    std::cout << "  " << std::left << std::setw(columnWidth) << "Draws"
              << std::setw(numberWidth) << draws << " "
              << std::fixed << std::setprecision(2) << drawPercentage << "%" << std::endl;

    std::cout << "\nWins by player:" << std::endl;
    for (const Player& player : players) {
        double winPercentage = (totalWins > 0) ? (static_cast<double>(player.getWins()) / totalGames) * 100.0 : 0.0;
        std::cout << "  " << std::left << std::setw(columnWidth) << player.getName()
                  << std::setw(numberWidth) << player.getWins() << " ("
                  << Player::playStyleToString(player.getPlayStyle()) << ") "
                  << std::setprecision(2) << winPercentage << "%" << std::endl;
    }

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