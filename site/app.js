import createQSCDWasmModule from "./wasm/qscd_wasm.js";

const usageCodes = {
  ScorePlus3: 1,
  Nullify: 2
};

const targetScore = 63;
const defaultCostLimit = 30;
const minTeamSize = 3;
const maxTeamSize = 6;
const minExpectedScore = 3;
const maxExpectedScore = 6;
const handSize = 5;
const boardRounds = 5;
const memberNames = ["Aさん", "Bさん", "Cさん", "Dさん", "Eさん", "Fさん"];

const state = {
  module: null,
  api: null,
  handle: 0,
  snapshot: null,
  selectedRevealCards: new Set(),
  selectedNullifyHandCardId: null,
  selectedNullifyTargetCardId: null,
  showDebug: false,
  nextDeckSeed: generateDeckSeed(),
  newGameTeamSize: 5,
  newGameExpectedScore: 4,
  continuationAddCount: 0,
  continuationExpectedScore: null,
  continuationRetiringColumns: new Set()
};

const root = document.querySelector("#app");

if (!root) {
  throw new Error("Missing #app root element");
}

function displayedNextCostLimit(snapshot) {
  if (!snapshot) {
    return defaultCostLimit;
  }
  if (snapshot.phase === "ContinuationCardsDrawn") {
    return snapshot.costLimit - (snapshot.pendingCostLimitReduction ?? 0);
  }
  return snapshot.costLimit ?? defaultCostLimit;
}

initialize();

async function initialize() {
  render();
  try {
    state.module = await createQSCDWasmModule({
      locateFile: (path) => `./wasm/${path}`
    });
    state.api = createApi(state.module);
    render();
  } catch (error) {
    root.innerHTML = `
      <section class="panel">
        <h1>QS=CD Web Demo</h1>
        <p class="error">Wasm の読み込みに失敗しました。公開ファイルに wasm/qscd_wasm.js と wasm/qscd_wasm.wasm が含まれているか確認してください。</p>
        <pre>${escapeHtml(String(error))}</pre>
      </section>
    `;
  }
}

function createApi(module) {
  return {
    createGame: module.cwrap("createGame", "number", ["number", "number", "number", "number", "number"]),
    destroyGame: module.cwrap("destroyGame", null, ["number"]),
    startRound: module.cwrap("startRound", "number", ["number"]),
    planHandUsage: module.cwrap("planHandUsage", "number", ["number", "number", "number", "number"]),
    revealCards: module.cwrap("revealCards", "number", ["number", "number", "number"]),
    applyNullify: module.cwrap("applyNullify", "number", ["number", "number", "number"]),
    finishRound: module.cwrap("finishRound", "number", ["number"]),
    requestExtraRound: module.cwrap("requestExtraRound", "number", ["number", "number"]),
    finishGame: module.cwrap("finishGame", "number", ["number"]),
    drawContinuationCards: module.cwrap("drawContinuationCards", "number", ["number"]),
    startContinuationGame: module.cwrap("startContinuationGame", "number", ["number", "number", "number", "number", "number", "number", "number", "number", "number"]),
    getStateSnapshot: module.cwrap("getStateSnapshot", "number", ["number"])
  };
}

function render() {
  root.innerHTML = `
    <header class="app-header">
      <div>
        <h1>QS=CD Project Board</h1>
        <p>Scope = Cost × Delivery を管理する Web デモ</p>
      </div>
      <div class="header-actions">
        <label class="debug-toggle"><input id="show-debug" type="checkbox" ${state.showDebug ? "checked" : ""} /> デバッグ情報を表示</label>
        <button id="new-game" type="button" ${state.api ? "" : "disabled"}>新しいゲーム</button>
      </div>
    </header>

    ${renderStartPanel()}
    ${state.snapshot ? renderGame() : ""}
  `;
  bindEvents();
}

