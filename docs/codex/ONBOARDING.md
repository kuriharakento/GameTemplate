# Codex 作業前に必ず読むこと

このリポジトリで演出シーケンサ（`SEQUENCER_PLAN.md`）を実装している。
Phase 0〜4 は実装済み。以降のタスクは `docs/codex/TASK-*.md` に1つずつ与える。

**ここに書いてあることは全て、実際に踏んで痛い目を見た内容である。読み飛ばすと同じ穴に落ちる。**

---

## 1. リポジトリ構成

| | パス | 既定ブランチ | 備考 |
|---|---|---|---|
| アプリ | リポジトリルート | `master` | `application/` |
| エンジン | `engine/` | `main` | **git submodule。別リポジトリ** |

- **デフォルトブランチ名が親と engine で違う**（`master` と `main`）。取り違えないこと。
- 統合ブランチは両方 `feature/sequencer`。作業ブランチはそこから切る。
- engine にある `origin/develop` は 76 コミット古い放置ブランチ。**使わない。**

### submodule の二重コミット

`engine/` 以下を変更したら、**必ず engine → 親の順に2回コミットする。**

```bash
cd engine
git add -A && git commit -m "..."
cd ..
git add engine && git commit -m "engine を ... へ更新"
```

親側のコミットを忘れると、他の環境でチェックアウトしたときに engine が古いままになる。

---

## 2. ビルドと実行

### ビルド

```bash
"C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"
msbuild GameTemplate.sln /p:Configuration=Debug /p:Platform=x64 /m /v:minimal
```

`リモート配置が時間がかかるか...` の warning は既存のもの。無視してよい。

### 実行（**最重要**）

**作業ディレクトリは必ずソリューションディレクトリ（リポジトリルート）にすること。**

```bash
# 正しい
cd C:\Users\kurih\source\repos\kuriharakento\GameTemplate
generated\outputs\Debug\GameTemplate.exe
```

`.vcxproj` の `LocalDebuggerWorkingDirectory` が `$(SolutionDir)` になっており、
シェーダーのパスが `Resources/shaders/...` の相対で解決される。

exe のあるディレクトリから起動すると **シェーダーが見つからず `SpriteCommon` の
初期化中に assert で落ちる（終了コード 3）**。
これを「動いている」と誤認して検証したつもりになる事故が実際に起きている。

### 新しいファイルを追加したら

`.vcxproj` と `.vcxproj.filters` の**両方**に手で追記する。

**`UpdateFilters.py` は使えない。この環境に Python が入っていない。**

```xml
<!-- KentoCompoEngine.vcxproj -->
<ClCompile Include="graphics\view\RenderView.cpp" />
<ClInclude Include="graphics\view\RenderView.h" />
```

```xml
<!-- KentoCompoEngine.vcxproj.filters -->
<ClCompile Include="graphics\view\RenderView.cpp">
  <Filter>graphics\view</Filter>
</ClCompile>
```

新しいディレクトリを作った場合は `.filters` に `<Filter Include="...">` の
エントリ（`UniqueIdentifier` 付き）も足すこと。

---

## 3. 検証のやり方

### D3D12 デバッグレイヤーを検証に使う

`DirectXCommon` が `SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true)` を
設定しており、GPUベース検証も有効になっている。

つまり **API の使い方を間違えると即座にプロセスが落ちる。**
逆に言えば「25秒以上動き続ける」ことが、そのまま
「D3D12 のエラーが1件も出ていない」証明になる。描画を触ったら必ずこれを確認すること。

### 絵の確認は人間に頼む

DX12 のフリップモデル・スワップチェインは **GDI のスクリーンキャプチャに映らない**
（真っ白または真っ黒の画像しか取れない）。
自動で絵を確認する手段は無い。**「絵が正しいか」は必ず人間に見てもらうこと。**
自分で確認できていないことを、できたかのように報告しない。

---

## 4. 地雷リスト

### 4.1 定数バッファの C++ / HLSL レイアウト不一致

**実際にこれで画面が真っ暗になった。**

HLSL の定数バッファは変数が 16 バイト境界をまたぐことを許さず、
またぐ場合は次の境界へ送られる。C++ は詰めて配置するのでズレる。

```
float3 pad4;          // Offset 144〜155
float2 invScreenSize; // 156 だと境界をまたぐ → 160 へ送られる
```

C++ 側は `float pad4[4]` にして 4 バイト埋めないと、以降の全フィールドが
4 バイトずれて読まれる。**コンパイルも実行も通ってしまい、絵が壊れる形でしか現れない。**

