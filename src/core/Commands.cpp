#include "qscd/core/Commands.hpp"

#include "qscd/core/DeckFactory.hpp"
#include "qscd/core/Invariants.hpp"
#include "qscd/core/Queries.hpp"

#include <algorithm>
#include <cstdint>
#include <random>
#include <set>

namespace qscd::core {

namespace {

Result<GameState, GameError> fail(GameErrorCode code, std::optional<CardId> cardId = std::nullopt) {
  return Result<GameState, GameError>::failure(GameError{code, cardId, std::nullopt, std::nullopt});
}

const CardDefinition* definitionFor(const GameState& state, CardId id) {
  const auto it = state.cardDefinitions.find(id);
  return it == state.cardDefinitions.end() ? nullptr : &it->second;
}

const Position* positionFor(const GameState& state, CardId id) {
  const auto it = state.positions.find(id);
  return it == state.positions.end() ? nullptr : &it->second;
}

bool isCurrentRoundBoardCard(const GameState& state, CardId id) {
  const auto* position = positionFor(state, id);
  const auto* board = position == nullptr ? nullptr : std::get_if<BoardPosition>(position);
  return board != nullptr && board->row == state.currentRound;
}

void insertDefinition(GameState& state, const CardDefinition& definition, Position position) {
  state.cardDefinitions[definition.id] = definition;
  state.positions[definition.id] = position;
  if (definition.category == CardCategory::MemberCard) {
    state.runtimeStates[definition.id] = RuntimeState::FaceDown;
  }
  if (definition.category == CardCategory::HandCard) {
    state.handUsages[definition.id] = UnusedHandUsage{};
  }
}

void reindexMemberDeck(GameState& state) {
  for (int i = 0; i < static_cast<int>(state.memberDeckOrder.size()); ++i) {
    state.positions[state.memberDeckOrder[static_cast<std::size_t>(i)]] = DeckPosition{i};
  }
}

void reindexContinuationDeck(GameState& state) {
  for (int i = 0; i < static_cast<int>(state.continuationDeckOrder.size()); ++i) {
    state.positions[state.continuationDeckOrder[static_cast<std::size_t>(i)]] = ContinuationDeckPosition{i};
  }
}

int nextTeamAreaSlot(const GameState& state) {
  int slot = rules::firstSlot;
  for (const auto& [id, position] : state.positions) {
    (void)id;
    if (const auto* area = std::get_if<ContinuationTeamAreaPosition>(&position); area != nullptr) {
      slot = std::max(slot, area->slot + rules::nextSlotOffset);
    }
  }
  return slot;
}

int nextMemberAreaSlot(const GameState& state, int column) {
  int slot = rules::firstSlot;
  for (const auto& [id, position] : state.positions) {
    (void)id;
    if (const auto* area = std::get_if<ContinuationMemberAreaPosition>(&position); area != nullptr && area->column == column) {
      slot = std::max(slot, area->slot + rules::nextSlotOffset);
    }
  }
  return slot;
}

bool isValidTeamSize(int teamSize) {
  return teamSize >= rules::minTeamSize && teamSize <= rules::maxTeamSize;
}

bool isValidExpectedScore(int expectedScore) {
  return expectedScore >= rules::minExpectedScore && expectedScore <= rules::maxExpectedScore;
}

std::uint32_t derivedSeed(std::uint32_t seed, std::uint32_t salt) {
  return seed ^ (salt + 0x9E3779B9U + (seed << 6U) + (seed >> 2U));
}

void shuffleDeck(std::vector<CardDefinition>& deck, std::uint32_t seed) {
  std::mt19937 engine{seed};
  std::shuffle(deck.begin(), deck.end(), engine);
}

Result<GameState, GameError> checkedSuccess(GameState state) {
  if (const auto invariant = validateInvariants(state); invariant.has_value()) {
    return Result<GameState, GameError>::failure(*invariant);
  }
  return Result<GameState, GameError>::success(std::move(state));
}

} // namespace

Result<GameState, GameError> startGame(const GameSettings& settings) {
  if (!isValidTeamSize(settings.teamSize)) {
    return fail(GameErrorCode::InvalidTeamSize);
  }
  if (!isValidExpectedScore(settings.globalExpectedScore)) {
    return fail(GameErrorCode::InvalidExpectedScore);
  }

  GameState state;
  state.targetScore = settings.targetScore;
  state.teamSize = settings.teamSize;
  state.globalExpectedScore = settings.globalExpectedScore;
  state.costLimit = settings.costLimit;
  state.deckSeed = settings.deckSeed;
  state.phase = GamePhase::GameStarted;
  state.members.assign(static_cast<std::size_t>(settings.teamSize), MemberState::Active);

  auto memberDeck = createMemberDeck();
  shuffleDeck(memberDeck, derivedSeed(settings.deckSeed, 0x4D454D42U));
  for (int i = 0; i < static_cast<int>(memberDeck.size()); ++i) {
    insertDefinition(state, memberDeck[static_cast<std::size_t>(i)], DeckPosition{i});
    state.memberDeckOrder.push_back(memberDeck[static_cast<std::size_t>(i)].id);
  }

  auto continuationDeck = createContinuationDeck();
  shuffleDeck(continuationDeck, derivedSeed(settings.deckSeed, 0x434F4E54U));
  for (int i = 0; i < static_cast<int>(continuationDeck.size()); ++i) {
    insertDefinition(state, continuationDeck[static_cast<std::size_t>(i)], ContinuationDeckPosition{i});
    state.continuationDeckOrder.push_back(continuationDeck[static_cast<std::size_t>(i)].id);
  }

  const auto handCards = createHandCards();
  for (int i = 0; i < static_cast<int>(handCards.size()); ++i) {
    insertDefinition(state, handCards[static_cast<std::size_t>(i)], HandPosition{i});
  }

  return checkedSuccess(std::move(state));
}

Result<GameState, GameError> startRound(const GameState& state) {
  if (state.phase != GamePhase::GameStarted && state.phase != GamePhase::RoundFinished && state.phase != GamePhase::ExtraRoundRequested) {
    return fail(GameErrorCode::InvalidPhase);
  }
  if (state.currentRound >= state.maxRound || state.currentRound >= rules::maxRoundCount) {
    return fail(GameErrorCode::RoundLimitExceeded);
  }
  const auto activeMembers = getActiveMembers(state);
  if (state.memberDeckOrder.size() < activeMembers.size()) {
    return fail(GameErrorCode::DeckEmpty);
  }

  GameState next = state;
  ++next.currentRound;
  for (const int column : activeMembers) {
    const CardId id = next.memberDeckOrder.front();
    next.memberDeckOrder.erase(next.memberDeckOrder.begin());
    next.positions[id] = BoardPosition{next.currentRound, column};
    next.runtimeStates[id] = RuntimeState::FaceDown;
  }
  reindexMemberDeck(next);
  next.phase = GamePhase::RoundStarted;
  return checkedSuccess(std::move(next));
}

Result<GameState, GameError> planHandUsage(const GameState& state, const HandPlan& handPlan) {
  if (state.phase != GamePhase::RoundStarted) {
    return fail(GameErrorCode::InvalidPhase);
  }
  if (handPlan.entries.size() > rules::maxHandCardsPerRound) {
    return fail(GameErrorCode::TooManyHandCards);
  }

  std::set<CardId> seen;
  for (const auto& entry : handPlan.entries) {
    if (!seen.insert(entry.handCardId).second) {
      return fail(GameErrorCode::InvalidHandUsage, entry.handCardId);
    }
    const auto* definition = definitionFor(state, entry.handCardId);
    const auto* position = positionFor(state, entry.handCardId);
    if (definition == nullptr) {
      return fail(GameErrorCode::CardNotFound, entry.handCardId);
    }
    if (definition->category != CardCategory::HandCard || position == nullptr || !std::holds_alternative<HandPosition>(*position)) {
      return fail(GameErrorCode::InvalidHandUsage, entry.handCardId);
    }
    if (std::holds_alternative<UnusedHandUsage>(entry.usage)) {
      return fail(GameErrorCode::InvalidHandUsage, entry.handCardId);
    }
  }

  GameState next = state;
  int slot = rules::firstSlot;
  for (const auto& entry : handPlan.entries) {
    next.positions[entry.handCardId] = BoardSidePosition{state.currentRound, slot++};
    next.handUsages[entry.handCardId] = entry.usage;
  }
  next.phase = GamePhase::HandPlanned;
  return checkedSuccess(std::move(next));
}

Result<GameState, GameError> revealCards(const GameState& state, const std::vector<CardId>& cardIds) {
  if (state.phase != GamePhase::RoundStarted && state.phase != GamePhase::HandPlanned) {
    return fail(GameErrorCode::InvalidPhase);
  }
  if (static_cast<int>(cardIds.size()) != calculateRevealLimit(state)) {
    return fail(GameErrorCode::InvalidRevealCount);
  }
  std::set<CardId> seen;
  const auto revealable = getRevealableCards(state);
  for (const auto id : cardIds) {
    if (!seen.insert(id).second || std::find(revealable.begin(), revealable.end(), id) == revealable.end()) {
      return fail(GameErrorCode::InvalidRevealTarget, id);
    }
  }

  GameState next = state;
  for (const auto id : cardIds) {
    next.runtimeStates[id] = RuntimeState::FaceUp;
  }
  next.phase = GamePhase::CardsRevealed;
  return checkedSuccess(std::move(next));
}

Result<GameState, GameError> revealCards(const GameState& state, const RevealSelection& selection) {
  return revealCards(state, selection.cardIds);
}

Result<GameState, GameError> applyNullify(const GameState& state, CardId handCardId, CardId targetCardId) {
  if (state.phase != GamePhase::CardsRevealed) {
    return fail(GameErrorCode::InvalidPhase);
  }
  const auto* handDefinition = definitionFor(state, handCardId);
  const auto* handPosition = positionFor(state, handCardId);
  if (handDefinition == nullptr) {
    return fail(GameErrorCode::CardNotFound, handCardId);
  }
  if (handDefinition->category != CardCategory::HandCard) {
    return fail(GameErrorCode::InvalidHandUsage, handCardId);
  }
  const auto* boardSide = handPosition == nullptr ? nullptr : std::get_if<BoardSidePosition>(handPosition);
  if (boardSide == nullptr || boardSide->row != state.currentRound) {
    return fail(GameErrorCode::InvalidHandUsage, handCardId);
  }
  const auto usageIt = state.handUsages.find(handCardId);
  const auto* nullifyUsage = usageIt == state.handUsages.end() ? nullptr : std::get_if<NullifyHandUsage>(&usageIt->second);
  if (nullifyUsage == nullptr || nullifyUsage->targetCardId.has_value()) {
    return fail(GameErrorCode::InvalidHandUsage, handCardId);
  }

  const auto* targetDefinition = definitionFor(state, targetCardId);
  if (targetDefinition == nullptr) {
    return fail(GameErrorCode::CardNotFound, targetCardId);
  }
  const auto runtimeIt = state.runtimeStates.find(targetCardId);
  if (targetDefinition->category != CardCategory::MemberCard || !isCurrentRoundBoardCard(state, targetCardId) ||
      runtimeIt == state.runtimeStates.end() || runtimeIt->second != RuntimeState::FaceUp) {
    return fail(GameErrorCode::InvalidNullifyTarget, targetCardId);
  }

  GameState next = state;
  next.runtimeStates[targetCardId] = RuntimeState::NullifiedAsFaceDown;
  next.handUsages[handCardId] = NullifyHandUsage{targetCardId};
  return checkedSuccess(std::move(next));
}

Result<GameState, GameError> finishRound(const GameState& state) {
  if (state.phase != GamePhase::CardsRevealed) {
    return fail(GameErrorCode::InvalidPhase);
  }
  GameState next = state;
  for (const auto id : getBoardCards(state, state.currentRound)) {
    const auto* definition = definitionFor(state, id);
    const auto runtimeIt = state.runtimeStates.find(id);
    const auto* position = positionFor(state, id);
    const auto* board = position == nullptr ? nullptr : std::get_if<BoardPosition>(position);
    if (definition != nullptr && board != nullptr && definition->kind == CardKind::MemberLeaveProject &&
        runtimeIt != state.runtimeStates.end() && runtimeIt->second == RuntimeState::FaceUp) {
      next.members[static_cast<std::size_t>(board->column)] = MemberState::LeftProject;
    }
  }
  next.phase = GamePhase::RoundFinished;
  return checkedSuccess(std::move(next));
}

Result<GameState, GameError> requestExtraRound(const GameState& state, const ExtraRoundOptions& options) {
  if (state.phase != GamePhase::RoundFinished) {
    return fail(GameErrorCode::InvalidPhase);
  }
  if (state.currentRound < rules::baseRoundCount || state.currentRound >= rules::maxRoundCount || calculateFinalScore(state) >= state.targetScore) {
    return fail(GameErrorCode::ExtraRoundNotAllowed);
  }

  GameState next = state;
  next.maxRound = std::max(next.maxRound, next.currentRound + rules::extraRoundRevealBonus);
  if (options.forcedUnpaidOvertime) {
    next.hasUsedForcedUnpaidOvertime = true;
    next.forcedUnpaidOvertimeRounds.push_back(next.currentRound + rules::extraRoundRevealBonus);
  }
  next.phase = GamePhase::ExtraRoundRequested;
  return checkedSuccess(std::move(next));
}

Result<GameState, GameError> finishGame(const GameState& state) {
  if (state.phase != GamePhase::RoundFinished) {
    return fail(GameErrorCode::InvalidPhase);
  }
  GameState next = state;
  next.finalScore = calculateFinalScore(next);
  next.finalCost = calculateFinalCost(next);
  next.phase = GamePhase::GameFinished;
  return checkedSuccess(std::move(next));
}

Result<GameState, GameError> drawContinuationCards(const GameState& state) {
  if (state.phase != GamePhase::GameFinished) {
    return fail(GameErrorCode::InvalidPhase);
  }
  const int drawCount = static_cast<int>(getActiveMembers(state).size());
  if (static_cast<int>(state.continuationDeckOrder.size()) < drawCount) {
    return fail(GameErrorCode::DeckEmpty);
  }

  GameState next = state;
  for (int column = rules::firstColumn; column < static_cast<int>(next.members.size()); ++column) {
    if (next.members[static_cast<std::size_t>(column)] != MemberState::Active) {
      continue;
    }
    const CardId id = next.continuationDeckOrder.front();
    next.continuationDeckOrder.erase(next.continuationDeckOrder.begin());
    const auto* definition = definitionFor(next, id);
    if (definition == nullptr) {
      return fail(GameErrorCode::CardNotFound, id);
    }

    switch (definition->kind) {
      case CardKind::ContinuationAudit:
        if (next.hasUsedForcedUnpaidOvertime) {
          next.isDefeatedByAudit = true;
        }
        next.positions[id] = OutOfGamePosition{};
        break;
      case CardKind::ContinuationResign:
        next.members[static_cast<std::size_t>(column)] = MemberState::Resigned;
        next.positions[id] = OutOfGamePosition{};
        break;
      case CardKind::ContinuationTeamScoreUp:
        next.positions[id] = ContinuationTeamAreaPosition{nextTeamAreaSlot(next)};
        break;
      case CardKind::ContinuationMemberCostUp:
      case CardKind::ContinuationMemberScoreUp:
        next.positions[id] = ContinuationMemberAreaPosition{column, nextMemberAreaSlot(next, column)};
        break;
      case CardKind::ContinuationNone:
        next.positions[id] = OutOfGamePosition{};
        break;
      default:
        return fail(GameErrorCode::InvalidCardId, id);
    }
  }
  reindexContinuationDeck(next);
  next.finalScore = calculateFinalScore(next);
  next.finalCost = calculateFinalCost(next);
  next.phase = GamePhase::ContinuationCardsDrawn;
  return checkedSuccess(std::move(next));
}

Result<GameState, GameError> startContinuationGame(const GameState& state, const ContinuationSettings& settings) {
  if (state.phase != GamePhase::ContinuationCardsDrawn && state.phase != GamePhase::GameFinished) {
    return fail(GameErrorCode::InvalidPhase);
  }
  if (!isValidTeamSize(settings.teamSize)) {
    return fail(GameErrorCode::InvalidTeamSize);
  }
  if (!isValidExpectedScore(settings.globalExpectedScore)) {
    return fail(GameErrorCode::InvalidExpectedScore);
  }
  if (settings.targetScore < calculateFinalScore(state)) {
    return fail(GameErrorCode::InvalidExpectedScore);
  }

  GameState next = state;
  next.targetScore = settings.targetScore;
  next.teamSize = settings.teamSize;
  next.globalExpectedScore = settings.globalExpectedScore;
  next.costLimit = settings.costLimit;
  next.deckSeed = settings.deckSeed;
  next.currentRound = rules::zeroScore;
  next.maxRound = rules::baseRoundCount;
  next.isContinuationGame = true;
  next.hasUsedForcedUnpaidOvertime = false;
  next.isDefeatedByAudit = false;
  next.forcedUnpaidOvertimeRounds.clear();
  next.finalScore.reset();
  next.finalCost.reset();
  next.members.assign(static_cast<std::size_t>(settings.teamSize), MemberState::Active);

  for (auto& [id, position] : next.positions) {
    const auto* definition = definitionFor(next, id);
    if (definition == nullptr) {
      continue;
    }
    if (definition->category == CardCategory::MemberCard || definition->category == CardCategory::HandCard) {
      position = OutOfGamePosition{};
    }
    if (settings.teamCompositionChanged && definition->kind == CardKind::ContinuationTeamScoreUp) {
      position = OutOfGamePosition{};
    }
  }

  next.memberDeckOrder.clear();
  auto memberDeck = createMemberDeck();
  shuffleDeck(memberDeck, derivedSeed(settings.deckSeed, 0x4D454D42U));
  for (int i = 0; i < static_cast<int>(memberDeck.size()); ++i) {
    insertDefinition(next, memberDeck[static_cast<std::size_t>(i)], DeckPosition{i});
    next.memberDeckOrder.push_back(memberDeck[static_cast<std::size_t>(i)].id);
  }

  const auto handCards = createHandCards();
  for (int i = 0; i < static_cast<int>(handCards.size()); ++i) {
    insertDefinition(next, handCards[static_cast<std::size_t>(i)], HandPosition{i});
  }

  next.phase = GamePhase::GameStarted;
  return checkedSuccess(std::move(next));
}

} // namespace qscd::core
