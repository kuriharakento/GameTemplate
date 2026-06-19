#include "StageManager.h"
#include "engine/ecs/components/TransformComponent.h"
#include "engine/ecs/components/MovementComponent.h"
#include "engine/ecs/components/ColliderComponent.h"
#include "application/ecs/CollisionConfig.h"
#include "engine/ecs/Registry.h"
#include "engine/ecs/Entity.h"
#include "engine/ecs/system/SystemManager.h"
#include "application/ecs/systems/EnemySpawnSystem.h"
#include "externals/imgui/imgui.h"
#include <memory>
#include "engine/ecs/components/InstancedRenderComponent.h"
#include "engine/ecs/components/TagComponent.h"
#include "application/ecs/components/PlayerComponent.h"
#include "application/ecs/components/StatusComponent.h"
#include "engine/ecs/components/CollisionResponseComponent.h"
#include "manager/editor/JsonEditor.h"
#include "engine/ecs/system/InstancedRenderSystem.h"
#include "engine/graphics/3d/InstancedModelRenderer.h"
#include "manager/graphics/ModelManager.h"
#ifdef USE_IMGUI
#include "manager/editor/DebugUIManager.h"
#endif

using namespace ecs;

StageManager::StageManager()
{
}

StageManager::~StageManager()
{
#ifdef USE_IMGUI
	if (DebugUIManager::HasInstance()) {
		DebugUIManager::GetInstance()->UnregisterDebugUI(this);
	}
#endif
	// 各ゲームオブジェクトを明示的に解放
	stageData_.reset();
	enemyManager_.reset();
	obstacleManager_.reset();
	stage_.reset();
}

#include "engine/manager/effect/PostProcessManager.h"

void StageManager::Initialize(Registry* registry, SystemManager* systemManager, Object3dCommon* object3dCommon, SpriteCommon* spriteCommon, LightManager* lightManager, CameraManager* camera, ShadowMapManager* shadowMapManager, PostProcessManager* postProcessManager)
{
	object3dCommon_ = object3dCommon;
	spriteCommon_ = spriteCommon;
	lightManager_ = lightManager;
	cameraManager_ = camera;
	shadowMapManager_ = shadowMapManager;
	postProcessManager_ = postProcessManager;
	registry_ = registry;
	systemManager_ = systemManager;

	// ステージデータの初期化
	stageData_ = std::make_unique<StageData>();

	// 障害物データの初期化
	obstacleData_ = std::make_shared<ObstacleData>();

	// デバッグエディターに登録（実行時編集を可能にする）
	JsonEditor::GetInstance()->Register("stageData", stageData_);
	JsonEditor::GetInstance()->Register("obstacleData", obstacleData_);

	// --- 各マネージャーの初期化 --- //

	enemyManager_ = std::make_unique<EnemyManager>();
	enemyManager_->Initialize(registry, systemManager, object3dCommon_, spriteCommon, camera, lightManager, shadowMapManager, nullptr); // ターゲットは後で設定
	enemyManager_->SetCameraManager(cameraManager_);

	// 障害物マネージャーの初期化
	obstacleManager_ = std::make_unique<ObstacleManager>();
	obstacleManager_->Initialize(registry_, object3dCommon_, lightManager_);

	// --- ECS レンダラーの初期化 --- //
	
	// 主要なモデルのレンダラーを事前生成
	// [BNS-Fix] ステージで使用されるモデル（"street", "knife" 等）を追加
	const std::vector<std::string> modelNames = { "player", "enemy", "wall", "cube", "street", "knife" };
	for (const auto& name : modelNames)
	{
		Model* model = ModelManager::GetInstance()->FindModel(name);
		if (model)
		{
			auto renderer = std::make_unique<InstancedModelRenderer>(10000);
			renderer->Initialize(object3dCommon_->GetDXCommon(), object3dCommon_->GetSrvManager(), model);
			ecsRenderers_[name] = std::move(renderer);
		}
	}

#ifdef USE_IMGUI
	DebugUIManager::GetInstance()->RegisterDebugUI(this, "Stage Manager", [this]() { this->DrawImGui(); }, DebugUIArea::Inspector);
#endif
}

