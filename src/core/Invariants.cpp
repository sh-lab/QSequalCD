#include "qscd/core/Invariants.hpp"

#include "qscd/core/Queries.hpp"

#include <set>

namespace qscd::core {

namespace {

GameError invariantError(std::optional<CardId> id = std::nullopt) {
  return GameError{GameErrorCode::InvariantViolation, id, std::nullopt, std::nullopt};
}

} // namespace

std::optional<GameError> validateInvariants(const GameState& state) {
  if (state.cardDefinitions.size() != state.positions.size()) {
    return invariantError();
  }

  std::set<std::pair<int, int>> boardSlots;
  std::set<CardId> ids;
  int memberCardCount = rules::zeroScore;
  int continuationCardCount = rules::zeroScore;
  int handCardCount = rules::zeroScore;
  for (const auto& [id, definition] : state.cardDefinitions) {
    if (!ids.insert(id).second || definition.id != id) {
      return invariantError(id);
    }
    const auto positionIt = state.positions.find(id);
    if (positionIt == state.positions.end()) {
      return invariantError(id);
    }

    const auto& position = positionIt->second;
    switch (definition.category) {
      case CardCategory::MemberCard:
        ++memberCardCount;
        break;
      case CardCategory::ContinuationCard:
        ++continuationCardCount;
        break;
      case CardCategory::HandCard:
        ++handCardCount;
        break;
    }
    if (const auto* board = std::get_if<BoardPosition>(&position)) {
      if (definition.category != CardCategory::MemberCard) {
        return invariantError(id);
      }
      if (!boardSlots.insert({board->row, board->column}).second) {
        return invariantError(id);
      }
      if (board->column >= rules::firstColumn && board->column < static_cast<int>(state.members.size()) &&
          state.members[static_cast<std::size_t>(board->column)] != MemberState::Active &&
          board->row == state.currentRound && state.phase == GamePhase::RoundStarted) {
        return invariantError(id);
      }
    }

    if (definition.category == CardCategory::HandCard) {
      const auto usageIt = state.handUsages.find(id);
      const bool used = usageIt != state.handUsages.end() && !std::holds_alternative<UnusedHandUsage>(usageIt->second);
      if (used && !std::holds_alternative<BoardSidePosition>(position)) {
        return invariantError(id);
      }
      if (std::holds_alternative<BoardPosition>(position)) {
        return invariantError(id);
      }
    }
  }

  const int expectedContinuationCardCount = state.continuationDeckSet == ContinuationDeckSet::Double
    ? rules::doubleContinuationDeckCardCount
    : rules::continuationDeckCardCount;
  if (memberCardCount != rules::memberDeckCardCount ||
      continuationCardCount != expectedContinuationCardCount ||
      handCardCount != rules::handCardCount) {
    return invariantError();
  }

  std::set<CardId> continuationDeckIds;
  for (int i = 0; i < static_cast<int>(state.continuationDeckOrder.size()); ++i) {
    const auto id = state.continuationDeckOrder[static_cast<std::size_t>(i)];
    if (!continuationDeckIds.insert(id).second) {
      return invariantError(id);
    }
    const auto definitionIt = state.cardDefinitions.find(id);
    const auto positionIt = state.positions.find(id);
    if (definitionIt == state.cardDefinitions.end() ||
        definitionIt->second.category != CardCategory::ContinuationCard ||
        positionIt == state.positions.end()) {
      return invariantError(id);
    }
    const auto* deck = std::get_if<ContinuationDeckPosition>(&positionIt->second);
    if (deck == nullptr || deck->index != i) {
      return invariantError(id);
    }
  }

  for (int round = rules::firstRoundNumber; round <= state.currentRound; ++round) {
    int plannedHandCards = rules::zeroScore;
    for (const auto& [id, position] : state.positions) {
      if (const auto* side = std::get_if<BoardSidePosition>(&position); side != nullptr && side->row == round) {
        ++plannedHandCards;
      }
    }
    if (plannedHandCards > rules::maxHandCardsPerRound) {
      return invariantError();
    }
  }

  return std::nullopt;
}

std::optional<GameError> validateInvariants(const ProjectState& state) {
  if (const auto gameInvariant = validateInvariants(state.currentGame); gameInvariant.has_value()) {
    return gameInvariant;
  }
  if (state.currentGame.memberDeckSet != state.memberDeckSet ||
      state.currentGame.continuationDeckSet != state.continuationDeckSet) {
    return invariantError();
  }
  if (state.completedGameCount < rules::zeroScore ||
      state.cumulativeFinalScore < rules::zeroScore ||
      state.cumulativeFinalCost < rules::zeroScore) {
    return invariantError();
  }
  if (state.mode == ProjectMode::Single && state.completedGameCount > rules::firstRoundNumber) {
    return invariantError();
  }
  if (state.mode == ProjectMode::Large && state.completedGameCount > rules::largeProjectGameCount) {
    return invariantError();
  }
  if (state.mode == ProjectMode::Endless &&
      (state.status == ProjectStatus::Cleared || state.status == ProjectStatus::Failed)) {
    return invariantError();
  }
  if (state.status == ProjectStatus::Cleared && state.mode == ProjectMode::Large &&
      (state.completedGameCount != rules::largeProjectGameCount ||
       state.cumulativeFinalScore < rules::largeProjectTargetScore)) {
    return invariantError();
  }
  if (state.status == ProjectStatus::Failed && state.mode == ProjectMode::Large &&
      (state.completedGameCount != rules::largeProjectGameCount ||
       state.cumulativeFinalScore >= rules::largeProjectTargetScore)) {
    return invariantError();
  }
  return std::nullopt;
}

bool hasQualityStateField() {
  return false;
}

} // namespace qscd::core
