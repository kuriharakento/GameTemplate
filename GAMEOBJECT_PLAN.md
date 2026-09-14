# GameObject / コンポーネント刷新 計画書

- 対象リポジトリ: `GameTemplate`（アプリ） / `KentoCompoEngine`（`engine/` サブモジュール）
- 作成日: 2026-09-14
- 目的: Unity の MonoBehaviour に近い書き心地でゲームの挙動を作れるようにし、プレハブで組み合わせを使い回せるようにする

---

## 1. 目的とスコープ

### 1.1 目的

1. コンポーネントに寿命の関数（Awake / OnEnable / Start / Update / LateUpdate / OnDisable / OnDestroy）と `enabled` を持たせる
2. GameObject とコンポーネントの組み合わせを、プレハブ（JSON）として保存・複製できるようにする

### 1.2 スコープ外（当面）

- Transform の作り直し（ワールド/ローカル、クォータニオン化、順番付きの子リスト）
- `engine/ecs/`（別の仕組みなので触らない）
- シーケンサ側の変更（GameObject 連携 A/B/C はこの計画の後）

---

## 2. 現状分析

| 項目 | 今の作り | 問題 |
|---|---|---|
| 基底 | `IGameObjectComponent` に `Update(GameObject* owner)` だけ | 初期化・後片付けの置き場が無い |
| 派生 | `IActionComponent`（Draw3D/Draw2D）と `ICollisionComponent` の仮想継承 | GameObject が2本の配列を持ち、`dynamic_pointer_cast` で振り分けている |
| 所有 | `unordered_map<string, shared_ptr>` | 所有者が曖昧。取得順が実質ランダム |
| 追加 | 名前文字列で `AddComponent(name, unique_ptr)` | 型で引けない |
| 有効/無効 | コライダーだけ自前の `isActive_` | 共通の仕組みが無い |
| 衝突通知 | `SetOnEnter` などの `std::function` | 宣言以外で誰も設定していない。Behaviour 側で受けられない |
| 衝突登録 | `ICollisionComponent` のコンストラクタ/デストラクタで `CollisionManager::Register/Unregister` | 生成時に持ち主が必須。無効化しても判定から外れない |
| 生成 | `ComponentFactory` + `REGISTER_COMPONENT`、`GameObjectEditor::AddComponentByName` | 土台として使える |
| 保存 | `JsonEditableBase` + `REGISTER_MEMBER` | 土台として使える |

影響範囲:

- GameObject の派生クラス: 0
- コンポーネント実装: 6（engine のコライダー4、application の `PhysicsComponent` / `StatusComponent`）
- `GetComponent` / `CreateGameObject` / `REGISTER_COMPONENT` の呼び出し: 26か所

---

## 3. コンポーネントの寿命

### 3.1 クラス構成

```
Component（抽象。持ち主・enabled・寿命の関数）
├─ Behaviour（中身を書く部品。Draw3D / Draw2D を持つ）
│   ├─ PhysicsComponent
│   └─ StatusComponent
└─ Collider（形を持つ部品。4 章）
    ├─ AABBCollider / OBBCollider / SphereCollider / RayCollider
```

- 古い `IGameObjectComponent` / `IActionComponent` / `ICollisionComponent` は残さず、全部こちらへ移す
- 寿命の関数は全部「空の仮想関数」。使うものだけ上書きする

### 3.2 呼ばれる時

| 関数 | 呼ばれる時 | 用途 |
|---|---|---|
| `Awake` | 追加直後に1回 | 自分の中の準備 |
| `OnEnable` | 有効になった時（Awake 直後も） | 管理役への登録 |
| `Start` | 最初の Update の直前に1回 | 他のコンポーネントを探す |
| `Update` | 毎フレーム | 挙動 |
| `LateUpdate` | 全オブジェクトの Update の後 | 追従カメラなど |
| `OnDisable` | 無効化・破棄の直前 | 登録解除 |
| `OnDestroy` | 破棄時 | 後片付け |
| `OnCollisionEnter/Stay/Exit` | コライダーの組ごと（Unity と同じ） | どの判定が当たったかは `info.self` で見分ける |
| `OnObjectCollisionEnter/Stay/Exit` | 相手の GameObject ごとに1回（独自） | 判定が複数あっても1回だけ処理したい時 |

