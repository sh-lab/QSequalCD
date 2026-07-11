#pragma once

#include "qscd/core/Card.hpp"
#include "qscd/core/Rules.hpp"

#include <cstdint>
#include <optional>
#include <unordered_map>
#include <vector>

namespace qscd::core {

struct GameSettings {
  int targetScore{};
  int teamSize{};
  int globalExpectedScore{};
  std::optional<int> costLimit;
  std::uint32_t deckSeed{rules::defaultDeckSeed};
};

struct ContinuationSettings {
  int targetScore{};
  int teamSize{};
  int globalExpectedScore{};
  std::optional<int> costLimit;
  bool teamCompositionChanged{};
  std::uint32_t deckSeed{rules::defaultDeckSeed};
  std::vector<int> retiringColumns;
};

struct HandPlanEntry {
  CardId handCardId;
  HandUsage usage;
};

struct HandPlan {
  std::vector<HandPlanEntry> entries;
};

struct RevealSelection {
  std::vector<CardId> cardIds;
};

struct ExtraRoundOptions {
  bool forcedUnpaidOvertime{};
};

struct GameState {
  int targetScore{};
  int teamSize{};
  int globalExpectedScore{};
  int currentRound{};
  int maxRound{rules::baseRoundCount};
  bool isContinuationGame{};
  bool hasUsedForcedUnpaidOvertime{};
  bool isDefeatedByAudit{};
  std::optional<int> costLimit;
  std::uint32_t deckSeed{rules::defaultDeckSeed};
  GamePhase phase{GamePhase::Created};

  std::unordered_map<CardId, CardDefinition> cardDefinitions;
  std::unordered_map<CardId, Position> positions;
  std::unordered_map<CardId, RuntimeState> runtimeStates;
  std::unordered_map<CardId, HandUsage> handUsages;
  std::vector<MemberState> members;
  std::vector<CardId> memberDeckOrder;
  std::vector<CardId> continuationDeckOrder;
  std::vector<int> forcedUnpaidOvertimeRounds;

  std::optional<int> finalScore;
  std::optional<int> finalCost;
};

} // namespace qscd::core
