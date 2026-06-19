#pragma once
#include "engine/ecs/system/ISystem.h"
#include "math/Vector3.h"

class Object3dCommon;
class CameraManager;
class LightManager;
class ShadowMapManager;

/**
 * @brief エネミーのスポーンを管理する。
 */
class EnemySpawnSystem : public ISystem
{
public:
    void Initialize(
        Object3dCommon* object3dCommon,
        LightManager* lightManager,
        CameraManager* cameraManager);

    void Update(Registry& registry) override;

    void Draw(Registry& registry, Camera* camera, LightManager* lightManager, ShadowMapManager* shadowMapManager) override;

    // 半径設定
    void SetSpawnRadius(float inner, float outer) { innerRadius_ = inner; outerRadius_ = outer; }
    void SetInitialSpawnRate(float rate) { spawnRate_ = rate; }
    
    // アクセサ (ImGui用)
    float GetInnerRadius() const { return innerRadius_; }
    void SetInnerRadius(float r) { innerRadius_ = r; }
    float GetOuterRadius() const { return outerRadius_; }
    void SetOuterRadius(float r) { outerRadius_ = r; }

private:
    /**
     * @brief 敵を1体スポーンさせる。
     */
    void SpawnEnemy(Registry& registry);

    /**
     * @brief レベルに応じた最大同時出現数を取得。
     */
    uint32_t GetMaxEnemies(uint32_t level);

    Object3dCommon* object3dCommon_ = nullptr;
    LightManager* lightManager_ = nullptr;
    CameraManager* cameraManager_ = nullptr;

    float tickTimer_ = 0.0f;
    float burstTimer_ = 0.0f;

    // 設定定数
    static constexpr uint32_t kMaxEnemiesLv1 = 6;
    static constexpr uint32_t kMaxEnemiesLv2 = 20;
    static constexpr uint32_t kMaxEnemiesLv3 = 40;
    static constexpr uint32_t kMaxEnemiesLv4 = 60;
    static constexpr uint32_t kMaxEnemiesLv5 = 250;

    float spawnRate_ = 1.0f; 
    float innerRadius_ = 60.0f;
    float outerRadius_ = 80.0f;
    float elapsedTime_ = 0.0f;
};
