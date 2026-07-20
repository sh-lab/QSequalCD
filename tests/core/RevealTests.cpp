#include "core/TestHelpers.hpp"

using namespace qscd::core;
using namespace qscd::core::tests;

void runRevealTests() {
  for (int expected = rules::minExpectedScore; expected <= rules::maxExpectedScore; ++expected) {
    auto state = startRoundValue(startedGame(defaultTargetScore, rules::maxTeamSize, expected));
    const int base = rules::revealLimitForExpectedScore(expected);
    QSCD_REQUIRE(calculateRevealLimit(state) == std::min(base + rules::teamSizeRevealBonus, static_cast<int>(getRevealableCards(state).size())));
  }

  auto leaveState = startedGame(defaultTargetScore, rules::maxTeamSize, rules::minExpectedScore);
  const auto leave = firstCardOfKind(leaveState, CardKind::MemberLeaveProject);
  putFirst(leaveState, leave);
  leaveState = startRoundValue(leaveState);
  leaveState = planNoHandsValue(leaveState);
  auto leaveRevealable = getRevealableCards(leaveState);
  leaveState = revealCards(leaveState, std::vector<CardId>{leave, leaveRevealable[secondHandIndex]}).value();
  leaveState = finishRoundValue(leaveState);
  leaveState = startRoundValue(leaveState);
  QSCD_REQUIRE(getActiveMembers(leaveState).size() == rules::defaultTeamSize);
  QSCD_REQUIRE(calculateRevealLimit(leaveState) == rules::revealLimitForExpectedScore(rules::minExpectedScore));

  auto state = startedGame(defaultTargetScore, rules::minTeamSize, rules::minExpectedScore);
  state = playSimpleRound(state);
  state = playSimpleRound(state);
  state = playSimpleRound(state);
  auto extra = requestExtraRound(state, ExtraRoundOptions{false});
  QSCD_REQUIRE(extra.hasValue());
  state = startRoundValue(extra.value());
  QSCD_REQUIRE(calculateRevealLimit(state) == rules::revealLimitForExpectedScore(rules::minExpectedScore) + rules::extraRoundRevealBonus);

  auto invalid = revealCards(state, std::vector<CardId>{});
  QSCD_REQUIRE(!invalid.hasValue());
  QSCD_REQUIRE(invalid.error().code == GameErrorCode::InvalidRevealCount);
}
