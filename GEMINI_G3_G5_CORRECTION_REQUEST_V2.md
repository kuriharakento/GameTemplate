# Gemini向け G3〜G5 第2次修正依頼書

## 1. 今回の判定

Debug/Release x64ビルドは成功した。G1とG2は受領可能な水準まで改善されたため、今回の修正対象から外す。

| Work Package | 判定 | 今回の対応 |
|---|---|---|
| G1 Module Capability Inventory | 受領 | 変更しない |
| G2 Particle ABI Guards | 受領 | 変更しない |
| G3 Baseline Benchmark Assets | 不合格 | 起動不能候補、descriptor budget、実行検証を修正 |
| G4 CPU Baseline Instrumentation | 要修正 | scope、CSV schema、counter集計、APIを修正 |
| G5 Lifecycle Regression Specification | 不合格 | 実装と異なる記述を修正 |

今回、G1/G2を「ついでに改善」しないこと。G3/G4/G5の修正を別commitに分けること。

## 2. 最優先: Benchmarkを実際に起動可能にする

### V2-001: Texture存在確認とTextureManagerのpath解決を一致させる

重要度: **Critical**

対象:

- `application/scene/debug/ParticleTestScene.cpp`
- `application/Resources/LoadGameResources.cpp`
- TextureManager/PathManagerの既存path解決処理（調査のみ。無断変更禁止）

現状:

```cpp
std::filesystem::exists("./Resources/circle2.png")
```

でbenchmark開始可否を判定している。しかしrepository上の実ファイルは次にある。

```text
application/Resources/textures/circle2.png
```

既存の`TextureManager::LoadTexture("./Resources/circle2.png")`は、filesystemの直接判定とは異なる検索規則を利用している可能性がある。このため、TextureManagerではロード可能でもbenchmark UIは`MISSING`と判断してStartを無効にする。

修正手順:

1. `TextureManager::LoadTexture`とPathManagerの検索規則を調査する。
2. 実行時のworking directoryをDebug/Releaseそれぞれ確認する。
3. benchmarkの事前確認はTextureManager/PathManagerと同じ解決処理を使う。
4. 同じpath解決処理を再実装しない。
5. 既存APIで「解決済みpath」や「ロード済みか」を取得できない場合、UI側のfilesystem事前判定を削除し、renderer初期化結果またはTextureManagerの既存error処理を使う案を報告する。
6. Debug/Release実行環境で実際にStartボタンを押し、B01が開始することを確認する。

禁止事項:

- repository上で偶然存在する絶対pathをハードコードする。
- `C:/Users/...`を使用する。
- textureを重複コピーして問題を隠す。
- TextureManager全体の公開APIを大規模変更する。

完了条件:

- Debug/Releaseの両方でtextureが正しく解決される。
- Startボタンが誤って無効にならない。
- texture不存在時はクラッシュせず、実際の解決失敗を表示する。

## 3. G3 Benchmark修正

### V2-101: B05 GPUのdescriptor budget超過を防ぐ

重要度: **Critical**

現状:

- `SrvManager::kMaxSRVCount = 512`
- GPU emitter 1個あたり、現行`GPUSimulator`は4 descriptorを使用する。
- `SpriteRenderer`は1 descriptorを使用する。
- B05は100 emittersである。
- Particle以外のtexture/renderer/subsystemも同じheapを使用する。

概算でParticleだけでも約500 descriptorとなり、B05 GPUは安全余裕がなく、assertまたはheap枯渇の可能性が高い。

修正手順:

1. benchmark開始前の現在descriptor使用数を取得できる既存APIがあるか調査する。
2. presetごとの追加descriptor概算を表にする。
3. `current + required <= kMaxSRVCount`を満たさないpreset/modeを実行させない。
4. UIに次を表示する。
   - 現在使用数
   - 推定追加数
   - 上限
   - 実行可能/不可
5. 既存SrvManagerに安全なremaining count取得APIがない場合、読み取り専用の最小API追加案を提示する。Allocate/Freeの設計変更はしない。
6. B05は次のいずれかにする。
   - CPU専用presetとして明示する。
   - GPUでは安全なemitter数へ縮小し、別IDまたは別設定として記録する。
   - descriptor pool改修後まで`SKIPPED_DESCRIPTOR_BUDGET`とする。
7. 「B05 GPU成功」と偽って報告しない。

完了条件:

- descriptor不足時にAllocate assertへ到達しない。
- UIとbenchmark文書にB05の制限が明記される。
- 実行した構成とskipした構成が結果に残る。

### V2-102: benchmark対象Effectのhandleを明示する

現状の`benchmarkEffectPtr_`は、`unique_ptr`をManagerへmoveした後もobjectが生存することを前提にしたraw pointerである。AutoRemove falseのため通常は有効だが、Managerの外部`Clear/RemoveEffect`には弱い。

