# QS=CD Core Architecture

このドキュメントは、カードゲーム「QS=CD」の C++20 コア実装アーキテクチャを定義する。

ゲームルールの正本は `docs/rule.md` とする。  
本ドキュメントでは、ルールそのものではなく、ルールを実装するための構造・責務・状態管理方針を定義する。

---

## 1. 基本方針

QS=CD コアは、UI に依存しない純粋なゲームロジックとして実装する。

対象外：

- SwiftUI
- Web UI
- 標準入出力
- ファイル入出力
- 通信処理
- 永続化処理

コアは、以下から利用されることを想定する。

- iOS アプリ
- Web 版
- テストコード
- 将来的な C ABI ラッパー

---

## 2. 参照ドキュメント

- ゲームルールの正本：`docs/rule.md`
- 実装アーキテクチャの正本：`docs/architecture.md`
- Copilot 向け指示：`.github/copilot-instructions.md`

ルール解釈に迷った場合は、必ず `docs/rule.md` を優先する。

---

## 3. 設計思想

QS=CD コアでは、カードそのものではなく、カードがどこに存在するかを重視する。

そのため、カード管理の中心は以下の形式とする。

```text
CardId -> CardDefinition
CardId -> Position
CardId -> RuntimeState
CardId -> HandUsage
```

カードそのものをキーにしてはならない。  
必ず一意な `CardId` をキーとして扱う。

---

## 4. 状態更新方針

QS=CD コアでは、コマンド系処理は現在の状態を直接変更するのではなく、次の状態を返す。

```text
Command(CurrentState, Input) -> Result<NextState, Error>
```

### 方針

- `GameState` は値として扱えるように設計する
- コマンドは入力状態を破壊的に変更しない
- 成功時は新しい `GameState` を返す
- 失敗時はエラーを返す
- エラー時、元の状態は変化しない
- UI 側は返された `GameState` を次の表示状態として扱う

この方針により、以下を実現する。

- テストしやすい
- 状態遷移を追いやすい
- Undo / Replay / Log 化しやすい
- SwiftUI の状態管理と相性がよい
- 不正な途中状態を避けやすい

---

## 5. Query と Command の分離

コア API は、Query と Command を明確に分離する。

---

### 5.1 Query

Query は状態を変更しない。

例：

```text
calculateRoundScore(state, round)
calculateFinalScore(state)
calculateFinalCost(state)
calculateRevealLimit(state)
getRevealableCards(state)
getActiveMembers(state)
getBoardCards(state)
getBoardSideCards(state)
getContinuationCardsForMember(state, column)
judgeResult(state)
```

Query は常に副作用を持たない。

```text
Query(GameState) -> Value
```

---

### 5.2 Command

Command はルールに従って次状態を返す。

例：

```text
startGame(settings) -> Result<GameState, Error>

startRound(state) -> Result<GameState, Error>

planHandUsage(state, handPlan) -> Result<GameState, Error>

revealCards(state, cardIds) -> Result<GameState, Error>

applyNullify(state, handCardId, targetCardId) -> Result<GameState, Error>

finishRound(state) -> Result<GameState, Error>

requestExtraRound(state, options) -> Result<GameState, Error>

finishGame(state) -> Result<GameState, Error>

drawContinuationCards(state) -> Result<GameState, Error>

startContinuationGame(state, continuationSettings) -> Result<GameState, Error>
```

Command は、入力状態を直接書き換えず、必ず新しい状態を返す。

---

## 6. 主要データ構造

---

## 6.1 CardId

すべてのカードは一意な `CardId` を持つ。

```text
CardId
```

`CardId` は以下を識別する。

- メンバーカード
- 手札カード
- 継続カード

同じ種類・同じ数値のカードが複数枚存在しても、各カードは個別の `CardId` を持つ。

---

## 6.2 CardDefinition

カードの不変情報を表す。

想定フィールド：

