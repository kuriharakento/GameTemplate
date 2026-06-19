#pragma once
#include "engine/ecs/system/ISystem.h"
#include "application/ecs/components/TurretComponent.h"

class SystemManager;

/**
 * @brief タレットの自動攻撃を管理する。
 */
class TurretSystem : public ISystem
{
public:
    void Update(Registry& registry) override;

    void SetSystemManager(SystemManager* systemManager) { systemManager_ = systemManager; }

private:
    /**
     * @brief レーザービームの更新。
     */
    void UpdateLaserBeam(EntityID turretEntity, ecs::TurretComponent& turret, Registry& registry, float dt);

    SystemManager* systemManager_ = nullptr;
};
