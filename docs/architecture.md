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

QS=CD コアでは、コマンド系処理は現在の状態を直接変更するのではなく、次の状態を返す。1ゲームの状態には `GameState`、プロジェクト全体の状態には `ProjectState` を使用する。

```text
Command(CurrentState, Input) -> Result<NextState, Error>
```

### 方針

- `GameState` と `ProjectState` は値として扱えるように設計する
- コマンドは入力状態を破壊的に変更しない
- 成功時は新しい `GameState` または `ProjectState` を返す
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
calculateProjectEfficiency(projectState)
judgeProjectResult(projectState)
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

startProject(projectSettings) -> Result<ProjectState, Error>

completeProjectGame(projectState, finishedGameState) -> Result<ProjectState, Error>
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

## 6.5 ProjectMode

プロジェクトの遊び方を表す。

```text
Single
Large
Endless
```

---

## 6.6 MemberDeckSet

メンバーカードセットを表す。

```text
Stable
HighRisk
```

---

## 6.7 ContinuationDeckSet

継続カードセットを表す。

```text
Standard
Double
```

`Double` は、ルール上の名称「W」に対応する内部識別子とする。

---

## 6.8 ProjectStatus

プロジェクト全体の進行状態を表す。

```text
InProgress
Cleared
Failed
Collapsed
```

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

## 11.1 ProjectState

複数ゲームにまたがるプロジェクト全体の状態を表す。`GameState` は1ゲームの状態に限定し、プロジェクトモード、固定カードセット、累積値、プロジェクト終了判定は `ProjectState` が保持する。

想定フィールド例：

```text
ProjectMode mode
MemberDeckSet memberDeckSet
ContinuationDeckSet continuationDeckSet
ProjectStatus status
GameState currentGame
int completedGameCount
int cumulativeFinalScore
int cumulativeFinalCost
```

### 方針

- プロジェクトモードとカードセットは、プロジェクト開始時にのみ設定する。
- 継続ゲーム開始時に、プロジェクトモードまたはカードセットを変更してはならない。
- 単発プロジェクトは1ゲーム終了後に `Cleared` または `Failed` となる。
- 大型プロジェクトは、途中敗北で `Collapsed`、3ゲーム合計200点以上で `Cleared`、200点未満で `Failed` となる。
- エンドレスプロジェクトは、通常敗北が発生するまで `InProgress` を維持し、敗北時に `Collapsed` となる。
- 累計最終スコアと累計最終コストは、完了した各ゲームの確定値を加算する。
- 最終効率評価の元データとして、除算結果だけでなく累計最終スコアと累計最終コストを保持する。

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

### 12.1 カードセット別の山札生成

- メンバーカード山札は、`MemberDeckSet` に従って生成する。
- 継続カード山札は、`ContinuationDeckSet` に従って生成する。
- 具体的なカード枚数は `docs/rule.md` を正本とする。
- 同じ種類・同じ数値のカードが複数枚存在する場合も、各カードへ一意な `CardId` を割り当てる。
- 継続ゲームのメンバーカード山札再構築には、`ProjectState` が保持する同一の `MemberDeckSet` を使用する。
- 継続カード山札の再構築では、`ProjectState` が保持する同一の `ContinuationDeckSet` を維持する。

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

プロジェクト全体の状態遷移は、次のとおりとする。

```text
ProjectCreated
  -> ProjectInProgress
  -> ProjectCleared
```

または：

```text
ProjectCreated
  -> ProjectInProgress
  -> ProjectFailed
```

または：

```text
ProjectCreated
  -> ProjectInProgress
  -> ProjectCollapsed
```

次ゲームを開始できるのは、`ProjectState` が `InProgress` であり、選択したプロジェクトモードの継続条件を満たす場合のみとする。

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

`ProjectState` を使用する場合、プロジェクトモードとカードセットは `ProjectState` の値を引き継ぎ、呼び出し側から変更値を受け取らない。

