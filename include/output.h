// =========================================================================
// output.h
// =========================================================================

#ifndef LCR_OUTPUT_H
#define LCR_OUTPUT_H

#include "json.hpp"

class Output {
public:
    enum OutputType {
        All,
        Totals,
    };

    static OutputType stringToOutputType(const std::string& str) {
        if (str == "All") return OutputType::All;
        if (str == "Totals") return OutputType::Totals;
        return OutputType::All; // Default
    }
};

NLOHMANN_JSON_SERIALIZE_ENUM( Output::OutputType, {
    {Output::OutputType::All, "all"},
    {Output::OutputType::Totals, "totals"}
})

#endif //LCR_OUTPUT_H
