#pragma once
#include "engine/ecs/Entity.h"
#include "engine/ecs/system/ISystem.h"
#include <cstdint>

/**
 * @brief 連鎖爆発（アナイアレイション）を管理する。
 */
class AnnihilationSystem : public ISystem
{
public:
    void Update(Registry& registry) override;

private:
    /**
     * @brief 爆発の実行。
     */
    void TriggerExplosion(EntityID sourceEntity, Registry& registry);
};

