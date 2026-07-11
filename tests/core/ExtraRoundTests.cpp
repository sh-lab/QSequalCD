#include "core/TestHelpers.hpp"

using namespace qscd::core;
using namespace qscd::core::tests;

void runExtraRoundTests() {
  auto earlyFinish = startedGame(lowTargetScore, rules::minTeamSize, rules::minExpectedScore);
  earlyFinish = playSimpleRound(earlyFinish);
  auto finish = finishGame(earlyFinish);
  QSCD_REQUIRE(!finish.hasValue());
  QSCD_REQUIRE(finish.error().code == GameErrorCode::FinishGameNotAllowed);

  earlyFinish = playSimpleRound(earlyFinish);
  earlyFinish = playSimpleRound(earlyFinish);
  finish = finishGame(earlyFinish);
  QSCD_REQUIRE(finish.hasValue());

  auto state = startedGame(unreachableTargetScore, rules::minTeamSize, rules::minExpectedScore);
  state = playSimpleRound(state);
  auto early = requestExtraRound(state, ExtraRoundOptions{false});
  QSCD_REQUIRE(!early.hasValue());
  QSCD_REQUIRE(early.error().code == GameErrorCode::ExtraRoundNotAllowed);

  state = playSimpleRound(state);
  state = playSimpleRound(state);
  finish = finishGame(state);
  QSCD_REQUIRE(!finish.hasValue());
  QSCD_REQUIRE(finish.error().code == GameErrorCode::FinishGameNotAllowed);

  auto extra = requestExtraRound(state, ExtraRoundOptions{true});
  QSCD_REQUIRE(extra.hasValue());
  state = startRoundValue(extra.value());
  QSCD_REQUIRE(calculateRevealLimit(state) == rules::revealLimitForExpectedScore(rules::minExpectedScore) + rules::extraRoundRevealBonus);
  state = planNoHandsValue(state);
  const auto revealable = getRevealableCards(state);
  state = revealCards(state, std::vector<CardId>{revealable[firstHandIndex], revealable[secondHandIndex]}).value();
  state = finishRoundValue(state);
  QSCD_REQUIRE(calculateFinalCost(state) == rules::baseRoundCount * rules::minTeamSize);
  finish = finishGame(state);
  QSCD_REQUIRE(!finish.hasValue());
  QSCD_REQUIRE(finish.error().code == GameErrorCode::FinishGameNotAllowed);

  extra = requestExtraRound(state, ExtraRoundOptions{false});
  QSCD_REQUIRE(extra.hasValue());
  state = playSimpleRound(extra.value());
  QSCD_REQUIRE(state.currentRound == rules::maxRoundCount);
  QSCD_REQUIRE(!requestExtraRound(state, ExtraRoundOptions{false}).hasValue());
  auto finished = finishGame(state);
  QSCD_REQUIRE(finished.hasValue());
  QSCD_REQUIRE(judgeResult(finished.value()) != JudgeResult::DefeatByAudit);
}