function bindEvents() {
  root.querySelector("#show-debug")?.addEventListener("change", (event) => {
    state.showDebug = event.currentTarget.checked;
    render();
  });

  root.querySelector("#new-game")?.addEventListener("click", startNewGame);
  root.querySelector("#team-size")?.addEventListener("input", (event) => {
    state.newGameTeamSize = clampNumber(Number(event.currentTarget.value), minTeamSize, maxTeamSize);
  });
  root.querySelector("#expected-score")?.addEventListener("input", (event) => {
    state.newGameExpectedScore = clampNumber(Number(event.currentTarget.value), minExpectedScore, maxExpectedScore);
  });

  bindCommand("#start-round", () => state.api.startRound(state.handle));
  bindCommand("#plan-hand", () => planHandUsage(readHandPlan()));
  bindCommand("#reveal-cards", () => revealCards([...state.selectedRevealCards]));
  bindCommand("#apply-nullify", () => {
    if (state.selectedNullifyHandCardId == null || state.selectedNullifyTargetCardId == null) {
      return 0;
    }
    return state.api.applyNullify(state.handle, state.selectedNullifyHandCardId, state.selectedNullifyTargetCardId);
  });
  bindCommand("#finish-round", () => state.api.finishRound(state.handle));
  bindCommand("#request-extra-round", () => {
    const forced = root.querySelector("#forced-unpaid-overtime")?.checked ?? false;
    return state.api.requestExtraRound(state.handle, forced ? 1 : 0);
  });
  bindCommand("#finish-game", () => {
    const finished = state.api.finishGame(state.handle);
    if (finished) {
      state.api.drawContinuationCards(state.handle);
    }
    return finished;
  });
  bindCommand("#draw-continuation", () => state.api.drawContinuationCards(state.handle));

  root.querySelector("#start-continuation")?.addEventListener("click", startContinuationGame);
  root.querySelector("#continuation-add-count")?.addEventListener("change", (event) => {
    state.continuationAddCount = clampNumber(Number(event.currentTarget.value), 0, maxContinuationAddCount(state.snapshot));
    render();
  });
  root.querySelector("#continuation-expected-score")?.addEventListener("input", (event) => {
    state.continuationExpectedScore = clampNumber(Number(event.currentTarget.value), minExpectedScore, maxExpectedScore);
    render();
  });
  root.querySelectorAll("[data-retire-column]").forEach((input) => {
    input.addEventListener("change", () => {
      const column = Number(input.dataset.retireColumn);
      if (input.checked) {
        state.continuationRetiringColumns.add(column);
      } else {
        state.continuationRetiringColumns.delete(column);
      }
      render();
    });
  });

  root.querySelectorAll("[data-reveal-card]").forEach((input) => {
    input.addEventListener("change", () => {
      const id = Number(input.dataset.revealCard);
      if (input.checked) {
        state.selectedRevealCards.add(id);
      } else {
        state.selectedRevealCards.delete(id);
      }
      render();
    });
  });

  root.querySelectorAll("[name='nullify-hand']").forEach((input) => {
    input.addEventListener("change", () => {
      state.selectedNullifyHandCardId = Number(input.value);
      render();
    });
  });

  root.querySelectorAll("[name='nullify-target']").forEach((input) => {
    input.addEventListener("change", () => {
      state.selectedNullifyTargetCardId = Number(input.value);
      render();
    });
  });
}

function renderStartPanel() {
  const displayedTargetScore = state.snapshot?.phase === "ContinuationCardsDrawn"
    ? Math.max(state.snapshot.score, targetScore)
    : state.snapshot?.targetScore ?? targetScore;
  const isFirstGame = !state.snapshot || !state.snapshot.isContinuationGame;
  return `
    <section class="panel start-panel">
      <div>
        <h2>ゲーム開始</h2>
        <p class="target">目標スコア: <strong>${displayedTargetScore}</strong></p>
        <p class="target">コスト制約: <strong>${displayedNextCostLimit(state.snapshot)}</strong></p>
        <p class="hint">${isFirstGame ? "最初のゲームの目標スコアは固定です。" : "継続ゲームでは前回最終スコア以上に更新されます。"} スコア達成とコスト制約の両立が勝利条件です。新しいゲームごとにシードを自動生成します。</p>
      </div>
      <div class="start-settings">
        <label>チーム人数
          <input id="team-size" type="number" min="${minTeamSize}" max="${maxTeamSize}" value="${state.newGameTeamSize}" />
        </label>
        <label>一人あたり見込み
          <input id="expected-score" type="number" min="${minExpectedScore}" max="${maxExpectedScore}" value="${state.newGameExpectedScore}" />
        </label>
        ${state.showDebug ? `<div class="seed">次の deckSeed: <code>${state.nextDeckSeed}</code></div>` : ""}
      </div>
    </section>
  `;
}

function renderGame() {
  const snapshot = state.snapshot;
  const canApplyNullify = canSelectNullify(snapshot);
  const continuationReady = snapshot.phase === "ContinuationCardsDrawn";

  return `
    ${renderStatusBar(snapshot)}
    ${snapshot.error ? `<section class="panel error-panel"><strong>エラー:</strong> ${escapeHtml(snapshot.error.code)}</section>` : ""}

    <section class="main-board">
      ${renderTeam(snapshot)}
      ${renderProjectBoard(snapshot)}
      ${renderOperationPanel(snapshot, canApplyNullify)}
    </section>

    ${renderRoundHistory(snapshot)}
    ${renderContinuationCards(snapshot)}
    ${continuationReady ? renderContinuationSetup(snapshot) : ""}
    ${state.showDebug ? renderDebug(snapshot) : ""}
  `;
}

function renderStatusBar(snapshot) {
  return `
    <section class="status-bar" aria-label="ステータス">
      <div><span>Scope</span><strong>${snapshot.score} / ${snapshot.targetScore}</strong></div>
      <div><span>Cost</span><strong>${snapshot.cost} / ${snapshot.costLimit}</strong></div>
      <div><span>Delivery</span><strong>${snapshot.currentRound} / ${snapshot.maxRound}</strong></div>
      <div><span>見込み</span><strong>${snapshot.globalExpectedScore}</strong></div>
      <div><span>介入</span><strong>${snapshot.handCards.length} / ${handSize}</strong></div>
      <div><span>状態</span><strong>${phaseLabel(snapshot.phase)}</strong></div>
    </section>
  `;
}

function renderTeam(snapshot) {
  const memberEffects = continuationMemberEffects(snapshot);
  return `
    <section class="panel team-panel">
      <h2>チーム</h2>
      <div class="team-list">
        ${Array.from({ length: snapshot.teamSize }, (_, column) => {
          const member = snapshot.members[column] ?? { state: "Active" };
          const effects = memberEffects.get(column) ?? [];
          return `
            <article class="team-member ${member.state}">
              <h3>${memberName(column)}</h3>
              <p>${memberStateLabel(member.state)}</p>
              <p>継続: ${effects.length ? effects.map(continuationEffectLabel).join("、") : "なし"}</p>
            </article>
          `;
        }).join("")}
      </div>
    </section>
  `;
}

