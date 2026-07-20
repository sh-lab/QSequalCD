#pragma once

#include "qscd/core/Card.hpp"
#include "qscd/core/Rules.hpp"

#include <vector>

namespace qscd::core {

std::vector<CardDefinition> createMemberDeck(MemberDeckSet set = MemberDeckSet::Stable);
std::vector<CardDefinition> createContinuationDeck(ContinuationDeckSet set = ContinuationDeckSet::Standard);
std::vector<CardDefinition> createHandCards();

} // namespace qscd::core
