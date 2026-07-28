# Media Foundation音声読み込み対応 計画書

## 1. 目的

`application/Resources/audio`に配置した音声を、ファイル名だけで読み込めるようにする。

```cpp
Audio::GetInstance()->Load("bgm.mp4", SoundGroup::BGM);
Audio::GetInstance()->PlayWave("bgm.mp4", true);
```

Media Foundationで音声を非圧縮PCMへデコードし、既存のXAudio2再生、ループ、音量、フェード、グループ制御を維持する。

## 2. 今回のスコープ

- 同期読み込みと全デコード方式を採用する
- `wav`、`mp3`、`aac`、`m4a`、`mp4`、`wma`を読み込み対象とする
- `mp4`では映像を処理せず、音声ストリームだけを読み込む
- 読み込み先は`application/Resources/audio`を既定とする
- 既存の`LoadWave`を残して後方互換性を維持する
- ストリーミング再生、シーク、動画再生は今回の対象外とする

注意: 拡張子が対象内でも、ファイル内のコーデックをWindowsのMedia Foundationがデコードできない場合は読み込みに失敗する。

## 3. 現状と前提

- `Audio`は`engine/audio/Audio.h`と`engine/audio/Audio.cpp`にある
- 読み込んだPCMは`SoundData::buffer`が所有する
- 再生はXAudio2へPCMバッファを渡している
- `PathManager`でApplication側のResourcesルートを解決できる
- COMは`WinApp::Initialize()`で`COINIT_MULTITHREADED`として初期化され、`WinApp::Finalize()`で終了している
- `Audio::Initialize()`は`WinApp`初期化後、`Audio::Finalize()`は`WinApp`終了前に呼ばれる前提とする
- `engine/`はサブモジュールなので、Engineの変更と親リポジトリの変更を分けて扱う
- 現在の`engine/`には本件以外の未コミット変更があるため、着手前後に差分を確認し、本件の変更と混ぜない

## 4. 採用設計

### 4.1 公開API

次の汎用読み込み関数を追加する。

```cpp
void Load(const std::string& filename, SoundGroup group);
void Load(const std::string& name, const std::string& filename, SoundGroup group);
```

- 1引数目がファイル名だけのオーバーロードでは、キャッシュキーにも同じファイル名を使う
- 明示名付きオーバーロードは、既存の名前ベース再生を維持したい箇所向けとする
- `LoadWave(name, filename, group)`は互換APIとして残し、内部から`Load(name, filename, group)`を呼ぶ
- `PlayWave`など既存の再生API名は今回変更しない

### 4.2 デコード方式

1. `PathManager`で`application/Resources/audio/<filename>`を解決する
2. `MFCreateSourceReaderFromURL`でファイルを開く
3. 全ストリームを無効化し、最初の音声ストリームだけを有効化する
4. Source Readerの出力を`MFMediaType_Audio` + `MFAudioFormat_PCM`に設定する
5. 確定したメディアタイプを`MFCreateWaveFormatExFromMFMediaType`で`WAVEFORMATEX`へ変換する
6. `ReadSample`を終端まで繰り返し、各`IMFMediaBuffer`のデータを`SoundData::buffer`へ追加する
7. PCM、フォーマット、グループを`SoundData`へ保存し、既存キャッシュへ登録する

Media FoundationのCOMオブジェクトは`Microsoft::WRL::ComPtr`で所有する。`WAVEFORMATEX`の一時領域はAPI指定どおり`CoTaskMemFree`で解放し、解放漏れが起きない局所的なRAIIを使う。

### 4.3 初期化と終了

- `Audio::Initialize()`で、XAudio2の初期化前に`MFStartup(MF_VERSION)`を1回呼ぶ
- `Audio`にMedia Foundationの初期化成功状態を保持する
- `Audio::Finalize()`で、初期化に成功していた場合だけ`MFShutdown()`を1回呼ぶ
- COMの初期化・終了は既存どおり`WinApp`が所有し、`Audio`から`CoInitializeEx`や`CoUninitialize`を呼ばない
- `MFStartup`失敗時にXAudio2だけを半端に初期化しないよう、初期化順と失敗時処理を揃える