- 「有効」= コンポーネントの `enabled` と、GameObject（親も含む）の `IsActive()` が両方真
- `GameObject::SetActive(false)` で全コンポーネントに `OnDisable` を送る

### 3.3 1フレームの流れ

1. `GameObjectManager::Update` が全オブジェクトの Update を回す（未 Start なら直前に Start）
2. 全オブジェクトの LateUpdate を回す
3. 保留していた追加・削除を処理し、破棄予定を片付ける（今の保留の仕組みを流用）

### 3.4 所有と取得

- `GameObject` が `std::vector<std::unique_ptr<Component>>` で所有（追加順）
- `T* AddComponent<T>(args...)` / `T* GetComponent<T>()` / `GetComponents<T>(std::vector<T*>& out)`
- 同じ型が複数あれば `GetComponent<T>()` は先に追加したものを返す
- 返す `T*` は所有しない。破棄はフレーム末尾なので、そのフレーム内は安全
- 名前文字列での追加は廃止。種類名から作るのは Factory（エディタ・プレハブ）経由だけ

---

## 4. コライダー

- 今の `ICollisionComponent` の中身（レイヤー、マスク、サイズ補正、サブステップ、前フレーム位置、`GetColliderType()`、`GetBroadphaseAABB()`）を `Collider` へ移す
- `OnEnable` で `CollisionManager::Register`、`OnDisable` で `Unregister`
  - コンストラクタに持ち主を渡さなくてよくなる
  - 無効化・非アクティブで自動的に判定から外れる
- 自前の `isActive_` は `enabled` に統合
- `SetOnEnter` などのコールバックは削除
- `CollisionManager` は今 `pair.a->CallOnEnter(infoA)` の所で、両方の GameObject の有効な全コンポーネントに `OnCollisionEnter(info)` を送る（Stay / Exit も同じ）
- `CollisionInfo` に「自分のコライダー」（`self`）を足し、どの判定に当たったかを分けられるようにする
- 見分けは、`AddComponent` の戻り値を持っておいて `info.self` と比べる（ラベルは足さない）
- 判定は1つの GameObject に並べて付け、各コライダーの「中心のずらし量」（Unity の `Collider.center`）で置き分ける。判定専用の子オブジェクトは既定の立方体モデルが描かれるので使わない
- Sphere / AABB / Ray は今 `owner->GetPosition()`（ローカル）を中心にしている。OBB と同じく持ち主のワールド行列から取るように揃える
- `OnObjectCollisionXxx` のために、GameObject の組ごとに「今触れているコライダーの組の数」を数える表を持つ（毎フレームの確保は使い回しで避ける）

### 4.1 レイの修正

今のレイは何にも当たらない（`CollisionManager` の判定表でレイの行・列が空）。今回の移行で次を直す。

| # | 問題 | 直し方 |
|---|---|---|
| 1 | 判定表に Ray×AABB / OBB / Sphere が未登録で、どれとも当たらない | 両方の並び順（Ray×X と X×Ray）で登録する |
| 2 | `Draw()` / `Init()` が誰にも呼ばれず、判定線が出ない | 他の形と同じく Update で線を描く。`Init()` は Awake / OnEnable に置き換えて削除 |
| 3 | `SetWorldDirection` の方向を正規化していない（判定は単位ベクトル前提） | 方向は常に正規化。長さ 0 なら前方（+Z） |
| 4 | 始点・向きが持ち主のローカル位置・角度（親を無視） | 持ち主のワールド行列から取る |
| 5 | かすった時の扱いが AABB（`<=` で外れ）と OBB（`<` で当たり）で違う | 「当たり」に揃える |
| 6 | ECS 側の `RayvsAABB_W` が `nullptr` を渡していて、呼ばれたら落ちる | 削除（未使用） |

### 4.2 Raycast（その場で撃つ）

