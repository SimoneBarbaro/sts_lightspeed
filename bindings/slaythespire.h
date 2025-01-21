//
// Created by keega on 9/24/2021.
//

#ifndef STS_LIGHTSPEED_SLAYTHESPIRE_H
#define STS_LIGHTSPEED_SLAYTHESPIRE_H

#include <vector>
#include <unordered_map>
#include <array>
#include "constants/PlayerStatusEffects.h"
#include "constants/Relics.h"
#include "constants/RelicPools.h"
#include "constants/Cards.h"
#include "constants/MonsterMoves.h"
#include "combat/Player.h"
#include "combat/Monster.h"
#include "sim/search/Action.h"
#include "sim/search/GameAction.h"

#include "constants/Rooms.h"

namespace sts {

    struct NNInterface {
        static constexpr int numCards = static_cast<int>(CardId::ZAP);
        static constexpr int MAP_REPRESENTATION_SIZE = 7 // edges to first row
            //valid edges to next row, 3 bits per node, 21 bits per row 
            + 14*7*3
            // room types - for each node there are 6 possible rooms, 
            // the first row is always monster, the 8th row is always treasure, 14th is always rest
            + 12*7*6; 
            
        static constexpr int observation_space_size = 4 // Basic player features
            // boss encoding
            + 10
            // deck card counts
            + static_cast<int>(NNInterface::numCards*2)
            // Relics one hot encodings
            + static_cast<int>(RelicId::INVALID)
            // map
            + MAP_REPRESENTATION_SIZE
            // events one hot encodings
            + static_cast<int>(Event::WORLD_OF_GOOP)
            // relics reward
            + static_cast<int>(RelicId::INVALID)
            // cards rewards
            + NNInterface::numCards*2*5
            // potions rewards
            + static_cast<int>(Potion::WEAK_POTION)
            // shop prices
            + 15;
        static constexpr int battle_observation_size = 9 // Basic player features
            // Player status features
            + static_cast<int>(PlayerStatus::THE_BOMB) 
            // player special info features
            + 8
            // hand+draw+discard+exhaust pile one hot encodings of each card
            + static_cast<int>(NNInterface::numCards*2*13)
            // Relics one hot encodings
            + static_cast<int>(RelicId::INVALID)
            // Monster features + monster statuses + monster intents and damage info
            + 5 * (3 + 13 + static_cast<int>(Intent::UNKNOWN) + 2);
        static constexpr int playerHpMax = 200;
        static constexpr int playerGoldMax = 1800;
        static constexpr int shopPriceMax = 500;
        static constexpr int cardCountMax = 7;
        static constexpr int maxStatusValue = 30;
        
        

        const std::vector<int> cardEncodeMap;
        const std::unordered_map<MonsterEncounter, int> bossEncodeMap;

        static inline NNInterface *theInstance = nullptr;

        NNInterface();

        int getCardIdx(Card c) const;
        std::array<int, NNInterface::battle_observation_size> encodeBattle(const GameContext &gc, const BattleContext &bc) const;
        void encodePlayer(std::array<int, NNInterface::battle_observation_size> &ret, int &offset, const Player &player) const;
        void encodeMonster(std::array<int, NNInterface::battle_observation_size> &ret, int &offset, const Monster &monster, bool hasRunicDome, const BattleContext &bc) const;
        std::array<int, observation_space_size> getObservationMaximums() const;
        std::array<int, battle_observation_size> getBattleObservationMaximums() const;
        std::array<int, observation_space_size> getObservation(const GameContext &gc) const;


        static std::vector<int> createOneHotCardEncodingMap();
        static std::unordered_map<MonsterEncounter, int> createBossEncodingMap();
        static NNInterface* getInstance();

    };

    namespace search {
        class ScumSearchAgent2;
        class BattleScumSearcher2;
        
        struct Encoding {
            static constexpr int action_space_size = 4;

            std::unordered_map<uint32_t, int> gameActionEncodeMap;
            std::unordered_map<uint32_t, int> battleActionEncodeMap;
            std::unordered_map<int, GameAction> gameActionDecodeMap;
            std::unordered_map<int, Action> battleActionDecodeMap;
            //std::array<int, action_space_size> encodeAction(const GameContext &gc, const sts::search::GameAction &action);
            //std::array<int, action_space_size> encodeBattleAction(const sts::search::Action &action);
            void createGameActionEncodeMap();
            void createBattleActionEncodeMap();
            static inline Encoding *theInstance = nullptr;

            Encoding();
            static Encoding* getInstance();

        };
    }


    class GameContext;
    class BattleContext;
    class Map;

    namespace py {

        void play();

        search::ScumSearchAgent2* getAgent();
        void setGc(const GameContext &gc);
        GameContext* getGc();

        void playout();
        std::vector<Card> getCardReward(GameContext &gc);
        void pickRewardCard(GameContext &gc, Card card);
        void skipRewardCards(GameContext &gc);

        std::vector<int> getNNMapRepresentation(const Map &map);
        Room getRoomType(const Map &map, int x, int y);
        bool hasEdge(const Map &map, int x, int y, int x2);

        void initGameContextFromJsonString(GameContext &gc, const std::string &json);
        void initBattleContextFromJsonString(BattleContext &bc, GameContext &gc, const std::string &json);
    }


}


#endif //STS_LIGHTSPEED_SLAYTHESPIRE_H
