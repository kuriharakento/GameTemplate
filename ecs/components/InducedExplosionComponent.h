#pragma once

namespace ecs
{
/**
 * @brief 敵に付与される誘爆スタックを管理する。
 */
struct InducedExplosionComponent
{
    int count_ = 0;
    
    static constexpr int kMaxCount = 3;
    static constexpr float kExplosionDamage = 500.0f;
    static constexpr float kExplosionRadius = 12.0f;
};
}
