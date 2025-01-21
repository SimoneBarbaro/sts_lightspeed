#include <iostream>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <thread>

#include "combat/BattleContext.h"
#include "game/GameContext.h"
#include "sim/ConsoleSimulator.h"
#include "sim/search/BattleScumSearcher2.h"
#include "sim/search/ScumSearchAgent2.h"
#include "sim/search/SimpleAgent.h"
#include "sim/PrintHelpers.h"
#include "sim/search/BattleScumSearcher2.h"

#include <nlohmann/json.hpp>
#include "slaythespire.h"

int main() {
    std::ifstream infile;

    uint32_t seed;
    char skip;
    std::cout << "seed:";
    std::cin >> seed;
    std::cout << "y to skip battles:";
    std::cin >> skip;
    sts::GameContext gc = sts::GameContext(sts::CharacterClass::IRONCLAD, seed, 0);

    gc.skipBattles = skip=='y';
    infile.close();
    infile.open("files/action-sequence.txt");

    std::ofstream os;
    os.open("files/outcome.log");
    int action;
    sts::BattleContext bc;
    bool inBattle = false; 

    while (infile >> action)
    {
        if (gc.screenState == sts::ScreenState::BATTLE) {
            if (!inBattle) {
                bc = sts::BattleContext();
                bc.init(gc);
                inBattle = true;
            }
            auto actions = sts::search::Action::enumerateAllAvailableActions(bc);
            os << bc << "\n";
            os << "Battle context available actions-> ";
            for (auto a : actions)
                a.printDesc(os, bc) << "(" << sts::search::Encoding::getInstance()->battleActionEncodeMap.at(a.bits) << ")" << ";";
            os << "\n";
            actions[action].printDesc(os << "Battle context execute action-> ", bc) << "\n";
            os.flush();
            actions[action].execute(bc);
            if (bc.outcome != sts::Outcome::UNDECIDED) {
                bc.exitBattle(gc);
            }
        } else {
            inBattle = false;
            auto actions = sts::search::GameAction::getAllActionsInState(gc);
            os << gc << "\n";
            os << "Game context available actions-> ";
            for (auto a : actions)
                a.printDesc(os, gc) << "(" << sts::search::Encoding::getInstance()->gameActionEncodeMap.at(a.bits) << ")" << ";";
            os << "\n";
            actions[action].printDesc(os << "Game context execute action-> ", gc) << "\n";
            os.flush();
            actions[action].execute(gc);
        }
    }
    os << gc << "\n";
    if (gc.screenState == sts::ScreenState::BATTLE) {
        os << "GC ready for combat" << gc << "\n";
        auto bc = sts::BattleContext();
        bc.init(gc);
        os << "BC: " << bc << "\n";
        auto battleActions = sts::search::Action::enumerateAllAvailableActions(bc);
        for (auto a : battleActions)
                a.printDesc(os, bc) << "(" << sts::search::Encoding::getInstance()->battleActionEncodeMap.at(a.bits) << ")" << ";";
            os << "\n";
    } else {
        auto actions = sts::search::GameAction::getAllActionsInState(gc);
        os << "Game context available actions-> ";
        for (auto a : actions)
            a.printDesc(os, gc) << "(" << sts::search::Encoding::getInstance()->gameActionEncodeMap.at(a.bits) << ")" << ";";
        os << "\n";
        os.close();
        // print map
        os << gc.map->toString() << std::endl;
        
        auto encoding = sts::NNInterface::getInstance()->getObservation(gc);
        os << "Game encoding: [\n";
        for (auto obs : encoding)
            os << obs << ",";
        os << "\n]";

        sts::printOutcome(std::cout, gc);
    }
}