// =========================================================================
// dice.h
// =========================================================================
#ifndef LCR_DICE_H
#define LCR_DICE_H

#include <random> // Required for random number generation

// Represents the special LCR dice
class Dice {
public:
    // Enum representing the possible outcomes of a dice roll
    enum Side {
        L,    // Pass chip to the Left
        C,    // Put chip in the Center (pot)
        R,    // Pass chip to the Right
        Dot,  // Keep chip (no action) - Appears on 2 sides now
        Wild  // Steal or Cancel chip according to player strategy - Appears on 1 side
    };

    // Simulates rolling a single LCR die
    static Side roll() {
        // Use a static generator for better performance and randomness distribution
        static std::random_device dev;
        static std::mt19937 rng(dev());
        // Distribution for a 6-sided die (1 to 6)
        static std::uniform_int_distribution<int> dist(1, 6);

        int rollValue = dist(rng);

        // Map the numerical roll to the corresponding Side enum value
        // 1 L, 1 C, 1 R, 1 Wild, 2 Dots
        switch (rollValue) {
            case 1: return Side::L;
            case 2: return Side::C;
            case 3: return Side::R;
            case 4: return Side::Wild; // One Wild side
            case 5: return Side::Dot;  // Two Dot sides
            case 6: return Side::Dot;
            default: // Should not happen
                return Side::Dot;
        }
    }
};

#endif //LCR_DICE_H