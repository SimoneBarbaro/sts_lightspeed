//
// Created by simoneBarbaro on 01/17/2025.
//

#ifndef STS_LIGHTSPEED_GAMESCCORING_H
#define STS_LIGHTSPEED_GAMESCCORING_H

namespace sts {
    class GameScoreTracker;
    struct GameScoreTracker {
        static constexpr const int OVERKILL_DAMAGE = 99;
        static constexpr const int CCCCOMBO_NUM_CARDS = 20;
        static constexpr const int BOSS_KILL_SCORES[] = {0, 50, 150, 300, 500, 750};
        int enemiesSlain = 0;
        int eliteKilledAct1 = 0;
        int eliteKilledAct2 = 0;
        int eliteKilledAct3 = 0;
        int bossesKilled = 0;
        int perfectElites = 0;
        int perfectBosses = 0;
        int isCcccombo = 0;
        int isOverkill = 0;
        int heartbreaker = 0;
        int questionmarkVisited = 0;
        int hpIncrease = 0;
        int totalGold = 0;
    };

}

#endif