付けっぱなしのセンサー（RayCollider）とは別に、Unity の `Physics.Raycast` 相当を足す。

- `bool CollisionManager::Raycast(const Ray& ray, uint32_t mask, RaycastHit& outHit) const`
- 当たった中で一番近いものを返す。`RaycastHit` は相手の GameObject・コライダー・距離・位置を持つ
- 無効なコライダーと非アクティブな GameObject は対象外
- 候補の絞り込みはブロードフェーズのグリッドを使い回す。呼び出しごとの動的確保はしない
- レイ同士は判定しない

### 4.3 クラス名

Unity に合わせて `Component` の語尾を取る（`SphereColliderComponent` → `SphereCollider` など4つ）。保存済み JSON の古い種類名も読めるよう、Factory に旧名を別名として登録する。

### 4.4 トンネリング対策の確認結果（2026-09-14）

仕組み: `useSubstep` のコライダーは、前フレームの位置から今の位置までを一定間隔で区切り、途中の各位置で判定する（`CollisionAlgorithm.cpp` の `*Substep*`）。押し出し量は、最初に当たった途中位置から逆算する。

| # | 問題 | 影響 | 根拠 |
|---|---|---|---|
| 1 | （問題ではない）`CollisionManager::CheckCollisions()` / `UpdatePreviousPositions()` は、衝突を使うゲームのシーンだけが呼ぶ設計。このリポジトリのシーンは衝突を使わないので呼び出しが無い | — | ユーザー確認済み |
| 2 | ブロードフェーズが今の形の箱だけで組を作る | 1フレームで壁の向こうへ抜けて別のセルに入ると組が作られず、細かい判定まで行かない。トンネリング対策の一番の穴 | `CollisionManager.cpp:259` |
| 3 | 区切る間隔が 1.0 固定（OBB の一部は 0.25） | 厚さ 1 未満の壁や小さい物は、区切りの間ですり抜ける | `CollisionAlgorithm.cpp:551` など |
| 4 | Sphere 同士の細かい判定は、当たった後の押し出し量を「今の位置」で計算している | すり抜けた後だと押し出し量が 0 になり、押し戻せない | `CollisionManager.cpp:123-129` |
| 5 | Sphere / AABB は生成時に前フレーム位置を入れていない（0,0,0 のまま） | フレームの途中で出した弾は、原点から今の位置まで掃いたことになり、線上の物に誤って当たる | `SphereColliderComponent.cpp` / `AABBColliderComponent.cpp` のコンストラクタ |
| 6 | 前フレーム位置はワールド座標、Sphere / AABB の中心はローカル座標 | 子オブジェクトだと掃く線がずれる（4 章の「ワールド行列から取る」で解消） | `CollisionManager.cpp:430` |
| 7 | 細かい判定の関数が表に無い組み合わせ（Ray など）は、普通の判定にも戻らない | `useSubstep` を付けると当たらなくなる | `CollisionManager.cpp:323-329` |
| 8 | 2D 用の細かい判定は、どこからも使われていない。使うと `AABBColliderComponent(nullptr)` が null を参照して落ちる | 今は実害なし | `dimension_` が読まれていない、`CollisionAlgorithm.cpp:1445` |

直し方（2〜8 はすべて今回の移行に含める）:

- 1: 直さない。シーンから呼ぶ設計のまま。呼ぶ順番（`UpdatePreviousPositions` → GameObject の Update / LateUpdate → `CheckCollisions`）を `CollisionManager.h` のコメントに書いておく
- 2: `useSubstep` のコライダーは、前の位置から今の位置までを覆う箱でセルに登録する
- 3: 区切る間隔を、2つの形の小さい方の半分の大きさから決める（上限回数は定数で抑える）
- 4: Sphere 同士にも途中位置から押し出し量を出す版を足す
- 5: `OnEnable` で前フレーム位置を今の位置で初期化する。瞬間移動用に `ResetPreviousPosition()` も用意する
- 7: 細かい判定の関数が無い組み合わせは、普通の判定に戻す
- 8: 2D 用の細かい判定と `dimension_` は削除（未使用）

