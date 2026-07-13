#include "qscd_wasm.hpp"

#include "qscd/core/Commands.hpp"
#include "qscd/core/Queries.hpp"

#include <algorithm>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#ifndef EMSCRIPTEN_KEEPALIVE
#define EMSCRIPTEN_KEEPALIVE
#endif

namespace {

using namespace qscd::core;

struct WasmGame {
  std::optional<GameState> state;
  std::optional<GameError> lastError;
  std::string snapshot;
  std::vector<std::pair<int, CardId>> lastDrawnContinuationCards;
};

std::unordered_map<QSCD_GameHandle, WasmGame> games;
QSCD_GameHandle nextHandle = 1;

CardId toCardId(std::uint32_t value) {
  return CardId{value};
}

GameError makeError(GameErrorCode code, std::optional<CardId> cardId = std::nullopt) {
  return GameError{code, cardId, std::nullopt, std::nullopt};
}

WasmGame* findGame(QSCD_GameHandle handle) {
  const auto it = games.find(handle);
  return it == games.end() ? nullptr : &it->second;
}

template <class Command>
std::int32_t applyCommand(QSCD_GameHandle handle, Command command) {
  auto* game = findGame(handle);
  if (game == nullptr) {
    return 0;
  }
  if (!game->state.has_value()) {
    game->lastError = makeError(GameErrorCode::InvalidPhase);
    return 0;
  }

  auto result = command(*game->state);
  if (!result.hasValue()) {
    game->lastError = result.error();
    return 0;
  }

  game->state = std::move(result.value());
  game->lastError.reset();
  return 1;
}

const char* phaseName(GamePhase phase) {
  switch (phase) {
    case GamePhase::Created: return "Created";
    case GamePhase::GameStarted: return "GameStarted";
    case GamePhase::RoundStarted: return "RoundStarted";
    case GamePhase::HandPlanned: return "HandPlanned";
    case GamePhase::CardsRevealed: return "CardsRevealed";
    case GamePhase::RoundFinished: return "RoundFinished";
    case GamePhase::ExtraRoundRequested: return "ExtraRoundRequested";
    case GamePhase::GameFinished: return "GameFinished";
    case GamePhase::ContinuationCardsDrawn: return "ContinuationCardsDrawn";
    case GamePhase::ContinuationGamePrepared: return "ContinuationGamePrepared";
  }
  return "Unknown";
}

const char* judgeName(JudgeResult result) {
  switch (result) {
    case JudgeResult::InProgress: return "InProgress";
    case JudgeResult::Victory: return "Victory";
    case JudgeResult::DefeatByScore: return "DefeatByScore";
    case JudgeResult::DefeatByCost: return "DefeatByCost";
    case JudgeResult::DefeatByAudit: return "DefeatByAudit";
  }
  return "Unknown";
}

const char* memberStateName(MemberState state) {
  switch (state) {
    case MemberState::Active: return "Active";
    case MemberState::LeftProject: return "LeftProject";
    case MemberState::Resigned: return "Resigned";
  }
  return "Unknown";
}

const char* runtimeStateName(RuntimeState state) {
  switch (state) {
    case RuntimeState::FaceDown: return "FaceDown";
    case RuntimeState::FaceUp: return "FaceUp";
    case RuntimeState::NullifiedAsFaceDown: return "NullifiedAsFaceDown";
  }
  return "Unknown";
}

const char* cardKindName(CardKind kind) {
  switch (kind) {
    case CardKind::MemberScore: return "MemberScore";
    case CardKind::MemberLeaveProject: return "MemberLeaveProject";
    case CardKind::Hand: return "Hand";
    case CardKind::ContinuationAudit: return "ContinuationAudit";
    case CardKind::ContinuationResign: return "ContinuationResign";
    case CardKind::ContinuationTeamScoreUp: return "ContinuationTeamScoreUp";
    case CardKind::ContinuationMemberCostUp: return "ContinuationMemberCostUp";
    case CardKind::ContinuationMemberScoreUp: return "ContinuationMemberScoreUp";
    case CardKind::ContinuationCostReductionPressure: return "ContinuationCostReductionPressure";
    case CardKind::ContinuationNone: return "ContinuationNone";
  }
  return "Unknown";
}

const char* handUsageName(const HandUsage& usage) {
  if (std::holds_alternative<UnusedHandUsage>(usage)) {
    return "Unused";
  }
  if (std::holds_alternative<ScorePlus3HandUsage>(usage)) {
    return "ScorePlus3";
  }
  if (std::holds_alternative<NullifyHandUsage>(usage)) {
    return "Nullify";
  }
  return "Unknown";
}

const char* errorCodeName(GameErrorCode code) {
  switch (code) {
    case GameErrorCode::InvalidPhase: return "InvalidPhase";
    case GameErrorCode::InvalidCardId: return "InvalidCardId";
    case GameErrorCode::CardNotFound: return "CardNotFound";
    case GameErrorCode::InvalidPosition: return "InvalidPosition";
    case GameErrorCode::InvalidHandUsage: return "InvalidHandUsage";
    case GameErrorCode::TooManyHandCards: return "TooManyHandCards";
    case GameErrorCode::InvalidRevealCount: return "InvalidRevealCount";
    case GameErrorCode::InvalidRevealTarget: return "InvalidRevealTarget";
    case GameErrorCode::InvalidNullifyTarget: return "InvalidNullifyTarget";
    case GameErrorCode::DeckEmpty: return "DeckEmpty";
    case GameErrorCode::RoundLimitExceeded: return "RoundLimitExceeded";
    case GameErrorCode::ExtraRoundNotAllowed: return "ExtraRoundNotAllowed";
    case GameErrorCode::FinishGameNotAllowed: return "FinishGameNotAllowed";
    case GameErrorCode::InvalidTeamSize: return "InvalidTeamSize";
    case GameErrorCode::InvalidExpectedScore: return "InvalidExpectedScore";
    case GameErrorCode::InvariantViolation: return "InvariantViolation";
  }
  return "Unknown";
}

const char* positionName(const Position& position) {
  if (std::holds_alternative<DeckPosition>(position)) {
    return "Deck";
  }
  if (std::holds_alternative<HandPosition>(position)) {
    return "Hand";
  }
  if (std::holds_alternative<BoardPosition>(position)) {
    return "Board";
  }
  if (std::holds_alternative<BoardSidePosition>(position)) {
    return "BoardSide";
  }
  if (std::holds_alternative<ContinuationDeckPosition>(position)) {
    return "ContinuationDeck";
  }
  if (std::holds_alternative<ContinuationMemberAreaPosition>(position)) {
    return "ContinuationMemberArea";
  }
  if (std::holds_alternative<ContinuationTeamAreaPosition>(position)) {
    return "ContinuationTeamArea";
  }
  if (std::holds_alternative<OutOfGamePosition>(position)) {
    return "OutOfGame";
  }
  return "Unknown";
}

const CardDefinition* definitionFor(const GameState& state, CardId id) {
  const auto it = state.cardDefinitions.find(id);
  return it == state.cardDefinitions.end() ? nullptr : &it->second;
}

const Position* positionFor(const GameState& state, CardId id) {
  const auto it = state.positions.find(id);
  return it == state.positions.end() ? nullptr : &it->second;
}

void appendError(std::ostringstream& out, const GameError& error) {
  out << "\"error\":{\"code\":\"" << errorCodeName(error.code) << "\"";
  if (error.cardId.has_value()) {
    out << ",\"cardId\":" << error.cardId->value;
  }
  if (error.row.has_value()) {
    out << ",\"row\":" << *error.row;
  }
  if (error.column.has_value()) {
    out << ",\"column\":" << *error.column;
  }
  out << '}';
}

std::string buildSnapshot(const WasmGame& game) {
  std::ostringstream out;
  out << '{';

  if (!game.state.has_value()) {
    out << "\"phase\":\"Invalid\",";
    out << "\"targetScore\":0,\"teamSize\":0,\"globalExpectedScore\":0,";
    out << "\"currentRound\":0,\"maxRound\":0,\"score\":0,\"cost\":0,";
    out << "\"judgeResult\":\"InProgress\",\"members\":[],\"board\":[],";
    out << "\"boardSideCards\":[],\"handCards\":[],\"revealableCards\":[],\"revealLimit\":0";
    if (game.lastError.has_value()) {
      out << ',';
      appendError(out, *game.lastError);
    }
    out << '}';
    return out.str();
  }

  const auto& state = *game.state;
  out << "\"phase\":\"" << phaseName(state.phase) << "\",";
  out << "\"targetScore\":" << state.targetScore << ',';
  out << "\"teamSize\":" << state.teamSize << ',';
  out << "\"globalExpectedScore\":" << state.globalExpectedScore << ',';
  out << "\"currentRound\":" << state.currentRound << ',';
  out << "\"maxRound\":" << state.maxRound << ',';
  out << "\"score\":" << (state.finalScore.has_value() ? *state.finalScore : calculateFinalScore(state)) << ',';
  out << "\"cost\":" << (state.finalCost.has_value() ? *state.finalCost : calculateFinalCost(state)) << ',';
  out << "\"judgeResult\":\"" << judgeName(judgeResult(state)) << "\",";
  out << "\"revealLimit\":" << calculateRevealLimit(state) << ',';
  out << "\"baseCostLimit\":" << state.baseCostLimit << ',';
  out << "\"costLimit\":" << state.costLimit << ',';
  out << "\"deckSeed\":" << state.deckSeed << ',';
  out << "\"isContinuationGame\":" << (state.isContinuationGame ? "true" : "false") << ',';
  out << "\"isDefeatedByAudit\":" << (state.isDefeatedByAudit ? "true" : "false") << ',';
  out << "\"hasUsedForcedUnpaidOvertime\":" << (state.hasUsedForcedUnpaidOvertime ? "true" : "false") << ',';
  out << "\"cumulativeCostLimitReduction\":" << state.cumulativeCostLimitReduction << ',';
  out << "\"pendingCostLimitReduction\":" << state.pendingCostLimitReduction << ',';

  out << "\"forcedUnpaidOvertimeRounds\":[";
  for (std::size_t i = 0; i < state.forcedUnpaidOvertimeRounds.size(); ++i) {
    if (i != 0) {
      out << ',';
    }
    out << state.forcedUnpaidOvertimeRounds[i];
  }
  out << "],";

  out << "\"members\":[";
  for (std::size_t i = 0; i < state.members.size(); ++i) {
    if (i != 0) {
      out << ',';
    }
    out << "{\"column\":" << i << ",\"state\":\"" << memberStateName(state.members[i]) << "\"}";
  }
  out << "],";

  out << "\"board\":[";
  bool firstRow = true;
  for (int row = 1; row <= std::max(state.currentRound, 0); ++row) {
    std::vector<std::pair<int, CardId>> rowCards;
    for (const auto& [id, position] : state.positions) {
      if (const auto* board = std::get_if<BoardPosition>(&position); board != nullptr && board->row == row) {
        rowCards.emplace_back(board->column, id);
      }
    }
    std::sort(rowCards.begin(), rowCards.end(), [](const auto& lhs, const auto& rhs) {
      return lhs.first < rhs.first;
    });
    if (!firstRow) {
      out << ',';
    }
    firstRow = false;
    out << "{\"row\":" << row << ",\"cards\":[";
    for (std::size_t i = 0; i < rowCards.size(); ++i) {
      if (i != 0) {
        out << ',';
      }
      const auto [column, id] = rowCards[i];
      const auto* definition = definitionFor(state, id);
      const auto runtimeIt = state.runtimeStates.find(id);
      const auto runtime = runtimeIt == state.runtimeStates.end() ? RuntimeState::FaceDown : runtimeIt->second;
      out << "{\"cardId\":" << id.value << ",\"column\":" << column;
      out << ",\"runtimeState\":\"" << runtimeStateName(runtime) << "\"";
      if (definition != nullptr) {
        out << ",\"kind\":\"" << cardKindName(definition->kind) << "\"";
        if (runtime == RuntimeState::FaceUp && definition->scoreValue.has_value()) {
          out << ",\"scoreValue\":" << *definition->scoreValue;
        }
      }
      out << '}';
    }
    out << "]}";
  }
  out << "],";

  std::vector<std::pair<std::pair<int, int>, CardId>> sideCards;
  for (const auto& [id, position] : state.positions) {
    if (const auto* side = std::get_if<BoardSidePosition>(&position); side != nullptr) {
      sideCards.push_back({{side->row, side->slot}, id});
    }
  }
  std::sort(sideCards.begin(), sideCards.end(), [](const auto& lhs, const auto& rhs) {
    return lhs.first < rhs.first;
  });
  out << "\"boardSideCards\":[";
  for (std::size_t i = 0; i < sideCards.size(); ++i) {
    if (i != 0) {
      out << ',';
    }
    const auto id = sideCards[i].second;
    const auto usageIt = state.handUsages.find(id);
    out << "{\"row\":" << sideCards[i].first.first << ",\"slot\":" << sideCards[i].first.second;
    out << ",\"cardId\":" << id.value;
    if (usageIt != state.handUsages.end()) {
      out << ",\"usage\":\"" << handUsageName(usageIt->second) << "\"";
      if (const auto* nullify = std::get_if<NullifyHandUsage>(&usageIt->second); nullify != nullptr && nullify->targetCardId.has_value()) {
        out << ",\"targetCardId\":" << nullify->targetCardId->value;
      }
    }
    out << '}';
  }
  out << "],";

  std::vector<std::pair<int, CardId>> handCards;
  for (const auto& [id, position] : state.positions) {
    if (const auto* hand = std::get_if<HandPosition>(&position); hand != nullptr) {
      handCards.emplace_back(hand->index, id);
    }
  }
  std::sort(handCards.begin(), handCards.end());
  out << "\"handCards\":[";
  for (std::size_t i = 0; i < handCards.size(); ++i) {
    if (i != 0) {
      out << ',';
    }
    out << "{\"cardId\":" << handCards[i].second.value << '}';
  }
  out << "],";

  const auto revealable = getRevealableCards(state);
  out << "\"revealableCards\":[";
  for (std::size_t i = 0; i < revealable.size(); ++i) {
    if (i != 0) {
      out << ',';
    }
    const auto id = revealable[i];
    const auto* position = positionFor(state, id);
    const auto* board = position == nullptr ? nullptr : std::get_if<BoardPosition>(position);
    out << "{\"cardId\":" << id.value;
    if (board != nullptr) {
      out << ",\"row\":" << board->row << ",\"column\":" << board->column;
    }
    out << '}';
  }
  out << "],";

  std::vector<CardId> continuationIds;
  for (const auto& [id, definition] : state.cardDefinitions) {
    if (definition.category == CardCategory::ContinuationCard) {
      continuationIds.push_back(id);
    }
  }
  std::sort(continuationIds.begin(), continuationIds.end());
  out << "\"continuationCards\":[";
  bool firstContinuation = true;
  for (const auto id : continuationIds) {
    const auto* definition = definitionFor(state, id);
    const auto* position = positionFor(state, id);
    if (definition == nullptr || position == nullptr) {
      continue;
    }
    if (std::holds_alternative<ContinuationDeckPosition>(*position)) {
      continue;
    }
    if (!firstContinuation) {
      out << ',';
    }
    firstContinuation = false;
    out << "{\"cardId\":" << id.value;
    out << ",\"kind\":\"" << cardKindName(definition->kind) << "\"";
    out << ",\"position\":\"" << positionName(*position) << "\"";
    if (const auto* member = std::get_if<ContinuationMemberAreaPosition>(position); member != nullptr) {
      out << ",\"column\":" << member->column << ",\"slot\":" << member->slot;
    }
    if (const auto* team = std::get_if<ContinuationTeamAreaPosition>(position); team != nullptr) {
      out << ",\"slot\":" << team->slot;
    }
    out << '}';
  }
  out << "],";

  out << "\"drawnContinuationCards\":[";
  for (std::size_t i = 0; i < game.lastDrawnContinuationCards.size(); ++i) {
    if (i != 0) {
      out << ',';
    }
    const auto [column, id] = game.lastDrawnContinuationCards[i];
    const auto* definition = definitionFor(state, id);
    const auto* position = positionFor(state, id);
    out << "{\"cardId\":" << id.value << ",\"column\":" << column;
    if (definition != nullptr) {
      out << ",\"kind\":\"" << cardKindName(definition->kind) << "\"";
    }
    if (position != nullptr) {
      out << ",\"position\":\"" << positionName(*position) << "\"";
    }
    out << '}';
  }
  out << ']';

  if (game.lastError.has_value()) {
    out << ',';
    appendError(out, *game.lastError);
  }
  out << '}';
  return out.str();
}

} // namespace

