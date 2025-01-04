//
// Created by keega on 9/24/2021.
//
#include <sstream>
#include <algorithm>

#include "sim/ConsoleSimulator.h"
#include "sim/search/ScumSearchAgent2.h"
#include "sim/SimHelpers.h"
#include "sim/PrintHelpers.h"
#include "game/Game.h"
#include "game/Map.h"

#include "slaythespire.h"

namespace sts {

    NNInterface::NNInterface() :
            cardEncodeMap(createOneHotCardEncodingMap()),
            bossEncodeMap(createBossEncodingMap()) {}

    int NNInterface::getCardIdx(Card c) const {
        int idx = cardEncodeMap[static_cast<int>(c.id)] * 2;
        if (idx == -1) {
            std::cerr << "attemped to get encoding idx for invalid card" << std::endl;
            assert(false);
        }

        if (c.isUpgraded()) {
            idx += 1;
        }

        return idx;
    }

    std::array<int,NNInterface::battle_observation_size> NNInterface::encodeBattle(const GameContext &gc, const BattleContext &bc) const {
        std::array<int,NNInterface::battle_observation_size> ret {};
        int offset = 0;
        encodePlayer(ret, offset, bc.player);
        // 10 cards in hand, need positional encoding
        for (CardInstance c : bc.cards.hand) {
            int idx = static_cast<int>(c.getId());
            idx += c.isUpgraded() ? NNInterface::numCards : 0;
            ret[offset + idx] = 1;
            offset += NNInterface::numCards*2;
        }
        // encode counts of cards for draw, exhaust and discard piles
        for (CardInstance c : bc.cards.drawPile) {
            int idx = static_cast<int>(c.getId());
            idx += c.isUpgraded() ? NNInterface::numCards : 0;
            ret[offset + idx] += 1;
        }
        offset += NNInterface::numCards*2;
        for (CardInstance c : bc.cards.discardPile) {
            int idx = static_cast<int>(c.getId());
            idx += c.isUpgraded() ? NNInterface::numCards : 0;
            ret[offset + idx] += 1;
        }
        offset += NNInterface::numCards*2;
        for (CardInstance c : bc.cards.exhaustPile) {
            int idx = static_cast<int>(c.getId());
            idx += c.isUpgraded() ? NNInterface::numCards : 0;
            ret[offset + idx] += 1;
        }

        // Encode relics
        for (auto r : gc.relics.relics) {
            int encodeIdx = offset + static_cast<int>(r.id);
            ret[encodeIdx] = 1;
        }
        // Encode monsters
        for (int i = 0; i < bc.monsters.monsterCount; i++) {
            if (!bc.monsters.arr[i].isDeadOrEscaped())
                NNInterface::encodeMonster(ret, offset, bc.monsters.arr[i], bc.player.hasRelic<RelicId::RUNIC_DOME>(), bc);
        }
        return ret;
    }

    void NNInterface::encodePlayer(std::array<int, NNInterface::battle_observation_size> &ret, int &offset, const sts::Player &player) const
    {
        ret[offset++] = player.curHp;
        ret[offset++] = player.maxHp;
        ret[offset++] = player.block;
        ret[offset++] = player.energy;
        // TODO maybe one hot encoding is better?
        ret[offset++] = static_cast<int>(player.cc);
        // These seems to be processed separately from the rest?
        ret[offset++] = player.getStatus<PlayerStatus::ARTIFACT>();
        ret[offset++] = player.getStatus<PlayerStatus::DEXTERITY>();
        ret[offset++] = player.getStatus<PlayerStatus::STRENGTH>();
        ret[offset++] = player.getStatus<PlayerStatus::FOCUS>();
        // TODO orbs
        // All the other statuses are in statusMap
        for (auto status : player.statusMap)
        {
            ret[offset + static_cast<int>(status.first)] = status.second;
        }
    }

