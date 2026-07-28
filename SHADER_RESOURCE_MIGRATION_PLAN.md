# Shader Resource Migration Plan

## 1. 目的

shaderの原本を engine/Resources/shaders/ へ一本化し、application/Resources/shaders/ の重複を廃止する。

この作業では、shader以外のtexture、model、audio、JSONなどは移動しない。
ルート直下に新しい Resources/ も作成しない。

## 2. 完了後の所有ルール

~~~text
engine/Resources/shaders/       Engineが所有する全shader
application/Resources/         ゲーム固有リソース（shaderは置かない）
~~~

- shaderの追加・修正は必ずEngine側で行う
- Application側にshaderのコピーや上書き機構を作らない
- GameTemplate実行時は、ソリューション直下を作業ディレクトリとする
- GameTemplateからのshader参照先は engine/Resources/shaders/ とする
- Engine単体実行時の参照も壊さない

## 3. 現状調査結果

2026-07-28時点の比較結果:

- application/Resources/shaders/: 39ファイル
- engine/Resources/shaders/: 39ファイル
- 内容が完全一致: 37ファイル
- Application側のみ: 0ファイル
- Engine側のみ: 0ファイル
- 内容が異なる: 2ファイル

差分があるファイル:

~~~text
Object3d.PS.hlsl
Sprite.PS.hlsl
~~~

差分理由:

- Object3d.PS.hlsl
  - Application側では CalculateHalfLambert の戻り値が float
  - Engine側では誤って float3
- Sprite.PS.hlsl
  - Application側ではUV変換結果を明示的に float2 へ変換済み
  - Engine側ではベクトルの暗黙切り捨て警告が発生する

Application側の2修正は、DXC警告による DirectXCommon.cpp:830 のassertを防ぐために必要。
Application側を削除する前に、必ずEngine側へ移植する。

## 4. 変更禁止事項

- application/Resources/shaders/ を先に削除しない
- 比較せずにApplication側をEngine側へ一括上書きしない
- shader以外のResourcesを移動しない
- engine/ と親リポジトリの変更を同一コミットにしない
- Engineのサブモジュール参照を、Engine側のcommit前に更新しない
- assertを無効化してDXC警告を隠さない
- 無関係なコード整理、命名変更、整形を行わない
- ファイル名の大文字・小文字をこの作業で一括修正しない
- commit、push、PR作成はユーザーの明示的な指示なしに行わない

## 5. 作業順序

### Phase 0: 作業前確認

親リポジトリ:

~~~powershell
git status --short --branch
git submodule status
git diff
~~~

Engine:

~~~powershell
git -C engine status --short --branch
git -C engine diff
~~~

未コミット変更がある場合は上書きしない。
今回の作業と重なる場合は作業を止めてユーザーへ確認する。

### Phase 1: Engine側へshader修正を移植

対象:

~~~text
engine/Resources/shaders/Object3d.PS.hlsl
engine/Resources/shaders/Sprite.PS.hlsl
~~~

必要な変更:

1. Object3d.PS.hlsl
   - CalculateHalfLambert の戻り値を float3 から float へ変更
2. Sprite.PS.hlsl
   - 行列計算結果から .xy を明示的に取り出して float2 へ格納
   - Sample にはその float2 を渡す

変更後、Application側とEngine側の39ファイルをSHA-256で比較し、すべて一致することを確認する。

完了条件:

~~~text
Identical: 39
Different: 0
Application-only: 0
Engine-only: 0
~~~

### Phase 2: Engineのshader検索先を整理

Application側shaderへの依存を削除する。

調査対象:

~~~text
engine/base/DirectXCommon.cpp
engine/graphics/deferred/GBufferPipeline.cpp
engine/graphics/deferred/LightPassPipeline.cpp
engine/effects/particle/gpu/GPUParticlePipeline.cpp
~~~

現在含まれている誤った、または廃止対象の候補:

~~~text
application/Resources/shaders/
../engine/Resources/shaders/
~~~

GameTemplateの作業ディレクトリはソリューション直下なので、優先候補は次とする。

~~~text
engine/Resources/shaders/<filename>
~~~

Engine単体での利用を維持する必要があるため、必要に応じて次も候補として残す。

~~~text
Resources/shaders/<filename>
../Resources/shaders/<filename>
~~~

検索順:

1. 呼び出し元から渡されたパスが存在すれば使用
2. engine/Resources/shaders/<filename>
3. Resources/shaders/<filename>
4. ../Resources/shaders/<filename>
5. どこにも存在しなければ、既存のエラー処理へ渡す

注意:

- 同じ検索候補が複数箇所にあるため、4ファイルすべてを確認する
- このタスクで大規模なパス管理リファクタは行わない
- 共通化が必要だと判断した場合は、実装前にユーザーへ相談する

参照残りの確認:

~~~powershell
rg -n "application/Resources/shaders|\.\./engine/Resources/shaders" engine
~~~

完了条件は該当0件。

### Phase 3: Engine側を先に検証

#### 3.1 DXC検証

Windows SDKの dxc.exe を使用し、engine/Resources/shaders/ にあるエントリーポイント main を持つshaderを検証する。

実行時と同じオプション:

~~~text
-E main
-T vs_6_0 / ps_6_0 / cs_6_0
-Zi
-Qembed_debug
-Od
-Zpr
~~~

補助用の .CS.hlsl には main がないファイルがあるため、ファイル名だけで一律コンパイルしない。
main を持つファイルだけを対象にする。

完了条件:

~~~text
DXC終了コード: 0
警告: 0
エラー: 0
~~~

#### 3.2 Engine単体ビルド

Engine変更なので、次を確認する。

~~~powershell
& '<MSBuild.exe>' engine/project/KentoCompo.sln /m /t:Build /p:Configuration=Debug /p:Platform=x64 /v:minimal
~~~

