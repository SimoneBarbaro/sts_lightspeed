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

int main() {
    std::string jsonString;
    std::ifstream infile;
    // Loading a save file in the form generated by the communication mod
    infile.open("files/save.json", std::ios_base::app);
    std::getline(infile, jsonString);
    nlohmann::json json = nlohmann::json::parse(jsonString);
    sts::GameContext gc;
    gc.initFromJson(json);
    //std::cout << gc;
    // print deck
    for (int i = 0; i < gc.deck.cards.size(); ++i) {
        std::cout << "Deck " << i << ": " << gc.deck.cards[i] << std::endl;
    }
    // print relics
    for (int i = 0; i < gc.relics.relics.size(); ++i) {
        std::cout << "Relic " << i << ": " << sts::relicNames[static_cast<int>(gc.relics.relics[i].id)] << std::endl;
    }
    // print potions
    for (int i = 0; i < gc.potions.size(); ++i) {
        std::cout << "Potion " << i << ": " << sts::potionNames[static_cast<int>(gc.potions[i])] << std::endl;
    }
    // print map
    std::cout << gc.map->toString() << std::endl;

    std::cout << "Loaded game context" << std::endl;

    sts::search::ScumSearchAgent2 agent;
    agent.simulationCountBase = 1;
    agent.rng = std::default_random_engine(gc.seed);

    agent.printActions = 1;
    agent.printLogs = 1;

    // Finish battle if already started one
    if (gc.screenState == sts::ScreenState::BATTLE) {
        sts::BattleContext bc;
        bc.initFromJson(gc, json["game_state"]["combat_state"]);
        agent.playoutBattle(bc);
        bc.exitBattle(gc);
    }
    if (gc.outcome == sts::GameOutcome::UNDECIDED)
        agent.playout(gc);

    sts::printOutcome(std::cout, gc);
}