```text
CardId id
CardCategory category
CardKind kind
optional<int> scoreValue
optional<EffectType> effectType
```

`CardDefinition` はゲーム中に変更しない。

---

## 6.3 CardCategory

カードの大分類を表す。

```text
MemberCard
HandCard
ContinuationCard
```

---

## 6.4 CardKind

カードの具体種別を表す。

例：

```text
MemberScore
MemberLeaveProject

Hand

ContinuationAudit
ContinuationResign
ContinuationTeamScoreUp
ContinuationMemberCostUp
ContinuationMemberScoreUp
ContinuationNone
```

具体的なカード構成・枚数は `docs/rule.md` に従う。

---

## 7. Position

カードの現在位置を表す。

```text
Deck(index)
Hand(index)

Board(row, column)
BoardSide(row, slot)

ContinuationDeck(index)
ContinuationMemberArea(column, slot)
ContinuationTeamArea(slot)

OutOfGame
```

---

### 7.1 Deck(index)

メンバーカード山札内の位置を表す。

---

### 7.2 Hand(index)

未使用の手札カードの位置を表す。

---

### 7.3 Board(row, column)

ラウンド行・メンバー列に配置されたメンバーカードを表す。

- `row` はラウンドを表す
- `column` はメンバー列を表す

`Board(row, column)` には、メンバーカードのみ配置できる。

---

### 7.4 BoardSide(row, slot)

そのラウンドの右側に置かれた手札カードを表す。

手札カードは `Board(row, column)` には配置しない。

---

### 7.5 ContinuationDeck(index)

継続カード山札内の位置を表す。

---

### 7.6 ContinuationMemberArea(column, slot)

メンバーに紐づく継続カード置き場を表す。

例：

- メンバーコストアップ
- メンバースコアアップ

---

### 7.7 ContinuationTeamArea(slot)

チーム全体に紐づく継続カード置き場を表す。

例：

- チームスコアアップ

---

### 7.8 OutOfGame

処理済み、または場に残らないカードを表す。

---

## 8. RuntimeState

カードの実行時状態を表す。

メンバーカードには少なくとも以下の状態が必要である。

```text
FaceDown
FaceUp
NullifiedAsFaceDown
```

---

### 8.1 FaceDown

裏向きカードとして扱う。

---

### 8.2 FaceUp

表向きカードとして扱う。

---

### 8.3 NullifiedAsFaceDown

表向きになったが、手札によって無効化され、そのラウンドでは裏向き扱いになっている状態。

---

## 9. HandUsage

手札カードの使用内容を表す。

```text
Unused
ScorePlus3
Nullify(targetCardId)
```

位置と使用内容は分離する。

- 手札カードの位置は `Position` で管理する
- 手札カードの用途は `HandUsage` で管理する

例：

```text
CardId -> Position::BoardSide(row, slot)
CardId -> HandUsage::ScorePlus3
```

または：

```text
CardId -> Position::BoardSide(row, slot)
CardId -> HandUsage::Nullify(targetCardId)
```

---

## 10. MemberState

メンバー列の状態を表す。

```text
Active
LeftProject
Resigned
```

---

### 10.1 Active

通常の稼働中メンバー。

---

### 10.2 LeftProject

ゲーム中に離脱カードによって離脱したメンバー。

この状態のメンバー列には、次ラウンド以降メンバーカードを配らない。

---

### 10.3 Resigned

継続カード「離任」によって、次ゲーム以降チームから外れたメンバー。

---

## 11. GameState

ゲーム全体の状態を表す。

想定フィールド例：

```text
targetScore
teamSize
globalExpectedScore
currentRound
maxRound
isContinuationGame
hasUsedForcedUnpaidOvertime
isDefeatedByAudit
optional<int> costLimit
deckSeed
```

カード管理：

```text
map<CardId, CardDefinition> cardDefinitions
map<CardId, Position> positions
map<CardId, RuntimeState> runtimeStates
map<CardId, HandUsage> handUsages
vector<MemberState> members
```