function renderProjectBoard(snapshot) {
  const rows = Array.from({ length: boardRounds }, (_, index) => boardRounds - index);
  const columns = Array.from({ length: snapshot.teamSize }, (_, index) => index);
  const boardByRound = new Map(snapshot.board.map((row) => [row.row, row]));

  return `
    <section class="panel project-panel">
      <h2>プロジェクト盤面</h2>
      <table class="project-board">
        <thead>
          <tr>
            <th></th>
            ${columns.map((column) => `<th>${memberName(column, false)}</th>`).join("")}
            <th>PM</th>
          </tr>
        </thead>
        <tbody>
          ${rows.map((round) => {
            const row = boardByRound.get(round);
            const unused = !row;
            return `
              <tr class="${unused ? "unused-round" : ""}">
                <th>Round ${round}</th>
                ${columns.map((column) => `<td>${renderBoardCell(row, column, unused)}</td>`).join("")}
                <td>${renderPmCell(snapshot, round, unused)}</td>
              </tr>
            `;
          }).join("")}
        </tbody>
      </table>
    </section>
  `;
}

function renderBoardCell(row, column, unused) {
  if (unused) {
    return `<div class="empty-card">未使用</div>`;
  }
  const card = row.cards.find((item) => item.column === column);
  if (!card) {
    return `<div class="empty-card">空き</div>`;
  }
  return renderCardFace(card);
}

function renderCardFace(card) {
  if (card.kind === "MemberLeaveProject" && card.runtimeState === "FaceUp") {
    return `<div class="state-card leave">離脱</div>`;
  }
  if (card.runtimeState === "FaceUp") {
    return `<div class="state-card success"><strong>${card.scoreValue ?? "-"}</strong><span>成果</span></div>`;
  }
  if (card.runtimeState === "NullifiedAsFaceDown") {
    return `<div class="state-card warning"><strong>?</strong><span>無効化</span></div>`;
  }
  return `<div class="state-card facedown"><strong>?</strong><span>裏向き</span></div>`;
}

function renderPmCell(snapshot, round, unused) {
  if (unused) {
    return `<div class="empty-card">未使用</div>`;
  }
  const sideCards = snapshot.boardSideCards.filter((card) => card.row === round);
  if (sideCards.length === 0) {
    return `<div class="empty-card">なし</div>`;
  }
  return sideCards.map((card) => `
    <div class="state-card pm">
      <strong>${card.usage === "ScorePlus3" ? "+3" : "無効化"}</strong>
      <span>${card.usage === "ScorePlus3" ? "スコア+3" : nullifyTargetLabel(card)}</span>
    </div>
  `).join("");
}

function renderOperationPanel(snapshot, canApplyNullify) {
  return `
    <section class="panel operation-panel">
      <h2>現在の操作</h2>
      <p class="phase-message">${operationMessage(snapshot)}</p>
      ${renderPmIntervention(snapshot)}
      ${renderPhaseControls(snapshot, canApplyNullify)}
    </section>
  `;
}

function renderPhaseControls(snapshot, canApplyNullify) {
  if (snapshot.phase === "GameStarted" || snapshot.phase === "ExtraRoundRequested") {
    return `<button id="start-round" type="button">ラウンドを開始</button>`;
  }
  if (snapshot.phase === "RoundStarted") {
    return `
      <p>使用計画を最大2枠で決めてください。</p>
      <button id="plan-hand" type="button">使用計画を確定</button>
    `;
  }
  if (snapshot.phase === "HandPlanned") {
    const selected = state.selectedRevealCards.size;
    return `
      <p>選択: ${selected} / ${snapshot.revealLimit}</p>
      <div class="reveal-list">
        ${revealableCardsInMemberOrder(snapshot).map((card) => `
          <label class="select-card">
            <input type="checkbox" data-reveal-card="${card.cardId}" ${state.selectedRevealCards.has(card.cardId) ? "checked" : ""} />
            Round ${card.row} / ${memberName(card.column)}
          </label>
        `).join("") || "<p>公開可能カードなし</p>"}
      </div>
      <button id="reveal-cards" type="button">公開する</button>
    `;
  }
  if (snapshot.phase === "CardsRevealed") {
    return `
      ${renderNullifySelection(snapshot)}
      <button id="apply-nullify" type="button" ${canApplyNullify ? "" : "disabled"}>無効化を適用</button>
      <button id="finish-round" type="button">ラウンドを終了</button>
    `;
  }
  if (snapshot.phase === "RoundFinished") {
    return renderRoundFinishedControls(snapshot);
  }
  if (snapshot.phase === "GameFinished") {
    return `<button id="draw-continuation" type="button">継続カードを処理</button>`;
  }
  if (snapshot.phase === "ContinuationCardsDrawn") {
    return `<p>継続カード処理済みです。次ゲームのチーム編成を確認してください。</p>`;
  }
  return `<p>操作待ちです。</p>`;
}