    void NNInterface::encodeMonster(std::array<int, NNInterface::battle_observation_size> &ret, int &offset, const sts::Monster &monster, bool hasRunicDome, const BattleContext &bc) const
    {
        ret[offset++] = monster.curHp;
        ret[offset++] = monster.maxHp;
        ret[offset++] = monster.block;
        ret[offset++] = monster.getStatus<MonsterStatus::ARTIFACT>();
        ret[offset++] = monster.getStatus<MonsterStatus::BLOCK_RETURN>();
        ret[offset++] = monster.getStatus<MonsterStatus::CHOKED>();
        ret[offset++] = monster.getStatus<MonsterStatus::CORPSE_EXPLOSION>();
        
        ret[offset++] = monster.getStatus<MonsterStatus::LOCK_ON>();
        ret[offset++] = monster.getStatus<MonsterStatus::MARK>();
        ret[offset++] = monster.getStatus<MonsterStatus::METALLICIZE>();
        ret[offset++] = monster.getStatus<MonsterStatus::PLATED_ARMOR>();
        ret[offset++] = monster.getStatus<MonsterStatus::POISON>();
        ret[offset++] = monster.getStatus<MonsterStatus::REGEN>();
        ret[offset++] = monster.getStatus<MonsterStatus::SHACKLED>();
        ret[offset++] = monster.getStatus<MonsterStatus::VULNERABLE>();
        ret[offset++] = monster.getStatus<MonsterStatus::WEAK>();

        ret[offset++] = monster.uniquePower0;
        ret[offset++] = monster.uniquePower1;
        // one hot encoding monster type
        int monsterIdx = static_cast<int>(monster.id);
        ret[offset + monsterIdx] = 1;
        Intent monsterIntent = getMoveIntent(monster.moveHistory[0]);
        if (hasRunicDome) {
            monsterIntent = Intent::UNKNOWN;
        }
        int intentIdx = static_cast<int>(monsterIntent);
        ret[offset+intentIdx] = 1;
        if (monsterIntent == Intent::ATTACK || monsterIntent == Intent::ATTACK_BUFF || monsterIntent == Intent::ATTACK_DEBUFF || monsterIntent == Intent::ATTACK_DEFEND) {
            // TODO check if this is the correct damage info
            DamageInfo damage = monster.getMoveBaseDamage(bc);
            ret[offset++] = damage.attackCount;
            ret[offset++] = damage.damage;
        } else { // We still want to increase offset when no damage is done
            offset += 2;
        }
        //int monsterMoveIdx = static_cast<int>(monster.moveHistory[0]);
        //ret[offset + monsterMoveIdx] = 1;
    }

    std::array<int,NNInterface::observation_space_size> NNInterface::getObservation(const GameContext &gc) const {
        std::array<int,observation_space_size> ret {};

        int offset = 0;

        ret[offset++] = std::min(gc.curHp, playerHpMax);
        ret[offset++] = std::min(gc.maxHp, playerHpMax);
        ret[offset++] = std::min(gc.gold, playerGoldMax);
        ret[offset++] = gc.floorNum;

        int bossEncodeIdx = offset + bossEncodeMap.at(gc.boss);
        ret[bossEncodeIdx] = 1;
        offset += 10;

        for (auto c : gc.deck.cards) {
            int encodeIdx = offset + getCardIdx(c);
            ret[encodeIdx] = std::min(ret[encodeIdx]+1, cardCountMax);
        }
        offset += 220;

        for (auto r : gc.relics.relics) {
            int encodeIdx = offset + static_cast<int>(r.id);
            ret[encodeIdx] = 1;
        }
        offset += 178;

        return ret;
    }