ソリューションに存在する構成名が異なる場合は、推測せず .sln を確認する。

#### 3.3 Engine側差分確認

~~~powershell
git -C engine diff --check
git -C engine diff
git -C engine status --short --branch
~~~

Engine側の変更だけになっていることを確認する。

### Phase 4: Engine側を独立してcommit

ユーザーからcommitの明示指示がある場合だけ実行する。

対象はEngineサブモジュール内の変更だけ。
親リポジトリの変更を混ぜない。

commit後にEngine側のcommit IDを記録する。

### Phase 5: Application側の重複shaderを削除

Phase 1の39ファイル完全一致と、Phase 3のEngine検証が完了してから実施する。

削除対象:

~~~text
application/Resources/shaders/
~~~

削除前に再度、Application側のみのファイルが0件であることを確認する。

削除後の確認:

~~~powershell
Test-Path application/Resources/shaders
rg -n "application/Resources/shaders" application engine
~~~

期待結果:

~~~text
application/Resources/shadersは存在しない
application/Resources/shadersへの参照は0件
~~~

### Phase 6: プロジェクトフィルター更新

Application側からshaderファイルを削除する前後で、GameTemplate.vcxproj の明示登録も削除する。

2026-07-28時点で登録されている対象:

~~~text
FxCompile:
- Resources\shaders\Object3d.PS.hlsl
- Resources\shaders\Object3d.Vs.hlsl
- Resources\shaders\Skybox.PS.hlsl
- Resources\shaders\Skybox.VS.hlsl

None:
- Resources\shaders\Line.hlsli
- Resources\shaders\Object3d.hlsli
- Resources\shaders\Particle.hlsli
- Resources\shaders\Skybox.hlsli
- Resources\shaders\Sprite.hlsli
~~~

上記9件だけを application/GameTemplate.vcxproj から削除する。
他の項目は整形・並べ替えしない。

その後、UpdateFilters.py で .filters を再生成する。

親リポジトリ:

~~~powershell
python UpdateFilters.py application/GameTemplate.vcxproj
rg -n -i "Resources[\\/]shaders|\.hlsl" application/GameTemplate.vcxproj application/GameTemplate.vcxproj.filters
git diff -- application/GameTemplate.vcxproj application/GameTemplate.vcxproj.filters
~~~

注意:

- shader登録を削除するため、.vcxproj と .filters の差分発生は正常
- rg の期待結果はshader登録0件
- UpdateFilters実行後の差分を必ず目視確認する
- shader削除と無関係なフィルター差分が出た場合は止めて確認する
- Engine側プロジェクトの登録更新が必要な場合は、Engine側の既存手順を確認して別途実施する

### Phase 7: 親リポジトリで統合検証

#### 7.1 ビルド

~~~powershell
& '<MSBuild.exe>' GameTemplate.sln /m /t:Build /p:Configuration=Debug /p:Platform=x64 /v:minimal
~~~

完了条件:

- 終了コード0
- generated/outputs/Debug/GameTemplate.exe が生成される
- shaderファイルが見つからないassertが出ない
- DXC警告による DirectXCommon.cpp:830 のassertが出ない

#### 7.2 実行確認

作業ディレクトリ:

~~~text
GameTemplate/
~~~

起動対象:

~~~text
generated/outputs/Debug/GameTemplate.exe
~~~

最低確認:

1. ウィンドウが起動する
2. タイトル画面まで進む
3. DXC関連assertが出ない
4. Engine標準描画が表示される
5. Debug出力にshader警告・エラーがない

#### 7.3 最終検索

~~~powershell
rg -n "application/Resources/shaders|\.\./engine/Resources/shaders" application engine
~~~

期待結果は0件。

### Phase 8: 親リポジトリのサブモジュール参照更新

Engine側のcommitとpushが完了し、親側でそのcommitを取得できる状態になってから行う。

確認:

~~~powershell
git submodule status
git diff --submodule=log
~~~

親側commitには次だけを含める。

- application/Resources/shaders/ の削除
- 必要なApplicationプロジェクト／フィルター更新
- Engineサブモジュール参照の更新

ユーザーの許可なしにcommit、pushしない。

## 6. 最終受け入れ条件

- [ ] shaderの原本が engine/Resources/shaders/ の39ファイルだけになっている
- [ ] application/Resources/shaders/ が存在しない
- [ ] Application側のみのshaderを失っていない
- [ ] Object3d.PS.hlsl の戻り値型修正がEngine側にある
- [ ] Sprite.PS.hlsl の暗黙切り捨て修正がEngine側にある
- [ ] Application shaderパスへの参照が0件
- [ ] DXC検証が警告0・エラー0
- [ ] Engine単体ビルドが成功
- [ ] GameTemplate.sln の Debug|x64 ビルドが成功
- [ ] GameTemplateがソリューション直下を作業ディレクトリとして起動
- [ ] DirectXCommon.cpp:830 のassertが再発しない
- [ ] 親リポジトリとEngineの変更が別コミット
- [ ] 変更ファイルがCRLF
- [ ] git diff --check が成功
- [ ] 実行していない検証を成功扱いしていない

## 7. Geminiへの完了報告フォーマット

~~~text
Summary:
- 実施した内容

Engine changes:
- 変更ファイル
- Engine commit ID（commit指示があった場合のみ）

Parent changes:
- 変更ファイル
- 削除したファイル数
- サブモジュール参照の変更有無

Shader audit:
- Engine shader数
- Application shader数
- DXC検証数
- warning / error数

Verification:
- 実行コマンド
- 構成
- 終了コード
- 実行確認内容
- 未確認項目

Remaining risks:
- 残っている問題
~~~