対処:
- `effects/postprocess/base/BasePostEffect.h` の `static_assert` 群を必ず更新する
- 期待値は `dxc -Fc` の逆アセンブルで実際のオフセットを確認してから書く

```bash
dxc.exe -T ps_6_0 -E main -Zpr -I engine\Resources\shaders \
  engine\Resources\shaders\PostEffect.PS.hlsl -Fc out.asm
# out.asm の先頭に "; Offset: N" 付きの cbuffer レイアウトが出る
```
（`dxc.exe` は `C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x64\` にある）

### 4.2 PSO の RTV フォーマット

PSO の `RTVFormats[0]` は、実際に描くレンダーターゲットのフォーマットと
一致していなければならない。**数値を直書きしないこと。**

`graphics/RenderFormats.h` の定数を使う:

| 定数 | 用途 |
|---|---|
| `kSceneColorFormat` | シーンを描く HDR ターゲット（`R16G16B16A16_FLOAT`） |
| `kDisplayColorFormat` | 画面に出す LDR（`R8G8B8A8_UNORM_SRGB`） |
| `kBloomBufferFormat` | ブルームの中間バッファ |

### 4.3 assert で落とさない経路

エディタでは「壊れた JSON を開く」「消したファイルを参照する」が日常的に起きる。
**アセット読み込みと JSON 読み込みの経路は、assert ではなく戻り値と
`Logger::Log(..., Logger::LogLevel::Error)` で失敗を返すこと。**

シェーダーのホットリロードも同様。`DirectXCommon::TryCompileShader()` が
assert しない版なので、実行中の再コンパイルにはこちらを使う。
`CompileSharder()` は起動時専用（失敗すると assert で止まる）。

---

## 5. 設計上の規約

破ると後続のフェーズが成立しなくなるものだけを挙げる。

### 5.1 `Evaluate(t)` の純関数契約（シーケンサ）

`ITrack::Evaluate(time, ctx)` は **それまでにどの時刻を評価したかに依存してはならない。**

- 「毎フレーム加算して進める」実装は禁止（`pos += vel * dt` を書いた時点で契約違反）
- deltaTime を引数に取らないのは意図的

これを守るとスクラブ・スキップ・早送り・途中再生が実装なしで成立する。
破るとカットシーンのスキップが**原理的に**実装できなくなる。

### 5.2 エディタの編集は必ずコマンド経由

- 値の書き換え → `CommandHistory::GetInstance()->Execute(...)`
- 粒度は「ユーザーの1操作 ＝ 1コマンド」。ドラッグは `ICommand::MergeWith()` で1件に統合
- 選択状態 → `SelectionContext` のみ。UI ごとに選択を持たない

### 5.3 描画パスの追加

**`MyGame::Draw()` に描画コードを書かない。** 描画順は
`engine/graphics/pipeline/` の `RenderPipeline` に集約されている。

```cpp
Framework::GetRenderPipeline()->InsertPassAfter("Forward", std::make_unique<MyPass>());
```

- パスは `RenderPassContext` から必要なものだけを取る。他のパスの存在を前提にしない
- **シーンに描くパスは `SceneColorResolve` より前に挿入すること**
- カメラとレンダーターゲットは `ctx.view`（`RenderView`）から取る。
  単一のグローバルを直接見ない（サブビューで破綻する）

### 5.4 コードスタイル

既存コードに合わせる。

- コメント・Doxygen は**日本語**
- タブインデント、`{` は次の行
- メンバ変数は末尾アンダースコア `foo_`
- 定数は `kCamelCase` の `constexpr`
- マジックナンバーを直書きせず名前付き定数にする

**「なぜそうしたか」をコメントに書く。** 何をしているかはコードを読めば分かる。

---

## 6. コミット

- コミットメッセージは**日本語**
- 1行目に要約、空行、本文に「何をしたか」ではなく**「なぜそうしたか」**
- 末尾に `Co-Authored-By:` 行は不要（人間がレビューして統合する）

---

## 7. タスク完了の条件（Definition of Done）

以下を全て満たして初めて完了とする。満たせない項目があれば、
**満たしたことにせず、満たせなかったと報告すること。**

1. `msbuild` が Debug/x64 で通る（新規警告を増やさない）
2. リポジトリルートを作業ディレクトリにして 25 秒以上動作し、落ちない
3. 描画を触った場合、D3D12 の ERROR / CORRUPTION が 0 件
4. 追加ファイルが `.vcxproj` と `.vcxproj.filters` の両方に登録されている
5. engine を変更した場合、engine と親リポジトリの両方でコミットされている
6. タスク仕様の「受け入れ条件」を全て満たしている
7. **絵が正しいかは人間の確認が必要**。その旨を報告に明記する
