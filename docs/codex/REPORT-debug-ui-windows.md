# デバッグUIウィンドウ化 報告

## 変更したファイルと内容

### engine

- `manager/editor/DebugUIManager.h`, `manager/editor/DebugUIManager.cpp`
  - 登録されたUIを、それぞれ `ImGui::Begin(name, &visible)` を持つ独立ウィンドウとして描くようにした。
  - 名前、分類、初期ドッキング先、初期表示を一つの設定表へまとめた。表にないUIは `Other`、初期表示は開く、登録時の `area` に対応する場所へドッキングする。
  - Windowメニューの分類を設定表から作り、閉じるボタンとメニューが同じ `visible` を更新するようにした。
  - 既存の `SavedUIState` と `imgui.ini` を使い、各UIとConsoleの表示状態を保存するようにした。
  - `Sequencer Gizmo` だけは独立ウィンドウにせず、Sceneウィンドウ内で余白や見出しを足さずに描くようにした。
  - 各独立ウィンドウで文字をウィンドウ幅に合わせて折り返すようにした。
- `manager/editor/GameObjectEditor.h`, `manager/editor/GameObjectEditor.cpp`
  - 一覧を `GameObject List`、詳細編集を `GameObject Inspector` に分けて登録した。
- `manager/editor/ConsoleLog.cpp`
  - 横スクロールをなくし、長いログをウィンドウ幅で折り返すようにした。

### 親リポジトリ

- `application/MyGame.cpp`
  - 旧Hierarchy、Inspector、Projectの集合ウィンドウとToolsメニューを削除した。
  - Windowメニューへ分類済みUI一覧を組み込んだ。
  - 下段35%、左、中央Scene、右上、右下の初期ドッキング配置へ変更した。
  - 設定表から実在する登録名だけをドッキングし、下段の初期選択を `Sequencer` にした。
  - Sceneへスクロール禁止フラグを付けた。

新しいソースファイルは追加していないため、`.vcxproj` と `.vcxproj.filters` の変更はない。

## 名前から決まる設定

| UI名 | 分類 | 初期ドッキング先 | 初期表示 |
|---|---|---|---|
| GameObject List | Scene | 左 | 開く |
| SceneManager | Scene | 左 | 開く |
| Title Scene | Scene | 左 | 開く |
| Feature Check | Scene | 左 | 開く |
| GameObject Inspector | Scene | 右上 | 開く |
| Sequencer Inspector | Sequencer | 右上 | 開く |
| Camera Manager | Scene | 右上 | 開く |
| Light Manager | Scene | 右上 | 開く |
| Cutscene | Sequencer | 右上 | 開く |
| Post Process | Rendering | 右下 | 開く |
| Atmosphere Fog | Rendering | 右下 | 開く |
| Light Beams | Rendering | 右下 | 開く |
| Outline | Rendering | 右下 | 開く |
| NPR Shading | Rendering | 右下 | 開く |
| Render Pipeline | Rendering | 右下 | 開く |
| Sequencer | Sequencer | 下 | 開く、最初のタブ |
| Console | System | 下 | 開く |
| Shader Hot Reload | Rendering | 下 | 開く |
| Time Manager | System | 右上 | 閉じる |
| Timer Manager | System | 右上 | 閉じる |
| Debug Camera | Scene | 右上 | 閉じる |
| TopDownCamera Settings | Scene | 右上 | 閉じる |
| Audio Debug | System | 下 | 閉じる |
| JSON Editor | System | 下 | 閉じる |
| Particle Editor | Effects | 右上 | 閉じる |
| Particle Manager | Effects | 右上 | 閉じる |
| CollisionManager Colliders | System | 下 | 閉じる |
| `Font Sprite:` で始まる名前 | System | 下 | 閉じる |
| Sequencer Gizmo | Other | Scene内へ直接描画 | 開く |
| 表にないUI | Other | 登録時のareaに対応 | 開く |

## 確認した内容

- 作業開始時、親とengineの両方で `git merge-base --is-ancestor feature/sequencer HEAD` が `YES` だった。
- 共有ワークツリーの追跡外 `application/Resources/json/sequence/sequence.json` には触れていない。
- 共有側からDirectXTexの生成済み `.inc` 14個を作業ワークツリーへコピーしてビルドに使用した。これらは除外対象でコミットしていない。
- `GameTemplate.sln` を `Debug|x64` でビルドし、終了コード0。警告はONBOARDINGに記載された既存のリモート配置警告3件だけで、新しい警告はなかった。
- 最終コードをリポジトリルートから30秒動作させ、途中終了しなかった。D3D12 ERROR / CORRUPTION による停止は0件だった。
- PrintWindow `PW_RENDERFULLCONTENT` で撮影し、次を画像で確認した。
  - 左、中央Scene、右上、右下、下段約35%の配置になっている。
  - 下段でSequencerが最初に選ばれている。
  - Sceneウィンドウにスクロールバーやギズモ用の見出しがない。
  - 表示中のUI本文が各ウィンドウ内に収まっている。

撮影画像:

`C:\Users\kurih\source\repos\kuriharakento\GameTemplate-debugui\generated\capture\debug_ui_final_30.png`

## 人間の確認が必要なこと

- 各ウィンドウの閉じるボタンとWindowメニューのチェックが相互に同期すること。
- Windowメニューから閉じたUIを再度開けること。
- マウスでウィンドウを別の場所へ付け替えられること。
- 再起動後も変更した表示状態とドッキング配置が復元されること。
- 実際の操作感と文字の読みやすさが期待どおりであること。

## コミット

- engine: `a9f48830af12dd3b8640cd7954483d904e6e5ae6`