function renderRoundFinishedControls(snapshot) {
  const reachedTarget = snapshot.score >= snapshot.targetScore;
  const withinCostLimit = snapshot.cost <= snapshot.costLimit;
  const beforeBaseRounds = snapshot.currentRound < 3;
  const canRequestExtra = snapshot.currentRound >= 3 && snapshot.currentRound < boardRounds && !reachedTarget;
  const canFinish = !beforeBaseRounds && (reachedTarget || snapshot.currentRound >= boardRounds);
  const costMessage = withinCostLimit
    ? "現在のコストは制約内です。"
    : "現在のコストは制約を超過しているため、このまま終了するとコスト敗北です。";

  if (beforeBaseRounds) {
    return `
      <p>最低3ラウンドまで続きます。次のラウンドを開始してください。</p>
      <button id="start-round" type="button">次のラウンドを開始</button>
    `;
  }

  if (canRequestExtra) {
    return `
      <p>目標未達です。最大5ラウンドまで追加ラウンドを行えます。</p>
      <label class="danger-option"><input id="forced-unpaid-overtime" type="checkbox" /> サビ残強制</label>
      <button id="request-extra-round" type="button">追加ラウンドへ進む</button>
      <button id="finish-game" type="button" disabled>ゲーム終了</button>
    `;
  }

  return `
    <p>${reachedTarget ? "スコア目標は達成済みです。" : "最大ラウンドに到達しました。"} ${costMessage}</p>
    <button id="finish-game" type="button" ${canFinish ? "" : "disabled"}>ゲーム終了</button>
  `;
}

function renderNullifySelection(snapshot) {
  const nullifyHandCards = availableNullifyHandCards(snapshot);
  const nullifyTargetCards = nullifiableBoardCards(snapshot);
  return `
    <div class="nullify-select">
      <div>
        <h3>無効化介入</h3>
        ${nullifyHandCards.map((card) => `
          <label class="select-card">
            <input type="radio" name="nullify-hand" value="${card.cardId}" ${state.selectedNullifyHandCardId === card.cardId ? "checked" : ""} />
            使用枠${card.slot + 1}
          </label>
        `).join("") || "<p>無効化の宣言なし</p>"}
      </div>
      <div>
        <h3>対象</h3>
        ${nullifyTargetCards.map((card) => `
          <label class="select-card">
            <input type="radio" name="nullify-target" value="${card.cardId}" ${state.selectedNullifyTargetCardId === card.cardId ? "checked" : ""} />
            ${memberName(card.column)} ${card.kind === "MemberLeaveProject" ? "離脱" : `成果${card.scoreValue ?? ""}`}
          </label>
        `).join("") || "<p>表向きカードなし</p>"}
      </div>
    </div>
  `;
}

function renderPmIntervention(snapshot) {
  const remaining = snapshot.handCards.length;
  const used = handSize - remaining;
  const resourceDots = [
    ...Array.from({ length: remaining }, () => `<span class="pm-dot available">●</span>`),
    ...Array.from({ length: used }, () => `<span class="pm-dot spent">○</span>`)
  ].join("");

  return `
    <div class="pm-intervention">
      <h3>PM介入</h3>
      <div class="pm-resource">
        <div>
          <span class="pm-resource-label">残り手札</span>
          <div class="pm-dots" aria-label="残りPM介入 ${remaining} / ${handSize}">${resourceDots}</div>
        </div>
        <strong>${remaining} / ${handSize}</strong>
      </div>
      <p class="hint">残り5回のPM介入を、いつ・どの方法で使うかを管理します。</p>
      ${renderPmInterventionSlots(snapshot)}
    </div>
  `;
}

function renderPmInterventionSlots(snapshot) {
  const currentPlan = snapshot.boardSideCards.filter((card) => card.row === snapshot.currentRound).sort((a, b) => a.slot - b.slot);
  return `
    <div class="intervention-slots">
      ${[0, 1].map((slot) => renderInterventionSlot(snapshot, slot, currentPlan[slot])).join("")}
    </div>
  `;
}

function renderInterventionSlot(snapshot, slot, plannedCard) {
  if (plannedCard) {
    return `
      <article class="intervention-slot locked">
        <h3>使用枠${slot + 1}</h3>
        <p>用途: ${usageLabel(plannedCard.usage)}</p>
      </article>
    `;
  }

  const disabled = snapshot.phase !== "RoundStarted" || slot >= snapshot.handCards.length;
  return `
    <article class="intervention-slot">
      <h3>使用枠${slot + 1}</h3>
      <label>用途
        <select name="plan-usage-${slot}" ${disabled ? "disabled" : ""}>
          <option value="">使用しない</option>
          <option value="ScorePlus3">スコア+3</option>
          <option value="Nullify">無効化</option>
        </select>
      </label>
    </article>
  `;
}

function renderRoundHistory(snapshot) {
  const rows = Array.from({ length: snapshot.currentRound }, (_, index) => index + 1);
  return `
    <section class="panel">
      <h2>ラウンド履歴・集計</h2>
      <table class="history-table">
        <thead>
          <tr><th>Round</th><th>表向きスコア</th><th>見込みスコア</th><th>PMスコア</th><th>ラウンドスコア</th><th>コスト</th></tr>
        </thead>
        <tbody>
          ${rows.map((round) => {
            const summary = roundSummary(snapshot, round);
            return `<tr><th>${round}</th><td>${summary.faceUpScore}</td><td>${summary.expectedScore}</td><td>${summary.pmScore}</td><td>${summary.roundScore}</td><td>${summary.cost}</td></tr>`;
          }).join("") || `<tr><td colspan="6">履歴なし</td></tr>`}
        </tbody>
      </table>
    </section>
  `;
}

