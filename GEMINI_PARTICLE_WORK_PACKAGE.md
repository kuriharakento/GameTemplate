# Gemini向け GPU Particle 作業分担書

## 1. 分担方針

Geminiには、既存挙動を変更せず、成果物をレビュー・比較しやすい「Phase 0: 観測性と安全網」を担当してもらう。

Pure GPU runtimeの中核は担当させない。特にAlive/Dead list、GPU counter、resource barrier、ExecuteIndirect、frame fence、descriptor retirement、SubEmitter/Ribbon GPU化はCodex側で実装する。これらは設計判断が相互依存し、局所的に正しそうな変更でもGPU hang、use-after-free、silent corruptionを起こし得るためである。

### 役割一覧

| 領域 | 担当 | 理由 |
|---|---|---|
| 現行moduleのcapability棚卸し | Gemini | 読み取り中心で、成果をコードと照合できる |
| Particle ABI compile-time guard | Gemini | 期待offsetを明示でき、コンパイラで検証できる |
| benchmark effect/scene | Gemini | 既存APIだけで作成でき、見た目と件数を確認できる |
| CPU profiler scopeと転送量counter | Gemini | 現行挙動を変えずbaselineを得られる |
| lifecycle回帰テスト仕様 | Gemini | Pure GPU移行前の期待動作を固定できる |
| GPU timestamp query基盤 | Codex | fence/readback/frame context設計と密接に関係する |
| GpuEmitterRuntime | Codex | Pure GPUの中核 |
| Alive/Dead/Counter protocol | Codex | 並行性と正当性の中核 |
| ExecuteIndirect/CommandSignature | Codex | renderer/root signature/resource stateと密接 |
| descriptor pool/deferred release | Codex | エンジン全体のresource lifetimeに影響 |
| module registry/compiler/asset migration | Codex | runtime ABIとasset互換性を決定する |
| editorの新runtime対応 | 共同、Codex設計後にGemini | API確定前に作ると手戻りになる |

## 2. Geminiの作業ルール

以下をGeminiへの依頼文にそのまま含めること。

1. 作業開始時に `git status --short` を記録する。既存変更はユーザーの所有物として変更・削除・整形しない。
2. 専用branchまたは専用worktreeを使用する。1タスク1commitとし、commitを混ぜない。
3. 最初に `PARTICLE_REFACTORING_PLAN.md` と本書を全文読む。
4. ファイルを変更する前に、対象APIの全call siteを `rg` で列挙する。
5. runtimeの挙動、moduleの計算式、JSON形式、shader、root signature、resource barrierを変更しない。
6. 不明点を推測でAPI化しない。既存の仕組みが見つからない場合は、実装を止めて調査結果と候補を報告する。
7. 大規模rename、全ファイルformat、include順の一括変更をしない。
8. warningを無効化しない。失敗するassert/testを削除・緩和して通さない。
9. 各commitに、変更ファイル、検証コマンド、検証結果、未解決事項を記載する。
10. `engine` が親repositoryからsubmoduleとして見える場合、親と`engine`内の両方のstatus/diffを報告する。

## 3. Work Package G1: Module Capability Inventory

### 目的

現存する全moduleについて、CPU/GPU対応状況と実行stageを、コードから再生成・監査できる形で固定する。推測で「対応」と判定してはいけない。

### 調査対象

- `engine/effects/particle/module/IModule.h`
- `engine/effects/particle/module/spawn/`
- `engine/effects/particle/module/update/`
- `engine/effects/particle/gpu/GPUSimulator.cpp`
- `engine/Resources/shaders/Particle*.hlsl`
- `engine/effects/particle/editor/ParticleEditor.cpp`
- `engine/effects/particle/serialization/ParticleEffectSerializer.cpp`

### 実装手順

