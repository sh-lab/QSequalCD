#pragma once

#include <cstdint>
#include <compare>
#include <functional>
#include <optional>
#include <variant>

namespace qscd::core {

enum class CardIdValue : std::uint32_t {
  MemberScore0_01 = 1,
  MemberScore0_02,

  MemberScore1_01,
  MemberScore1_02,
  MemberScore1_03,

  MemberScore2_01,
  MemberScore2_02,
  MemberScore2_03,
  MemberScore2_04,

  MemberScore3_01,
  MemberScore3_02,
  MemberScore3_03,
  MemberScore3_04,
  MemberScore3_05,
  MemberScore3_06,

  MemberScore4_01,
  MemberScore4_02,
  MemberScore4_03,
  MemberScore4_04,
  MemberScore4_05,
  MemberScore4_06,
  MemberScore4_07,
  MemberScore4_08,
  MemberScore4_09,
  MemberScore4_10,

  MemberScore5_01,
  MemberScore5_02,
  MemberScore5_03,
  MemberScore5_04,
  MemberScore5_05,
  MemberScore5_06,
  MemberScore5_07,

  MemberScore6_01,
  MemberScore6_02,

  MemberLeaveProject_01,

  Hand_01 = 100,
  Hand_02,
  Hand_03,
  Hand_04,
  Hand_05,

  ContinuationAudit_01 = 200,
  ContinuationResign_01,
  ContinuationTeamScoreUp_01,
  ContinuationTeamScoreUp_02,
  ContinuationTeamScoreUp_03,
  ContinuationMemberCostUp_01,
  ContinuationMemberCostUp_02,
  ContinuationMemberCostUp_03,
  ContinuationMemberScoreUp_01,
  ContinuationMemberScoreUp_02,
  ContinuationMemberScoreUp_03,
  ContinuationNone_01,
  ContinuationNone_02,
  ContinuationNone_03,
};

struct CardId {
  std::uint32_t value{};

  friend bool operator==(CardId lhs, CardId rhs) = default;
  friend auto operator<=>(CardId lhs, CardId rhs) = default;
};

constexpr CardId cardId(CardIdValue value) {
  return CardId{static_cast<std::uint32_t>(value)};
}

enum class CardCategory {
  MemberCard,
  HandCard,
  ContinuationCard,
};

enum class CardKind {
  MemberScore,
  MemberLeaveProject,
  Hand,
  ContinuationAudit,
  ContinuationResign,
  ContinuationTeamScoreUp,
  ContinuationMemberCostUp,
  ContinuationMemberScoreUp,
  ContinuationNone,
};

enum class EffectType {
  LeaveProject,
  Audit,
  Resign,
  TeamScoreUp,
  MemberCostUp,
  MemberScoreUp,
  None,
};

struct CardDefinition {
  CardId id;
  CardCategory category{};
  CardKind kind{};
  std::optional<int> scoreValue;
  std::optional<EffectType> effectType;

  friend bool operator==(const CardDefinition&, const CardDefinition&) = default;
};

struct DeckPosition { int index{}; friend bool operator==(const DeckPosition&, const DeckPosition&) = default; };
struct HandPosition { int index{}; friend bool operator==(const HandPosition&, const HandPosition&) = default; };
struct BoardPosition { int row{}; int column{}; friend bool operator==(const BoardPosition&, const BoardPosition&) = default; };
struct BoardSidePosition { int row{}; int slot{}; friend bool operator==(const BoardSidePosition&, const BoardSidePosition&) = default; };
struct ContinuationDeckPosition { int index{}; friend bool operator==(const ContinuationDeckPosition&, const ContinuationDeckPosition&) = default; };
struct ContinuationMemberAreaPosition { int column{}; int slot{}; friend bool operator==(const ContinuationMemberAreaPosition&, const ContinuationMemberAreaPosition&) = default; };
struct ContinuationTeamAreaPosition { int slot{}; friend bool operator==(const ContinuationTeamAreaPosition&, const ContinuationTeamAreaPosition&) = default; };
struct OutOfGamePosition { friend bool operator==(const OutOfGamePosition&, const OutOfGamePosition&) = default; };

using Position = std::variant<
  DeckPosition,
  HandPosition,
  BoardPosition,
  BoardSidePosition,
  ContinuationDeckPosition,
  ContinuationMemberAreaPosition,
  ContinuationTeamAreaPosition,
  OutOfGamePosition
>;

enum class RuntimeState {
  FaceDown,
  FaceUp,
  NullifiedAsFaceDown,
};

struct UnusedHandUsage { friend bool operator==(const UnusedHandUsage&, const UnusedHandUsage&) = default; };
struct ScorePlus3HandUsage { friend bool operator==(const ScorePlus3HandUsage&, const ScorePlus3HandUsage&) = default; };
struct NullifyHandUsage {
  std::optional<CardId> targetCardId;
  friend bool operator==(const NullifyHandUsage&, const NullifyHandUsage&) = default;
};

using HandUsage = std::variant<UnusedHandUsage, ScorePlus3HandUsage, NullifyHandUsage>;

enum class MemberState {
  Active,
  LeftProject,
  Resigned,
};

enum class GamePhase {
  Created,
  GameStarted,
  RoundStarted,
  HandPlanned,
  CardsRevealed,
  RoundFinished,
  ExtraRoundRequested,
  GameFinished,
  ContinuationCardsDrawn,
  ContinuationGamePrepared,
};

enum class JudgeResult {
  InProgress,
  Victory,
  DefeatByScore,
  DefeatByCost,
  DefeatByAudit,
};

enum class GameErrorCode {
  InvalidPhase,
  InvalidCardId,
  CardNotFound,
  InvalidPosition,
  InvalidHandUsage,
  TooManyHandCards,
  InvalidRevealCount,
  InvalidRevealTarget,
  InvalidNullifyTarget,
  DeckEmpty,
  RoundLimitExceeded,
  ExtraRoundNotAllowed,
  FinishGameNotAllowed,
  InvalidTeamSize,
  InvalidExpectedScore,
  InvariantViolation,
};

struct GameError {
  GameErrorCode code{};
  std::optional<CardId> cardId;
  std::optional<int> row;
  std::optional<int> column;
};

} // namespace qscd::core

template <>
struct std::hash<qscd::core::CardId> {
  std::size_t operator()(qscd::core::CardId id) const noexcept {
    return std::hash<std::uint32_t>{}(id.value);
  }
};
