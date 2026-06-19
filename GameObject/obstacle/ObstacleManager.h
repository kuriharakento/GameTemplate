#pragma once
#include <vector>
#include <memory>

#include "Obstacle.h"
#include "ObstacleData.h"
#include "application/stage/StageData.h"
#include "engine/ecs/Registry.h"
#include "engine/ecs/components/TransformComponent.h"
#include "engine/ecs/components/InstancedRenderComponent.h"
#include "application/ecs/components/ObstacleComponent.h"

class CameraManager;
class LightManager;
class Object3dCommon;

class ObstacleManager
{
public:
	// 初期化
	void Initialize(Registry* registry, Object3dCommon* object3dCommon, LightManager* lightManager);
	~ObstacleManager();
	// 更新
	// ロジック更新
	void Update();
	void DrawImGui();
	// 行列更新
	void UpdateTransforms(CameraManager* camera);
	// 描画
	void Draw(CameraManager* camera);
	// シャドウ描画
	void DrawShadow();

	void Clear();
	void CreateObstacles();
	void ApplyObstacleData();
	void LoadObstacleData(const std::string& path);
	void SetCulling(bool culling) { culling_ = culling; }
	void SetObstacleData(ObstacleData* data);
	const std::vector<std::unique_ptr<Obstacle>>& GetObstacles() const { return obstacles_; }

	// ECS: 障害物のRegistry
	const Registry* GetRegistry() const { return registry_; }
	uint32_t GetObstacleCount() const { return registry_ ? registry_->GetActiveEntityCount() : 0; }

private:
	void CreateObstacle(const GameObjectInfo& info);
	void CreateBarrierBlock(const GameObjectInfo& info);
	void CreateFloor(const GameObjectInfo& info);
	void SyncNewObstacleData();

	// EntityをRegistryに登録し、ECSコンポーネントを付与する
	void RegisterToRegistry(const GameObjectInfo& info, ecs::ObstacleComponent::Type type, bool hasCollider);

private:
	Object3dCommon* object3dCommon_ = nullptr;
	LightManager* lightManager_ = nullptr;
	ObstacleData* obstacleData_ = nullptr;

	// 障害物オブジェクト（実体を保持）
	std::vector<std::unique_ptr<Obstacle>> obstacles_;

	// ECS Registry: Transform/Render/ObstacleComponentを管理
	Registry* registry_ = nullptr;

	bool culling_ = false;
};