---

## 5. プレハブ

### 5.1 形式

```json
{
  "version": 1,
  "name": "Enemy",
  "tag": "Enemy",
  "transform": { "scale": [1,1,1], "rotate": [0,0,0], "translate": [0,0,0] },
  "model": "enemy.obj",
  "components": [
    { "type": "SphereCollider", "enabled": true, "fields": { "radius": 1.0 } },
    { "type": "PhysicsComponent", "enabled": true, "fields": { "...": "REGISTER_MEMBER した値" } }
  ],
  "children": [ { "...同じ形...": "" } ]
}
```

- `fields` は各コンポーネントの `REGISTER_MEMBER` をそのまま使う
- 子オブジェクトも入れる
- 置き場所: `application/Resources/json/prefab/`

### 5.2 API とエディタ

- `GameObjectManager::Instantiate(prefabPath, transform)` → 生成して `GameObject*` を返す（所有はマネージャー）
- GameObjectEditor に「プレハブとして保存」「プレハブから生成」を足す
- 生成時の GUID は毎回新しく振る（プレハブ自体には保存しない）

---

### 5.3 実装メモ（プレハブ作業で使う）

- 今ある `GameObject::SaveJson` / `LoadJson`（`GameObject.cpp:632` 付近）は、コンポーネントを「種類名 → 中身」のオブジェクトで持つ。同じ種類を2つ持てないので、プレハブは 5.1 の配列の形にする。`SaveJson` / `LoadJson` の形式は変えない（既存データを壊さない）
- 種類名からの生成は `ComponentFactory::Create(typeName, owner)` を使う。`fields` の読み書きは各コンポーネントの `JsonEditableBase::Serialize` / `Deserialize`（`REGISTER_MEMBER` の値）を使う
- モデル名はプレハブに入れる。モデルを持たないオブジェクトは `"model"` を省く
- `Instantiate` は、読み込めない種類名や壊れた JSON を見たらログを出して `nullptr` を返す。途中まで作ったオブジェクトは残さない
- 確認は DebugScene で行う: 体と足元の2判定を持つオブジェクトをプレハブに保存 → 2個生成 → 両方で衝突通知が出ること、GUID が別々なこと
- 衝突の Exit 中の登録解除（`CollisionManager`）は Claude が別に直すので触らない

## 6. 進め方

区切りごとに、ビルド（Debug|x64）→ 動作確認（衝突まわりは DebugScene、それ以外は FeatureCheck）→ engine → 親の順でコミット。

1. **寿命の仕組み**: Component / Behaviour / Collider、6個の移行、`AddComponent<T>` / `GetComponent<T>`、LateUpdate、衝突通知、レイの修正と Raycast、トンネリング対策の修正（4.1〜4.4）
   - 確認は新しく作る `application/scene/debug/DebugScene` で行う。Update で `CollisionManager::UpdatePreviousPositions` → `GameObjectManager::Update` → `CheckCollisions` の順に呼ぶ（衝突を使うシーンの手本も兼ねる）
   - 置くもの: 薄い壁と速い弾（すり抜け）、体と足元の2判定を持つオブジェクト（`info.self` と `OnObjectCollisionEnter`）、箱・球・OBB に向けたレイと `Raycast`、当たった回数と相手を出す ImGui 欄
   - 追加するファイルは .vcxproj と .filters の両方に入れ、既存シーンと同じ方法で登録する
2. **プレハブ**: 保存・読み込み・`Instantiate`・エディタのボタン
3. （その後）シーケンサとの GameObject 連携 A / B / C

---

## 7. 未決事項

- 今のところ無し。実装中に出てきたらここに足す

---

## 8. シーケンサとの連携（B / C）

1〜2 は取り込み済み（engine `a4dde62` 以降 / `feature/sequencer`）。A（GameObject にシーケンスを持たせてゲームから再生）は CutsceneManager で足りているのでやらない。

### 8.1 B: GameObject から直接トラックを作る