function renderContinuationCards(snapshot) {
  const teamCards = activeContinuationCards(snapshot).filter((card) => card.position === "ContinuationTeamArea");
  const memberCards = activeContinuationCards(snapshot).filter((card) => card.position === "ContinuationMemberArea");
  return `
    <section class="panel continuation-panel">
      <h2>継続カード</h2>
      <div class="continuation-grid">
        <div>
          <h3>チーム系</h3>
          ${teamCards.length ? groupedContinuation(teamCards).map(([kind, count]) => `<p>${continuationEffectLabel(kind)} ×${count}</p>`).join("") : "<p>なし</p>"}
        </div>
        <div>
          <h3>メンバー系</h3>
          ${memberCards.length ? memberCards.map((card) => `<p>${memberName(card.column)}: ${continuationEffectLabel(card.kind)}</p>`).join("") : "<p>なし</p>"}
        </div>
      </div>
    </section>
  `;
}

function renderContinuationSetup(snapshot) {
  normalizeContinuationSelection(snapshot);
  const activeColumns = continuationActiveColumns(snapshot);
  const forcedRetiringColumns = snapshot.members
    .map((member, column) => member.state === "Resigned" ? column : null)
    .filter((column) => column != null);
  const selectedRetiringColumns = [...state.continuationRetiringColumns].sort((a, b) => a - b);
  const nextSize = continuationNextTeamSize(snapshot);
  const nextSizeValid = nextSize >= minTeamSize && nextSize <= maxTeamSize;
  const nextCostLimit = snapshot.costLimit - (snapshot.pendingCostLimitReduction ?? 0);
  const nextExpectedScore = state.continuationExpectedScore ?? snapshot.globalExpectedScore;
  const compositionChanged = forcedRetiringColumns.length > 0 || selectedRetiringColumns.length > 0 || state.continuationAddCount > 0;
  const preview = continuationPreview(snapshot);
  return `
    <section class="panel continuation-setup">
      <h2>継続ゲーム開始</h2>
      <div class="continuation-grid">
        <div>
          <h3>今回獲得</h3>
          ${snapshot.drawnContinuationCards.length ? `<ul>${snapshot.drawnContinuationCards.map((card) => `<li>${memberName(card.column)}: ${continuationEffectLabel(card.kind)}</li>`).join("")}</ul>` : "<p>なし</p>"}
        </div>
        <div>
          <h3>適用中カード</h3>
          ${renderContinuationPreviewCards(preview)}
        </div>
        <div>
          <h3>チーム編成</h3>
          <div class="team-edit">
            <label>追加要員数
              <input id="continuation-add-count" type="number" min="0" max="${maxContinuationAddCount(snapshot)}" value="${state.continuationAddCount}" />
            </label>
            <span>次ゲーム人数: <strong class="${nextSizeValid ? "" : "warning-text"}">${nextSize} / ${minTeamSize}〜${maxTeamSize}</strong></span>
            <label>一人あたり見込み
              <input id="continuation-expected-score" type="number" min="${minExpectedScore}" max="${maxExpectedScore}" value="${nextExpectedScore}" />
            </label>
            <span>次ゲームコスト制約: <strong>${nextCostLimit}</strong></span>
          </div>
          <div class="retire-select">
            <h4>離任させるメンバー</h4>
            ${activeColumns.map((column) => `
              <label class="select-card">
                <input type="checkbox" data-retire-column="${column}" ${state.continuationRetiringColumns.has(column) ? "checked" : ""} />
                ${memberName(column)}
              </label>
            `).join("") || "<p>選択可能な在籍メンバーなし</p>"}
          </div>
          <div class="next-team">
            ${renderNextTeamMembers(snapshot, preview)}
            ${Array.from({ length: state.continuationAddCount }, (_, index) => `<span class="joining">追加要員${index + 1}</span>`).join("")}
          </div>
          ${compositionChanged ? `<p class="warning-text">次ゲーム開始時にチームスコアアップ失効</p>` : ""}
          ${nextSizeValid ? "" : `<p class="warning-text">チーム人数は${minTeamSize}〜${maxTeamSize}にしてください。</p>`}
        </div>
      </div>
      ${state.showDebug ? `<p class="seed">次ゲーム deckSeed: <code>${state.nextDeckSeed}</code></p>` : ""}
      <button id="start-continuation" type="button" ${nextSizeValid ? "" : "disabled"}>次ゲームを開始</button>
    </section>
  `;
}

function renderDebug(snapshot) {
  const debug = {
    deckSeed: snapshot.deckSeed,
    gameId: state.handle,
    phase: snapshot.phase,
    round: snapshot.currentRound,
    score: snapshot.score,
    cost: snapshot.cost,
    targetScore: snapshot.targetScore,
    baseCostLimit: snapshot.baseCostLimit,
    cumulativeCostLimitReduction: snapshot.cumulativeCostLimitReduction,
    teamSize: snapshot.teamSize,
    globalExpectedScore: snapshot.globalExpectedScore,
    forcedUnpaidOvertimeRounds: snapshot.forcedUnpaidOvertimeRounds,
    pendingCostLimitReduction: snapshot.pendingCostLimitReduction
  };
  return `
    <details class="panel debug-panel" open>
      <summary>デバッグ情報</summary>
      <pre>${escapeHtml(JSON.stringify(debug, null, 2))}</pre>
    </details>
  `;
}

