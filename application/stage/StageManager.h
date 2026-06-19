#pragma once

// system
#include "graphics/3d/Object3dCommon.h"
#include "graphics/2d/SpriteCommon.h"
#include "manager/scene/CameraManager.h"
#include "manager/scene/LightManager.h"

// app
#include "Stage.h"
#include "StageData.h"
#include "application/gameObject/combatable/character/enemy/EnemyManager.h"
#include "application/gameObject/combatable/character/player/Player.h"
#include "application/gameObject/obstacle/ObstacleManager.h"
#include "engine/ecs/Registry.h"
#include "engine/ecs/Entity.h"
#include "engine/ecs/system/SystemManager.h"
#include <unordered_map>
class PostProcessManager;
class ShadowMapManager;
class InstancedModelRenderer;

/**
 * @brief ステージ全体とゲームオブジェクトを統合管理するマネージャークラス
 * 
 * ゲーム内のステージ全体の初期化、更新、描画を一元管理します。
 * JSONファイルからステージデータを読み込み、プレイヤー、敵、障害物、
 * ステージ構造（エリア・ウェーブ）などを生成・管理します。
 * 
 * 責任範囲:
 * - ステージデータのJSONファイル読み込み
 * - ゲームオブジェクト（プレイヤー、敵、障害物）の生成と管理
 * - Stageクラスによるエリア・ウェーブ進行の制御
 * - デバッグUI（ImGui）による編集・制御機能
 * 
 * 管理するオブジェクト:
 * - Player: プレイヤー（1体）
 * - EnemyManager: 敵の管理
 * - ObstacleManager: 障害物の管理
 * - Stage: エリア・ウェーブの進行制御
 * 
 * 使用例:
 * @code
 * // ステージマネージャーの初期化
 * StageManager stageManager;
 * stageManager.Initialize(objCommon, lightManager, cameraManager);
 * 
 * // ステージをロード
 * stageManager.LoadStage("field");
 * 
 * // ゲームループ
 * while (gameRunning) {
 *     stageManager.Update();
 *     stageManager.UpdateTransforms(cameraManager);
 *     stageManager.Draw();
 * }
 * @endcode
 */
class StageManager
{
public:
	/**
	 * @brief コンストラクタ
	 */
	StageManager();

	/**
	 * @brief デストラクタ
	 * 
	 * 管理している全てのゲームオブジェクトとデータを解放します。
	 */
	~StageManager();

	/**
	 * @brief ステージマネージャーの初期化
	 * 
	 * 各マネージャー（敵、障害物）の初期化と、
	 * JSON編集システムへのデータ登録を行います。
	 * 
	 * @param camera カメラマネージャーへのポインタ
	 * @param shadowMapManager シャドウマップマネージャーへのポインタ
	 * @param postProcessManager ポストプロセスマネージャーへのポインタ（グレースケール用）
	 */
	void Initialize(Registry* registry, SystemManager* systemManager, Object3dCommon* object3dCommon, SpriteCommon* spriteCommon, LightManager* lightManager, CameraManager* camera, ShadowMapManager* shadowMapManager, PostProcessManager* postProcessManager = nullptr);

	/**
	 * @brief ステージマネージャーの更新処理
	 * 
	 * プレイヤー、敵、障害物、ステージの更新を行います。
	 * デバッグモードでは、障害物データの同期も行います。
	 * 毎フレーム呼び出す必要があります。
	 */
	void Update();

	/**
	 * @brief トランスフォーム（行列）の更新
	 * 
	 * 全てのゲームオブジェクトの行列計算を行います。
	 * Update()の後、Draw()の前に呼び出す必要があります。
	 * 
	 * @param camera カメラマネージャーへのポインタ（ビュー行列の計算に使用）
	 */
	void UpdateTransforms(CameraManager* camera);

	/**
	 * @brief ステージの描画
	 * 
	 * プレイヤー、敵、障害物の3D描画を行います。
	 */
	void Draw3D();

	/**
	 * @brief シャドウ描画
	 * 
	 * プレイヤー、敵のシャドウマップへの描画を行います。
	 */
	void DrawShadow();

	/**
	 * @brief 2D描画
	 * 
	 * プレイヤー、敵の2D描画を行います。
	 */
	void Draw2D();

	/**
	 * @brief ImGuiによるデバッグUI表示
	 * 
	 * ステージ管理用のデバッグUIを表示します。
	 * デバッグビルド時のみ有効です（USE_IMGUIマクロで制御）。
	 * 
	 * 機能:
	 * - 敵の全クリア
	 * - 障害物の全クリア
	 * - ステージのロード
	 */
	void DrawImGui();

