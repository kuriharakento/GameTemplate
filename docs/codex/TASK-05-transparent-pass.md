# TASK-05: 半透明パスの分離とソート

- 対応: `SEQUENCER_PLAN.md` 4.4 / Phase 5
- 作業ブランチ: `codex/phase5-transparent`（親・engine の両方に作成済み）
- 前提: `docs/codex/ONBOARDING.md` を先に読むこと

---

## 1. なぜやるか

現在のフォワード描画は以下の固定順で、**半透明のソートが存在しない。**

```
Forward (Draw3D + デバッグライン) → Skybox → Particle
```

パーティクルは常に最後に描かれ、深度書き込みの扱いも不透明物と同じままである。
このままビーム・煙・ガラスが同居する演出を作ると、
**手前の煙が奥のビームを消す／半透明同士が重なると描画順で色が変わる**という形で破綻する。

Phase 8 で大気フォグとコーンメッシュのビームを入れる予定であり、
**それらは全て半透明である。** 入れる前にここを整理しておかないと、
破綻した状態を前提にパラメータを詰めることになり、後で全部やり直しになる。

---

## 2. 目標とする描画順

```
GBuffer → Lighting
  → ForwardOpaque   （不透明。深度書き込み ON）
  → Skybox          （不透明の後、半透明の前）
  → Transparent     （半透明。深度書き込み OFF、奥から手前へソート）
  → SceneColorResolve
```

Skybox が不透明と半透明の間に入るのは、
- 不透明を先に描いておけば Skybox が深度テストで大部分棄却されて安く済む
- 半透明は Skybox の上に乗る必要がある

の両方を満たすため。

---

## 3. やること

### 3.1 半透明かどうかの区分を `IRenderable3d` に持たせる

現状 `IRenderable3d` は `RenderingType { Deferred, Forward }` しか持たず、
**不透明／半透明の区別が無い。** これを追加する。

```cpp
/**
 * @brief 描画キュー。半透明はソートが必要なため不透明と分けて描く
 */
enum class RenderQueue
{
    Opaque,      //!< 不透明。深度書き込み ON、描画順は問わない
    Transparent, //!< 半透明。深度書き込み OFF、奥から手前へ描く
};
```

- `IRenderable3d` に `GetRenderQueue()` / `SetRenderQueue()` を追加
- `Object3d` と `SkinnedObject3d` に実装。既定は `Opaque`
- **既定が `Opaque` なので、既存のオブジェクトの見た目は変わらないこと**

`BlendMode`（`math/BlendMode.h`）は既に存在するが、これは合成方法の指定であって
描画順の話ではない。混同しないこと。半透明パスで使うブレンドの既定は
`BlendMode::Alpha` 相当とする。

### 3.2 半透明用の PSO を `Object3dCommon` に追加

現在の PSO は深度書き込み ON・ブレンド無しの1本のみ。半透明用にもう1本作る。

- `DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO`（深度書き込み OFF）
- `DepthEnable = TRUE`（深度テストは行う。不透明物の裏に回った半透明は隠れる）
- アルファブレンドを有効化
- **`RTVFormats[0]` は `kSceneColorFormat` を使うこと**（直書き禁止。ONBOARDING 4.2）

`Object3dCommon::CommonRenderingSetting()` に相当する、
半透明用のパイプラインを設定する関数を用意する。

### 3.3 パスを分割する

`engine/graphics/pipeline/StandardRenderPasses.{h,cpp}` を変更する。

- 既存の `ForwardPass` を `ForwardOpaquePass` に改名（名前文字列も `"ForwardOpaque"` に）
  - デバッグライン（`lightManager->DrawDebugLines()`）と `LineManager::RenderLines()` は
    こちらに残す
- `TransparentPass` を新設（名前文字列 `"Transparent"`）
- `ParticlePass` は**削除し、パーティクルの描画は `TransparentPass` の中で行う**
  - パーティクルは本質的に半透明であり、独立したパスとして最後に置くのが
    そもそもの問題の原因だったため
- `BuildStandardRenderPipeline` / `BuildSceneOnlyRenderPipeline` の並びを
  「2. 目標とする描画順」に合わせる

### 3.4 半透明オブジェクトをソートする

`GameObjectManager` に半透明の描画経路を追加する。

- `Draw3D()` は `RenderQueue::Opaque` のものだけを描くようにする
- `DrawTransparent(CameraManager*)` を新設し、以下を行う
  1. `RenderQueue::Transparent` かつアクティブ、かつ現在のレイヤーマスクに合致する
     オブジェクトを集める
  2. **カメラからの距離で降順（遠い順）にソートする**
  3. その順に描く
- `SceneManager` / `IScene` にも `DrawTransparent()` を通す経路を足す
  （`Draw3D` / `DrawGBuffer` と同じ形）

距離はオブジェクトのワールド座標とカメラ座標の**距離の2乗**で比較すること
（平方根は順序に影響しないので計算しない）。

既存のレイヤーマスクによる絞り込み（`IsVisibleInLayerMask`）を
**必ず同じように適用すること。** ここを忘れるとサブビューで半透明だけが漏れて描かれる。

---

## 4. やらないこと

スコープを勝手に広げないこと。以下は**このタスクではやらない。**

- 半透明のオブジェクト単位ではなく三角形単位のソート（過剰）
- OIT（Order Independent Transparency）
- 深度プリパス
- パーティクル同士のソート（パーティクルシステム内部の話であり別問題）
- ブレンドモードごとの PSO の作り分け（必要になったら別タスクで）

---

## 5. 受け入れ条件

1. `RenderQueue::Opaque` のオブジェクトの見た目が**従来と変わらない**
   （既定が Opaque なので、何も設定しなければ現状維持になること）
2. `RenderQueue::Transparent` に設定したオブジェクトが、
   カメラから遠い順に描かれる
3. 半透明オブジェクトが深度バッファを書き換えない
   （半透明の後ろにある別の半透明が消えない）
4. パーティクルが `Transparent` パスの中で描かれている
5. Skybox が不透明の後・半透明の前に描かれている
6. `Render Pipeline` デバッグUIのパス一覧が
   `... ForwardOpaque / Skybox / Transparent / SceneColorResolve ...` になっている
7. サブビュー（`BuildSceneOnlyRenderPipeline`）でも同じ並びになっている
8. ONBOARDING の「タスク完了の条件」を全て満たしている

---

## 6. 動作確認の手引き

見た目の検証には半透明オブジェクトが必要になる。
**テスト用のオブジェクトをアプリ側のシーンに恒久的に足さないこと。**
確認したら消すか、確認方法を報告に書いて人間に委ねること。

確認したいこと:
- 半透明を2枚重ねてカメラを回したとき、前後関係が入れ替わっても正しく見えるか
- 半透明の後ろにある不透明が透けて見えるか
- 従来の不透明オブジェクトの見た目が変わっていないか

**絵の最終確認は人間が行う。** 自動キャプチャでは確認できない（ONBOARDING 3 参照）。

---

## 7. 報告してほしいこと

- 変更したファイルと、その理由
- `IRenderable3d` にインターフェースを足したことで影響を受けた実装クラス
- 受け入れ条件のうち、**自分で確認できたものと、人間の確認が必要なもの**の区別
- 途中で仕様に無理があると判断した点があれば、その内容と代替案