    std::array<int,NNInterface::observation_space_size> NNInterface::getObservationMaximums() const {
        std::array<int,observation_space_size> ret {};
        int spaceOffset = 0;

        ret[0] = playerHpMax;
        ret[1] = playerHpMax;
        ret[2] = playerGoldMax;
        ret[3] = 60;
        spaceOffset += 3;

        std::fill(ret.begin()+spaceOffset, ret.end(), 1);
        spaceOffset += 10;

        std::fill(ret.begin()+spaceOffset, ret.end(), cardCountMax);
        spaceOffset += 220;

        std::fill(ret.begin()+spaceOffset, ret.end(), 1);
        spaceOffset += 178;

        return ret;
    }

    std::vector<int> NNInterface::createOneHotCardEncodingMap() {
        std::vector<CardId> redCards;
        for (int i = static_cast<int>(CardId::INVALID); i <= static_cast<int>(CardId::ZAP); ++i) {
            auto cid = static_cast<CardId>(i);
            auto color = getCardColor(cid);
            if (color == CardColor::RED) {
                redCards.push_back(cid);
            }
        }

        std::vector<CardId> colorlessCards;
        for (int i = 0; i < srcColorlessCardPoolSize; ++i) {
            colorlessCards.push_back(srcColorlessCardPool[i]);
        }
        std::sort(colorlessCards.begin(), colorlessCards.end(), [](auto a, auto b) {
            return std::string(getCardEnumName(a)) < std::string(getCardEnumName(b));
        });

        std::vector<int> encodingMap(372);
        std::fill(encodingMap.begin(), encodingMap.end(), 0);

        int hotEncodingIdx = 0;
        for (auto x : redCards) {
            encodingMap[static_cast<int>(x)] = hotEncodingIdx++;
        }
        for (auto x : colorlessCards) {
            encodingMap[static_cast<int>(x)] = hotEncodingIdx++;
        }

        return encodingMap;
    }

    std::unordered_map<MonsterEncounter, int> NNInterface::createBossEncodingMap() {
        std::unordered_map<MonsterEncounter, int> bossMap;
        bossMap[ME::SLIME_BOSS] = 0;
        bossMap[ME::HEXAGHOST] = 1;
        bossMap[ME::THE_GUARDIAN] = 2;
        bossMap[ME::CHAMP] = 3;
        bossMap[ME::AUTOMATON] = 4;
        bossMap[ME::COLLECTOR] = 5;
        bossMap[ME::TIME_EATER] = 6;
        bossMap[ME::DONU_AND_DECA] = 7;
        bossMap[ME::AWAKENED_ONE] = 8;
        bossMap[ME::THE_HEART] = 9;
        return bossMap;
    }

    NNInterface* NNInterface::getInstance() {
        if (theInstance == nullptr) {
            theInstance = new NNInterface;
        }
        return theInstance;
    }

}

namespace sts::py {

    void play() {
        sts::SimulatorContext ctx;
        sts::ConsoleSimulator sim;
        sim.play(std::cin, std::cout, ctx);
    }

    search::ScumSearchAgent2* getAgent() {
        static search::ScumSearchAgent2 *agent = nullptr;
        if (agent == nullptr) {
            agent = new search::ScumSearchAgent2();
            agent->pauseOnCardReward = true;
        }
        return agent;
    }

    void playout(GameContext &gc) {
        auto agent = getAgent();
        agent->playout(gc);
    }

    std::vector<Card> getCardReward(GameContext &gc) {
        const bool inValidState = gc.outcome == GameOutcome::UNDECIDED &&
                                  gc.screenState == ScreenState::REWARDS &&
                                  gc.info.rewardsContainer.cardRewardCount > 0;

        if (!inValidState) {
            std::cerr << "GameContext was not in a state with card rewards, check that the game has not completed first." << std::endl;
            return {};
        }

        const auto &r = gc.info.rewardsContainer;
        const auto &cardList = r.cardRewards[r.cardRewardCount-1];
        return std::vector<Card>(cardList.begin(), cardList.end());
    }