	/**
	 * @brief ステージデータをJSONファイルから読み込む
	 * 
	 * 指定されたステージ名のディレクトリから、ステージデータと
	 * エリア・ウェーブデータを読み込み、ゲームオブジェクトを生成します。
	 * 
	 * 読み込まれるファイル:
	 * - stage/{stageName}.json: ステージ上の固定オブジェクト配置
	 * - stage/{stageName}_area.json: エリアとウェーブの定義
	 * 
	 * @param stageName ステージ名（ディレクトリ名）
	 */
	void LoadStage(const std::string& stageName);

	/**
	 * @brief ステージデータをもとに各ゲームオブジェクトの情報を分類・生成
	 * 
	 * StageDataから読み込んだGameObjectInfoリストを、
	 * タイプごとに分類して対応するマネージャーに渡します。
	 * 
	 * 処理内容:
	 * - "PlayerSpawn": プレイヤーの生成
	 * - "Obstacle", "BarrierBlock": 障害物の生成
	 * - その他のタイプは必要に応じて追加可能
	 */
	void CreateInfosFromStageData();

	/**
	 * @brief ステージがクリアされているかを取得
	 * @return true: ステージクリア済み, false: 未クリア
	 */
	bool IsStageCleared() const { return stage_ ? stage_->IsCleared() : false; }

	/**
	 * @brief プレイヤーのエンティティIDを取得
	 * @return エンティティID（kInvalidEntityなら未生成）
	 */
	EntityID GetPlayerEntity() const { return playerEntity_; }

	/**
	 * @brief プレイヤーの位置座標へのポインタを取得（カメラ注視用）
	 * @return 位置ポインタ
	 */
	const Vector3* GetPlayerPositionPtr() const { return &lastPlayerPos_; }

	/**
	 * @brief プレイヤーの位置を取得
	 * @return 位置
	 */
	Vector3 GetPlayerPosition() const { return lastPlayerPos_; }

	/**
	 * @brief プレイヤーの回転を取得
	 * @return 回転
	 */
	Vector3 GetPlayerRotation() const { return lastPlayerRot_; }

	/**
	 * @brief プレイヤーが生存しているかを取得
	 * @return true: 生存, false: 死亡
	 */
	bool IsPlayerAlive() const;

	/**
	 * @brief プレイヤーオブジェクトを取得 (移行用：ECS化後は廃止予定)
	 * @return プレイヤーへのポインタ（常にnullptr）
	 */
	Player* GetPlayer() const { return nullptr; }

	/**
	 * @brief 敵マネージャーを取得
	 * @return EnemyManagerへのポインタ
	 */
	EnemyManager* GetEnemyManager() const { return enemyManager_.get(); }

	/**
	 * @brief 障害物マネージャーを取得
	 * @return ObstacleManagerへのポインタ
	 */
	ObstacleManager* GetObstacleManager() const { return obstacleManager_.get(); }

	/**
	 * @brief ステージオブジェクトを取得
	 * @return Stageへのポインタ（未生成の場合はnullptr）
	 */
	Stage* GetStage() const { return stage_.get(); }

private:
	Object3dCommon* object3dCommon_;  ///< 3Dオブジェクトの共通データへのポインタ
	LightManager* lightManager_;      ///< ライトマネージャーへのポインタ
	CameraManager* cameraManager_;    ///< カメラマネージャーへのポインタ
	SpriteCommon* spriteCommon_;    ///< 2Dスプライトの共通データへのポインタ
	ShadowMapManager* shadowMapManager_; ///< シャドウマップマネージャーへのポインタ
	PostProcessManager* postProcessManager_; ///< ポストプロセスマネージャーへのポインタ

	std::shared_ptr<StageData> stageData_;      ///< ステージデータ（固定オブジェクト配置）
	std::shared_ptr<ObstacleData> obstacleData_; ///< 障害物データ

	// -------- ECS -------- //
	Registry* registry_ = nullptr;
	SystemManager* systemManager_ = nullptr;
	EntityID playerEntity_ = kInvalidEntity;

	Vector3 lastPlayerPos_ = { 0, 0, 0 }; ///< 外部参照用の安定した位置
	Vector3 lastPlayerRot_ = { 0, 0, 0 }; ///< 外部参照用の安定した回転

	// -------- ゲームオブジェクト -------- //
	std::unordered_map<std::string, std::unique_ptr<InstancedModelRenderer>> ecsRenderers_;

	std::unique_ptr<EnemyManager> enemyManager_;   ///< 敵マネージャー
	std::unique_ptr<ObstacleManager> obstacleManager_; ///< 障害物マネージャー
	std::unique_ptr<Stage> stage_;                 ///< ステージ（エリア・ウェーブ管理）
};