function startNewGame() {
  if (!state.api) {
    return;
  }
  if (state.handle !== 0) {
    state.api.destroyGame(state.handle);
  }
  const seed = generateDeckSeed();
  state.nextDeckSeed = seed;
  state.handle = state.api.createGame(targetScore, state.newGameTeamSize, state.newGameExpectedScore, defaultCostLimit, seed);
  resetContinuationDraft();
  updateSnapshot(readSnapshot());
  state.nextDeckSeed = generateDeckSeed();
}

function startContinuationGame() {
  if (!state.api || !state.snapshot) {
    return;
  }
  const seed = generateDeckSeed();
  normalizeContinuationSelection(state.snapshot);
  const nextTeamSize = continuationNextTeamSize(state.snapshot);
  if (nextTeamSize < minTeamSize || nextTeamSize > maxTeamSize) {
    return;
  }
  const retiringColumns = [...state.continuationRetiringColumns].sort((a, b) => a - b);
  const compositionChanged = state.snapshot.members.some((member) => member.state !== "Active") ||
    retiringColumns.length > 0 ||
    state.continuationAddCount > 0;
  withUint32Array(retiringColumns, (retiringColumnsPtr) => {
    state.api.startContinuationGame(
      state.handle,
      Math.max(state.snapshot.score, targetScore),
      nextTeamSize,
      state.continuationExpectedScore ?? state.snapshot.globalExpectedScore,
      state.snapshot.baseCostLimit ?? defaultCostLimit,
      compositionChanged ? 1 : 0,
      seed,
      retiringColumnsPtr,
      retiringColumns.length
    );
  });
  resetContinuationDraft();
  updateSnapshot(readSnapshot());
  state.nextDeckSeed = generateDeckSeed();
}

function resetContinuationDraft() {
  state.continuationAddCount = 0;
  state.continuationExpectedScore = null;
  state.continuationRetiringColumns.clear();
}

function bindCommand(selector, command) {
  root.querySelector(selector)?.addEventListener("click", () => {
    command();
    updateSnapshot(readSnapshot());
  });
}

function updateSnapshot(snapshot) {
  if (!snapshot) {
    return;
  }
  state.snapshot = snapshot;
  state.selectedRevealCards.clear();
  state.selectedNullifyHandCardId = null;
  state.selectedNullifyTargetCardId = null;
  if (snapshot.phase === "ContinuationCardsDrawn") {
    if (state.continuationExpectedScore == null) {
      state.continuationExpectedScore = clampNumber(snapshot.globalExpectedScore, minExpectedScore, maxExpectedScore);
    }
    normalizeContinuationSelection(snapshot);
  }
  render();
}

function readSnapshot() {
  const ptr = state.api.getStateSnapshot(state.handle);
  return JSON.parse(readCString(ptr));
}

function readCString(ptr) {
  if (!ptr) {
    return "";
  }
  const heap = new Uint8Array(state.module.HEAPU32.buffer);
  let end = ptr;
  while (heap[end] !== 0) {
    end += 1;
  }
  let result = "";
  const chunkSize = 0x8000;
  for (let offset = ptr; offset < end; offset += chunkSize) {
    const chunkEnd = Math.min(offset + chunkSize, end);
    result += String.fromCharCode(...heap.subarray(offset, chunkEnd));
  }
  return result;
}

function planHandUsage(plan) {
  const cardIds = plan.map((item) => item.cardId);
  const usages = plan.map((item) => usageCodes[item.usage]);
  withUint32AndInt32Arrays(cardIds, usages, (cardIdsPtr, usagesPtr) => {
    state.api.planHandUsage(state.handle, cardIdsPtr, usagesPtr, plan.length);
  });
}

function revealCards(cardIds) {
  withUint32Array(cardIds, (ptr) => {
    state.api.revealCards(state.handle, ptr, cardIds.length);
  });
}

function withUint32Array(values, callback) {
  const bytes = Math.max(values.length * 4, 4);
  const ptr = state.module._malloc(bytes);
  try {
    state.module.HEAPU32.set(values, ptr >>> 2);
    callback(ptr);
  } finally {
    state.module._free(ptr);
  }
}

function withUint32AndInt32Arrays(unsignedValues, signedValues, callback) {
  const bytes = Math.max(unsignedValues.length * 4, 4);
  const unsignedPtr = state.module._malloc(bytes);
  const signedPtr = state.module._malloc(bytes);
  try {
    state.module.HEAPU32.set(unsignedValues, unsignedPtr >>> 2);
    state.module.HEAP32.set(signedValues, signedPtr >>> 2);
    callback(unsignedPtr, signedPtr);
  } finally {
    state.module._free(unsignedPtr);
    state.module._free(signedPtr);
  }
}

function readHandPlan() {
  const availableHandCards = [...(state.snapshot?.handCards ?? [])];
  let nextCardIndex = 0;
  return [0, 1].map((slot) => {
    const usage = root.querySelector(`[name='plan-usage-${slot}']`)?.value;
    if (!usage) {
      return null;
    }
    const card = availableHandCards[nextCardIndex];
    nextCardIndex += 1;
    return card ? { cardId: card.cardId, usage } : null;
  }).filter(Boolean);
}

function availableNullifyHandCards(snapshot) {
  return snapshot.boardSideCards.filter((card) =>
    card.row === snapshot.currentRound &&
    card.usage === "Nullify" &&
    card.targetCardId == null
  );
}

