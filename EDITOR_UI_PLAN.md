# エディタ UI（DebugUIManager）作り直し 計画書

- 対象リポジトリ: `GameTemplate`（アプリ） / `KentoCompoEngine`（`engine/` サブモジュール）
- 作成日: 2026-09-14
- 目的: 仕組みごとにウィンドウが散らばった今の ImGui を、Unity のエディタのように「選ぶ場所」「選んだ物を直す場所」「全体の設定」に分け直して、見て分かる画面にする

---

## 1. 目的とスコープ

### 1.1 目的

1. Inspector を1つにまとめ、`SelectionContext` で今選んでいる物の詳細を出す
2. 描画設定などの「全体の設定」を、新しく作る Settings ウィンドウにまとめる（左に項目一覧、右に中身）
3. ウィンドウの置き場所の決め方を、登録時の指定に一本化する（名前の表と名前での特別扱いをやめる）

### 1.2 スコープ外（当面）

- 各仕組みの `DrawImGui()` の中身（呼ぶ場所を変えるだけで、中身は書き直さない）
- タイムラインの使い勝手（範囲選択、コピー、制御点のドラッグなど）。この計画の後に別で扱う
- ImGui 本体や ImGuizmo の更新

### 1.3 前提

- GameObject まわり（`GameObjectEditor`、`CollisionManager` の登録行）は、別作業 `GAMEOBJECT_PLAN.md`（Codex 担当）が触る。**その作業が `feature/sequencer` に取り込まれてから着手する**

---

## 2. 現状分析

### 2.1 仕組み

- `RegisterDebugUI(owner, name, drawFunc, area)` で、仕組みごとに1つずつウィンドウを登録する（全部で約30個）
- 保存は `unordered_map<void*, std::vector<DebugUI>>`
- 置き場所は `DebugUIManager.cpp:53-84` の名前の表 `kWindowConfigs`（名前・分類・ドッキング先・初期表示）で決まる。表に無いものだけ `area` から決まり、初めて開くときは画面中央に浮く
- 初期配置は `application/MyGame.cpp:225-253` が `GetDockWindowNames()` を使ってドッキングを組む
- 表示のオン・オフと area は imgui.ini に保存される

### 2.2 問題

| # | 問題 | 根拠 |
|---|---|---|
| 1 | Inspector が「選んだ物の詳細」になっていない。GameObject Inspector、Sequencer Inspector、Camera Manager、Light Manager などがタブで並び、キーを選んでも Sequencer Inspector が裏のタブだと何も起きないように見える | `kWindowConfigs` の RightTop |
| 2 | 置き場所の決まりが二重。登録時の area と名前の表のどちらでも決まり、名前を変えると表も直す必要がある | `GetDockLocation()` |
| 3 | 名前の文字列で特別扱いしている（`"Sequencer Gizmo"` を Scene 内で描く、`"Font Sprite:"` を前方一致で判定） | `DebugUIManager.cpp:261, 280, 328`, `:83` |
| 4 | `unordered_map` なので、ウィンドウとメニューの並びが起動ごとに変わる | `debugUIs_` |
| 5 | `showHierarchy_` / `showInspector_` / `showProject_` を持っているが、`Draw()` で使っていない | `DebugUIManager.h:195-198` |
| 6 | 描画設定が右下と下の段のタブに散らばり、表に無いもの（Anti-Aliasing、Depth Of Field、Volumetric Light、Floor Reflection、3D Text、Shadow Maps）は浮いて出るか「Other」に入る | `Framework.cpp` の登録と `kWindowConfigs` |

### 2.3 登録されているウィンドウと移し先

| 今のウィンドウ | 移し先 |
|---|---|
| GameObject List | Hierarchy |
| SceneManager、Title Scene、Feature Check | Hierarchy の下に、シーンごとのパネルとしてタブで並べる |
| Light Manager、Camera Manager | 一覧は Hierarchy、選んだライト・カメラの詳細は Inspector |
| GameObject Inspector、Sequencer Inspector | Inspector（選択の種類で切り替え） |
| Sequencer | Project（下の段） |
| Cutscene | Project の隣のタブ（Sequencer と一緒に使うので今のまま） |
| Post Process、Atmosphere Fog、Light Beams、Outline、NPR Shading、Anti-Aliasing、Depth Of Field、Volumetric Light、Floor Reflection、3D Text、Shadow Maps | Settings / Rendering |
| Time Manager、Timer Manager、Audio Debug | Settings / System |
| Debug Camera、TopDownCamera Settings | Settings / Camera |
| Particle Manager | Settings / Effects |
| Particle Editor、JSON Editor | 道具ウィンドウ（下の段。初期は非表示） |
| Render Pipeline、Shader Hot Reload、CollisionManager Colliders、Font Sprite:* | 道具ウィンドウ（下の段。初期表示は今の表どおり） |
| Console | 下の段（今のまま） |
| Sequencer Gizmo | Scene へのオーバーレイ（名前でなく種類で扱う） |

---

## 3. 新しい画面構成

