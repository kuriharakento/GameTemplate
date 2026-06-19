#pragma once
#include "engine/ecs/system/ISystem.h"
#include "engine/ecs/Entity.h"

class CameraManager;
class SystemManager;

/**
 * @brief プレイヤーの入力に基づき、移動やスキルの発動を制御する。
 */
class PlayerActionSystem : public ISystem
{
public:
    void Update(Registry& registry) override;

    void SetCameraManager(CameraManager* cameraManager) { cameraManager_ = cameraManager; }
    void SetSystemManager(SystemManager* systemManager) { systemManager_ = systemManager; }
    void SetObject3dCommon(class Object3dCommon* common) { object3dCommon_ = common; }

private:
    /**
     * @brief 回避（Dodge）の更新。
     */
    void UpdateDodge(EntityID entity, Registry& registry, float deltaTime);

    /**
     * @brief 通常移動の更新。
     */
    void UpdateMovement(EntityID entity, Registry& registry, float deltaTime);

    /**
     * @brief スキルの更新。
     */
    void UpdateSkills(EntityID entity, Registry& registry, float deltaTime);

    /**
     * @brief 各スキルの個別処理。
     */
    void UpdateLMB(EntityID entity, Registry& registry, float deltaTime);
    void UpdateSkill1(EntityID entity, Registry& registry, float deltaTime);
    void UpdateSkill2(EntityID entity, Registry& registry, float deltaTime);
    void UpdateR(EntityID entity, Registry& registry, float deltaTime);

    /**
     * @brief スキル1の派生処理。
     */
    void FireBombWave(EntityID entity, Registry& registry);
    void FireHomingMissile(EntityID entity, Registry& registry);

    /**
     * @brief スキル2の派生処理。
     */
    void SpawnDecoy(EntityID entity, Registry& registry);
    void SpawnTurret(EntityID entity, Registry& registry);

    /**
     * @brief 誘爆を発生させる。
     */
    void SpawnExplosion(EntityID sourceEntity, Registry& registry);

    CameraManager* cameraManager_ = nullptr;
    SystemManager* systemManager_ = nullptr;
    class Object3dCommon* object3dCommon_ = nullptr;
};
