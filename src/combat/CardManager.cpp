//
// Created by gamerpuppy on 7/24/2021.
//

#include "combat/CardManager.h"
#include "combat/BattleContext.h"

#include "game/GameContext.h"
#include "game/Card.h"

using namespace sts;

void CardManager::init(const sts::GameContext &gc, BattleContext &bc) {

//    masterDeckSize = gc.deck.size();
    nextUniqueCardId = gc.deck.size();
    handPainCount = false;
    handNormalityCount = false;
    strikeCount = 0;

    fixed_list<int, Deck::MAX_SIZE> idxs(gc.deck.size());
    for (int i = 0; i < idxs.size(); ++i) {
        idxs[i] = i;
    }
    java::Collections::shuffle(idxs.begin(), idxs.end(), java::Random(bc.shuffleRng.randomLong()));

    drawPile.resize(gc.deck.size());
    discardPile.clear();
    exhaustPile.clear();

    int normalCount = 0;
    bool isInnateMemo[Deck::MAX_SIZE];

    for (int i = 0; i < gc.deck.size(); ++i) {
        const int deckIdx = idxs[i];
        const auto &deckCard = gc.deck.cards[deckIdx];
//        if (deckCard.isStrikeCard()) { ++strikeCount; } do at createDeckCardInstanceInDrawPile

        bool isBottled = std::find(gc.deck.bottleIdxs.begin(), gc.deck.bottleIdxs.end(), deckIdx) != gc.deck.bottleIdxs.end();
        isInnateMemo[i] = deckCard.isInnate() || isBottled;

        if (!isInnateMemo[i]) {
            ++normalCount;
        }
    }

    int normalIdx = 0;
    int innateIdx = normalCount;

    for (int i = 0; i < gc.deck.size(); ++i) {
        const int deckIdx = idxs[i];
        const auto &deckCard = gc.deck.cards[deckIdx];

        if (isInnateMemo[i]) {
            createDeckCardInstanceInDrawPile(deckCard, deckIdx, innateIdx);
            ++innateIdx;
        } else {
            createDeckCardInstanceInDrawPile(deckCard, deckIdx, normalIdx);
            ++normalIdx;
        }
    }

    int innateCount =  innateIdx - normalCount;
    if (innateCount  > bc.player.cardDrawPerTurn) {
        bc.addToBot( Actions::DrawCards(innateCount-bc.player.cardDrawPerTurn) );
    }
}

void CardManager::createDeckCardInstanceInDrawPile(const Card &card, int deckIdx, int drawIdx) {
    auto &c = drawPile[drawIdx];
    c = CardInstance(card);
    c.setUniqueId(deckIdx);
    notifyCreateCard(c);
    notifyAddToDrawPile(c);
}

void CardManager::createTempCardInDrawPile(int idx, CardInstance c) {
    c.uniqueId = static_cast<std::int16_t>(nextUniqueCardId++);
    notifyCreateCard(c);
    notifyAddToDrawPile(c);
    drawPile.insert(drawPile.begin()+idx, c);
}

void CardManager::createTempCardInDiscard(CardInstance c) {
    c.uniqueId = static_cast<std::int16_t>(nextUniqueCardId++);
    notifyCreateCard(c);
    notifyAddToDiscardPile(c);
    discardPile.push_back(c);
}

void CardManager::createTempCardInHand(CardInstance c) {
    c.uniqueId = static_cast<std::int16_t>(nextUniqueCardId++);
    notifyCreateCard(c);
    notifyAddToHand(c);
    hand[cardsInHand++] = c;
}

// **************** START Remove Methods ****************

void CardManager::removeFromDrawPileAtIdx(int idx) {
    notifyRemoveFromDrawPile(drawPile[idx]);
    drawPile.erase(drawPile.begin()+idx);
}

CardInstance CardManager::popFromDrawPile() {
    auto c = drawPile.back();
    notifyRemoveFromDrawPile(c);
    drawPile.pop_back();
    return c;
}

void CardManager::removeFromHandAtIdx(int idx) {
    notifyRemoveFromHand(hand[idx]);
    eraseAtIdxInHand(idx);
}

void CardManager::removeFromHandById(std::uint16_t uniqueId) {
    for (int i = 0; i < cardsInHand; ++i) {
        if (hand[i].getUniqueId() == uniqueId) {
            notifyRemoveFromHand(hand[i]);
            eraseAtIdxInHand(i);
        }
    }
}

void CardManager::removeFromDiscard(int idx) {
    notifyRemoveFromDiscardPile(*(discardPile.begin()+idx));
    discardPile.erase(discardPile.begin()+idx);
}

void CardManager::removeFromExhaustPile(int idx) {
    exhaustPile.erase(exhaustPile.begin()+idx);
}

// **************** END Remove Methods ****************

// **************** START Move Methods ****************

void CardManager::moveToHand(const CardInstance &c) {
    notifyAddToHand(c);
    hand[cardsInHand++] = c;
}

