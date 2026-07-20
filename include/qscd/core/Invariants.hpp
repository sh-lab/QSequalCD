#pragma once

#include "qscd/core/GameState.hpp"

#include <optional>

namespace qscd::core {

std::optional<GameError> validateInvariants(const GameState& state);
std::optional<GameError> validateInvariants(const ProjectState& state);
bool hasQualityStateField();

} // namespace qscd::core
