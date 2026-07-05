# QS=CD Core

QS=CD Core は、カードゲーム「QS=CD」の C++20 コアロジック実装です。コアは UI、標準入出力、ファイル入出力、通信、永続化に依存しない純粋ロジックとして実装し、テストコード、Web デモ、将来的な C ABI ラッパーから利用できる構造にしています。

QS=CD Core is the C++20 core logic implementation for the card game "QS=CD". The core is implemented as pure game logic without dependencies on UI, standard I/O, file I/O, networking, or persistence, so it can be used from tests, the Web demo, and a future C ABI wrapper.

ルールの正本は [`docs/rule.md`](docs/rule.md)、実装アーキテクチャの正本は [`docs/architecture.md`](docs/architecture.md) です。

The authoritative game rules are in [`docs/rule.md`](docs/rule.md), and the authoritative implementation architecture is in [`docs/architecture.md`](docs/architecture.md).

## QS=CD とは / What is QS=CD

QS=CD は、不確実性の高いプロジェクト運営をテーマにしたカードゲームです。プレイヤーはチームを率いながら、

- 見込みを設定する
- 限られた情報の中で判断する
- 手札を使ってリスクへ対応する
- 離脱や監査といったイベントへ対処する
- スコアとコストを管理することで目標達成を目指します。

QQS=CD is a card game about managing uncertainty in project execution. 
Players balance estimation, risk, score, and cost while guiding a team toward a target result.

---

## Design Philosophy

QS=CD は、以下の考え方を中心に設計されています。
- 完全な情報は得られない
- 見積りは予測ではなく約束である
- リソースは有限である
- 短期的な成果は長期的なリスクを生むことがある
- 将来への影響を考慮して意思決定する

そのため QS=CD は、単なる最適化パズルではなく、リスク管理と意思決定をテーマにしたゲームとして設計されています。

QS=CD is intentionally designed as a risk-management and decision-making game rather than a pure optimization puzzle.


## リポジトリ構成 / Repository layout

| Path | 日本語 | English |
| --- | --- | --- |
| `include/qscd/core` | 公開 C++ コアヘッダー | Public C++ core headers |
| `src/core` | コア実装 | Core implementation |
| `tests/core` | ネイティブコアテスト | Native core tests |
| `bindings/wasm` | ブラウザデモ用 Emscripten/WASM バインディング | Emscripten/WASM binding for the browser demo |
| `apps/web-demo` | Web デモのソースアセット | Web demo source assets |
| `site` | GitHub Pages 向け静的デモ出力 | Static GitHub Pages demo output |
| `scripts` | ビルドスクリプト | Build scripts |
| `docs` | ゲームルールとアーキテクチャ文書 | Game rules and architecture documents |

## 必要環境 / Requirements

- CMake 3.20 以上 / CMake 3.20 or newer
- C++20 対応コンパイラ / C++20 compiler
- Emscripten SDK（Web デモをビルドする場合のみ） / Emscripten SDK, only when building the Web demo

## コアのビルドとテスト / Build and test the core

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --target qscd_core_tests
ctest --test-dir build --output-on-failure
```

## Web デモのビルド / Build the Web demo

ブラウザデモは、C++ コアを WASM にコンパイルして利用する静的サイトです。

The browser demo is a static site backed by the C++ core compiled to WASM.

```sh
scripts/build-web-demo.sh
```

このコマンドは以下を行います。

This command:

1. `bindings/wasm` を Emscripten でビルドします。 / Builds `bindings/wasm` with Emscripten.
2. `apps/web-demo/wasm` の WASM 成果物を更新します。 / Updates the WASM artifacts in `apps/web-demo/wasm`.
3. 公開用の静的サイトを `site/` に再生成します。 / Recreates the deployable static site in `site/`.

ローカル確認:

To check it locally:

```sh
cd site
python3 -m http.server
```

表示されたローカル URL をブラウザで開いてください。

Then open the shown local URL in a browser.

---

## ステータス / Status

- コアロジック: 完了
- テスト: 完了
- 継続ゲーム: 完了
- WebAssembly バインディング: 完了
- ブラウザデモ: 公開中
- 今後の予定: デモの改善および UI のブラッシュアップ

- Core logic: Complete
- Tests: Complete
- Continuation game support: Complete
- WebAssembly bindings: Complete
- Browser demo: Available
- Planned work: Demo improvements and UI polish

---

## ライセンス / License

MIT License