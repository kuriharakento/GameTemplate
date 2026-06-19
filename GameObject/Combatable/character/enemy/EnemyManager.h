#pragma once
#include "application/stage/StageData.h"
#include "application/gameObject/combatable/weapon/Bullet.h"
#include "math/AABB.h"
#include <vector>
#include <memory>
#include <unordered_map>
#include "engine/ecs/Registry.h"
#include "engine/ecs/Entity.h"
#include "engine/graphics/3d/InstancedModelRenderer.h"
#include "engine/ecs/system/SystemManager.h"

class ShadowMapManager;
class Object3dCommon;
class SpriteCommon;
class CameraManager;
class LightManager;
class GameObject;

class EnemyManager
{
public:
	// ECS Integration: Initialize時に外部（GamePlayScene）で構築されたRegistryを受け取る
	void Initialize(Registry* registry, SystemManager* systemManager, Object3dCommon* object3dCommon, SpriteCommon* spriteCommon, CameraManager* camera, LightManager* lightManager, ShadowMapManager* shadowMapManager, GameObject* target);
	~EnemyManager();
	void Update();
	void DrawImGui();
	void DrawStandard3D(CameraManager* camera);

	// ECSのエンティティ生成専用ファクトリメソッド群
	void AddPistolEnemy(uint32_t count);
	void AddAssaultEnemy(uint32_t count);
	void AddShotgunEnemy(uint32_t count);
	void AddKnifeEnemy(uint32_t count);
	void SetEnemyData(const std::vector<GameObjectInfo>& data);
	void AddEnemiesFromGameObjectInfo(const std::vector<GameObjectInfo>& data);
	
	void SetTarget(GameObject* target) { target_ = target; }
	void SetCameraManager(CameraManager* camera) { camera_ = camera; }
	void SetOnAllEnemiesDefeatedCallback(std::function<void()> callback) { onAllEnemiesDefeatedCallback_ = std::move(callback); }
	void Clear();

	// 敵が発射した弾を管理する（弾だけはGameObjectのまま）
	void AddEnemyBullet(std::unique_ptr<Bullet> bullet) { enemyBullets_.push_back(std::move(bullet)); }

	// 情報取得用
	Object3dCommon* GetObject3dCommon() const { return object3dCommon_; }
	LightManager* GetLightManager() const { return lightManager_; }
	Registry* GetRegistry() const { return registry_; }

private:
	void CreateAssaultEnemyFromData();
	void AssignInstancedRenderComponent(EntityID entity, const std::string& modelName);
	void AddCollisionComponents(EntityID entity);

	std::function<void()> onAllEnemiesDefeatedCallback_ = nullptr;

	Object3dCommon* object3dCommon_ = nullptr;
	SpriteCommon* spriteCommon_ = nullptr;
	LightManager* lightManager_ = nullptr;
	GameObject* target_ = nullptr; 
	CameraManager* camera_ = nullptr;
	ShadowMapManager* shadowMapManager_ = nullptr;
	AABB emitRange_ = {};
	
	// ECS連携ポインタ（所有権はGamePlaySceneなど外部にある）
	Registry* registry_ = nullptr;
	SystemManager* systemManager_ = nullptr;
	
	
	// EnemyBase等GameObjectリストは完全撤廃し、弾のリストのみを管理する
	std::vector<std::unique_ptr<Bullet>> enemyBullets_;
	std::vector<GameObjectInfo> enemyData_;
};