山札順：

```text
vector<CardId> memberDeckOrder
vector<CardId> continuationDeckOrder
```

実装では `std::unordered_map` を使ってもよい。

---

## 12. 山札管理

山札順は `Position` だけでなく、順序専用の配列でも保持してよい。

```text
memberDeckOrder
continuationDeckOrder
```

山札順は、ゲーム開始時のシード値から決定的にシャッフルして生成する。
同じシード値で開始したゲームは同じ山札順になり、異なるシード値では異なる山札順になる。

`Position` と山札順の整合性を保つこと。

例：

```text
memberDeckOrder[0] のカードは Position::Deck(0)
memberDeckOrder[1] のカードは Position::Deck(1)
```

山札からカードを引いた場合、該当カードの `Position` を新しい場所へ移動する。

---

## 13. 状態遷移

状態遷移は、以下のような段階で進む。

```text
Created
  -> GameStarted
  -> RoundStarted
  -> HandPlanned
  -> CardsRevealed
  -> RoundFinished
  -> GameFinished
```

追加ラウンドがある場合：

```text
RoundFinished
  -> ExtraRoundRequested
  -> RoundStarted
```

継続ゲームがある場合：

```text
GameFinished
  -> ContinuationCardsDrawn
  -> ContinuationGamePrepared
  -> GameStarted
```

状態名を enum として持つかどうかは実装判断とする。  
ただし、不正な順序のコマンドを受け付けないこと。

---

## 14. Command 設計

Command は、入力状態と引数を受け取り、次状態を返す。

---

### 14.1 startGame

```text
startGame(initialSettings) -> Result<GameState, Error>
```

初期設定から新規ゲーム状態を作る。

---

### 14.2 startRound

```text
startRound(state) -> Result<GameState, Error>
```

現在ラウンドを開始し、ルールに従ってメンバーカードを配布する。

---

### 14.3 planHandUsage

```text
planHandUsage(state, handPlan) -> Result<GameState, Error>
```

そのラウンドで使用する手札枚数と用途を宣言する。

---

### 14.4 revealCards

```text
revealCards(state, cardIds) -> Result<GameState, Error>
```

指定されたメンバーカードを表向きにする。

表にする枚数は `docs/rule.md` の定義に従う。

---

### 14.5 applyNullify

```text
applyNullify(state, handCardId, targetCardId) -> Result<GameState, Error>
```

手札による無効化を適用する。

---

### 14.6 finishRound

```text
finishRound(state) -> Result<GameState, Error>
```

ラウンドを終了し、スコア・離脱効果などを処理する。

---

### 14.7 requestExtraRound

```text
requestExtraRound(state, options) -> Result<GameState, Error>
```

条件を満たす場合に追加ラウンドを要求する。

---

### 14.8 finishGame

```text
finishGame(state) -> Result<GameState, Error>
```

ゲームを終了し、最終スコア・最終コスト・勝敗を確定する。

---

### 14.9 drawContinuationCards

```text
drawContinuationCards(state) -> Result<GameState, Error>
```

継続ゲーム用のカードを処理する。

---

### 14.10 startContinuationGame

```text
startContinuationGame(state, continuationSettings) -> Result<GameState, Error>
```

継続状態を引き継いで次ゲームを開始する。

---

## 15. Query 設計

Query は状態を変更しない。

例：

```text
calculateRoundScore(state, round)
calculateFinalScore(state)
calculateFinalCost(state)
calculateRevealLimit(state)
getRevealableCards(state)
getActiveMembers(state)
getBoardCards(state)
getBoardSideCards(state)
getContinuationCardsForMember(state, column)
judgeResult(state)
```

Query は副作用を持ってはならない。

---

## 16. Result / Error 方針

Command はエラーを返せるようにする。

例：

```text
Result<GameState, GameError>
```

想定されるエラー：

