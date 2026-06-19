#pragma once
#include <cstdint>

namespace ecs
{
    /**
     * @brief 敵の種類。
     */
    enum class EnemyType : uint32_t
    {
        Melee = 0,
        Charger = 1,
    };

    /**
     * @brief 敵の種別を識別する。
     */
    struct EnemyTypeComponent
    {
        EnemyType type = EnemyType::Melee;
    };
}