void StageManager::Update()
{

	// プレイヤーデータの同期（カメラ用など）
	if (playerEntity_ == kInvalidEntity) return; // Changed return true to return; as Update is void.
	if (registry_ && registry_->HasComponent<ecs::StatusComponent>(playerEntity_)) // Changed condition to check for StatusComponent and registry
	{
		if (registry_->HasComponent<TransformComponent>(playerEntity_))
		{
			auto& transform = registry_->GetComponent<TransformComponent>(playerEntity_);
			lastPlayerPos_ = transform.localPosition_;
			lastPlayerRot_ = transform.localRotation_;
		}
	}

	// プレイヤーの更新（ECS SystemManagerで一括更新されるため、ここでは何もしない）

	// 敵マネージャーの更新
	if (enemyManager_)
	{
		enemyManager_->Update();
	}
	
	// 障害物の更新
	if (obstacleManager_)
	{
		obstacleManager_->Update();
	}

	// ステージの更新（エリア・ウェーブ進行）
	stage_->Update(cameraManager_);

#ifdef _DEBUG
	// デバッグモード：ステージデータと障害物データを同期
	// JSONエディターでの編集内容を即座に反映するため
	std::vector< GameObjectInfo> obstacleInfos;
	for (const auto& data : stageData_->gameObjects)
	{
		if (data.type == "Obstacle" || data.type == "BarrierBlock" || data.type == "Floor")
		{
			obstacleInfos.push_back(data);
		}
	}
	obstacleData_->SetObstacles(obstacleInfos);
#endif
}

void StageManager::UpdateTransforms(CameraManager* camera)
{
	// プレイヤーの行列更新（ECS SystemManagerで一括更新されるため、ここでは何もしない）



	// 障害物マネージャーの行列更新
	if (obstacleManager_)
	{
		obstacleManager_->UpdateTransforms(cameraManager_);
	}
}

void StageManager::Draw3D()
{
	// ECS インスタンス描画
	if (registry_)
	{
		InstancedRenderSystem::DrawGrouped(*registry_, ecsRenderers_, cameraManager_->GetActiveCamera(), lightManager_, shadowMapManager_);
	}

	// [Fix] ECS インスタンス描画でルートシグネチャが上書きされるため、通常描画用に設定を戻す
	if (object3dCommon_)
	{
		object3dCommon_->CommonRenderingSetting();
	}

	// 敵マネージャーの通常描画（非ECS：弾など）
	if (enemyManager_)
	{
		enemyManager_->DrawStandard3D(cameraManager_);
	}

	// 障害物の描画は ECS 側 (InstancedRenderSystem) で一括して行われるため、
	// 従来の GameObject 版 Draw は不要（呼び出すとルートシグネチャの不一致でクラッシュする）
	/*
	if (obstacleManager_)
	{
		obstacleManager_->Draw(cameraManager_);
	}
	*/
}

void StageManager::DrawShadow()
{
	// プレイヤーのシャドウ描画（TODO: ECSでのシャドウ描画対応）



	// 障害物のシャドウ描画
	if (obstacleManager_)
	{
		obstacleManager_->DrawShadow();
	}
}

void StageManager::Draw2D()
{
	// プレイヤーの2D描画（TODO: ECSでのUI表示対応）

}

void StageManager::DrawImGui()
{
#ifdef USE_IMGUI
	// 敵を全クリアするボタン（デバッグ用）
	if(ImGui::Button("Clear Enemies"))
	{
		if (enemyManager_)
		{
			enemyManager_->Clear();
		}
	}

	// 障害物を全クリアするボタン（デバッグ用）
	if (ImGui::Button("Clear Obstacles"))
	{
		if (obstacleManager_)
		{
			obstacleManager_->Clear();
		}
	}

	// プレイヤー情報の表示 (現在は GamePlayScene::DrawImGui で一括表示)
#endif
}

