#pragma once

#include <cstdint>

extern "C" {

using QSCD_GameHandle = std::uint32_t;

enum QSCD_WasmHandUsageCode : std::int32_t {
  QSCD_WASM_HAND_USAGE_SCORE_PLUS_3 = 1,
  QSCD_WASM_HAND_USAGE_NULLIFY = 2,
};

QSCD_GameHandle createGame(
  std::int32_t targetScore,
  std::int32_t teamSize,
  std::int32_t globalExpectedScore,
  std::int32_t hasCostLimit,
  std::int32_t costLimit,
  std::uint32_t deckSeed
);

void destroyGame(QSCD_GameHandle handle);

std::int32_t startRound(QSCD_GameHandle handle);
std::int32_t planHandUsage(
  QSCD_GameHandle handle,
  const std::uint32_t* handCardIds,
  const std::int32_t* usageCodes,
  std::int32_t count
);
std::int32_t revealCards(QSCD_GameHandle handle, const std::uint32_t* cardIds, std::int32_t count);
std::int32_t applyNullify(QSCD_GameHandle handle, std::uint32_t handCardId, std::uint32_t targetCardId);
std::int32_t finishRound(QSCD_GameHandle handle);
std::int32_t requestExtraRound(QSCD_GameHandle handle, std::int32_t forcedUnpaidOvertime);
std::int32_t finishGame(QSCD_GameHandle handle);
const char* getStateSnapshot(QSCD_GameHandle handle);

}
