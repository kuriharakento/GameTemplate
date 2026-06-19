#pragma once
#include "engine/ecs/system/ISystem.h"

/**
 * @brief スプリンクラーの自動起爆を管理する。
 */
class SprinklerSystem : public ISystem
{
public:
    void Update(Registry& registry) override;
};