---

### 14.11 startProject

```text
startProject(projectSettings) -> Result<ProjectState, Error>
```

プロジェクトモード、メンバーカードセット、継続カードセット、初期ゲーム設定から、新しいプロジェクト状態を作る。

---

### 14.12 completeProjectGame

```text
completeProjectGame(projectState, finishedGameState) -> Result<ProjectState, Error>
```

終了したゲームの最終スコアと最終コストを累積し、選択したプロジェクトモードに従って、次ゲームへ進むか、クリアまたは破綻として終了するかを判定する。

- 単発プロジェクトでは、監査処理後の勝敗をプロジェクト結果とする。
- 大型プロジェクトでは、途中敗北を破綻として処理し、3ゲーム終了時に合計200点以上ならクリア、200点未満なら失敗と判定する。
- エンドレスプロジェクトでは、敗北するまで次ゲームを許可する。

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
calculateProjectEfficiency(projectState)
judgeProjectResult(projectState)
```

`calculateProjectEfficiency` は、大型プロジェクトの累計最終スコアと累計最終コストを使用する。累計最終コストが0の場合を含め、除算不能な状態を表現できる戻り値とする。

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
InvalidProjectMode
ProjectAlreadyFinished
ProjectSetChangeNotAllowed
InvariantViolation
```

エラー発生時、元の `GameState` または `ProjectState` は変更されない。

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
- プロジェクトの累計最終スコア
- プロジェクトの累計最終コスト
- 大型プロジェクトの最終効率評価
- プロジェクトのクリア・破綻判定

計算結果を `GameState` または `ProjectState` にキャッシュするかどうかは実装判断とする。
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

20. プロジェクトモードはプロジェクト開始後に変更できない

21. メンバーカードセットはプロジェクト開始後に変更できない

22. 継続カードセットはプロジェクト開始後に変更できない

23. 選択したカードセットの構成枚数は docs/rule.md の定義に一致する

24. ProjectState の累計値は、完了したゲームの確定済み最終値だけを加算する

25. 終了済みまたは破綻済みのプロジェクトから次ゲームを開始できない
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
QSCD_ProjectHandle
qscd_create_game(...)
qscd_start_project(...)
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
- 安定型メンバーカードセットの構成枚数
- ハイリスク型メンバーカードセットの構成枚数
- 標準継続カードセットの構成枚数
- W継続カードセットの構成枚数
- プロジェクト開始後にカードセットを変更できないこと
- 単発プロジェクトが1ゲームで終了すること
- 単発プロジェクト終了時に監査だけが勝敗を変更すること
- 大型プロジェクトが途中敗北で破綻すること
- 大型プロジェクトが3ゲーム合計200点以上でクリアすること
- 大型プロジェクトが3ゲーム合計200点未満で失敗すること
- 大型プロジェクトの累計最終スコア、累計最終コスト、最終効率評価
- エンドレスプロジェクトが通常敗北まで継続すること
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

---

## 22. ホストアプリケーションとの責務分離

プロジェクトモード、カードセットの構成、山札生成、効果、累積値、クリア・破綻判定はC++コアの責務とする。

iOSアプリやWeb版などのホストアプリケーションは、次の責務を持つ。

- プロジェクトモードとカードセットの選択UI
- 表示文言とローカライズ
- 状態の永続化と復元
- iOS版におけるStoreKit購入、購入復元、利用権限の確認
- 利用可能な選択肢だけをコアへ渡す制御

C++コアは、カードセットが有料か無料かを認識しない。StoreKit、商品ID、購入状態などの課金情報を `GameState` または `ProjectState` に保持してはならない。

ホストアプリケーションが利用権限を確認した後、選択された `ProjectMode`、`MemberDeckSet`、`ContinuationDeckSet` をコアへ渡す。コアは購入状態ではなく、渡されたルール設定の妥当性とゲーム中の不変性だけを検証する。