1. `IModule`派生classをすべて列挙する。
2. 各classについて次を記録する。
   - C++ class名
   - `GetName()`の永続名
   - `ModulePhase`
   - priority
   - serializer load/save対応
   - editor追加UI対応
   - CPU `Execute()`実装の有無
   - `IsGPUSupported()`の値
   - `DispatchGPU()`の有無
   - main `ParticleCompute.hlsl`内での処理有無
   - 個別compute shader/PSOの有無
   - Sprite/Mesh/Ribbonとの関係
   - 既知のGPU semantic差
3. `IsGPUSupported()==false`でもmain shaderのflag分岐で処理されるmoduleがあるため、単一条件だけでGPU対応と判断しない。
4. 結果を `engine/effects/particle/docs/PARTICLE_MODULE_CAPABILITY_MATRIX.md` に表として作成する。
5. コード行への参照を付ける。ただし行番号だけでなくsymbol/file名も記載する。
6. 次の分類を必ず使用する。
   - `CPU_ONLY`
   - `HYBRID_MAIN_CS`
   - `HYBRID_SEPARATE_CS`
   - `RENDERER_CPU_DEPENDENT`
   - `UNKNOWN_NEEDS_TEST`
7. 非対応moduleがGPUモードでどうなるかを明記する。「おそらく動く」は禁止する。

### 成果物

- capability matrix 1ファイル
- module総数と分類別件数
- serializer/editor/runtimeの不一致一覧
- 未確認項目一覧

### 完了条件

- `rg "class .*Module" engine/effects/particle/module` の全classが表に存在する。
- `GetName()`の重複、serializer片方向のみ、editorのみ存在、GPU flagとDispatch不一致を明示できる。
- runtime codeは変更しない。

## 4. Work Package G2: Particle ABI Guards

### 目的

C++ `Particle` の128-byteレイアウトが意図せず変わった場合、コンパイル時に停止させる。

### 期待レイアウト

| Field | Offset |
|---|---:|
| `position` | 0 |
| `velocity` | 16 |
| `scale` | 32 |
| `rotation` | 48 |
| `color` | 64 |
| `initialColor` | 80 |
| `age` | 96 |
| `lifetime` | 100 |
| `ribbonWidth` | 104 |
| `flags` | 108 |
| `id` | 112 |
| `ribbonId` | 116 |
| `spriteIndex` | 120 |
| `pad3` | 124 |
| total size | 128 |
| alignment | 16 |

### 実装手順

1. `Particle.h`に必要最小限の`<cstddef>`と`<type_traits>`を追加する。
2. `Particle`定義直後に以下の種類のguardを追加する。
   - `std::is_standard_layout_v<Particle>`
   - `sizeof(Particle) == 128`
   - `alignof(Particle) == 16`
   - 上表の各`offsetof`
3. `ParticleGPU`についても、C++側shader includeと照合してsize/offset表を先に報告する。値を確認できない場合は推測でassertを追加しない。
4. `ParticleCompute.hlsl`、`ParticleConvert.CS.hlsl`、個別module shaderの`Particle`定義が同じfield順か監査し、差分を文書化する。
5. このタスクではgenerated header/codegenを導入しない。既存ABIを固定するだけに留める。

### 禁止事項

- `Particle`のfield追加、削除、並べ替え
- HLSL構造体の修正
- pack pragmaによる強制
- assertを通すためのpadding変更

### 完了条件

- Debug/Releaseのengine buildが成功する。
- 期待offsetを1つ仮に変更するとcompileが失敗することを確認し、その仮変更は戻す。
- `git diff`にABI guard以外の振る舞い変更がない。

## 5. Work Package G3: Baseline Benchmark Assets

### 目的

旧CPU pathと旧hybrid GPU pathを同一条件で比較できる再現可能なbenchmarkを作る。

### シナリオ

最低限、次のケースを用意する。