function nullifiableBoardCards(snapshot) {
  return snapshot.board.flatMap((row) =>
    row.row === snapshot.currentRound
      ? row.cards
        .filter((card) => card.runtimeState === "FaceUp")
        .map((card) => ({ ...card, row: row.row }))
      : []
  );
}

function canSelectNullify(snapshot) {
  const nullifyHandCards = availableNullifyHandCards(snapshot);
  const nullifyTargetCards = nullifiableBoardCards(snapshot);
  return nullifyHandCards.some((card) => card.cardId === state.selectedNullifyHandCardId) &&
    nullifyTargetCards.some((card) => card.cardId === state.selectedNullifyTargetCardId);
}

function revealableCardsInMemberOrder(snapshot) {
  return [...snapshot.revealableCards].sort((left, right) =>
    ((left.row ?? Number.MAX_SAFE_INTEGER) - (right.row ?? Number.MAX_SAFE_INTEGER)) ||
    ((left.column ?? Number.MAX_SAFE_INTEGER) - (right.column ?? Number.MAX_SAFE_INTEGER)) ||
    (left.cardId - right.cardId)
  );
}

function roundSummary(snapshot, round) {
  const row = snapshot.board.find((item) => item.row === round);
  const cards = row?.cards ?? [];
  const forced = snapshot.forcedUnpaidOvertimeRounds?.includes(round) ?? false;
  const faceUpScore = cards.reduce((sum, card) => sum + (card.runtimeState === "FaceUp" && card.kind === "MemberScore" ? (card.scoreValue ?? 0) : 0), 0);
  const expectedScore = cards.reduce((sum, card) => sum + (card.runtimeState === "FaceDown" || card.runtimeState === "NullifiedAsFaceDown" ? snapshot.globalExpectedScore : 0), 0);
  const pmScore = snapshot.boardSideCards.filter((card) => card.row === round && card.usage === "ScorePlus3").length * 3;
  return {
    faceUpScore,
    expectedScore,
    pmScore,
    roundScore: faceUpScore + expectedScore + pmScore,
    cost: forced ? 0 : cards.length
  };
}

function activeContinuationCards(snapshot) {
  return (snapshot.continuationCards ?? []).filter((card) =>
    card.position === "ContinuationMemberArea" || card.position === "ContinuationTeamArea"
  );
}

function continuationPreview(snapshot) {
  const columnMap = continuationColumnMap(snapshot);
  const compositionChanged = snapshot.members.some((member) => member.state !== "Active") ||
    state.continuationRetiringColumns.size > 0 ||
    state.continuationAddCount > 0;
  const applied = [];
  const expired = [];
  const effectsByNextColumn = new Map();

  activeContinuationCards(snapshot).forEach((card) => {
    if (card.position === "ContinuationTeamArea") {
      if (compositionChanged) {
        expired.push({ ...card, reason: "チーム構成変更で失効し山札へ戻る" });
      } else {
        applied.push({ ...card, scope: "team" });
      }
      return;
    }

    const nextColumn = columnMap.get(card.column);
    if (nextColumn == null) {
      expired.push({ ...card, reason: "メンバー離任で失効し山札へ戻る" });
      return;
    }

    const previewCard = { ...card, scope: "member", nextColumn };
    applied.push(previewCard);
    const effects = effectsByNextColumn.get(nextColumn) ?? [];
    effects.push(card.kind);
    effectsByNextColumn.set(nextColumn, effects);
  });

  return { applied, expired, effectsByNextColumn, columnMap };
}

function continuationColumnMap(snapshot) {
  const map = new Map();
  let nextColumn = 0;
  snapshot.members.forEach((member, column) => {
    if (member.state === "Active" && !state.continuationRetiringColumns.has(column)) {
      map.set(column, nextColumn);
      nextColumn += 1;
    }
  });
  return map;
}

function renderContinuationPreviewCards(preview) {
  if (preview.applied.length === 0 && preview.expired.length === 0) {
    return "<p>なし</p>";
  }
  const applied = preview.applied.map((card) => {
    if (card.scope === "team") {
      return `<p>チーム: ${continuationEffectLabel(card.kind)}</p>`;
    }
    return `<p>${memberName(card.nextColumn)}${memberMoveLabel(card.column, card.nextColumn)}: ${continuationEffectLabel(card.kind)}</p>`;
  }).join("");
  const expired = preview.expired.map((card) => {
    const owner = card.position === "ContinuationTeamArea" ? "チーム" : memberName(card.column);
    return `<p class="warning-text">${owner}: ${continuationEffectLabel(card.kind)}（${card.reason}）</p>`;
  }).join("");
  return `${applied}${expired}`;
}

function renderNextTeamMembers(snapshot, preview) {
  const retainedMembers = [];
  snapshot.members.forEach((member, oldColumn) => {
    const nextColumn = preview.columnMap.get(oldColumn);
    if (nextColumn == null) {
      const plannedRetire = state.continuationRetiringColumns.has(oldColumn);
      const forcedRetire = member.state === "Resigned";
      const label = forcedRetire ? "離任予定" : plannedRetire ? "離任選択" : member.state === "LeftProject" ? "離脱済み" : "除外";
      retainedMembers.push(`<span class="resigning">${memberName(oldColumn)}（${label}）</span>`);
      return;
    }
    const effects = preview.effectsByNextColumn.get(nextColumn) ?? [];
    retainedMembers.push(`
      <span>
        ${memberName(nextColumn)}${memberMoveLabel(oldColumn, nextColumn)}
        ${effects.length ? `<small>継続: ${effects.map(continuationEffectLabel).join("、")}</small>` : ""}
      </span>
    `);
  });
  return retainedMembers.join("");
}

