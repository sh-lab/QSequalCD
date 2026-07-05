#pragma once

#include "qscd/core/GameState.hpp"

#include <vector>

namespace qscd::core {

int calculateRoundScore(const GameState& state, int round);
int calculateFinalScore(const GameState& state);
int calculateFinalCost(const GameState& state);
int calculateRevealLimit(const GameState& state);
std::vector<CardId> getRevealableCards(const GameState& state);
std::vector<int> getActiveMembers(const GameState& state);
std::vector<CardId> getBoardCards(const GameState& state, int round);
std::vector<CardId> getBoardSideCards(const GameState& state, int round);
std::vector<CardId> getContinuationCardsForMember(const GameState& state, int column);
JudgeResult judgeResult(const GameState& state);

} // namespace qscd::core