extern "C" {

EMSCRIPTEN_KEEPALIVE QSCD_GameHandle createGame(
  std::int32_t targetScore,
  std::int32_t teamSize,
  std::int32_t globalExpectedScore,
  std::int32_t costLimit,
  std::uint32_t deckSeed
) {
  const QSCD_GameHandle handle = nextHandle++;
  WasmGame game;

  GameSettings settings{
    targetScore,
    teamSize,
    globalExpectedScore,
    costLimit,
    deckSeed,
  };
  auto result = qscd::core::startGame(settings);
  if (result.hasValue()) {
    game.state = std::move(result.value());
  } else {
    game.lastError = result.error();
  }
  games.emplace(handle, std::move(game));
  return handle;
}

EMSCRIPTEN_KEEPALIVE void destroyGame(QSCD_GameHandle handle) {
  games.erase(handle);
}

EMSCRIPTEN_KEEPALIVE std::int32_t startRound(QSCD_GameHandle handle) {
  return applyCommand(handle, [](const GameState& state) {
    return qscd::core::startRound(state);
  });
}

EMSCRIPTEN_KEEPALIVE std::int32_t planHandUsage(
  QSCD_GameHandle handle,
  const std::uint32_t* handCardIds,
  const std::int32_t* usageCodes,
  std::int32_t count
) {
  auto* game = findGame(handle);
  if (game == nullptr) {
    return 0;
  }
  if (count < 0 || (count > 0 && (handCardIds == nullptr || usageCodes == nullptr))) {
    game->lastError = makeError(GameErrorCode::InvalidHandUsage);
    return 0;
  }

  HandPlan plan;
  for (std::int32_t i = 0; i < count; ++i) {
    HandUsage usage;
    if (usageCodes[i] == QSCD_WASM_HAND_USAGE_SCORE_PLUS_3) {
      usage = ScorePlus3HandUsage{};
    } else if (usageCodes[i] == QSCD_WASM_HAND_USAGE_NULLIFY) {
      usage = NullifyHandUsage{std::nullopt};
    } else {
      game->lastError = makeError(GameErrorCode::InvalidHandUsage, toCardId(handCardIds[i]));
      return 0;
    }
    plan.entries.push_back(HandPlanEntry{toCardId(handCardIds[i]), usage});
  }

  return applyCommand(handle, [&plan](const GameState& state) {
    return qscd::core::planHandUsage(state, plan);
  });
}

EMSCRIPTEN_KEEPALIVE std::int32_t revealCards(QSCD_GameHandle handle, const std::uint32_t* cardIds, std::int32_t count) {
  auto* game = findGame(handle);
  if (game == nullptr) {
    return 0;
  }
  if (count < 0 || (count > 0 && cardIds == nullptr)) {
    game->lastError = makeError(GameErrorCode::InvalidRevealTarget);
    return 0;
  }

  std::vector<CardId> ids;
  ids.reserve(static_cast<std::size_t>(count));
  for (std::int32_t i = 0; i < count; ++i) {
    ids.push_back(toCardId(cardIds[i]));
  }

  return applyCommand(handle, [&ids](const GameState& state) {
    return qscd::core::revealCards(state, ids);
  });
}

EMSCRIPTEN_KEEPALIVE std::int32_t applyNullify(QSCD_GameHandle handle, std::uint32_t handCardId, std::uint32_t targetCardId) {
  return applyCommand(handle, [handCardId, targetCardId](const GameState& state) {
    return qscd::core::applyNullify(state, toCardId(handCardId), toCardId(targetCardId));
  });
}

EMSCRIPTEN_KEEPALIVE std::int32_t finishRound(QSCD_GameHandle handle) {
  return applyCommand(handle, [](const GameState& state) {
    return qscd::core::finishRound(state);
  });
}

EMSCRIPTEN_KEEPALIVE std::int32_t requestExtraRound(QSCD_GameHandle handle, std::int32_t forcedUnpaidOvertime) {
  return applyCommand(handle, [forcedUnpaidOvertime](const GameState& state) {
    return qscd::core::requestExtraRound(state, ExtraRoundOptions{forcedUnpaidOvertime != 0});
  });
}

EMSCRIPTEN_KEEPALIVE std::int32_t finishGame(QSCD_GameHandle handle) {
  return applyCommand(handle, [](const GameState& state) {
    return qscd::core::finishGame(state);
  });
}

EMSCRIPTEN_KEEPALIVE std::int32_t drawContinuationCards(QSCD_GameHandle handle) {
  auto* game = findGame(handle);
  if (game == nullptr) {
    return 0;
  }
  if (!game->state.has_value()) {
    game->lastError = makeError(GameErrorCode::InvalidPhase);
    return 0;
  }

  std::vector<std::pair<int, CardId>> drawnCards;
  const auto activeMembers = getActiveMembers(*game->state);
  const auto drawCount = std::min(activeMembers.size(), game->state->continuationDeckOrder.size());
  drawnCards.reserve(drawCount);
  for (std::size_t i = 0; i < drawCount; ++i) {
    drawnCards.emplace_back(activeMembers[i], game->state->continuationDeckOrder[i]);
  }

  auto result = qscd::core::drawContinuationCards(*game->state);
  if (!result.hasValue()) {
    game->lastError = result.error();
    return 0;
  }

  game->state = std::move(result.value());
  game->lastDrawnContinuationCards = std::move(drawnCards);
  game->lastError.reset();
  return 1;
}

EMSCRIPTEN_KEEPALIVE std::int32_t startContinuationGame(
  QSCD_GameHandle handle,
  std::int32_t targetScore,
  std::int32_t teamSize,
  std::int32_t globalExpectedScore,
  std::int32_t costLimit,
  std::int32_t teamCompositionChanged,
  std::uint32_t deckSeed,
  const std::uint32_t* retiringColumns,
  std::int32_t retiringColumnCount
) {
  auto* game = findGame(handle);
  if (game == nullptr) {
    return 0;
  }
  if (retiringColumnCount < 0 || (retiringColumnCount > 0 && retiringColumns == nullptr)) {
    game->lastError = makeError(GameErrorCode::InvalidPosition);
    return 0;
  }
  ContinuationSettings settings{
    targetScore,
    teamSize,
    globalExpectedScore,
    costLimit,
    teamCompositionChanged != 0,
    deckSeed,
  };
  settings.retiringColumns.reserve(static_cast<std::size_t>(retiringColumnCount));
  for (std::int32_t i = 0; i < retiringColumnCount; ++i) {
    settings.retiringColumns.push_back(static_cast<int>(retiringColumns[i]));
  }
  const auto result = applyCommand(handle, [&settings](const GameState& state) {
    return qscd::core::startContinuationGame(state, settings);
  });
  if (result != 0) {
    game->lastDrawnContinuationCards.clear();
  }
  return result;
}

EMSCRIPTEN_KEEPALIVE const char* getStateSnapshot(QSCD_GameHandle handle) {
  static const std::string invalidHandle = "{\"phase\":\"Invalid\",\"error\":{\"code\":\"InvalidCardId\"}}";
  auto* game = findGame(handle);
  if (game == nullptr) {
    return invalidHandle.c_str();
  }
  game->snapshot = buildSnapshot(*game);
  return game->snapshot.c_str();
}

}
