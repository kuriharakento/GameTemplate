#pragma once
#include "engine/ecs/system/ISystem.h"
#include "engine/ecs/Entity.h"

/**
 * @brief 弾丸（Projectile）の移動、寿命、演出を管理する。
 */
class ProjectileSystem : public ISystem
{
public:
    void Update(Registry& registry) override;
};
