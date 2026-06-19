#pragma once
#include <cstdint>

namespace ecs
{
    /**
     * @brief 敵に付与されるダメージボーナススタック。
     */
    struct DamageStackComponent
    {
        int count_ = 0;

        static constexpr float kDamagePerStack = 5.0f;
        static constexpr int kMaxStacks = 10;
    };
}
