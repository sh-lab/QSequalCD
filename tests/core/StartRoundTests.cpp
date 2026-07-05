#include "core/TestHelpers.hpp"

using namespace qscd::core;
using namespace qscd::core::tests;

void runStartRoundTests() {
  auto state = startedGame();
  auto round = startRound(state);
  QSCD_REQUIRE(round.hasValue());
  QSCD_REQUIRE(getBoardCards(round.value(), rules::firstRoundNumber).size() == rules::minTeamSize);
  for (const auto id : getBoardCards(round.value(), rules::firstRoundNumber)) {
    QSCD_REQUIRE(round.value().cardDefinitions.at(id).category == CardCategory::MemberCard);
    QSCD_REQUIRE(round.value().runtimeStates.at(id) == RuntimeState::FaceDown);
  }

  state.members[rules::secondColumn] = MemberState::LeftProject;
  state.members[rules::thirdColumn] = MemberState::Resigned;
  round = startRound(state);
  QSCD_REQUIRE(round.hasValue());
  QSCD_REQUIRE(getBoardCards(round.value(), rules::firstRoundNumber).size() == rules::memberCostUpValue);

  state = startedGame();
  state.memberDeckOrder.clear();
  round = startRound(state);
  QSCD_REQUIRE(!round.hasValue());
  QSCD_REQUIRE(round.error().code == GameErrorCode::DeckEmpty);
}
