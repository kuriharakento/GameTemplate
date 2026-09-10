# TASK-05 実装報告

## 実装内容と変更理由

すべて engine 内の変更。親・engine とも `codex/phase5-transparent`。

| ファイル（engine 相対） | 理由 |
|---|---|
| `graphics/3d/IRenderable3d.h` | 合成方法とは独立した RenderQueue と取得・設定インターフェースを追加。 |
| `graphics/3d/Object3d.h` | Opaque を既定とする描画キューを保持。 |
| `graphics/3d/SkinnedObject3d.h` | 同じインターフェースと既定値を実装。 |
| `graphics/3d/Object3dCommon.h`, `.cpp` | 既存PSOを維持し、深度テストON・深度書き込みOFFのアルファブレンドPSOと設定関数を追加。RTVは kSceneColorFormat。 |
| `graphics/3d/SkinnedObject3d.cpp` | スキニング計算後のPSO再設定でも半透明設定を維持する。 |
| `graphics/pipeline/StandardRenderPasses.h`, `.cpp` | ForwardOpaque / Skybox / Transparent / SceneColorResolve に変更。ParticlePassを削除し、Transparent内で粒子を描画。デバッグラインはForwardOpaqueに維持。標準・サブビュー共通。 |
| `gameobject/manager/GameObjectManager.h`, `.cpp` | Draw3DとG-Bufferから半透明を除外。アクティブ状態・レイヤーマスクを適用し、ワールド位置と描画カメラの距離の2乗で降順安定ソート。親を先に更新して子も重複なく収集し、子の再帰描画で順序が崩れないようオブジェクト単位で描画。シーン直接登録のObject3dも同じソートに含める。 |
| `gameobject/base/GameObject.cpp` | 既存の子再帰描画経由でも半透明が不透明・G-Bufferへ混入しないようにする。 |
| `scene/interface/BaseScene.h` | 半透明描画をGameObjectManagerへ通し、直接登録オブジェクトも渡す。既存描画経路ではOpaqueのみを描く。 |
| `scene/manager/SceneManager.h`, `.cpp` | 描画中のCameraManagerを伴ってシーンへ半透明描画を中継する。サブビューでは既存のRenderCameraOverrideが適用される。 |

インターフェース追加の影響を受ける実装クラスは **Object3d と SkinnedObject3d の2クラス**。
定数バッファ・HLSLのレイアウトは変更していない。新規C++ファイルはないため、vcxproj / filters の追加登録は不要。
アプリ側のシーンへの恒久的なテストオブジェクト追加は行っていない。

## 自分で確認したこと

- Debug/x64 の msbuild 成功。警告はONBOARDING記載のリモート配置警告3件のみ。新規コンパイラ警告なし。
- リポジトリルートを作業ディレクトリとして最終exeを実行し、30秒間生存。その後ウィンドウを閉じ、終了コード0。
- GPUベース検証とERROR / CORRUPTIONのブレーク設定が有効であることをコードで確認。この実行では停止なし。ONBOARDINGの判定基準で当該実行のERROR / CORRUPTIONは0件相当。InfoQueueから件数を直接取得したわけではない。
- 最初の実行は例外0x87aで失敗。追加した深度リソース遷移がForwardOpaqueの遷移と重複していたため削除し、再ビルド・再実行で上記成功を確認した。
- git diff --check 成功。

| 受け入れ条件 | 確認範囲 |
|---|---|
| 1. Opaqueの見た目維持 | 既定Opaqueと既存PSO維持をコードで確認。絵は未確認。 |
| 2. 半透明の遠い順描画 | 距離の2乗・降順比較・ソート後の描画順をコードで確認。半透明を重ねた実描画は未確認。 |
| 3. 半透明の深度書き込みOFF | PSOのDepthWriteMask ZERO、DepthEnable有効をコードで確認。重なりの絵は未確認。 |
| 4. パーティクルがTransparent内 | 呼び出し位置とParticlePass削除をコードで確認。 |
| 5. Skyboxの位置 | パイプライン構築順をコードで確認。 |
| 6. デバッグUIの一覧 | GetNameと登録順をコードで確認。UI表示そのものは未確認。 |
| 7. サブビューの同じ並び | BuildSceneOnlyRenderPipelineをコードで確認。実際のサブビュー表示は未確認。 |
| 8. ONBOARDINGの完了条件 | ビルド・30秒動作は確認済み。目視の受け入れ確認が残るため、全条件を満たしたとはしていない。engineは1eed76eでコミット済み。親のコミットには、このengine参照更新と本報告を含める。 |

## 人間による確認手順

1. 一時的に2つのObject3dまたはGameObjectのRenderable3dを `RenderQueue::Transparent` に設定し、色のalphaを0.5程度にする。GameObjectはGameObjectManagerへ登録し、直接Object3dを使う場合はBaseScene::RegisterObjectで登録する。
2. 独自のDraw3Dオーバーライドからテスト用半透明を直接Drawしない（不透明パスとの二重描画を避ける）。
3. 2枚を重ね、カメラを回して前後が入れ替わったときの合成を確認する。不透明物を背後・手前にも置き、透過と深度テストを確認する。
4. Object3dとSkinnedObject3dの両方、既存不透明物の見た目、粒子とSkyboxの合成を確認する。
5. Render Pipeline UIで `ForwardOpaque / Skybox / Transparent / SceneColorResolve` を確認する。
6. 別カメラ・別レイヤーマスクのサブビューで、カメラ基準の順序とレイヤー除外を確認する。
7. テスト用オブジェクトを削除する。

## 仕様との差異・制限

- 仕様はISceneと記載しているが、現在のシーン基底クラスはBaseScene。この既存クラスへ経路を追加した。
- 仕様の現状説明と異なり、既存Object3dCommonはすでにアルファブレンド有効。見た目維持を優先して変更せず、深度書き込みOFFのPSOを追加した。
- パーティクルもすでに深度書き込みOFFなので、PSO変更は不要だった。
- 半透明オブジェクトのソート後にパーティクルを描く。パーティクルとメッシュを統合した距離ソートやパーティクル内部のソートは行っていない。このパス移動だけで粒子とメッシュの任意の交差・重なりまで解決するものではない。
- 三角形単位ソート、OIT、深度プリパス、ブレンドモード別PSO追加は行っていない。
