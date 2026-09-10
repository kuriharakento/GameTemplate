# アセット読み込み失敗時の継続処理 報告

## 変更したファイルと内容

- `engine/audio/Audio.h`, `engine/audio/Audio.cpp`
  - WAV のオープン、ヘッダー、フォーマット、data チャンク、データ長を検証し、失敗時は空の `SoundData` と Error ログを返すようにした。
  - Media Foundation の `DecodeAudioFile` が `false` を返した場合も Error ログを出し、音声を登録しないようにした。
  - 未登録名および XAudio2 の再生処理失敗では Error ログを出して再生を中止するようにした。
  - 同じファイルや未登録名の失敗を記録し、更新処理から繰り返し呼ばれてもログを出し続けないようにした。
- `engine/manager/graphics/TextureManager.h`, `engine/manager/graphics/TextureManager.cpp`
  - 読み込みまたはミップ生成に失敗したパスを `textures/white1x1.png` の index に結び付けるようにした。
  - `GetTextureIndexByFilePath` に未登録パスが渡された場合も Error ログを出し、白テクスチャの index を返すようにした。
  - 白テクスチャ自身が失敗した場合は再帰せず index 0 を返す。失敗パスを記録して同じログと再試行を繰り返さない。
  - `textures/white1x1.png` は、実行時のアプリリソースルート `application/Resources` と既存の検索処理により、実在する `application/Resources/textures/white1x1.png` に解決されることを実行で確認した。
- `engine/graphics/3d/Model.h`, `engine/graphics/3d/Model.cpp`, `engine/manager/graphics/ModelManager.h`, `engine/manager/graphics/ModelManager.cpp`
  - モデル初期化を `bool` にし、Assimp の読み込み失敗、メッシュなし、非三角形面を Error ログ付きで返すようにした。
  - 失敗したモデルはキャッシュに登録せず、`FindModel` が `nullptr` を返す状態を保つようにした。
  - 同じ失敗モデルを毎フレーム読み直さないように記録するようにした。
- `engine/graphics/3d/SkinnedModel.h`, `engine/graphics/3d/SkinnedModel.cpp`, `engine/graphics/3d/SkinnedObject3d.cpp`, `engine/manager/graphics/SkinnedModelManager.h`, `engine/manager/graphics/SkinnedModelManager.cpp`
  - スキニングモデルの Assimp 読み込み失敗と非三角形面を Error ログ付きで `nullptr` として返し、キャッシュへ登録しないようにした。
  - `SkinnedModel::Initialize` が失敗を返し、`SkinnedObject3d` がモデルを保持しないことで既存の null 判定による描画スキップにつなげた。
  - 同じ失敗パスは再試行せず、ログを繰り返さないようにした。
- `engine/graphics/2d/FontSprite.h`, `engine/graphics/2d/FontSprite.cpp`
  - フォントメトリクスが存在しない、JSON が壊れている、文字情報が空の場合を Error ログ付きで失敗として返すようにした。
  - 失敗時は非表示にし、文字スプライトを作成・描画しないようにした。

新しいソースファイルは追加していないため、`.vcxproj` と `.vcxproj.filters` の変更はない。

## assert の扱い

### 変更した assert

- `Audio::LoadWave`
  - ファイルオープン、RIFF/WAVE、fmt、data、データ読み込みの assert を入力ファイルの検証と Error ログに変更した。外部ファイルの欠落や破損はエディタで通常起こるため。
- `Audio::PlayWave`
  - SourceVoice 作成、バッファ送信、再生開始の `SUCCEEDED(hr)` assert を Error ログと早期 return に変更した。再生できない音を飛ばして編集を継続するため。
- `TextureManager::LoadTexture`
  - ファイルデコードとミップ生成の assert を白テクスチャへの代替に変更した。失敗した元パスの後続参照も有効な index にするため。
- `TextureManager::GetTextureIndexByFilePath`
  - 未登録パスの assert を Error ログと白テクスチャ index の返却に変更した。描画時の参照でアプリを終了させないため。
- `Model::LoadMaterialTemplateFile`, `Model::LoadModelFile`
  - マテリアルのオープン、Assimp シーン、メッシュ有無、三角形面の assert を Error ログと失敗戻り値に変更した。壊れたモデルを登録せず描画対象から外すため。
