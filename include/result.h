// =========================================================================
// result.h
// =========================================================================

#ifndef LCR_RESULT_H
#define LCR_RESULT_H

#include <string>
#include <vector>
#include "player.h"
#include "json.hpp"

class Result {
public:
    int gameId;
    std::string winnerName;
    Player::PlayStyle winnerStrategy;
    int numberOfRounds;
    int numberOfPlayers;
    int initialChipsPerPlayer; // Assuming uniform start
    std::vector<Player::PlayStyle> allPlayerStrategies; // Strategies used in this game
    bool draw; // Flag if no winner (should be rare)

    // Default constructor (optional, but can be useful)
    Result() : gameId(-1), winnerName(""), winnerStrategy(Player::PlayStyle::StealFromHighest), // Default placeholder
               numberOfRounds(0), numberOfPlayers(0), initialChipsPerPlayer(0), draw(false) {}

    // Parameterized constructor
    Result(int id, const std::string& wName, Player::PlayStyle wStrat, int rounds, int numP, int initChips, const std::vector<Player::PlayStyle>& allStrats, bool isDraw = false)
            : gameId(id), winnerName(wName), winnerStrategy(wStrat), numberOfRounds(rounds),
              numberOfPlayers(numP), initialChipsPerPlayer(initChips), allPlayerStrategies(allStrats), draw(isDraw) {}
};

void to_json(nlohmann::json& j, const Result& result) {
    j = nlohmann::json{
            {"winnerStrategy", Player::playStyleToString(result.winnerStrategy)},
            {"draw", result.draw},
            {"gameId", result.gameId},
            {"winnerName", result.winnerName},
            {"numberOfRounds", result.numberOfRounds},
            {"numberOfPlayers", result.numberOfPlayers},
            {"initialChipsPerPlayer", result.initialChipsPerPlayer},
            {"allPlayerStrategies", result.allPlayerStrategies}
    };
}

// --- nlohmann/json integration for Result ---
// Define how to convert Result class to/from JSON
// Using NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE for simplicity if all members are public
// If members were private, you'd write custom to_json/from_json functions
//NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Result, gameId, winnerName, winnerStrategy, numberOfRounds, numberOfPlayers, initialChipsPerPlayer, allPlayerStrategies, draw);
#endif // LCR_RESULT_H