- **置き場所**: GameObject の Inspector の下に「Sequencer」欄を足す。`SequencerEditor` が `DebugUIManager::RegisterInspector(this, SelectionKind::GameObject, ...)` で登録する（同じ種類の Inspector は登録順に続けて描く仕組みが既にある。BeamRenderer がスポットライトの欄を足しているのと同じ形）。GameObjectEditor 側からシーケンサを知らないようにするため
- **ボタン**:
  - 「Transform トラックを作る」: Transform トラックを追加し、役の名前をオブジェクト名にして、プレビュー用の割り当て（`previewObjectBindings_`、GUID で持つ）もこのオブジェクトにする。同じ役名がすでに別のオブジェクトに割り当てられていたら、名前に番号を足して重ならないようにする
  - 「Component トラックを作る」: 8.2 のトラックを同じ手順で作る（コンポーネントと項目は Inspector で選ぶ）
  - 「カメラの注目点 A にする」「B にする」: 選択中（無ければ最初）のカメラトラックの Aim Role A / B に役を入れ、プレビュー割り当ても済ませる。カメラトラックが無ければボタンを無効にして理由を出す
- **Undo**: トラックの追加は今の `AddTrack` と同じく `SequenceStructureCommand`。注目点の設定は `TrackEditCommand`。追加と割り当てを1回の Undo で戻せるよう、`CommandHistory` のトランザクションでまとめる
- 作ったトラックを選択して、Sequencer でそのまま編集を続けられるようにする

### 8.2 C: コンポーネントの値をトラックで動かす

- **新しいトラック** `ComponentTrack`（種別名 `"Component"`、`TrackType` に `Component` を足す。JSON は文字列で保存しているので末尾に足せば既存データは壊れない）
  - 設定: 役（GameObject）、コンポーネントの種類名（Factory の正規名。`GameObject::GetComponentTypeNames()` と同じ名前）、項目名（`JsonEditableBase` に登録された名前）
  - チャンネル: 項目の値の型に合わせて1本。数値 → `FloatCurve`、`{x,y,z}` → `Vector3Curve`、`{x,y,z,w}`（色）→ `Vector4Curve`。それ以外の型は選べないようにする
  - もう1本「Enabled」チャンネル（`FloatCurve`、0.5 以上でオン）で、コンポーネントのオン・オフを動かす。キーの補間は既定で Constant にする
- **Evaluate**: 役から GameObject を引き、種類名でコンポーネントを探し、`JsonEditableBase::SetValue(項目名, 値)` で書く。キーの無いチャンネルには触らない。ITrack の純関数契約を守る（前フレームの値を土台にしない）
  - 毎フレーム JSON を作るので確保が起きる。前に書いた値と同じなら書かない（書く値の結果は変わらないので純関数のまま）など、無駄な書き込みを減らす工夫を入れる。気になる規模なら後で測る
- **CaptureState / RestoreState**: 書き換える前の項目の値（JSON）と Enabled を退避して戻す
- **RecordKey**: 今の値をキーとして打つ（`Serialize()` から項目を読む）
- **Inspector**: 役・コンポーネント・項目を選ぶコンボ。コンポーネントと項目の一覧は、役にプレビュー割り当て中のオブジェクトから作る。割り当てが無ければ文字で入力できるようにする
- 追加するファイル（`sequencer/track/ComponentTrack.h/.cpp`）は engine の .vcxproj と .filters の両方に入れ、`TrackFactory` に登録する

### 8.3 進め方

- 作業ブランチ: `feature/gameobject-sequencer`（engine と親の両方）。`git worktree add` で別ディレクトリに作り、`engine/externals/DirectXTex/Shaders/Compiled/*.inc`（14個）をコピーしてからビルドする
- B → C の順に、止まらずに全部やる。区切りごとに Debug|x64 でビルド（終了コード 0）→ engine → 親の順でコミット。最後に Release|x64 もビルドする
- 確認用に FeatureCheck か DebugScene へ最小の確認を足してよい（例: Component トラックでライトの強さや色を動かす）。起動確認と見た目の確認は Claude とユーザーがやる
- `CollisionManager` は触らない
