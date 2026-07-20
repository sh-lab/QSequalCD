#include "core/TestHelpers.hpp"

using namespace qscd::core;
using namespace qscd::core::tests;

namespace {

ProjectState startedProject(
  ProjectMode mode,
  MemberDeckSet memberDeckSet = MemberDeckSet::Stable,
  ContinuationDeckSet continuationDeckSet = ContinuationDeckSet::Standard
) {
  ProjectSettings settings;
  settings.mode = mode;
  settings.memberDeckSet = memberDeckSet;
  settings.continuationDeckSet = continuationDeckSet;
  settings.initialGameSettings = GameSettings{};
  auto result = startProject(settings);
  QSCD_REQUIRE(result.hasValue());
  return result.value();
}

GameState finishedGame(const ProjectState& project, int score, int cost, bool defeatedByAudit = false) {
  GameState game = project.currentGame;
  game.finalScore = score;
  game.finalCost = cost;
  game.targetScore = score;
  game.costLimit = cost;
  game.isDefeatedByAudit = defeatedByAudit;
  game.phase = GamePhase::ContinuationCardsDrawn;
  return game;
}

ProjectState completeVictory(ProjectState project, int score, int cost) {
  auto completed = completeProjectGame(project, finishedGame(project, score, cost));
  QSCD_REQUIRE(completed.hasValue());
  return completed.value();
}

} // namespace

void runProjectTests() {
  auto single = startedProject(ProjectMode::Single);
  QSCD_REQUIRE(single.currentGame.memberDeckSet == MemberDeckSet::Stable);
  QSCD_REQUIRE(single.currentGame.continuationDeckSet == ContinuationDeckSet::Standard);
  single = completeVictory(single, rules::defaultTargetScore, rules::defaultCostLimit);
  QSCD_REQUIRE(single.status == ProjectStatus::Cleared);
  QSCD_REQUIRE(single.completedGameCount == rules::firstRoundNumber);
  QSCD_REQUIRE(!startContinuationGame(single, ContinuationSettings{}).hasValue());

  auto auditedSingle = startedProject(ProjectMode::Single);
  auto audited = completeProjectGame(
    auditedSingle,
    finishedGame(auditedSingle, rules::defaultTargetScore, rules::defaultCostLimit, true)
  );
  QSCD_REQUIRE(audited.hasValue());
  QSCD_REQUIRE(audited.value().status == ProjectStatus::Failed);

  auto large = startedProject(ProjectMode::Large, MemberDeckSet::HighRisk, ContinuationDeckSet::Double);
  QSCD_REQUIRE(large.currentGame.costLimit == rules::defaultCostLimit);
  QSCD_REQUIRE(large.currentGame.memberDeckSet == MemberDeckSet::HighRisk);
  QSCD_REQUIRE(large.currentGame.continuationDeckSet == ContinuationDeckSet::Double);
  QSCD_REQUIRE(large.currentGame.cardDefinitions.size() ==
               rules::memberDeckCardCount + rules::doubleContinuationDeckCardCount + rules::handCardCount);

  large = completeVictory(large, 70, 20);
  QSCD_REQUIRE(large.status == ProjectStatus::InProgress);
  QSCD_REQUIRE(large.completedGameCount == rules::firstRoundNumber);
  QSCD_REQUIRE(large.cumulativeFinalScore == 70);
  QSCD_REQUIRE(large.cumulativeFinalCost == 20);

  ContinuationSettings continuationSettings;
  continuationSettings.targetScore = 70;
  continuationSettings.teamSize = rules::defaultTeamSize;
  continuationSettings.globalExpectedScore = rules::defaultExpectedScore;
  continuationSettings.costLimit = rules::defaultCostLimit;
  auto continued = startContinuationGame(large, continuationSettings);
  QSCD_REQUIRE(continued.hasValue());
  QSCD_REQUIRE(continued.value().currentGame.memberDeckSet == MemberDeckSet::HighRisk);
  QSCD_REQUIRE(continued.value().currentGame.continuationDeckSet == ContinuationDeckSet::Double);
  QSCD_REQUIRE(continued.value().currentGame.memberDeckOrder.size() == rules::memberDeckCardCount);

  large = continued.value();
  large = completeVictory(large, 70, 20);
  continued = startContinuationGame(large, continuationSettings);
  QSCD_REQUIRE(continued.hasValue());
  large = completeVictory(continued.value(), 70, 20);
  QSCD_REQUIRE(large.status == ProjectStatus::Cleared);
  QSCD_REQUIRE(large.completedGameCount == rules::largeProjectGameCount);
  QSCD_REQUIRE(large.cumulativeFinalScore == 210);
  QSCD_REQUIRE(large.cumulativeFinalCost == 60);
  const auto efficiency = calculateProjectEfficiency(large);
  QSCD_REQUIRE(efficiency.has_value());
  QSCD_REQUIRE(*efficiency == 3.5);
  QSCD_REQUIRE(judgeProjectResult(large) == ProjectStatus::Cleared);

  auto underTarget = startedProject(ProjectMode::Large);
  underTarget = completeVictory(underTarget, 60, 15);
  underTarget = completeVictory(underTarget, 60, 15);
  underTarget = completeVictory(underTarget, 60, 15);
  QSCD_REQUIRE(underTarget.status == ProjectStatus::Failed);
  QSCD_REQUIRE(underTarget.cumulativeFinalScore == 180);

  auto collapsedLarge = startedProject(ProjectMode::Large);
  GameState costDefeat = finishedGame(collapsedLarge, rules::defaultTargetScore, rules::defaultCostLimit + 1);
  costDefeat.costLimit = rules::defaultCostLimit;
  auto collapsed = completeProjectGame(collapsedLarge, costDefeat);
  QSCD_REQUIRE(collapsed.hasValue());
  QSCD_REQUIRE(collapsed.value().status == ProjectStatus::Collapsed);

  auto endless = startedProject(ProjectMode::Endless);
  endless = completeVictory(endless, rules::defaultTargetScore, rules::defaultCostLimit);
  QSCD_REQUIRE(endless.status == ProjectStatus::InProgress);
  auto endlessAudit = completeProjectGame(
    endless,
    finishedGame(endless, rules::defaultTargetScore, rules::defaultCostLimit, true)
  );
  QSCD_REQUIRE(endlessAudit.hasValue());
  QSCD_REQUIRE(endlessAudit.value().status == ProjectStatus::Collapsed);
  QSCD_REQUIRE(!calculateProjectEfficiency(endlessAudit.value()).has_value());

  auto changedSetProject = startedProject(ProjectMode::Endless);
  GameState changedSetGame = finishedGame(changedSetProject, rules::defaultTargetScore, rules::defaultCostLimit);
  changedSetGame.memberDeckSet = MemberDeckSet::HighRisk;
  auto changedSet = completeProjectGame(changedSetProject, changedSetGame);
  QSCD_REQUIRE(!changedSet.hasValue());
  QSCD_REQUIRE(changedSet.error().code == GameErrorCode::ProjectSetChangeNotAllowed);
}
