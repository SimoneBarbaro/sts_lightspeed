//
// Created by SimoneBarbaro on 04/01/2025.
//

#ifndef STS_LIGHTSPEED_INTENT_H
#define STS_LIGHTSPEED_INTENT_H

#include <cstdint>
#include <string>

namespace sts {
    enum class Intent : std::uint8_t {
        INVALID = 0,
        ATTACK,
        ATTACK_BUFF,
        ATTACK_DEBUFF,
        ATTACK_DEFEND,
        BUFF,
        DEBUFF,
        STRONG_DEBUFF,
        DEBUG,
        DEFEND,
        DEFEND_DEBUFF,
        DEFEND_BUFF,
        ESCAPE,
        MAGIC,
        NONE,
        SLEEP,
        STUN,
        UNKNOWN
    };
}

#endif //STS_LIGHTSPEED_INTENT_H
