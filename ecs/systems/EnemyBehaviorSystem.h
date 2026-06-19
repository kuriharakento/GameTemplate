#pragma once
#include "engine/ecs/Registry.h"
#include "engine/ecs/system/ISystem.h"
#include "application/ecs/components/EnemyTypeComponent.h"
#include "math/Vector3.h"

/**
 * @brief 敵の振る舞いを更新する。
 */
class EnemyBehaviorSystem : public ISystem
{
public:
    void Update(Registry& registry) override;

private:
    /**
     * @brief 近接型の更新。
     */
    void UpdateMeleeBehavior(EntityID entity, Registry& registry, const Vector3& targetPos, float dt);
    
    /**
     * @brief 遠距離型の更新。
     */
    void UpdateRangedBehavior(EntityID entity, Registry& registry, const Vector3& targetPos, float dt);

    /**
     * @brief 突進型の更新。
     */
    void UpdateChargerBehavior(EntityID entity, Registry& registry, const Vector3& targetPos, float dt);
};
