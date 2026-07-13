#pragma once

#include "qscd/core/Commands.hpp"
#include "qscd/core/DeckFactory.hpp"
#include "qscd/core/Invariants.hpp"
#include "qscd/core/Queries.hpp"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace qscd::core::tests {

inline constexpr int lowTargetScore = rules::memberCostUpValue;
inline constexpr int defaultTargetScore = rules::defaultTargetScore;
inline constexpr int unreachableTargetScore = 1000;
inline constexpr int invalidLowTeamSize = rules::minTeamSize - rules::memberCostUpValue;
inline constexpr int invalidHighTeamSize = rules::maxTeamSize + rules::memberCostUpValue;
inline constexpr int invalidLowExpectedScore = rules::minExpectedScore - rules::memberCostUpValue;
inline constexpr int invalidHighExpectedScore = rules::maxExpectedScore + rules::memberCostUpValue;
inline constexpr int firstHandIndex = rules::firstSlot;
inline constexpr int secondHandIndex = rules::secondColumn;
inline constexpr int thirdHandIndex = rules::thirdColumn;
inline constexpr int invalidScoreValue = -rules::memberCostUpValue;
inline constexpr CardId invalidCardId{rules::zeroScore};

inline void require(bool condition, const char* expression, const char* file, int line) {
  if (!condition) {
    std::cerr << file << ":" << line << ": requirement failed: " << expression << "\n";
    std::exit(EXIT_FAILURE);
  }
}

#define QSCD_REQUIRE(expr) ::qscd::core::tests::require(static_cast<bool>(expr), #expr, __FILE__, __LINE__)

inline GameState startedGame(int targetScore = defaultTargetScore, int teamSize = rules::minTeamSize, int expectedScore = rules::minExpectedScore) {
  auto result = startGame(GameSettings{targetScore, teamSize, expectedScore, rules::defaultCostLimit});
  QSCD_REQUIRE(result.hasValue());
  return result.value();
}

inline std::vector<CardId> handCards(const GameState& state) {
  std::vector<CardId> cards;
  for (const auto& [id, definition] : state.cardDefinitions) {
    if (definition.category == CardCategory::HandCard && std::holds_alternative<HandPosition>(state.positions.at(id))) {
      cards.push_back(id);
    }
  }
  std::sort(cards.begin(), cards.end());
  return cards;
}

inline CardId firstCardOfKind(const GameState& state, CardKind kind) {
  for (const auto& [id, definition] : state.cardDefinitions) {
    if (definition.kind == kind) {
      return id;
    }
  }
  QSCD_REQUIRE(false);
  return invalidCardId;
}

inline CardId firstContinuationOfKind(const GameState& state, CardKind kind) {
  for (const auto& [id, definition] : state.cardDefinitions) {
    if (definition.category == CardCategory::ContinuationCard && definition.kind == kind) {
      return id;
    }
  }
  QSCD_REQUIRE(false);
  return invalidCardId;
}

inline void putFirst(GameState& state, CardId id) {
  auto& deck = state.memberDeckOrder;
  deck.erase(std::remove(deck.begin(), deck.end(), id), deck.end());
  deck.insert(deck.begin(), id);
}

inline void putContinuationFirst(GameState& state, const std::vector<CardId>& ids) {
  auto& deck = state.continuationDeckOrder;
  for (const auto id : ids) {
    deck.erase(std::remove(deck.begin(), deck.end(), id), deck.end());
  }
  deck.insert(deck.begin(), ids.begin(), ids.end());
}

inline GameState startRoundValue(const GameState& state) {
  auto result = startRound(state);
  QSCD_REQUIRE(result.hasValue());
  return result.value();
}

inline GameState planNoHandsValue(const GameState& state) {
  auto result = planHandUsage(state, HandPlan{});
  QSCD_REQUIRE(result.hasValue());
  return result.value();
}

inline GameState revealRequiredValue(const GameState& state) {
  const auto revealable = getRevealableCards(state);
  const int count = calculateRevealLimit(state);
  std::vector<CardId> selected(revealable.begin(), revealable.begin() + count);
  auto result = revealCards(state, selected);
  QSCD_REQUIRE(result.hasValue());
  return result.value();
}

inline GameState finishRoundValue(const GameState& state) {
  auto result = finishRound(state);
  QSCD_REQUIRE(result.hasValue());
  return result.value();
}

inline GameState playSimpleRound(GameState state) {
  state = startRoundValue(state);
  state = planNoHandsValue(state);
  state = revealRequiredValue(state);
  return finishRoundValue(state);
}

inline GameState playBaseRounds(GameState state) {
  for (int round = rules::firstRoundNumber; round <= rules::baseRoundCount; ++round) {
    state = playSimpleRound(state);
  }
  return state;
}

} // namespace qscd::core::tests
