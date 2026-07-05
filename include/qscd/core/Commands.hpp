#pragma once

#include "qscd/core/GameState.hpp"
#include "qscd/core/Result.hpp"

#include <vector>

namespace qscd::core {

Result<GameState, GameError> startGame(const GameSettings& settings);
Result<GameState, GameError> startRound(const GameState& state);
Result<GameState, GameError> planHandUsage(const GameState& state, const HandPlan& handPlan);
Result<GameState, GameError> revealCards(const GameState& state, const std::vector<CardId>& cardIds);
Result<GameState, GameError> revealCards(const GameState& state, const RevealSelection& selection);
Result<GameState, GameError> applyNullify(const GameState& state, CardId handCardId, CardId targetCardId);
Result<GameState, GameError> finishRound(const GameState& state);
Result<GameState, GameError> requestExtraRound(const GameState& state, const ExtraRoundOptions& options);
Result<GameState, GameError> finishGame(const GameState& state);
Result<GameState, GameError> drawContinuationCards(const GameState& state);
Result<GameState, GameError> startContinuationGame(const GameState& state, const ContinuationSettings& settings);

} // namespace qscd::core
