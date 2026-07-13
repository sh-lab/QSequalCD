#pragma once

#include "qscd/core/Card.hpp"

#include <array>
#include <cstdint>
#include <utility>

namespace qscd::core::rules {

inline constexpr int minTeamSize = 3;
inline constexpr int maxTeamSize = 6;
inline constexpr int minExpectedScore = 3;
inline constexpr int maxExpectedScore = 6;
inline constexpr int defaultTargetScore = 63;
inline constexpr int defaultCostLimit = 30;
inline constexpr int baseRoundCount = 3;
inline constexpr int maxRoundCount = 5;
inline constexpr int maxHandCardsPerRound = 2;

inline constexpr int handCardCount = 5;
inline constexpr int memberDeckCardCount = 35;
inline constexpr int continuationDeckCardCount = 14;
inline constexpr int leaveProjectCardCount = 1;
inline constexpr int auditCardCount = 1;
inline constexpr int resignCardCount = 1;
inline constexpr int teamScoreUpCardCount = 2;
inline constexpr int memberCostUpCardCount = 3;
inline constexpr int memberScoreUpCardCount = 3;
inline constexpr int costReductionPressureCardCount = 1;
inline constexpr int noneContinuationCardCount = 3;

inline constexpr int scorePlus3HandValue = 3;
inline constexpr int memberScoreUpValue = 2;
inline constexpr int memberCostUpValue = 1;
inline constexpr int costReductionPressureValue = 1;
inline constexpr int teamSizeRevealBonusThreshold = 6;
inline constexpr int teamSizeRevealBonus = 1;
inline constexpr int extraRoundRevealBonus = 1;
inline constexpr int firstRoundNumber = 1;
inline constexpr int firstSlot = 0;
inline constexpr int firstColumn = 0;
inline constexpr int secondColumn = 1;
inline constexpr int thirdColumn = 2;
inline constexpr int nextSlotOffset = 1;
inline constexpr int zeroScore = 0;
inline constexpr int memberScoreKindCount = 7;
inline constexpr int revealLimitExpectedScoreOffset = 2;
inline constexpr std::uint32_t defaultDeckSeed = 0x51534344U;

inline constexpr std::array<std::pair<int, int>, memberScoreKindCount> memberScoreDistribution{{
  {0, 2},
  {1, 3},
  {2, 4},
  {3, 6},
  {4, 10},
  {5, 7},
  {6, 2},
}};

inline constexpr int revealLimitForExpectedScore(int expectedScore) {
  return expectedScore - revealLimitExpectedScoreOffset;
}

} // namespace qscd::core::rules