void StageManager::LoadStage(const std::string& stageName)
{
	// ステージファイルのパスを構築
	std::string dirpath = "stage/" + stageName;
	std::string json = ".json";
	std::string areaJson = "_area" + json;

	// 既存のゲームオブジェクトをクリア
	enemyManager_->Clear();       // 敵を全削除
	obstacleManager_->Clear();    // 障害物と床を全削除

	// ステージデータ（固定オブジェクト配置）をロード
	stageData_->LoadJson(dirpath + json);

	// ステージデータから各ゲームオブジェクトを生成
	CreateInfosFromStageData();

	// ステージ（エリア・ウェーブ管理）を初期化
	stage_ = std::make_unique<Stage>(
		object3dCommon_,
		lightManager_,
		enemyManager_.get(),
		dirpath + areaJson  // エリア・ウェーブ定義ファイル
	);

	// ステージを開始
	stage_->Start();
}

void StageManager::CreateInfosFromStageData()
{
	// 障害物情報を格納するリスト
	std::vector<GameObjectInfo> obstacleInfos;

	// ステージデータの各オブジェクトをタイプごとに分類
#ifdef _DEBUG
	OutputDebugStringA(("stageData_ gameObjects count: " + std::to_string(stageData_->gameObjects.size()) + "\n").c_str());
	for (const auto& obj : stageData_->gameObjects)
	{
		OutputDebugStringA(("  type=" + obj.type + " name=" + obj.name + "\n").c_str());
	}
#endif
	for(const auto& objInfo : stageData_->gameObjects)
	{
		// 無効化されているオブジェクトはスキップ
		if (objInfo.disabled) continue;

		// タイプに応じて処理を分岐
		if (objInfo.type == "PlayerSpawn")
		{
			// プレイヤーのスポーン処理
			if (playerEntity_ == kInvalidEntity && registry_)
			{
				playerEntity_ = registry_->CreateEntity();
				
				// コンポーネント付与
				ecs::TagComponent playerTag;
				playerTag.type = ecs::TagComponent::Type::Player;
				registry_->AddComponent<ecs::TagComponent>(playerEntity_, playerTag);
				registry_->AddComponent<TransformComponent>(playerEntity_, { objInfo.transform.translate, objInfo.transform.rotate, objInfo.transform.scale });
				MovementComponent movement;
				movement.useGravity_ = true;
				registry_->AddComponent<MovementComponent>(playerEntity_, movement);
				registry_->AddComponent<PlayerComponent>(playerEntity_, {});
				registry_->AddComponent<ecs::StatusComponent>(playerEntity_, ecs::StatusComponent{});
				registry_->AddComponent<CollisionResponseComponent>(playerEntity_, {});

				// コライダー設定 (OBB)
				ecs::ColliderComponent col;
				col.type_ = ColliderType::OBB;
				col.useSubstep_ = true;
				// [Fix] 初期位置を同期（原点への押し戻しを防ぐ）
				col.previousPosition_ = objInfo.transform.translate;

				// フィルタリング設定
				col.layer = CollisionLayer::kPlayer;
				col.mask = CollisionLayer::kEnemy | CollisionLayer::kObstacle | CollisionLayer::kEnemyBullet;

				registry_->AddComponent<ecs::ColliderComponent>(playerEntity_, col);

				// 描画設定
				InstancedRenderComponent irc;
				irc.modelName_ = "player";
				registry_->AddComponent<InstancedRenderComponent>(playerEntity_, irc);

				// 敵マネージャーのターゲット設定（EntityIDへの対応は追って検討）
				// 現状は GameObject 依存が残っている可能性があるため null 設定
				enemyManager_->SetTarget(nullptr);
			}
		}
		else if (objInfo.type == "EnemySpawn")
		{
			// 敵のスポーン処理
			// 注: 現在は未使用（敵はウェーブ管理システムで生成される）
		}
		else if (objInfo.type == "Obstacle" || objInfo.type == "BarrierBlock" || objInfo.type == "Floor")
		{
			// 障害物・床の情報を収集（Floorはコライダーなしで生成される）
			obstacleInfos.push_back(objInfo);
		}
	}

	// 障害物データを設定
	obstacleData_->SetObstacles(obstacleInfos);
	obstacleManager_->SetObstacleData(obstacleData_.get());
}

bool StageManager::IsPlayerAlive() const
{
	if (playerEntity_ == kInvalidEntity || !registry_)
	{
		return false;
	}

	if (!registry_->HasComponent<ecs::StatusComponent>(playerEntity_))
	{
		return false;
	}

	const auto& status = registry_->GetComponent<ecs::StatusComponent>(playerEntity_);
	return status.isAlive_;
}