修正:

- pointerの所有者がParticleManagerであることをコメントで明記する。
- Stop時はpointerをnullにしてからManagerから削除する。
- benchmark以外のEffectを`ParticleManager::Clear()`で全削除しない方法を優先する。
- 個別削除APIが既にある場合、benchmark Effectだけを削除する。
- 外部から対象Effectが削除された場合にpointerを使わない保証を説明する。
- 新しいshared ownership設計は導入しない。

完了条件:

- benchmarkが既存Editor Effectや他のscene Effectを不必要に削除しない。
- Start/Stop/scene終了でdangling pointerがない。

### V2-103: benchmark実行結果を残す

次のmatrixを実行する。

| Preset | CPU Debug | GPU Debug | CPU Release | GPU Release |
|---|---|---|---|---|
| B01 | 必須 | 必須 | 必須 | 必須 |
| B02 | 必須 | 必須 | 必須 | 必須 |
| B03 | 必須 | 必須 | 必須 | 必須 |
| B04 | 必須 | 必須 | 必須 | 必須 |
| B05 | 必須 | budget判定に従う | 必須 | budget判定に従う |
| B06 | 必須 | 必須 | 必須 | 必須 |
| B07 | 必須 | 必須。ただしhybrid/Trailの意味を記録 | 必須 | 必須。ただしhybrid/Trailの意味を記録 |

各セルに次を記録する。

- `PASS`
- `FAIL: 理由`
- `SKIPPED: 理由`

成果物:

- `engine/effects/particle/docs/PARTICLE_BENCHMARK_RESULTS.md`
- 実際に生成されたbaseline CSV

benchmark結果文書には次を含める。

- date/time
- commit/diff identifier
- GPU名/driver（取得可能な範囲）
- Debug/Release
- resolution
- VSync/FPS limiterの取得可否
- preset/mode
- 実行秒数
- sample count
- active count
- validation error
- skip理由

### V2-104: 実行耐久テスト

以下を実行し、回数と結果を記載する。

1. B01 CPU Start/Stop ×100
2. B01 GPU Start/Stop ×100
3. EditorでEffectを開いた状態からStart/Stop ×20
4. benchmark実行中にscene終了 ×20
5. preset不正値の直接呼出または同等のguard test
6. texture解決失敗test
7. descriptor budget不足test

手動で100回クリックする必要はない。benchmark debug helperまたは既存test手段で繰り返せる場合は使用してよい。ただしproduction APIへtest専用関数を露出しない。

## 4. G4 Diagnostics修正

### V2-201: CSV schemaの列名と値を一致させる

重要度: **High**

現状:

CSV header:

```text
active_particle_count_type
```

実際の値:

```cpp
activeCount
```

これはschema不一致である。

修正後の最低列:

```text
preset
simulation_mode
build
resolution_width
resolution_height
resolution_source
vsync_state
fps_limiter_state
scope_name
last_ms
average_ms
min_ms
max_ms
sample_count
cpu_upload_memcpy_bytes_last_frame
cpu_upload_memcpy_bytes_average
gpu_upload_copy_bytes_last_frame
gpu_upload_copy_bytes_average
gpu_readback_copy_bytes_last_frame
gpu_readback_copy_bytes_average
cpu_readback_memcpy_bytes_last_frame
cpu_readback_memcpy_bytes_average
emitter_count
active_particle_count
active_particle_count_type
sampling_seconds
result_status
notes
```

要件:

- 実値を取得できない項目は空欄または`unknown`にする。
- 数値列へ`unknown`文字列を入れない。source/state列を使う。
- `active_particle_count_type`は`CPU_VECTOR_EXACT`、`HYBRID_CPU_VECTOR_1F_DELAYED`、`LAST_KNOWN`等の文字列にする。
- CSVのheaderと各rowの列数をtestで比較する。
- comma、quote、newlineを含むnotesを正しくescapeする。

### V2-202: byte counterをsampling統計化する

現状は`ParticleManager::Update()`のたびに`ResetCounters()`されるため、Exportされるのは終了直前の1フレーム分だけである。

修正:

各byte分類について最低限、以下を保持する。

- current frame
- sampling total
- sampling average per frame
- sampling min per frame
- sampling max per frame
- sampled frame count

byte分類:

- CPU upload memcpy
- GPU upload CopyBufferRegion requested
- GPU readback CopyBufferRegion requested
- CPU readback memcpy

フレーム境界契約:

1. `BeginFrameCounters()`でcurrentを0にする。
2. 各処理がcurrentへ加算する。
3. `EndFrameCounters()`でsampling統計へcommitする。
4. warm-up中はsampling統計へcommitしない。
5. sampling開始時にsampling統計をResetする。