function memberMoveLabel(oldColumn, nextColumn) {
  return oldColumn === nextColumn ? "" : `<span class="muted">（旧${memberName(oldColumn)}）</span>`;
}

function continuationMemberEffects(snapshot) {
  const effects = new Map();
  activeContinuationCards(snapshot)
    .filter((card) => card.position === "ContinuationMemberArea")
    .forEach((card) => {
      const list = effects.get(card.column) ?? [];
      list.push(card.kind);
      effects.set(card.column, list);
    });
  return effects;
}

function groupedContinuation(cards) {
  const counts = new Map();
  cards.forEach((card) => counts.set(card.kind, (counts.get(card.kind) ?? 0) + 1));
  return [...counts.entries()];
}

function continuationActiveColumns(snapshot) {
  return snapshot.members
    .map((member, column) => member.state === "Active" ? column : null)
    .filter((column) => column != null);
}

function continuationNextTeamSize(snapshot) {
  const activeCount = continuationActiveColumns(snapshot).length;
  return activeCount - state.continuationRetiringColumns.size + state.continuationAddCount;
}

function maxContinuationAddCount(snapshot) {
  const activeCount = continuationActiveColumns(snapshot).length;
  return Math.max(0, maxTeamSize - activeCount + state.continuationRetiringColumns.size);
}

function normalizeContinuationSelection(snapshot) {
  if (!snapshot) {
    resetContinuationDraft();
    return;
  }
  const activeColumns = new Set(continuationActiveColumns(snapshot));
  [...state.continuationRetiringColumns].forEach((column) => {
    if (!activeColumns.has(column)) {
      state.continuationRetiringColumns.delete(column);
    }
  });
  state.continuationAddCount = clampNumber(state.continuationAddCount, 0, maxContinuationAddCount(snapshot));
}

function generateDeckSeed() {
  if (globalThis.crypto?.getRandomValues) {
    const values = new Uint32Array(1);
    globalThis.crypto.getRandomValues(values);
    return values[0] >>> 0;
  }
  return (Date.now() ^ Math.floor(Math.random() * 0xFFFFFFFF)) >>> 0;
}

function clampNumber(value, min, max) {
  if (Number.isNaN(value)) {
    return min;
  }
  return Math.min(max, Math.max(min, value));
}

function memberName(column, suffix = true) {
  const name = memberNames[column] ?? `${column + 1}`;
  return suffix ? name : name.replace("さん", "");
}

function phaseLabel(phase) {
  const labels = {
    Created: "作成",
    GameStarted: "ゲーム開始",
    RoundStarted: "使用計画",
    HandPlanned: "カード公開",
    CardsRevealed: "公開後処理",
    RoundFinished: "ラウンド終了",
    ExtraRoundRequested: "追加ラウンド",
    GameFinished: "ゲーム終了",
    ContinuationCardsDrawn: "継続処理",
    ContinuationGamePrepared: "継続準備"
  };
  return labels[phase] ?? phase;
}

function operationMessage(snapshot) {
  if (snapshot.phase === "GameStarted" || snapshot.phase === "ExtraRoundRequested") {
    return "次のラウンドを開始してください";
  }
  if (snapshot.phase === "RoundStarted") {
    return "今ラウンドで使うPM介入を最大2回まで計画してください";
  }
  if (snapshot.phase === "HandPlanned") {
    return `カードを${snapshot.revealLimit}枚公開してください`;
  }
  if (snapshot.phase === "CardsRevealed") {
    return "必要なら無効化対象を選び、ラウンドを終了してください";
  }
  if (snapshot.phase === "RoundFinished") {
    if (snapshot.currentRound < 3) {
      return "最低3ラウンドまで進行してください";
    }
    if (snapshot.score < snapshot.targetScore && snapshot.currentRound < boardRounds) {
      return "目標未達のため追加ラウンドへ進んでください";
    }
    return "ゲームを終了できます";
  }
  if (snapshot.phase === "ContinuationCardsDrawn") {
    return "継続ゲーム開始前のチーム編成を行ってください";
  }
  return "操作を選択してください";
}

function memberStateLabel(value) {
  const labels = {
    Active: "在籍中",
    LeftProject: "離脱済み",
    Resigned: "離任予定"
  };
  return labels[value] ?? value;
}

function usageLabel(value) {
  return value === "ScorePlus3" ? "スコア+3" : "無効化";
}

function continuationEffectLabel(kind) {
  const labels = {
    ContinuationAudit: "監査",
    ContinuationResign: "離任",
    ContinuationTeamScoreUp: "チームスコアアップ",
    ContinuationMemberCostUp: "コスト+1",
    ContinuationMemberScoreUp: "スコア+2",
    ContinuationCostReductionPressure: "コスト削減圧力",
    ContinuationNone: "何もなし"
  };
  return labels[kind] ?? kind;
}

function nullifyTargetLabel(card) {
  return card.targetCardId == null ? "対象未選択" : "対象選択済み";
}

function escapeHtml(value) {
  return String(value)
    .replaceAll("&", "&amp;")
    .replaceAll("<", "&lt;")
    .replaceAll(">", "&gt;")
    .replaceAll('"', "&quot;")
    .replaceAll("'", "&#039;");
}
