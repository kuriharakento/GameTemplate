#pragma once
#include <cstdint>

namespace ecs
{
    /**
     * @brief 障害物固有の属性を保持する。
     */
    struct ObstacleComponent
    {
        /**
         * @brief 障害物の種別。
         */
        enum class Type : uint32_t
        {
            Obstacle     = 0,
            BarrierBlock = 1,
            Floor        = 2,
        };

        Type type = Type::Obstacle;
        bool hasCollider = true;
    };
}
