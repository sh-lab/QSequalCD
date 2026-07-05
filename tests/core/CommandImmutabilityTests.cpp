#include "core/TestHelpers.hpp"

using namespace qscd::core;
using namespace qscd::core::tests;

void runCommandImmutabilityTests() {
  auto state = startRoundValue(startedGame());
  const auto before = state;
  auto invalid = revealCards(state, std::vector<CardId>{});
  QSCD_REQUIRE(!invalid.hasValue());
  QSCD_REQUIRE(state.currentRound == before.currentRound);
  QSCD_REQUIRE(state.phase == before.phase);
  QSCD_REQUIRE(state.positions == before.positions);
  QSCD_REQUIRE(state.runtimeStates == before.runtimeStates);
}
