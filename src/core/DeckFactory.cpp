#include "qscd/core/DeckFactory.hpp"

namespace qscd::core {

namespace {

CardDefinition memberScore(CardId id, int score) {
  return CardDefinition{id, CardCategory::MemberCard, CardKind::MemberScore, score, std::nullopt};
}

CardDefinition effectCard(CardId id, CardCategory category, CardKind kind, EffectType effect) {
  return CardDefinition{id, category, kind, std::nullopt, effect};
}

} // namespace

std::vector<CardDefinition> createMemberDeck() {
  return {
    memberScore(cardId(CardIdValue::MemberScore0_01), 0),
    memberScore(cardId(CardIdValue::MemberScore0_02), 0),
    memberScore(cardId(CardIdValue::MemberScore1_01), 1),
    memberScore(cardId(CardIdValue::MemberScore1_02), 1),
    memberScore(cardId(CardIdValue::MemberScore1_03), 1),
    memberScore(cardId(CardIdValue::MemberScore2_01), 2),
    memberScore(cardId(CardIdValue::MemberScore2_02), 2),
    memberScore(cardId(CardIdValue::MemberScore2_03), 2),
    memberScore(cardId(CardIdValue::MemberScore2_04), 2),
    memberScore(cardId(CardIdValue::MemberScore3_01), 3),
    memberScore(cardId(CardIdValue::MemberScore3_02), 3),
    memberScore(cardId(CardIdValue::MemberScore3_03), 3),
    memberScore(cardId(CardIdValue::MemberScore3_04), 3),
    memberScore(cardId(CardIdValue::MemberScore3_05), 3),
    memberScore(cardId(CardIdValue::MemberScore3_06), 3),
    memberScore(cardId(CardIdValue::MemberScore4_01), 4),
    memberScore(cardId(CardIdValue::MemberScore4_02), 4),
    memberScore(cardId(CardIdValue::MemberScore4_03), 4),
    memberScore(cardId(CardIdValue::MemberScore4_04), 4),
    memberScore(cardId(CardIdValue::MemberScore4_05), 4),
    memberScore(cardId(CardIdValue::MemberScore4_06), 4),
    memberScore(cardId(CardIdValue::MemberScore4_07), 4),
    memberScore(cardId(CardIdValue::MemberScore4_08), 4),
    memberScore(cardId(CardIdValue::MemberScore4_09), 4),
    memberScore(cardId(CardIdValue::MemberScore4_10), 4),
    memberScore(cardId(CardIdValue::MemberScore5_01), 5),
    memberScore(cardId(CardIdValue::MemberScore5_02), 5),
    memberScore(cardId(CardIdValue::MemberScore5_03), 5),
    memberScore(cardId(CardIdValue::MemberScore5_04), 5),
    memberScore(cardId(CardIdValue::MemberScore5_05), 5),
    memberScore(cardId(CardIdValue::MemberScore5_06), 5),
    memberScore(cardId(CardIdValue::MemberScore5_07), 5),
    memberScore(cardId(CardIdValue::MemberScore6_01), 6),
    memberScore(cardId(CardIdValue::MemberScore6_02), 6),
    effectCard(cardId(CardIdValue::MemberLeaveProject_01), CardCategory::MemberCard, CardKind::MemberLeaveProject, EffectType::LeaveProject),
  };
}

std::vector<CardDefinition> createContinuationDeck() {
  return {
    effectCard(cardId(CardIdValue::ContinuationAudit_01), CardCategory::ContinuationCard, CardKind::ContinuationAudit, EffectType::Audit),
    effectCard(cardId(CardIdValue::ContinuationResign_01), CardCategory::ContinuationCard, CardKind::ContinuationResign, EffectType::Resign),
    effectCard(cardId(CardIdValue::ContinuationTeamScoreUp_01), CardCategory::ContinuationCard, CardKind::ContinuationTeamScoreUp, EffectType::TeamScoreUp),
    effectCard(cardId(CardIdValue::ContinuationTeamScoreUp_02), CardCategory::ContinuationCard, CardKind::ContinuationTeamScoreUp, EffectType::TeamScoreUp),
    effectCard(cardId(CardIdValue::ContinuationMemberCostUp_01), CardCategory::ContinuationCard, CardKind::ContinuationMemberCostUp, EffectType::MemberCostUp),
    effectCard(cardId(CardIdValue::ContinuationMemberCostUp_02), CardCategory::ContinuationCard, CardKind::ContinuationMemberCostUp, EffectType::MemberCostUp),
    effectCard(cardId(CardIdValue::ContinuationMemberCostUp_03), CardCategory::ContinuationCard, CardKind::ContinuationMemberCostUp, EffectType::MemberCostUp),
    effectCard(cardId(CardIdValue::ContinuationMemberScoreUp_01), CardCategory::ContinuationCard, CardKind::ContinuationMemberScoreUp, EffectType::MemberScoreUp),
    effectCard(cardId(CardIdValue::ContinuationMemberScoreUp_02), CardCategory::ContinuationCard, CardKind::ContinuationMemberScoreUp, EffectType::MemberScoreUp),
    effectCard(cardId(CardIdValue::ContinuationMemberScoreUp_03), CardCategory::ContinuationCard, CardKind::ContinuationMemberScoreUp, EffectType::MemberScoreUp),
    effectCard(cardId(CardIdValue::ContinuationCostReductionPressure_01), CardCategory::ContinuationCard, CardKind::ContinuationCostReductionPressure, EffectType::CostReductionPressure),
    effectCard(cardId(CardIdValue::ContinuationNone_01), CardCategory::ContinuationCard, CardKind::ContinuationNone, EffectType::None),
    effectCard(cardId(CardIdValue::ContinuationNone_02), CardCategory::ContinuationCard, CardKind::ContinuationNone, EffectType::None),
    effectCard(cardId(CardIdValue::ContinuationNone_03), CardCategory::ContinuationCard, CardKind::ContinuationNone, EffectType::None),
  };
}

std::vector<CardDefinition> createHandCards() {
  return {
    CardDefinition{cardId(CardIdValue::Hand_01), CardCategory::HandCard, CardKind::Hand, std::nullopt, std::nullopt},
    CardDefinition{cardId(CardIdValue::Hand_02), CardCategory::HandCard, CardKind::Hand, std::nullopt, std::nullopt},
    CardDefinition{cardId(CardIdValue::Hand_03), CardCategory::HandCard, CardKind::Hand, std::nullopt, std::nullopt},
    CardDefinition{cardId(CardIdValue::Hand_04), CardCategory::HandCard, CardKind::Hand, std::nullopt, std::nullopt},
    CardDefinition{cardId(CardIdValue::Hand_05), CardCategory::HandCard, CardKind::Hand, std::nullopt, std::nullopt},
  };
}

} // namespace qscd::core
