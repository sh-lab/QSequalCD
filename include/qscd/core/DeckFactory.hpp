#pragma once

#include "qscd/core/Card.hpp"
#include "qscd/core/Rules.hpp"

#include <vector>

namespace qscd::core {

std::vector<CardDefinition> createMemberDeck();
std::vector<CardDefinition> createContinuationDeck();
std::vector<CardDefinition> createHandCards();

} // namespace qscd::core
