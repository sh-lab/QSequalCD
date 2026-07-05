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
  for (const auto& [id, definition] : state.cardDefinitions) {
    if (!ids.insert(id).second || definition.id != id) {
      return invariantError(id);
    }
    const auto positionIt = state.positions.find(id);
    if (positionIt == state.positions.end()) {
      return invariantError(id);
    }

    const auto& position = positionIt->second;
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

bool hasQualityStateField() {
  return false;
}

} // namespace qscd::core