ParticleManager::Updateの先頭だけでResetし、Draw側のcounterが次frameへ混ざる設計にしない。どこからどこまでが1frameかを確認する。

### V2-203: Upload timerを分離する

現状の`UploadMapCopy`は`GPUSimulator::UploadParticles()`全体を囲み、次の両方を含む。

- CPU Map/memcpy/Unmap
- resource barrier/CopyBufferRegion command recording

修正:

scopeを分割する。

- `UploadCpuMapMemcpy`
- `UploadCommandRecording`

必要ならReadbackも次を明確化する。

- `ReadbackCpuMapMemcpy`
- `ReadbackCopyCommandRecording`はDispatch内のcopy recordingとして別scope

RAII timerの範囲をblockで限定し、名前と実測範囲を一致させる。

### V2-204: 複数Emitter/Renderer集計の意味を明記する

現在、同一scopeへ複数emitter/rendererの各呼出時間を順次記録するため、B04/B05の`average/min/max/sample_count`は「1 frameの合計」ではなく「1 callあたり」の統計になる。

修正方法はどちらか選ぶ。

#### 推奨A: per-callとper-frame totalを両方持つ

- `EmitterUpdateCpuPerCall`
- `EmitterUpdateCpuFrameTotal`
- `RendererUpdatePerCall`
- `RendererUpdateFrameTotal`
- `RendererDrawRecordingPerCall`
- `RendererDrawRecordingFrameTotal`

#### 最小B: per-callのまま明示

- scope名に`PerCall`を付ける。
- Managerまたはbenchmark側でframe totalを別途計測する。
- CSV/文書にsample countがframe countではなくcall countであることを書く。

B04/B05の比較にはframe totalが必要なので、per-callのみで完了にしない。

### V2-205: Release既定の計測状態

現状は`enabled_ = true`で、Releaseでも常時計測される。

修正:

- Debugは既定ONでもよい。
- Releaseは既定OFFにする。
- benchmark開始時だけ明示的にONにし、終了時に以前の状態へ戻す。
- benchmark中にUIからOFFにした場合、結果を`INVALID_PROFILING_DISABLED`として扱う。

### V2-206: 不要なParticleManager public wrappersを削除

現在次が残っている。

- `ResetFrameStats`
- `AddUploadBytes`
- `AddReadbackBytes`
- `GetLastUpdateDurationMs`
- `GetLastUploadBytes`
- `GetLastReadbackBytes`

repository内の利用箇所は定義以外にない。

修正:

- 未使用なら削除する。
- benchmark UIは`ParticleDiagnostics`を直接読み取る。
- runtime managerの公開APIにbenchmark互換wrapperを残さない。

### V2-207: CSV writerの責務を整理

`ParticleDiagnostics.h`はheader-only singletonで、filesystemとofstreamを含み、測定収集とCSV I/Oを同じclassが担当している。

この修正で大規模再設計は不要だが、最低限次を行う。

- CSV export実装を`.cpp`へ移す。
- `ParticleDiagnostics.cpp`を`.vcxproj/.filters`へ登録する。
- CSV escape helperを局所化する。
- export失敗理由を呼出側が表示できるようにする。
- header-onlyの各translation unitへのI/O実装展開を避ける。

## 5. G5 Lifecycle Specificationの修正

### V2-301: Restartの記述を実装に合わせる

現行実装:

```cpp
void ParticleEmitter::Restart()
{
    // particles_は残す
    emitterAge_ = 0;
    currentLoopCount_ = 0;
    isEmitting_ = true;
    ...
    module->Reset();
}
```

文書の「Reset後にPlay」は誤り。

正しくは:

- 既存particleは保持する。
- emitter lifecycleとmodule内部生成状態を戻す。
- rendererはResetしない。
- 新規生成を再開する。

### V2-302: duration=0の記述をmodule別にする

`duration=0 + Once`は常に無限ではない。

- SpawnRateは`IsComplete()==false`のため自発的には完了しない。
- finite SpawnBurstは全loop終了後`IsComplete()==true`になり、Emitterは生成停止する。
- Spawn moduleが複数ある場合は全Spawn module完了が必要。
- Spawn moduleが存在しない場合の現行挙動も確認する。

文書をこの条件分岐に合わせる。

### V2-303: SimulationMode切替の記述を修正

現行`SetSimulationMode()`は先に`ClearParticles()`を呼ぶ。既存粒子をCPU/GPU間で移送して維持する仕様ではない。

文書:

- Observed current behavior: 切替時に既存CPU particle vectorをclearする。
- GPU simulator内部のcount/bufferが同時にどうなるかを確認する。
- Expected behavior: 現行互換として全粒子を消すのか、将来は移送するのかを`DECISION_REQUIRED`にする。
- 「同期upload/readbackで維持される」と断定しない。

