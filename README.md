# QS=CD Core

QS=CD Core は、カードゲーム「QS=CD」の C++20 コアロジック実装です。コアは UI、標準入出力、ファイル入出力、通信、永続化に依存しない純粋ロジックとして実装し、テストコード、Web デモ、将来的な C ABI ラッパーから利用できる構造にしています。

QS=CD Core is the C++20 core logic implementation for the card game "QS=CD". The core is implemented as pure game logic without dependencies on UI, standard I/O, file I/O, networking, or persistence, so it can be used from tests, the Web demo, and a future C ABI wrapper.

ルールの正本は [`docs/rule.md`](docs/rule.md)、設計思想は [`docs/design-philosophy.md`](docs/design-philosophy.md)、実装アーキテクチャの正本は [`docs/architecture.md`](docs/architecture.md) です。

The authoritative game rules are in [`docs/rule.md`](docs/rule.md), the design philosophy is in [`docs/design-philosophy.md`](docs/design-philosophy.md), and the authoritative implementation architecture is in [`docs/architecture.md`](docs/architecture.md).

## QS=CD とは / What is QS=CD

QS=CD は、期間型のソフトウェア開発プロジェクトを題材にしたソロカードゲームです。プレイヤーは、与えられた目標、コスト、期限のもとでチームを率いるPMとして、プロジェクトが破綻する前の完了を目指します。

プレイヤーはチームを率いながら、

- 見込みを設定する
- 限られた情報の中で判断する
- 手札を使ってリスクへ対応する
- 離脱や監査といったイベントへ対処する
- スコアとコストを管理することで目標達成を目指します。

QS=CD is a solo card game inspired by fixed-term software development projects. Players act as project managers working within given targets, costs, and deadlines, and try to complete the project before it collapses.

---

## Design Philosophy

QS=CD は、次の考え方を中心に設計されています。

- Q（品質）は固定値1であり、交渉や削減の対象にしない
- C（コスト）とD（デリバリー）は、S（スコープ）を成立させる構成要素である
- 本質的な調整対象はSだが、ゲーム内のPMには与えられたSを変更する権限がない
- 人数、期間、要求水準が増えるほど不確実性が高まり、見込みどおりに進まない範囲が増える
- PMとメンバーが制御できる範囲には限界がある
- プロジェクトには、破綻したものと、破綻する前に必要な成果を満たして終了したものがある

`QS = CD`はスコア計算式ではなく、本作の根底にある思想を表します。詳しくは[設計思想](docs/design-philosophy.md)を参照してください。

`QS = CD` is a design principle rather than a score calculation formula. See the [design philosophy](docs/design-philosophy.md) for details.


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

## Web Demo

ブラウザで遊べるデモはこちらです。

A playable browser demo is available here.

https://sh-lab.github.io/QSequalCD/

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

- ソースコードおよび付随文書（以下のCC BY 4.0対象文書を除く）：[MIT License](LICENSE)
- [`docs/rule.md`](docs/rule.md)および[`docs/design-philosophy.md`](docs/design-philosophy.md)：[Creative Commons Attribution 4.0 International（CC BY 4.0）](https://creativecommons.org/licenses/by/4.0/deed.ja)

- Source code and accompanying documentation (excluding the CC BY 4.0 documents below): [MIT License](LICENSE)
- [`docs/rule.md`](docs/rule.md) and [`docs/design-philosophy.md`](docs/design-philosophy.md): [Creative Commons Attribution 4.0 International (CC BY 4.0)](https://creativecommons.org/licenses/by/4.0/)
