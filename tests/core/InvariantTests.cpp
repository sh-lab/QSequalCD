#include "core/TestHelpers.hpp"

using namespace qscd::core;
using namespace qscd::core::tests;

void runInvariantTests() {
  auto state = startedGame();
  QSCD_REQUIRE(!validateInvariants(state).has_value());
  QSCD_REQUIRE(!hasQualityStateField());

  state = startRoundValue(state);
  const auto hands = handCards(state);
  const auto board = getBoardCards(state, rules::firstRoundNumber);
  state.positions[hands[firstHandIndex]] = BoardPosition{rules::firstRoundNumber, rules::firstColumn};
  QSCD_REQUIRE(validateInvariants(state).has_value());

  state = startedGame();
  state = startRoundValue(state);
  state.positions[board[firstHandIndex]] = BoardPosition{rules::firstRoundNumber, rules::firstColumn};
}