## 5. エラー処理方針

既存実装の`assert`だけに依存するとReleaseで失敗を検出できないため、新しい内部デコード関数は成功・失敗を呼び出し側へ返す。

推奨する内部境界:

```cpp
bool DecodeAudioFile(const std::filesystem::path& path, SoundGroup group, SoundData& output);
```

- 失敗時はキャッシュへ登録しない
- 空のPCM、音声ストリームなし、非対応形式、ファイルなしを失敗として扱う
- 失敗した出力オブジェクトに途中結果を残さない
- エラーログ機構が既存Audioにないため、本件だけで新しいログ基盤は追加しない
- 公開`Load`の戻り値変更は既存APIへの影響があるため、初回実装では`void`を維持する
- `HRESULT`は各段階で確認し、後続APIへ無効なポインタやメディアタイプを渡さない

## 6. 変更対象

### Engineサブモジュール

- `engine/audio/Audio.h`
  - 汎用`Load` APIを追加
  - Media Foundation初期化状態を追加
  - 内部デコード関数を宣言
  - 追加・修正する公開関数コメントをDoxygen形式にする
- `engine/audio/Audio.cpp`
  - Media Foundationの初期化、終了、全デコードを実装
  - `PathManager`によるパス解決を共通化
  - `LoadWave`を互換ラッパーにする
- `engine/KentoCompoEngine.vcxproj`
  - 必要なMedia Foundationヘッダーはソースから参照する
  - 静的ライブラリ側に不要なリンク設定を入れない

### 親リポジトリ

- `application/GameTemplate.vcxproj`
  - DebugとReleaseの`AdditionalDependencies`に`Mfplat.lib`、`Mfreadwrite.lib`、`Mfuuid.lib`を追加する
  - 既存のAssimp依存と`%(AdditionalDependencies)`を保持する
- 必要な場合のみ`application/Resources/audio`へ手動確認用の小さい音声ファイルを置く
  - 著作権とリポジトリ容量を確認できない素材は追加しない
  - バイナリ素材の追加はユーザー確認後に行う

補足: Audio用の新規`.cpp`または`.h`を分割しない限り、`.vcxproj.filters`の変更は不要。

## 7. タスク分割

### Task A: 事前確認（担当: Gemini可）

- 親リポジトリと`engine/`の`git status`、ブランチ、差分を記録する
- `Framework`における`WinApp`と`Audio`の初期化・終了順を確認する
- DebugとReleaseの既存リンク依存を記録する
- ファイルは変更しない

完了条件:

- 本件開始前の差分一覧が残っている
- COMとAudioの寿命順が計画の前提どおりであることを確認できる

### Task B: リンク設定（担当: Gemini可）

- `application/GameTemplate.vcxproj`のDebugとReleaseへ3つのライブラリを追加する
- `%(AdditionalDependencies)`を削除しない
- XMLの無関係な整形を行わない
- 改行をCRLFのまま維持する

完了条件:

- DebugとReleaseの両方に同じMedia Foundation依存がある
- Assimpなど既存依存が残っている
- `git diff --check`で問題がない

### Task C: Media Foundation初期化管理（担当: 中核実装）

- `Audio::Initialize()`と`Finalize()`に対になる`MFStartup`と`MFShutdown`を実装する
- 成功状態を保持し、失敗時や複数回終了時の不正な`MFShutdown`を防ぐ
- COMの所有権は`WinApp`から移動しない

完了条件:

- `MFStartup`と`MFShutdown`が必ず1対1になる
- 既存XAudio2初期化・終了を壊さない

### Task D: PCMデコーダー（担当: 中核実装）

