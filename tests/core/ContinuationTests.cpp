#include "core/TestHelpers.hpp"

using namespace qscd::core;
using namespace qscd::core::tests;

void runContinuationTests() {
  auto state = startedGame(lowTargetScore, rules::minTeamSize, rules::minExpectedScore);
  state.members[rules::secondColumn] = MemberState::LeftProject;
  state = playBaseRounds(state);
  state = finishGame(state).value();
  const auto audit = firstContinuationOfKind(state, CardKind::ContinuationAudit);
  const auto resign = firstContinuationOfKind(state, CardKind::ContinuationResign);
  putContinuationFirst(state, {audit, resign});
  auto drawn = drawContinuationCards(state);
  QSCD_REQUIRE(drawn.hasValue());
  QSCD_REQUIRE(std::holds_alternative<ContinuationDeckPosition>(drawn.value().positions.at(audit)));
  QSCD_REQUIRE(drawn.value().members[rules::secondColumn] == MemberState::LeftProject);
  QSCD_REQUIRE(drawn.value().members[rules::thirdColumn] == MemberState::Resigned);
  QSCD_REQUIRE(!drawn.value().isDefeatedByAudit);

  state = startedGame(unreachableTargetScore, rules::minTeamSize, rules::minExpectedScore);
  state = playBaseRounds(state);
  state = requestExtraRound(state, ExtraRoundOptions{true}).value();
  state = playSimpleRound(state);
  state = requestExtraRound(state, ExtraRoundOptions{false}).value();
  state = playSimpleRound(state);
  state = finishGame(state).value();
  putContinuationFirst(state, {firstContinuationOfKind(state, CardKind::ContinuationAudit)});
  drawn = drawContinuationCards(state);
  QSCD_REQUIRE(drawn.hasValue());
  QSCD_REQUIRE(drawn.value().isDefeatedByAudit);
  QSCD_REQUIRE(judgeResult(drawn.value()) == JudgeResult::DefeatByAudit);

  state = startedGame(lowTargetScore, rules::minTeamSize, rules::minExpectedScore);
  state = playBaseRounds(state);
  state = finishGame(state).value();
  const auto teamUp = firstContinuationOfKind(state, CardKind::ContinuationTeamScoreUp);
  const auto costUp = firstContinuationOfKind(state, CardKind::ContinuationMemberCostUp);
  const auto scoreUp = firstContinuationOfKind(state, CardKind::ContinuationMemberScoreUp);
  putContinuationFirst(state, {teamUp, costUp, scoreUp});
  const int finalizedScore = *state.finalScore;
  const int finalizedCost = *state.finalCost;
  drawn = drawContinuationCards(state);
  QSCD_REQUIRE(drawn.hasValue());
  QSCD_REQUIRE(drawn.value().finalScore.has_value());
  QSCD_REQUIRE(drawn.value().finalCost.has_value());
  QSCD_REQUIRE(*drawn.value().finalScore == finalizedScore);
  QSCD_REQUIRE(*drawn.value().finalCost == finalizedCost);
  QSCD_REQUIRE(calculateFinalScore(drawn.value()) >= calculateFinalScore(state) + rules::minTeamSize + rules::memberScoreUpValue);
  QSCD_REQUIRE(calculateFinalCost(drawn.value()) == calculateFinalCost(state) + rules::memberCostUpValue);
  const auto pressure = firstContinuationOfKind(state, CardKind::ContinuationCostReductionPressure);
  auto pressureState = state;
  putContinuationFirst(pressureState, {pressure});
  auto pressureDrawn = drawContinuationCards(pressureState);
  QSCD_REQUIRE(pressureDrawn.hasValue());
  QSCD_REQUIRE(std::holds_alternative<ContinuationDeckPosition>(pressureDrawn.value().positions.at(pressure)));
  QSCD_REQUIRE(pressureDrawn.value().pendingCostLimitReduction == rules::costReductionPressureValue);
  QSCD_REQUIRE(pressureDrawn.value().cumulativeCostLimitReduction == rules::zeroScore);
  QSCD_REQUIRE(*pressureDrawn.value().finalScore == finalizedScore);
  QSCD_REQUIRE(*pressureDrawn.value().finalCost == finalizedCost);
  auto pressureNext = startContinuationGame(
    pressureDrawn.value(),
    ContinuationSettings{*pressureDrawn.value().finalScore, rules::minTeamSize, rules::minExpectedScore, rules::defaultCostLimit}
  );
  QSCD_REQUIRE(pressureNext.hasValue());
  QSCD_REQUIRE(pressureNext.value().costLimit == rules::defaultCostLimit - rules::costReductionPressureValue);
  QSCD_REQUIRE(pressureNext.value().pendingCostLimitReduction == rules::zeroScore);
  QSCD_REQUIRE(pressureNext.value().cumulativeCostLimitReduction == rules::costReductionPressureValue);
  auto pressureNextFinished = playBaseRounds(pressureNext.value());
  pressureNextFinished = finishGame(pressureNextFinished).value();
  auto pressureThird = startContinuationGame(
    pressureNextFinished,
    ContinuationSettings{*pressureNextFinished.finalScore, rules::minTeamSize, rules::minExpectedScore, rules::defaultCostLimit}
  );
  QSCD_REQUIRE(pressureThird.hasValue());
  QSCD_REQUIRE(pressureThird.value().costLimit == rules::defaultCostLimit - rules::costReductionPressureValue);
  QSCD_REQUIRE(pressureThird.value().cumulativeCostLimitReduction == rules::costReductionPressureValue);
  QSCD_REQUIRE(pressureThird.value().pendingCostLimitReduction == rules::zeroScore);
  constexpr std::uint32_t continuationSeed = 24680U;
  const int nextTargetScore = *drawn.value().finalScore;
  auto next = startContinuationGame(drawn.value(), ContinuationSettings{nextTargetScore, rules::minTeamSize, rules::minExpectedScore, rules::defaultCostLimit, true, continuationSeed});
  QSCD_REQUIRE(next.hasValue());
  auto repeatedNext = startContinuationGame(drawn.value(), ContinuationSettings{nextTargetScore, rules::minTeamSize, rules::minExpectedScore, rules::defaultCostLimit, true, continuationSeed});
  QSCD_REQUIRE(repeatedNext.hasValue());
  auto differentNext = startContinuationGame(drawn.value(), ContinuationSettings{nextTargetScore, rules::minTeamSize, rules::minExpectedScore, rules::defaultCostLimit, true, continuationSeed + rules::memberCostUpValue});
  QSCD_REQUIRE(differentNext.hasValue());
  QSCD_REQUIRE(next.value().deckSeed == continuationSeed);
  QSCD_REQUIRE(next.value().memberDeckOrder == repeatedNext.value().memberDeckOrder);
  QSCD_REQUIRE(next.value().memberDeckOrder != differentNext.value().memberDeckOrder);
  QSCD_REQUIRE(std::holds_alternative<ContinuationDeckPosition>(next.value().positions.at(teamUp)));

  auto retiredNext = startContinuationGame(
    drawn.value(),
    ContinuationSettings{nextTargetScore, rules::minTeamSize, rules::minExpectedScore, rules::defaultCostLimit, true, continuationSeed, {rules::firstColumn}}
  );
  QSCD_REQUIRE(retiredNext.hasValue());
  QSCD_REQUIRE(std::holds_alternative<ContinuationDeckPosition>(retiredNext.value().positions.at(teamUp)));
  const auto* movedCostUp = std::get_if<ContinuationMemberAreaPosition>(&retiredNext.value().positions.at(costUp));
  QSCD_REQUIRE(movedCostUp != nullptr);
  QSCD_REQUIRE(movedCostUp->column == rules::firstColumn);
  const auto* movedScoreUp = std::get_if<ContinuationMemberAreaPosition>(&retiredNext.value().positions.at(scoreUp));
  QSCD_REQUIRE(movedScoreUp != nullptr);
  QSCD_REQUIRE(movedScoreUp->column == rules::secondColumn);

  auto memberExpiredNext = startContinuationGame(
    drawn.value(),
    ContinuationSettings{nextTargetScore, rules::minTeamSize, rules::minExpectedScore, rules::defaultCostLimit, true, continuationSeed, {rules::secondColumn}}
  );
  QSCD_REQUIRE(memberExpiredNext.hasValue());
  QSCD_REQUIRE(std::holds_alternative<ContinuationDeckPosition>(memberExpiredNext.value().positions.at(teamUp)));
  QSCD_REQUIRE(std::holds_alternative<ContinuationDeckPosition>(memberExpiredNext.value().positions.at(costUp)));
  const auto* retainedScoreUp = std::get_if<ContinuationMemberAreaPosition>(&memberExpiredNext.value().positions.at(scoreUp));
  QSCD_REQUIRE(retainedScoreUp != nullptr);
  QSCD_REQUIRE(retainedScoreUp->column == rules::secondColumn);

  state = startedGame(lowTargetScore, rules::minTeamSize, rules::minExpectedScore);
  state = playBaseRounds(state);
  state = finishGame(state).value();
  const auto none = firstContinuationOfKind(state, CardKind::ContinuationNone);
  putContinuationFirst(state, {
    firstContinuationOfKind(state, CardKind::ContinuationAudit),
    firstContinuationOfKind(state, CardKind::ContinuationResign),
    none,
  });
  const auto deckSizeBeforeNone = state.continuationDeckOrder.size();
  drawn = drawContinuationCards(state);
  QSCD_REQUIRE(drawn.hasValue());
  QSCD_REQUIRE(std::holds_alternative<ContinuationDeckPosition>(drawn.value().positions.at(none)));
  QSCD_REQUIRE(drawn.value().continuationDeckOrder.size() == deckSizeBeforeNone);
}