void CardManager::moveToExhaustPile(const CardInstance &c) {
    notifyRemoveFromCombat(c);
    exhaustPile.push_back(c);
}


void CardManager::insertToDrawPile(int drawPileIdx, const CardInstance &c) {
    notifyAddToDrawPile(c);
    drawPile.insert(drawPile.begin()+drawPileIdx, c);
}

void CardManager::moveToDrawPileTop(const CardInstance &c) {
    notifyAddToDrawPile(c);
    drawPile.push_back(c);
}

void CardManager::shuffleIntoDrawPile(Random &cardRandomRng, const CardInstance &c) {
    if (drawPile.empty()) {
        moveToDrawPileTop(c);
    } else {
        int idx = cardRandomRng.random(static_cast<int>(drawPile.size()-1));
        insertToDrawPile(idx, c);
    }
}

void CardManager::moveToDiscardPile(const CardInstance &c) {
    // todo check flurries, weave
    notifyAddToDiscardPile(c);
    discardPile.push_back(c);
}

void CardManager::moveDiscardPileIntoToDrawPile() {
    if (drawPile.empty()) {
        drawPileBloodCardCount = discardPileBloodCardCount;
        drawPile = discardPile;

    } else {
        for (const auto &c : discardPile) {
            moveToDrawPileTop(c);
        }
    }

    discardPileBloodCardCount = 0;
    discardPile.clear();
}

// **************** END Move Methods ****************


// **************** BEGIN NOTIFY METHODS ****************

void CardManager::notifyCreateCard(const CardInstance &c) {
    if (c.isStrikeCard()) {
        ++strikeCount;
    }
}

void CardManager::notifyRemoveFromCombat(const CardInstance &c) {
    if (c.isStrikeCard()) {
        --strikeCount;
    }
}

void CardManager::notifyAddToHand(const CardInstance &c) {
    if (c.isBloodCard()) {
        ++handBloodCardCount;
    }

    switch (c.id) {
        case CardId::NORMALITY:
            ++handNormalityCount;
            break;

        case CardId::PAIN:
            ++handPainCount;
            break;

        default:
            break;
    }
}

void CardManager::notifyRemoveFromHand(const CardInstance &c) {
    if (c.isBloodCard()) {
        --handBloodCardCount;
    }

    switch (c.id) {
        case CardId::NORMALITY:
            --handNormalityCount;
            break;

        case CardId::PAIN:
            --handPainCount;
            break;

        default:
            break;
    }
}

void CardManager::notifyAddToDrawPile(const CardInstance &c) {
    if (c.isBloodCard()) {
        ++drawPileBloodCardCount;
    }
}

void CardManager::notifyRemoveFromDrawPile(const CardInstance &c) {
    if (c.isBloodCard()) {
        --drawPileBloodCardCount;
    }
}

void CardManager::notifyAddToDiscardPile(const CardInstance &c) {
    if (c.isBloodCard()) {
        ++discardPileBloodCardCount;
    }
}


void CardManager::notifyRemoveFromDiscardPile(const CardInstance &c) {
    if (c.isBloodCard()) {
        --discardPileBloodCardCount;
    }
}

// **************** END NOTIFY METHODS ****************

void CardManager::eraseAtIdxInHand(int idx) {
#ifdef sts_asserts
    assert(idx < cardsInHand);
#endif

    for (int x = idx; x < cardsInHand-1; ++x) {
        hand[x] = hand[x+1];
    }
    --cardsInHand;
}

int CardManager::getRandomCardIdxInHand(Random &rng) {
    return rng.random(cardsInHand-1);
}

void CardManager::resetAttributesAtEndOfTurn() {
    for (int i = 0; i < cardsInHand; ++i) {
        hand[i].setCostForTurn(hand[i].cost);
    }

    for (auto &c : discardPile) {
        c.setCostForTurn(c.cost);
    }

    for (auto &c : drawPile) {
        c.setCostForTurn(c.cost);
    }
}

// **************** BEGIN SPECIAL HELPERS ****************

void CardManager::draw(BattleContext &bc, int amount) {
    int evolve = bc.player.getStatus<PS::EVOLVE>();
    int fireBreathing = bc.player.getStatus<PS::FIRE_BREATHING>();

    int confusedCount = 0;

    for (int i = 0; i < amount; i++) {
        auto c = popFromDrawPile();

        if (bc.player.hasStatus<PS::CONFUSED>()) {
            if (c.cost >= 0) {  // todo status and curses affected by this?
                const auto newCost = bc.cardRandomRng.random(3);
                if (c.cost != newCost) {
                    c.costForTurn = newCost;
                    c.cost = newCost;
                }
                c.freeToPlayOnce = false;
            }
        }

        if (bc.player.hasStatus<PS::CORRUPTION>() && c.getType() == CardType::SKILL) {
            c.costForTurn = 0;

        } else if (c.getType() == CardType::STATUS) {
            if (evolve) {
                bc.addToBot( Actions::DrawCards(evolve) );
            }

            if (fireBreathing) {
                bc.addToBot( Actions::DamageAllEnemy(fireBreathing) );
            }
        } else if (c.getType() == CardType::CURSE) {
            if (fireBreathing) {
                bc.addToBot( Actions::DamageAllEnemy(fireBreathing) );
            }

        }

        moveToHand(c);
    }

}

