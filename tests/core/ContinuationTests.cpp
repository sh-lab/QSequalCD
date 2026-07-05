#include "core/TestHelpers.hpp"

using namespace qscd::core;
using namespace qscd::core::tests;

void runContinuationTests() {
  auto state = startedGame(lowTargetScore, rules::minTeamSize, rules::minExpectedScore);
  state.members[rules::secondColumn] = MemberState::LeftProject;
  state = playSimpleRound(state);
  state = finishGame(state).value();
  const auto audit = firstContinuationOfKind(state, CardKind::ContinuationAudit);
  const auto resign = firstContinuationOfKind(state, CardKind::ContinuationResign);
  putContinuationFirst(state, {audit, resign});
  auto drawn = drawContinuationCards(state);
  QSCD_REQUIRE(drawn.hasValue());
  QSCD_REQUIRE(std::holds_alternative<OutOfGamePosition>(drawn.value().positions.at(audit)));
  QSCD_REQUIRE(drawn.value().members[rules::secondColumn] == MemberState::LeftProject);
  QSCD_REQUIRE(drawn.value().members[rules::thirdColumn] == MemberState::Resigned);
  QSCD_REQUIRE(!drawn.value().isDefeatedByAudit);

  state = startedGame(unreachableTargetScore, rules::minTeamSize, rules::minExpectedScore);
  state = playSimpleRound(playSimpleRound(playSimpleRound(state)));
  state = requestExtraRound(state, ExtraRoundOptions{true}).value();
  state = playSimpleRound(state);
  state = finishGame(state).value();
  putContinuationFirst(state, {firstContinuationOfKind(state, CardKind::ContinuationAudit)});
  drawn = drawContinuationCards(state);
  QSCD_REQUIRE(drawn.hasValue());
  QSCD_REQUIRE(drawn.value().isDefeatedByAudit);
  QSCD_REQUIRE(judgeResult(drawn.value()) == JudgeResult::DefeatByAudit);

  state = startedGame(lowTargetScore, rules::minTeamSize, rules::minExpectedScore);
  state = playSimpleRound(state);
  state = finishGame(state).value();
  const auto teamUp = firstContinuationOfKind(state, CardKind::ContinuationTeamScoreUp);
  const auto costUp = firstContinuationOfKind(state, CardKind::ContinuationMemberCostUp);
  const auto scoreUp = firstContinuationOfKind(state, CardKind::ContinuationMemberScoreUp);
  putContinuationFirst(state, {teamUp, costUp, scoreUp});
  drawn = drawContinuationCards(state);
  QSCD_REQUIRE(drawn.hasValue());
  const int scoreWithContinuation = calculateFinalScore(drawn.value());
  const int costWithContinuation = calculateFinalCost(drawn.value());
  QSCD_REQUIRE(scoreWithContinuation >= calculateFinalScore(state) + rules::minTeamSize + rules::memberScoreUpValue);
  QSCD_REQUIRE(costWithContinuation == calculateFinalCost(state) + rules::memberCostUpValue);
  constexpr std::uint32_t continuationSeed = 24680U;
  auto next = startContinuationGame(drawn.value(), ContinuationSettings{scoreWithContinuation, rules::minTeamSize, rules::minExpectedScore, std::nullopt, true, continuationSeed});
  QSCD_REQUIRE(next.hasValue());
  auto repeatedNext = startContinuationGame(drawn.value(), ContinuationSettings{scoreWithContinuation, rules::minTeamSize, rules::minExpectedScore, std::nullopt, true, continuationSeed});
  QSCD_REQUIRE(repeatedNext.hasValue());
  auto differentNext = startContinuationGame(drawn.value(), ContinuationSettings{scoreWithContinuation, rules::minTeamSize, rules::minExpectedScore, std::nullopt, true, continuationSeed + rules::memberCostUpValue});
  QSCD_REQUIRE(differentNext.hasValue());
  QSCD_REQUIRE(next.value().deckSeed == continuationSeed);
  QSCD_REQUIRE(next.value().memberDeckOrder == repeatedNext.value().memberDeckOrder);
  QSCD_REQUIRE(next.value().memberDeckOrder != differentNext.value().memberDeckOrder);
  QSCD_REQUIRE(std::holds_alternative<OutOfGamePosition>(next.value().positions.at(teamUp)));

  state = startedGame(lowTargetScore, rules::minTeamSize, rules::minExpectedScore);
  state = playSimpleRound(state);
  state = finishGame(state).value();
  const auto none = firstContinuationOfKind(state, CardKind::ContinuationNone);
  putContinuationFirst(state, {none});
  drawn = drawContinuationCards(state);
  QSCD_REQUIRE(drawn.hasValue());
  QSCD_REQUIRE(std::holds_alternative<OutOfGamePosition>(drawn.value().positions.at(none)));
}
