#include "core/TestHelpers.hpp"

using namespace qscd::core;
using namespace qscd::core::tests;

void runFinishRoundTests() {
  auto state = startedGame(defaultTargetScore, rules::minTeamSize, rules::minExpectedScore);
  const auto leave = firstCardOfKind(state, CardKind::MemberLeaveProject);
  putFirst(state, leave);
  state = startRoundValue(state);
  state = planNoHandsValue(state);
  state = revealCards(state, std::vector<CardId>{leave}).value();
  state = finishRoundValue(state);
  QSCD_REQUIRE(state.members[rules::firstColumn] == MemberState::LeftProject);
  QSCD_REQUIRE(calculateFinalCost(state) == rules::minTeamSize - rules::memberCostUpValue);
  state = startRoundValue(state);
  const int secondRound = rules::firstRoundNumber + rules::memberCostUpValue;
  QSCD_REQUIRE(getBoardCards(state, secondRound).size() == rules::maxHandCardsPerRound);
  QSCD_REQUIRE(calculateRoundScore(state, secondRound) == rules::maxHandCardsPerRound * rules::minExpectedScore);
}
