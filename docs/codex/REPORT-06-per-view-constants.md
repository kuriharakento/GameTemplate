# TASK-06 実装報告

実装の大部分は Codex が行い、未コミットの途中の状態を Claude が引き継いで仕上げた。

## 変更内容

| ファイル（engine 相対） | 内容 |
|---|---|
| `graphics/FrameConstantAllocator.{h,cpp}` | 新規。UPLOAD ヒープのバッファを常時 Map し、256 バイト境界で切り出して CPU 書き込み先と GPU アドレスを返す。`BeginFrame()` で先頭に戻す |
| `framework/Framework.{h,cpp}` | 割り当て器を所有。`ExecuteRenderPipeline()` の冒頭で `BeginFrame()` |
| `graphics/3d/Object3dCommon.h` | 割り当て器と `CameraManager` を保持 |
| `graphics/3d/Object3d.{h,cpp}` | `Draw` / `DrawGBuffer` / `DrawShadowOnly` で、描画のたびに WVP とカメラを新しい領域へ書いてバインドする |
| `graphics/3d/SkinnedObject3d.{h,cpp}` | 同上 |
| `gameobject/manager/GameObjectManager.cpp` | `DrawTransparent()` 内の描画時更新（`UpdateTransform` / `Update(0.0f, ...)`）を削除 |

## GPU がまだ読んでいる領域を上書きしないか（仕様 2.2）

**上書きしない。** `DirectXCommon::PostDraw()`（`base/DirectXCommon.cpp` 180 行付近）は、
Present の後に `Signal` → `SetEventOnCompletion` → `WaitForSingleObject` で
**毎フレーム GPU の完了を待っている。** 次のフレームの `BeginFrame()` が走る時点で、
前フレームのコマンドは必ず実行し終わっているため、バッファ1本を毎フレーム先頭に戻す方式で安全。

将来フレームを重ねて実行する（完了を待たない）ように変える場合は、
バッファを同時実行フレーム数ぶんに区切る必要がある。

## 採用した方式と容量

- バッファ1本、毎フレーム先頭に戻す
- 容量: `2048 × 2 × 256` バイト（= 1 MiB）。1回の描画で WVP とカメラの2領域を使うので、
  1フレームに約 2048 回の描画に耐える
- サブビューを増やすと描画回数はビュー数に比例して増える。
  中継映像などを入れる段階で容量を見直すこと
- 容量を超えた場合は assert せず、そのオブジェクトの描画をスキップする。
  ログはフレームごとに1回だけ出す（Claude が修正。当初は描画のたびに出てログが埋まる実装だった）

## 描画時の行列の作り方

描画のたびに `World × アクティブカメラの ViewProjection` で WVP を作り直す。
サブビューの描画中は `CameraManager::SetRenderCameraOverride()` でアクティブカメラが
差し替わっているため、自動的にそのビューのカメラの行列になる。

World 行列は `GameObject::Update()` が毎フレーム `renderable3d_->Update()` と
`UpdateWorldMatrix()` で更新しているため、描画時の更新は不要。
これにより `DrawTransparent()` の描画時更新を取り除けた。

## 確認したこと

- Debug/x64 のビルド成功（新規のコンパイラ警告なし）
- リポジトリルートを作業ディレクトリにして 25 秒動作、落ちない（D3D12 ERROR でブレークする設定）
- 古い定数バッファを使ったまま残っている描画経路を検索し、3D オブジェクトには残っていないことを確認

## 人間の確認が必要なもの

- 本編の見た目が従来と変わらないこと（不透明・半透明・スキニング・シャドウ）

## 残課題

- ~~**Skybox** も WVP の定数バッファを1つしか持たず、描画時の行列が
  ビューごとに正しくならない~~ → TASK-07（`claude/task07-skybox-constants`）で解決。
  `Skybox::Draw(Camera*, FrameConstantAllocator*)` を追加し、`SkyboxPass` から
  差し替え後のアクティブカメラと割り当て器を渡すようにした。
  割り当て器は `RenderPassContext::frameConstantAllocator` として全パスから使える
- Sprite も同様だが、2D はサブビューで描かないので対応不要
- サブビューを実際に使う機能はまだ無いため、「2つのビューでそれぞれ正しい行列で描かれる」ことは
  コード上の確認のみ。実動作の確認はカメラプレビューを作るタスクで行う