### V2-304: SpawnRate accumulatorのsymbolとPure GPU方針を修正

現行symbolは`spawnAccumulator_`ではなく`timeSinceLastSpawn_`である。

Pure GPU計画では定期SpawnRate/Burstのaccumulatorを`EmitterStateBuffer`上へ移す。そのため次の記述は削除する。

```text
スポンアキュムレータはCPU側で処理してスポン命令を発行するため差異なし
```

正しくは:

- 現行はCPU module state。
- Pure GPUではGPU EmitterStateへ移行予定。
- 同一fixed timestep/seedで生成数の意味を維持する。

### V2-305: 観測と推測を再確認

以下は実装を読んだだけで断定せず、実行確認または`DECISION_REQUIRED`にする。

- Local/World follow挙動
- large deltaTime spikeを「維持すべき」とするか
- Effect削除時の即時GPU resource解放
- scene終了時のfence wait
- Stop+KillがGPU modeで同一frameに消えるか
- completion queueへ「即座に入る」という表現

特に、既存の望ましくない挙動を`Expected behavior to preserve: 同等`と機械的に書かない。現行観測、望ましい仕様、未決定を分離する。

## 6. Whitespaceと差分品質

### V2-401: `git diff --check`を通す

現在、親repositoryとengineの両方でtrailing whitespaceが検出される。

修正対象は今回追加・変更した行だけに限定する。

禁止事項:

- repository全体の空白一括修正
- 無関係ファイルのformat
- 改行コードの全変換

完了条件:

```text
git diff --check
git -C engine diff --check
```

の両方がexit code 0。

## 7. Buildと実行検証

### 必須build

```text
Debug|x64
Release|x64
```

記録:

- command
- exit code
- warning count
- error count

### 必須実行検証

- Texture解決後にB01が開始できる。
- B01〜B07 matrixの結果がある。
- B05 GPUはbudget判定結果がある。
- Start/Stop耐久結果がある。
- Editor Effect保持状態からのStart/Stop結果がある。
- scene終了結果がある。
- D3D12 debug layer/GPU validationの結果がある。
- CSVが実際に生成され、header/row列数が一致する。
- CSVの`active_particle_count`と`active_particle_count_type`が別列である。
- Release benchmark開始前後でdiagnostics enable状態が復元される。

GUI実行できない環境では、未実施をPASSとしない。`NOT_RUN_ENVIRONMENT_LIMITATION`として明記する。

## 8. Repository整理

- MediaFoundation関連commit/diffを変更しない。
- `MEDIA_FOUNDATION_PLAN.md`の削除状態を変更しない。
- G3/G4/G5の修正だけをpath単位で提示する。
- engine内Particle差分と親application差分を分けて報告する。
- commit/pushはユーザーの明示依頼なしに行わない。

## 9. 再提出フォーマット

```text
Correction IDs completed:

Changed files:
- ...

Texture resolution:
- Working directory:
- Requested path:
- Resolved path:
- B01 start result:

Descriptor budget:
- Heap capacity:
- Current usage:
- Per-emitter estimate:
- B05 CPU result:
- B05 GPU result/skip reason:

Diagnostics:
- Upload CPU memcpy scope:
- Upload command recording scope:
- Byte sampling aggregation:
- Per-call vs frame-total:
- Release default:

CSV:
- Output file:
- Header column count:
- First data row column count:
- active count:
- active count type:

Lifecycle corrections:
- Restart:
- duration=0:
- SimulationMode switch:
- SpawnRate state:

Validation:
- git diff --check:
- engine git diff --check:
- Debug build:
- Release build:
- B01-B07 matrix:
- Start/Stop x100:
- Scene exit x20:
- D3D12 validation:

NOT_RUN:
- ...

Pre-existing changes preserved:
- ...
```

## 10. 最終受領条件

次をすべて満たした場合のみG3〜G5を完了とする。

- Benchmarkが実際に開始できる。
- B05 GPUがdescriptor不足へ安全に対処する。
- benchmarkが他のEffectを不要に削除しない。
- B01〜B07の実行または正当なskip結果がある。
- CSV headerとrowが一致する。
- active countとcount typeが分離される。
- byte counterがsampling統計を持つ。
- Upload map/memcpyとcommand recordingが分離される。
- multi-emitterのframe totalを比較できる。
- Release既定でdiagnosticsが無効である。
- 未使用ParticleManager diagnostics wrapperが削除される。
- Restart、duration=0、mode切替、SpawnRateの文書が実装と一致する。
- 観測、期待仕様、未決定が区別される。
- 親/engine双方の`git diff --check`が成功する。
- Debug/Release buildが成功する。
- 未実施項目をPASSとして報告していない。
