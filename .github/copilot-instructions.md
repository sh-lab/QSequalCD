# GitHub Copilot Instructions for QS=CD Core

このリポジトリは、カードゲーム「QS=CD」のC++20コアロジックを実装するためのものです。

## 参照ドキュメント

- ゲームルールの正本は `docs/rule.md`
- アーキテクチャの正本は `docs/architecture.md`

Copilotは必ずこの2つのドキュメントを優先してください。

## 禁止事項

- `docs/rule.md` にないルールを追加しない
- `docs/architecture.md` に反する構造を作らない
- UI、SwiftUI、Web UI、標準入出力、ファイル入出力を実装しない
- Qを数値状態として `GameState` に保持しない
- カード枚数、カード効果、勝利条件、敗北条件を勝手に変更しない
- ゲームバランス調整を勝手に行わない

## 実装方針

- C++20で実装する
- コアは純粋ロジックにする
- 将来的にC ABIラッパーを提供できる構造にする
- 外部公開APIにはC++ STL型を直接出さない
- テストしやすい設計にする