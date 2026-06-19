#pragma once
#include <cstdint>
#include "engine/ecs/Entity.h"

namespace ecs
{
    /**
     * @brief スプリンクラーの状態を管理する。
     */
    struct SprinklerComponent
    {
        // 所有プレイヤー。所有しない
        EntityID owner_ = kInvalidEntity;

        float range_ = 15.0f;
        float checkInterval_ = 0.5f;
        float checkTimer_ = 0.0f;
        float lifetime_ = 8.0f;
    };
}
