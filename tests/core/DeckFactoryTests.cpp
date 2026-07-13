#include "core/TestHelpers.hpp"

using namespace qscd::core;
using namespace qscd::core::tests;

void runDeckFactoryTests() {
  const auto members = createMemberDeck();
  QSCD_REQUIRE(members.size() == rules::memberDeckCardCount);
  std::map<int, int> scoreCounts;
  int leaveCount = 0;
  for (const auto& card : members) {
    if (card.kind == CardKind::MemberScore) {
      ++scoreCounts[card.scoreValue.value_or(invalidScoreValue)];
    } else if (card.kind == CardKind::MemberLeaveProject) {
      ++leaveCount;
    }
  }
  for (const auto& [score, count] : rules::memberScoreDistribution) {
    QSCD_REQUIRE(scoreCounts[score] == count);
  }
  QSCD_REQUIRE(leaveCount == rules::leaveProjectCardCount);

  const auto continuation = createContinuationDeck();
  QSCD_REQUIRE(continuation.size() == rules::continuationDeckCardCount);
  std::map<CardKind, int> continuationCounts;
  for (const auto& card : continuation) {
    ++continuationCounts[card.kind];
  }
  QSCD_REQUIRE(continuationCounts[CardKind::ContinuationAudit] == rules::auditCardCount);
  QSCD_REQUIRE(continuationCounts[CardKind::ContinuationResign] == rules::resignCardCount);
  QSCD_REQUIRE(continuationCounts[CardKind::ContinuationTeamScoreUp] == rules::teamScoreUpCardCount);
  QSCD_REQUIRE(continuationCounts[CardKind::ContinuationMemberCostUp] == rules::memberCostUpCardCount);
  QSCD_REQUIRE(continuationCounts[CardKind::ContinuationMemberScoreUp] == rules::memberScoreUpCardCount);
  QSCD_REQUIRE(continuationCounts[CardKind::ContinuationCostReductionPressure] == rules::costReductionPressureCardCount);
  QSCD_REQUIRE(continuationCounts[CardKind::ContinuationNone] == rules::noneContinuationCardCount);

  const auto hands = createHandCards();
  QSCD_REQUIRE(hands.size() == rules::handCardCount);

  std::set<CardId> ids;
  for (const auto& card : members) {
    QSCD_REQUIRE(ids.insert(card.id).second);
  }
  for (const auto& card : continuation) {
    QSCD_REQUIRE(ids.insert(card.id).second);
  }
  for (const auto& card : hands) {
    QSCD_REQUIRE(ids.insert(card.id).second);
  }
  QSCD_REQUIRE(ids.size() == rules::memberDeckCardCount + rules::continuationDeckCardCount + rules::handCardCount);
  QSCD_REQUIRE(members.front().id == cardId(CardIdValue::MemberScore0_01));
  QSCD_REQUIRE(hands.front().id == cardId(CardIdValue::Hand_01));
  QSCD_REQUIRE(continuation.front().id == cardId(CardIdValue::ContinuationAudit_01));
}
