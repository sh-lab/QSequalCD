#include "core/TestHelpers.hpp"

using namespace qscd::core;
using namespace qscd::core::tests;

void runScoringTests() {
  auto state = startRoundValue(startedGame(defaultTargetScore, rules::minTeamSize, rules::minExpectedScore + rules::memberCostUpValue));
  const auto hands = handCards(state);
  state = planHandUsage(state, HandPlan{{{hands[firstHandIndex], ScorePlus3HandUsage{}}, {hands[secondHandIndex], NullifyHandUsage{std::nullopt}}}}).value();
  const auto revealable = getRevealableCards(state);
  const auto faceUp = revealable[firstHandIndex];
  const auto nullifiedTarget = revealable[secondHandIndex];
  state = revealCards(state, std::vector<CardId>{faceUp, nullifiedTarget}).value();
  const int expectedScore = rules::minExpectedScore + rules::memberCostUpValue;
  const int faceUpScore = state.cardDefinitions.at(faceUp).scoreValue.value_or(rules::zeroScore);
  state = applyNullify(state, hands[secondHandIndex], nullifiedTarget).value();
  QSCD_REQUIRE(calculateRoundScore(state, rules::firstRoundNumber) ==
               faceUpScore + expectedScore + expectedScore + rules::scorePlus3HandValue);
}