    void pickRewardCard(GameContext &gc, Card card) {
        const bool inValidState = gc.outcome == GameOutcome::UNDECIDED &&
                                  gc.screenState == ScreenState::REWARDS &&
                                  gc.info.rewardsContainer.cardRewardCount > 0;
        if (!inValidState) {
            std::cerr << "GameContext was not in a state with card rewards, check that the game has not completed first." << std::endl;
            return;
        }
        auto &r = gc.info.rewardsContainer;
        gc.deck.obtain(gc, card);
        r.removeCardReward(r.cardRewardCount-1);
    }

    void skipRewardCards(GameContext &gc) {
        const bool inValidState = gc.outcome == GameOutcome::UNDECIDED &&
                                  gc.screenState == ScreenState::REWARDS &&
                                  gc.info.rewardsContainer.cardRewardCount > 0;
        if (!inValidState) {
            std::cerr << "GameContext was not in a state with card rewards, check that the game has not completed first." << std::endl;
            return;
        }

        if (gc.hasRelic(RelicId::SINGING_BOWL)) {
            gc.playerIncreaseMaxHp(2);
        }

        auto &r = gc.info.rewardsContainer;
        r.removeCardReward(r.cardRewardCount-1);
    }

    void initGameContextFromJsonString(GameContext &gc, const std::string &json) {
        nlohmann::json j = nlohmann::json::parse(json);
        gc.initFromJson(j);
    }

    void initBattleContextFromJsonString(BattleContext &bc, GameContext &gc, const std::string &json) {
        nlohmann::json j = nlohmann::json::parse(json);
        bc.initFromJson(gc, j);
    }

    // BEGIN MAP THINGS ****************************

    std::vector<int> getNNMapRepresentation(const Map &map) {
        std::vector<int> ret;

        // 7 bits
        // push edges to first row
        for (int x = 0; x < 7; ++x) {
            if (map.getNode(x,0).edgeCount > 0) {
                ret.push_back(true);
            } else {
                ret.push_back(false);
            }
        }

        // for each node in a row, push valid edges to next row, 3 bits per node, 21 bits per row
        // skip 14th row because it is invariant
        // 21 * 13 == 273 bits
        for (int y = 0; y < 14; ++y) {
            for (int x = 0; x < 7; ++x) {

                bool localEdgeValues[3] {false, false, false};
                auto node = map.getNode(x,y);
                for (int i = 0; i < node.edgeCount; ++i) {
                    auto edge = node.edges[i];
                    if (edge < x) {
                        localEdgeValues[0] = true;
                    } else if (edge == x) {
                        localEdgeValues[1] = true;
                    } else {
                        localEdgeValues[2] = true;
                    }
                }
                ret.insert(ret.end(), localEdgeValues, localEdgeValues+3);
            }
        }

        // room types - for each node there are 6 possible rooms,
        // the first row is always monster, the 8th row is always treasure, 14th is always rest
        // this gives 14-3 valid rows == 11
        // 11 * 6 * 7 = 462 bits
        for (int y = 1; y < 14; ++y) {
            if (y == 8) {
                continue;
            }
            for (int x = 0; x < 7; ++x) {
                auto roomType = map.getNode(x,y).room;
                for (int i = 0; i < 6; ++i) {
                    ret.push_back(static_cast<int>(roomType) == i);
                }
            }
        }

        return ret;
    };

    Room getRoomType(const Map &map, int x, int y) {
        if (x < 0 || x > 6 || y < 0 || y > 14) {
            return Room::INVALID;
        }

        return map.getNode(x,y).room;
    }

    bool hasEdge(const Map &map, int x, int y, int x2) {
        if (x == -1) {
            return map.getNode(x2,0).edgeCount > 0;
        }

        if (x < 0 || x > 6 || y < 0 || y > 14) {
            return false;
        }


        auto node = map.getNode(x,y);
        for (int i = 0; i < node.edgeCount; ++i) {
            if (node.edges[i] == x2) {
                return true;
            }
        }
        return false;
    }

}