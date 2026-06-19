#pragma once
#include "engine/ecs/system/ISystem.h"

/**
 * @brief デコイのロジックを管理する。
 */
class DecoySystem : public ISystem
{
public:
    void Update(Registry& registry) override;
};
