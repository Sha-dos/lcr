//
// Created by 64001830 on 12/28/24.
//

#ifndef LCR_DICE_H
#define LCR_DICE_H

#include <random>

class Dice {

public:
    enum Side {
        L,
        C,
        R,
        Dot,
        Wild
    };

    static Side roll() {
        std::random_device dev;

        std::mt19937 rng(dev());
        std::uniform_int_distribution<std::mt19937::result_type> dist(1,6);
        switch (dist(rng)) {
            case 1:
                return Side::L;
            case 2:
                return Side::C;
            case 3:
                return Side::R;
            case 4:
                return Side::Dot;
            case 5:
                return Side::Wild;
            case 6:
                return Side::Dot;
            default:
                return Side::Dot;
        }
    }
};

#endif //LCR_DICE_H
