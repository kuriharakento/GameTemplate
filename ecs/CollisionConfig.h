#pragma once
#include <cstdint>

/**
 * @brief 衝突判定レイヤー。
 */
namespace CollisionLayer
{
    static constexpr uint32_t kNone         = 0;
    static constexpr uint32_t kPlayer       = 1 << 0;
    static constexpr uint32_t kEnemy        = 1 << 1;
    static constexpr uint32_t kObstacle     = 1 << 2;
    static constexpr uint32_t kPlayerBullet = 1 << 3;
    static constexpr uint32_t kEnemyBullet  = 1 << 4;
    static constexpr uint32_t kDecoy        = 1 << 5;
    
    static constexpr uint32_t kAll = 0xFFFFFFFF;
}
