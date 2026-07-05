#include "qscd/core/Queries.hpp"

#include <algorithm>
#include <set>

namespace qscd::core {

namespace {

const CardDefinition* definitionFor(const GameState& state, CardId id) {
  const auto it = state.cardDefinitions.find(id);
  return it == state.cardDefinitions.end() ? nullptr : &it->second;
}

bool isForcedRound(const GameState& state, int round) {
  return std::find(state.forcedUnpaidOvertimeRounds.begin(), state.forcedUnpaidOvertimeRounds.end(), round) != state.forcedUnpaidOvertimeRounds.end();
}

} // namespace

int calculateRoundScore(const GameState& state, int round) {
  int score = rules::zeroScore;
  for (const auto& [id, position] : state.positions) {
    const auto* board = std::get_if<BoardPosition>(&position);
    if (board == nullptr || board->row != round) {
      continue;
    }
    const auto* definition = definitionFor(state, id);
    if (definition == nullptr || definition->category != CardCategory::MemberCard) {
      continue;
    }
    const auto runtimeIt = state.runtimeStates.find(id);
    const auto runtime = runtimeIt == state.runtimeStates.end() ? RuntimeState::FaceDown : runtimeIt->second;
    if (runtime == RuntimeState::FaceDown || runtime == RuntimeState::NullifiedAsFaceDown) {
      score += state.globalExpectedScore;
    } else if (definition->kind == CardKind::MemberScore && definition->scoreValue.has_value()) {
      score += *definition->scoreValue;
    }
  }

  for (const auto& [id, position] : state.positions) {
    const auto* boardSide = std::get_if<BoardSidePosition>(&position);
    if (boardSide == nullptr || boardSide->row != round) {
      continue;
    }
    const auto usageIt = state.handUsages.find(id);
    if (usageIt != state.handUsages.end() && std::holds_alternative<ScorePlus3HandUsage>(usageIt->second)) {
      score += rules::scorePlus3HandValue;
    }
  }
  return score;
}

int calculateFinalScore(const GameState& state) {
  int score = rules::zeroScore;
  for (int round = rules::firstRoundNumber; round <= state.currentRound; ++round) {
    score += calculateRoundScore(state, round);
  }
  for (const auto& [id, position] : state.positions) {
    const auto* definition = definitionFor(state, id);
    if (definition == nullptr || definition->category != CardCategory::ContinuationCard) {
      continue;
    }
    if (definition->kind == CardKind::ContinuationMemberScoreUp && std::holds_alternative<ContinuationMemberAreaPosition>(position)) {
      score += rules::memberScoreUpValue;
    }
    if (definition->kind == CardKind::ContinuationTeamScoreUp && std::holds_alternative<ContinuationTeamAreaPosition>(position)) {
      score += state.teamSize;
    }
  }
  return score;
}

int calculateFinalCost(const GameState& state) {
  int cost = rules::zeroScore;
  for (const auto& [id, position] : state.positions) {
    const auto* board = std::get_if<BoardPosition>(&position);
    if (board != nullptr) {
      const auto* definition = definitionFor(state, id);
      if (definition != nullptr && definition->category == CardCategory::MemberCard && !isForcedRound(state, board->row)) {
        cost += rules::memberCostUpValue;
      }
      continue;
    }
    const auto* definition = definitionFor(state, id);
    if (definition != nullptr && definition->kind == CardKind::ContinuationMemberCostUp && std::holds_alternative<ContinuationMemberAreaPosition>(position)) {
      ++cost;
    }
  }
  return cost;
}

int calculateRevealLimit(const GameState& state) {
  int limit = rules::zeroScore;
  if (state.globalExpectedScore >= rules::minExpectedScore && state.globalExpectedScore <= rules::maxExpectedScore) {
    limit = rules::revealLimitForExpectedScore(state.globalExpectedScore);
  }
  if (state.teamSize == rules::teamSizeRevealBonusThreshold) {
    limit += rules::teamSizeRevealBonus;
  }
  if (state.currentRound > rules::baseRoundCount) {
    limit += rules::extraRoundRevealBonus;
  }
  const auto revealable = getRevealableCards(state);
  return std::min(limit, static_cast<int>(revealable.size()));
}

std::vector<CardId> getRevealableCards(const GameState& state) {
  std::vector<CardId> cards;
  for (const auto& [id, position] : state.positions) {
    const auto* board = std::get_if<BoardPosition>(&position);
    if (board == nullptr || board->row != state.currentRound) {
      continue;
    }
    const auto* definition = definitionFor(state, id);
    const auto runtimeIt = state.runtimeStates.find(id);
    if (definition != nullptr && definition->category == CardCategory::MemberCard &&
        runtimeIt != state.runtimeStates.end() && runtimeIt->second == RuntimeState::FaceDown) {
      cards.push_back(id);
    }
  }
  std::sort(cards.begin(), cards.end());
  return cards;
}

std::vector<int> getActiveMembers(const GameState& state) {
  std::vector<int> members;
  for (int i = 0; i < static_cast<int>(state.members.size()); ++i) {
    if (state.members[static_cast<std::size_t>(i)] == MemberState::Active) {
      members.push_back(i);
    }
  }
  return members;
}

std::vector<CardId> getBoardCards(const GameState& state, int round) {
  std::vector<CardId> cards;
  for (const auto& [id, position] : state.positions) {
    if (const auto* board = std::get_if<BoardPosition>(&position); board != nullptr && board->row == round) {
      cards.push_back(id);
    }
  }
  std::sort(cards.begin(), cards.end());
  return cards;
}

std::vector<CardId> getBoardSideCards(const GameState& state, int round) {
  std::vector<CardId> cards;
  for (const auto& [id, position] : state.positions) {
    if (const auto* side = std::get_if<BoardSidePosition>(&position); side != nullptr && side->row == round) {
      cards.push_back(id);
    }
  }
  std::sort(cards.begin(), cards.end());
  return cards;
}

std::vector<CardId> getContinuationCardsForMember(const GameState& state, int column) {
  std::vector<CardId> cards;
  for (const auto& [id, position] : state.positions) {
    if (const auto* area = std::get_if<ContinuationMemberAreaPosition>(&position); area != nullptr && area->column == column) {
      cards.push_back(id);
    }
  }
  std::sort(cards.begin(), cards.end());
  return cards;
}

JudgeResult judgeResult(const GameState& state) {
  if (state.phase != GamePhase::GameFinished &&
      state.phase != GamePhase::ContinuationCardsDrawn &&
      state.phase != GamePhase::ContinuationGamePrepared) {
    return JudgeResult::InProgress;
  }
  if (state.isDefeatedByAudit) {
    return JudgeResult::DefeatByAudit;
  }
  const int finalScore = calculateFinalScore(state);
  if (finalScore < state.targetScore) {
    return JudgeResult::DefeatByScore;
  }
  const int finalCost = calculateFinalCost(state);
  if (state.costLimit.has_value() && finalCost > *state.costLimit) {
    return JudgeResult::DefeatByCost;
  }
  return JudgeResult::Victory;
}

} // namespace qscd::core
