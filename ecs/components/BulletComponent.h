#pragma once
#include <cstdint>
#include "engine/ecs/Entity.h"

namespace ecs
{
    /**
     * @brief 弾丸の属性を管理する。
     */
    struct BulletComponent
    {
        // 弾丸の種別
        enum class Type
        {
            Player,
            Enemy
        };

        float damage_ = 10.0f;
        bool isAlive_ = true;
        EntityID owner_ = kInvalidEntity;
        Type type_ = Type::Player;
    };
}