| ID | Emitters | Target alive | Renderer | Modules | 用途 |
|---|---:|---:|---|---|---|
| B01 | 1 | 1,000 | Sprite | SpawnRate/Lifetime/Velocity | 小規模overhead |
| B02 | 1 | 10,000 | Sprite | B01 + Gravity/Drag | 標準負荷 |
| B03 | 1 | 65,536 | Sprite | B02 | 現行default上限 |
| B04 | 10 | 合計65,536 | Sprite | B02 | emitter dispatch overhead |
| B05 | 100 | 合計10,000 | Sprite | 最小module | descriptor/manager overhead |
| B06 | 1 | 10,000 | Mesh | B02 | indexed instancing |
| B07 | 1 | 安全な既定値 | Ribbon | ribbon関連 | CPU renderer baseline |

250kは現行capacity/APIで安全に設定できることを確認できた場合だけ追加する。assertやallocation失敗が起きる構成を既定benchmarkにしない。

### 実装手順

1. 既存 `application/scene/debug/ParticleTestScene.*` の起動方法と責務を調査する。
2. 既存sceneを壊さず、benchmark presetを選択できる最小追加にする。別sceneが必要なら理由を先に報告する。
3. random seed、camera transform、texture、blend、simulation mode、duration、warm-up秒数を固定する。
4. CPUとGPU-hybridで同一effect definitionを使う。機能非対応moduleは両者から外すか、差分として記録する。
5. benchmark中にeditorの粒子markerを無効化する。marker走査をbaselineに混ぜない。
6. VSync、resolution、build typeを画面またはログに表示する。
7. 各presetに期待target countとoverflow有無を表示する。

### 成果物

- benchmark scene/preset code
- 操作手順 `engine/effects/particle/docs/PARTICLE_BENCHMARK.md`
- 各presetの設定表
- 実行できなかったcaseと理由

### 完了条件

- 同じpresetを再起動して同じ設定になる。
- CPU/GPU-hybridを明示切替できる。
- benchmark追加前の通常ParticleTestSceneの操作を壊さない。
- Release buildで最低B01〜B06が起動し、D3D12 debug errorがない。

## 6. Work Package G4: CPU Baseline Instrumentation

### 目的

GPU timestamp基盤へ踏み込まず、CPU側の処理時間と既知の転送量を測定できるようにする。

### 測定項目

- `ParticleManager::Update`
- emitter CPU update
- hybrid GPU update全体
- `ReadbackParticles`のCPU map/memcpy時間
- `RemoveDeadParticles`
- `UpdateGPUSpawns`
- `UploadParticles`のCPU map/memcpy/command recording時間
- renderer `Update`
- renderer `Draw`のCPU command recording時間
- frameごとのupload/readback要求bytes
- emitter数、CPU vector size、last-known GPU count

### 実装手順

1. 既存profiler/timer/debug UIの有無を検索する。
2. 既存基盤がある場合だけそれを使う。似たprofilerを新設しない。
3. 基盤がない場合、実装に入らず次を報告する。
   - 利用可能なTimer/DebugUI API
   - 最小instrumentation案
   - thread safetyと集計周期
4. scopeはRAIIにし、早期returnでも閉じるようにする。
5. sampling表示はrolling average、min/max、sample countを持つ。毎フレーム文字列allocationやログ出力を行わない。
6. byte counterは実際の`copyCount * sizeof(Particle)`を記録する。GPU実行時間と称してCPU command recording時間を表示しない。
7. instrumentation自体のON/OFFを用意し、Release既定では低負荷または無効にする。

### 禁止事項

- timestamp query/readback heapの独自実装
- fence waitの追加
- `ExecuteAndWait()`の追加
- command listのClose/Execute
- GPU同期を使った正確な時間測定

### 完了条件

- 表示名に`CPU`または`command recording`が明記され、GPU時間と誤認しない。
- instrumentation OFFで主要pathの分岐・出力が増えない、または計測可能な最小overheadである。
- B01〜B06の結果を同じフォーマットで保存できる。

## 7. Work Package G5: Lifecycle Regression Specification

### 目的

Pure GPU化で破壊しやすい既存のライフサイクル意味論を、実装前にテストケースとして固定する。

### 必須ケース