- `SkinnedModel::Initialize`, `SkinnedModelManager::LoadModel`
  - 共有モデル取得と三角形面の assert を失敗戻り値に変更した。無効な共有リソースから GPU バッファを作らないため。
- `FontSprite::LoadFontMetrics`
  - JSON オープンの assert を Error ログと `false` に変更した。欠落・編集中・破損した JSON で文字だけを非表示にするため。

### 残した assert

- `Audio::Initialize` の XAudio2 作成、MasteringVoice 作成、Media Foundation 初期化: 起動時の音声基盤初期化の失敗であり、アセット単体の失敗ではないため。
- `TextureManager::LoadTexture` の SRV 上限: プログラム側のリソース管理上限違反のため。
- `TextureManager::GetMetadata(uint32_t)` の index 検証: プログラムが渡す index の誤りを検出するため。
- `SkinnedModel::CreateSkinningBuffers` の GPU リソース作成: GPU リソース作成失敗であり、対象のファイル入力失敗ではないため。
- `FontSprite::Initialize` の `spriteCommon` 検証: `nullptr` を渡す呼び出し側のバグを検出するため。

指定された分類は妥当と判断し、上記以外へ広げていない。

## 検証

### 自分で確認できたこと

- 開始時に親・engine の両方で `git merge-base --is-ancestor feature/sequencer HEAD` が `YES`。
- Debug/x64 の `msbuild` は成功。警告は ONBOARDING 記載の既存のリモート配置警告3件で、新しい警告はなかった。
- 最終コードをリポジトリルートから起動して30秒継続。終了は検証側から行い、D3D12 ERROR / CORRUPTION による停止は0件だった。
- 一時的に次を追加して起動し、30秒継続した。検証後にコードとファイルは削除し、コミットしていない。
  - 存在しないPNG、0バイトPNG
  - 存在しない glTF、テキストだけの `.gltf`
  - 壊れた WAV、存在しない音声名の `PlayWave`
  - 存在しないフォント
- 存在しないPNG、0バイトPNG、白テクスチャの index はすべて `14` だった。存在しないモデルと壊れたモデルは `FindModel == nullptr` だった。
- 同じ存在しないテクスチャと未登録音声名を2回ずつ呼び、各エラーが1回だけ出ることを確認した。

実際に得られたログの抜粋:

```text
テクスチャを読み込めませんでした: application/Resources/missing.png
テクスチャを読み込めませんでした: application/Resources/empty.png
テクスチャ代替確認: missing=14, empty=14, fallback=14
モデルファイルを読み込めませんでした: Resources/models/missing_model/missing_model.gltf (Unable to open file "Resources/models/missing_model/missing_model.gltf".)
モデルファイルを読み込めませんでした: application\\Resources\\models/broken_model/broken_model.gltf (No suitable reader found for the file format of file "application\\Resources\\models/broken_model/broken_model.gltf".)
モデル未登録確認: missing=OK, broken=OK
スキニングモデルを読み込めませんでした: Resources/models/missing_skin/missing_skin.gltf (Unable to open file "Resources/models/missing_skin/missing_skin.gltf".)
音声ファイルを読み込めませんでした: application\\Resources\\audio\\application/Resources/broken.wav
WAVヘッダーを読み取れませんでした: application/Resources/broken.wav
登録されていない音声は再生できません: missing_audio
フォントメトリクスを開けませんでした: application\\Resources\\fonts\\missing_font_metrics.json
```

通常の出力先は `Logger::Log` の実装どおり Visual Studio の出力ウィンドウ（`OutputDebugStringW`）と、生成済みならエディタ内の `ConsoleLog`。ファイルへの常設出力はない。上記採取時だけ Logger に一時的なファイル出力を足し、採取後に元へ戻した。

### 人間の確認が必要なこと

- 代替テクスチャが画面上で白く表示されること。DX12 のフリップモデルでは自動キャプチャで絵を判定できないため未確認。

## コミット

- engine: `3fc3fb44f3dc40f1a21c35351425c60062f04c93`