```text
InvalidPhase
InvalidCardId
CardNotFound
InvalidPosition
InvalidHandUsage
TooManyHandCards
InvalidRevealCount
InvalidRevealTarget
InvalidNullifyTarget
DeckEmpty
RoundLimitExceeded
ExtraRoundNotAllowed
InvalidTeamSize
InvalidExpectedScore
InvariantViolation
```

エラー発生時、元の `GameState` は変更されない。

---

## 17. 計算方針

以下の計算は Query として実装する。

- ラウンドスコア
- 最終スコア
- 最終コスト
- 表にする枚数
- 勝敗判定
- 継続カード効果の合計
- メンバーごとの補正値

計算結果を `GameState` にキャッシュするかどうかは実装判断とする。  
ただし、キャッシュする場合は不整合が起きないようにする。

---

## 18. Invariant

以下の不変条件を守る。

```text
1. CardId は一意である

2. 1つの CardId は常に1つの Position だけを持つ

3. CardDefinition は不変情報であり、ゲーム中に変更しない

4. Board(row, column) には最大1枚の MemberCard しか存在しない

5. Board(row, column) に HandCard を置いてはいけない

6. HandCard は BoardSide(row, slot) にのみ使用済みカードとして配置できる

7. HandCard はコストに含めない

8. ContinuationCard は通常コストに含めない

9. LeftProject または Resigned の列には新規 MemberCard を配らない

10. 表にする枚数は docs/rule.md の定義に従う

11. 手札使用枚数は docs/rule.md の定義に従う

12. 手札による無効化対象は docs/rule.md の定義に従う

13. NullifiedAsFaceDown のカードは、そのラウンドでは裏向きカードとして扱う

14. 無効化された離脱カードは離脱効果を発生させない

15. Q は GameState に保持しない

16. サビ残強制ラウンドのコスト処理は docs/rule.md の定義に従う

17. 離脱済みメンバーの継続カード処理は docs/rule.md の定義に従う

18. 場に残らない継続カードは、効果処理後 OutOfGame に移動する

19. チーム構成変更時のチームスコアアップ処理は docs/rule.md の定義に従う
```

Invariant はテスト可能にする。

---

## 19. C ABI を見据えた設計

C++ 内部では以下を使用してよい。

```text
std::vector
std::optional
std::variant
std::unordered_map
std::string
```

ただし、将来的な外部公開 API には C++ STL 型を直接出さない。

外部公開時は、以下のような C ABI ラッパーを用意する想定とする。

```text
QSCD_GameHandle
qscd_create_game(...)
qscd_destroy_game(...)
qscd_start_round(...)
qscd_plan_hand_usage(...)
qscd_reveal_cards(...)
qscd_finish_round(...)
qscd_get_state_snapshot(...)
```

C ABI ラッパーはこのアーキテクチャの対象外だが、将来追加しやすいようにしておく。

---

## 20. テスト方針

少なくとも以下をテストする。

- 初期設定の範囲チェック
- カードIDの一意性
- 配布処理
- 表にする枚数計算
- 手札使用制限
- 無効化処理
- 離脱カード処理
- 離脱後の配布禁止
- ラウンドスコア計算
- 最終スコア計算
- 最終コスト計算
- サビ残強制時のコスト除外
- 監査による敗北
- 継続カード処理
- 離脱済みメンバーが継続カードを引かないこと
- チーム構成変更時のチームスコアアップ失効
- 不正コマンド時に状態が変わらないこと

---

## 21. 実装上の禁止事項

- UI 処理を書かない
- SwiftUI 依存コードを書かない
- Web UI 依存コードを書かない
- 標準入出力に依存しない
- ファイル入出力に依存しない
- `docs/rule.md` にないルールを追加しない
- カード枚数を勝手に変更しない
- カード効果を勝手に変更しない
- 勝敗条件を勝手に変更しない
- Q を数値状態として `GameState` に保持しない
- ゲームバランス調整を勝手に行わない