- Source Readerで最初の音声ストリームをPCMへデコードする
- `ComPtr`と局所RAIIでMedia Foundation資源を管理する
- `ReadSample`のフラグ、終端、メディアタイプ変更、空サンプルを適切に処理する
- `DWORD`、`size_t`、`UINT32`間の変換で警告やオーバーフローを起こさない
- 成功時だけ完成した`SoundData`を返す

完了条件:

- WAVと圧縮音声が同じ`SoundData`形式になる
- 音声のないMP4でクラッシュしない
- 途中失敗でロック中バッファやCOM資源が残らない

### Task E: 公開Loadと互換処理（担当: 中核実装）

- ファイル名だけで使える`Load(filename, group)`を追加する
- 明示名付き`Load(name, filename, group)`を追加する
- `LoadWave(name, filename, group)`を新実装へ委譲する
- 同じキーが読み込み済みなら既存どおり再読み込みしない
- 既存`PlayWave`、フェード、グループ制御を変更しない

完了条件:

- `Load("bgm.mp4", SoundGroup::BGM)`後に`PlayWave("bgm.mp4", true)`で参照できる
- 既存の`LoadWave`呼び出しがビルドできる

### Task F: フィルター確認（担当: Gemini可）

- `UpdateFilters.py`を確認モードとして使える場合だけ実行する
- 今回は既存Audioファイルだけを変更するため、通常はfilters差分がないことを確認する
- 自動生成結果に無関係な差分が出た場合は適用しない

完了条件:

- 必要なファイルがVisual Studio上で`audio`フィルターに存在する
- 無関係なfilters変更がない

### Task G: ビルド・実行確認（担当: Gemini可、最終判定は実装担当）

1. `GameTemplate.sln`を`Debug|x64`でビルドする
2. 可能なら`Release|x64`もビルドする
3. WAV、MP3、音声付きMP4を1つずつロード・再生する
4. ループ、停止、音量、フェードが従来どおり動くことを確認する
5. 存在しないファイル、音声のないMP4、非対応コーデックでクラッシュしないことを確認する

記録する内容:

- 実行コマンド
- 構成
- 終了コード
- 最初のエラーまたは警告
- 実行確認したファイル形式と結果

## 8. Geminiへ任せない方がよい作業

- COMとMedia Foundationの所有権・寿命の変更
- `ReadSample`ループとバッファのロック解除処理
- `HRESULT`失敗時の制御フロー
- `WAVEFORMATEX`の所有権とサイズ管理
- ストリーミング再生への拡張
- 既存公開APIの削除・改名
- `engine/`のコミット、push、親リポジトリのサブモジュール参照更新

## 9. 実装時の注意点

- `MFStartup`はワーカースレッドから呼ぶ設計に変更しない
- `IMFMediaBuffer::Lock`成功後は、すべての経路で`Unlock`する
- PCM合計サイズが`XAUDIO2_BUFFER::AudioBytes`の表現範囲を超える場合は読み込みを失敗させる
- 全デコード方式なので、長尺・高音質BGMはメモリ使用量が大きくなる
- 既存コードの文字化けコメントや無関係な整形には触れない
- 新規・変更ファイルはCRLFを維持する
- Engine変更と親リポジトリ変更は別コミットにする。ただし、commitやpushはユーザーの明示依頼があるまで行わない

## 10. 完了条件

- ファイル名だけでApplication側のaudioリソースを読み込める
- WAV、MP3、音声付きMP4をPCMへデコードしてXAudio2で再生できる
- 既存のWAV読み込み呼び出しが引き続き利用できる
- 既存の再生、ループ、音量、フェード、グループ制御に回帰がない
- `GameTemplate.sln`の`Debug|x64`ビルドが成功する
- 実行時の読み込み・再生を確認する
- 親リポジトリとEngineサブモジュールの変更範囲を分けて報告できる

## 11. 対象外として残す次期タスク

- 長尺BGM向けストリーミングデコード
- 再生位置取得とシーク
- 非同期ロード
- デコード済みPCMの容量制限とLRUキャッシュ
- 公開API名を`Play`、`Stop`、`Unload`へ統一する整理
