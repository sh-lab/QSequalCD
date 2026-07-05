#include "core/TestHelpers.hpp"

using namespace qscd::core;
using namespace qscd::core::tests;

namespace {

std::vector<CardId> sortedIds(std::vector<CardId> ids) {
  std::sort(ids.begin(), ids.end());
  return ids;
}

std::vector<CardId> canonicalMemberIds() {
  std::vector<CardId> ids;
  for (const auto& card : createMemberDeck()) {
    ids.push_back(card.id);
  }
  return sortedIds(ids);
}

std::vector<CardId> canonicalContinuationIds() {
  std::vector<CardId> ids;
  for (const auto& card : createContinuationDeck()) {
    ids.push_back(card.id);
  }
  return sortedIds(ids);
}

} // namespace

void runStartGameTests() {
  QSCD_REQUIRE(startGame(GameSettings{defaultTargetScore, invalidLowTeamSize, rules::minExpectedScore, std::nullopt}).error().code == GameErrorCode::InvalidTeamSize);
  QSCD_REQUIRE(startGame(GameSettings{defaultTargetScore, invalidHighTeamSize, rules::minExpectedScore, std::nullopt}).error().code == GameErrorCode::InvalidTeamSize);
  QSCD_REQUIRE(startGame(GameSettings{defaultTargetScore, rules::minTeamSize, invalidLowExpectedScore, std::nullopt}).error().code == GameErrorCode::InvalidExpectedScore);
  QSCD_REQUIRE(startGame(GameSettings{defaultTargetScore, rules::minTeamSize, invalidHighExpectedScore, std::nullopt}).error().code == GameErrorCode::InvalidExpectedScore);

  const auto state = startedGame();
  QSCD_REQUIRE(state.phase == GamePhase::GameStarted);
  QSCD_REQUIRE(!hasQualityStateField());
  QSCD_REQUIRE(handCards(state).size() == rules::handCardCount);
  QSCD_REQUIRE(state.members.size() == rules::minTeamSize);
  std::set<CardId> ids;
  for (const auto& [id, definition] : state.cardDefinitions) {
    (void)definition;
    QSCD_REQUIRE(ids.insert(id).second);
  }
  QSCD_REQUIRE(ids.size() == rules::memberDeckCardCount + rules::continuationDeckCardCount + rules::handCardCount);
  for (const auto member : state.members) {
    QSCD_REQUIRE(member == MemberState::Active);
  }

  constexpr std::uint32_t firstSeed = 12345U;
  constexpr std::uint32_t secondSeed = 67890U;
  const auto firstSeedState = startGame(GameSettings{defaultTargetScore, rules::minTeamSize, rules::minExpectedScore, std::nullopt, firstSeed}).value();
  const auto repeatedFirstSeedState = startGame(GameSettings{defaultTargetScore, rules::minTeamSize, rules::minExpectedScore, std::nullopt, firstSeed}).value();
  const auto secondSeedState = startGame(GameSettings{defaultTargetScore, rules::minTeamSize, rules::minExpectedScore, std::nullopt, secondSeed}).value();
  QSCD_REQUIRE(firstSeedState.deckSeed == firstSeed);
  QSCD_REQUIRE(firstSeedState.memberDeckOrder == repeatedFirstSeedState.memberDeckOrder);
  QSCD_REQUIRE(firstSeedState.continuationDeckOrder == repeatedFirstSeedState.continuationDeckOrder);
  QSCD_REQUIRE(firstSeedState.memberDeckOrder != secondSeedState.memberDeckOrder);
  QSCD_REQUIRE(firstSeedState.continuationDeckOrder != secondSeedState.continuationDeckOrder);
  QSCD_REQUIRE(sortedIds(firstSeedState.memberDeckOrder) == canonicalMemberIds());
  QSCD_REQUIRE(sortedIds(firstSeedState.continuationDeckOrder) == canonicalContinuationIds());
  QSCD_REQUIRE(firstSeedState.memberDeckOrder != canonicalMemberIds());
  QSCD_REQUIRE(firstSeedState.continuationDeckOrder != canonicalContinuationIds());
}
