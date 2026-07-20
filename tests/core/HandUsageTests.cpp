#include "core/TestHelpers.hpp"

using namespace qscd::core;
using namespace qscd::core::tests;

void runHandUsageTests() {
  auto state = startRoundValue(startedGame(defaultTargetScore, rules::minTeamSize, rules::minExpectedScore));
  const auto hands = handCards(state);
  auto tooMany = planHandUsage(state, HandPlan{{{hands[firstHandIndex], ScorePlus3HandUsage{}}, {hands[secondHandIndex], ScorePlus3HandUsage{}}, {hands[thirdHandIndex], ScorePlus3HandUsage{}}}});
  QSCD_REQUIRE(!tooMany.hasValue());
  QSCD_REQUIRE(tooMany.error().code == GameErrorCode::TooManyHandCards);

  auto planned = planHandUsage(state, HandPlan{{{hands[firstHandIndex], ScorePlus3HandUsage{}}, {hands[secondHandIndex], NullifyHandUsage{std::nullopt}}}});
  QSCD_REQUIRE(planned.hasValue());
  QSCD_REQUIRE(std::holds_alternative<BoardSidePosition>(planned.value().positions.at(hands[firstHandIndex])));

  auto revealable = getRevealableCards(planned.value());
  auto revealed = revealCards(planned.value(), std::vector<CardId>{revealable[firstHandIndex]});
  QSCD_REQUIRE(revealed.hasValue());

  auto unplanned = applyNullify(revealed.value(), hands[thirdHandIndex], revealable[firstHandIndex]);
  QSCD_REQUIRE(!unplanned.hasValue());
  QSCD_REQUIRE(unplanned.error().code == GameErrorCode::InvalidHandUsage);

  auto nullified = applyNullify(revealed.value(), hands[secondHandIndex], revealable[firstHandIndex]);
  QSCD_REQUIRE(nullified.hasValue());
  QSCD_REQUIRE(nullified.value().runtimeStates.at(revealable[firstHandIndex]) == RuntimeState::NullifiedAsFaceDown);
  QSCD_REQUIRE(calculateRoundScore(nullified.value(), rules::firstRoundNumber) ==
               (rules::minTeamSize * rules::minExpectedScore) + rules::scorePlus3HandValue);

  auto leaveState = startedGame(defaultTargetScore, rules::minTeamSize, rules::minExpectedScore);
  const auto leave = firstCardOfKind(leaveState, CardKind::MemberLeaveProject);
  putFirst(leaveState, leave);
  leaveState = startRoundValue(leaveState);
  const auto leaveHands = handCards(leaveState);
  leaveState = planHandUsage(leaveState, HandPlan{{{leaveHands[firstHandIndex], NullifyHandUsage{std::nullopt}}}}).value();
  leaveState = revealCards(leaveState, std::vector<CardId>{leave}).value();
  leaveState = applyNullify(leaveState, leaveHands[firstHandIndex], leave).value();
  leaveState = finishRoundValue(leaveState);
  QSCD_REQUIRE(leaveState.members[rules::firstColumn] == MemberState::Active);
  QSCD_REQUIRE(calculateFinalCost(leaveState) == rules::minTeamSize);

  auto previousRoundState = startRoundValue(startedGame(defaultTargetScore, rules::minTeamSize, rules::minExpectedScore));
  previousRoundState = planNoHandsValue(previousRoundState);
  const auto previousRoundCards = getRevealableCards(previousRoundState);
  const auto previousRoundCard = previousRoundCards[firstHandIndex];
  previousRoundState = revealCards(previousRoundState, std::vector<CardId>{previousRoundCard}).value();
  previousRoundState = finishRoundValue(previousRoundState);
  previousRoundState = startRoundValue(previousRoundState);
  const auto previousRoundHands = handCards(previousRoundState);
  previousRoundState = planHandUsage(previousRoundState, HandPlan{{{previousRoundHands[firstHandIndex], NullifyHandUsage{std::nullopt}}}}).value();
  const auto currentRoundCards = getRevealableCards(previousRoundState);
  previousRoundState = revealCards(previousRoundState, std::vector<CardId>{currentRoundCards[firstHandIndex]}).value();
  const auto beforePreviousRoundAttempt = previousRoundState;
  auto previousRoundNullify = applyNullify(previousRoundState, previousRoundHands[firstHandIndex], previousRoundCard);
  QSCD_REQUIRE(!previousRoundNullify.hasValue());
  QSCD_REQUIRE(previousRoundNullify.error().code == GameErrorCode::InvalidNullifyTarget);
  QSCD_REQUIRE(previousRoundState.phase == beforePreviousRoundAttempt.phase);
  QSCD_REQUIRE(previousRoundState.positions == beforePreviousRoundAttempt.positions);
  QSCD_REQUIRE(previousRoundState.runtimeStates == beforePreviousRoundAttempt.runtimeStates);
  QSCD_REQUIRE(previousRoundState.handUsages == beforePreviousRoundAttempt.handUsages);

  auto reuseState = startRoundValue(startedGame(defaultTargetScore, rules::minTeamSize, rules::minExpectedScore + rules::memberCostUpValue));
  const auto reuseHands = handCards(reuseState);
  reuseState = planHandUsage(reuseState, HandPlan{{{reuseHands[firstHandIndex], NullifyHandUsage{std::nullopt}}}}).value();
  const auto reuseRevealable = getRevealableCards(reuseState);
  reuseState = revealCards(reuseState, std::vector<CardId>{reuseRevealable[firstHandIndex], reuseRevealable[secondHandIndex]}).value();
  reuseState = applyNullify(reuseState, reuseHands[firstHandIndex], reuseRevealable[firstHandIndex]).value();
  const auto beforeReuseAttempt = reuseState;
  auto reusedNullify = applyNullify(reuseState, reuseHands[firstHandIndex], reuseRevealable[secondHandIndex]);
  QSCD_REQUIRE(!reusedNullify.hasValue());
  QSCD_REQUIRE(reusedNullify.error().code == GameErrorCode::InvalidHandUsage);
  QSCD_REQUIRE(reuseState.phase == beforeReuseAttempt.phase);
  QSCD_REQUIRE(reuseState.positions == beforeReuseAttempt.positions);
  QSCD_REQUIRE(reuseState.runtimeStates == beforeReuseAttempt.runtimeStates);
  QSCD_REQUIRE(reuseState.handUsages == beforeReuseAttempt.handUsages);
}