```
┌──────────┬────────────────────────┬──────────────┐
│Hierarchy │ Scene                  │ Inspector    │
│ GameObj  │                        │ （選んだ物）  │
│ Lights   │                        │              │
│ Cameras  │                        ├──────────────┤
│──────────│                        │ Settings     │
│シーンパネル│                        │ 左:項目 右:中身│
├──────────┴────────────────────────┴──────────────┤
│ Project（Sequencer / Cutscene） | Console | 道具          │
└──────────────────────────────────────────────────┘
```

| 場所 | 役目 |
|---|---|
| Hierarchy | シーンの物を選ぶ。GameObject（木）、ライト、カメラ |
| Inspector | `SelectionContext` の主対象の詳細。種類ごとに登録された描画を呼ぶ。何も選んでいなければ操作の案内を出す |
| Settings | 全体の設定。左に分類つきの項目一覧（Rendering / System / Camera / Effects）、右に選んだ項目の中身 |
| Project | 横に長い作業画面（Sequencer、Cutscene） |
| Console / 道具 | ログと、不具合の切り分け用の道具 |
| Scene | ゲーム画面とギズモ |

---

## 4. 登録 API

名前の表をやめ、登録時に種類と置き場所を渡す。

```cpp
// 選んだ物の詳細（Inspector）。選択の種類ごとに1つ
void RegisterInspector(void* owner, SelectionKind kind, std::function<void(const SelectionItem&)> draw);

// 全体の設定（Settings）。category は "Rendering" など、name は項目名
void RegisterSettingsPage(void* owner, const std::string& category, const std::string& name, std::function<void()> draw);

// 独立したウィンドウ（Hierarchy のパネル、Project、道具）
void RegisterWindow(void* owner, const std::string& name, EditorDock dock, std::function<void()> draw, bool defaultVisible = true);

// Scene 画像の上に重ねる描画（ギズモなど）
void RegisterSceneOverlay(void* owner, std::function<void()> draw);

void Unregister(void* owner);
```

- 登録は `std::vector` に登録順で持つ。並びが毎回同じになる。`Unregister` は owner で消す
- `EditorDock` は `Left / Right / RightBottom / Bottom` の置き場所。今の `DebugUIDockLocation` を置き換える
- 旧 `RegisterDebugUI` と `DebugUIArea`、`kWindowConfigs`、名前での特別扱い、未使用の `showHierarchy_` などは削除する。呼び出し元は全部新しい API に移す（engine 約30か所、application 2か所）
- imgui.ini に保存するのは「道具ウィンドウの表示のオン・オフ」「Settings で選んでいる項目」「UI の拡大率」。area の保存は無くなる

---

## 5. Inspector の切り替え

- `SelectionContext::GetPrimary().kind` を見て、その種類に登録された描画を呼ぶ
- 何も選んでいないときは「Hierarchy で物を選ぶか、Sequencer でトラックやキーを選ぶと、ここに詳細が出ます」と出す
- 種類ごとの担当:
  - `GameObject` → GameObjectEditor（今の GameObject Inspector の中身）
  - `SequenceTrack` / `SequenceKey` → SequencerEditor（今の Sequencer Inspector の中身）
  - `Light`（新規） → LightManager（今の Light Manager の詳細部分）
  - `Camera`（新規） → CameraManager（今の Camera Manager の詳細部分）
- `SelectionKind` に `Light` と `Camera` を足す。`SelectionItem` にはライトの種類と名前、カメラの名前を持たせる（ポインタでは持たない。今の GameObject が GUID で持つのと同じ理由）
- Light Manager と Camera Manager の `DrawImGui()` は「一覧」と「詳細」に分ける。一覧は Hierarchy、詳細は Inspector へ

---

## 6. Settings ウィンドウ

- 左に分類ごとの木（`TreeNode`）、右に選んだ項目の `draw()` を呼ぶ
- 左の一覧は幅を固定し、右は `BeginChild` で縦にスクロールする
- 項目名で絞り込む検索欄を上に置く
- 各仕組みは `RegisterSettingsPage(this, "Rendering", "Depth Of Field", ...)` を1行呼ぶだけにする

---

## 7. 進め方

区切りごとに、ビルド（Debug|x64）→ 起動して見た目を確認（ユーザー）→ engine → 親の順でコミット。ImGui の画面はシーンの描画先に入らないので、自動の画像確認はできない。見た目はユーザーに確認してもらう。

1. **土台**: 新しい登録 API、登録順の保存、名前の表と特別扱いの削除、`MyGame.cpp` の初期配置を新しい置き場所に合わせる
2. **Settings**: Settings ウィンドウを作り、2.3 の Settings 行を移す
3. **Inspector**: 1つの Inspector にまとめ、GameObject とシーケンサーを移す
4. **Hierarchy**: GameObject・ライト・カメラの一覧を作り、`SelectionKind` に Light / Camera を足して Inspector に詳細を出す
5. （その後）タイムラインの使い勝手

---

## 8. 未決事項

- 今のところ無し。実装中に出てきたらここに足す