- Play直後、Pause/Resume、Stop + Complete、Stop + Kill、Reset、Restart
- duration 0、Once/Infinite/Multiple、loop count境界
- start delay中の既存粒子更新
- SpawnRate、単発/反復SpawnBurstの`IsComplete`
- 最後の粒子死亡とeffect停止のタイミング
- `ClearParticles`、simulation mode切替、max particles変更
- follow target移動、follow emitter、Local/World
- deltaTime 0、通常値、大きなspike、time scale 0
- effect削除とrenderer差替え

### 実装手順

1. `ParticleEmitter.cpp`、`ParticleEffect.cpp`、`ParticleManager.cpp`のcall graphをまとめる。
2. 各ケースを Given/When/Then 形式で文書化する。
3. 現行挙動が明らかなbugまたは曖昧な場合、「期待仕様」を勝手に決めず `DECISION_REQUIRED` と記載する。
4. 自動test frameworkが既にある場合のみtest化する。新しいframework導入や外部dependency追加は行わない。
5. GPU移行後に許容する差（例: completion通知が数frame遅延）を別欄にする。

### 成果物

- `engine/effects/particle/docs/PARTICLE_LIFECYCLE_SPEC.md`
- call-site一覧
- `DECISION_REQUIRED`一覧
- 可能なら既存framework上のCPU regression tests

### 完了条件

- `GetParticles().empty()/size()`を使う全call siteが記録されている。
- `IsEmitting`とalive particleの違いがケースに含まれる。
- 現行挙動と新仕様案を混同していない。

## 8. Geminiに渡さない作業

以下のファイル・領域は、明示的な再割当がない限り変更禁止とする。

- `engine/effects/particle/gpu/GPUSimulator.*`
- `engine/effects/particle/gpu/GPUParticlePipeline.*`
- `engine/Resources/shaders/Particle*.hlsl`
- `engine/base/DirectXCommon.*`
- `engine/manager/system/SrvManager.*`
- renderer root signature/PSO
- serializerの保存形式
- `SimulationMode` enum
- 新 `GpuEmitterRuntime` とそのresource classes

例外はG1の読み取り調査とG2の`Particle.h`へのassert追加だけである。

## 9. 実行順と依存関係

1. **G1 Capability Inventory** — 最初に実施。コード変更なし。
2. **G5 Lifecycle Specification** — G1と並行可能。コード変更なしを基本とする。
3. **G2 ABI Guards** — 独立commit。
4. **G3 Benchmark Assets** — G1の安全なmodule集合を使う。
5. **G4 CPU Instrumentation** — benchmarkへ組み込み、baselineを取得する。

G1/G5のレビューが終わる前にG3/G4をmainへ統合しない。棚卸しの誤りがbenchmark条件へ伝播するためである。

## 10. Geminiの最終報告テンプレート

```text
Task ID:
Branch / commit:

Changed files:
- ...

What was implemented:
- ...

Commands run:
- ...

Results:
- Build Debug:
- Build Release:
- Tests:
- Manual benchmark:
- D3D12 validation:

Pre-existing changes preserved:
- ...

Known limitations / DECISION_REQUIRED:
- ...

Things deliberately not changed:
- ...
```

「成功しました」だけの報告は受け付けない。実行したコマンド、exit code、結果、未実施項目を分けて報告させる。

## 11. Codex側の担当

GeminiのG1〜G5をレビューした後、Codex側で以下を進める。

1. module capability/lifecycle decisionの確定。
2. `GpuEmitterRuntime` resource modelとcounter protocol。
3. Spawn/Update/Compact/Finalize compute passes。
4. Sprite render build、CommandSignature、ExecuteIndirect。
5. completion record、generation handle、async debug snapshot。
6. descriptor budgetに基づくpool/arena選択。
7. fence付きdeferred release。
8. module registry/compilerとasset migration。
9. GPU events、SubEmitter、Mesh、Ribbon。
10. frame contextリング化と必要に応じたasync compute検討。

Geminiの成果はそのまま無条件に採用せず、各commitを個別reviewし、capability表はコード検索、ABIはcompiler、benchmarkはcapture、計測値は測定定義と照合してから統合する。
