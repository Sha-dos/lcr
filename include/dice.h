//
// Created by 64001830 on 12/28/24.
//

#ifndef LCR_DICE_H
#define LCR_DICE_H

#include <random>

class Dice {
public:
    // Enum representing the possible outcomes of a dice roll
    enum Side {
        L,    // Pass chip to the Left
        C,    // Put chip in the Center (pot)
        R,    // Pass chip to the Right
        Dot,  // Keep chip (no action) - Appears on 3 sides
        Wild  // Steal chip according to player strategy
    };

    // Simulates rolling a single LCR die
    static Side roll() {
        // Use a static generator for better performance and randomness distribution
        // than creating a new one each time.
        static std::random_device dev;
        static std::mt19937 rng(dev());
        // Distribution for a 6-sided die (1 to 6)
        static std::uniform_int_distribution<int> dist(1, 6);

        int rollValue = dist(rng);

        switch (rollValue) {
            case 1: return Side::L;
            case 2: return Side::C;
            case 3: return Side::R;
            case 4: return Side::Wild;
            case 5: return Side::Dot;
            case 6: return Side::Dot;
            default: // Should not happen with dist(1,6)
                return Side::Dot;
        }
    }
};

#endif //LCR_DICE_H