void CardManager::onTookDamage() {
    // this method will fail catastrophically if the bloodCardCounts are not correct
    const bool hasAnyBloodCards = handBloodCardCount | drawPileBloodCardCount | discardPileBloodCardCount;
    if (!hasAnyBloodCards) {
        return;
    }

    int i = 0;
    int foundBloodCards = 0;
    while (foundBloodCards < handBloodCardCount) {
        if (hand[i].isBloodCard()) {
            hand[i].tookDamage();
            ++foundBloodCards;
        }
        ++i;
    }

    i = 0;
    foundBloodCards = 0;
    while (foundBloodCards < drawPileBloodCardCount) {
        if (drawPile[i].isBloodCard()) {
            drawPile[i].tookDamage();
            ++foundBloodCards;
        }
        ++i;
    }

    i = 0;
    foundBloodCards = 0;
    while (foundBloodCards < discardPileBloodCardCount) {
        if (discardPile[i].isBloodCard()) {
            discardPile[i].tookDamage();
            ++foundBloodCards;
        }
        ++i;
    }
}

void upgradeRampage(CardInstance &c, std::int16_t upgradeAmount) {
    c.specialData += upgradeAmount;
}

void CardManager::findAndUpgradeRampage(const CardInstance &purgeCard) {
    const auto upgradeAmount = static_cast<int16_t>(purgeCard.upgraded ? 8 : 5);

    // special checks for most common scenarios
    if (!discardPile.empty() && discardPile.back().uniqueId == purgeCard.uniqueId) {
        upgradeRampage(discardPile.back(), upgradeAmount);
        return;
    }
    if (!exhaustPile.empty() && exhaustPile.back().uniqueId == purgeCard.uniqueId) {
        upgradeRampage(exhaustPile.back(), upgradeAmount);
        return;
    }

    for (int i = static_cast<int>(discardPile.size())-2; i >= 0; --i) {
        auto &c = discardPile[i];
        if (c.uniqueId == purgeCard.uniqueId) {
            upgradeRampage(c, upgradeAmount);
            return;
        }
    }

    for (int i = static_cast<int>(exhaustPile.size())-2; i >= 0; --i) {
        auto &c = exhaustPile[i];
        if (c.uniqueId == purgeCard.uniqueId) {
            upgradeRampage(c, upgradeAmount);
            return;
        }
    }

    for (int i = 0; i < drawPile.size(); ++i) {
        auto &c = drawPile[i];
        if (c.uniqueId == purgeCard.uniqueId) {
            upgradeRampage(c, upgradeAmount);
            return;
        }
    }

    for (int i = 0; i < cardsInHand; ++i) {
        auto &c = hand[i];
        if (c.uniqueId == purgeCard.uniqueId) {
            upgradeRampage(c, upgradeAmount);
            return;
        }
    }

}

// **************** END SPECIAL HELPERS ****************


namespace sts {

//    std::ostream &operator<<(std::ostream &os, const CardInstance &c) {
//        return os << "("
//            << c.getName()
//            << ", uid:" << std::to_string(c.uniqueId)
//            << ", u:" << std::to_string(c.upgraded)
//            << ", c:" << c.cost
//            << ", ct:" << c.costForTurn
//            << ")";
//    }

    template<typename Forward_Iterator>
    void printArray(std::ostream &os, Forward_Iterator begin, Forward_Iterator end) {
        os << "{ ";
        while (begin != end && begin+1 != end) {
            os << *begin << ", ";
            ++begin;
        }
        if (begin != end) {
            os << *begin;
        }
        os << " }";
    }

    std::ostream &operator<<(std::ostream &os, const CardManager &c) {
        os << "CardManager: {";

        os << "\n\tdrawPile: " << c.drawPile.size() << " ";
        printArray(os, c.drawPile.begin(), c.drawPile.end());

        os << ",\n\tdiscardPile: " << c.discardPile.size() << " ";
        printArray(os, c.discardPile.begin(), c.discardPile.end());

        os << ",\n\texhaustPile: " << c.exhaustPile.size() << " ";
        printArray(os, c.exhaustPile.begin(), c.exhaustPile.end());

        os << ",\n\thand: " << c.cardsInHand << " ";
        printArray(os, c.hand.begin(), c.hand.begin()+c.cardsInHand);

        os << "\n\t" << "handNormalityCount: " << c.handNormalityCount;
        const auto s = ", ";
        os << s << "handPainCount: " << c.handPainCount;
        os << s << "strikeCount: " << c.strikeCount;

        os << s << "handBloodCardCount: " << c.handBloodCardCount;
        os << s << "drawPileBloodCardCount: " << c.drawPileBloodCardCount;
        os << s << "discardPileBloodCardCount: " << c.discardPileBloodCardCount;

        os << "\n}\n";

        return os;
    }


}
