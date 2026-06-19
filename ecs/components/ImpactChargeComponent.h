#pragma once
#include <cstdint>

namespace ecs
{
    /**
     * @brief 敵に付与されたインパクト・チャージ（爆弾）の状態を管理する。
     */
    struct ImpactChargeComponent
    {
        uint8_t stackCount_ = 0;
        static constexpr uint8_t kMaxStacks = 4;

        float explosionRadius_ = 5.0f;
        float explosionDamageBase_ = 50.0f;
        bool isPrimed_ = false;
    };
}
