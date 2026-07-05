import createQSCDWasmModule from "./wasm/qscd_wasm.js";

const usageCodes = {
  ScorePlus3: 1,
  Nullify: 2
};

const defaultDeckSeed = 0x51534344;

const state = {
  module: null,
  api: null,
  handle: 0,
  snapshot: null,
  selectedRevealCards: new Set(),
  selectedNullifyHandCardId: null,
  selectedNullifyTargetCardId: null,
  showDebugCardIds: false
};

const root = document.querySelector("#app");

if (!root) {
  throw new Error("Missing #app root element");
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
        <pre>${String(error)}</pre>
      </section>
    `;
  }
}

function createApi(module) {
  return {
    createGame: module.cwrap("createGame", "number", ["number", "number", "number", "number", "number", "number"]),
    destroyGame: module.cwrap("destroyGame", null, ["number"]),
    startRound: module.cwrap("startRound", "number", ["number"]),
    planHandUsage: module.cwrap("planHandUsage", "number", ["number", "number", "number", "number"]),
    revealCards: module.cwrap("revealCards", "number", ["number", "number", "number"]),
    applyNullify: module.cwrap("applyNullify", "number", ["number", "number", "number"]),
    finishRound: module.cwrap("finishRound", "number", ["number"]),
    requestExtraRound: module.cwrap("requestExtraRound", "number", ["number", "number"]),
    finishGame: module.cwrap("finishGame", "number", ["number"]),
    getStateSnapshot: module.cwrap("getStateSnapshot", "number", ["number"])
  };
}

function render() {
  root.innerHTML = `
    <section class="panel">
      <h1>QS=CD Web Demo</h1>
      <p class="hint">C++20 コアロジックを WebAssembly で動かすブラウザデモです。</p>
      <form id="new-game-form" class="settings">
        <label>targetScore <input name="targetScore" type="number" value="60" required /></label>
        <label>teamSize <input name="teamSize" type="number" value="5" required /></label>
        <label>globalExpectedScore <input name="globalExpectedScore" type="number" value="4" required /></label>
        <label>deckSeed <input name="deckSeed" type="number" min="0" max="4294967295" value="${defaultDeckSeed}" required /></label>
        <label>costLimit optional <input name="costLimit" type="number" placeholder="なし" /></label>
        <button type="submit" ${state.api ? "" : "disabled"}>ゲーム開始</button>
      </form>
    </section>
    ${state.snapshot ? renderGame() : ""}
  `;

  root.querySelector("#new-game-form")?.addEventListener("submit", (event) => {
    event.preventDefault();
    if (!state.api) {
      return;
    }

    const form = new FormData(event.currentTarget);
    if (state.handle !== 0) {
      state.api.destroyGame(state.handle);
    }
    const costLimit = optionalNumberValue(form, "costLimit");
    state.handle = state.api.createGame(
      numberValue(form, "targetScore"),
      numberValue(form, "teamSize"),
      numberValue(form, "globalExpectedScore"),
      costLimit == null ? 0 : 1,
      costLimit ?? 0,
      numberValue(form, "deckSeed")
    );
    updateSnapshot(readSnapshot());
  });

  bindCommand("#start-round", () => callAndSnapshot(() => state.api.startRound(state.handle)));
  bindCommand("#plan-hand", () => callAndSnapshot(() => planHandUsage(readHandPlan())));
  bindCommand("#reveal-cards", () => callAndSnapshot(() => revealCards([...state.selectedRevealCards])));
  bindCommand("#apply-nullify", () => {
    if (state.selectedNullifyHandCardId == null || state.selectedNullifyTargetCardId == null) {
      return state.snapshot;
    }
    return callAndSnapshot(() => state.api.applyNullify(state.handle, state.selectedNullifyHandCardId, state.selectedNullifyTargetCardId));
  });
  bindCommand("#finish-round", () => callAndSnapshot(() => state.api.finishRound(state.handle)));
  bindCommand("#request-extra-round", () => {
    const forced = root.querySelector("#forced-unpaid-overtime")?.checked ?? false;
    return callAndSnapshot(() => state.api.requestExtraRound(state.handle, forced ? 1 : 0));
  });
  bindCommand("#finish-game", () => callAndSnapshot(() => state.api.finishGame(state.handle)));

  root.querySelector("#show-debug-card-ids")?.addEventListener("change", (event) => {
    state.showDebugCardIds = event.currentTarget.checked;
    render();
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

function renderGame() {
  const snapshot = state.snapshot;
  const revealableCards = revealableCardsInMemberOrder(snapshot);
  const nullifyHandCards = availableNullifyHandCards(snapshot);
  const nullifyTargetCards = nullifiableBoardCards(snapshot);
  const canApplyNullify =
    nullifyHandCards.some((card) => card.cardId === state.selectedNullifyHandCardId) &&
    nullifyTargetCards.some((card) => card.cardId === state.selectedNullifyTargetCardId);

  return `
    <section class="panel status">
      <h2>状態</h2>
      <div class="grid">
        <div>phase: <strong>${snapshot.phase}</strong></div>
        <div>judge: <strong>${snapshot.judgeResult}</strong></div>
        <div>round: ${snapshot.currentRound} / ${snapshot.maxRound}</div>
        <div>score: ${snapshot.score} / ${snapshot.targetScore}</div>
        <div>cost: ${snapshot.cost}${snapshot.costLimit == null ? "" : ` / ${snapshot.costLimit}`}</div>
        <div>deckSeed: ${snapshot.deckSeed ?? "-"}</div>
        <div>teamSize: ${snapshot.teamSize}</div>
        <div>globalExpectedScore: ${snapshot.globalExpectedScore}</div>
        <div>revealLimit: ${snapshot.revealLimit}</div>
      </div>
      ${snapshot.error ? `<p class="error">error: ${snapshot.error.code}${state.showDebugCardIds && snapshot.error.cardId ? ` cardId=${snapshot.error.cardId}` : ""}</p>` : ""}
    </section>

    <section class="panel commands">
      <h2>操作</h2>
      <button id="start-round" type="button">ラウンド開始</button>
      <button id="finish-round" type="button">ラウンド終了</button>
      <label><input id="forced-unpaid-overtime" type="checkbox" /> サビ残強制</label>
      <button id="request-extra-round" type="button">追加ラウンド要求</button>
      <button id="finish-game" type="button">ゲーム終了</button>
      <label><input id="show-debug-card-ids" type="checkbox" ${state.showDebugCardIds ? "checked" : ""} /> デバッグ: カードID表示</label>
    </section>

    <section class="panel">
      <h2>メンバー</h2>
      <div class="members">
        ${snapshot.members.map((member) => `<span class="member ${member.state}">列 ${member.column}: ${member.state}</span>`).join("")}
      </div>
    </section>

    <section class="panel">
      <h2>手札使用計画</h2>
      <div class="hand-plan">
        ${snapshot.handCards.map((card, index) => renderHandPlanCard(card.cardId, index)).join("") || "<p>未使用手札なし</p>"}
      </div>
      <button id="plan-hand" type="button">planHandUsage</button>
    </section>

    <section class="panel">
      <h2>カード公開</h2>
      <p class="hint">公開枚数の正当性は C++ コアが判定します。</p>
      <div class="cards">
        ${revealableCards.map((card) => `
          <label class="card">
            <input type="checkbox" data-reveal-card="${card.cardId}" ${state.selectedRevealCards.has(card.cardId) ? "checked" : ""} />
            row ${card.row ?? "-"} col ${card.column ?? "-"}${debugCardId(card.cardId)}
          </label>
        `).join("") || "<p>公開可能カードなし</p>"}
      </div>
      <button id="reveal-cards" type="button">revealCards (${state.selectedRevealCards.size} selected)</button>
    </section>

    <section class="panel">
      <h2>Nullify 対象選択</h2>
      <div class="nullify">
        <div>
          <h3>Nullify 手札</h3>
          ${nullifyHandCards.map((card) => `
            <label class="card">
              <input type="radio" name="nullify-hand" value="${card.cardId}" ${state.selectedNullifyHandCardId === card.cardId ? "checked" : ""} />
              Nullify 手札 ${card.slot + 1}${debugCardId(card.cardId)}${debugTargetCardId(card.targetCardId)}
            </label>
          `).join("") || "<p>Nullify 宣言なし</p>"}
        </div>
        <div>
          <h3>対象カード</h3>
          ${nullifyTargetCards.map((card) => `
            <label class="card">
              <input type="radio" name="nullify-target" value="${card.cardId}" ${state.selectedNullifyTargetCardId === card.cardId ? "checked" : ""} />
              row ${card.row} col ${card.column} ${card.kind ?? ""}${debugCardId(card.cardId)}
            </label>
          `).join("") || "<p>表向きカードなし</p>"}
        </div>
      </div>
      <button id="apply-nullify" type="button" ${canApplyNullify ? "" : "disabled"}>applyNullify</button>
    </section>

    <section class="panel board">
      <h2>場</h2>
      ${snapshot.board.map((row) => `
        <div class="board-row">
          <h3>Round ${row.row}</h3>
          <div class="cards">
            ${row.cards.map((card) => renderBoardCard(card)).join("") || "<span class=\"empty\">カードなし</span>"}
            ${snapshot.boardSideCards.filter((card) => card.row === row.row).map((card) => `
              <div class="card side">手札 ${card.slot + 1}${debugCardId(card.cardId)}<br />${card.usage}${debugTargetCardId(card.targetCardId, "<br />")}</div>
            `).join("")}
          </div>
        </div>
      `).join("") || "<p>ラウンド未開始</p>"}
    </section>
  `;
}

function renderHandPlanCard(cardId, index) {
  return `
    <div class="card">
      <label><input type="checkbox" name="hand-card" value="${cardId}" /> 手札 ${index + 1}${debugCardId(cardId)}</label>
      <select name="hand-usage-${cardId}">
        <option value="ScorePlus3">ScorePlus3</option>
        <option value="Nullify">Nullify</option>
      </select>
    </div>
  `;
}

function renderBoardCard(card) {
  const detail = card.runtimeState === "FaceUp"
    ? `${card.kind ?? ""}${card.scoreValue == null ? "" : ` ${card.scoreValue}`}`
    : card.runtimeState;
  return `<div class="card ${card.runtimeState}">col ${card.column}${debugCardId(card.cardId)}<br />${detail}</div>`;
}

function revealableCardsInMemberOrder(snapshot) {
  return [...snapshot.revealableCards].sort((left, right) =>
    ((left.row ?? Number.MAX_SAFE_INTEGER) - (right.row ?? Number.MAX_SAFE_INTEGER)) ||
    ((left.column ?? Number.MAX_SAFE_INTEGER) - (right.column ?? Number.MAX_SAFE_INTEGER)) ||
    (left.cardId - right.cardId)
  );
}

function debugCardId(cardId) {
  return state.showDebugCardIds ? ` <span class="debug">cardId ${cardId}</span>` : "";
}

function debugTargetCardId(cardId, prefix = " ") {
  return state.showDebugCardIds && cardId != null ? `${prefix}<span class="debug">target ${cardId}</span>` : "";
}

function bindCommand(selector, command) {
  root.querySelector(selector)?.addEventListener("click", () => updateSnapshot(command()));
}

function updateSnapshot(snapshot) {
  if (!snapshot) {
    return;
  }
  state.snapshot = snapshot;
  state.selectedRevealCards.clear();
  state.selectedNullifyHandCardId = null;
  state.selectedNullifyTargetCardId = null;
  render();
}

function callAndSnapshot(command) {
  if (!state.api || state.handle === 0) {
    return state.snapshot;
  }
  command();
  return readSnapshot();
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
  return [...root.querySelectorAll("input[name='hand-card']:checked")].map((input) => {
    const cardId = Number(input.value);
    const select = root.querySelector(`select[name='hand-usage-${cardId}']`);
    return { cardId, usage: select?.value ?? "ScorePlus3" };
  });
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

function numberValue(form, name) {
  return Number(form.get(name));
}

function optionalNumberValue(form, name) {
  const raw = form.get(name);
  return raw == null || raw === "" ? null : Number(raw);
}
